/* gr_editt.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart editing module part III - the nightname continues */

/* SKS / RCM October 1991 */

#include "common/gflags.h"

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
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

#ifndef __cs_template_h
#include "cs-template.h" /* includes template.h */
#endif

#ifndef __cs_event_h
#include "cs-event.h"   /* includes event.h -> menu.h, wimp.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"    /* includes dbox.h */
#endif

#ifndef __drawmod_h
#include "drawmod.h"
#endif

#ifndef __fontlxtr_h
#include "cmodules/riscos/fontlxtr.h" /* includes fontlist.h */
#endif

#ifndef __fontselect_h
#include "fontselect.h"
#endif

#include "cmodules/mlec.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

/*
callback functions
*/

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_handler);

null_event_proto(static, gr_chartedit_selected_object_drag_null_handler);

/*
internal functions
*/

_Check_return_
static STATUS
gr_chartedit_text_editor_make(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    BOOL submenu);

typedef struct MLSUBMENU_STRUCT * MLSUBMENU_HANDLE;

typedef MLSUBMENU_HANDLE * P_MLSUBMENU_HANDLE;

static void
mlsubmenu_destroy(
    P_MLSUBMENU_HANDLE mlsubmenup);

/******************************************************************************
*
* add in any text objects as a grouped layer
*
******************************************************************************/

_Check_return_
extern STATUS
gr_text_addin(
    P_GR_CHART cp)
{
    struct GR_CHART_TEXT * text = &cp->text;
    GR_CHART_OBJID id;
    GR_DIAG_OFFSET textsStart;
    GR_TEXTSTYLE   textstyle;
    P_GR_TEXT      t;
    LIST_ITEMNO    key;
    P_GR_TEXT_GUTS gutsp;
    STATUS res;
    GR_CHART_VALUE value;
    PC_U8          textsrc;

    status_return(res = gr_chart_group_new(cp, &textsStart, gr_chart_objid_anon));

    for(t = collect_first(GR_TEXT, &text->lbr, &key);
        t;
        t = collect_next( GR_TEXT, &text->lbr, &key))
    {
        /* ignore ones we aren't using */
        if(t->bits.unused)
            continue;

        gr_chart_objid_from_text(key, &id);

        gr_chart_objid_textstyle_query(cp, id, &textstyle);

        gutsp.mp = t + 1;

        if(t->bits.live_text)
        {
            gr_travel_dsp(cp, gutsp.dsp, 0, &value);

            textsrc = value.data.text;
        }
        else
            textsrc = gutsp.textp;

        status_break(res = gr_chart_text_new(cp, id, &t->box, textsrc, &textstyle));

        res = STATUS_DONE;
    }

    gr_chart_group_end(cp, &textsStart);

    return(res);
}

/* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */

static LIST_ITEMNO gr_text_key_to_use = 0;

extern void
gr_chart_text_order_set(
    P_GR_CHART_HANDLE chp /*const*/,
    S32 key)
{
    UNREFERENCED_PARAMETER(chp);

    gr_text_key_to_use = key;
}

extern LIST_ITEMNO
gr_text_key_for_new(
    P_GR_CHART cp)
{
    P_GR_TEXT    t;
    LIST_ITEMNO key;

    /* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */
    if(gr_text_key_to_use != 0)
    {
        key = gr_text_key_to_use;
        gr_text_key_to_use = 0;
        return(key);
    }

    /* add to end of list or reuse dead one */
    key = list_numitem(cp->text.lbr);

    if(key)
    {
        --key;

        if((t = gr_text_search_key(cp, key)) != NULL)
            if(!t || t->bits.unused)
                return(key);
    }

    return(++key);
}

/* create text object near (x, y) */

/*extern*/ const NLISTS_BLK
gr_text_nlists_blk_proto =
{
    NULL,
    sizeof(GR_TEXT) + 1024,
    4096
};

_Check_return_
extern STATUS
gr_text_new(
    P_GR_CHART        cp,
    LIST_ITEMNO      key,
    PC_U8           szText,
    PC_GR_POINT point)
{
    NLISTS_BLK new_lbr = gr_text_nlists_blk_proto;
    U32 nBytes;
    P_GR_TEXT t;
    STATUS status;

    if(szText)
        nBytes = sizeof32(GR_TEXT) + strlen32p1(szText) /*CH_NULL*/;
    else
        nBytes = sizeof32(GR_TEXT) + sizeof32(GR_DATASOURCE);

    new_lbr.lbr = cp->text.lbr;

    if(NULL == (t = collect_add_entry_bytes(GR_TEXT, &new_lbr, nBytes, &key, &status)))
        return(status);

    cp->text.lbr = new_lbr.lbr;

    zero_struct_ptr_fn(t);

    if(point)
    {
        t->box.x0 = point->x;
        t->box.y0 = point->y;
    }
    else
    {
        t->box.x0 = (key * cp->text.style.base.height);
        t->box.y0 = (key * cp->text.style.base.height * 12) / 10;
        t->box.y0 = t->box.y0 % cp->core.layout.height;
        t->box.y0 = cp->core.layout.height - t->box.y0;
    }
    t->box.x1 = t->box.x0 + (szText ? strlen(szText) : 1) * SYSCHARWIDTH_PIXIT;
    t->box.y1 = t->box.y0 + (cp->text.style.base.height * 3) / 2;

    if(szText)
        /* set contents to desired string */
        strcpy((P_U8) (t + 1), szText);
    else
        t->bits.live_text = 1;

    return(1);
}

/******************************************************************************
*
* query/set the position and size of a text object
*
******************************************************************************/

extern void
gr_text_box_query(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    _OutRef_    P_GR_BOX p_box)
{
    P_GR_TEXT t;

    zero_struct_ptr_fn(p_box);

    if((t = gr_text_search_key(cp, key)) != NULL)
        *p_box = t->box;
}

