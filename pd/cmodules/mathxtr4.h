/* mathxtr4.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2025 Stuart Swales */

/* Additional math routines */

/* SKS Dec 2014 */

#ifndef MATHXTR4_H
#define MATHXTR4_H

/*
exported functions
*/

/* log(e) B(a,b) */

_Check_return_
extern F64
mathxtra_ln_beta(
    _InVal_     F64 a,
    _InVal_     F64 b);

/* log(e) (1 / B(a,b)) */

_Check_return_
static inline F64
mathxtra_ln_reciprocal_beta(
    _InVal_     F64 alpha,
    _InVal_     F64 beta)
{
    return(-mathxtra_ln_beta(alpha, beta));
}

/* Ix(alpha, beta) is the regularized incomplete beta function */

_Check_return_
extern F64
mathxtra_Ix_beta(
    _InVal_     F64 x, /* in [0..1] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters, both > 0 */
    _InVal_     F64 beta);

/* log(e) (n k) */

_Check_return_
extern F64
mathxtra_ln_binomial_coefficient(
    _InVal_     S32 n,
    _InVal_     S32 k);

/* log(e) n! */

_Check_return_
static inline F64
mathxtra_ln_factorial(
    _InVal_     S32 n)
{
    return(lgamma(n + 1.0));
}

/* Regularized gamma function P(s,x) */

_Check_return_
extern F64
mathxtra_P_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Regularized gamma function Q(s,x) */

_Check_return_
extern F64
mathxtra_Q_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Lower incomplete gamma function y(s,x) (Little gamma) */

_Check_return_
extern F64
mathxtra_y_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Upper incomplete gamma function Gu(s,x) (Big gamma) */

_Check_return_
extern F64
mathxtra_Gu_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

#endif /* MATHXTR4_H */

/* end of mathxtr4.h  */
