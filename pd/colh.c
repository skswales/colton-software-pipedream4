/* colh.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Column heading and contents line code for PipeDream 4 */

/* RCM Aug 1991 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __akbd_h
#include "akbd.h"
#endif

#ifndef __resspr_h
#include "resspr.h"     /* includes sprite.h */
#endif

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

#include "riscos_x.h"
#include "riscdraw.h"
#include "pd_x.h"
#include "ridialog.h"
#include "colh_x.h"
#include "pointer.h"

/* exported routines */

extern void
colh_colour_icons(void);

extern void
colh_draw_column_headings(void);

extern void
colh_draw_contents_of_number_cell(void);

extern void
colh_draw_drag_state(
    int size);

extern void
colh_draw_edit_state_indicator(void);

extern void
colh_draw_mark_state_indicator(
    BOOL sheet_marked);

extern void
colh_draw_cell_count(
    _In_opt_z_      char *text);

extern void
colh_draw_cell_count_in_document(
    _In_opt_z_      char *text);

extern BOOL
colh_event_handler(
    wimp_eventstr *e,
    void *handle);

extern void
colh_pointershape_drag_notification(
    BOOL start);

extern void
colh_position_icons(void);

extern void
riscos_setpointershape(
    pointer_shape *shape);

static void
colh_forced_draw_column_headings(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block);

static void
colh_really_draw_column_headings(void);

static void
draw_status_box(
    _In_opt_z_  char *text);

static void
draw_status_box_no_interlock(
    _In_opt_z_  char *text);

static BOOL
colh_event_Mouse_Click(
    const WimpMouseClickEvent * const mouse_click);

static BOOL
colh_event_Mouse_Click_single(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked);

static BOOL
colh_event_Mouse_Click_double(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked);

static BOOL
colh_event_Mouse_Click_start_drag(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked);

static BOOL
colh_event_User_Message(
    /*poked*/ WimpMessage * const user_message);

static void
EditContentsLine(void);

static void
colh_pointershape_starttracking(void);

static void
colh_pointershape_endtracking(void);

/* callback function */

null_event_proto(static, colh_pointershape_null_handler);

static BOOL drag_in_column_header = FALSE;
static DOCNO track_docno;

extern void
redefine_icon(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    WimpCreateIconBlock * const create)
{
    WimpGetIconStateBlock icon_state;

    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    if( (create->icon.bbox.xmin != icon_state.icon.bbox.xmin) ||
        (create->icon.bbox.ymin != icon_state.icon.bbox.ymin) ||
        (create->icon.bbox.xmax != icon_state.icon.bbox.xmax) ||
        (create->icon.bbox.ymax != icon_state.icon.bbox.ymax) ||
        (create->icon.flags  != icon_state.icon.flags)  )
    {
        /* icon size or flags are wrong, so... */

        WimpCaret caret;
        int new_icon_handle;

        { /* must force-redraw max of current box and required box as resize may scroll the text */
        BBox redraw_area;
        redraw_area.xmin = MIN(create->icon.bbox.xmin, icon_state.icon.bbox.xmin); /* update_area */
        redraw_area.ymin = MIN(create->icon.bbox.ymin, icon_state.icon.bbox.ymin);
        redraw_area.xmax = MAX(create->icon.bbox.xmax, icon_state.icon.bbox.xmax);
        redraw_area.ymax = MAX(create->icon.bbox.ymax, icon_state.icon.bbox.ymax);
        void_WrapOsErrorReporting(tbl_wimp_force_redraw(window_handle, redraw_area.xmin, redraw_area.ymin, redraw_area.xmax, redraw_area.ymax));
        } /*block*/

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        {   /* ensure we don't restore to undefined window handle */
            caret.window_handle = -1;
            caret.icon_handle = -1;
        }
        else
        {
            if( (caret.window_handle == window_handle) &&
                (caret.icon_handle == icon_handle) )
            {   /* hide caret */
                void_WrapOsErrorReporting(tbl_wimp_set_caret_position(-1, -1, 0, 0, 0x02000000, 0));
            }
        }

        /* delete current definition */
        void_WrapOsErrorReporting(wimp_delete_icon(window_handle, icon_handle));

        /* and re-create icon with required size */
        create->window_handle = window_handle;
        void_WrapOsErrorReporting(tbl_wimp_create_icon(0, create, &new_icon_handle));
        assert(icon_handle == new_icon_handle); /* MUST retain same icon number */

        if( (caret.window_handle == window_handle) &&
            (caret.icon_handle == icon_handle) )
        {   /* restore caret */
            void_WrapOsErrorReporting(
                tbl_wimp_set_caret_position(caret.window_handle, caret.icon_handle,
                                            caret.xoffset, caret.yoffset,
                                            caret.height, caret.index));
        }
    }
}

static void
set_icon_colours(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    _InVal_     U32 fg_colour_option_index,
    _InVal_     U32 bg_colour_option_index)
{
    const unsigned int colour_bits =
        (WimpIcon_FGColour * wimp_colour_index_from_option(fg_colour_option_index)) |
        (WimpIcon_BGColour * wimp_colour_index_from_option(bg_colour_option_index));
    const unsigned int colour_mask =
        (WimpIcon_FGColour * 0xFU) |
        (WimpIcon_BGColour * 0xFU);
    BOOL change_value;

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    change_value = ((icon_state.icon.flags & colour_mask) != colour_bits);

    if(!change_value)
    {
        trace_0(TRACE_APP_PD4, "colours not changed");
        return;
    }
    } /*block*/

    {
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word = (int) colour_bits;
    set_icon_state_block.clear_word = (int) colour_mask;
    void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
}

static void
set_icon_flags(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    int value,
    int mask)
{
    BOOL change_value;

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    change_value = ((icon_state.icon.flags & mask) != value);

    if(!change_value)
    {
        trace_0(0/*TRACE_APP_PD4*/, "icon flags not changed");
        return;
    }
    } /*block*/

    {
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word = value;
    set_icon_state_block.clear_word = mask;
    void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
}

extern void
set_icon_text(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    _In_z_      const char * text)
{
    BOOL change_text;

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    change_text = (0 != strcmp(icon_state.icon.data.it.buffer, text));

    if(change_text)
        strcpy(icon_state.icon.data.it.buffer, text);

    if(!change_text)
        return;
    } /*block*/

    { /* redraw iff changed */
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word = 0;
    set_icon_state_block.clear_word = 0;
    void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
}

static void
set_icon_text_and_flags(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    _In_z_      const char * text,
    int value,
    int mask)
{
    BOOL change_text;
    BOOL change_value;

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    change_text = (0 != strcmp(icon_state.icon.data.it.buffer, text));

    if(change_text)
        strcpy(icon_state.icon.data.it.buffer, text);

    change_value = ((icon_state.icon.flags & mask) != value);

    if(!change_text && !change_value)
        return;
    } /*block*/

    { /* redraw iff changed */
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word = value;
    set_icon_state_block.clear_word = mask;
    void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
}

/******************************************************************************
*
* Called by riscos_create_document_window (riscos.c) and cache_mode_variables(riscdraw.c)
* to tweak the positions/colours/flags etc. of some of the icons in the colh window.
* Some of the state is taken from the template file, some is calculated and set by this routine.
*
******************************************************************************/