_Check_return_
extern STATUS
gr_text_box_set(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    _InRef_     PC_GR_BOX p_box)
{
    P_GR_TEXT t;

    if((t = gr_text_search_key(cp, key)) != NULL)
    {
        t->box = *p_box;

        gr_box_sort(&t->box, &t->box); /* always ensure sorted */
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* multi line edit control for text objects
*
******************************************************************************/

typedef enum MLSUBMENU_STATES
{
    MLSUBMENU_WAIT = 0,
    MLSUBMENU_ENDED_CANCEL,
    MLSUBMENU_ENDED_OK
}
MLSUBMENU_STATES;

typedef struct MLSUBMENU_STRUCT
{
    MLEC_HANDLE mlec;

    HOST_WND mls_pane_window_handle;
    WimpWindowWithBitset * mls_pane_template;

    MLSUBMENU_STATES state;
}
MLSUBMENU_STRUCT;

#define MLSUBMENU_BUTTON_OK      (1)
#define MLSUBMENU_BUTTON_CANCEL  (2)
#define MLSUBMENU_BUTTON_NEWLINE (3)

/******************************************************************************
*
* Create and open a submenu containing a multiline text editor
*
******************************************************************************/

_Check_return_
static STATUS
mlsubmenu_create(
    P_MLSUBMENU_HANDLE mlsubmenup,
    P_U8Z template_name,
    P_U8Z title)
{
    STATUS           res;
    int              err = 0;
    MLSUBMENU_HANDLE mlsubmenu;
    MLEC_HANDLE      mlec;
    template *       template_handle_pane;
    WimpWindowWithBitset * template_pane;
    HOST_WND         mls_pane_win_handle;
    BBox             initial_extent; /* work area extent */

    trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - in");

    if(NULL == (*mlsubmenup = mlsubmenu = al_ptr_calloc_elem(struct MLSUBMENU_STRUCT, 1, &res)))
        return(res);

    mlsubmenu->state = MLSUBMENU_ENDED_CANCEL;

    trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - allocated block OK");

    if((err = mlec_create(&mlec)) >= 0)
    {
        trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - mlec_create OK");

        mlsubmenu->mlec = mlec;

        template_handle_pane = template_find_new(template_name);

        if(template_handle_pane)
        {
            trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - template_find OK");

            mlsubmenu->mls_pane_template = template_pane = template_copy_new(template_handle_pane);

            if(template_pane)
            {
                os_error * e;

                initial_extent = template_pane->extent;

                trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - template_copy OK");

                e = tbl_wimp_create_window((WimpWindow *) template_pane, &mls_pane_win_handle);

                if(NULL == e)
                {
                    /* Created window */

                    trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_create - wimp_create_wind OK");

                    mlsubmenu->mls_pane_window_handle = mls_pane_win_handle;

                    win_settitle(mlsubmenu->mls_pane_window_handle, title);

                    mlec_attach(mlec, HOST_WND_NONE, mlsubmenu->mls_pane_window_handle, &initial_extent, NULL); /* NB no menu on the window */

                    mlsubmenu->state = MLSUBMENU_WAIT;

                    mlec_attach_eventhandler(mlec, mlsubmenu_mlec_event_handler, mlsubmenu, TRUE);

                    return(0);
                }
              /*else                         */
              /*    no room to create window */
            }
          /*else                         */
          /*    no room to copy template */
        }
      /*else                        */
      /*    failed to find template */
    }
  /*else                          */
  /*    mlec_create gave an error */

    mlsubmenu_destroy(mlsubmenup);      /* destroys as much as we created */

    *mlsubmenup = NULL;

    if(err >= 0)
        err = status_nomem();

    return(err);
}

/******************************************************************************
*
* Returns
*  > 0  Successful edit, user OKed it by pressing RETURN
*  = 0  Failed edit, user canceled by ESCAPE, CLOSE or click outside window
*  < 0  Error
*
******************************************************************************/

_Check_return_
static S32
mlsubmenu_process(
    P_MLSUBMENU_HANDLE mlsubmenup,
    BOOL submenu)
{
    MLSUBMENU_HANDLE mlsubmenu = * mlsubmenup;

    if(submenu)
    {
        int x, y;

        event_read_submenupos(&x, &y);

        trace_2(TRACE_MODULE_GR_CHART, "mlsubmenu_process, opening submenu at (%d,%d)", x, y);

        winx_create_submenu(mlsubmenu->mls_pane_window_handle, x, y);
    }
    else
    {
        wimp_mousestr m;

        if(NULL != WrapOsErrorReporting(wimp_get_point_info(&m)))
        {
            m.x = m.y = 0;
        }
        else
        {
            m.x -= 32; /* try to be a bit into the window */
            m.y += 32;
        }

        trace_2(TRACE_MODULE_GR_CHART, "mlsubmenu_process, opening menu at (%d,%d)", m.x, m.y);

        winx_create_menu(mlsubmenu->mls_pane_window_handle, m.x, m.y);
    }

    mlec_claim_focus(mlsubmenu->mlec);

    /* wait for editing to complete, user process will receive upcalls from wm_events_get() */
    while(mlsubmenu->state == MLSUBMENU_WAIT)
    {
#if FALSE
        if(!mlsubmenu_check_open(   )
            mlsubmenu->state = MLSUBMENU_ENDED_CANCEL;
        else
#endif
            wm_events_get(TRUE);
    }

    trace_0(TRACE_MODULE_GR_CHART, "mlsubmenu_process - out");
    return(mlsubmenu->state == MLSUBMENU_ENDED_OK);
}

_Check_return_
static STATUS
mlsubmenu_settext(
    P_MLSUBMENU_HANDLE mlsubmenup,
    P_U8 text)
{
    return(mlec_SetText((*mlsubmenup)->mlec, text));
}

_Check_return_
static STATUS
mlsubmenu_gettext(
    P_MLSUBMENU_HANDLE mlsubmenup,
    P_U8 destptr,
    int destsize)
{
    return(mlec_GetText((*mlsubmenup)->mlec, destptr, destsize));
}

_Check_return_
static S32
mlsubmenu_gettextlen(
    P_MLSUBMENU_HANDLE mlsubmenup)
{
    return(mlec_GetTextLen((*mlsubmenup)->mlec));
}

static void
mlsubmenu_destroy(
    P_MLSUBMENU_HANDLE mlsubmenup)
{
    if(*mlsubmenup)
    {
        if((*mlsubmenup)->mlec)
        {
            mlec_detach((*mlsubmenup)->mlec);
            mlec_destroy(&((*mlsubmenup)->mlec));
        }

        if(HOST_WND_NONE != (*mlsubmenup)->mls_pane_window_handle)
        {
            WimpDeleteWindowBlock delete_window_block;
            delete_window_block.window_handle = (*mlsubmenup)->mls_pane_window_handle;
            void_WrapOsErrorReporting(tbl_wimp_delete_window(&delete_window_block));
        }

        template_copy_dispose(&(*mlsubmenup)->mls_pane_template);

        al_ptr_dispose(P_P_ANY_PEDANTIC(mlsubmenup));
    }
}

/******************************************************************************
*
* callback function from mlec
*
******************************************************************************/

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_IsClose)
{
    MLSUBMENU_HANDLE mlsubmenu = handle;

    UNREFERENCED_PARAMETER(rc);
    UNREFERENCED_PARAMETER(p_eventdata);

    winx_close_window(mlsubmenu->mls_pane_window_handle);

    mlsubmenu->state = MLSUBMENU_ENDED_CANCEL;

    return(mlec_event_closed);
}

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_IsEscape)
{
    MLSUBMENU_HANDLE mlsubmenu = handle;

    UNREFERENCED_PARAMETER(rc);
    UNREFERENCED_PARAMETER(p_eventdata);

    /* being a submenu, we probably never receive this, the code in win (or the wimp) sends us a close instead */
    winx_close_window(mlsubmenu->mls_pane_window_handle); /* close window and rest of menu-tree */

    mlsubmenu->state = MLSUBMENU_ENDED_CANCEL;

    return(mlec_event_escape);
}

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_IsReturn)
{
    MLSUBMENU_HANDLE mlsubmenu = handle;

    UNREFERENCED_PARAMETER(rc);
    UNREFERENCED_PARAMETER(p_eventdata);

    winx_close_window(mlsubmenu->mls_pane_window_handle); /* close window and rest of menu-tree */

    mlsubmenu->state = MLSUBMENU_ENDED_OK;

    return(mlec_event_return);
}

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_IsClick)
{
    MLSUBMENU_HANDLE mlsubmenu = handle;

    const WimpMouseClickEvent * const mouse_click = (const WimpMouseClickEvent *) p_eventdata;

    UNREFERENCED_PARAMETER(rc);

    if(mouse_click->buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust)) /* 'Select' or 'Adjust' */
    {
        switch(mouse_click->icon_handle)
        {
        case MLSUBMENU_BUTTON_OK:
            winx_close_window(mlsubmenu->mls_pane_window_handle); /* close window and rest of menu-tree */
            mlsubmenu->state = MLSUBMENU_ENDED_OK;
            return(mlec_event_click);

        case MLSUBMENU_BUTTON_CANCEL:
            winx_close_window(mlsubmenu->mls_pane_window_handle);
            mlsubmenu->state = MLSUBMENU_ENDED_CANCEL;
            return(mlec_event_click);

        case MLSUBMENU_BUTTON_NEWLINE:
            mlec__insert_newline(mlsubmenu->mlec);
            return(mlec_event_click);
        }
    }

    return(mlec_event_unknown);
}

