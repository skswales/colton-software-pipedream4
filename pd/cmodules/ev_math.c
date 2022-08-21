/* ev_math.c */

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
#include "cmodules/mathxtr2.h"

/* local header file */
#include "ev_evali.h"

#include <math.h>
#include <float.h>
#include <errno.h>

/*
declare complex number type for internal usage
*/

typedef struct _COMPLEX
{
    F64 r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

/*
internal functions
*/

static S32
complex_result_reals(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part);

_Check_return_
static STATUS
err_from_errno(void)
{
    switch(errno)
    {
    default:        return(STATUS_OK);
    case EDOM:      return(create_error(EVAL_ERR_ARGRANGE));
    case ERANGE:    return(create_error(EVAL_ERR_OUTOFRANGE));
    }
}

/******************************************************************************
*
* the constant one as a complex number
*
******************************************************************************/

static const COMPLEX
complex_unity = { 1.0, 0.0 };

/******************************************************************************
*
* check that an array is a 2 by 1
* array of two real numbers
*
* --out--
* <0  array was unsuitable
* >=0 n1 and n2 set up with numbers
*
******************************************************************************/

static S32
complex_check_array(
    P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n,
    P_EV_DATA p_ev_data_in)
{
    S32 res;
    EV_DATA data1, data2;
    EV_IDNO t1, t2;

    /* extract elements from the array */
    t1 = array_range_index(&data1, p_ev_data_in, 0, 0, EM_REA);
    t2 = array_range_index(&data2, p_ev_data_in, 1, 0, EM_REA);

    if(t1 != RPN_DAT_REAL ||
       t2 != RPN_DAT_REAL)
        {
        data_set_error(p_ev_data_res, EVAL_ERR_BADCOMPLEX);
        res = -1;
        }
    else
        {
        n->r = data1.arg.fp;
        n->i = data2.arg.fp;
        res = 0;
        }

    return(res);
}

/******************************************************************************
*
* find ln of complex number for internal routines
*
******************************************************************************/

/* NB z = r.e^i.theta -> ln(z) = ln(r) + i.theta */

static S32
complex_lnz(
    PC_COMPLEX in,
    P_COMPLEX out)
{
    F64 r = in->r * in->r + in->i * in->i;
    S32 res = 0;

    errno = 0;

    out->r = log(r)/2.0;

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        res = EVAL_ERR_BAD_LOG;

    out->i = atan2(in->i, in->r);

    return(res);
}

/******************************************************************************
*
* make a complex number result array from
* an internal complex number type
*
******************************************************************************/

static S32
complex_result_complex(
    P_EV_DATA p_ev_data,
    PC_COMPLEX z)
{
    return(complex_result_reals(p_ev_data, z->r, z->i));
}

/******************************************************************************
*
* make a complex number array in the given
* data structure and give it a complex number
*
******************************************************************************/

static S32
complex_result_reals(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part)
{
    P_EV_DATA elep;

    if(status_ok(ss_array_make(p_ev_data, 2, 1)))
        {
        elep          = ss_array_element_index_wr(p_ev_data, 0, 0);
        elep->did_num = RPN_DAT_REAL;
        elep->arg.fp  = real_part;

        elep          = ss_array_element_index_wr(p_ev_data, 1, 0);
        elep->did_num = RPN_DAT_REAL;
        elep->arg.fp  = imag_part;
        }

    return(p_ev_data->did_num);
}

/******************************************************************************
*
* find w*ln(z) for internal routines
*
******************************************************************************/

static S32
complex_wlnz(
    _InRef_     PC_COMPLEX w,
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX out)
{
    COMPLEX lnz;
    S32     res;

    if((res = complex_lnz(z, &lnz)) < 0)
        return(res);

    out->r = w->r * lnz.r  -  w->i * lnz.i;
    out->i = w->r * lnz.i  +  w->i * lnz.r;
    return(0);
}

/*-------------------------------------------------------------------------*/

/******************************************************************************
*
* acosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosh)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_acosh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input less than 1 invalid */
    /* large positive input causes overflow ***in current algorithm*** */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosec)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_acosec(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of magnitude less than 1 invalid */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosech)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_acosech(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of zero invalid */
    /* input of magnitude near zero causes overflow */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acot)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = mx_acot(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* no error cases */
}

/******************************************************************************
*
* acoth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acoth)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_acoth(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of magnitude less than 1 invalid */
    /* input of magnitude near 1 causes overflow */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asec)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_asec(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of values less than 1 invalid */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asech)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_asech(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* negative input or positive values greater than one invalid */
    /* input of zero or small positive value causes overflow */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asinh)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = mx_asinh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* no error case */
}

/******************************************************************************
*
* atan_2(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan_2)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = atan2(args[1]->arg.fp, args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* both input args zero? */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* atanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atanh)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_atanh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of magnitude 1 or greater invalid */
    /* input of near magnitude 1 causes overflow */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosh)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = cosh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosec)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_cosec(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosech)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_cosech(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cot)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_cot(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* coth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_coth)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_coth(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* sgn(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sgn)
{
    exec_func_ignore_parms();

    if(args[0]->arg.fp > DBL_MIN)
        p_ev_data_res->arg.fp = 1.0;
    else if(args[0]->arg.fp < -DBL_MIN)
        p_ev_data_res->arg.fp = -1.0;
    else
        p_ev_data_res->arg.fp = 0.0;

    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* sec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sec)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = mx_sec(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* sech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sech)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = mx_sech(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* no error cases */
}

/******************************************************************************
*
* sinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sinh)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = sinh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* tanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tanh)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = tanh(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* no error cases */
}

/*-------------------------------------------------------------------------*/

/******************************************************************************
*
* add two complex numbers
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cadd)
{
    COMPLEX in1, in2;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(complex_check_array(p_ev_data_res, &in1, args[0]) < 0)
        return;
    if(complex_check_array(p_ev_data_res, &in2, args[1]) < 0)
        return;

    /* output a complex number array */
    complex_result_reals(p_ev_data_res, in1.r + in2.r, in1.i + in2.i);
}

/******************************************************************************
*
* subtract second complex number from first
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_csub)
{
    COMPLEX in1, in2;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(complex_check_array(p_ev_data_res, &in1, args[0]) < 0)
        return;
    if(complex_check_array(p_ev_data_res, &in2, args[1]) < 0)
        return;

    /* output a complex number array */
    complex_result_reals(p_ev_data_res, in1.r - in2.r, in1.i - in2.i);
}

