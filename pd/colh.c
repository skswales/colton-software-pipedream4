/* colh.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Column heading and contents line code for PD4 */

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
colh_draw_contents_of_numslot(void);

extern void
colh_draw_drag_state(
    int size);

extern void
colh_draw_edit_state_indicator(void);

extern void
colh_draw_mark_state_indicator(
    BOOL sheet_marked);

extern void
colh_draw_slot_count(
    char *text);

extern void
colh_draw_slot_count_in_document(
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
set_icon_text(
    wimp_w window,
    wimp_i icon,
    char *text);

extern void
setpointershape(
    pointer_shape *shape);

static void
colh_forced_draw_column_headings(
    RISCOS_REDRAWSTR *r);

static void
colh_really_draw_column_headings(void);

static void
draw_status_box(
    _In_opt_z_      char *text);

static void
draw_status_box_no_interlock(
    _In_opt_z_      char *text);

static void
application_button_click_in_colh(
    wimp_mousestr *pointer);

static void
application_singleclick_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked);

static void
application_doubleclick_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked);

static void
application_startdrag_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked);

static void
colh_message(
    wimp_msgstr * m);

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
    wimp_w window,
    wimp_i icon,
    wimp_icreate *create)
{
    wimp_icon current;
    wimp_get_icon_info(window, icon, &current);

    if((create->i.box.x0 != current.box.x0) ||
       (create->i.box.x1 != current.box.x1) ||
       (create->i.box.y0 != current.box.y0) ||
       (create->i.box.y1 != current.box.y1) ||
       (create->i.flags  != current.flags)
      )
        {
        /* icon size or flags are wrong, so... */

        wimp_redrawstr redraw;
        wimp_caretstr  carrot;
        wimp_i         newhandle;

        /* must force-redraw max of current box and required box as resize may scroll the text */

        redraw.w = window;
        redraw.box.x0 = MIN(create->i.box.x0, current.box.x0);
        redraw.box.x1 = MAX(create->i.box.x1, current.box.x1);
        redraw.box.y0 = MIN(create->i.box.y0, current.box.y0);
        redraw.box.y1 = MAX(create->i.box.y1, current.box.y1);

        wimpt_noerr(wimp_force_redraw(&redraw));

        wimp_get_caret_pos(&carrot);
        if((carrot.w == window) && (carrot.i == icon))
            {
            wimp_caretstr killit;

            killit.w = (wimp_w)-1;
            killit.i = (wimp_i)-1;
            killit.x = killit.y = killit.index = 0;
            killit.height = 0x02000000;

            wimp_set_caret_pos(&killit);
            }

        /* delete current definition */
        wimpt_complain(wimp_delete_icon(window, icon));

        /* and re-create icon with required size */
        create->w = window;
        wimpt_complain(wimp_create_icon(create, &newhandle));
        assert(icon == newhandle);              /* MUST retain same icon number */

        if((carrot.w == window) && (carrot.i == icon))
            wimp_set_caret_pos(&carrot);

        }
}

static void
set_icon_colours(
    wimp_w window,
    wimp_i icon,
    int fgcol,
    int bgcol)
{
    wimp_icon  info;
    unsigned int colbits = (wimp_IFORECOL * logcol(fgcol) + wimp_IBACKCOL * logcol(bgcol));
    unsigned int colmask = (wimp_IFORECOL * 0xFU + wimp_IBACKCOL * 0xFU);

    wimp_get_icon_info(window, icon, &info);

    if((info.flags & colmask) != colbits)
        wimp_set_icon_state(window, icon, (wimp_iconflags) colbits, (wimp_iconflags) colmask);
    else
        trace_0(TRACE_APP_PD4, "colours not changed");
}

static void
set_icon_flags(
    wimp_w window,
    wimp_i icon,
    int value,
    int mask)
{
    wimp_icon  info;

    wimp_get_icon_info(window, icon, &info);

    if((info.flags & mask) != value)
        wimp_set_icon_state(window, icon, (wimp_iconflags) value, (wimp_iconflags) mask);
#if FALSE
    else
        trace_0(TRACE_APP_PD4, "icon flags not changed");
#endif
}

extern void
set_icon_text(
    wimp_w window,
    wimp_i icon,
    char *text)
{
    wimp_icon info;

    wimp_get_icon_info(window, icon, &info);
                /* really GetIconState, but some twat gave it the wrong name */

    if(0 != strcmp(info.data.indirecttext.buffer, text))
        {
        /* redraw iff changed */
        strcpy(info.data.indirecttext.buffer, text);

        wimp_set_icon_state(window, icon, (wimp_iconflags) 0, (wimp_iconflags) 0);
        }
}

static void
set_icon_text_and_flags(
    wimp_w window,
    wimp_i icon,
    char *text,
    int value,
    int mask)
{
    wimp_icon info;
    BOOL      newtext;

    wimp_get_icon_info(window, icon, &info);
                /* really GetIconState, but some twat gave it the wrong name */

    newtext = strcmp(info.data.indirecttext.buffer, text);

    if(newtext || ((info.flags & mask) != value))
        {
        /* redraw iff changed */
        if(newtext)
            strcpy(info.data.indirecttext.buffer, text);

        wimp_set_icon_state(window, icon, (wimp_iconflags) value, (wimp_iconflags) mask);
        }
}

