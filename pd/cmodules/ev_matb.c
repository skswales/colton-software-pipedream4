/* ev_matb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2022 Stuart Swales */

/* More mathematical function routines for evaluator */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"

/* local header file */
#include "ev_evali.h"

/******************************************************************************
*
* More mathematical functions for OpenDocument / Microsoft Excel support
*
******************************************************************************/

/******************************************************************************
*
* STRING base(n, radix {, minimum_length})
*
******************************************************************************/

PROC_EXEC_PROTO(c_base)
{
    STATUS status = STATUS_OK;
    S32 radix;
    S32 minimum_length = 1;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    radix = ss_data_get_integer(args[1]);

    if( (radix < 2) || (radix > 36) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    if(n_args > 2)
    {
        minimum_length = ss_data_get_integer(args[2]);

        if( (minimum_length < 1) || (minimum_length > 256) )
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    for(i = 0; (i < minimum_length) && status_ok(status); ++i)
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_DIGIT_ZERO);

    if(status_ok(status))
        status = quick_ublock_printf(&quick_ublock_format, ustr_bptr("B%d"), radix);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status))
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]);

    if(status_ok(status))
        status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* decimal() helper functions
*
******************************************************************************/

_Check_return_
static STATUS
from_base_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 radix,
    _OutRef_    P_F64 p_f64);

_Check_return_
static STATUS
from_base_decode_real(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_        F64 f64,
    _InVal_     U32 radix,
    _OutRef_    P_F64 p_f64)
{
    char buffer[64];

    f64 = real_floor(f64);

    *p_f64 = 0.0;

    if((f64 < 0.0) /*|| (f64 > 9999999999.0)*/) /* range check input, decimal digits (can't have A-Z) */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    consume_int(xsnprintf(buffer, sizeof(buffer), "%.0f", f64));

    return(from_base_decode_string(p_ss_data_res, uchars_bptr(buffer), strlen32(buffer), radix, p_f64));
}

_Check_return_
static STATUS
from_base_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 radix,
    _OutRef_    P_F64 p_f64)
{
    U32 wss;
    U32 initial_buf_idx;
    U32 buf_idx;
    F64 f64 = 0.0;

    *p_f64 = 0.0;

    wss = ss_string_skip_leading_whitespace_uchars(uchars, uchars_n);
    buf_idx = wss;

    if(16 == radix)
    {   /* test for hex prefixes */
        if( ((uchars_n - buf_idx) >= 2) &&
            (('x' == PtrGetByteOff(uchars, buf_idx + 0)) || ('X' == PtrGetByteOff(uchars, buf_idx + 0))) &&
            isxdigit(PtrGetByteOff(uchars, buf_idx + 1)) )
        {
            buf_idx += 1; /* skip that prefix */
        }
        else
        if( ((uchars_n - buf_idx) >= 3) &&
            ('0' == PtrGetByteOff(uchars, buf_idx + 0)) &&
            (('x' == PtrGetByteOff(uchars, buf_idx + 1)) || ('X' == PtrGetByteOff(uchars, buf_idx + 1))) &&
            isxdigit(PtrGetByteOff(uchars, buf_idx + 2)) )
        {
            buf_idx += 2; /* skip that prefix */
        }
    }

    initial_buf_idx = buf_idx;

    while(buf_idx < uchars_n)
    {
        U32 bytes_of_char = 1; /*uchars_bytes_of_char_off(uchars, buf_idx);*/
        U8 u8;
        U32 digit;

        assert((buf_idx + bytes_of_char) <= uchars_n);
        if((buf_idx + bytes_of_char) > uchars_n)
            break;

        u8 = PtrGetByteOff(uchars, buf_idx);

        if(isdigit(u8))
        {
            digit = (U32) toupper(u8) - UCH_DIGIT_ZERO;
        }
        else if(isalpha(u8))
        {   /* a..z, A..Z */
            digit = (((U32) toupper(u8) - UCH_LATIN_CAPITAL_LETTER_A) + 10);
        }
        else
        {
            break;
        }

        if(digit >= radix)
        {
            if((16 == radix) && (18 == digit)) /* read 'H' or 'h' as suffix? */
                buf_idx += bytes_of_char; /* skip that optional suffix */
            else if((2 == radix) && (11 == digit)) /* read 'B' or 'b' as suffix? */
                buf_idx += bytes_of_char; /* skip that optional suffix */
            break;
        }

        f64 = (f64 * radix) + digit;

        buf_idx += bytes_of_char;
    }

    if(buf_idx == initial_buf_idx) /* nothing read? */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_NO_VALID_DATA));

    wss = ss_string_skip_internal_whitespace_uchars(uchars, uchars_n, buf_idx);
    buf_idx += wss;

    if(buf_idx < uchars_n) /* trailing garbage? */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_NO_VALID_DATA));

    *p_f64 = f64;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_base(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     U32 radix,
    _OutRef_    P_F64 p_f64)
{
    if(ss_data_is_real(p_ss_data))
        status_return(from_base_decode_real(p_ss_data_res, ss_data_get_real(p_ss_data), radix, p_f64));
    else
        status_return(from_base_decode_string(p_ss_data_res, ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data), radix, p_f64));

    return(STATUS_OK);
}