/******************************************************************************
*
* multiply two complex numbers
* (a+ib)*(c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cmul)
{
    COMPLEX in1, in2;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(complex_check_array(p_ev_data_res, &in1, args[0]) < 0)
        return;
    if(complex_check_array(p_ev_data_res, &in2, args[1]) < 0)
        return;

    /* output a complex number array */
    complex_result_reals(p_ev_data_res,  in1.r * in2.r - in1.i * in2.i,
                                     in1.i * in2.r + in1.r * in2.i);
}

/******************************************************************************
*
* divide two complex numbers
* (a+ib)/(c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

static BOOL
do_complex_divide(
    P_EV_DATA p_ev_data_res,
    PC_COMPLEX in1,
    PC_COMPLEX in2,
    P_COMPLEX out)
{
    F64 divisor;

    /* c*c + d*d */
    divisor = (in2->r * in2->r) + (in2->i * in2->i);

    /* check for divide by 0 */
    if(divisor < DBL_MIN)
        {
        data_set_error(p_ev_data_res, create_error(EVAL_ERR_DIVIDEBY0));
        return(FALSE);
        }

    out->r = (in1->r * in2->r + in1->i * in2->i) / divisor;
    out->i = (in1->i * in2->r - in1->r * in2->i) / divisor;

    return(TRUE);
}

PROC_EXEC_PROTO(c_cdiv)
{
    COMPLEX in1, in2, out;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(complex_check_array(p_ev_data_res, &in1, args[0]) < 0)
        return;
    if(complex_check_array(p_ev_data_res, &in2, args[1]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &in1, &in2, &out))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* find radius of complex number
* should this be called c_abs?
*
******************************************************************************/

PROC_EXEC_PROTO(c_cradius)
{
    COMPLEX in;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    p_ev_data_res->arg.fp  = mx_fhypot(in.r, in.i); /* SKS does carefully */
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* find theta of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_ctheta)
{
    COMPLEX in;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    p_ev_data_res->arg.fp  = atan2(in.i, in.r); /* note silly C library ordering */
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* ln(complex number)
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

PROC_EXEC_PROTO(c_cln)
{
    COMPLEX in, out;
    S32     res;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if((res = complex_lnz(&in, &out)) < 0)
        data_set_error(p_ev_data_res, res);
    else
        /* output a complex number array */
        complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* complex z^w
*
******************************************************************************/

static BOOL
do_complex_power(
    P_EV_DATA p_ev_data_res,
    PC_COMPLEX in1,
    PC_COMPLEX in2,
    P_COMPLEX out)
{
    S32 res;
    COMPLEX wlnz;

    /* find and check wlnz */
    if((res = complex_wlnz(in2, in1, &wlnz)) < 0)
        {
        data_set_error(p_ev_data_res, res);
        return(FALSE);
        }

    out->r = exp(wlnz.r) * cos(wlnz.i);
    out->i = exp(wlnz.r) * sin(wlnz.i);
    return(TRUE);
}

PROC_EXEC_PROTO(c_cpower)
{
    COMPLEX in1, in2, wlnz;
    S32 res;

    exec_func_ignore_parms();

    /* check the inputs are complex number arrays */
    if(complex_check_array(p_ev_data_res, &in1, args[0]) < 0)
        return;
    if(complex_check_array(p_ev_data_res, &in2, args[1]) < 0)
        return;

    /* find and check wlnz */
    if((res = complex_wlnz(&in2, &in1, &wlnz)) < 0)
        data_set_error(p_ev_data_res, res);
    else if(do_complex_power(p_ev_data_res, &in1, &in2, &wlnz))
        /* output a complex number array */
        complex_result_complex(p_ev_data_res, &wlnz);
}

/******************************************************************************
*
* exp(complex number)
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
******************************************************************************/

PROC_EXEC_PROTO(c_cexp)
{
    COMPLEX in;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    /* make exp(a) */
    in.r = exp(in.r);

    /* output a complex number array */
    complex_result_reals(p_ev_data_res, in.r * cos(in.i), in.r * sin(in.i));
}

/******************************************************************************
*
* sin(complex number)
* sin(a+ib) = (exp(-b)+exp(b))sin(a)/2 + i((exp(b)-exp(-b))cos(a)/2)
*
******************************************************************************/

static void
do_complex_sin(
    PC_COMPLEX in,
    P_COMPLEX out)
{
    F64 eb, emb;

    /* make exp(b) and exp(-b) */
    eb =  exp(in->i);
    emb = 1.0 / eb;

    out->r = (eb + emb) * sin(in->r) / 2.0;
    out->i = (eb - emb) * cos(in->r) / 2.0;
}

PROC_EXEC_PROTO(c_csin)
{
    COMPLEX in, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sin(&in, &out);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* cos(complex number)
* cos(a+ib) = (exp(-b)+exp(b))cos(a)/2 + i((exp(-b)-exp(b))sin(a)/2)
*
******************************************************************************/

static void
do_complex_cos(
    PC_COMPLEX in,
    P_COMPLEX out)
{
    F64 eb, emb;

    /* make exp(b) and exp(-b) */
    eb =  exp(in->i);
    emb = 1.0 / eb;

    out->r = (emb + eb) * cos(in->r) / 2.0;
    out->i = (emb - eb) * sin(in->r) / 2.0;
}

PROC_EXEC_PROTO(c_ccos)
{
    COMPLEX in, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_cos(&in, &out);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* tan(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ctan)
{
    COMPLEX in, sin, cos, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &out))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* cosec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ccosec)
{
    COMPLEX in, out, out2;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sin(&in, &out);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &out, &out2))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out2);
}

/******************************************************************************
*
* sec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_csec)
{
    COMPLEX in, out, out2;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_cos(&in, &out);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &out, &out2))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out2);
}

/******************************************************************************
*
* cot(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ccot)
{
    COMPLEX in, sin, cos, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &out))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* sinh(complex number)
* sinh(a+ib) = (exp(a)-exp(-a))cos(b)/2 + i((exp(a)+exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_sinh(
    const COMPLEX * in,
    COMPLEX * out)
{
    F64 ea, ema;

    /* make exp(b) and exp(-b) */
    ea =  exp(in->r);
    ema = 1.0 / ea;

    out->r = (ea - ema) * cos(in->i) / 2.0;
    out->i = (ea + ema) * sin(in->i) / 2.0;
}