#if FALSE
MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_IsOpen)
{
    wimp__openstr * panep = (wimp_openstr *) p_eventdata;
    wimp__openstr   main;

    wimp_open_wind(data);       /* open pane, so scrolling works */

    main.w = mlsubmenu->mls_main_window_handle;
    main.visible_area.x0 = panep->visible_area.x0 - mlsubmenu->margin.x0;
    main.visible_area.x1 = panep->visible_area.x1 - mlsubmenu->margin.x1;
    main.visible_area.y0 = panep->visible_area.y0 - mlsubmenu->margin.y0;
    main.visible_area.y1 = panep->visible_area.y1 - mlsubmenu->margin.y1;
    main.xscroll = 0;
    main.yscroll = 0;
    main.behind = panep->behind;

    mlsubmenu_open_window(mlsubmenu, &main);

    return(mlec_event_opened);
}
#endif

MLEC_EVENT_PROTO(static, mlsubmenu_mlec_event_handler)
{
    trace_1(TRACE_MODULE_GR_CHART, "mlsubmenu_mlec_event_handler, rc=%d", rc);

    switch(rc)
    {
    case Mlec_IsClose:
        return(mlsubmenu_mlec_event_IsClose(rc, handle, p_eventdata));

    case Mlec_IsEscape:
        return(mlsubmenu_mlec_event_IsEscape(rc, handle, p_eventdata));

    case Mlec_IsReturn:
        return(mlsubmenu_mlec_event_IsReturn(rc, handle, p_eventdata));

    case Mlec_IsClick:
        return(mlsubmenu_mlec_event_IsClick(rc, handle, p_eventdata));

    default:
        return(mlec_event_unknown);
    }
}

/******************************************************************************
*
* fill style dialog processing
*
******************************************************************************/

typedef struct FILLSTYLE_ENTRY
{
    IMAGE_CACHE_HANDLE cah;
    char leafname[BUF_MAX_PATHSTRING]; /* SKS 26oct96 now cater for long leafnames (was BUF_MAX_LEAFNAME) */
}
FILLSTYLE_ENTRY, * P_FILLSTYLE_ENTRY; typedef const FILLSTYLE_ENTRY * PC_FILLSTYLE_ENTRY;

static NLISTS_BLK fillstyle_list =
{
    NULL,
    sizeof(FILLSTYLE_ENTRY),
    sizeof(FILLSTYLE_ENTRY) * 16
};

#define fillstyle_list_search_key(key) \
    collect_goto_item(FILLSTYLE_ENTRY, &fillstyle_list.lbr, (key))

static GR_FILL_PATTERN_HANDLE
gr_chartedit_riscos_fillstyle_pattern_query(
    const LIST_ITEMNO new_fillstyle_key)
{
    PC_FILLSTYLE_ENTRY entryp;

    if((entryp = fillstyle_list_search_key(new_fillstyle_key)) != NULL)
        return((GR_FILL_PATTERN_HANDLE) entryp->cah);

    return(GR_FILL_PATTERN_NONE);
}

static void
gr_chartedit_encode_selected_fillstyle(
    P_GR_CHARTEDITOR cep,
    _HwndRef_   HOST_WND window_handle,
    PC_GR_FILLSTYLE fillstyle,
    _InVal_     LIST_ITEMNO fillstyle_key)
{
    /* encode icons from current state (ok, ought to
     * use tristates but RISC OS doesn't have the concept)
    */
    P_GR_CHART cp;
    PC_FILLSTYLE_ENTRY entryp;
    const char * leafname = "";
    void (* fadeproc) (_HwndRef_ HOST_WND window_handle, _InVal_ int icon_handle);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    if((entryp = fillstyle_list_search_key(fillstyle_key)) != NULL)
        leafname = entryp->leafname;

    winf_setfield(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME, leafname);

    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_SOLID,
                    (fillstyle->pattern == GR_FILL_PATTERN_NONE) || !fillstyle->bits.notsolid);
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN,
                    (fillstyle->pattern != GR_FILL_PATTERN_NONE) && fillstyle->bits.pattern);

    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_ISOTROPIC, fillstyle->bits.isotropic);
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_RECOLOUR,  !fillstyle->bits.norecolour);

    /* fade out picture icons once picture fill deselected */
    fadeproc = !winf_getonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN) ? winf_fadefield : winf_unfadefield;

    (* fadeproc) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME);
    (* fadeproc) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_INC);
    (* fadeproc) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_DEC);
    (* fadeproc) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_ISOTROPIC);
    (* fadeproc) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_RECOLOUR);
}

