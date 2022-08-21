/* link_ev.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Link routines required and used by evaluator module */

/* MRJC January 1991 */

#include "common/gflags.h"

#include "datafmt.h"

#include "cmodules/ev_evali.h"

#include "colh_x.h"

#include "userIO_x.h"

#include "version_x.h"

/******************************************************************************
*
* make a dialog title
*
******************************************************************************/

static void
dialog_title_make(
    _InVal_     EV_DOCNO docno,
    char *buffer /*out*/,
    _InVal_     U32 elemof_buffer,
    const char *string)
{
    P_SS_DOC p_ss_doc;

    xstrkpy(buffer, elemof_buffer, string);

    if(NULL != (p_ss_doc = ev_p_ss_doc_from_docno(docno)))
    {
        xstrkat(buffer, elemof_buffer, ": [");

        xstrkat(buffer, elemof_buffer, p_ss_doc->docu_name.leaf_name);

        xstrkat(buffer, elemof_buffer, "]");
    }
}

/******************************************************************************
*
* send alert dialog to user
*
* --out--
* -1 = close/cancel
*  0 = but1/return
*  1 = but2
*
******************************************************************************/

static userIO_handle user_alert = NULL;

extern S32
ev_alert(
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_U8Z message,
    _In_opt_z_  PC_U8Z but1_text,
    _In_opt_z_  PC_U8Z but2_text)
{
    S32 res;
    char title[BUF_EV_LONGNAMLEN];

    dialog_title_make(docno, title, elemof32(title), "ALERT from PipeDream");
    if(user_alert)
        userIO_alert_user_close(&user_alert);

    res = userIO_alert_user_open(&user_alert, title, message,
                                 str_isblank(but1_text) ? OK_STR : but1_text,
                                 (NULL != but2_text) && str_isblank(but2_text) ? Cancel_STR : but2_text);

    return(res);
}

/******************************************************************************
*
* close alert dialog
*
******************************************************************************/

extern void
ev_alert_close(void)
{
    userIO_alert_user_close(&user_alert);
}

/******************************************************************************
*
* ask about status of alert dialog box
*
******************************************************************************/

extern S32
ev_alert_poll(void)
{
    return(userIO_alert_user_poll(&user_alert));
}

/******************************************************************************
*
* create a new docu thunk (from compiler) to hold some ss instance data
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_SS_DOC
ev_create_new_ss_doc(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _OutRef_    P_EV_DOCNO p_ev_docno)
{
    DOCNO docno;

    *p_ev_docno = DOCNO_NONE;

    if(DOCNO_NONE == (docno = create_new_docu_thunk(p_docu_name)))
        return(NULL);

    *p_ev_docno = (EV_DOCNO) docno;

    return(ev_p_ss_doc_from_docno(*p_ev_docno));
}

/* dispose of a docu thunk */

extern void
ev_destroy_ss_doc(
    _InVal_     EV_DOCNO ev_docno)
{
    destroy_docu_thunk((DOCNO) ev_docno);
}

/******************************************************************************
*
* get an external string
*
* a pointer is returned if possible,
* otherwise it's copied to the supplied
* buffer
*
* --out--
* <0 pointer to string only in *outpp
* =0 no string found
* >0 string copied to buffer - n bytes long
*
******************************************************************************/

extern S32
ev_external_string(
    _InRef_     PC_EV_SLR slrp,
    P_U8 *outpp,
    _Out_writes_z_(elemof_buffer) P_USTR buffer/*out*/,
    _InVal_     U32 elemof_buffer)
{
    P_CELL p_cell;
    char tbuf[LIN_BUFSIZ];

    if(NULL == (p_cell = travel_externally(ev_slr_docno(slrp), ev_slr_col(slrp), ev_slr_row(slrp))))
        return(0);

    if(p_cell->type != SL_TEXT)
        return(0);

    if(!(p_cell->flags & SL_TREFS))
    {
        *outpp = p_cell->content.text;
        return(-1);
    }

    (void) expand_cell(
                ev_slr_docno(slrp), p_cell, (ROW) ev_slr_row(slrp), tbuf, elemof32(tbuf) /*fwidth*/,
                DEFAULT_EXPAND_REFS /*expand_refs*/,
                EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ |
                EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT /*!allow_fonty_result*/ /*expand_flags*/,
                TRUE /*cff*/);

    xstrkpy(buffer, elemof_buffer, tbuf); /* plain non-fonty string */

    *outpp = buffer;

    return(strlen(buffer));
}

/******************************************************************************
*
* this routine is called by uref to update external
* format cells with dependencies. there is a subtle difference
* between these cells and external dependencies
*
******************************************************************************/