PROC_EXEC_PROTO(c_csinh)
{
    COMPLEX in, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sinh(&in, &out);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* cosh(complex number)
* cosh(a+ib) = (exp(a)+exp(-a))cos(b)/2 + i((exp(a)-exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_cosh(
    const COMPLEX * in,
    COMPLEX * out)
{
    F64 ea, ema;

    /* make exp(b) and exp(-b) */
    ea =  exp(in->r);
    ema = 1.0 / ea;

    out->r = (ea + ema) * cos(in->i) / 2.0;
    out->i = (ea - ema) * sin(in->i) / 2.0;
}

PROC_EXEC_PROTO(c_ccosh)
{
    COMPLEX in, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_cosh(&in, &out);

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* tanh(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ctanh)
{
    COMPLEX in, sin, cos, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &out))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
}

/******************************************************************************
*
* cosech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ccosech)
{
    COMPLEX in, out, out2;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sinh(&in, &out);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &out, &out2))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out2);
}

/******************************************************************************
*
* sech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_csech)
{
    COMPLEX in, out, out2;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_cosh(&in, &out);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &out, &out2))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out2);
}

/******************************************************************************
*
* coth(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ccoth)
{
    COMPLEX in, sin, cos, out;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &out))
        return;

    /* output a complex number array */
    complex_result_complex(p_ev_data_res, &out);
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
    S32 type,
    P_COMPLEX z /*inout*/,
    PC_F64 mult_z_by_i,
    PC_F64 mult_res_by_i)
{
    COMPLEX out;
    COMPLEX half;
    COMPLEX temp;
    S32     res;

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
        if(!do_complex_power(p_ev_data_res, &out, &half, &temp))
            return;

        /* z + it  */
        temp.r += z->r;
        temp.i += z->i;
        }

    /* ln it to out */
    if((res = complex_lnz(&temp, &out)) != 0)
        {
        data_set_error(p_ev_data_res, res);
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
    complex_result_reals(p_ev_data_res, out.r, out.i);
}

/*
complex arctan
*/

PROC_EXEC_PROTO(c_catan)
{
    COMPLEX z;

    static const F64 c_catan_z   = -1.0;
    static const F64 c_catan_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, &c_catan_z, &c_catan_res);
}

/*
complex arctanh
*/

PROC_EXEC_PROTO(c_catanh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, NULL, NULL);
}

/*
complex arccos
*/

PROC_EXEC_PROTO(c_cacos)
{
    COMPLEX z;

    static const F64 c_cacos_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, &c_cacos_res);
}

/*
complex arccosh
*/

PROC_EXEC_PROTO(c_cacosh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, NULL);
}

/*
complex arcsin
*/

PROC_EXEC_PROTO(c_casin)
{
    COMPLEX z;

    static const F64 c_asin_z   = -1.0;
    static const F64 c_asin_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, &c_asin_z, &c_asin_res);
}

/*
complex arcsinh
*/

PROC_EXEC_PROTO(c_casinh)
{
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &z, args[0]) < 0)
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, NULL, NULL);
}

/*
complex arccot
*/

PROC_EXEC_PROTO(c_cacot)
{
    COMPLEX in, z;

    static const F64 c_cacot_z   = -1.0;
    static const F64 c_cacot_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, &c_cacot_z, &c_cacot_res);
}

/*
complex arccoth
*/

PROC_EXEC_PROTO(c_cacoth)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, NULL, NULL);
}

/*
complex arcsec
*/

PROC_EXEC_PROTO(c_casec)
{
    COMPLEX in, z;

    static const F64 c_casec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, &c_casec_res);
}

/*
complex arcsech
*/

PROC_EXEC_PROTO(c_casech)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, NULL, NULL);
}

/*
complex arccosec
*/

PROC_EXEC_PROTO(c_cacosec)
{
    COMPLEX in, z;

    static const F64 c_acosec_z   = -1.0;
    static const F64 c_acosec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, &c_acosec_z, &c_acosec_res);
}

/*
complex arccosech
*/

PROC_EXEC_PROTO(c_cacosech)
{
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a complex number array */
    if(complex_check_array(p_ev_data_res, &in, args[0]) < 0)
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, NULL, NULL);
}

/* [that's enough complex algebra - Ed] */

/******************************************************************************
*
*  Matrices
*
******************************************************************************/

/* M[j,k] = mat[n*j+k] - data stored by cols across row, then cols across next row ... */

#define _em(_array, _n_cols_in_row, _row_idx, _col_idx) ( \
    (_array) + (_row_idx) * (_n_cols_in_row))[_col_idx]

/* Conventionally, the entry in the (one-based)
 * i-th row and the j-th column of a matrix is referred to
 * as the i,j, (i,j), or (i,j)th entry of the matrix
 * Easiest for C to keep zero-based...
 */

#define _Aij(_array, _n_cols_in_row, _i_idx, _j_idx) \
    _em(_array, _n_cols_in_row, _i_idx, _j_idx)

/* So watch out when using array_range_index() to fill and
 * ss_array_element_index_borrow() for result as those use x=col, y=row args
 */

/******************************************************************************
*
* evaluate the determinant of an m-square matrix
*
******************************************************************************/

static STATUS
determinant(
    /*_In_count_x_(m*m)*/ PC_F64 ap /*[m][m]*/,
    /*_In_*/        U32 m,
    /*_Out_*/       P_F64 dp)
{
    STATUS status = STATUS_OK;

    switch(m)
    {
    case 0:
        *dp = 1.0;
        break; /* not as silly a question as rjm first thought! (see Bloom, Linear Algebra and Geometry) */

    case 1:
        *dp = *ap;
        break;

    case 2:
#define a11 (_Aij(ap, 2, 0, 0))
#define a12 (_Aij(ap, 2, 0, 1))
#define a21 (_Aij(ap, 2, 1, 0))
#define a22 (_Aij(ap, 2, 1, 1))
        *dp = (a11 * a22) - (a12 * a21);
#undef a11
#undef a12
#undef a21
#undef a22
        break;

    case 3:
#define a11 (_Aij(ap, 3, 0, 0))
#define a12 (_Aij(ap, 3, 0, 1))
#define a13 (_Aij(ap, 3, 0, 2))
#define a21 (_Aij(ap, 3, 1, 0))
#define a22 (_Aij(ap, 3, 1, 1))
#define a23 (_Aij(ap, 3, 1, 2))
#define a31 (_Aij(ap, 3, 2, 0))
#define a32 (_Aij(ap, 3, 2, 1))
#define a33 (_Aij(ap, 3, 2, 2))
        *dp = + a11 * (a22 * a33 - a23 * a32)
              - a12 * (a21 * a33 - a23 * a31)
              + a13 * (a21 * a32 - a22 * a31);
#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33
        break;

    default:
        { /* recurse evaluating minors */
        P_F64 minor;
        U32 minor_m = m - 1;
        F64 minor_D;
        U32 col_idx, i_col_idx, o_col_idx;
        U32 i_row_idx, o_row_idx;

        *dp = 0.0;

        if(NULL == (minor = al_ptr_alloc_bytes(F64, minor_m * sizeof32(*minor), &status)))
            return(status); /* unable to determine */

        for(col_idx = 0; col_idx < m; ++col_idx)
        {
            /* build minor by removing all of top row and current column */
            for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
            {
                if(i_col_idx == col_idx)
                    ++i_col_idx;

                for(i_row_idx = 1, o_row_idx = 0; o_row_idx < minor_m; ++i_row_idx, ++o_row_idx)
                {
                    _em(minor, minor_m, o_row_idx, o_col_idx) = _em(ap, m, i_row_idx, i_col_idx);
                }
            }

            status_break(status = determinant(minor, minor_m, &minor_D));

            if(col_idx & 1)
                minor_D = -minor_D; /* make into cofactor */

            *dp += (_em(ap, m, 0, col_idx) * minor_D);
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));

        break;
        }
    }

    return(status);
}

