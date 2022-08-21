/* expedit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Expression editors - formula line & formula window */

/* RCM July 1991 */

#include "common/gflags.h"

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

#include "datafmt.h"

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

#include "cmodules/mlec.h"

#include "colh_x.h"

static void
start_editor(
    S32 caretpos,
    BOOL force_inbox);

/******************************************************************************
*
* Formula line specific routines
*
******************************************************************************/

static S32
formline_cursor_getpos(void);

static void
formline_cursor_setpos(
    S32 pos);

/******************************************************************************
*
* Formula window specific routines
*
******************************************************************************/

static S32
formwind_create_fill_open(
    P_FORMULA_WINDOW_HANDLE formwindp,
    _InVal_     DOCNO docno,
    char *text,
    S32 cursorcol,
    S32 cursorrow,
    BOOL newline,
    char *title);

static S32
formwind_create(
    P_FORMULA_WINDOW_HANDLE formwindp,
    _InVal_     DOCNO docno);

static void
formwind_destroy(
    P_FORMULA_WINDOW_HANDLE formwindp);

static void
formwind_open_infront_withfocus(
    FORMULA_WINDOW_HANDLE formwind);

static S32
formwind_settext(
    FORMULA_WINDOW_HANDLE formwind,
    char *text,
    S32 cursorcol,
    S32 cursorrow,
    BOOL newline);

static BOOL
formwind_event_handler(
    wimp_eventstr *e,
    void *handle);

static void
formwind_open_window(
    FORMULA_WINDOW_HANDLE formwind,
    wimp_openstr *main);

static void
formwind_canceledit(void);

static BOOL
formwind_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp);

static void
formwind_buildtitlestring(
    _Out_writes_z_(elemof_buffer) P_U8Z title,
    _InVal_     U32 elemof_buffer,
    COL col,
    ROW row);

static void
formwind_setextent(
    FORMULA_WINDOW_HANDLE formwind,
    wimp_box paneWorkArea);

mlec_event_proto(static, formwind_mlec_event_handler); /* callback function */

static void
formwind_pointershape_starttracking(
    FORMULA_WINDOW_HANDLE formwind);

static void
formwind_pointershape_endtracking(
    FORMULA_WINDOW_HANDLE formwind);

null_event_proto(static, formwind_pointershape_null_handler); /* callback function */

#define FORMWIND_BUTTON_OK         (0)
#define FORMWIND_BUTTON_CANCEL     (1)
#define FORMWIND_FUNCTION_SELECTOR (2)
#define FORMWIND_BUTTON_NEWLINE    (3)

static BOOL
check_text_or_fixit(
    S32 err);

static S32
cvt_colrow_to_offset(
    char *text,
    S32 col,
    S32 row);

static void
cvt_offset_to_colrow(
    char *text,
    S32 offset,
    P_S32 p_col,
    P_S32 p_row,
    BOOL *multilinep);

static BOOL     formwind_position_valid = FALSE;
static wimp_box formwind_position_box   = {0, 0, 0, 0};

/******************************************************************************
*
* Function selector specific routines.
*
******************************************************************************/

extern void
function__event_menu_maker(void);

extern menu
function__event_menu_filler(
    void *handle);

extern BOOL
function__event_menu_proc(
    void *handle,
    char *hit,
    BOOL submenurequest);

/******************************************************************************
*
* Called when user presses Ctrl-X or chooses Edit formula from the menu tree.
*
******************************************************************************/

extern void
EditFormula_fn(void)
{
    expedit_editcurrentslot(lecpos, FALSE);     /* either in contents line or formula box as appropriate */
}

/******************************************************************************
*
* For the advanced user who 'knows' he needs the formula window.
*
******************************************************************************/

extern void
EditFormulaInWindow_fn(void)
{
#if TRUE
    /* Tests performed by expedit_editcurrentslot */
#else
    if(xf_inexpression)
        return;

    if(xf_inexpression_box)
        return;
#endif

    if(xf_inexpression_line)
        {
        expedit_transfer_line_to_box(FALSE);    /* don't force a newline */
        return;
        }

    expedit_editcurrentslot(lecpos, TRUE);      /* force use of formula box */
}

extern void
expedit_editcurrentslot(
    S32 caretpos,
    BOOL force_inbox)
{
    P_CELL tcell;

    if(xf_inexpression_box)
        {
        trace_0(TRACE_APP_EXPEDIT, "expedit_editcurrentslot - fronting the formula window");

        win_send_front(editexpression_formwind->mainwindow, TRUE);
        return;
        }

    trace_0(TRACE_APP_EXPEDIT, "expedit_editcurrentslot");

    if(xf_inexpression_line)
        return;

    if(!mergebuf() || xf_inexpression)
        return;

    *linbuf = '\0';

    tcell = travel_here();

    if(tcell)
        {
        if(tcell->type == SL_PAGE)
            {
            xf_acquirecaret = TRUE;
            return;
            }

        if(tcell->justify & PROTECTED)
            {
            reperr_null(create_error(ERR_PROTECTED));
            xf_acquirecaret = TRUE;
            return;
            }

        prccon(linbuf, tcell);
        }

    start_editor(caretpos, force_inbox);
}

/******************************************************************************
*
* Called by inschr (in bufedit) to put user in formula line when in 'numeric mode'.
*
******************************************************************************/

extern void
expedit_editcurrentslot_freshcontents(
    BOOL force_inbox)
{
    *linbuf = '\0';

    start_editor(0, force_inbox);
}

/******************************************************************************
*
* If current cell is a text cell, try compiling it, reporting any errors.
*
******************************************************************************/