extern void
colh_position_icons(void)
{
    WimpCreateIconBlock heading;
    WimpCreateIconBlock coordinate;
    WimpCreateIconBlock border;

    /* Position the column heading 'click detection' icon.        */
    /* This invisible icon exists to make click detection easier, */
    /* and acts as a guide for plotting the headings & margins.   */

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = heading.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_COLUMN_HEADINGS;
    void_WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state));
    heading.icon = icon_state.icon;
    } /*block*/

    /* heading bbox.ymin & bbox.ymax taken from template file, other fields set here */
    heading.icon.bbox.xmin = texttooffset_x(COLUMNHEAD_t_X0 + borderwidth);
    heading.icon.bbox.xmax = 0x3FFF;
    /* use       bbox.ymin   from template */
    /* use       bbox.ymin   from template */
    heading.icon.flags = (int) (wimp_IBTYPE * wimp_BCLICKDRAGDOUBLE);

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = border.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_STATUS_BORDER;
    void_WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state));
    border.icon = icon_state.icon;
    } /*block*/

    border.icon.bbox.xmin = texttooffset_x(CELLCOORDS_t_X0);
    border.icon.bbox.ymin = heading.icon.bbox.ymin;
    border.icon.bbox.xmax = texttooffset_x(CELLCOORDS_t_X1 + borderwidth);
    border.icon.bbox.ymax = heading.icon.bbox.ymax;

    border.icon.flags = (int) (
        wimp_IBORDER | (WimpIcon_FGColour * wimp_colour_index_from_option(COI_GRID))       |
        wimp_IFILLED | (WimpIcon_BGColour * wimp_colour_index_from_option(COI_BORDERBACK)) |
        wimp_IRJUST );

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = coordinate.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_STATUS_TEXT;
    void_WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state));
    coordinate.icon = icon_state.icon;
    } /*block*/

    coordinate.icon.bbox.xmin = border.icon.bbox.xmin;
    coordinate.icon.bbox.ymin = border.icon.bbox.ymin + dy;
    coordinate.icon.bbox.xmax = border.icon.bbox.xmax - dx;
    coordinate.icon.bbox.ymax = border.icon.bbox.ymax - dy;

    coordinate.icon.flags = (int) (
        coordinate.icon.flags & ~(wimp_ITEXT | wimp_IFILLED));
    coordinate.icon.flags = (int) (
        coordinate.icon.flags & ~((WimpIcon_FGColour * 0xFU) |
                                  (WimpIcon_BGColour * 0xFU) |
                                   wimp_IHCENTRE) );
    coordinate.icon.flags = (int) (
        coordinate.icon.flags | ((WimpIcon_FGColour * wimp_colour_index_from_option(COI_CURRENT_BORDERFORE)) |
                                 (WimpIcon_BGColour * wimp_colour_index_from_option(COI_CURRENT_BORDERBACK)) |
                                  wimp_IRJUST) );
    /* rest of flags should be wimp_IHCENTRE | wimp_IVCENTRE | wimp_INDIRECT */

    redefine_icon(colh_window_handle, COLH_COLUMN_HEADINGS, &heading);
    redefine_icon(colh_window_handle, COLH_STATUS_BORDER, &border);
    redefine_icon(colh_window_handle, COLH_STATUS_TEXT, &coordinate);

    colh_draw_edit_state_indicator();

    /* The contents/formula line is maintained by openpane. */
}

extern void
colh_colour_icons(void)
{
    set_icon_colours(colh_window_handle, COLH_STATUS_BORDER, COI_GRID              , COI_BORDERBACK        );
    set_icon_colours(colh_window_handle, COLH_STATUS_TEXT  , COI_CURRENT_BORDERFORE, COI_CURRENT_BORDERBACK);
}

/******************************************************************************
*
*                       display the column headings
*
******************************************************************************/

extern void
colh_draw_column_headings(void)
{
    WimpGetIconStateBlock icon_state;

    xf_drawcolumnheadings = FALSE;      /* unset my redraw flag */

    if(!displaying_borders)
        return;

    trace_0(TRACE_APP_PD4, "\n*** colh_draw_column_headings()");

    icon_state.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_COLUMN_HEADINGS;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    please_update_window(colh_forced_draw_column_headings, colh_window_handle,
                         icon_state.icon.bbox.xmin, icon_state.icon.bbox.ymin,
                         icon_state.icon.bbox.xmax, icon_state.icon.bbox.ymax);
}

extern void
colh_forced_draw_column_headings(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block)
{
    UNREFERENCED_PARAMETER_InRef_(redraw_window_block);

    trace_0(TRACE_APP_PD4, "colh_forced_draw_column_headings()");

    colh_really_draw_column_headings();
}

static void
colh_maybe_draw_column_headings(void)
{
    trace_0(TRACE_APP_PD4, "colh_maybe_draw_column_headings()");

    if(!displaying_borders)
        return;

    /*>>>Do source level clipping here*/
    colh_really_draw_column_headings();
}

static void
colh_really_draw_column_headings(void)
{
    P_SCRCOL cptr;
    S32 coff;
    COL colno;
    S32 cwid;

    WimpIconBlock border;
    WimpIconBlock number;
    GDI_COORD window_width;

    trace_0(TRACE_APP_PD4, "colh_really_draw_column_headings()");

    { /* get the window width */
    WimpGetWindowStateBlock window_state;
    window_state.window_handle = colh_window_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
        return;
    window_width = BBox_width(&window_state.visible_area);
    } /*block*/

    window_width += charwidth; /* i.e. comfortably wider than window */

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_COLUMN_HEADINGS;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;
    border = icon_state.icon;
    } /*block*/

    number.bbox.xmin = border.bbox.xmin;
    number.bbox.ymin = border.bbox.ymin + dy;
    number.bbox.ymax = border.bbox.ymax - dy;

    coff = 0;

    for(; !((cptr = horzvec_entry(coff))->flags & LAST); ++coff)
    {
        COLOURS_OPTION_INDEX fg_colours_option_index, bg_colours_option_index;

        if(window_width <= number.bbox.xmin)
            break; /* off the right */

        colno = cptr->colno;
        cwid = colwidth(colno);

        border.bbox.xmin = number.bbox.xmin - dx;
        border.bbox.xmax = number.bbox.xmin + cwid*charwidth;
        number.bbox.xmax = border.bbox.xmax - dx;

        if(colno == curcol)
        {
            fg_colours_option_index = COI_CURRENT_BORDERFORE;
            bg_colours_option_index = COI_CURRENT_BORDERBACK;
        }
        else
        {
            fg_colours_option_index = COI_BORDERFORE;
            bg_colours_option_index = COI_BORDERBACK;

            if(incolfixes(colno))
                bg_colours_option_index = COI_FIXED_BORDERBACK;
        }

        border.flags = (int) (
            wimp_IBORDER | (WimpIcon_FGColour * wimp_colour_index_from_option(COI_GRID)) ); /*COI_BORDERFORE*/

        number.flags = (int) (
            wimp_ITEXT |
            wimp_IHCENTRE | wimp_IVCENTRE |
          /*wimp_IBORDER |*/ (WimpIcon_FGColour * wimp_colour_index_from_option(fg_colours_option_index)) |
            wimp_IFILLED |   (WimpIcon_BGColour * wimp_colour_index_from_option(bg_colours_option_index)) );

        (void) write_col(number.data.t, elemof32(number.data.t), colno);

        if(NULL != WrapOsErrorReporting(tbl_wimp_plot_icon(&number)))
            break;

        if(NULL != WrapOsErrorReporting(tbl_wimp_plot_icon(&border)))
            break;

        number.bbox.xmin = border.bbox.xmax;
    }

    /* Clear rest of column heading line upto right edge of window */

    if(window_width > number.bbox.xmin)
    {
        border.bbox.xmin = number.bbox.xmin - dx;
        border.bbox.xmax = window_width;

        border.flags = (int) (
            /*wimp_ITEXT |*/
            /*wimp_IHCENTRE | wimp_IVCENTRE |*/
            wimp_IBORDER | (WimpIcon_FGColour * wimp_colour_index_from_option(COI_GRID)) | /*COI_BORDERFORE*/
            wimp_IFILLED | (WimpIcon_BGColour * wimp_colour_index_from_option(COI_BORDERBACK)) );

        void_WrapOsErrorReporting(tbl_wimp_plot_icon(&border));
    }

    /* Draw the right margin indicator (a down arrow) for the current column */

    number.bbox.xmax = texttooffset_x(calcad(curcoloffset) + colwrapwidth(curcol));
    number.bbox.xmin = number.bbox.xmax - 2 * charwidth;

    if(window_width > number.bbox.xmin)
    {
        number.flags = (int) (wimp_ISPRITE | wimp_IHCENTRE | wimp_IVCENTRE);

#if FALSE
        number.flags |= wimp_IBORDER;     /* Useful for debugging */
#endif

        number.flags = (int) (number.flags |
                              (WimpIcon_FGColour * wimp_colour_index_from_option(colislinked(curcol)
                                                                             ? COI_CURRENT_BORDERFORE
                                                                             : COI_BORDERFORE)) );

        xstrkpy(number.data.s, elemof32(number.data.s), "pd4downarro");

        void_WrapOsErrorReporting(tbl_wimp_plot_icon(&number));
    }

    killcolourcache(); /* cos Wimp_PlotIcon changes foreground and background colours */
    setcolours(COI_FORE, COI_BACK);
}

