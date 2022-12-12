/* ev_exec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Semantic routines for evaluator */

/* MRJC April 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/******************************************************************************
*
* LOGICAL unary not operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_not)
{
    const bool not_result = !(ss_data_get_logical(args[0]));

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, not_result);
}

/******************************************************************************
*
* NUMBER unary plus operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_plus)
{
    exec_func_ignore_parms();

    assert(ss_data_is_number(args[0]));
    *p_ss_data_res = *(args[0]);
}

/******************************************************************************
*
* NUMBER unary minus operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_minus)
{
    exec_func_ignore_parms();

    if(ss_data_is_real(args[0]))
        ss_data_set_real(p_ss_data_res, -ss_data_get_real(args[0]));
    else
        ss_data_set_integer(p_ss_data_res, -ss_data_get_integer(args[0]));
}

/******************************************************************************
*
* VALUE binary addition
*
* res = a+b
*
******************************************************************************/

static bool
date_date_add_number_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     SS_DATE_DATE ss_date_date,
    _InVal_     S32 s32)
{
    if( INT32_MIN != (p_ss_data_res->arg.ss_date.date = int32_add_overflowed(s32, ss_date_date)) )
        return(true);

    ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
    return(false);
}

static bool
date_time_add_number_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     SS_DATE_TIME ss_date_time,
    _InVal_     S32 s32)
{
    if( INT32_MIN != (p_ss_data_res->arg.ss_date.time = int32_add_overflowed(s32, ss_date_time)) )
        return(true);

    ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
    return(false);
}

static void
date_add_number_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date,
    _InVal_     S32 s32)
{
    ss_data_set_data_id(p_ss_data_res, DATA_ID_DATE);

    if(SS_DATE_NULL != p_ss_date->date)
    {
        if(!date_date_add_number_calc(p_ss_data_res, p_ss_date->date, s32))
            return;

        p_ss_data_res->arg.ss_date.time = p_ss_date->time;
    }
    else
    {
        p_ss_data_res->arg.ss_date.date = SS_DATE_NULL;

        if(!date_time_add_number_calc(p_ss_data_res, p_ss_date->time, s32))
            return;

        ss_date_normalise(&p_ss_data_res->arg.ss_date);
    }
}

static void
date_add_date_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date_addend_a,
    _InRef_     PC_SS_DATE p_ss_date_addend_b)
{
    ss_data_set_data_id(p_ss_data_res, DATA_ID_DATE);

    if( !date_date_add_number_calc(p_ss_data_res, p_ss_date_addend_a->date, p_ss_date_addend_b->date) ||
        !date_time_add_number_calc(p_ss_data_res, p_ss_date_addend_a->time, p_ss_date_addend_b->time) )
    {
        return;
    }

    ss_date_normalise(&p_ss_data_res->arg.ss_date);
}

static void
add_harder_addend_a_is_number(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_addend_a,
    _InRef_     PC_SS_DATA p_ss_data_addend_b)
{
    switch(ss_data_get_data_id(p_ss_data_addend_b))
    {
    /* number + date */
    case DATA_ID_DATE:
        date_add_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_b), ss_data_get_integer(p_ss_data_addend_a));
        break;
    }
}

static void
add_harder_addend_a_is_date(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     P_SS_DATA p_ss_data_addend_a,
    _InRef_     P_SS_DATA p_ss_data_addend_b)
{
    switch(ss_data_get_data_id(p_ss_data_addend_b))
    {
    case DATA_ID_REAL:
        ss_data_real_to_integer_force(p_ss_data_addend_b);

        /*FALLTHRU*/

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        /* date + number */
        date_add_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_a), ss_data_get_integer(p_ss_data_addend_b));
        break;

    case DATA_ID_DATE:
        /* date + date */
        date_add_date_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_a), ss_data_get_date(p_ss_data_addend_b));
        break;
    }
}

static void
add_harder(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     P_SS_DATA p_ss_data_addend_a,
    _InRef_     P_SS_DATA p_ss_data_addend_b)
{
    switch(ss_data_get_data_id(p_ss_data_addend_a))
    {
    case DATA_ID_REAL:
        ss_data_real_to_integer_force(p_ss_data_addend_a);

    /*FALLTHRU*/

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        add_harder_addend_a_is_number(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;

    case DATA_ID_DATE:
        add_harder_addend_a_is_date(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;
    }
}

PROC_EXEC_PROTO(c_bop_add)
{
    exec_func_ignore_parms();

    if(two_nums_add_try(p_ss_data_res, args[0], args[1]))
        return;

    add_harder(p_ss_data_res, args[0], args[1]);
}

/******************************************************************************
*
* VALUE binary subtraction
*
* res = a-b
*
******************************************************************************/

static void
date_subtract_date_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date_minuend,
    _InRef_     PC_SS_DATE p_ss_date_subtrahend)
{
    ss_data_set_data_id(p_ss_data_res, DATA_ID_DATE);

    if( !date_date_add_number_calc(p_ss_data_res, p_ss_date_minuend->date, -p_ss_date_subtrahend->date) ||
        !date_time_add_number_calc(p_ss_data_res, p_ss_date_minuend->time, -p_ss_date_subtrahend->time) )
    {
        return;
    }

    if( ss_data_get_date(p_ss_data_res)->date && ss_data_get_date(p_ss_data_res)->time )
    {
        ss_date_normalise(&p_ss_data_res->arg.ss_date);
    }
    else
    {
        ss_data_set_integer(p_ss_data_res,
                            (ss_data_get_date(p_ss_data_res)->date
                                    ? ss_data_get_date(p_ss_data_res)->date
                                    : ss_data_get_date(p_ss_data_res)->time));
    }
}

static void
subtract_harder_minuend_is_number(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_minuend,
    _InRef_     PC_SS_DATA p_ss_data_subtrahend)
{
    UNREFERENCED_PARAMETER_InRef_(p_ss_data_minuend);

    switch(ss_data_get_data_id(p_ss_data_subtrahend))
    {
    /* number - date */
    case DATA_ID_DATE:
        ss_data_set_error(p_ss_data_res, EVAL_ERR_UNEXDATE);
        break;
    }
}

static void
subtract_harder_minuend_is_date(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     P_SS_DATA p_ss_data_minuend,
    _InRef_     P_SS_DATA p_ss_data_subtrahend)
{
    switch(ss_data_get_data_id(p_ss_data_subtrahend))
    {
    case DATA_ID_REAL:
        ss_data_real_to_integer_force(p_ss_data_subtrahend);

        /*FALLTHRU*/

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        /* date - number */
        date_add_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_minuend), -ss_data_get_integer(p_ss_data_subtrahend));
        break;

    case DATA_ID_DATE:
        /* date - date */
        date_subtract_date_calc(p_ss_data_res, ss_data_get_date(p_ss_data_minuend), ss_data_get_date(p_ss_data_subtrahend));
        break;
    }
}