extern void
expedit_recompilecurrentslot_reporterrors(void)
{
    P_CELL  tcell;
    S32    err, at_pos;

    if(!mergebuf() || xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    *linbuf = '\0';

    tcell = travel_here();

    if(tcell)
        {
        if(tcell->type != SL_TEXT)
            return;

        if(tcell->justify & PROTECTED)
            {
            reperr_null(create_error(ERR_PROTECTED));
            xf_acquirecaret = TRUE;
            return;
            }

        prccon(linbuf, tcell);
        slot_in_buffer = buffer_altered = TRUE;
        edtslr_col = curcol;
        edtslr_row = currow;

      /*seteex_nodraw();*/
        merexp_reterr(&err, &at_pos, TRUE);
      /*endeex_nodraw();*/

        if(err < 0)
            {
            if((at_pos >= 0) && !check_text_or_fixit(err))
                {
                start_editor(at_pos, FALSE);    /* use line or box as appropriate */
                return;
                }

            merexp_reterr(NULL, NULL, TRUE);        /* force expression into sheet as a text cell */
            }

        slot_in_buffer = buffer_altered = FALSE;
        lecpos = lescrl = 0;
        }
}

extern void
expedit_transfer_line_to_box(
    BOOL newline)
{
    if(xf_inexpression_line)
        {
        wimp_icon  icon;
        S32        caretpos;
        char       title[LIN_BUFSIZ];
        S32        err;

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

        caretpos = formline_cursor_getpos();    /* either actual or last known caret position */

        formwind_buildtitlestring(title, elemof32(title), edtslr_col, edtslr_row);  /* Build string, eg "Formula window: A1" */

        if((err = formwind_create_fill_open(&editexpression_formwind,
                                            current_docno(),
                                            icon.data.indirecttext.buffer,
                                            caretpos,   /* cursor col */
                                            0,          /* cursor row */
                                            newline,    /* performed AFTER cursor positioning */
                                            title)
           ) >= 0
          )
            {
            xf_inexpression_line = FALSE;
            colh_draw_contents_of_numslot();
            xf_inexpression_box  = TRUE;        /* All other flags remain the same */
            colh_draw_edit_state_indicator();
            }
        }
}

extern BOOL
expedit_insert_char(
    char ch)
{
    BOOL had_error = FALSE;

    if(xf_inexpression_line)
        {
        wimp_icon  icon;
        S32        caretpos;
        S32        length;
        char      *currpos;

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

        caretpos = formline_cursor_getpos();
        length   = strlen(icon.data.indirecttext.buffer);

        caretpos = MAX(caretpos, 0);
        caretpos = MIN(caretpos, length);

        if((length + 1) < icon.data.indirecttext.bufflen)
            {
            currpos = icon.data.indirecttext.buffer + caretpos;
            memmove32(currpos+1, currpos, (length - caretpos + 1));
            *currpos = ch;

            /* Nudge caret position, and force a redraw */
            ++caretpos;
            formline_cursor_setpos(caretpos);
            wimp_set_icon_state(colh_window, (wimp_i)COLH_CONTENTS_LINE, (wimp_iconflags) 0, (wimp_iconflags) 0);
            }
        else
            had_error = TRUE;
        }
    else if(xf_inexpression_box)
        {
        had_error = (mlec__insert_char(editexpression_formwind->mlec, ch) < 0);
        }

    return(had_error);
}

extern BOOL
expedit_insert_string(
    const char *insertstr)
{
    S32 insertlen = strlen(insertstr);
    BOOL had_error = FALSE;

    if(insertlen <= 0)
        return(FALSE);

    if(xf_inexpression_line)
        {
        wimp_icon  icon;
        S32        caretpos;
        S32        length;
        char      *currpos;

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

        caretpos = formline_cursor_getpos();
        length   = strlen(icon.data.indirecttext.buffer);

        caretpos = MAX(caretpos, 0);
        caretpos = MIN(caretpos, length);

        if((length + insertlen) < icon.data.indirecttext.bufflen)
            {
            currpos = icon.data.indirecttext.buffer + caretpos;
            memmove32(currpos+insertlen, currpos, (length - caretpos + 1)); /* make a gap */
            memmove32(currpos, insertstr, insertlen);                       /* splice text in */

            /* Nudge caret position, and force a redraw */
            caretpos += insertlen;
            formline_cursor_setpos(caretpos);
            wimp_set_icon_state(colh_window, (wimp_i)COLH_CONTENTS_LINE, (wimp_iconflags) 0, (wimp_iconflags) 0);
            }
        else
            had_error = TRUE;
        }
    else if(xf_inexpression_box)
        {
        had_error = (mlec__insert_text(editexpression_formwind->mlec, (char*)insertstr) < 0);  /* cvt (const char*) to (char *) */
        }

    return(had_error);
}

extern void
expedit_get_cursorpos(
    P_S32 p_col,
    P_S32 p_row)
{
    if(xf_inexpression_line)
        {
        *p_col = formline_cursor_getpos();
        *p_row = 0;
        }
    else if(xf_inexpression_box)
        {
        mlec__cursor_getpos(editexpression_formwind->mlec, p_col, p_row);
        }
}

extern void
expedit_set_cursorpos(
    S32 col,
    S32 row)
{
    if(xf_inexpression_line)
        {
        formline_cursor_setpos(col);
        }
    else if(xf_inexpression_box)
        {
        mlec__cursor_setpos(editexpression_formwind->mlec, col, row);
        }
}

extern void
expedit_delete_bit_of_line(
    S32 col_start,
    S32 col_end,
    S32 row)
{
    if(xf_inexpression_line)
        {
        wimp_icon icon;
        S32       start, end, length;

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

        length = strlen(icon.data.indirecttext.buffer);

        start = MIN(col_start, col_end);
        end   = MAX(col_start, col_end);

        start = MAX(start, 0);
        end   = MIN(end  , length);

        if(start < end)
            {
            /* chop characters from the buffer, position the caret at start of cut, and force a redraw */
            memmove32(&icon.data.indirecttext.buffer[start], &icon.data.indirecttext.buffer[end], (length - end + 1));
            formline_cursor_setpos(start);
            wimp_set_icon_state(colh_window, (wimp_i)COLH_CONTENTS_LINE, (wimp_iconflags) 0, (wimp_iconflags) 0);
            }
        }
    else if(xf_inexpression_box)
        {
        mlec__cursor_setpos   (editexpression_formwind->mlec, col_start, row);
        mlec__selection_adjust(editexpression_formwind->mlec, col_end  , row);
        mlec__selection_delete(editexpression_formwind->mlec);
        }
}

extern BOOL
expedit_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp)
{
    if(xf_inexpression_box)
        formwind_mergebacktext(reporterror, caretposp);

    if(xf_inexpression_line)
        formline_mergebacktext(reporterror, caretposp);

    return(TRUE);
}

#ifdef UNUSED

extern void
expedit_canceledit(void)
{
    if(xf_inexpression_box)
        formwind_canceledit();

    if(xf_inexpression_line)
        formline_canceledit();
}

#endif /* UNUSED */

/******************************************************************************
*
* When a document is destroyed, this routine is
* called to remove its formula window (if any)
*
******************************************************************************/

