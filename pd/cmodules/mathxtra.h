/* mathxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* SKS July 1991 */

#ifndef __mathxtra_h
#define __mathxtra_h

/*
function declarations
*/

#define PRAGMA_SIDE_EFFECTS_OFF
#include "coltsoft/pragma.h"
/* note that ANSI errno is volatile to enable this sort of CSE optimization */

extern F64
mx_acosh(
    F64 x);

extern F64
mx_acosec(
    F64 x);

extern F64
mx_acosech(
    F64 x);

extern F64
mx_acot(
    F64 x);

extern F64
mx_acoth(
    F64 x);

extern F64
mx_asec(
    F64 x);

extern F64
mx_asech(
    F64 x);

extern F64
mx_asinh(
    F64 x);

extern F64
mx_atanh(
    F64 x);

extern F64
mx_cosec(
    F64 x);

extern F64
mx_cosech(
    F64 x);

extern F64
mx_cot(
    F64 x);

extern F64
mx_coth(
    F64 x);

static __forceinline F64
mx_fsquare(
    F64 x)
{
    return(x * x);
}

extern F64
mx_fhypot(
    F64 x,
    F64 y);

extern F64
mx_sec(
    F64 x);

extern F64
mx_sech(
    F64 x);

#define PRAGMA_SIDE_EFFECTS
#include "coltsoft/pragma.h"

#endif /* __mathxtra_h */

/* end of mathxtra.h */
