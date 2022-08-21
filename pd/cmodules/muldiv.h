/* muldiv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* SKS July 1991 */

#ifndef __muldiv_h
#define __muldiv_h

#ifdef __cplusplus
extern "C" {
#endif

/*
exported structure
*/

typedef struct _UMUL64_RESULT
{
    U32 LowPart;
    U32 HighPart;
}
UMUL64_RESULT;

/*
exported functions
*/

#define PRAGMA_NO_SIDE_EFFECTS
#include "coltsoft/pragma.h"
/* no CSE disturbance here */

extern void
muldiv64_init(void);

/* careful 32-bit * 32-bit / 32-bit a la BCPL */
extern S32
muldiv64(
    S32 a,
    S32 b,
    S32 c);

extern S32
muldiv64_a_b_GR_SCALE_ONE(
    S32 a,
    S32 b);

extern S32
muldiv64_ceil(
    S32 a,
    S32 b,
    S32 c);

extern S32
muldiv64_floor(
    S32 a,
    S32 b,
    S32 c);

extern S32
muldiv64_round_floor(
    S32 a,
    S32 b,
    S32 c);

/* ditto, but limit against +/-S32_MAX on overflows
*/
extern S32
muldiv64_limiting(
    S32 a,
    S32 b,
    S32 c);

/* the overflow from a prior muldiv()
*/
extern S32
muldiv64_overflow(void);

/* the remainder from a prior muldiv()
*/
extern S32
muldiv64_rem(void);

extern void
umul64(
    U32 a,
    U32 b,
    UMUL64_RESULT * result /*out*/);

#define PRAGMA_SIDE_EFFECTS
#include "coltsoft/pragma.h"

#ifdef __cplusplus
}
#endif

#endif /* __muldiv_h */

/* end of muldiv.h */