extern void
expedit_close_file(
    _InVal_     DOCNO docno)
{
    DOCNO old_docno;

    trace_0(TRACE_APP_EXPEDIT, "expedit_close_file");

    old_docno = change_document_using_docno(docno);

    if(xf_inexpression_box)
        formwind_canceledit();

    select_document_using_docno(old_docno);
}

extern BOOL
expedit_owns_window(
    P_DOCU p_docu,
    wimp_w window)
{
    if(p_docu->Xxf_inexpression_box)
        {
        if(window == p_docu->Xeditexpression_formwind->panewindow)
            return(TRUE);
        }

    return(FALSE);
}

static void
start_editor(
    S32 caretpos,
    BOOL force_inbox)
{
    S32   cursorcol, cursorrow;
    BOOL  multiline;

    cvt_offset_to_colrow(linbuf, caretpos, &cursorcol, &cursorrow, &multiline);

    if(multiline || force_inbox || !borbit)
        {
        /* Fire up the formula box */

        char title[LIN_BUFSIZ];
        S32 err;

        formwind_buildtitlestring(title, elemof32(title), curcol, currow);          /* Build string, eg "Formula window: A1" */

        if((err = formwind_create_fill_open(&editexpression_formwind,
                                            current_docno(),
                                            linbuf,
                                            cursorcol,
                                            cursorrow,
                                            FALSE,
                                            title)
           ) >= 0
          )
            {
            output_buffer = slot_in_buffer = buffer_altered = xf_blockcursor = TRUE;

            edtslr_col = curcol;
            edtslr_row = currow;

            xf_inexpression_box = TRUE;

            mark_row(currowoffset);
            draw_screen();
            }
        }
    else
        {
#if FALSE
        assert((cursorcol == caretpos) && (cursorrow == 0));
#else
        assert((cursorrow == 0));       /* caret IS allowed beyond end of line but editors dont pad with spaces to that point */
#endif
        {
        /* Formula fits in one line, so edit it on the contents line */

        wimp_icon     icon;
      /*wimp_caretstr carrot;*/

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);
        assert(icon.data.indirecttext.bufflen == LIN_BUFSIZ);
        strcpy(icon.data.indirecttext.buffer, linbuf);
        wimp_set_icon_state(colh_window, (wimp_i)COLH_CONTENTS_LINE, (wimp_iconflags) 0, (wimp_iconflags) 0);
#if TRUE
        formline_cursor_setpos(caretpos);
        formline_claim_focus();
#else
        carrot.w = colh_window;
        carrot.i = (wimp_i)COLH_CONTENTS_LINE;
        carrot.x = carrot.y = carrot.height = -1;
        carrot.index = caretpos;

        wimp_set_caret_pos(&carrot);
#endif
        }
        {                   /* Show cell as being editted */
        /*seteex();*/
        output_buffer = slot_in_buffer = buffer_altered = xf_blockcursor = TRUE;

        edtslr_col = curcol;
        edtslr_row = currow;        /* save current position */

        xf_inexpression_line = TRUE;

        mark_row(currowoffset);
        colh_draw_edit_state_indicator();
        draw_screen();
        }
        }
}

/******************************************************************************
*
* Formula line routines.
*
* The formula line is a single-line expression editor based on a window manager indirected writable text icon.
* This icon, 'COLH_CONTENTS_LINE', is dual purpose, being used as the 'current cell contents' line until
* clicked-on, at which point it becomes an editor.
*
******************************************************************************/

extern void
formline_claim_focus(void)
{
    wimp_caretstr carrot;    /* nyeeeer, whats up doc? */

    wimp_get_caret_pos(&carrot);

    if((carrot.w != colh_window) || (carrot.i != (wimp_i)COLH_CONTENTS_LINE))
        {
        carrot.w      = colh_window;
        carrot.i      = (wimp_i)COLH_CONTENTS_LINE;
        carrot.x      = -1;
        carrot.y      = -1;
        carrot.height = -1;
        carrot.index  = formline_stashedcaret;   /* index into string, set by formline_cursor_setpos */
                                                 /* or when caret was stolen from us                 */
        wimp_set_caret_pos(&carrot);
        }
  /*else                                      */
  /*    formline_stashedcaret = carrot.index; */
}

static S32
formline_cursor_getpos(void)
{
    wimp_caretstr  carrot;
    wimp_icon      icon;
    S32            caretpos;
    S32            length;

    wimp_get_caret_pos(&carrot);
    wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

    length = strlen(icon.data.indirecttext.buffer);

    /* If the caret is within the icon, return its string index position,      */
    /* else someone has stolen the caret from us, so return the stashed value. */

    if((carrot.w == colh_window) && (carrot.i == (wimp_i)COLH_CONTENTS_LINE))
        caretpos = carrot.index;
    else
        caretpos = formline_stashedcaret;

    caretpos = MAX(caretpos, 0);
    caretpos = MIN(caretpos, length);

    formline_stashedcaret = caretpos;

    return(caretpos);
}

static void
formline_cursor_setpos(
    S32 pos)
{
    wimp_caretstr  carrot;
    wimp_icon      icon;
    S32            length;

    wimp_get_caret_pos(&carrot);
    wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);

    length = strlen(icon.data.indirecttext.buffer);

    pos = MAX(pos, 0);
    pos = MIN(pos, length);

    formline_stashedcaret = pos;

    /* If caret is within the icon, and is not at required place, reposition it. */

    if((carrot.w == colh_window)                &&
       (carrot.i == (wimp_i)COLH_CONTENTS_LINE) &&
       (carrot.index != pos)
      )
        {
        carrot.index = pos;
        wimp_set_caret_pos(&carrot);
        }
}

extern BOOL
formline_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp)
{
    if(xf_inexpression_line)
        {
        wimp_icon icon;
        S32       err, at_pos;

        /* next/prev match and its friends want to know where the caret is */
        if(caretposp)
            *caretposp = formline_cursor_getpos();

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &icon);
        assert(icon.data.indirecttext.bufflen == LIN_BUFSIZ);
        strcpy(linbuf, icon.data.indirecttext.buffer);

        buffer_altered = TRUE;
        slot_in_buffer = TRUE;

        if(reporterror)
            {
            merexp_reterr(&err, &at_pos, TRUE);

            if(err < 0)
                {
                if((at_pos >= 0) && !check_text_or_fixit(err))
                    {
                    formline_cursor_setpos(at_pos);     /* show position of error */
                    return(FALSE);                      /* retain editing state   */
                    }

                merexp_reterr(NULL, NULL, TRUE);        /* force expression into sheet as a text cell */
                }
            }
        else
            merexp_reterr(NULL, NULL, TRUE);

        if(macro_recorder_on)
            output_formula_to_macro_file(linbuf);

        formline_canceledit();
        }

    return(TRUE);
}

