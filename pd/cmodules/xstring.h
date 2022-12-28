/* xstring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for string handling */

/* SKS May 1990 */

#ifndef __xstring_h
#define __xstring_h

#if RISCOS && CROSS_COMPILE
#define u8_from_int(i) ((U8) (i))
#else
#define u8_from_int(i) (i)
#endif

/* skip spaces on byte-oriented string (either U8 or UTF-8, but not TSTR) */

#define PtrSkipSpaces(__ptr_type, ptr__ref) /*void*/ \
    while(CH_SPACE == PtrGetByte(ptr__ref)) \
        PtrIncByte(__ptr_type, ptr__ref)

/* skip spaces on characted-oriented string (either U8 or TSTR, but not UTF-8) */

#if RISCOS && 0
#define StrSkipSpaces(ptr__ref) /*void*/ \
    do  { \
        do { /*EMPTY*/ } while(CH_SPACE == *((ptr__ref)++)); \
        --(ptr__ref); \
    } while(0)
#else
#define StrSkipSpaces(ptr__ref) /*void*/ \
    while(CH_SPACE == *(ptr__ref)) \
        ++(ptr__ref)
#endif

/*
exported functions
*/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
_bfind(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_bytecount_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit);

#define bfind(key, p_start, n_entries, entry_size, __base_type, p_proc_bsearch, p_hit) ( \
    (__base_type *) _bfind(key, p_start, n_entries, entry_size, p_proc_bsearch, p_hit) )

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
xlsearch(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_bytecount_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch);

#define lsearch(key, p_start, n_entries, entry_size, __base_type, p_proc_bsearch) ( \
    (__base_type *) xlsearch(key, p_start, n_entries, entry_size, p_proc_bsearch) )

extern void __cdecl /* declared as qsort replacement */
check_sorted(
    const void * base,
    size_t num,
    size_t width,
    int (__cdecl * p_proc_compare) (
        const void * a1,
        const void * a2));

_Check_return_
extern U32
fast_strtoul(
    _In_z_      PC_U8Z p_u8_in, /* NB NOT USTR */
    _OutRef_opt_ P_P_U8Z endptr);

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
C_stricmp(
    _In_z_      PC_U8Z a,
    _In_z_      PC_U8Z b);

extern int
C_strnicmp(
    _In_z_/*n*/ PC_U8Z a,
    _In_z_/*n*/ PC_U8Z b,
    _In_        U32 n);

#endif

/* 32-bit (rather than size_t) sized strlen() */
#define strlen32(s) /*U32*/ \
    ((U32) strlen(s))

#define strlen32p1(s) /*U32*/ \
    (1U /*CH_NULL*/ + strlen32(s))

/* these are optimised for ARM Norcroft C */

_Check_return_
static inline U32
_inl_strlen32(
    _In_z_      PC_U8Z s)
{
    U32 len = 0;
    for(;;)
    {
        if(0 == s[len])
           break;
        ++len;
    }
    return(len);
}

#if 1
_Check_return_
static inline U32
_inl_strlen32p1(
    _In_z_      PC_U8Z s)
{
    PC_U8Z p = s;
    while(*p++)
       /*EMPTY*/;
    return(p - s); /* length includes terminator */
}
#else
#define _inl_strlen32p1(s) \
    (1U /*CH_NULL*/ + _inl_strlen32(s))
#endif

/*
strcpy / _s() etc. replacements that ensure CH_NULL-termination
*/

extern void
xstrkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src);

extern void
xstrnkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

extern void
xstrkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src);

extern void
xstrnkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

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

#include "cmodules/xtstring.h"
#include "cmodules/xustring.h"

#endif /* __xstring_h */

/* end of xstring.h */