/******************************************************************************
*
* Called by riscos_createmainwindow (c.riscos)
* and cachemodevariables(c.riscdraw) to tweek the
* positions/colours/flags etc of some of the icons
* in the colh_window. Some of the state is taken
* from the template file, some is calculated and
* set by this routine.
*
******************************************************************************/

extern void
colh_position_icons(void)
{
    wimp_icreate heading;
    wimp_icreate coordinate;
    wimp_icreate border;
    wimp_icreate bar;

    /* Position the column heading 'click detection' icon.        */
    /* This invisible icon exists to make click detection easier, */
    /* and acts as a guide for plotting the headings & margins.   */

    /* box.y0 & box.y1 taken from template file, other fields set here */

    wimp_get_icon_info(colh_window, (wimp_i)COLH_COLUMN_HEADINGS, &heading.i);

    heading.w        = colh_window;
    heading.i.box.x0 = texttooffset_x(COLUMNHEAD_t_X0 + borderwidth);
    heading.i.box.x1 = 0x3FFF;
    /* use    box.y0   from template */
    /* use    box.y1   from template */
    heading.i.flags  = (wimp_iconflags) (wimp_IBTYPE * wimp_BCLICKDRAGDOUBLE);

#if FALSE
    /* so we can see it for debugging purposes */
    heading.i.flags |= wimp_IBORDER | (wimp_IFORECOL * logcol(BORDERFOREC));
#endif

    redefine_icon(colh_window, (wimp_i)COLH_COLUMN_HEADINGS, &heading);

    wimp_get_icon_info(colh_window, (wimp_i)COLH_STATUS_BORDER, &border.i);

    border.w        = colh_window;
    border.i.box.x0 = texttooffset_x(SLOTCOORDS_t_X0);
    border.i.box.x1 = texttooffset_x(SLOTCOORDS_t_X1 + borderwidth);
    border.i.box.y0 = heading.i.box.y0;
    border.i.box.y1 = heading.i.box.y1;

    border.i.flags = (wimp_iconflags) (wimp_IBORDER | wimp_IFILLED |
                                      (wimp_IFORECOL * logcol(GRIDC))       |
                                      (wimp_IBACKCOL * logcol(BORDERBACKC)) |
                                       wimp_IRJUST);

    wimp_get_icon_info(colh_window, (wimp_i)COLH_STATUS_TEXT, &coordinate.i);

    coordinate.w        = colh_window;
    coordinate.i.box.x0 = border.i.box.x0;
    coordinate.i.box.x1 = border.i.box.x1 - dx;
    coordinate.i.box.y0 = border.i.box.y0 + dy;
    coordinate.i.box.y1 = border.i.box.y1 - dy;

    coordinate.i.flags = (wimp_iconflags) (coordinate.i.flags &
                            ~(wimp_ITEXT | wimp_IFILLED));
    coordinate.i.flags = (wimp_iconflags) (coordinate.i.flags &
                            ~((wimp_IFORECOL * 0xFU) +
                              (wimp_IBACKCOL * 0xFU) +
                               wimp_IHCENTRE));
    coordinate.i.flags = (wimp_iconflags) (coordinate.i.flags |
                           ((wimp_IFORECOL * logcol(CURBORDERFOREC)) +
                            (wimp_IBACKCOL * logcol(CURBORDERBACKC)) +
                             wimp_IRJUST));
    /* rest of flags should be wimp_IHCENTRE | wimp_IVCENTRE | wimp_INDIRECT */

    redefine_icon(colh_window, (wimp_i)COLH_STATUS_BORDER, &border);
    redefine_icon(colh_window, (wimp_i)COLH_STATUS_TEXT, &coordinate);

    wimp_get_icon_info(colh_window, (wimp_i)COLH_STATUS_BAR, &bar.i);

    bar.w        = colh_window;
#if TRUE
    /* RCM & MRJC think these (original) dimensions look better */
    bar.i.box.x0 = dx * 6;
    bar.i.box.x1 = border.i.box.x1 - dx * 7;
#else
    bar.i.box.x0 = border.i.box.x0 + charwidth;           /*dx * 6;*/
    bar.i.box.x1 = border.i.box.x1 - 3*2 - dx/*mode indep. + mode dep.*/; /*dx * 7;*/
#endif
    bar.i.box.y0 = border.i.box.y0 + dy * 2;
    bar.i.box.y1 = border.i.box.y1 - dy * 2;

    bar.i.flags = (wimp_iconflags) (
                  wimp_IBORDER | (wimp_IFORECOL * logcol(FORE))    |
                  wimp_IFILLED | (wimp_IBACKCOL * logcol(BACK))    |
                  (colh_mark_state_indicator ? wimp_ISELECTED : 0) |
                  (wimp_IBTYPE * wimp_BCLICKDEBOUNCE) );

    redefine_icon(colh_window, (wimp_i)COLH_STATUS_BAR, &bar);

    colh_draw_edit_state_indicator();

    /* The contents/formula line is maintained by openpane. */
}

extern void
colh_colour_icons(void)
{
    int invert = (colh_mark_state_indicator ? wimp_ISELECTED : 0);

    set_icon_colours(colh_window, (wimp_i)COLH_STATUS_BORDER, GRIDC         , BORDERBACKC   );
    set_icon_colours(colh_window, (wimp_i)COLH_STATUS_TEXT  , CURBORDERFOREC, CURBORDERBACKC);
    set_icon_colours(colh_window, (wimp_i)COLH_STATUS_BAR   , FORE          , BACK          );
    set_icon_flags  (colh_window, (wimp_i)COLH_STATUS_BAR   , invert        , wimp_ISELECTED);
}

