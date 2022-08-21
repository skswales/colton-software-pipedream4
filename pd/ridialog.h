/* ridialog.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Dialog box handling for RISC OS PipeDream */

/* Stuart K. Swales 14-Mar-1989 */

#if RISCOS

#ifndef __pd__riscdialog_h
#define __pd__riscdialog_h

/*
exported functions from riscdialog.c
*/

extern void
pausing_null(void);

extern void
riscos_install_pipedream(void);

extern S32
riscdialog_alert_user(
    char *title,
    char *message,
    char *button1,
    char *button2);

extern void
riscdialog_dispose(void);

extern void
riscdialog_dopause(
    S32 nseconds);

extern S32
riscdialog_ended(void);

extern S32
riscdialog_execute(
    dialog_proc proc,
    const char *dname,
    DIALOG *dptr,
    _InVal_     U32 boxnumber);

extern S32
riscdialog_fillin_for_browse(
    void *d);

extern void
riscdialog_front_dialog(void);

extern void
riscdialog_getfontsize(void);

extern void
riscdialog_initialise_once(void);

extern BOOL
riscdialog_in_dialog(void);

extern S32
riscdialog_input_from_user(
    char *title,
    char *message,
    char *button1,
    char *button2,
    char *text_out,
    int max_len);

enum RISCDIALOG_QUERY_DC_REPLY
{
    riscdialog_query_DC_DISCARD = 1,
    riscdialog_query_DC_CANCEL  = 2
};

extern enum RISCDIALOG_QUERY_DC_REPLY
riscdialog_query_DC(
    const char * statement,
    const char * question);

enum RISCDIALOG_QUERY_YN_REPLY
{
    riscdialog_query_YES     = 1,
    riscdialog_query_CANCEL  = 2,
    riscdialog_query_NO      = 3
};

extern enum RISCDIALOG_QUERY_YN_REPLY
riscdialog_query_YN(
    const char * statement,
    const char * question);

enum RISCDIALOG_QUERY_SDC_REPLY
{
    riscdialog_query_SDC_SAVE    = 1,
    riscdialog_query_SDC_CANCEL  = 2,
    riscdialog_query_SDC_DISCARD = 3
};

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_SDC(
    const char * statement,
    const char * question);

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_quit_SDC(
    const char * statement,
    const char * question);

extern enum RISCDIALOG_QUERY_SDC_REPLY
riscdialog_query_save_or_discard_existing(void);

extern S32
riscdialog_replace_dbox(
    const char *mess1,
    const char *mess2);

extern void
riscdialog_replace_dbox_end(void);

extern BOOL
riscdialog_warning(void);

extern BOOL
riscdialog_choose_colour(
    _OutRef_    P_U32 p_wimp_colour_value,
    _In_z_      PC_U8Z title);

/*
shared dprocs
*/

extern void
dproc_anasubgram(
    DIALOG *dptr);

extern void
dproc_numtext(
    DIALOG *dptr);

extern void
dproc_onecomponoff(
    DIALOG *dptr);

extern void
dproc_onenumeric(
    DIALOG *dptr);

extern void
dproc_onespecial(
    DIALOG *dptr);

extern void
dproc_onetext(
    DIALOG *dptr);

extern void
dproc_twotext(
    DIALOG *dptr);

/*
unique dprocs
*/

extern void
dproc_aboutfile(
    DIALOG *dptr);

extern void
dproc_aboutprog(
    DIALOG *dptr);

extern void
dproc_browse(
    DIALOG *dptr);

extern void
dproc_chartopts(
    DIALOG *dptr);

extern void
dproc_checkdoc(
    DIALOG *dptr);

extern void
dproc_checked(
    DIALOG *dptr);

extern void
dproc_checking(
    DIALOG *dptr);

extern void
dproc_colours(
    DIALOG *dptr);

#if defined(EXTENDED_COLOUR)

extern void
dproc_extended_colours(
    DIALOG *dptr);

#else

#define dproc_extended_colours dproc_colours

#endif /* EXTENDED_COLOUR */

extern void
dproc_createlinking(
    DIALOG *dptr);

extern void
dproc_decimal(
    DIALOG *dptr);

extern void
dproc_defkey(
    DIALOG *dptr);

extern void
dproc_deffnkey(
    DIALOG *dptr);

extern void
dproc_dumpdict(
    DIALOG *dptr);

extern void
dproc_loadfile(
    DIALOG *dptr);

extern void
dproc_microspace(
    DIALOG *dptr);

extern void
dproc_options(
    DIALOG *dptr);

extern void
dproc_overwrite(
    DIALOG *dptr);

extern void
dproc_pagelayout(
    DIALOG *dptr);

extern void
dproc_print(
    DIALOG *dptr);

extern void
dproc_printconfig(
    DIALOG *dptr);

extern void
dproc_savefile(
    DIALOG *dptr);

extern void
dproc_save_deleted(
    DIALOG *dptr);

extern void
dproc_search(
    DIALOG *dptr);

extern void
dproc_sort(
    DIALOG *dptr);

extern void
dproc_createdict(
    DIALOG *dptr);

extern void
dproc_loadtemplate(
    DIALOG *dptr);

extern void
dproc_edit_driver(
    DIALOG *dptr);

extern void
dproc_execfile(
    DIALOG *dptr);

extern void
dproc_edit_name(
    DIALOG *dptr);

extern void
dproc_formula_error(
    DIALOG *dptr);

extern void
dproc_insert_page_number(
    DIALOG *dptr);

extern void
dproc_insert_date(
    DIALOG *dptr);

extern void
dproc_insert_time(
    DIALOG *dptr);

#endif  /* __pd__riscdialog_h */

#endif  /* RISCOS */

/* end of ridialog.h */