static inline void
subtract_harder(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     P_SS_DATA p_ss_data_minuend,
    _InRef_     P_SS_DATA p_ss_data_subtrahend)
{
    switch(ss_data_get_data_id(p_ss_data_minuend))
    {
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        subtract_harder_minuend_is_number(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;

    case DATA_ID_DATE:
        subtract_harder_minuend_is_date(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;
    }
}

PROC_EXEC_PROTO(c_bop_sub)
{
    exec_func_ignore_parms();

    if(two_nums_subtract_try(p_ss_data_res, args[0], args[1]))
        return;

    subtract_harder(p_ss_data_res, args[0], args[1]);
}

/******************************************************************************
*
* VALUE binary multiplication
*
* res = a*b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_mul)
{
    exec_func_ignore_parms();

    consume_bool(two_nums_multiply_try(p_ss_data_res, args[0], args[1]));
}

/******************************************************************************
*
* VALUE binary division
*
* res = a/b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_div)
{
    exec_func_ignore_parms();

    consume_bool(two_nums_divide_try(p_ss_data_res, args[0], args[1]));
}

/******************************************************************************
*
* NUMBER res = a ^ b
*
* NUMBER power(a, b) is equivalent OpenDocument / Microsoft Excel function
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_power)
{
    const F64 a = ss_data_get_real(args[0]);
    const F64 b = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    errno = 0;

    if(0.5 == b)
        ss_data_set_real_try_integer(p_ss_data_res, sqrt(a));
    else
        ss_data_set_real_try_integer(p_ss_data_res, pow(a, b));

    if(errno /* == EDOM */)
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);

        /* test for negative numbers to power of 1/odd number (odd roots are OK) */
        if( (a < 0.0) && (b <= 1.0) )
        {
            SS_DATA test;

            ss_data_set_real(&test, 1.0 / b);

            if(ss_data_real_to_integer_try(&test))
                ss_data_set_real_try_integer(p_ss_data_res, -(pow(-(a), b)));
        }
    }
}

/******************************************************************************
*
* LOGICAL res = a && b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_and)
{
    const bool and_result = ss_data_get_logical(args[0]) && ss_data_get_logical(args[1]);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, and_result);
}

/******************************************************************************
*
* LOGICAL res = a|b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_or)
{
    const bool or_result = ss_data_get_logical(args[0]) || ss_data_get_logical(args[1]);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, or_result);
}

/******************************************************************************
*
* LOGICAL res = a==b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_eq)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) == 0));
}

/******************************************************************************
*
* LOGICAL res = a>b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_gt)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) > 0));
}

/******************************************************************************
*
* LOGICAL res = a>=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_gteq)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) >= 0));
}

/******************************************************************************
*
* LOGICAL res = a<b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_lt)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) < 0));
}

/******************************************************************************
*
* LOGICAL res = a<=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_lteq)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) <= 0));
}

/******************************************************************************
*
* LOGICAL res = a!=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_neq)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, (ss_data_compare(args[0], args[1]) != 0));
}

/******************************************************************************
*
* VALUE if(num, true_res, false_res)
*
******************************************************************************/

PROC_EXEC_PROTO(c_if)
{
    exec_func_ignore_parms();

    if(ss_data_get_logical(args[0]))
        ss_data_resource_copy(p_ss_data_res, args[1]);
    else if(n_args > 2)
        ss_data_resource_copy(p_ss_data_res, args[2]);
    else
        exec_func_status_return(p_ss_data_res, EVAL_ERR_NA);
}

/******************************************************************************
*
* set the value of a cell
*
******************************************************************************/

static S32
poke_slot(
    _InRef_     PC_EV_SLR slrp,
    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_SLR cur_slrp)
{
    P_EV_CELL p_ev_cell;
    S32 res;

    /* can't overwrite formula cells */
    if((ev_travel(&p_ev_cell, slrp) > 0) && (p_ev_cell->parms.type != EVS_CON_DATA))
        res = create_error(EVAL_ERR_UNEXFORMULA);
    #ifdef SLOTS_MOVE
    else if( (ev_slr_docno(slrp) == ev_slr_docno(cur_slrp)) &&
             (ev_slr_col(slrp) == ev_slr_col(cur_slrp)) &&
             (ev_slr_row(slrp) <= ev_slr_row(cur_slrp)) )
        res = create_error(EVAL_ERR_OWNCOLUMN);
    #endif
    else
    {
        SS_DATA temp;
        EV_RESULT ev_result;

        ev_result_free_resources(&p_ev_cell->ev_result);

        /* make sure we have a copy of data */
        ss_data_resource_copy(&temp, p_ss_data);

        ss_data_to_result_convert(&ev_result, &temp);

        if((res = ev_make_slot(slrp, &ev_result)) < 0)
            ev_result_free_resources(&ev_result);
        else
        {
            UREF_PARM urefb;

            /* notify anybody about the change */
            urefb.slr1   = *slrp;
            urefb.action = UREF_CHANGE;
            ev_uref(&urefb);
            urefb.action = UREF_REDRAW;
            ev_uref(&urefb);
        }
    }

    return(res);
}

