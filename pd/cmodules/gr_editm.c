/* gr_editm.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart editing module part II - menu and dialog box handing */

/* SKS October 1991 */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

#define USE_RISCOS_COLOUR_PICKER 1

/*
internal functions
*/

#if RISCOS

#ifndef __cs_menu_h
#include "cs-menu.h"
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

#if defined(USE_RISCOS_COLOUR_PICKER)
#ifndef                 __colourpick_h
#include "cmodules/riscos/colourpick.h" /* no includes */
#endif
#else
#ifndef __dboxtcol_h
#include "dboxtcol.h"   /* no includes */
#endif
#endif /* USE_RISCOS_COLOUR_PICKER */

#ifndef                 __tristate_h
#include "cmodules/riscos/tristate.h" /* no includes */
#endif

static void
gr_chartedit_selection_fillcolour_edit(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_hintcolour_edit(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_linecolour_edit(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_linepattern_edit(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_linewidth_edit(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_textcolour_edit(
    P_GR_CHARTEDITOR cep);

#endif /* RISCOS */

static void
gr_chartedit_options_process(
    P_GR_CHARTEDITOR cep);

static void
gr_chartedit_selection_series_process(
    P_GR_CHARTEDITOR cep);

#if RISCOS

/*
callbacks from RISC OS
*/

static S32
gr_chartedit_riscos_menu_handler(
    P_ANY handle,
    P_U8 hit,
    S32 submenurequest);

static menu
gr_chartedit_riscos_menu_maker(
    P_ANY handle);

#endif /* RISCOS */

/* --------------------------------------------------------------------------------- */

extern void
gr_chartedit_menu_finalise(
    P_GR_CHARTEDITOR cep)
{
    assert(cep);

    #if RISCOS
    /* remove menu handlers from window */
    consume_bool(event_attachmenumaker_to_w_i(cep->riscos.window_handle, -1, NULL, NULL, NULL));
    #endif
}

/*ncr*/
extern S32
gr_chartedit_menu_initialise(
    P_GR_CHARTEDITOR cep)
{
    assert(cep);

    #if RISCOS
    /* set up menu handling for editing window */
    if(!event_attachmenumaker_to_w_i(
            cep->riscos.window_handle, -1,
            gr_chartedit_riscos_menu_maker,
            gr_chartedit_riscos_menu_handler,
            (P_ANY) cep->ceh))
        return(0);
    #endif

    return(1);
}

#if RISCOS

/*
the main editing menu
*/

static menu gr_chartedit_riscos_menu = NULL;

/*
its submenus, required for deletion and encoding
*/

static menu gr_chartedit_riscos_submenu_save           = NULL;
static menu gr_chartedit_riscos_submenu_gallery        = NULL;
static menu gr_chartedit_riscos_submenu_selection      = NULL;
static menu gr_chartedit_riscos_submenu_selection_text = NULL;
static menu gr_chartedit_riscos_submenu_legend         = NULL;

/******************************************************************************
*
* RISC OS event processing for the chart editor menu tree
*
******************************************************************************/

/******************************************************************************
*
* callback from RISC OS to deal with submenu opening and menu choices
*
******************************************************************************/

static void
gr_chartedit_riscos_menu_handler_needs_sprites(
    P_GR_CHARTEDITOR cep,
    PC_GR_CHART cp,
    P_U8 hit,
    S32 submenurequest)
{
    S32 selection    = hit[0];
    S32 subselection = hit[1];
    BOOL needs_sprites = FALSE;

    switch(selection)
    {
    case GR_CHART_MO_CHART_OPTIONS:
    case GR_CHART_MO_CHART_SAVE:
        break;

    case GR_CHART_MO_CHART_GALLERY:
        switch(subselection)
        {
        case mo_no_selection:
            if(submenurequest)
                break;

            switch(cp->axes[0].charttype)
            {
            case GR_CHARTTYPE_PIE:
            case GR_CHARTTYPE_BAR:
            case GR_CHARTTYPE_LINE:
            case GR_CHARTTYPE_SCAT:
                needs_sprites = TRUE;
                break;

            default:
                break;
            }
            break;

        case GR_CHART_MO_GALLERY_PIE:
        case GR_CHART_MO_GALLERY_BAR:
        case GR_CHART_MO_GALLERY_LINE:
        case GR_CHART_MO_GALLERY_SCATTER:
            needs_sprites = TRUE;
            break;

        case GR_CHART_MO_GALLERY_OVERLAYS:
        case GR_CHART_MO_GALLERY_PREFERRED:
        case GR_CHART_MO_GALLERY_SET_PREFERRED:
        case GR_CHART_MO_GALLERY_SAVE_PREFERRED:
            break;

        default:
            break;
        }
        break;

    case GR_CHART_MO_CHART_SELECTION:
        {
        switch(subselection)
        {
        case mo_no_selection:
            /* mostly ignore clicks on 'Selection =>' */
            if(submenurequest)
                break;

            switch(cep->selection.id.name)
            {
            case GR_CHART_OBJNAME_AXIS:
                needs_sprites = TRUE;
                break;

            case GR_CHART_OBJNAME_SERIES:
            case GR_CHART_OBJNAME_DROPSER:
            default:
                break;
            }
            break;

        case GR_CHART_MO_SELECTION_FILLSTYLE:
        case GR_CHART_MO_SELECTION_FILLCOLOUR:
            break;

        case GR_CHART_MO_SELECTION_LINEPATTERN:
        case GR_CHART_MO_SELECTION_LINEWIDTH:
            needs_sprites = TRUE;
            break;

        case GR_CHART_MO_SELECTION_LINECOLOUR:
        case GR_CHART_MO_SELECTION_TEXT:
        case GR_CHART_MO_SELECTION_TEXTCOLOUR:
        case GR_CHART_MO_SELECTION_TEXTSTYLE:
        case GR_CHART_MO_SELECTION_HINTCOLOUR:
            break;

        case GR_CHART_MO_SELECTION_AXIS:
            needs_sprites = TRUE;
            break;

        case GR_CHART_MO_SELECTION_SERIES:
            break;
        }
        }
        break;

    case GR_CHART_MO_CHART_LEGEND:
    case GR_CHART_MO_CHART_NEW_TEXT:
        break;

    default:
        break;
    }

    if(needs_sprites)
        gr_chart_ensure_sprites();
}

static S32
gr_chartedit_riscos_menu_handler(
    P_ANY handle,
    P_U8 hit,
    S32 submenurequest)
{
    GR_CHARTEDIT_HANDLE ceh          = handle;
    P_GR_CHARTEDITOR    cep;
    P_GR_CHART          cp;
    S32                 selection    = hit[0];
    S32                 subselection = hit[1];
    S32                 processed    = TRUE;
    S32                 modified     = FALSE;

    trace_3(TRACE_MODULE_GR_CHART,
            "gr_chartedit_riscos_menu_handler([%d][%d]): submenurequest = %s",
            selection, subselection, report_boolstring(submenurequest));

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    gr_chartedit_riscos_menu_handler_needs_sprites(cep, cp, hit, submenurequest);

    switch(selection)
    {
    case GR_CHART_MO_CHART_OPTIONS:
        gr_chartedit_options_process(cep);
        break;

    case GR_CHART_MO_CHART_SAVE:
        {
        BOOL ask;
        S32 res;

        switch(subselection)
        {
        case mo_no_selection:
#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
            if(submenurequest)
            {
                /* use dialog */
                gr_chart_save_draw_file_with_dialog(&cep->ch);
                break;
            }
#endif
            if(submenurequest)
            {
                processed = FALSE;
                break;
            }

            /* click on 'Save =>' - save file as is */
            ask = !file_is_rooted(cp->core.currentfilename);

            if(!ask)
            {
                /* chart file already has name, save to there */
                res = gr_chart_save_chart_without_dialog(&cep->ch, NULL);

                if(res < 0)
                    messagef(string_lookup(GR_CHART_MSG_CHARTSAVE_ERROR), string_lookup(res));

                break;
            }

            /* deliberate drop thru ... */

        case GR_CHART_MO_SAVE_CHART:
            gr_chart_save_chart_with_dialog(&cep->ch);
            break;

        case GR_CHART_MO_SAVE_DRAWFILE:
            gr_chart_save_draw_file_with_dialog(&cep->ch);
            break;
        }
        }
        break;

    case GR_CHART_MO_CHART_GALLERY:
        switch(subselection)
        {
        case mo_no_selection:
            if(submenurequest)
            {
                processed = FALSE;
                break;
            }

            /* display gallery appropriate for the main axes <<< improve to use selection */
            switch(cp->axes[0].charttype)
            {
            case GR_CHARTTYPE_PIE:
                gr_chartedit_gallery_pie_process(cep);
                break;

            case GR_CHARTTYPE_BAR:
                gr_chartedit_gallery_bar_process(cep);
                break;

            case GR_CHARTTYPE_LINE:
                gr_chartedit_gallery_line_process(cep);
                break;

            case GR_CHARTTYPE_SCAT:
                gr_chartedit_gallery_scat_process(cep);
                break;

            default:
                processed = FALSE;
                break;
            }
            break;

        case GR_CHART_MO_GALLERY_PIE:
            gr_chartedit_gallery_pie_process(cep);
            break;

        case GR_CHART_MO_GALLERY_BAR:
            gr_chartedit_gallery_bar_process(cep);
            break;

        case GR_CHART_MO_GALLERY_LINE:
            gr_chartedit_gallery_line_process(cep);
            break;

        case GR_CHART_MO_GALLERY_SCATTER:
            gr_chartedit_gallery_scat_process(cep);
            break;

        case GR_CHART_MO_GALLERY_OVERLAYS:
            {
            if(cp->axes_idx_max != 0)
                cp->axes_idx_max = 0;
            else
                cp->axes_idx_max = 1;

            cp->bits.realloc_series = 1;
            modified = 1;
            }
            break;

        case GR_CHART_MO_GALLERY_PREFERRED:
            gr_chart_preferred_use(&cep->ch);
            modified = 1;
            break;

        case GR_CHART_MO_GALLERY_SET_PREFERRED:
            gr_chart_preferred_set(&cep->ch);
            break;

        case GR_CHART_MO_GALLERY_SAVE_PREFERRED:
            {
            U8 filename[BUF_MAX_PATHSTRING];

            /* direct callback to client to obtain a name */
            if(gr_chart_preferred_get_name(filename, MAX_PATHSTRING))
                gr_chart_preferred_save(filename);
            }
            break;

        default:
            break;
        }
        break;

    case GR_CHART_MO_CHART_SELECTION:
        {
        switch(subselection)
        {
        case mo_no_selection:
            /* mostly ignore clicks on 'Selection =>' */
            if(submenurequest)
                processed = FALSE;
            else
                switch(cep->selection.id.name)
                {
                case GR_CHART_OBJNAME_AXIS:
                    gr_chartedit_selection_axis_process(cep);
                    break;

                case GR_CHART_OBJNAME_SERIES:
                case GR_CHART_OBJNAME_DROPSER:
                    gr_chartedit_selection_series_process(cep);
                    break;

                default:
                    processed = FALSE;
                    break;
                }
            break;

        case GR_CHART_MO_SELECTION_FILLSTYLE:
            gr_chartedit_selection_fillstyle_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_FILLCOLOUR:
            gr_chartedit_selection_fillcolour_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_LINEPATTERN:
            gr_chartedit_selection_linepattern_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_LINEWIDTH:
            gr_chartedit_selection_linewidth_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_LINECOLOUR:
            gr_chartedit_selection_linecolour_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_TEXT:
            {
            switch(hit[2])
            {
            case mo_no_selection:
                if(submenurequest)
                {
                    processed = FALSE;
                    break;
                }

            /* else deliberate drop thru ... */

            case GR_CHART_MO_SELECTION_TEXT_EDIT:
                if(gr_chartedit_selection_text_edit(cep, submenurequest) > 0)
                    modified = 1;
                break;

            case GR_CHART_MO_SELECTION_TEXT_DELETE:
                gr_chartedit_selection_text_delete(cep);
                modified = 1;
                break;
            }
            }
            break;

        case GR_CHART_MO_SELECTION_TEXTCOLOUR:
            gr_chartedit_selection_textcolour_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_TEXTSTYLE:
            gr_chartedit_selection_textstyle_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_HINTCOLOUR:
            gr_chartedit_selection_hintcolour_edit(cep);
            break;

        case GR_CHART_MO_SELECTION_AXIS:
            gr_chartedit_selection_axis_process(cep);
            break;

        case GR_CHART_MO_SELECTION_SERIES:
            gr_chartedit_selection_series_process(cep);
            break;
        }
        }
        break;

    case GR_CHART_MO_CHART_LEGEND:
        {
        switch(subselection)
        {
        case mo_no_selection:
            if(submenurequest)
                processed = FALSE;
            else
            {
                /* turn legend on/off */
                cp->legend.bits.on = !cp->legend.bits.on;

                /* in due course this will be done automagically */
                if(cep->selection.id.name == GR_CHART_OBJNAME_LEGEND)
                    gr_chartedit_selection_clear(cep);

                modified = 1;
            }
            break;

        case GR_CHART_MO_LEGEND_ARRANGE:
            cp->legend.bits.in_rows = !cp->legend.bits.in_rows;

            /* always turn legend on too */
            cp->legend.bits.on = TRUE;

            modified = 1;
            break;

        default:
            break;
        }
        }
        break;

    case GR_CHART_MO_CHART_NEW_TEXT:
        {
        int mouse_x, mouse_y;
        HOST_WND window_handle;
        int icon_handle;
        GR_POINT point;
        LIST_ITEMNO key;
        char new_text[32];
        STATUS res;

        event_read_menuclickdata(&mouse_x, &mouse_y, &window_handle, &icon_handle);

        if(icon_handle != -1)
            /* can't do anything interesting with this */
            break;

        gr_chartedit_riscos_point_from_abs(cp, &point, mouse_x, mouse_y);

        /* add to end of list or reuse dead one */
        key = gr_text_key_for_new(cp);

        (void) xsnprintf(new_text, elemof32(new_text), string_lookup(GR_CHART_MSG_TEXT_ZD), key);

        res = gr_chartedit_text_new_and_edit(cp, key, submenurequest, new_text, &point);

        modified = status_done(res);

        if(status_fail(res))
            gr_chartedit_winge(res);
        }
        break;

    default:
        trace_2(TRACE_MODULE_GR_CHART, "unprocessed gr_chartedit menu hit %d, %d", selection, subselection);
        break;
    }

    /* look up our handle again; any persistent dialog processing
     * (especially that bloody FontSelector) may not have yet noticed
     * that the parent chart editor has gone away, so we'd better not
     * go poking our noses into its structure!
    */
    cep = gr_chartedit_cep_from_ceh(ceh);
    if(cep)
    {
        /* clear out that temporary selection if we aren't going to use it again */
        if(!submenurequest)
            if(!event_is_menu_recreate_pending())
                gr_chartedit_selection_clear_if_temp(cep);

        if(modified)
            gr_chart_modify_and_rebuild(&cep->ch);
    }

    return(processed);
}

/******************************************************************************
*
* create/encode/reencode the chart editor menu tree on RISC OS
*
******************************************************************************/

static menu
gr_chartedit_riscos_menu_maker(
    P_ANY handle)
{
    GR_CHARTEDIT_HANDLE ceh = handle;
    P_GR_CHARTEDITOR cep;
    P_GR_CHART cp;
    BOOL tick, fade;

    cep = gr_chartedit_cep_from_ceh(ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    if(!gr_chartedit_riscos_menu)
    {
        /* create main chart menu just the once*/

        gr_chartedit_riscos_menu = menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_CHART),
                                              string_lookup(GR_CHART_MSG_MENU_CHART));

        /* create and add submenus */

        gr_chartedit_riscos_submenu_save =
            menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_SAVE),
                       string_lookup(GR_CHART_MSG_MENU_SAVE));

        menu_submenu(gr_chartedit_riscos_menu,
                     GR_CHART_MO_CHART_SAVE,
                     gr_chartedit_riscos_submenu_save);

        gr_chartedit_riscos_submenu_gallery =
            menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_GALLERY),
                       string_lookup(GR_CHART_MSG_MENU_GALLERY));

        menu_submenu(gr_chartedit_riscos_menu,
                     GR_CHART_MO_CHART_GALLERY,
                     gr_chartedit_riscos_submenu_gallery);

        gr_chartedit_riscos_submenu_selection =
            menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_SELECTION),
                       string_lookup(GR_CHART_MSG_MENU_SELECTION));

        menu_submenu(gr_chartedit_riscos_menu,
                     GR_CHART_MO_CHART_SELECTION,
                     gr_chartedit_riscos_submenu_selection);

        gr_chartedit_riscos_submenu_selection_text =
            menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_SELECTION_TEXT),
                       string_lookup(GR_CHART_MSG_MENU_SELECTION_TEXT));

        menu_submenu(gr_chartedit_riscos_submenu_selection,
                     GR_CHART_MO_SELECTION_TEXT,
                     gr_chartedit_riscos_submenu_selection_text);

        gr_chartedit_riscos_submenu_legend =
            menu_new_c(string_lookup(GR_CHART_MSG_MENUHDR_LEGEND),
                       string_lookup(GR_CHART_MSG_MENU_LEGEND));

        menu_submenu(gr_chartedit_riscos_menu,
                     GR_CHART_MO_CHART_LEGEND,
                     gr_chartedit_riscos_submenu_legend);
    }

    gr_chartedit_fontselect_kill(cep);

    /* try making a temporary (undisplayed) selection iff over
     * display area and no selection already made
    */
    gr_chartedit_selection_clear_if_temp(cep);

    if(cep->selection.id.name == GR_CHART_OBJNAME_ANON)
    {
        struct _wimp_eventdata_but but;
        GR_POINT point;
        GR_DIAG_OFFSET hitObject[64];
        unsigned int hitIndex;

        event_read_menuclickdata(&but.m.x, &but.m.y, &but.m.w, &but.m.i);

        if(but.m.i == -1)
        {
            gr_chartedit_riscos_point_from_abs(cp, &point, but.m.x, but.m.y);

            hitIndex = gr_nodbg_bring_me_the_head_of_yuri_gagarin(cep, &cep->selection.id, hitObject,
                                                                  point.x, point.y, FALSE);

            gr_chart_objid_find_parent(&cep->selection.id);

            /* just use as an id selector */

            if(cep->selection.id.name != GR_CHART_OBJNAME_ANON)
                cep->selection.temp = TRUE;
        }
    }

    { /* grey out 'Save =>' if no diagram to save */
    P_GR_DIAG p_gr_diag;

    tick = FALSE;
    fade = TRUE;

    gr_chart_diagram(&cep->ch, &p_gr_diag);

    if(p_gr_diag)
        fade = (p_gr_diag->gr_riscdiag.dd_allocsize == 0);

    menu_setflags(gr_chartedit_riscos_menu,
                  GR_CHART_MO_CHART_SAVE,
                  tick, fade);
    }

    /* tick gallery as to base class of current chart */
    {
    menu m            = gr_chartedit_riscos_submenu_gallery; /* keep static loaded over menu calls */
    S32 tick_pie     = FALSE;
    S32 tick_bar     = FALSE;
    S32 tick_line    = FALSE;
    S32 tick_scatter = FALSE;
    S32 tick_overlay = FALSE;

    fade = FALSE;

    if(cp->axes_idx_max > 0)
        tick_overlay = TRUE;

    switch(cp->axes[0].charttype)
    {
    default:
        assert(0);
    case GR_CHARTTYPE_PIE:
        tick_pie    = TRUE;
        tick_overlay = FALSE;
        break;

    case GR_CHARTTYPE_BAR:
        tick_bar = TRUE;
        break;

    case GR_CHARTTYPE_LINE:
        tick_line = TRUE;
        break;

    case GR_CHARTTYPE_SCAT:
        tick_scatter = TRUE;
        break;
    }

    menu_setflags(m, GR_CHART_MO_GALLERY_PIE,
                  tick_pie, fade);

    menu_setflags(m, GR_CHART_MO_GALLERY_BAR,
                  tick_bar, fade);

    menu_setflags(m, GR_CHART_MO_GALLERY_LINE,
                  tick_line, fade);

    menu_setflags(m, GR_CHART_MO_GALLERY_SCATTER,
                  tick_scatter, fade);

    menu_setflags(m, GR_CHART_MO_GALLERY_OVERLAYS,
                  tick_overlay, fade);

    fade = !gr_chart_preferred_query();

    menu_setflags(m, GR_CHART_MO_GALLERY_PREFERRED,
                  FALSE, fade);

    menu_setflags(m, GR_CHART_MO_GALLERY_SAVE_PREFERRED,
                  FALSE, fade);
    }

    /* grey out 'Selection' if none */
    {
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    tick = FALSE;
    fade = (cep->selection.id.name == GR_CHART_OBJNAME_ANON);

    menu_setflags(gr_chartedit_riscos_menu,
                  GR_CHART_MO_CHART_SELECTION,
                  tick, fade);

    if(fade)
        /* reinstate default title */
        xstrkpy(title, elemof32(title), string_lookup(GR_CHART_MSG_MENUHDR_SELECTION));
    else
        gr_chart_object_name_from_id(title, elemof32(title), cep->selection.id);

    menu_settitle(gr_chartedit_riscos_submenu_selection, title);
    }

    /* grey out bits of 'Selection' submenu as appropriate */
    {
    menu m               = gr_chartedit_riscos_submenu_selection; /* keep static loaded over menu calls */
    S32 fade_fillstyle  = FALSE;
    S32 fade_fillcolour = FALSE;
    S32 fade_linestyle  = FALSE;
    S32 fade_text       = TRUE;
    S32 fade_textstyle  = TRUE;
    S32 fade_axis       = TRUE;
    S32 fade_series     = TRUE;

    tick = FALSE;

    switch(cep->selection.id.name)
    {
    case GR_CHART_OBJNAME_ANON:
        /* fade all that is by default unfaded */
        fade_fillstyle  = TRUE;
        fade_fillcolour = TRUE;
        fade_linestyle  = TRUE;
        break;

    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
    case GR_CHART_OBJNAME_LEGEND:
        fade_textstyle  = FALSE;
        break;

    case GR_CHART_OBJNAME_TEXT:
        fade_fillstyle  = TRUE;
        fade_fillcolour = TRUE;
        fade_linestyle  = TRUE;
        fade_text       = FALSE;
        fade_textstyle  = FALSE;
        break;

    case GR_CHART_OBJNAME_DROPSER:
    case GR_CHART_OBJNAME_DROPPOINT:
        fade_fillstyle = TRUE;

        /* deliberate drop thru ... */

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
        fade_textstyle = FALSE;
        fade_series    = FALSE;
        fade_axis      = (cp->axes[0].charttype == GR_CHARTTYPE_PIE);
        break;

    case GR_CHART_OBJNAME_BESTFITSER:
        fade_fillstyle  = TRUE;
        fade_fillcolour = TRUE;
        fade_series     = FALSE;
        fade_axis       = (cp->axes[0].charttype == GR_CHARTTYPE_PIE);
        break;

    case GR_CHART_OBJNAME_AXIS:
        fade_fillstyle  = TRUE;
        fade_fillcolour = TRUE;
        fade_textstyle  = FALSE;
        fade_axis       = FALSE;
        break;

    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        fade_fillstyle  = TRUE;
        fade_fillcolour = TRUE;
        fade_axis       = FALSE;
        break;

    default:
        break;
    }

    menu_setflags(m, GR_CHART_MO_SELECTION_FILLCOLOUR,
                  tick, fade_fillcolour);

    menu_setflags(m, GR_CHART_MO_SELECTION_FILLSTYLE,
                  tick, fade_fillstyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_LINECOLOUR,
                  tick, fade_linestyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_LINEPATTERN,
                  tick, fade_linestyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_LINEWIDTH,
                  tick, fade_linestyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_TEXT,
                  tick, fade_text);

    menu_setflags(m, GR_CHART_MO_SELECTION_TEXTSTYLE,
                  tick, fade_textstyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_TEXTCOLOUR,
                  tick, fade_textstyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_HINTCOLOUR,
                  tick, fade_textstyle);

    menu_setflags(m, GR_CHART_MO_SELECTION_AXIS,
                  tick, fade_axis);

    menu_setflags(m, GR_CHART_MO_SELECTION_SERIES,
                  tick, fade_series);
    }

    /* tick 'Legend' if on */
    /* tick 'Legend => Arrange vertically' if on */
    {
    S32 tick_legend;
    S32 tick_arrange;

    fade = FALSE;
    tick_legend  =  cp->legend.bits.on;
    tick_arrange = !cp->legend.bits.in_rows;

    menu_setflags(gr_chartedit_riscos_menu,
                  GR_CHART_MO_CHART_LEGEND,
                  tick_legend, fade);

    menu_setflags(gr_chartedit_riscos_submenu_legend,
                  GR_CHART_MO_LEGEND_ARRANGE,
                  tick_arrange, fade);
    }

    /* close down the galleria on new menus */
    if(!event_is_menu_being_recreated())
        ;

    return(gr_chartedit_riscos_menu);
}

