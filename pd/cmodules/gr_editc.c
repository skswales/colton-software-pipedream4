/* gr_editc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart editing module */

/* SKS July 1991 */

/* SKS after PD 4.12 26mar92 - must NOT put flex anchors in listed data */

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
gr_chartedit_riscos_setup_update(
    P_GR_CHARTEDITOR cep,
    flex_ptr pdf_handle,
    _Out_       WimpUpdateWindowBlock * update_window);

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
    flex_ptr pdf_handle,
    _InVal_     BOOL immediate);

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
    void_WrapOsErrorReporting(winx_delete_window(&cep->riscos.window_handle));
    template_copy_dispose(&cep->riscos.window_template);

    /* SKS after PD 4.12 26mar92 - deallocate core for selection fake GR_DIAG */
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
        winx_send_front_window_request(cep->riscos.window_handle, FALSE);
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

    zero_struct_ptr_fn(cep);

    /* SKS after PD 4.12 26mar92 - allocate core for selection fake GR_DIAG */
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
        cep->riscos.window_template = template_copy_new(template_h);

        if(cep->riscos.window_template != NULL)
        {
            /* keep window at the same posn,size,scroll for chart if previously edited */
            cep->riscos.init_scroll_x = cep->riscos.window_template->xscroll;
            cep->riscos.init_scroll_y = cep->riscos.window_template->yscroll;

            cp = gr_chart_cp_from_ch(cep->ch);
            assert(cp);

            if(cp->core.editsave.open.visible_area.xmin == cp->core.editsave.open.visible_area.xmax)
            {
                cp->core.editsave.open.visible_area = cep->riscos.window_template->visible_area;
                cp->core.editsave.open.scroll_x = cep->riscos.window_template->xscroll;
                cp->core.editsave.open.scroll_y = cep->riscos.window_template->yscroll;
            }

            void_WrapOsErrorReporting(
                winx_create_window(cep->riscos.window_template,
                                   &cep->riscos.window_handle,
                                   gr_chartedit_riscos_event_handler,
                                   (P_ANY) ceh));
        }
    }

