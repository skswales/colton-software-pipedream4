
/* help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2014 Stuart Swales */

/* Help system for PipeDream */

/* SKS 2013 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

extern void
Help_fn(void)
{
    PTSTR tempstr = "Run <PipeDream$Dir>.!Help";
    reportf("StartTask %s", tempstr);
    wimp_starttask(tempstr);
}

/* end of help.c */
