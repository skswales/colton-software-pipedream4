/* mathxcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions */

/* Split off from ev_mcpx.c December 2019 */

#include "common/gflags.h"

#include "cmodules/mathxcpx.h"

#include "cmodules/mathxtra.h" /* for mathxtra_square() */

#ifndef                     MATHNUMS_H
#include "cmodules/coltsoft/mathnums.h" /* for _log10_e */
#endif

#pragma no_check_stack /* nothing goes deep here */

#define mathxtra_square mx_fsquare /* for transition */

/*
internal functions
*/

#if defined(USE_OWN_COMPLEX_IMPL)

enum HYP_FN_TYPE
{
    C_COSH = 1,
    C_SINH = 2,
    C_TANH = 3
};

#define PC_DBL PC_F64

_Check_return_
static COMPLEX
do_inverse_hyperbolic(
    _InRef_     PC_COMPLEX z_in,
    _InVal_     enum HYP_FN_TYPE type,
    _InRef_opt_ PC_DBL mult_z_by_i,
    _InRef_opt_ PC_DBL mult_res_by_i);

#endif /* defined(USE_OWN_COMPLEX_IMPL) */

/******************************************************************************
*
* the constant zero as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_zero = COMPLEX_INIT( 1.0, 0.0 );

/******************************************************************************
*
* the constant Real one as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_Re_one = COMPLEX_INIT( 1.0, 0.0 );

/******************************************************************************
*
* the constant Real one half as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_Re_one_half = COMPLEX_INIT( 0.5, 0.0 );

/******************************************************************************
*
* complex infinity
*
******************************************************************************/

static const COMPLEX
complex_infinity = COMPLEX_INIT( INFINITY, INFINITY );

/******************************************************************************
*
* complex NaN
*
******************************************************************************/

#if defined(UNUSED_IN_PD) || defined(USE_OWN_COMPLEX_IMPL)

static const COMPLEX
complex_nan = COMPLEX_INIT( NAN, NAN );

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* return modulus of complex number
*
******************************************************************************/

_Check_return_
extern double
complex_modulus( /* |z| */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    return(hypot(complex_real(z), complex_imag(z)));
#else
    // See C99 7.3.8.1, G.6
    // no error case
    return(cabs(*z));
#endif
}

/******************************************************************************
*
* return argument of complex number
*
******************************************************************************/

_Check_return_
extern double
complex_argument( /* arg(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    // See C99 7.12.4.4, F.9.1.4
    // domain error (NaN) *possible* when both inputs are 0
    // value returned in [-pi, +pi]
    return(atan2(complex_imag(z), complex_real(z))); /* note silly C library ordering (y, x) */
#else
    // See C99 7.3.9.1, G.6
    // no error case
    // value returned in [-pi, +pi]
    return(carg(*z));
#endif
}

/******************************************************************************
*
* add two complex numbers
*
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_add( /* z1 + z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2)
{
#if defined(USE_OWN_COMPLEX_IMPL) || defined(_MSC_VER)
    return(complex_ri( /*r*/ complex_real(z1) + complex_real(z2),
                       /*i*/ complex_imag(z1) + complex_imag(z2) ));
#else
    return(*z1 + *z2);
#endif
}

/******************************************************************************
*
* subtract second complex number from first
*
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_subtract( /* z1 - z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2)
{
#if defined(USE_OWN_COMPLEX_IMPL) || defined(_MSC_VER)
    return(complex_ri( /*r*/ complex_real(z1) - complex_real(z2),
                       /*i*/ complex_imag(z1) - complex_imag(z2) ));
#else
    return(*z1 - *z2);
#endif
}

/******************************************************************************
*
* multiply two complex numbers
*
* (a+ib) * (c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_multiply( /* z1 * z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    return(complex_ri( /*r*/ complex_real(z1) * complex_real(z2) - complex_imag(z1) * complex_imag(z2),
                       /*i*/ complex_imag(z1) * complex_real(z2) + complex_real(z1) * complex_imag(z2) ));
#elif defined(_MSC_VER)
    return(_Cmulcc((*z1), (*z2)));
#else
    return(*z1 * *z2);
#endif
}

/******************************************************************************
*
* divide two complex numbers
*
* (a+ib) / (c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_divide( /* z1 / z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2)
{
    /* c*c + d*d */
    const double divisor = mathxtra_square(complex_real(z2)) + mathxtra_square(complex_imag(z2));

    /* check for divide by zero about to trap */
    if(fabs(divisor) < DBL_MIN)
        return(complex_infinity);

