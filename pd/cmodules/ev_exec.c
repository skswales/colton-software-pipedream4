/* ev_exec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
* NUMBER unary plus
*
******************************************************************************/

PROC_EXEC_PROTO(c_uplus)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_REAL)
        ev_data_set_real(p_ev_data_res, args[0]->arg.fp);
    else
        ev_data_set_integer(p_ev_data_res, args[0]->arg.integer);
}

/******************************************************************************
*
* NUMBER unary minus
*
******************************************************************************/

PROC_EXEC_PROTO(c_uminus)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_REAL)
        ev_data_set_real(p_ev_data_res, -args[0]->arg.fp);
    else
        ev_data_set_integer(p_ev_data_res, -args[0]->arg.integer);
}

/******************************************************************************
*
* BOOLEAN unary not
*
******************************************************************************/

PROC_EXEC_PROTO(c_not)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (args[0]->arg.integer == 0 ? TRUE : FALSE));
}

/******************************************************************************
*
* BOOLEAN binary and
* res = a && b
*
******************************************************************************/

PROC_EXEC_PROTO(c_and)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, ((args[0]->arg.integer != 0) && (args[1]->arg.integer != 0)));
}

/******************************************************************************
*
* VALUE binary multiplication
* res = a*b
*
******************************************************************************/

PROC_EXEC_PROTO(c_mul)
{
    exec_func_ignore_parms();

    consume_bool(two_nums_multiply_try(p_ev_data_res, args[0], args[1]));
}

/******************************************************************************
*
* VALUE binary addition
* res = a+b
*
******************************************************************************/

PROC_EXEC_PROTO(c_add)
{
    exec_func_ignore_parms();

    if(two_nums_add_try(p_ev_data_res, args[0], args[1]))
        return;

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
        real_to_integer_force(args[0]);

    /*FALLTHRU*/

    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        switch(args[1]->did_num)
        {
        /* number + date */
        case RPN_DAT_DATE:
            p_ev_data_res->did_num = RPN_DAT_DATE;

            if(EV_DATE_NULL != args[1]->arg.ev_date.date)
            {
                p_ev_data_res->arg.ev_date.date = args[1]->arg.ev_date.date + args[0]->arg.integer;
                p_ev_data_res->arg.ev_date.time = args[1]->arg.ev_date.time;
            }
            else
            {
                p_ev_data_res->arg.ev_date.date = EV_DATE_NULL;
                p_ev_data_res->arg.ev_date.time = args[1]->arg.ev_date.time + args[0]->arg.integer;
                ss_date_normalise(&p_ev_data_res->arg.ev_date);
            }
            break;
        }
        break;

    case RPN_DAT_DATE:
        switch(args[1]->did_num)
        {
        /* date + number */
        case RPN_DAT_REAL:
            real_to_integer_force(args[1]);

            /*FALLTHRU*/

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            p_ev_data_res->did_num = RPN_DAT_DATE;

            if(EV_DATE_NULL != args[0]->arg.ev_date.date)
            {
                p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date + args[1]->arg.integer;
                p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time;
            }
            else
            {
                p_ev_data_res->arg.ev_date.date = EV_DATE_NULL;
                p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time + args[1]->arg.integer;
                ss_date_normalise(&p_ev_data_res->arg.ev_date);
            }
            break;

        /* date + date */
        case RPN_DAT_DATE:
            p_ev_data_res->did_num = RPN_DAT_DATE;
            p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date + args[1]->arg.ev_date.date;
            p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time + args[1]->arg.ev_date.time;
            ss_date_normalise(&p_ev_data_res->arg.ev_date);
            break;
        }

        break;
    }
}

/******************************************************************************
*
* VALUE binary subtraction
* res = a-b
*
******************************************************************************/

