/* ev_fnstr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* String function routines for evaluator */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "cmodules/mathxtr3.h"

#include "cmodules/numform.h"

/******************************************************************************
*
* String functions
*
******************************************************************************/

/******************************************************************************
*
* STRING char(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_char)
{
    S32 n = args[0]->arg.integer;
    U8 ch = (U8) n;

    exec_func_ignore_parms();

    if((U32) n >= 256U) /* Some users are known to put highlight characters in so don't get too picky */
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    status_assert(ss_string_make_uchars(p_ev_data_res, &ch, 1));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING clean("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_clean)
{
    U32 bytes_of_buffer = 0;
    int pass = 1;

    exec_func_ignore_parms();

    do  {
        U32 o_idx = 0;
        U32 i_idx = 0;

        /* clean the string of unprintable chracters during transfer */
        while(i_idx < args[0]->arg.string.size)
        {
            U32 bytes_of_char;
            UCS4 ucs4 = uchars_char_decode_off(args[0]->arg.string.uchars, i_idx, /*ref*/bytes_of_char);

            i_idx += bytes_of_char;

#if !USTR_IS_L1STR
            if(!ucs4_validate(ucs4))
                continue;
#endif

            if(ucs4_is_latin1(ucs4) && !t5_isprint((U8) ucs4))
                continue;

            if(pass == 2)
            {
                assert((o_idx + bytes_of_char) <= bytes_of_buffer);
                (void) uchars_char_encode_off(p_ev_data_res->arg.string_wr.uchars, bytes_of_buffer, o_idx, ucs4);
            }

            o_idx += bytes_of_char;
        }

        if(pass == 1)
        {
            bytes_of_buffer = o_idx;

            if(status_fail(ss_string_make_uchars(p_ev_data_res, NULL, bytes_of_buffer)))
                return;
        }
    }
    while(++pass <= 2);
}

#endif

/******************************************************************************
*
* INTEGER code("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_code)
{
    S32 code_result;
    
    exec_func_ignore_parms();

    if(0 == args[0]->arg.string.size)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    code_result = (S32) *(args[0]->arg.string.uchars);

    ev_data_set_integer(p_ev_data_res, code_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING dollar(number {, decimals})
*
* a bit like our old STRING() function but with a currency symbol and thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dollar)
{
    STATUS status;
    S32 decimal_places = 2;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    status = quick_ublock_ucs4_add(&quick_ublock_format, CH_POUND_SIGN); /* FIXME */

    if(status_ok(status))
        status = quick_ublock_ustr_add(&quick_ublock_format, USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_ucs4_add(&quick_ublock_format, CH_FULL_STOP);

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_ucs4_add(&quick_ublock_format, CH_NUMBER_SIGN);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ev_data_res);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

#endif

/******************************************************************************
*
* BOOLEAN exact("text1", "text2")
*
******************************************************************************/

PROC_EXEC_PROTO(c_exact)
{
    BOOL exact_result = FALSE;

    exec_func_ignore_parms();

    if(args[0]->arg.string.size == args[1]->arg.string.size)
        if(0 == memcmp32(args[0]->arg.string.uchars, args[1]->arg.string.uchars, (U32) args[0]->arg.string.size))
            exact_result = TRUE;

    ev_data_set_boolean(p_ev_data_res, exact_result);
}

/******************************************************************************
*
* INTEGER find("find_text", "in_text" {, start_n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_find)
{
    S32 find_result = 0;
    S32 find_len = args[1]->arg.string.size;
    S32 start_n = 1;
    P_U8 ptr;

    exec_func_ignore_parms();

    if(n_args > 2)
        start_n = args[2]->arg.integer;

    if(start_n <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    start_n -= 1; /* SKS 12apr95 could have caused exception with find("str", "") as find_len == 0 */

    if( start_n > find_len)
        start_n = find_len;

    if(NULL != (ptr = (P_U8) memstr32(args[1]->arg.string.uchars + start_n, find_len - start_n, args[0]->arg.string.uchars, args[0]->arg.string.size))) /*strstr replacement*/
        find_result = 1 + (S32) (ptr - args[1]->arg.string.uchars);

    ev_data_set_integer(p_ev_data_res, find_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING fixed(number {, decimals {, no_commas}})
*
* a bit like our old STRING() function but with thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fixed)
{
    STATUS status;
    S32 decimal_places = 2;
    BOOL no_commas = FALSE;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    if(n_args > 2)
        no_commas = (0 != args[2]->arg.integer);

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    status = quick_ublock_ustr_add(&quick_ublock_format, no_commas ? USTR_TEXT("0") : USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_a7char_add(&quick_ublock_format, '.');

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_a7char_add(&quick_ublock_format, '#');

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ev_data_res);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

#endif

/******************************************************************************
*
* STRING formula_text(ref)
*
******************************************************************************/

PROC_EXEC_PROTO(c_formula_text)
{
    P_EV_SLOT p_ev_slot;

    exec_func_ignore_parms();

    if(ev_travel(&p_ev_slot, &args[0]->arg.slr) > 0)
    {
        U8Z buffer[4096];
        EV_DOCNO docno = args[0]->arg.slr.docno;
        EV_OPTBLOCK optblock;

        ev_set_options(&optblock, docno);
        optblock.lf = optblock.cr = 0;

        ev_decode_slot(docno, buffer, p_ev_slot, &optblock);

        status_assert(ss_string_make_ustr(p_ev_data_res, buffer));
    }
}

/******************************************************************************
*
* STRING join(string1 {, stringn ...})
*
******************************************************************************/

PROC_EXEC_PROTO(c_join)
{
    STATUS status;
    S32 i;
    S32 len = 0;
    P_U8 p_u8;

    exec_func_ignore_parms();

    for(i = 0; i < n_args; ++i)
        len += args[i]->arg.string.size;

    if(0 == len)
        return;

    if(NULL == (p_u8 = al_ptr_alloc_bytes(P_U8, len + 1/*NULLCH*/, &status)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    p_ev_data_res->arg.string.uchars = p_u8;
    p_ev_data_res->arg.string.size = len;
    p_ev_data_res->did_num = RPN_TMP_STRING;

    for(i = 0; i < n_args; ++i)
    {
        U32 arglen = args[i]->arg.string.size;
        memcpy32(p_u8, args[i]->arg.string.uchars, arglen);
        p_u8 += arglen;
    }

    *p_u8 = NULLCH;
}

/******************************************************************************
*
* STRING left(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_left)
{
    const S32 len = args[0]->arg.string.size;
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        if((n = args[1]->arg.integer) < 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.uchars, n));
}

/******************************************************************************
*
* INTEGER length(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_length)
{
    S32 length_result = args[0]->arg.string.size;

    exec_func_ignore_parms();
    
    ev_data_set_integer(p_ev_data_res, length_result);
}

/******************************************************************************
*
* STRING lower("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_lower)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.uchars;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) tolower(*ptr);
    }
}

/******************************************************************************
*
* STRING mid(string, start, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mid)
{
    S32 len = args[0]->arg.string.size;
    S32 start = args[1]->arg.integer - 1; /* ensure that this will not go -ve */
    S32 n = args[2]->arg.integer;

    exec_func_ignore_parms();

    if((args[1]->arg.integer <= 0) || (n < 0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    start = MIN(start, len);
    n = MIN(n, len - start);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.uchars + start, n));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* VALUE n
*
******************************************************************************/

PROC_EXEC_PROTO(c_n)
{
    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        ev_data_set_integer(p_ev_data_res, ss_dateval_to_serial_number(&args[0]->arg.ev_date.date));
        break;

    case RPN_DAT_BOOL8:
        ev_data_set_integer(p_ev_data_res, args[0]->arg.boolean);
        break;

    case RPN_DAT_STRING:
        /* can't discriminate here between strings and text cells */
    default:
        status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
        break;
    }
}

#endif

/******************************************************************************
*
* STRING proper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_proper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.uchars;
        U32 offset_in_word = 0;

        for(x = 0; x < len; ++x, ++ptr)
        {
            if((*ptr == ' ') || (*ptr == '-') /* SKS 27aug97 Barnes-Wallis */)
                offset_in_word = 0;
            else
            {
                if((0 == offset_in_word)
                || ((2 == offset_in_word) && ('\'' == ptr[-1]) && ('O' == ptr[-2])))
                    /* SKS 17jul97 proper case O'Connor (but not A'chir?) */
                    *ptr = (U8) toupper(*ptr);
                else
                    *ptr = (U8) tolower(*ptr);

                ++offset_in_word;
            }
        }
    }
}