/******************************************************************************
*
* display contents of current number cell on the contents line
*
******************************************************************************/

extern void
colh_draw_contents_of_number_cell(void)
{
    char      text[LIN_BUFSIZ];
    P_CELL     tcell;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    text[0] = CH_NULL;

    tcell = travel_here();

    if((NULL != tcell)  &&  (tcell->type == SL_NUMBER))
        prccon(text, tcell); /* decompile tcell into text */

    set_icon_text(colh_window_handle, COLH_CONTENTS_LINE, text);
}

extern void
colh_draw_drag_state(
    int size)
{
    if(size >= 0)
    {
        char array[32];
        consume_int(sprintf(array, "%d", size));
        draw_status_box_no_interlock(array);
    }
    else
        draw_status_box_no_interlock(NULL); /* Finished, so put the silly bar back */

}

extern void
colh_draw_edit_state_indicator(void)
{
    int shade = (xf_inexpression_line ? 0 : wimp_INOSELECT);

    set_icon_flags(colh_window_handle, COLH_BUTTON_OK    , shade, wimp_INOSELECT);
    set_icon_flags(colh_window_handle, COLH_BUTTON_CANCEL, shade, wimp_INOSELECT);
}

extern void
colh_draw_mark_state_indicator(
    BOOL sheet_marked)
{
    int invert = (sheet_marked ? wimp_ISELECTED : 0);

    colh_mark_state_indicator = sheet_marked;

    set_icon_flags(colh_window_handle, COLH_BUTTON_MARK, invert, wimp_ISELECTED);
}

extern void
colh_draw_cell_count(
    _In_opt_z_      char *text)
{
    draw_status_box(text);
}

extern void
colh_draw_cell_count_in_document(
    _In_opt_z_      char *text)
{
    P_DOCU p_docu;

    if(slotcount_docno == DOCNO_NONE)
        return;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(slotcount_docno)))  ||  docu_is_thunk(p_docu))
        slotcount_docno = DOCNO_NONE;
    else
    {
        DOCNO old_docno = current_docno();

        select_document(p_docu);

        colh_draw_cell_count(text);

        select_document_using_docno(old_docno);
    }
}

static void
draw_status_box(
    _In_opt_z_      char *text)
{
    /* Don't widdle on status box if dragging, cos the drag code uses it */
    /* to show the column width/right margin whilst dragging them.       */

    if((drag_type == DRAG_COLUMN_WIDTH) || (drag_type == DRAG_COLUMN_WRAPWIDTH))
        return;

    draw_status_box_no_interlock(text);
}

static void
draw_status_box_no_interlock(
    _In_opt_z_      char *text)
{
    if(text)
    {
#if TRUE
        set_icon_text_and_flags(colh_window_handle, COLH_STATUS_TEXT, text,
                                                                (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));

#else
      /*set_icon_flags(colh_window_handle, COLH_STATUS_TEXT  , (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));*/
        set_icon_text(colh_window_handle , COLH_STATUS_TEXT  , text);

        set_icon_flags(colh_window_handle, COLH_STATUS_TEXT  , (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));
#endif
    }
    else
    {
        set_icon_flags(colh_window_handle, COLH_STATUS_TEXT  , (                          0), (wimp_ITEXT   | wimp_IFILLED));
        set_icon_flags(colh_window_handle, COLH_STATUS_BORDER, (               wimp_IFILLED), (               wimp_IFILLED));
    }
}

/******************************************************************************
*
* process redraw window request for colh window
*
******************************************************************************/

static BOOL
colh_event_Redraw_Window_Request(
    const WimpRedrawWindowRequestEvent * const redraw_window_request)
{
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;

    trace_0(TRACE_APP_PD4, "colh_Redraw_Window_Request()");

    redraw_window_block.window_handle = redraw_window_request->window_handle;
    assert(redraw_window_block.window_handle == colh_window_handle);

    /* wimp errors in redraw are fatal */
    if(NULL != WrapOsErrorReporting(tbl_wimp_redraw_window(&redraw_window_block, &more)))
        more = FALSE;

    while(more)
    {
        graphics_window = * ((PC_GDI_BOX) &redraw_window_block.redraw_area);

        colh_maybe_draw_column_headings();

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&redraw_window_block, &more)))
            more = FALSE;
    }

    return(TRUE);
}

/******************************************************************************
*
* process key pressed event for colh window
*
******************************************************************************/

static BOOL
colh_event_Key_Pressed(
    /*mutated*/ wimp_eventstr * const e,
    const WimpKeyPressedEvent * const key_pressed)
{
    BOOL processed = TRUE;

    switch(key_pressed->key_code)
    {
    case 13:
        if(host_ctrl_pressed())
        {
            expedit_transfer_line_to_box(TRUE);     /* force newline */
            break;
        }

        formline_mergebacktext(FALSE, NULL);        /* don't report formula compilation errors */
        break;                                      /* don't want to know where the caret was  */

    case 27:
        formline_cancel_edit();
        break;

    case akbd_UpK:                  /* up arrow   */
    case akbd_DownK:                /* down arrow */
    case akbd_TabK:                 /* tab        */
    case akbd_TabK + akbd_Sh:       /* shift tab  */
    case akbd_TabK + akbd_Ctl:      /* ctrl tab   */
    case akbd_TabK + akbd_Sh + akbd_Ctl: /* ctrl shift tab */
        formline_mergebacktext(FALSE, NULL);        /* MUST NOT report formula compilation errors, */
                                                    /* cos we want event block intact to send the  */
                                                    /* key to PipeDream's main window              */
#if FALSE
    case 388:       /* f4 - search     */
    case 390:       /* f6 - next match */
#endif
        e->data.key.c.w = main_window_handle;
        e->data.key.c.i = 0;
        wimpt_fake_event(e);                        /* send key to PipeDream's main window */
        break;

    default:
#if TRUE
        /* can't test for F4/F6 for search/next-match cos user may have changed the keys, so try... */
        if(e->data.key.chcode >= 256)
        {
            e->data.key.c.w = main_window_handle;
            e->data.key.c.i = 0;
            wimpt_fake_event(e);            /* ...sending all likely looking keys to PipeDream's main window */
            break;
        }
#endif
        processed = FALSE;
        break;
    }

    return(processed);
}

/******************************************************************************
*
* process caret lose event for colh window
*
******************************************************************************/