/******************************************************************************
*
* NUMBER decimal(number_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_decimal)
{
    const S32 radix = ss_data_get_integer(args[1]);
    F64 f64;

    exec_func_ignore_parms();

    if( (radix < 2) || (radix > 36) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    if(status_fail(from_base(p_ss_data_res, args[0], radix, &f64)))
        return;

    ss_data_set_real_try_integer(p_ss_data_res, f64);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* NUMBER factdouble(n)
*
******************************************************************************/

static void
factdouble_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 n)
{
    if(n <= 3)
    {
        if(n < -1)
        {
            ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
            return;
        }

        ss_data_set_real(p_ss_data_out, (n > 0) ? n : 1); /* 3!!=3, 2!!=2, 1!!=1, 0!!=1, -1!!=1 */
        return;
    }

    if(0 == (n & 1)) /* is_even */
    {   /* where n = 2k, n!! = 2^k * k! */
        const S32 k = n >> 1;

        factorial_calc(p_ss_data_out, k); /* may return integer or real or error */

        switch(ss_data_get_data_id(p_ss_data_out))
        {
        case DATA_ID_WORD32:
            ss_data_set_real(p_ss_data_out, (F64) p_ss_data_out->arg.integer); /* now go to fp for the single multiply */

            /*FALLTHRU*/

        case DATA_ID_REAL:
            ss_data_set_real(p_ss_data_out, ss_data_get_real(p_ss_data_out) * pow(2.0, k));
            break;

        default:
        case DATA_ID_ERROR:
            break;
        }
    }
    else /* is_odd */
    {   /* n!! = n! / (n - 1)!! */
        SS_DATA ss_data_numer;
        SS_DATA ss_data_denom;

        factorial_calc(&ss_data_numer, n); /* may return integer or real or error */

        factdouble_calc(&ss_data_denom, n - 1); /* may return real or error */

        if(!two_nums_divide_try(p_ss_data_out, &ss_data_numer, &ss_data_denom, TRUE /*propagate_errors*/))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);
    }
}

PROC_EXEC_PROTO(c_factdouble)
{
    const S32 n = ss_data_get_integer(args[0]);

    exec_func_ignore_parms();

    factdouble_calc(p_ss_data_res, n); /* may return real or error */

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

/******************************************************************************
*
* NUMBER odf.int(number)
*
* NB OpenDocument and Microsoft Excel truncate towards -infinity, unlike Fireworkz, PipeDream and Lotus 1-2-3
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_int)
{
    exec_func_ignore_parms();

    if(ss_data_is_real(args[0]))
    {
        ss_data_set_real_try_integer(p_ss_data_res, real_floor(ss_data_get_real(args[0])));
        return;
    }

    /* all other types can be handled by our friend */
    c_int(args, n_args, p_ss_data_res, p_cur_slr);
}