PROC_EXEC_PROTO(c_mdeterm)
{
    S32 xs, ys;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    if(xs != ys)
    {
        data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);
        return;
    }

    if(xs == 0)
    {
        p_ev_data_res->did_num = RPN_DAT_REAL;
        p_ev_data_res->arg.fp = 1.0; /* yes, really */
    }

    {
    STATUS status = STATUS_OK;
    S32 m = xs;
    F64 nums[3*3]; /* don't really need to allocate for small ones */
    P_F64 a /*[n][n]*/;

    if(m <= 3)
        a = nums;
    else
        a = al_ptr_alloc_bytes(F64, m * (m * sizeof32(*a)), &status);

    if(NULL != a)
    {
        S32 i, j;

        assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

        /* load up the matrix (a) */
        for(i = 0; i < m; ++i)
        {
            for(j = 0; j < m; ++j)
            {
                EV_DATA data;

                if(array_range_index(&data, args[0], j, i, EM_REA) != RPN_DAT_REAL) /* NB j,i */
                    status_break(status = EVAL_ERR_MATRIX_NOT_NUMERIC);

                _Aij(a, m, i, j) = data.arg.fp;
            }
        }

        if(status_ok(status = determinant(a, m, &p_ev_data_res->arg.fp)))
            p_ev_data_res->did_num = RPN_DAT_REAL;

        if(a != nums)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&a));
    }

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/*
inverse of square matrix A is 1/det * adjunct(A)
*/

PROC_EXEC_PROTO(c_minverse)
{
    STATUS status = STATUS_OK;
    S32 xs, ys;
    U32 m, minor_m;
    P_F64 a /*[m][m]*/ = NULL;
    P_F64 adj /*[m][m]*/ = NULL;
    P_F64 minor /*[minor_m][minor_m]*/ = NULL;
    U32 i, j;
    F64 D, minor_D;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    if(xs != ys)
    {
        data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);
        return;
    }

    m = xs;

    if(status_fail(ss_array_make(p_ev_data_res, m, m)))
        return;

    if(NULL == (a = al_ptr_alloc_bytes(F64, m * (m * sizeof32(*a)), &status)))
        goto endpoint;

    assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

    if(NULL == (adj = al_ptr_alloc_bytes(F64, m * (m * sizeof32(*adj)), &status)))
        goto endpoint;

    /* load up the matrix (a) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            EV_DATA ev_data;

            if(array_range_index(&ev_data, args[0], j, i, EM_REA) != RPN_DAT_REAL) /* NB j,i */
            {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endpoint;
            }

            _Aij(a, m, i, j) = ev_data.arg.fp;
        }
    }

    if(status_fail(status = determinant(a, m, &D)))
        goto endpoint;

    if(0.0 == D)
    {
        status = EVAL_ERR_MATRIX_SINGULAR;
        goto endpoint;
    }

    /* compute adjunct(A) */

    /* step 1 - compute matrix of minors */
    minor_m = m - 1;

    if(NULL == (minor = al_ptr_alloc_bytes(F64, minor_m * (minor_m * sizeof32(*minor)), &status)))
        goto endpoint;

    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            /* build minor(i,j) by removing all of current row and current column */
            U32 in_i, in_j, out_i, out_j;

            for(in_i = 0; in_i < m; ++in_i)
            {
                if(in_i == i) continue;

                out_i = in_i;

                if(out_i > i) --out_i;

                for(in_j = 0; in_j < m; ++in_j)
                {
                    if(in_j == j) continue;

                    out_j = in_j;

                    if(out_j > j) --out_j;

                    _Aij(minor, minor_m, out_i, out_j) = _Aij(a, m, in_i, in_j);
                }
            }

            if(status_fail(status = determinant(minor, minor_m, &minor_D)))
                goto endpoint;

            _Aij(adj, m, i, j) = minor_D;
       }
    }

    /* step 2 - convert to matrix of cofactors (multiply by +/- checkerboard) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            if(0 != ((i & 1) ^ (j & 1)))
                _Aij(adj, m, i, j) = - _Aij(adj, m, i, j);
        }
    }

    /* step 3 - transpose the matrix of cofactors => adjunct(A) (simple to do this in formation of result) */

    /* copy out the inverse(A) == adjunct(A) / determinant(A) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, j, i); /* NB j,i */
            p_ev_data->did_num = RPN_DAT_REAL;
            p_ev_data->arg.fp  = _Aij(adj, m, j, i) / D; /* NB j,i here too for transpose step 3 above */
        }
    }

endpoint:

    al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&adj));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&a));

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
*  Matrix Multiply
*
*  arg[0] is of dimensions a*b
*
*  arg[1] is of dimensions b*c
*
*  return matrix of dimensions a*c
*
******************************************************************************/

