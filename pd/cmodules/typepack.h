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
readval_F64(
    _In_bytecount_c_(sizeof(F64)) PC_BYTE from);

_Check_return_
extern F64
readval_F64_from_8087(
    _In_bytecount_c_(sizeof(F64)) PC_BYTE from);

_Check_return_
extern F64
readval_F64_from_ARM(
    _In_bytecount_c_(sizeof(F64)) PC_BYTE from);

_Check_return_
extern U32
readuword(
    _In_reads_bytes_(size) PC_BYTE from,
    S32 size);

_Check_return_
extern U16
readval_U16(
    _In_bytecount_c_(sizeof(U16)) PC_BYTE from);

_Check_return_
extern U32
readval_U32(
    _In_bytecount_c_(sizeof(U32)) PC_BYTE from);

_Check_return_
extern S16
readval_S16(
    _In_bytecount_c_(sizeof(S16)) PC_BYTE from);

_Check_return_
extern S32
readval_S32(
    _In_bytecount_c_(sizeof(S32)) PC_BYTE from);

/*
writing to memory
*/

extern S32
writeval_F64(
    _Out_bytecapcount_x_(sizeof(F64)) P_BYTE to,
    _In_        F64 f64);

extern S32
writeval_F64_as_8087(
    _Out_bytecapcount_x_(sizeof(F64)) P_BYTE to,
    _In_        F64 f64);

extern S32
writeval_F64_as_ARM(
    _Out_bytecapcount_x_(sizeof(F64)) P_BYTE to,
    _In_        F64 f64);

extern S32
writeuword(
    _Out_writes_bytes_(size) P_BYTE to,
    U32 word,
    S32 size);

extern S32
writeval_U16(
    _Out_bytecapcount_x_(sizeof(U16)) P_BYTE to,
    _InVal_     U16 u16);

extern S32
writeval_U32(
    _Out_bytecapcount_x_(sizeof(U32)) P_BYTE to,
    _In_        U32 u32);

extern S32
writeval_S16(
    _Out_bytecapcount_x_(sizeof(S16)) P_BYTE to,
    _InVal_     S16 s16);

extern S32
writeval_S32(
    _Out_bytecapcount_x_(sizeof(S32)) P_BYTE to,
    _In_        S32 s32);

#endif /* TYPEPACK_EXPORT_ONLY_MACROS */

#ifndef TYPEPACK_EXPORT_ONLY_FUNCTIONS

/*
functions as macros
*/

/*
reading from memory
*/

#if WINDOWS

/*
x86 can do transparent unaligned access, albeit more slowly
*/

#define readval_U16(from) (* (PC_U16) (from))

#define readval_U32(from) (* (PC_U32) (from))

#define readval_S16(from) (* (PC_S16) (from))

#define readval_S32(from) (* (PC_S32) (from))

#endif /* WINDOWS */

#if RISCOS

/*
NB. Norcroft C currently has ability to do compile-time checks
*/

/* Norcroft C short loading is poor; relies on current memory access and word/halfword alignment */
#define readval_U16(from) ( \
    ___assert((___typeof(from) & 0x10000000) == 0x10000000, "Illegal type used in readval_U16"), \
    (U16) \
    ((* (((unsigned char *) (from)) + 0)) <<  0) | \
    ((* (((unsigned char *) (from)) + 1)) <<  8)   )

#define readval_U32(from) ( \
    ___assert((___typeof(from) & 0x10000000) == 0x10000000, "Illegal type used in readval_U32"), \
    (U32) \
    ((U32) (* (((unsigned char *) (from)) + 0)) <<  0) | \
    ((U32) (* (((unsigned char *) (from)) + 1)) <<  8) | \
    ((U32) (* (((unsigned char *) (from)) + 2)) << 16) | \
    ((U32) (* (((unsigned char *) (from)) + 3)) << 24)   )

/* sign-extension required */
#define readval_S16(from) ( \
    ___assert((___typeof(from) & 0x10000000) == 0x10000000, "Illegal type used in readval_S16"), \
    (S16) \
    (       (* (((unsigned char *) (from)) + 0)) <<  0)        | \
    (((S32) (* (((unsigned char *) (from)) + 1)) << 24) >> 16)   )

#define readval_S32(from) ( \
    ___assert((___typeof(from) & 0x10000000) == 0x10000000, "Illegal type used in readval_S32"), \
    (S32) \
    ((U32) (* (((unsigned char *) (from)) + 0)) <<  0) | \
    ((U32) (* (((unsigned char *) (from)) + 1)) <<  8) | \
    ((U32) (* (((unsigned char *) (from)) + 2)) << 16) | \
    ((U32) (* (((unsigned char *) (from)) + 3)) << 24)   )

#endif /* RISCOS */

/*
writing to memory
*/

/* used below for things that work ok by simple pointer op */
#define __writeval_generic(to, val, type) ( \
    * (type *) (to) = (val), \
    sizeof(type) )

#if WINDOWS

#define writeval_U16(to, word) __writeval_generic(to, word, U16)

#define writeval_U32(to, word) __writeval_generic(to, word, U32)

#define writeval_S16(to, word) __writeval_generic(to, word, S16)

#define writeval_S32(to, word) __writeval_generic(to, word, S32)

#endif /* WINDOWS */

#if RISCOS

#if TRUE
/* Norcroft C can now write unaligned shorts ok on ARM using STRB,SHR,STRB */
#define writeval_U16(to, word) __writeval_generic(to, word, U16)
#define writeval_S16(to, word) __writeval_generic(to, word, S16)
#else
/* cast to prevent sign extension */
typedef struct ___writeword16str
{
    unsigned char b0;
    unsigned char b1;
}
__writeword16str;

#define __writeval_16(to, word, type) ( \
    ___assert((___typeof(to) & 0x10000000) == 0x10000000, "Illegal type used in __writeval_16"), \
    ((__writeword16str *) (to))->b0 = (unsigned char) (((U16) (word)) >>  0), \
    ((__writeword16str *) (to))->b1 = (unsigned char) (((U16) (word)) >>  8), \
    sizeof(type) )

#define writeval_U16(to, word) __writeval_16(to, word, U16)
#define writeval_S16(to, word) __writeval_16(to, word, S16)
#endif

#if FALSE
/* Norcroft C can't yet be persuaded to write unaligned longs on ARM */
#define writeval_U32(to, word) __writeval_generic(to, word, U32)
#define writeval_S32(to, word) __writeval_generic(to, word, S32)
#else
/* cast to prevent sign extension */
typedef struct ___writeword32str
{
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;
}
__writeword32str;

#define __writeval_32(to, word, type) ( \
    ___assert((___typeof(to) & 0x10000000) == 0x10000000, "Illegal type used in __writeval_32"), \
    ((__writeword32str *) (to))->b0 = (unsigned char) (((U32) (word)) >>  0), \
    ((__writeword32str *) (to))->b1 = (unsigned char) (((U32) (word)) >>  8), \
    ((__writeword32str *) (to))->b2 = (unsigned char) (((U32) (word)) >> 16), \
    ((__writeword32str *) (to))->b3 = (unsigned char) (((U32) (word)) >> 24), \
    sizeof(type) )

#define writeval_U32(to, word) __writeval_32(to, word, U32)
#define writeval_S32(to, word) __writeval_32(to, word, S32)
#endif

#endif /* RISCOS */

#endif /* TYPEPACK_EXPORT_ONLY_FUNCTIONS */

#endif /* __typepack_h */

/* end of typepack.h */
