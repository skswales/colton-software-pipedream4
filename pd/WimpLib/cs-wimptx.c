/* cs-wimptx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

#include "os.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

/*
exported for macros
*/

unsigned int wimptx__XEigFactor = 1U;
unsigned int wimptx__YEigFactor = 2U;

unsigned int wimptx__display_size_cx = 1280U;
unsigned int wimptx__display_size_cy = 1024U;

int wimptx__title_height   = 40;
int wimptx__hscroll_height = 40;
int wimptx__vscroll_width  = 40;

wimp_palettestr wimptx__palette;

extern _kernel_oserror *
wimp_poll_coltsoft(
    _In_        wimp_emask mask,
    _Out_       /*WimpPollBlock*/ void * block,
    _Inout_opt_ int * pollword,
    _Out_       int * event_code);

extern int /*button_clicked*/
wimp_ReportError_wrapped(
    const _kernel_oserror * e,
    int errflags,
    const char * name,
    const char * spritename,
    const char * spritearea,
    const char * buttons)
{
    int button_clicked = 1;

    static _kernel_swi_regs rs; /* flatter for stack overflow handler */

    rs.r[0] = (int) e;
    rs.r[1] = errflags;
    rs.r[2] = (int) name;
    rs.r[3] = (int) spritename;
    rs.r[4] = (int) spritearea;
    rs.r[5] = (int) buttons;

    riscos_hourglass_off();

    if(NULL == _kernel_swi(Wimp_ReportError, &rs, &rs))
        button_clicked = rs.r[1];

    riscos_hourglass_on();

    return(button_clicked);
}

extern os_error *
os_set_error(
    int errnum,
    const char * errmess)
{
    static os_error e;

    e.errnum = errnum;
    e.errmess[0] = CH_NULL;
    strncat(e.errmess, errmess, sizeof(e.errmess));

    return(&e);
}

extern const _kernel_oserror *
wimp_reporterror_rf(
    _In_        const _kernel_oserror * e,
    _InVal_     int errflags_in,
    _Out_       int * p_button_clicked,
    _In_opt_z_  const char * message,
    _InVal_     int error_level)
{
    static _kernel_oserror g_e;

    int errflags = errflags_in;

    *p_button_clicked = 1;

#if 0
    g_e.errnum = 1 /* lie to over-knowledgeable RISC PC error handler - was e->errnum */;
#else
    g_e.errnum = e->errnum;
#endif

    {
    const char * p_u8 = e->errmess;
    char * p_u8_out = g_e.errmess;
    int len = 0;
    int non_blanks = 0;

    for(;;)
    {
        U8 c = *p_u8++;
        *p_u8_out++ = c;
        if(CH_NULL == c)
            break;
        ++len;

        if((c != 0x20) && (c != 0xA0))
            ++non_blanks;

        if((c <= 0x1F) || (c == 0x7F))
            /* Found control characters in error string */
            p_u8_out[-1] = CH_FULL_STOP;
    }

    if(0 == len)
        strcpy(g_e.errmess, "No characters in error string");
    else if(0 == non_blanks)
        strcpy(g_e.errmess, "All characters in error string are blank");
    }

    e = &g_e;

    if(wimptx_os_version_query() >= RISC_OS_3_5)
    {
        if(NULL != message)
            errflags |= Wimp_ReportError_NoAppName;

        errflags |= Wimp_ReportError_UseCategory; /* new-style */
        errflags |= Wimp_ReportError_Category(error_level); /* 11..9 */
    }

    *p_button_clicked = wimp_ReportError_wrapped(e, errflags, message ? message : wimpt_programname(), wimptx_get_spritename(), (const char *) 1 /*Wimp Sprite Area*/, NULL);

    return(e);
}

static int wimptx__os_version;

static int wimptx__platform_features; /* 19aug96 */

extern void
wimptx_os_version_determine(void)
{
    wimptx__os_version = (_kernel_osbyte(0x81, 0, 0xFF) & 0xFF);

    { /* 19aug96 */
    _kernel_swi_regs rs;
    rs.r[0] = 0; /*Read code features*/
    if(NULL == _kernel_swi(/*OS_PlatformFeatures*/ 0x6D, &rs, &rs))
        wimptx__platform_features = rs.r[0];
    } /*block*/
}

extern int
wimptx_os_version_query(void)
{
    return(wimptx__os_version);
}

extern int
wimptx_platform_features_query(void)
{
    return(wimptx__platform_features);
}

/*****************************************
*
* recache variables after a palette change
*
*****************************************/

extern void
wimptx_checkpalette(void)
{  
    {
    _kernel_swi_regs rs;
    (void) wimpt_complain(_kernel_swi(ColourTrans_InvalidateCache, &rs, &rs));
    } /*block*/

    (void) wimpt_complain(wimp_readpalette(&wimptx__palette));

    /* copy over only info needed */
#if TRACE
    {
    int i;
    for(i = 0; i < sizeof(wimptx__palette.c)/sizeof(wimptx__palette.c[0]); ++i)
        tracef2("palette entry %2d = &%8.8X", i, wimptx__palette.c[i].word);
    }
#endif
}

static const char * wimptx__spritename;

extern const char *
wimptx_get_spritename(void)
{
    return(wimptx__spritename);
}

extern void
wimptx_set_spritename(const char * spritename)
{
    wimptx__spritename = spritename;
}

static const char * wimptx__taskname;

extern const char *
wimptx_get_taskname(void)
{
    return(wimptx__taskname ? wimptx__taskname : wimpt_programname());
}

