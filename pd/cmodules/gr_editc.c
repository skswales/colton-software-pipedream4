/* gr_editc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Chart editing module */

/* SKS July 1991 */

/* SKS after 4.12 26mar92 - must NOT put flex anchors in listed data */

#include "common/gflags.h"

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef __os_h
#include "os.h"
#endif

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_flex_h
#include "cs-flex.h"    /* includes flex.h */
#endif

#ifndef __cs_template_h
#include "cs-template.h" /* includes template.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_xferrecv_h
#include "cs-xferrecv.h"
#endif

#ifndef __colourtran_h
#include "colourtran.h"
#endif

#ifndef __drawmod_h
#include "drawmod.h"
#endif

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

/*
internal functions
*/

static void
gr_chartedit_riscos_point_from_rel(
    P_GR_CHART cp,
    _OutRef_    P_GR_POINT point,
    GR_OSUNIT x,
    GR_OSUNIT y);

static void
gr_chartedit_selection_display(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_xor(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_riscos_setup_redrawstr(
    P_GR_CHARTEDITOR cep,
    flex_ptr pdf_handle,
    wimp_redrawstr * r /*out*/);

#if RISCOS

/*
callbacks from RISC OS
*/

static S32
gr_chartedit_riscos_event_handler(
    wimp_eventstr * e,
    P_ANY           handle);

static void
gr_chartedit_riscos_force_redraw(
    P_GR_CHARTEDITOR cep,
    flex_ptr        pdf_handle,
    S32             immediate);

static S32
gr_chartedit_riscos_unknown_event_previewer(
    wimp_eventstr * e,
    P_ANY           handle);

#endif /* RISCOS */

/*
a list of chart editors
*/

static NLISTS_BLK
gr_chart_editors =
{
    NULL,
    sizeof(GR_CHARTEDITOR),
    sizeof(GR_CHARTEDITOR) * 8
};

/******************************************************************************
*
* use external handle to dial up reference to underlying chart editor structure
*
******************************************************************************/

extern P_GR_CHARTEDITOR
gr_chartedit_cep_from_ceh(
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep = NULL;

    if(ceh)
    {
        cep = collect_goto_item(GR_CHARTEDITOR, &gr_chart_editors.lbr, (LIST_ITEMNO) ceh);

        myassert1x(cep != NULL, "gr_chartedit_cep_from_ceh: failed to find chart editor handle &%p", ceh);
    }

    return(cep);
}

/******************************************************************************
*
* ensure diagram gets redrawn sometime
*
******************************************************************************/

extern void
gr_chartedit_diagram_update_later(
    P_GR_CHARTEDITOR cep)
{
    P_GR_CHART cp;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    if(NULL != cp->core.p_gr_diag)
        gr_chartedit_riscos_force_redraw(cep, &cp->core.p_gr_diag->gr_riscdiag.draw_diag.data, FALSE);
}

/******************************************************************************
*
* dispose of a chart editor and its baggage
*
******************************************************************************/

extern void
gr_chartedit_dispose(
    P_GR_CHARTEDIT_HANDLE  cehp /*inout*/)
{
    GR_CHARTEDIT_HANDLE ceh;
    P_GR_CHARTEDITOR     cep;
    P_GR_CHART           cp;
    P_GR_TEXT            t;
    LIST_ITEMNO         key;

    trace_2(TRACE_MODULE_GR_CHART,
            "gr_chartedit_dispose(&%p->&%p)",
            report_ptr_cast(cehp), report_ptr_cast(cehp ? *cehp : NULL));

    if(!cehp)
        return;
    ceh = *cehp;
    if(!ceh)
        return;
    *cehp = 0;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    /* loop removing any editing windows from text objects */
    assert(cep->ch);
    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* kill the link from chart to editor */
    cp->core.ceh = 0;

    for(t = collect_first(GR_TEXT, &cp->text.lbr, &key);
        t;
        t = collect_next( GR_TEXT, &cp->text.lbr, &key))
    {
        /* kill off the editing window for this object if there is one */
        if(t->bits.being_edited)
            gr_chartedit_text_editor_kill(cp, key);
    }

    /* remove (eventually) any FontSelector or other persistent dialog processes */
    if(cep->riscos.processing_fontselector)
        gr_chartedit_fontselect_kill(cep);

    /* don't delete other lists, they belong to the chart, not the editor */

    /* remove menu handlers from window */
    gr_chartedit_menu_finalise(cep);

    /* remove uk previewer from window */
    win_remove_unknown_event_processor(gr_chartedit_riscos_unknown_event_previewer, ceh);

    /* remove window and then discard window def. copy (contained all indirected data) */
    wimpt_complain(win_delete_wind(&cep->riscos.w));
    template_copy_dispose(&cep->riscos.ww);

    /* SKS after 4.12 26mar92 - deallocate core for selection fake GR_DIAG */
    gr_chartedit_selection_kill_repr(cep);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&cep->selection.p_gr_diag));

    /* reconvert ceh explicitly for subtract */
    key = (LIST_ITEMNO) ceh;

    trace_1(TRACE_MODULE_GR_CHART,
            "gr_chartedit_dispose: collect_subtract_entry %d from gr_chart_editors list", key);
    collect_subtract_entry(&gr_chart_editors.lbr, key);
}

/******************************************************************************
*
* ensure chart editor window at front
*
******************************************************************************/

extern void
gr_chartedit_front(
    PC_GR_CHARTEDIT_HANDLE cehp)
{
    P_GR_CHARTEDITOR cep;

    cep = gr_chartedit_cep_from_ceh(*cehp);

    if(cep)
        /* make window visible sometime */
        win_send_front(cep->riscos.w, FALSE);
}

/******************************************************************************
*
* create a chart editor
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chartedit_new(
    _OutRef_    P_GR_CHARTEDIT_HANDLE cehp,
    GR_CHART_HANDLE ch,
    gr_chartedit_notify_proc nproc,
    P_ANY nhandle)
{
    static LIST_ITEMNO  cepkey_gen = 0x17985000; /* NB. not tbs! */

    GR_CHARTEDIT_HANDLE ceh;
    P_GR_CHARTEDITOR    cep;
    LIST_ITEMNO         key;
    P_GR_CHART          cp;
    template *          template_h;
    STATUS              res;

    trace_4(TRACE_MODULE_GR_CHART,
            "gr_chartedit_new(&%p, &%p, (&%p, &%p))",
            report_ptr_cast(cehp), report_ptr_cast(ch), report_procedure_name(report_proc_cast(nproc)), report_ptr_cast(nhandle));

    *cehp = NULL;

    /* has this chart already got an editor? */
    cp = gr_chart_cp_from_ch(ch);
    assert(cp);

    if(cp->core.ceh)
        return(create_error(GR_CHART_ERR_ALREADY_EDITING));

    /* add to list of chart editors */
    key = cepkey_gen++;

    if(NULL == (cep = collect_add_entry_elem(GR_CHARTEDITOR, &gr_chart_editors, &key, &res)))
        return(res);

    zero_struct_ptr(cep);

    /* SKS after 4.12 26mar92 - allocate core for selection fake GR_DIAG */
    if(NULL == (cep->selection.p_gr_diag = al_ptr_calloc_elem(GR_DIAG, 1, &res)))
    {
        collect_subtract_entry(&gr_chart_editors.lbr, key);
        return(res);
    }

    /* convert ceh explicitly */
    ceh = (GR_CHARTEDIT_HANDLE) key;

    *cehp = ceh;

    trace_3(TRACE_MODULE_GR_CHART,
            "obtained core &%p with key %d, external handle &%p",
            report_ptr_cast(cep), key, report_ptr_cast(ceh));

    cep->ceh          = ceh;
    cep->ch           = ch;
    cep->notifyproc   = nproc ? nproc   : gr_chartedit_notify_default;
    cep->notifyhandle = nproc ? nhandle : NULL;

    /* create window */
    template_h = template_find_new(GR_CHARTEDIT_TEM);

    if(template_h)
    {
        cep->riscos.ww = template_copy_new(template_h);

        if(cep->riscos.ww != NULL)
        {
            /* keep window at the same posn,size,scroll for chart if previously edited */
            cep->riscos.init_scx = cep->riscos.ww->scx;
            cep->riscos.init_scy = cep->riscos.ww->scy;

            cp = gr_chart_cp_from_ch(cep->ch);
            assert(cp);

            if(cp->core.editsave.open_box.x0 == cp->core.editsave.open_box.x1)
            {
                cp->core.editsave.open_box = cep->riscos.ww->box;
                cp->core.editsave.open_scx = cep->riscos.ww->scx;
                cp->core.editsave.open_scy = cep->riscos.ww->scy;
            }

            wimpt_complain(win_create_wind(cep->riscos.ww,
                                           &cep->riscos.w,
                                           gr_chartedit_riscos_event_handler,
                                           (P_ANY) ceh));
        }
    }

