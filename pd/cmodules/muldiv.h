/* muldiv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#ifndef __muldiv_h
#define __muldiv_h

#if defined(__cplusplus)
extern "C" {
#endif

/*
exported functions
*/

extern void
muldiv64_init(void);

/* careful 32-bit * 32-bit / 32-bit a la BCPL */

_Check_return_
extern S32
muldiv64(
    _InVal_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator);

/* no need for muldiv64_remainder, muldiv64_ceil, muldiv64_floor in PipeDream */

_Check_return_
extern S32
muldiv64_overflow(void);

/* muldiv64, but limit against +/-S32_MAX on overflows */

_Check_return_
extern S32
muldiv64_limiting(
    _InVal_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator);

/* no need for muldiv64_round_floor in PipeDream */

#if defined(__cplusplus)
}
#endif

#endif /* __muldiv_h */

/* end of muldiv.h */
