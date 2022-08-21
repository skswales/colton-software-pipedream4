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

_Check_return_
static bool
f64_is_divisor_too_small(
    _InVal_     F64 divisor)
{
    if(isgreater(fabs(divisor), F64_MIN))
        return(false);/* attempt the division, should be OK */

    return(true); /* don't attempt the division, flag a DIV/0 error */
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* p_ss_data_res = error iff either p_ss_data_1 or p_ss_data_2 are error
*
******************************************************************************/

_Check_return_ _Success_(return)
static inline bool
two_nums_propagate_errors(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    if(ss_data_is_error(p_ss_data_1))
    {
        *p_ss_data_res = *p_ss_data_1;
        return(TRUE);
    }

    if(ss_data_is_error(p_ss_data_2))
    {
        *p_ss_data_res = *p_ss_data_2;
        return(TRUE);
    }

    return(FALSE);
}

#endif

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 + p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_add_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) + ss_data_get_integer(p_ss_data_2));
        return(TRUE);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) + ss_data_get_real(p_ss_data_2));
        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 / p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_divide_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        if(0 == ss_data_get_integer(p_ss_data_2))
            ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
        else
        {
            const div_t d = div((int) ss_data_get_integer(p_ss_data_1), (int) ss_data_get_integer(p_ss_data_2));

            if(0 == d.rem)
                ss_data_set_integer(p_ss_data_res, d.quot);
            else
                ss_data_set_real(p_ss_data_res, (F64) ss_data_get_integer(p_ss_data_1) / ss_data_get_integer(p_ss_data_2));
        }

        return(TRUE);

    case TWO_REALS:
        if(f64_is_divisor_too_small(ss_data_get_real(p_ss_data_2)))
            ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
        else
            ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) / ss_data_get_real(p_ss_data_2));

        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 * p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_multiply_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) * ss_data_get_integer(p_ss_data_2));
        return(TRUE);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) * ss_data_get_real(p_ss_data_2));
        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 - p_ss_data_2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_subtract_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    switch(two_nums_type_match(p_ss_data_1, p_ss_data_2, TRUE))
    {
    case TWO_INTS:
        ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) - ss_data_get_integer(p_ss_data_2));
        return(TRUE);

    case TWO_REALS:
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) - ss_data_get_real(p_ss_data_2));
        return(TRUE);
    }

    return(FALSE);
}

/* end of ev_arith.c */