/******************************************************************************
*
* STRING replace("text", start_n, chars_n, "with_text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_replace)
{
    STATUS status = STATUS_OK;
    const S32 len = args[0]->arg.string.size;
    S32 start_n_chars = args[1]->arg.integer - 1; /* one-based API */
    S32 excise_n_chars = args[2]->arg.integer;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, BUF_EV_MAX_STRING_LEN);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if((start_n_chars < 0) || (excise_n_chars < 0))
        status = EVAL_ERR_ARGRANGE;
    else
    {
        if( start_n_chars > len)
            start_n_chars = len;

        if(0 != start_n_chars)
            status = quick_ublock_uchars_add(&quick_ublock_result, args[0]->arg.string.uchars, start_n_chars);

        /* NB even if 0 == excise_n_chars; that's just an insert operation */
        if(status_ok(status))
            status = quick_ublock_uchars_add(&quick_ublock_result, args[3]->arg.string.uchars, args[3]->arg.string.size);

        if(status_ok(status) && ((start_n_chars + excise_n_chars) < len))
        {
            S32 end_n_chars = len - (start_n_chars + excise_n_chars);

            status = quick_ublock_uchars_add(&quick_ublock_result, args[0]->arg.string.uchars + (start_n_chars + excise_n_chars), end_n_chars);
        }

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ev_data_res, quick_ublock_uptr(&quick_ublock_result), quick_ublock_bytes(&quick_ublock_result)));
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);

    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* STRING rept("text", n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rept)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    const U32 len = args[0]->arg.string.size;
    S32 n = args[1]->arg.integer;
    P_U8 p_u8 = buffer;
    S32 x;

    exec_func_ignore_parms();

    if( n > (S32) ((elemof32(buffer)-1) / len))
        n = (S32) ((elemof32(buffer)-1) / len);

    if(n <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    for(x = 0; x < n; ++x)
    {
        memcpy32(p_u8, args[0]->arg.string.uchars, len);
        p_u8 += len;
    }

    status_assert(ss_string_make_uchars(p_ev_data_res, buffer, (U32) n * len));
}

