/* ev_matb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2016 Stuart Swales */

/* More mathematical function routines for evaluator */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"

/* local header file */
#include "ev_evali.h"

#include <math.h>
#include <float.h>
#include <errno.h>

/******************************************************************************
*
* More mathematical functions for OpenDocument / Microsoft Excel support
*
******************************************************************************/

/******************************************************************************
*
* REAL log(x:number {, base:number=10})
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    const F64 x = args[0]->arg.fp;
    const F64 b = (n_args > 1) ? args[1]->arg.fp : 10.0;
    F64 log_result;

    exec_func_ignore_parms();

    errno = 0;

    if(b == 1.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return;
    }
    else if(b == 2.0)
    {
        log_result = log2(x);
    }
    else if(b == 10.0)
    {
        log_result = log10(x);
    }
    else
    {
        const F64 ln_x = log(x);
        const F64 ln_b = log(b);

        log_result = ln_x / ln_b;
    }

    ev_data_set_real(p_ev_data_res, log_result);

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER ceiling(n {, multiple})
*
* NUMBER floor(n {, multiple})
*
* REAL round(n, decimals)
*
******************************************************************************/

extern void
round_common(
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     U32 rpn_did_num)
{
    BOOL negate_result = FALSE;
    F64 f64, multiplier;

    switch(rpn_did_num)
    {
    case RPN_FNV_CEILING:
    case RPN_FNV_FLOOR:
        {
        f64 = args[0]->arg.fp;

        if(n_args <= 1)
        {
            multiplier = 1.0;
        }
        else
        {
            F64 multiple =  args[1]->arg.fp;

            if(multiple < 0.0) /* Handle the Excel way of doing these */
            {
                multiple = fabs(multiple);

                if(f64 > 0.0)
                {
                    ev_data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SIGNS);
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
                ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
                return;
            }

            multiplier = 1.0 / multiple; 
        }

        break;
        }

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_ROUND:
  /*case RPN_FNF_ROUNDDOWN:*/
  /*case RPN_FNF_ROUNDUP:*/
  /*case RPN_FNV_TRUNC:*/
#endif
        {
        S32 decimal_places = 2;

        if(n_args > 1)
            decimal_places = MIN(15, args[1]->arg.integer);

        switch(args[0]->did_num)
        {
        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            if(decimal_places >= 0)
            {   /* if we have an integer number to be rounded at, or to the right of, the decimal, it's already there */
                /* NOP */
                *p_ev_data_res = *(args[0]);
                return;
            }

            f64 = (F64) args[0]->arg.integer; /* have to do it like this */
            break;

        default:
            f64 = args[0]->arg.fp;
            break;
        }

        /* operate on positive number */
        if(f64 < 0.0)
        {
            negate_result = TRUE;
            f64 = -f64;
        }

        multiplier = pow(10.0, (F64) decimal_places);
        break;
        }
    }

    f64 = f64 * multiplier;

    switch(rpn_did_num)
    {
  /*case RPN_FNF_MROUND:*/
    case RPN_FNV_ROUND:
        f64 = floor(f64 + 0.5); /* got a positive number here */
        break;

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_FLOOR:
  /*case RPN_FNF_ROUNDDOWN:*/
  /*case RPN_FNV_TRUNC:*/
#endif
        f64 = floor(f64);
        break;

    case RPN_FNV_CEILING:
  /*case RPN_FNF_ROUNDUP:*/
        f64 = ceil(f64);
        break;
    }

    f64 = f64 / multiplier;

    ev_data_set_real(p_ev_data_res, negate_result ? -f64 : f64);
}

PROC_EXEC_PROTO(c_ceiling)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_CEILING);

    (void) real_to_integer_try(p_ev_data_res);
}

PROC_EXEC_PROTO(c_floor)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_FLOOR);

    (void) real_to_integer_try(p_ev_data_res);
}

PROC_EXEC_PROTO(c_round)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND);

    (void) real_to_integer_try(p_ev_data_res);
}

/* end of ev_matb.c */