extern void
formline_canceledit(void)
{
    /* c.f. endeex in c.numbers */

    if(xf_inexpression_line)
        {
        xf_inexpression_line = FALSE;
        slot_in_buffer = buffer_altered = FALSE;
        lecpos = lescrl = 0;

        /* need to redraw more if inverse cursor moved */
        if((edtslr_col != curcol)  ||  (edtslr_row != currow))
            mark_row(currowoffset);
        output_buffer = out_currslot = TRUE;
        xf_blockcursor = FALSE;

        xf_caretreposition = TRUE;
        xf_acquirecaret = TRUE;     /* Give caret back to sheet */

        colh_draw_edit_state_indicator();
        draw_screen();
        }
}

/******************************************************************************
*
* Formula window routines.
*
* The formula window is a multi-line expression editor based on a multi-line edit control (mlec).
* It consists of two windows: "ExpEdMain" which contains various icons (tick, cross and formula)
* and is overlayed by a pane window "ExpEdPane" which is maintained by mlec.
*
******************************************************************************/

static S32
formwind_create_fill_open(
    P_FORMULA_WINDOW_HANDLE formwindp,
    _InVal_     DOCNO docno,
    char *text,
    S32 cursorcol,
    S32 cursorrow,
    BOOL newline,
    char *title)
{
    S32 err;

    if((err = formwind_create(formwindp, docno)) >= 0)
        {
        win_settitle((*formwindp)->mainwindow, title);

        formwind_open_infront_withfocus(*formwindp);

#if TRUE
        if((err = formwind_settext(*formwindp, text, cursorcol, cursorrow, newline)) < 0)
            formwind_destroy(formwindp);
#else
        err = formwind_settext(*formwindp, text, cursorcol, cursorrow, newline);
#endif
        }

    return(err);
}

static S32
formwind_create(
    P_FORMULA_WINDOW_HANDLE formwindp,
    _InVal_     DOCNO docno)
{
    S32              err = 0;
    FORMULA_WINDOW_HANDLE  formwind;
    MLEC_HANDLE      mlec;
    template        *templatehanmain;
    template        *templatehanpane;
    wimp_wind       *templatemain;
    wimp_wind       *templatepane;
    wimp_w           main_win_handle;
    wimp_w           pane_win_handle;
    wimp_box         mainBB, paneBB;  /* visible area     */
    wimp_box         paneExtent;      /* work area extent */

    *formwindp = formwind = al_ptr_alloc_elem(FORMULA_WINDOW, 1, &err);

    if(formwind)
        {
        formwind->mlec         = NULL;
        formwind->mainwindow   = formwind->panewindow   = 0;
        formwind->maintemplate = formwind->panetemplate = NULL;
        formwind->tracking     = FALSE;

        if((err = mlec_create(&mlec)) >= 0)
            {
            formwind->mlec = mlec;

            templatehanmain = template_find_new("ExpEdMain");  /*>>>use a manifest??*/
            templatehanpane = template_find_new("ExpEdPane");  /*>>>use a manifest??*/

            if(templatehanmain && templatehanpane)
                {
                formwind->maintemplate = templatemain = template_copy_new(templatehanmain);
                formwind->panetemplate = templatepane = template_copy_new(templatehanpane);

                if(templatemain && templatepane)
                    {
                    os_error * e;
                    mainBB = templatemain->box;

                    e = win_create_wind(templatemain, &main_win_handle,
                                         formwind_event_handler, (void*)(uintptr_t)docno);

                    if(!e)
                        {
                        /* Created main window */

                        formwind->mainwindow   = main_win_handle;

                        paneBB     = templatepane->box;
                        paneExtent = templatepane->ex;

                        e = wimp_create_wind(templatepane, &pane_win_handle);

                        if(!e)
                            {
                            /* Created pane window */

                            formwind->panewindow   = pane_win_handle;

                            mlec_attach(mlec, main_win_handle, pane_win_handle, paneExtent, formwind_menu_title);

                            /* To recap:                                                 */
                            /* We've created the edit box main window (where the buttons */
                            /* live) and the pane window (where mlec displays the text)  */
                            /* various event handlers have been set up and the mlec data */
                            /* structures tied to the windows                            */

                            formwind->margin.x0    = paneBB.x0 - mainBB.x0;
                            formwind->margin.y0    = paneBB.y0 - mainBB.y0;
                            formwind->margin.x1    = paneBB.x1 - mainBB.x1;
                            formwind->margin.y1    = paneBB.y1 - mainBB.y1;

                            formwind->docno    = docno;

                            mlec_attach_eventhandler(mlec, formwind_mlec_event_handler, formwind, TRUE);

                            /* Create function paster menu */
                            function__event_menu_maker();
                            event_attachmenumaker_to_w_i(main_win_handle,
                                                        (wimp_i)FORMWIND_FUNCTION_SELECTOR,
                                                        function__event_menu_filler,
                                                        function__event_menu_proc,
                                                        (void *)(uintptr_t)docno);

                            formwind_setextent(formwind, paneExtent);

                            return(0);
                            }
                      /*else                             */
                      /*    unable to create pane window */
                        }
                  /*else                             */
                  /*    unable to create main window */
                    }
              /*else                        */
              /*    template copying failed */
                }
          /*else                    */
          /*    templates not found */
            }
      /*else                          */
      /*    mlec_create gave an error */
        }
  /*else                            */
  /*    no room for FORMULA_WINDOW_HANDLE */

    formwind_destroy(formwindp);        /* destroys as much as we created */

    *formwindp = NULL;

    if(err >= 0)
        err = status_nomem();

    return(err);
}

static void
formwind_destroy(
    P_FORMULA_WINDOW_HANDLE formwindp)
{
    if(*formwindp)
        {
        formwind_pointershape_endtracking(*formwindp);

        if((*formwindp)->mlec)
            {
            mlec_detach((*formwindp)->mlec);
            mlec_destroy(&((*formwindp)->mlec));
            }

        /* SKS after 4.12 use win_ not wimp_ so that event handlers are released and
         * don't give exception-causing events after window structures freed
         * also changed _noerr to _complain and template free to template_copy_dispose
        */
        if((*formwindp)->mainwindow)
            wimpt_complain(win_delete_wind(&(*formwindp)->mainwindow));

        if((*formwindp)->panewindow)
            wimpt_complain(win_delete_wind(&(*formwindp)->panewindow));

        if((*formwindp)->maintemplate)
            template_copy_dispose((wimp_wind **) &(*formwindp)->maintemplate);

        if((*formwindp)->panetemplate)
            template_copy_dispose((wimp_wind **) &(*formwindp)->panetemplate);

        al_ptr_dispose(P_P_ANY_PEDANTIC(formwindp));
        }
}

