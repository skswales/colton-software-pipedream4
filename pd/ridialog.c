/* ridialog.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Dialog box handling for RISC OS PipeDream */

/* Stuart K. Swales 14-Mar-1989 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __cs_template_h
#include "cs-template.h"
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

#ifndef __cs_xfersend_h
#include "cs-xfersend.h"
#endif

#ifndef __print_h
#include "print.h"
#endif

#ifndef                 __colourpick_h
#include "cmodules/riscos/colourpick.h" /* no includes */
#endif

#include "riscos_x.h"
#include "pd_x.h"
#include "version_x.h"

#ifndef          __ss_const_h
#include "cmodules/ss_const.h"
#endif

#include "riscmenu.h"

#include "ridialog.h"

#include "fileicon.h"

/* ----------------------------------------------------------------------- */

static dbox dialog__dbox = NULL;

/* whether dialog box filled in OK */
static BOOL dialog__fillin_ok;

/* whether dialog box wants to persist */
static BOOL dialog__may_persist;

/* certain dialog boxes may wish to take a peek at raw events */
static dbox_raw_handler_proc dialog__extra_raw_eventhandler = NULL;

static void
dialog__register_help_for(
    dbox d);

/******************************************************************************
*
* create a named dialog box
*
******************************************************************************/

static BOOL
dialog__create_reperr(
    const char *dboxname,
    dbox *dd)
{
    char * errorp;
    dbox d = dbox_new_new(dboxname, &errorp);

    *dd = d;

    if(!d)
    {
        if(errorp)
            consume_bool(reperr(ERR_OUTPUTSTRING, errorp));
        else
            reperr_null(ERR_NOROOMFORDBOX);
    }

    return((BOOL) d);
}

static BOOL
dialog__create(
    const char * dboxname)
{
    trace_1(TRACE_APP_DIALOG, "dialog__create(%s)", dboxname);

    if(NULL != dialog__dbox)
    {
        trace_1(TRACE_APP_DIALOG, "dialog__dbox already exists: " PTR_XTFMT " - hope it's the right one! (persisting?)", report_ptr_cast(dialog__dbox));
    }
    else
    {
        dialog__extra_raw_eventhandler = NULL;

        (void) dialog__create_reperr(dboxname, &dialog__dbox);

        if(NULL != dialog__dbox)
            dialog__register_help_for(dialog__dbox);
    }

    return(NULL != dialog__dbox);
}

/******************************************************************************
*
* fillin a dialog box until any action is returned
*
******************************************************************************/

static dbox_field
dialog__fillin_for(
    dbox d)
{
    DOCNO prior_docno = current_docno();
    dbox_field f;

    f = dbox_fillin(d);

    if(DOCNO_NONE != prior_docno) /* consider iconbar... */
    {
        P_DOCU p_docu = p_docu_from_docno(prior_docno); /* in case document swapped or deleted */

        if((NO_DOCUMENT == p_docu)  ||  docu_is_thunk(p_docu))
        {
            reportf("dialog__fillin_for(&%p) returning %d to a killed caller context", report_ptr_cast(d), f);
            f = dbox_CLOSE;
        }
        else
            select_document(p_docu);
    }

    trace_2(TRACE_APP_DIALOG, "dialog__fillin_for(&%p) yields %d", report_ptr_cast(d), f);
    return(f);
}

extern S32
riscdialog_fillin_for_browse(
    void * d)
{
    return((S32) dialog__fillin_for((dbox) d));
}

static dbox_field
dialog__fillin(
    BOOL has_cancel)
{
    dbox d = dialog__dbox;
    dbox_field f;

    trace_0(TRACE_APP_DIALOG, "dialog__fillin()");

    dbox_show(d);

    f = dialog__fillin_for(d);

    if(has_cancel && (1 == f)) /* SKS 19oct96 */
        f = dbox_CLOSE;

    dialog__fillin_ok = (f == dbox_OK);

    trace_2(TRACE_APP_DIALOG, "dialog__fillin returns field %d, ok=%s", f, report_boolstring(dialog__fillin_ok));
    return(f);
}

/******************************************************************************
*
* fillin a dialog box until either closed or 'OK'ed
*
******************************************************************************/

static void
dialog__simple_fillin(
    BOOL has_cancel)
{
    dbox_field f;

    trace_0(TRACE_APP_DIALOG, "dialog__simple_fillin()");

    do { f = dialog__fillin(has_cancel); } while((f != dbox_CLOSE)  &&  (f != dbox_OK));

    trace_1(TRACE_APP_DIALOG, "dialog__simple_fillin returns dialog__fillin_ok=%s", report_boolstring(dialog__fillin_ok));
}

/******************************************************************************
*
*  dispose of a dialog box
*
******************************************************************************/

static void
dialog__dispose(void)
{
    trace_1(TRACE_APP_DIALOG, "dialog__dispose(): dialog__dbox = &%p", report_ptr_cast(dialog__dbox));

    dbox_dispose(&dialog__dbox);
}

/******************************************************************************
*
*  explicit disposal of a dialog box
*
*  NB menu tree will be killed iff needed by winx_close_wind
*
******************************************************************************/

extern void
riscdialog_dispose(void)
{
    dialog__dispose();
}

/******************************************************************************
*
* should we persist or go away?
*
******************************************************************************/

extern S32
riscdialog_ended(void)
{
    BOOL ended;

    if(!dialog__dbox)
    {
        ended = TRUE;                /* already disposed of */
        trace_0(TRACE_APP_DIALOG, "riscdialog_ended = TRUE as dialog__dbox already disposed");
    }
    else
    {
        ended = !dialog__may_persist;
        trace_1(TRACE_APP_DIALOG, "riscdialog_ended = %s", report_boolstring(ended));

        if(ended)
            /* kill menu tree too (probably dead but don't take chances) */
            riscdialog_dispose();
    }

    return(ended);
}

extern BOOL
riscdialog_in_dialog(void)
{
    return(NULL != dialog__dbox);
}

extern void
riscdialog_front_dialog(void)
{
    if(NULL != dialog__dbox)
    {
        winx_send_front_window_request(dbox_window_handle(dialog__dbox), FALSE);
    }
}

extern BOOL
riscdialog_warning(void)
{
    if(NULL != dialog__dbox)
    {
        riscdialog_front_dialog();

        return(reperr_null(ERR_ALREADY_IN_DIALOG));
    }

    if(pdfontselect_is_active())
        return(reperr_null(ERR_ALREADY_IN_DIALOG));

    return(TRUE);
}

static void
try_to_restore_caret(
    const WimpCaret * const caret,
    BOOL try_restore_to_current_document)
{
    if( try_restore_to_current_document &&
        is_current_document() &&
        (caret->window_handle == main_window_handle) )
    {
        xf_acquirecaret = TRUE; /* in case we've moved in the current document */
        return;
    }

    /* generic restore attempt */
    void_WrapOsErrorReporting(
        tbl_wimp_set_caret_position(caret->window_handle, caret->icon_handle,
                                    caret->xoffset, caret->yoffset,
                                    caret->height, caret->index));
}

/******************************************************************************
*
*  ensure created, encode, fillin and decode a dialog box
*
******************************************************************************/

extern S32
riscdialog_execute(
    dialog_proc dproc,
    const char * dname,
    DIALOG * dptr,
    _InVal_     U32 boxnumber)
{
    const char * title;

    dialog__fillin_ok = FALSE; /* will be set TRUE by a good fillin */
    dialog__may_persist = FALSE; /* will be set TRUE by a good fillin AND adjust-click */

    clearmousebuffer(); /* Keep RJM happy.  RJM says you know it makes sense */

    if(!dname)
    {
        trace_2(TRACE_APP_DIALOG, "riscdialog_execute(%s, \"\", " PTR_XTFMT ")",
                    report_procedure_name(report_proc_cast(dproc)), report_ptr_cast(dptr));

        trace_0(TRACE_APP_DIALOG, "calling dialog procedure without creating dialog__dbox");
        dproc(dptr);
    }
    else
    {
        trace_3(TRACE_APP_DIALOG, "riscdialog_execute(%s, \"%s\", " PTR_XTFMT ")",
                    report_procedure_name(report_proc_cast(dproc)), dname, report_ptr_cast(dptr));

        switch(boxnumber)
        {
        /* opnclsdict */
        case D_USER_OPEN:
            title = Open_user_dictionary_dialog_STR;
            break;

        /* opnclsdict */
        case D_USER_CLOSE:
            title = Close_user_dictionary_dialog_STR;
            break;

        /* unlockdict */
        case D_USER_LOCK:
            title = Lock_dictionary_dialog_STR;
            break;

        case D_USER_UNLOCK:
            title = Unlock_dictionary_dialog_STR;
            break;

        /* anasubgram */
        case D_USER_ANAG:
            title = Anagrams_dialog_STR;
            break;

        case D_USER_SUBGRAM:
            title = Subgrams_dialog_STR;
            break;

        /* insdelword */
        case D_USER_INSERT:
            title = Insert_word_in_user_dictionary_dialog_STR;
            break;

        case D_USER_DELETE:
            title = Delete_word_from_user_dictionary_dialog_STR;
            break;

        case D_SAVE_TEMPLATE:
            title = Save_template_STR;
            break;

        case D_EXECFILE:
            title = Do_macro_file_dialog_STR;
            break;

        case D_MACRO_FILE:
            title = Record_macro_file_dialog_STR;
            break;

        /* insremhigh */
        case D_INSHIGH:
            title = Insert_highlights_dialog_STR;
            break;

        case D_REMHIGH:
            title = Remove_highlights_dialog_STR;
            break;

        case D_LOAD_TEMPLATE:
            title = Load_Template_STR;
            break;

        case D_EDIT_DRIVER:
            title = Edit_printer_driver_STR;
            break;

        case D_INSERT_DATE:
            title = Insert_date_STR;
            break;

        case D_INSERT_TIME:
            title = Insert_time_STR;
            break;

        default:
            title = NULL;
            break;
        }

        if(title)
            template_settitle(template_find_new(dname), title);

        if(dialog__create(dname))
        {
            dproc(dptr);

            if(!dialog__fillin_ok) /* never persist if faulty */
            {
                trace_0(TRACE_APP_DIALOG, "disposing because of faulty fillin etc.");
                dialog__dispose();
            }
        }
        else
            trace_0(TRACE_APP_DIALOG, "failed to create dialog__dbox");
    }

    dialog__may_persist = dialog__fillin_ok ? dbox_persist() : FALSE;

    trace_1(TRACE_APP_DIALOG, "riscdialog_execute returns %s", report_boolstring(dialog__fillin_ok));
    return(dialog__fillin_ok);
}

#define dboxquery_FYes       0    /* action */
#define dboxquery_FMsg       1    /* string output */
#define dboxquery_FNo        2    /* action */
#define dboxquery_FCancel    3    /* optional cancel button */
#define dboxquery_FWriteable 4    /* dummy writeable icon off the bottom so we acquire focus */

static enum RISCDIALOG_QUERY_YN_REPLY
mydboxquery(
    /*const*/ char * statement,
    /*const*/ char * question,
    /*const*/ char * dboxname,
    dbox * dd,
    _InVal_     BOOL allow_adjust)
{
    enum RISCDIALOG_QUERY_YN_REPLY YN_res;
    dbox d;
    dbox_field f;

    trace_1(TRACE_APP_DIALOG, "mydboxquery(%s)", question);

    if(NULL == *dd)
    {
        if(!dialog__create_reperr(dboxname, dd))
            /* out of space - embarassing */
            return(riscdialog_query_CANCEL);
    }

    if(NULL == question)
        return(riscdialog_query_YES);

    d = *dd;

    dbox_setfield(d, 1, statement); /* the larger (upper) field */
    dbox_setfield(d, 5, question);  /* the smaller (lower) field */

    dialog__register_help_for(d);

    dbox_show(d);

    f = dialog__fillin_for(d);

    dbox_hide(d); /* don't dispose !!! */

    switch(f)
    {
    case dboxquery_FYes:
        YN_res = riscdialog_query_YES;
        if(allow_adjust && riscos_adjust_clicked())
            YN_res = riscdialog_query_NO;
        dialog__fillin_ok = TRUE;
        break;

    case dboxquery_FNo:
        YN_res = riscdialog_query_NO;
        if(allow_adjust && riscos_adjust_clicked())
            YN_res = riscdialog_query_YES;
        dialog__fillin_ok = TRUE;
        break;

    default:
        assert(f == dbox_CLOSE);

    case dboxquery_FCancel:
        YN_res = riscdialog_query_CANCEL;
        dialog__fillin_ok = FALSE;
        break;
    }

    trace_3(TRACE_APP_DIALOG, "mydboxquery(%s, %s) returns %d", trace_string(statement), question, (int) YN_res);
    return(YN_res);
}

/******************************************************************************
*
*
******************************************************************************/

static dbox queryYN_dbox = NULL;
static dbox querySDC_dbox = NULL;
static dbox quitSDC_dbox = NULL;

extern void
riscdialog_initialise_once(void)
{
    /* create at startup so we can always prompt user for most common things */
    if( (riscdialog_query_CANCEL == riscdialog_query_YN(NULL, NULL))       ||
        (riscdialog_query_CANCEL == riscdialog_query_SDC(NULL, NULL))      ||
        (riscdialog_query_CANCEL == riscdialog_query_quit_SDC(NULL, NULL)) )
    {
        exit(EXIT_FAILURE);
    }
}

