/* typepack.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Library module for type packing and unpacking */

/* MRJC November 1989 */

#include "common/gflags.h"

#include "cmodules/typepack.h"

/******************************************************************************
*
* input
*
******************************************************************************/

/******************************************************************************
*
* read a double from unaligned memory (bytes are in 8087 format)
*
******************************************************************************/

_Check_return_
extern F64
readval_F64_from_8087(
    _In_bytecount_c_(sizeof(F64)) PC_ANY from)
{
    union
    {
        F64 f64;
        BYTE bytes[sizeof32(F64)];
    } u;

#if RISCOS

    /* 8087 has exponent word as 2nd word, ARM at 1st word */

    /* mantissa word in 1st */
    u.bytes[4] = PtrGetByteOff(from, 0);
    u.bytes[5] = PtrGetByteOff(from, 1);
    u.bytes[6] = PtrGetByteOff(from, 2);
    u.bytes[7] = PtrGetByteOff(from, 3);

    /* exponent word in 2nd */
    u.bytes[0] = PtrGetByteOff(from, 4);
    u.bytes[1] = PtrGetByteOff(from, 5);
    u.bytes[2] = PtrGetByteOff(from, 6);
    u.bytes[3] = PtrGetByteOff(from, 7);

#elif WINDOWS

    PREFAST_ONLY_ZERO_STRUCT(u); /* otherwise it winges */

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);
    u.bytes[2] = PtrGetByteOff(from, 2);
    u.bytes[3] = PtrGetByteOff(from, 3);

    u.bytes[4] = PtrGetByteOff(from, 4);
    u.bytes[5] = PtrGetByteOff(from, 5);
    u.bytes[6] = PtrGetByteOff(from, 6);
    u.bytes[7] = PtrGetByteOff(from, 7);

#else
#error unknown FP layout
#endif

    return(u.f64);
}

/******************************************************************************
*
* read a double from unaligned memory (bytes are in ARM format)
*
******************************************************************************/

_Check_return_
extern F64
readval_F64_from_ARM(
    _In_bytecount_c_(sizeof(F64)) PC_ANY from)
{
    union
    {
        F64 f64;
        BYTE bytes[sizeof32(F64)];
    } u;

#if RISCOS

    /* ARM can't do unaligned FP load */

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);
    u.bytes[2] = PtrGetByteOff(from, 2);
    u.bytes[3] = PtrGetByteOff(from, 3);

    u.bytes[4] = PtrGetByteOff(from, 4);
    u.bytes[5] = PtrGetByteOff(from, 5);
    u.bytes[6] = PtrGetByteOff(from, 6);
    u.bytes[7] = PtrGetByteOff(from, 7);

#elif WINDOWS

    PREFAST_ONLY_ZERO_STRUCT(u); /* otherwise it winges */

    /* 8087 has exponent word as 2nd word, ARM at 1st word */

    /* exponent word in 1st */
    u.bytes[4] = PtrGetByteOff(from, 0);
    u.bytes[5] = PtrGetByteOff(from, 1);
    u.bytes[6] = PtrGetByteOff(from, 2);
    u.bytes[7] = PtrGetByteOff(from, 3);

    /* mantissa word in 2nd */
    u.bytes[0] = PtrGetByteOff(from, 4);
    u.bytes[1] = PtrGetByteOff(from, 5);
    u.bytes[2] = PtrGetByteOff(from, 6);
    u.bytes[3] = PtrGetByteOff(from, 7);

#else
#error unknown FP layout
#endif

    return(u.f64);
}

/******************************************************************************
*
* read an unsigned 16-bit value from unaligned memory
*
******************************************************************************/

#ifndef readval_U16

_Check_return_
extern U16
readval_U16(
    _In_bytecount_c_(sizeof(U16)) PC_ANY from)
{
    union
    {
        U16 u16;
        BYTE bytes[sizeof32(U16)];
    } u;

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);

    return(u.u16);
}

#endif /* readval_U16 */

/******************************************************************************
*
* read an unsigned 32-bit value from unaligned memory
*
******************************************************************************/

#ifndef readval_U32

_Check_return_
extern U32
readval_U32(
    _In_bytecount_c_(sizeof(U32)) PC_ANY from)
{
    union
    {
        U32 u32;
        BYTE bytes[sizeof32(U32)];
    } u;

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);
    u.bytes[2] = PtrGetByteOff(from, 2);
    u.bytes[3] = PtrGetByteOff(from, 3);

    return(u.u32);
}

#endif /* readval_U32 */

/******************************************************************************
*
* read a signed 16-bit value from unaligned memory
*
******************************************************************************/

#ifndef readval_S16

_Check_return_
extern S16
readval_S16(
    _In_bytecount_c_(sizeof(S16)) PC_ANY from)
{
    union
    {
        S16 s16;
        BYTE bytes[sizeof32(S16)];
    } u;

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);

    return(u.s16);
}

#endif /* readval_S16 */

/******************************************************************************
*
* read a signed 32-bit value from unaligned memory
*
******************************************************************************/

#ifndef readval_S32

