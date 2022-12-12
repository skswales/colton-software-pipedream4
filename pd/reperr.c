/* reperr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Error reporting */

#include "common/gflags.h"

#ifndef MAKE_MESSAGE_FILE
#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif
#endif /* MAKE_MESSAGE_FILE */

#include "errdef.h"

#include "cmodules/stringlk.h"

#ifndef MAKE_MESSAGE_FILE
_Check_return_
_Ret_maybenull_
extern const char *
strings_lookup(
    S32 stringid);
#endif

#include <stdlib.h>
#include <stdarg.h>

/* now limit cmodules exports to error definitions (if not already got) */
#define ERRDEF_EXPORT

#include "cmodules/spell.h"

#include "cmodules/ev_eval.h"

#include "cmodules/vsload.h"

#include "cmodules/file.h"

#ifndef MLEC_OFF
#include "cmodules/mlec.h"
#endif

#include "cmodules/gr_chart.h"

#ifndef MAKE_MESSAGE_FILE
#define errorstring(errnum, value)  "", /* reserve a pointer to it */
#else
#define errorstring(errnum, value)  make_errorstring(errnum, value);
#endif

#ifdef MAKE_MESSAGE_FILE

#ifndef MSGS_LOOKUP_DELIMITER_STR
#define MSGS_LOOKUP_DELIMITER_STR "_"
#endif

extern int
main(
    int argc,
    char * argv[]);

static void
make_errorstring(
    STATUS errnum,
    PC_U8 value)
{
    S32 base, increment, moduleno;
    const char * appformat = "e%d:";
    const char * modformat = "e%d" MSGS_LOOKUP_DELIMITER_STR "%d:";

    errnum    = -errnum;
    base      = -MODULE_ERR_BASE;
    increment = +MODULE_ERR_INCREMENT;

    if(errnum >= base)
    {
        moduleno = errnum / increment;
        errnum   = errnum % increment;
    }
    else
        moduleno = 0;

    if(moduleno == 0)
        printf(appformat, errnum);
    else
        printf(modformat, moduleno, errnum);

    /* output value */
    puts(value);
}

#endif /* MAKE_MESSAGE_FILE */

#ifdef MAKE_MESSAGE_FILE
static void
make_reperr_init(void)
{
    fputs("# PipeDream 4 en_GB error messages (" __DATE__ ")" "\n", stdout);

/*
errors local to this application
*/
    {APP_ERRLIST_DEF};

/*
spelling checker error table
*/
    {SPELL_ERRLIST_DEF};

/*
evaluator errors
*/
    {EVAL_ERRLIST_DEF};

    {FILE_ERRLIST_DEF};

#ifndef MLEC_OFF
    {MLEC_ERRLIST_DEF};
#endif

    {GR_CHART_ERRLIST_DEF};

} /* end of make_reperr_init() */

#endif /* MAKE_MESSAGE_FILE */

#ifndef MAKE_MESSAGE_FILE

extern void
reperr_init(void)
{
    /* empty, all of a sudden */
}

/******************************************************************************
*
*  reperr() - report error, doesn't yet do system messages
*
*  --out--
*   error reported, been_error set TRUE
*
******************************************************************************/

extern void
messagef(
    _In_z_ _Printf_format_string_ PC_U8Z format,
    ...)
{
    os_error err;
    va_list args;

    va_start(args, format);

#if defined(RISCOS_CMD_UTIL)
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
#else
    reportf("*** Message follows: ***");
    vreportf(format, args);

    va_end(args);

    va_start(args, format);

    err.errnum = 0;

    if(vsnprintf(err.errmess, elemof32(err.errmess), format, args) < 0)
        err.errmess[0] = CH_NULL;

    void_WrapOsErrorReporting(&err);
#endif

    va_end(args);
}

extern void
message_output(
    _In_z_      PC_U8Z buffer)
{
    os_error err;

    reportf("*** Message follows: ***");
    report_output(buffer);

    err.errnum = 0;

    (void) xstrkpy(err.errmess, elemof32(err.errmess), buffer);

    void_WrapOsErrorReporting(&err);
}

/*ncr*/
extern BOOL /*FALSE*/
reperr(
    STATUS errornumber,
    _In_opt_z_  PC_U8Z text,
    ...)
{
    os_error err;
    char array[256];
    PC_U8 errorp;
    va_list args;

    trace_2(TRACE_APP_PD4, "reperr(%d, %s)", errornumber, trace_string(text));

    /* unusual start due to text being passed too */
    va_start(args, errornumber);

    if(errornumber == ERR_OUTPUTSTRING)
    {
        errorp = "%s";
    }
    else
    {
        errorp = reperr_getstr(errornumber);

        if(NULL == strstr(errorp, "%s"))
        {
            /* if we have an arg then append it if not a %s error */
            if(NULL != text)
            {
                xstrkpy(array, elemof32(array), errorp);
                xstrkat(array, elemof32(array), " %s");
                errorp = array;
            }
        }
    }

#if defined(RISCOS_CMD_UTIL)
    vfprintf(stderr, errorp, args);
    fputc('\n', stderr);

    va_end(args);

    been_error = TRUE;

    return(FALSE);
#else
    err.errnum = 0;

    if(vsnprintf(err.errmess, elemof32(err.errmess), errorp, args) < 0)
        err.errmess[0] = CH_NULL;

    va_end(args);

    return(reperr_kernel_oserror(&err));
#endif

}