extern enum RISCDIALOG_QUERY_DC_REPLY
riscdialog_query_DC(
    const char * statement,
    const char * question)
{
    dbox d = NULL;
    enum RISCDIALOG_QUERY_YN_REPLY YN_res =
        mydboxquery(de_const_cast(char *, statement), de_const_cast(char *, question), "queryDC", &d, FALSE);
    enum RISCDIALOG_QUERY_DC_REPLY DC_res;

    switch(YN_res)
    {
    case riscdialog_query_NO:
        DC_res = riscdialog_query_DC_CANCEL;
        break;

    default:
        DC_res = (enum RISCDIALOG_QUERY_DC_REPLY) YN_res;
        break;
    }

    dbox_dispose(&d);

    return(DC_res);
}

extern enum RISCDIALOG_QUERY_YN_REPLY
riscdialog_query_YN(
    const char * statement,
    const char * question)
{
    enum RISCDIALOG_QUERY_YN_REPLY YN_res =
        mydboxquery(de_const_cast(char *, statement), de_const_cast(char *, question), "queryYN", &queryYN_dbox, TRUE);

    return(YN_res);
}

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_SDC(
    const char * statement,
    const char * question)
{
    enum RISCDIALOG_QUERY_YN_REPLY YN_res =
        mydboxquery(de_const_cast(char *, statement), de_const_cast(char *, question), "querySDC", &querySDC_dbox, FALSE);
    enum RISCDIALOG_QUERY_SDC_REPLY SDC_res;

    SDC_res = (enum RISCDIALOG_QUERY_SDC_REPLY) YN_res;

    return(SDC_res);
}

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_quit_SDC(
    const char * statement,
    const char * question)
{
    enum RISCDIALOG_QUERY_YN_REPLY YN_res =
        mydboxquery(de_const_cast(char *, statement), de_const_cast(char *, question), "quitSDC", &quitSDC_dbox, FALSE);
    enum RISCDIALOG_QUERY_SDC_REPLY SDC_res;

    SDC_res = (enum RISCDIALOG_QUERY_SDC_REPLY) YN_res;

    return(SDC_res);
}

static enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_save_or_discard_existing_do_save(void)
{
    dbox_noted_position_save(); /* in case called from Quit sequence */
    application_process_command(N_SaveFile);
    dbox_noted_position_restore();

    /* may have failed to save or saved to unsafe receiver */
    if(been_error  ||  xf_filealtered)
        return(riscdialog_query_SDC_CANCEL);

    return(riscdialog_query_SDC_SAVE);
}

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_save_or_discard_existing(void)
{
    char statement_buffer[256];
    enum RISCDIALOG_QUERY_SDC_REPLY SDC_res;

    if(!xf_filealtered)
    {
        trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_or_discard_existing() returns DISCARD because file not altered");
        return(riscdialog_query_SDC_DISCARD);
    }

    consume_int(xsnprintf(statement_buffer, elemof32(statement_buffer), save_edited_file_Zs_SDC_S_STR, currentfilename));

    SDC_res = riscdialog_query_SDC(statement_buffer, save_edited_file_SDC_Q_STR);

    switch(SDC_res)
    {
    default:
        break;

    case riscdialog_query_SDC_SAVE:
        trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_or_discard_existing() got SAVE; saving file");
        SDC_res = riscdialog_query_save_or_discard_existing_do_save();
        break;
    }

    trace_1(TRACE_APP_DIALOG, "riscdialog_query_save_or_discard_existing() returns %d", (int) SDC_res);
    return(SDC_res);
}

/******************************************************************************
*
*               dialog box field manipulation procedures
*
******************************************************************************/

/******************************************************************************
*
*  get/set text fields of icons
*
******************************************************************************/

static BOOL
dialog__getfield(
    dbox_field f,
    DIALOG * dptr)
{
    char ** var = &dptr->textfield;
    char tempstring[256];

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));

    trace_3(TRACE_APP_DIALOG, "dialog__getfield(%d, " PTR_XTFMT ") yields \"%s\"",
            f, report_ptr_cast(var), tempstring);

    /* translate ctrl chars too */

    return(mystr_set(var, tempstring));
}

static BOOL
dialog__getfield_high(
    dbox_field f,
    DIALOG * dptr)
{
    char ** var = &dptr->textfield;
    char tempstring[256];
    char * src;
    char * dst;
    char ch;

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));

    trace_3(TRACE_APP_DIALOG, "dialog__getfield(%d, " PTR_XTFMT ") yields \"%s\"",
            f, report_ptr_cast(var), tempstring);

    /* translate ctrl chars */

    src = dst = tempstring;

    do  {
        if((ch = *src++) == '^')
        {
            ch = *src++;
            if(ishighlighttext(ch))
                ch = (ch - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT;
            else if(ch != '^')
                *dst++ = '^';
        }

        *dst++ = ch;
    }
    while(ch != CH_NULL);

    return(mystr_set(var, tempstring));
}

static void
dialog__setfield_high(
    dbox_field f,
    const DIALOG * dptr)
{
    char array[256];
    U32 i = 0;
    char ch;
    const char *str = dptr->textfield;

    trace_3(TRACE_APP_DIALOG, "dialog__setfield_high(%d, (&%p) \"%s\")",
                                    f, str, trace_string(str));

    if(!str)
        str = (const char *) NULLSTR;

    /* translate ctrl chars */

    do  {
        if(i >= sizeof(array) - 1)
            ch = CH_NULL;
        else
            ch = *str++;

        if((ch == '^')  ||  ishighlight(ch))
        {
            array[i++] = '^';
            if(ch != '^')
                ch = (ch - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;
        }

        array[i++] = ch;
    }
    while(ch != CH_NULL);

    dbox_setfield(dialog__dbox, f, array);
}

static void
dialog__setfield_str(
    dbox_field f,
    const char * str)
{
    reportf(/*trace_3(TRACE_APP_DIALOG,*/ "dialog__setfield_str(%d, (&%p) \"%s\")",
                                    f, str, trace_string(str));

    if(!str)
        str = (const char *) NULLSTR;

    dbox_setfield(dialog__dbox, f, (char *) str);
}

static void
dialog__setfield(
    dbox_field f,
    const DIALOG * dptr)
{
    dialog__setfield_str(f, dptr->textfield);
}

/* no getchar() needed */

static void
dialog__setchar(
    dbox_field f,
    const DIALOG * dptr)
{
    char tempstring[2];

    tempstring[0] = iscntrl(dptr->option) ? ' ' : dptr->option;
    tempstring[1] = CH_NULL;

    dbox_setfield(dialog__dbox, f, tempstring);    /* no translation */
}

/* no dialog__patch_get_colour() needed */

static void
dialog__patch_set_colour(
    dbox_field f,
    const DIALOG * dptr)
{
    const HOST_WND window_handle = dbox_window_handle(dialog__dbox);
    const int icon_handle = dbox_field_to_icon_handle(dialog__dbox, f);

    trace_4(TRACE_APP_DIALOG, "dialog__setcolour(%d, %d) window_handle %d icon_handle %d",
                f, dptr->option & 0x0F, window_handle, icon_handle);

    {
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word   = (int) ( ((dptr->option & 0x0FU) * WimpIcon_BGColour) );
    set_icon_state_block.clear_word = (int) ( ((               0x0FU) * WimpIcon_BGColour) );
    void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
}

/* a bumpable item is composed of an Inc, Dec and Value fields */

/******************************************************************************
*
* --in--
*    field actually hit, two fields to select between
*
* --out--
*    field we pretend to have hit
*
******************************************************************************/

static BOOL
dialog__adjust(
    dbox_field * fp,
    dbox_field val)
{
    dbox_field dec = val - 1;
    dbox_field inc = dec - 1;

    return(dbox_adjusthit(fp, inc, dec, riscos_adjust_clicked()));
}

/******************************************************************************
*
*  get/set text fields of numeric icons
*  for F_NUMBER, F_COLOUR, F_CHAR type entries
*
******************************************************************************/

static void
dialog__getnumericlimited(
    dbox_field f,
    DIALOG * dptr,
    S32 minval,
    S32 maxval)
{
    int num = dbox_getnumeric(dialog__dbox, f);

    num = MAX(num, minval);
    num = MIN(num, maxval);

    dptr->option = num;

    trace_2(TRACE_APP_DIALOG, "dialog__getnumeric(%d) yields %d", f, dptr->option);
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST));
}

static void
dialog__getnumeric(
    dbox_field f,
    DIALOG * dptr)
{
    dialog__getnumericlimited(f, dptr, 0, 255);
}

static void
dialog__setnumeric(
    dbox_field f,
    DIALOG * dptr)
{
    int num = dptr->option;

    trace_2(TRACE_APP_DIALOG, "dialog__setnumeric(%d, %d)", f, num);
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST));

    dbox_setnumeric(dialog__dbox, f, num);
}

static void
dialog__setnumeric_colour(
    dbox_field f,
    DIALOG * dptr)
{
    int num = dptr->option & 0x0F;

    trace_2(TRACE_APP_DIALOG, "dialog__setnumeric(%d, %d)", f, num);
    assert(dptr->type==F_COLOUR);

    dbox_setnumeric(dialog__dbox, f, num);
}

static void
dialog__bumpnumericlimited(
    dbox_field valuefield,
    DIALOG * dptr,
    dbox_field hit,
    S32 minval,
    S32 maxval)
{
    S32 delta = host_shift_pressed() ? 5 : 1;
#ifdef OLD_BUMP_NUMERIC
    S32 range = maxval - minval + 1;            /* i.e. 0..255 is 256 */
#endif
    S32 num;

    /* one is allowed to bump a composite textfield as a number */
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST) || (dptr->type==F_COMPOSITE));

    /* read current value back */
    dialog__getnumericlimited(valuefield, dptr, minval, maxval);

    /* always inc,dec,value */
    if(hit+2 == valuefield)
    {
        num = dptr->option + delta;

#ifndef OLD_BUMP_NUMERIC
        if(num > maxval)
            num = maxval;
#else
        if(num > maxval)
            num -= range;
#endif
    }
    else
    {
        num = dptr->option - delta;

#ifndef OLD_BUMP_NUMERIC
        if(num < minval)
            num = minval;
#else
        if(num < minval)
            num += range;
#endif
    }

    dptr->option = num;

    dialog__setnumeric(valuefield, dptr);
}

/* Normal numeric fields range from 0x00..0xFF */

static void
dialog__bumpnumeric(
    dbox_field valuefield,
    DIALOG *dptr,
    dbox_field hit)
{
    dialog__bumpnumericlimited(valuefield, dptr, hit, 0, 255);
}

static LIST_ITEMNO
dialog__initkeyfromstring(
    P_P_LIST_BLOCK listpp,
    PC_U8 str)
{
    PC_LIST lptr;

    for(lptr = first_in_list(listpp);
        lptr;
        lptr = next_in_list( listpp))
    {
        if(0 == C_stricmp(lptr->value, str))
            return(list_atitem(*listpp));
    }

    return(0);
}

static void
dialog__bumpstring(
    dbox_field valuefield,
    dbox_field hit,
    P_P_LIST_BLOCK listpp,
    P_LIST_ITEMNO key)
{
    P_LIST entry;

    trace_0(TRACE_APP_DIALOG, "bump that string - ");

    /* always inc,dec,value */
    if(hit+2 == valuefield)
    {
        trace_0(TRACE_APP_DIALOG, "up");

        *key = *key + 1;

        entry = search_list(listpp, *key);

        trace_1(TRACE_APP_DIALOG, "entry=%p", report_ptr_cast(entry));

        if(!entry)
        {
            *key = 0;

            entry = search_list(listpp, *key);
        }
    }
    else
    {
        LIST_ITEMNO lastkey = list_numitem(*listpp) - 1;

        trace_0(TRACE_APP_DIALOG, "down");

        if(*key == 0)
        {
            trace_1(TRACE_APP_DIALOG, "numitem=%i", list_numitem(*listpp));
            *key = lastkey;
        }
        else
        {
            *key = *key - 1;

            entry = search_list(listpp, *key);

            if(!entry)
                *key = lastkey; /* we'd got lost - restart at end of list */
        }

        entry = search_list(listpp, *key);
    }

    if(entry)
        trace_3(TRACE_APP_DIALOG, "entry=%p, ->key=%d, ->value = %s", report_ptr_cast(entry), entry->key, entry->value);
    else
        trace_0(TRACE_APP_DIALOG, "list empty");

    dialog__setfield_str(valuefield, entry ? entry->value : NULL);
}

/******************************************************************************
*
* get/set a text option
* for F_SPECIAL type entries
*
* --in--
*    dptr->option is char to set
*
* --out--
*    dptr->option = char that is set
*
******************************************************************************/

static const char *
dialog__getspecial(
    dbox_field f,
    DIALOG * dptr)
{
    dbox d = dialog__dbox;
    char tempstring[2];
    PC_U8 optptr, optlistptr;
    char ch;

    dbox_getfield(d, f, tempstring, sizeof(tempstring));

    ch = toupper(tempstring[0]);

    optlistptr = *dptr->optionlist;

    optptr = (ch == CH_NULL) ? NULL : (PC_U8) strchr(optlistptr, ch);

    if(!optptr)
    {
        optptr = optlistptr;
        dptr->option = *optptr;    /* ensure sensible */
    }
    else
        dptr->option = ch;

    trace_3(TRACE_APP_DIALOG, "dialog__getspecial(%d) returns option '%c' optptr &%p",
                f, dptr->option, optptr);
    assert(dptr->type == F_SPECIAL);

    return(optptr);
}

static void
dialog__setspecial(
    dbox_field f,
    const DIALOG * dptr)
{
    char tempstring[2];
    assert(dptr->type == F_SPECIAL);

    tempstring[0] = (char) dptr->option;
    tempstring[1] = CH_NULL;

    dialog__setfield_str(f, tempstring);
}

