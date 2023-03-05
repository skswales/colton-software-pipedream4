/* help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2023 Stuart Swales */

/* Help system for PipeDream */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __res_h
#include "res.h"
#endif

#include "cs-msgs.h"

#if defined(NORCROFT_INLINE_SWIX)
#include "swis.h"
#endif

static char * help_messages___block;

static void
help_messages_init(void)
{
    if(NULL != help_messages___block) return;

    char filename[256];
    int tmp_length = res_findname("HelpMess", filename);
    if(tmp_length > 0)
        messages_readfile(&help_messages___block, filename);
}

extern const char *
help_messages_lookup(const char * tag_and_default)
{
    help_messages_init();

    return(messages_lookup(&help_messages___block, tag_and_default, FALSE /*->lookup is case-sensitive*/));
}

static void
help_starttask(_In_z_ PCTSTR command)
{
    // reportf("StartTask %s", command);

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
        if( (NULL == _swix(0x4E380 /*URI_Version*/, _IN(0), 0)) )
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