/******************************************************************************
*
*                       display the column headings
*
******************************************************************************/

extern void
colh_draw_column_headings(void)
{
    wimp_icon icon;

    xf_drawcolumnheadings = FALSE;      /* unset my redraw flag */

    if(!borbit)
        return;

    trace_0(TRACE_APP_PD4, "\n*** colh_draw_column_headings()");

    wimp_get_icon_info(colh_window, (wimp_i)COLH_COLUMN_HEADINGS, &icon);

    please_update_window(colh_forced_draw_column_headings, colh_window,
                         icon.box.x0, icon.box.y0, icon.box.x1, icon.box.y1);
}

extern void
colh_forced_draw_column_headings(
    RISCOS_REDRAWSTR *r)
{
    IGNOREPARM(r);

    trace_0(TRACE_APP_PD4, "colh_forced_draw_column_headings()");

    colh_really_draw_column_headings();
}

static void
colh_maybe_draw_column_headings(void)
{
    trace_0(TRACE_APP_PD4, "colh_maybe_draw_column_headings()");

    if(!borbit)
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

    wimp_wstate wind;
    wimp_icon   border;
    wimp_icon   number;
    int fgcol, bgcol;
    S32 wind_width;

    trace_0(TRACE_APP_PD4, "colh_really_draw_column_headings()");

    wimp_get_wind_state(colh_window, &wind);

    wimp_get_icon_info(colh_window, (wimp_i)COLH_COLUMN_HEADINGS, &border);

    number.box.x0 = border.box.x0;
    number.box.y0 = border.box.y0 + dy;
    number.box.y1 = border.box.y1 - dy;

    wind_width = (wind.o.box.x1 - wind.o.box.x0) + charwidth;   /* ie comfortably wider than window */

    coff = 0;

    for(; !((cptr = horzvec_entry(coff))->flags & LAST)  &&  (wind_width > number.box.x0); ++coff)
        {
        colno = cptr->colno;
        cwid = colwidth(colno);

        border.box.x0 = number.box.x0 - dx;
        border.box.x1 = number.box.x0 + cwid*charwidth;
        number.box.x1 = border.box.x1 - dx;

        if(colno == curcol)
            {
            fgcol = CURBORDERFOREC;
            bgcol = CURBORDERBACKC;
            }
        else
            {
            fgcol = BORDERFOREC;
            bgcol = BORDERBACKC;

            if(incolfixes(colno))
                bgcol = FIXBORDERBACKC;
            }

        border.flags = (wimp_iconflags) (wimp_IBORDER | (wimp_IFORECOL * logcol(GRIDC))); /*BORDERFOREC*/

        number.flags = (wimp_iconflags) ( wimp_ITEXT /*| wimp_IBORDER*/ | wimp_IHCENTRE | wimp_IVCENTRE | wimp_IFILLED
                     | (wimp_IFORECOL * logcol(fgcol))
                     | (wimp_IBACKCOL * logcol(bgcol)) );

        (void) write_col(number.data.text, elemof32(number.data.text), colno);

        wimp_ploticon(&number);
        wimp_ploticon(&border);

        number.box.x0 = border.box.x1;
        }

    /* Clear rest of column heading line upto right edge of window */

    if(wind_width > number.box.x0)
        {
        border.box.x0 = number.box.x0 - dx;
        border.box.x1 = wind_width;

        border.flags = (wimp_iconflags) ( /*wimp_ITEXT |*/ wimp_IBORDER | /*wimp_IHCENTRE | wimp_IVCENTRE |*/ wimp_IFILLED
                     | (wimp_IFORECOL * logcol(GRIDC))         /*BORDERFOREC*/
                     | (wimp_IBACKCOL * logcol(BORDERBACKC)) );

        wimp_ploticon(&border);
        }

    /* Draw the right margin indicator (a down arrow) for the current column */

    number.box.x1 = texttooffset_x(calcad(curcoloffset) + colwrapwidth(curcol));
    number.box.x0 = number.box.x1 - 2 * charwidth;

    if(wind_width > number.box.x0)
        {
      /*number.flags = wimp_ITEXT | wimp_IBORDER | wimp_IHCENTRE | wimp_IVCENTRE | wimp_IFILLED;*/
        number.flags = (wimp_iconflags) (wimp_ITEXT | wimp_IHCENTRE | wimp_IVCENTRE);

#ifndef POUNDS_BLEEDING_RJM
        number.flags = (wimp_iconflags) (wimp_ISPRITE  | wimp_IHCENTRE | wimp_IVCENTRE);
#else
        number.flags = (wimp_iconflags) (wimp_ITEXT | wimp_IHCENTRE | wimp_IVCENTRE);
#endif

#if FALSE
        number.flags |= wimp_IBORDER;     /* Useful for debugging */
#endif

        number.flags = (wimp_iconflags) (number.flags |
            (colislinked(curcol)
                ? (wimp_IFORECOL * logcol(CURBORDERFOREC))
                : (wimp_IFORECOL * logcol(BORDERFOREC)) ));  /*FORE*/

#ifndef POUNDS_BLEEDING_RJM
        xstrkpy(number.data.sprite_name, elemof32(number.data.sprite_name), "pd4downarro");
#else
        number.data.text[0] = 138;      /* a down arrow */
        number.data.text[1] = 0;
#endif
        wimp_ploticon(&number);
        }

    killcolourcache();          /* cos wimp_ploticon changes foreground and background */
    setcolour(FORE, BACK);      /* colours under our feet!                             */
}

