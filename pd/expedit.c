/* expedit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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

#include "riscos_x.h"
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
formline_cursor_get_position(void);

static void
formline_cursor_set_position(
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
    _In_        const WimpOpenWindowBlock * const open_window);

static void
formwind_cancel_edit(void);

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
formwind_set_extent(
    FORMULA_WINDOW_HANDLE formwind,
    const BBox * const pane_extent);

MLEC_EVENT_PROTO(static, formwind_mlec_event_handler); /* callback function */

static void
formwind_pointershape_start_tracking(
    FORMULA_WINDOW_HANDLE formwind);

static void
formwind_pointershape_end_tracking(
    FORMULA_WINDOW_HANDLE formwind);

null_event_proto(static, formwind_pointershape_null_handler); /* callback function */

#define FORMWIND_BUTTON_OK         (0)
#define FORMWIND_BUTTON_CANCEL     (1)
#define FORMWIND_FUNCTION_SELECTOR (2)
#define FORMWIND_BUTTON_NEWLINE    (3)
#define FORMWIND_BUTTON_COPY       (4)
#define FORMWIND_BUTTON_CUT        (5)
#define FORMWIND_BUTTON_PASTE      (6)

static void
formwind_button_copy(void);

static void
formwind_button_cut(
    BOOL adjust);

static void
formwind_button_paste(void);

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

static BOOL formwind_visible_area_valid = FALSE;
static BBox formwind_visible_area = { 0, 0, 0, 0 };

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
    expedit_edit_current_cell(lecpos, FALSE); /* either in contents line or formula box as appropriate */
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
    /* Tests performed by expedit_edit_current_cell */
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

    expedit_edit_current_cell(lecpos, TRUE); /* force use of formula box */
}

extern void
expedit_edit_current_cell(
    S32 caretpos,
    BOOL force_inbox)
{
    P_CELL tcell;

    if(xf_inexpression_box)
    {
        trace_0(TRACE_APP_EXPEDIT, "expedit_edit_current_cell - fronting the formula window");

        winx_send_front_window_request(editexpression_formwind->fw_main_window_handle, TRUE);
        return;
    }

    trace_0(TRACE_APP_EXPEDIT, "expedit_edit_current_cell");

    if(xf_inexpression_line)
        return;

    if(!mergebuf() || xf_inexpression)
        return;

    linbuf[0] = CH_NULL;

    tcell = travel_here();

    if(NULL != tcell)
    {
        if(tcell->type == SL_PAGE)
        {
            xf_acquirecaret = TRUE;
            return;
        }

        if(tcell->justify & PROTECTED)
        {
            reperr_null(ERR_PROTECTED);
            xf_acquirecaret = TRUE;
            return;
        }

        prccon(linbuf, tcell);
    }

    start_editor(caretpos, force_inbox);
}

/******************************************************************************
*
* Called by inschr (in bufedit) to put user in formula line when in 'number mode'.
*
******************************************************************************/

extern void
expedit_edit_current_cell_freshcontents(
    BOOL force_inbox)
{
    linbuf[0] = CH_NULL;

    start_editor(0, force_inbox);
}

/******************************************************************************
*
* If current cell is a text cell, try compiling it, reporting any errors.
*
******************************************************************************/