/******************************************************************************
*
* process options dialog box
*
******************************************************************************/

typedef struct GR_CHARTEDIT_LAYOUT
{
    F64 in_width;
    F64 in_height;

    F64 pct_margin_left;
    F64 pct_margin_bottom;
    F64 pct_margin_right;
    F64 pct_margin_top;
}
GR_CHARTEDIT_LAYOUT;

static const F64 in_increment =  0.10;
static const F64 in_min_limit =  1.00;
static const F64 in_max_limit = 50.00;
static const S32 in_decplaces =     2;

static const F64 pct_margin_increment =  0.5;
static const F64 pct_margin_min_limit =  0.0;
static const F64 pct_margin_max_limit = 33.0;
static const S32 pct_margin_decplaces =    1;

/* encode icons from current structure (not necessarily reflected in chart structure) */

static void
gr_chartedit_options_encode(
    _HwndRef_   HOST_WND window_handle,
    const GR_CHARTEDIT_LAYOUT * const layout)
{
    F64 tmp;

    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH,  &layout->in_width,  in_decplaces);
    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT, &layout->in_height, in_decplaces);

    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT,
                   &layout->pct_margin_left,   pct_margin_decplaces);
    tmp = layout->in_width  * layout->pct_margin_left   / 100.0;
    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_LEFT,
                   &tmp, in_decplaces);

    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM,
                   &layout->pct_margin_bottom, pct_margin_decplaces);
    tmp = layout->in_height * layout->pct_margin_bottom / 100.0;
    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_BOTTOM,
                   &tmp, in_decplaces);

    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT,
                   &layout->pct_margin_right,  pct_margin_decplaces);
    tmp = layout->in_width  * layout->pct_margin_right  / 100.0;
    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_RIGHT,
                   &tmp, in_decplaces);

    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP,
                   &layout->pct_margin_top,    pct_margin_decplaces);
    tmp = layout->in_height * layout->pct_margin_top    / 100.0;
    winf_setdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_TOP,
                   &tmp, in_decplaces);
}