static void
formwind_open_infront_withfocus(
    FORMULA_WINDOW_HANDLE formwind)
{
    wimp_wstate  state;

    /* Get the state of the window */
    if(wimpt_complain(wimp_get_wind_state(formwind->mainwindow, &state)) == 0)
        {
        state.o.behind = (wimp_w)-1;    /* Make sure window is opened in front */

        /* Robert wants all formula windows to open/reopen where user last placed one */
        if(formwind_position_valid)
            state.o.box = formwind_position_box;

        formwind_open_window(formwind, &state.o);

        mlec_claim_focus(formwind->mlec);
        }
}

static S32
formwind_settext(
    FORMULA_WINDOW_HANDLE formwind,
    char *text,
    S32 cursorcol, S32 cursorrow,
    BOOL newline)
{
    S32 err;

    if((err = mlec_SetText(formwind->mlec, text)) >= 0)
        {
        mlec__cursor_setpos(formwind->mlec, cursorcol, cursorrow);

        /* if newline required, do one only if text is present */
        if(newline && *text)
            err = mlec__insert_newline(formwind->mlec);
        }

    return(err);
}

static void
formwind_buildtitlestring(
    _Out_writes_z_(elemof_buffer) P_U8Z title,
    _InVal_     U32 elemof_buffer,
    COL col,
    ROW row)
{
    U8 buffer[BUF_MAX_REFERENCE];

    (void) write_ref(buffer, elemof32(buffer), current_docno(), col, row); /* always current doc */

    (void) xsnprintf(title, elemof_buffer, formwind_title_STR, buffer);
}

static BOOL
formwind_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    MLEC_HANDLE mlec;

    if(!select_document_using_callback_handle(handle))
        return(FALSE);

    mlec = editexpression_formwind->mlec;

    /* Deal with event */
    switch (e->e)
        {
        case wimp_EOPEN:                                                /* 2 */
            formwind_open_window(editexpression_formwind, &e->data.o);  /* main & pane */
            break;

        case wimp_EREDRAW:                                              /* 1 */
            return(FALSE);              /* default action */
            break;

        case wimp_ECLOSE:       /* treat a close request as a cancel */ /* 3 */
            formwind_canceledit();
            break;

        case wimp_EPTRENTER:
            formwind_pointershape_starttracking(editexpression_formwind);
            break;

        case wimp_EPTRLEAVE:
            formwind_pointershape_endtracking(editexpression_formwind);
            break;

        case wimp_EBUT:                                                 /* 6 */
            if(e->data.but.m.bbits & 0x5)   /* 'select' or 'adjust' */
                {
                switch ((int)e->data.but.m.i)
                    {
                    case FORMWIND_BUTTON_OK:
                        formwind_mergebacktext(TRUE, NULL);     /* report formula compilation errors */
                        break;

                    case FORMWIND_BUTTON_CANCEL:
                        formwind_canceledit();
                        break;

                    case FORMWIND_BUTTON_NEWLINE:
                        mlec__insert_newline(mlec);
                        break;
                    }
                }
            break;

        default:   /* Ignore any other events */
            return(FALSE);
            break;
        }

    /* done something, so... */
    return(TRUE);
}

/******************************************************************************
*
* Process open window requests sent to formwind_event_handler for "ExpEdMain",
* this involves carefull openning and positioning of both "ExpEdMain" and "ExpEdPane".
*
******************************************************************************/

