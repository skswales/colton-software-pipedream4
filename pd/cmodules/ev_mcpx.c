/* ev_mcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions for evaluator */

/* MRJC May 1991; SKS May 2014 split off from ev_math; SKS December 2019 split off complex core functions */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"

#include "cmodules/mathxcpx.h"

/* local header file */
#include "ev_evali.h"

#ifndef MATHXCPX_H

/*
declare complex number type for internal usage
*/

typedef struct COMPLEX
{
    F64 r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

#endif /* MATHXCPX_H */

/*
internal functions
*/

static void
complex_result_reals(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part);

#if defined(COMPLEX_STRING)

static void
complex_result_reals_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part,
    _InVal_     U8 imag_suffix_char);

#endif

/******************************************************************************
*
* check that arg is a 2 by 1 array of two real numbers (or one real)
*
* --out--
* <0  arg data was unsuitable
* >=0 COMPLEX z was set up with numbers
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_arg_array(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _InRef_     P_SS_DATA p_ss_data_in)
{
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
    case DATA_ID_RANGE:
        { /* extract elements from the array */
        SS_DATA ss_data_1, ss_data_2;
        const EV_IDNO t1 = array_range_index(&ss_data_1, p_ss_data_in, 0, 0, EM_REA);
        const EV_IDNO t2 = array_range_index(&ss_data_2, p_ss_data_in, 1, 0, EM_REA);

        if( (DATA_ID_REAL == t1) && (DATA_ID_REAL == t2) )
        {
            complex_set_ri(z, ss_data_get_real(&ss_data_1), ss_data_get_real(&ss_data_2));
            return(STATUS_OK);
        }

        break;
        }

    default:
        if(ss_data_is_real(p_ss_data_in))
        {
            complex_set_ri(z, ss_data_get_real(p_ss_data_in), 0.0);
            return(STATUS_OK);
        }

        break;
    }

    *z = complex_zero;
    return(ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX));
}

#if defined(COMPLEX_STRING)

/******************************************************************************
*
* check that a string is suitable for use as Excel-style complex number
*
* --out--
* <0  string was unsuitable
* >=0 COMPLEX z was set up with numbers
*
* parse <real>[+|-]<imag>[i|j], which may be just
* <real> or
* <imag>[i|j] or even
* {[+|-]}[i|j]
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_arg_ustr(
    _InoutRef_  P_COMPLEX z,
    _InoutRef_  P_U8 p_imag_suffix_char,
    _In_z_      PC_USTR ustr)
{
    F64 real_part = 0.0;
    F64 imag_part = 0.0;
    BOOL negate = FALSE;
    F64 f64;
    U32 uchars_read;
    PC_USTR new_ustr;

    /* read a number */
    f64 = ustrtod(ustr, &new_ustr);
    uchars_read = PtrDiffBytesU32(new_ustr, ustr);

    if(0 == uchars_read)
    {   /* no number read - but we don't need a number when fed just "i" or "-i" */
        if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        {
            negate = TRUE;
            ustr_IncByte(ustr);
        }
        else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        {
            negate = FALSE;
            ustr_IncByte(ustr);
        }
        else
            negate = FALSE;
    }
    else
    {   /* read a number OK */

        if(CH_NULL == PtrGetByteOff(ustr, uchars_read))
        {   /* early exit, got just a real part */
            real_part = f64;
            complex_set_ri(z, real_part, 0.0);
            return(STATUS_OK);
        }

        ustr = new_ustr;
    }

    if((PtrGetByte(ustr) == 'i') || (PtrGetByte(ustr) == 'j'))
    {
        if(CH_NULL != PtrGetByteOff(ustr, 1))
            return(EVAL_ERR_BADCOMPLEX);
        *p_imag_suffix_char = PtrGetByte(ustr);
        if(0 != uchars_read)
            imag_part = f64; /* <imag>[i|j] */
        else
            imag_part = negate ? -1.0 : 1.0; /* {[+|-]}[i|j] */
        complex_set_ri(z, 0.0, imag_part);
        return(STATUS_OK);
    }

    if(0 == uchars_read)
        return(EVAL_ERR_BADCOMPLEX);

    /* read a number OK - that number must be the real part then */
    real_part = f64;

    /* and must be followed by [+|-]{number}i */
    if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        negate = TRUE;
    else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        negate = FALSE;
    else
        return(EVAL_ERR_BADCOMPLEX);

    /* read another number */
    f64 = ustrtod(ustr, &new_ustr);
    uchars_read = PtrDiffBytesU32(new_ustr, ustr);

    if(0 == uchars_read)
    {   /* no number read - but we don't need another number when fed just "<real>[+|]i" */
        if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        {
            negate = TRUE;
            ustr_IncByte(ustr);
        }
        else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        {
            negate = FALSE;
            ustr_IncByte(ustr);
        }
        else
            return(EVAL_ERR_BADCOMPLEX);
    }
    else
    {   /* read another number OK - that number must be the imag part then */
        ustr = new_ustr;
    }

    if((PtrGetByte(ustr) == 'i') || (PtrGetByte(ustr) == 'j'))
    {
        if(CH_NULL != PtrGetByteOff(ustr, 1))
            return(EVAL_ERR_BADCOMPLEX);
        *p_imag_suffix_char = PtrGetByte(ustr);
        if(0 != uchars_read)
            imag_part = f64; /* <real>[+|-]<imag>[i|j] */
        else
            imag_part = negate ? -1.0 : 1.0; /* <real>[+|-][i|j] */
        complex_set_ri(z, real_part, imag_part);
        return(STATUS_OK);
    }

    return(EVAL_ERR_BADCOMPLEX);
}

