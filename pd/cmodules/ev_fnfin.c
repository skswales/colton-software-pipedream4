/* ev_fnfin.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Financial function routines for evaluator */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

/******************************************************************************
*
* Financial functions
*
******************************************************************************/

/*
Financial functions calculated using array_range processing in ev_exec.c:
IRR()
MIRR()
NPV()
*/

/******************************************************************************
*
* REAL cterm(interest, fv, pv)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cterm)
{
    F64 interest = args[0]->arg.fp + 1.0;
    F64 fv = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;
    F64 cterm_result;
    BOOL fv_negative = FALSE; /* SKS - allow it to work out debts like Fireworkz */
    BOOL pv_negative = FALSE;
    exec_func_ignore_parms();

    if(fv < 0.0)
    {
        fv = fabs(fv);
        fv_negative = TRUE;
    }

    if(pv < 0.0)
    {
        pv = fabs(pv);
        pv_negative = TRUE;
    }

    if(fv_negative != pv_negative)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SIGNS);
        return;
    }

    if(interest <= F64_MIN)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    cterm_result = (log(fv) - log(pv)) / log(interest);

    ev_data_set_real(p_ev_data_res, cterm_result);
}

/******************************************************************************
*
* REAL ddb(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ddb)
{
    F64 cost = args[0]->arg.fp;
    F64 value = cost;
    F64 salvage = args[1]->arg.fp;
    S32 life = (S32) args[2]->arg.fp;
    S32 period = (S32) args[3]->arg.fp;
    F64 cur_period = 0.0; /* ddb_result */
    S32 i;

    exec_func_ignore_parms();

    if(cost < 0.0     ||
       salvage > cost ||
       life < 1       ||
       period < 1     ||
       period > life)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    for(i = 0; i < period; ++i)
    {
        cur_period = (value * 2) / life;
        if(value - cur_period < salvage)
            cur_period = value - salvage;
        value -= cur_period;
    }

    ev_data_set_real(p_ev_data_res, cur_period);
}

/******************************************************************************
*
* REAL fv(payment, interest, term)
*
* REAL fv_xls(interest, term, payment)
*
******************************************************************************/

_Check_return_
static F64
fv_common(
    _InVal_     F64 payment,
    _InVal_     F64 interest,
    _InVal_     F64 term)
{
    /* payment * ((1 + interest) ^ term - 1) / interest */
    return(payment * (pow(1.0 + interest, term) - 1.0) / interest);
}

PROC_EXEC_PROTO(c_fv)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* fv(payment, interest, term) = payment * ((1 + interest) ^ term - 1) / interest */
    ev_data_set_real(p_ev_data_res, fv_common(payment, interest, term));
}

#if 0 /* just for diff minimization */

PROC_EXEC_PROTO(c_fv_xls)
{
    F64 interest = args[0]->arg.fp;
    F64 term = args[1]->arg.fp;
    F64 payment = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* fv_xls(interest, term, payment) = - payment * ((1 + interest) ^ term - 1) / interest */
    ev_data_set_real(p_ev_data_res, - fv_common(payment,  interest,  term));
}

#endif

/******************************************************************************
*
* REAL pmt(principal, interest, term)
*
* REAL pmt_xls(rate, nper, pv)
*
******************************************************************************/

static F64
pmt_common(
    _InVal_     F64 principal,
    _InVal_     F64 interest,
    _InVal_     F64 term)
{
    /* principal * interest / (1-(interest+1)^(-term)) */
    return(principal * interest / (1.0 - pow(interest + 1.0, - term)));
}

PROC_EXEC_PROTO(c_pmt)
{
    F64 principal = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* pmt(principal, interest, term) = principal * interest / (1-(interest+1)^(-term)) */
    ev_data_set_real(p_ev_data_res, pmt_common(principal, interest, term));
}

#if 0 /* just for diff minimisation */

PROC_EXEC_PROTO(c_pmt_xls)
{
    F64 rate = args[0]->arg.fp;
    F64 nper = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* pmt_xls(rate, nper, pv) = - pv * rate / (1-(rate+1)^(-nper)) */
    ev_data_set_real(p_ev_data_res, - pmt_common(pv, rate, nper));
}

#endif

/******************************************************************************
*
* REAL pv(payment, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pv)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    /* payment * (1-(1+interest)^(-term) / interest */
    F64 pv_result = payment * (1.0 - pow(1.0 + interest, -term)) / interest;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, pv_result);
}

/******************************************************************************
*
* REAL rate(fv, pv, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rate)
{
    F64 fv = args[0]->arg.fp;
    F64 pv = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    /* rate(fv, pv, term) = (fv / pv) ^ (1/term) -1  */
    F64 rate_result = pow((fv / pv), 1.0 / term) - 1.0;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, rate_result);
}

/******************************************************************************
*
* REAL sln(cost, salvage, life)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sln)
{
    F64 cost = args[0]->arg.fp;
    F64 salvage = args[1]->arg.fp;
    F64 life = args[2]->arg.fp;
    F64 sln_result = (cost - salvage) / life;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, sln_result);
}

/******************************************************************************
*
* REAL syd(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_syd)
{
    F64 cost = args[0]->arg.fp;
    F64 salvage = args[1]->arg.fp;
    F64 life = args[2]->arg.fp;
    F64 period = args[3]->arg.fp;
    F64 syd_result;

    exec_func_ignore_parms();

    /* syd(cost, salvage, life, period) = (cost-salvage) * (life-period+1) / (life*(life+1)/2) */
    syd_result = ((cost - salvage) * (life - period + 1.0)) / ((life * (life + 1.0) / 2.0));

    ev_data_set_real(p_ev_data_res, syd_result);
}

/******************************************************************************
*
* REAL term(payment, interest, fv)
*
* REAL nper(rate, payment, pv)
*
******************************************************************************/

static F64
term_nper_common(
    _InVal_     F64 payment,
    _InVal_     F64 interest,
    _InVal_     F64 fv)
{
    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    F64 numer = log(1.0 + (fv * interest / payment));
    F64 denom = log(1.0 + interest);
    F64 result = numer / denom;
    return(result);
}

#if 0 /* just for diff minimisation */

PROC_EXEC_PROTO(c_nper)
{
    F64 rate = args[0]->arg.fp;
    F64 payment = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* Excel: really up to 5 args but I don't yet support that */

    /* Excel: like TERM() but different parameter order and result sign */
    /* nper(rate, payment, pv) = - ln(1+(pv * rate/payment)) / ln(1+rate) */
    ev_data_set_real(p_ev_data_res, - term_nper_common(payment, rate, pv));
}

#endif

PROC_EXEC_PROTO(c_term)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp ;
    F64 fv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    ev_data_set_real(p_ev_data_res, term_nper_common(payment, interest, fv));
}

/* end of ev_fnfin.c */