_Check_return_
extern S32
readval_S32(
    _In_bytecount_c_(sizeof(S32)) PC_ANY from)
{
    union
    {
        S32 s32;
        BYTE bytes[sizeof32(S32)];
    } u;

    u.bytes[0] = PtrGetByteOff(from, 0);
    u.bytes[1] = PtrGetByteOff(from, 1);
    u.bytes[2] = PtrGetByteOff(from, 2);
    u.bytes[3] = PtrGetByteOff(from, 3);

    return(u.s32);
}

#endif /* readval_S32 */

/******************************************************************************
*
* output
*
******************************************************************************/

/******************************************************************************
*
* output double as 8087 format
*
******************************************************************************/

extern void
writeval_F64_as_8087(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64)
{
    union
    {
        F64 f64;
        BYTE bytes[sizeof32(F64)];
    } u;

    u.f64 = f64;

#if RISCOS

    /* ARM placed exp word 1st, 8087 requires exp word 2nd */

    /* mantissa word out 1st */
    PtrPutByteOff(to, 0, u.bytes[4]);
    PtrPutByteOff(to, 1, u.bytes[5]);
    PtrPutByteOff(to, 2, u.bytes[6]);
    PtrPutByteOff(to, 3, u.bytes[7]);

    /* exponent word out 2nd */
    PtrPutByteOff(to, 4, u.bytes[0]);
    PtrPutByteOff(to, 5, u.bytes[1]);
    PtrPutByteOff(to, 6, u.bytes[2]);
    PtrPutByteOff(to, 7, u.bytes[3]);

#elif WINDOWS

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
    PtrPutByteOff(to, 2, u.bytes[2]);
    PtrPutByteOff(to, 3, u.bytes[3]);

    PtrPutByteOff(to, 4, u.bytes[4]);
    PtrPutByteOff(to, 5, u.bytes[5]);
    PtrPutByteOff(to, 6, u.bytes[6]);
    PtrPutByteOff(to, 7, u.bytes[7]);

#else
#error unknown FP layout
#endif
}

/******************************************************************************
*
* output double as ARM format
*
******************************************************************************/

extern void
writeval_F64_as_ARM(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64)
{
    union
    {
        F64 f64;
        BYTE bytes[sizeof32(F64)];
    } u;

    u.f64 = f64;

#if RISCOS

    /* ARM can't do unaligned FP store */

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
    PtrPutByteOff(to, 2, u.bytes[2]);
    PtrPutByteOff(to, 3, u.bytes[3]);

    PtrPutByteOff(to, 4, u.bytes[4]);
    PtrPutByteOff(to, 5, u.bytes[5]);
    PtrPutByteOff(to, 6, u.bytes[6]);
    PtrPutByteOff(to, 7, u.bytes[7]);

#elif WINDOWS

    /* 8087 placed exp word 2nd, ARM requires exp word 1st */

    /* exponent word out 1st */
    PtrPutByteOff(to, 0, u.bytes[4]);
    PtrPutByteOff(to, 1, u.bytes[5]);
    PtrPutByteOff(to, 2, u.bytes[6]);
    PtrPutByteOff(to, 3, u.bytes[7]);

    /* mantissa word out 2nd */
    PtrPutByteOff(to, 4, u.bytes[0]);
    PtrPutByteOff(to, 5, u.bytes[1]);
    PtrPutByteOff(to, 6, u.bytes[2]);
    PtrPutByteOff(to, 7, u.bytes[3]);

#else
#error unknown FP layout
#endif
}

/******************************************************************************
*
* output unsigned 16-bit value
*
******************************************************************************/

#ifndef writeval_U16

extern void
writeval_U16(
    _Out_bytecapcount_x_(sizeof(U16)) P_ANY to,
    _InVal_     U16 u16)
{
    union
    {
        U16 u16;
        BYTE bytes[sizeof32(U16)];
    } u;

    u.u16 = u16;

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
}

#endif /* writeval_U16 */

/******************************************************************************
*
* output unsigned 32-bit value
*
******************************************************************************/

#ifndef writeval_U32

extern void
writeval_U32(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32)
{
    union
    {
        U32 u32;
        BYTE bytes[sizeof32(U32)];
    } u;

    u.u32 = u32;

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
    PtrPutByteOff(to, 2, u.bytes[2]);
    PtrPutByteOff(to, 3, u.bytes[3]);
}

#endif /* writeval_U32 */

/******************************************************************************
*
* output signed 16-bit value
*
******************************************************************************/

#ifndef writeval_S16

extern void
writeval_S16(
    _Out_bytecapcount_x_(sizeof(S16)) P_ANY to,
    _InVal_     S16 s16)
{
    union
    {
        S16 s16;
        BYTE bytes[sizeof32(S16)];
    } u;

    u.s16 = s16;

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
}

#endif /* writeval_S16 */

/******************************************************************************
*
* output signed 32-bit value
*
******************************************************************************/

#ifndef writeval_S32

extern void
writeval_S32(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32)
{
    union
    {
        S32 s32;
        BYTE bytes[sizeof32(S32)];
    } u;

    u.s32 = s32;

    PtrPutByteOff(to, 0, u.bytes[0]);
    PtrPutByteOff(to, 1, u.bytes[1]);
    PtrPutByteOff(to, 2, u.bytes[2]);
    PtrPutByteOff(to, 3, u.bytes[3]);
}

#endif /* writeval_S32 */

/* end of typepack.c */