/******************************************************************************
*
*           display contents of current numeric slot on the contents line
*
******************************************************************************/

extern void
colh_draw_contents_of_numslot(void)
{
    char      text[LIN_BUFSIZ];
    P_SLOT     tslot;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    tslot = travel_here();

    if(!tslot  ||  (tslot->type == SL_TEXT)  ||  (tslot->type == SL_PAGE))
        *text = '\0';
    else
        prccon(text, tslot);   /* decompile tslot into text */

    set_icon_text(colh_window, (wimp_i)COLH_CONTENTS_LINE, text);
}

extern void
colh_draw_drag_state(
    int size)
{
    if(size >= 0)
        {
        char array[32];

        (void) sprintf(array, "%d", size);

        draw_status_box_no_interlock(array);
        }
    else
        draw_status_box_no_interlock(NULL);     /* Finished, so put the silly bar back */

}

extern void
colh_draw_edit_state_indicator(void)
{
    int shade = (xf_inexpression_line ? 0 : wimp_INOSELECT);

    set_icon_flags(colh_window, (wimp_i)COLH_BUTTON_OK    , shade, wimp_INOSELECT);
    set_icon_flags(colh_window, (wimp_i)COLH_BUTTON_CANCEL, shade, wimp_INOSELECT);
}

extern void
colh_draw_mark_state_indicator(
    BOOL sheet_marked)
{
    int invert = (sheet_marked ? wimp_ISELECTED : 0);

    colh_mark_state_indicator = sheet_marked;

    set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BAR, invert, wimp_ISELECTED);
}

extern void
colh_draw_slot_count(
    _In_opt_z_      char *text)
{
    draw_status_box(text);
}

extern void
colh_draw_slot_count_in_document(
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

        colh_draw_slot_count(text);

        select_document_using_docno(old_docno);
        }

}

static void
draw_status_box(
    _In_opt_z_      char *text)
{
    /* Don't widdle on status box if dragging, cos the drag code uses it */
    /* to show the column width/right margin whilst dragging them.       */

    if((dragtype == DRAG_COLUMN_WIDTH) || (dragtype == DRAG_COLUMN_WRAPWIDTH))
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
        set_icon_text_and_flags(colh_window, (wimp_i)COLH_STATUS_TEXT, text,
                                                                (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));

#else
      /*  set_icon_flags(colh_window, (wimp_i)COLH_STATUS_TEXT  , (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));*/
        set_icon_text(colh_window , (wimp_i)COLH_STATUS_TEXT  , text);

      /*set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BAR   , (                          0), (wimp_IBORDER | wimp_IFILLED));*/
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_TEXT  , (wimp_ITEXT   | wimp_IFILLED), (wimp_ITEXT   | wimp_IFILLED));
#endif
#if FALSE
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BAR   , (                          0), (wimp_IBORDER | wimp_IFILLED));
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BORDER, (                          0), (               wimp_IFILLED));
#endif
        }
    else
        {
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_TEXT  , (                          0), (wimp_ITEXT   | wimp_IFILLED));
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BORDER, (               wimp_IFILLED), (               wimp_IFILLED));
        set_icon_flags(colh_window, (wimp_i)COLH_STATUS_BAR   , (wimp_IBORDER | wimp_IFILLED), (wimp_IBORDER | wimp_IFILLED));
        }
}

/******************************************************************************
*
* process redraw window request for colh_window
*
******************************************************************************/

static void
colh_redraw_request(
    wimp_eventstr *e)
{
    os_error * bum;
    wimp_redrawstr redraw;
    S32 redrawindex;

    trace_0(TRACE_APP_PD4, "colh_redraw_request()");

    redraw.w = e->data.o.w;     /* should be colh_window */

    /* wimp errors in redraw are fatal */
    bum = wimpt_complain(wimp_redraw_wind(&redraw, &redrawindex));

#if TRACE_ALLOWED
    if(!redrawindex)
        trace_0(TRACE_APP_PD4, "no rectangles to redraw");
#endif

    while(!bum  &&  redrawindex)
        {
      /*graphics_window = *((coord_box *) &redraw.g);*/

        colh_maybe_draw_column_headings();

        bum = wimpt_complain(wimp_get_rectangle(&redraw, &redrawindex));
        }
}

/******************************************************************************
*
* process events sent to colh_window
*
* c.f. main_event_handler & rear_event_handler in c.riscos
*
******************************************************************************/