static BOOL
colh_event_Lose_Caret(
    const WimpLoseCaretEvent * const lose_caret)
{
    trace_6(TRACE_APP_PD4, "colh_event_handler - Lose_Caret: old window %d icon %d x %d y %d height %8.8X index %d",
            lose_caret->window_handle, lose_caret->icon_handle,
            lose_caret->xoffset, lose_caret->yoffset,
            lose_caret->height, lose_caret->index);

    if( xf_inexpression_line  &&
        (lose_caret->window_handle == colh_window_handle) &&
        (lose_caret->icon_handle == COLH_CONTENTS_LINE) &&
        (lose_caret->index >= 0)
      )
    {
        trace_0(TRACE_APP_PD4, "colh_event_handler - stashing caret position");

        /* Someone is stealing the caret from the contents/formula-line,
         * stash position away so it can be restored when PipeDream reclaims the caret.
         * Also means we can paste formula etc. into formula line at correct place.
         */
        formline_stashedcaret = lose_caret->index;
    }

    return(FALSE /*processed*/);
}

/******************************************************************************
*
* process caret gain event for colh window
*
******************************************************************************/

static BOOL
colh_event_Gain_Caret(void)
{
    /* This document is gaining the caret, and will show the cell count (recalculation status) from now on */
    if(slotcount_docno != current_docno())
    {
        colh_draw_cell_count_in_document(NULL); /* kill the current indicatior (if any) */

        slotcount_docno = current_docno();
    }

    return(TRUE);
}

/******************************************************************************
*
* process events sent to colh window
*
* c.f. main_event_handler & rear_event_handler in c.riscos
*
******************************************************************************/

#define colh_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "colh")

extern BOOL
colh_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;

    if(!select_document_using_callback_handle(handle))
    {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to colh event handler"), report_ptr_cast(handle));
        return(FALSE);
    }

    trace_4(TRACE_APP_PD4, TEXT("colh_event_handler: event %s, handle ") PTR_XTFMT TEXT(" window %d, document ") PTR_XTFMT,
             report_wimp_event(event_code, event_data), report_ptr_cast(handle), (int) colh_window_handle, report_ptr_cast(current_document()));

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_Redraw_Window_Request(&event_data->redraw_window_request));

    case Wimp_EOpenWindow:    /* colh window always opened as a pane on top of rear window */
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(TRUE);

    case Wimp_ECloseWindow:   /* colh window has no close box */
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(TRUE);

    case Wimp_EPointerEnteringWindow:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        if(!drag_in_column_header)
            colh_pointershape_starttracking();
        return(TRUE);

    case Wimp_EPointerLeavingWindow:                /* When we start dragging, the wimp sends a pointer-leaving-window-event, */
        /*colh_event_handler_report(event_code, event_data, handle);*/
        if(!drag_in_column_header)                  /* we ignore this, so the pointer retains its dragcolumn or dragmargin    */
            colh_pointershape_endtracking();        /* shape. When the drag has finshed we remove the tracker, the wimp may   */
        return(TRUE);                               /* then send a pointer-entering-window-event.                             */

    case Wimp_EScrollRequest: /* colh window has no scroll bars  */
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(TRUE);

    case Wimp_EMouseClick:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_Mouse_Click(&event_data->mouse_click));

    case Wimp_EKeyPressed:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_Key_Pressed(e, &event_data->key_pressed));

    case Wimp_ELoseCaret:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_Lose_Caret(&event_data->lose_caret));

    case Wimp_EGainCaret:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_Gain_Caret());

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        return(colh_event_User_Message(&event_data->user_message));

    default:
        /*colh_event_handler_report(event_code, event_data, handle);*/
        trace_1(TRACE_APP_PD4, "unprocessed wimp event to colh window: %s",
                report_wimp_event(event_code, event_data));
        return(FALSE);
    }
}

/******************************************************************************
*
* A little novel entitled 'Why has that pink duck got one leg'
*
* Chapter 1: A note on wimp button-click types
*
* +----------------------------+-----------------------------------+
* |                            | Return codes from clicking select |
* | Button type                |  Once  |  Twice   |  Once & hold  |
* |----------------------------+--------+----------+---------------|
* |  3 Click                   |    &4  |   &4  &4 |     &4        |
* |  4 Click/Drag              |    &4  |   &4  &4 |     &4  &40   |
* | 10 Double-click/Click/Drag |  &400  | &400  &4 |   &400  &40   |
* | 14 Write/Click/Drag        |    &4  |   &4  &4 |     &4  &40   |
* +----------------------------+--------+----------+---------------+
*
*
* Chapter 2: Button types used for the colh window
*   The window background is        Double-click/Click/Drag,
*   the editable contents line is   Write/Click/Drag,
*   the OK and Cancel buttons are   Click
*
*
* Chapter 3: Bend it until it breaks
*   Life seems easier if we fake the click events from non 'Double-click/Click/Drag'
*   buttons to look like the first click from a 'Double-click/Click/Drag' button.
*
******************************************************************************/

static BOOL             /* new stuff */
colh_event_Mouse_Click(
    const WimpMouseClickEvent * const mouse_click)
{
    static BOOL initially_select_clicked = FALSE;

    const int icon_handle = mouse_click->icon_handle;
    int buttons = mouse_click->buttons;
    BOOL select_clicked;

    trace_4(TRACE_APP_PD4,
            "colh_event_Mouse_Click: (%d, %d) buttons %X, icon %d",
            mouse_click->mouse_x, mouse_click->mouse_y, buttons, icon_handle);

    /* ensure we can find cell for positioning, overlap tests etc. must allow spellcheck as we may move */
    if(!mergebuf())
        return(TRUE);
    filbuf();
    /* but guaranteed that we can simply slot_in_buffer = FALSE for movement */

    if(icon_handle == COLH_COLUMN_HEADINGS)
    {
        /* Have to cope with Pink 'Plonker' Duck Man's ideas on double clicks (fixed in new RISC OS+):
         * He says that left then right (or vice versa) is a double click!
         */
        if(buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust))
        {
            select_clicked = ((buttons & Wimp_MouseButtonSelect) != 0);

            /* must be same button that started us off */
            if(initially_select_clicked != select_clicked)
                buttons = select_clicked ? Wimp_MouseButtonSingleSelect : Wimp_MouseButtonSingleAdjust; /* force a single click */
        }

        if(     buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust))
        {
            select_clicked = ((buttons & Wimp_MouseButtonSelect) != 0);

            return(colh_event_Mouse_Click_double(mouse_click, select_clicked));
        }
        else if(buttons & (Wimp_MouseButtonDragSelect | Wimp_MouseButtonDragAdjust))
        {
            select_clicked = ((buttons & Wimp_MouseButtonDragSelect) != 0);

            return(colh_event_Mouse_Click_start_drag(mouse_click, select_clicked));
        }
        else if(buttons & (Wimp_MouseButtonSingleSelect | Wimp_MouseButtonSingleAdjust))
        {
            select_clicked = ((buttons & Wimp_MouseButtonSingleSelect) != 0);

            initially_select_clicked = select_clicked;

            return(colh_event_Mouse_Click_single(mouse_click, select_clicked));
        }
    }
    else
    {
        if(buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust))
        {
            select_clicked = ((buttons & Wimp_MouseButtonSelect) != 0);

            initially_select_clicked = select_clicked;

            return(colh_event_Mouse_Click_single(mouse_click, select_clicked));
        }
    }

    return(FALSE);
}

