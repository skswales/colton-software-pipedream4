/* xustring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for UCHAR-based string handling */

/* SKS Apr 2014 */

#include "common/gflags.h"

#include "xstring.h"

_Check_return_
extern U32
fast_ustrtoul(
    _In_z_      PC_USTR ustr_in,
    _OutRef_opt_ P_PC_USTR endptr)
{
    return(fast_strtoul((PC_U8Z) ustr_in, (P_P_U8Z) endptr));
}

/******************************************************************************
*
* convert column string into column number
*
* --out--
* number of character scanned
*
******************************************************************************/

_Check_return_
extern S32
stox(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_S32 p_col)
{
    S32 n_scanned = 0;
    S32 col = 0;
    int c;

    profile_ensure_frame();

    c = PtrGetByte(ustr); ustr_IncByte(ustr);
    c = toupper(c);

    if((c >= 'A') && (c <= 'Z'))
    {
        col = (c - 'A');
        n_scanned = 1;

        for(;;)
        {
            c = PtrGetByte(ustr); ustr_IncByte(ustr);
            c = toupper(c);

            if((c < 'A') || (c > 'Z'))
                break;

            if(col > ((S32_MAX / 26) - 1))
                break; /* would overflow on increasing one place */
            col += 1;
            col *= 26;

            if(col > ((S32_MAX - c) + 'A'))
                break; /* would overflow on adding in this component */
            col += c;
            col -= 'A';

            ++n_scanned;
        }
    }

    *p_col = col;

    return(n_scanned);
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
    (S32)26,                    /* need this entry as loop looks at nlp-1 */

    (S32)26 +
        (S32)26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26 +
                    (S32)26*26*26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26 +
                    (S32)26*26*26*26*26 +
                        (S32)26*26*26*26*26*26
};

static const S32
nth_power[] =
{
    (S32)26,                    /* dummy entry - never used */
    (S32)26*26,
    (S32)26*26*26,
    (S32)26*26*26*26,
    (S32)26*26*26*26*26,
    (S32)26*26*26*26*26*26
};

/*ncr*/
extern U32 /*number of bytes in converted buffer*/
xtos_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case)
{
    P_U8 c;
    BOOL force;
    U32 digit_n, i;
    PC_S32 nlp, npp;
    S32 col, nl, np;
    S32 digit[1 + 1 + elemof32(nth_power)];

    profile_ensure_frame();

    assert(elemof_buffer >= 2); /* at least one digit and terminator please! */

    col = x;
    digit_n = 0;

    if(col >= 26 /*nth_letter[0]*/)
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
    else if(col < 0) /* avoid catastrophe */
        col = 0;

    digit[digit_n++] = col;

    /* make the string */
    for(i = 0, c = ustr_buf; (i < elemof_buffer-1) && (i < digit_n); ++i)
    {
        *c++ = (char) (digit[i] + (upper_case ? 'A' : 'a'));
    }

    *c = CH_NULL;
    return(PtrDiffBytesU32(c, ustr_buf));
}

#if USTR_IS_SBSTR