/******************************************************************************
*
* return string for error number
*
******************************************************************************/

extern PC_U8
reperr_getstr(
    STATUS errornumber)
{
    PC_U8 res;

    res = string_lookup(errornumber);
    if(!res)
        res = "Error not found";

    trace_2(TRACE_APP_PD4, "reperr_getstr(%d) yields '%s'", errornumber, trace_string(res));
    return(res);
}

#if defined(ERR_NOTINSTALLED)

/******************************************************************************
*
* 'Not installed' error
*
******************************************************************************/

extern BOOL /*FALSE*/
reperr_not_installed(
    STATUS errornumber)
{
    return(reperr(errornumber, reperr_getstr(ERR_NOTINSTALLED)));
}

#endif

/******************************************************************************
*
*  reperr with no 2nd parm
*
******************************************************************************/

/*ncr*/
extern BOOL /*FALSE*/
reperr_null(
    STATUS errornumber)
{
    return(reperr(errornumber, NULL));
}

/*ncr*/
extern BOOL /*FALSE*/
rep_fserr(
    _In_z_      PC_U8 str)
{
    return(reperr(ERR_OUTPUTSTRING, str));
}

/*ncr*/
extern BOOL /*FALSE*/
reperr_kernel_oserror(
    _In_        _kernel_oserror * const p_kernel_oserror)
{
    assert(NULL != p_kernel_oserror);

    consume_bool(report_if_kernel_oserror(p_kernel_oserror));

    return(FALSE);
}

/*ncr*/
extern BOOL /*TRUE->is_error*/
report_if_kernel_oserror(
    _In_        _kernel_oserror * const p_kernel_oserror)
{
    /* for many callers, it's perfectly OK to do tbl_wimp_*() whatever, just call this, to hopefully get smallest code where ultimate speed is no concern */
    if(NULL == p_kernel_oserror)
        return(FALSE);

    int button_clicked;

    keyidx = NULL;
    cbuff_offset = 0;

    report_output("*** Error follows ***");

    if(been_error++ < 3) /* allow just a few errors to be reported before we start suppressing them */
    {
        consume(_kernel_oserror *, wimp_reporterror_rf(p_kernel_oserror, Wimp_ReportError_OK, &button_clicked, NULL, 2/*Warning*/));
    }
    else
    {   /* but do log suppressed errors */
        report_output(p_kernel_oserror->errmess);
    }

    return(TRUE);
}

/* replacements for RISC_OSLib */

extern os_error *
wimpt_complain(
    os_error * e)
{
    consume_bool(report_if_kernel_oserror(e));

    return(e);
}

extern os_error *
wimp_reporterror(
    os_error * e,
    wimp_errflags errflags,
    char * name)
{
    int button_clicked;

    UNREFERENCED_PARAMETER(name); /* always the program name from RISC_OSLib anyway */

    return(de_const_cast(os_error *, wimp_reporterror_rf(e, (int) errflags, &button_clicked, NULL, 2 /*Warning*/)));
}

#endif /* MAKE_MESSAGE_FILE */

#if !CROSS_COMPILE

#ifdef MAKE_MESSAGE_FILE

extern int
main(
    int argc,
    char * argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    make_reperr_init();

    return(ferror(stdout)
                ? EXIT_FAILURE
                : EXIT_SUCCESS);
}

#endif /* MAKE_MESSAGE_FILE */

#ifdef create_error
#undef create_error
#endif

_Check_return_
extern STATUS
create_error(
    _InVal_     STATUS status)
{
    /*status_assertc(status); not in PipeDream! */ /* just keep this function for breakpoints */
    return(status);
}

#ifdef status_nomem
#undef status_nomem
#endif

_Check_return_
extern STATUS
status_nomem(void)
{
    STATUS status = STATUS_NOMEM;
    status_assertc(status);
    return(status);
}

#ifdef status_check
#undef status_check
#endif

_Check_return_
extern STATUS
status_check(void)
{
    STATUS status = STATUS_CHECK;
    status_assertc(status);
    return(status);
}

#endif /* CROSS_COMPILE */

/* end of reperr.c */