static BOOL             /* new stuff */
colh_event_Mouse_Click_single(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked)
{
    DOCNO docno   = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL) && (docno == blk_docno);
    BOOL acquire  = FALSE;                                        /* don't want caret on block marking operations */
    BOOL motion   = FALSE;
    BOOL extend   = !select_clicked || host_shift_pressed();          /* unshifted-adjust or shift-anything */
    S32  funcnum = 0;
    S32  highlight_number = -1;

    trace_0(TRACE_APP_PD4, "colh_event_Mouse_Click_single");

    switch(mouse_click->icon_handle)
    {
    case COLH_BUTTON_OK:
        if(select_clicked)
        {
            formline_mergebacktext(FALSE, NULL);        /* don't report formula compilation errors */
            break;                                      /* don't want to know where the caret was  */
        }
        funcnum = N_EditFormulaInWindow;
        break;

    case COLH_BUTTON_CANCEL:
        if(select_clicked)
            formline_cancel_edit();
        break;

    case COLH_CONTENTS_LINE:
        if(select_clicked)
            EditContentsLine();
        break;

    case COLH_FUNCTION_SELECTOR:
        if(select_clicked)
        {   /* should be handled by function menu code */
            assert0();
            break;
        }
        funcnum = N_EditFormulaInWindow;
        break;

    case COLH_BUTTON_REPLICD:
        funcnum = (select_clicked) ? N_ReplicateDown: N_ReplicateUp;
        break;

    case COLH_BUTTON_REPLICR:
        funcnum = (select_clicked) ? N_ReplicateRight : N_ReplicateLeft;
        break;

    case COLH_BUTTON_TOTEXT:
        funcnum = (select_clicked) ? N_ToText : N_ExchangeNumbersText;
        break;

    case COLH_BUTTON_TONUMBER:
        funcnum = (select_clicked) ? N_ToNumber : N_ToConstant;
        break;

    case COLH_BUTTON_GRAPH:
        funcnum = (select_clicked) ? N_ChartNew : N_ChartOptions;
        break;

    case COLH_BUTTON_SAVE:
        funcnum = (select_clicked) ? N_SaveFile : N_SaveFileAsIs;
        break;

    case COLH_BUTTON_PRINT:
        funcnum = (select_clicked) ? N_Print : N_PageLayout;
        break;

    case COLH_BUTTON_LJUSTIFY:
        funcnum = (select_clicked) ? N_LeftAlign : N_FreeAlign;
        break;

    case COLH_BUTTON_CJUSTIFY:
        funcnum = (select_clicked) ? N_CentreAlign : 0;
        break;

    case COLH_BUTTON_RJUSTIFY:
        funcnum = (select_clicked) ? N_RightAlign : 0;
        break;

    case COLH_BUTTON_FJUSTIFY:
        {
        if(select_clicked)
        {
            if(d_options_JU == 'Y')
                d_options_JU = 'N';
            else
            {
                d_options_JU = 'Y';
                d_options_WR = 'Y';
            }

            update_variables();
        }

        break;
        }

    case COLH_BUTTON_FONT:
        funcnum = (select_clicked) ? N_PRINTERFONT : N_INSERTFONT;
        break;

    case COLH_BUTTON_BOLD:
        funcnum = N_Bold;
        highlight_number = HIGH_BOLD;
        break;

    case COLH_BUTTON_ITALIC:
        funcnum = N_Italic;
        highlight_number = HIGH_ITALIC;
        break;

    case COLH_BUTTON_UNDERLINED:
        funcnum = N_Underline;
        highlight_number = HIGH_UNDERLINE;
        break;

    case COLH_BUTTON_SUBSCRIPT:
        funcnum = N_Subscript;
        highlight_number = HIGH_SUBSCRIPT;
        break;

    case COLH_BUTTON_SUPERSCRIPT:
        funcnum = N_Superscript;
        highlight_number = HIGH_SUPERSCRIPT;
        break;

    case COLH_BUTTON_LEADTRAIL:
        funcnum = (select_clicked) ? N_LeadingCharacters : N_TrailingCharacters;
        break;

    case COLH_BUTTON_DECPLACES:
        funcnum = (select_clicked) ? N_DecimalPlaces : N_DefaultFormat;
        break;

    case COLH_BUTTON_COPY:
        funcnum = (select_clicked) ? N_CopyBlockToPasteList : N_MoveBlock;
        break;

    case COLH_BUTTON_DELETE:
        funcnum = (select_clicked) ? N_DeleteBlock : N_ClearBlock;
        break;

    case COLH_BUTTON_PASTE:
        funcnum = (select_clicked) ? N_Paste : 0;
        break;

    case COLH_BUTTON_FORMATBLOCK:
        funcnum = (select_clicked) ? N_FormatBlock : 0;
        break;

    case COLH_BUTTON_SEARCH:
        funcnum = (select_clicked) ? N_Search : N_NextMatch;
        break;

    case COLH_BUTTON_SORT:
        funcnum = (select_clicked) ? N_SortBlock : N_TransposeBlock;
        break;

    case COLH_BUTTON_SPELLCHECK:
        funcnum = (select_clicked) ? N_CheckDocument : N_BrowseDictionary;
        break;

    case COLH_BUTTON_CMD_RECORD:
        funcnum = (select_clicked) ? N_RecordMacroFile : 0;
        break;

    case COLH_BUTTON_CMD_EXEC:
        funcnum = (select_clicked) ? N_DoMacroFile : 0;
        break;

    case COLH_BUTTON_MARK:
    case COLH_STATUS_TEXT:
        /* if editing, suppress setting/clearing of marks */
        if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
            break;

        if(select_clicked && !blkindoc)
        {
            trace_0(TRACE_APP_PD4, "click on cell coordinates - mark entire sheet");
            funcnum = N_MarkSheet;
        }
        else
        {
            if(!select_clicked)
                colh_draw_mark_state_indicator(blkindoc); /* may need to re-invert as Window Manager has already set the icon state */
            trace_0(TRACE_APP_PD4, "click on cell coordinates - clear markers");
            funcnum = N_ClearMarkedBlock;
        }
        break;

    case COLH_COLUMN_HEADINGS:
        {
        pointer_shape * shape;
        int   subposition;
        COL  tcol;
        coord tx;

        /* if editing, suppress (curcol,currow) movement and selection dragging etc. */
        if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        {
            acquire = !extend;
            break;
        }

        colh_where_in_column_headings(mouse_click, &shape, &subposition, &tcol, &tx);
        riscos_setpointershape(shape);

        switch(subposition)
        {
        case OVER_COLUMN_CENTRE:
            if(extend)
            {
                /* either alter current block or set new block:
                 * mergebuf has been done by caller to ensure cell marking correct
                 */
                if(blkindoc)
                {
                    trace_0(TRACE_APP_PD4, "alter number of marked columns");
                    make_single_mark_into_block();
                    alter_marked_block(tcol, ACTIVE_ROW);
                }
                else
                {
                    trace_0(TRACE_APP_PD4, "mark all rows from caret to given column");
#if 1 /* SKS */
                    set_marked_block(curcol, numrow-1, tcol, 0, TRUE);
#else
                    set_marked_block(curcol, 0, tcol, numrow-1, TRUE);
#endif
                }
            }
            else
            {
                trace_0(TRACE_APP_PD4, "not extending");

                if(tcol != curcol)
                {
                    /* position caret in new column (same row); mergebuf has been done by caller */
                    slot_in_buffer = FALSE;
                    chknlr(tcol, currow);
                    lescrl = lecpos = 0;
                    motion = TRUE;
                }

                acquire = TRUE;
            }

            break;

        default:
            acquire = TRUE;
            break;
        }

        break;
        }

    } /* switch(icon) */

    if(highlight_number >= 0)
    {
        /* is there a marked block in this document? */
        if((NO_COL != blkstart.col) && (blk_docno == current_docno()))
        {
            /* can't think of a kosher way to do this */ /* and why doesn't it get redrawn? */
            block_highlight_core(!select_clicked, highlight_number);

            /* populate the dialogue box option in case we then wish to remove! */
            d_inshigh[0].option = (highlight_number - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;

            funcnum = 0;
        }
        else
        {
            if(!select_clicked)
                funcnum = 0;
        }
    }

    if(0 != funcnum)
        application_process_command(funcnum);

    /* reselect document e.g. N_PRINTERFONT screws us up */
    select_document_using_docno(docno);

    if(acquire)
        xf_caretreposition = TRUE;

    if(xf_caretreposition  ||  motion  ||  (highlight_number >= 0))
    {
        draw_screen();
        draw_caret();
    }

    return(TRUE);
}

