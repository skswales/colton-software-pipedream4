/* xstring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Library module for string handling (eg MS extensions in RISC OS) */

/* SKS May 1990 */

#include "common/gflags.h"

#include "xstring.h"

/* strtol, strtoul without whitespace stripping and base rubbish */

extern U32
fast_strtoul(
    _In_z_      PC_USTR p_u8_in,
    _Out_opt_   P_USTR * endptr)
{
    PC_USTR p_u8 = p_u8_in;
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
        digit = c - '0';
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
        *endptr = ok ? (P_USTR) (p_u8 - 1) : (P_USTR) p_u8_in;

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
#define   SWAPTYPE  int
#elif WINDOWS
#define   SWAPTYPE  int
#endif

#ifdef SWAPTYPE
typedef SWAPTYPE * P_SWAPTYPE;
#endif

extern void
memswap32(
    _Inout_updates_bytes_(n_bytes) P_ANY p1,
    _Inout_updates_bytes_(n_bytes) P_ANY p2,
    _In_        U32 nbytes)
{
    P_U8 c1;
    P_U8 c2;
    U8 c;
    U32 i;

#ifdef SWAPTYPE
    /* copy aligned words at a time if possible */
    if( (((SWAPTYPE) p1 & (sizeof(SWAPTYPE)-1)) == 0) &&
        (((SWAPTYPE) p2 & (sizeof(SWAPTYPE)-1)) == 0) )
        {
        P_SWAPTYPE s1, s2;
        SWAPTYPE s;

        s1 = p1;
        s2 = p2;

        while(nbytes > sizeof32(s))
            {
            nbytes -= sizeof32(s);
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

    for(i = 0; i < nbytes; ++i)
        {
         c    = *c1;
        *c1++ = *c2;
        *c2++ =  c;
        }
}

#undef SWAPTYPE

_Check_return_
extern U32
xustrlen32(
    _In_reads_z_(elemof_buffer) PC_USTR buffer,
    _InVal_     U32 elemof_buffer)
{
    PC_USTR p = memchr(buffer, NULLCH, elemof_buffer);
    assert(elemof_buffer);
    if(NULL == p)
        return(elemof_buffer - 1);
    return(PtrDiffBytesU32(p, buffer));
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

/******************************************************************************
*
* do a string assignment
* if the length of the new string is 0, set to 0
* if cannot allocate new string, return -1
* up to caller to distingush these cases
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
            a[l] = NULLCH;
            return(STATUS_DONE);
            }

        al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
        }

    if(0 == l)
        return(STATUS_OK);

    if(NULL != (*aa = a = _al_ptr_alloc(l + 1/*NULLCH*/, &status)))
    {
        void_memcpy32(a, b, l + 1/*NULLCH*/);

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

    if(NULL != (*aa = a = _al_ptr_alloc(n + 1 /*NULLCH*/, &status)))
    {
        if(NULL == b)
        {   /* NULL == b allows client to allocate for a string of n characters (and the NULLCH) */
            a[0] = NULLCH; /* allows append */
        }
        else
        {
            void_memcpy32(a, b, n);
            a[n] = NULLCH;
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

#if RISCOS

/******************************************************************************
*
* case insensitive lexical comparison of two strings
*
******************************************************************************/

extern int
_stricmp(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b)
{
    int c1, c2;

    for(;;)
        {
        c1 = *a++;
        c1 = tolower(c1);
        c2 = *b++;
        c2 = tolower(c2);

        if(c1 != c2)
            return(c1 - c2);

        if(c1 == 0)
            return(0);          /* no need to check c2 */
        }
}

/******************************************************************************
*
* case insensitive lexical comparison of part of two strings
*
******************************************************************************/

extern int
_strnicmp(
    _In_z_      PC_USTR a,
    _In_z_      PC_USTR b,
    size_t n)
{
    int c1, c2;

    while(n-- > 0)
        {
        c1 = *a++;
        c1 = tolower(c1);
        c2 = *b++;
        c2 = tolower(c2);

        if(c1 != c2)
            return(c1 - c2);

        if(c1 == 0)
            return(0);          /* no need to check c2 */
        }

    return(0);
}

#endif /* RISCOS */

/******************************************************************************
*
* this routine is a quick hack from PD3 stringcmp,
* skips control characters
* uses wild ^? single and ^# multiple, ^^ == ^
* needs rewriting long-term for generality!
*
* leading and trailing spaces are insignificant
*
******************************************************************************/

#define is_control(ch) ((ch) > 0 && (ch) < ' ')

extern S32
stricmp_wild(
    _In_z_      PC_USTR ptr1,
    _In_z_      PC_USTR ptr2)
{
    PC_U8 x, y, ox, oy, end_x, end_y;
    char ch;
    S32 wild_x;
    S32 pos_res;

    /*trace_2(0, "wild_stricmp('%s', '%s')\n", ptr1, ptr2);*/

    /* skip leading spaces */
    while(*ptr2 == ' ')
        ++ptr2;
    while(*ptr1 == ' ')
        ++ptr1;

    /* find end of strings */
    end_x = ptr2;
    while(*end_x)
        ++end_x;
    if(end_x > ptr2)
        while(end_x[-1] == ' ')
            --end_x;

    /* must skip leading hilites in template string for final rejection */
    while(is_control(*ptr2))
        ++ptr2;
    x = ptr2 - 1;

    end_y = ptr1;
    while(*end_y)
        ++end_y;
    if(end_y > ptr1)
        while(end_y[-1] == ' ')
            --end_y;

    y = ptr1;

STAR:
    /* skip a char & hilites in template string */
    do { ch = *++x; } while(is_control(ch));
    /*trace_1(0, "wild_stricmp STAR (x hilite skipped): x -> '%s'\n", x);*/

    wild_x = (ch == '^');
    if(wild_x)
        ++x;

    oy = y;

    /* loop1: */
    while(TRUE)
        {
        while(is_control(*y))
            ++y;

        /* skip a char & hilites in second string */
        do { ch = *++oy; } while(is_control(ch));
        /*trace_1(0, "wild_stricmp loop1 (oy hilite skipped): oy -> '%s'", oy);*/

        ox = x;

        /* loop3: */
        while(TRUE)
            {
            if(wild_x)
                switch(*x)
                    {
                    case '#':
                        /*trace_0(0, "wild_stricmp loop3: ^# found in first string: goto STAR to skip it & hilites");*/
                        goto STAR;

                    case '^':
                        /*trace_0(0, "wild_stricmp loop3: ^^ found in first string: match as ^");*/
                        wild_x = FALSE;

                    default:
                        break;
                    }

            /*trace_3(0, "wild_stricmp loop3: x -> '%s', y -> '%s', wild_x %s\n", x, y, trace_boolstring(wild_x));*/

            /* are we at end of y string? */
            if(y == end_y)
                {
                /*trace_1(0, "wild_stricmp: end of y string: returns %d\n", (*x == '\0') ? 0 : -1);*/
                if(x == end_x)
                    return(0);       /* equal */
                else
                    return(-1);      /* first bigger */
                }

            /* see if characters at x and y match */
            pos_res = toupper(*y) - toupper(*x);
            if(pos_res)
                {
                /* single character wildcard at x? */
                if(!wild_x  ||  *x != '?'  ||  *y == ' ')
                    {
                    y = oy;
                    x = ox;

                    if(x == ptr2)
                        {
                        /*trace_1(0, "wild_stricmp: returns %d\n", pos_res);*/
                        return(pos_res);
                        }

                    /*trace_0(0, "wild_stricmp: chars differ: restore ptrs & break to loop1");*/
                    break;
                    }
                }

            /* characters at x and y match, so increment x and y */
            /*trace_0(0, "wild_stricmp: chars at x & y match: ++x, ++y & hilite skip both & keep in loop3");*/
            do { ch = *++x; } while(is_control(ch));

            wild_x = (ch == '^');
            if(wild_x)
                ++x;

            do { ch = *++y; } while(is_control(ch));
            }
        }
}

/******************************************************************************
*
* convert column string into column number
*
* --out--
* number of character scanned
*
******************************************************************************/

extern S32
stox(
    _In_        PC_U8 string,
    _OutRef_    P_S32 p_col)
{
    S32 scanned;
    int c;

    c = toupper(*string);
    ++string;
    scanned = 0;

    if((c >= 'A') && (c <= 'Z'))
        {
        S32 col;

        col     = (c - 'A');
        scanned = 1;

        for(;;)
            {
            c = toupper(*string);
            ++string;
            if((c < 'A') || (c > 'Z'))
                break;

            if(col > ((S32_MAX / 26) - 1))
                break;
            col += 1;
            col *= 26;

            if(col > ((S32_MAX - c) + 'A'))
                break;
            col += c;
            col -= 'A';

            ++scanned;
            }

        *p_col = col;
        }

    return(scanned);
}

/******************************************************************************
*
* convert column to a string
*
* --out--
* length of resulting string
*
******************************************************************************/

static const S32
nth_letter[] =
{
    26,                         /* need this entry as loop looks at nlp-1 */
    26 + 26*26,
    26 + 26*26 + 26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26 + 26*26*26*26*26,
    26 + 26*26 + 26*26*26 + 26*26*26*26 + 26*26*26*26*26 + 26*26*26*26*26*26
};

static const S32
nth_power[] =
{
    26,                         /* dummy entry - never used */
    26*26,
    26*26*26,
    26*26*26*26,
    26*26*26*26*26,
    26*26*26*26*26*26
};

extern S32
xtos_buf(
    _Out_writes_z_(elemof_buffer) P_U8 string /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case)
{
    P_U8 c;
    BOOL force;
    U32 digit_n, i;
    PC_S32 nlp, npp;
    S32 col, nl, np;
    S32 digit[elemof32(nth_power)];

    col     = x;
    digit_n = 0;

    if(col >= 26)
        {
        if(col >= nth_letter[1])
            {
            force = FALSE;
            nlp = nth_letter + elemof32(nth_letter) - 1;
            npp = nth_power  + elemof32(nth_power)  - 1;

            do  {
                if(force || (col >= *nlp))
                    {
                    nl = *(nlp - 1);
                    np = *npp;
                    col -= nl;
                    digit[digit_n++] = col / np - 1;
                    col = col % np + nl;
                    force = TRUE;
                    }

                --nlp;
                --npp;
                }
            while(nlp > nth_letter);        /* don't ever loop with nth_letter[0] */
            }

        /* nl == 0 */
        digit[digit_n++] = col / 26 - 1;
        col = col % 26;
        }

    digit[digit_n++] = col;

    /* make the string */
    for(i = 0, c = string; i < digit_n; ++i)
        {
        if(i == elemof_buffer)
            break;

        *c++ = (char) (digit[i] + (upper_case ? 'A' : 'a'));
        }

    if(i < elemof_buffer)
        *c = NULLCH;
    else
        string[elemof_buffer - 1] = NULLCH;

    return(c - string);
}

/*
portable string copy functions that ensure NULLCH termination without buffer overflow

strcpy(), strncat() etc and even their _s() variants are all a bit 'wonky'

copying to dst buffer is limited by dst_n characters

dst buffer is always then NULLCH terminated within dst_n characters limit
*/

/*
append up characters from src (until NULLCH found) to dst (subject to dst limit)
*/

extern void
void_strkat(
    _Inout_updates_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_USTR src)
{
    U32 dst_idx = 0;
    U8 ch;

    /* find out where in dst buffer to append the string */
    for(;; dst_idx++)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = NULLCH;
            return;
        }

        ch = dst[dst_idx];

        /* append here? */
        if(NULLCH == ch)
            break;
    }

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string? */
        if(NULLCH == ch)
            break;

        /* is there room to put both this character *and a NULLCH* to destination buffer? */
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
    dst[dst_idx] = NULLCH;
}

/*
append up to src_n characters from src (or fewer, if NULLCH found) to dst (subject to dst limit)
*/

extern void
void_strnkat(
    _Inout_updates_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_reads_(src_n) PC_U8 src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    U8 ch;

    /* find out where in dst buffer to append the string */
    for(;; dst_idx++)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = NULLCH;
            return;
        }

        ch = dst[dst_idx];

        /* append here? */
        if(NULLCH == ch)
            break;
    }

    /* copy src to this point */
    while(src_idx < src_n)
    {
        ch = src[src_idx++];

        /* finished source string before reaching source limit? */
        if(NULLCH == ch)
            break;

        /* is there room to put both this character *and a NULLCH* to destination buffer? */
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
    dst[dst_idx] = NULLCH;
}

/*
copy characters from src (until NULLCH found) to dst (subject to dst limit)
*/

extern void
void_strkpy(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_USTR src)
{
    U32 dst_idx = 0;
    U8 ch;

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string? */
        if(NULLCH == ch)
            break;

        /* is there room to put both this character *and a NULLCH* to destination buffer? */
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
    dst[dst_idx] = NULLCH;
}

/*
copy up to src_n characters from src (or fewer, if NULLCH found) to dst (subject to dst limit)
*/

extern void
void_strnkpy(
    _Out_writes_z_(dst_n) P_USTR dst,
    _InVal_     U32 dst_n,
    _In_reads_(src_n) PC_U8 src,
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
        if(NULLCH == ch)
            break;

        /* is there room to put both this character *and a NULLCH* to destination buffer? */
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
    dst[dst_idx] = NULLCH;
}

/*
a portable but inexact replacement for snprintf(), which Microsoft CRT doesn't have... also ensures NULLCH termination
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

#if RISCOS && 1 /* C99 CRT */

    int ret;

    if(0 == dst_n)
        return(0);

    va_start(args, format);
    ret = vsnprintf(dst, dst_n, format, args);
    va_end(args);

    if(ret >= (int) dst_n) /* limit the answer */
        ret = strlen32(dst);

    return(ret);

#else

#error Oops

#endif

}

/* end of xstring.c */
