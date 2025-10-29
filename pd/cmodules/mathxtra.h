/* mathxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#ifndef MATHXTRA_H
#define MATHXTRA_H

/*
exported functions
*/

#define PRAGMA_SIDE_EFFECTS_OFF
#include "coltsoft/pragma.h"
/* note that ANSI errno is volatile to enable this sort of CSE optimization */

_Check_return_
extern F64
mathxtra_acosh(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_acosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_acosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_acot(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_acoth(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_asec(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_asech(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_asinh(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_atanh(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_cosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_cosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_cot(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_coth(
    _InVal_     F64 x);

/* return the square of a number (or more likely, hard expression) */

_Check_return_
static inline F64
mathxtra_square(
    _InVal_     F64 x)
{
    return(x * x);
}

_Check_return_
extern F64
mathxtra_hypot(
    _InVal_     F64 x,
    _InVal_     F64 y);

_Check_return_
extern F64
mathxtra_sec(
    _InVal_     F64 x);

_Check_return_
extern F64
mathxtra_sech(
    _InVal_     F64 x);

#define PRAGMA_SIDE_EFFECTS
#include "coltsoft/pragma.h"

_Check_return_
extern double FreeBSD_jn(int, double);

#define bessel_jn(n, x) FreeBSD_jn(n, x)

_Check_return_
extern double FreeBSD_yn(int, double);

#define bessel_yn(n, x) FreeBSD_yn(n, x)

#endif /* MATHXTRA_H */

/* end of mathxtra.h  */
