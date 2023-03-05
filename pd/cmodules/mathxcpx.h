/* mathxcpx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions */

/* Split off from ev_mcpx.c December 2019 */

#ifndef MATHXCPX_H
#define MATHXCPX_H

#if defined(__CC_NORCROFT) && defined(__SOFTFP__) // ??? && !defined(USE_OWN_COMPLEX_IMPL)
#include <complex.h>

/* must be included AFTER complex.h to override standard library's functions & macros */
#include "external/SSwales/apcs_softpcs/apcs_softpcs_complex.h"
#endif /* __CC_NORCROFT */

#if defined(__cplusplus) || (__STDC_VERSION__ < 199901L) || defined(__STDC_NO_COMPLEX__) || defined(USE_OWN_COMPLEX_IMPL) || 0
#if !defined(USE_OWN_COMPLEX_IMPL)
#define USE_OWN_COMPLEX_IMPL 1 /* for subsequent testing */
#endif
#endif /* ... */

/*
declare complex number type for internal usage
*/

#if !defined(USE_OWN_COMPLEX_IMPL)

#include <complex.h>

typedef
#if defined(_MSC_VER)
    _Dcomplex /* still not C99! */
#else
    double _Complex
#endif
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

#if defined(CMPLX)
#define COMPLEX_INIT(real_value, imag_value) CMPLX(real_value, imag_value)
#elif defined(_MSC_VER)
#define COMPLEX_INIT(real_value, imag_value) { (real_value), (imag_value) } /* yuk */
#else
#define COMPLEX_INIT(real_value, imag_value) ((real_value) + I*(imag_value))
#endif

_Check_return_
static
#if !defined(__SOFTFP__) /* compiler barf */
inline
#endif
double
complex_real(
    _InRef_ PC_COMPLEX z)
{
    return(creal(*z));
}

_Check_return_
static inline double
complex_imag(
    _InRef_ PC_COMPLEX z)
{
    return(cimag(*z));
}

_Check_return_
static inline COMPLEX
complex_ri(
    _InVal_ double real_value,
    _InVal_ double imag_value)
{
#if defined(CMPLX)
    return(CMPLX(real_value, imag_value));
#elif defined(_MSC_VER)
    return(_Cbuild(real_value, imag_value));
#else
    const COMPLEX z = COMPLEX_INIT( real_value, imag_value );
    return(z);
#endif
}

static inline void
complex_set_ri(
    _OutRef_ P_COMPLEX z,
    _InVal_ double real_value,
    _InVal_ double imag_value)
{
#if defined(CMPLX)
    *z = CMPLX(real_value, imag_value);
#elif defined(_MSC_VER)
    *z = _Cbuild(real_value, imag_value);
#else
    *z =  COMPLEX_INIT( real_value, imag_value );
#endif
}

#define inline_if_native_complex inline

#else /* defined(USE_OWN_COMPLEX_IMPL) */

typedef struct COMPLEX
{
    double r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

#define COMPLEX_INIT(real_value, imag_value) { (real_value), (imag_value) }

_Check_return_
static
#if !(defined(__CC_NORCROFT) && defined(__SOFTFP__))
inline
#endif
double
complex_real(
    _InRef_ PC_COMPLEX z)
{
    return(z->r);
}

_Check_return_
static inline double
complex_imag(
    _InRef_ PC_COMPLEX z)
{
    return(z->i);
}

_Check_return_
static inline COMPLEX
complex_ri(
    _InVal_ double real_value,
    _InVal_ double imag_value)
{
    const COMPLEX z = COMPLEX_INIT( real_value,  imag_value );
    return(z);
}

static inline void
complex_set_ri(
    _OutRef_    P_COMPLEX z,
    _InVal_     double real_value,
    _InVal_     double imag_value)
{
    z->r = real_value;
    z->i = imag_value;
}

#define inline_if_native_complex /*nothing*/

#endif/* defined(USE_OWN_COMPLEX_IMPL) */

/*
exported data
*/

extern const COMPLEX
complex_zero;

extern const COMPLEX
complex_Re_one;

extern const COMPLEX
complex_Re_one_half;

/*
exported functions
*/

/* z = x + iy = r.e^i@ */

_Check_return_
extern double
complex_modulus( /* |z| */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern double
complex_argument( /* arg(z) */
    _InRef_     PC_COMPLEX z);

/* basic complex arithmetic */

_Check_return_
extern COMPLEX
complex_add( /* z1 + z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2);

_Check_return_
extern COMPLEX
complex_subtract( /* z1 - z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2);

_Check_return_
extern COMPLEX
complex_multiply( /* z1 * z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2);

_Check_return_
extern COMPLEX
complex_divide( /* z1 / z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2);

_Check_return_
extern COMPLEX
complex_conjugate( /* conj(z) */
    _InRef_     PC_COMPLEX z);

/* complex exponent, logarithms, powers */

_Check_return_
extern COMPLEX
complex_exp( /* e^z */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_log_e( /* log_e(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_log_2( /* log_2(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_log_10( /* log_10(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_power( /* z1^z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2);

_Check_return_
extern COMPLEX
complex_square_root( /* sqrt(z) */
    _InRef_     PC_COMPLEX z);

/* complex trig functions */

_Check_return_
extern COMPLEX
complex_cosine( /* cos(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_sine( /* sin(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_tangent( /* tan(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_secant( /* sec(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_cosecant( /* csc(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_cotangent( /* cot(z) */
    _InRef_     PC_COMPLEX z);

/* complex inverse trig functions */

_Check_return_
extern COMPLEX
complex_arc_cosine( /* arccos(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_arc_sine( /* arcsin(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_arc_tangent( /* arctan(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_arc_secant( /* arcsec(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_arc_cosecant( /* arccsc(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_arc_cotangent( /* arccot(z) */
    _InRef_     PC_COMPLEX z);

/* complex hyperbolic functions */

_Check_return_
extern COMPLEX
complex_hyperbolic_cosine( /* cosh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_hyperbolic_sine( /* sinh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_hyperbolic_tangent( /* tanh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_hyperbolic_secant( /* sech(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_hyperbolic_cosecant( /* csch(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_hyperbolic_cotangent( /* coth(z) */
    _InRef_     PC_COMPLEX z);

/* complex inverse hyperbolic functions */

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cosine( /* acosh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_sine( /* asinh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_tangent( /* atanh(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_secant( /* asech(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cosecant( /* acsch(z) */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern COMPLEX
complex_inverse_hyperbolic_cotangent( /* acoth(z) */
    _InRef_     PC_COMPLEX z);

#endif /* MATHXCPX_H */

/* end of mathxcpx.h  */