extern S32
ev_ext_uref(
    _InRef_     PC_EV_SLR byslrp,
    S16 byoffset,
    _InRef_     PC_UREF_PARM upp)
{
    switch(upp->action)
    {
    default: default_unhandled();
    case UREF_CHANGE:
    case UREF_REDRAW:
        break;

    case UREF_UREF:
    case UREF_DELETE:
    case UREF_SWAP:
    case UREF_CHANGEDOC:
    case UREF_REPLACE:
    case UREF_SWAPCELL:
        {
        P_CELL tcell;

        tcell = travel_externally(ev_slr_docno(byslrp),
                                  (COL) ev_slr_col(byslrp),
                                  (ROW) ev_slr_row(byslrp));

        assert(NULL != tcell);

        /*eportf("ex_ext_uref: text_csr_uref byoffset %d", byoffset);*/
        (void) text_csr_uref(tcell->content.text + byoffset, upp);

        break;
        }
    }

    return(0);
}

/******************************************************************************
*
* get input from user
*
* --out--
* -1 = close/cancel
*  0 = but1/return
*  1 = but2
*
******************************************************************************/

static userIO_handle user_input = NULL;

extern S32
ev_input(
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_U8Z message,
    _In_opt_z_  PC_U8Z but1_text,
    _In_opt_z_  PC_U8Z but2_text)
{
    S32 res;
    char title[BUF_EV_LONGNAMLEN];

    dialog_title_make(docno, title, elemof32(title), "PipeDream wants INPUT");
    if(user_input)
        userIO_input_from_user_close(&user_input);

    res = userIO_input_from_user_open(&user_input, title, message,
                                      str_isblank(but1_text) ? OK_STR : but1_text,
                                      (NULL != but2_text) && str_isblank(but2_text) ? Cancel_STR : but2_text);

    return(res);
}

/******************************************************************************
*
* get rid of input dialog box
*
******************************************************************************/

extern void
ev_input_close(void)
{
    userIO_input_from_user_close(&user_input);
}

/******************************************************************************
*
* ask status/get input from input box
*
******************************************************************************/

extern S32
ev_input_poll(
    P_U8 result_string,
    S32 buf_size)
{
    return(userIO_input_from_user_poll(&user_input, result_string, buf_size));
}

/******************************************************************************
*
* make a cell for the evaluator of the required size
*
******************************************************************************/

extern S32
ev_make_slot(
    _InRef_     PC_EV_SLR slrp,
    P_EV_RESULT p_ev_result)
{
    P_CELL sl;
    P_DOCU p_docu;
    DOCNO docno;
    char justify, format;
    EV_PARMS parms;
    S32 res;
    DOCNO old_docno;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        char buffer[BUF_EV_LONGNAMLEN];
        ev_trace_slr(buffer, elemof32(buffer), "ev_make_slot $$", slrp);
        trace_0(TRACE_MODULE_EVAL, buffer);
    }
#endif

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(ev_slr_docno(slrp))))  ||  docu_is_thunk(p_docu))
        return(EVAL_ERR_CANTEXTREF);

    docno = p_docu->docno;

    if((NULL != (sl = travel_externally(docno, ev_slr_col(slrp), ev_slr_row(slrp))))  &&  (sl->type == SL_NUMBER))
    {
        justify = sl->justify;
        format  = sl->format;
    }
    else
    {
        justify = J_RIGHT;
        format  = 0;
    }

    old_docno = change_document_using_docno(docno);

    filealtered(TRUE);

    if(ev_slr_row(slrp) >= numrow)
    {
        out_rebuildvert = TRUE;
        out_screen      = TRUE;
        xf_interrupted  = TRUE;
    }

    if(ev_slr_col(slrp) >= numcol)
    {
        out_rebuildhorz = TRUE;
        out_screen      = TRUE;
        xf_interrupted  = TRUE;
    }

    parms.type = EVS_CON_DATA;
    parms.control = 0;
    parms.circ = 0;

    if((res = merge_compiled_exp(slrp,
                                 justify,
                                 format,
                                 NULL,
                                 0,
                                 p_ev_result,
                                 &parms,
                                 0,
                                 TRUE)) < 0)
        res = status_nomem();

    select_document_using_docno(old_docno);

    return(res);
}

/******************************************************************************
*
* return number of cols in a doc
*
******************************************************************************/

_Check_return_
extern EV_COL
ev_numcol(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(docno)))  ||  docu_is_thunk(p_docu))
        return(0);

    return((EV_COL) p_docu->Xnumcol);
}

/******************************************************************************
*
* return number of rows in a doc
*
******************************************************************************/

_Check_return_
extern EV_ROW
ev_numrow(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(docno)))  ||  docu_is_thunk(p_docu))
        return(0);

    return((EV_ROW) p_docu->Xnumrow);
}

/******************************************************************************
*
* convert a value to a string
*
******************************************************************************/

_Check_return_
extern STATUS
ev_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_SS_DATA p_ss_data)
{
    NUMFORM_PARMS numform_parms /*= NUMFORM_PARMS_INIT*/;

    numform_parms.ustr_numform_numeric =
    numform_parms.ustr_numform_datetime =
    numform_parms.ustr_numform_texterror = ustr;

    numform_parms.p_numform_context = get_p_numform_context();

    return(numform(p_quick_ublock, P_QUICK_UBLOCK_NONE, p_ss_data, &numform_parms));
}