#if RISCOS
    if(!cep->riscos.w)
    {
        gr_chartedit_dispose(cehp);
        return(0);
    }

    /* add uk previewer to window */
    win_add_unknown_event_processor(gr_chartedit_riscos_unknown_event_previewer, ceh);

    /* make window visible sometime */
    win_send_front(cep->riscos.w, FALSE);
#endif

    /* set up menu handling for editing window */
    gr_chartedit_menu_initialise(cep);

    /* clear out selection icon */
    gr_chartedit_selection_display(cep);

    /* and note the editor handle in the chart */
    cp = gr_chart_cp_from_ch(cep->ch);
    cp->core.ceh = ceh;

    /* set the window title */
    gr_chartedit_setwintitle(cp);

    return(1);
}

/******************************************************************************
*
* perform an upcall to the owner of the chart editor
*
******************************************************************************/

/*ncr*/
static S32
gr_chartedit_notify(
    P_GR_CHARTEDITOR cep,
    GR_CHARTEDIT_NOTIFY_TYPE ntype,
    P_ANY nextra)
{
    gr_chartedit_notify_proc nproc;
    P_ANY nhandle;
    S32 res;

    if(!cep)
    {
        nproc   = gr_chartedit_notify_default;
        nhandle = NULL;
    }
    else
    {
        nproc   = cep->notifyproc;
        nhandle = cep->notifyhandle;
    }

    res = (* nproc) (nhandle, cep ? cep->ceh : 0, ntype, nextra);

    return(res);
}

/******************************************************************************
*
* default chart editor notify upcall processor
*
* should be called in default case from external processors
*
******************************************************************************/

gr_chartedit_notify_proto(extern, gr_chartedit_notify_default, handle, ceh, ntype, nextra)
{
    P_GR_CHARTEDITOR cep = NULL;
    S32             res = 1;

    IGNOREPARM(handle);

    switch(ntype)
    {
    case GR_CHARTEDIT_NOTIFY_RESIZEREQ:
        /* always allow resize ops */
        break;

    case GR_CHARTEDIT_NOTIFY_SELECTREQ:
        /* always allow select ops */
        break;

    case GR_CHARTEDIT_NOTIFY_CLOSEREQ:
        /* naughty: owner of editor ought to want to know about this */
        /* also, mustn't have a lock here */
        assert(0);
        gr_chartedit_dispose(&ceh);
        break;

    case GR_CHARTEDIT_NOTIFY_TITLEREQ:
        {
        GR_CHARTEDIT_NOTIFY_TITLE_STR * t = nextra;
        P_GR_CHART    cp;
        #if RISCOS
        PC_U8 prog = wimpt_programname();
        #else
        PC_U8 prog = "Charts";
        #endif
        PC_U8 filename;

        cep = gr_chartedit_cep_from_ceh(ceh);
        assert(cep);

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        filename = cp->core.currentfilename;

        (void) xsnprintf(t->title, elemof32(t->title), "%s: %s", prog, filename);
        }
        break;

    default:
        myassert1x(0, "gr_chartedit_notify_default called with unknown notify type %d", ntype);
        res = 0;
        break;
    }

    return(res);
}

#if RISCOS

/******************************************************************************
*
* work out what object was clicked on in the diagram being edited
*
******************************************************************************/

extern unsigned int
gr_chartedit_riscos_correlate(
    P_GR_CHARTEDITOR  cep,
    PC_GR_POINT point,
    _OutRef_    P_GR_CHART_OBJID id,
    /*out*/ GR_DIAG_OFFSET * hitObject /*[]*/,
    size_t           hitObjectDepth,
    S32              adjustclicked)
{
    P_GR_CHART cp;
    P_GR_DIAG_OBJECT pObject;
    GR_POINT selpoint;
    GR_SIZE selsize;
    unsigned int hitIndex;
    GR_DIAG_OFFSET endObject;

    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    if(adjustclicked)
    {
        /* tighter click box if ADJUST clicked (but remember some objects may be truly subpixel ...) */
        selsize.cx = gr_pixit_from_riscos(1);
        selsize.cy = gr_pixit_from_riscos(1);
    }
    else
    {
        /* larger (but still small) click box if SELECT clicked */
        selsize.cx = gr_pixit_from_riscos(4);
        selsize.cy = selsize.cx;
    }

    selpoint = *point;

    /* scanning backwards from the front of the diagram */
    endObject = GR_DIAG_OBJECT_LAST;

    *id = gr_chart_objid_anon;

    if(NULL == cp->core.p_gr_diag)
    {
        hitObject[0] = GR_DIAG_OBJECT_NONE;
            return(0);
    }

    for(;;)
    {
        gr_diag_object_correlate_between(cp->core.p_gr_diag, &selpoint, &selsize,
                                         hitObject,
                                         hitObjectDepth - 1,
                                         GR_DIAG_OBJECT_FIRST,
                                         endObject);

        if(hitObject[0] == GR_DIAG_OBJECT_NONE)
            return(0);

        /* find deepest named hit */
        for(hitIndex = 1;
            hitIndex < hitObjectDepth;
            hitIndex++)
            if(hitObject[hitIndex] == GR_DIAG_OBJECT_NONE)
            {
                --hitIndex;
                break;
            }

        pObject.p_byte = gr_diag_getoffptr(BYTE, cp->core.p_gr_diag, hitObject[hitIndex]);

        /* read back the id field of this object */
        *id = pObject.hdr->objid;

        /* if hit anonymous object at end then loop up to but not including this object */
        if(id->name == GR_CHART_OBJNAME_ANON)
        {
            endObject = hitObject[hitIndex];
            continue;
        }

        return(hitIndex);
    }
}

#endif /* RISCOS */

#if RISCOS

/******************************************************************************
*
* convert absolute screen coords to diagram bottom-left relative
*
******************************************************************************/

extern void
gr_chartedit_riscos_point_from_abs(
    P_GR_CHART cp,
    _OutRef_    P_GR_POINT point,
    GR_OSUNIT x,
    GR_OSUNIT y)
{
    assert(cp);

    /* make relative and subtract display offset (note that it never had a scy contribution!) */
    x -= cp->core.editsave.open_box.x0 - cp->core.editsave.open_scx;
    y -= cp->core.editsave.open_box.y0;

    gr_chartedit_riscos_point_from_rel(cp, point, x, y);
}

/******************************************************************************
*
* convert window work area relative coords to diagram bottom-left relative
*
******************************************************************************/