#if TRUE
static void
formwind_open_window(
    FORMULA_WINDOW_HANDLE formwind,
    wimp_openstr *open)
{
    wimp_wstate   main, pane;

    wimp_get_wind_state(formwind->panewindow, &pane);           /* All(?) we want are the scroll offsets */

    pane.o.w      = formwind->panewindow;
    pane.o.box.x0 = open->box.x0 + formwind->margin.x0;         /* position pane relative to requested main position */
    pane.o.box.y0 = open->box.y0 + formwind->margin.y0;
    pane.o.box.x1 = open->box.x1 + formwind->margin.x1;
    pane.o.box.y1 = open->box.y1 + formwind->margin.y1;
                                                                /* retaining scroll offsets */
    pane.o.behind = open->behind;

    if(pane.o.behind == formwind->mainwindow)
        pane.o.behind = formwind->panewindow;

    trace_2(TRACE_APP_PD4, "opening pane before, x=%d, behind=%d",pane.o.box.x0,pane.o.behind);

    wimp_open_wind(&pane.o);

    /* Pane work area extent may prevent it openning at requested size, so read actual pane dimensions */
    /* and calculate appropriate main parameters                                                       */
    wimp_get_wind_state(formwind->panewindow, &pane);

    main.o.w      = formwind->mainwindow;
    main.o.box.x0 = pane.o.box.x0 - formwind->margin.x0;
    main.o.box.x1 = pane.o.box.x1 - formwind->margin.x1;
    main.o.box.y0 = pane.o.box.y0 - formwind->margin.y0;
    main.o.box.y1 = pane.o.box.y1 - formwind->margin.y1;
    main.o.scx    = main.o.scy = 0;
    main.o.behind = formwind->panewindow;

    wimp_open_wind(&main.o);

    /* if we were called because the pane is being resized, its possible the dimensions we tried to open main */
    /* with are naff (typically too small), so resize the pane!!!!                                            */

    wimp_get_wind_state(formwind->mainwindow, &main);

    /* note position of window, so all other formula windows are openned at the same place (Rob requested this) */
    formwind_position_box   = main.o.box;
    formwind_position_valid = TRUE;

    pane.o.w      = formwind->panewindow;
    pane.o.box.x0 = main.o.box.x0 + formwind->margin.x0;        /* position pane relative to requested main position */
    pane.o.box.y0 = main.o.box.y0 + formwind->margin.y0;
    pane.o.box.x1 = main.o.box.x1 + formwind->margin.x1;
    pane.o.box.y1 = main.o.box.y1 + formwind->margin.y1;
                                                                /* retaining scroll offsets & window stack position */
    pane.o.behind = formwind->panewindow;
    wimp_open_wind(&pane.o);
}
#else
static void
formwind_open_window(
    FORMULA_WINDOW_HANDLE formwind,
    wimp_openstr *main)
{
    MLEC_HANDLE mlec = formwind->mlec;
    wimp_wstate pane;
    wimp_wstate oldmain;

    wimp_get_wind_state(formwind->panewindow, &pane);         /* All(?) we want are the scroll offsets */
    wimp_get_wind_state(formwind->mainwindow, &oldmain);

    trace_0(TRACE_APP_PD4, "In formwind_open_window ");

    /* To mimimise repaints, when main window is moving left, open pane window  */
    /* (shifted left) first, then do open main and open pane. The second open   */
    /* pane is needed incase the main window positioning was modified to keep   */
    /* it on screen.                                                            */

#if (1)
    if (main->box.x0 <= oldmain.o.box.x0)
        {
        pane.o.w      = formwind->panewindow;

        pane.o.box.x0 = main->box.x0 + formwind->margin.x0;     /* position pane relative to main */
        pane.o.box.y0 = main->box.y0 + formwind->margin.y0;
        pane.o.box.x1 = main->box.x1 + formwind->margin.x1;
        pane.o.box.y1 = main->box.y1 + formwind->margin.y1;

        if (pane.o.box.x1 > main->box.x1) pane.o.box.x1 = main->box.x1; /* trim pane to stay within main */
        if (pane.o.box.y0 < main->box.y0) pane.o.box.y0 = main->box.y0;

        /*pane.o.x = pane.o.y = 0;*/             /* zero the scrolls!!!! - wrong for this application */
        pane.o.behind = main->behind;

        trace_2(TRACE_APP_PD4, "opening pane before, x=%d, behind=%d",pane.o.box.x0,pane.o.behind);

        wimp_open_wind(&pane.o);
        }
#endif

    /* Open main window, then open pane (based on the actual position values */
    /* returned by wimp_open_window, incase window positioning was modified  */
    /* to keep it on screen.                                                 */

    wimp_get_wind_state(formwind->panewindow, &pane);

    /* if pane window already on screen, and trying to open main window */
    /* at same layer as pane window, open main behind pane, to reduce   */
    /* flicker                                                          */

    if((pane.flags & wimp_WOPEN) && (main->behind == pane.o.behind))
        main->behind = formwind->panewindow;

    trace_2(TRACE_APP_PD4, "opening main, x=%d, behind=%d",main->box.x0,main->behind);

    wimp_open_wind(main);       /* open main (paper) window */

    pane.o.box.x0 = main->box.x0 + formwind->margin.x0;     /* position pane relative to main */
    pane.o.box.y0 = main->box.y0 + formwind->margin.y0;
    pane.o.box.x1 = main->box.x1 + formwind->margin.x1;
    pane.o.box.y1 = main->box.y1 + formwind->margin.y1;

    if (pane.o.box.x1 > main->box.x1) pane.o.box.x1 = main->box.x1;     /* trim pane to stay within main */
    if (pane.o.box.y0 < main->box.y0) pane.o.box.y0 = main->box.y0;

#if (1)
    /* hitting 'back' on main window will cause 'pane' to go behind 'main' */
    /* so read behind position of 'main' after openning                    */

    {   wimp_wstate main;
        wimp_get_wind_state(formwind->mainwindow, &main);
        pane.o.behind = main.o.behind;
    }
#else
    pane.o.behind = main->behind;
#endif

    trace_2(TRACE_APP_PD4, "opening pane after, x=%d, behind=%d",pane.o.box.x0,pane.o.behind);
    wimp_open_wind(&pane.o);

    trace_0(TRACE_APP_PD4, "leaving formwind_open_window");
}
#endif

/******************************************************************************
*
* Process close window requests sent to formwind_event_handler for "ExpEdMain".
*
******************************************************************************/

static void
formwind_canceledit(void)
{
    /* c.f. endeex in c.numbers */

    xf_inexpression_box = FALSE;
    slot_in_buffer = buffer_altered = FALSE;
    lecpos = lescrl = 0;

    /* need to redraw more if inverse cursor moved */
    if((edtslr_col != curcol)  ||  (edtslr_row != currow))
        mark_row(currowoffset);
    output_buffer = out_currslot = TRUE;
    xf_blockcursor = FALSE;

    formwind_destroy(&editexpression_formwind);

    xf_acquirecaret = TRUE;     /* Give caret back to sheet */
    xf_caretreposition = TRUE;  /* Give caret back to sheet */

    draw_screen();
}

static BOOL
formwind_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp)
{
    S32   err, len, at_pos;

    len = mlec_GetTextLen(editexpression_formwind->mlec);

    if(len >= LIN_BUFSIZ)
        {
        reperr_null(create_error(ERR_LINETOOLONG));
        return(FALSE);
        }

    /* read text into linbuf */
    mlec_GetText(editexpression_formwind->mlec, linbuf, LIN_BUFSIZ);

    /* next/prev match and its friends want to know where the caret is */
    if(caretposp)
        {
        S32 col, row;

        mlec__cursor_getpos(editexpression_formwind->mlec, &col, &row);
        *caretposp = cvt_colrow_to_offset(linbuf, col, row);
        }

    buffer_altered = TRUE;
    slot_in_buffer = TRUE;

    if(reporterror)
        {
        merexp_reterr(&err, &at_pos, TRUE);

        if(err < 0)
            {
            S32 cursorcol, cursorrow;

            cvt_offset_to_colrow(linbuf, at_pos, &cursorcol, &cursorrow, NULL);

            if((at_pos >= 0) && !check_text_or_fixit(err))
                {
                mlec__cursor_setpos(editexpression_formwind->mlec, cursorcol, cursorrow);       /* show position of error */
                return(FALSE);                                                                  /* retain editing state   */
                }

            merexp_reterr(NULL, NULL, TRUE);            /* force expression into sheet as a text cell */
            }
        }
    else
        merexp_reterr(NULL, NULL, TRUE);

    if(macro_recorder_on)
        output_formula_to_macro_file(linbuf);

    formwind_canceledit();

    return(TRUE);
}