#if RISCOS
    if(HOST_WND_NONE == cep->riscos.window_handle)
    {
        gr_chartedit_dispose(cehp);
        return(0);
    }

    /* add uk previewer to window */
    win_add_unknown_event_processor(gr_chartedit_riscos_unknown_event_previewer, ceh);

    /* make window visible sometime */
    winx_send_front_window_request(cep->riscos.window_handle, FALSE);
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

    UNREFERENCED_PARAMETER(handle);

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
    BOOL             adjust_clicked)
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

    if(adjust_clicked)
    {
        /* tighter click box if Adjust clicked (but remember some objects may be truly subpixel ...) */
        selsize.cx = gr_pixit_from_riscos(1);
        selsize.cy = gr_pixit_from_riscos(1);
    }
    else
    {
        /* larger (but still small) click box if Select clicked */
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
        {
            if(hitObject[hitIndex] == GR_DIAG_OBJECT_NONE)
            {
                --hitIndex;
                break;
            }
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

    /* make relative and subtract display offset (note that it never had a scroll_y contribution!) */
    x -= cp->core.editsave.open.visible_area.xmin - cp->core.editsave.open.scroll_x;
    y -= cp->core.editsave.open.visible_area.ymin;

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
    winf_setfield(cep->riscos.window_handle, GR_CHARTEDIT_TEM_ICON_SELECTION, title);
    #endif
}

extern void
gr_chartedit_selection_kill_repr(
    P_GR_CHARTEDITOR cep)
{
    assert(cep);

    if(cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
    {
        /* SKS after PD 4.12 26mar92 - called on dispose after window deletion */
        if(cep->riscos.window_handle)
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
    linestyle.fg = gr_colour_from_riscDraw(wimptx_RGB_for_wimpcolour(11));
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

static inline int /*colnum*/
gr_editc_colourtrans_ReturnColourNumber(
    wimp_paletteword entry)
{
    _kernel_swi_regs rs;
    rs.r[0] = entry.word;
    return(_kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) ? 0 : rs.r[0]);
}

_Check_return_
static inline _kernel_oserror *
gr_editc_colourtrans_SetGCOL(
    _In_        unsigned int word,
    _In_        int flags,
    _In_        int gcol_action)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    rs.r[3] = flags;
    rs.r[4] = gcol_action;
    assert((rs.r[3] & 0xfffffe7f) == 0); /* just bits 7 and 8 are valid */
    return(_kernel_swi(ColourTrans_SetGCOL, &rs, &rs)); /* ignore gcol_out */
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
    _kernel_oserror * e;

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
        e = gr_editc_colourtrans_SetGCOL(* (PC_U32) &pObject.path->fillcolour, 0, rip->action);

        if(NULL != e)
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

        if(NULL != e)
            return(0);
    }

    /* stroke the path */

    if(pObject.path->pathcolour != (DRAW_COLOUR) -1)
    {
        // e = gr_editc_colourtrans_SetGCOL(* (PC_U32) &pObject.path->pathcolour, 0, rip->action);
        wimp_paletteword os_rgb_foreground;
        wimp_paletteword os_rgb_background;

        os_rgb_foreground.word = pObject.path->pathcolour;

        os_rgb_background.bytes.gcol  = 0;
        os_rgb_background.bytes.red   = 0xFF; /*p_rgb_background->r;*/
        os_rgb_background.bytes.green = 0xFF; /*p_rgb_background->g;*/
        os_rgb_background.bytes.blue  = 0xFF; /*p_rgb_background->b;*/

        { /* New machines usually demand this mechanism */
        int colnum_foreground = gr_editc_colourtrans_ReturnColourNumber(os_rgb_foreground);
        int colnum_background = gr_editc_colourtrans_ReturnColourNumber(os_rgb_background);
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

        if(NULL != e)
            return(0);
    }

    return(1);
}

static void
gr_chartedit_selection_xor_core(
    P_GR_CHARTEDITOR cep,
    _In_        const WimpRedrawWindowBlock * const redraw_window_block)
{
    P_GR_CHART cp;
    int xmin;
    int ymax;
    GR_RISCDIAG_RENDERINFO rin;

    if(!cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        return;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* files always displayed at bottom left (therefore NO scroll_y contribution!) */
    xmin = cp->core.editsave.open.visible_area.xmin - cp->core.editsave.open.scroll_x;
    ymax = cp->core.editsave.open.visible_area.ymin; /* NB! */

    /* add in display offset */
    xmin += cep->riscos.diagram_off_x;
    ymax += cep->riscos.diagram_off_y;

    rin.origin.x    = (GR_COORD) xmin * GR_PIXITS_PER_RISCOS;
    rin.origin.y    = (GR_COORD) ymax * GR_PIXITS_PER_RISCOS;
    rin.scale16.x   = cep->riscos.scale_from_diag16.x;
    rin.scale16.y   = cep->riscos.scale_from_diag16.y;
    rin.cliprect.x0 = (GR_COORD) redraw_window_block->redraw_area.xmin * GR_PIXITS_PER_RISCOS;
    rin.cliprect.y0 = (GR_COORD) redraw_window_block->redraw_area.ymin * GR_PIXITS_PER_RISCOS;
    rin.cliprect.x1 = (GR_COORD) redraw_window_block->redraw_area.xmax * GR_PIXITS_PER_RISCOS;
    rin.cliprect.y1 = (GR_COORD) redraw_window_block->redraw_area.ymax * GR_PIXITS_PER_RISCOS;
    rin.action      = 3;

    status_consume(gr_riscdiag_path_render(&cep->selection.p_gr_diag->gr_riscdiag, selectionPathStart, &rin));
}

static void
gr_chartedit_selection_xor(
    P_GR_CHARTEDITOR cep)
{
    WimpUpdateAndRedrawWindowBlock update_and_redraw_window_block;
    BOOL more;

    if(0 == cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        return;

    gr_chartedit_riscos_setup_update(cep, &cep->selection.p_gr_diag->gr_riscdiag.draw_diag.data, &update_and_redraw_window_block.update);

    if(NULL != WrapOsErrorReporting(tbl_wimp_update_window(&update_and_redraw_window_block.redraw, &more)))
        more = FALSE;

    while(more)
    {
        gr_chartedit_selection_xor_core(cep, &update_and_redraw_window_block.redraw);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&update_and_redraw_window_block.redraw, &more)))
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

    consume_int(sprintf(buffer, "%.1f%%", pct_scale));

    winf_setfield(cep->riscos.window_handle, GR_CHARTEDIT_TEM_ICON_ZOOM, buffer);
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

        win_settitle(cep->riscos.window_handle, t.title);
    }
}

/* SKS after PD 4.12 25mar92 - finally got round to putting this in! */

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

        winf_setfield(cep->riscos.window_handle, GR_CHARTEDIT_TEM_ICON_BASE, err ? string_lookup(err) : "");
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

static BOOL
gr_chartedit_riscos_Open_Window_Request(
    const WimpOpenWindowRequestEvent * const p_open_window_request,
    GR_CHARTEDIT_HANDLE ceh)
{
    /*updated*/ WimpOpenWindowRequestEvent open_window_request = *p_open_window_request;
    P_GR_CHARTEDITOR cep;
    P_GR_CHART       cp;
    GR_POINT new_size;
    GR_CHARTEDIT_NOTIFY_RESIZE_STR s;
    S32 max_poss_width, max_poss_height;
    S32 resize_y = FALSE;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* peruse the requested parameters for silliness */
    max_poss_width  = wimptx_display_size_cx() - leftline_width /*- wimptx_vscroll_width()*/ + rightline_width;
    max_poss_height = wimptx_display_size_cy() - wimptx_title_height() - wimptx_hscroll_height() /* - bottomline_height */;

    if( open_window_request.visible_area.xmax > open_window_request.visible_area.xmin + max_poss_width)
        open_window_request.visible_area.xmax = open_window_request.visible_area.xmin + max_poss_width;

    if( open_window_request.visible_area.ymin < open_window_request.visible_area.ymax - max_poss_height)
        open_window_request.visible_area.ymin = open_window_request.visible_area.ymax - max_poss_height;

    if(0 == cep->riscos.heading_size)
    {   /* on first open read baseline from an icon offset in the window */
        WimpGetIconStateBlock icon_state;
        icon_state.window_handle = cep->riscos.window_handle;
        icon_state.icon_handle = GR_CHARTEDIT_TEM_ICON_BASE;
        if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
            icon_state.icon.bbox.ymin = -32;

        /*                   +ve =                                   small -ve/+ve +                      larger +ve */
        cep->riscos.heading_size = (GR_OSUNIT) icon_state.icon.bbox.ymin + cp->core.editsave.open.scroll_y;
    }

    /* widening, no loss of precision */
    new_size.x = gr_pixit_from_riscos(BBox_width( &open_window_request.visible_area) -
                                      ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_LM_OS + GR_CHARTEDIT_DISPLAY_RM_OS));
    new_size.y = gr_pixit_from_riscos(BBox_height(&open_window_request.visible_area) -
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
        open_window_request.visible_area.xmax  = open_window_request.visible_area.xmin;
        open_window_request.visible_area.xmax += (int) gr_os_from_pixit_floor(s.new_size.x);
        open_window_request.visible_area.xmax += GR_CHARTEDIT_DISPLAY_LM_OS;
        open_window_request.visible_area.xmax += GR_CHARTEDIT_DISPLAY_RM_OS;

        open_window_request.visible_area.ymin  = open_window_request.visible_area.ymax;
        open_window_request.visible_area.ymin -= (int) gr_os_from_pixit_floor(s.new_size.y);
        open_window_request.visible_area.ymin -= (int) cep->riscos.heading_size;
        open_window_request.visible_area.ymin -= GR_CHARTEDIT_DISPLAY_BM_OS;
        open_window_request.visible_area.ymin -= GR_CHARTEDIT_DISPLAY_TM_OS;
    }

    if(NULL != WrapOsErrorReporting(winx_open_window(&open_window_request)))
        return(TRUE);

    /* open_window_request has been updated to give actual opened state */

    cp->core.editsave.open.visible_area = open_window_request.visible_area; /* keep useful abs screen info for updates/redraw event */
    cp->core.editsave.open.scroll_x = open_window_request.xscroll;
    cp->core.editsave.open.scroll_y = open_window_request.yscroll;

    cep->size.x = gr_pixit_from_riscos(BBox_width( &open_window_request.visible_area) -
                                       ((GR_OSUNIT) GR_CHARTEDIT_DISPLAY_LM_OS + GR_CHARTEDIT_DISPLAY_RM_OS));
    cep->size.y = gr_pixit_from_riscos(BBox_height(&open_window_request.visible_area) -
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
        BBox redraw_area;

        /* NB. all are work area offsets */
        redraw_area.xmin = cp->core.editsave.open.scroll_x;
        redraw_area.xmax = cp->core.editsave.open.scroll_x + BBox_width( &cp->core.editsave.open.visible_area); /* entire window to right */
        redraw_area.ymax = cp->core.editsave.open.scroll_y - (int) cep->riscos.heading_size;
        redraw_area.ymin = cp->core.editsave.open.scroll_y - BBox_height(&cp->core.editsave.open.visible_area); /* entire window to bottom */

        void_WrapOsErrorReporting(
            tbl_wimp_force_redraw(cep->riscos.window_handle,
                                  redraw_area.xmin, redraw_area.ymin,
                                  redraw_area.xmax, redraw_area.ymax));
    }

    /* whether p_gr_diag exists or not is irrelevent a ce moment */

    gr_chartedit_set_scales(cep);

    return(TRUE);
}