static void
dialog__bumpspecial(
    dbox_field valuefield,
    DIALOG * dptr,
    dbox_field hit)
{
    /* Read current value */
    PC_U8 optptr = dialog__getspecial(valuefield, dptr);
    PC_U8 optlistptr;

    optlistptr = *dptr->optionlist;

    /* always inc,dec,value */
    if(hit+2 == valuefield)
    {
        if(*++optptr == CH_NULL)
            optptr = optlistptr;
    }
    else
    {
        if(optptr-- == optlistptr)
            optptr = optlistptr + strlen(optlistptr) - 1;
    }

    dptr->option = *optptr;

    dialog__setspecial(valuefield, dptr);
}

/******************************************************************************
*
* get/set text fields
* for F_ARRAY type entries
* option is index into optionlist[]
*
******************************************************************************/

static void
dialog__getarray(
    dbox_field f,
    DIALOG * dptr)
{
    PC_U8 ** array = (PC_U8 **) dptr->optionlist;
    PC_U8 ** ptr;
    char tempstring[256];
    int res = 0;

    ptr = array;

    dbox_getfield(dialog__dbox, f, tempstring, sizeof(tempstring));
    trace_1(TRACE_APP_DIALOG, "dialog__getarray got \"%s\"", tempstring);
    assert(dptr->type == F_ARRAY);

    do  {
        trace_1(TRACE_APP_DIALOG, "comparing with \"%s\"", **ptr);
        if(0 == C_stricmp(**ptr, tempstring))
        {
            res = ptr - array;
            break;
        }
    }
    while(*++ptr);

    trace_3(TRACE_APP_DIALOG, "dialog__getarray(%d, " PTR_XTFMT ") yields %d",
            f, report_ptr_cast(array), res);
    dptr->option = (char) res;
}

static void
dialog__setarray(
    dbox_field f,
    const DIALOG * dptr)
{
    PC_U8 ** array = (PC_U8 **) dptr->optionlist;

    trace_3(TRACE_APP_DIALOG, "dialog__setarray(%d, &%p) option is %d --- ", f, report_ptr_cast(dptr), dptr->option);
    assert(dptr->type == F_ARRAY);

    dialog__setfield_str(f, *array[dptr->option]);
}

static S32
dialog__lastarrayopt(
    PC_U8 ** array)
{
    PC_U8 ** ptr;
    ptr = array - 1;
    do
        /* nothing */;
    while(*++ptr);
    trace_1(TRACE_APP_DIALOG, "last array element is array[%d", ptr - array - 1);
    return(ptr - array - 1);
}

static void
dialog__bumparray(
    dbox_field valuefield,
    DIALOG * dptr,
    dbox_field hit)
{
    PC_U8 ** array = (PC_U8 **) dptr->optionlist;

    assert(dptr->type == F_ARRAY);

    /* always inc,dec,value */
    if(hit+2 == valuefield)
    {
        if(dptr->option++ == dialog__lastarrayopt(array))
            dptr->option = 0;
    }
    else
    {
        if(dptr->option-- == 0)
            dptr->option = dialog__lastarrayopt(array);
    }

    dialog__setfield_str(valuefield, *array[dptr->option]);
}

/******************************************************************************
*
* get/set on/off fields
* for F_SPECIAL type entries
* NB. the optionlist is ignored
*
* --in--
*    'N' to clear, otherwise set
*
* --out--
*    'N' if clear, 'Y' otherwise
*
******************************************************************************/