PROC_EXEC_PROTO(c_mmult)
{
    S32 x1s, x2s, y1s, y2s;

    exec_func_ignore_parms();

    /* check it is an array and get x and y sizes */
    array_range_sizes(args[0], &x1s, &y1s);
    array_range_sizes(args[1], &x2s, &y2s);

    if(x1s != y2s)
        {
        /* whinge about dimensions */
        data_set_error(p_ev_data_res, EVAL_ERR_MISMATCHED_MATRICES);
        return;
        }

    if(status_ok(ss_array_make(p_ev_data_res, x2s, y1s)))
        {
        S32 x, y;

        for(x = 0; x < x2s; x++)
            for(y = 0; y < y1s; y++)
                {
                F64 product = 0.0;
                S32 elem;
                P_EV_DATA elep;

                for(elem=0; elem < x1s; elem++)
                    {
                    EV_DATA data1, data2;

                    array_range_index(&data1, args[0], elem, y, EM_REA);
                    array_range_index(&data2, args[1], x, elem, EM_REA);

                    if(data1.did_num == RPN_DAT_REAL && data2.did_num == RPN_DAT_REAL)
                        product += data1.arg.fp * data2.arg.fp;
                    else
                        {
                        ss_data_free_resources(p_ev_data_res);
                        data_set_error(p_ev_data_res, create_error(EVAL_ERR_MATRIX_NOT_NUMERIC));
                        return;
                        }
                    }

                elep = ss_array_element_index_wr(p_ev_data_res, x, y);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = product;
                }
        }
}

PROC_EXEC_PROTO(c_transpose)
{
    S32 xs, ys;
    S32 x, y;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    /* make a y by x array and swap elements */
    if(status_ok(ss_array_make(p_ev_data_res, ys, xs)))
        for(x = 0; x < xs; x++)
            for(y = 0; y < ys; y++)
                {
                EV_DATA temp;
                array_range_index(&temp, args[0], x, y, EM_ANY);
                ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, y, x), &temp);
                }
}

/******************************************************************************
*
* GROWTH(logest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_growth)
{
    S32 x_vars;
    S32 x, y;
    S32 err = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &x, &y);

    x_vars = x - 1;

    if( (x < 2  /*at least c,m*/)  ||
        ((y != 1 /*no stats*/)  &&
         (y != 3 /*stats*/   )  )  )
        {
        data_set_error(p_ev_data_res, create_error(EVAL_ERR_MATRIX_WRONG_SIZE));
        return;
        }

    array_range_sizes(args[1], &x, &y);

    /* SKS after 4.12 28apr92 - same reason as TREND() update */
    if(x == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, 1, y)))
            {
            S32 row;

            for(row = 0; row < y; ++row)
                {
                EV_DATA xdata;
                EV_DATA adata;
                F64 product;
                S32 ci;
                S32 negative;
                P_EV_DATA elep;

                /* NB. product computed carefully using logs */

                /* start with the constant */
                array_range_index(&adata, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(adata.did_num != RPN_DAT_REAL)
                    {
                    err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                    goto endlabel;
                    }

                errno = 0;

                /* if initial y data was all -ve then this is a possibility ... */
                negative = (adata.arg.fp < 0);

                product = log(fabs(adata.arg.fp));

                /* loop across a row multiplying product by coefficients ^ x variables*/
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&adata, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(adata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    array_range_index(&xdata, args[1],
                                      ci, /* NB. extract from nth col ! */
                                      row,
                                      EM_REA);

                    if(xdata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    product += log(adata.arg.fp) * xdata.arg.fp;
                    }

                if(errno /* == EDOM, ERANGE */)
                    {
                    err = create_error(EVAL_ERR_BAD_LOG);
                    goto endlabel;
                    }

                product = exp(product); /* convert sum of products into a0 * PI(ai ** xi) */

                if(product == HUGE_VAL) /* don't test for underflow case */
                    {
                    err = create_error(EVAL_ERR_OUTOFRANGE);
                    goto endlabel;
                    }

                if(negative)
                    product = -product;

                elep          = ss_array_element_index_wr(p_ev_data_res, 0, row);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = product;
                }

            } /*fi*/
        }
    else if(y == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, x, 1)))
            {
            S32 col;

            for(col = 0; col < x; ++col)
                {
                EV_DATA xdata;
                EV_DATA adata;
                F64 product;
                S32 ci;
                S32 negative;
                P_EV_DATA elep;

                /* NB. product computed carefully using logs */

                /* start with the constant */
                array_range_index(&adata, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(adata.did_num != RPN_DAT_REAL)
                    {
                    err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                    goto endlabel;
                    }

                errno = 0;

                /* if initial y data was all -ve then this is a possibility ... */
                negative = (adata.arg.fp < 0);

                product = log(fabs(adata.arg.fp));

                /* loop multiplying product by coefficients ^ x variables*/
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&adata, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(adata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    array_range_index(&xdata, args[1],
                                      col,
                                      ci, /* NB. extract from nth row ! */
                                      EM_REA);

                    if(xdata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    product += log(adata.arg.fp) * xdata.arg.fp;
                    }

                if(errno /* == EDOM, ERANGE */)
                    {
                    err = create_error(EVAL_ERR_BAD_LOG);
                    goto endlabel;
                    }

                product = exp(product); /* convert sum of products into a0 * PI(ai ** xi) */

                if(product == HUGE_VAL) /* don't test for underflow case */
                    {
                    err = create_error(EVAL_ERR_OUTOFRANGE);
                    goto endlabel;
                    }

                if(negative)
                    product = -product;

                elep          = ss_array_element_index_wr(p_ev_data_res, col, 0);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = product;
                }

            } /*fi*/
        }
    else
        {
        data_set_error(p_ev_data_res, create_error(EVAL_ERR_MATRIX_WRONG_SIZE));
        return;
        }

endlabel:;

    if(err)
        {
        ss_data_free_resources(p_ev_data_res);

        data_set_error(p_ev_data_res, err);
        }
}

/******************************************************************************
*
* TREND(linest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_trend)
{
    S32 x_vars;
    S32 x, y;
    S32 err = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &x, &y);

    x_vars = x - 1;

    if( (x < 2  /*at least c,m*/)  ||
        ((y != 1 /*no stats*/)  &&
         (y != 3 /*stats*/  )   )  )
        {
        data_set_error(p_ev_data_res, create_error(EVAL_ERR_MATRIX_WRONG_SIZE));
        return;
        }

    array_range_sizes(args[1], &x, &y);

    /* SKS after 4.12 28apr92 - allow TREND() to receive data
     * in untransposed form ie. more naturally matched to linest data
    */
    if(x == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, 1, y)))
            {
            S32 row;

            for(row = 0; row < y; ++row)
                {
                EV_DATA xdata;
                EV_DATA adata;
                F64 sum;
                S32 ci;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&adata, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(adata.did_num != RPN_DAT_REAL)
                    {
                    err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                    goto endlabel;
                    }

                sum = adata.arg.fp;

                /* loop across a row summing coefficients * x variables */
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&adata, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(adata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    array_range_index(&xdata, args[1],
                                      ci, /* NB. extract from nth col! */
                                      row,
                                      EM_REA);

                    if(xdata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    sum += adata.arg.fp * xdata.arg.fp;
                    }

                elep          = ss_array_element_index_wr(p_ev_data_res, 0, row);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = sum;
                }
            } /*fi*/
        }
    else if(y == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, x, 1)))
            {
            S32 col;

            for(col = 0; col < x; ++col)
                {
                EV_DATA xdata;
                EV_DATA adata;
                F64 sum;
                S32 ci;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&adata, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(adata.did_num != RPN_DAT_REAL)
                    {
                    err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                    goto endlabel;
                    }

                sum = adata.arg.fp;

                /* loop down a column summing coefficients * x variables */
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&adata, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(adata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    array_range_index(&xdata, args[1],
                                      col,
                                      ci, /* NB. extract from nth row! */
                                      EM_REA);

                    if(xdata.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    sum += adata.arg.fp * xdata.arg.fp;
                    }

                elep          = ss_array_element_index_wr(p_ev_data_res, col, 0);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = sum;
                }

            } /*fi*/
        }
    else
        {
        data_set_error(p_ev_data_res, create_error(EVAL_ERR_MATRIX_WRONG_SIZE));
        return;
        }

