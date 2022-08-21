/* mysignal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Define access to signal functions */

#ifndef __mysignal_h
#define __mysignal_h

/* get standard definitions */
#include <signal.h>

typedef struct
{
    void (* oldhandler) (int);
    S32 sigset;
}
mysignal_save;

/* signal handler is a near-normal library callback */
#define SIGENTRY

#define mysignal_start(sig, proc, pss, hInst) \
    { \
    (pss)->sigset     = sig; \
    (pss)->oldhandler = signal(sig, proc); \
    }

#define mysignal_end(pss) \
    (void) signal((pss)->sigset, (pss)->oldhandler)

#endif /* __mysignal_h */

/* end of mysignal.h */