_Check_return_
static STATUS
complex_check_arg_string(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_in)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    *z = complex_zero;
    *p_imag_suffix_char = CH_NULL;

    if(!ss_data_is_string(p_ss_data_in))
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX));

    /* need a CH_NULL-terminated string to parse */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock, ss_data_get_string(p_ss_data_in), ss_data_get_string_size(p_ss_data_in))))
        status = quick_ublock_nullch_add(&quick_ublock);

    if(status_ok(status))
        status = complex_check_arg_ustr(z, p_imag_suffix_char, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    if(status_fail(status))
        return(ss_data_set_error(p_ss_data_res, status));

    return(STATUS_OK);
}

_Check_return_
static STATUS
complex_check_arg_string_pair(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z1,
    _OutRef_    P_COMPLEX z2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_1,
    _InRef_     P_SS_DATA p_ss_data_2)
{
    U8 imag_suffix_char_1, imag_suffix_char_2;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    *p_imag_suffix_char = CH_NULL;

    status_return(complex_check_arg_string(p_ss_data_res, z1, &imag_suffix_char_1, p_ss_data_1));
    status_return(complex_check_arg_string(p_ss_data_res, z2, &imag_suffix_char_2, p_ss_data_2));

    if((CH_NULL == imag_suffix_char_1) && (CH_NULL == imag_suffix_char_2))
         *p_imag_suffix_char = 'i';
    else if(imag_suffix_char_1 == imag_suffix_char_2)
        *p_imag_suffix_char = imag_suffix_char_1;
    else if(CH_NULL == imag_suffix_char_1)
        *p_imag_suffix_char = imag_suffix_char_2;
    else if(CH_NULL == imag_suffix_char_2)
        *p_imag_suffix_char = imag_suffix_char_1;
    else /* (imag_suffix_char_1 != imag_suffix_char_2) */
#if 1
        *p_imag_suffix_char = 'i';
#else
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_MIXED_SUFFIXES));
#endif

    return(STATUS_OK);
}

#endif /* COMPLEX_STRING */

_Check_return_
static STATUS
complex_check_arg(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data)
{
#if defined(COMPLEX_STRING)
    if(ss_data_is_string(p_ss_data))
        return(complex_check_arg_string(p_ss_data_res, z, p_imag_suffix_char, p_ss_data));
#endif /* COMPLEX_STRING */

    *p_imag_suffix_char = CH_NULL;

    return(complex_check_arg_array(p_ss_data_res, z, p_ss_data));
}

