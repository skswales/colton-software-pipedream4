/* muldiv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2021 Stuart Swales */

/* SKS October 2021 */

#include "common/gflags.h"

#include "cmodules/muldiv.h"

/* muldiv64, but limit against +/-S32_MAX on overflows */

#define  PRAGMA_CHECK_STACK_OFF 1
#include "cmodules/coltsoft/pragma.h"

extern S32
muldiv64_limiting(S32 a, S32 b, S32 c)
{
    S32 retval = muldiv64(a, b, c);
    const S32 overflow = muldiv64_overflow();

    if(0 == overflow)
        return(retval);

    retval = (overflow < 0) ? -S32_MAX : +S32_MAX;
    return(retval);
}

/* end of muldiv.c */