PROC_EXEC_PROTO(c_sub)
{
    exec_func_ignore_parms();

    if(two_nums_subtract_try(p_ev_data_res, args[0], args[1]))
        return;

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        switch(args[1]->did_num)
        {
        /* number - date */
        case RPN_DAT_DATE:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXDATE);
            break;
        }
        break;

    case RPN_DAT_DATE:
        switch(args[1]->did_num)
        {
        /* date - number */
        case RPN_DAT_REAL:
            real_to_integer_force(args[1]);

            /*FALLTHRU*/

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            p_ev_data_res->did_num = RPN_DAT_DATE;

            if(EV_DATE_NULL != args[0]->arg.ev_date.date)
            {
                p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date - args[1]->arg.integer;
                p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time;
            }
            else
            {
                p_ev_data_res->arg.ev_date.date = EV_DATE_NULL;
                p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time - args[1]->arg.integer;
                ss_date_normalise(&p_ev_data_res->arg.ev_date);
            }
            break;

        /* date - date */
        case RPN_DAT_DATE:
            p_ev_data_res->did_num = RPN_DAT_DATE;
            p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date - args[1]->arg.ev_date.date;
            p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time - args[1]->arg.ev_date.time;

            if(p_ev_data_res->arg.ev_date.date && p_ev_data_res->arg.ev_date.time)
            {
                ss_date_normalise(&p_ev_data_res->arg.ev_date);
            }
            else
            {
                ev_data_set_integer(p_ev_data_res,
                                    (p_ev_data_res->arg.ev_date.date
                                         ? p_ev_data_res->arg.ev_date.date
                                         : p_ev_data_res->arg.ev_date.time));
            }

            break;
        }

        break;
    }
}

/******************************************************************************
*
* VALUE binary division
* res = a/b
*
******************************************************************************/

PROC_EXEC_PROTO(c_div)
{
    exec_func_ignore_parms();

    consume_bool(two_nums_divide_try(p_ev_data_res, args[0], args[1]));
}

/******************************************************************************
*
* NUMBER res = a ^ b
*
* NUMBER power(a, b) is equivalent OpenDocument / Microsoft Excel function
*
******************************************************************************/

PROC_EXEC_PROTO(c_power)
{
    const F64 a = args[0]->arg.fp;
    const F64 b = args[1]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real_ti(p_ev_data_res, pow(a, b));

    if(errno /* == EDOM */)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);

        /* test for negative numbers to power of 1/odd number (odd roots are OK) */
        if((a < 0.0) && (b <= 1.0))
        {
            EV_DATA test;

            ev_data_set_real(&test, 1.0 / b);

            if(real_to_integer_try(&test))
                ev_data_set_real_ti(p_ev_data_res, -(pow(-(a), b)));
        }
    }
}

/******************************************************************************
*
* BOOLEAN res = a|b
*
******************************************************************************/

PROC_EXEC_PROTO(c_or)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, ((args[0]->arg.integer != 0) || (args[1]->arg.integer != 0)));
}

/******************************************************************************
*
* BOOLEAN res = a==b
*
******************************************************************************/

PROC_EXEC_PROTO(c_eq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) == 0));
}

/******************************************************************************
*
* BOOLEAN res = a>b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gt)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) > 0));
}

/******************************************************************************
*
* BOOLEAN res = a>=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gteq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) >= 0));
}

/******************************************************************************
*
* BOOLEAN res = a<b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lt)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) < 0));
}

/******************************************************************************
*
* BOOLEAN res = a<=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lteq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) <= 0));
}

/******************************************************************************
*
* BOOLEAN res = a!=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_neq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) != 0));
}

/******************************************************************************
*
* VALUE if(num, true_res, false_res)
*
******************************************************************************/

PROC_EXEC_PROTO(c_if)
{
    exec_func_ignore_parms();

    if(args[0]->arg.boolean)
        ss_data_resource_copy(p_ev_data_res, args[1]);
    else if(n_args > 2)
        ss_data_resource_copy(p_ev_data_res, args[2]);
    else
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NA);
}

/******************************************************************************
*
* set the value of a cell
*
******************************************************************************/