_Check_return_
static STATUS
complex_check_arg_pair(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z1,
    _OutRef_    P_COMPLEX z2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_1,
    _InRef_     P_SS_DATA p_ss_data_2)
{
#if defined(COMPLEX_STRING)
    /* easy if both are strings */
    if( ss_data_is_string(p_ss_data_1) && ss_data_is_string(p_ss_data_2) )
        return(complex_check_arg_string_pair(p_ss_data_res, z1, z2, p_imag_suffix_char, p_ss_data_1, p_ss_data_2));

    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    if(ss_data_is_string(p_ss_data_1))
        status_return(complex_check_arg_string(p_ss_data_res, z1, p_imag_suffix_char, p_ss_data_1));
    else
        status_return(complex_check_arg_array(p_ss_data_res, z1, p_ss_data_1));

    if(ss_data_is_string(p_ss_data_2))
        return(complex_check_arg_string(p_ss_data_res, z2, p_imag_suffix_char, p_ss_data_2));
    else
        return(complex_check_arg_array(p_ss_data_res, z2, p_ss_data_2));
#else
    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    status_return(complex_check_arg_array(p_ss_data_res, z1, p_ss_data_1));
           return(complex_check_arg_array(p_ss_data_res, z2, p_ss_data_2));
#endif /* COMPLEX_STRING */
}

/******************************************************************************
*
* make a complex number result from an internal complex number type
*
******************************************************************************/

static void
complex_result_complex(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
#if defined(COMPLEX_STRING)
    if(CH_NULL != imag_suffix_char)
    {
        complex_result_reals_string(p_ss_data, complex_real(z), complex_imag(z), imag_suffix_char);
        return;
    }
#else
    UNREFERENCED_PARAMETER_InVal_(imag_suffix_char);
#endif

    complex_result_reals(p_ss_data, complex_real(z), complex_imag(z));
}

/******************************************************************************
*
* make a complex number result from processing an internal complex number type
*
******************************************************************************/

static void
complex_result_complex_one_arg(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char,
    COMPLEX (*fn) (_InRef_ PC_COMPLEX z) )
{
    COMPLEX fn_result = (*fn) (z);

    /* output a complex number */
    complex_result_complex(p_ss_data, &fn_result, imag_suffix_char);
}

/******************************************************************************
*
* make a complex number result from processing an internal complex number type
*
******************************************************************************/

static void
complex_result_complex_two_args(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _InVal_     U8 imag_suffix_char,
    COMPLEX (*fn) (_InRef_ PC_COMPLEX z1, _InRef_ PC_COMPLEX z2) )
{
    COMPLEX fn_result = (*fn) (z1, z2);

    /* output a complex number */
    complex_result_complex(p_ss_data, &fn_result, imag_suffix_char);
}

/******************************************************************************
*
* make a complex number result from the given complex number
*
******************************************************************************/

static void
complex_result_reals(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part)
{
    P_SS_DATA elep;

    if(status_fail(ss_array_make(p_ss_data, 2, 1)))
        return;

    elep = ss_array_element_index_wr(p_ss_data, 0, 0);
    ss_data_set_real(elep, real_part);

    elep = ss_array_element_index_wr(p_ss_data, 1, 0);
    ss_data_set_real(elep, imag_part);
}

#if defined(COMPLEX_STRING)

#if defined(UNUSED_KEEP_ALIVE)

static inline void
complex_result_complex_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
    complex_result_reals_string(p_ss_data, complex_real(z), complex_imag(z), imag_suffix_char);
}

#endif /* UNUSED */

