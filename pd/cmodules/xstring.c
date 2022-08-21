/* xstring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for string handling */

/* SKS May 1990 */

#include "common/gflags.h"

#include "xstring.h"

/******************************************************************************
*
* slower way to clear memory, but it saves space when speed is not important
*
******************************************************************************/

extern void
memclr32(
    _Out_writes_bytes_(n_bytes) P_ANY ptr,
    _InVal_     U32 n_bytes)
{
    (void) memset(ptr, 0, n_bytes);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* like bsearch but always return a pointer even if not perfect match
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
_bfind(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit)
{
    PC_ANY p_any = p_start;

    *p_hit = FALSE;

    if(n_entries)
    {
        S32 e = n_entries;
        S32 s = 0;
        int res = 0;

        for(;;)
        {
            S32 n = (e - s) / 2;
            S32 t = s + n;

            p_any = PtrAddBytes(PC_ANY, p_start, t * entry_size);

            if((res = p_proc_bsearch(key, p_any)) == 0)
            {
                *p_hit = TRUE;
                return(de_const_cast(P_ANY, p_any));
            }

            if(res > 0)
                s = t;
            else
                e = t;

            if(!n)
                break;
        }

        /* always return pointer to element to insert before */
        if(res > 0)
            p_any = PtrAddBytes(PC_ANY, p_any, entry_size);
    }

    return(de_const_cast(P_ANY, p_any));
}

/******************************************************************************
*
* like bsearch but always return a pointer even if not perfect match
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
xlsearch(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch)
{
    int res = 0;
    S32 t;

    for(t = 0; t < n_entries; ++t)
    {
        PC_ANY p_any = PtrAddBytes(PC_ANY, p_start, t * entry_size);

        if((res = p_proc_bsearch(key, p_any)) == 0)
            return(de_const_cast(P_ANY, p_any));
    }

    return(NULL);
}

extern void __cdecl /* declared as qsort replacement */
check_sorted(
    const void * base,
    size_t num,
    size_t width,
    int (__cdecl * p_proc_compare) (
        const void * a1,
        const void * a2))
{
    const char * p = (const char *) base;
    size_t i;
    assert((S32)num > 0);
    if(num > 1)
        for(i = 0; i < num - 1; ++i, p += width)
        {
            int res = (* p_proc_compare) (p, p + width);

            if(res == 0)
            {
                messagef(TEXT("check_sorted: table ") PTR_XTFMT TEXT(" has identical members at index %d, %d"), base, i, i + 1);
                break;
            }
            else if(res > 0)
            {
                messagef(TEXT("check_sorted: table ") PTR_XTFMT TEXT(" members compare incorrectly at index %d, %d"), base, i, i + 1);
                break;
            }
        }
}

#endif

/* strtol, strtoul without whitespace stripping and base rubbish */

#if 0 /* just for diff minimization */

_Check_return_
extern S32
fast_strtol(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_opt_ P_P_U8Z endptr)
{
    PC_U8Z p_u8 = p_u8_in;
    BOOL negative = FALSE;
    P_U8Z this_endptr;
    U32 u32;

    switch(*p_u8)
    {
    case CH_PLUS_SIGN:  ++p_u8;                  break;
    case CH_MINUS_SIGN__BASIC: ++p_u8; negative = TRUE; break;
    default:                                     break;
    }

    u32 = fast_strtoul(p_u8, &this_endptr);

    if(endptr)
        *endptr = ((PC_U8Z) this_endptr != p_u8) ? this_endptr : (P_U8Z) p_u8_in;

    if(u32 > S32_MAX)
    {
        errno = ERANGE;
        u32 = S32_MAX;
    }

    if(negative)
        return(- (S32) u32);

    return(+ (S32) u32);
}

#endif

_Check_return_
extern U32
fast_strtoul(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_opt_ P_P_U8Z endptr)
{
    PC_U8Z p_u8 = p_u8_in;
    int c = *p_u8++;
    BOOL ok = FALSE;
    BOOL overflowed = FALSE;
    U32 hi = 0;
    U32 lo = 0;

    profile_ensure_frame();

    for(;;)
    {
        int digit;
        if(!isdigit(c))
            break;
        digit = c - CH_DIGIT_ZERO;
        lo *= 10;
        lo += digit;
        hi *= 10;
        hi += (lo >> 16);
        lo &= 0x0000FFFFU;
        if(hi >= 0x000010000U)
            overflowed = TRUE;
        ok = TRUE;
        c = *p_u8++;
    }

    if(endptr)
        *endptr = ok ? (P_U8Z) (p_u8 - 1) : (P_U8Z) p_u8_in;

    if(overflowed)
    {
        errno = ERANGE;
        return(U32_MAX);
    }

    return((hi << 16) | lo);
}

/******************************************************************************
*
* find first occurence of b in a
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(size_b)
extern P_ANY
memstr32(
    _In_reads_bytes_(size_a) PC_ANY a,
    _InVal_     U32 size_a,
    _In_reads_bytes_(size_b) PC_ANY b,
    _InVal_     U32 size_b)
{
    if(size_a >= size_b)
    {
        U32 n = size_a - size_b;
        U32 i;

        for(i = 0; i <= n; ++i) /* always at least one comparison to do */
        {
            PC_BYTE t = ((PC_BYTE) a) + i;

            if(0 == memcmp(t, b, (size_t) size_b))
                return((P_ANY) de_const_cast(P_BYTE, t));
        }
    }

    return(NULL);
}

