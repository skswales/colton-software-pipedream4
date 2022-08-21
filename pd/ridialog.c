/* ridialog.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

#include "riscos_x.h"
#include "pd_x.h"
#include "version_x.h"

#include "riscmenu.h"

#include "ridialog.h"

#include "fileicon.h"

/* ----------------------------------------------------------------------- */

static dbox dialog__dbox = NULL;

/* whether dialog box filled in OK */
static BOOL dialog__fillin_ok;

/* whether dialog box wants to persist */
static BOOL dialog__may_persist;

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
            rep_fserr(errorp);
        else
            reperr_null(create_error(ERR_NOROOMFORDBOX));
        }

    return((BOOL) d);
}

static BOOL
dialog__create(
    const char * dboxname)
{
    trace_1(TRACE_APP_DIALOG, "dialog__create(%s)\n", dboxname);

    if(NULL != dialog__dbox)
        {
        trace_1(TRACE_APP_DIALOG, "dialog__dbox already exists: " PTR_XTFMT " - hope it's the right one! (persisting?)\n", report_ptr_cast(dialog__dbox));
        }
    else
        {
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
            reportf("dialog__fillin_for(&%p) returning %d to a killed caller context\n", report_ptr_cast(d), f);
            f = dbox_CLOSE;
            }
        else
            select_document(p_docu);
        }

    trace_2(TRACE_APP_DIALOG, "dialog__fillin_for(&%p) yields %d\n", report_ptr_cast(d), f);
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

    trace_0(TRACE_APP_DIALOG, "dialog__fillin()\n");

    dbox_show(d);

    f = dialog__fillin_for(d);

    if(has_cancel && (1 == f)) /* SKS 19oct96 */
        f = dbox_CLOSE;

    dialog__fillin_ok = (f == dbox_OK);

    trace_2(TRACE_APP_DIALOG, "dialog__fillin returns field %d, ok=%s\n", f, trace_boolstring(dialog__fillin_ok));
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

    trace_0(TRACE_APP_DIALOG, "dialog__simple_fillin()\n");

    do { f = dialog__fillin(has_cancel); } while((f != dbox_CLOSE)  &&  (f != dbox_OK));

    trace_1(TRACE_APP_DIALOG, "dialog__simple_fillin returns dialog__fillin_ok=%s\n", trace_boolstring(dialog__fillin_ok));
}

/******************************************************************************
*
*  dispose of a dialog box
*
******************************************************************************/

static void
dialog__dispose(void)
{
    trace_1(TRACE_APP_DIALOG, "dialog__dispose(): dialog__dbox = &%p\n", report_ptr_cast(dialog__dbox));

    dbox_dispose(&dialog__dbox);
}

/******************************************************************************
*
*  explicit disposal of a dialog box
*
*  NB menu tree will be killed iff needed by win_close_wind
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
        trace_0(TRACE_APP_DIALOG, "riscdialog_ended = TRUE as dialog__dbox already disposed\n");
        }
    else
        {
        ended = !dialog__may_persist;
        trace_1(TRACE_APP_DIALOG, "riscdialog_ended = %s\n", trace_boolstring(ended));

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
        win_send_front(dbox_syshandle(dialog__dbox), FALSE);
        }
}

extern BOOL
riscdialog_warning(void)
{
    if(NULL == dialog__dbox)
        return(TRUE);

    riscdialog_front_dialog();

    return(reperr_null(ERR_ALREADY_IN_DIALOG));
}

/******************************************************************************
*
*  ensure created, encode, fillin and decode a dialog box
*
******************************************************************************/

static S32
dialog__raw_eventhandler(
    dbox d,
    void * v_e,
    void * handle)
{
    wimp_eventstr * e = (wimp_eventstr *) v_e;
    BOOL processed = FALSE;

    IGNOREPARM(d);
    IGNOREPARM(handle);

    switch(e->e)
        {
        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            if(e->data.msg.hdr.action == wimp_MHELPREQUEST)
                {
                trace_0(TRACE_APP_DIALOG, "help request on pd dialog box\n");
                riscos_sendhelpreply(&e->data.msg, help_dialog_window);
                processed = TRUE;
                }
            break;

        default:
            break;
        }

    return(processed);
}

static void
dialog__register_help_for(
    dbox d)
{
    dbox_raw_eventhandler(d, dialog__raw_eventhandler, NULL);
}