static S32
poke_slot(
    _InRef_     PC_EV_SLR slrp,
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR cur_slrp)
{
    P_EV_CELL p_ev_cell;
    S32 res;

    /* can't overwrite formula cells */
    if(ev_travel(&p_ev_cell, slrp) > 0 && p_ev_cell->parms.type != EVS_CON_DATA)
        res = create_error(EVAL_ERR_UNEXFORMULA);
    #ifdef SLOTS_MOVE
    else if((slrp->docno == cur_slrp->docno) &&
            (slrp->col == cur_slrp->col) &&
            (slrp->row <= cur_slrp->row) )
        res = create_error(EVAL_ERR_OWNCOLUMN);
    #endif
    else
    {
        EV_DATA temp;
        EV_RESULT ev_result;

        ev_result_free_resources(&p_ev_cell->ev_result);

        /* make sure we have a copy of data */
        ss_data_resource_copy(&temp, p_ev_data);

        ev_data_to_result_convert(&ev_result, &temp);

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
    S32 err;

    exec_func_ignore_parms();

    /* initialise output so we can free OK */
    p_ev_data_res->did_num = RPN_DAT_BLANK;
    err = 0;

    switch(args[0]->did_num)
    {
    case RPN_DAT_SLR:
        {
        if(!slr_equal(p_cur_slr, &args[0]->arg.slr))
            err = poke_slot(&args[0]->arg.slr, args[1], p_cur_slr);

        if(!err)
            ss_data_resource_copy(p_ev_data_res, args[1]);

        break;
        }

    case RPN_DAT_RANGE:
        {
        RANGE_SCAN_BLOCK rsb;
        EV_SLR top_left;

        if((err = range_scan_init(&args[0]->arg.range, &rsb)) >= 0)
        {
            EV_DATA element;
            S32 first;

            first = 1;
            top_left = rsb.slr_of_result;

            while(range_scan_element(&rsb,
                                     &element,
                                     EM_ANY) != RPN_FRM_END)
            {
                S32 res;
                EV_DATA poke_data;

                switch(args[1]->did_num)
                {
                case RPN_DAT_RANGE:
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
                    ss_data_resource_copy(p_ev_data_res, &poke_data);

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
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, err);
    }
}

/*
range-ey functions
*/

static void
args_array_range_proc(
    P_EV_DATA args[EV_MAX_ARGS],
    S32 n_args,
    P_EV_DATA p_ev_data_res,
    _InVal_     EV_IDNO function_id);

static void
array_range_proc(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data);

static void
array_range_proc_array(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data);

static void
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block);

static void
array_range_proc_item(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data);

static S32
lookup_array_range_proc_array(
    P_LOOKUP_BLOCK lkbp,
    P_EV_DATA p_ev_data);

static S32
lookup_array_range_proc_item(
    P_LOOKUP_BLOCK lkbp,
    P_EV_DATA p_ev_data);

static EV_IDNO
npv_calc(
    P_EV_DATA p_ev_data_res,
    P_F64 intp,
    P_EV_DATA array);

static void
npv_item(
    P_EV_DATA p_ev_data,
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

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_AVG);
}

/******************************************************************************
*
* count(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_count)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_COUNT);
}

/******************************************************************************
*
* counta(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_counta)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_COUNTA);
}

/******************************************************************************
*
* irr(guess, array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_irr)
{
    F64 r = args[0]->arg.fp;
    F64 last_npv = 1.0;
    F64 last_r = 0.0;
    S32 iteration_count;

    exec_func_ignore_parms();

    for(iteration_count = 0; iteration_count < 40; ++iteration_count)
    {
        F64 this_npv, this_r;
        EV_DATA npv;

        if(npv_calc(&npv, &r, args[1]) != RPN_DAT_REAL)
            return;

        this_npv = npv.arg.fp;
        this_r = r;

        if(fabs(this_npv) < 0.0000001)
        {
            ev_data_set_real(p_ev_data_res, r);
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

    ev_data_set_error(p_ev_data_res, EVAL_ERR_IRR);
}

/******************************************************************************
*
* max(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_max)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_MAX);
}

/******************************************************************************
*
* min(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_min)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_MIN);
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

    stat_block_init(&stb, RPN_FNF_MIRR, args[1]->arg.fp + 1, args[2]->arg.fp + 1);

    array_range_proc(&stb, args[0]);

    array_range_proc_finish(p_ev_data_res, &stb);
}

/******************************************************************************
*
* npv(interest, array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_npv)
{
    exec_func_ignore_parms();

    npv_calc(p_ev_data_res, &args[0]->arg.fp, args[1]);
}

/******************************************************************************
*
* std(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_std)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_STD);
}

/******************************************************************************
*
* stdp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_stdp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_STDP);
}

/******************************************************************************
*
* sum(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_SUM);
}

/******************************************************************************
*
* var(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_var)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_VAR);
}

/******************************************************************************
*
* varp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_varp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNV_VARP);
}

/******************************************************************************
*
* process the arguments of the statistical functions
*
******************************************************************************/

static void
args_array_range_proc(
    P_EV_DATA args[EV_MAX_ARGS],
    S32 n_args,
    P_EV_DATA p_ev_data_res,
    _InVal_     EV_IDNO function_id)
{
    S32 i;
    STAT_BLOCK stb;

    stat_block_init(&stb, function_id, 0, 0);

    for(i = 0; i < n_args; ++i)
        array_range_proc(&stb, args[i]);

    array_range_proc_finish(p_ev_data_res, &stb);
}

/******************************************************************************
*
* process data for arrays and ranges for the statistical functions
*
******************************************************************************/

static void
array_range_proc(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        array_range_proc_array(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_RANGE:
        {
        RANGE_SCAN_BLOCK rsb;

        if(range_scan_init(&p_ev_data->arg.range, &rsb) >= 0)
        {
            EV_DATA element;

            while(range_scan_element(&rsb, &element, EM_REA | EM_ARY | EM_BLK | EM_DAT | EM_INT) != RPN_FRM_END)
            {
                switch(element.did_num)
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
        array_range_proc_item(p_stat_block, p_ev_data);
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
    P_EV_DATA p_ev_data)
{
    ARRAY_SCAN_BLOCK asb;

    if(array_scan_init(&asb, p_ev_data))
    {
        EV_DATA element;

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
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        switch(p_stat_block->running_data.did_num)
        {
        default: default_unhandled();
#if CHECKING
        case RPN_DAT_DATE:
        case RPN_DAT_REAL:
#endif
            *p_ev_data = p_stat_block->running_data;
            break;

        /*case RPN_DAT_BOOL8:*/ /* really shouldn't occur */
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            ev_data_set_integer(p_ev_data, p_stat_block->running_data.arg.integer);
            break;
        }
    }
    else
        ev_data_set_real(p_ev_data, 0.0);
}

static void
array_range_proc_finish_average(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        switch(p_stat_block->running_data.did_num)
        {
        default:
            p_stat_block->running_data.arg.fp = (F64) p_stat_block->running_data.arg.integer;

            /*FALLTHRU*/

        case RPN_DAT_REAL:
            {
            F64 avg_result = p_stat_block->running_data.arg.fp / (F64) p_stat_block->count;
            ev_data_set_real_ti(p_ev_data, avg_result);
            break;
            }

        case RPN_DAT_DATE:
            *p_ev_data = p_stat_block->running_data;
#if (EV_DATE_NULL == 0)
            p_ev_data->arg.ev_date.date /= p_stat_block->count;
            p_ev_data->arg.ev_date.time /= p_stat_block->count;
#else
            if(EV_DATE_NULL != p_ev_data->arg.ev_date.date)
                p_ev_data->arg.ev_date.date /= p_stat_block->count;
            if(EV_TIME_NULL != p_ev_data->arg.ev_date.time)
                p_ev_data->arg.ev_date.time /= p_stat_block->count;
#endif
            ss_date_normalise(&p_ev_data->arg.ev_date);
            break;
        }
    }
    else
        ev_data_set_real(p_ev_data, 0.0);
}

static void
array_range_proc_finish_COUNT(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ev_data_set_integer(p_ev_data, p_stat_block->count);
}

static void
array_range_proc_finish_COUNTA(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ev_data_set_integer(p_ev_data, p_stat_block->count_a);
}

_Check_return_
static F64
calc_variance(
    _InRef_     P_STAT_BLOCK p_stat_block,
    _InVal_     BOOL population_statistics)
{
    F64 n = (F64) p_stat_block->count;
    F64 n_sum_x2 = p_stat_block->sum_x2 * (F64) p_stat_block->count;
    F64 sum_2 = p_stat_block->running_data.arg.fp * p_stat_block->running_data.arg.fp;
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
    _OutRef_    P_EV_DATA p_ev_data,
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

        ev_data_set_real(p_ev_data, standard_deviation);
        return;
    }

    ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
}

static void
array_range_proc_finish_variance(
    _OutRef_    P_EV_DATA p_ev_data,
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

        ev_data_set_real(p_ev_data, variance);
        return;
    }

    ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
}

static void
array_range_proc_finish_NPV(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
        ev_data_set_real(p_ev_data, p_stat_block->running_data.arg.fp);
    else
        ev_data_set_real(p_ev_data, 0.0);
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
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(p_stat_block->count_a > 1)
        ev_data_set_real(p_ev_data, array_range_proc_MIRR_help(p_stat_block));
    else
        ev_data_set_real(p_ev_data, 0.0);
}

static void
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
    case ARRAY_RANGE_MIN:
    case ARRAY_RANGE_SUM:
        array_range_proc_finish_running_data(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_AVERAGE:
        array_range_proc_finish_average(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_COUNT:
        array_range_proc_finish_COUNT(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_COUNTA:
        array_range_proc_finish_COUNTA(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDP:
        array_range_proc_finish_standard_deviation(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARP:
        array_range_proc_finish_variance(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_NPV:
        array_range_proc_finish_NPV(p_ev_data, p_stat_block);
        break;

    case ARRAY_RANGE_MIRR:
        array_range_proc_finish_MIRR(p_ev_data, p_stat_block);
        break;

    default:
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
    P_EV_DATA p_ev_data)
{
    if(0 == p_stat_block->count)
    {
        p_stat_block->running_data = *p_ev_data;
    }
    else
    {
        P_EV_DATA args[2];
        EV_DATA result_data;
        EV_SLR dummy_slr;

        args[0] = p_ev_data;
        args[1] = &p_stat_block->running_data;

        c_add(args, 2, &result_data, &dummy_slr);

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
    P_EV_DATA p_ev_data)
{
    IGNOREPARM(p_stat_block);
    IGNOREPARM(p_ev_data);
}

static void
array_range_proc_date(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_SUM:
        /* cope with dates and times within a SUM by calling the add routine */
        array_range_proc_item_add(p_stat_block, p_ev_data);
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_MAX:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ev_data) < 0))
            p_stat_block->running_data = *p_ev_data;
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_MIN:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ev_data) > 0))
            p_stat_block->running_data = *p_ev_data;
        p_stat_block->count += 1;
        break;

    default:
        break;
    }

    p_stat_block->count_a += 1;
}

_Check_return_
static BOOL
array_range_proc_number_try_integer(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    BOOL int_done = 0;

    switch(p_stat_block->exec_array_range_id)
    {
    default: default_unhandled();
        break;

    case ARRAY_RANGE_MAX:
        if((0 == p_stat_block->count) || (p_ev_data->arg.integer > p_stat_block->running_data.arg.integer))
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer);
        int_done = 1;
        break;

    case ARRAY_RANGE_MIN:
        if((0 == p_stat_block->count) || (p_ev_data->arg.integer < p_stat_block->running_data.arg.integer))
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer);
        int_done = 1;
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        /* only dealing with individual narrower integer types here but SKS shows this may eventually overflow WORD16 */
        if(0 == p_stat_block->count)
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer);
        else
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer + p_stat_block->running_data.arg.integer);
        int_done = 1;
        break;
    }

    if(!int_done)
        return(FALSE);

    p_stat_block->count   += 1;
    p_stat_block->count_a += 1;
    return(TRUE);
}