extern BOOL
colh_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    BOOL processed = TRUE;

    if(!select_document_using_callback_handle(handle))
    {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to colh event handler"), report_ptr_cast(handle));
        return(FALSE);
    }

    trace_4(TRACE_APP_PD4, TEXT("colh_event_handler: event %s, handle ") PTR_XTFMT TEXT(" window %d, document ") PTR_XTFMT,
             report_wimp_event(e->e, &e->data), report_ptr_cast(handle), (int) colh_window, report_ptr_cast(current_document()));

    switch(e->e)
        {
        case wimp_EOPEN:    /* colh_window always opened as a pane on top of rear_window */
        case wimp_ECLOSE:   /* colh_window has no close box */
        case wimp_ESCROLL:  /* or scroll bars  */
            break;

        case wimp_EPTRENTER:
            if(!drag_in_column_header)
                colh_pointershape_starttracking();
            break;

        case wimp_EPTRLEAVE:                            /* When we start dragging, the wimp sends a pointer-leaving-window-event, */
            if(!drag_in_column_header)                  /* we ignore this, so the pointer retains its dragcolumn or dragmargin    */
                colh_pointershape_endtracking();        /* shape. When the drag has finshed we remove the tracker, the wimp may   */
            break;                                      /* then send a pointer-entering-window-event.                             */

        case wimp_EREDRAW:
            colh_redraw_request(e);
            break;

        case wimp_EBUT:
            application_button_click_in_colh(&(e->data.but.m));
            break;

        case wimp_EKEY:
            switch(e->data.key.chcode)
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
                    formline_canceledit();
                    break;

                case akbd_UpK:                  /* up arrow   */
                case akbd_DownK:                /* down arrow */
                case akbd_TabK:                 /* tab        */
                case akbd_TabK + akbd_Sh:       /* shift tab  */
                case akbd_TabK + akbd_Ctl:      /* ctrl tab   */
                case akbd_TabK + akbd_Sh + akbd_Ctl: /* ctrl shift tab */
                    formline_mergebacktext(FALSE, NULL);        /* MUST NOT report formula compilation errors, */
                                                                /* cos we want event block intact to send the  */
                                                                /* key to PipeDream's main_window              */
#if FALSE
                case 388:       /* f4 - search     */
                case 390:       /* f6 - next match */
#endif
                    e->data.key.c.w = main_window;
                    e->data.key.c.i = (wimp_i)0;
                    wimpt_fake_event(e);                        /* send key to PipeDream's main_window */
                    break;

                default:
#if TRUE
                    /* can't test for f4/f6 for search/next-match cos user may have changed the keys, so try... */
                    if(e->data.key.chcode >= 256)
                        {
                        e->data.key.c.w = main_window;
                        e->data.key.c.i = (wimp_i)0;
                        wimpt_fake_event(e);            /* ...sending all likely looking keys to PipeDream's main_window */
                        break;
                        }
#endif
                    processed = FALSE;
                    break;
                }
            break;

        case wimp_EGAINCARET:
            /* This document is gaining the caret, and will show the slot count (recalculation status) from now on */
            if(slotcount_docno != current_docno())
                {
                colh_draw_slot_count_in_document(NULL); /* kill the current indicatior (if any) */

                slotcount_docno = current_docno();
                }
            break;

        case wimp_ELOSECARET:
            trace_6(TRACE_APP_PD4, "colh_event_handler - LoseCaret: old window %d icon %d x %d y %d height %8.8X index %d",
                    e->data.c.w, e->data.c.i,
                    e->data.c.x, e->data.c.y,
                    e->data.c.height, e->data.c.index);

            if(xf_inexpression_line         &&
               (e->data.c.w == colh_window) &&
               (e->data.c.i == (wimp_i)COLH_CONTENTS_LINE) &&
               (e->data.c.index >= 0)
              )
                {
                trace_0(TRACE_APP_PD4, "colh_event_handler - stashing caret position");

                /* Someone is stealing the caret from the contents/formula-line, stash position away so it can be restored    */
                /* when pipedream reclaims the caret. Also means we can paste formula etc into formula line at correct place. */
                formline_stashedcaret = e->data.c.index;
                }
            processed = FALSE;
            break;

        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            colh_message(&e->data.msg);
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed wimp event to colh_window: %s",
                    report_wimp_event(e->e, &e->data));
            processed = FALSE;
            break;
        }

    return(processed);
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
* Chapter 2: Button types used for the colh_window
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

static void             /* new stuff */
application_button_click_in_colh(
    wimp_mousestr *pointer)
{
    static BOOL initially_selectclicked = FALSE;

    wimp_i icon = pointer->i;
    int buttonstate = pointer->bbits;
    BOOL selectclicked;

    trace_4(TRACE_APP_PD4,
            "application_button_click_in_colh: (%d, %d) bstate %X, icon %d",
            pointer->x, pointer->y, buttonstate, (int)icon);

    /* ensure we can find slot for positioning, overlap tests etc. must allow spellcheck as we may move */
    if(!mergebuf())
        return;
    filbuf();
    /* but guaranteed that we can simply slot_in_buffer = FALSE for movement */

    if((icon == (wimp_i)COLH_COLUMN_HEADINGS))
        {
        /* have to cope with Pink 'Plonker' Duck Man's ideas on double clicks (fixed in new RISC OS+):
        * He says that left then right (or vice versa) is a double click!
        */
        if(buttonstate & (wimp_BLEFT | wimp_BRIGHT))
            {
            selectclicked = ((buttonstate & wimp_BLEFT) != 0);

            /* must be same button that started us off */
            if(initially_selectclicked != selectclicked)
                buttonstate = buttonstate << 8; /* force a single click */
            }

        if(     buttonstate & (wimp_BLEFT      | wimp_BRIGHT))
            {
            selectclicked = ((buttonstate & wimp_BLEFT) != 0);

            application_doubleclick_in_colh(icon, pointer, selectclicked);
            }
        else if(buttonstate & (wimp_BDRAGLEFT  | wimp_BDRAGRIGHT))
            {
            selectclicked = ((buttonstate & wimp_BDRAGLEFT) != 0);

            application_startdrag_in_colh(icon, pointer, selectclicked);
            }
        else if(buttonstate & (wimp_BCLICKLEFT | wimp_BCLICKRIGHT))
            {
            selectclicked = ((buttonstate & wimp_BCLICKLEFT) != 0);

            initially_selectclicked = selectclicked;
            application_singleclick_in_colh(icon, pointer, selectclicked);
            }
        }
    else
        {
        if(     buttonstate & (wimp_BLEFT      | wimp_BRIGHT))
            {
            selectclicked = ((buttonstate & wimp_BLEFT) != 0);

            initially_selectclicked = selectclicked;
            application_singleclick_in_colh(icon, pointer, selectclicked);
            }
        }
}