static void
dialog__getonoff(
    dbox_field f,
    DIALOG * dptr)
{
    int option = (dbox_getnumeric(dialog__dbox, f) == 0) ? 'N' : 'Y';

    trace_2(TRACE_APP_DIALOG, "dialog__getonoff(%d) yields '%c'", f, option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(1 == strlen(*dptr->optionlist) - 1);

    dptr->option = (char) option;
}

static void
dialog__setonoff(
    dbox_field f,
    const DIALOG * dptr)
{
    trace_2(TRACE_APP_DIALOG, "dialog__setonoff(%d, '%c')", f, dptr->option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(1 == strlen(*dptr->optionlist) - 1);

    /* only 'N' sets false */
    dbox_setnumeric(dialog__dbox, f, dptr->option ^ 'N');
}

/******************************************************************************
*
* get/set a group of radio buttons
* for F_SPECIAL type entries
*
* --in--
*    dptr->option == button to set
*
* --out--
*    dptr->option == button that is set
*                    first if none set
*
******************************************************************************/

static dbox_field
dialog__whichradio(
    dbox_field start,
    dbox_field end)
{
    dbox d = dialog__dbox;
    dbox_field f = end + 1;

    do { f--; } while((f > start)  &&  (dbox_getnumeric(d, f) == 0));
    /* if none set, gets first */

    trace_3(TRACE_APP_DIALOG, "dialog__whichradio(%d, %d) returns field %d", start, end, f);

    return(f);
}

static void
dialog__getradio(
    dbox_field start,
    dbox_field end,
    DIALOG * dptr)
{
    dbox_field f = dialog__whichradio(start, end);
    PC_U8 optlistptr;

    optlistptr = *dptr->optionlist;

    dptr->option = optlistptr[f - start];

    trace_5(TRACE_APP_DIALOG, "dialog__getradio(%d, %d, \"%s\") returns field %d, option '%c'",
            start, end, (const char *) dptr->optionlist, f, dptr->option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(end - start == strlen(optlistptr) - 1);
}

static void
dialog__setradio(
    dbox_field start,
    dbox_field end,
    const DIALOG * dptr)
{
    dbox d = dialog__dbox;
    dbox_field f = start;
    dbox_field this;
    PC_U8 optlistptr;

    optlistptr = *dptr->optionlist;

    this = start + (dbox_field) (strchr(optlistptr, dptr->option) - (char *) (optlistptr));

    trace_5(TRACE_APP_DIALOG, "dialog__setradio(%d, %d, \"%s\", '%c'), index %d",
            start, end, optlistptr, dptr->option, this);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(end - start == strlen(optlistptr) - 1);

    do
        dbox_setnumeric(d, f, (f == this));
    while(++f <= end);
}

/******************************************************************************
*
* get/set on/off field & text value
* for F_COMPOSITE with Y/N type entries
*
******************************************************************************/

static void
dialog__getcomponoff(
    dbox_field f,
    DIALOG * dptr)
{
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    dialog__getonoff(f,   dptr);
    dialog__getfield(f+1, dptr);
}

static void
dialog__setcomponoff(
    dbox_field f,
    const DIALOG * dptr)
{
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    dialog__setonoff(f,   dptr);
    dialog__setfield(f+1, dptr);
}

static BOOL
dialog__raw_event_Message_HelpRequest(
    /*poked*/ WimpMessage * const user_message)
{
    trace_0(TRACE_APP_DIALOG, "HelpRequest on PipeDream dialog box");

    riscos_send_Message_HelpReply(user_message, help_dialog_window);

    return(TRUE);
}

static BOOL
dialog__raw_event_User_Message(
    /*poked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MHelpRequest:
        return(dialog__raw_event_Message_HelpRequest(user_message));

    default:
        return(FALSE); 
    }
}

static BOOL
dialog__raw_eventhandler(
    dbox d,
    void * event,
    void * handle)
{
    const int event_code = ((WimpEvent *) event)->event_code;
    WimpPollBlock * const event_data = &((WimpEvent *) event)->event_data;

    UNREFERENCED_PARAMETER(d);
    UNREFERENCED_PARAMETER(handle);

    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        if(dialog__raw_event_User_Message(&event_data->user_message))
            return(TRUE);

        /*FALLTHRU*/

    default:
        if(NULL != dialog__extra_raw_eventhandler)
            if((*  dialog__extra_raw_eventhandler)(d, event, handle))
                return(TRUE);

        return(FALSE);
    }
}

static void
dialog__register_help_for(
    dbox d)
{
    dbox_raw_eventhandler(d, dialog__raw_eventhandler, NULL);
}

/******************************************************************************
*
*                                dialog boxes
*
******************************************************************************/

/******************************************************************************
*
*  single text value dialog box
*
******************************************************************************/

#define onetext_Name 2

extern void
dproc_onetext(
    DIALOG * dptr)
{
    assert_dialog(0, D_SAVE_TEMPLATE);
    assert_dialog(0, D_NAME);
    assert_dialog(0, D_EXECFILE);
    assert_dialog(0, D_GOTO);
    assert_dialog(0, D_USER_OPEN);
    assert_dialog(0, D_USER_CLOSE);

    dialog__setfield(onetext_Name, &dptr[0]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(onetext_Name, &dptr[0]);
}

/******************************************************************************
*
*  two text values dialog box
*
******************************************************************************/

#define twotext_First  2
#define twotext_Second 3

extern void
dproc_twotext(
    DIALOG * dptr)
{
    assert_dialog(1, D_REPLICATE);
    assert_dialog(1, D_USER_DELETE);
    assert_dialog(1, D_USER_INSERT);
    assert_dialog(1, D_USER_MERGE);
    assert_dialog(1, D_USER_PACK);
    assert_dialog(1, D_DEF_CMD);
    assert_dialog(1, D_DEFINE_NAME);

    dialog__setfield(twotext_First,  &dptr[0]);
    dialog__setfield(twotext_Second, &dptr[1]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(twotext_First,  &dptr[0]);
    dialog__getfield(twotext_Second, &dptr[1]);
}

/******************************************************************************
*
* single numeric value (+/-) dialog box
*
******************************************************************************/

#define onenumeric_Inc   2
#define onenumeric_Dec   3
#define onenumeric_Value 4

extern void
dproc_onenumeric(
    DIALOG * dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSERT_PAGE_BREAK);
    assert_dialog(0, D_DELETED);

    dialog__setnumeric(onenumeric_Value, &dptr[0]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   onenumeric_Value))
            dialog__bumpnumeric(onenumeric_Value, &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed onenumeric action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getnumeric(onenumeric_Value, &dptr[0]);
}

/******************************************************************************
*
* single special value (+/-) dialog box
*
******************************************************************************/

#define onespecial_Inc   2
#define onespecial_Dec   3
#define onespecial_Value 4

extern void
dproc_onespecial(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSHIGH);
    assert_dialog(0, D_REMHIGH);

    dialog__setspecial(onespecial_Value, &dptr[0]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   onespecial_Value))
            dialog__bumpspecial(onespecial_Value, &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed onespecial action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getspecial(onespecial_Value, &dptr[0]);
}

/******************************************************************************
*
*  number (+/-) and text value dialog box
*
******************************************************************************/

#define numtext_Inc    2
#define numtext_Dec    3
#define numtext_Number 4
#define numtext_Text   5

extern void
dproc_numtext(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_WIDTH);
    assert_dialog(1, D_MARGIN);

    dialog__setnumeric(numtext_Number, &dptr[0]);
    dialog__setfield(numtext_Text,     &dptr[1]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   numtext_Number))
            dialog__bumpnumeric(numtext_Number, &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed numtext action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getnumeric(numtext_Number, &dptr[0]);
    dialog__getfield(numtext_Text,     &dptr[1]);
}

/******************************************************************************
*
*  single on/off composite dialog box
*
******************************************************************************/

#define onecomponoff_OnOff 2
#define onecomponoff_Text  3

extern void
dproc_onecomponoff(
    DIALOG *dptr)
{
    assert_dialog(0, D_USER_LOCK);
    assert_dialog(0, D_USER_UNLOCK);

    dialog__setcomponoff(onecomponoff_OnOff, &dptr[0]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getcomponoff(onecomponoff_OnOff, &dptr[0]);
}

/******************************************************************************
*
* about file dialog box
*
******************************************************************************/

#define aboutfile_Icon        0
#define aboutfile_Modified    1
#define aboutfile_Type        2
#define aboutfile_Name        3
#define aboutfile_Date        4

extern void
dproc_aboutfile(
    DIALOG *dptr)
{
    char tempstring[256];

    UNREFERENCED_PARAMETER(dptr);

    dialog__setfield_str(aboutfile_Name, riscos_obtainfilename(currentfilename));

    dialog__setfield_str(aboutfile_Modified, xf_filealtered ? YES_STR : NO_STR);

    { /* textual representation of file type */
    const FILETYPE_RISC_OS filetype = currentfiletype(current_filetype_option);
    _kernel_swi_regs rs;

    rs.r[0] = 18;
    rs.r[1] = 0;
    rs.r[2] = filetype;
    rs.r[3] = 0;
    if(NULL != WrapOsErrorReporting(_kernel_swi(OS_FSControl, &rs, &rs)))
        tempstring[0] = CH_NULL;
    else
    {
        * (int *) &tempstring[0] = rs.r[2];
        * (int *) &tempstring[4] = rs.r[3];
    }

    consume_int(sprintf(&tempstring[8], " (%3.3X)", filetype));

    dialog__setfield_str(aboutfile_Type, tempstring);

    /* make appropriate file icon in dbox */
    fileicon(dbox_syshandle(dialog__dbox), aboutfile_Icon, filetype);
    } /*block*/

    { /* textual representation of file modification date/time */
    _kernel_swi_regs rs;

    rs.r[0] = (int) &currentfileinfo;
    rs.r[1] = (int) tempstring;
    rs.r[2] = sizeof32(tempstring);
    if(NULL != WrapOsErrorReporting(_kernel_swi(OS_ConvertStandardDateAndTime, &rs, &rs)))
        tempstring[0] = CH_NULL;

    tempstring[sizeof(tempstring)-1] = CH_NULL; /* Ensure terminated */

    dialog__setfield_str(aboutfile_Date, tempstring);
    } /*block*/

    dialog__simple_fillin(FALSE);
}

/******************************************************************************
*
* about program dialog box
*
******************************************************************************/

#define aboutprog_Name      0
#define aboutprog_Author    1
#define aboutprog_Version   2
#define aboutprog_Web       7

extern void
dproc_aboutprog(
    DIALOG *dptr)
{
    dbox_field f;

    UNREFERENCED_PARAMETER(dptr);

    dialog__setfield_str(aboutprog_Author,     "\xA9" " 1987-2019 Colton Software");

    dialog__setfield_str(aboutprog_Version,    applicationversion);

    while(((f = dialog__fillin(FALSE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(aboutprog_Web == f)
        {
            char buffer[1024];
            if(NULL == _kernel_getenv("PipeDream$Web", buffer, elemof32(buffer)-1))
                status_consume(ho_help_url(buffer));
            break;
        }
    }
}

/******************************************************************************
*
* sort dbox
*
******************************************************************************/

#define sort_FirstColumn      2        /* pairs of on/off(ascending) & text(column) */

/* RJM switches out update refs on 14.9.91 */
#if 1
#define sort_MultiRowRecords  sort_FirstColumn + 2*SORT_FIELD_DEPTH + 0
#else
#define sort_UpdateReferences sort_FirstColumn + 2*SORT_FIELD_DEPTH + 0
#define sort_MultiRowRecords  sort_FirstColumn + 2*SORT_FIELD_DEPTH + 1
#endif

extern void
dproc_sort(
    DIALOG *dptr)
{
    S32 i;

    assert_dialog(SORT_MULTI_ROW, D_SORT);

    i = 0;
    do  {
        dialog__setfield(2*i + sort_FirstColumn + 0, &dptr[2*i + SORT_FIELD_COLUMN]);
        dialog__setonoff(2*i + sort_FirstColumn + 1, &dptr[2*i + SORT_FIELD_ASCENDING]);
    }
    while(++i < SORT_FIELD_DEPTH);

#if 0
    dialog__setonoff(sort_UpdateReferences, &dptr[SORT_UPDATE_REFS]);
#endif

    dialog__setonoff(sort_MultiRowRecords,  &dptr[SORT_MULTI_ROW]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    i = 0;
    do  {
        dialog__getfield(2*i + sort_FirstColumn + 0, &dptr[2*i + SORT_FIELD_COLUMN]);
        dialog__getonoff(2*i + sort_FirstColumn + 1, &dptr[2*i + SORT_FIELD_ASCENDING]);
    }
    while(++i < SORT_FIELD_DEPTH);

#if 0
    dialog__getonoff(sort_UpdateReferences, &dptr[SORT_UPDATE_REFS]);
#endif
    dialog__getonoff(sort_MultiRowRecords,  &dptr[SORT_MULTI_ROW]);
}

/******************************************************************************
*
* load template dbox
*
******************************************************************************/

/* inc field 2 */
/* dec field 3 */
#define NAME_FIELD 4

extern void
dproc_loadtemplate(
    DIALOG * dptr)
{
    dbox_field  f;
    const P_P_LIST_BLOCK list = &templates_list;
    LIST_ITEMNO key;
    P_LIST entry;

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(NAME_FIELD, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,  NAME_FIELD))
            dialog__bumpstring(NAME_FIELD, f, list, &key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed load template action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(NAME_FIELD, &dptr[0]);
}

#undef NAME_FIELD

/******************************************************************************
*
* do macro dbox
*
******************************************************************************/

/* inc field 2 */
/* dec field 3 */
#define NAME_FIELD 4

extern void
dproc_edit_driver(
    DIALOG * dptr)
{
    dbox_field  f;
    const P_P_LIST_BLOCK list = &pdrivers_list;
    LIST_ITEMNO key;
    P_LIST entry;

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(NAME_FIELD, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,  NAME_FIELD))
            dialog__bumpstring(NAME_FIELD, f, list, &key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed edit driver action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(NAME_FIELD, &dptr[0]);
}

#undef NAME_FIELD

/******************************************************************************
*
* do macro dbox
*
******************************************************************************/

/* inc field 2 */
/* dec field 3 */
#undef NAME_FILED
#define NAME_FIELD 4

extern void
dproc_execfile(
    DIALOG * dptr)
{
    dbox_field  f;
    const P_P_LIST_BLOCK list = &macros_list;
    LIST_ITEMNO key;
    P_LIST entry;

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(NAME_FIELD, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,  NAME_FIELD))
            dialog__bumpstring(NAME_FIELD, f, list, &key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed execfile action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(NAME_FIELD, &dptr[0]);
}

#undef NAME_FIELD

/******************************************************************************
*
* create dictionary dbox
*
******************************************************************************/

#define createdict_First  2
/* inc field 3 */
/* dec field 4 */
#define createdict_Second 5

extern void
dproc_createdict(
    DIALOG *dptr)
{
    dbox_field  f;
    const P_P_LIST_BLOCK list = &languages_list;
    LIST_ITEMNO key;
    P_LIST entry;

    assert_dialog(1, D_USER_CREATE);

    dialog__setfield(createdict_First,  &dptr[0]);

    key = dialog__initkeyfromstring(list,
                                    !str_isblank(dptr[1].textfield)
                                        ? dptr[1].textfield
                                        : DEFAULT_DICTDEFN_FILE_STR);

    entry = search_list(list, key);

    dialog__setfield_str(createdict_Second, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,  createdict_Second))
            dialog__bumpstring(createdict_Second, f, list, &key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed createdict action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(createdict_First,  &dptr[0]);
    dialog__getfield(createdict_Second, &dptr[1]);
}

/******************************************************************************
*
*  load file dbox
*
******************************************************************************/

enum LOADFILE_OFFSETS
{
    loadfile_Name = 4,
    loadfile_Slot,
    loadfile_SlotSpec,
    loadfile_RowRange,
    loadfile_RowRangeSpec,
#define loadfile_filetype_stt loadfile_Auto
    loadfile_Auto,
    loadfile_PipeDream,
    loadfile_Tab,
    loadfile_CSV,
    loadfile_FWP,
    loadfile_ViewSheet,
    loadfile_VIEW,
    loadfile_Paragraph
#define loadfile_filetype_end loadfile_Paragraph
};

#ifndef __cs_xferrecv_h
#include "cs-xferrecv.h"
#endif

static BOOL
loadfile__raw_event_Message_DataLoad(
    /*poked*/ WimpMessage * const user_message)
{
    char * filename;
    const FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);

    UNREFERENCED_PARAMETER_InRef_(user_message); /* xferrecv uses last message mechanism */

    reportf("Message_DataLoad(dialog): file type &%03X, name %u:%s", filetype, strlen32(filename), filename);

    dialog__setfield_str(loadfile_Name, filename);

    xferrecv_insertfileok();

    return(TRUE);
}

static BOOL
loadfile__raw_event_User_Message(
    /*poked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MDataLoad:
        return(loadfile__raw_event_Message_DataLoad(user_message));

    default:
        return(FALSE); 
    }
}

static BOOL
loadfile__raw_eventhandler(
    dbox d,
    void * event,
    void * handle)
{
    const int event_code = ((WimpEvent *) event)->event_code;
    WimpPollBlock * const event_data = &((WimpEvent *) event)->event_data;

    UNREFERENCED_PARAMETER(d);
    UNREFERENCED_PARAMETER(handle);

    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(loadfile__raw_event_User_Message(&event_data->user_message));

    default:
        return(FALSE);
    }
}

extern void
dproc_loadfile(
    DIALOG *dptr)
{
    WimpCaret caret;
    BOOL can_restore_caret = TRUE;
    BOOL do_restore_caret = TRUE;

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        can_restore_caret = FALSE;

    assert_dialog(3, D_LOAD);

    dialog__setfield(loadfile_Name,                                &dptr[0]);
    dialog__setcomponoff(loadfile_Slot,                            &dptr[1]);
    dialog__setcomponoff(loadfile_RowRange,                        &dptr[2]);
    dialog__setradio(loadfile_filetype_stt, loadfile_filetype_end, &dptr[3]);

    { /* Open and process Load dialog box as submenu iff invoked that way */
    wimp_eventstr * e = wimpt_last_event();
    BOOL submenu = ((e->e == Wimp_EUserMessage)  &&  (e->data.msg.hdr.action == Wimp_MMenuWarning));
    if(submenu)
    {
        dialog__simple_fillin(TRUE);

        if(!dialog__fillin_ok)
            return;
    }
    else
    {
        dbox_field f;

        dialog__extra_raw_eventhandler = loadfile__raw_eventhandler;

        dbox_showstatic(dialog__dbox);

        while((f = dialog__fillin_for(dialog__dbox)) != dbox_CLOSE)
        {
            if(dbox_OK == f)
            {
                dialog__fillin_ok = TRUE;
                break;
            }

            if(1 == f)
            {   /* have to deal with Cancel manually in modal dialogue box */
                f = dbox_CLOSE;
                break;
            }
        }

        if(can_restore_caret && do_restore_caret)
            try_to_restore_caret(&caret, FALSE /*just generic for load*/);
    }
    }/*block*/

    dialog__getfield(loadfile_Name,                                &dptr[0]);
    dialog__getcomponoff(loadfile_Slot,                            &dptr[1]);
    dialog__getcomponoff(loadfile_RowRange,                        &dptr[2]);
    dialog__getradio(loadfile_filetype_stt, loadfile_filetype_end, &dptr[3]);
}

/******************************************************************************
*
*  save file dbox
*
******************************************************************************/

enum SAVEFILE_OFFSETS
{
    savefile_Name = 1,    /* == xfersend_FName */
    savefile_Icon,        /* == xfersend_FIcon */
    savefile_Cancel,
    savefile_format_frame,
    savefile_format_label,
    savefile_RowSelection,
    savefile_RowSelectionSpec,
    savefile_MarkedBlock,
    savefile_SeparatorInc,
    savefile_SeparatorDec,
    savefile_Separator,
#define savefile_filetype_stt savefile_PipeDream
    savefile_PipeDream,
    savefile_Tab,
    savefile_CSV,
    savefile_FWP,
    savefile_VIEW,
    savefile_Paragraph
#define savefile_filetype_end savefile_Paragraph
};

typedef struct SAVEFILE_CALLBACK_INFO
{
    DIALOG *   dptr;
    DOCNO      docno;
}
SAVEFILE_CALLBACK_INFO;

/* xfersend calling us to handle clicks on the dbox fields */

static void
savefile_clickproc(
    dbox d,
    dbox_field f,
    int *filetypep,
    void * handle)
{
    SAVEFILE_CALLBACK_INFO * i = handle;
    DIALOG * dptr;
    PC_U8 optlistptr;

    UNREFERENCED_PARAMETER(d);

    trace_3(TRACE_APP_DIALOG, "savefile_clickproc(%d, &%p, &%p)", f, report_ptr_cast(filetypep), handle);

    if(!select_document_using_docno(i->docno))
        return;

    /* now data^ valid */
    dptr = i->dptr;

    if(dialog__adjust(&f, savefile_Separator))
        dialog__bumparray(savefile_Separator, &dptr[SAV_LINESEP], f);
    else
    {
        if((f >= savefile_filetype_stt)  &&  (f <= savefile_filetype_end))
        {
            optlistptr = *dptr[SAV_FORMAT].optionlist;
            *filetypep = currentfiletype(optlistptr[f - savefile_filetype_stt]);
        }
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed savefile_clickproc action %d", f);
    }
}

/* xfersend calling us to save a file */

static BOOL
savefile_saveproc(
    _In_z_      /*const*/ char * filename /*low lifetime*/,
    void * handle)
{
    SAVEFILE_CALLBACK_INFO * i = (SAVEFILE_CALLBACK_INFO *) handle;
    DIALOG * dptr;
    wimp_eventstr * e;
    BOOL recording;
    BOOL res = TRUE;

    trace_2(TRACE_APP_DIALOG, "savefile_saveproc(%s, %d)", filename, (int) handle);

    if(!select_document_using_docno(i->docno))
        return(FALSE);

    /* now data^ valid */
    dptr = i->dptr;

    consume_bool(mystr_set(&dptr[SAV_NAME].textfield, filename)); /* esp. for macro recorder */

    dialog__getcomponoff(savefile_RowSelection,                    &dptr[SAV_ROWCOND]);
    dialog__getonoff(savefile_MarkedBlock,                         &dptr[SAV_BLOCK]);
    dialog__getarray(savefile_Separator,                           &dptr[SAV_LINESEP]);
    dialog__getradio(savefile_filetype_stt, savefile_filetype_end, &dptr[SAV_FORMAT]);

    /* try to look for wally saves to the same window */
    e = wimpt_last_event();

    trace_1(TRACE_APP_DIALOG, "last wimp event was %s", report_wimp_event(e->e, &e->data));

    if(e->e != Wimp_EKeyPressed)
    {
        HOST_WND pointer_window_handle;
        /*int pointer_icon_handle;*/

        if(e->e == Wimp_EMouseClick) /* sodding dbox hitfield faking event ... */
        {
            pointer_window_handle = e->data.but.m.w;
            /*pointer_icon_handle = e->data.but.m.i;*/
        }
        else
        {
            wimp_mousestr ms;
            if(NULL != WrapOsErrorReporting(wimp_get_point_info(&ms)))
            {
                pointer_window_handle = HOST_WND_NONE;
            }
            else
            {
                pointer_window_handle = ms.w;
                /*pointer_icon_handle = ms.i;*/
            }
            /*trace_4(TRACE_APP_DIALOG, "mouse position at %d %d, window %d, icon %d",
                    ms.x, ms.y, pointer_window_handle, pointer_icon_handle);*/
        }

        if( (pointer_window_handle == rear_window_handle) ||
            (pointer_window_handle == main_window_handle) ||
            (pointer_window_handle == colh_window_handle) )
        {
            if('Y' != dptr[SAV_BLOCK].option)
            {
                /* do allow save of marked block into self, e.g. copying a selection */
                /* could do more checking, e.g. curpos not in marked block for save */
                res = reperr_null(ERR_CANTSAVETOITSELF);
            }
         }
    }

    if(res)
    {
        SAVE_FILE_OPTIONS save_file_options;

        /* spurt pseudo-save-command to macro file */
        recording = macro_recorder_on;

        if(recording)
        {
            DHEADER * dhptr = &dialog_head[D_SAVE];

            out_comm_start_to_macro_file(N_SaveFile);
            out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items, TRUE);
        }

        zero_struct(save_file_options);
        save_file_options.filetype_option = dptr[SAV_FORMAT].option;
        save_file_options.line_sep_option = dptr[SAV_LINESEP].option;
        if(('Y' == dptr[SAV_ROWCOND].option)  &&  !str_isblank(dptr[SAV_ROWCOND].textfield))
            save_file_options.row_condition = dptr[SAV_ROWCOND].textfield;
        if('Y' == dptr[SAV_BLOCK].option)
            save_file_options.saving_block = TRUE;
        save_file_options.temp_file = !xfersend_file_is_safe();
        res = savefile(filename, &save_file_options);

        if(recording)
            out_comm_end_to_macro_file(N_SaveFile);
    }

    return(res);
}

/* xfersend calling us to print a file */

static BOOL
savefile_printproc(
    _In_z_      /*const*/ char * filename,
    void * handle)
{
    SAVEFILE_CALLBACK_INFO * i = (SAVEFILE_CALLBACK_INFO *) handle;
    BOOL recording;
    BOOL res;

    UNREFERENCED_PARAMETER(filename);

    trace_2(TRACE_APP_DIALOG, "savefile_printproc(%s, %d)", filename, (int) handle);

    if(!select_document_using_docno(i->docno))
        return(xfersend_printFailed);

    /* spurt pseudo-print-command to macro file */
    recording = macro_recorder_on;

    if(recording)
    {
        DHEADER * dhptr = &dialog_head[D_PRINT];

        out_comm_start_to_macro_file(N_Print);
        out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items, TRUE);
        out_comm_end_to_macro_file(N_Print);
    }

    print_document();

    res = been_error ? xfersend_printFailed : xfersend_printPrinted;

    been_error = FALSE;

    return(res);
}

extern void
dproc_savefile(
    DIALOG * dptr)
{
    SAVEFILE_CALLBACK_INFO i;
    PCTSTR filename = riscos_obtainfilename(dptr[SAV_NAME].textfield);
    FILETYPE_RISC_OS filetype = currentfiletype(dptr[SAV_FORMAT].option);
    S32 estsize = 42;
    WimpCaret caret;
    BOOL can_restore_caret = TRUE;
    BOOL do_restore_caret = FALSE;

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        can_restore_caret = FALSE;

    i.dptr = dptr;
    i.docno = current_docno();

    assert_dialog(SAV_FORMAT, D_SAVE);
    assert_dialog(SAV_FORMAT, D_SAVE_POPUP);

    dialog__setfield_str(savefile_Name,                            filename);
    dialog__setcomponoff(savefile_RowSelection,                    &dptr[SAV_ROWCOND]);
    dialog__setonoff(savefile_MarkedBlock,                         &dptr[SAV_BLOCK]);
    dialog__setarray(savefile_Separator,                           &dptr[SAV_LINESEP]);
    dialog__setradio(savefile_filetype_stt, savefile_filetype_end, &dptr[SAV_FORMAT]);

    dialog__register_help_for(dialog__dbox);

    { /* Open Save dialog box as submenu iff invoked that way */
    wimp_eventstr * e = wimpt_last_event();
    BOOL submenu = ((e->e == Wimp_EUserMessage)  &&  (e->data.msg.hdr.action == Wimp_MMenuWarning));
    if(submenu)
        dbox_show(dialog__dbox);
    else
    {
        do_restore_caret = TRUE;
        dbox_showstatic(dialog__dbox);
    }
    } /*block*/

    /* SKS 22oct96 */
    xfersend_set_cancel_button(savefile_Cancel);

    /* all the action takes place in here */
    dialog__fillin_ok =
        xfersend_x(
            filetype, (char *) filename, estsize,
            savefile_saveproc,         /* save file */
            NULL,                      /* send file - RAM transmit */
            savefile_printproc,        /* print file */
            &i, dialog__dbox,
            savefile_clickproc, &i);

    if(select_document_using_docno(i.docno))
        if(can_restore_caret && do_restore_caret)
            try_to_restore_caret(&caret, TRUE);

    dialog__fillin_ok = FALSE;
}

/******************************************************************************
*
* overwrite existing file dialog box
*
******************************************************************************/

extern void
dproc_overwrite(
    DIALOG *dptr)
{
    assert_dialog(0, D_OVERWRITE);

    switch(riscdialog_query_YN(Overwrite_existing_file_YN_Q_STR, ""))
    {
    case riscdialog_query_YES:
        dptr[0].option = 'Y';
        break;

    case riscdialog_query_NO:
        dptr[0].option = 'N';
        break;

    case riscdialog_query_CANCEL:
        break;
    }
}

/******************************************************************************
*
*  continue deletion on store fail dialog box
*
******************************************************************************/

extern void
dproc_save_deleted(
    DIALOG *dptr)
{
    assert_dialog(0, D_SAVE_DELETED);

    switch(riscdialog_query_YN(Cannot_store_block_YN_S_STR, Cannot_store_block_YN_Q_STR))
    {
    case riscdialog_query_YES:
        dptr[0].option = 'Y';
        break;

    case riscdialog_query_NO:
        dptr[0].option = 'N';
        break;

    case riscdialog_query_CANCEL:
        break;
    }
}

/******************************************************************************
*
*  colours dialog box
*
******************************************************************************/

/* these come in sets of four */
#define colours_Inc      6  /*1*/
#define colours_Dec      7  /*2*/
#define colours_Number   8  /*3*/
#define colours_Patch    9  /*4*/

extern void
dproc_colours(
    DIALOG *dptr)
{
    dbox_field f;
    S32 i;
    DIALOG temp_dialog[N_COLOURS]; /* update temp copy! */

    assert_dialog(N_COLOURS - 1, D_COLOURS);

    for(i = 0; i < N_COLOURS; i++)
        temp_dialog[i] = dptr[i];

    for(i = 0; i < N_COLOURS; i++)
    {
        dialog__setnumeric_colour(4*i + colours_Number, &dptr[i]);
        dialog__patch_set_colour( 4*i + colours_Patch,  &dptr[i]);
    }

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        for(i = 0; i < N_COLOURS; i++)
        {
            if(dialog__adjust(&f, 4*i + colours_Number))
            {
                dialog__bumpnumericlimited(4*i + colours_Number, &temp_dialog[i], f, 0x00, 0x0F);
                dialog__patch_set_colour(  4*i + colours_Patch,  &temp_dialog[i]);
            }
        }
    }

    /* only update colour globals if OK */
    if(!dialog__fillin_ok)
        return;

    for(i = 0; i < N_COLOURS; i++)
        dialog__getnumericlimited(4*i + colours_Number, &dptr[i], 0x00, 0x0F);
}

extern BOOL
riscdialog_choose_colour(
    _OutRef_    P_U32 p_wimp_colour_value,
    _In_z_      PC_U8Z title)
{
    const DOCNO prior_docno = current_docno(); /* ensure preserved otherwise we will go ape on return */
    RGB rgb;
    BOOL ok;
    BOOL submenu;

    { /* suss how to open (as per dbox.c) */
    const wimp_eventstr * const e = wimpt_last_event();
    submenu = ((e->e == Wimp_EUserMessage)  &&  (e->data.msg.hdr.action == Wimp_MMenuWarning));
    } /*block*/

    rgb_set(&rgb, 0, 0, 0);

    ok = riscos_colour_picker(submenu ? COLOUR_PICKER_SUBMENU : COLOUR_PICKER_MENU,
                              submenu ? HOST_WND_NONE : main_window_handle,
                              &rgb, FALSE /*allow_transparent*/, title);

    if(DOCNO_NONE != prior_docno) /* consider iconbar... */
    {
        P_DOCU p_docu = p_docu_from_docno(prior_docno); /* in case document swapped or deleted */

        if((NO_DOCUMENT == p_docu)  ||  docu_is_thunk(p_docu))
        {
            reportf("riscdialog_choose_colour(%s) returning to a killed caller context", title);
            ok = FALSE;
        }
        else
            select_document(p_docu);
    }

    if(ok)
    {
        U32 new_wimp_colour_value = ((U32) rgb.r << 8) | ((U32) rgb.g << 16) | ((U32) rgb.b << 24);

        riscos_set_wimp_colour_value_index_byte(&new_wimp_colour_value);

#if 0 /* not this one, eh? */
        if(0 != (0x10 & new_wimp_colour_value))
            if((new_wimp_colour_value & ~0xFF) == wimptx_RGB_for_wimpcolour(new_wimp_colour_value & 0x0F))
                new_wimp_colour_value = new_wimp_colour_value & 0x0F; /* stay with index where possible for users */
#endif

        *p_wimp_colour_value = new_wimp_colour_value;
    }

    return(ok);
}

#if defined(EXTENDED_COLOUR)

typedef struct RISCDIALOG_COLOURS_EDIT_CALLBACK_INFO
{
    U32 cur_col;
    BOOL modified;
}
RISCDIALOG_COLOURS_EDIT_CALLBACK_INFO;

static dbox_field
riscdialog_colour_picker(
    _InoutRef_  P_RGB p_rgb,
    _In_z_      PC_U8Z title)
{
    const DOCNO prior_docno = current_docno(); /* ensure preserved otherwise we can't update windvars! */
    dbox_field f;
    BOOL ok;

    ok = riscos_colour_picker(COLOUR_PICKER_MENU, dbox_window_handle(dialog__dbox), p_rgb, FALSE /*allow_transparent*/, title);

    f = ok ? dbox_OK : dbox_CLOSE;

    if(DOCNO_NONE != prior_docno) /* consider iconbar... */
    {
        P_DOCU p_docu = p_docu_from_docno(prior_docno); /* in case document swapped or deleted */

        if((NO_DOCUMENT == p_docu)  ||  docu_is_thunk(p_docu))
        {
            reportf("riscdialog_colour_picker(%s) returning to a killed caller context", title);
            f = dbox_CLOSE;
        }
        else
            select_document(p_docu);
    }

    return(f);
}

static BOOL
riscdialog_colours_edit(
    DIALOG * dptr,
    _In_z_      PC_U8Z p_title)
{
    RISCDIALOG_COLOURS_EDIT_CALLBACK_INFO ci;
    U32 cur_wimp_colour_value = (U32) dptr->option;
    U32 cur_col;
    U8 title[32];

    if(0 != (0x10 & cur_wimp_colour_value))
        cur_col = cur_wimp_colour_value & ~0x1F;
    else
        cur_col = wimptx_RGB_for_wimpcolour(cur_wimp_colour_value & 0x0F);

    ci.cur_col = cur_col;

    ci.modified = FALSE;

    xstrkpy(title, elemof32(title), p_title);

    {
    const U32 red   = (cur_col >>  8) & 0xFF;
    const U32 green = (cur_col >> 16) & 0xFF;
    const U32 blue  = (cur_col >> 24) & 0xFF;
    RGB rgb;
    rgb_set(&rgb, red, green, blue);
    if(dbox_CLOSE != riscdialog_colour_picker(&rgb, title))
    {
        ci.cur_col = ((U32) rgb.r << 8) | ((U32) rgb.g << 16) | ((U32) rgb.b << 24);
        ci.modified = TRUE;
    }
    } /*block*/

    if(ci.modified)
    {
        U32 new_wimp_colour_value = ci.cur_col;

        riscos_set_wimp_colour_value_index_byte(&new_wimp_colour_value);

#if 1
        if(0 != (0x10 & new_wimp_colour_value))
            if((new_wimp_colour_value & ~0xFF) == wimptx_RGB_for_wimpcolour(new_wimp_colour_value & 0x0F))
                new_wimp_colour_value = new_wimp_colour_value & 0x0F; /* stay with index where possible for users */
#endif

        dptr->option = new_wimp_colour_value;
    }

    return(ci.modified);
}

#define component_sum(r, g, b) ( \
    3*(r) + 4*(g) + (b) )