static void
array_range_proc_number(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
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

    if(TWO_INTS == two_nums_type_match(p_ev_data, &p_stat_block->running_data, size_worry))
        if(array_range_proc_number_try_integer(p_stat_block, p_ev_data))
            return;

    /* two_nums_type_match will have promoted */
    assert(RPN_DAT_REAL == p_ev_data->did_num);
    assert(RPN_DAT_REAL == p_stat_block->running_data.did_num);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
        if((0 == p_stat_block->count) || (p_ev_data->arg.fp > p_stat_block->running_data.arg.fp))
            p_stat_block->running_data.arg.fp = p_ev_data->arg.fp;
        break;

    case ARRAY_RANGE_MIN:
        if((0 == p_stat_block->count) || (p_ev_data->arg.fp < p_stat_block->running_data.arg.fp))
            p_stat_block->running_data.arg.fp = p_ev_data->arg.fp;
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        if(p_stat_block->running_data.did_num == RPN_DAT_REAL)
        {
            if(0 == p_stat_block->count)
                p_stat_block->running_data.arg.fp  = p_ev_data->arg.fp;
            else
                p_stat_block->running_data.arg.fp += p_ev_data->arg.fp;
        }
        else
            array_range_proc_item_add(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDP:
    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARP:
        {
        const F64 x2 = p_ev_data->arg.fp * p_ev_data->arg.fp;

        if(0 == p_stat_block->count)
        {
            p_stat_block->sum_x2 = x2;
            p_stat_block->running_data.arg.fp = p_ev_data->arg.fp;
        }
        else
        {
            p_stat_block->sum_x2 += x2;
            p_stat_block->running_data.arg.fp += p_ev_data->arg.fp;
        }

        break;
        }

    case ARRAY_RANGE_NPV:
        npv_item(p_ev_data, &p_stat_block->count, &p_stat_block->parm, &p_stat_block->temp, &p_stat_block->running_data.arg.fp);
        p_stat_block->count -= 1;
        break;

    case ARRAY_RANGE_MIRR:
        if(p_ev_data->arg.fp >= 0)
            npv_item(p_ev_data, &p_stat_block->count1, &p_stat_block->parm1, &p_stat_block->temp1, &p_stat_block->result1);
        else
            npv_item(p_ev_data, &p_stat_block->count,  &p_stat_block->parm,  &p_stat_block->temp,  &p_stat_block->running_data.arg.fp);
        p_stat_block->count -= 1;
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
    P_EV_DATA p_ev_data)
{
    IGNOREPARM(p_ev_data);

    p_stat_block->count_a += 1;
}

static void
array_range_proc_string(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    if(!ss_string_is_blank(p_ev_data))
    {
        array_range_proc_others(p_stat_block, p_ev_data);
        return;
    }
}

static void
array_range_proc_item(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        array_range_proc_number(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_DATE:
        array_range_proc_date(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_BLANK:
        array_range_proc_blank(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_STRING:
        array_range_proc_string(p_stat_block, p_ev_data);
        break;

    default:
        array_range_proc_others(p_stat_block, p_ev_data);
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
    P_EV_DATA cond_flagp)
{
    EV_SLR cur_slot;

    cur_slot      = p_stack_dbase->dbase_rng.s;
    cur_slot.col += p_stack_dbase->offset.col;
    cur_slot.row += p_stack_dbase->offset.row;

    /* work out state of condition */
    if((arg_normalise(cond_flagp, EM_REA, NULL, NULL) == RPN_DAT_REAL) &&
       (cond_flagp->arg.fp != 0.0))
    {
        EV_DATA data;

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
    P_EV_DATA p_ev_data,
    P_STACK_DBASE p_stack_dbase)
{
    array_range_proc_finish(p_ev_data, p_stack_dbase->p_stat_block);
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
    P_EV_DATA p_ev_data)
{
    S32 res = 0;

    switch(p_ev_data->did_num)
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        lkbp->in_array = 1;
        res = lookup_array_range_proc_array(lkbp, p_ev_data);
        break;

    case RPN_DAT_RANGE:
        {
        if(lkbp->in_range || range_scan_init(&p_ev_data->arg.range, &lkbp->rsb) >= 0)
        {
            EV_DATA element;

            lkbp->in_range = 0;
            while(range_scan_element(&lkbp->rsb, &element, EM_REA | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_INT) != RPN_FRM_END)
            {
                switch(element.did_num)
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
                        lkbp->result_data.did_num = RPN_DAT_SLR;
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
        if((res = lookup_array_range_proc_item(lkbp, p_ev_data)) >= 0)
        {
            lkbp->result_data = *p_ev_data;
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
    P_EV_DATA p_ev_data)
{
    S32 res = 0;
    ARRAY_SCAN_BLOCK asb;

    if(array_scan_init(&asb, p_ev_data))
    {
        EV_DATA element;

        while(array_scan_element(&asb, &element, EM_REA | EM_STR | EM_DAT | EM_AR0 | EM_BLK | EM_INT) != RPN_FRM_END)
        {
            if(element.did_num == RPN_DAT_BLANK)
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
    P_EV_DATA p_ev_data)
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

        /* SKS 22oct96 for 4.50 reversed to make wildcard lookups work */
        if(0 == (match = ss_data_compare(p_ev_data, &lkbp->target_data)))
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
    _InRef_opt_ P_EV_DATA p_ev_data_target,
    S32 lookup_id,
    S32 choose_count,
    S32 match)
{
    if(p_ev_data_target)
        ss_data_resource_copy(&lkbp->target_data, p_ev_data_target);
    else
        lkbp->target_data.did_num = RPN_DAT_BLANK;

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
    P_EV_DATA p_ev_data_res,
    P_STACK_LOOKUP p_stack_lookup)
{
    switch(p_stack_lookup->p_lookup_block->lookup_id)
    {
    case LOOKUP_LOOKUP:
        {
        EV_DATA data;

        array_range_mono_index(&data,
                               &p_stack_lookup->arg2,
                               p_stack_lookup->p_lookup_block->count - 1,
                               EM_REA | EM_SLR | EM_STR | EM_DAT | EM_BLK | EM_INT);
        /* MRJC 27.3.92 */
        ss_data_resource_copy(p_ev_data_res, &data);
        break;
        }

    case LOOKUP_HLOOKUP:
        {
        if(p_stack_lookup->p_lookup_block->result_data.did_num != RPN_DAT_SLR)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        else
        {
            ss_data_resource_copy(p_ev_data_res, &p_stack_lookup->p_lookup_block->result_data);
            p_ev_data_res->arg.slr.row += (EV_ROW) p_stack_lookup->arg2.arg.integer;
        }
        break;
        }

    case LOOKUP_VLOOKUP:
        {
        if(p_stack_lookup->p_lookup_block->result_data.did_num != RPN_DAT_SLR)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        else
        {
            ss_data_resource_copy(p_ev_data_res, &p_stack_lookup->p_lookup_block->result_data);
            p_ev_data_res->arg.slr.col += (EV_COL) p_stack_lookup->arg2.arg.integer;
        }
        break;
        }

    case LOOKUP_MATCH:
        if(p_stack_lookup->p_lookup_block->in_array)
            ev_data_set_integer(p_ev_data_res, p_stack_lookup->p_lookup_block->count);
        else
            ss_data_resource_copy(p_ev_data_res, &p_stack_lookup->p_lookup_block->result_data);
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
    P_EV_DATA p_ev_data,
    P_F64 intp,
    P_EV_DATA array)
{
    STAT_BLOCK stb;

    stat_block_init(&stb, RPN_FNF_NPV, *intp + 1, 0);

    array_range_proc(&stb, array);

    array_range_proc_finish(p_ev_data, &stb);

    return(p_ev_data->did_num);
}

/******************************************************************************
*
* process an individual item for npv/mirr
*
******************************************************************************/

static void
npv_item(
    P_EV_DATA p_ev_data,
    P_S32 p_count,
    P_F64 p_parm,
    P_F64 p_temp,
    P_F64 p_result)
{
    if(!*p_count)
    {
        *p_temp    = *p_parm;                       /* interest */
        *p_result  = p_ev_data->arg.fp / *p_parm;   /* interest */
    }
    else
    {
        *p_temp   *= *p_parm;                       /* interest */
        *p_result += p_ev_data->arg.fp / *p_temp;
    }

    *p_count += 1;
}

/******************************************************************************
*
* initialise stats data block
*
******************************************************************************/

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

    switch(function_id)
    {
    case RPN_FNV_MAX:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MAX;
        ev_data_set_integer(&p_stat_block->running_data, 0);
        break;

    case RPN_FNV_MIN:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MIN;
        ev_data_set_integer(&p_stat_block->running_data, 0);
        break;

    case RPN_FNV_SUM:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SUM;
        ev_data_set_integer(&p_stat_block->running_data, 0);
        break;

    case RPN_FNV_AVG:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_AVERAGE;
        ev_data_set_integer(&p_stat_block->running_data, 0);
        break;

    case RPN_FNF_NPV:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_NPV;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNF_IRR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_IRR;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNF_MIRR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MIRR;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNV_STD:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STD;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNV_STDP:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STDP;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNV_VAR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VAR;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNV_VARP:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VARP;
        ev_data_set_real(&p_stat_block->running_data, 0.0);
        break;

    case RPN_FNV_COUNT:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNT;
        break;

    case RPN_FNV_COUNTA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNTA;
        break;

    default: default_unhandled();
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