static void
gr_chartedit_options_process(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    P_GR_CHART cp;
    BOOL ok, persist, pending_reflect_modify;
    GR_CHARTEDIT_LAYOUT layout;

    d = dbox_new_new(GR_CHARTEDIT_TEM_OPTIONS, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    dbox_motion_updates(d);

    window_handle = dbox_window_handle(d);

    cp = gr_chart_cp_from_ch(cep->ch);

    pending_reflect_modify = 0;

    do  {
        /* load current settings into structure */
        layout.in_width          = (F64) cp->core.layout.width  / GR_PIXITS_PER_INCH;
        layout.in_height         = (F64) cp->core.layout.height / GR_PIXITS_PER_INCH;

        layout.pct_margin_bottom = (F64) cp->core.layout.margins.bottom / cp->core.layout.height * 100.0;
        layout.pct_margin_left   = (F64) cp->core.layout.margins.left   / cp->core.layout.width  * 100.0;
        layout.pct_margin_top    = (F64) cp->core.layout.margins.top    / cp->core.layout.height * 100.0;
        layout.pct_margin_right  = (F64) cp->core.layout.margins.right  / cp->core.layout.width  * 100.0;

        for(;;)
        {
            gr_chartedit_options_encode(window_handle, &layout);

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_OPTIONS_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            if(f != dbox_OK)
            {
                const int hit_icon_handle = dbox_field_to_icon_handle(d, f);

                /* adjusters all modify but rebuild at end */
                pending_reflect_modify = 1;

                switch(hit_icon_handle)
                {
                /* nothing other than inc/dec/vals */
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH:
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT:
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT:
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM:
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT:
                case GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP:
                    /* only purpose here is to get state reencoded on caret motion */
                    break;

                default:
                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH,
                                       &layout.in_width,
                                       &in_increment,
                                       &in_min_limit,
                                       &in_max_limit,
                                        in_decplaces))
                        break;

                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT,
                                       &layout.in_height,
                                       &in_increment,
                                       &in_min_limit,
                                       &in_max_limit,
                                        in_decplaces))
                        break;

                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT,
                                       &layout.pct_margin_left,
                                       &pct_margin_increment,
                                       &pct_margin_min_limit,
                                       &pct_margin_max_limit,
                                        pct_margin_decplaces))
                        break;

                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM,
                                       &layout.pct_margin_bottom,
                                       &pct_margin_increment,
                                       &pct_margin_min_limit,
                                       &pct_margin_max_limit,
                                        pct_margin_decplaces))
                        break;

                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT,
                                       &layout.pct_margin_right,
                                       &pct_margin_increment,
                                       &pct_margin_min_limit,
                                       &pct_margin_max_limit,
                                        pct_margin_decplaces))
                        break;

                    if(winf_bumpdouble(window_handle, hit_icon_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP,
                                       &layout.pct_margin_top,
                                       &pct_margin_increment,
                                       &pct_margin_min_limit,
                                       &pct_margin_max_limit,
                                        pct_margin_decplaces))
                        break;

                    assert(0);
                    break;
                }
            }

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH,
                             &layout.in_width,
                             &pending_reflect_modify,
                             &in_min_limit,
                             &in_max_limit,
                              in_decplaces);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT,
                             &layout.in_height,
                             &pending_reflect_modify,
                             &in_min_limit,
                             &in_max_limit,
                              in_decplaces);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT,
                             &layout.pct_margin_left,
                             &pending_reflect_modify,
                             &pct_margin_min_limit,
                             &pct_margin_max_limit,
                              pct_margin_decplaces);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM,
                             &layout.pct_margin_bottom,
                             &pending_reflect_modify,
                             &pct_margin_min_limit,
                             &pct_margin_max_limit,
                              pct_margin_decplaces);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT,
                             &layout.pct_margin_right,
                             &pending_reflect_modify,
                             &pct_margin_min_limit,
                             &pct_margin_max_limit,
                              pct_margin_decplaces);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP,
                             &layout.pct_margin_top,
                             &pending_reflect_modify,
                             &pct_margin_min_limit,
                             &pct_margin_max_limit,
                              pct_margin_decplaces);

            if(f == dbox_OK)
                break;
        }

        ok = (f == dbox_OK);

        if(ok)
        {
            /* set chart from modified structure */
            cp->core.layout.width          = (GR_COORD) (layout.in_width  * GR_PIXITS_PER_INCH);
            cp->core.layout.height         = (GR_COORD) (layout.in_height * GR_PIXITS_PER_INCH);

            cp->core.layout.margins.left   = (GR_COORD) (layout.pct_margin_left   / 100.0 * cp->core.layout.width );
            cp->core.layout.margins.bottom = (GR_COORD) (layout.pct_margin_bottom / 100.0 * cp->core.layout.height);
            cp->core.layout.margins.right  = (GR_COORD) (layout.pct_margin_right  / 100.0 * cp->core.layout.width );
            cp->core.layout.margins.top    = (GR_COORD) (layout.pct_margin_top    / 100.0 * cp->core.layout.height);

            /* update dependent values */
            cp->core.layout.size.x         = cp->core.layout.width  - (cp->core.layout.margins.left   + cp->core.layout.margins.right);
            cp->core.layout.size.y         = cp->core.layout.height - (cp->core.layout.margins.bottom + cp->core.layout.margins.top  );

            gr_chart_modify_and_rebuild(&cep->ch);
        }

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    dbox_dispose(&d);
}