static inline U32 /* bytes currently in output */
decode_fp(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     F64 f64)
{
    U32 len = ustr_xsnprintf(ustr_buf, elemof_buffer, USTR_TEXT("%.15g"), f64);
    P_U8 exp;

    /* search for exponent and remove leading zeros because they are confusing */
    /* also remove the + for good measure */
    if(NULL != (exp = strchr((char *) ustr_buf, 'e')))
    {
        U8 sign = *(++exp);
        P_U8 exps;

        if(CH_MINUS_SIGN__BASIC == sign)
            ++exp;

        exps = exp;

        if(CH_PLUS_SIGN == sign)
            ++exp;

        while(CH_DIGIT_ZERO == *exp)
            ++exp;

        if(exp != exps)
        {
            memmove32(exps, exp, strlen32p1(exp) /*CH_NULL*/);

            len -= PtrDiffBytesU32(exp, exps);
        }
    }

    return(len);
}

static void
complex_result_reals_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part,
    _InVal_     U8 imag_suffix_char)
{
    STATUS status = STATUS_OK;
    BOOL real_zero = real_part == 0.0;
  /*BOOL real_negative = real_part < 0.0;*/
  /*F64 real_abs = fabs(real_part);*/
  /*U8 real_sign_char = real_negative ? CH_MINUS_SIGN__BASIC : CH_PLUS_SIGN;*/
    BOOL imag_zero = imag_part == 0.0;
    BOOL imag_negative = imag_part < 0.0;
    F64 imag_abs = fabs(imag_part);
    U8 imag_sign_char = imag_negative ? CH_MINUS_SIGN__BASIC : CH_PLUS_SIGN;
    UCHARZ fp_buffer[64];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    /* always output real part first if it is non-zero */
    if(!real_zero)
    {
        const U32 len = decode_fp(ustr_bptr(fp_buffer), elemof32(fp_buffer), real_part); /* including sign of real part */
        status = quick_ublock_uchars_add(&quick_ublock, ustr_bptr(fp_buffer), len);
    }

    if(imag_zero)
    {
        if(real_zero)
            status = quick_ublock_a7char_add(&quick_ublock, CH_DIGIT_ZERO);
    }
    else
    {
        /* append [+|-]<imag_abs>[i|j] */
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_sign_char);
        if( status_ok(status) && (1.0 != imag_abs) )
        {
            const U32 len = decode_fp(ustr_bptr(fp_buffer), elemof32(fp_buffer), imag_abs);
            status = quick_ublock_uchars_add(&quick_ublock, ustr_bptr(fp_buffer), len);
        }
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_suffix_char);
    }

    if(status_fail(status))
        ss_data_set_error(p_ss_data, status);
    else
        status_assert(ss_string_make_uchars(p_ss_data, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

    quick_ublock_dispose(&quick_ublock);
}

#endif /* COMPLEX_STRING */

/******************************************************************************
*
* COMPLEX process a single complex number
*
******************************************************************************/

static void
complex_process_complex_one_arg(
    _OutRef_    P_SS_DATA p_ss_data_res,
    COMPLEX (*fn) (_InRef_ PC_COMPLEX z),
    _InRef_     P_P_SS_DATA args /*[]*/)
{
    U8 imag_suffix_char;
    COMPLEX z;

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    /* output a complex number */
    complex_result_complex_one_arg(p_ss_data_res, &z, imag_suffix_char, fn);
}

/******************************************************************************
*
* COMPLEX process a pair of complex numbers
*
******************************************************************************/

static void
complex_process_complex_two_args(
    _OutRef_    P_SS_DATA p_ss_data_res,
    COMPLEX (*fn) (_InRef_ PC_COMPLEX z1, _InRef_ PC_COMPLEX z2),
    _InRef_     P_P_SS_DATA args /*[]*/)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2;

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    /* output a complex number */
    complex_result_complex_two_args(p_ss_data_res, &z1, &z2, imag_suffix_char, fn);
}