extern S32
riscdialog_execute(
    dialog_proc dproc,
    const char * dname,
    DIALOG * dptr,
    S32 boxnumber)
{
    const char * title;

    dialog__fillin_ok = FALSE; /* will be set TRUE by a good fillin */
    dialog__may_persist = FALSE; /* will be set TRUE by a good fillin AND adjust-click */

    clearmousebuffer(); /* Keep RJM happy.  RJM says you know it makes sense */

    if(!dname)
        {
        trace_2(TRACE_APP_DIALOG, "riscdialog_execute(%s, \"\", " PTR_XTFMT ")\n",
                    report_procedure_name(report_proc_cast(dproc)), report_ptr_cast(dptr));

        trace_0(TRACE_APP_DIALOG, "calling dialog procedure without creating dialog__dbox\n");
        dproc(dptr);
        }
    else
        {
        trace_3(TRACE_APP_DIALOG, "riscdialog_execute(%s, \"%s\", " PTR_XTFMT ")\n",
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
                trace_0(TRACE_APP_DIALOG, "disposing because of faulty fillin etc.\n");
                dialog__dispose();
                }
            }
        else
            trace_0(TRACE_APP_DIALOG, "failed to create dialog__dbox\n");
        }

    dialog__may_persist = dialog__fillin_ok ? dbox_persist() : FALSE;

    trace_1(TRACE_APP_DIALOG, "riscdialog_execute returns %s\n", trace_boolstring(dialog__fillin_ok));
    return(dialog__fillin_ok);
}

#define dboxquery_FYes       0    /* action */
#define dboxquery_FMsg       1    /* string output */
#define dboxquery_FNo        2    /* action */
#define dboxquery_FCancel    3    /* optional cancel button */
#define dboxquery_FWriteable 4    /* dummy writeable icon off the bottom so we acquire focus */

static S32
mydboxquery(
    /*const*/ char * question,
    /*const*/ char * dboxname,
    dbox * dd)
{
    S32 res;
    dbox d;
    dbox_field f;

    trace_1(TRACE_APP_DIALOG, "mydboxquery(%s)\n", question);

    if(!*dd)
        {
        if(!dialog__create_reperr(dboxname, dd))
            /* out of space - embarassing */
            return(riscdialog_query_CANCEL);
        }

    if(!question)
        return(riscdialog_query_YES);

    d = *dd;

    dbox_setfield(d, 1, question);

    dialog__register_help_for(d);

    dbox_show(d);

    f = dialog__fillin_for(d);

    dbox_hide(d);            /* don't dispose !!! */

    switch(f)
        {
        case dboxquery_FYes:
            res = win_adjustclicked() ? riscdialog_query_NO : riscdialog_query_YES;
            dialog__fillin_ok = TRUE;
            break;

        case dboxquery_FNo:
            res = win_adjustclicked() ? riscdialog_query_YES : riscdialog_query_NO;
            dialog__fillin_ok = TRUE;
            break;

        default:
            assert(f == dbox_CLOSE);

        case dboxquery_FCancel:
            res = riscdialog_query_CANCEL;
            dialog__fillin_ok = FALSE;
            break;
        }

    trace_2(TRACE_APP_DIALOG, "mydboxquery(%s) returns %d\n", question, res);
    return(res);
}

/******************************************************************************
*
*
******************************************************************************/

static dbox queryYN_dbox = NULL;

extern void
riscdialog_initialise_once(void)
{
    /* create at startup so we can always prompt user with YN (for most things) */
    if(riscdialog_query_CANCEL == riscdialog_query_YN(NULL))
        exit(EXIT_FAILURE);
}

extern S32
riscdialog_query_YN(
    const char * mess)
{
    return(mydboxquery((char *) mess, "queryYN", &queryYN_dbox));
}

extern S32
riscdialog_query_DC(
    const char * mess)
{
    dbox d = NULL;
    S32 res = mydboxquery((char *) mess, "queryDC", &d);

    dbox_dispose(&d);

    return((res == riscdialog_query_NO) ? riscdialog_query_CANCEL : res);
}

extern S32
riscdialog_query_SDC(
    const char * mess)
{
    dbox d = NULL;
    S32 res = mydboxquery((char *) mess, "querySDC", &d);

    dbox_dispose(&d);

    return(res);
}