extern void
expedit_recompile_current_cell_reporterrors(void)
{
    P_CELL  tcell;
    S32    err, at_pos;

    if(!mergebuf() || xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    linbuf[0] = CH_NULL;

    tcell = travel_here();

    if(NULL == tcell)
        return;

    if(tcell->type != SL_TEXT)
        return;

    if(tcell->justify & PROTECTED)
    {
        reperr_null(ERR_PROTECTED);
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

static BOOL /*TRUE->failed*/
expedit_get_icon_state(WimpGetIconStateBlock * p_icon_state)
{
    return(WrapOsErrorReporting_IsError(tbl_wimp_get_icon_state_x(colh_window_handle, COLH_CONTENTS_LINE, p_icon_state)));
}

extern void
expedit_transfer_line_to_box(
    BOOL newline)
{
    char title[LIN_BUFSIZ];
    /*const*/ char * formula_in_line;

    if(!xf_inexpression_line)
        return;

    formwind_buildtitlestring(title, elemof32(title), edtslr_col, edtslr_row);  /* Build string, e.g. "Formula window: A1" */

    {
    WimpGetIconStateBlock icon_state;
    if( expedit_get_icon_state(&icon_state) )
        return;
    formula_in_line = icon_state.icon.data.it.buffer;
    } /*block*/

    const S32 caretpos = formline_cursor_get_position(); /* either actual or last known caret position */

    if((      formwind_create_fill_open(&editexpression_formwind,
                                        current_docno(),
                                        formula_in_line,
                                        caretpos,   /* cursor col */
                                        0,          /* cursor row */
                                        newline,    /* performed AFTER cursor positioning */
                                        title)
       ) < 0
      )
        return;

    xf_inexpression_line = FALSE;
    colh_draw_contents_of_number_cell();
    xf_inexpression_box  = TRUE;        /* All other flags remain the same */
    colh_draw_edit_state_indicator();
}

#define expedit_force_redraw() \
    winf_changedfield(colh_window_handle, COLH_CONTENTS_LINE) /* just poke it for redraw */

static /*inline*/ BOOL
expedit_insert_char_in_line(
    _InVal_     char ch)
{
    WimpGetIconStateBlock icon_state;
    char *currpos;

    if( expedit_get_icon_state(&icon_state) )
        return(TRUE);

    const S32 length = strlen(icon_state.icon.data.it.buffer);

    S32 caretpos = formline_cursor_get_position();

    caretpos = MAX(caretpos, 0);
    caretpos = MIN(caretpos, length);

    if( (length + 1) >= icon_state.icon.data.it.buffer_size )
        return(TRUE); /* can't insert */

    currpos = icon_state.icon.data.it.buffer + caretpos;
    memmove32(currpos+1, currpos, (length - caretpos + 1));
    *currpos = ch;

    /* Nudge caret position, and force a redraw */
    ++caretpos;
    formline_cursor_set_position(caretpos);
    expedit_force_redraw();

    return(FALSE);
}

extern BOOL
expedit_insert_char(
    _InVal_     char ch)
{
    BOOL had_error = FALSE;

    if(xf_inexpression_line)
    {
        had_error = expedit_insert_char_in_line(ch);
    }
    else if(xf_inexpression_box)
    {
        had_error = (mlec__insert_char(editexpression_formwind->mlec, ch) < 0);
    }

    return(had_error);
}

static /*inline*/ BOOL
expedit_insert_string_in_line(
    _In_z_      const char *insertstr,
    _InVal_     S32 insertlen)
{
    WimpGetIconStateBlock icon_state;
    char *currpos;

    if( expedit_get_icon_state(&icon_state) )
        return(TRUE);

    const S32 length = strlen(icon_state.icon.data.it.buffer);

    S32 caretpos = formline_cursor_get_position();

    caretpos = MAX(caretpos, 0);
    caretpos = MIN(caretpos, length);

    if( (length + insertlen) >= icon_state.icon.data.it.buffer_size )
        return(TRUE); /* can't insert */

    currpos = icon_state.icon.data.it.buffer + caretpos;
    memmove32(currpos+insertlen, currpos, (length - caretpos + 1)); /* make a gap */
    memmove32(currpos, insertstr, insertlen);                       /* splice text in */

    /* Nudge caret position, and force a redraw */
    caretpos += insertlen;
    formline_cursor_set_position(caretpos);
    expedit_force_redraw();

    return(FALSE);
}

extern BOOL
expedit_insert_string(
    _In_z_      const char *insertstr)
{
    S32 insertlen = strlen(insertstr);
    BOOL had_error = FALSE;

    if(insertlen <= 0)
        return(FALSE);

    if(xf_inexpression_line)
    {
        had_error = expedit_insert_string_in_line(insertstr, insertlen);
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
        *p_col = formline_cursor_get_position();
        *p_row = 0;
    }
    else if(xf_inexpression_box)
    {
        mlec__cursor_get_position(editexpression_formwind->mlec, p_col, p_row);
    }
}

extern void
expedit_set_cursorpos(
    S32 col,
    S32 row)
{
    if(xf_inexpression_line)
    {
        formline_cursor_set_position(col);
    }
    else if(xf_inexpression_box)
    {
        mlec__cursor_set_position(editexpression_formwind->mlec, col, row);
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
        WimpGetIconStateBlock icon_state;
        if( expedit_get_icon_state(&icon_state) )
            return;

        const S32 length = strlen(icon_state.icon.data.it.buffer);

        S32 start = MIN(col_start, col_end);
        S32 end   = MAX(col_start, col_end);

        start = MAX(start, 0);
        end   = MIN(end  , length);

        if(start < end)
        {   /* chop characters from the buffer, position the caret at start of cut, and force a redraw */
            memmove32(&icon_state.icon.data.it.buffer[start], &icon_state.icon.data.it.buffer[end], (length - end + 1));
            formline_cursor_set_position(start);
            expedit_force_redraw();
        }
    }
    else if(xf_inexpression_box)
    {
        mlec__cursor_set_position(editexpression_formwind->mlec, col_start, row);
        mlec__selection_adjust(editexpression_formwind->mlec, col_end, row);
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
expedit_cancel_edit(void)
{
    if(xf_inexpression_box)
        formwind_cancel_edit();

    if(xf_inexpression_line)
        formline_cancel_edit();
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
        formwind_cancel_edit();

    select_document_using_docno(old_docno);
}

extern BOOL
expedit_owns_window(
    P_DOCU p_docu,
    _HwndRef_   HOST_WND window_handle)
{
    if(p_docu->Xxf_inexpression_box)
    {
        if(window_handle == p_docu->Xeditexpression_formwind->fw_pane_window_handle)
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

    if(multiline || force_inbox || !displaying_borders)
    {
        /* Fire up the formula box */

        char title[LIN_BUFSIZ];
        S32 err;

        formwind_buildtitlestring(title, elemof32(title), curcol, currow);          /* Build string, e.g. "Formula window: A1" */

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
        { /* Formula fits in one line, so edit it on the contents line */
        WimpGetIconStateBlock icon_state;
        if( !expedit_get_icon_state(&icon_state) )
            xstrkpy(icon_state.icon.data.it.buffer, icon_state.icon.data.it.buffer_size, linbuf);
        expedit_force_redraw();
        } /*block*/

        formline_cursor_set_position(caretpos);
        formline_claim_focus();

        { /* Show cell as being editted */
        /*seteex();*/
        output_buffer = slot_in_buffer = buffer_altered = xf_blockcursor = TRUE;

        edtslr_col = curcol;
        edtslr_row = currow; /* save current position */

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
* The formula line is a single-line expression editor based on a Window Manager indirected writable text icon.
* This icon, 'COLH_CONTENTS_LINE', is dual purpose, being used as the 'current cell contents' line until
* clicked-on, at which point it becomes an editor.
*
******************************************************************************/

extern void
formline_claim_focus(void)
{
    WimpCaret caret;

    if( WrapOsErrorReporting_IsError(tbl_wimp_get_caret_position(&caret)) )
        return;

    if( (caret.window_handle != colh_window_handle) ||
        (caret.icon_handle != COLH_CONTENTS_LINE) )
    {
        /* formline_stashedcaret is index into string,
         * set by formline_cursor_set_position or when caret was stolen from us
         */
        void_WrapOsErrorReporting(
            tbl_wimp_set_caret_position(colh_window_handle, COLH_CONTENTS_LINE,
                                        -1, -1,
                                        -1, formline_stashedcaret));
    }
  /*else                                     */
  /*    formline_stashedcaret = caret.index; */
}

static int
formline_cursor_get_position(void)
{
    WimpCaret caret;
    int pos;
    int length;

    if( WrapOsErrorReporting_IsError(tbl_wimp_get_caret_position(&caret)) )
        return(0);

    /* If the caret is within the icon, return its string index position,      */
    /* else someone has stolen the caret from us, so return the stashed value. */

    if( (caret.window_handle == colh_window_handle) &&
        (caret.icon_handle == COLH_CONTENTS_LINE) )
    {
        pos = caret.index;
    }
    else
        pos = formline_stashedcaret;

    {
    WimpGetIconStateBlock icon_state;
    if( expedit_get_icon_state(&icon_state) )
        length = 0;
    else
        length = strlen(icon_state.icon.data.it.buffer);
    } /*block*/

    pos = MAX(pos, 0);
    pos = MIN(pos, length);

    formline_stashedcaret = pos;

    return(pos);
}

static void
formline_cursor_set_position(
    int pos)
{
    int length;

    {
    WimpGetIconStateBlock icon_state;
    if( expedit_get_icon_state(&icon_state) )
        return;
    length = strlen(icon_state.icon.data.it.buffer);
    } /*block*/

    pos = MAX(pos, 0);
    pos = MIN(pos, length);

    formline_stashedcaret = pos;

    { /* If caret is within the icon, and is not at required place, reposition it. */
    WimpCaret caret;

    if( WrapOsErrorReporting_IsError(tbl_wimp_get_caret_position(&caret)) )
        return;

    if( (caret.window_handle == colh_window_handle) &&
        (caret.icon_handle == COLH_CONTENTS_LINE) &&
        (caret.index != pos) )
    {
        void_WrapOsErrorReporting(
            tbl_wimp_set_caret_position(caret.window_handle, caret.icon_handle,
                                        caret.xoffset, caret.yoffset,
                                        caret.height, pos));
    }
    } /*block*/
}

extern BOOL
formline_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp)
{
    if(xf_inexpression_line)
    {
        S32 err, at_pos;

        /* next/prev match and its friends want to know where the caret is */
        if(caretposp)
            *caretposp = formline_cursor_get_position();

        {
        WimpGetIconStateBlock icon_state;
        if( expedit_get_icon_state(&icon_state) )
            linbuf[0] = CH_NULL;
        else
        {
            assert(icon_state.icon.data.it.buffer_size == LIN_BUFSIZ);
            strcpy(linbuf, icon_state.icon.data.it.buffer);
        }
        } /*block*/

        buffer_altered = TRUE;
        slot_in_buffer = TRUE;

        if(reporterror)
        {
            merexp_reterr(&err, &at_pos, TRUE);

            if(err < 0)
            {
                if((at_pos >= 0) && !check_text_or_fixit(err))
                {
                    formline_cursor_set_position(at_pos);     /* show position of error */
                    return(FALSE);                      /* retain editing state   */
                }

                merexp_reterr(NULL, NULL, TRUE);        /* force expression into sheet as a text cell */
            }
        }
        else
            merexp_reterr(NULL, NULL, TRUE);

        if(macro_recorder_on)
            output_formula_to_macro_file(linbuf);

        formline_cancel_edit();
    }

    return(TRUE);
}

extern void
formline_cancel_edit(void)
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
        win_settitle((*formwindp)->fw_main_window_handle, title);

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
    template        *template_handle_main;
    template        *template_handle_pane;
    HOST_WND         fw_main_window_handle;
    HOST_WND         fw_pane_window_handle;
    BBox             fw_main_visible_area;
    BBox             fw_pane_visible_area;
    BBox             fw_pane_extent;

    *formwindp = formwind = al_ptr_alloc_elem(FORMULA_WINDOW, 1, &err);

    if(formwind)
    {
        formwind->mlec = NULL;
        formwind->fw_main_window_handle = HOST_WND_NONE;
        formwind->fw_pane_window_handle = HOST_WND_NONE;
        formwind->fw_main_template = NULL;
        formwind->fw_pane_template = NULL;
        formwind->tracking = FALSE;

        if((err = mlec_create(&mlec)) >= 0)
        {
            formwind->mlec = mlec;

            template_handle_main = template_find_new("ExpEdMain"); /*>>>use a manifest??*/
            template_handle_pane = template_find_new("ExpEdPane"); /*>>>use a manifest??*/

            if(template_handle_main && template_handle_pane)
            {
                WimpWindowWithBitset * fw_main_template;
                WimpWindowWithBitset * fw_pane_template;

                formwind->fw_main_template = fw_main_template = template_copy_new(template_handle_main);
                formwind->fw_pane_template = fw_pane_template = template_copy_new(template_handle_pane);

                if(formwind->fw_main_template && formwind->fw_pane_template)
                {
                    os_error * e;

                    fw_main_visible_area = fw_main_template->visible_area;

                    e = winx_create_window(formwind->fw_main_template, &fw_main_window_handle,
                                           formwind_event_handler, (void*)(uintptr_t)docno);

                    if(NULL == e)
                    {
                        /* Created main window */

                        formwind->fw_main_window_handle = fw_main_window_handle;

                        fw_pane_visible_area = fw_pane_template->visible_area;

                        fw_pane_extent = fw_pane_template->extent;

                        e = tbl_wimp_create_window(formwind->fw_pane_template, &fw_pane_window_handle);

                        if(NULL == e)
                        {
                            /* Created pane window */

                            formwind->fw_pane_window_handle = fw_pane_window_handle;

                            mlec_attach(mlec,
                                        formwind->fw_main_window_handle,
                                        formwind->fw_pane_window_handle,
                                        &fw_pane_extent,
                                        formwind_menu_title);

                            /* To recap:                                                 */
                            /* We've created the edit box main window (where the buttons */
                            /* live) and the pane window (where mlec displays the text)  */
                            /* various event handlers have been set up and the mlec data */
                            /* structures tied to the windows                            */

                            formwind->offset_left   = fw_pane_visible_area.xmin - fw_main_visible_area.xmin;
                            formwind->offset_bottom = fw_pane_visible_area.ymin - fw_main_visible_area.ymin;
                            formwind->offset_right  = fw_pane_visible_area.xmax - fw_main_visible_area.xmax;
                            formwind->offset_top    = fw_pane_visible_area.ymax - fw_main_visible_area.ymax;

                            formwind->docno = docno;

                            mlec_attach_eventhandler(mlec, formwind_mlec_event_handler, formwind, TRUE);

                            /* Create function paster menu */
                            function__event_menu_maker();

                            event_attachmenumaker_to_w_i(formwind->fw_main_window_handle,
                                                        FORMWIND_FUNCTION_SELECTOR,
                                                        function__event_menu_filler,
                                                        function__event_menu_proc,
                                                        (void *)(uintptr_t)docno);

                            formwind_set_extent(formwind, &fw_pane_extent);

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
        formwind_pointershape_end_tracking(*formwindp);

        if((*formwindp)->mlec)
        {
            mlec_detach((*formwindp)->mlec);
            mlec_destroy(&((*formwindp)->mlec));
        }

        /* SKS after PD 4.12 use win_ not wimp_ so that event handlers are released and
         * don't give exception-causing events after window structures freed
         * also changed _noerr to _complain and template free to template_copy_dispose
        */
        if((*formwindp)->fw_main_window_handle)
            void_WrapOsErrorReporting(winx_delete_window(&(*formwindp)->fw_main_window_handle));

        if((*formwindp)->fw_pane_window_handle)
            void_WrapOsErrorReporting(winx_delete_window(&(*formwindp)->fw_pane_window_handle));

        template_copy_dispose((WimpWindowWithBitset **) &(*formwindp)->fw_main_template);

        template_copy_dispose((WimpWindowWithBitset **) &(*formwindp)->fw_pane_template);

        al_ptr_dispose(P_P_ANY_PEDANTIC(formwindp));
    }
}

static void
formwind_open_infront_withfocus(
    FORMULA_WINDOW_HANDLE formwind)
{
    WimpGetWindowStateBlock window_state;

    /* Get the state of the main window */
    if( WrapOsErrorReporting_IsError(tbl_wimp_get_window_state_x(formwind->fw_main_window_handle, &window_state)) )
        return;

    assert(0 == window_state.xscroll);
    assert(0 == window_state.yscroll);

    {
    WimpOpenWindowBlock open_window_block;

    open_window_block.window_handle = formwind->fw_main_window_handle;

    /* Robert wants all subsequent formula windows to open where user last placed one */
    if(formwind_visible_area_valid)
    {
        open_window_block.visible_area = formwind_visible_area;

        open_window_block.xscroll = 0;
        open_window_block.yscroll = 0;
    }
    else
    {
        open_window_block.visible_area = window_state.visible_area;

        open_window_block.xscroll = window_state.xscroll;
        open_window_block.yscroll = window_state.yscroll;
    }

    open_window_block.behind = (wimp_w) -1; /* Make sure window is opened in front */

    formwind_open_window(formwind, &open_window_block);
    } /*block*/

    mlec_claim_focus(formwind->mlec);
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
        mlec__cursor_set_position(formwind->mlec, cursorcol, cursorrow);

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
formwind_event_Mouse_Click(
    const WimpMouseClickEvent * const mouse_click)
{
    /* 'Select' or 'Adjust' click? */
    if(mouse_click->buttons & Wimp_MouseButtonSelect)
    {
        switch(mouse_click->icon_handle)
        {
        case FORMWIND_BUTTON_OK:
            formwind_mergebacktext(TRUE, NULL); /* report formula compilation errors */
            break;

        case FORMWIND_BUTTON_CANCEL:
            formwind_cancel_edit();
            break;

        case FORMWIND_BUTTON_NEWLINE:
            mlec__insert_newline(editexpression_formwind->mlec);
            break;

        case FORMWIND_BUTTON_COPY:
            formwind_button_copy();
            break;

        case FORMWIND_BUTTON_CUT:
            formwind_button_cut(false);
            break;

        case FORMWIND_BUTTON_PASTE:
            formwind_button_paste();
            break;
        }
    }
    else if(mouse_click->buttons & Wimp_MouseButtonAdjust)
    {
        switch(mouse_click->icon_handle)
        {
        case FORMWIND_BUTTON_CUT:
            formwind_button_cut(true);
            break;

        default:
            break;
        }
    }

    /* done something, so... */
    return(TRUE);
}

static BOOL
formwind_event_Open_Window_Request(
    const WimpOpenWindowRequestEvent * const open_window_request)
{
    formwind_open_window(editexpression_formwind, open_window_request); /* main & pane */

    return(TRUE);
}

static BOOL
formwind_event_Close_Window_Request(
    const WimpCloseWindowRequestEvent * const close_window_request)
{
    UNREFERENCED_PARAMETER_InRef_(close_window_request);

    formwind_cancel_edit(); /* treat a close request as a cancel */

    return(TRUE);
}

static BOOL
formwind_event_Pointer_Leaving_Window(
    const WimpPointerLeavingWindowEvent * const pointer_leaving_window)
{
    UNREFERENCED_PARAMETER_InRef_(pointer_leaving_window);

    formwind_pointershape_end_tracking(editexpression_formwind);

    return(TRUE);
}

static BOOL
formwind_event_Pointer_Entering_Window(
    const WimpPointerEnteringWindowEvent * const pointer_entering_window)
{
    UNREFERENCED_PARAMETER_InRef_(pointer_entering_window);

    formwind_pointershape_start_tracking(editexpression_formwind);

    return(TRUE);
}

static BOOL
formwind_event_Message_HelpRequest(
    /*acked*/ WimpMessage * const user_message)
{
    const int icon_handle = user_message->data.help_request.icon_handle;
    char abuffer[256+128/*bit of slop*/];
    const U32 prefix_len = 0;
    const char * msg = NULL;

    trace_0(TRACE_APP_PD4, "Help request on formula window");

    if(drag_type != NO_DRAG_ACTIVE) /* stop pointer and message changing whilst dragging */
        return(TRUE);

    /* Terse messages for better BubbleHelp response */
    abuffer[0] = CH_NULL;

    switch(icon_handle)
    {
    case -1:
        break;

    case FORMWIND_BUTTON_OK:
        msg = help_formwind_button_edit_ok;
        break;

    case FORMWIND_BUTTON_CANCEL:
        msg = help_formwind_button_edit_cancel;
        break;

    case FORMWIND_FUNCTION_SELECTOR:
        msg = help_formwind_button_function;
        break;

    case FORMWIND_BUTTON_NEWLINE:
        msg = help_formwind_button_newline;
        break;

    case FORMWIND_BUTTON_COPY:
        msg = help_formwind_button_copy;
        break;

    case FORMWIND_BUTTON_CUT:
        msg = help_formwind_button_cut;
        break;

    case FORMWIND_BUTTON_PASTE:
        msg = help_formwind_button_paste;
        break;
    }

    if(msg)
        xstrkpy(abuffer + prefix_len, elemof32(abuffer) - prefix_len, msg);

    riscos_send_Message_HelpReply(user_message, abuffer);

    return(TRUE);
}

static BOOL
formwind_event_User_Message(
    /*poked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MHelpRequest:
        return(formwind_event_Message_HelpRequest(user_message));

    default:
        return(FALSE);
    }
}

#define fwin_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "fwin")

static BOOL
formwind_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;

    if(!select_document_using_callback_handle(handle))
        return(FALSE);

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(FALSE); /* default action */

    case Wimp_EOpenWindow:
        return(formwind_event_Open_Window_Request(&event_data->open_window_request));

    case Wimp_ECloseWindow:
        return(formwind_event_Close_Window_Request(&event_data->close_window_request));

    case Wimp_EPointerLeavingWindow:
        return(formwind_event_Pointer_Leaving_Window(&event_data->pointer_leaving_window));

    case Wimp_EPointerEnteringWindow:
        return(formwind_event_Pointer_Entering_Window(&event_data->pointer_entering_window));

    case Wimp_EMouseClick:
        return(formwind_event_Mouse_Click(&event_data->mouse_click));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        /*fwin_event_handler_report(event_code, event_data, handle);*/
        return(formwind_event_User_Message(&event_data->user_message));

    default: /* Ignore other events */
        /*fwin_event_handler_report(event_code, event_data, handle);*/
        trace_1(TRACE_APP_PD4, "unprocessed wimp event to formula window: %s",
                report_wimp_event(event_code, event_data));
        return(FALSE);
    }
}

/******************************************************************************
*
* Process open window requests sent to formwind_event_handler for "ExpEdMain",
* this involves careful opening and positioning of both "ExpEdMain" and "ExpEdPane".
*
******************************************************************************/

static void
formwind_open_window(
    FORMULA_WINDOW_HANDLE formwind,
    _In_        const WimpOpenWindowBlock * const open_window)
{
    WimpGetWindowStateBlock window_state;

    void_WrapOsErrorReporting(tbl_wimp_get_window_state_x(formwind->fw_pane_window_handle, &window_state)); /* All we want here are the scroll offsets */
    const int pane_xscroll = window_state.xscroll;
    const int pane_yscroll = window_state.yscroll;

    { /* position pane window relative to the requested main window position */
    WimpOpenWindowBlock pane_open_window_block;

    pane_open_window_block.window_handle = formwind->fw_pane_window_handle;

    pane_open_window_block.visible_area.xmin = open_window->visible_area.xmin + formwind->offset_left;
    pane_open_window_block.visible_area.ymin = open_window->visible_area.ymin + formwind->offset_bottom;
    pane_open_window_block.visible_area.xmax = open_window->visible_area.xmax + formwind->offset_right;
    pane_open_window_block.visible_area.ymax = open_window->visible_area.ymax + formwind->offset_top;

    pane_open_window_block.xscroll = pane_xscroll;
    pane_open_window_block.yscroll = pane_yscroll;

    pane_open_window_block.behind = open_window->behind;

    if(pane_open_window_block.behind == formwind->fw_main_window_handle)
        pane_open_window_block.behind = formwind->fw_pane_window_handle;

    void_WrapOsErrorReporting(tbl_wimp_open_window(&pane_open_window_block));
    } /*block*/

    /* Pane work area extent may prevent it opening at requested size, so read actual pane dimensions */
    /* and calculate appropriate main window parameters                                               */
    void_WrapOsErrorReporting(tbl_wimp_get_window_state_x(formwind->fw_pane_window_handle, &window_state));

    {
    WimpOpenWindowBlock main_open_window_block;

    main_open_window_block.window_handle = formwind->fw_main_window_handle;

    main_open_window_block.visible_area.xmin = window_state.visible_area.xmin - formwind->offset_left;
    main_open_window_block.visible_area.ymin = window_state.visible_area.ymin - formwind->offset_bottom;
    main_open_window_block.visible_area.xmax = window_state.visible_area.xmax - formwind->offset_right;
    main_open_window_block.visible_area.ymax = window_state.visible_area.ymax - formwind->offset_top;

    main_open_window_block.xscroll = 0;
    main_open_window_block.yscroll = 0;

    main_open_window_block.behind = formwind->fw_pane_window_handle;

    void_WrapOsErrorReporting(tbl_wimp_open_window(&main_open_window_block));
    } /*block*/

    /* if we were called because the pane is being resized, its possible the dimensions we tried to open main */
    /* with are naff (typically too small), so resize the pane!!!!                                            */
    void_WrapOsErrorReporting(tbl_wimp_get_window_state_x(formwind->fw_main_window_handle, &window_state));

    /* note position of window, so all other formula windows are opened at the same place (Rob requested this) */
    formwind_visible_area = window_state.visible_area;
    formwind_visible_area_valid = TRUE;

    { /* position pane window relative to the determined main window position */
    WimpOpenWindowBlock pane_open_window_block;

    pane_open_window_block.window_handle = formwind->fw_pane_window_handle;

    pane_open_window_block.visible_area.xmin = window_state.visible_area.xmin + formwind->offset_left;
    pane_open_window_block.visible_area.ymin = window_state.visible_area.ymin + formwind->offset_bottom;
    pane_open_window_block.visible_area.xmax = window_state.visible_area.xmax + formwind->offset_right;
    pane_open_window_block.visible_area.ymax = window_state.visible_area.ymax + formwind->offset_top;

    pane_open_window_block.xscroll = pane_xscroll;
    pane_open_window_block.yscroll = pane_yscroll;

    pane_open_window_block.behind = formwind->fw_pane_window_handle;

    void_WrapOsErrorReporting(tbl_wimp_open_window(&pane_open_window_block));
    } /*block*/
}

/******************************************************************************
*
* Process close window requests sent to formwind_event_handler for "ExpEdMain".
*
******************************************************************************/

static void
formwind_cancel_edit(void)
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
        reperr_null(ERR_LINETOOLONG);
        return(FALSE);
    }

    /* read text into linbuf */
    mlec_GetText(editexpression_formwind->mlec, linbuf, LIN_BUFSIZ);

    /* next/prev match and its friends want to know where the caret is */
    if(caretposp)
    {
        S32 col, row;

        mlec__cursor_get_position(editexpression_formwind->mlec, &col, &row);
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
                mlec__cursor_set_position(editexpression_formwind->mlec, cursorcol, cursorrow);       /* show position of error */
                return(FALSE);                                                                        /* retain editing state   */
            }

            merexp_reterr(NULL, NULL, TRUE);            /* force expression into sheet as a text cell */
        }
    }
    else
        merexp_reterr(NULL, NULL, TRUE);

    if(macro_recorder_on)
        output_formula_to_macro_file(linbuf);

    formwind_cancel_edit();

    return(TRUE);
}

static void
report_if_error(
    int err)
{
    if(err < 0)
        message_output(string_lookup(err));
}

static void
formwind_button_copy(void)
{
    S32 err = mlec__selection_copy(editexpression_formwind->mlec);

    report_if_error(err);
}

static void
formwind_button_cut(
    BOOL adjust)
{
    S32 err = 0;

    if(adjust)
        mlec__selection_delete(editexpression_formwind->mlec);
    else
        err = mlec__selection_cut(editexpression_formwind->mlec);

    report_if_error(err);
}

static void
formwind_button_paste(void)
{
    S32 err = mlec__atcursor_paste(editexpression_formwind->mlec);

    report_if_error(err);
}

static void
formwind_set_extent(
    FORMULA_WINDOW_HANDLE formwind,
    const BBox * const pane_extent)
{
    BBox bbox;

    bbox.xmin = 0;
    bbox.ymin = pane_extent->ymin - formwind->offset_bottom + formwind->offset_top;
    bbox.xmax = pane_extent->xmax + formwind->offset_left - formwind->offset_right;
    bbox.ymax = 0;

    void_WrapOsErrorReporting(tbl_wimp_set_extent(formwind->fw_main_window_handle, &bbox));
}

MLEC_EVENT_PROTO(static, formwind_event_Mlec_IsOpen)
{
    FORMULA_WINDOW_HANDLE formwind = handle;
    /*updated*/ WimpOpenWindowBlock * panep = (WimpOpenWindowBlock *) p_eventdata;
    WimpOpenWindowBlock main_open_window_block;

    UNREFERENCED_PARAMETER(rc);

    void_WrapOsErrorReporting(tbl_wimp_open_window(panep)); /* open pane, so scrolling works */

    main_open_window_block.window_handle = formwind->fw_main_window_handle;

    main_open_window_block.visible_area.xmin = panep->visible_area.xmin - formwind->offset_left;
    main_open_window_block.visible_area.ymin = panep->visible_area.ymin - formwind->offset_bottom;
    main_open_window_block.visible_area.xmax = panep->visible_area.xmax - formwind->offset_right;
    main_open_window_block.visible_area.ymax = panep->visible_area.ymax - formwind->offset_top;

    main_open_window_block.xscroll = 0;
    main_open_window_block.yscroll = 0;

    main_open_window_block.behind = panep->behind;

    formwind_open_window(formwind, &main_open_window_block);

    return(mlec_event_return);
}

MLEC_EVENT_PROTO(static, formwind_event_Mlec_IsWorkAreaChanged)
{
    FORMULA_WINDOW_HANDLE formwind = handle;
    const BBox * const pane_extent = (const BBox *) p_eventdata;

    UNREFERENCED_PARAMETER(rc);

    formwind_set_extent(formwind, pane_extent);

    return(mlec_event_workareachanged);
}

MLEC_EVENT_PROTO(static, formwind_mlec_event_handler)
{
    FORMULA_WINDOW_HANDLE formwind = handle;
    DOCNO old_docno = change_document_using_docno(formwind->docno);
    MLEC_EVENT_RETURN_CODE res = mlec_event_unknown;

    trace_1(TRACE_APP_EXPEDIT, "formwind_mlec_event_handler, rc=%d", rc);

    switch(rc)
    {
    case Mlec_IsOpen:
        res = formwind_event_Mlec_IsOpen(rc, handle, p_eventdata);
        break;

    case Mlec_IsClose:
        formwind_cancel_edit();
        res = mlec_event_closed;
        break;

    case Mlec_IsEscape:
        formwind_cancel_edit();
        res = mlec_event_escape;
        break;

    case Mlec_IsReturn:
        formwind_mergebacktext(TRUE, NULL);         /* report formula compilation errors */
        res = mlec_event_return;
        break;

    case Mlec_IsWorkAreaChanged:
        res = formwind_event_Mlec_IsWorkAreaChanged(rc, handle, p_eventdata);
        break;

    default:
        break;  
    }

    select_document_using_docno(old_docno);
    return(res);
}

static void
formwind_pointershape_start_tracking(
    FORMULA_WINDOW_HANDLE formwind)
{
    if(formwind->tracking)
        return;

    trace_0(TRACE_APP_EXPEDIT, "formwind_pointershape_start_tracking()");

    formwind->tracking = TRUE;

    status_assert(Null_EventHandlerAdd(formwind_pointershape_null_handler, formwind, 0));
}

static void
formwind_pointershape_end_tracking(
    FORMULA_WINDOW_HANDLE formwind)
{
    if(!formwind->tracking)
        return;

    formwind->tracking = FALSE;

    Null_EventHandlerRemove(formwind_pointershape_null_handler, formwind);

    riscos_setpointershape(POINTER_DEFAULT);
}

/******************************************************************************
*
* Call-back from null engine.
*
******************************************************************************/

null_event_proto(static, formwind_pointershape_null_event)
{
    FORMULA_WINDOW_HANDLE formwind = (FORMULA_WINDOW_HANDLE) p_null_event_block->client_handle;
    wimp_mousestr pointer;

    trace_0(TRACE_APP_EXPEDIT, "formwind null call");

    if(NULL != wimp_get_point_info(&pointer))
        return(NULL_EVENT_COMPLETED);

    if(pointer.w == formwind->fw_main_window_handle) /* should be true, as we release null events when pointer leaves window */
    {
        switch((int)pointer.i)
        {
        case FORMWIND_FUNCTION_SELECTOR:
            trace_0(TRACE_APP_EXPEDIT, "change pointer to drop-down-menu");
            riscos_setpointershape(POINTER_DROPMENU);
            break;

        default:
            trace_0(TRACE_APP_EXPEDIT, "restore default pointer");
            riscos_setpointershape(POINTER_DEFAULT);
            break;
        }
    }

    return(NULL_EVENT_COMPLETED);
}

null_event_proto(static, formwind_pointershape_null_handler)
{
    trace_1(TRACE_APP_EXPEDIT, "formwind_pointershape_null_handler, rc=%d", p_null_event_block->rc);

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY: /* call to ask if we want nulls */
        return(NULL_EVENTS_REQUIRED);

    case NULL_EVENT:
        return(formwind_pointershape_null_event(p_null_event_block));

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
    dialog_box_end(); /* take priority */

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
    S32 pos;
    S32 cursorcol = 0;
    S32 cursorrow = 0;
    BOOL multiline = FALSE;

    for(pos = 0; *text != CH_NULL; text++, pos++)
    {
        /* SKS after PD 4.12 27mar92 - wimp icon treats ctrlchar as terminators so don't edit on line if they're present */
        if(*text < 0x20)
        {
            multiline = TRUE;
            if(pos < offset)
            {
                cursorcol++;
                if(*text == LF)
                {
                    cursorcol = 0;
                    cursorrow++;
                }
            }
        }
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

static BOOL
definename_fn_core(void)
{
    char        *name = d_define_name[0].textfield;
    char        *contents = d_define_name[1].textfield;
    EV_DOCNO     cur_docno = (EV_DOCNO) current_docno();
    EV_OPTBLOCK  optblock;
    STATUS       err;

    trace_1(TRACE_APP_EXPEDIT,"Name     : %s", trace_string(name));
    trace_1(TRACE_APP_EXPEDIT,"Refers to: %s", trace_string(contents));

    /* define the name */
    ev_set_options(&optblock, cur_docno);

    err = ev_name_make(name, cur_docno, contents, &optblock, FALSE);

    trace_1(TRACE_APP_EXPEDIT,"err=%d", err);

    if(err < 0)
        reperr_null(err);

    filealtered(TRUE);

    return(TRUE);
}

extern void
DefineName_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEFINE_NAME))
    {
        if(!definename_fn_core())
            break;

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
    char        *contents = NULL; /* SKS 04nov95 for PD 4.50 keep dataflower (and below comment) happy */
    EV_RESOURCE  resource;
    EV_OPTBLOCK  optblock;
    char         namebuf[BUF_MAX_PATHSTRING + EV_INTNAMLEN]; /* name may be prefixed by sheet name */
    char         argbuf[LIN_BUFSIZ + 1];
    S32          argcount;
    EV_DOCNO     cur_docno;
    STATUS       err;

    trace_1(TRACE_APP_EXPEDIT, "EditName_fn(%d)", itemno);

    cur_docno = (EV_DOCNO) current_docno();

    if(itemno >= 0)
    {
        ev_set_options(&optblock, cur_docno);

        ev_enum_resource_init(&resource, category, cur_docno, cur_docno, &optblock); /* names local to this document */

        if(ev_enum_resource_get(&resource, &itemno, namebuf, (MAX_PATHSTRING + EV_INTNAMLEN), argbuf, LIN_BUFSIZ, &argcount) >= 0)
        {
            /*>>>should we change trim name placed in [0].textfield to fit???, NB must keep whole namebuf for ev_name_make*/

            if( init_dialog_box(D_EDIT_NAME) &&
                mystr_set(&d_edit_name[0].textfield, namebuf) &&
                mystr_set(&d_edit_name[1].textfield, argbuf)  )
            {
                if(dialog_box_start())
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
                }

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

            if(insert_string_check_number(namebuf, FALSE))      /* insert function, starts an editor if */
            {                                                   /* current cell is empty / number cell  */
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
