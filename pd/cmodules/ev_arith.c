/* ev_arith.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Split off from ev_help.c */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

#if 0 /* just for diff minimization */

/******************************************************************************
*
* p_ss_data_res = error iff either p_ss_data_1 or p_ss_data_2 are error
*
******************************************************************************/

_Check_return_ _Success_(return)
static bool
two_nums_bop_propagate_errors(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    PC_SS_DATA p_ss_data_err = NULL;

    if(ss_data_is_error(p_ss_data_2))
        p_ss_data_err = p_ss_data_2;
    if(ss_data_is_error(p_ss_data_1))   /* no else - eliminates a branch */
        p_ss_data_err = p_ss_data_1;    /* any error from data_1 has priority */

    if(NULL == p_ss_data_err)
        return(false);

    return(true);
}

#endif

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 + p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern bool
two_nums_add_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) + ss_data_get_integer(p_ss_data_2));
        return(true);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) + ss_data_get_real(p_ss_data_2));
        return(true);
    }

    return(false);
}

/* see also ss_data_add() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 - p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern bool
two_nums_subtract_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) - ss_data_get_integer(p_ss_data_2));
        return(true);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) - ss_data_get_real(p_ss_data_2));
        return(true);
    }

    return(false);
}

/* see also ss_data_subtract() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 * p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern bool
two_nums_multiply_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) * ss_data_get_integer(p_ss_data_2));
        return(true);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) * ss_data_get_real(p_ss_data_2));
        return(true);
    }

    return(false);
}

/* see also ss_data_multiply() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 / p_ss_data_2
*
******************************************************************************/

_Check_return_
static inline bool
f64_is_p_divisor_too_small(
    _InRef_     PC_F64 p_divisor)
{
    if(isgreater(fabs(*p_divisor), F64_MIN))
        return(false);/* attempt the division, should be OK */

    return(true); /* don't attempt the division, flag a DIV/0 error */
}

_Check_return_
static inline bool
f64_is_divisor_too_small(
    _InVal_     F64 divisor)
{
    return(f64_is_p_divisor_too_small(&divisor));
}

_Check_return_
static bool
calc_divide_by_zero(
    _OutRef_    P_SS_DATA p_ss_data_res)
{
    ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
    return(true);
}

#define FASTER_REAL_DIVIDE 1

#if defined(FASTER_REAL_DIVIDE)

_Check_return_
static bool
calc_divide_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     PC_F64 p_dividend,
    _InVal_     PC_F64 p_divisor)
{
    if(f64_is_p_divisor_too_small(p_divisor))
        return(calc_divide_by_zero(p_ss_data_res));

    ss_data_set_real(p_ss_data_res, *p_dividend / *p_divisor);
    return(true);
}

#define two_nums_divide_try_two_reals(p_ss_data_res, pc_ss_data_1, pc_ss_data_2) \
    calc_divide_two_reals(p_ss_data_res, ss_data_get_pc_real(pc_ss_data_1), ss_data_get_pc_real(pc_ss_data_2))

#else /* !FASTER_REAL_DIVIDE */

_Check_return_
static bool
calc_divide_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     F64 dividend,
    _InVal_     F64 divisor)
{
    if(f64_is_divisor_too_small(divisor))
        return(calc_divide_by_zero(p_ss_data_res));

    ss_data_set_real(p_ss_data_res, dividend / divisor);
    return(true);
}

_Check_return_
static bool
two_nums_divide_try_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    const F64 dividend = ss_data_get_real(p_ss_data_1);
    const F64 divisor  = ss_data_get_real(p_ss_data_2);

    return(calc_divide_two_reals(p_ss_data_res, dividend, divisor));
}

#endif /* FASTER_REAL_DIVIDE */

_Check_return_
static bool
two_nums_divide_try_two_integers_as_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S32 s32_dividend,
    _InVal_     S32 s32_divisor)
{
    const F64 dividend = (F64) s32_dividend;
    const F64 divisor  = (F64) s32_divisor;

#if defined(FASTER_REAL_DIVIDE)
    return(calc_divide_two_reals(p_ss_data_res, &dividend, &divisor));
#else
    return(calc_divide_two_reals(p_ss_data_res, dividend, divisor));
#endif /* FASTER_REAL_DIVIDE */
}

_Check_return_
static bool
two_nums_divide_try_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    const S32 dividend = ss_data_get_integer(p_ss_data_1);
    const S32 divisor  = ss_data_get_integer(p_ss_data_2);

    if(0 != divisor)
    {
        const S32 quotient  = dividend / divisor;
        const S32 remainder = dividend % divisor;

        if(0 == remainder)
        {
            ss_data_set_integer(p_ss_data_res, quotient);
            return(true);
        }
    }

    return(two_nums_divide_try_two_integers_as_reals(p_ss_data_res, dividend, divisor));
}

_Check_return_ _Success_(return)
extern bool
two_nums_divide_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        return(two_nums_divide_try_two_integers(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    case TWO_REALS:
        return(two_nums_divide_try_two_reals(p_ss_data_res, p_ss_data_1, p_ss_data_2));
    }

    return(false);
}

/* see also ss_data_divide() */

/* end of ev_arith.c */