/******************************************************************************
*
* STRING reverse("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_reverse)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 half_len = p_ev_data_res->arg.string_wr.size / 2;
        P_U8 ptr_lo = p_ev_data_res->arg.string_wr.uchars;
        P_U8 ptr_hi = p_ev_data_res->arg.string_wr.uchars + p_ev_data_res->arg.string_wr.size;

        for(x = 0; x < half_len; ++x)
        {
            U8 c = *--ptr_hi; /* SKS 04jul96 this was complete twaddle before */
            *ptr_hi = *ptr_lo;
            *ptr_lo++ = c;
        }
    }
}

/******************************************************************************
*
* STRING right(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_right)
{
    const S32 len = args[0]->arg.string.size;
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        if((n = args[1]->arg.integer) < 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.uchars + len - n, n));
}

/******************************************************************************
*
* STRING string(n {, x})
*
******************************************************************************/

PROC_EXEC_PROTO(c_string)
{
    S32 decimal_places = 2;
    F64 rounded_value;
    U8Z buffer[BUF_EV_MAX_STRING_LEN];

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be large for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
        rounded_value = p_ev_data_res->arg.fp;
    else
        rounded_value = (F64) p_ev_data_res->arg.integer; /* take care now we can get integer types back */

    (void) xsnprintf(buffer, elemof32(buffer), "%.*f", (int) decimal_places, rounded_value);

    status_assert(ss_string_make_ustr(p_ev_data_res, buffer));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING substitute(text, old_text, new_text {, instance_num})
*
******************************************************************************/

PROC_EXEC_PROTO(c_substitute)
{
    STATUS status = STATUS_OK;
    U32 text_size = args[0]->arg.string.size;
    PC_UCHARS text_uchars = args[0]->arg.string.uchars;
    U32 old_text_size = args[1]->arg.string.size;
    PC_UCHARS old_text_uchars = args[1]->arg.string.uchars;
    U32 new_text_size = args[2]->arg.string.size;
    PC_UCHARS new_text_uchars = args[2]->arg.string.uchars;
    U32 instance_to_replace = 0;

    exec_func_ignore_parms();

    if(n_args > 3)
        instance_to_replace = args[3]->arg.integer;

    if(0 == old_text_size)
    {   /* nothing to be matched for substitution - just copy all the current text over to the result */
        status_assert(ss_string_make_uchars(p_ev_data_res, text_uchars, text_size));
    }
    else
    {
        U32 instance = 1;
        U32 text_i = 0;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, BUF_EV_MAX_STRING_LEN);
        quick_ublock_with_buffer_setup(quick_ublock_result);

        while(text_i < text_size)
        {
            const U32 text_size_remain = text_size - text_i;
            PC_UCHARS text_to_test;
            U32 bytes_of_char;

            if(text_size_remain < old_text_size)
                break;

            text_to_test = uchars_AddBytes(text_uchars, text_i);

            if( ((instance == instance_to_replace) || (0 == instance_to_replace)) &&
                (0 == short_memcmp32(text_to_test, old_text_uchars, old_text_size)) )
            {   /* found an instance of old_text in text to replace */

                /* copy the replacement string (which may be empty) to the output */
                status_break(status = quick_ublock_uchars_add(&quick_ublock_result, new_text_uchars, new_text_size));

                /* skip over all the matched characters in the current string */
                text_i += old_text_size;

                ++instance;

                continue;
            }

            /* otherwise copy the character from the current string to the output */
            bytes_of_char = uchars_bytes_of_char(text_to_test);

            status_break(status = quick_ublock_uchars_add(&quick_ublock_result, text_to_test, bytes_of_char));

            text_i += bytes_of_char;

            assert(text_i <= text_size);
        }

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ev_data_res, quick_ublock_uchars(&quick_ublock_result), quick_ublock_bytes(&quick_ublock_result)));

        quick_ublock_dispose(&quick_ublock_result);
    }
}