endlabel:;

    if(err)
        {
        ss_data_free_resources(p_ev_data_res);

        data_set_error(p_ev_data_res, err);
        }
}

/******************************************************************************
*
* linest(known_y's [, known_x's [, stats [, known_a's [, known_ye's ]]]])
*
* result is an array of {estimation parameters[;
*                        estimated errors for the above;
*                        chi-squared]}
*
******************************************************************************/

typedef struct _for_first_linest
{
    PC_F64 x; /* const */
    S32 x_vars;
    PC_F64 y; /* const */
    P_F64 a;
}
for_first_linest;

static F64
ligp(
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row)
{
    for_first_linest * lidatap = (for_first_linest *) handle;

    switch(colID)
        {
        case LINEST_A_COLOFF:
            return(0.0);

        case LINEST_Y_COLOFF:
            return(lidatap->y[row]);

        default:
            {
            S32 coloff = colID - LINEST_X_COLOFF;
            return((lidatap->x + row * lidatap->x_vars) [coloff]);
            }
        }
}

static STATUS
lipp(
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row,
    _InRef_     PC_F64 value)
{
    for_first_linest * lidatap = (for_first_linest *) handle;

    switch(colID)
        {
        case LINEST_A_COLOFF:
            lidatap->a[row] = *value;
            break;

        default: default_unhandled(); break;
        }

    return(STATUS_DONE);
}

typedef struct _liarray
{
    P_F64 val;
    S32 rows;
    S32 cols;
}
liarray;