static void
gr_chartedit_riscos_point_from_rel(
    P_GR_CHART cp,
    _OutRef_    P_GR_POINT point,
    GR_OSUNIT x,
    GR_OSUNIT y)
{
    P_GR_CHARTEDITOR cep;

    assert(cp);

    cep = gr_chartedit_cep_from_ceh(cp->core.ceh);
    assert(cep);

    /* subtract display offset */
    x -= cep->riscos.diagram_off_x;
    y -= cep->riscos.diagram_off_y;

    point->x = gr_pixit_from_riscos(x);
    point->y = gr_pixit_from_riscos(y);

    gr_point_scale(point, point, NULL, &cep->riscos.scale_from_screen16);
}

#endif /* RISCOS */

/******************************************************************************
*
* clear selection - queue a redraw to take place later
*
******************************************************************************/

extern void
gr_chartedit_selection_clear(
    P_GR_CHARTEDITOR cep)
{
    GR_CHART_OBJID id;

    assert(cep);

    cep->selection.temp = 0;

    id = gr_chart_objid_anon;

    (void) gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_SELECTREQ, &id);

    gr_chartedit_selection_kill_repr(cep);

    cep->selection.id = id;

    gr_chartedit_selection_display(cep);
}

/******************************************************************************
*
* temporary selections never created repr so no redraw needed
*
******************************************************************************/

extern void
gr_chartedit_selection_clear_if_temp(
    P_GR_CHARTEDITOR cep)
{
    if(cep->selection.temp)
    {
        cep->selection.temp = 0;
        cep->selection.id   = gr_chart_objid_anon;
    }
}

static void
gr_chartedit_selection_display(
    P_GR_CHARTEDITOR cep)
{
    char title[BUF_MAX_GR_CHART_OBJID_REPR];

    assert(cep);

    gr_chart_object_name_from_id(title, elemof32(title), cep->selection.id);

    #if RISCOS
    win_setfield(cep->riscos.w, (wimp_i) GR_CHARTEDIT_TEM_ICON_SELECTION, title);
    #endif
}

extern void
gr_chartedit_selection_kill_repr(
    P_GR_CHARTEDITOR cep)
{
    assert(cep);

    if(cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
    {
        /* SKS after 4.12 26mar92 - called on dispose after window deletion */
        if(cep->riscos.w)
            gr_chartedit_selection_xor(cep);

        gr_riscdiag_diagram_delete(&cep->selection.p_gr_diag->gr_riscdiag);
    }
}

/******************************************************************************
*
* make a selection - best to do it snappily!
*
******************************************************************************/

/*ncr*/
static BOOL
gr_chartedit_selection_make(
    P_GR_CHARTEDITOR cep,
    const GR_CHART_OBJID * id)
{
    assert(cep);

    if(gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_SELECTREQ, (P_ANY) id))
    {
        /* worthwhile doing it again? */
        if(0 != memcmp(&cep->selection.id, id, sizeof(*id)))
        {
            /* remove the old one */
            gr_chartedit_selection_kill_repr(cep);

            cep->selection.id = *id;

            if(cep->selection.temp)
                return(1);

            gr_chartedit_selection_display(cep);

            /* build a new selection representation */
            gr_chartedit_selection_make_repr(cep);

            return(1);
        }
    }

    return(0);
}

#if RISCOS

/* build a selection representation */

static GR_DIAG_OFFSET selectionPathStart;

extern void
gr_chartedit_selection_make_repr(
    P_GR_CHARTEDITOR cep)
{
    P_GR_CHART        cp;
    GR_DIAG_OFFSET    hitObject;
    P_GR_DIAG_OBJECT  pObject;
    GR_DIAG_PROCESS_T process;
    GR_LINESTYLE      linestyle;
    S32               simple_box;
    STATUS res;

    assert(cep);

    if(cep->selection.temp)
        return;

    if(cep->selection.id.name == GR_CHART_OBJNAME_ANON)
        return;

    gr_linestyle_init(&linestyle);

    /* Draw in an unmottled colour */
    linestyle.fg = gr_colour_from_riscDraw(wimpt_RGB_for_wimpcolour(11));
    #ifdef GR_CHART_FILL_LINE_INTERSTICES
    gr_colour_set_NONE(linestyle.bg);
    #endif

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* search **actual** diagram for first object of right class OR ONE OF ITS CHILDREN */
    zero_struct(process);
    process.recurse       = 1;
    process.find_children = 1;

    hitObject = gr_diag_object_search_between(cp->core.p_gr_diag,
                                              cep->selection.id,
                                              GR_DIAG_OBJECT_FIRST,
                                              GR_DIAG_OBJECT_LAST,
                                              process);

    if(hitObject != GR_DIAG_OBJECT_NONE)
    {
        U32 objectSize;

        pObject.p_byte = gr_diag_getoffptr(BYTE, cp->core.p_gr_diag, hitObject);

        objectSize         = pObject.hdr->n_bytes;
        cep->selection.box = pObject.hdr->bbox;

        simple_box = 1;

        /* scan rest of actual diagram and accumulate bboxes for objects of same class OR THEIR CHILDREN */

        for(;;)
        {
            hitObject = gr_diag_object_search_between(cp->core.p_gr_diag,
                                                      cep->selection.id,
                                                      hitObject + objectSize,
                                                      GR_DIAG_OBJECT_LAST,
                                                      process);

            if(hitObject == GR_DIAG_OBJECT_NONE)
                break;

            pObject.p_byte = gr_diag_getoffptr(BYTE, cp->core.p_gr_diag, hitObject);
            objectSize = pObject.hdr->n_bytes;
            gr_box_union(&cep->selection.box, NULL, &pObject.hdr->bbox);
        }
    }
    else
    {
        cep->selection.box.x0 = 0;
        cep->selection.box.y0 = 0;
        cep->selection.box.x1 = 0;
        cep->selection.box.y1 = 0;

        simple_box = 0;
    }

    if(simple_box)
    {
        linestyle.width = 0;

        /* never, even if the program is buggy, make a huge dot-dashed path
         * as it takes forever to render; limit to twice the 'paper' size
        */
        linestyle.pattern = (GR_LINE_PATTERN_HANDLE) 1;

        if((cep->selection.box.x1 - cep->selection.box.x0) > 2 * cp->core.layout.width)
            linestyle.pattern = GR_LINE_PATTERN_STANDARD;

        if((cep->selection.box.y1 - cep->selection.box.y0) > 2 * cp->core.layout.height)
            linestyle.pattern = GR_LINE_PATTERN_STANDARD;

        for(;;)
        {
            DRAW_BOX draw_box;

            status_break(res = gr_riscdiag_diagram_new(&cep->selection.p_gr_diag->gr_riscdiag, "selection", 0));

            if((cep->selection.box.x1 - cep->selection.box.x0) < 2 * GR_PIXITS_PER_RISCOS)
                cep->selection.box.x1 = cep->selection.box.x0  + 2 * GR_PIXITS_PER_RISCOS;

            if((cep->selection.box.y1 - cep->selection.box.y0) < 2 * GR_PIXITS_PER_RISCOS)
                cep->selection.box.y1 = cep->selection.box.y0  + 2 * GR_PIXITS_PER_RISCOS;

            draw_box_from_gr_box(&draw_box, &cep->selection.box);

            res = gr_riscdiag_rectangle_new(&cep->selection.p_gr_diag->gr_riscdiag, &selectionPathStart, &draw_box, &linestyle, NULL);

            if(status_done(res))
            {
                gr_riscdiag_diagram_end(&cep->selection.p_gr_diag->gr_riscdiag);

                gr_chartedit_selection_xor(cep);
            }
            else
                gr_riscdiag_diagram_delete(&cep->selection.p_gr_diag->gr_riscdiag);

            break;
        }

        if(status_fail(res))
            gr_chartedit_winge(res);
    }
}