#endif

/******************************************************************************
*
* REAL log(x:number {, base:number=10})
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    const F64 x = ss_data_get_real(args[0]);
    const F64 b = (n_args > 1) ? ss_data_get_real(args[1]) : 10.0;
    F64 log_result;

    exec_func_ignore_parms();

    errno = 0;

    if(b == 1.0)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_DIVIDEBY0);

    if(b == 2.0)
    {
        log_result = log2(x);
    }
    else if(b == 10.0)
    {
        log_result = log10(x);
    }
    else
    {
        const F64 log2_x = log2(x);
        const F64 log2_b = log2(b);

        log_result = log2_x / log2_b;
    }

    ss_data_set_real(p_ss_data_res, log_result);

    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_LOG);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* REAL odf.log10(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_log10)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, log10(number));

    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER odf.mod(a, b)
*
* Remainder has the same sign as the divisor (same as OpenOffice and Microsoft Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1], FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        {
        const S32 s32_a = ss_data_get_integer(args[0]);
        const S32 s32_b = ss_data_get_integer(args[1]);
        div_t d;
        S32 s32_odf_mod_result;

        /* SKS after PD 4.11 03feb92 - gave FP error not zero if trap taken */
        if(s32_b == 0)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_DIVIDEBY0);

        d = div((int) s32_a, (int) s32_b);

        s32_odf_mod_result = d.rem; /* remainder has the same sign as the dividend */

        if(0 != s32_odf_mod_result)
        {
            if((s32_odf_mod_result < 0) != (s32_b < 0))
            {
                s32_odf_mod_result += s32_b;
            }
        }

        ss_data_set_integer(p_ss_data_res, s32_odf_mod_result);
        break;
        }

    case TWO_REALS:
        {
        const F64 f64_a = ss_data_get_real(args[0]);
        const F64 f64_b = ss_data_get_real(args[1]);
        F64 f64_odf_mod_result;

        errno = 0;

        f64_odf_mod_result = fmod(f64_a, f64_b); /* remainder has the same sign as the dividend */

        if(0.0 != f64_odf_mod_result)
        {
            if((f64_odf_mod_result < 0) != (f64_b < 0))
            {
                f64_odf_mod_result += f64_b;
            }
        }

        ss_data_set_real_try_integer(p_ss_data_res, f64_odf_mod_result);

        /* would have divided by zero? */
        if(errno /* == EDOM */)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_DIVIDEBY0);

        break;
        }
    }
}

#endif

/******************************************************************************
*
* NUMBER ceiling(n {, multiple})
*
* NUMBER floor(n {, multiple})
*
* NUMBER mround(n, multiple)
*
* NUMBER round(n {, decimals})
*
* NUMBER rounddown(n, decimals)
*
* NUMBER roundup(n, decimals)
*
* NUMBER trunc(n {, decimals})
*
******************************************************************************/