extern S32
riscdialog_query_save_existing(void)
{
    char tempstring[256];
    S32 res;

    if(!xf_filealtered)
        {
        trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_existing() returns NO because file not altered\n");
        return(riscdialog_query_NO);
        }

    (void) xsnprintf(tempstring, elemof32(tempstring), save_edited_file_Zs_STR, currentfilename);

    res = riscdialog_query_YN(tempstring);

    switch(res)
        {
        case riscdialog_query_CANCEL:
            #if TRACE_ALLOWED
            trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_existing() returns CANCEL\n");
            return(res);
            #else
            /* deliberate drop thru ... */
            #endif

        case riscdialog_query_NO:
            trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_existing() returns NO\n");
            return(res);

        default:
            assert(0);

        case riscdialog_query_YES:
            trace_0(TRACE_APP_DIALOG, "riscdialog_query_save_existing() got YES; saving file\n");
            break;
        }

    dbox_noted_position_save(); /* in case called from Quit sequence */
    application_process_command(N_SaveFile);
    dbox_noted_position_restore();

    /* may have failed to save or saved to unsafe receiver */
    if(been_error  ||  xf_filealtered)
        res = riscdialog_query_CANCEL;

    trace_1(TRACE_APP_DIALOG, "riscdialog_query_save_existing() returns %d\n", res);
    return(res);
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

    trace_3(TRACE_APP_DIALOG, "dialog__getfield(%d, " PTR_XTFMT ") yields \"%s\"\n",
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

    trace_3(TRACE_APP_DIALOG, "dialog__getfield(%d, " PTR_XTFMT ") yields \"%s\"\n",
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
    while(ch != '\0');

    return(mystr_set(var, tempstring));
}

static void
dialog__setfield_high(
    dbox_field f,
    const DIALOG * dptr)
{
    char array[256];
    S32 i = 0;
    char ch;
    const char *str = dptr->textfield;

    trace_3(TRACE_APP_DIALOG, "dialog__setfield_high(%d, (&%p) \"%s\")\n",
                                    f, str, trace_string(str));

    if(!str)
        str = (const char *) NULLSTR;

    /* translate ctrl chars */

    do  {
        if(i >= sizeof(array) - 1)
            ch = '\0';
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
    while(ch != '\0');

    dbox_setfield(dialog__dbox, f, array);
}

static void
dialog__setfield_str(
    dbox_field f,
    const char * str)
{
    trace_3(TRACE_APP_DIALOG, "dialog__setfield_str(%d, (&%p) \"%s\")\n",
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
    tempstring[1] = '\0';

    dbox_setfield(dialog__dbox, f, tempstring);    /* no translation */
}

/* no getcolour() needed */

static void
dialog__setcolour(
    dbox_field f,
    const DIALOG * dptr)
{
    wimp_w w = dbox_syshandle(dialog__dbox);
    wimp_i i = dbox_field_to_icon(dialog__dbox, f);

    trace_4(TRACE_APP_DIALOG, "dialog__setcolour(%d, %d) w %d i %d",
                f, dptr->option & 0x0F, w, i);

    wimpt_safe(wimp_set_icon_state(w, i,
               (wimp_iconflags) /* EOR */ ((dptr->option & 0x0FL) * (U32) wimp_IBACKCOL),
               (wimp_iconflags) /* BIC */ ((               0x0FL) * (U32) wimp_IBACKCOL)));
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

    return(dbox_adjusthit(fp, inc, dec, riscos_adjustclicked()));
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

    trace_2(TRACE_APP_DIALOG, "dialog__getnumeric(%d) yields %d\n", f, dptr->option);
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

    trace_2(TRACE_APP_DIALOG, "dialog__setnumeric(%d, %d)\n", f, num);
    assert((dptr->type==F_NUMBER) || (dptr->type==F_COLOUR) || (dptr->type==F_CHAR) || (dptr->type==F_LIST));

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
    S32 range = maxval - minval + 1;            /* ie 0..255 is 256 */
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
    LIST * lptr;
    LIST_ITEMNO key;

    key = 0;

    for(lptr = first_in_list(listpp);
        lptr;
        lptr = next_in_list( listpp))
        {
        if(0 == _stricmp(lptr->value, str))
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
    LIST * entry;

    trace_0(TRACE_APP_DIALOG, "bump that string - ");

    /* always inc,dec,value */
    if(hit+2 == valuefield)
        {
        trace_0(TRACE_APP_DIALOG, "up\n");

        *key = *key + 1;

        entry = search_list(listpp, *key);

        trace_1(TRACE_APP_DIALOG, "entry=%p\n", report_ptr_cast(entry));

        if(!entry)
            {
            *key = 0;

            entry = search_list(listpp, *key);
            }
        }
    else
        {
        LIST_ITEMNO lastkey = list_numitem(*listpp) - 1;

        trace_0(TRACE_APP_DIALOG, "down\n");

        if(*key == 0)
            {
            trace_1(TRACE_APP_DIALOG, "numitem=%i\n", list_numitem(*listpp));
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
        trace_3(TRACE_APP_DIALOG, "entry=%p, ->key=%d, ->value = %s\n", report_ptr_cast(entry), entry->key, entry->value);
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

    optptr = (ch == '\0') ? NULL : (PC_U8) strchr(optlistptr, ch);

    if(!optptr)
        {
        optptr = optlistptr;
        dptr->option = *optptr;    /* ensure sensible */
        }
    else
        dptr->option = ch;

    trace_3(TRACE_APP_DIALOG, "dialog__getspecial(%d) returns option '%c' optptr &%p\n",
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
    tempstring[1] = '\0';

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
        if(*++optptr == '\0')
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
    trace_1(TRACE_APP_DIALOG, "dialog__getarray got \"%s\"\n", tempstring);
    assert(dptr->type == F_ARRAY);

    do  {
        trace_1(TRACE_APP_DIALOG, "comparing with \"%s\"\n", **ptr);
        if(0 == _stricmp(**ptr, tempstring))
            {
            res = ptr - array;
            break;
            }
        }
    while(*++ptr);

    trace_3(TRACE_APP_DIALOG, "dialog__getarray(%d, " PTR_XTFMT ") yields %d\n",
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
    trace_1(TRACE_APP_DIALOG, "last array element is array[%d\n", ptr - array - 1);
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

    trace_2(TRACE_APP_DIALOG, "dialog__getonoff(%d) yields '%c'\n", f, option);
    assert((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE));
    assert(1 == strlen(*dptr->optionlist) - 1);

    dptr->option = (char) option;
}

static void
dialog__setonoff(
    dbox_field f,
    const DIALOG * dptr)
{
    trace_2(TRACE_APP_DIALOG, "dialog__setonoff(%d, '%c')\n", f, dptr->option);
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

    trace_3(TRACE_APP_DIALOG, "dialog__whichradio(%d, %d) returns field %d\n", start, end, f);

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

    trace_5(TRACE_APP_DIALOG, "dialog__getradio(%d, %d, \"%s\") returns field %d, option '%c'\n",
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

    trace_5(TRACE_APP_DIALOG, "dialog__setradio(%d, %d, \"%s\", '%c'), index %d\n",
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

    assert_dialog(0, D_INSPAGE);
    assert_dialog(0, D_DELETED);

    dialog__setnumeric(onenumeric_Value, &dptr[0]);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,   onenumeric_Value))
            dialog__bumpnumeric(onenumeric_Value, &dptr[0], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed onenumeric action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed onespecial action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed numtext action %d\n", f);
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
* generic yes/no/cancel dialog box
*
* NB. can't be called directly as it needs a string
*
******************************************************************************/

static void
dproc__query(
    DIALOG *dptr,
    const char *mess)
{
    S32 res = riscdialog_query_YN(mess);

    switch(res)
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

extern FILETYPE_RISC_OS
currentfiletype(
    char filetype_option)
{
    if(TAB_CHAR != filetype_option)
        return(rft_from_filetype_option(filetype_option));

    if(TAB_CHAR == current_filetype_option)
        return((FILETYPE_RISC_OS) ((currentfileinfo.load >> 8) & 0xFFF));

    return(FILETYPE_TEXT);
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
    _kernel_swi_regs rs;
    char tempstring[256];
    FILETYPE_RISC_OS filetype = currentfiletype(current_filetype_option);

    IGNOREPARM(dptr);

    dialog__setfield_str(aboutfile_Name, riscos_obtainfilename(currentfilename));

    dialog__setfield_str(aboutfile_Modified, xf_filealtered ? YES_STR : NO_STR);

    rs.r[0] = 18;
    rs.r[1] = 0;
    rs.r[2] = filetype;
    rs.r[3] = 0;
    wimpt_complain(_kernel_swi(OS_FSControl, &rs, &rs));
    * (int *) &tempstring[0] = rs.r[2];
    * (int *) &tempstring[4] = rs.r[3];

    (void) sprintf(&tempstring[8], " (%3.3X)", filetype);

    dialog__setfield_str(aboutfile_Type, tempstring);

    rs.r[0] = (int) &currentfileinfo;
    rs.r[1] = (int) tempstring;
    rs.r[2] = sizeof32(tempstring);
    if(wimpt_complain(_kernel_swi(OS_ConvertStandardDateAndTime, &rs, &rs)))
        tempstring[0] = NULLCH;

    tempstring[sizeof(tempstring)-1] = NULLCH; /* Ensure terminated */

    dialog__setfield_str(aboutfile_Date, tempstring);

    /* make appropriate file icon in dbox */
    fileicon(dbox_syshandle(dialog__dbox), aboutfile_Icon, filetype);

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

extern void
dproc_aboutprog(
    DIALOG *dptr)
{
    IGNOREPARM(dptr);

    dialog__setfield_str(aboutprog_Author,     "© 1988-2014, Colton Software");

    dialog__setfield_str(aboutprog_Version,    applicationversion);

    dialog__simple_fillin(FALSE);
}

/******************************************************************************
*
* sort dbox
*
******************************************************************************/

#define sort_FirstColumn      2        /* pairs of on/off(ascending) & text(column) */

/* rjm switches out update refs on 14.9.91 */
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
    LIST_ITEMNO key;
    LIST *      entry;

    if(!str_isblank(dptr[0].textfield))
        key = dialog__initkeyfromstring(&ltemplate_or_driver_list, dptr[0].textfield);
    else
        key = 0;

    entry = search_list(&ltemplate_or_driver_list, key);

    dialog__setfield_str(NAME_FIELD, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,  NAME_FIELD))
            dialog__bumpstring(NAME_FIELD, f, &ltemplate_or_driver_list, &key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed load template or edit driver action %d\n", f);
        }

    if(!dialog__fillin_ok)
        return;

    dialog__getfield(NAME_FIELD, &dptr[0]);
}

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
    LIST *      entry;
    LIST_ITEMNO language_key;

    assert_dialog(1, D_USER_CREATE);

    dialog__setfield(createdict_First,  &dptr[0]);

    language_key = dialog__initkeyfromstring(&language_list,
                                             !str_isblank(dptr[1].textfield)
                                                    ? dptr[1].textfield
                                                    : DEFAULT_DICTDEFN_FILE_STR);

    entry = search_list(&language_list, language_key);

    dialog__setfield_str(createdict_Second, entry ? entry->value : NULL);

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        if(dialog__adjust(&f,  createdict_Second))
            dialog__bumpstring(createdict_Second, f, &language_list, &language_key);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed createdict action %d\n", f);
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

typedef enum
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
}
loadfile_offsets;

extern void
dproc_loadfile(
    DIALOG *dptr)
{
    assert_dialog(3, D_LOAD);

    dialog__setfield(loadfile_Name,                                &dptr[0]);
    dialog__setcomponoff(loadfile_Slot,                            &dptr[1]);
    dialog__setcomponoff(loadfile_RowRange,                        &dptr[2]);
    dialog__setradio(loadfile_filetype_stt, loadfile_filetype_end, &dptr[3]);

    dialog__simple_fillin(TRUE);

    if(!dialog__fillin_ok)
        return;

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

typedef enum
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
}
savefile_offsets;

typedef struct _savefile_callback_info
{
    DIALOG *   dptr;
    DOCNO      docno;
}
savefile_callback_info;

/* xfersend calling us to handle clicks on the dbox fields */

static void
savefile_clickproc(
    dbox d,
    dbox_field f,
    int *filetypep,
    void * handle)
{
    savefile_callback_info * i = handle;
    DIALOG * dptr;
    PC_U8 optlistptr;

    IGNOREPARM(d);

    trace_3(TRACE_APP_DIALOG, "savefile_clickproc(%d, &%p, &%p)\n", f, report_ptr_cast(filetypep), handle);

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
            trace_1(TRACE_APP_DIALOG, "unprocessed savefile_clickproc action %d\n", f);
        }
}

/* xfersend calling us to save a file */

static BOOL
savefile_saveproc(
    _In_z_      /*const*/ char * filename /*low lifetime*/,
    void * handle)
{
    savefile_callback_info * i = (savefile_callback_info *) handle;
    DIALOG * dptr;
    wimp_eventstr * e;
    wimp_mousestr ms;
    BOOL recording;
    BOOL res = TRUE;

    trace_2(TRACE_APP_DIALOG, "savefile_saveproc(%s, %d)\n", filename, (int) handle);

    if(!select_document_using_docno(i->docno))
        return(FALSE);

    /* now data^ valid */
    dptr = i->dptr;

    (void) mystr_set(&dptr[SAV_NAME].textfield, filename); /* esp. for macro recorder */

    dialog__getcomponoff(savefile_RowSelection,                    &dptr[SAV_ROWCOND]);
    dialog__getonoff(savefile_MarkedBlock,                         &dptr[SAV_BLOCK]);
    dialog__getarray(savefile_Separator,                           &dptr[SAV_LINESEP]);
    dialog__getradio(savefile_filetype_stt, savefile_filetype_end, &dptr[SAV_FORMAT]);

    /* try to look for wally saves to the same window */
    e = wimpt_last_event();

    trace_1(TRACE_APP_DIALOG, "last wimp event was %s\n", report_wimp_event(e->e, &e->data));

    if(e->e != wimp_EKEY)
        {
        if(e->e == wimp_EBUT) /* sodding dbox hitfield faking event ... */
            {
            ms.w = e->data.but.m.w;
            ms.i = e->data.but.m.i;
            }
        else
            {
            wimpt_safe(wimp_get_point_info(&ms));
            trace_4(TRACE_APP_DIALOG, "mouse position at %d %d, window %d, icon %d\n",
                    ms.x, ms.y, ms.w, ms.i);
            }

        if((ms.w == rear__window) || (ms.w == main__window) || (ms.w == colh__window))
            if('Y' != dptr[SAV_BLOCK].option)
                /* do allow save of marked block into self, e.g. copying a selection */
                /* could do more checking, eg. curpos not in marked block for save */
                res = reperr_null(create_error(ERR_CANTSAVETOITSELF));
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
    savefile_callback_info * i = (savefile_callback_info *) handle;
    BOOL recording;
    BOOL res;

    IGNOREPARM(filename);

    trace_2(TRACE_APP_DIALOG, "savefile_printproc(%s, %d)\n", filename, (int) handle);

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
    wimp_caretstr current;
    savefile_callback_info i;
    PCTSTR filename = riscos_obtainfilename(dptr[SAV_NAME].textfield);
    FILETYPE_RISC_OS filetype = currentfiletype(dptr[SAV_FORMAT].option);
    S32 estsize = 42;
    BOOL restore_caret = FALSE;

    wimp_get_caret_pos(&current);

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
    BOOL submenu = ((e->e == wimp_ESEND)  &&  (e->data.msg.hdr.action == wimp_MMENUWARN));
    if(submenu)
        dbox_show(dialog__dbox);
    else
        {
        restore_caret = TRUE;
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
        if(restore_caret)
            {
            if(current.w == main__window)
                xf_acquirecaret = TRUE; /* in case we've moved in the current document */
            else
                wimp_set_caret_pos(&current); /* generic restore attempt */
            }

    dialog__fillin_ok = FALSE;
}

/******************************************************************************
*
* overwrite file dialog box
*
******************************************************************************/

extern void
dproc_overwrite(
    DIALOG *dptr)
{
    assert_dialog(0, D_OVERWRITE);

    dproc__query(dptr, Overwrite_existing_file_STR);
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

    dproc__query(dptr, Cannot_store_block_STR);
}

/******************************************************************************
*
*  colours dialog box
*
******************************************************************************/

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
    DIALOG temp_dialog[N_COLOURS];        /* update temp copy! */

    assert_dialog(N_COLOURS - 1, D_COLOURS);

    for(i = 0; i < N_COLOURS; i++)
        {
        dialog__setnumeric(4*i + colours_Number, &dptr[i]);
        dialog__setcolour( 4*i + colours_Patch,  &dptr[i]);
        temp_dialog[i] = dptr[i];
        }

    while(((f = dialog__fillin(TRUE)) != dbox_CLOSE)  &&  (f != dbox_OK))
        {
        for(i = 0; i < N_COLOURS; i++)
            if(dialog__adjust(&f, 4*i + colours_Number))
                {
                dialog__bumpnumericlimited(4*i + colours_Number,
                                                        &temp_dialog[i], f, 0x00, 0x0F);
                dialog__setcolour(4*i + colours_Patch,  &temp_dialog[i]);
                }
        }

    /* only update colour globals if OK */
    if(dialog__fillin_ok)
        for(i = 0; i < N_COLOURS; i++)
            dialog__getnumericlimited(4*i + colours_Number, &dptr[i], 0x00, 0x0F);
}

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
#define options_DateEnglish     22
#define options_DateAmerican    23
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
    dialog__setradio(options_DateText, options_DateAmerican,      &dptr[10]);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed options action %d\n", f);
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
    dialog__getradio(options_DateText, options_DateAmerican,      &dptr[10]);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed pagelayout action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed options action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed insertchar action %d\n", f);

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
            trace_1(TRACE_APP_DIALOG, "unprocessed defkey action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed deffnkey action %d\n", f);
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

        /* rjm on 22.9.91 thinks d_driver_PT is a windvar at this point */
        /* sks confirms rjm's comment - only the windvars for a particular
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
            trace_1(TRACE_APP_DIALOG, "unprocessed print action %d\n", f);
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
    LIST_ITEMNO key;
    LIST *      entry;

    assert_dialog(6, D_DRIVER);

    dialog__setarray(printconfig_Type,     &dptr[0]);
    dialog__setfield(printconfig_Server,   &dptr[2]);
    dialog__setarray(printconfig_Baud,     &dptr[3]);
    dialog__setspecial(printconfig_Data,   &dptr[4]);
    dialog__setspecial(printconfig_Parity, &dptr[5]);
    dialog__setspecial(printconfig_Stop,   &dptr[6]);

    /* leave field blank unless driver selected. then he's forced to select default file to regain default settings */
    if(!str_isblank(dptr[1].textfield))
        {
        key = dialog__initkeyfromstring(&ltemplate_or_driver_list, dptr[1].textfield);
        entry = search_list(&ltemplate_or_driver_list, key);
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
            dialog__bumpstring(    printconfig_Driver,            f, &ltemplate_or_driver_list, &key);
        else if(dialog__adjust(&f, printconfig_Baud))
            dialog__bumparray(     printconfig_Baud,    &dptr[3], f);
        else if(dialog__adjust(&f, printconfig_Data))
            dialog__bumpspecial(   printconfig_Data,    &dptr[4], f);
        else if(dialog__adjust(&f, printconfig_Parity))
            dialog__bumpspecial(   printconfig_Parity,  &dptr[5], f);
        else if(dialog__adjust(&f, printconfig_Stop))
            dialog__bumpspecial(   printconfig_Stop,    &dptr[6], f);
        else
            trace_1(TRACE_APP_DIALOG, "unprocessed printconfig action %d\n", f);
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
            trace_1(TRACE_APP_DIALOG, "unprocessed microspace action %d\n", f);
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
* problem or returning the expression as a text slot
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

    trace_2(TRACE_APP_DIALOG, "riscdialog_replace_dbox(%s, %s)\n",
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
    trace_1(TRACE_APP_DIALOG, "riscdialog_replace_dbox() returns %d\n", res);
    return(res);
}

extern void
riscdialog_replace_dbox_end(void)
{
    trace_0(TRACE_APP_DIALOG, "riscdialog_replace_dbox_end()\n");

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
    trace_1(TRACE_APP_DIALOG, "timesofar = %d\n", monotime_diff(starttime));
    if(monotime_diff(starttime) >= lengthtime)
        /* Cause dbox_CLOSE to be returned to the main process */
        win_send_close(dbox_syshandle(dialog__dbox));
}

extern void
riscdialog_dopause(
    S32 nseconds)
{
    dbox d;
    dbox_field f;

    trace_1(TRACE_APP_DIALOG, "riscdialog_dopause(%d)\n", nseconds);

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

    pausing_docno = DOCNO_NONE;

    dialog__dispose();
}

/* end of ridialog.c */
