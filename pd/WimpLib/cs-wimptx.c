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

#include "colourtran.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

extern os_error *
wimp_Poll_SFM(wimp_emask mask, wimp_eventstr *result);

extern wimp_errflags
wimp_ReportError_wrapped(
    const _kernel_oserror * e,
    wimp_errflags flags,
    const char * name,
    const char * spritename,
    const char * spritearea,
    const char * buttons)
{
    wimp_errflags errflags = wimp_ECANCEL;
    _kernel_swi_regs rs;

    rs.r[0] = (int) e;
    rs.r[1] = flags;
    rs.r[2] = (int) name;
    rs.r[3] = (int) spritename;
    rs.r[4] = (int) spritearea;
    rs.r[5] = (int) buttons;

    riscos_hourglass_off();

    (void) _kernel_swi(Wimp_ReportError, &rs, &rs);
    errflags = (wimp_errflags) rs.r[1];

    riscos_hourglass_on();

    return(errflags);
}

/*
exported for macros
*/

int wimpt__xeig  = 1;
int wimpt__xsize = 1280;

int wimpt__yeig  = 2;
int wimpt__ysize = 1024;

int wimpt__title_height   = 40;
int wimpt__hscroll_height = 40;
int wimpt__vscroll_width  = 40;

wimp_palettestr wimpt__palette;

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

/* replacement for one in wimp.c to use new mechanism */

extern os_error *
wimp_reporterror(
    os_error * e,
    wimp_errflags flags,
    char * name)
{
    wimp_errflags flags_dummy;
    return(wimp_reporterror_rf(e, flags, name, &flags_dummy, NULL));
}
 
extern os_error *
wimp_reporterror_rf(
    const _kernel_oserror * e,
    wimp_errflags flags,
    const char *name,
    wimp_errflags * flags_out,
    const char *message)
{
    static _kernel_oserror ie;

    ie.errnum = 1 /* lie to over-knowledgeable RISC PC error handler - was e->errnum */;

    {
    const char * p_u8 = e->errmess;
    char * p_u8_out = ie.errmess;
    int len = 0;
    int non_blanks = 0;

    for(;;)
    {
        U8 c = *p_u8++;
        *p_u8_out++ = c;
        if(!c)
            break;
        ++len;

        if((c != 0x20) && (c != 0xA0))
            ++non_blanks;

        if((c < 0x20) || (c == 0x7F))
            /* Found control characters in error string */
            p_u8_out[-1] = '.';
    }

    if(!len || !non_blanks)
        e = os_set_error(1, "No printing characters in error string");
    else
        e = &ie;
    }

    if(message)
    {
        flags = (wimp_errflags) (flags | wimp_ENOERRORFROM);
        name = message;
    }

    if(wimpt_os_version_query() >= RISC_OS_3_5)
    {
        flags = (wimp_errflags) (flags | wimp_EUSECATEGORY); /* new-style */
        flags = (wimp_errflags) (flags | wimp_ECAT_WARNING); /* error */
    }

    *flags_out = wimp_ReportError_wrapped(e, flags, name, wimpt_get_spritename(), (const char *) 1 /*Wimp Sprite Area*/, NULL);

    return(de_const_cast(os_error *, e));
}

static int wimpt__os_version;

static int wimpt__platform_features; /* 19aug96 */

extern void
wimpt_os_version_determine(void)
{
    wimpt__os_version = (_kernel_osbyte(0x81, 0, 0xFF) & 0xFF);

    { /* 19aug96 */
    _kernel_swi_regs rs;
    rs.r[0] = 0; /*Read code features*/
    if(NULL == _kernel_swi(/*OS_PlatformFeatures*/ 0x6D, &rs, &rs))
        wimpt__platform_features = rs.r[0];
    } /*block*/
}

extern int
wimpt_os_version_query(void)
{
    return(wimpt__os_version);
}

extern int
wimpt_platform_features_query(void)
{
    return(wimpt__platform_features);
}

/*****************************************
*
* recache variables after a palette change
*
*****************************************/

extern void
wimpt_checkpalette(void)
{
    colourtran_invalidate_cache();

    wimpt_safe(wimp_readpalette(&wimpt__palette));

    /* copy over only info needed */
#if TRACE
    {
    int i;
    for(i = 0; i < sizeof(wimpt__palette.c)/sizeof(wimpt__palette.c[0]); ++i)
        tracef2("palette entry %2d = &%8.8X", i, wimpt__palette.c[i]);
    }
#endif
}

static const char * wimpt__spritename;

extern const char *
wimpt_get_spritename(void)
{
    return(wimpt__spritename);
}

extern void
wimpt_set_spritename(const char * spritename)
{
    wimpt__spritename = spritename;
}

static const char * wimpt__taskname;

extern const char *
wimpt_get_taskname(void)
{
    return(wimpt__taskname ? wimpt__taskname : wimpt_programname());
}

extern void
wimpt_set_taskname(const char * taskname)
{
    wimpt__taskname = taskname;
}

extern int wimpt__must_die;
int wimpt__must_die = 0;

/*
SKS added wimp_poll wrapping atentry/atexit procs
*/

static wimpt_atentry_t wimpt__atentryproc;
static wimpt_atexit_t  wimpt__atexitproc;

extern wimpt_atentry_t
wimpt_atentry(wimpt_atentry_t pfnNewProc)
{
    wimpt_atentry_t pfnOldProc = wimpt__atentryproc;
    wimpt__atentryproc = pfnNewProc;
    return(pfnOldProc);
}

extern wimpt_atexit_t
wimpt_atexit(wimpt_atexit_t pfnNewProc)
{
    wimpt_atexit_t pfnOldProc = wimpt__atexitproc;
    wimpt__atexitproc = pfnNewProc;
    return(pfnOldProc);
}

#include "wimpt.c"

#undef profile_ensure_frame
extern void
profile_ensure_frame(void);
extern void
profile_ensure_frame(void)
{
    /*EMPTY*/
}

/* end of cs-wimptx.c */