/******************************************************************************
*
* edit selected series attributes
*
******************************************************************************/

typedef struct GR_CHARTEDIT_SERIES_STATE
{
    int tristate_cumulative : 2;
    int tristate_point_vary : 2;
    int tristate_best_fit   : 2;
    int tristate_fill_axis  : 2;
}
GR_CHARTEDIT_SERIES_STATE;

/* encode icons from current structure (not necessarily reflected in chart structure) */

static void
gr_chartedit_selection_series_encode(
    _HwndRef_   HOST_WND window_handle,
    const GR_CHARTEDIT_SERIES_STATE * const state)
{
    riscos_tristate_set(window_handle, GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CUMULATIVE, state->tristate_cumulative);
    riscos_tristate_set(window_handle, GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_POINT_VARY, state->tristate_point_vary);
    riscos_tristate_set(window_handle, GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_BEST_FIT,   state->tristate_best_fit);
    riscos_tristate_set(window_handle, GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_FILL_AXIS,  state->tristate_fill_axis);
}

static void
gr_chartedit_selection_series_process(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    P_GR_CHART cp;
    GR_SERIES_IDX modifying_series_idx;
    P_GR_SERIES serp;
    BOOL ok, persist, pending_reflect_modify;
    GR_CHARTEDIT_SERIES_STATE state;

    d = dbox_new_new(GR_CHARTEDIT_TEM_SELECTION_SERIES, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    window_handle = dbox_window_handle(d);

    cp = gr_chart_cp_from_ch(cep->ch);

    pending_reflect_modify = 0;

    switch(cep->selection.id.name)
    {
    default:
        modifying_series_idx = 0;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSER:
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        modifying_series_idx = gr_series_idx_from_external(cp, cep->selection.id.no);
        break;
    }

    /* always reflect the name of the series we will be
     * modifying, not the actual selected object
    */
    {
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR];
    GR_CHART_OBJID modifying_id;

    gr_chart_objid_from_series_idx(cp, modifying_series_idx, &modifying_id);

    gr_chart_object_name_from_id(title, elemof32(title), modifying_id);

    win_settitle(window_handle, title);
    } /*block*/

    do  {
        /* load current settings into structure */
        serp = getserp(cp, modifying_series_idx);

        state.tristate_cumulative = RISCOS_TRISTATE_DONT_CARE;
        state.tristate_point_vary = RISCOS_TRISTATE_DONT_CARE;
        state.tristate_best_fit   = RISCOS_TRISTATE_DONT_CARE;
        state.tristate_fill_axis  = RISCOS_TRISTATE_DONT_CARE;

        for(;;)
        {
            gr_chartedit_selection_series_encode(window_handle, &state);

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            if(f != dbox_OK)
            {
                const int hit_icon_handle = dbox_field_to_icon_handle(d, f);

                /* adjusters all modify but rebuild at end */
                pending_reflect_modify = 1;

                switch(hit_icon_handle)
                {
                case GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CUMULATIVE:
                    state.tristate_cumulative = riscos_tristate_hit(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_POINT_VARY:
                    state.tristate_point_vary = riscos_tristate_hit(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_BEST_FIT:
                    state.tristate_best_fit   = riscos_tristate_hit(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_FILL_AXIS:
                    state.tristate_fill_axis  = riscos_tristate_hit(window_handle, hit_icon_handle);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_REMOVE_SERIES:
                    {
                    GR_DATASOURCE_NO ds;

                    serp = getserp(cp, modifying_series_idx);

                    for(ds = 0; ds < GR_SERIES_MAX_DATASOURCES; ++ds)
                        if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
                            gr_chart_subtract_datasource_using_dsh(cp, serp->datasources.dsh[ds]);

                    switch(cep->selection.id.name)
                    {
                    case GR_CHART_OBJNAME_SERIES:
                    case GR_CHART_OBJNAME_POINT:
                    case GR_CHART_OBJNAME_DROPSER:
                    case GR_CHART_OBJNAME_DROPPOINT:
                    case GR_CHART_OBJNAME_BESTFITSER:
                        cep->selection.id = gr_chart_objid_anon;
                        break;

                    default:
                        break;
                    }

                    gr_chart_modify_and_rebuild(&cep->ch);

                    goto endpoint;
                    }

                default:
                    assert(0);
                    break;
                }
            }

            if(f == dbox_OK)
                break;
        }

        ok = (f == dbox_OK);

        if(ok)
        {
            /* set chart from modified structure */
            serp = getserp(cp, modifying_series_idx);

            if(state.tristate_cumulative != RISCOS_TRISTATE_DONT_CARE)
            {
                serp->bits.cumulative        = (state.tristate_cumulative == RISCOS_TRISTATE_ON);
                serp->bits.cumulative_manual = 1;

                * (int *) &serp->valid = 0;
            }

            if(state.tristate_point_vary != RISCOS_TRISTATE_DONT_CARE)
            {
                serp->bits.point_vary        = (state.tristate_point_vary == RISCOS_TRISTATE_ON);
                serp->bits.point_vary_manual = 1;
            }

            if(state.tristate_best_fit != RISCOS_TRISTATE_DONT_CARE)
            {
                serp->bits.best_fit          = (state.tristate_best_fit == RISCOS_TRISTATE_ON);
                serp->bits.best_fit_manual   = 1;
            }

            if(state.tristate_fill_axis != RISCOS_TRISTATE_DONT_CARE)
            {
                serp->bits.fill_axis         = (state.tristate_fill_axis == RISCOS_TRISTATE_ON);
                serp->bits.fill_axis_manual  = 1;
            }

            gr_chart_modify_and_rebuild(&cep->ch);
        }

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    endpoint:;

    dbox_dispose(&d);
}

/******************************************************************************
*
* callback from colour picker to set an object colour
*
******************************************************************************/

typedef enum GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_TAG
{
    gr_chartedit_riscos_colpick_callback_linecolour,
    gr_chartedit_riscos_colpick_callback_fillcolour,
    gr_chartedit_riscos_colpick_callback_textcolour,
    gr_chartedit_riscos_colpick_callback_hintcolour
}
GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_TAG;

typedef struct GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO
{
    GR_CHARTEDIT_HANDLE ceh;
    GR_CHART_OBJID      id;

    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_TAG tag;

    union GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO_STYLE
    {
        GR_LINESTYLE line;
        GR_FILLSTYLE fill;
        GR_TEXTSTYLE text;
    }
    style;
}
GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO;

#if defined(USE_RISCOS_COLOUR_PICKER)

static GR_COLOUR
gr_colour_from_rgb(
    _InRef_     PC_RGB p_rgb)
{
    GR_COLOUR colour;

    * (int *) &colour = 0;

    colour.red   = p_rgb->r;
    colour.green = p_rgb->g;
    colour.blue  = p_rgb->b;

    if(0 == p_rgb->transparent)
        colour.visible  = 1;

    return(colour);
}

static void
gr_colour_to_rgb(
    _OutRef_    P_RGB p_rgb,
    _InVal_     GR_COLOUR colour)
{
    rgb_set(p_rgb, colour.red, colour.green, colour.blue);

    if(!colour.visible)
    {   /* transparent (if it gets this far) */
        p_rgb->transparent = 0xFF;
    }
}

static void
gr_chartedit_riscos_colour_picker_handler(
    _InRef_     PC_RGB p_rgb,
    P_ANY handle)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * const i = handle;
    P_GR_CHARTEDITOR cep;
    P_GR_CHART cp;
    GR_COLOUR col;
    BOOL modify = FALSE;
    STATUS res = 1;

    col = gr_colour_from_rgb(p_rgb);

    cep = gr_chartedit_cep_from_ceh(i->ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    switch(i->tag)
    {
    case gr_chartedit_riscos_colpick_callback_linecolour:
        {
        P_GR_LINESTYLE cur_style = &i->style.line;
        GR_LINESTYLE  new_style = *cur_style;

        new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1;
            *cur_style = new_style;
            res = gr_chart_objid_linestyle_set(cp, i->id, cur_style);
        }
        break;
        }

    case gr_chartedit_riscos_colpick_callback_fillcolour:
        {
        P_GR_FILLSTYLE cur_style = &i->style.fill;
        GR_FILLSTYLE  new_style = *cur_style;

        new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1;
            *cur_style = new_style;
            res = gr_chart_objid_fillstyle_set(cp, i->id, cur_style);
        }
        break;
        }

    case gr_chartedit_riscos_colpick_callback_textcolour:
    case gr_chartedit_riscos_colpick_callback_hintcolour:
        {
        P_GR_TEXTSTYLE cur_style = &i->style.text;
        GR_TEXTSTYLE  new_style = *cur_style;

        if(i->tag == gr_chartedit_riscos_colpick_callback_hintcolour)
            new_style.bg = col;
        else
            new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1; /* yes, in EITHER case */
            *cur_style = new_style;
            res = gr_chart_objid_textstyle_set(cp, i->id, cur_style);
        }
        break;
        }

    default:
        break;
    }

    if(status_fail(res))
        gr_chartedit_winge(res);

    if(modify)
        gr_chart_modify_and_rebuild(&cep->ch);
}

static void
gr_chartedit_selection_colour_edit_common(
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * i,
    _In_z_      PC_U8Z title,
    _InoutRef_  P_RGB p_rgb)
{
    BOOL ok;

    ok = riscos_colour_picker(COLOUR_PICKER_SUBMENU,
                              HOST_WND_NONE,
                              p_rgb, TRUE /*allow_transparent*/, title);

    if(ok)
        gr_chartedit_riscos_colour_picker_handler(p_rgb, i);
}

#else /* NOT defined(USE_RISCOS_COLOUR_PICKER) */

static void
gr_chartedit_riscos_dboxtcol_handler(
    dboxtcol_colour ecol,
    P_ANY handle)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * const i = handle;
    P_GR_CHARTEDITOR cep;
    P_GR_CHART cp;
    GR_COLOUR col;
    BOOL modify = FALSE;
    STATUS res = 1;

    col = gr_colour_from_riscDraw(ecol);

    cep = gr_chartedit_cep_from_ceh(i->ceh);
    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    switch(i->tag)
    {
    case gr_chartedit_riscos_colpick_callback_linecolour:
        {
        P_GR_LINESTYLE cur_style = &i->style.line;
        GR_LINESTYLE  new_style = *cur_style;

        new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1;
            *cur_style = new_style;
            res = gr_chart_objid_linestyle_set(cp, i->id, cur_style);
        }
        break;
        }

    case gr_chartedit_riscos_colpick_callback_fillcolour:
        {
        P_GR_FILLSTYLE cur_style = &i->style.fill;
        GR_FILLSTYLE  new_style = *cur_style;

        new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1;
            *cur_style = new_style;
            res = gr_chart_objid_fillstyle_set(cp, i->id, cur_style);
        }
        break;
        }

    case gr_chartedit_riscos_colpick_callback_textcolour:
    case gr_chartedit_riscos_colpick_callback_hintcolour:
        {
        P_GR_TEXTSTYLE cur_style = &i->style.text;
        GR_TEXTSTYLE  new_style = *cur_style;

        if(i->tag == gr_chartedit_riscos_colpick_callback_hintcolour)
            new_style.bg = col;
        else
            new_style.fg = col;

        modify = (0 != memcmp(&new_style, cur_style, sizeof(new_style)));

        if(modify)
        {
            new_style.fg.manual = 1; /* yes, in EITHER case */
            *cur_style = new_style;
            res = gr_chart_objid_textstyle_set(cp, i->id, cur_style);
        }
        break;
        }

    default:
        break;
    }

    if(status_fail(res))
        gr_chartedit_winge(res);

    if(modify)
        gr_chart_modify_and_rebuild(&cep->ch);
}