static BOOL
gr_chartedit_riscos_Close_Window_Request(
    const WimpCloseWindowRequestEvent * const close_window_request,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;

    UNREFERENCED_PARAMETER_InRef_(close_window_request);

    /* call owner of editor to get him to do something about the close */
    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    (void) gr_chartedit_notify(cep, GR_CHARTEDIT_NOTIFY_CLOSEREQ, 0);

    /* DO NOT LOOK AT ANY CHART RELATED VARIABLES NOW ! */
    return(TRUE);
}

/******************************************************************************
*
* redraw the contents of the chart editing window
* for this time round the redraw/update loop
*
******************************************************************************/

static void
gr_chartedit_riscos_redraw_core(
    P_GR_CHARTEDITOR cep,
    _In_        const WimpRedrawWindowBlock * const redraw_window_block)
{
    WimpRedrawWindowBlock passed_redraw_window_block = *redraw_window_block;
    P_GR_CHART cp;
    P_GR_DIAG p_gr_diag;

    assert(cep);

    assert(cep->ch);
    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* ensure clipped to viewable area of box */
    {
    BBox display_box; /* (abs screen coords) */

    display_box = cp->core.editsave.open.visible_area;

    /* viewable box now smaller than whole window */
    display_box.ymax -= (int) cep->riscos.heading_size;

    if(!gr_box_intersection((P_GR_BOX) &passed_redraw_window_block.redraw_area, NULL, (P_GR_BOX) &display_box))
        return;
    }

    /* set our own graphics window */
    wimpt_safe(riscos_vdu_define_graphics_window(passed_redraw_window_block.redraw_area.xmin,
                                                 passed_redraw_window_block.redraw_area.ymin,
                                                 passed_redraw_window_block.redraw_area.xmax - wimpt_dx(),
                                                 passed_redraw_window_block.redraw_area.ymax - wimpt_dy()));

    gr_chart_diagram(&cep->ch, &p_gr_diag);

    if(NULL != p_gr_diag)
    {
        /* origin (NOT top left) of Draw file taken by renderer to be at x0,y1 (abs screen coords) */
        int xmin;
        int ymax;

        /* always display files starting at bottom left (therefore NO scroll_y contribution!) */

        xmin = cp->core.editsave.open.visible_area.xmin - cp->core.editsave.open.scroll_x;
        ymax = cp->core.editsave.open.visible_area.ymin; /* NB! */

        /* add in display offset */
        xmin += cep->riscos.diagram_off_x;
        ymax += cep->riscos.diagram_off_y;

        if(0 != p_gr_diag->gr_riscdiag.dd_allocsize)
            status_assert(draw_do_render(gr_riscdiag_getoffptr(BYTE, &p_gr_diag->gr_riscdiag, 0), p_gr_diag->gr_riscdiag.dd_allocsize,
                                         xmin, ymax,
                                         cep->riscos.scale_from_diag, cep->riscos.scale_from_diag,
                                         (P_GDI_BOX) &passed_redraw_window_block.redraw_area));
    }

    /* restore caller's graphics window */
    wimpt_safe(riscos_vdu_define_graphics_window(redraw_window_block->redraw_area.xmin,
                                                 redraw_window_block->redraw_area.ymin,
                                                 redraw_window_block->redraw_area.xmax - wimpt_dx(),
                                                 redraw_window_block->redraw_area.ymax - wimpt_dy()));

    /* and the selection too */
    if(0 != cep->selection.p_gr_diag->gr_riscdiag.draw_diag.length)
        gr_chartedit_selection_xor_core(cep, redraw_window_block);
}