#define component_sum_threshold (component_sum(255, 255, 255) / 2)

static void
extended_colours_update_patch(
    _InVal_     int icon_handle,
    _In_        U32 wimp_colour_value)
{
    const HOST_WND window_handle = dbox_window_handle(dialog__dbox);
    U8Z buffer[32];
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    /*reportf("ecup: %i was '%s' and '%s'", icon_handle, icon_state.icon.data.it.buffer, icon_state.icon.data.it.validation);*/

    /* set some appropriate text in the patch */
    if(0 != (0x10 & wimp_colour_value))
    {   /* display as HTML #RRGGBB */
        const U32 red   = (wimp_colour_value >>  8) & 0xFF;
        const U32 green = (wimp_colour_value >> 16) & 0xFF;
        const U32 blue  = (wimp_colour_value >> 24) & 0xFF;
        consume_int(sprintf(buffer, "#%.2X%.2X%.2X", red, green, blue));
    }
    else
    {   /* display as decimal colour index */
        consume_int(sprintf(buffer, " %d", wimp_colour_value & 0x0F));
    }
    xstrkpy(icon_state.icon.data.it.buffer, icon_state.icon.data.it.buffer_size, buffer);

    /* set this colour as the patch background */
    if(0 != (0x10 & wimp_colour_value))
    {
        const U32 bg_red   = (wimp_colour_value >>  8) & 0xFF;
        const U32 bg_green = (wimp_colour_value >> 16) & 0xFF;
        const U32 bg_blue  = (wimp_colour_value >> 24) & 0xFF;

        /* choosing a contrasting text foreground colour to suit */
        const U32 fg_wimp_colour_value =
            (component_sum(bg_red, bg_green, bg_blue) > component_sum_threshold)
                ? 0x00000000 /*black*/
                : 0xFFFFFF00 /*white*/ ;
        const U32 fg_red   = (fg_wimp_colour_value >>  8) & 0xFF;
        const U32 fg_green = (fg_wimp_colour_value >> 16) & 0xFF;
        const U32 fg_blue  = (fg_wimp_colour_value >> 24) & 0xFF;

        consume_int(sprintf(buffer, "R2;C%.2X%.2X%.2X/%.2X%.2X%.2X", fg_blue, fg_green, fg_red, bg_blue, bg_green, bg_red)); /* BBGGRR */
    }
    else
    {
        strcpy(buffer, "R2");
    }
    xstrkpy(icon_state.icon.data.it.validation, sizeof("R2;C123456/789ABC"), buffer); /* no validation_size */

    /*reportf("ecup: %i now '%s' and '%s'", icon_handle, icon_state.icon.data.it.buffer, icon_state.icon.data.it.validation);*/

    if(0 != (0x10 & wimp_colour_value))
    {   /* get Window Manager to use the values we just poked */
        winf_changedfield(window_handle, icon_handle);
    }
    else
    {   /* it is sensible to do this the old way with colour indices in icon flags word */
        const U32 bg_wimp_colour_value = wimptx_RGB_for_wimpcolour(wimp_colour_value & 0x0F);
        const U32 bg_red   = (bg_wimp_colour_value >>  8) & 0xFF;
        const U32 bg_green = (bg_wimp_colour_value >> 16) & 0xFF;
        const U32 bg_blue  = (bg_wimp_colour_value >> 24) & 0xFF;

        /* choosing a contrasting text foreground colour to suit */
        const U32 fg_wimp_colour_index =
            (component_sum(bg_red, bg_green, bg_blue) > component_sum_threshold)
                ? 7 /*black*/
                : 0 /*white*/ ;

        WimpSetIconStateBlock set_icon_state_block;
        set_icon_state_block.window_handle = window_handle;
        set_icon_state_block.icon_handle = icon_handle;
        set_icon_state_block.EOR_word = (int) (
            ((wimp_colour_value & 0x0FU) * WimpIcon_BGColour) |
            ((fg_wimp_colour_index     ) * WimpIcon_FGColour) );
        set_icon_state_block.clear_word = (int) (
            ((                    0x0FU) * WimpIcon_BGColour) |
            ((                    0x0FU) * WimpIcon_FGColour) );
        void_WrapOsErrorReporting(tbl_wimp_set_icon_state(&set_icon_state_block));
    }
}