#endif /* defined(USE_RISCOS_COLOUR_PICKER) */

static void
gr_chartedit_selection_fillcolour_edit_do(
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * i,
    _In_z_      PC_U8Z title)
{
#if defined(USE_RISCOS_COLOUR_PICKER)
    RGB rgb;

    gr_colour_to_rgb(&rgb, i->style.fill.fg);

    gr_chartedit_selection_colour_edit_common(i, title, &rgb);
#else
    dboxtcol_colour col = gr_colour_to_riscDraw(i->style.fill.fg);

    dboxtcol(&col, TRUE /*allow_transparent*/,
             de_const_cast(char *, title),
             gr_chartedit_riscos_dboxtcol_handler, i);
#endif
}

static void
gr_chartedit_selection_fillcolour_edit(
    P_GR_CHARTEDITOR cep)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO i;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;
    i.tag = gr_chartedit_riscos_colpick_callback_fillcolour;

    {
    S32 appendage = GR_CHART_MSG_EDIT_APPEND_FILL;

    switch(i.id.name)
    {
    case GR_CHART_OBJNAME_PLOTAREA:
        if(i.id.no == 0)
            /* 'Plot area area' sounds silly */
            appendage = 0;
        break;

    case GR_CHART_OBJNAME_CHART:
        appendage = GR_CHART_MSG_EDIT_APPEND_AREA;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), i.id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_COLOUR));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_fillstyle_query(cp, i.id, &i.style.fill);
    }

    gr_chartedit_selection_fillcolour_edit_do(&i, title);
}

