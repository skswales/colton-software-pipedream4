/* xtstring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for TCHAR-based string handling */

/* SKS Nov 2006 */

#ifndef __xtstring_h
#define __xtstring_h

/*
exported functions
*/

#if 0 /* just for diff minimization */

_Check_return_
extern STATUS
al_tstr_append(
    _InoutRef_  P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr);

static inline void
al_tstr_clr(
    _InoutRef_ P_ARRAY_HANDLE_TSTR p_array_handle_tstr)
{
    al_array_dispose(p_array_handle_tstr);
}

_Check_return_
extern STATUS
al_tstr_set(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr);

static inline void
tstr_clr(
    _Inout_opt_ P_PTSTR aa)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
}

_Check_return_
extern STATUS
tstr_set(
    _OutRef_    P_PTSTR aa,
    _In_opt_z_  PCTSTR tstr);

_Check_return_
extern STATUS
tstr_set_n(
    _OutRef_    P_PTSTR aa,
    _In_reads_opt_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n);

_Check_return_
extern STATUS
tstr_set_from_ustr(
    _OutRef_    P_PTSTR aa,
    _In_opt_z_  PC_USTR ustr);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
tstr_compare_equals(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_n2(
    _In_/*tchars_n_a*/ PCTSTR tstr_a,
    _InVal_     U32 tchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*tchars_n_b*/ PCTSTR tstr_b,
    _InVal_     U32 tchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_nocase(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
tstr_compare_equals_nocase(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_n2_nocase(
    _In_/*tchars_n_a*/ PCTSTR tstr_a,
    _InVal_     U32 tchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*tchars_n_b*/ PCTSTR tstr_b,
    _InVal_     U32 tchars_n_b /*strlen_with,without_NULLCH*/);

#endif

/*
strcpy / _s() etc. replacements that ensure CH_NULL-termination
*/

#if TSTR_IS_SBSTR

#define tstr_xstrkat  xstrkat
#define tstr_xstrnkat xstrnkat
#define tstr_xstrkpy  xstrkpy
#define tstr_xstrnkpy xstrnkpy

#endif /* TSTR_IS_SBSTR */

#if TSTR_IS_SBSTR

#define  tstr_xsnprintf  xsnprintf
#define tstr_xvsnprintf xvsnprintf

#else /* TSTR_IS_SBSTR */

_Check_return_
extern int __cdecl
tstr_xsnprintf(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

_Check_return_
extern int __cdecl
tstr_xvsnprintf(
    _Out_writes_z_(dst_n) PTSTR,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args);

#endif /* TSTR_IS_SBSTR */

#if TSTR_IS_SBSTR

#define tstrlen(s)                  strlen(s)

#define tstrcmp(s1, s2)             strcmp(s1, s2)
#define tstrncmp(s1, s2, n)         strncmp(s1, s2, n)
#if 1
#define tstricmp(s1, s2)            sb_stricmp(s1, U32_MAX, s2, U32_MAX)
#define tstrnicmp(s1, s2, n)        sb_strnicmp(s1, s2, n)
#else
#define tstricmp(s1, s2)            C_stricmp(s1, s2)
#define tstrnicmp(s1, s2, n)        C_strnicmp(s1, s2, n)
#endif

#define tstrchr(s, c)               strchr(s, c)
#define tstrrchr(s, c)              strrchr(s, c)
#define tstrstr(s1, s2)             strstr(s1, s2)
#define tstrpbrk(s1, s2)            strpbrk(s1, s2)

#define tstrtol(p, pp, r)           strtol(p, pp, r)
#define tstrtoul(p, pp, r)          strtoul(p, pp, r)

#endif /* TSTR_IS_SBSTR */

/* 32-bit (rather than size_t) sized strlen() */
#define tstrlen32(tstr) \
    ((U32) tstrlen(tstr))

#define tstrlen32p1(s) \
    (1U /*CH_NULL*/ + tstrlen32(s))

#if TSTR_IS_SBSTR

#define _sbstr_from_tstr(tstr)  ((PC_SBSTR) (tstr)) /* no conversion is required */
#define _tstr_from_sbstr(sbstr) ((PCTSTR) (sbstr))

#else /* TSTR_IS_SBSTR */

_Check_return_
_Ret_z_ /* never NULL */
extern PC_SBSTR /*low-lifetime*/
_sbstr_from_tstr(
    _In_z_      PCTSTR tstr);

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_sbstr(
    _In_z_      PC_SBSTR sbstr);

#endif /* TSTR_IS_SBSTR */

#if TSTR_IS_SBSTR && USTR_IS_SBSTR

#define _tstr_from_ustr(ustr) ((PCTSTR) (ustr))
#define _ustr_from_tstr(tstr) ((PC_USTR) (tstr))

#endif /* TSTR_IS_SBSTR && USTR_IS_SBSTR */

#endif /* __xtstring_h */

/* end of xtstring.h */