extern void
wimptx_set_taskname(const char * taskname)
{
    wimptx__taskname = taskname;
}

/*
SKS added wimp_poll wrapping atentry/atexit procs
*/

/*static wimptx_atentry_t wimptx__atentryproc;*/
static wimptx_atexit_t  wimptx__atexitproc;

#if defined(UNUSED)
extern wimptx_atentry_t
wimptx_atentry(wimptx_atentry_t pfnNewProc)
{
    wimptx_atentry_t pfnOldProc = wimptx__atentryproc;
    wimptx__atentryproc = pfnNewProc;
    return(pfnOldProc);
}
#endif

extern wimptx_atexit_t
wimptx_atexit(wimptx_atexit_t pfnNewProc)
{
    wimptx_atexit_t pfnOldProc = wimptx__atexitproc;
    wimptx__atexitproc = pfnNewProc;
    return(pfnOldProc);
}

static BOOL g_host_must_die = FALSE;

_Check_return_
extern BOOL
host_must_die_query(void)
{
    return(g_host_must_die);
}

extern void
host_must_die_set(
    _InVal_     BOOL must_die)
{
    g_host_must_die = must_die;
}

/* if message system ok then all well and good, but there is a chance of fulkup */

static jmp_buf * program_safepoint = NULL;

extern void
wimptx_set_safepoint(jmp_buf * safe)
{
    program_safepoint = safe;
}

#if !CROSS_COMPILE
#pragma no_check_stack
#endif

static void
wimptx_jmp_safepoint(int signum)
{
    if(program_safepoint)
        longjmp(*program_safepoint, signum);
}

static void
wimptx_internal_stack_overflow_handler(int sig)
{
    /* Pass it up to someone who may have some stack to handle it! */

    /* give it your best shot else we come back and die soon */
    wimptx_jmp_safepoint(sig);

    exit(EXIT_FAILURE);
}

#if !CROSS_COMPILE
#pragma check_stack
#endif

static void wimptx_signal_handler(int sig);
#define handler wimptx_signal_handler
#define errhandler wimptx_signal_handler

#undef BOOL
#undef TRUE
#undef FALSE

#include "wimpt.c"

/* keep defaults for these in case of msgs death */

static const U8Z
error_fatal_str[] =
    "wimpt2:%s has gone wrong (%s). "
    "Click Continue to quit, losing data";

static const U8Z
error_serious_str[] =
    "wimptx2:%s has gone wrong (%s). "
    "Click Continue to try to resume or Cancel to quit, losing data.";

static void
wimptx_signal_handler(int sig)
{
    _kernel_oserror err;
    char causebuffer[64];
    const char * cause;
    BOOL must_die;
    BOOL jump_back = FALSE;
    int errflags;
    int button_clicked;

    switch(sig)
    {
    case SIGOSERROR:
        cause = _kernel_last_oserror()->errmess;
        break;

    default:
        consume_int(snprintf(causebuffer, elemof32(causebuffer), "signal%d", sig));
        if((cause = msgs_lookup(causebuffer)) == causebuffer)
        {
            consume_int(snprintf(causebuffer, elemof32(causebuffer), msgs_lookup("(%d)"), sig));
            cause = causebuffer;
        }
        break;
    }

    if(NULL == program_safepoint)
        host_must_die_set(TRUE);

    must_die = host_must_die_query();
    host_must_die_set(TRUE); /* trap errors in lookup/sprintf etc */

    err.errnum = sig;
    err.errnum |= (1U << 31); /* helpful Quit button for the users on RISC OS 3.5+ */
    consume_int(snprintf(err.errmess, elemof32(err.errmess),
                         msgs_lookup(de_const_cast(char *, must_die ? error_fatal_str : error_serious_str)),
                         wimpt_programname(),
                         cause));

    errflags = (must_die ? Wimp_ReportError_OK : Wimp_ReportError_OK | Wimp_ReportError_Cancel);

    errflags |= Wimp_ReportError_UseCategory; /* new-style */
    errflags |= Wimp_ReportError_Category(3); /*STOP*/ /* error */

    if(must_die)
    {
        button_clicked = wimp_ReportError_wrapped(&err, errflags, wimpt_programname(), wimptx_get_spritename(), (const char *) 1 /*Wimp Sprite Area*/, NULL);
        /* ignore button_clicked - we must die */
    }
    else
    {
        button_clicked = wimp_ReportError_wrapped(&err, errflags, wimpt_programname(), wimptx_get_spritename(), (const char *) 1 /*Wimp Sprite Area*/, NULL);
        jump_back = (1 == button_clicked);
    }

    /* reinstall ourselves, as SIG_DFL will have been restored (as defined by the ANSI spec) */
    /* helps to trap any further errors during closedown rather than giving postmortem dump */
    switch(sig)
    {
    case SIGSTAK:
        (void) signal(sig, &wimptx_internal_stack_overflow_handler);
        return;

    default:
        break;
    }

    (void) signal(sig, &wimptx_signal_handler);

    if(jump_back)
        /* give it your best shot else we come back and die soon */
        wimptx_jmp_safepoint(sig);

    exit(EXIT_FAILURE);
}

/* call from routine that longjmp() returns to! */

extern void
wimptx_stack_overflow_handler(int sig)
{
    wimptx_signal_handler(sig);
}

#undef profile_ensure_frame
extern void
profile_ensure_frame(void);
extern void
profile_ensure_frame(void)
{
    /*EMPTY*/
}

/* end of cs-wimptx.c */