/******************************************************************************
*
*  extended colours dialog box
*
******************************************************************************/

#define extcolours_Patch (6)
#define extcolours_Label (6 + 1*N_COLOURS)
#define extcolours_Edit  (6 + 2*N_COLOURS)

extern void
dproc_extended_colours(
    DIALOG *dptr)
{
    dbox_field f;
    S32 i;
    DIALOG temp_dialog[N_COLOURS]; /* update temp copy! */
    WimpCaret caret;
    BOOL can_restore_caret = TRUE;
    BOOL do_restore_caret = FALSE;

    assert_dialog(N_COLOURS - 1, D_EXTENDED_COLOURS);

    for(i = 0; i < N_COLOURS; i++)
        temp_dialog[i] = dptr[i];

    for(i = 0; i < N_COLOURS; i++)
        extended_colours_update_patch(i + extcolours_Patch, get_option_as_u32(&dptr[i].option));

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        can_restore_caret = FALSE;

    do_restore_caret = TRUE; /* have to restore manually as it's not a real submenu window */
    dbox_showstatic(dialog__dbox);

    while((f = dialog__fillin_for(dialog__dbox)) != dbox_CLOSE)
    {
        if(dbox_OK == f)
        {
            dialog__fillin_ok = TRUE;
            break;
        }

        if(1 == f)
        {   /* have to deal with Cancel manually in modal dialogue box */
            f = dbox_CLOSE;
            break;
        }

        if((f >= extcolours_Edit) && (f < extcolours_Edit + N_COLOURS))
        {   /* one of the Edit.. buttons has been clicked */
            /* move from this modal dialogue box to the colour picker */
            const S32 idx = f - extcolours_Edit;
            WimpGetIconStateBlock icon_state;
            icon_state.window_handle = dbox_window_handle(dialog__dbox);
            icon_state.icon_handle = idx + extcolours_Label;
            if(NULL == WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
            {
                const char * title = icon_state.icon.data.it.buffer;

                if(riscdialog_colours_edit(&temp_dialog[idx], title))
                    extended_colours_update_patch(idx + extcolours_Patch, (U32) temp_dialog[idx].option);
            }
        }
    }

    if(can_restore_caret && do_restore_caret)
        try_to_restore_caret(&caret, FALSE /*just generic for colours*/);

    /* only update colour globals if OK (and these are the same data as dproc_colours) */
    /* reportf("colours dfok %d: ", dialog__fillin_ok); */
    if(!dialog__fillin_ok)
        return;

    for(i = 0; i < N_COLOURS; i++)
    {
        dptr[i].option = temp_dialog[i].option;
        reportf("colours[%d]: " U32_XTFMT, i, dptr[i].option);
    }
}

#endif /* EXTENDED_COLOUR */

/******************************************************************************
*
*  options dialog box
*
******************************************************************************/

#define options_Title           2
#define options_SlotText        3
#define options_SlotNumbers     4
#define options_InsWrapRow      5
#define options_InsWrapCol      6
#define options_InsertOnReturn  7
#define options_Kerning         8
#define options_Wrap            9
#define options_Justify         10
#define options_Borders         11
#define options_Grid            12
#define options_DecimalInc      13
#define options_DecimalDec      14
#define options_Decimal         15
#define options_NegMinus        16
#define options_NegBrackets     17
#define options_ThousandsInc    18
#define options_ThousandsDec    19
#define options_Thousands       20
#define options_DateText        21
#define options_DateDM          22 /* British */
#define options_DateMD          23 /* American */
#define options_LeadingChars    24
#define options_TrailingChars   25
#define options_TextAtChar      26

extern void
dproc_options(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(14, D_OPTIONS);

    dialog__setfield_high(options_Title,                          &dptr[0]);
    dialog__setradio(options_SlotText, options_SlotNumbers,       &dptr[1]);
    dialog__setradio(options_InsWrapRow, options_InsWrapCol,      &dptr[2]);
    dialog__setonoff(options_Borders,                             &dptr[3]);
    dialog__setonoff(options_Justify,                             &dptr[4]);
    dialog__setonoff(options_Wrap,                                &dptr[5]);
    dialog__setspecial(options_Decimal,                           &dptr[6]);
    dialog__setradio(options_NegMinus, options_NegBrackets,       &dptr[7]);
    dialog__setarray(options_Thousands,                           &dptr[8]);
    dialog__setonoff(options_InsertOnReturn,                      &dptr[9]);
    dialog__setradio(options_DateText, options_DateMD,            &dptr[10]);
    dialog__setfield_high(options_LeadingChars,                   &dptr[11]);
    dialog__setfield_high(options_TrailingChars,                  &dptr[12]);
    dialog__setfield(options_TextAtChar,                          &dptr[13]);
    dialog__setonoff(options_Grid,                                &dptr[14]);
    dialog__setonoff(options_Kerning,                             &dptr[15]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
          if(dialog__adjust(&f,    options_Decimal))
            dialog__bumpspecial(options_Decimal,    &dptr[6], f);
        else if(dialog__adjust(&f,    options_Thousands))
            dialog__bumparray(options_Thousands,    &dptr[8], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed options action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield_high(options_Title,                          &dptr[0]);
    dialog__getradio(options_SlotText, options_SlotNumbers,       &dptr[1]);
    dialog__getradio(options_InsWrapRow, options_InsWrapCol,      &dptr[2]);
    dialog__getonoff(options_Borders,                             &dptr[3]);
    dialog__getonoff(options_Justify,                             &dptr[4]);
    dialog__getonoff(options_Wrap,                                &dptr[5]);
    dialog__getspecial(options_Decimal,                           &dptr[6]);
    dialog__getradio(options_NegMinus, options_NegBrackets,       &dptr[7]);
    dialog__getarray(options_Thousands,                           &dptr[8]);
    dialog__getonoff(options_InsertOnReturn,                      &dptr[9]);
    dialog__getradio(options_DateText, options_DateMD,            &dptr[10]);
    dialog__getfield_high(options_LeadingChars,                   &dptr[11]);
    dialog__getfield_high(options_TrailingChars,                  &dptr[12]);
    dialog__getfield(options_TextAtChar,                          &dptr[13]);
    dialog__getonoff(options_Grid,                                &dptr[14]);
    dialog__getonoff(options_Kerning,                             &dptr[15]);
}

/******************************************************************************
*
*  page layout dialog box
*
******************************************************************************/

#define pagelayout_LengthInc     6
#define pagelayout_LengthDec     7
#define pagelayout_Length        8
#define pagelayout_SpacingInc    9
#define pagelayout_SpacingDec    10
#define pagelayout_Spacing       11
#define pagelayout_StartPage     12
#define pagelayout_TopInc        13
#define pagelayout_TopDec        14
#define pagelayout_Top           15
#define pagelayout_HeaderInc     16
#define pagelayout_HeaderDec     17
#define pagelayout_Header        18
#define pagelayout_FooterInc     19
#define pagelayout_FooterDec     20
#define pagelayout_Footer        21
#define pagelayout_BottomInc     22
#define pagelayout_BottomDec     23
#define pagelayout_Bottom        24
#define pagelayout_LeftInc       25
#define pagelayout_LeftDec       26
#define pagelayout_Left          27
#define pagelayout_PageWidthInc  28
#define pagelayout_PageWidthDec  29
#define pagelayout_PageWidth     30
#define pagelayout_HeaderText    31
#define pagelayout_HeaderText1   32
#define pagelayout_HeaderText2   33
#define pagelayout_FooterText    34
#define pagelayout_FooterText1   35
#define pagelayout_FooterText2   36

extern void
dproc_pagelayout(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(14, D_POPTIONS);

    dialog__setnumeric(pagelayout_Length,           &dptr[0]);
    dialog__setnumeric(pagelayout_Spacing,          &dptr[1]);
    dialog__setfield(pagelayout_StartPage,          &dptr[2]);
    dialog__setnumeric(pagelayout_Top,              &dptr[3]);
    dialog__setnumeric(pagelayout_Header,           &dptr[4]);
    dialog__setnumeric(pagelayout_Footer,           &dptr[5]);
    dialog__setnumeric(pagelayout_Bottom,           &dptr[6]);
    dialog__setnumeric(pagelayout_Left,             &dptr[7]);

    dialog__setfield_high(pagelayout_HeaderText,    &dptr[8]);
    dialog__setfield_high(pagelayout_HeaderText1,   &dptr[9]);
    dialog__setfield_high(pagelayout_HeaderText2,   &dptr[10]);
    dialog__setfield_high(pagelayout_FooterText,    &dptr[11]);
    dialog__setfield_high(pagelayout_FooterText1,   &dptr[12]);
    dialog__setfield_high(pagelayout_FooterText2,   &dptr[13]);

    dialog__setnumeric(pagelayout_PageWidth,        &dptr[14]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
          if(dialog__adjust(&f, pagelayout_Length))
            dialog__bumpnumeric(pagelayout_Length,    &dptr[0], f);

        else if(dialog__adjust(&f, pagelayout_Spacing))
            dialog__bumpnumeric(pagelayout_Spacing,   &dptr[1], f);

        else if(dialog__adjust(&f, pagelayout_Top))
            dialog__bumpnumeric(pagelayout_Top,       &dptr[3], f);

        else if(dialog__adjust(&f, pagelayout_Header))
            dialog__bumpnumeric(pagelayout_Header,    &dptr[4], f);

        else if(dialog__adjust(&f, pagelayout_Footer))
            dialog__bumpnumeric(pagelayout_Footer,    &dptr[5], f);

        else if(dialog__adjust(&f, pagelayout_Bottom))
            dialog__bumpnumeric(pagelayout_Bottom,    &dptr[6], f);

        else if(dialog__adjust(&f, pagelayout_Left))
            dialog__bumpnumeric(pagelayout_Left,      &dptr[7], f);

        else if(dialog__adjust(&f, pagelayout_PageWidth))
            dialog__bumpnumeric(pagelayout_PageWidth, &dptr[14], f);

        else
            trace_1(TRACE_APP_DIALOG, "unprocessed pagelayout action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getnumeric(pagelayout_Length,           &dptr[0]);
    dialog__getnumeric(pagelayout_Spacing,          &dptr[1]);
    dialog__getfield(pagelayout_StartPage,          &dptr[2]);
    dialog__getnumeric(pagelayout_Top,              &dptr[3]);
    dialog__getnumeric(pagelayout_Header,           &dptr[4]);
    dialog__getnumeric(pagelayout_Footer,           &dptr[5]);
    dialog__getnumeric(pagelayout_Bottom,           &dptr[6]);
    dialog__getnumeric(pagelayout_Left,             &dptr[7]);

    dialog__getfield_high(pagelayout_HeaderText,    &dptr[8]);
    dialog__getfield_high(pagelayout_HeaderText1,   &dptr[9]);
    dialog__getfield_high(pagelayout_HeaderText2,   &dptr[10]);
    dialog__getfield_high(pagelayout_FooterText,    &dptr[11]);
    dialog__getfield_high(pagelayout_FooterText1,   &dptr[12]);
    dialog__getfield_high(pagelayout_FooterText2,   &dptr[13]);

    dialog__getnumeric(pagelayout_PageWidth,        &dptr[14]);
}

#define chartopts_AutoRC     4
#define chartopts_Columns    5
#define chartopts_Rows       6

extern void
dproc_chartopts(
    DIALOG * dptr)
{
    assert_dialog(0, D_CHART_OPTIONS);

    dialog__setradio(chartopts_AutoRC, chartopts_Rows, &dptr[0]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getradio(chartopts_AutoRC, chartopts_Rows, &dptr[0]);
}

/******************************************************************************
*
*  decimal places dialog box
*
******************************************************************************/

#define decimal_Inc      2
#define decimal_Dec      3
#define decimal_Value    4

extern void
dproc_decimal(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_DECIMAL);

    dialog__setspecial(decimal_Value,    &dptr[0]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
          if(dialog__adjust(&f,    decimal_Value))
            dialog__bumpspecial(decimal_Value,    &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed options action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getspecial(decimal_Value,    &dptr[0]);
}

#if 0
/******************************************************************************
*
* insert character dbox
*
******************************************************************************/

#define insertchar_Inc        2
#define insertchar_Dec        3
#define insertchar_Number     4
#define insertchar_Char       5

extern void
dproc_insertchar(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(0, D_INSCHAR);

    dialog__setnumeric(insertchar_Number,    &dptr[0]);
    dialog__setchar(insertchar_Char,         &dptr[0]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f, insertchar_Number))
            dialog__bumpnumericlimited(insertchar_Number,   &dptr[0], f, 0x01, 0xFF);
        else if(f == insertchar_Char)
            dialog__getnumericlimited(insertchar_Number,    &dptr[0], 0x01, 0xFF);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed insertchar action %d", f);

        dialog__setchar(insertchar_Char, &dptr[0]);
    }

    dialog__getnumericlimited(insertchar_Number,    &dptr[0], 0x01, 0xFF);
}
#endif

/******************************************************************************
*
*  define key dbox
*
******************************************************************************/

/* ordered this way to get caret into definition at start */
#define defkey_Definition    2
#define defkey_Inc           3
#define defkey_Dec           4
#define defkey_Number        5
#define defkey_Char          6

extern void
dproc_defkey(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_DEFKEY);

    dialog__setnumeric(defkey_Number,           &dptr[0]);
    dialog__setchar(defkey_Char,                &dptr[0]);
    dialog__setfield_high(defkey_Definition,    &dptr[1]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   defkey_Number))
        {
            dialog__bumpnumeric(defkey_Number,  &dptr[0], f);
            dialog__setchar(defkey_Char,        &dptr[0]);
        }
        else if(f == defkey_Char)
        {
            dialog__getnumeric(defkey_Number,   &dptr[0]);
            dialog__setchar(defkey_Char,        &dptr[0]);
        }
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed defkey action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getnumeric(defkey_Number,           &dptr[0]);
    dialog__getfield_high(defkey_Definition,    &dptr[1]);
}

/******************************************************************************
*
* define function key dbox
*
******************************************************************************/

#define deffnkey_Inc        2
#define deffnkey_Dec        3
#define deffnkey_Key        4
#define deffnkey_Definition 5

extern void
dproc_deffnkey(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_DEF_FKEY);

    dialog__setarray(deffnkey_Key,                &dptr[0]);
    dialog__setfield_high(deffnkey_Definition,    &dptr[1]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f, deffnkey_Key))
            dialog__bumparray(deffnkey_Key,    &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed deffnkey action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getarray(deffnkey_Key,                &dptr[0]);
    dialog__getfield_high(deffnkey_Definition,    &dptr[1]);
}

/******************************************************************************
*
*  print dbox
*
******************************************************************************/

#define print_PrinterType        4
#define print_CopiesInc          5
#define print_CopiesDec          6
#define print_Copies             7
#define print_ScaleInc           8
#define print_ScaleDec           9
#define print_Scale              10
#define print_Portrait           11
#define print_Landscape          12
#define print_Printer            13
#define print_Screen             14
#define print_File               15
#define print_FileName           16
#define print_ParmFile           17
#define print_ParmFileName       18
#define print_OmitBlankFields    19
#define print_MarkedBlock        20
#define print_TwoSided           21
#define print_TwoSidedSpec       22
#define print_WaitBetween        23

extern void
dproc_print(
    DIALOG *dptr)
{
    print_infostr pinfo;
    char tempstring[256];
    const char * description;
    const char * drivername = NULL;
    void (* fadeproc) (dbox d, dbox_field f);
    dbox d;
    dbox_field f;

    assert_dialog(P_SCALE, D_PRINT);

    dialog__setradio(print_Printer, print_File,       &dptr[P_PSFON]);
    dialog__setfield(print_FileName,                  &dptr[P_PSFNAME]);

    dialog__setonoff(print_ParmFile,                  &dptr[P_DATABON]);
    dialog__setfield(print_ParmFileName,              &dptr[P_DATABNAME]);

    dialog__setonoff(print_OmitBlankFields,           &dptr[P_OMITBLANK]);
    dialog__setonoff(print_MarkedBlock,               &dptr[P_BLOCK]);

    dialog__setonoff(print_TwoSided,                  &dptr[P_TWO_ON]);
    dialog__setfield(print_TwoSidedSpec,              &dptr[P_TWO_MARGIN]);

    dialog__setnumeric(print_Copies,                  &dptr[P_COPIES]);
    dialog__setonoff(print_WaitBetween,               &dptr[P_WAIT]);
    dialog__setradio(print_Portrait, print_Landscape, &dptr[P_ORIENT]);
    dialog__setnumeric(print_Scale,                   &dptr[P_SCALE]);

        /* RJM on 22.9.91 thinks d_driver_PT is a windvar at this point */
        /* SKS confirms RJM's comment - only the windvars for a particular
         * dialog get swapped in and out of the dialog structure
        */
      if(d_driver_PT != driver_riscos)
        {
        description = product_ui_id();
        drivername  = d_driver_PD;
        }
    else if(print_info(&pinfo))
        description = No_RISC_OS_STR;
    else
        description = pinfo.description;

    (void) xsnprintf(tempstring, elemof32(tempstring),
            Zs_printer_driver_STR,
            description,
            drivername ? drivername : NULLSTR,
            drivername ? " "        : NULLSTR);

    dialog__setfield_str(print_PrinterType,           tempstring);

    /* fade fields appropriately */
    fadeproc = (d_driver_PT != driver_riscos)
                            ? dbox_fadefield
                            : dbox_unfadefield;

    d = dialog__dbox;

    (* fadeproc) (d, print_Portrait);
    (* fadeproc) (d, print_Landscape);

    (* fadeproc) (d, print_ScaleInc);
    (* fadeproc) (d, print_ScaleDec);
    (* fadeproc) (d, print_Scale);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   print_Scale))
            dialog__bumpnumericlimited(print_Scale,   &dptr[P_SCALE], f,  1, 999);
        else if(dialog__adjust(&f,   print_Copies))
            dialog__bumpnumericlimited(print_Copies,  &dptr[P_COPIES], f, 1, 999);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed print action %d", f);
    }

    /* SKS 20.10.91 - cheap and nasty way of setting these is to press ESCAPE from Print dialog
     * as otherwise you have to do a print to reflect the new page layout into the windvars
    */
    dialog__getradio(print_Portrait, print_Landscape, &dptr[P_ORIENT]);
    dialog__getnumericlimited(print_Scale,            &dptr[P_SCALE],  1, 999);

    if(!dialog__fillin_ok)
        return;

    dialog__getradio(print_Printer, print_File,       &dptr[P_PSFON]);
    dialog__getfield(print_FileName,                  &dptr[P_PSFNAME]);

    dialog__getonoff(print_ParmFile,                  &dptr[P_DATABON]);
    dialog__getfield(print_ParmFileName,              &dptr[P_DATABNAME]);

    dialog__getonoff(print_OmitBlankFields,           &dptr[P_OMITBLANK]);
    dialog__getonoff(print_MarkedBlock,               &dptr[P_BLOCK]);

    dialog__getonoff(print_TwoSided,                  &dptr[P_TWO_ON]);
    dialog__getfield(print_TwoSidedSpec,              &dptr[P_TWO_MARGIN]);

    dialog__getnumericlimited(print_Copies,           &dptr[P_COPIES], 1, 999);
    dialog__getonoff(print_WaitBetween,               &dptr[P_WAIT]);
}