#if RISCOS
#define SWAPTYPE U32
#elif WINDOWS
#define SWAPTYPE U32
#endif

#ifdef SWAPTYPE
typedef SWAPTYPE * P_SWAPTYPE;
#endif

extern void
memswap32(
    _Inout_updates_bytes_(n_bytes) P_ANY p1,
    _Inout_updates_bytes_(n_bytes) P_ANY p2,
    _In_        U32 n_bytes)
{
    P_U8 c1;
    P_U8 c2;
    U8 c;
    U32 i;

    profile_ensure_frame();

#ifdef SWAPTYPE
    /* copy aligned words at a time if possible */
    if( (((SWAPTYPE) p1 & (sizeof(SWAPTYPE)-1)) == 0) &&
        (((SWAPTYPE) p2 & (sizeof(SWAPTYPE)-1)) == 0) )
    {
        P_SWAPTYPE s1, s2;
        SWAPTYPE s;

        s1 = p1;
        s2 = p2;

        while(n_bytes > sizeof32(s))
        {
            n_bytes -= sizeof32(s);
             s    = *s1;
            *s1++ = *s2;
            *s2++ =  s;
        }

        c1 = (P_U8) s1;
        c2 = (P_U8) s2;
    }
    else
    {
        c1 = p1;
        c2 = p2;
    }
#endif

    for(i = 0; i < n_bytes; ++i)
    {
         c    = *c1;
        *c1++ = *c2;
        *c2++ =  c;
    }
}

#undef SWAPTYPE

#if 0 /* just for diff minimization */

/*
reverse an array
*/

extern void
memrev32(
    _Inout_updates_bytes_x_(n_elements * element_width) P_ANY p,
    _InVal_     U32 n_elements,
    _InVal_     U32 element_width)
{
    P_BYTE a = (P_BYTE) p;
    P_BYTE b = a + (n_elements * element_width); /* point beyond the last element */
    const U32 half_n_elements = n_elements / 2; /* only need to go halfway */
    U32 i;

    for(i = 0; i < half_n_elements; ++i)
    {
        b -= element_width;
        memswap32(a, b, element_width);
        a += element_width;
    }
}

#endif

#if !WINDOWS

#include <ctype.h> /* for "C"isalpha and friends */

/******************************************************************************
*
* case insensitive lexical comparison of two strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

extern int
C_stricmp(
    _In_z_      PC_U8Z a,
    _In_z_      PC_U8Z b)
{
    int res;

    profile_ensure_frame();

    for(;;)
    {
        int c_a = *a++;
        int c_b = *b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = /*"C"*/tolower(c_a); /* ASCII, no remapping */
            c_b = /*"C"*/tolower(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator -> equal */
    }
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