#if defined(USE_OWN_COMPLEX_IMPL) || defined(_MSC_VER)
    return(complex_ri( /*r*/ (complex_real(z1) * complex_real(z2) + complex_imag(z1) * complex_imag(z2)) / divisor,
                       /*i*/ (complex_imag(z1) * complex_real(z2) - complex_real(z1) * complex_imag(z2)) / divisor ));
#else
    return(*z1 / *z2);
#endif
}

/******************************************************************************
*
* return complex conjugate of complex number
*
******************************************************************************/

#if defined(UNUSED_IN_PD)

_Check_return_
extern COMPLEX
complex_conjugate( /* conj(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL) || defined(__SOFTFP__) /* compiler barf */
    return(complex_ri( /*r*/   complex_real(z),
                       /*i*/ - complex_imag(z) ));
#else
    return(conj(*z));
#endif
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* return complex exponent
*
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
* notes on real cos() function:
*   See C99 7.12.4.5, F.9.1.1
*   domain error (NaN) for ±inf input
*   range error *possible* for large magnitude input (imprecise value)
*   value returned in [-1, +1]
*
* notes on real sin() function:
*   See C99 7.12.4.6, F.9.1.6
*   domain error (NaN) for ±inf input
*   range error *possible* for large magnitude input (imprecise value)
*   value returned in [-1, +1]
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_exp( /* e^z */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* make exp(a) */
    const double exp_a = exp(complex_real(z));

    return(complex_ri( /*r*/ exp_a * cos(complex_imag(z)),
                       /*i*/ exp_a * sin(complex_imag(z)) ));
#else
    return(cexp(*z));
#endif
}

/******************************************************************************
*
* complex natural logarithm
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_log_e( /* log_e(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    const double r_squared = mathxtra_square(complex_real(z)) + mathxtra_square(complex_imag(z));

    if(0.0 == r_squared)
        return(complex_nan); /* ln(0) is undefined */

    const double ln_z_real_part = log(r_squared) * 0.5; /* x saves a sqrt() */

    return(complex_ri( /*r*/ ln_z_real_part,
                       /*i*/ complex_argument(z) ));
#else
    return(clog(*z));
#endif
}

/******************************************************************************
*
* complex logarithm to base two
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

#if defined(UNUSED_IN_PD)

_Check_return_
extern COMPLEX
complex_log_2( /* log_2(z) */
    _InRef_     PC_COMPLEX z)
{
    const double r_squared = mathxtra_square(complex_real(z)) + mathxtra_square(complex_imag(z));

    if(0.0 == r_squared)
        return(complex_nan); /* ln(0) is undefined */

    const double log2_real_part = log2(r_squared) * 0.5; /* x saves a sqrt() */

    return(complex_ri( /*r*/ log2_real_part,
                       /*i*/ complex_argument(z) * _log2_e )); /* rotate */
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* complex logarithm to base ten
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

#if defined(UNUSED_IN_PD)

_Check_return_
extern COMPLEX
complex_log_10( /* log_10(z) */
    _InRef_     PC_COMPLEX z)
{
    const double r_squared = mathxtra_square(complex_real(z)) + mathxtra_square(complex_imag(z));

    if(0.0 == r_squared)
        return(complex_nan); /* ln(0) is undefined */

    const double log10_real_part = log10(r_squared) * 0.5; /* x saves a sqrt() */

    return(complex_ri( /*r*/ log10_real_part,
                       /*i*/ complex_argument(z) * _log10_e )); /* rotate */
}

#endif /* UNUSED_IN_PD */

#if defined(USE_OWN_COMPLEX_IMPL)

/******************************************************************************
*
* return w*ln(z) for internal routines
*
******************************************************************************/

_Check_return_
static COMPLEX
complex_w_ln_z(
    _InRef_     PC_COMPLEX w,
    _InRef_     PC_COMPLEX z)
{
    const double Re_w = complex_real(w);
    const double Im_w = complex_imag(w);

    const COMPLEX ln_z = complex_log_e(z);
    const double Re_ln_z = complex_real(&ln_z);
    const double Im_ln_z = complex_imag(&ln_z);

    return(complex_ri( /*r*/ Re_w * Re_ln_z  -  Im_w * Im_ln_z,
                       /*i*/ Re_w * Im_ln_z  +  Im_w * Re_ln_z ));
}

#endif /* USE_OWN_COMPLEX_IMPL */

/******************************************************************************
*
* complex z1^z2
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_power( /* z1^z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* find and check w*ln(z) */
    const COMPLEX w_ln_z = complex_w_ln_z(z2, z1);
    const double Re_w_ln_z = complex_real(&w_ln_z);
    const double Im_w_ln_z = complex_imag(&w_ln_z);

    return(complex_ri( /*r*/ exp(Re_w_ln_z) * cos(Im_w_ln_z),
                       /*i*/ exp(Re_w_ln_z) * sin(Im_w_ln_z) ));
#else
    return(cpow(*z1, *z2));
#endif
}