static void             /* new stuff */
application_singleclick_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked)
{
    DOCNO docno   = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL) && (docno == blk_docno);
    BOOL acquire  = FALSE;                                        /* don't want caret on block marking operations */
    BOOL motion   = FALSE;
    BOOL extend   = !selectclicked || host_shift_pressed();          /* unshifted-adjust or shift-anything */
    S32  funcnum = 0;
    S32  highlight_number = -1;

    trace_0(TRACE_APP_PD4, "application_singleclick_in_colh");

    switch((int)icon)
        {
        case COLH_BUTTON_OK:
            formline_mergebacktext(FALSE, NULL);        /* don't report formula compilation errors */
            break;                                      /* don't want to know where the caret was  */

        case COLH_BUTTON_CANCEL:
            formline_canceledit();
            break;

        case COLH_CONTENTS_LINE:
            EditContentsLine();
            break;

        case COLH_FUNCTION_SELECTOR:

            /* deliberate drop thru ... */

        case COLH_PIPEDREAM_LOGO:
            funcnum = N_EditFormulaInWindow;
            break;

        case COLH_BUTTON_REPLICD:
            funcnum = (selectclicked) ? N_ReplicateDown: N_Replicate;
            break;

        case COLH_BUTTON_REPLICR:
            funcnum = (selectclicked) ? N_ReplicateRight : N_Replicate;
            break;

        case COLH_BUTTON_TOTEXT:
            funcnum = (selectclicked) ? N_ToText : N_ExchangeNumbersText;
            break;

        case COLH_BUTTON_TONUMBER:
            funcnum = (selectclicked) ? N_ToNumber : N_ExchangeNumbersText;
            break;

        case COLH_BUTTON_GRAPH:
            funcnum = (selectclicked) ? N_ChartNew : N_ChartOptions;
            break;

        case COLH_BUTTON_SAVE:
            funcnum = (selectclicked) ? N_SaveFile : N_SaveFileAsIs;
            break;

        case COLH_BUTTON_PRINT:
            funcnum = (selectclicked) ? N_Print : N_PageLayout;
            break;

        case COLH_BUTTON_LJUSTIFY:
            funcnum = N_LeftAlign;
            break;

        case COLH_BUTTON_CJUSTIFY:
            funcnum = N_CentreAlign;
            break;

        case COLH_BUTTON_RJUSTIFY:
            funcnum = N_RightAlign;
            break;

        case COLH_BUTTON_FJUSTIFY:
            {
            if(d_options_JU == 'Y')
                d_options_JU = 'N';
            else
                {
                d_options_JU = 'Y';
                d_options_WR = 'Y';
                }

            update_variables();
            break;
            }

        case COLH_BUTTON_FONT:
            funcnum = (selectclicked) ? N_PRINTERFONT : N_INSERTFONT;
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
            funcnum = (selectclicked) ? N_LeadingCharacters : N_TrailingCharacters;
            break;

        case COLH_BUTTON_DECPLACES:
            funcnum = N_DecimalPlaces;
            break;

        case COLH_BUTTON_COPY:
            funcnum = (selectclicked) ? N_CopyBlockToPasteList : N_MoveBlock;
            break;

        case COLH_BUTTON_DELETE:
            funcnum = (selectclicked) ? N_DeleteBlock : N_ClearBlock;
            break;

        case COLH_BUTTON_PASTE:
            funcnum = N_Paste;
            break;

        case COLH_BUTTON_FORMATBLOCK:
            funcnum = N_FormatBlock;
            break;

        case COLH_BUTTON_SEARCH:
            funcnum = (selectclicked) ? N_Search : N_NextMatch;
            break;

        case COLH_BUTTON_SORT:
            funcnum = (selectclicked) ? N_SortBlock : N_TransposeBlock;
            break;

        case COLH_BUTTON_SPELLCHECK:
            funcnum = (selectclicked) ? N_CheckDocument : N_BrowseDictionary;
            break;

        case COLH_BUTTON_CMD_RECORD:
            funcnum = N_RecordMacroFile;
            break;

        case COLH_BUTTON_CMD_EXEC:
            funcnum = N_DoMacroFile;
            break;

        case COLH_STATUS_BAR:
        case COLH_STATUS_TEXT:
            /* if editing, suppress setting/clearing of marks */
            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                break;

            if(selectclicked && !blkindoc)
                {
                trace_0(TRACE_APP_PD4, "click on slot coordinates - mark entire sheet");
                funcnum = N_MarkSheet;
                }
            else
                {
                trace_0(TRACE_APP_PD4, "click on slot coordinates - clear markers");
                funcnum = N_ClearMarkedBlock;
                }
            break;

        case COLH_COLUMN_HEADINGS:
            {
            pointer_shape *shape;
            int   subposition;
            COL  tcol;
            coord tx;

            /* if editing, suppress (curcol,currow) movement and selection dragging etc. */
            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                {
                acquire = !extend;
                break;
                }

            colh_where_in_column_headings(pointer, &shape, &subposition, &tcol, &tx);
            setpointershape(shape);

            switch(subposition)
                {
                case OVER_COLUMN_CENTRE:
                    if(extend)
                        {
                        /* either alter current block or set new block:
                         * mergebuf has been done by caller to ensure slot marking correct
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
            /* can't think of a kosher way to do this */
            block_highlight_core(!selectclicked, highlight_number);
            funcnum = 0;
            }
        }

    if(0 != funcnum)
        application_process_command(funcnum);

    /* reselect document eg N_PRINTERFONT screws us up */
    select_document_using_docno(docno);

    if(acquire)
        xf_caretreposition = TRUE;

    if(xf_caretreposition  ||  motion)
        {
        draw_screen();
        draw_caret();
        }
}

static void             /* new stuff */
application_doubleclick_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked)
{
    BOOL extend = !selectclicked || host_shift_pressed();          /* unshifted-adjust or shift-anything */

    trace_0(TRACE_APP_PD4, "application_doubleclick_in_colh");

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  /* everything suppressed whilst editing */
        return;

    switch((int)icon)
        {
        case COLH_COLUMN_HEADINGS:
            {
            pointer_shape *shape;
            int            subposition;
            COL           tcol;
            coord          tx;

            colh_where_in_column_headings(pointer, &shape, &subposition, &tcol, &tx);
            setpointershape(shape);

            if(extend)
                return;

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
}

static void             /* new stuff */
application_startdrag_in_colh(
    wimp_i icon,
    wimp_mousestr *pointer,
    BOOL selectclicked)
{
    DOCNO docno   = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL) && (docno == blk_docno);
    BOOL extend   = !selectclicked || host_shift_pressed();          /* unshifted-adjust or shift-anything */

    trace_0(TRACE_APP_PD4, "application_startdrag_in_colh");

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  /* everything suppressed whilst editing */
        return;

    switch((int)icon)
        {
        case COLH_COLUMN_HEADINGS:
            {
            pointer_shape *shape;
            int            subposition;
            COL           tcol;
            coord          tx;
            coord          ty   = -1; /*>>>don't really know this - suck it and see!*/

            colh_where_in_column_headings(pointer, &shape, &subposition, &tcol, &tx);
            setpointershape(shape);     /* Must do this, incase colh_pointershape_null_handler hasn't been called yet. */

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
            }
            break;

        } /* switch(icon) */
}

/******************************************************************************
*
* Called when user clicks on the editable slot-contents line.
*
* The contents line is a writable icon, so when clicked on,
* receives the caret, whether it wants it, or not!.
*
******************************************************************************/

static void
EditContentsLine(void)
{
    wimp_caretstr carrot;
    S32           index;

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

    wimp_get_caret_pos(&carrot);
    index = (((carrot.w == colh_window)                &&
              (carrot.i == (wimp_i)COLH_CONTENTS_LINE) &&
              (carrot.index >= 0)
             )
             ? carrot.index
             : 0
            );

    expedit_editcurrentslot(index, FALSE);      /* either in contents line or formula box as appropriate */
}

/******************************************************************************
*
* Process message events for colh_window
*
******************************************************************************/

static void
colh_HELPREQUEST(
    wimp_msgstr * m)
{
    wimp_mousestr * pointer = &m->data.helprequest.m;
    wimp_i icon = pointer->i;
    char abuffer[256+128/*bit of slop*/];
    U32 prefix_len;
    char * buffer;
    char * alt_msg;
    const char * msg = NULL;

    if(dragtype != NO_DRAG_ACTIVE) /* stop pointer and message changing whilst dragging */
        return;

    #if 1
    /* Terse messages for better BubbleHelp response */
    abuffer[0] = NULLCH;
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
        switch((int)icon)
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

            case COLH_PIPEDREAM_LOGO:
                /* they can sodding well use the new UI if they want help! */
                break;

            case COLH_STATUS_BAR:
            case COLH_STATUS_TEXT:
                /* if editing, suppress setting/clearing of marks */
                break;

            case COLH_COLUMN_HEADINGS:                                                       /* if editing, all movement and drags are suppressed */
                break;

            default:
                msg = help_colh_inexpression_line;
                break;
            }
        }
    else
        switch((int)icon)
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

            case COLH_PIPEDREAM_LOGO:
                /* they can sodding well use the new UI if they want help! */
                break;

            case COLH_STATUS_BAR:
            case COLH_STATUS_TEXT:
                /* if editing, suppress setting/clearing of marks */
                if(xf_inexpression)
                    break;

                msg = help_top_left_corner;
                break;

            case COLH_COLUMN_HEADINGS:
                {
                pointer_shape *shape;
                int subposition;
                COL tcol;
                char cbuf[32];

                /* if editing, all movement and drags are suppressed */
                if(xf_inexpression)
                    break;

                colh_where_in_column_headings(pointer, &shape, &subposition, &tcol, NULL);
                setpointershape(shape);

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

    if(msg)
        xstrkpy(buffer, elemof32(abuffer) - prefix_len, msg);

    #if FALSE
    if(append_drag_msg)
        xstrkat(abuffer, elemof32(abuffer), help_drag_file_to_insert);
    #endif

    riscos_sendhelpreply(m, (strlen32p1(abuffer) < 240) ? abuffer : alt_msg);
}