/******************************************************************************
*
* STRING t(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_t)
{
    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    default:
        status_consume(ss_string_make_uchars(p_ev_data_res, NULL, 0));
        break;

    case RPN_DAT_STRING:
        status_consume(ss_string_dup(p_ev_data_res, args[0]));
        break;
    }
}

#endif

_Check_return_
static STATUS
ev_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_EV_DATA p_ev_data)
{
    NUMFORM_PARMS numform_parms /*= NUMFORM_PARMS_INIT*/;

    numform_parms.ustr_numform_numeric =
    numform_parms.ustr_numform_datetime =
    numform_parms.ustr_numform_texterror = ustr;

    numform_parms.p_numform_context = P_DATA_NONE; /* use default */

    return(numform(p_quick_ublock, P_QUICK_UBLOCK_NONE, p_ev_data, &numform_parms));
}

/******************************************************************************
*
* STRING text(n, numform_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_text)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, args[1]->arg.string.uchars, args[1]->arg.string.size)))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* STRING trim("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_trim)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    BOOL gap;
    P_U8 s_ptr, e_ptr, i_ptr, o_ptr;

    exec_func_ignore_parms();

    memcpy32(buffer, args[0]->arg.string.uchars, args[0]->arg.string.size);

    s_ptr = buffer;
    e_ptr = buffer + args[0]->arg.string.size;

    /* skip leading spaces */
    while(*s_ptr == ' ')
        ++s_ptr;

    /* skip trailing spaces */
    while((e_ptr > s_ptr) && (e_ptr[-1] == ' '))
        --e_ptr;

    *e_ptr = NULLCH;

    /* crunge multiple spaces */
    for(i_ptr = o_ptr = s_ptr, gap = 0; i_ptr <= e_ptr; ++i_ptr)
    {
        if(*i_ptr == ' ')
        {
            if(gap)
                continue;
            else
                gap = TRUE;
        }
        else
            gap = FALSE;

        *o_ptr++ = *i_ptr;
    }

    status_assert(ss_string_make_ustr(p_ev_data_res, s_ptr));
}

/******************************************************************************
*
* STRING upper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_upper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.uchars;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) toupper(*ptr);
    }
}

/******************************************************************************
*
* NUMBER value("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_value)
{
    U8Z buffer[256];
    U32 len;
    P_U8 ptr;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, sizeof32(buffer)-1);
    memcpy32(buffer, args[0]->arg.string.uchars, len);
    buffer[len] = NULLCH;

    ev_data_set_real(p_ev_data_res, strtod(buffer, &ptr));

    if(ptr == buffer)
        ev_data_set_integer(p_ev_data_res, 0); /* SKS 08sep97 now behaves as documented */
    else
        real_to_integer_try(p_ev_data_res);
}

/* end of ev_fnstr.c */