/******************************************************************************
*
* complex square root
*
******************************************************************************/

#if defined(UNUSED_IN_PD) || defined(USE_OWN_COMPLEX_IMPL)

_Check_return_
extern COMPLEX
complex_square_root( /* sqrt(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    if( (0.0 == complex_real(z)) && (0.0 == complex_imag(z)) )
        return(complex_zero);

    return(complex_power(z, &complex_Re_one_half));
#else
    return(csqrt(*z));
#endif
}

#endif /* UNUSED_IN_PD */

/******************************************************************************
*
* complex trig functions and their inverses
*
******************************************************************************/

#pragma force_fpargs_in_regs
static COMPLEX
complex_reciprocal(
    _InVal_     COMPLEX z)
{
    return(complex_Re_one / z);
}
#pragma no_force_fpargs_in_regs

static void
complex_reciprocal_helper(
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    *z_out = complex_reciprocal(*z);
}

/******************************************************************************
*
* complex cosine
*
* cos(a+ib) = (exp(-b)+exp(b))cos(a)/2 + i((exp(-b)-exp(b))sin(a)/2)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_cosine( /* cos(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* make exp(b) and exp(-b) */
    const double exp_b = exp(complex_imag(z));
    const double exp_mb = 1.0 / exp_b;
    const double Re_z = complex_real(z);

    return(complex_ri( /*r*/ (exp_mb + exp_b) * cos(Re_z) * 0.5,
                       /*i*/ (exp_mb - exp_b) * sin(Re_z) * 0.5 ));
#else
    return(ccos(*z));
#endif
}

/******************************************************************************
*
* complex arc cosine
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_cosine( /* arccos(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    static const double c_acos_mult_res = +1.0;

    return(do_inverse_hyperbolic(z, C_COSH, NULL, &c_acos_mult_res));
#else
    return(cacos(*z));
#endif
}

/******************************************************************************
*
* complex secant
*
* sec(z) is defined as 1 / cos(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_secant( /* sec(z) */
    _InRef_     PC_COMPLEX z)
{
    return(complex_reciprocal(complex_cosine(z)));
}

/******************************************************************************
*
* complex arc secant
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_secant( /* arcsec(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_arc_cosine(&reciprocal_z));
}

/******************************************************************************
*
* complex sine
*
* sin(a+ib) = (exp(-b)+exp(b))sin(a)/2 + i((exp(b)-exp(-b))cos(a)/2)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_sine( /* sin(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* make exp(b) and exp(-b) */
    const double exp_b = exp(complex_imag(z));
    const double exp_mb = 1.0 / exp_b;
    const double Re_z = complex_real(z);

    return(complex_ri( /*r*/ (exp_b + exp_mb) * sin(Re_z) * 0.5,
                       /*i*/ (exp_b - exp_mb) * cos(Re_z) * 0.5 ));
#else
    return(csin(*z));
#endif
}

/******************************************************************************
*
* complex arc sine
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_sine( /* arc sine(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    static const double c_asin_mult_z   = -1.0;
    static const double c_asin_mult_res = +1.0;

    return(do_inverse_hyperbolic(z, C_SINH, &c_asin_mult_z, &c_asin_mult_res));
#else
    return(casin(*z));
#endif
}

/******************************************************************************
*
* complex cosecant
*
* csc(z) is defined as 1 / sin(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_cosecant( /* csc(z) */
    _InRef_     PC_COMPLEX z)
{
    return(complex_reciprocal(complex_sine(z)));
}