/******************************************************************************
*
* render path object using the Draw module
*
******************************************************************************/

static int /*colnum*/
colourtrans_ReturnColourNumber(
    wimp_paletteword entry)
{
    _kernel_swi_regs rs;
    rs.r[0] = entry.word;
    return(_kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) ? 0 : rs.r[0]);
}

_Check_return_
static STATUS
gr_riscdiag_path_render(
    P_GR_RISCDIAG p_gr_riscdiag,
    GR_DIAG_OFFSET pathStart,
    PC_GR_RISCDIAG_RENDERINFO rip)
{
    P_DRAW_OBJECT pObject;
    P_DRAW_DASH_HEADER line_dash_pattern = NULL;
    P_ANY path_seq;
    drawmod_pathelemptr bodge_path_seq;
    drawmod_filltype fill;
    drawmod_line line;
    drawmod_transmat path_xform;
    int gcol_out;
    os_error * e;

    /* form output transformation matrix */
    path_xform[0] = (int) rip->scale16.x;
    path_xform[1] = 0;
    path_xform[2] = 0;
    path_xform[3] = (int) rip->scale16.y;
    path_xform[4] = (int) (rip->origin.x * GR_RISCDRAW_PER_PIXIT);
    path_xform[5] = (int) (rip->origin.y * GR_RISCDRAW_PER_PIXIT);

    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, pathStart);

    /* path really starts here (unless dashed) */
    path_seq = pObject.path + 1;

    if(pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
    {
        line_dash_pattern = (P_DRAW_DASH_HEADER) path_seq;

        path_seq = PtrAddBytes(P_U8, line_dash_pattern, sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * line_dash_pattern->dashcount);
    }

    bodge_path_seq.bytep = path_seq;

    /* fill the path */

    if(pObject.path->fillcolour != (DRAW_COLOUR) -1)
    {
        e = colourtran_setGCOL(* (wimp_paletteword *) &pObject.path->fillcolour, 0, rip->action, &gcol_out);

        if(e)
            return(0);

        fill = (drawmod_filltype) (fill_FBint | fill_FNonbint);

        /* two bit winding rule field */
        if(pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
            fill = (drawmod_filltype) (fill | ((pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK) >> 5));

        e = drawmod_fill(bodge_path_seq /* path sequence */,
                         fill_Default   /* fill style    */,
                         &path_xform    /* xform matrix  */,
                         /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
                         2 * GR_RISCDRAW_PER_RISCOS);

        if(e)
            return(0);
    }

    /* stroke the path */

    if(pObject.path->pathcolour != (DRAW_COLOUR) -1)
    {
        // e = colourtran_setGCOL(* (wimp_paletteword *) &pObject.path->pathcolour, 0, rip->action, &gcol_out);
        wimp_paletteword os_rgb_foreground;
        wimp_paletteword os_rgb_background;

        os_rgb_foreground.word = pObject.path->pathcolour;

        os_rgb_background.bytes.gcol  = 0;
        os_rgb_background.bytes.red   = 0xFF; /*p_rgb_background->r;*/
        os_rgb_background.bytes.green = 0xFF; /*p_rgb_background->g;*/
        os_rgb_background.bytes.blue  = 0xFF; /*p_rgb_background->b;*/

        { /* New machines usually demand this mechanism */
        int colnum_foreground = colourtrans_ReturnColourNumber(os_rgb_foreground);
        int colnum_background = colourtrans_ReturnColourNumber(os_rgb_background);
        _kernel_swi_regs rs;
        rs.r[0] = 3;
        rs.r[1] = colnum_foreground ^ colnum_background;
        (void) _kernel_swi(OS_SetColour, &rs, &rs);
        } /*block*/

        fill = (drawmod_filltype) (fill_FBint | fill_FNonbint | fill_FBext);

        /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
        line.flatness            = 2 * GR_RISCDRAW_PER_RISCOS;
        line.thickness           =  pObject.path->pathwidth;
        line.spec.join           = (pObject.path->pathstyle.flags & DRAW_PS_JOIN_PACK_MASK)     >> DRAW_PS_JOIN_PACK_SHIFT;
        line.spec.leadcap        = (pObject.path->pathstyle.flags & DRAW_PS_STARTCAP_PACK_MASK) >> DRAW_PS_STARTCAP_PACK_SHIFT;
        line.spec.trailcap       = (pObject.path->pathstyle.flags & DRAW_PS_ENDCAP_PACK_MASK)   >> DRAW_PS_ENDCAP_PACK_SHIFT;
        line.spec.reserved8      = 0;
        line.spec.mitrelimit     = 0x000A0000;     /* 10.0 "PostScript default" from DrawFiles doc'n */
        line.spec.lead_tricap_w  =
        line.spec.trail_tricap_w = (pObject.path->pathstyle.tricap_w * pObject.path->pathwidth) / 16;
        line.spec.lead_tricap_h  =
        line.spec.trail_tricap_h = (pObject.path->pathstyle.tricap_h * pObject.path->pathwidth) / 16;
        line.dash_pattern        = (P_ANY) line_dash_pattern;

        e = drawmod_stroke(bodge_path_seq /* path sequence */,
                           fill           /* fill style    */,
                           &path_xform    /* xform matrix  */,
                           &line          /* line style    */);

        if(e)
            return(0);
    }

    return(1);
}

static void
gr_chartedit_selection_xor_core(
    P_GR_CHARTEDITOR cep,
    const wimp_redrawstr * er)
{
    P_GR_CHART cp;
    wimp_redrawstr r;
    GR_RISCDIAG_RENDERINFO rin;

    if(!cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        return;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    r = *er;

    /* files always displayed at bottom left (therefore NO scy contribution!) */
    r.box.x0 = cp->core.editsave.open_box.x0 - cp->core.editsave.open_scx;;
    r.box.y1 = cp->core.editsave.open_box.y0; /* NB! */

        /* add in display offset */
    r.box.x0 += cep->riscos.diagram_off_x;
    r.box.y1 += cep->riscos.diagram_off_y;

    rin.origin.x    = (GR_COORD) r.box.x0 * GR_PIXITS_PER_RISCOS;
    rin.origin.y    = (GR_COORD) r.box.y1 * GR_PIXITS_PER_RISCOS;
    rin.scale16.x   = cep->riscos.scale_from_diag16.x;
    rin.scale16.y   = cep->riscos.scale_from_diag16.y;
    rin.cliprect.x0 = (GR_COORD) r.g.x0 * GR_PIXITS_PER_RISCOS;
    rin.cliprect.y0 = (GR_COORD) r.g.y0 * GR_PIXITS_PER_RISCOS;
    rin.cliprect.x1 = (GR_COORD) r.g.x1 * GR_PIXITS_PER_RISCOS;
    rin.cliprect.y1 = (GR_COORD) r.g.y1 * GR_PIXITS_PER_RISCOS;
    rin.action      = 3;

    status_consume(gr_riscdiag_path_render(&cep->selection.p_gr_diag->gr_riscdiag, selectionPathStart, &rin));
}

static void
gr_chartedit_selection_xor(
    P_GR_CHARTEDITOR cep)
{
    wimp_redrawstr r;
    S32            more;

    if(!cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        return;

    gr_chartedit_riscos_setup_redrawstr(cep, &cep->selection.p_gr_diag->gr_riscdiag.draw_diag.data, &r);

    if(wimpt_complain(wimp_update_wind(&r, &more)))
        more = FALSE;

    while(more)
    {
        gr_chartedit_selection_xor_core(cep, &r);

        if(wimpt_complain(wimp_get_rectangle(&r, &more)))
            more = FALSE;
    }
}

/******************************************************************************
*
* RISC OS event handling for the chart editor window
*
******************************************************************************/

extern void
gr_chartedit_set_scales(
    P_GR_CHARTEDITOR cep)
{
    P_GR_DIAG p_gr_diag;
    P_DRAW_FILE_HEADER p_draw_file_header = NULL;
    char buffer[32];
    F64 pct_scale;
    S32 diagbelow;

    gr_chart_diagram(&cep->ch, &p_gr_diag);

    if(p_gr_diag)
        p_draw_file_header = gr_riscdiag_getoffptr(DRAW_FILE_HEADER, &p_gr_diag->gr_riscdiag, 0);

    cep->riscos.scale_from_diag = 1.0;
    cep->riscos.diagram_off_x   = GR_CHARTEDIT_DISPLAY_LM_OS;
    cep->riscos.diagram_off_y   = GR_CHARTEDIT_DISPLAY_BM_OS;
    diagbelow                   = 0;

    if(p_draw_file_header)
    {
        P_GR_CHART  cp;
        F64 scale_from_layout;
        S32 diagsize;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        /* size has already had margins subtracted so ok to use like this */
        scale_from_layout = (F64) cep->size.y / (F64) cp->core.layout.size.y;

        diagbelow = (p_draw_file_header->bbox.y0 < 0) ? -p_draw_file_header->bbox.y0 : 0;
        diagsize   = p_draw_file_header->bbox.y1 + diagbelow;

        if(diagsize)
            cep->riscos.scale_from_diag = (((F64) cep->size.y) * GR_RISCDRAW_PER_PIXIT) / diagsize;

        /* impose limit for stupid diagram creation */
        if( cep->riscos.scale_from_diag < scale_from_layout / 8.0)
            cep->riscos.scale_from_diag = scale_from_layout / 8.0;

        /* 0.0010 is safe limit imposed by rendering code */
        if( cep->riscos.scale_from_diag < 0.0010)
            cep->riscos.scale_from_diag = 0.0010;
    }

    cep->riscos.scale_from_diag16.x   = gr_scale_from_f64(cep->riscos.scale_from_diag);
    cep->riscos.scale_from_diag16.y   = cep->riscos.scale_from_diag16.x;

    /* backconvert in an attempt to get similar scaling between drawfdiag and my stuff */
    cep->riscos.scale_from_diag       = gr_f64_from_scale(cep->riscos.scale_from_diag16.x);

    cep->riscos.scale_from_screen16.x = gr_scale_from_f64(1.0 / cep->riscos.scale_from_diag);
    cep->riscos.scale_from_screen16.y = cep->riscos.scale_from_screen16.x;

    pct_scale = cep->riscos.scale_from_diag * 100.0;

    if(diagbelow)
    {
        GR_COORD scalebelow = gr_coord_scale(diagbelow, cep->riscos.scale_from_diag16.y);
        cep->riscos.diagram_off_y += ((int) scalebelow) / GR_RISCDRAW_PER_RISCOS;
    }

    if((S32) pct_scale == pct_scale)
        (void) sprintf(buffer, "%d%%",   (S32) pct_scale);
    else
        (void) sprintf(buffer, "%.1f%%",        pct_scale);

    win_setfield(cep->riscos.w, GR_CHARTEDIT_TEM_ICON_ZOOM, buffer);
}

extern void
gr_chartedit_setwintitle(
    P_GR_CHART cp)
{
    assert(cp);

    if(cp->core.ceh)
    {
        GR_CHARTEDIT_NOTIFY_TITLE_STR t;
        P_GR_CHARTEDITOR cep;

        cep = gr_chartedit_cep_from_ceh(cp->core.ceh);
        assert(cep);

        gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_TITLEREQ, &t);

        if(cp->core.modified)
            xstrkat(t.title, elemof32(t.title), " *");

        win_settitle(cep->riscos.w, t.title);
    }
}

/* SKS after 4.12 25mar92 - finally got round to putting this in! */

extern void
gr_chartedit_warning(
    P_GR_CHART cp,
    _InVal_     STATUS err)
{
    if(!cp->core.ceh)
    {
        if(status_fail(err))
            gr_chartedit_winge(err);
    }
    else
    {
        P_GR_CHARTEDITOR cep;

        cep = gr_chartedit_cep_from_ceh(cp->core.ceh);
        assert(cep);

        win_setfield(cep->riscos.w, (wimp_i) GR_CHARTEDIT_TEM_ICON_BASE, err ? string_lookup(err) : "");
    }
}

/******************************************************************************
*
* note that the 'switch statement from hell' has been broken up here ...
*
******************************************************************************/

#define iconbar_height    (96)

#define topline_height    (wimpt_dy())
#define bottomline_height (wimpt_dy())
#define leftline_width    (wimpt_dx())
#define rightline_width   (wimpt_dx())

static void
gr_chartedit_riscos_open(
    wimp_openstr * eo,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;
    P_GR_CHART       cp;
    wimp_openstr    o;
    GR_POINT new_size;
    GR_CHARTEDIT_NOTIFY_RESIZE_STR s;
    S32             max_poss_width, max_poss_height;
    S32             resize_y = FALSE;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    o = *eo;

    /* peruse the requested parameters for silliness */
    max_poss_width  = wimpt_xsize() - leftline_width /*- wimpt_vscroll_width()*/ + rightline_width;
    max_poss_height = wimpt_ysize() - wimpt_title_height() - wimpt_hscroll_height() /* - bottomline_height */;

    if( o.box.x1 > o.box.x0 + max_poss_width)
        o.box.x1 = o.box.x0 + max_poss_width;

    if( o.box.y0 < o.box.y1 - max_poss_height)
        o.box.y0 = o.box.y1 - max_poss_height;

    if(!cep->riscos.heading_size)
    {
        /* on first open read baseline from an icon offset in the window */
        wimp_icon icon;
        wimpt_safe(wimp_get_icon_info(cep->riscos.w, GR_CHARTEDIT_TEM_ICON_BASE, &icon));
        /*                   +ve = small -ve/+ve + larger +ve */
        cep->riscos.heading_size  = (GR_OSUNIT) icon.box.y0 + cp->core.editsave.open_scy;
    }

    /* widening, no loss of precision */
    new_size.x = gr_pixit_from_riscos(((GR_OSUNIT) o.box.x1 - o.box.x0) -
                                 ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_LM_OS + GR_CHARTEDIT_DISPLAY_RM_OS));
    new_size.y = gr_pixit_from_riscos(((GR_OSUNIT) o.box.y1 - o.box.y0) -
                                 ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_BM_OS + GR_CHARTEDIT_DISPLAY_TM_OS) -
                                 cep->riscos.heading_size);

    /* don't allow box to go too small */
    new_size.x = MAX(new_size.x, 32 * GR_PIXITS_PER_RISCOS);
    new_size.y = MAX(new_size.y, 32 * GR_PIXITS_PER_RISCOS);

    /* simple moves,fronts,backs,tinys of the editing window are irrelevant */

    s.old_size = cep->size; /* remember for even when we think this ain't going to be a resize ... see below */

    if((new_size.x != cep->size.x)  ||  (new_size.y != cep->size.y))
    {
        S32 allow_resize;

        /* ask owner if this resize op is ok */
        s.new_size = new_size;

        allow_resize = (gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_RESIZEREQ, &s) != 0);

        if(!allow_resize)
            /* try to open at old size */
            s.new_size = s.old_size;

        /* only tweak the bottom right-hand corner, top left stays as requested */
        o.box.x1  = o.box.x0;
        o.box.x1 += (int) gr_os_from_pixit_floor(s.new_size.x);
        o.box.x1 += GR_CHARTEDIT_DISPLAY_LM_OS;
        o.box.x1 += GR_CHARTEDIT_DISPLAY_RM_OS;

        o.box.y0  = o.box.y1;
        o.box.y0 -= (int) gr_os_from_pixit_floor(s.new_size.y);
        o.box.y0 -= (int) cep->riscos.heading_size;
        o.box.y0 -= GR_CHARTEDIT_DISPLAY_BM_OS;
        o.box.y0 -= GR_CHARTEDIT_DISPLAY_TM_OS;
    }

    if(!wimpt_complain(win_open_wind(&o)))
        /* o is updated to give actual opened state */
    {
        cp->core.editsave.open_box = o.box; /* keep useful abs screen info for updates/redraw event */
        cp->core.editsave.open_scx = o.scx;
        cp->core.editsave.open_scy = o.scy;

        cep->size.x = gr_pixit_from_riscos(((GR_OSUNIT) o.box.x1 - o.box.x0) -
                                           ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_LM_OS + GR_CHARTEDIT_DISPLAY_RM_OS));
        cep->size.y = gr_pixit_from_riscos(((GR_OSUNIT) o.box.y1 - o.box.y0) -
                                           ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_BM_OS + GR_CHARTEDIT_DISPLAY_TM_OS) -
                                           cep->riscos.heading_size);

        /* opening might force resize anyway */
        resize_y = (cep->size.y != s.old_size.y);

        if(resize_y)
        {
            /* get ***all*** this old area (but not the heading) redrawn sometime */

            /* important thing to bear in mind is that at this point we
             * don't have the old open info so we can't selectively erase
             * the old diagram, but it don't matter as it's the only object
             * in this window!
            */
            wimp_redrawstr r;

            r.w = cep->riscos.w;

            /* NB. all are work area offsets */
            r.box.x0 = cp->core.editsave.open_scx;
            r.box.x1 = cp->core.editsave.open_scx + (cp->core.editsave.open_box.x1 - cp->core.editsave.open_box.x0); /* entire window to right */
            r.box.y1 = cp->core.editsave.open_scy - (int) cep->riscos.heading_size;
            r.box.y0 = cp->core.editsave.open_scy - (cp->core.editsave.open_box.y1 - cp->core.editsave.open_box.y0); /* entire window to bottom */
            wimpt_complain(wimp_force_redraw(&r));
        }

        /* whether p_gr_diag exists or not is irrelevent a ce moment */

        gr_chartedit_set_scales(cep);
    }
}