static void
gr_chartedit_build_fillstyle_list(
    P_GR_CHARTEDITOR cep,
    _InVal_     GR_CHART_OBJID id)
{
    P_GR_CHART       cp;
    GR_FILLSTYLE     fillstyle;
    PC_U8            pict_dir;
    IMAGE_CACHE_HANDLE current_cah;
    LIST_ITEMNO      current_key;
    U8               combined_path[BUF_MAX_PATHSTRING * 4];
    U8               fullname[BUF_MAX_PATHSTRING];
    P_FILE_OBJENUM   enumstrp;
    P_FILE_OBJINFO   infostrp;
    P_FILLSTYLE_ENTRY entryp;
    S32              res;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_fillstyle_query(cp, id, &fillstyle);

    current_cah = (IMAGE_CACHE_HANDLE) fillstyle.pattern;
    current_key = -1;

    /* throw away any existing fillstyle_list, then (re)build it */
    collect_delete(&fillstyle_list.lbr);

    /* enumerate Markers or Pictures into a list and image_cache_ensure_entry each of them */
    switch(id.name)
    {
    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
        {
        GR_SERIES_IDX series_idx = gr_series_idx_from_external(cp, id.no);
        PC_GR_AXES p_axes = gr_axesp_from_series_idx(cp, series_idx);

        if( (p_axes->charttype == GR_CHARTTYPE_LINE) ||
            (p_axes->charttype == GR_CHARTTYPE_SCAT) )
        {
            P_GR_SERIES serp = getserp(cp, series_idx);
            GR_CHARTTYPE charttype = serp->charttype;

            if( (charttype == GR_CHARTTYPE_NONE) ||
                (charttype == p_axes->charttype)  )
            {
                pict_dir = "Markers";
                break;
            }
        }
        }

    /* else deliberate drop thru ... */

    default:
        pict_dir = "Pictures";
        break;
    }

    /* enumerate all ***files*** in subdirectory pict_dir found in the
     * directories relative to the chart or listed in the applications path variable
    */

    file_combine_path(combined_path, elemof32(combined_path),
                      file_is_rooted(cp->core.currentfilename)
                                   ? cp->core.currentfilename
                                   : NULL,
                      file_get_search_path());

    trace_2(TRACE_MODULE_GR_CHART, "path=%u:'%s'", strlen32(combined_path), combined_path);

    for(infostrp = file_find_first_subdir(&enumstrp, combined_path, FILE_WILD_MULTIPLE_STR, pict_dir);
        infostrp;
        infostrp = file_find_next(&enumstrp))
    {
        if(file_objinfo_type(infostrp) == FILE_OBJECT_FILE)
        {
            U8 leafname[BUF_MAX_PATHSTRING]; /* SKS 26oct96 cater for long leafnames (was MAX_LEAFNAME) */
            IMAGE_CACHE_HANDLE new_cah;
            LIST_ITEMNO new_key;

            file_objinfo_name(infostrp, leafname, elemof32(leafname));

            file_find_query_dirname(&enumstrp, fullname, elemof32(fullname));

            xstrkat(fullname, elemof32(fullname), FILE_DIR_SEP_STR);
            xstrkat(fullname, elemof32(fullname), leafname);

            trace_2(TRACE_MODULE_GR_CHART, "fullname is %u:'%s'", strlen32(fullname), fullname);

            res = image_cache_entry_ensure(&new_cah, fullname);   /* Tutu said to ignore errors */

            /* create a list entry (add to end), returns a key */
            new_key = -1;

            if(NULL != (entryp = collect_add_entry_elem(FILLSTYLE_ENTRY, &fillstyle_list, &new_key, &res)))
            {
                /* note key if we added current picture to list. doesn't matter
                 * about matching IMAGE_CACHE_HANDLE_NONE as that's trapped below
                */
                if(current_cah == new_cah)
                    current_key = new_key;

                entryp->cah = new_cah;
                xstrkpy(entryp->leafname, elemof32(entryp->leafname), leafname);
            }
        }
    }

    file_find_close(&enumstrp);     /* Not really needed, done by file_find_next in this case */

    /* if the current fillstyle pattern is not already there then insert at front of list */
    if((current_cah != IMAGE_CACHE_HANDLE_NONE) && (current_key == -1))
    {
        image_cache_name_query(&current_cah, fullname, sizeof(fullname)-1);

        current_key = 0;

        if(NULL != (entryp = collect_insert_entry_elem(FILLSTYLE_ENTRY, &fillstyle_list, current_key, &res)))
        {
            entryp->cah = current_cah;
            strcpy(entryp->leafname, file_leafname(fullname));
        }

        status_assert(res);
    }
}

_Check_return_
static inline _kernel_oserror *
gr_editm_colourtrans_SetGCOL(
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

static void
fillstyle_redraw_core(
    IMAGE_CACHE_HANDLE cah,
    const WimpRedrawWindowBlock * const redraw_window_block,
    const BBox * const p_picture_bbox)
{
    WimpRedrawWindowBlock picture_redraw_window_block = *redraw_window_block;
    P_DRAW_DIAG p_draw_diag;

    GDI_POINT gdi_org;

    const int dx = wimpt_dx();
    const int dy = wimpt_dy();

    /* origin (NOT top left) of Draw file taken by renderer to be at x0,y1 (abs screen coords) */
    int xmin;
    int ymax;

    gdi_org.x = redraw_window_block->visible_area.xmin - redraw_window_block->xscroll;
    gdi_org.y = redraw_window_block->visible_area.ymax - redraw_window_block->yscroll;

    xmin = p_picture_bbox->xmin + gdi_org.x;
    ymax = p_picture_bbox->ymax + gdi_org.y;

    /* ensure clipped to 'picture box' */
    {
    BBox picture_bbox; /* (abs screen coords) */

    picture_bbox.xmin = p_picture_bbox->xmin + gdi_org.x;
    picture_bbox.ymin = p_picture_bbox->ymin + gdi_org.y;
    picture_bbox.xmax = p_picture_bbox->xmax + gdi_org.x;
    picture_bbox.ymax = p_picture_bbox->ymax + gdi_org.y;

    if(!gr_box_intersection((P_GR_BOX) &picture_redraw_window_block.redraw_area, NULL, (P_GR_BOX) &picture_bbox))
        return;
    }

    /* set our own graphics window */
    wimpt_safe(riscos_vdu_define_graphics_window(picture_redraw_window_block.redraw_area.xmin,
                                                 picture_redraw_window_block.redraw_area.ymin,
                                                 picture_redraw_window_block.redraw_area.xmax - dx,
                                                 picture_redraw_window_block.redraw_area.ymax - dy));

    /* clear background to white */
    gr_editm_colourtrans_SetGCOL(0xFFFFFF00, 0x80, 0);
    wimpt_safe(bbc_vdu(bbc_ClearGraph));

    p_draw_diag = image_cache_loaded_ensure(&cah);

    if(p_draw_diag)
    {
        const int icon_wid = BBox_width( p_picture_bbox);
        const int icon_hei = BBox_height(p_picture_bbox);
        DRAW_BOX draw_bound = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
        S32 diag_wid = (draw_bound.x1 - draw_bound.x0) >> 8; /* Draw -> OS units */
        S32 diag_hei = (draw_bound.y1 - draw_bound.y0) >> 8;
        S32 scaled_diag_wid, scaled_diag_hei;
        S32 xshift = 0;
        S32 yshift = 0;
        F64 scaleX, scaleY, scale;

        diag_wid = MAX(diag_wid, 16);
        diag_hei = MAX(diag_hei, 16);

        scaleX = (F64) icon_wid / (F64) diag_wid;
        scaleY = (F64) icon_hei / (F64) diag_hei;

        scale = MIN(scaleX, scaleY);

        scaled_diag_wid = (S32) (diag_wid * scale);
        scaled_diag_hei = (S32) (diag_hei * scale);

        if(scaled_diag_wid < icon_wid)
            xshift = (((icon_wid - scaled_diag_wid) / 2) + (dx-1)) & ~(dx-1); /* round to pixels */
        if(scaled_diag_hei < icon_hei)
            yshift = (((icon_hei - scaled_diag_hei) / 2) + (dy-1)) & ~(dy-1);

        status_assert(draw_do_render(p_draw_diag->data, p_draw_diag->length,
                                     xmin + xshift, ymax + yshift - icon_hei, /* D.Elworthy, your're a twat! */
                                     scale, scale,
                                     (P_GDI_BOX) &picture_redraw_window_block.redraw_area));
    }

    /* restore caller's graphics window */
    wimpt_safe(riscos_vdu_define_graphics_window(redraw_window_block->redraw_area.xmin,
                                                 redraw_window_block->redraw_area.ymin,
                                                 redraw_window_block->redraw_area.xmax - dx,
                                                 redraw_window_block->redraw_area.ymax - dy));
}

static BOOL
fillstyle_event_Redraw_Window_Request(
    const WimpRedrawWindowRequestEvent * const redraw_window_request,
    P_ANY handle)
{
    LIST_ITEMNO * keyp = handle;
    PC_FILLSTYLE_ENTRY entryp;
    IMAGE_CACHE_HANDLE cah;
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;

    redraw_window_block.window_handle = redraw_window_request->window_handle;

    cah = 0;

    if((entryp = fillstyle_list_search_key(*keyp)) != NULL)
        cah = entryp->cah;

    /* only redrawing required is of the 'picture' (a draw file identified by cah), */
    /* which must be scaled to fit within the limits of its icon                    */
    if(NULL != WrapOsErrorReporting(tbl_wimp_redraw_window(&redraw_window_block, &more)))
        more = FALSE;

    while(more)
    {
        /* get the window relative bbox of the picture icon */
        WimpGetIconStateBlock icon_state;
        icon_state.window_handle = redraw_window_block.window_handle;
        icon_state.icon_handle = GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_PICT;
        if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        {
            more = FALSE;
            break;
        }

        fillstyle_redraw_core(cah, &redraw_window_block, &icon_state.icon.bbox);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&redraw_window_block, &more)))
            more = FALSE;
    }

    return(TRUE);
}