/******************************************************************************
*
* VALUE set_value(slr/range, value) - set the value of a cell
*
******************************************************************************/

PROC_EXEC_PROTO(c_setvalue)
{
    S32 err = 0;

    exec_func_ignore_parms();

    /* initialise output so we can free OK */
    ss_data_set_data_id(p_ss_data_res, DATA_ID_BLANK);

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_SLR:
        {
        if(!slr_equal(p_cur_slr, &args[0]->arg.slr))
            err = poke_slot(&args[0]->arg.slr, args[1], p_cur_slr);

        if(!err)
            ss_data_resource_copy(p_ss_data_res, args[1]);

        break;
        }

    case DATA_ID_RANGE:
        {
        RANGE_SCAN_BLOCK rsb;
        EV_SLR top_left;

        if((err = range_scan_init(&args[0]->arg.range, &rsb)) >= 0)
        {
            SS_DATA element;
            S32 first;

            first = 1;
            top_left = rsb.slr_of_result;

            while(range_scan_element(&rsb,
                                     &element,
                                     EM_ANY) != RPN_FRM_END)
            {
                S32 res;
                SS_DATA poke_data;

                switch(ss_data_get_data_id(args[1]))
                {
                case DATA_ID_RANGE:
                case RPN_TMP_ARRAY:
                case RPN_RES_ARRAY:
                    array_range_index(&poke_data,
                                      args[1],
                                      rsb.slr_of_result.col - rsb.range.s.col,
                                      rsb.slr_of_result.row - rsb.range.s.row, EM_ANY);
                    break;
                default:
                    poke_data = *args[1];
                    break;
                }

                /* poke cell duplicates resources,
                 * but don't poke to ourselves
                 */
                if(!slr_equal(p_cur_slr, &rsb.slr_of_result))
                {
                    if((res = poke_slot(&rsb.slr_of_result, &poke_data, p_cur_slr)) < 0)
                        if(!err)
                            err = res;
                }

                if(first)
                    ss_data_resource_copy(p_ss_data_res, &poke_data);

                first = 0;
            }
        }
        break;
        }

    /* we can't cope with arrays to poke */
    case RPN_RES_ARRAY:
    case RPN_TMP_ARRAY:
        err = create_error(EVAL_ERR_UNEXARRAY);
        break;
    }

    if(err < 0)
    {
        /* free anything we might have saved */
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, err);
    }
}

/*
range-ey functions
*/

static void
args_array_range_proc(
    P_SS_DATA args[EV_MAX_ARGS],
    S32 n_args,
    P_SS_DATA p_ss_data_res,
    _InVal_     EV_IDNO function_id);

static void
array_range_proc(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data);

static void
array_range_proc_array(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data);

static void
array_range_proc_finish(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block);

static void
array_range_proc_item(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data);

static S32
lookup_array_range_proc_array(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data);

static S32
lookup_array_range_proc_item(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data);

static EV_IDNO
npv_calc(
    P_SS_DATA p_ss_data_res,
    P_F64 intp,
    P_SS_DATA array);

static void
npv_item(
    P_SS_DATA p_ss_data,
    P_S32 p_count,
    P_F64 p_parm,
    P_F64 p_temp,
    P_F64 p_result);

/******************************************************************************
*
* avg(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_avg)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_AVG);
}

/******************************************************************************
*
* count(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_count)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_COUNT);
}

/******************************************************************************
*
* counta(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_counta)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_COUNTA);
}

/******************************************************************************
*
* irr(guess, array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_irr)
{
    F64 r = ss_data_get_real(args[0]);
    F64 last_npv = 1.0;
    F64 last_r = 0.0;
    S32 iteration_count;

    exec_func_ignore_parms();

    for(iteration_count = 0; iteration_count < 40; ++iteration_count)
    {
        F64 this_npv, this_r;
        SS_DATA npv;

        if(npv_calc(&npv, &r, args[1]) != DATA_ID_REAL)
            return;

        this_npv = ss_data_get_real(&npv);
        this_r = r;

        if(fabs(this_npv) < 0.0000001)
        {
            ss_data_set_real(p_ss_data_res, r);
            return;
        }

        if(0 == iteration_count)
        {   /* generate another point */
            r /= 2.0;
        }
        else
        {   /* new trial rate calculated using secant method */
            F64 step_r = this_npv * (r - last_r) / (this_npv - last_npv);
            r -= step_r;
        }

        last_npv = this_npv;
        last_r = this_r;
    }

    exec_func_status_return(p_ss_data_res, EVAL_ERR_IRR);
}

/******************************************************************************
*
* max(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_max)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_MAX);
}

/******************************************************************************
*
* min(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_min)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_MIN);
}

/******************************************************************************
*
* mirr(values_array, finance_rate, reinvest_rate)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mirr)
{
    STAT_BLOCK stb;

    exec_func_ignore_parms();

    stat_block_init(&stb, RPN_FNF_MIRR, ss_data_get_real(args[1]) + 1.0, ss_data_get_real(args[2]) + 1.0);

    array_range_proc(&stb, args[0]);

    array_range_proc_finish(p_ss_data_res, &stb);
}

/******************************************************************************
*
* npv(interest, array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_npv)
{
    exec_func_ignore_parms();

    npv_calc(p_ss_data_res, &args[0]->arg.fp, args[1]);
}

/******************************************************************************
*
* std(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_std)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_STD);
}

/******************************************************************************
*
* stdp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_stdp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_STDP);
}

/******************************************************************************
*
* sum(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_SUM);
}

/******************************************************************************
*
* var(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_var)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_VAR);
}

/******************************************************************************
*
* varp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_varp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ss_data_res, RPN_FNV_VARP);
}

/******************************************************************************
*
* process the arguments of the statistical functions
*
******************************************************************************/

