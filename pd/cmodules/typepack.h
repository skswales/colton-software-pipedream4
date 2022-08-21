/* typepack.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* External header file for module typepack.c */

/* MRJC November 1989 */

#ifndef __typepack_h
#define __typepack_h

#ifdef  TYPEPACK_EXPORT_ONLY_FUNCTIONS
#undef  TYPEPACK_EXPORT_ONLY_MACROS
#endif

#ifndef TYPEPACK_EXPORT_ONLY_MACROS

/*
exported functions
*/

/*
reading from memory
*/

_Check_return_
extern F64
readval_F64_from_8087(
    _In_bytecount_c_(sizeof(F64)) PC_ANY from);

_Check_return_
extern F64
readval_F64_from_ARM(
    _In_bytecount_c_(sizeof(F64)) PC_ANY from);

_Check_return_
static inline U32
readuword_LE(
    _In_reads_bytes_(size) PC_ANY from,
    _InVal_     S32 size)
{
    U32 res = 0;
    S32 i = size;

    while(--i >= 0)
    {
        res <<= 8;
        res |= PtrGetByteOff(from, i);
    }

    return(res);
}

_Check_return_
extern U16
readval_U16(
    _In_bytecount_c_(sizeof(U16)) PC_ANY from);

_Check_return_
extern U32
readval_U32(
    _In_bytecount_c_(sizeof(U32)) PC_ANY from);

_Check_return_
extern S16
readval_S16(
    _In_bytecount_c_(sizeof(S16)) PC_ANY from);

_Check_return_
extern S32
readval_S32(
    _In_bytecount_c_(sizeof(S32)) PC_ANY from);

/*
writing to memory
*/

extern void
writeval_F64_as_8087(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64);

extern void
writeval_F64_as_ARM(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64);

static inline S32
writeuword_LE(
    _Out_writes_bytes_(size) P_ANY to,
    _In_        U32 word,
    _InVal_     S32 size)
{
    S32 i;

    for(i = 0; i < size; ++i)
    {
        PtrPutByteOff(to, i, (U8) (word & 0xFF));
        word >>= 8;
    }

    return(size);
}

extern void
writeval_U16(
    _Out_bytecapcount_x_(sizeof(U16)) P_ANY to,
    _InVal_     U16 u16);

extern void
writeval_U32(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32);

extern void
writeval_S16(
    _Out_bytecapcount_x_(sizeof(S16)) P_ANY to,
    _InVal_     S16 s16);

extern void
writeval_S32(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32);

#endif /* TYPEPACK_EXPORT_ONLY_MACROS */

#ifndef TYPEPACK_EXPORT_ONLY_FUNCTIONS

/*
functions as macros
*/

/*
reading from memory - assume unaligned access works, even if slower
*/

#define readval_U16(from) (* (PC_U16) (from))
#define readval_U32(from) (* (PC_U32) (from))
#define readval_S16(from) (* (PC_S16) (from))
#define readval_S32(from) (* (PC_S32) (from))

#if RISCOS

/* Norcroft C short loading is poor; relies on current memory access and word/halfword alignment */
#undef  readval_U16
#define readval_U16(from) __inline_readval_U16(from)
static inline U16
__inline_readval_U16(
    _In_bytecount_c_(2) PC_ANY from)
{
    return(
        ((U16) (* (((PC_U8) (from)) + 0)) <<  0) |
        ((U16) (* (((PC_U8) (from)) + 1)) <<  8) );
}

#undef  readval_U32
#define readval_U32(from) ( (U32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) <<  0) | \
    ((U32) (* (((PC_U8) (from)) + 1)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 2)) << 16) | \
    ((U32) (* (((PC_U8) (from)) + 3)) << 24)   ) )

/* sign-extension required */
#undef  readval_S16
#define readval_S16(from) ( (S16) ( \
    ( (S32) (* (((PC_U8) (from)) + 0)) <<  0)        | \
    (((S32) (* (((PC_U8) (from)) + 1)) << 24) >> 16)   ) )

#undef  readval_S32
#define readval_S32(from) ( (S32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) <<  0) | \
    ((U32) (* (((PC_U8) (from)) + 1)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 2)) << 16) | \
    ((U32) (* (((PC_U8) (from)) + 3)) << 24)   ) )

#endif /* RISCOS */

/*
writing to memory - assume unaligned access works, even if slower
*/

/* used below for things that work ok by simple pointer op */
#define __writeval_generic(to, val, type) ( \
    * (type *) (to) = (val) )

#define writeval_U16(to, word) __writeval_generic(to, word, U16)
#define writeval_U32(to, word) __writeval_generic(to, word, U32)
#define writeval_S16(to, word) __writeval_generic(to, word, S16)
#define writeval_S32(to, word) __writeval_generic(to, word, S32)

#if RISCOS

/* Norcroft C writes unaligned shorts OK on ARM using STRB,SHR,STRB */

/* Norcroft C can't yet be persuaded to write unaligned longs on ARM */
#undef  writeval_U32
#define writeval_U32(to, word) (void) writeuword_LE(to, word, sizeof32(U32))

#undef  writeval_S32
#define writeval_S32(to, word) (void) writeuword_LE(to, word, sizeof32(S32))

#endif /* RISCOS */

#endif /* TYPEPACK_EXPORT_ONLY_FUNCTIONS */

#endif /* __typepack_h */

/* end of typepack.h */