extern int
C_strnicmp(
    _In_z_/*n*/ PC_U8Z a,
    _In_z_/*n*/ PC_U8Z b,
    _In_        U32 n)
{
    int res;

    profile_ensure_frame();

    while(n-- > 0)
    {
        int c_a = *a++;
        int c_b = *b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = /*"C"*/tolower(c_a); /* ASCII, no remapping */
            c_b = /*"C"*/tolower(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    return(0); /* span of n leading chars is equal */
}

#endif /* RISCOS */

#if 0 /* just for diff minimization */

/******************************************************************************
*
* case sensitive (binary) comparison of leading chars of two SBCHAR strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(sbstr_compare_n2(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(0 == sbstr_compare_n2(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/)
{
   int res;
   U32 limit = MIN(sbchars_n_a, sbchars_n_b);
   U32 i;
   U32 remain_a, remain_b;

   profile_ensure_frame();

   /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
   for(i = 0; i < limit; ++i)
   {
       int c_a = *sbstr_a++;
       int c_b = *sbstr_b++;

       res = c_a - c_b;

       if(0 != res)
           return(res);

       if(CH_NULL == c_a)
           return(0); /* ended together at the terminator (before limit) -> equal */
   }

   /* matched up to the comparison limit */

   /* which string has the greater number of chars left over? */
   remain_a = sbchars_n_a - limit;
   remain_b = sbchars_n_b - limit;

   if(remain_a == remain_b)
       return(0); /* ended together at the specified finite lengths -> equal */

   /* sort out any string length residuals */
   assert((sbchars_n_a != strlen_with_NULLCH) || (sbchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

   if(sbchars_n_a >= strlen_without_NULLCH)
       remain_a = strlen32_n(sbstr_a, sbchars_n_a);

   if(sbchars_n_b >= strlen_without_NULLCH)
       remain_a = strlen32_n(sbstr_b, sbchars_n_b);

   /*if(remain_a == remain_b)
       return(0);*/ /* ended together at the determined finite lengths -> equal */

   res = (int) remain_a - (int) remain_b;

   return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two SBCHAR (U8 Latin-N) strings
*
* uses sbchar_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(sbstr_compare_n2_nocase(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(0 == sbstr_compare_n2_nocase(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2_nocase(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(sbchars_n_a, sbchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        int c_a = *sbstr_a++;
        int c_b = *sbstr_b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = sbchar_sortbyte(c_a);
            c_b = sbchar_sortbyte(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = sbchars_n_a - limit;
    remain_b = sbchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((sbchars_n_a != strlen_with_NULLCH) || (sbchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(sbchars_n_a >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_a, sbchars_n_a);

    if(sbchars_n_b >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_b, sbchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

#endif

/*
portable string copy functions that ensure CH_NULL termination without buffer overflow

strcpy(), strncat() etc. and even their _s() variants are all a bit 'wonky'

copying to dst buffer is limited by dst_n characters

dst buffer is always then CH_NULL-terminated within dst_n characters limit
*/

/*
append up characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

extern void
xstrkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src)
{
    U32 dst_idx = 0;
    U8 ch;

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = CH_NULL;
            return;
        }

        ch = dst[dst_idx];

        /* append here? */
        if(CH_NULL == ch)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if(1 >= (dst_n - dst_idx))
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        return;
    }

    /* ensure terminated */
    dst[dst_idx] = CH_NULL;
}

/*
append up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

extern void
xstrnkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    U8 ch;

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = CH_NULL;
            return;
        }

        ch = dst[dst_idx];

        /* append here? */
        if(CH_NULL == ch)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    while(src_idx < src_n)
    {
        ch = src[src_idx++];

        /* finished source string before reaching source limit? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if(1 >= (dst_n - dst_idx))
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        return;
    }

    /* ensure terminated */
    dst[dst_idx] = CH_NULL;
}

/*
copy characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

extern void
xstrkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src)
{
    U32 dst_idx = 0;
    U8 ch;

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if(1 >= (dst_n - dst_idx))
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        return;
    }

    /* ensure terminated */
    dst[dst_idx] = CH_NULL;
}

/*
copy up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

extern void
xstrnkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    U8 ch;

    /* copy src to this point */
    while(src_idx < src_n)
    {
        ch = src[src_idx++];

        /* finished source string before reaching source limit? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if(1 >= (dst_n - dst_idx))
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        return;
    }

    /* ensure terminated */
    dst[dst_idx] = CH_NULL;
}

/*
a portable but inexact replacement for (v)snprintf(), which Microsoft CRT doesn't have... also ensures CH_NULL termination
*/

_Check_return_
extern int __cdecl
xsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        ...)
{
    va_list args;
    int ret;

#if RISCOS && 1 /* C99 CRT */

    if(0 == dst_n)
        return(0);

    va_start(args, format);
    ret = vsnprintf(dst, dst_n, format, args);
    va_end(args);

    if(ret < 0)
    {
        ret = 0;
        dst[0] = CH_NULL; /* ensure terminated */
    }
    else if((U32) ret >= dst_n) /* limit to what actually was achieved */
    {
        ret = strlen32(dst);
    }

#else

#error See Fireworkz if needed

#endif /* OS */

    return(ret);

}

/******************************************************************************
*
* do a string assignment
* if the length of the new string is 0, set to 0
* if cannot allocate new string, return -1
* up to caller to distinguish these cases
*
******************************************************************************/

_Check_return_
extern STATUS
str_set(
    _InoutRef_  P_P_USTR aa,
    _In_opt_z_  PC_USTR b)
{
    STATUS status;
    P_USTR a = *aa;
    U32 l = b ? strlen32(b) : 0;

    if(NULL != a)
    {
        /* some of variable already set to this? */
        if((0 != l) && (0 == strncmp(a, b, l)))
        {
            /* terminate at new offset */
            a[l] = CH_NULL;
            return(STATUS_DONE);
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
    }

    if(0 == l)
        return(STATUS_OK);

    if(NULL != (*aa = a = _al_ptr_alloc(l + 1/*CH_NULL*/, &status)))
    {
        memcpy32(a, b, l + 1/*CH_NULL*/);

        status = STATUS_DONE;
    }

    return(status);
}

_Check_return_
extern STATUS
str_set_n(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b,
    _InVal_     U32 n)

{
    STATUS status;
    P_U8 a;

    if(0 == n)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    if(NULL != (*aa = a = _al_ptr_alloc(n + 1 /*CH_NULL*/, &status)))
    {
        if(NULL == b)
        {   /* NULL == b allows client to allocate for a string of n characters (and the CH_NULL) */
            a[0] = CH_NULL; /* allows append */
        }
        else
        {
            memcpy32(a, b, n);
            a[n] = CH_NULL;
            assert(n <= strlen32(a));
        }
        status = STATUS_DONE;
    }

    return(status);
}

/******************************************************************************
*
*  swap two str_set() strings
*
******************************************************************************/

extern void
str_swap(
    _InoutRef_  P_P_USTR aa,
    _InoutRef_  P_P_USTR bb)
{
    P_USTR c;

    c   = *bb;
    *bb = *aa;
    *aa = c;
}

/******************************************************************************
*
* find first occurrence of b in a, or NULL, case insensitive
*
******************************************************************************/

extern P_USTR
stristr(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b)
{
    int i;

    for(;;)
    {
        i = 0;

        for(;;)
        {
            if(!b[i])
                return((P_U8) a);

            if(tolower(a[i]) != tolower(b[i]))
                break;

            ++i;
        }

        if(!*a++)
            return(NULL);
    }
}

/******************************************************************************
*
* compare first n characters from a and b backwards
* (useful for similar tag searches)
*
* --out--
*       number of leading characters same compared backwards
*                 ~~~~~~~
******************************************************************************/

extern int
strrncmp(
    _In_reads_(n) PC_U8 a,
    _In_reads_(n) PC_U8 b,
    _In_        U32 n)
{
    if(n)
    {
        a += n;
        b += n;

        do
            if(*--a != *--b)
                break;
        while(--n);
    }

    return(n);
}

/* end of xstring.c */