static BOOL
gr_chartedit_riscos_Redraw_Window_Request(
    const WimpRedrawWindowRequestEvent * const redraw_window_request,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;
    P_GR_CHART cp;
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;

    assert(ceh);
    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    assert(cep->ch);
    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    redraw_window_block.window_handle = redraw_window_request->window_handle;

    if(NULL != WrapOsErrorReporting(tbl_wimp_redraw_window(&redraw_window_block, &more)))
        more = FALSE;

    while(more)
    {
        gr_chartedit_riscos_redraw_core(cep, &redraw_window_block);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&redraw_window_block, &more)))
            more = FALSE;
    }

    return(TRUE);
}

/******************************************************************************
*
* force the redraw of part of the chart editing window that
* contains the draw file referenced by dd_handle, either
* deferred or immediate update
*
******************************************************************************/

static void
gr_chartedit_riscos_setup_update(
    P_GR_CHARTEDITOR  cep,
    flex_ptr pdf_handle,
    _Out_       WimpUpdateWindowBlock * update_window)
{
    P_GR_CHART cp;
    const int dx = wimpt_dx();
    const int dy = wimpt_dy();
    PC_DRAW_FILE_HEADER pDiagHdr;
    DRAW_BOX box;

    assert(pdf_handle && *pdf_handle);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    update_window->window_handle = cep->riscos.window_handle;

    /* always displaying files starting at bottom left */
    /* NB. all are work area offsets (incl,incl,excl,excl) */

    update_window->update_area.xmin = cep->riscos.init_scroll_x;
    /*                    large -ve */
    update_window->update_area.ymin =
    /*                                 small +ve                                  large +ve                   small -ve */
        cp->core.editsave.open.visible_area.ymin - cp->core.editsave.open.visible_area.ymax + cep->riscos.init_scroll_y;

    /* add in the display offset */
    update_window->update_area.xmin += cep->riscos.diagram_off_x;
    update_window->update_area.ymin += cep->riscos.diagram_off_y;

    /* start with null size box and expand sides as appropriate */
    update_window->update_area.xmax = update_window->update_area.xmin;
    update_window->update_area.ymax = update_window->update_area.ymin;

    pDiagHdr = (PC_DRAW_FILE_HEADER) *pdf_handle;

    /* only redraw bboxed area (taking care to expand to encompass pixels!) */
    box = pDiagHdr->bbox;

    gr_box_scale((P_GR_BOX) &box, (P_GR_BOX) &box, NULL, &cep->riscos.scale_from_diag16);

    update_window->update_area.xmin += (int) gr_round_os_to_floor((GR_OSUNIT) box.x0, GR_RISCDRAW_PER_RISCOS * dx) * dx;
    update_window->update_area.ymin += (int) gr_round_os_to_floor((GR_OSUNIT) box.y0, GR_RISCDRAW_PER_RISCOS * dy) * dy;
    update_window->update_area.xmax += (int) gr_round_os_to_ceil( (GR_OSUNIT) box.x1, GR_RISCDRAW_PER_RISCOS * dx) * dx + dx; /* excl */
    update_window->update_area.ymax += (int) gr_round_os_to_ceil( (GR_OSUNIT) box.y1, GR_RISCDRAW_PER_RISCOS * dy) * dy + dy; /* excl */
}