extern void
round_common(
    P_SS_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     U32 rpn_did_num)
{
    BOOL negate_result = FALSE;
    F64 f64, multiplier;

    switch(rpn_did_num)
    {
    case RPN_FNV_CEILING:
    case RPN_FNV_FLOOR:
        {
        f64 = ss_data_get_real(args[0]);

        if(n_args <= 1)
        {
            multiplier = 1.0;
        }
        else
        {
            F64 multiple = ss_data_get_real(args[1]);

            if(multiple < 0.0) /* Handle the Excel way of doing these */
            {
                multiple = fabs(multiple);

                if(f64 > 0.0)
                {
                    ss_data_set_error(p_ss_data_res, EVAL_ERR_MIXED_SIGNS);
                    return;
                }

                /* rounds towards (or away from) zero, so operate on positive number */
                if(f64 < 0.0)
                {
                    f64 = -f64; /* both are now positive */
                    negate_result = TRUE;
                }
            }

            /* check for divide by zero about to trap */
            if(multiple < F64_MIN)
            {
                ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
                return;
            }

            multiplier = 1.0 / multiple; 
        }

        break;
        }

#if 0 /* just for diff minimization */

    case RPN_FNF_MROUND:
        {
        F64 multiple = fabs(ss_data_get_real(args[1])); /* Unlike Excel, we allow mixed signs for MROUND */

        f64 = ss_data_get_real(args[0]);

        /* rounds towards (or away from) zero, so operate on positive number */
        if(f64 < 0.0)
        {
            f64 = -f64;
            negate_result = TRUE;
        }

        /* check for divide by zero about to trap */
        if(multiple < F64_MIN)
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
            return;
        }

        multiplier = 1.0 / multiple; 
        break;
        }

#endif

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_ROUND:
  /*case RPN_FNF_ROUNDDOWN: diff minimization*/
  /*case RPN_FNF_ROUNDUP: diff minimization*/
  /*case RPN_FNV_TRUNC: diff minimization*/
#endif
        {
        S32 decimal_places = 2;

        if(n_args > 1)
            decimal_places = MIN(15, ss_data_get_integer(args[1]));

#if 0 /* diff minimization */
        else if(RPN_FNV_TRUNC == rpn_did_num)
            decimal_places = 0;
#endif

        switch(ss_data_get_data_id(args[0]))
        {
        case DATA_ID_LOGICAL:
        case DATA_ID_WORD16:
        case DATA_ID_WORD32:
            if(decimal_places >= 0)
            {   /* if we have an integer number to be rounded at, or to the right of, the decimal, it's already there */
                /* NOP */
                *p_ss_data_res = *(args[0]);
                return;
            }

            f64 = (F64) ss_data_get_integer(args[0]); /* have to do it like this */
            break;

        default:
            f64 = ss_data_get_real(args[0]);
            break;
        }

        /* these ones all round towards (or away from) zero, so operate on positive number */
        if(f64 < 0.0)
        {
            f64 = -f64;
            negate_result = TRUE;
        }

        multiplier = pow(10.0, (F64) decimal_places);
        break;
        }
    }

    f64 = f64 * multiplier;

    switch(rpn_did_num)
    {
  /*case RPN_FNF_MROUND: diff minimization*/
    case RPN_FNV_ROUND:
        f64 = floor(f64 + 0.5); /* got a positive number here */
        break;

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_FLOOR:
  /*case RPN_FNF_ROUNDDOWN: diff minimization*/
  /*case RPN_FNV_TRUNC: diff minimization*/
#endif
        f64 = floor(f64);
        break;

    case RPN_FNV_CEILING:
  /*case RPN_FNF_ROUNDUP: diff minimization*/
        f64 = ceil(f64);
        break;
    }

    f64 = f64 / multiplier;

    ss_data_set_real(p_ss_data_res, negate_result ? -f64 : f64);
}

PROC_EXEC_PROTO(c_ceiling)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_CEILING);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

PROC_EXEC_PROTO(c_floor)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_FLOOR);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

#if 0 /* just for diff minimization */

PROC_EXEC_PROTO(c_mround)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNF_MROUND);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

#endif

PROC_EXEC_PROTO(c_round)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

#if 0 /* just for diff minimization */

PROC_EXEC_PROTO(c_rounddown)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNF_ROUNDDOWN);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

PROC_EXEC_PROTO(c_roundup)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNF_ROUNDUP);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

PROC_EXEC_PROTO(c_trunc)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_TRUNC);

    consume_bool(ss_data_real_to_integer_try(p_ss_data_res));
}