/******************************************************************************
*
* printer configuration dialog box
*
******************************************************************************/

#define printconfig_TypeInc        2
#define printconfig_TypeDec        3
#define printconfig_Type           4
#define printconfig_DriverInc      5
#define printconfig_DriverDec      6
#define printconfig_Driver         7
#define printconfig_Server         8
#define printconfig_BaudInc        9
#define printconfig_BaudDec        10
#define printconfig_Baud           11
#define printconfig_DataInc        12
#define printconfig_DataDec        13
#define printconfig_Data           14
#define printconfig_ParityInc      15
#define printconfig_ParityDec      16
#define printconfig_Parity         17
#define printconfig_StopInc        18
#define printconfig_StopDec        19
#define printconfig_Stop           20

extern void
dproc_printconfig(
    DIALOG * dptr)
{
    dbox_field  f;
    const P_P_LIST_BLOCK list = &pdrivers_list;
    LIST_ITEMNO key;
    P_LIST entry;

    assert_dialog(6, D_DRIVER);

    dialog__setarray(printconfig_Type,     &dptr[0]);
    dialog__setfield(printconfig_Server,   &dptr[2]);
    dialog__setarray(printconfig_Baud,     &dptr[3]);
    dialog__setspecial(printconfig_Data,   &dptr[4]);
    dialog__setspecial(printconfig_Parity, &dptr[5]);
    dialog__setspecial(printconfig_Stop,   &dptr[6]);

    /* leave field blank unless driver selected, in which case user is forced to select default file to regain default settings */
    if(!str_isblank(dptr[1].textfield))
    {
        key = dialog__initkeyfromstring(list, dptr[1].textfield);
        entry = search_list(list, key);
    }
    else
    {
        key = 0x70000000; /* large number, when inc or dec it won't find the entry so will go sensible */
        entry = NULL;
    }

    dialog__setfield_str(printconfig_Driver, entry ? entry->value : NULL); /* SKS 15.10.91 */

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
          if(dialog__adjust(&f,    printconfig_Type))
            dialog__bumparray(     printconfig_Type,    &dptr[0], f);
        else if(dialog__adjust(&f, printconfig_Driver))
            dialog__bumpstring(    printconfig_Driver,            f, list, &key);
        else if(dialog__adjust(&f, printconfig_Baud))
            dialog__bumparray(     printconfig_Baud,    &dptr[3], f);
        else if(dialog__adjust(&f, printconfig_Data))
            dialog__bumpspecial(   printconfig_Data,    &dptr[4], f);
        else if(dialog__adjust(&f, printconfig_Parity))
            dialog__bumpspecial(   printconfig_Parity,  &dptr[5], f);
        else if(dialog__adjust(&f, printconfig_Stop))
            dialog__bumpspecial(   printconfig_Stop,    &dptr[6], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed printconfig action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getarray(printconfig_Type,     &dptr[0]);
    dialog__getfield(printconfig_Driver,   &dptr[1]);
    dialog__getfield(printconfig_Server,   &dptr[2]);
    dialog__getarray(printconfig_Baud,     &dptr[3]);
    dialog__getspecial(printconfig_Data,   &dptr[4]);
    dialog__getspecial(printconfig_Parity, &dptr[5]);
    dialog__getspecial(printconfig_Stop,   &dptr[6]);
}

/******************************************************************************
*
*  microspace dbox
*
******************************************************************************/

#define microspace_OnOff    2
#define microspace_Inc      3
#define microspace_Dec      4
#define microspace_Pitch    5

extern void
dproc_microspace(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(1, D_MSPACE);

    dialog__setonoff(microspace_OnOff,   &dptr[0]);
    dialog__setnumeric(microspace_Pitch, &dptr[1]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f,   microspace_Pitch))
            dialog__bumpnumeric(microspace_Pitch, &dptr[1], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed microspace action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getonoff(microspace_OnOff,   &dptr[0]);
    dialog__getnumeric(microspace_Pitch, &dptr[1]);
}

/******************************************************************************
*
*  search dbox
*
******************************************************************************/

#define search_TargetString        2
#define search_Replace             3
#define search_ReplaceString       4
#define search_Confirm             5
#define search_UpperLower          6
#define search_ExpSlots            7
#define search_MarkedBlock         8
#define search_ColRange            9
#define search_ColRangeSpec        10
#define search_AllFiles            11
#define search_FromThisFile        12

extern void
dproc_search(
    DIALOG *dptr)
{
    assert_dialog(5, D_SEARCH);

    dialog__setfield(search_TargetString,    &dptr[0]);
    dialog__setcomponoff(search_Replace,     &dptr[1]);
    dialog__setonoff(search_Confirm,         &dptr[2]);
    dialog__setonoff(search_UpperLower,      &dptr[3]);
    dialog__setonoff(search_ExpSlots,        &dptr[4]);
    dialog__setonoff(search_MarkedBlock,     &dptr[5]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(search_TargetString,    &dptr[0]);
    dialog__getcomponoff(search_Replace,     &dptr[1]);
    dialog__getonoff(search_Confirm,         &dptr[2]);
    dialog__getonoff(search_UpperLower,      &dptr[3]);
    dialog__getonoff(search_ExpSlots,        &dptr[4]);
    dialog__getonoff(search_MarkedBlock,     &dptr[5]);
}

/* --------------------- SPELL related dialog boxes ---------------------- */

/******************************************************************************
*
* check document dialog box
*
******************************************************************************/

#define checkdoc_MarkedBlock 2
#if 0
#define checkdoc_AllFiles    3
#endif

extern void
dproc_checkdoc(
    DIALOG *dptr)
{
    assert_dialog(0, D_CHECKON);

    dialog__setonoff(checkdoc_MarkedBlock, &dptr[0]);
#if 0
    dialog__setonoff(checkdoc_AllFiles,    &dptr[1]);
#endif

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getonoff(checkdoc_MarkedBlock, &dptr[0]);
#if 0
    dialog__getonoff(checkdoc_AllFiles,    &dptr[1]);
#endif
}

/******************************************************************************
*
* checking document dialog box
*
******************************************************************************/

#define checking_ChangeWord          2
#define checking_ChangeWordString    3
#define checking_Browse              4
#define checking_UserDict            5
#define checking_UserDictName        6

extern void
dproc_checking(
    DIALOG *dptr)
{
    assert_dialog(2, D_CHECK);

    dialog__setcomponoff(checking_ChangeWord, &dptr[0]);
    dialog__setonoff(checking_Browse,         &dptr[1]);
    dialog__setcomponoff(checking_UserDict,   &dptr[2]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getcomponoff(checking_ChangeWord, &dptr[0]);
    dialog__getonoff(checking_Browse,         &dptr[1]);
    dialog__getcomponoff(checking_UserDict,   &dptr[2]);
}

/******************************************************************************
*
* checked document dialog box
*
******************************************************************************/

#define checked_Unrecognized 1
#define checked_Added        2

extern void
dproc_checked(
    DIALOG *dptr)
{
    assert_dialog(1, D_CHECK_MESS);

    dialog__setfield(checked_Unrecognized, &dptr[0]);
    dialog__setfield(checked_Added,        &dptr[1]);

    dialog__simple_fillin(FALSE);

    /* no return values */
}

/******************************************************************************
*
*  browse word dialog box
*
******************************************************************************/

#define browse_Word            2
#define browse_UserDict        3
#define browse_UserDictName    4

extern void
dproc_browse(
    DIALOG *dptr)
{
    assert_dialog(1, D_USER_BROWSE);

    dialog__setfield(browse_Word,         &dptr[0]);
    dialog__setcomponoff(browse_UserDict, &dptr[1]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(browse_Word,         &dptr[0]);
    dialog__getcomponoff(browse_UserDict, &dptr[1]);
}

/******************************************************************************
*
* anagram/subgram dialog boxes
*
******************************************************************************/

#define anasubgram_WordTemplate    2
#define anasubgram_UserDict        3
#define anasubgram_UserDictName    4

extern void
dproc_anasubgram(
    DIALOG *dptr)
{
    assert_dialog(1, D_USER_ANAG);
    assert_dialog(1, D_USER_SUBGRAM);

    dialog__setfield(anasubgram_WordTemplate, &dptr[0]);
    dialog__setcomponoff(anasubgram_UserDict, &dptr[1]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(anasubgram_WordTemplate, &dptr[0]);
    dialog__getcomponoff(anasubgram_UserDict, &dptr[1]);
}

/******************************************************************************
*
* dump user dictionary dbox
*
******************************************************************************/

#define dumpdict_WordTemplate    2
#define dumpdict_FileName        3
#define dumpdict_UserDict        4
#define dumpdict_UserDictName    5

extern void
dproc_dumpdict(
    DIALOG *dptr)
{
    assert_dialog(2, D_USER_DUMP);

    dialog__setfield(dumpdict_WordTemplate, &dptr[0]);
    dialog__setfield(dumpdict_FileName,     &dptr[1]);
    dialog__setcomponoff(dumpdict_UserDict, &dptr[2]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(dumpdict_WordTemplate, &dptr[0]);
    dialog__getfield(dumpdict_FileName,     &dptr[1]);
    dialog__getcomponoff(dumpdict_UserDict, &dptr[2]);
}

/******************************************************************************
*
* define name dialog box - uses dproc_twotext
*
******************************************************************************/

/******************************************************************************
*
* edit name dialog box
*
******************************************************************************/

#define edit_name_Name     2
#define edit_name_RefersTo 3
#define edit_name_Undefine 4

extern void
dproc_edit_name(
    DIALOG *dptr)
{
    dbox_field f;

    assert_dialog(2, D_EDIT_NAME);

    dialog__setfield(edit_name_Name,     &dptr[0]);
    dialog__setfield(edit_name_RefersTo, &dptr[1]);

    dptr[2].option = 'E';

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        /* SKS says note that ok is set iff f == dbox_OK so this will stop the caller loop */
        if(f == edit_name_Undefine)
        {
            dptr[2].option = 'U';
            break;
        }
    }

    if(!dialog__fillin_ok)
        return;

  /*dialog__getfield(edit_name_Name,     &dptr[0]);*/
    dialog__getfield(edit_name_RefersTo, &dptr[1]);
}

/******************************************************************************
*
* report a badly formed expression on exit from an editor
* give user the choice of remaining in the editor to fix the
* problem or returning the expression as a text cell
*
******************************************************************************/

#define formula_error_TextSlot    ((dbox_field) 0)
#define formula_error_Message     ((dbox_field) 1)
#define formula_error_EditFormula ((dbox_field) 2)

extern void
dproc_formula_error(
    DIALOG *dptr)
{
    dbox_field f;

    assert(dbox_OK == formula_error_TextSlot);
    assert_dialog(1, D_FORMULA_ERROR);

    dptr[0].option = 'E';
    dialog__setfield(formula_error_Message, &dptr[1]);

#if TRUE
    do  {
        f = dialog__fillin(FALSE);
    }
    while((f != dbox_CLOSE) && (f != dbox_OK) && (f != formula_error_EditFormula));
#else
    dialog__simple_fillin(TRUE);
#endif

    dptr[0].option = (dialog__fillin_ok ? 'T' : 'E');
}

/******************************************************************************
*
* insert page number dbox
*
******************************************************************************/

#define insert_page_number_Format 4
#define insert_page_number_Sample 5

_Check_return_
static STATUS
ss_numform(
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

/* output ss_data using the format we just selected */

static void
D_N_P_format_common(
    /*out*/     P_U8Z sample,
    _InVal_     U32 sample_n,
    _In_reads_(format_n) PC_UCHARS format,
    _InVal_     U32 format_n,
    _InRef_     PC_SS_DATA p_ss_data)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    /* like c_text */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, format, format_n)))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ss_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data);

    if(status_fail(status))
        xstrkpy(sample, sample_n, reperr_getstr(status));
    else
        xstrkpy(sample, sample_n, quick_ublock_ustr(&quick_ublock_result));
}

static void
display_D_N_P_sample(
    _InVal_     dbox_field f_format,
    _InVal_     dbox_field f_sample,
    _InRef_     PC_SS_DATA p_ss_data)
{
    char format[256];
    char sample[256];

    dbox_getfield(dialog__dbox, f_format, format, sizeof32(format));

    D_N_P_format_common(sample, sizeof32(sample), format, strlen32(format), p_ss_data);

    dbox_setfield(dialog__dbox, f_sample, sample);
}

extern void
dproc_insert_page_number(
    DIALOG *dptr)
{
    dbox_field f;
    const P_P_LIST_BLOCK list = &page_number_formats_list;
    LIST_ITEMNO key;
    P_LIST entry;
    SS_DATA ss_data;

    assert_dialog(1, D_INSERT_PAGE_NUMBER);

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(insert_page_number_Format, entry ? entry->value : NULL);

    ss_data_set_integer(&ss_data, curpnm);
    display_D_N_P_sample(insert_page_number_Format, insert_page_number_Sample, &ss_data);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f, insert_page_number_Format))
        {
            dialog__bumpstring(insert_page_number_Format, f, list, &key);

            display_D_N_P_sample(insert_page_number_Format, insert_page_number_Sample, &ss_data);
        }
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed insert_page_number action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(insert_page_number_Format, &dptr[0]);
}