PROC_EXEC_PROTO(c_linest)
{
    static const liarray empty = { NULL, 0, 0 };

    liarray known_y;
    liarray known_x;
    S32     stats;
    liarray known_a;
    liarray known_e;
    liarray result_a;
    S32     data_in_cols;
    S32     y_items;
    S32     x_vars;
    S32     err = 0;
    for_first_linest lidata;

    exec_func_ignore_parms();

    stats    = 0;

    known_y  = empty;
    known_x  = empty;
    known_a  = empty;
    known_e  = empty;
    result_a = empty;

    switch(nargs)
        {
        default:
        case 5:
            /* check known_ye's is an array and get x and y sizes */
            array_range_sizes(args[4], &known_e.cols, &known_e.rows);

            /* deliberate drop thru ... */

        case 4:
            /* check known_a's is an array and get x and y sizes */
            array_range_sizes(args[3], &known_a.cols, &known_a.rows);

            /* deliberate drop thru ... */

        case 3:
             stats = (args[2]->arg.fp != 0.0);

            /* deliberate drop thru ... */

        case 2:
            /* check known_x's is an array and get x and y sizes */
            array_range_sizes(args[1], &known_x.cols, &known_x.rows);

            /* deliberate drop thru ... */

        case 1:
            /* check known_y's is an array and get x and y sizes */
            array_range_sizes(args[0], &known_y.cols, &known_y.rows);

            /* y data usually in a column, but allow for a row */
            data_in_cols = (known_y.rows > known_y.cols);
            y_items = data_in_cols ? known_y.rows : known_y.cols;
            break;
        }

    /* number of independent x variables (usually separate columns,
     * but can be separate rows if y data is so arranged)
    */
    if(nargs < 2)
        /* we always invent x if not supplied */
        x_vars = 1;
    else
        x_vars = (data_in_cols) ? known_x.cols : known_x.rows;

    /* simple to allocate y and x arrays together */
    known_y.val = list_allocptr((S32) y_items * ((S32) x_vars + 1) * (S32) sizeof(F64));
    if(!known_y.val)
        {
        err = status_nomem();
        goto endlabel;
        }
    known_x.val = &known_y.val[y_items];

    result_a.cols = x_vars + 1;
    result_a.rows = stats ? 3 : 1;
    result_a.val  = list_allocptr(result_a.cols * (S32) result_a.rows * (S32) sizeof(F64));
    if(!result_a.val)
        {
        err = status_nomem();
        goto endlabel;
        }

    if(known_a.cols != 0)
        {
        if(known_a.cols > result_a.cols)
            {
            err = create_error(EVAL_ERR_MISMATCHED_MATRICES);
            goto endlabel;
            }

        known_a.val = list_allocptr(result_a.cols * (S32) sizeof(F64));
        if(!known_a.val)
            {
            err = status_nomem();
            goto endlabel;
            }
        }

    if(known_e.rows != 0)
        {
        /* neatly covers both arrangements of y,e */
        if((known_e.rows > known_y.rows) || (known_e.rows > known_y.rows))
            {
            err = create_error(EVAL_ERR_MISMATCHED_MATRICES);
            goto endlabel;
            }

        known_e.val = list_allocptr(y_items * (S32) sizeof(F64));
        if(!known_e.val)
            {
            err = status_nomem();
            goto endlabel;
            }
        }

    if(status_ok(ss_array_make(p_ev_data_res, result_a.cols, result_a.rows)))
        {
        S32     i, j;
        EV_DATA data;

        /* copy y data into working array */
        for(i = 0; i < y_items; ++i)
            {
            array_range_index(&data, args[0],
                              (data_in_cols) ? 0 : i,
                              (data_in_cols) ? i : 0,
                              EM_REA);

            if(data.did_num != RPN_DAT_REAL)
                {
                err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                goto endlabel;
                }

            known_y.val[i] = data.arg.fp;
            }

        /* copy x data into working array */
        for(i = 0; i < y_items; ++i)
            {
            if(nargs >= 2)
                for(j = 0; j < x_vars; ++j)
                    {
                    array_range_index(&data, args[1],
                                      (data_in_cols) ? j : i,
                                      (data_in_cols) ? i : j,
                                      EM_REA);

                    if(data.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    (known_x.val + i * x_vars)[j] = data.arg.fp;
                    }
            else
                known_x.val[i] = (F64) i + 1.0; /* make simple {1.0, 2.0, 3.0, ...} */
            }

        /* <<< ignore the a data for the mo */
        if(known_a.val)
            ;

        /* copy (possibly partial) ye data into working array */
        if(known_e.val)
            for(i = 0; i < y_items; ++i)
                {
                if(i < ((data_in_cols) ? known_e.rows : known_e.cols))
                    {
                    array_range_index(&data, args[4],
                                      (data_in_cols) ? 0 : i,
                                      (data_in_cols) ? i : 0,
                                      EM_REA);

                    if(data.did_num != RPN_DAT_REAL)
                        {
                        err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                        goto endlabel;
                        }

                    known_e.val[i] = data.arg.fp;
                    }
                else
                    known_e.val[i] = 1.0;
                }

        /* and then ignore it! */

/* bodgey bit from here ... */

        lidata.x = known_x.val;
        lidata.x_vars = x_vars;

        lidata.y = known_y.val;

        lidata.a = result_a.val;

        linest(ligp, lipp, &lidata, x_vars, y_items);

/* ... to here */

        /* copy result a data from working array */

        for(j = 0; j < result_a.cols; ++j)
            {
            F64 res;
            P_EV_DATA elep;

            res = (result_a.val + 0 * result_a.cols)[j];

            elep          = ss_array_element_index_wr(p_ev_data_res, j, 0);
            elep->did_num = RPN_DAT_REAL;
            elep->arg.fp  = res;

            if(stats)
                {
                /* estimated estimation errors */
#if 1 /* until yielded */
                res = 0;
#else
                res = (result_a.val + 1 * result_a.cols)[j];
#endif

                elep          = ss_array_element_index_wr(p_ev_data_res, j, 1);
                elep->did_num = RPN_DAT_REAL;
                elep->arg.fp  = res;
                }
            }

        if(stats)
            {
            /* chi-squared */
            F64 chi_sq;
            P_EV_DATA elep;

            chi_sq = 0.0;

            elep          = ss_array_element_index_wr(p_ev_data_res, 0, 2);
            elep->did_num = RPN_DAT_REAL;
            elep->arg.fp  = chi_sq;
            }

        } /*fi*/

endlabel:;

    if(err)
        {
        ss_data_free_resources(p_ev_data_res);

        data_set_error(p_ev_data_res, err);
        }

    list_disposeptr((void **) &known_e.val);
    list_disposeptr((void **) &known_a.val);
    list_disposeptr((void **) &result_a.val);
    list_disposeptr((void **) &known_y.val); /* and x too */
}

/******************************************************************************
*
* oh god its logest. what a pathetic excuse of a routine
* when all you have to do is to tell the user to transform
* his data in an intelligent manner in the first place
* and use linest... I blame Excel personally
*
* it can't even fit y = k * m ^ -x !
*
******************************************************************************/

PROC_EXEC_PROTO(c_logest)
{
    EV_DATA y_data;
    EV_DATA a_data;
    EV_DATA data;
    P_EV_DATA elep;
    P_EV_DATA targs0, targs3;
    F64 val;
    S32 x, y;
    S32 col, row;
    S32 err = 0;
    S32 sign = 0; /* not yet set */

    exec_func_ignore_parms();

    /* perform quick preprocess of a data, taking logs */

    if(nargs > 3)
        {
        array_range_sizes(args[0], &x, &y);

        if(args[3]->did_num == RPN_DAT_RANGE)
            {
            if(status_fail(ss_array_make(&a_data, x, y)))
                return;
            }
        else
            ss_data_resource_copy(&a_data, args[3]);

        for(col = 0; col < x; ++col)
            {
            if(array_range_index(&data, args[3], col, 0, EM_REA) != RPN_DAT_REAL)
                {
                err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                goto endlabel2;
                }

            val = data.arg.fp;
            /* do sign forcing before y transform */
            if(col == 0)
                sign = (val < 0) ? -1 : +1;
            data.arg.fp = log(val);

            *ss_array_element_index_wr(&a_data, col, 0) = data;
            }
        }

    /* perform quick preprocess of y data, taking logs */

    array_range_sizes(args[0], &x, &y);

    if(args[0]->did_num == RPN_DAT_RANGE)
        {
        if(status_fail(ss_array_make(&y_data, x, y)))
            return;
        }
    else
        ss_data_resource_copy(&y_data, args[0]);

    /* simple layout-independent transform of y data */
    for(col = 0; col < x; ++col)
        for(row = 0; row < y; ++row)
            {
            if(array_range_index(&data, args[0], col, row, EM_REA) != RPN_DAT_REAL)
                {
                err = create_error(EVAL_ERR_MATRIX_NOT_NUMERIC);
                goto endlabel;
                }

            if(data.arg.fp < 0.0)
                {
                if(!sign)
                    sign = -1;
                else if(sign != -1)
                    {
                    err = create_error(EVAL_ERR_MIXED_SIGNS);
                    goto endlabel;
                    }

                data.arg.fp = log(-data.arg.fp);
                }
            else if(data.arg.fp > 0.0)
                {
                if(!sign)
                    sign = +1;
                else if(sign != +1)
                    {
                    err = create_error(EVAL_ERR_MIXED_SIGNS);
                    goto endlabel;
                    }

                data.arg.fp = log(data.arg.fp);
                }
            /* else 0.0 ... interesting case ... not necessarily wrong but very hard */

            /* poke back transformed data */
            elep          = ss_array_element_index_wr(&y_data, col, row);
            elep->did_num = RPN_DAT_REAL;
            elep->arg.fp  = data.arg.fp;
            }

    /* ask mrjc whether this is legal */
    targs0  = args[0];
    args[0] = &y_data;

    if(nargs > 3)
        {
        targs3  = args[3];
        args[3] = &a_data;
        }
    else
        targs3  = NULL;

    /* call my friend to do the hard work */
    c_linest(args, nargs, p_ev_data_res, p_cur_slr);

    args[0] = targs0;

    if(targs3)
        args[3] = targs3;

    /* and transform first row of coefficients insitu using exp, and sign the constant if needed */

    if(p_ev_data_res->did_num == RPN_TMP_ARRAY)
        {
        array_range_sizes(p_ev_data_res, &x, &y);

        for(col = 0; col < x; ++col)
            {
            elep  = ss_array_element_index_wr(p_ev_data_res, col, 0);
            assert(elep->did_num == RPN_DAT_REAL);
            val   = exp(elep->arg.fp);
            if((col == 0) && (sign == -1))
                val = -val;
            elep->arg.fp = val;
            }
        }

endlabel:;

    ss_data_free_resources(&y_data);

endlabel2:;

    ss_data_free_resources(&a_data);

    if(err)
        data_set_error(p_ev_data_res, err);
}

extern double lgamma_r(double, int *);

static F64
ln_fact(
    S32 n)
{
    int sign;

    if(n < 0)
        {
        errno = EDOM;
        return(-HUGE_VAL);
        }

    return(lgamma_r(n + 1.0, &sign));
}

/******************************************************************************
*
* computes the product of all integers
* in the range start..end inclusive
* start, end assumed to be +ve, non-zero
*
******************************************************************************/

static void
product_between(
    P_EV_DATA p_ev_data_res /*inout*/,
    S32 start,
    S32 end)
{
    S32 i;

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
        p_ev_data_res->arg.fp = start;
    else
        p_ev_data_res->arg.integer = start;

    for(i = start + 1; i <= end; ++i)
        {
        /* keep in integer where possible */
        if(RPN_DAT_REAL != p_ev_data_res->did_num)
            {
            UMUL64_RESULT result;

            umul64(p_ev_data_res->arg.integer, i, &result);

            if ((0 == result.HighPart) && (0 == (result.LowPart >> 31)))
                { /* result still fits in integer */
                p_ev_data_res->arg.integer = (S32) result.LowPart;
                continue;
                }

            /* have to go to fp now result has outgrown integer and retry the multiply */
            p_ev_data_res->arg.fp = p_ev_data_res->arg.integer;
            p_ev_data_res->did_num = RPN_DAT_REAL;
            }

        p_ev_data_res->arg.fp *= i;
        }

    if(RPN_DAT_REAL != p_ev_data_res->did_num)
        p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

PROC_EXEC_PROTO(c_beta)
{
    STATUS err = STATUS_OK;

    exec_func_ignore_parms();

    errno = 0;

    {
    int sign;
    F64 z = args[0]->arg.fp;
    F64 w = args[1]->arg.fp;
    F64 a = lgamma_r(z, &sign);
    F64 b = lgamma_r(w, &sign);
    F64 c = lgamma_r(z + w, &sign);
    F64 d = exp(a + b - c);

    p_ev_data_res->arg.fp  = d;
    p_ev_data_res->did_num = RPN_DAT_REAL;
    }

    if(errno)
        err = err_from_errno();

    if(status_fail(err))
        data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_binom)
{
    /* SKS 02apr99 call my friend to do the hard work */
    c_combin(args, nargs, p_ev_data_res, p_cur_slr);
}

PROC_EXEC_PROTO(c_combin) /* nCk = n!/((n-k)!*k!) */
{
    STATUS err = STATUS_OK;
    S32 n = args[0]->arg.integer;
    S32 k = args[1]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if((n < 0) || (k < 0) || ((n - k) < 0))
        err = create_error(EVAL_ERR_ARGRANGE);
    else if(n <= 163) /* SKS 02apr99 bigger range */
        {
        EV_DATA ev_data_divisor;

        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, n - k + 1, n);

        /* calculate divisor in same format as dividend
         * NB divisor is always smaller than dividend so this is ok
         */
        ev_data_divisor.did_num = p_ev_data_res->did_num;
        product_between(&ev_data_divisor, 1, k);

        if(RPN_DAT_REAL == p_ev_data_res->did_num)
            {
            p_ev_data_res->arg.fp /= ev_data_divisor.arg.fp;

            /* combination always integer result - see if we can get one! */
            fp_to_integer_try(p_ev_data_res);
            }
        else
            {
            /* no worries about remainders - combination always integer result! */
            p_ev_data_res->arg.integer /= ev_data_divisor.arg.integer;

            p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
            }
        }
    else
        {
        F64 ln_numer = ln_fact(n);
        F64 ln_denom = ln_fact(n - k) + ln_fact(k);

        p_ev_data_res->arg.fp  = exp(ln_numer - ln_denom);
        p_ev_data_res->did_num = RPN_DAT_REAL;

        if(errno)
            err = err_from_errno();

        fp_to_integer_try(p_ev_data_res);
        }

    if(status_fail(err))
        data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_fact)
{
    STATUS err = STATUS_OK;
    S32 n = args[0]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if(n < 0)
        err = create_error(EVAL_ERR_ARGRANGE);
    else if(n <= 163) /* SKS 13nov96 bigger range */
        {
        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, 1, n);
        }
    else
        {
        p_ev_data_res->arg.fp  = exp(ln_fact(n));
        p_ev_data_res->did_num = RPN_DAT_REAL;

        if(errno)
            err = err_from_errno();
        }

    if(status_fail(err))
        data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_gammaln)
{
    STATUS err = STATUS_OK;

    exec_func_ignore_parms();

    errno = 0;

    {
    int sign;
    p_ev_data_res->arg.fp  = lgamma_r(args[0]->arg.fp, &sign);
    p_ev_data_res->did_num = RPN_DAT_REAL;
    } /*block*/

    if(errno)
        err = err_from_errno();

    if(status_fail(err))
        data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_permut) /* nPk = n!/(n-k)! */
{
    STATUS err = STATUS_OK;
    S32 n = args[0]->arg.integer;
    S32 k = args[1]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if((n < 0) || (k < 0) || ((n - k) < 0))
        err = create_error(EVAL_ERR_ARGRANGE);
    else if(n <= 163) /* SKS 02apr99 bigger range */
        {
        /* every number in the divisor term (n-k)!
         * is present in the dividend n! so cancels out
         * leaving just the product of the numbers from
         * n-k+1 to n to be calculated
         */

        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, n - k + 1, n);
        }
    else
        {
        F64 ln_numer = ln_fact(n);
        F64 ln_denom = ln_fact(n - k);

        p_ev_data_res->arg.fp  = exp(ln_numer - ln_denom);
        p_ev_data_res->did_num = RPN_DAT_REAL;

        if(errno)
            err = err_from_errno();

        fp_to_integer_try(p_ev_data_res);
        }

    if(status_fail(err))
        data_set_error(p_ev_data_res, err);
}

/* end of ev_math.c */