/******************************************************************************
*
* add two complex numbers
*
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_add)
{
    exec_func_ignore_parms();

    complex_process_complex_two_args(p_ss_data_res, complex_add, args);
}

/******************************************************************************
*
* subtract second complex number from first
*
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sub)
{
    exec_func_ignore_parms();

    complex_process_complex_two_args(p_ss_data_res, complex_subtract, args);
}

/******************************************************************************
*
* multiply two complex numbers
*
* (a+ib) * (c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_mul)
{
    exec_func_ignore_parms();

    complex_process_complex_two_args(p_ss_data_res, complex_multiply, args);
}

/******************************************************************************
*
* divide two complex numbers
*
* (a+ib) / (c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_div)
{
    exec_func_ignore_parms();

    complex_process_complex_two_args(p_ss_data_res, complex_divide, args);
}

#if defined(UNUSED_IN_PD) /* just for diff minimization */

/******************************************************************************
*
* COMPLEX c_complex(real_part:number {, imag_part:number {, suffix:string}})
*
* STRING odf.complex(real_part:number, imag_part:number {, suffix:string})
*
******************************************************************************/

static void
c_c_complex_common(
    _In_reads_(n_args) P_SS_DATA args[],
    _InVal_     S32 n_args,
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     U8 default_imag_suffix_char)
{
    COMPLEX complex_result;
    U8 imag_suffix_char = default_imag_suffix_char;

    assert(n_args >= 1);
    complex_set_ri(&complex_result, ss_data_get_real(args[0]), (n_args > 1) ? ss_data_get_real(args[1]) : 0.0);

    if(n_args > 2)
    {
        const PC_UCHARS uchars = ss_data_get_string(args[2]);

        if( (1 != ss_data_get_string_size(args[2])) || ((PtrGetByte(uchars) != 'i') && (PtrGetByte(uchars) != 'j')) )
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX);
            return;
        }

        imag_suffix_char = PtrGetByte(uchars);
    }

    complex_result_complex(p_ss_data_res, &complex_result, imag_suffix_char);
}

PROC_EXEC_PROTO(c_c_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ss_data_res, CH_NULL /* only a string result if suffix supplied */);
}

PROC_EXEC_PROTO(c_odf_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ss_data_res, 'i' /* always a string result, may be overridden */);
}

/******************************************************************************
*
* COMPLEX find complex conjugate of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_conjugate)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_conjugate, args[0]);
}

/******************************************************************************
*
* REAL find real part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_real)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    ss_data_set_real(p_ss_data_res, complex_real(&z));
}

/******************************************************************************
*
* REAL find imaginary part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_imaginary)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    ss_data_set_real(p_ss_data_res, complex_imag(&z));
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* REAL find radius of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_radius)
{
    U8 imag_suffix_char;
    COMPLEX z;
    F64 radius_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    radius_result = complex_modulus(&z);

    ss_data_set_real(p_ss_data_res, radius_result);
}

/******************************************************************************
*
* REAL find theta of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_theta)
{
    U8 imag_suffix_char;
    COMPLEX z;
    F64 theta_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    theta_result = complex_argument(&z);

    ss_data_set_real(p_ss_data_res, theta_result);
}

/******************************************************************************
*
* c_round(complex number {, decimal_places:number=2})
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_round)
{
    U8 imag_suffix_char;
    COMPLEX z, round_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    { /* round components separately to the desired number of decimal places */
    F64 round_result_real, round_result_imag;
    SS_DATA number, decimal_places;
    P_SS_DATA local_args[2];

    local_args[0] = &number;
    local_args[1] = &decimal_places;

    ss_data_set_integer_fn(&decimal_places,
        (n_args > 1)
            ? MIN(15, ss_data_get_integer(args[1]))
            : 2);

    ss_data_set_real(&number, complex_real(&z));
    round_common(local_args, 2, p_ss_data_res, RPN_FNV_ROUND);
    assert(ss_data_is_real(p_ss_data_res));
    round_result_real = ss_data_get_real(p_ss_data_res);

    ss_data_set_real(&number, complex_imag(&z));
    round_common(local_args, 2, p_ss_data_res, RPN_FNV_ROUND);
    assert(ss_data_is_real(p_ss_data_res));
    round_result_imag = ss_data_get_real(p_ss_data_res);

    complex_set_ri(&round_result, round_result_real, round_result_imag);
    } /*block*/

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &round_result, imag_suffix_char);
}

