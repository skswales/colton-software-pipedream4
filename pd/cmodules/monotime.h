/* monotime.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Monotonic timer */

/* SKS October 1990 */

#ifndef __monotime_h
#define __monotime_h

/*
types
*/

/* a type that represents a monotonic time value */
typedef U32 MONOTIME;

/* a type that represents the difference between two times */
typedef S32 MONOTIMEDIFF;

#if RISCOS
/* centisecond */
#define MONOTIME_TICKS_PER_SECOND      100
#define MONOTIME_MILLISECONDS_PER_TICK 10
#else
/* millisecond */
#define MONOTIME_TICKS_PER_SECOND      1000
#define MONOTIME_MILLISECONDS_PER_TICK 1
#endif

/* convert millisecond value to something we can
 * directly compare MONOTIMEs against
*/
#define MONOTIME_VALUE(ms) ( \
    (ms) / MONOTIME_MILLISECONDS_PER_TICK )

/* maximum number of seconds between two times that we are able to
 * correctly determine the difference between as a MONOTIMEDIFF
*/
#define MONOTIME_UNIQUE_PERIOD (7FFFFFFFL / MONOTIME_TICKS_PER_SECOND)

/*
exported functions
*/

/* return the current monotonic time.
 * (RISC OS: centiseconds since the machine last executed a power-on reset)
 * (WINDOWS: milliseconds since Windows was started)
*/

#if defined(NORCROFT_INLINE_ASM) || defined(NORCROFT_INLINE_SWIX_NOT_YET)
//#include "swis.h"
#define OS_ReadMonotonicTime 0x00000042
#define _XOS(swi_no) ((swi_no) | (1U << 17))

_Check_return_
static inline MONOTIME
monotime(void)
{
    MONOTIME result;
    #if defined(NORCROFT_INLINE_SWIX_NOT_YET)
    (void) _swix(OS_ReadMonotonicTime, _OUT(0), &result);
    #elif defined(NORCROFT_INLINE_ASM)
    __asm {
        SVC     #_XOS(OS_ReadMonotonicTime), /*in*/ {}, /*out*/ {R0}, /*corrupted*/ {PSR};
        MOV     result, R0;
    }
    #endif
    return(result);
}
#else
_Check_return_
extern MONOTIME
monotime(void);
#endif

/* return the difference between the current monotonic time and a previous
 * value thereof.
*/

_Check_return_
static inline MONOTIMEDIFF
monotime_diff(
    _InVal_     MONOTIME oldtime)
{
    return(monotime() - (MONOTIMEDIFF) oldtime);
}

/* return the difference between the two monotonic time values.
*/

_Check_return_
static inline MONOTIMEDIFF
monotime_difftimes(
    _InVal_     MONOTIME a,
    _InVal_     MONOTIME b)
{
    return((MONOTIMEDIFF) a - (MONOTIMEDIFF) b);
}

#endif /* __monotime_h */

/* end of monotime.h */