static void
args_array_range_proc(
    P_SS_DATA args[EV_MAX_ARGS],
    S32 n_args,
    P_SS_DATA p_ss_data_res,
    _InVal_     EV_IDNO function_id)
{
    S32 i;
    STAT_BLOCK stb;

    stat_block_init(&stb, function_id, 0, 0);

    for(i = 0; i < n_args; ++i)
        array_range_proc(&stb, args[i]);

    array_range_proc_finish(p_ss_data_res, &stb);
}

/******************************************************************************
*
* process data for arrays and ranges for the statistical functions
*
******************************************************************************/

static void
array_range_proc(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        array_range_proc_array(p_stat_block, p_ss_data);
        break;

    case DATA_ID_RANGE:
        {
        RANGE_SCAN_BLOCK rsb;

        if(range_scan_init(&p_ss_data->arg.range, &rsb) >= 0)
        {
            SS_DATA element;

            while(range_scan_element(&rsb, &element, EM_REA | EM_ARY | EM_BLK | EM_DAT | EM_INT) != RPN_FRM_END)
            {
                switch(element.data_id)
                {
                case RPN_TMP_ARRAY:
                case RPN_RES_ARRAY:
                    array_range_proc_array(p_stat_block, &element);
                    break;
                default:
                    array_range_proc_item(p_stat_block, &element);
                    break;
                }
            }
        }

        break;
        }

    default:
        array_range_proc_item(p_stat_block, p_ss_data);
        break;
    }
}

/******************************************************************************
*
* process an array for array_range
*
******************************************************************************/

static void
array_range_proc_array(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    ARRAY_SCAN_BLOCK asb;

    if(array_scan_init(&asb, p_ss_data))
    {
        SS_DATA element;

        while(array_scan_element(&asb, &element, EM_REA | EM_AR0 | EM_BLK | EM_DAT | EM_INT) != RPN_FRM_END)
            array_range_proc_item(p_stat_block, &element);
    }
}

/******************************************************************************
*
* calculate the final result for the array_range functions
*
******************************************************************************/

static void
array_range_proc_finish_running_data(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 == p_stat_block->count)
        ss_data_set_real(p_ss_data, 0.0);
    else
    {
        switch(p_stat_block->running_data.data_id)
        {
        default:
#if CHECKING
            default_unhandled();
        case DATA_ID_REAL:
        case DATA_ID_DATE:
#endif
            *p_ss_data = p_stat_block->running_data;
            break;

        case DATA_ID_LOGICAL: /* really shouldn't occur */
        case DATA_ID_WORD16:
        case DATA_ID_WORD32:
            ss_data_set_integer(p_ss_data, ss_data_get_integer(&p_stat_block->running_data));
            break;
        }
    }
}

static void
array_range_proc_finish_average(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 == p_stat_block->count)
        ss_data_set_real(p_ss_data, 0.0);
    else
    {
        switch(p_stat_block->running_data.data_id)
        {
        default:
            ss_data_set_real(&p_stat_block->running_data, (F64) ss_data_get_integer(&p_stat_block->running_data));

            /*FALLTHRU*/

        case DATA_ID_REAL:
            {
            F64 avg_result = ss_data_get_real(&p_stat_block->running_data) / (F64) p_stat_block->count;
            ss_data_set_real_try_integer(p_ss_data, avg_result);
            break;
            }

        case DATA_ID_DATE:
            *p_ss_data = p_stat_block->running_data;
#if (SS_DATE_NULL == 0)
            p_ss_data->arg.ss_date.date /= p_stat_block->count;
            p_ss_data->arg.ss_date.time /= p_stat_block->count;
#else
            if(SS_DATE_NULL != p_ss_data->arg.ss_date.date)
                p_ss_data->arg.ss_date.date /= p_stat_block->count;
            if(SS_TIME_NULL != p_ss_data->arg.ss_date.time)
                p_ss_data->arg.ss_date.time /= p_stat_block->count;
#endif
            ss_date_normalise(&p_ss_data->arg.ss_date);
            break;
        }
    }
}

static void
array_range_proc_finish_COUNT(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ss_data_set_integer(p_ss_data, p_stat_block->count);
}

static void
array_range_proc_finish_COUNTA(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ss_data_set_integer(p_ss_data, p_stat_block->count_a);
}

_Check_return_
static F64
calc_variance(
    _InRef_     P_STAT_BLOCK p_stat_block,
    _InVal_     BOOL population_statistics)
{
    F64 n = (F64) p_stat_block->count;
    F64 n_sum_x2 = p_stat_block->sum_x2 * (F64) p_stat_block->count;
    F64 sum_2 = ss_data_get_real(&p_stat_block->running_data) * ss_data_get_real(&p_stat_block->running_data);
    F64 delta = n_sum_x2 - sum_2;
    F64 variance;

    if(delta < 0.0)
        delta = 0.0;

    variance = delta / n;
    variance /= population_statistics ? n : (n - 1.0);

    return(variance);
}

static void
array_range_proc_finish_standard_deviation(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    BOOL population_statistics = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_STDP:
        population_statistics = TRUE;
        break;

    default:
        break;
    }

    if(p_stat_block->count > (population_statistics ? 0 /*stdp*/: 1 /*std*/))
    {
        F64 variance = calc_variance(p_stat_block, population_statistics);
        F64 standard_deviation = sqrt(variance);

        ss_data_set_real(p_ss_data, standard_deviation);
        return;
    }

    ss_data_set_error(p_ss_data, EVAL_ERR_DIVIDEBY0);
}