/******************************************************************************
*
* get pointer to spreadsheet instance data
*
******************************************************************************/

_Check_return_
_Ret_maybenone_
extern P_SS_DOC
ev_p_ss_doc_from_docno(
    _InVal_     EV_DOCNO docno)
{
    P_DOCU p_docu;

    if(NO_DOCUMENT == (p_docu = p_docu_from_docno(docno)))
        return(NULL);

    return(&p_docu->ss_instance_data.ss_doc);
}

/******************************************************************************
*
* keep recalculating till all done
*
******************************************************************************/

extern void
ev_recalc_all(void)
{
    escape_enable();

    while(!ctrlflag  &&  ev_todo_check())
        ev_recalc();

    escape_disable();

    /*colh_draw_cell_count_in_document(NULL);*/
}

/******************************************************************************
*
* tell our friends what's going on
*
******************************************************************************/

extern void
ev_recalc_status(
    S32 to_calc)
{
    if(0 != to_calc)
    {
        char buffer[BUF_EV_INTNAMLEN];

        consume_int(sprintf(buffer, S32_FMT, to_calc));

        colh_draw_cell_count_in_document(buffer);
    }
    else
        colh_draw_cell_count_in_document(NULL);
}

/******************************************************************************
*
* iff reporting, output info about this ERROR_CUSTOM
*
******************************************************************************/

extern void
ev_report_ERROR_CUSTOM(
    _InRef_     PC_SS_DATA p_ss_data)
{
    U8Z buffer[BUF_MAX_REFERENCE];
    const EV_DOCNO report_ev_docno = (EV_DOCNO) p_ss_data->arg.ss_error.docno;

    if(!reporting_is_enabled())
        return;

    (void) ev_write_docname(buffer, report_ev_docno, report_ev_docno); /* current_docno() is likely to be DOCNO_NONE */

    reportf("ERROR_CUSTOM: %s Row: %d Error: %s",
            buffer,
            p_ss_data->arg.ss_error.row + 1,
            reperr_getstr(p_ss_data->arg.ss_error.status));
}

/******************************************************************************
*
* set evaluator options block from PD options
*
******************************************************************************/

extern void
ev_set_options(
    _OutRef_    P_EV_OPTBLOCK p_optblock,
    _InVal_     EV_DOCNO docno)
{
    P_DOCU p_docu;

    p_optblock->american_date = false;
    p_optblock->upper_case = false;
    p_optblock->lf = true;
    p_optblock->upper_case_slr = true;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(docno)))  ||  docu_is_thunk(p_docu))
        return;

    if(p_docu->Xd_options_DF == 'A')
        p_optblock->american_date = 1;
}

/******************************************************************************
*
* check if there is owt on the todo list
*
******************************************************************************/

extern S32
ev_todo_check(void)
{
    EV_SLR slr;

    return(todo_get_slr(&slr));
}

/******************************************************************************
*
* travel to a cell and return an evaluator type pointer to the cell
* --out--
* <0 non-evaluator cell
* =0 no cell found
* >0 evaluator cell
*
******************************************************************************/

extern S32
ev_travel(
    _OutRef_    P_P_EV_CELL p_p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    P_CELL p_cell;

#if 1 /* NB this is a hackers' efficient version of ev_travel(); for the official version, see below */

    P_LIST_ITEM it;
    P_DOCU p_docu;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        char buffer[BUF_EV_LONGNAMLEN];
        ev_trace_slr(buffer, elemof32(buffer), "ev_travel $$", p_ev_slr);
        trace_0(TRACE_MODULE_EVAL, buffer);
    }
#endif

    *p_p_ev_cell = NULL;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(ev_slr_docno(p_ev_slr))))  ||  docu_is_thunk(p_docu))
    {
        assert(DOCNO_NONE != ev_slr_docno(p_ev_slr));
        assert(ev_slr_docno(p_ev_slr) < DOCNO_MAX);
        assert(NO_DOCUMENT != p_docu);
        assert((NO_DOCUMENT != p_docu) && docu_is_thunk(p_docu));
        return(0);
    }

    if(NULL == (it = list_gotoitem(x_indexcollb(p_docu, ev_slr_col(p_ev_slr)), ev_slr_row(p_ev_slr))))
        return(0);

    p_cell = slot_contents(it);

#else

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        char buffer[BUF_EV_LONGNAMLEN];
        ev_trace_slr(buffer, elemof32(buffer), "ev_travel $$", p_ev_slr);
        trace_0(TRACE_MODULE_EVAL, buffer);
    }
#endif

    *p_p_ev_cell = NULL;

    if(NULL == (p_cell = travel_externally(ev_slr_docno(p_ev_slr), ev_slr_col(p_ev_slr), ev_slr_row(p_ev_slr))))
        return(0);

#endif

    if(p_cell->type != SL_NUMBER)
        return(-1);

    *p_p_ev_cell = &p_cell->content.number.guts;

    return(1);
}

/* end of link_ev.c */