static void
colh_message(
    wimp_msgstr * m)
{
    int action = m->hdr.action;

    switch(action)
        {
        default:
            break;

        case wimp_MHELPREQUEST:
            trace_0(TRACE_APP_PD4, "Help request on colh_window");
            colh_HELPREQUEST(m);
            break;
        }
}

extern void
colh_where_in_column_headings(
    wimp_mousestr *pointer,
    pointer_shape **shapep,
    int *subpositionp,
    P_COL p_col,
    coord *txp)
{
    wimp_wstate    r;
    int            orgx, orgy;
    int            rel_x, rel_y;
    int            arrowcentre;
    pointer_shape *shape;
    int            drag_type;
    coord          drag_tx;
    COL           drag_tcol;
    coord          tx;

    wimp_get_wind_state(pointer->w, &r);
    orgx = r.o.box.x0 - r.o.scx;
    orgy = r.o.box.y1 - r.o.scy;
    rel_x = pointer->x - orgx;
    rel_y = pointer->y - orgy;

    tx        = div_round_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0), charwidth);
    shape     = POINTER_DEFAULT;
    drag_type = OVER_COLUMN_DEAD_SPACE;

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
        coord  prev_tx   = div_round_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0) - hitband, charwidth);
        coord  prev_coff = calcoff(prev_tx);

        drag_tcol = 0; /* SKS 29sep96 paranoia */
        drag_tx   = 0;

        if(coff >= 0)
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
                coord next_tx = div_round_floor_fn(rel_x - texttooffset_x(COLUMNHEAD_t_X0) + hitband, charwidth);

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
            if(prev_coff >= 0)
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
* Called the when pointer leaves the colh_window.
*
* Remove the null_handler and restore the default pointer shape.
*
******************************************************************************/