static BOOL
fillstyle_raw_event_handler(
    dbox d,
    void * event,
    void * handle)
{
    const int event_code = ((WimpEvent *) event)->event_code;
    WimpPollBlock * const event_data = &((WimpEvent *) event)->event_data;

    UNREFERENCED_PARAMETER(d);

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(fillstyle_event_Redraw_Window_Request(&event_data->redraw_window_request, handle));

    default:
        return(FALSE);
    }
}

extern S32
gr_chartedit_selection_fillstyle_edit(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    BOOL ok, persist, reflect_modify;
    P_GR_CHART cp;
    GR_FILLSTYLE fillstyle;
    LIST_ITEMNO fillstyle_key;
    PC_FILLSTYLE_ENTRY entryp;
    S32 disallow_piccie = 0;
    S32 res = 1;

    assert(cep);

    d = dbox_new_new(GR_CHARTEDIT_TEM_FILLSTYLE, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return(0);
    }

    dbox_show(d);

    window_handle = dbox_window_handle(d);

    {
    U8   title[BUF_MAX_GR_CHART_OBJID_REPR + 32];
    S32 appendage = GR_CHART_MSG_EDIT_APPEND_FILL;

    switch(cep->selection.id.name)
    {
    case GR_CHART_OBJNAME_PLOTAREA:
        if(cep->selection.id.no)
            disallow_piccie = 1;
        else
            /* 'area area' sounds silly */
            appendage = 0;
        break;

    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_LEGEND:
        appendage = GR_CHART_MSG_EDIT_APPEND_AREA;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), cep->selection.id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_STYLE));

    win_settitle(window_handle, title);
    }

    (disallow_piccie ? winf_fadefield : winf_unfadefield) (window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN);

    if(!disallow_piccie)
    {
        gr_chartedit_build_fillstyle_list(cep, cep->selection.id);

        dbox_raw_eventhandler(d, fillstyle_raw_event_handler, &fillstyle_key);
    }

    reflect_modify = 0;

    do  {
        /* load chart structure up to local structure */
        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);
        gr_chart_objid_fillstyle_query(cp, cep->selection.id, &fillstyle);

        {
        LIST_ITEMNO key;

        fillstyle_key = 0;

        for(entryp = collect_first(FILLSTYLE_ENTRY, &fillstyle_list.lbr, &key);
            entryp;
            entryp = collect_next( FILLSTYLE_ENTRY, &fillstyle_list.lbr, &key))
            if((IMAGE_CACHE_HANDLE) fillstyle.pattern == entryp->cah)
            {
                fillstyle_key = key;
                break;
            }
        } /*block*/

        for(;;)
        {
            gr_chartedit_encode_selected_fillstyle(cep, window_handle, &fillstyle, fillstyle_key);

            res = 1;

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_FILLSTYLE_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            if(f != dbox_OK)
            {
                const int hit_icon_handle = dbox_field_to_icon_handle(d, f);

                switch(hit_icon_handle)
                {
                case GR_CHARTEDIT_TEM_FILLSTYLE_ICON_SOLID:
                    fillstyle.bits.notsolid = !winf_getonoff(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN:
                    fillstyle.bits.pattern = winf_getonoff(window_handle, hit_icon_handle);

                    fillstyle.pattern = fillstyle.bits.pattern
                                                ? gr_chartedit_riscos_fillstyle_pattern_query(fillstyle_key)
                                                : GR_FILL_PATTERN_NONE;
                    break;

                case GR_CHARTEDIT_TEM_FILLSTYLE_ICON_ISOTROPIC:
                    fillstyle.bits.isotropic = winf_getonoff(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_FILLSTYLE_ICON_RECOLOUR:
                    fillstyle.bits.norecolour = !winf_getonoff(window_handle, hit_icon_handle);
                    break;

                default:
                    if(winf_adjustbumphit(&f, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME)) /* inverts dirn. for click */
                    {                                                                     /* with adjust button      */
                        LIST_ITEMNO last_key;
                        const char * leafname;

                        last_key = list_numitem(fillstyle_list.lbr);
                        if(!last_key)
                            break;

                        if(f == GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_INC)
                            entryp = collect_next(FILLSTYLE_ENTRY, &fillstyle_list.lbr, &fillstyle_key);
                        else
                            entryp = collect_prev(FILLSTYLE_ENTRY, &fillstyle_list.lbr, &fillstyle_key);

                        if(NULL == entryp)
                        {
                            /* either: 'next' whilst showing last item or 'prev' whilst showing first item, */
                            /*         so wrap to other end of list                                         */
                            fillstyle_key = (f == GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_INC ? 0 : last_key - 1);

                            if((entryp = fillstyle_list_search_key(fillstyle_key)) == NULL)
                            {
                                fillstyle_key = 0;
                                assert(0);  /* SKS is more paranoid */
                                break;      /* not found - will never happen! (or so says RCM) */
                            }
                        }

                        /* show the picture's leafname, trigger a (later) redraw of picture icon */
                        leafname = entryp->leafname;

                        winf_setfield(    window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME, leafname);
                        winf_changedfield(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_PICT);
                    }
                    break;
                }
            }

            /* check what the current pattern is set to */
            fillstyle.bits.pattern = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN);

            fillstyle.pattern = fillstyle.bits.pattern
                                        ? gr_chartedit_riscos_fillstyle_pattern_query(fillstyle_key)
                                        : GR_FILL_PATTERN_NONE;

            if(f == dbox_OK)
            {
                fillstyle.fg.manual = 1;

                res = gr_chart_objid_fillstyle_set(cp, cep->selection.id, &fillstyle);

                if(status_fail(res))
                    gr_chartedit_winge(res);

                gr_chart_modify_and_rebuild(&cep->ch);

                if(res < 0)
                    f = dbox_CLOSE;

                break;
            }
        }

        ok = (f == dbox_OK);

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    dbox_dispose(&d);

    return(res);
}

/******************************************************************************
*
* use Acorn-supplied FontSelector code to select a text style for objects
*
******************************************************************************/

typedef struct GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO
{
    GR_CHARTEDIT_HANDLE ceh;
    GR_CHART_OBJID      id;

    GR_TEXTSTYLE        style;
}
GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO, * P_GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO;