static void
gr_chartedit_riscos_force_redraw(
    P_GR_CHARTEDITOR cep,
    flex_ptr pdf_handle,
    _InVal_     BOOL immediate)
{
    WimpUpdateAndRedrawWindowBlock update_and_redraw_window_block;
    BOOL more;

    assert(cep);

    if(NULL == *pdf_handle)
        return;

    gr_chartedit_riscos_setup_update(cep, pdf_handle, &update_and_redraw_window_block.update);

    if(immediate)
    {
        /* call redraw handler in update loop to get more instant effects */
        if(NULL != WrapOsErrorReporting(tbl_wimp_update_window(&update_and_redraw_window_block.redraw, &more)))
            more = FALSE;

        while(more)
        {
            gr_chartedit_riscos_redraw_core(cep, &update_and_redraw_window_block.redraw);

            if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&update_and_redraw_window_block.redraw, &more)))
               more = FALSE;
        }
    }
    else
    {
        /* queue a redraw for this bit of window */
        void_WrapOsErrorReporting(
            tbl_wimp_force_redraw(update_and_redraw_window_block.update.window_handle,
                                  update_and_redraw_window_block.update.update_area.xmin,
                                  update_and_redraw_window_block.update.update_area.ymin,
                                  update_and_redraw_window_block.update.update_area.xmax,
                                  update_and_redraw_window_block.update.update_area.ymax));
    }
}