static void
colh_pointershape_endtracking(void)
{
    trace_1(TRACE_APP_PD4, "colh_pointershape_endtracking(): track_docno=%u", track_docno);

    Null_EventHandlerRemove(colh_pointershape_null_handler, &track_docno);

    setpointershape(POINTER_DEFAULT);
}

/******************************************************************************
*
* Call-back from null engine.
*
******************************************************************************/

null_event_proto(static, colh_pointershape_null_handler)
{
    P_DOCNO dochanp = (P_DOCNO) p_null_event_block->client_handle;

    trace_1(TRACE_APP_PD4, "colh_pointershape_null_handler, rc=%d", p_null_event_block->rc);

    switch(p_null_event_block->rc)
        {
        case NULL_QUERY:
            trace_0(TRACE_APP_PD4, "colh call to ask if we want nulls");
            return(dragtype == NO_DRAG_ACTIVE
                   ? NULL_EVENTS_REQUIRED
                   : NULL_EVENTS_OFF
                  );

        case NULL_EVENT:
            {
            wimp_mousestr pointer;

            trace_0(TRACE_APP_PD4, "colh null call");

            if(select_document_using_docno(*dochanp))
            if(NULL == wimp_get_point_info(&pointer))
                {
                /*(if(pointer.w == colh_window)*/    /* should be true, as we release null events when pointer leaves window */
                    {
                    switch((int)pointer.i)
                        {
                        case COLH_COLUMN_HEADINGS:
                            /* if editing, all movement and drags are suppressed, so show default pointer */
                            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                                {
                                setpointershape(POINTER_DEFAULT);
                                }
                            else
                                {
                                pointer_shape *shape;

                                colh_where_in_column_headings(&pointer, &shape, NULL, NULL, NULL);
                                setpointershape(shape);
                                }
                            break;

                        #if 0
                        case COLH_FUNCTION_SELECTOR:
                            trace_0(TRACE_APP_PD4, "change pointer to drop-down-menu");

                            setpointershape(POINTER_DROPMENU);
                            break;
                        #endif

                        default:
                            trace_0(TRACE_APP_PD4, "restore default pointer");

                            setpointershape(POINTER_DEFAULT);
                            break;
                        }
                    }
                }
            }
            return(NULL_EVENT_COMPLETED);

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
setpointershape(
    pointer_shape *shape)
{
    if(current_pointer_shape != shape)
        {
        riscos_hourglass_off();

        if(shape)
            {
            sprite_id sprite;

            sprite.tag = sprite_id_name;
            sprite.s.name = shape->name;

            wimpt_safe(pointer_set_shape((sprite_area *)resspr_area() /* -1 */, &sprite, shape->x, shape->y));
            }
        else
            {
            /* POINTER_DEFAULT */

            pointer_reset_shape();
            }

        current_pointer_shape = shape;

        riscos_hourglass_on();
        }
}

/* end of colh.c */