/******************************************************************************
*
* redraw the contents of the chart editing window
* for this time round the redraw/update loop
*
******************************************************************************/

static void
gr_chartedit_riscos_redraw_core(
    P_GR_CHARTEDITOR        cep,
    const wimp_redrawstr * r)
{
    P_GR_CHART cp;
    wimp_redrawstr passed_r;
    P_GR_DIAG p_gr_diag;

    assert(cep);

    assert(cep->ch);
    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    passed_r = *r;

    /* origin (NOT top left) of Draw file taken by renderer to be at r.box.x0,r.box.y1 (abs screen coords) */
    /* r.box.x1,r.box.y0 ignored by draw_render_diag */

    /* ensure clipped to viewable area of box */
    {
    wimp_box display_box; /* (abs screen coords) */

    display_box     = cp->core.editsave.open_box;

    /* viewable box now smaller than whole window */
    display_box.y1 -= (int) cep->riscos.heading_size;

    if(!gr_box_intersection((P_GR_BOX) &passed_r.g, NULL, (P_GR_BOX) &display_box))
        return;
    }

    /* set our own graphics window */
    wimpt_safe(bbc_gwindow(passed_r.g.x0,              passed_r.g.y0,
                           passed_r.g.x1 - wimpt_dx(), passed_r.g.y1 - wimpt_dy()));

    gr_chart_diagram(&cep->ch, &p_gr_diag);

    if(p_gr_diag)
    {
        /* always display files starting at bottom left (therefore NO scy contribution!) */

        passed_r.box.x0 = cp->core.editsave.open_box.x0 - cp->core.editsave.open_scx;
        passed_r.box.y1 = cp->core.editsave.open_box.y0; /* NB! */

        /* add in display offset */
        passed_r.box.x0 += cep->riscos.diagram_off_x;
        passed_r.box.y1 += cep->riscos.diagram_off_y;

        passed_r.scx    = 0;
        passed_r.scy    = 0;

        if(p_gr_diag->gr_riscdiag.dd_allocsize)
            status_assert(draw_do_render(gr_riscdiag_getoffptr(BYTE, &p_gr_diag->gr_riscdiag, 0), p_gr_diag->gr_riscdiag.dd_allocsize,
                                         passed_r.box.x0, passed_r.box.y1, cep->riscos.scale_from_diag, cep->riscos.scale_from_diag, (P_GDI_BOX) &passed_r.g));
    }

    /* restore caller's graphics window */
    wimpt_safe(bbc_gwindow(r->g.x0,              r->g.y0,
                           r->g.x1 - wimpt_dx(), r->g.y1 - wimpt_dy()));

    /* and the selection too. use this redrawstr rather than updating window */
    if(cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        gr_chartedit_selection_xor_core(cep, &passed_r);
}

static void
gr_chartedit_riscos_redraw(
    struct _wimp_eventdata_redraw * er,
    GR_CHARTEDIT_HANDLE             ceh)
{
    P_GR_CHARTEDITOR cep;
    P_GR_CHART       cp;
    wimp_redrawstr  r;
    S32             more;

    assert(ceh);
    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    assert(cep->ch);
    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    r.w = er->w;

    if(!wimpt_complain(wimp_redraw_wind(&r, &more)))
        while(more)
        {
            gr_chartedit_riscos_redraw_core(cep, &r);

            if(wimpt_complain(wimp_get_rectangle(&r, &more)))
                more = FALSE;
        }
}

/******************************************************************************
*
* force the redraw of part of the chart editing window that
* contains the draw file referenced by dd_handle, either
* deferred or immediate update
*
******************************************************************************/

static void
gr_chartedit_riscos_setup_redrawstr(
    P_GR_CHARTEDITOR  cep,
    flex_ptr         pdf_handle,
    wimp_redrawstr * r /*out*/)
{
    P_GR_CHART cp;
    S32 dx, dy;
    PC_DRAW_FILE_HEADER pDiagHdr;
    DRAW_BOX box;

    assert(pdf_handle && *pdf_handle);
    assert(r);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    dx = wimpt_dx();
    dy = wimpt_dy();

    r->w = cep->riscos.w;

    /* always displaying files starting at bottom left */
    /* NB. all are work area offsets (incl,incl,excl,excl) */

    r->box.x0 = cep->riscos.init_scx;
    /* large -ve        small +ve              large +ve              small -ve */
    r->box.y0 = cp->core.editsave.open_box.y0 - cp->core.editsave.open_box.y1 + cep->riscos.init_scy;

    /* add in the display offset */
    r->box.x0 += cep->riscos.diagram_off_x;
    r->box.y0 += cep->riscos.diagram_off_y;

    /* start with null size box and expand sides as appropriate */
    r->box.x1 = r->box.x0;
    r->box.y1 = r->box.y0;

    pDiagHdr = (P_ANY) *pdf_handle;

    /* only redraw bboxed area (taking care to expand to encompass pixels!) */
    box = pDiagHdr->bbox;

    gr_box_scale((P_GR_BOX) &box, (P_GR_BOX) &box, NULL, &cep->riscos.scale_from_diag16);

    r->box.x0 += (int) gr_round_os_to_floor((GR_OSUNIT) box.x0, GR_RISCDRAW_PER_RISCOS * dx) * dx;
    r->box.y0 += (int) gr_round_os_to_floor((GR_OSUNIT) box.y0, GR_RISCDRAW_PER_RISCOS * dy) * dy;
    r->box.x1 += (int) gr_round_os_to_ceil( (GR_OSUNIT) box.x1, GR_RISCDRAW_PER_RISCOS * dx) * dx + dx; /* excl */
    r->box.y1 += (int) gr_round_os_to_ceil( (GR_OSUNIT) box.y1, GR_RISCDRAW_PER_RISCOS * dy) * dy + dy; /* excl */
}

static void
gr_chartedit_riscos_force_redraw(
    P_GR_CHARTEDITOR cep,
    flex_ptr        pdf_handle,
    S32             immediate)
{
    wimp_redrawstr r;

    assert(cep);

    if(!*pdf_handle)
        return;

    gr_chartedit_riscos_setup_redrawstr(cep, pdf_handle, &r);

    if(immediate)
    {
        S32 more;

        /* call redraw handler in update loop to get more instant effects */
        if(!wimpt_complain(wimp_update_wind(&r, &more)))
            while(more)
            {
                gr_chartedit_riscos_redraw_core(cep, &r);

                if(wimpt_complain(wimp_get_rectangle(&r, &more)))
                    more = FALSE;
            }
    }
    else
        /* queue a redraw for this bit of window */
        wimpt_complain(wimp_force_redraw(&r));
}

static void
gr_chartedit_riscos_button_click(
    const struct _wimp_eventdata_but * but,
    GR_CHARTEDIT_HANDLE ceh)
{
    /* find out what object we clicked on and tell owner */
    P_GR_CHARTEDITOR cep;
    S32             adjustclicked;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    /* no end-of-menu event on RISC OS 2.00 means we have to do this ourselves */
    gr_chartedit_selection_clear_if_temp(cep);

    adjustclicked = but->m.bbits & (wimp_BRIGHT | (wimp_BRIGHT << 8));

    switch(but->m.i)
    {
    case -1:
        /* display section background */
        {
        /* use adjust clicked state to enter deeper into structure than clicked */
        /* ie allow selection of point rather than looping back to enclosing series */
        GR_CHART_OBJID id;
        GR_DIAG_OFFSET hitObject[64];
        unsigned int hitIndex;
        GR_POINT point;
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        gr_chartedit_riscos_point_from_abs(cp, &point, but->m.x, but->m.y);

        if(but->m.bbits & ((wimp_BLEFT | wimp_BRIGHT) << 4))
        {
            /* drag */
            GR_POINT workareaoff;

            workareaoff.x = cp->core.editsave.open_scx;
            workareaoff.y = cp->core.editsave.open_scy - ((GR_OSUNIT) cp->core.editsave.open_box.y1 - cp->core.editsave.open_box.y0);

            /* add in display offset */
            workareaoff.x += cep->riscos.diagram_off_x;
            workareaoff.y += cep->riscos.diagram_off_y;

            switch(cep->selection.id.name)
            {
            case GR_CHART_OBJNAME_TEXT:
            case GR_CHART_OBJNAME_LEGEND:
                gr_chartedit_selected_object_drag_start(cep, &point, &workareaoff);
                break;

            default:
                break;
            }
            break;
        }

        hitIndex = gr_nodbg_bring_me_the_head_of_yuri_gagarin(cep, &id, hitObject,
                                                              point.x, point.y, adjustclicked);

        if(!adjustclicked)
            gr_chart_objid_find_parent(&id);

        if(hitObject[0] == GR_DIAG_OBJECT_NONE)
            /* clear selection - queue a redraw to take place later */
            gr_chartedit_selection_clear(cep);
        else
            gr_chartedit_selection_make(cep, &id);

        }
        break;

    case 0:
        /* editing section background */
        break;

    case 1:
        {
        /* selection display icon - either clear selection or select Chart/Plot area/Legend in cycle */
        if(adjustclicked)
            gr_chartedit_selection_clear(cep);
        else
        {
            GR_CHART_OBJID id;
            P_GR_CHART      cp;

            cp = gr_chart_cp_from_ch(cep->ch);

            id = cep->selection.id;

            switch(id.name)
            {
            case GR_CHART_OBJNAME_CHART:
                id.name   = GR_CHART_OBJNAME_PLOTAREA;
                id.has_no = 1;
                break;

            case GR_CHART_OBJNAME_PLOTAREA:
                if(cp->d3.bits.use)
                    if(++id.no < GR_CHART_N_PLOTAREAS)
                        break;

                if(cp->axes[0].charttype == GR_CHARTTYPE_PIE)
                    /* no axes to select */
                    goto maybe_select_legend;

                id.name   = GR_CHART_OBJNAME_AXIS;
                id.has_no = 1;
                id.no     = 0;

            /* deliberate drop thru ... */

            case GR_CHART_OBJNAME_AXIS:
                {
                U32 eaxes = 2;

                if(cp->d3.bits.use)
                    ++eaxes;

                eaxes *= (cp->axes_idx_max + 1);

                ++id.no;

                /* skip 3-D Axes 3 and 6 until punters can do anything to 'em */
                if(cp->d3.bits.use)
                    if((id.no == 3) || (id.no == 6))
                        ++id.no;

                /* skip second category axis for similar reasons */
                if((cp->axes[0].charttype == GR_CHARTTYPE_BAR) || (cp->axes[0].charttype == GR_CHARTTYPE_LINE))
                    if(id.no == (cp->d3.bits.use ? 4 : 3))
                        ++id.no;

                if(id.no <= eaxes)
                    break;
                }

            maybe_select_legend:;
                if(cp->legend.bits.on)
                {
                    id = gr_chart_objid_legend;
                    break;
                }

                /* deliberate drop thru ... */

            case GR_CHART_OBJNAME_LEGEND:
                gr_chart_objid_from_series_no(0, &id);
                goto select_next_series;
                break;

            case GR_CHART_OBJNAME_DROPSER:
                /* select associated series as hitting series may be hard */
                id.name = GR_CHART_OBJNAME_SERIES;
                break;

            case GR_CHART_OBJNAME_SERIES:
            select_next_series:
                {
                if(++id.no > (U32) cp->series.n)
                {
                    /* find first text object */
                    gr_chart_objid_from_text(0, &id);
                    goto select_next_text;
                }
                break;
                }

            case GR_CHART_OBJNAME_POINT:
            case GR_CHART_OBJNAME_DROPPOINT:
                {
                GR_SERIES_IDX series_idx;

                series_idx = gr_series_idx_from_external(cp, id.no);

                if(++id.subno > (U32) gr_travel_series_n_items_total(cp, series_idx))
                {
                    /* try finding P1 of next series */
                    id.subno = 1;
                    goto select_next_series;
                }
                break;
                }

            case GR_CHART_OBJNAME_TEXT:
            select_next_text:
                {
                P_GR_TEXT    t;
                LIST_ITEMNO key;

                key = id.no;

                if((t = collect_next(GR_TEXT, &cp->text.lbr, &key)) == NULL)
                    goto default_case;

                id.no = (U8) key;

                if(t->bits.unused)
                    goto select_next_text;
                }
                break;

            default:
            case GR_CHART_OBJNAME_ANON:
            default_case:
                id = gr_chart_objid_chart;
                break;
            }

            gr_chartedit_selection_make(cep, &id);
        }
        }

    default:
        break;
    }
}

static void
gr_chartedit_riscos_drag_complete(
    const wimp_box * dragboxp,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    gr_chartedit_selected_object_drag_complete(cep, dragboxp);
}

static S32
gr_chartedit_riscos_message(
    const wimp_msgstr * msg,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;
    S32             processed = TRUE;

    switch(msg->hdr.action)
    {
    case wimp_MDATASAVE:
        {
        /* possible object dragged into main window - would normally ask for DATALOAD
         * except that we don't actually want anything saving to us that isn't already on disc
        */
        S32 size;
        FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkimport(&size);

        if(gr_chart_saving_chart(NULL))
        {
            message_output(string_lookup(create_error(GR_CHART_ERR_NO_SELF_INSERT)));
            break;
        }

        switch(filetype)
        {
        case FILETYPE_DIRECTORY:
        case FILETYPE_APPLICATION:
            messagef(string_lookup(create_error(FILE_ERR_ISADIR)), msg->data.datasave.leaf);
            break;

        case FILETYPE_PIPEDREAM:
            /* can't detect at this point whether 'tis bog standard or a chart */
            break;

        default:
            if(gr_cache_can_import(filetype))
                message_output(string_lookup(create_error(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED)));
            break;
        }
        }
        break;

    case wimp_MDATALOAD:
        {
        /* when punter drops a Draw file on us, find out what object
         * we dropped on, unless we have a selection, in which case use that
        */
        P_U8 filename;
        FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);
        /* sets up reply too */

        trace_1(TRACE_MODULE_GR_CHART, "chart editing window got a DataLoad for %s", report_tstr(filename));

        if(gr_chart_saving_chart(NULL))
        {
            message_output(string_lookup(create_error(GR_CHART_ERR_NO_SELF_INSERT)));
            break;
        }

        switch(filetype)
        {
        case FILETYPE_DIRECTORY:
        case FILETYPE_APPLICATION:
            messagef(string_lookup(create_error(FILE_ERR_ISADIR)), filename);
            break;

        case FILETYPE_PIPEDREAM:
            break;

        default:
            if(gr_cache_can_import(filetype))
            {
                GR_CHART_OBJID  id;
                P_GR_CHART       cp;
                GR_CACHE_HANDLE cah;
                S32             res;

                cep = gr_chartedit_cep_from_ceh(ceh);
                assert(cep);

                cp = gr_chart_cp_from_ch(cep->ch);
                assert(cp);

                /* given that end-of-menu event is not available on RISC OS 2.00
                 * we must clear out the temp state ourselves
                */
                gr_chartedit_selection_clear_if_temp(cep);

                id = cep->selection.id;

                if(id.name == GR_CHART_OBJNAME_ANON)
                {
                    const wimp_msgdataload * dataload = &wimpt_last_event()->data.msg.data.dataload;

                    trace_0(TRACE_MODULE_GR_CHART, "no current selection - look for drop on object");

                    switch(dataload->i)
                    {
                    default:
                        /* other regions are uninteresting */
                        break;

                    case -1:
                        /* display section background */
                        {
                        GR_DIAG_OFFSET hitObject[64];
                        unsigned int hitIndex;
                        GR_POINT point;

                        gr_chartedit_riscos_point_from_abs(cp, &point, dataload->x, dataload->y);

                        hitIndex = gr_nodbg_bring_me_the_head_of_yuri_gagarin(cep, &id, hitObject,
                                                                              point.x, point.y, FALSE);

                        gr_chart_objid_find_parent(&id);

                        /* just use the above as an id selector */
                        }
                        break;
                    }
                }

                if(id.name == GR_CHART_OBJNAME_ANON)
                {
                    trace_0(TRACE_MODULE_GR_CHART, "still nothing hit - how boring");
                    break;
                }

                /* load the file, disallowing recolouring */

                res = gr_cache_entry_ensure(&cah, filename);

                if(status_done(res))
                {
                    GR_FILLSTYLE style;

                    gr_chart_objid_fillstyle_query(cp, id, &style);

                    style.fg.manual = 1;
                    style.pattern = (GR_FILL_PATTERN_HANDLE) cah;

                    style.bits.notsolid   = 1;
                    style.bits.pattern    = 1;
                    style.bits.norecolour = 1;

                    res = gr_chart_objid_fillstyle_set(cp, id, &style);
                }

                if(status_fail(res))
                    gr_chartedit_winge(res);
                else
                    gr_chart_modify_and_rebuild(&cep->ch);
            } /* fi(can import) */

            break;
        } /* esac(filetype) */

        /* note that this is mandatory */
        xferrecv_insertfileok();
        }
        break;

    default:
        trace_2(TRACE_MODULE_GR_CHART,
                "unprocessed wimp message to chartedit window: %s, ceh &%p",
                report_wimp_message(msg, FALSE), report_ptr_cast(ceh));
        processed = FALSE;
        break;
    }

    return(processed);
}