static void
formwind_setextent(
    FORMULA_WINDOW_HANDLE formwind,
    wimp_box paneWorkArea)
{
    wimp_redrawstr  blk;

    blk.w = formwind->mainwindow;

    blk.box.x0 = 0;
    blk.box.y0 = paneWorkArea.y0 - formwind->margin.y0 + formwind->margin.y1;
    blk.box.y1 = 0;
    blk.box.x1 = paneWorkArea.x1 + formwind->margin.x0 - formwind->margin.x1;

    wimp_set_extent(&blk);
}

mlec_event_proto(static, formwind_mlec_event_handler)
{
    FORMULA_WINDOW_HANDLE formwind = handle;
    DOCNO old_docno;
    MLEC_EVENT_RETURN_CODE res = mlec_event_unknown;

    trace_1(TRACE_APP_EXPEDIT, "formwind_mlec_event_handler, rc=%d", rc);

    old_docno = change_document_using_docno(formwind->docno);

    switch(rc)
        {
        case Mlec_IsOpen:
            {
            wimp_openstr *panep = (wimp_openstr*)p_eventdata;
            wimp_openstr  main;

            wimp_open_wind(panep);      /* open pane, so scrolling works */

            main.w      = formwind->mainwindow;
            main.box.x0 = panep->box.x0 - formwind->margin.x0;
            main.box.x1 = panep->box.x1 - formwind->margin.x1;
            main.box.y0 = panep->box.y0 - formwind->margin.y0;
            main.box.y1 = panep->box.y1 - formwind->margin.y1;
            main.scx    = main.scy = 0;
            main.behind = panep->behind;

            formwind_open_window(formwind, &main);

            res = mlec_event_openned;
            }
            break;

        case Mlec_IsClose:
            formwind_canceledit();
            res = mlec_event_closed;
            break;

        case Mlec_IsEscape:
            formwind_canceledit();
            res = mlec_event_escape;
            break;

        case Mlec_IsReturn:
            formwind_mergebacktext(TRUE, NULL);         /* report formula compilation errors */
            res = mlec_event_return;
            break;
#if TRUE
        case Mlec_IsWorkAreaChanged:
            {
            wimp_redrawstr *paneExtent = (wimp_redrawstr*)p_eventdata;

            formwind_setextent(formwind, paneExtent->box);
            res = mlec_event_workareachanged;
            }
            break;
#endif
      /*default:    */
      /*    res = 0;*/
      /*    break;  */
        }

    select_document_using_docno(old_docno);
    return(res);
}

static void
formwind_pointershape_starttracking(
    FORMULA_WINDOW_HANDLE formwind)
{
    if(!formwind->tracking)
        {
        trace_0(TRACE_APP_EXPEDIT, "formwind_pointershape_starttracking()");

        formwind->tracking = TRUE;

        status_assert(Null_EventHandlerAdd(formwind_pointershape_null_handler, formwind, 0));
        }
}

static void
formwind_pointershape_endtracking(
    FORMULA_WINDOW_HANDLE formwind)
{
    if(formwind->tracking)
        {
        Null_EventHandlerRemove(formwind_pointershape_null_handler, formwind);

        setpointershape(POINTER_DEFAULT);

        formwind->tracking = FALSE;
        }
}

/******************************************************************************
*
* Call-back from null engine.
*
******************************************************************************/