static BOOL             /* new stuff */
colh_event_Mouse_Click_double(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked)
{
    BOOL extend = !select_clicked || host_shift_pressed();          /* unshifted-Adjust or shift-anything */

    trace_0(TRACE_APP_PD4, "colh_event_Mouse_Click_double");

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  /* everything suppressed whilst editing */
        return(TRUE);

    switch(mouse_click->icon_handle)
    {
    case COLH_COLUMN_HEADINGS:
        {
        pointer_shape * shape;
        int subposition;
        COL tcol;
        coord tx;

        colh_where_in_column_headings(mouse_click, &shape, &subposition, &tcol, &tx);
        riscos_setpointershape(shape);

        if(extend)
            return(TRUE);

        switch(subposition)
        {
        case OVER_COLUMN_MARGIN_ADJUSTOR:
            {
            P_S32 widp, wwidp;

            trace_0(TRACE_APP_PD4, "On the right margin arrow - linking margin to column width");

            readpcolvars(tcol, &widp, &wwidp);
            *wwidp = 0;

            if(colislinked(tcol))
                adjust_this_linked_column(tcol);

            xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
            filealtered(TRUE);

            draw_screen();

            break;
            }

        case OVER_COLUMN_WIDTH_ADJUSTOR:
            {
            application_process_command(N_AutoWidth);
            break;
            }

        case OVER_COLUMN_CENTRE:
            trace_0(TRACE_APP_PD4, "in column headings - mark column");
#if 1 /* SKS */
            set_marked_block(tcol, numrow-1, tcol, 0, TRUE);
#else
            set_marked_block(tcol, 0, tcol, numrow-1, TRUE);
#endif
            break;

        default:
            trace_0(TRACE_APP_PD4, "off left/right of column headings - ignored");
            break;

        } /* switch(subposition) */

        break;
        }

    } /* switch(icon) */

    return(TRUE);
}

static BOOL             /* new stuff */
colh_event_Mouse_Click_start_drag(
    const WimpMouseClickEvent * const mouse_click,
    BOOL select_clicked)
{
    DOCNO docno   = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL) && (docno == blk_docno);
    BOOL extend   = !select_clicked || host_shift_pressed();          /* unshifted-adjust or shift-anything */

    trace_0(TRACE_APP_PD4, "colh_event_Mouse_Click_start_drag");

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  /* everything suppressed whilst editing */
        return(TRUE);

    switch(mouse_click->icon_handle)
    {
    case COLH_COLUMN_HEADINGS:
        {
        pointer_shape * shape;
        int subposition;
        COL tcol;
        coord tx;
        coord ty = -1; /*>>>don't really know this - suck it and see!*/

        colh_where_in_column_headings(mouse_click, &shape, &subposition, &tcol, &tx);
        riscos_setpointershape(shape); /* Must do this, incase colh_pointershape_null_handler hasn't been called yet. */

        switch(subposition)
        {
        case OVER_COLUMN_MARGIN_ADJUSTOR:
            trace_0(TRACE_APP_PD4, "Dragging the right margin");
            prepare_for_drag_column_wrapwidth(tx, tcol, extend);        /* extend means align margins of columns to */
            start_drag(DRAG_COLUMN_WRAPWIDTH);                          /* our right as well                        */
            break;

        case OVER_COLUMN_WIDTH_ADJUSTOR:
            trace_1(TRACE_APP_PD4, "Dragging width of column %d", tcol);
#if FALSE
            slot_in_buffer = FALSE;     /* dragging tends to change curcol; mergebuf has been done by caller */
#endif
            prepare_for_drag_column_width(tx, tcol);
            start_drag(DRAG_COLUMN_WIDTH);
            break;

        case OVER_COLUMN_CENTRE:
            trace_1(TRACE_APP_PD4, "In middle of column %d, ", tcol);

            /* mark all rows over given columns */
            if(blkindoc && extend)
            {
                trace_0(TRACE_APP_PD4, "in column headings - continuing all rows mark");
                make_single_mark_into_block();
            }
            else
            {
                trace_0(TRACE_APP_PD4, "in column headings - starting all rows mark");
#if 1 /* SKS */
                prepare_for_drag_mark(tx, ty, tcol, numrow-1, tcol, 0);
#else
                prepare_for_drag_mark(tx, ty, tcol, 0, tcol, numrow-1);
#endif
            }

            start_drag(MARK_ALL_ROWS);
            break;

        default:
            trace_0(TRACE_APP_PD4, "off left/right of column headings - ignored");
            break;

        } /* switch(subposition) */
        break;
        }

    } /* switch(icon) */

    return(TRUE);
}

/******************************************************************************
*
* Called when user clicks on the editable cell-contents line.
*
* The contents line is a writable icon, so when clicked on,
* receives the caret, whether it wants it, or not!.
*
******************************************************************************/