static void
array_range_proc_finish_variance(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    BOOL population_statistics = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_VARP:
        population_statistics = TRUE;
        break;

    default:
        break;
    }

    if(p_stat_block->count > (population_statistics ? 0 /*varp*/: 1 /*var*/))
    {
        F64 variance = calc_variance(p_stat_block, population_statistics);

        ss_data_set_real(p_ss_data, variance);
        return;
    }

    ss_data_set_error(p_ss_data, EVAL_ERR_DIVIDEBY0);
}

static void
array_range_proc_finish_NPV(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 == p_stat_block->count)
        ss_data_set_real(p_ss_data, 0.0);
    else
        ss_data_set_real(p_ss_data, ss_data_get_real(&p_stat_block->running_data));
}

static inline F64
array_range_proc_MIRR_help(
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    return(  pow(-p_stat_block->result1
                * pow(p_stat_block->parm1, (F64) p_stat_block->count1)
                / (p_stat_block->running_data.arg.fp * p_stat_block->parm),
                1.0 / ((F64) p_stat_block->count_a - 1.0)
                ) - 1.0  );
}

static void
array_range_proc_finish_MIRR(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(p_stat_block->count_a > 1)
        ss_data_set_real(p_ss_data, array_range_proc_MIRR_help(p_stat_block));
    else
        ss_data_set_real(p_ss_data, 0.0);
}

static void
array_range_proc_finish(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    switch(p_stat_block->exec_array_range_id)
    {
    default:
#if CHECKING
        default_unhandled();
        /*FALLTHRU*/
    case ARRAY_RANGE_MAX:
    case ARRAY_RANGE_MIN:
    case ARRAY_RANGE_SUM:
#endif
        array_range_proc_finish_running_data(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_AVERAGE:
        array_range_proc_finish_average(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_COUNT:
        array_range_proc_finish_COUNT(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_COUNTA:
        array_range_proc_finish_COUNTA(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDP:
        array_range_proc_finish_standard_deviation(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARP:
        array_range_proc_finish_variance(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_NPV:
        array_range_proc_finish_NPV(p_ss_data, p_stat_block);
        break;

    case ARRAY_RANGE_MIRR:
        array_range_proc_finish_MIRR(p_ss_data, p_stat_block);
        break;
    }
}

/******************************************************************************
*
* mini routine to call add for two data items
*
******************************************************************************/

static void
array_range_proc_item_add(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if(0 == p_stat_block->count)
    {
        p_stat_block->running_data = *p_ss_data;
    }
    else
    {
        P_SS_DATA args[2];
        SS_DATA result_data;
        EV_SLR dummy_slr;

        args[0] = p_ss_data;
        args[1] = &p_stat_block->running_data;

        c_bop_add(args, 2, &result_data, &dummy_slr);

        p_stat_block->running_data = result_data;
    }
}

/******************************************************************************
*
* process a data item for array_range
*
******************************************************************************/

static void
array_range_proc_blank(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    UNREFERENCED_PARAMETER(p_stat_block);
    UNREFERENCED_PARAMETER(p_ss_data);
}

static void
array_range_proc_date(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ss_data) < 0))
            p_stat_block->running_data = *p_ss_data;
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_MIN:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ss_data) > 0))
            p_stat_block->running_data = *p_ss_data;
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_SUM:
        /* cope with dates and times within a SUM by calling the add routine */
        array_range_proc_item_add(p_stat_block, p_ss_data);
        p_stat_block->count += 1;
        break;

    default:
        break;
    }

    p_stat_block->count_a += 1;
}

_Check_return_
static bool
array_range_proc_number_integer_MAX(
    P_STAT_BLOCK p_stat_block,
    PC_SS_DATA p_ss_data)
{
    if( (0 == p_stat_block->count) || (ss_data_get_integer(p_ss_data) > ss_data_get_integer(&p_stat_block->running_data)) )
        ss_data_set_integer(&p_stat_block->running_data, ss_data_get_integer(p_ss_data));

    return(true);
}

_Check_return_
static bool
array_range_proc_number_integer_MIN(
    P_STAT_BLOCK p_stat_block,
    PC_SS_DATA p_ss_data)
{
    if( (0 == p_stat_block->count) || (ss_data_get_integer(p_ss_data) < ss_data_get_integer(&p_stat_block->running_data)) )
        ss_data_set_integer(&p_stat_block->running_data, ss_data_get_integer(p_ss_data));

    return(true);
}

_Check_return_
static bool
array_range_proc_number_integer_sum(
    P_STAT_BLOCK p_stat_block,
    PC_SS_DATA p_ss_data)
{
    /* only dealing with individual narrower integer types here but SKS shows this may eventually overflow WORD16 */
    if(0 == p_stat_block->count)
        ss_data_set_integer(&p_stat_block->running_data, ss_data_get_integer(p_ss_data));
    else
        ss_data_set_integer(&p_stat_block->running_data, ss_data_get_integer(p_ss_data) + ss_data_get_integer(&p_stat_block->running_data));

    return(true);
}