null_event_proto(static, formwind_pointershape_null_handler)
{
    FORMULA_WINDOW_HANDLE formwind = (FORMULA_WINDOW_HANDLE) p_null_event_block->client_handle;

    trace_1(TRACE_APP_EXPEDIT, "formwind_pointershape_null_handler, rc=%d", p_null_event_block->rc);

    switch(p_null_event_block->rc)
        {
        case NULL_QUERY:
            trace_0(TRACE_APP_EXPEDIT, "call to ask if we want nulls");
            return(NULL_EVENTS_REQUIRED);

        case NULL_EVENT:
            {
            wimp_mousestr pointer;

            trace_0(TRACE_APP_EXPEDIT, "null call");

            if(NULL == wimp_get_point_info(&pointer))
                {
                if(pointer.w == formwind->mainwindow)   /* should be true, as we release null events when pointer leaves window */
                    {
                    switch((int)pointer.i)
                        {
                        case FORMWIND_FUNCTION_SELECTOR:
                            trace_0(TRACE_APP_EXPEDIT, "change pointer to drop-down-menu");
                            setpointershape(POINTER_DROPMENU);
                            break;

                        default:
                            trace_0(TRACE_APP_EXPEDIT, "restore default pointer");
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
* An expression returned from the formula line or formula window
* won't compile. Report the error, ask the user if he wants the
* expression treated as text, or to remain in the editor to fix
* the problem.
*
* Returns
*   TRUE   Treat as text (or return)
*   FALSE  Edit expression (or escape/close)
*
******************************************************************************/

static BOOL
check_text_or_fixit(
    S32 err)
{
    if(!dialog_box_start())
        return(FALSE);

    if(!mystr_set(&d_formula_error[1].textfield, string_lookup(err)))
        return(FALSE);

    if(!dialog_box(D_FORMULA_ERROR))
        return(FALSE);

    dialog_box_end();

    return(d_formula_error[0].option = 'T');
}

static S32
cvt_colrow_to_offset(
    char *text,
    S32 col,
    S32 row)
{
    char  ch;
    char *ptr = text;

    while((row > 0) && (0 != (ch = *ptr)))
        {
        if(ch == LF)
            --row;

        ptr++;
        }

    while((col > 0) && (0 != (ch = *ptr)))
        {
        if(ch == LF)
            break;

        --col;
        ptr++;
        }

    return(ptr - text);
}

static void
cvt_offset_to_colrow(
    char *text,
    S32 offset,
    P_S32 p_col,
    P_S32 p_row,
    BOOL *multilinep)
{
    S32 pos, cursorcol, cursorrow;
    BOOL multiline;

    for(multiline = FALSE, pos = cursorcol = cursorrow = 0; *text != '\0'; text++, pos++)
        {
#if 1
        /* SKS after 4.12 27mar92 - wimp icon treats ctrlchar as terminators so don't edit on line if they're present */
        if(*text < 0x20)
            {
            multiline = TRUE;
            if(pos < offset)
                {
                cursorcol++;
                if(*text == LF)
                    {
                    cursorcol = 0; cursorrow++;
                    }
                }
            }
#else
        if(*text == LF)
            {
            multiline = TRUE;
            if(pos < offset)
                {
                cursorcol = 0; cursorrow++;
                }
            }
#endif
        else
            {
            if(pos < offset)
                cursorcol++;
            }
        }

    *p_col = cursorcol;
    *p_row = cursorrow;

    if(multilinep)
        *multilinep = multiline;
}

/******************************************************************************
*
* Define a name.
*
* Reached from the 'Define names' entry of the function menu.
*
******************************************************************************/

extern void
DefineName_fn(void)
{
    char        *name;
    char        *contents;
    EV_OPTBLOCK  optblock;
    EV_DOCNO     cur_docno;
    S32          err;

    trace_0(TRACE_APP_EXPEDIT, "DefineName_fn()");

    if(!dialog_box_start())
        return;

    cur_docno = (EV_DOCNO) current_docno();

    while(dialog_box(D_DEFINE_NAME))
        {
        name     = d_define_name[0].textfield;
        contents = d_define_name[1].textfield;

        trace_1(TRACE_APP_EXPEDIT,"Name     : %s", trace_string(name));
        trace_1(TRACE_APP_EXPEDIT,"Refers to: %s", trace_string(contents));

        /* define the name */
        ev_set_options(&optblock, cur_docno);

        err = ev_name_make(name, cur_docno, contents, &optblock, FALSE);

        trace_1(TRACE_APP_EXPEDIT,"err=%d", err);

        if(err < 0)
            reperr_null(err);

        filealtered(TRUE);

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();
}

/******************************************************************************
*
* Edit or undefine a name.
*
* Reached from the 'Edit names' sub-directory of the function menu.
*
******************************************************************************/

extern void
EditName_fn(
    _InVal_     enum EV_RESOURCE_TYPES category,
    S32 itemno)
{
    char        *contents = NULL; /* SKS 04nov95 for 450 keep dataflower (and below comment) happy */
    EV_RESOURCE  resource;
    EV_OPTBLOCK  optblock;
    char         namebuf[BUF_MAX_PATHSTRING + EV_INTNAMLEN]; /* name may be prefixed by sheet name */
    char         argbuf[LIN_BUFSIZ + 1];
    S32          argcount;
    EV_DOCNO     cur_docno;
    S32          err;

    trace_1(TRACE_APP_EXPEDIT, "EditName_fn(%d)", itemno);

    cur_docno = (EV_DOCNO) current_docno();

    if(itemno >= 0)
        {
        ev_set_options(&optblock, cur_docno);

        ev_enum_resource_init(&resource, category, cur_docno, cur_docno, &optblock); /* names local to this document */

        if(ev_enum_resource_get(&resource, &itemno, namebuf, (MAX_PATHSTRING + EV_INTNAMLEN), argbuf, LIN_BUFSIZ, &argcount) >= 0)
            {
            /*>>>should we change trim name placed in [0].textfield to fit???, NB must keep whole namebuf for ev_name_make*/

            if( dialog_box_start() &&
                mystr_set(&d_edit_name[0].textfield, namebuf) &&
                mystr_set(&d_edit_name[1].textfield, argbuf)  )
                {
                while(dialog_box(D_EDIT_NAME))
                    {
                    contents =  d_edit_name[1].textfield;

                    trace_1(TRACE_APP_EXPEDIT,"Name     : %s", trace_string(namebuf));
                    trace_1(TRACE_APP_EXPEDIT,"Change to: %s", trace_string(contents));

                    /* redefine the name */
                    ev_set_options(&optblock, cur_docno);

                    err = ev_name_make(namebuf, cur_docno, contents, &optblock, FALSE);

                    trace_1(TRACE_APP_EXPEDIT,"err=%d", err);

                    if(err < 0)
                        reperr_null(err);

                    filealtered(TRUE);

                    if(!dialog_box_can_persist())
                        break;
                    }

                dialog_box_end();

                if(d_edit_name[2].option == 'U')
                    {
                    trace_1(TRACE_APP_EXPEDIT,"Undefine : %s", trace_string(namebuf));

                    /* undefine the name */
                    ev_set_options(&optblock, cur_docno);

                    err = ev_name_make(namebuf, cur_docno, contents /*<<< SKS says NULL surely? */, &optblock, TRUE);

                    trace_1(TRACE_APP_EXPEDIT,"err=%d", err);

                    if(err < 0)
                        reperr_null(err);

                    filealtered(TRUE);
                    }
                }
            }
        }
}

extern void
PasteName_fn(
    _InVal_     enum EV_RESOURCE_TYPES category,
    S32 itemno)
{
    EV_RESOURCE resource;
    EV_OPTBLOCK optblock;
    char        namebuf[BUF_MAX_PATHSTRING + EV_INTNAMLEN + LIN_BUFSIZ]; /* room for name (plus sheet prefix), */
                                                                       /* plus plenty of '(,,,,,,)'          */
    char        argbuf[256];
    S32         argcount;
    EV_DOCNO    cur_docno;
    S32         i;

    trace_0(TRACE_APP_EXPEDIT, "PasteName_fn");

    if(itemno >= 0)
        {
        cur_docno = (EV_DOCNO) current_docno();

        ev_set_options(&optblock, cur_docno);

        ev_enum_resource_init(&resource, category, DOCNO_NONE, cur_docno, &optblock); /* all names */

        if(ev_enum_resource_get(&resource, &itemno, namebuf, (MAX_PATHSTRING + EV_INTNAMLEN), argbuf, sizeof(argbuf) - 1, &argcount) >= 0)
            {
            trace_1(TRACE_APP_EXPEDIT, " selected '%s'", namebuf);

            if(argcount)
                {
                xstrkat(namebuf, elemof32(namebuf), "(");

                for(i = argcount; i > 1; i--)
                    xstrkat(namebuf, elemof32(namebuf), ",");

                xstrkat(namebuf, elemof32(namebuf), ")");
                }

            if(insert_string_check_numeric(namebuf, FALSE))     /* insert function, starts an editor if current cell */
                {                                               /* is empty and sheet is in numeric entry mode       */
                /* insert worked, so place caret after '(' */
                if(argcount)
                    {
                    /*backspace caret by argcount*/

                    if(xf_inexpression_box || xf_inexpression_line)
                        {
                        S32 col, row;

                        expedit_get_cursorpos(&col, &row);
                        expedit_set_cursorpos(col - argcount, row);
                        }
                    else
                        {
                        lecpos = MAX(lecpos - argcount, 0);
                        }
                    }
                }

            xf_acquirecaret = TRUE;     /* Give caret back to sheet */
            xf_caretreposition = TRUE;  /* Give caret back to sheet */

            draw_screen();
            }
        }
}

/* end of expedit.c */