static void
gr_chartedit_selection_hintcolour_edit_do(
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * i,
    _In_z_      PC_U8Z title)
{
#if defined(USE_RISCOS_COLOUR_PICKER)
    RGB rgb;

    gr_colour_to_rgb(&rgb, i->style.text.bg);

    gr_chartedit_selection_colour_edit_common(i, title, &rgb);
#else
    dboxtcol_colour col = gr_colour_to_riscDraw(i->style.text.bg);

    dboxtcol(&col, TRUE /*allow_transparent*/,
             de_const_cast(char *, title),
             gr_chartedit_riscos_dboxtcol_handler, i);
#endif
}

static void
gr_chartedit_selection_hintcolour_edit(
    P_GR_CHARTEDITOR cep)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO i;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;
    i.tag = gr_chartedit_riscos_colpick_callback_hintcolour;

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

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_HINTCOLOUR));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_textstyle_query(cp, i.id, &i.style.text);
    }

    gr_chartedit_selection_hintcolour_edit_do(&i, title);
}

static void
gr_chartedit_selection_linecolour_edit_do(
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * i,
    _In_z_      PC_U8Z title)
{
#if defined(USE_RISCOS_COLOUR_PICKER)
    RGB rgb;

    gr_colour_to_rgb(&rgb, i->style.line.fg);

    gr_chartedit_selection_colour_edit_common(i, title, &rgb);
#else
    dboxtcol_colour col = gr_colour_to_riscDraw(i->style.line.fg);

    dboxtcol(&col, TRUE /*allow_transparent*/,
             de_const_cast(char *, title),
             gr_chartedit_riscos_dboxtcol_handler, i);
#endif
}