/******************************************************************************
*
* insert date dbox
*
******************************************************************************/

#define insert_date_Format 4
#define insert_date_Sample 5

extern void
dproc_insert_date(
    DIALOG *dptr)
{
    dbox_field f;
    const P_P_LIST_BLOCK list = &date_formats_list;
    LIST_ITEMNO key;
    P_LIST entry;
    SS_DATA ss_data;

    assert_dialog(1, D_INSERT_DATE);

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(insert_date_Format, entry ? entry->value : NULL);

    /* like c_now */
    ss_data.data_id = DATA_ID_DATE;
    ss_date_set_from_local_time(&ss_data.arg.ss_date);
    display_D_N_P_sample(insert_date_Format, insert_date_Sample, &ss_data);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f, insert_date_Format))
        {
            dialog__bumpstring(insert_date_Format, f, list, &key);

            display_D_N_P_sample(insert_date_Format, insert_date_Sample, &ss_data);
        }
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed insert_date action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(insert_date_Format, &dptr[0]);
}

/******************************************************************************
*
* insert time dbox
*
******************************************************************************/

#define insert_time_Format 4
#define insert_time_Sample 5

extern void
dproc_insert_time(
    DIALOG *dptr)
{
    dbox_field f;
    const P_P_LIST_BLOCK list = &time_formats_list;
    LIST_ITEMNO key;
    P_LIST entry;
    SS_DATA ss_data;

    assert_dialog(1, D_INSERT_TIME);

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(list, key);

    dialog__setfield_str(insert_time_Format, entry ? entry->value : NULL);

    /* like c_now */
    ss_data.data_id = DATA_ID_DATE;
    ss_date_set_from_local_time(&ss_data.arg.ss_date);
    display_D_N_P_sample(insert_time_Format, insert_time_Sample, &ss_data);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        if(dialog__adjust(&f, insert_time_Format))
        {
            dialog__bumpstring(insert_time_Format, f, list, &key);

            display_D_N_P_sample(insert_time_Format, insert_time_Sample, &ss_data);
        }
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed insert_time action %d", f);
    }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(insert_time_Format, &dptr[0]);
}

/******************************************************************************
*
* search/replacing dialog box handling
*
******************************************************************************/

#define replace_No      0        /* RETURN(dbox_OK) -> No */
#define replace_Yes     1
#define replace_All     2
#define replace_Stop    3
#define replace_Found   4
#define replace_Hidden  5

extern S32
riscdialog_replace_dbox(
    const char *mess1,
    const char *mess2)
{
    S32 res;
    dbox d;

    trace_2(TRACE_APP_DIALOG, "riscdialog_replace_dbox(%s, %s)",
                trace_string(mess1), trace_string(mess2));

    if(!dialog__dbox)
        if(!dialog__create_reperr("replace", &dialog__dbox))
            /* failed miserably */
            return(ESCAPE);

    d = dialog__dbox;

    dbox_setfield(d, replace_Found,
                  mess1 ? (char *) mess1 : (char *) NULLSTR);

    dbox_setfield(d, replace_Hidden,
                  mess2 ? (char *) mess2 : (char *) NULLSTR);

    dialog__register_help_for(d);

    /* maybe front: note that it's not showstatic */
    dbox_show(d);

    switch(dialog__fillin_for(d))
    {
    case replace_Yes:
        res = 'Y';
        break;

    case replace_All:
        res = 'A';
        break;

    case dbox_CLOSE:
    case replace_Stop:
        res = ESCAPE;
        break;

 /* case dbox_OK: */
 /* case replace_No: */
    default:
        res = 'N';
        break;
    }

    /* note that this dialog box is left hanging around
     * and must be closed explicitly with the below call
     * if the user is foolish enough to go clickaround he gets closed
    */
    trace_1(TRACE_APP_DIALOG, "riscdialog_replace_dbox() returns %d", res);
    return(res);
}

extern void
riscdialog_replace_dbox_end(void)
{
    trace_0(TRACE_APP_DIALOG, "riscdialog_replace_dbox_end()");

    dialog__dispose();
}

/******************************************************************************
*
*  pause function handling
*
******************************************************************************/

#define pausing_Continue    0        /* RETURN(dbox_OK) -> Continue */
#define pausing_Pause       1

static MONOTIME starttime;
static MONOTIMEDIFF lengthtime;

/* pausing null processor */

extern void
pausing_null(void)
{
    trace_1(TRACE_APP_DIALOG, "timesofar = %d", monotime_diff(starttime));
    if(monotime_diff(starttime) >= lengthtime)
        /* Cause dbox_CLOSE to be returned to the main process */
        winx_send_close_window_request(dbox_window_handle(dialog__dbox));
}

extern void
riscdialog_dopause(
    S32 nseconds)
{
    dbox d;
    dbox_field f;

    trace_1(TRACE_APP_DIALOG, "riscdialog_dopause(%d)", nseconds);

    if(!dialog__create_reperr("pausing", &dialog__dbox))
        /* failed miserably */
        return;

    d = dialog__dbox;

    dialog__register_help_for(d);

    /* maybe front: note that it's not showstatic */
    dbox_show(d);

    pausing_docno = current_docno();

    starttime   = monotime();
    lengthtime  = (MONOTIMEDIFF) 100 * nseconds;

    while(((f = dialog__fillin_for(d)) != dbox_CLOSE)  &&  (f != dbox_OK))
    {
        switch(f)
        {
        case pausing_Pause:
            /* stop null events so process never times out */
            /* now needs explicit click (or dbox closure) */
            pausing_docno = DOCNO_NONE;
            break;

        default:
            assert(0);

        case pausing_Continue:
            break;
        }
    }

    pausing_docno = DOCNO_NONE;

    dialog__dispose();
}

/* end of ridialog.c */