extern void
gr_chartedit_fontselect_kill(
    P_GR_CHARTEDITOR cep)
{
    if(cep)
        cep->riscos.processing_fontselector = 0;

    fontselect_closewindows();
}

static void
gr_chartedit_fontselect_try_me(
    PC_U8 font_name,
    _InVal_     F64 width,
    _InVal_     F64 height,
    P_GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO i)
{
    P_GR_CHARTEDITOR cep;
    S32             modify = 0;
    P_GR_TEXTSTYLE   cur_style = &i->style;
    GR_TEXTSTYLE    new_style = *cur_style;

    cep = gr_chartedit_cep_from_ceh(i->ceh);
    /* note that chart editor may have gone away during FontSelector processing! */
    if(!cep)
        return;

    zero_struct_fn(new_style.szFontName);
    xstrkpy(new_style.szFontName, sizeof32(new_style.szFontName), font_name);

    new_style.width  = (GR_COORD) (width  * GR_PIXITS_PER_POINT);
    new_style.height = (GR_COORD) (height * GR_PIXITS_PER_POINT);

    modify = memcmp(&new_style, cur_style, sizeof(new_style));

    if(modify)
    {
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        new_style.fg.manual = 1;
        *cur_style = new_style;
        gr_chartedit_winge(gr_chart_objid_textstyle_set(cp, i->id, cur_style));

        gr_chart_modify_and_rebuild(&cep->ch);
    }
}

static BOOL
gr_chartedit_fontselect_unknown_fn(
    const char * font_name,
    _InVal_     F64 width,
    _InVal_     F64 height,
    const wimp_eventstr * e,
    P_ANY try_handle,
    BOOL try_anyway)
{
    P_GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO i = try_handle;
    BOOL close_windows = FALSE;

    if(try_anyway)
        gr_chartedit_fontselect_try_me(font_name, width, height, i);
    else
        switch(e->e)
        {
        default:
            break;
        }

    return(close_windows);
}

extern S32
gr_chartedit_selection_textstyle_edit(
    P_GR_CHARTEDITOR cep)
{
    GR_CHARTEDIT_RISCOS_FONTSELECT_CALLBACK_INFO i;
    F64 width, height;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    /* can have only one font selector per program */
    if(!fontselect_prepare_process())
    {
        /* that ***nasty*** piece of code relies on the first level monitoring of
         * the state of the font selector returning without further ado to its caller
         * and several milliseconds later the Window Manager will issue yet another
         * submenu open request. or so we hope. if the punter is miles too quick,
         * it will merely close the old font selector and not open a new one, so
         * he'll have to go and move over the right arrow again!
        */
        return(0);
    }

    event_clear_current_menu();

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;

    {
    S32 appendage = 0;

    switch(i.id.name)
    {
    case GR_CHART_OBJNAME_PLOTAREA:
        i.id = gr_chart_objid_chart;

        /* deliberate drop thru ... */

    case GR_CHART_OBJNAME_CHART:
        appendage = GR_CHART_MSG_EDIT_APPEND_BASE;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), i.id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    switch(i.id.name)
    {
    case GR_CHART_OBJNAME_TEXT:
        appendage = GR_CHART_MSG_EDIT_APPEND_STYLE;
        break;

    default:
        appendage = GR_CHART_MSG_EDIT_APPEND_TEXTSTYLE;
        break;
    }

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_textstyle_query(cp, i.id, &i.style);
    }

    width  = i.style.width  / (F64) GR_PIXITS_PER_POINT;
    height = i.style.height / (F64) GR_PIXITS_PER_POINT;

    cep->riscos.processing_fontselector = 1;

    fontselect_process(title,                                    /* win title   */
                       fontselect_SETFONT | fontselect_SETTITLE, /* flags       */
                       i.style.szFontName, /* font name */
                       width, /* font width */
                       height, /* font height */
                       NULL, NULL,
                       gr_chartedit_fontselect_unknown_fn, &i);

    /* we may be dead */
    cep = gr_chartedit_cep_from_ceh(i.ceh);
    if(cep)
        cep->riscos.processing_fontselector = 0;

    return(0);
}

/******************************************************************************
*
* remove the selected text and its attributes
*
******************************************************************************/

extern void
gr_chartedit_selection_text_delete(
    P_GR_CHARTEDITOR cep)
{
    P_GR_CHART cp;

    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    gr_text_delete(cp, cep->selection.id.no);
}

extern void
gr_text_delete(
    P_GR_CHART cp,
    LIST_ITEMNO key)
{
    P_GR_CHARTEDITOR cep;
    P_GR_TEXT        t;
    P_GR_TEXT_GUTS   gutsp;

    t = gr_text_search_key(cp, key);
    assert(t);

    if(t->bits.live_text)
    {
        /* kill client dep unless it was him who killed us */
        gutsp.mp = t + 1;
        if(gutsp.dsp->dsh != GR_DATASOURCE_HANDLE_NONE)
            gr_travel_dsp(cp, gutsp.dsp, 0, NULL);
    }

    if(cp->core.ceh)
    {
        cep = gr_chartedit_cep_from_ceh(cp->core.ceh);
        if(cep->selection.id.name == GR_CHART_OBJNAME_TEXT)
            if(cep->selection.id.no == key)
                gr_chartedit_selection_clear(cep);
    }

    /* don't change key numbering of subsequent entries in either list */
    collect_subtract_entry(&cp->text.lbr,       key);
    collect_subtract_entry(&cp->text.style.lbr, key);
}

enum GR_OBJECT_DRAG_TYPE
{
    GR_OBJECT_DRAG_REPOSITION = 0,
    GR_OBJECT_DRAG_RESIZE_TL  = 1,
    GR_OBJECT_DRAG_RESIZE_BR  = 2,
};

static GR_POINT drag_start_point;
static GR_POINT drag_curr_point;
static enum GR_OBJECT_DRAG_TYPE drag_type;

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Rectangle outline                                                         */
/*                                                                           */
/* tests for height or width zero, to prevent double plotting                */
/*                                                                           */

static void
displ_box(
    int x0,
    int y0,
    int x1,
    int y1)
{
    wimpt_safe(bbc_move(x0, y0));

    if((x0 == x1) || (y0 == y1))
        wimpt_safe(bbc_draw(x1, y1));
    else
    {
        wimpt_safe(os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x1, y0));
        wimpt_safe(os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x1, y1));
        wimpt_safe(os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x0, y1));
        wimpt_safe(os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x0, y0));
    }
}

