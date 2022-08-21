/* ev_mcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Semantic routines for evaluator */

/* MRJC May 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"

/* local header file */
#include "ev_evali.h"

#include <math.h>
#include <float.h>
#include <errno.h>

/*
declare complex number type for internal usage
*/

typedef struct COMPLEX
{
    F64 r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

/*
internal functions
*/

static void
complex_result_reals(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part);

/******************************************************************************
*
* the constant one as a complex number
*
******************************************************************************/

static const COMPLEX
complex_unity = { 1.0, 0.0 };

/******************************************************************************
*
* check that an array is a 2 by 1 array of two real numbers
*
* --out--
* <0  array was unsuitable
* >=0 n->r and n->i set up with numbers
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_array(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n,
    _InRef_     P_EV_DATA p_ev_data_in)
{
    if(p_ev_data_in->did_num == RPN_DAT_REAL)
    {
        n->r = p_ev_data_in->arg.fp;
        n->i = 0.0;
    }
    else
    {
        EV_DATA ev_data1, ev_data2;

        /* extract elements from the array */
        EV_IDNO t1 = array_range_index(&ev_data1, p_ev_data_in, 0, 0, EM_REA);
        EV_IDNO t2 = array_range_index(&ev_data2, p_ev_data_in, 1, 0, EM_REA);

        if((t1 != RPN_DAT_REAL) || (t2 != RPN_DAT_REAL))
        {
            n->r = n->i = 0.0;
            return(ev_data_set_error(p_ev_data_res, EVAL_ERR_BADCOMPLEX));
        }

        n->r = ev_data1.arg.fp;
        n->i = ev_data2.arg.fp;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* find ln of complex number for internal routines
*
******************************************************************************/

/* NB z = r.e^i.theta -> ln(z) = ln(r) + i.theta */

_Check_return_
static STATUS
complex_lnz(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    const F64 r = in->r * in->r + in->i * in->i;
    STATUS status = STATUS_OK;

    errno = 0;

    out->r = log(r) / 2.0;

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    out->i = atan2(in->i, in->r);

    return(status);
}

/******************************************************************************
*
* make a complex number result array from
* an internal complex number type
*
******************************************************************************/

static inline void
complex_result_complex(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_COMPLEX z)
{
    complex_result_reals(p_ev_data, z->r, z->i);
}

/******************************************************************************
*
* make a complex number array in the given
* data structure and give it a complex number
*
******************************************************************************/

static void
complex_result_reals(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part)
{
    P_EV_DATA elep;

    if(status_fail(ss_array_make(p_ev_data, 2, 1)))
        return;

    elep = ss_array_element_index_wr(p_ev_data, 0, 0);
    ev_data_set_real(elep, real_part);

    elep = ss_array_element_index_wr(p_ev_data, 1, 0);
    ev_data_set_real(elep, imag_part);
}

/******************************************************************************
*
* find w*ln(z) for internal routines
*
******************************************************************************/

_Check_return_
static STATUS
complex_wlnz(
    _InRef_     PC_COMPLEX w,
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX out)
{
    COMPLEX lnz;

    status_return(complex_lnz(z, &lnz));

    out->r = w->r * lnz.r  -  w->i * lnz.i;
    out->i = w->r * lnz.i  +  w->i * lnz.r;

    return(STATUS_OK);
}

/******************************************************************************
*
* add two complex numbers
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_add)
{
    COMPLEX in1, in2, add_result;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(status_fail(complex_check_array(p_ev_data_res, &in1, args[0])))
        return;
    if(status_fail(complex_check_array(p_ev_data_res, &in2, args[1])))
        return;

    add_result.r = in1.r + in2.r;
    add_result.i = in1.i + in2.i;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &add_result);
}

/******************************************************************************
*
* subtract second complex number from first
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sub)
{
    COMPLEX in1, in2, sub_result;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(status_fail(complex_check_array(p_ev_data_res, &in1, args[0])))
        return;
    if(status_fail(complex_check_array(p_ev_data_res, &in2, args[1])))
        return;

    sub_result.r = in1.r - in2.r;
    sub_result.i = in1.i - in2.i;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &sub_result);
}

/******************************************************************************
*
* multiply two complex numbers
* (a+ib)*(c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_mul)
{
    COMPLEX in1, in2, mul_result;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(status_fail(complex_check_array(p_ev_data_res, &in1, args[0])))
        return;
    if(status_fail(complex_check_array(p_ev_data_res, &in2, args[1])))
        return;

    mul_result.r = in1.r * in2.r - in1.i * in2.i;
    mul_result.i = in1.i * in2.r + in1.r * in2.i;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &mul_result);
}

/******************************************************************************
*
* divide two complex numbers
* (a+ib)/(c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

_Check_return_
static BOOL
do_complex_divide(
    /**/        P_EV_DATA p_ev_data_res,
    _InRef_     PC_COMPLEX in1,
    _InRef_     PC_COMPLEX in2,
    _OutRef_    P_COMPLEX out)
{
    /* c*c + d*d */
    const F64 divisor = (in2->r * in2->r) + (in2->i * in2->i);

    /* check for divide by 0 about to trap */
    if(divisor < F64_MIN)
        {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return(FALSE);
        }

    out->r = (in1->r * in2->r + in1->i * in2->i) / divisor;
    out->i = (in1->i * in2->r - in1->r * in2->i) / divisor;

    return(TRUE);
}