_Check_return_
static BOOL
array_range_proc_number_try_integer(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    bool int_done = false;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
        int_done = array_range_proc_number_integer_MAX(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_MIN:
        int_done = array_range_proc_number_integer_MIN(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        int_done = array_range_proc_number_integer_sum(p_stat_block, p_ss_data);
        break;

    default: default_unhandled();
        break;
    }

    if(!int_done)
        return(FALSE);

    p_stat_block->count   += 1;
    p_stat_block->count_a += 1;
    return(TRUE);
}

static void
array_range_proc_number_MAX(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if( (0 == p_stat_block->count) || (ss_data_get_real(p_ss_data) > ss_data_get_real(&p_stat_block->running_data)) )
        ss_data_set_real(&p_stat_block->running_data, ss_data_get_real(p_ss_data));
}

static void
array_range_proc_number_MIN(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if( (0 == p_stat_block->count) || (ss_data_get_real(p_ss_data) < ss_data_get_real(&p_stat_block->running_data)) )
        ss_data_set_real(&p_stat_block->running_data, ss_data_get_real(p_ss_data));
}

static void
array_range_proc_number_sum(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if(p_stat_block->running_data.data_id == DATA_ID_REAL)
    {
        if(0 == p_stat_block->count)
            ss_data_set_real(&p_stat_block->running_data, ss_data_get_real(p_ss_data));
        else
            p_stat_block->running_data.arg.fp += ss_data_get_real(p_ss_data);
    }
    else
        array_range_proc_item_add(p_stat_block, p_ss_data);
}

static void
array_range_proc_number_standard_deviation(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    const F64 x2 = ss_data_get_real(p_ss_data) * ss_data_get_real(p_ss_data);

    if(0 == p_stat_block->count)
    {
        p_stat_block->sum_x2 = x2;
        p_stat_block->running_data.arg.fp  = ss_data_get_real(p_ss_data);
    }
    else
    {
        p_stat_block->sum_x2 += x2;
        p_stat_block->running_data.arg.fp += ss_data_get_real(p_ss_data);
    }
}

static void
array_range_proc_number_NPV(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    npv_item(p_ss_data, &p_stat_block->count, &p_stat_block->parm, &p_stat_block->temp, &p_stat_block->running_data.arg.fp);

    p_stat_block->count -= 1;
}

static void
array_range_proc_number_MIRR(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if(ss_data_get_real(p_ss_data) >= 0.0)
        npv_item(p_ss_data, &p_stat_block->count1, &p_stat_block->parm1, &p_stat_block->temp1, &p_stat_block->result1);
    else
        npv_item(p_ss_data, &p_stat_block->count,  &p_stat_block->parm,  &p_stat_block->temp,  &p_stat_block->running_data.arg.fp);

    p_stat_block->count -= 1;
}

static void
array_range_proc_number(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    BOOL size_worry = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        size_worry = TRUE; /* need to worry about adding >16-bit integers and overflowing */
        break;

    default:
        break;
    }

    if(TWO_INTS == two_nums_type_match(p_ss_data, &p_stat_block->running_data, size_worry))
        if(array_range_proc_number_try_integer(p_stat_block, p_ss_data))
            return;

    /* two_nums_type_match will have promoted */
    assert(ss_data_is_number(p_ss_data));
    assert(ss_data_is_number(&p_stat_block->running_data));

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
        array_range_proc_number_MAX(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_MIN:
        array_range_proc_number_MIN(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        array_range_proc_number_sum(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDP:
    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARP:
        array_range_proc_number_standard_deviation(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_NPV:
        array_range_proc_number_NPV(p_stat_block, p_ss_data);
        break;

    case ARRAY_RANGE_MIRR:
        array_range_proc_number_MIRR(p_stat_block, p_ss_data);
        break;

    default: default_unhandled();
#if CHECKING
    case ARRAY_RANGE_COUNT:
    case ARRAY_RANGE_COUNTA:
#endif
        break;
    }

    p_stat_block->count   += 1;
    p_stat_block->count_a += 1;
}

static void
array_range_proc_others(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    UNREFERENCED_PARAMETER(p_ss_data);

    p_stat_block->count_a += 1;
}

static void
array_range_proc_string(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    if(!ss_string_is_blank(p_ss_data))
    {
        array_range_proc_others(p_stat_block, p_ss_data);
        return;
    }
}

static void
array_range_proc_item(
    P_STAT_BLOCK p_stat_block,
    P_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        array_range_proc_number(p_stat_block, p_ss_data);
        break;

    case DATA_ID_DATE:
        array_range_proc_date(p_stat_block, p_ss_data);
        break;

    case DATA_ID_BLANK:
        array_range_proc_blank(p_stat_block, p_ss_data);
        break;

    case RPN_DAT_STRING:
        array_range_proc_string(p_stat_block, p_ss_data);
        break;

    default:
        array_range_proc_others(p_stat_block, p_ss_data);
        break;
    }
}

/******************************************************************************
*
* call back for database function processing
*
******************************************************************************/

extern void
dbase_sub_function(
    P_STACK_DBASE p_stack_dbase,
    P_SS_DATA cond_flagp)
{
    EV_SLR cur_slot;

    cur_slot      = p_stack_dbase->dbase_rng.s;
    cur_slot.col += p_stack_dbase->offset.col;
    cur_slot.row += p_stack_dbase->offset.row;

    /* work out state of condition */
    if( (arg_normalise(cond_flagp, EM_REA, NULL, NULL) == DATA_ID_REAL) &&
        (ss_data_get_real(cond_flagp) != 0.0) )
    {
        SS_DATA data;

#if TRACE_ALLOWED
        if_constant(tracing(TRACE_MODULE_EVAL))
        {
            char buffer[BUF_EV_LONGNAMLEN];
            ev_trace_slr(buffer, elemof32(buffer), "DBASE processing cell: $$", &cur_slot);
            trace_0(TRACE_MODULE_EVAL, buffer);
        }
#endif

        ev_slr_deref(&data, &cur_slot, FALSE);
        array_range_proc(p_stack_dbase->p_stat_block, &data);
    }
#if TRACE_ALLOWED
    else if_constant(tracing(TRACE_MODULE_EVAL))
    {
        char buffer[BUF_EV_LONGNAMLEN];
        ev_trace_slr(buffer, elemof32(buffer), "DBASE failed cell: $$", &cur_slot);
        trace_0(TRACE_MODULE_EVAL, buffer);
    }
#endif
}

/******************************************************************************
*
* finish off a database function
*
******************************************************************************/

extern void
dbase_sub_function_finish(
    P_SS_DATA p_ss_data,
    P_STACK_DBASE p_stack_dbase)
{
    array_range_proc_finish(p_ss_data, p_stack_dbase->p_stat_block);
}

/******************************************************************************
*
* process data for arrays and ranges for
* the statistical functions
*
* <0 stopped at break count
* =0 no success
* >0 got there
*
******************************************************************************/

extern S32
lookup_array_range_proc(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data)
{
    S32 res = 0;

    switch(ss_data_get_data_id(p_ss_data))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        lkbp->in_array = 1;
        res = lookup_array_range_proc_array(lkbp, p_ss_data);
        break;

    case DATA_ID_RANGE:
        {
        if(lkbp->in_range || range_scan_init(&p_ss_data->arg.range, &lkbp->rsb) >= 0)
        {
            SS_DATA element;

            lkbp->in_range = 0;
            while(range_scan_element(&lkbp->rsb, &element, EM_REA | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_INT) != RPN_FRM_END)
            {
                switch(element.data_id)
                {
                case RPN_TMP_ARRAY:
                case RPN_RES_ARRAY:
                    res = lookup_array_range_proc_array(lkbp, &element);
                    break;

                default:
                    if((res = lookup_array_range_proc_item(lkbp, &element)) >= 0)
                    {
                        /* save position */
                        lkbp->result_data.arg.slr = lkbp->rsb.slr_of_result;
                        lkbp->result_data.data_id = DATA_ID_SLR;
                        lkbp->had_one = 1;
                    }
                    else
                        res = 1;
                    break;
                }

                if(res)
                    break;

                if(!--lkbp->break_count)
                {
                    lkbp->in_range = 1;
                    lkbp->break_count = LOOKUP_BREAK_COUNT;
                    res = -1;
                    break;
                }
            }
        }

        break;
        }

    default:
        if((res = lookup_array_range_proc_item(lkbp, p_ss_data)) >= 0)
        {
            lkbp->result_data = *p_ss_data;
            lkbp->had_one = 1;
        }
        else
            res = 1;
        break;
    }

    /* if we didn't find a match, yet we are
     * not looking for an exact match, return the last
     * element we passed (if we did)
     */
    if(!res && lkbp->match)
        res = lkbp->had_one;

    return(res);
}

/******************************************************************************
*
* process an array for array_range
*
******************************************************************************/

static S32
lookup_array_range_proc_array(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data)
{
    S32 res = 0;
    ARRAY_SCAN_BLOCK asb;

    if(array_scan_init(&asb, p_ss_data))
    {
        SS_DATA element;

        while(array_scan_element(&asb, &element, EM_REA | EM_STR | EM_DAT | EM_AR0 | EM_BLK | EM_INT) != RPN_FRM_END)
        {
            if(element.data_id == DATA_ID_BLANK)
                continue;

            if((res = lookup_array_range_proc_item(lkbp, &element)) >= 0)
            {
                lkbp->result_data = element;
                lkbp->had_one = 1;
            }
            else
                res = 1;

            if(res)
                break;
        }
    }

    return(res);
}

/******************************************************************************
*
* process an item for lookup
*
* --out--
*  1 if found
*  0 if not yet found
* -1 if twas the previous one
*
******************************************************************************/

static S32
lookup_array_range_proc_item(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data)
{
    S32 res;

    res = 0;

    switch(lkbp->lookup_id)
    {
    case LOOKUP_CHOOSE:
        {
        if(!--lkbp->choose_count)
            res = 1;
        break;
        }

    default:
        {
        S32 match;

        /* SKS 22oct96 for PD 4.50 reversed to make wildcard lookups work */
        if(0 == (match = ss_data_compare(p_ss_data, &lkbp->target_data)))
            res = 1;
        else
        {
            match = -match;

            if((match < 0 && lkbp->match > 0) || (match > 0 && lkbp->match < 0))
                res = -1;
        }
        break;
        }
    }

    if(res >= 0)
        lkbp->count += 1;

    return(res);
}

/******************************************************************************
*
* initialise lookup block
*
******************************************************************************/

extern void
lookup_block_init(
    P_LOOKUP_BLOCK lkbp,
    _InRef_opt_ P_SS_DATA p_ss_data_target,
    S32 lookup_id,
    S32 choose_count,
    S32 match)
{
    if(p_ss_data_target)
        ss_data_resource_copy(&lkbp->target_data, p_ss_data_target);
    else
        lkbp->target_data.data_id = DATA_ID_BLANK;

    lkbp->lookup_id = lookup_id;
    lkbp->choose_count = choose_count;
    lkbp->in_range = 0;
    lkbp->in_array = 0;
    lkbp->count = 0;
    lkbp->break_count = LOOKUP_BREAK_COUNT;

    switch(lookup_id)
    {
    case LOOKUP_HLOOKUP:
    case LOOKUP_VLOOKUP:
        lkbp->match = 1;
        break;
    case LOOKUP_MATCH:
        lkbp->match = match;
        break;
    case LOOKUP_LOOKUP:
    default:
        lkbp->match = 0;
        break;
    }

    lkbp->had_one = 0;
}

/******************************************************************************
*
* finish off a lookup
*
******************************************************************************/

extern void
lookup_finish(
    P_SS_DATA p_ss_data_res,
    P_STACK_LOOKUP p_stack_lookup)
{
    switch(p_stack_lookup->p_lookup_block->lookup_id)
    {
    case LOOKUP_LOOKUP:
        {
        SS_DATA data;

        array_range_mono_index(&data,
                               &p_stack_lookup->arg2,
                               p_stack_lookup->p_lookup_block->count - 1,
                               EM_REA | EM_SLR | EM_STR | EM_DAT | EM_BLK | EM_INT);
        /* MRJC 27.3.92 */
        ss_data_resource_copy(p_ss_data_res, &data);
        break;
        }

    case LOOKUP_HLOOKUP:
        {
        if(p_stack_lookup->p_lookup_block->result_data.data_id != DATA_ID_SLR)
            ss_data_set_error(p_ss_data_res, EVAL_ERR_UNEXARRAY);
        else
        {
            ss_data_resource_copy(p_ss_data_res, &p_stack_lookup->p_lookup_block->result_data);
            p_ss_data_res->arg.slr.row += (EV_ROW) ss_data_get_integer(&p_stack_lookup->arg2);
        }
        break;
        }

    case LOOKUP_VLOOKUP:
        {
        if(p_stack_lookup->p_lookup_block->result_data.data_id != DATA_ID_SLR)
            ss_data_set_error(p_ss_data_res, EVAL_ERR_UNEXARRAY);
        else
        {
            ss_data_resource_copy(p_ss_data_res, &p_stack_lookup->p_lookup_block->result_data);
            p_ss_data_res->arg.slr.col += (EV_COL) ss_data_get_integer(&p_stack_lookup->arg2);
        }
        break;
        }

    case LOOKUP_MATCH:
        if(p_stack_lookup->p_lookup_block->in_array)
            ss_data_set_integer(p_ss_data_res, p_stack_lookup->p_lookup_block->count);
        else
            ss_data_resource_copy(p_ss_data_res, &p_stack_lookup->p_lookup_block->result_data);
        break;
    }
}

/******************************************************************************
*
* calculate npv
*
******************************************************************************/

static EV_IDNO
npv_calc(
    P_SS_DATA p_ss_data,
    P_F64 intp,
    P_SS_DATA array)
{
    STAT_BLOCK stb;

    stat_block_init(&stb, RPN_FNF_NPV, *intp + 1, 0);

    array_range_proc(&stb, array);

    array_range_proc_finish(p_ss_data, &stb);

    return(ss_data_get_data_id(p_ss_data));
}

/******************************************************************************
*
* process an individual item for npv/mirr
*
******************************************************************************/

static void
npv_item(
    P_SS_DATA p_ss_data,
    P_S32 p_count,
    P_F64 p_parm,
    P_F64 p_temp,
    P_F64 p_result)
{
    if(0 == *p_count)
    {
        *p_temp    = *p_parm;                               /* interest */
        *p_result  = ss_data_get_real(p_ss_data) / *p_parm; /* interest */
    }
    else
    {
        *p_temp   *= *p_parm;                               /* interest */
        *p_result += ss_data_get_real(p_ss_data) / *p_temp;
    }

    *p_count += 1;
}

/******************************************************************************
*
* initialise stats data block
*
******************************************************************************/

static void
stat_block_init_real(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InVal_     enum EXEC_ARRAY_RANGE exec_array_range_id)
{
    p_stat_block->exec_array_range_id = exec_array_range_id;
    ss_data_set_real(&p_stat_block->running_data, 0.0);
}

extern void
stat_block_init(
    P_STAT_BLOCK p_stat_block,
    _InVal_     EV_IDNO function_id,
    F64 parm,
    F64 parm1)
{
    p_stat_block->count       = 0;
    p_stat_block->count_a     = 0;

    p_stat_block->count1      = 0;
    p_stat_block->temp        = 0;
    p_stat_block->temp1       = 0;
    p_stat_block->parm        = parm;
    p_stat_block->parm1       = parm1;
    p_stat_block->result1     = 0;

    ss_data_set_integer(&p_stat_block->running_data, 0);

    switch(function_id)
    {
    default:
#if CHECKING
        default_unhandled();
        /*FALLTHRU*/
    case RPN_FNV_COUNT:
#endif
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNT;
        break;

    case RPN_FNV_COUNTA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNTA;
        break;

    case RPN_FNV_MAX:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MAX;
        break;

    case RPN_FNV_MIN:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MIN;
        break;

    case RPN_FNV_SUM:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SUM;
        break;

    case RPN_FNV_AVG:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_AVERAGE;
        break;

    case RPN_FNV_STD:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_STD);
        break;

    case RPN_FNV_STDP:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_STDP);
        break;

    case RPN_FNV_VAR:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_VAR);
        break;

    case RPN_FNV_VARP:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_VARP);
        break;

    case RPN_FNF_NPV:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_NPV);
        break;

    case RPN_FNF_MIRR:
        stat_block_init_real(p_stat_block, ARRAY_RANGE_MIRR);
        break;
    }