#if 0 /* just for diff minimization */

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two counted UCHAR sequences
*
* NB uses t5_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b)
{
    int res;
    PC_U8 str_a = (PC_U8) uchars_a;
    PC_U8 str_b = (PC_U8) uchars_b;
    U32 limit = MIN(uchars_n_a, uchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

    for(i = 0; i < limit; ++i)
    {
        int c_a = *str_a++;
        int c_b = *str_b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = t5_sortbyte(c_a);
            c_b = t5_sortbyte(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of chars left over? */
    remain_a = uchars_n_a - limit;
    remain_b = uchars_n_b - limit;

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the specified finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

#endif

/******************************************************************************
*
* this routine is a quick hack from PD3 stringcmp
*
* skips control characters
* uses wild ^? single and ^# multiple, ^^ == ^
* needs rewriting long-term for generality!
*
* leading and trailing spaces are insignificant
*
******************************************************************************/

#if 0 /* just for diff minimization */

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase_wild(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b)

#endif

#define is_control(ch) ((ch) > 0 && (ch) < ' ')

extern S32
stricmp_wild(
    _In_z_      PC_USTR ptr1,
    _In_z_      PC_USTR ptr2)
{
    PC_U8 x, y, ox, oy, end_x, end_y;
    char ch;
    S32 wild_x;
    int sb_x, sb_y;
    S32 pos_res;

    profile_ensure_frame();

    /*trace_2(0, TEXT("wild_stricmp('%s', '%s')"), ptr1, ptr2);*/

    /* skip leading spaces */
    while(*ptr2 == CH_SPACE)
        ++ptr2;
    while(*ptr1 == CH_SPACE)
        ++ptr1;

    /* find end of strings */
    end_x = ptr2;
    while(*end_x)
        ++end_x;
    if(end_x > ptr2)
        while(end_x[-1] == CH_SPACE)
            --end_x;

    /* must skip leading hilites in template string for final rejection */
    while(is_control(*ptr2))
        ++ptr2;
    x = ptr2 - 1;

    end_y = ptr1;
    while(*end_y)
        ++end_y;
    if(end_y > ptr1)
        while(end_y[-1] == CH_SPACE)
            --end_y;

    y = ptr1;

STAR:
    /* skip a char & hilites in template string */
    do { ch = *++x; } while(is_control(ch));
    /*trace_1(0, TEXT("wild_stricmp STAR (x hilite skipped): x -> '%s'"), x);*/

    wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
    if(wild_x)
        ++x;

    oy = y;

    /* loop1: */
    for(;;)
    {
        while(is_control(*y))
            ++y;

        /* skip a char & hilites in second string */
        do { ch = *++oy; } while(is_control(ch));
        /*trace_1(0, TEXT("wild_stricmp loop1 (oy hilite skipped): oy -> '%s'"), oy);*/

        ox = x;

        /* loop3: */
        for(;;)
        {
            if(wild_x)
                switch(*x)
                {
                case CH_NUMBER_SIGN:
                    /*trace_0(0, TEXT("wild_stricmp loop3: ^# found in first string: goto STAR to skip it & hilites"));*/
                    goto STAR;

                case CH_CIRCUMFLEX_ACCENT:
                    /*trace_0(0, TEXT("wild_stricmp loop3: ^^ found in first string: match as ^"));*/
                    wild_x = FALSE;

                default:
                    break;
                }

            /*trace_3(0, TEXT("wild_stricmp loop3: x -> '%s', y -> '%s', wild_x %s"), x, y, report_boolstring(wild_x));*/

            /* are we at end of y string? */
            if(y == end_y)
            {
                /*trace_1(0, TEXT("wild_stricmp: end of y string: returns %d"), (*x == CH_NULL) ? 0 : -1);*/
                if(x == end_x)
                    return(0);       /* equal */
                else
                    return(-1);      /* first is bigger */
            }

            /* see if characters at x and y match */
            sb_x = toupper(*x);
            sb_y = toupper(*y);
            pos_res = sb_y - sb_x;

            if(0 != pos_res)
            {
                /* single character wildcard at x? */
                if(!wild_x  ||  (*x != CH_QUOTATION_MARK)  ||  (*y == CH_SPACE))
                {
                    y = oy;
                    x = ox;

                    if(x == ptr2)
                    {
                        /*trace_1(0, TEXT("wild_stricmp: returns %d"), pos_res);*/
                        return(pos_res);
                    }

                    /*trace_0(0, TEXT("wild_stricmp: chars differ: restore ptrs & break to loop1"));*/
                    break;
                }
            }

            /* characters at x and y match, so increment x and y */
            /*trace_0(0, TEXT("wild_stricmp: chars at x & y match: ++x, ++y & hilite skip both & keep in loop3"));*/
            do { ch = *++x; } while(is_control(ch));

            wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
            if(wild_x)
                ++x;

            do { ch = *++y; } while(is_control(ch));
        }
    }
}

#endif /* USTR_IS_SBSTR */

#if 0 /* just for diff minimization */

/*
conversion routines
*/

/*ncr*/
extern U32
sbstr_buf_from_ustr(
    _Out_writes_z_(elemof_buffer) P_SBSTR buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
#if USTR_IS_SBSTR
    UNREFERENCED_PARAMETER_InVal_(sbchar_codepage);
    /* no conversion needed, don't waste any more time/space */
    return(xstrnkpy(buffer, elemof_buffer, ustr, ustrlen32_n(ustr, uchars_n)));
#else
    return(sbstr_buf_from_utf8str(buffer, elemof_buffer, sbchar_codepage, ustr, uchars_n));
#endif /* USTR_IS_SBSTR */
}

/*
low-lifetime conversion routines
*/

#if (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING && 0)

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_ustr(
    _In_z_      PC_USTR ustr)
{
    if(NULL == ustr)
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NULL>>"));
    }

    if(PTR_IS_NONE(ustr))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NONE>>"));
    }

    if(contains_inline(ustr, ustrlen32(ustr)))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - CONTAINS INLINES>>"));
    }

    return(_tstr_from_sbstr((PC_SBSTR) ustr));
}

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR /*low-lifetime*/
_ustr_from_tstr(
    _In_z_      PCTSTR tstr)
{
    if(NULL == tstr)
        return(("<<ustr_from_tstr - NULL>>"));

    if(PTR_IS_NONE(tstr))
        return(("<<ustr_from_tstr - NONE>>"));

    return((PC_USTR) _sbstr_from_tstr(tstr));
}

#else /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING) */

/* either no conversion is required or host-specific implementation */

#endif /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING) */

#endif

/* end of xustring.c */