/******************************************************************************
*
* complex arc cosecant
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_cosecant( /* arccsc(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_arc_sine(&reciprocal_z));
}

/******************************************************************************
*
* complex tangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_tangent( /* tan(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    const COMPLEX sin_z = complex_sine(z);
    const COMPLEX cos_z = complex_cosine(z);

    return(complex_divide(&sin_z, &cos_z));
#else
    return(ctan(*z));
#endif
}

/******************************************************************************
*
* complex arc tangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_tangent( /* arctan(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    static const double c_atan_mult_z   = -1.0;
    static const double c_atan_mult_res = +1.0;

    return(do_inverse_hyperbolic(z, C_TANH, &c_atan_mult_z, &c_atan_mult_res));
#else
    return(catan(*z));
#endif
}

/******************************************************************************
*
* complex cotangent
*
* cot(z) is defined as 1 / tan(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_cotangent( /* cot(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    const COMPLEX sin_z = complex_sine(z);
    const COMPLEX cos_z = complex_cosine(z);

    return(complex_divide(&cos_z, &sin_z));
#else
    return(complex_reciprocal(ctan(*z)));
#endif
}

/******************************************************************************
*
* complex arc cotangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_arc_cotangent( /* arccot(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_arc_tangent(&reciprocal_z));
}

/******************************************************************************
*
* complex hyperbolic functions and their inverses
*
******************************************************************************/

/******************************************************************************
*
* complex hyperbolic cosine
*
* cosh(a+ib) = (exp(a)+exp(-a))cos(b)/2 + i((exp(a)-exp(-a))sin(b)/2)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_cosine( /* cosh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* make exp(a) and exp(-a) */
    const double exp_a = exp(complex_real(z));
    const double exp_ma = 1.0 / exp_a;

    return(complex_ri( /*r*/ (exp_a + exp_ma) * cos(complex_imag(z)) * 0.5,
                       /*i*/ (exp_a - exp_ma) * sin(complex_imag(z)) * 0.5 ));
#else
    return(ccosh(*z));
#endif
}

/******************************************************************************
*
* complex inverse hyperbolic cosine
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cosine( /* acosh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    return(do_inverse_hyperbolic(z, C_COSH, NULL, NULL));
#else
    return(cacosh(*z));
#endif
}

/******************************************************************************
*
* complex hyperbolic secant
*
* sech(z) is defined as 1 / cosh(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_secant( /* sech(z) */
    _InRef_     PC_COMPLEX z)
{
    return(complex_reciprocal(complex_hyperbolic_cosine(z)));
}

/******************************************************************************
*
* complex inverse hyperbolic secant
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_secant( /* asech(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_inverse_hyperbolic_cosine(&reciprocal_z));
}

/******************************************************************************
*
* complex hyperbolic sine
*
* sinh(a+ib) = (exp(a)-exp(-a))cos(b)/2 + i((exp(a)+exp(-a))sin(b)/2)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_sine( /* sinh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    /* make exp(a) and exp(-a) */
    const double exp_a = exp(complex_real(z));
    const double exp_ma = 1.0 / exp_a;

    return(complex_ri( /*r*/ (exp_a - exp_ma) * cos(complex_imag(z)) * 0.5,
                       /*i*/ (exp_a + exp_ma) * sin(complex_imag(z)) * 0.5 ));
#else
    return(csinh(*z));
#endif
}

/******************************************************************************
*
* complex inverse hyperbolic sine
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_sine( /* asinh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    return(do_inverse_hyperbolic(z, C_SINH, NULL, NULL));
#else
    return(casinh(*z));
#endif
}

/******************************************************************************
*
* complex hyperbolic cosecant
*
* csch(z) is defined as 1 / sinh(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_cosecant( /* csch(z) */
    _InRef_     PC_COMPLEX z)
{
    return(complex_reciprocal(complex_hyperbolic_sine(z)));
}

/******************************************************************************
*
* complex inverse hyperbolic cosecant
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cosecant( /* acsch(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_inverse_hyperbolic_sine(&reciprocal_z));
}

/******************************************************************************
*
* complex hyperbolic tangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_tangent( /* tanh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    const COMPLEX sinh_z = complex_hyperbolic_sine(z);
    const COMPLEX cosh_z = complex_hyperbolic_cosine(z);

    return(complex_divide(&sinh_z, &cosh_z));
#else
    return(ctanh(*z));
#endif
}

/******************************************************************************
*
* complex inverse hyperbolic tangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_tangent( /* atanh(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(USE_OWN_COMPLEX_IMPL)
    return(do_inverse_hyperbolic(z, C_TANH, NULL, NULL));
#else
    return(catanh(*z));
#endif
}

/******************************************************************************
*
* complex hyperbolic cotangent
*
* coth(z) is defined as 1 / tanh(z)
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_hyperbolic_cotangent( /* coth(z) */
    _InRef_     PC_COMPLEX z)
{
    return(complex_reciprocal(complex_hyperbolic_tangent(z)));
}