static void
object_dragging_eor_bbox(
    P_GR_CHARTEDITOR cep,
    P_GR_CHART cp,
    _InRef_     PC_GR_POINT start_point,
    _InRef_     PC_GR_POINT curr_point)
{
    GR_POINT moveby;
    GR_POINT tl, br;
    GDI_POINT gdi_org;
    BBox gdi_outline;

    moveby.x = curr_point->x - start_point->x;
    moveby.y = curr_point->y - start_point->y;

    tl.x = cep->selection.box.x0;
    tl.y = cep->selection.box.y1;

    br.x = cep->selection.box.x1;
    br.y = cep->selection.box.y0;

    switch(drag_type)
    {
    default: default_unhandled();
    case GR_OBJECT_DRAG_REPOSITION:
        /* reposition adjusts all coords */
        tl.x += moveby.x;
        tl.y += moveby.y;
        br.x += moveby.x;
        br.y += moveby.y;
        break;

    case GR_OBJECT_DRAG_RESIZE_TL:
        /* resize TL adjusts (x0, y1) only */
        tl.x += moveby.x;
        tl.y += moveby.y;
        break;

    case GR_OBJECT_DRAG_RESIZE_BR:
        /* resize BR adjusts (x1, y0) only */
        br.x += moveby.x;
        br.y += moveby.y;
        break;
    }

    /* scale by chart zoom factor (pixit->pixit) */
    gr_point_scale(&tl, &tl, NULL, &cep->riscos.scale_from_diag16);
    gr_point_scale(&br, &br, NULL, &cep->riscos.scale_from_diag16);

    /* plot origin */
    gdi_org.x = cp->core.editsave.open.visible_area.xmin - cp->core.editsave.open.scroll_x;
    gdi_org.y = cp->core.editsave.open.visible_area.ymin;

    /* add in display offset */
    gdi_org.x += cep->riscos.diagram_off_x;
    gdi_org.y += cep->riscos.diagram_off_y;

    /* bbox in absolute screen coords */
    gdi_outline.xmin = (int) (gdi_org.x + gr_riscos_from_pixit(tl.x));
    gdi_outline.ymin = (int) (gdi_org.y + gr_riscos_from_pixit(br.y));
    gdi_outline.xmax = (int) (gdi_org.x + gr_riscos_from_pixit(br.x));
    gdi_outline.ymax = (int) (gdi_org.y + gr_riscos_from_pixit(tl.y));

    {
    WimpUpdateAndRedrawWindowBlock update_and_redraw_window_block;
    BOOL more;

    update_and_redraw_window_block.update.window_handle = cep->riscos.window_handle;
    update_and_redraw_window_block.update.update_area.xmin = -0x1FFFFFFF;
    update_and_redraw_window_block.update.update_area.ymin = -0x1FFFFFFF;
    update_and_redraw_window_block.update.update_area.xmax =  0x1FFFFFFF;
    update_and_redraw_window_block.update.update_area.ymax = 0; /* 0x1FFFFFFF; */     /* RCM says that SKS claimed zero was a good upper limit */

    if(NULL != WrapOsErrorReporting(tbl_wimp_update_window(&update_and_redraw_window_block.redraw, &more)))
        more = FALSE;

    while(more)
    {
        bbc_gcol(3,15); /*>>>eor in light-blue */

        displ_box(gdi_outline.xmin, gdi_outline.ymin, gdi_outline.xmax, gdi_outline.ymax);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&update_and_redraw_window_block.redraw, &more)))
            more = FALSE;
    }
    } /*block*/
}

extern void
gr_chartedit_selected_object_drag_start(
    P_GR_CHARTEDITOR cep,
    PC_GR_POINT point,
    PC_GR_POINT workareaoff)
{
    GR_POINT resize_patch_tl;
    GR_POINT resize_patch_br;
    wimp_dragstr dragstr;
    P_GR_CHART cp;

    UNREFERENCED_PARAMETER(workareaoff);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    /* bbox of object is in cep->selection.box, point clicked in point, both are in gr_pixits */

    trace_4(TRACE_MODULE_GR_CHART, "gr_chartedit_selected_object_drag point (%d,%d), workareaoff(%d,%d)",
                                   point->x, point->y, workareaoff->x, workareaoff->y);

    /* treat the top left hand and lower right hand corners of the selection box as 'drag-to-resize', */
    /* treat the rest of the selection box as 'drag-to-reposition'                                    */
    drag_type = GR_OBJECT_DRAG_REPOSITION;

    {
    GR_SIZE half_size;
    GR_SIZE patch_size;

    half_size.cx = (cep->selection.box.x1 - cep->selection.box.x0) / 2;
    half_size.cy = (cep->selection.box.y1 - cep->selection.box.y0) / 2;

    patch_size.cx = MIN(gr_pixit_from_riscos(8), half_size.cx);
    patch_size.cy = MIN(gr_pixit_from_riscos(8), half_size.cy);

    resize_patch_tl.x = cep->selection.box.x0 + patch_size.cx;
    resize_patch_tl.y = cep->selection.box.y1 - patch_size.cy;

    resize_patch_br.x = cep->selection.box.x1 - patch_size.cx;
    resize_patch_br.y = cep->selection.box.y0 + patch_size.cy;
    } /*block*/

    if((point->x >= resize_patch_br.x) && (point->y <= resize_patch_br.y))
        drag_type = GR_OBJECT_DRAG_RESIZE_BR;
    else if((point->x <= resize_patch_tl.x) && (point->y >= resize_patch_tl.y))
        drag_type = GR_OBJECT_DRAG_RESIZE_TL;

    /* confine the drag to the chart edit window */

    dragstr.window    = cep->riscos.window_handle; /* Needed by winx_drag_box, so it can direct Wimp_EUserDrag to us */
    dragstr.type      = wimp_USER_HIDDEN;
#if FALSE
    /* Window Manager ignores inner box on hidden drags */
    dragstr.box.x0    = mx;
    dragstr.box.y0    = my;
    dragstr.box.x1    = mx+30;
    dragstr.box.y1    = my+30;
#endif
    dragstr.parent.x0 = cp->core.editsave.open.visible_area.xmin;
    dragstr.parent.y0 = cp->core.editsave.open.visible_area.ymin;
    dragstr.parent.x1 = cp->core.editsave.open.visible_area.xmax;
    dragstr.parent.y1 = cp->core.editsave.open.visible_area.ymax;

    if(status_ok(Null_EventHandlerAdd(gr_chartedit_selected_object_drag_null_handler, (P_ANY) cp->core.ch, 0)))
    {
        void_WrapOsErrorReporting(winx_drag_box(&dragstr)); /* NB winx_drag_box NOT wimp_drag_box */

        drag_start_point = drag_curr_point = *point;

        object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point);
    }
}