static BOOL
gr_chartedit_riscos_Mouse_Click(
    const WimpMouseClickEvent * mouse_click,
    GR_CHARTEDIT_HANDLE ceh)
{
    /* find out what object we clicked on and tell owner */
    P_GR_CHARTEDITOR cep;
    BOOL adjust_clicked;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    gr_chartedit_fontselect_kill(cep);

    /* no end-of-menu event on RISC OS 2.00 means we have to do this ourselves */
    gr_chartedit_selection_clear_if_temp(cep);

    adjust_clicked = mouse_click->buttons & (Wimp_MouseButtonAdjust | (Wimp_MouseButtonAdjust << 8));

    switch(mouse_click->icon_handle)
    {
    case -1:
        /* display section background */
        {
        /* use adjust clicked state to enter deeper into structure than clicked */
        /* i.e. allow selection of point rather than looping back to enclosing series */
        GR_CHART_OBJID id;
        GR_DIAG_OFFSET hitObject[64];
        unsigned int hitIndex;
        GR_POINT point;
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        gr_chartedit_riscos_point_from_abs(cp, &point, mouse_click->mouse_x, mouse_click->mouse_y);

        if(mouse_click->buttons & ((Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust) << 4))
        {
            /* drag */
            GR_POINT workareaoff;

            workareaoff.x = cp->core.editsave.open.scroll_x;
            workareaoff.y = cp->core.editsave.open.scroll_y - BBox_height(&cp->core.editsave.open.visible_area);

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
                                                              point.x, point.y, adjust_clicked);

        if(!adjust_clicked)
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
        if(adjust_clicked)
            gr_chartedit_selection_clear(cep);
        else
        {
            GR_CHART_OBJID id;
            P_GR_CHART cp;

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

    return(TRUE);
}

static BOOL
gr_chartedit_riscos_User_Drag_complete(
    const BBox * const dragboxp,
    GR_CHARTEDIT_HANDLE ceh)
{
    P_GR_CHARTEDITOR cep;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    gr_chartedit_selected_object_drag_complete(cep, dragboxp);

    return(TRUE);
}

static BOOL
gr_chartedit_riscos_Message_DataSave(
    /*acked*/ WimpMessage * const user_message,
    GR_CHARTEDIT_HANDLE ceh)
{
    /* possible object dragged into main window - would normally ask for DataLoad
     * except that we don't actually want anything saving to us that isn't already on disc
     */
    S32 size;
    const FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkimport(&size);

    UNREFERENCED_PARAMETER(ceh);

    if(gr_chart_saving_chart(NULL))
    {
        message_output(string_lookup(create_error(GR_CHART_ERR_NO_SELF_INSERT)));
        return(TRUE);
    }

    switch(filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        messagef(string_lookup(FILE_ERR_ISADIR), user_message->data.data_save.leaf_name);
        break;

    case FILETYPE_PD_CHART:
        break;

    case FILETYPE_PIPEDREAM:
        /* can't detect at this point whether 'tis bog standard or a chart */
        break;

    default:
        if(image_cache_can_import(filetype) || image_convert_can_convert(filetype))
            message_output(string_lookup(create_error(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED)));
        break;
    }

    return(TRUE);
}

static void
gr_chartedit_riscos_Message_DataLoad_to_image_cache(
    /*acked*/ WimpMessage * const user_message,
    GR_CHARTEDIT_HANDLE ceh,
    P_U8 filename)
{
    P_GR_CHARTEDITOR cep;
    P_GR_CHART cp;
    GR_CHART_OBJID id;
    IMAGE_CACHE_HANDLE cah;
    S32 res;

    UNREFERENCED_PARAMETER_InoutRef_(user_message);

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
        return;
    }

    /* load the file, disallowing recolouring */

    res = image_cache_entry_ensure(&cah, filename);

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
}

static BOOL
gr_chartedit_riscos_Message_DataLoad(
    /*acked*/ WimpMessage * const user_message,
    GR_CHARTEDIT_HANDLE ceh)
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
        message_output(string_lookup(GR_CHART_ERR_NO_SELF_INSERT));
        return(TRUE);
    }

    switch(filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        messagef(string_lookup(FILE_ERR_ISADIR), filename);
        break;

    case FILETYPE_PD_CHART:
    case FILETYPE_PIPEDREAM:
        break;

    default:
        if(image_cache_can_import(filetype))
            gr_chartedit_riscos_Message_DataLoad_to_image_cache(user_message, ceh, filename);
        break;
    }

    /* note that this is mandatory */
    xferrecv_insertfileok();

    return(TRUE);
}

