/* xstring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Library module for string handling (eg MS extensions in RISC OS) */

/* SKS May 1990 */

#ifndef __xstring_h
#define __xstring_h

#if 0 /*def __lint*/
#define u8_from_int(i) ((U8) (i))
#else
#define u8_from_int(i) (i)
#endif

/*
exported functions
*/

extern U32
fast_strtoul(
    _In_z_      PC_USTR p_u8_in,
    _Out_opt_   P_USTR * endptr);

_Check_return_
_Ret_writes_bytes_maybenull_(size_b)
extern P_ANY
memstr32(
    _In_reads_bytes_(size_a) PC_ANY a,
    _InVal_     U32 size_a,
    _In_reads_bytes_(size_b) PC_ANY b,
    _InVal_     U32 size_b);

extern void
memswap32(
    _Inout_updates_bytes_(n_bytes) P_ANY p1,
    _Inout_updates_bytes_(n_bytes) P_ANY p2,
    _In_        U32 n_bytes);

extern P_USTR
stristr(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b);

extern int
strrncmp(
    _In_reads_(n) PC_U8 a,
    _In_reads_(n) PC_U8 b,
    _In_        U32 n);

static inline void
str_clr(
    _InoutRef_opt_ P_P_USTR aa)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
}

_Check_return_
extern STATUS
str_set(
    _InoutRef_  P_P_USTR aa,
    _In_opt_z_  PC_USTR b);

_Check_return_
extern STATUS
str_set_n(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b,
    _InVal_     U32 n);

extern void
str_swap(
    _InoutRef_  P_P_USTR aa,
    _InoutRef_  P_P_USTR bb);

#if RISCOS

extern int
_stricmp(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b);

extern int
_strnicmp(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b,
    size_t n);

#endif

/* 32-bit (rather than size_t) sized strlen() */
#define strlen32(s) \
    ((U32) strlen(s))

#define strlen32p1(s) \
    (1U /*NULLCH*/ + strlen32(s))

#define ustrlen32(tstr) \
    ((U32) strlen(tstr))

#define ustrlen32p1(s) \
    (1U /*NULLCH*/ + ustrlen32(s))

#define tstrlen32(tstr) \
    ((U32) tstrlen(tstr))

#define tstrlen32p1(s) \
    (1U /*NULLCH*/ + tstrlen32(s))

extern S32
stricmp_wild(
    _In_z_      PC_USTR ptr1,
    _In_z_      PC_USTR ptr2);

extern S32
stox(
    _In_        PC_U8 string,
    _OutRef_    P_S32 p_col);

extern S32
xtos_ubuf(
    _Out_writes_z_(elemof_buffer) P_USTR buffer /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case);

_Check_return_
extern U32
xustrlen32(
    _In_reads_z_(elemof_buffer) PC_USTR buffer,
    _InVal_     U32 elemof_buffer);

/*
strcpy / _s() etc. replacements that ensure NULLCH-termination
*/

#define TRUNCATE U32_MAX

extern void
safe_strkat(
    _Inout_updates_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_USTR src);

extern void
safe_strnkat(
    _Inout_updates_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_reads_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

extern void
safe_strkpy(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_USTR src);

extern void
safe_strnkpy(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_reads_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

#if USTR_IS_L1STR

#define safe_ustrkat  safe_strkat
#define safe_ustrnkat safe_strnkat
#define safe_ustrkpy  safe_strkpy
#define safe_ustrnkpy safe_strnkpy

#endif /* USTR_IS_L1STR */

#if TSTR_IS_L1STR

#define safe_tstrkat  safe_strkat
#define safe_tstrnkat safe_strnkat
#define safe_tstrkpy  safe_strkpy
#define safe_tstrnkpy safe_strnkpy

#endif /* TSTR_IS_L1STR */

_Check_return_
extern int __cdecl
xsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        ...);

_Check_return_
extern int __cdecl
xvsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        va_list args);

#if USTR_IS_L1STR

#define  xusnprintf  xsnprintf
#define xuvsnprintf xvsnprintf

#else /* USTR_IS_L1STR */

_Check_return_
extern int __cdecl
xusnprintf(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_USTR format,
    /**/        ...);

_Check_return_
extern int __cdecl
xuvsnprintf(
    _Out_writes_z_(dst_n) P_USTR,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_USTR format,
    /**/        va_list args);

#endif /* USTR_IS_L1STR */

#if TSTR_IS_L1STR

#define  xtsnprintf  xsnprintf
#define xtvsnprintf xvsnprintf

#else /* TSTR_IS_L1STR */

_Check_return_
extern int __cdecl
xtsnprintf(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

_Check_return_
extern int __cdecl
xtvsnprintf(
    _Out_writes_z_(dst_n) PTSTR,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args);

#endif /* TSTR_IS_L1STR */

#if TSTR_IS_L1STR

#define tstrlen(s)                  strlen(s)

#define tstrcmp(s1, s2)             strcmp(s1, s2)
#define tstrncmp(s1, s2, n)         strncmp(s1, s2, n)
#if 1
#define tstricmp(s1, s2)            l1_stricmp(s1, U32_MAX, s2, U32_MAX)
#define tstrnicmp(s1, s2, n)        l1_strnicmp(s1, s2, n)
#else
#define tstricmp(s1, s2)            _stricmp(s1, s2)
#define tstrnicmp(s1, s2, n)        _strnicmp(s1, s2, n)
#endif

#define tstrchr(s, c)               strchr(s, c)
#define tstrrchr(s, c)              strrchr(s, c)
#define tstrstr(s1, s2)             strstr(s1, s2)
#define tstrpbrk(s1, s2)            strpbrk(s1, s2)

#define tstrtol(p, pp, r)           strtol(p, pp, r)
#define tstrtoul(p, pp, r)          strtoul(p, pp, r)

#endif /* TSTR_IS_L1STR */

#if TSTR_IS_L1STR

#define _l1str_from_tstr(tstr)  ((PC_L1STR) (tstr))
#define _tstr_from_l1str(l1str) ((PCTSTR) (l1str))

#endif /* TSTR_IS_L1STR */

#if USTR_IS_L1STR

#define _l1str_from_ustr(ustr)  ((PC_L1STR) (ustr))
#define _ustr_from_l1str(l1str) ((PC_USTR) (l1str))

#endif /* USTR_IS_L1STR */

#if TSTR_IS_L1STR && USTR_IS_L1STR

#define _tstr_from_ustr(ustr) ((PCTSTR) (ustr))
#define _ustr_from_tstr(tstr) ((PC_USTR) (tstr))

#endif /* TSTR_IS_L1STR && USTR_IS_L1STR */

#endif /* __xstring_h */

/* end of xstring.h */