/******************************************************************************
*
* NUMBER quotient(numerator:number, denominator:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_quotient)
{
    exec_func_ignore_parms();

    if(!two_nums_divide_try(p_ss_data_res, args[0], args[1], FALSE))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_CALC_FAILURE);

    if(DATA_ID_REAL == ss_data_get_data_id(p_ss_data_res))
    {
        F64 quotient_result;

        /* lose the fractional part */
        (void) modf(ss_data_get_real(p_ss_data_res), &quotient_result);

        ss_data_set_real_try_integer(p_ss_data_res, quotient_result);
    }
}

#endif

/******************************************************************************
*
* NUMBER seriessum(x:number, n:number, m:number, coefficients:array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_seriessum)
{
    const F64 x = ss_data_get_real(args[0]);
    const F64 n = ss_data_get_real(args[1]);
    const F64 m = ss_data_get_real(args[2]);
    const PC_SS_DATA array_coefficients = args[3];
    S32 coefficient_size;
    F64 seriessum_result = 0.0;
#if 1
    /* Use Horner's method to increase accuracy so that terms being summed are generally of increasing magnitude */
    const F64 x_to_power_n = (n == 0.0) ? 1.0 : pow(x, n); /* NB it is useful to define 0^0=1 for polynomials */
    const F64 x_to_power_m = pow(x, m);
    S32 i;

    exec_func_ignore_parms();

    array_range_mono_size(array_coefficients, &coefficient_size);

    /* come down from the highest index */
    for(i = 1; i <= coefficient_size; ++i)
    {
        F64 term;
        SS_DATA ss_data_coefficient;

        (void) array_range_mono_index(&ss_data_coefficient, array_coefficients, coefficient_size - i, EM_REA);

        term = ss_data_get_real(&ss_data_coefficient);

        if(1 == i)
            seriessum_result = term; /* avoids first fma() which are usually expensive */
        else
            seriessum_result = fma(seriessum_result, x_to_power_m, term);
    }

    seriessum_result *= x_to_power_n; /* finish off with a x^n */
#else
    F64 power = n;
    F64 x_to_power = pow(x, power);
    S32 idx;

    exec_func_ignore_parms();

    array_range_mono_size(array_coefficients, &coefficient_size);

    for(idx = 0; idx < coefficient_size; ++idx)
    {
        F64 term;
        SS_DATA ss_data_coefficient;

        (void) array_range_mono_index(&ss_data_coefficient, array_coefficients, idx, EM_REA);

        term = ss_data_coefficient.arg.fp * x_to_power;

        seriessum_result += term;

        /* advance */
        power += m;
        if(m == 1.0)
            x_to_power *= x;
        else if(m == 2.0)
            x_to_power *= (x * x);
        else
            x_to_power = pow(x, power);
    }
#endif

    ss_data_set_real(p_ss_data_res, seriessum_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* NUMBER sumproduct(array_1, array_2, ...)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sumproduct)
{
    F64 sum = 0.0;
    S32 x_size, y_size;
    S32 i;

    exec_func_ignore_parms();

    /* ensure all arrays are the same size */
    array_range_sizes(args[0], &x_size, &y_size);

    for(i = 1; i < n_args; ++i)
    {
        const PC_SS_DATA array_i = args[i];
        S32 x_size_i, y_size_i;
        array_range_sizes(array_i, &x_size_i, &y_size_i);
        if( (x_size != x_size_i) || (y_size != y_size_i) )
        {
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_VALUE);
        }
    }

    { /* for each element in all arrays, calculate the product */
    S32 ix, iy;

    for(iy = 0; iy < y_size; ++iy)
    {
        for(ix = 0; ix < x_size; ++ix)
        {
            F64 product = 1.0;

            for(i = 0; i < n_args; ++i)
            {
                const PC_SS_DATA array_i = args[i];
                SS_DATA ss_data;

                (void) array_range_index(&ss_data, array_i, ix, iy, EM_REA);

                product *= ss_data_get_real(&ss_data);
            }

            /* all products of {arrays}[i][j] are added to the total */
            sum += product;
        }
    }
    } /*block*/

    ss_data_set_real_try_integer(p_ss_data_res, sum);
}

