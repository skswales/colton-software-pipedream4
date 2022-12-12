/* help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2022 Stuart Swales */

/* Help system for PipeDream */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#if defined(NORCROFT_INLINE_SWIX)
#include "swis.h"
#endif

static void
help_starttask(PCTSTR command)
{
    reportf("StartTask %s", command);

#if defined(NORCROFT_INLINE_SWIX)
    void_WrapOsErrorReporting(_swix(Wimp_StartTask, _IN(0), command));
#else
    void_WrapOsErrorReporting(wimp_starttask((PTSTR) command));
#endif
}

extern void
Help_fn(void)
{
    const PCTSTR tempstr = "Run <PipeDream$Dir>.!Help index/htm";

    help_starttask(tempstr);
}

extern void
InteractiveHelp_fn(void)
{
    const PCTSTR tempstr = "Run <Help$Start>.!Run";

    help_starttask(tempstr);
}

extern void
ShowLicence_fn(void)
{
    const PCTSTR tempstr = "Run <PipeDream$Dir>.!Help licenc/htm"; /* NB limit to ten chars for systems without long file names */

    help_starttask(tempstr);
}

_Check_return_
extern STATUS
ho_help_url(
    _In_z_      PCTSTR url)
{
    TCHARZ tempstr[1024];

    if( (NULL == _kernel_getenv("Alias$Open_URI_http", tempstr, elemof32(tempstr))) &&
        (CH_NULL != tempstr[0]) )
    {
        _kernel_swi_regs rs;
        rs.r[0] = 0;
        if( (NULL == _kernel_swi(0x4E380 /*URI_Version*/, &rs, &rs)) )
        {
            consume_int(snprintf(tempstr, elemof32(tempstr), "URIdispatch %s", url));

            help_starttask(tempstr);

            return(STATUS_OK);
        }
    }

    if( (NULL == _kernel_getenv("Alias$URLOpen_HTTP", tempstr, elemof32(tempstr))) &&
        (CH_NULL != tempstr[0]) )
    {
        consume_int(snprintf(tempstr, elemof32(tempstr), "URLOpen_HTTP %s", url));

        help_starttask(tempstr);

        return(STATUS_OK);
    }

    return(-1/*create_error(ERR_HELP_URL_FAILURE)*/);
}

/* end of help.c */