extern void
gr_chartedit_selected_object_drag_complete(
    P_GR_CHARTEDITOR cep,
    const BBox * const dragboxp)
{
    GR_POINT moveby;
    P_GR_CHART cp;

    UNREFERENCED_PARAMETER_InRef_(dragboxp);

    trace_0(TRACE_MODULE_GR_CHART, "gr_chartedit_selected_object_drag_complete");

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point);

    moveby.x = drag_curr_point.x - drag_start_point.x;
    moveby.y = drag_curr_point.y - drag_start_point.y;

    Null_EventHandlerRemove(gr_chartedit_selected_object_drag_null_handler, (P_ANY) cp->core.ch);

    switch(cep->selection.id.name)
    {
    case GR_CHART_OBJNAME_TEXT:
        {
        GR_BOX box;

        gr_text_box_query(cp, cep->selection.id.no, &box);

        switch(drag_type)
        {
        default: default_unhandled();
        case GR_OBJECT_DRAG_REPOSITION:
            /* reposition adjusts all coords */
            box.x0 += moveby.x;
            box.y0 += moveby.y;
            box.x1 += moveby.x;
            box.y1 += moveby.y;
            break;

        case GR_OBJECT_DRAG_RESIZE_TL:
            /* resize TL adjusts (x0, y1) only */
            box.x0 += moveby.x;
            box.y1 += moveby.y;
            break;

        case GR_OBJECT_DRAG_RESIZE_BR:
            /* resize BR adjusts (x1, y0) only */
            box.x1 += moveby.x;
            box.y0 += moveby.y;
            break;
        }

        gr_box_sort(&box, &box);

        status_consume(gr_text_box_set(cp, cep->selection.id.no, &box));
        break;
        }

    case GR_CHART_OBJNAME_LEGEND:
        {
        GR_BOX box;

        box.x0 = cp->legend.posn.x;
        box.y0 = cp->legend.posn.y;
        box.x1 = cp->legend.posn.x + cp->legend.size.x;
        box.y1 = cp->legend.posn.y + cp->legend.size.y;

        switch(drag_type)
        {
        default: default_unhandled();
        case GR_OBJECT_DRAG_REPOSITION:
            /* reposition adjusts all coords */
            box.x0 += moveby.x;
            box.x1 += moveby.x;
            box.y0 += moveby.y;
            box.y1 += moveby.y;
            break;

        case GR_OBJECT_DRAG_RESIZE_TL:
            /* resize TL adjusts (x0, y1) only */
            box.x0 += moveby.x;
            box.y1 += moveby.y;
            break;

        case GR_OBJECT_DRAG_RESIZE_BR:
            /* resize BR adjusts (x1, y0) only */
            box.x1 += moveby.x;
            box.y0 += moveby.y;
            break;
        }

        gr_box_sort(&box, &box);

        cp->legend.posn.x = box.x0;
        cp->legend.posn.y = box.y0;
        cp->legend.size.x = box.x1 - box.x0;
        cp->legend.size.y = box.y1 - box.y0;

        cp->legend.bits.manual = 1;
        break;
        }
    }

    gr_chart_modify_and_rebuild(&cep->ch);
}

null_event_proto(static, gr_chartedit_selected_object_drag_null_handler)
{
    GR_CHART_HANDLE ch = (GR_CHART_HANDLE) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(NULL_EVENTS_REQUIRED);

    case NULL_EVENT:
        {
        P_GR_CHART cp;
        wimp_mousestr mouse;
        GR_POINT point;

        trace_0(TRACE_MODULE_GR_CHART, "gr_chartedit_selected_object_drag_null_handler, ");

        cp = gr_chart_cp_from_ch(ch);
        assert(cp);

        (void) wimp_get_point_info(&mouse);

        gr_chartedit_riscos_point_from_abs(cp, &point, mouse.x, mouse.y);

        trace_2(TRACE_MODULE_GR_CHART, "point (%d,%d)", point.x, point.y);

        if((drag_curr_point.x != point.x) || (drag_curr_point.y != point.y))
        {
            P_GR_CHARTEDITOR cep;
            cep = gr_chartedit_cep_from_ceh(cp->core.ceh);
            object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point); /* remove from old position */
            drag_curr_point = point;
            object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point); /* show at new position */
        }
      /*else            */
      /*    no movement */
        }
        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

extern S32
gr_chartedit_selection_text_edit(
    P_GR_CHARTEDITOR cep,
    BOOL submenurequest)
{
    P_GR_CHART cp;
    S32       res;

    assert(cep);

    if(cep->selection.id.name != GR_CHART_OBJNAME_TEXT)
        return(0);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    res = gr_chartedit_text_editor_make(cp, cep->selection.id.no, submenurequest);

    return(res);
}

/******************************************************************************
*
* remove the editing window from the given text object in this chart
*
******************************************************************************/

extern void
gr_chartedit_text_editor_kill(
    P_GR_CHART cp,
    LIST_ITEMNO key)
{
    P_GR_TEXT t;

    if((t = gr_text_search_key(cp, key)) != NULL)
    {
        if(t->bits.being_edited)
        {
            t->bits.being_edited = 0;

            /* <<< kill kill kill */
        }
        else
            trace_1(TRACE_MODULE_GR_CHART, "text object " U32_FMT " not being edited", key);
    }
}

/******************************************************************************
*
* start up an editing window on the given text object in this chart
*
******************************************************************************/

_Check_return_
static STATUS
gr_chartedit_text_editor_make(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    BOOL submenu)
{
    P_GR_TEXT         t;
    P_GR_TEXT_GUTS    gutsp;
    MLSUBMENU_HANDLE mlsubmenu;
    STATUS res = 1;

    if((t = gr_text_search_key(cp, key)) == NULL)
        /* wot? */
        return(1);

    if(t->bits.being_edited)
        /* sounds ok ... */
        return(1);

    mlsubmenu = NULL;

    {
    GR_CHART_OBJID id;
    gr_chart_objid_from_text(key, &id);
    res = mlsubmenu_create(&mlsubmenu, GR_CHART_TEM_TEXT_EDITOR, (P_U8) gr_chart_object_name_from_id_quick(id));
    status_return(res);
    } /*block*/

    gutsp.mp = (t + 1);

    if(t->bits.live_text)
    {
        GR_CHART_VALUE value;

        gr_travel_dsp(cp, gutsp.dsp, 0, &value);

        res = mlsubmenu_settext(&mlsubmenu, value.data.text);
    }
    else
    {
        res = mlsubmenu_settext(&mlsubmenu, gutsp.textp);
    }

    if(status_ok(res))
        res = mlsubmenu_process(&mlsubmenu, submenu);

    if(status_done(res))
    {
        /* successful edit, copy over text descriptor and new text*/
        NLISTS_BLK edit_lbr = gr_text_nlists_blk_proto;
        GR_TEXT temp;
        U32 nBytes;
        STATUS add_res;

        nBytes = mlsubmenu_gettextlen(&mlsubmenu) + 1; /* + 1 for CH_NULL */

        /* save current header */
        temp = *t;

        edit_lbr.lbr = cp->text.lbr; /* for API */

        /* overwrite existing entry */
        if(NULL != (t = collect_add_entry_bytes(GR_TEXT, &edit_lbr, nBytes + sizeof32(GR_TEXT), &key, &add_res)))
        {
            P_GR_TEXT_GUTS gutsp;

            assert(cp->text.lbr == edit_lbr.lbr);
            cp->text.lbr = edit_lbr.lbr;

            /* copy over header */
            *t = temp;

            gutsp.mp = t + 1;

            /* validate as used after editing complete */
            t->bits.unused = 0;

            if(t->bits.live_text)
            {
                t->bits.live_text = 0;

                /* kill client dep */
                gr_travel_dsp(cp, gutsp.dsp, 0, NULL);
            }

            /* copy over edited text */
            status_consume(mlsubmenu_gettext(&mlsubmenu, gutsp.textp, nBytes));
        }
        else
        {
            res = add_res; /* failure to add */
        }
    }

    mlsubmenu_destroy(&mlsubmenu);

    return(res);
}

_Check_return_
extern STATUS
gr_chartedit_text_new_and_edit(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    BOOL submenu,
    PC_U8 text,
    P_GR_POINT point)
{
    P_GR_TEXT t;

    status_return(gr_text_new(cp, key, text, point));

    t = gr_text_search_key(cp, key);
    assert(t);

    t->bits.unused = 1; /* as yet */

    return(gr_chartedit_text_editor_make(cp, key, submenu));
}

/* end of gr_editt.c */
