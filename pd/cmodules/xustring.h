/* xustring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for UCHAR-based string handling */

/* SKS Apr 2014 */

#ifndef __xustring_h
#define __xustring_h

/*
exported functions
*/

#if 0 /* just for diff minimization */

_Check_return_
extern STATUS
al_ustr_append(
    _InoutRef_  P_ARRAY_HANDLE_USTR p_array_handle_ustr,
    _In_z_      PC_USTR ustr);

static inline void
al_ustr_clr(
    _InoutRef_  P_ARRAY_HANDLE_USTR p_array_handle_ustr)
{
    al_array_dispose(p_array_handle_ustr);
}

_Check_return_
extern STATUS
al_ustr_set(
    _OutRef_    P_ARRAY_HANDLE_USTR p_array_handle_ustr,
    _In_z_      PC_USTR ustr);

static inline void
ustr_clr(
    _Inout_opt_ P_P_USTR aa)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
}

_Check_return_
extern STATUS
ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b);

_Check_return_
extern STATUS
ustr_set_n(
    _OutRef_    P_P_USTR aa,
    _In_reads_opt_(uchars_n) PC_UCHARS b,
    _InVal_     U32 uchars_n);

#endif

_Check_return_
extern U32
fast_ustrtoul(
    _In_z_      PC_USTR ustr_in,
    _OutRef_opt_ P_PC_USTR endptr);

_Check_return_
extern S32
stricmp_wild(
    _In_z_      PC_USTR ptr1,
    _In_z_      PC_USTR ptr2);

_Check_return_
extern S32
stox(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_S32 p_col);

/*ncr*/
extern U32 /*number of bytes in converted buffer*/
xtos_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case);

/*
strcpy / _s() etc. replacements that ensure CH_NULL-termination
*/

#if USTR_IS_SBSTR

#define ustr_xstrkat  xstrkat
#define ustr_xstrnkat xstrnkat
#define ustr_xstrkpy  xstrkpy
#define ustr_xstrnkpy xstrnkpy

#endif /* USTR_IS_SBSTR */

#define uchars_IncByte(uchars__ref) \
    PtrIncByte(PC_UCHARS, uchars__ref)

#define uchars_IncByte_wr(uchars_wr__ref) \
    PtrIncByte(P_UCHARS, uchars_wr__ref)

#define uchars_IncBytes(uchars__ref, add) \
    PtrIncBytes(PC_UCHARS, uchars__ref, add)

#define uchars_IncBytes_wr(uchars_wr__ref, add) \
    PtrIncBytes(P_UCHARS, uchars_wr__ref, add)

#define uchars_DecByte(uchars__ref) \
    PtrDecByte(PC_UCHARS, uchars__ref)

#define uchars_AddBytes(uchars, add) \
    PtrAddBytes(PC_UCHARS, uchars, add)

#define uchars_AddBytes_wr(uchars_wr, add) \
    PtrAddBytes(P_UCHARS, uchars_wr, add)

#define ustr_IncByte(ustr__ref) \
    PtrIncBytes(PC_USTR, ustr__ref, 1)

#define ustr_IncByte_wr(ustr_wr__ref) \
    PtrIncBytes(P_USTR, ustr_wr__ref, 1)

#define ustr_IncBytes(ustr__ref, add) \
    PtrIncBytes(PC_USTR, ustr__ref, add)

#define ustr_IncBytes_wr(ustr_wr__ref, add) \
    PtrIncBytes(P_USTR, ustr_wr__ref, add)

#define ustr_DecByte(ustr__ref) \
    PtrDecBytes(PC_USTR, ustr__ref, 1)

#define ustr_DecByte_wr(ustr__ref) \
    PtrDecBytes(P_USTR, ustr__ref, 1)

#define ustr_AddBytes(ustr, add) \
    PtrAddBytes(PC_USTR, ustr, add)

#define ustr_AddBytes_wr(ustr_wr, add) \
    PtrAddBytes(P_USTR, ustr_wr, add)

#define ustr_SkipSpaces(ustr__ref) \
    PtrSkipSpaces(PC_USTR, ustr__ref)

#if USTR_IS_SBSTR

#define  ustr_xsnprintf  xsnprintf
#define ustr_xvsnprintf xvsnprintf

/* 32-bit (rather than size_t) sized strlen() */
#define ustrlen32(tstr) \
    ((U32) strlen(tstr))

#define ustrlen32p1(s) \
    (1U /*CH_NULL*/ + ustrlen32(s))

#else /* USTR_IS_SBSTR */

_Check_return_
extern int __cdecl
ustr_xsnprintf(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_USTR format,
    /**/        ...);

_Check_return_
extern int __cdecl
ustr_xvsnprintf(
    _Out_writes_z_(dst_n) P_USTR,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_USTR format,
    /**/        va_list args);

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define _sbstr_from_ustr(ustr)  ((PC_SBSTR) (ustr))
#define _ustr_from_sbstr(sbstr) ((PC_USTR) (sbstr))

#endif /* USTR_IS_SBSTR */

_Check_return_
static inline F64
ustrtod(
    _In_z_      PC_USTR ustr_in,
    _OutRef_opt_ P_PC_USTR endptr)
{
    return((F64) strtod((const char *) ustr_in, (char **) endptr));
}

#endif /* __xustring_h */

/* end of xustring.h */