PROC_EXEC_PROTO(c_c_div)
{
    COMPLEX in1, in2, div_result;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(status_fail(complex_check_array(p_ev_data_res, &in1, args[0])))
        return;
    if(status_fail(complex_check_array(p_ev_data_res, &in2, args[1])))
        return;

    if(!do_complex_divide(p_ev_data_res, &in1, &in2, &div_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &div_result);
}

/******************************************************************************
*
* REAL find radius of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_radius)
{
    COMPLEX in;
    F64 radius_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    radius_result = mx_fhypot(in.r, in.i); /* SKS does carefully */

    ev_data_set_real(p_ev_data_res, radius_result);
}

/******************************************************************************
*
* REAL find theta of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_theta)
{
    COMPLEX in;
    F64 theta_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    theta_result = atan2(in.i, in.r); /* note silly C library ordering */

    ev_data_set_real(p_ev_data_res, theta_result);
}

/******************************************************************************
*
* ln(complex number)
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_ln)
{
    COMPLEX in, ln_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(status_fail(status = complex_lnz(&in, &ln_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &ln_result);
}

/******************************************************************************
*
* complex z^w
*
******************************************************************************/

_Check_return_
static STATUS
do_complex_power(
    _InRef_     PC_COMPLEX in1,
    _InRef_     PC_COMPLEX in2,
    _OutRef_    P_COMPLEX out)
{
    COMPLEX wlnz;

    /* find and check wlnz */
    status_return(complex_wlnz(in2, in1, &wlnz));

    out->r = exp(wlnz.r) * cos(wlnz.i);
    out->i = exp(wlnz.r) * sin(wlnz.i);

    return(STATUS_OK);
}

PROC_EXEC_PROTO(c_c_power)
{
    COMPLEX in1, in2, power_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(status_fail(complex_check_array(p_ev_data_res, &in1, args[0])))
        return;
    if(status_fail(complex_check_array(p_ev_data_res, &in2, args[1])))
        return;

    if(status_fail(status = do_complex_power(&in1, &in2, &power_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &power_result);
}

/******************************************************************************
*
* exp(complex number)
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_exp)
{
    COMPLEX in, exp_result;
    F64 ea;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    /* make exp(a) */
    ea = exp(in.r);

    exp_result.r = ea * cos(in.i);
    exp_result.i = ea * sin(in.i);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &exp_result);
}

/******************************************************************************
*
* sin(complex number)
* sin(a+ib) = (exp(-b)+exp(b))sin(a)/2 + i((exp(b)-exp(-b))cos(a)/2)
*
******************************************************************************/

static void
do_complex_sin(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(b) and exp(-b) */
    const F64 eb =  exp(in->i);
    const F64 emb = 1.0 / eb;

    out->r = (eb + emb) * sin(in->r) / 2.0;
    out->i = (eb - emb) * cos(in->r) / 2.0;
}

PROC_EXEC_PROTO(c_c_sin)
{
    COMPLEX in, sin_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sin(&in, &sin_result);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &sin_result);
}

/******************************************************************************
*
* cos(complex number)
* cos(a+ib) = (exp(-b)+exp(b))cos(a)/2 + i((exp(-b)-exp(b))sin(a)/2)
*
******************************************************************************/