static void
EditContentsLine(void)
{
    WimpCaret caret;
    int index = 0;

    /* If already editing the contents line, a click repositions the caret, */
    /* this is done totally by the wimp, so requires no action by us.       */

    if(xf_inexpression_line)
        return;

    /* If using another editor, ignore the click and give the caret back. */

    if(xf_inexpression || xf_inexpression_box)
    {
        xf_acquirecaret = TRUE;
        return;
    }

    /* Start editing the formula, place caret where user clicked */

    if(NULL == WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
    {
        if( (caret.window_handle == colh_window_handle) &&
            (caret.icon_handle == COLH_CONTENTS_LINE) &&
            (caret.index >= 0) )
        {
            index = caret.index;
        }
    }

    expedit_edit_current_cell(index, FALSE); /* either in contents line or formula box as appropriate */
}

/******************************************************************************
*
* Process message events for colh window
*
******************************************************************************/

static BOOL
colh_event_Message_HelpRequest(
    /*acked*/ WimpMessage * const user_message)
{
    const int icon_handle = user_message->data.help_request.icon_handle;
    char abuffer[256+128/*bit of slop*/];
    U32 prefix_len;
    char * buffer;
    char * alt_msg;
    const char * msg = NULL;

    trace_0(TRACE_APP_PD4, "Help request on colh window");

    if(drag_type != NO_DRAG_ACTIVE) /* stop pointer and message changing whilst dragging */
        return(TRUE);

    #if 1
    /* Terse messages for better BubbleHelp response */
    abuffer[0] = CH_NULL;
    prefix_len = 0;
    #else
    /* default message */
    xstrkpy(abuffer, elemof32(abuffer), help_main_window);
    prefix_len = strlen(abuffer);
    #endif

    alt_msg = buffer = abuffer + prefix_len;        /* remember a possible cut place */

    if(xf_inexpression_box)
        msg = help_colh_inexpression_box;
    else if(xf_inexpression_line)
    {
        switch(icon_handle)
        {
        case -1:
            break;

        case COLH_BUTTON_OK:
            msg = colh_button_edit_ok;
            break;

        case COLH_BUTTON_CANCEL:
            msg = colh_button_edit_cancel;
            break;

        case COLH_CONTENTS_LINE:
            msg = help_colh_contents_line_inexpression;
            break;

        case COLH_FUNCTION_SELECTOR:
            msg = help_colh_transfer_to_window;
            break;

        case COLH_BUTTON_MARK:
        case COLH_STATUS_TEXT:
            /* if editing, suppress setting/clearing of marks */
            break;

        case COLH_COLUMN_HEADINGS:
            /* if editing, all movement and drags are suppressed */
            break;

        default:
            msg = help_colh_inexpression_line;
            break;
        }
    }
    else
    {
        switch(icon_handle)
        {
        case -1:
            break;

        case COLH_BUTTON_OK:
            /* no action when not editing */
            break;

        case COLH_BUTTON_CANCEL:
            /* no action when not editing */
            break;

        case COLH_CONTENTS_LINE:
            msg = help_colh_contents_line;
            break;

        case COLH_FUNCTION_SELECTOR:
            msg = help_colh_edit_in_window;
            break;

        case COLH_STATUS_TEXT:
            /* if editing, suppress setting/clearing of marks */
            if(xf_inexpression)
                break;

            msg = help_top_left_corner;
            break;

        case COLH_COLUMN_HEADINGS:
            {
            pointer_shape * shape;
            int subposition;
            COL tcol;
            char cbuf[32];

            /* if editing, all movement and drags are suppressed */
            if(xf_inexpression)
                break;

            colh_where_in_column_headings((const WimpMouseClickEvent *) &user_message->data.help_request, &shape, &subposition, &tcol, NULL);
            riscos_setpointershape(shape);

            switch(subposition)
            {
            case OVER_COLUMN_CENTRE:
                msg = help_col_border;
                break;

            case OVER_COLUMN_MARGIN_ADJUSTOR:
                (void) write_col(cbuf, elemof32(cbuf), tcol);

                (void) xsnprintf(buffer, elemof32(abuffer) - prefix_len,
                        "%s%s.|M%s",
                        help_drag_right_margin,
                        cbuf,
                        help_double_right_margin
                       );
                break;

            case OVER_COLUMN_WIDTH_ADJUSTOR:
                (void) write_col(cbuf, elemof32(cbuf), tcol);

                (void) xsnprintf(buffer, elemof32(abuffer) - prefix_len,
                        "%s%s.|M",
                        help_drag_column_width,
                        cbuf
                       );
                break;
            }

            break;
            }

        case COLH_BUTTON_REPLICD:
            msg = colh_button_replicd;
            break;

        case COLH_BUTTON_REPLICR:
            msg = colh_button_replicr;
            break;

        case COLH_BUTTON_TOTEXT:
            msg = colh_button_totext;
            break;

        case COLH_BUTTON_TONUMBER:
            msg = colh_button_tonumber;
            break;

        case COLH_BUTTON_GRAPH:
            msg = colh_button_graph;
            break;

        case COLH_BUTTON_SAVE:
            msg = colh_button_save;
            break;

        case COLH_BUTTON_PRINT:
            msg = colh_button_print;
            break;

        case COLH_BUTTON_LJUSTIFY:
            msg = colh_button_ljustify;
            break;

        case COLH_BUTTON_CJUSTIFY:
            msg = colh_button_cjustify;
            break;

        case COLH_BUTTON_RJUSTIFY:
            msg = colh_button_rjustify;
            break;

        case COLH_BUTTON_FJUSTIFY:
            msg = colh_button_fjustify;
            break;

        case COLH_BUTTON_FONT:
            msg = colh_button_font;
            break;

        case COLH_BUTTON_BOLD:
            msg = colh_button_bold;
            break;

        case COLH_BUTTON_ITALIC:
            msg = colh_button_italic;
            break;

        case COLH_BUTTON_UNDERLINED:
            msg = colh_button_underlined;
            break;

        case COLH_BUTTON_SUBSCRIPT:
            msg = colh_button_subscript;
            break;

        case COLH_BUTTON_SUPERSCRIPT:
            msg = colh_button_superscript;
            break;

        case COLH_BUTTON_LEADTRAIL:
            msg = colh_button_leadtrail;
            break;

        case COLH_BUTTON_DECPLACES:
            msg = colh_button_decplaces;
            break;

        case COLH_BUTTON_MARK:
            msg = colh_button_mark;
            break;

        case COLH_BUTTON_COPY:
            msg = colh_button_copy;
            break;

        case COLH_BUTTON_DELETE:
            msg = colh_button_delete;
            break;

        case COLH_BUTTON_PASTE:
            msg = colh_button_paste;
            break;

        case COLH_BUTTON_FORMATBLOCK:
            msg = colh_button_formatblock;
            break;

        case COLH_BUTTON_SEARCH:
            msg = colh_button_search;
            break;

        case COLH_BUTTON_SORT:
            msg = colh_button_sort;
            break;

        case COLH_BUTTON_SPELLCHECK:
            msg = colh_button_spellcheck;
            break;

        case COLH_BUTTON_CMD_RECORD:
            msg = colh_button_cmd_record;
            break;

        case COLH_BUTTON_CMD_EXEC:
            msg = colh_button_cmd_exec;
            break;
        }
    }

    if(msg)
        xstrkpy(buffer, elemof32(abuffer) - prefix_len, msg);

    #if FALSE
    if(append_drag_msg)
        xstrkat(abuffer, elemof32(abuffer), help_drag_file_to_insert);
    #endif

    riscos_send_Message_HelpReply(user_message, (strlen32p1(abuffer) <= 236) ? abuffer : alt_msg);

    return(TRUE);
}

static BOOL
colh_event_User_Message(
    /*poked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MHelpRequest:
        return(colh_event_Message_HelpRequest(user_message));

    default:
        return(FALSE);
    }
}

extern void
colh_where_in_column_headings(
    const WimpMouseClickEvent * const mouse_click,
    pointer_shape **shapep,
    int *subpositionp,
    P_COL p_col,
    coord *txp)
{
    GDI_POINT gdi_org;
    int rel_x, rel_y;
    int arrowcentre;
    pointer_shape *shape = POINTER_DEFAULT;
    int drag_type = OVER_COLUMN_DEAD_SPACE;
    coord drag_tx = 0;
    COL drag_tcol = 0;
    coord tx;

    { /* calculate window origin */
    WimpGetWindowStateBlock window_state;
    window_state.window_handle = mouse_click->window_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
        goto had_error;
    gdi_org.x = window_state.visible_area.xmin - window_state.xscroll;
    gdi_org.y = window_state.visible_area.ymax - window_state.yscroll;
    } /*block*/

    rel_x = mouse_click->mouse_x - gdi_org.x;
    rel_y = mouse_click->mouse_y - gdi_org.y;

    tx = idiv_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0), charwidth);

    /* The arrow shown belongs to curcol (even if it is a linked column) */
    arrowcentre = texttooffset_x(calcad(curcoloffset) + colwrapwidth(curcol) - 1);

    if(((arrowcentre - charwidth/2) <= rel_x) && (rel_x < (arrowcentre + charwidth/2))
#if TRUE
        /* switch this out to allow linked right margins to be dragged */
        && !colislinked(curcol)
#endif
      )
    {
        trace_0(TRACE_APP_PD4, "Over right margin marker");

        shape     = POINTER_DRAGMARGIN;

        drag_type = OVER_COLUMN_MARGIN_ADJUSTOR;
        drag_tx   = tx;
        drag_tcol = curcol;

#if FALSE
        /* switch this in to allow linked right margins to be dragged */

        /* We want to return the rightmost column of a linked range, so its right margin */
        /* is directly modified, those to the left are then calculated from it.          */

        if(colislinked(drag_tcol))
        {
            COL scanend = numcol - 1;

            while(linked_column_linkright(drag_tcol) && (drag_tcol < scanend))
                ++drag_tcol;
        }
#endif
    }
    else
    {       /* 'over right margin marker' test failed */

        gcoord hitband   = charwidth/2;
        coord  coff      = calcoff(tx);
        coord  prev_tx   = idiv_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0) - hitband, charwidth);
        coord  prev_coff = calcoff(prev_tx);

        drag_tcol = 0; /* SKS 29sep96 paranoia */
        drag_tx   = 0;

        if(horzvec_entry_valid(coff))
        {
            COL tcol = col_number(coff);

            if((tcol > 0) && (prev_coff != coff))
            {
                trace_2(TRACE_APP_PD4, "At left of column %d, ready to drag width of column %d", tcol, tcol-1);

                drag_type = OVER_COLUMN_WIDTH_ADJUSTOR;
                drag_tx   = tx;
                drag_tcol = tcol-1;

                shape     = (colwidth(drag_tcol) == 0 ? POINTER_DRAGCOLUMN_RIGHT : POINTER_DRAGCOLUMN);
            }
            else
            {
                coord next_tx = idiv_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0) + hitband, charwidth);

                if(coff != calcoff(next_tx))
                {
                    trace_2(TRACE_APP_PD4, "At right of column %d, ready to drag width of column %d", tcol, tcol);

                    shape     = POINTER_DRAGCOLUMN;

                    drag_type = OVER_COLUMN_WIDTH_ADJUSTOR;
                    drag_tx   = tx;
                    drag_tcol = tcol;
                }
                else
                {
                    trace_1(TRACE_APP_PD4, "Over middle of column %d, ", tcol);

                    shape     = POINTER_DEFAULT;    /*>>>might be nice to use another shape */

                    drag_type = OVER_COLUMN_CENTRE;
                    drag_tx   = tx;
                    drag_tcol = tcol;
                }
            }
        }
        else
        {
            if(horzvec_entry_valid(prev_coff))
            {
                COL prev_tcol = col_number(prev_coff); /* exists, is on screen, slightly to left of drag point */
                COL tcol      = prev_tcol + 1;         /* assume there is a zero width col we should drag      */

                if( (tcol < numcol) &&                  /* if it exists */
                    (colwidth(tcol) == 0)
                  )
                {
                    shape     = POINTER_DRAGCOLUMN_RIGHT;

                    drag_type = OVER_COLUMN_WIDTH_ADJUSTOR;
                    drag_tx   = tx;
                    drag_tcol = tcol;
                }
                else
                {
                    shape     = POINTER_DRAGCOLUMN;

                    drag_type = OVER_COLUMN_WIDTH_ADJUSTOR;
                    drag_tx   = tx;
                    drag_tcol = prev_tcol;
                }

                trace_1(TRACE_APP_PD4, "off right of column headings, ready to drag width of column %d", drag_tcol);
            }
            else
                trace_0(TRACE_APP_PD4, "off left/right of column headings - ignored");
        }
    }