/******************************************************************************
*
* complex inverse hyperbolic cotangent
*
******************************************************************************/

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cotangent( /* acoth(z) */
    _InRef_     PC_COMPLEX z)
{
    COMPLEX reciprocal_z; complex_reciprocal_helper(z, &reciprocal_z);

    return(complex_inverse_hyperbolic_tangent(&reciprocal_z));
}

#if defined(USE_OWN_COMPLEX_IMPL)

/******************************************************************************
*
* do the work for inverse hyperbolic cosh and sinh and their many relations
*
* acosh(z) = ln(z + (z*z - 1) ^.5)
* asinh(z) = ln(z + (z*z + 1) ^.5)
*
* rob thinks the following apply, from the expansions given further on
* arccos(z) = -i acosh(z)
* arcsin(z) =  i asinh(-iz)
*
*   acosh(z) = +- i arccos(z)
* i acosh(z) =      arccos(z)
*
*   asinh(z)   = -i arcsin(iz)
* i asinh(z)   =    arcsin(iz)
* i asinh(z/i) =    arcsin(z)
* i asinh(-iz) =    arcsin(z)
*
*   atanh(z)   = -i arctan(iz)
*   atanh(-iz) = -i arctan(z)
* i atanh(-iz) =    arctan(z)
*
******************************************************************************/

_Check_return_
static COMPLEX
do_inverse_hyperbolic(
    _InRef_     PC_COMPLEX z_in,
    _InVal_     enum HYP_FN_TYPE type,
    _InRef_opt_ PC_DBL mult_z_by_i,
    _InRef_opt_ PC_DBL mult_res_by_i)
{
    /* maybe preprocess z
        multiply the input by   i * mult_z_by_i
         i(a + ib) = -b + ia
        -i(a + ib) =  b - ia
        mult_z_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    COMPLEX z = *z_in;

    if(NULL != mult_z_by_i)
        z = complex_ri( /*r*/ complex_imag(&z) * -(*mult_z_by_i),
                        /*i*/ complex_real(&z) *  (*mult_z_by_i) );

    COMPLEX temp;

    if(type == C_TANH)
    {
        /* temp = (1+z)/(1-z) */
        const COMPLEX z1 = complex_ri( 1.0 + complex_real(&z),   complex_imag(&z) );
        const COMPLEX z2 = complex_ri( 1.0 - complex_real(&z), - complex_imag(&z) );

        temp = complex_divide(&z1, &z2);
    }
    else
    {
        /* temp = z + sqrt(furtle) */
        const double add_in_middle = (type == C_COSH ? -1.0 : +1.0);

        /* furtle = z*z + add_in_middle */
        const COMPLEX furtle = complex_ri( /*r*/ mathxtra_square(complex_real(&z)) - mathxtra_square(complex_imag(&z))  +  add_in_middle,
                                           /*i*/ complex_real(&z) * complex_imag(&z) * 2.0 );

        /* sqrt(furtle) into temp */
        temp = complex_square_root(&furtle);

        /* temp = z + sqrt(furtle) */
        temp = complex_ri( /*r*/ complex_real(&z) + complex_real(&temp),
                           /*i*/ complex_imag(&z) + complex_imag(&temp) );
    }

    /* out = ln(temp) */
    COMPLEX out = complex_log_e(&temp);

    /* now its in out, halve it for arctans (just in magnitude) */
    if(type == C_TANH)
        out = complex_ri( complex_real(&out) / 2.0, complex_imag(&out) / 2.0 );

    /* maybe postprocess out
        multiply the output by   i * mult_res_by_i
         i(a+ ib)  = -b + ia
        -i(a + ib) =  b - ia
        mult_res_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(NULL != mult_res_by_i)
        out = complex_ri( /*r*/ complex_imag(&out) * -(*mult_res_by_i),
                          /*i*/ complex_real(&out) *  (*mult_res_by_i) );

    return(out);
}

#endif /* defined(USE_OWN_COMPLEX_IMPL) */

/* [that's enough complex algebra - Ed] */

/* end of mathxcpx.c */