static void
do_complex_cos(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(b) and exp(-b) */
    const F64 eb =  exp(in->i);
    const F64 emb = 1.0 / eb;

    out->r = (emb + eb) * cos(in->r) / 2.0;
    out->i = (emb - eb) * sin(in->r) / 2.0;
}

PROC_EXEC_PROTO(c_c_cos)
{
    COMPLEX in, cos_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_cos(&in, &cos_result);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &cos_result);
}

/******************************************************************************
*
* tan(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tan)
{
    COMPLEX in, sin, cos, tan_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &tan_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &tan_result);
}

/******************************************************************************
*
* cosec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosec)
{
    COMPLEX in, temp, cosec_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sin(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &cosec_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &cosec_result);
}

/******************************************************************************
*
* sec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sec)
{
    COMPLEX in, temp, sec_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_cos(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &sec_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &sec_result);
}

/******************************************************************************
*
* cot(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cot)
{
    COMPLEX in, sin, cos, cot_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &cot_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &cot_result);
}

/******************************************************************************
*
* sinh(complex number)
* sinh(a+ib) = (exp(a)-exp(-a))cos(b)/2 + i((exp(a)+exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_sinh(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(a) and exp(-a) */
    const F64 ea = exp(in->r);
    const F64 ema = 1.0 / ea;

    out->r = (ea - ema) * cos(in->i) / 2.0;
    out->i = (ea + ema) * sin(in->i) / 2.0;
}

PROC_EXEC_PROTO(c_c_sinh)
{
    COMPLEX in, sinh_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sinh(&in, &sinh_result);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &sinh_result);
}

/******************************************************************************
*
* cosh(complex number)
* cosh(a+ib) = (exp(a)+exp(-a))cos(b)/2 + i((exp(a)-exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_cosh(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(a) and exp(-a) */
    const F64 ea = exp(in->r);
    const F64 ema = 1.0 / ea;

    out->r = (ea + ema) * cos(in->i) / 2.0;
    out->i = (ea - ema) * sin(in->i) / 2.0;
}

PROC_EXEC_PROTO(c_c_cosh)
{
    COMPLEX in, cosh_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_cosh(&in, &cosh_result);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &cosh_result);
}

/******************************************************************************
*
* tanh(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tanh)
{
    COMPLEX in, sin, cos, tanh_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &tanh_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &tanh_result);
}

/******************************************************************************
*
* cosech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosech)
{
    COMPLEX in, temp, cosech_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sinh(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &cosech_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &cosech_result);
}

/******************************************************************************
*
* sech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sech)
{
    COMPLEX in, temp, sech_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_cosh(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &sech_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &sech_result);
}

/******************************************************************************
*
* coth(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_coth)
{
    COMPLEX in, sin, cos, coth_result;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &coth_result))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &coth_result);
}

/******************************************************************************
*
* do the work for arccosh and arcsinh and their many relations
*
* arccosh(z) = ln(z + (z*z - 1) ^.5)
* arcsinh(z) = ln(z + (z*z + 1) ^.5)
*
* rob thinks the following apply, from the expansions given further on
* arccos(z) = -i arccosh(z)
* arcsin(z) =  i arcsinh(-iz)
*
*   arccosh(z) = +- i arccos(z)
* i arccosh(z) =      arccos(z)
*
*   arcsinh(z)   = -i arcsin(iz)
* i arcsinh(z)   =    arcsin(iz)
* i arcsinh(z/i) =    arcsin(z)
* i arcsinh(-iz) =    arcsin(z)
*
*   arctanh(z)   = -i arctan(iz)
*   arctanh(-iz) = -i arctan(z)
* i arctanh(-iz) =    arctan(z)
*
******************************************************************************/

#define C_COSH 1
#define C_SINH 2
#define C_TANH 3