#if CHECKING
    p_stat_block->_function_id = function_id;
#endif
}

extern void
dbase_stat_block_init(
    P_STAT_BLOCK stbp,
    _InVal_     U32 no_exec_id)
{
    EV_IDNO function_id;

    switch(no_exec_id)
    {
    case DBASE_DAVG:
        function_id = RPN_FNV_AVG;
        break;

    default: default_unhandled();
    case DBASE_DCOUNT:
        function_id = RPN_FNV_COUNT;
        break;

    case DBASE_DCOUNTA:
        function_id = RPN_FNV_COUNTA;
        break;

    case DBASE_DMAX:
        function_id = RPN_FNV_MAX;
        break;

    case DBASE_DMIN:
        function_id = RPN_FNV_MIN;
        break;

    case DBASE_DSTD:
        function_id = RPN_FNV_STD;
        break;

    case DBASE_DSTDP:
        function_id = RPN_FNV_STDP;
        break;

    case DBASE_DSUM:
        function_id = RPN_FNV_SUM;
        break;

    case DBASE_DVAR:
        function_id = RPN_FNV_VAR;
        break;

    case DBASE_DVARP:
        function_id = RPN_FNV_VARP;
        break;
    }

    stat_block_init(stbp, function_id, 0.0, 0.0);
}

/* end of ev_exec.c */