had_error:;
    if(shapep)
        *shapep = shape;

    if(subpositionp)
        *subpositionp = drag_type;

    if(p_col)
        *p_col = drag_tcol;

    if(txp)
        *txp = drag_tx;
}

extern void
colh_pointershape_drag_notification(
    BOOL start)
{
    drag_in_column_header = start;

    if(start)
        trace_0(TRACE_APP_PD4, "colh_pointershape_drag_notification - STARTING");
    else
    {
        trace_0(TRACE_APP_PD4, "colh_pointershape_drag_notification - ENDING");

        colh_pointershape_endtracking();
    }
}

static void
colh_pointershape_starttracking(void)
{
    track_docno = current_docno();

    trace_1(TRACE_APP_PD4, "colh_pointershape_starttracking(): track_docno=%u", track_docno);

    status_assert(Null_EventHandlerAdd(colh_pointershape_null_handler, &track_docno, 0));
}

/******************************************************************************
*
* Called the when pointer leaves the colh window.
*
* Remove the null_handler and restore the default pointer shape.
*
******************************************************************************/

static void
colh_pointershape_endtracking(void)
{
    trace_1(TRACE_APP_PD4, "colh_pointershape_endtracking(): track_docno=%u", track_docno);

    Null_EventHandlerRemove(colh_pointershape_null_handler, &track_docno);

    riscos_setpointershape(POINTER_DEFAULT);
}

/******************************************************************************
*
* Call-back from null engine.
*
******************************************************************************/

null_event_proto(static, colh_pointershape_null_query)
{
    UNREFERENCED_PARAMETER_CONST(p_null_event_block);

    trace_0(TRACE_APP_PD4, "colh call to ask if we want nulls");

    return( (drag_type == NO_DRAG_ACTIVE)
                        ? NULL_EVENTS_REQUIRED
                        : NULL_EVENTS_OFF );
}

null_event_proto(static, colh_pointershape_null_event)
{
    P_DOCNO dochanp = (P_DOCNO) p_null_event_block->client_handle;
    wimp_mousestr pointer;

    trace_0(TRACE_APP_PD4, "colh null call");

    if(!select_document_using_docno(*dochanp))
        return(NULL_EVENT_COMPLETED);

    if(NULL != wimp_get_point_info(&pointer))
        return(NULL_EVENT_COMPLETED);

    /*(if(pointer.w == colh_window_handle)*/ /* should be true, as we release null events when pointer leaves window */
    {
        switch((int)pointer.i)
        {
        case COLH_COLUMN_HEADINGS:
            /* if editing, all movement and drags are suppressed, so show default pointer */
            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
            {
                riscos_setpointershape(POINTER_DEFAULT);
            }
            else
            {
                pointer_shape * shape;
                colh_where_in_column_headings((const WimpMouseClickEvent *) &pointer, &shape, NULL, NULL, NULL);
                riscos_setpointershape(shape);
            }
            break;

        #if 0
        case COLH_FUNCTION_SELECTOR:
            trace_0(TRACE_APP_PD4, "change pointer to drop-down-menu");
            riscos_setpointershape(POINTER_DROPMENU);
            break;
        #endif

        default:
            trace_0(TRACE_APP_PD4, "restore default pointer");
            riscos_setpointershape(POINTER_DEFAULT);
            break;
        }
    }

    return(NULL_EVENT_COMPLETED);
}

null_event_proto(static, colh_pointershape_null_handler)
{
    trace_1(TRACE_APP_PD4, "colh_pointershape_null_handler, rc=%d", p_null_event_block->rc);

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(colh_pointershape_null_query(p_null_event_block));

    case NULL_EVENT:
        return(colh_pointershape_null_event(p_null_event_block));

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

/******************************************************************************
*
* Change pointer shape to that of a given sprite.
*
* If shape == NULL the RISC OS default pointer shape is selected.
*
******************************************************************************/

pointer_shape pointer_dropmenu         = { "ptr_menu"   , 0 , 0 };
pointer_shape pointer_dragcolumn       = { "ptr_column" , 12, 5 };
pointer_shape pointer_dragcolumn_right = { "ptr_colum_r", 12, 5 };
pointer_shape pointer_dragmargin       = { "ptr_margin" , 12, 5 };

static pointer_shape *current_pointer_shape = POINTER_DEFAULT;

extern void
riscos_setpointershape(
    pointer_shape * shape)
{
    if(current_pointer_shape == shape)
        return;

    riscos_hourglass_off();

    if(shape)
    {
        sprite_id sprite;

        sprite.tag = sprite_id_name;
        sprite.s.name = shape->name;

        void_WrapOsErrorReporting(pointer_set_shape((sprite_area *)resspr_area() /* -1 */, &sprite, shape->x, shape->y));
    }
    else
    {
        /* POINTER_DEFAULT */

        pointer_reset_shape();
    }

    current_pointer_shape = shape;

    riscos_hourglass_on();
}

/* end of colh.c */