static void
gr_chartedit_selection_linecolour_edit(
    P_GR_CHARTEDITOR cep)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO i;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;
    i.tag = gr_chartedit_riscos_colpick_callback_linecolour;

    {
    S32 appendage = GR_CHART_MSG_EDIT_APPEND_LINE;

    switch(i.id.name)
    {
    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
    case GR_CHART_OBJNAME_LEGEND:
        appendage = GR_CHART_MSG_EDIT_APPEND_BORDER;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), i.id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_COLOUR));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_linestyle_query(cp, i.id, &i.style.line);
    }

    gr_chartedit_selection_linecolour_edit_do(&i, title);
}

static void
gr_chartedit_riscos_linepattern_callback(
    P_GR_CHARTEDITOR  cep,
    _InVal_     GR_CHART_OBJID id,
    _HwndRef_   HOST_WND window_handle,
    P_GR_LINESTYLE    cur_style)
{
    int icon_handle;
    GR_LINESTYLE new_style;

    assert(cep);

    /* copy over bits we won't modify */
    new_style = *cur_style;

    /* process using selection from dialog box */

    /* line pattern */
    icon_handle =
        winf_whichonoff(window_handle,
                        GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST,
                        GR_CHARTEDIT_TEM_LINEPATTERN_ICON_LAST,
                        GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST);

    new_style.pattern = (GR_LINE_PATTERN_HANDLE)
                        (icon_handle - GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST);

    if(0 != memcmp(&new_style, cur_style, sizeof(new_style)))
    {
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        new_style.fg.manual = 1;
        *cur_style = new_style;
        gr_chartedit_winge(gr_chart_objid_linestyle_set(cp, id, cur_style));

        gr_chart_modify_and_rebuild(&cep->ch);
    }
}

static void
gr_chartedit_riscos_linepattern_encode(
    P_GR_CHARTEDITOR  cep,
    _InVal_     GR_CHART_OBJID id,
    _HwndRef_   HOST_WND window_handle,
    P_GR_LINESTYLE    cur_style /*out*/)
{
    /* encode icons from current state (ok, ought to
     * use tristates but RISC OS doesn't have the concept)
    */
    P_GR_CHART cp;
    int icon_handle;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_linestyle_query(cp, id, cur_style);

    /* line pattern */

    for(icon_handle  = GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST;
        icon_handle <= GR_CHARTEDIT_TEM_LINEPATTERN_ICON_LAST;
        icon_handle++)
    {
        winf_setonoff(window_handle, icon_handle,
                      ((int) cur_style->pattern ==
                       (icon_handle - GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST)));
    }
}

static void
gr_chartedit_selection_linepattern_edit(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    BOOL ok, persist;
    GR_CHART_OBJID modifying_id;
    GR_LINESTYLE linestyle;

    assert(cep);

    d = dbox_new_new(GR_CHARTEDIT_TEM_LINEPATTERN, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    window_handle = dbox_window_handle(d);

    modifying_id = cep->selection.id;

    {
    U8  title[BUF_MAX_GR_CHART_OBJID_REPR + 32];
    S32 appendage = GR_CHART_MSG_EDIT_APPEND_LINE;

    switch(modifying_id.name)
    {
    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
    case GR_CHART_OBJNAME_LEGEND:
        appendage = GR_CHART_MSG_EDIT_APPEND_BORDER;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), modifying_id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_STYLE));

    win_settitle(window_handle, title);
    }

    do  {
        gr_chartedit_riscos_linepattern_encode(cep, modifying_id, window_handle, &linestyle);

        for(;;)
        {
            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_LINEPATTERN_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            if(f != dbox_OK)
            {
                switch(f)
                {
                default:
                    break;
                }
            }

            if(f == dbox_OK)
                break;
        }

        ok = (f == dbox_OK);

        if(ok)
            gr_chartedit_riscos_linepattern_callback(cep, modifying_id, window_handle, &linestyle);

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    dbox_dispose(&d);
}