static S32
gr_chartedit_riscos_event_handler(
    wimp_eventstr * e,
    P_ANY handle)
{
    GR_CHARTEDIT_HANDLE ceh       = handle;
    S32                 processed = TRUE;

    trace_2(TRACE_MODULE_GR_CHART,
            "gr_chartedit_event_handler: %s, ceh &%p",
            report_wimp_event(e->e, &e->data), report_ptr_cast(ceh));

    switch(e->e)
    {
    case wimp_ECLOSE:
        {
        P_GR_CHARTEDITOR cep;

        /* call owner of editor to get him to do something about the close */
        cep = gr_chartedit_cep_from_ceh(ceh);
        assert(cep);

        (void) gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_CLOSEREQ, 0);

        /* DO NOT LOOK AT ANY CHART RELATED VARIABLES NOW ! */
        }
        break;

    case wimp_EOPEN:
        gr_chartedit_riscos_open(&e->data.o, ceh);
        break;

    case wimp_EREDRAW:
        gr_chartedit_riscos_redraw(&e->data.redraw, ceh);
        break;

    case wimp_EBUT:
        {
        P_GR_CHARTEDITOR cep;

        cep = gr_chartedit_cep_from_ceh(ceh);
        assert(cep);

        gr_chartedit_fontselect_kill(cep);

        gr_chartedit_riscos_button_click(&e->data.but, ceh);
        }
        break;

    case wimp_EUSERDRAG:
        gr_chartedit_riscos_drag_complete(&e->data.dragbox, ceh);
        break;

    case wimp_EPTRLEAVE:
    case wimp_EPTRENTER:
    case wimp_EGAINCARET:
    case wimp_ELOSECARET:
        processed = FALSE;
        break;

    case wimp_ESEND:
    case wimp_ESENDWANTACK:
        processed = gr_chartedit_riscos_message(&e->data.msg, ceh);
        break;

    default:
        trace_2(TRACE_MODULE_GR_CHART,
                "unprocessed wimp event to chartedit window: %s, ceh &%p",
                report_wimp_event(e->e, &e->data), report_ptr_cast(ceh));
        processed = FALSE;
        break;
    }

    return(processed);
}

static S32
gr_chartedit_riscos_unknown_event_previewer(
    wimp_eventstr * e,
    P_ANY           handle)
{
    P_GR_CHARTEDITOR cep;

    switch(e->e)
    {
    default:
        break;

    case wimp_ESEND:
    case wimp_ESENDWANTACK:
        switch(e->data.msg.hdr.action)
        {
        default:
            break;

        case wimp_PALETTECHANGE:
            /* redraw our Draw file as colour translation may have changed */
            cep = gr_chartedit_cep_from_ceh(handle);
            assert(cep);

            gr_chartedit_diagram_update_later(cep);
            break;
        }
        break;
    }

    return(0); /* unclaimed event */
}

#endif /* RISCOS */

/* end of gr_editc.c */