static void
do_arc_cosh_sinh_tanh(
    P_EV_DATA p_ev_data_res,
    _InVal_     S32 type,
    /*inout*/ P_COMPLEX z,
    _InRef_opt_ PC_F64 mult_z_by_i,
    _InRef_opt_ PC_F64 mult_res_by_i)
{
    COMPLEX out;
    COMPLEX half;
    COMPLEX temp;
    STATUS status;

    /* maybe preprocess z
        multiply the input by   i * mult_z_by_i
         i(a + ib) = -b + ia
        -i(a + ib) =  b - ia
        mult_z_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(mult_z_by_i)
        {
        F64 t = z->r;

        z->r = z->i * -*mult_z_by_i;
        z->i = t    *  *mult_z_by_i;
        }

    if(type == C_TANH)
        {
        /* do temp = (1+z)/(1-z) */
        COMPLEX in1, in2;

        in1.r = 1.0 + z->r;
        in1.i = z->i;
        in2.r = 1.0 - z->r;
        in2.i =     - z->i;

        if(!do_complex_divide(p_ev_data_res, &in1, &in2, &temp))
            return;
        }
    else
        {
        /* find z*z */
        out.r = z->r * z->r - z->i * z->i;
        out.i = z->r * z->i * 2.0;

        /* z*z + add_in_middle */
        out.r += (type == C_COSH ? -1.0 : +1.0);

        /* sqrt it into temp */
        half.r = 0.5;
        half.i = 0.0;
        if(status_fail(status = do_complex_power(&out, &half, &temp)))
        {
            ev_data_set_error(p_ev_data_res, status);
            return;
        }

        /* z + it  */
        temp.r += z->r;
        temp.i += z->i;
        }

    /* ln it to out */
    if(status_fail(status = complex_lnz(&temp, &out)))
        {
        ev_data_set_error(p_ev_data_res, status);
        return;
        }

    /* now its in out, halve it for arctans */
    if(type == C_TANH)
        {
        /* halve it */
        out.r /= 2.0;
        out.i /= 2.0;
        }

    /* maybe postprocess out
        multiply the output by   i * mult_res_by_i
         i(a+ ib)  = -b + ia
        -i(a + ib) =  b - ia
        mult_res_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(mult_res_by_i)
        {
        F64 t = out.r;

        out.r = out.i  * -*mult_res_by_i;
        out.i = t      *  *mult_res_by_i;
        }

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/*
complex arctan
*/

PROC_EXEC_PROTO(c_c_atan)
{
    COMPLEX z;

    static const F64 c_catan_z   = -1.0;
    static const F64 c_catan_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, &c_catan_z, &c_catan_res);
}

/*
complex arctanh
*/

PROC_EXEC_PROTO(c_c_atanh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, NULL, NULL);
}

/*
complex arccos
*/

PROC_EXEC_PROTO(c_c_acos)
{
    COMPLEX z;

    static const F64 c_cacos_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, &c_cacos_res);
}

/*
complex arccosh
*/

PROC_EXEC_PROTO(c_c_acosh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, NULL);
}

/*
complex arcsin
*/

PROC_EXEC_PROTO(c_c_asin)
{
    COMPLEX z;

    static const F64 c_asin_z   = -1.0;
    static const F64 c_asin_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, &c_asin_z, &c_asin_res);
}

/*
complex arcsinh
*/

PROC_EXEC_PROTO(c_c_asinh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &z, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, NULL, NULL);
}

/*
complex arccot
*/

PROC_EXEC_PROTO(c_c_acot)
{
    COMPLEX in, z;

    static const F64 c_cacot_z   = -1.0;
    static const F64 c_cacot_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, &c_cacot_z, &c_cacot_res);
}

/*
complex arccoth
*/

PROC_EXEC_PROTO(c_c_acoth)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, NULL, NULL);
}

/*
complex arcsec
*/

PROC_EXEC_PROTO(c_c_asec)
{
    COMPLEX in, z;

    static const F64 c_casec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, &c_casec_res);
}

/*
complex arcsech
*/

PROC_EXEC_PROTO(c_c_asech)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, NULL);
}

/*
complex arccosec
*/

PROC_EXEC_PROTO(c_c_acosec)
{
    COMPLEX in, z;

    static const F64 c_acosec_z   = -1.0;
    static const F64 c_acosec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, &c_acosec_z, &c_acosec_res);
}

/*
complex arccosech
*/

PROC_EXEC_PROTO(c_c_acosech)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(status_fail(complex_check_array(p_ev_data_res, &in, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, NULL, NULL);
}

/* [that's enough complex algebra - Ed] */

/* end of ev_mcpx.c */