/******************************************************************************
*
* NUMBER sum_x2my2(array_x, array_y)
*
* NUMBER sum_x2py2(array_x, array_y)
*
******************************************************************************/

static void
sumx2opy2_common(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA array_x,
    _InRef_     PC_SS_DATA array_y,
    _InVal_     BOOL subtract_y2)
{
    STATUS status = STATUS_OK;
    S32 x_size[2];
    S32 y_size[2];

    array_range_sizes(array_x, &x_size[0], &y_size[0]);
    array_range_sizes(array_y, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
        status = EVAL_ERR_ODF_NA;
    else if((0 == x_size[0]) || (0 == y_size[0]))
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 sum = 0.0;
        S32 ix, iy;

        for(iy = 0; iy < y_size[0]; ++iy)
        {
            for(ix = 0; ix < x_size[0]; ++ix)
            {
                SS_DATA ss_data_x, ss_data_y;
                F64 x2, y2;

                (void) array_range_index(&ss_data_x, array_x, ix, iy, EM_REA);
                (void) array_range_index(&ss_data_y, array_y, ix, iy, EM_REA);

                x2 = ss_data_get_real(&ss_data_x) * ss_data_get_real(&ss_data_x);
                y2 = ss_data_get_real(&ss_data_y) * ss_data_get_real(&ss_data_y);

                if(subtract_y2)
                    y2 = -y2;

                sum += (x2 + y2);
            }
        }

        ss_data_set_real_try_integer(p_ss_data_res, sum);
    }

    exec_func_status_return(p_ss_data_res, status);
}

PROC_EXEC_PROTO(c_sum_x2my2)
{
    const PC_SS_DATA array_x = args[0];
    const PC_SS_DATA array_y = args[1];

    exec_func_ignore_parms();

    sumx2opy2_common(p_ss_data_res, array_x, array_y, TRUE); /* sum(x^2 - y^2) */
}

PROC_EXEC_PROTO(c_sum_x2py2)
{
    const PC_SS_DATA array_x = args[0];
    const PC_SS_DATA array_y = args[1];

    exec_func_ignore_parms();

    sumx2opy2_common(p_ss_data_res, array_x, array_y, FALSE); /* sum(x^2 + y^2) */
}

/******************************************************************************
*
* NUMBER sum_xmy2(array_x, array_y)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum_xmy2)
{
    STATUS status = STATUS_OK;
    const PC_SS_DATA array_x = args[0];
    const PC_SS_DATA array_y = args[1];
    S32 x_size[2];
    S32 y_size[2];

    exec_func_ignore_parms();

    array_range_sizes(array_x, &x_size[0], &y_size[0]);
    array_range_sizes(array_y, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
        status = EVAL_ERR_ODF_NA;
    else if((0 == x_size[0]) || (0 == y_size[0]))
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 sum = 0.0;
        S32 ix, iy;

        for(iy = 0; iy < y_size[0]; ++iy)
        {
            for(ix = 0; ix < x_size[0]; ++ix)
            {
                SS_DATA ss_data_x, ss_data_y;
                F64 difference;

                (void) array_range_index(&ss_data_x, array_x, ix, iy, EM_REA);
                (void) array_range_index(&ss_data_y, array_y, ix, iy, EM_REA);

                difference = ss_data_get_real(&ss_data_x) - ss_data_get_real(&ss_data_y);

                sum += (difference*difference);
            }
        }

        ss_data_set_real_try_integer(p_ss_data_res, sum);
    }

    exec_func_status_return(p_ss_data_res, status);
}

#endif

/* end of ev_matb.c */