/******************************************************************************
*
* c_exp(complex number) - complex exponent
*
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_exp)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_exp, args);
}

/******************************************************************************
*
* c_ln(complex number) - complex natural logarithm
*
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_ln)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_log_e, args);
}

#if defined(UNUSED_IN_PD) /* just for diff minimization */

/******************************************************************************
*
* c_log_10(complex number)
*
* z = r.e^i.theta -> ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_log_10)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_log_10, args);
}

/******************************************************************************
*
* c_log_2(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_log_2)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_log_2, args);
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* c_power(complex number) - complex z^w
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_power)
{
    exec_func_ignore_parms();

    complex_process_complex_two_args(p_ss_data_res, complex_power, args);
}

#if defined(UNUSED_IN_PD) /* just for diff minimization */

/******************************************************************************
*
* c_sqrt(complex number) - complex square root
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sqrt)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_square_root, args);
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* c_cos(complex number) - complex cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cos)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_cosine, args);
}

/******************************************************************************
*
* c_acos(complex number) - complex arc cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acos)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_cosine, args);
}

/******************************************************************************
*
* c_sin(complex number) - complex sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sin)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_sine, args);
}

/******************************************************************************
*
* c_asin(complex number) - complex arc sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asin)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_sine, args);
}

/******************************************************************************
*
* c_tan(complex number) - complex tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tan)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_tangent, args);
}

/******************************************************************************
*
* c_atan(complex number) - complex arc tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_atan)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_tangent, args);
}

/******************************************************************************
*
* c_cosec(complex number) - complex cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosec)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_cosecant, args);
}

/******************************************************************************
*
* c_acosec(complex number) - complex arc cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosec)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_cosecant, args);
}

/******************************************************************************
*
* c_sec(complex number) - complex secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sec)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_secant, args);
}

/******************************************************************************
*
* c_asec(complex number) - complex arc secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asec)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_secant, args);
}

/******************************************************************************
*
* c_cot(complex number) - complex cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cot)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_cotangent, args);
}

/******************************************************************************
*
* c_acot(complex number) - complex arc cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acot)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_arc_cotangent, args);
}

/******************************************************************************
*
* c_cosh(complex number) - complex hyperbolic cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_cosine, args);
}

/******************************************************************************
*
* c_acosh(complex number) - complex inverse hyperbolic cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_cosine, args);
}

/******************************************************************************
*
* c_sinh(complex number) - complex hyperbolic sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sinh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_sine, args);
}

/******************************************************************************
*
* c_asinh(complex number) - complex inverse hyperbolic sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asinh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_sine, args);
}

/******************************************************************************
*
* c_tanh(complex number) - complex hyperbolic tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tanh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_tangent, args);
}

/******************************************************************************
*
* c_atanh(complex number) - complex inverse hyperbolic tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_atanh)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_tangent, args);
}

/******************************************************************************
*
* c_cosech(complex number) - complex hyperbolic cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosech)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_cosecant, args);
}

/******************************************************************************
*
* c_acosech(complex number) - complex inverse hyperbolic cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosech)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_cosecant, args);
}

/******************************************************************************
*
* c_sech(complex number) - complex hyperbolic secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sech)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_secant, args);
}

/******************************************************************************
*
* c_asech(complex number) - complex inverse hyperbolic secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asech)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_secant, args);
}

/******************************************************************************
*
* c_coth(complex number) - complex hyperbolic cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_coth)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_hyperbolic_cotangent, args);
}

/******************************************************************************
*
* c_acoth(complex number) - complex inverse hyperbolic cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acoth)
{
    exec_func_ignore_parms();

    complex_process_complex_one_arg(p_ss_data_res, complex_inverse_hyperbolic_cotangent, args);
}

/* end of ev_mcpx.c */