static const GR_COORD
gr_chartedit_riscos_dialog_linewidth[] = /* pixits */
{
    0,                              /*w0 == 'Thin' */
    (1 * GR_PIXITS_PER_POINT) / 10, /*w1*/
    (1 * GR_PIXITS_PER_POINT) / 4,  /*w2*/
    (1 * GR_PIXITS_PER_POINT) / 2,  /*w3*/
    (1 * GR_PIXITS_PER_POINT) / 1,  /*w4*/
    (2 * GR_PIXITS_PER_POINT) / 1,  /*w5*/
    (4 * GR_PIXITS_PER_POINT) / 1,  /*w6*/
};

static void
gr_chartedit_riscos_linewidth_callback(
    P_GR_CHARTEDITOR  cep,
    _InVal_     GR_CHART_OBJID id,
    _HwndRef_   HOST_WND window_handle,
    P_GR_LINESTYLE    cur_style)
{
    F64 dval;
    GR_COORD ival;
    int icon_handle;
    GR_LINESTYLE new_style;

    assert(cep);

    /* copy over bits we won't modify */
    new_style = *cur_style;

    /* process using selection from dialog box */

    /* line width */
    dval = winf_getdouble(window_handle, GR_CHARTEDIT_TEM_LINEWIDTH_ICON_USER, NULL, INT_MAX);

    /* allow number field as default hit if conversion kosher */
    if((dval < 0.0)  ||  (dval > (F64) INT_MAX /* covers HUGE_VAL case */))
        ival = -1;
    else
        ival = (GR_COORD) (dval * GR_PIXITS_PER_POINT);

    if((ival == cur_style->width)  ||  (ival == -1))
    {
        icon_handle =
            winf_whichonoff(window_handle,
                            GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST,
                            GR_CHARTEDIT_TEM_LINEWIDTH_ICON_LAST,
                            GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST);

        new_style.width = gr_chartedit_riscos_dialog_linewidth[icon_handle - GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST];
    }
    else
        new_style.width = ival;

    if(0 != memcmp(&new_style, cur_style, sizeof(new_style)))
    {
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        new_style.fg.manual = 1;
        *cur_style = new_style;
        gr_chartedit_winge(gr_chart_objid_linestyle_set(cp, id, cur_style));

        gr_chart_modify_and_rebuild(&cep->ch);
    }
}

static void
gr_chartedit_riscos_linewidth_encode(
    P_GR_CHARTEDITOR  cep,
    _InVal_     GR_CHART_OBJID id,
    _HwndRef_   HOST_WND window_handle,
    P_GR_LINESTYLE    cur_style /*out*/)
{
    /* encode icons from current state (ok, ought to
     * use tristates but RISC OS doesn't have the concept)
    */
    P_GR_CHART cp;
    int icon_handle;
    F64 dval;

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_linestyle_query(cp, id, cur_style);

    /* line width */

    if(cur_style->width == 0)
        winf_setfield(window_handle, GR_CHARTEDIT_TEM_LINEWIDTH_ICON_USER,
                      string_lookup(GR_CHART_MSG_EDITOR_LINEWIDTH_THIN));
    else
    {
        dval = (F64) cur_style->width / GR_PIXITS_PER_POINT;
        winf_setdouble(window_handle, GR_CHARTEDIT_TEM_LINEWIDTH_ICON_USER, &dval, 2);
    }

    for(icon_handle  = GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST;
        icon_handle <= GR_CHARTEDIT_TEM_LINEWIDTH_ICON_LAST;
        icon_handle++)
    {
        winf_setonoff(window_handle, icon_handle,
                      (cur_style->width ==
                       gr_chartedit_riscos_dialog_linewidth[icon_handle - GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST]));
    }
}

static void
gr_chartedit_selection_linewidth_edit(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    BOOL ok, persist;
    GR_CHART_OBJID modifying_id;
    GR_LINESTYLE linestyle;

    assert(cep);

    d = dbox_new_new(GR_CHARTEDIT_TEM_LINEWIDTH, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    window_handle = dbox_window_handle(d);

    modifying_id = cep->selection.id;

    {
    U8  title[BUF_MAX_GR_CHART_OBJID_REPR + 32];
    S32 appendage = GR_CHART_MSG_EDIT_APPEND_LINE;

    switch(modifying_id.name)
    {
    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
    case GR_CHART_OBJNAME_LEGEND:
        appendage = GR_CHART_MSG_EDIT_APPEND_BORDER;
        break;

    default:
        break;
    }

    gr_chart_object_name_from_id(title, elemof32(title), modifying_id);

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));

    xstrkat(title, elemof32(title), string_lookup(GR_CHART_MSG_EDIT_APPEND_STYLE));

    win_settitle(window_handle, title);
    }

    do  {
        gr_chartedit_riscos_linewidth_encode(cep, modifying_id, window_handle, &linestyle);

        for(;;)
        {
            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_LINEWIDTH_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            if(f != dbox_OK)
            {
                int icon_handle;

                switch(f)
                {
                case GR_CHARTEDIT_TEM_LINEWIDTH_ICON_USER:
                    /* clicking in user editing box turns off all width buttons */
                    for(icon_handle  = GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST;
                        icon_handle <= GR_CHARTEDIT_TEM_LINEWIDTH_ICON_LAST;
                        icon_handle++)
                    {
                        winf_setonoff(window_handle, icon_handle, 0);
                    }
                    break;

                default:
                    break;
                }
            }

            if(f == dbox_OK)
                break;
        }

        ok = (f == dbox_OK);

        if(ok)
            gr_chartedit_riscos_linewidth_callback(cep, modifying_id, window_handle, &linestyle);

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    dbox_dispose(&d);
}

static void
gr_chartedit_selection_textcolour_edit_do(
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO * i,
    _In_z_      PC_U8Z title)
{
#if defined(USE_RISCOS_COLOUR_PICKER)
    RGB rgb;

    gr_colour_to_rgb(&rgb, i->style.text.fg);

    gr_chartedit_selection_colour_edit_common(i, title, &rgb);
#else
    dboxtcol_colour col;

    col = gr_colour_to_riscDraw(i->style.text.fg);

    dboxtcol(&col, TRUE /*allow_transparent*/,
             de_const_cast(char *, title),
             gr_chartedit_riscos_dboxtcol_handler, i);
#endif
}

static void
gr_chartedit_selection_textcolour_edit(
    P_GR_CHARTEDITOR cep)
{
    GR_CHARTEDIT_RISCOS_COLPICK_CALLBACK_INFO i;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;
    i.tag = gr_chartedit_riscos_colpick_callback_textcolour;

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
        appendage = GR_CHART_MSG_EDIT_APPEND_COLOUR;
        break;

    default:
        appendage = GR_CHART_MSG_EDIT_APPEND_TEXTCOLOUR;
        break;
    }

    if(appendage)
        xstrkat(title, elemof32(title), string_lookup(appendage));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_textstyle_query(cp, i.id, &i.style.text);
    }

    gr_chartedit_selection_textcolour_edit_do(&i, title);
}

#endif /* RISCOS */

extern void
gr_chartedit_winge(
    _InVal_     STATUS err)
{
    if(status_fail(err))
        messagef(string_lookup(err), ""); /* SKS after PD 4.12 03apr92 - for error strings that demand args */
}

/* end of gr_editm.c */