static BOOL
gr_chartedit_riscos_User_Message(
    /*acked*/ WimpMessage * const user_message,
    GR_CHARTEDIT_HANDLE ceh)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MDataSave:
        return(gr_chartedit_riscos_Message_DataSave(user_message, ceh));

    case Wimp_MDataLoad:
        return(gr_chartedit_riscos_Message_DataLoad(user_message, ceh));

    default:
        trace_2(TRACE_MODULE_GR_CHART,
                "unprocessed wimp message to chartedit window: %s, ceh &%p",
                report_wimp_message(user_message, FALSE), report_ptr_cast(ceh));
        return(FALSE);
    }
}

#define gr_chartedit_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "ched")

static S32
gr_chartedit_riscos_event_handler(
    wimp_eventstr * e,
    P_ANY handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;
    const GR_CHARTEDIT_HANDLE ceh = handle;

    trace_2(TRACE_MODULE_GR_CHART,
            "gr_chartedit_event_handler: %s, ceh &%p",
            report_wimp_event(event_code, event_data), report_ptr_cast(ceh));

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(gr_chartedit_riscos_Redraw_Window_Request(&event_data->redraw_window_request, ceh));

    case Wimp_EOpenWindow:
        return(gr_chartedit_riscos_Open_Window_Request(&event_data->open_window_request, ceh));

    case Wimp_ECloseWindow:
        return(gr_chartedit_riscos_Close_Window_Request(&event_data->close_window_request, ceh));

    case Wimp_EMouseClick:
        return(gr_chartedit_riscos_Mouse_Click(&event_data->mouse_click, ceh));

    case Wimp_EUserDrag:
        return(gr_chartedit_riscos_User_Drag_complete(&event_data->user_drag_box.bbox, ceh));

    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
    case Wimp_EGainCaret:
    case Wimp_ELoseCaret:
        return(FALSE);

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(gr_chartedit_riscos_User_Message(&event_data->user_message, ceh));

    default:
        trace_2(TRACE_MODULE_GR_CHART,
                "unprocessed wimp event to chartedit window: %s, ceh &%p",
                report_wimp_event(event_code, event_data), report_ptr_cast(ceh));
        return(FALSE);
    }
}

static void
gr_chartedit_riscos_unknown_event_User_Message_previewer(
    wimp_eventstr * e,
    P_ANY           handle)
{
    switch(e->data.msg.hdr.action)
    {
    default:
        break;

    case Wimp_MPaletteChange:
        { /* Redraw our Draw file as colour translation may have changed */
        P_GR_CHARTEDITOR cep = gr_chartedit_cep_from_ceh(handle);
        assert(cep);
        gr_chartedit_diagram_update_later(cep);
        break;
        }
    }
}

static S32
gr_chartedit_riscos_unknown_event_previewer(
    wimp_eventstr * e,
    P_ANY           handle)
{
    switch(e->e)
    {
    default:
        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        gr_chartedit_riscos_unknown_event_User_Message_previewer(e, handle);
        break;
    }

    return(0); /* unclaimed event */
}

#endif /* RISCOS */

/* end of gr_editc.c */
