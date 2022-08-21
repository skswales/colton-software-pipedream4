/* ev_exec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

    two_nums_times(p_ev_data_res, args[0], args[1]);
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

    if(two_nums_plus(p_ev_data_res, args[0], args[1]))
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

                    if(EV_DATE_INVALID != args[1]->arg.ev_date.date)
                        {
                        p_ev_data_res->arg.ev_date.date = args[1]->arg.ev_date.date + args[0]->arg.integer;
                        p_ev_data_res->arg.ev_date.time = args[1]->arg.ev_date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.ev_date.date = EV_DATE_INVALID;
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

                    if(EV_DATE_INVALID != args[0]->arg.ev_date.date)
                        {
                        p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date + args[1]->arg.integer;
                        p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.ev_date.date = EV_DATE_INVALID;
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

    if(two_nums_minus(p_ev_data_res, args[0], args[1]))
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

                    if(EV_DATE_INVALID != args[0]->arg.ev_date.date)
                        {
                        p_ev_data_res->arg.ev_date.date = args[0]->arg.ev_date.date - args[1]->arg.integer;
                        p_ev_data_res->arg.ev_date.time = args[0]->arg.ev_date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.ev_date.date = EV_DATE_INVALID;
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

    two_nums_divide(p_ev_data_res, args[0], args[1]);
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
    EV_IDNO function_id);

static void
array_range_proc(
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_array(
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    P_STAT_BLOCK stbp);

static void
array_range_proc_item(
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_item_add(
    P_STAT_BLOCK stbp,
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

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DAVG);
}

/******************************************************************************
*
* count(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_count)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DCOUNT);
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

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DMAX);
}

/******************************************************************************
*
* min(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_min)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DMIN);
}

/******************************************************************************
*
* mirr(values_array, finance_rate, reinvest_rate
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

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DSTD);
}

/******************************************************************************
*
* stdp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_stdp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DSTDP);
}

/******************************************************************************
*
* sum(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DSUM);
}

/******************************************************************************
*
* var(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_var)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DVAR);
}

/******************************************************************************
*
* varp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_varp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, n_args, p_ev_data_res, RPN_FNF_DVARP);
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
    EV_IDNO function_id)
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
* process data for arrays and ranges for
* the statistical functions
*
******************************************************************************/

static void
array_range_proc(
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
        {
        case RPN_TMP_ARRAY:
        case RPN_RES_ARRAY:
            array_range_proc_array(stbp, p_ev_data);
            break;

        case RPN_DAT_RANGE:
            {
            RANGE_SCAN_BLOCK rsb;

            if(range_scan_init(&p_ev_data->arg.range, &rsb) >= 0)
                {
                EV_DATA element;

                while(range_scan_element(&rsb,
                                         &element,
                                         EM_REA | EM_ARY | EM_BLK | EM_DAT) != RPN_FRM_END)
                    switch(element.did_num)
                        {
                        case RPN_TMP_ARRAY:
                        case RPN_RES_ARRAY:
                            array_range_proc_array(stbp, &element);
                            break;
                        default:
                            array_range_proc_item(stbp, &element);
                            break;
                        }
                }

            break;
            }

        default:
            array_range_proc_item(stbp, p_ev_data);
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
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data)
{
    ARRAY_SCAN_BLOCK asb;

    if(array_scan_init(&asb, p_ev_data))
        {
        EV_DATA element;

        while(array_scan_element(&asb, &element,
                                 EM_REA | EM_AR0 | EM_BLK | EM_DAT) != RPN_FRM_END)
            array_range_proc_item(stbp, &element);
        }
}

/*
* SKS 27oct96 purchased from Fireworkz
*
* helper routine for std/var(p)
* which accounts for small errors
* in our friendly floating point
*/

static F64
array_range_proc_stdvar_help(
    P_STAT_BLOCK stbp,
    BOOL n_1)
{
    F64 n_sum_x2 = stbp->temp * (F64) stbp->count;
    F64 sum_2 = stbp->running_data.arg.fp * stbp->running_data.arg.fp;
    F64 res = n_sum_x2 - sum_2;

    res = (res < 0.0) ? 0.0 : res; /* ensure we never go gaga */

    res /= (F64) stbp->count;
    res /= n_1 ? (F64) stbp->count - 1.0 : (F64) stbp->count;

    return(res);
}

/******************************************************************************
*
* calculate the final result for the array_range functions
*
******************************************************************************/

static void
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    P_STAT_BLOCK stbp)
{
    ev_data_set_real(p_ev_data, 0.0);

    switch(stbp->function_id)
    {
    case RPN_FNF_DAVG:
        if(stbp->count)
        {
            if(stbp->running_data.did_num != RPN_DAT_REAL)
                ev_data_set_real(p_ev_data, (F64) stbp->running_data.arg.integer / (F64) stbp->count);
            else
                ev_data_set_real(p_ev_data, stbp->running_data.arg.fp / (F64) stbp->count);
        }
        break;

    case RPN_FNF_DMAX:
    case RPN_FNF_DMIN:
    case RPN_FNF_DSUM:
        if(stbp->count)
        {
            switch(stbp->running_data.did_num)
            {
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
                ev_data_set_integer(p_ev_data, stbp->running_data.arg.integer);
                break;

            default:
                *p_ev_data = stbp->running_data;
                break;
            }
        }
        break;

    case RPN_FNF_DCOUNT:
        ev_data_set_integer(p_ev_data, stbp->count);
        break;

    case RPN_FNF_DCOUNTA:
        ev_data_set_integer(p_ev_data, stbp->count_a);
        break;

    case RPN_FNF_DSTD:
        if(stbp->count > 1)
            ev_data_set_real(p_ev_data, sqrt(array_range_proc_stdvar_help(stbp, 1)));
        break;

    case RPN_FNF_DSTDP:
        if(stbp->count)
            ev_data_set_real(p_ev_data, sqrt(array_range_proc_stdvar_help(stbp, 0)));
        break;

    case RPN_FNF_DVAR:
        if(stbp->count > 1)
            ev_data_set_real(p_ev_data, array_range_proc_stdvar_help(stbp, 1));
        break;

    case RPN_FNF_DVARP:
        if(stbp->count)
            ev_data_set_real(p_ev_data, array_range_proc_stdvar_help(stbp, 0));
        break;

    case RPN_FNF_NPV:
        if(stbp->count)
            ev_data_set_real(p_ev_data, stbp->running_data.arg.fp);
        break;

    case RPN_FNF_MIRR:
        if(stbp->count_a > 1)
        {
            ev_data_set_real(p_ev_data,
                             pow(-stbp->result1
                                * pow(stbp->parm1, (F64) stbp->count1)
                                / (stbp->running_data.arg.fp * stbp->parm),
                                1.0 / ((F64) stbp->count_a - 1.0)
                               ) - 1.0);
        }
        break;

    default:
        break;
    }
}

/******************************************************************************
*
* process a data item for array_range
*
******************************************************************************/

static void
array_range_proc_item(
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        BOOL size_worry = FALSE;
        BOOL int_done = 0;

        switch(stbp->function_id)
        {
        case RPN_FNF_DAVG:
        case RPN_FNF_DSUM:
            size_worry = TRUE; /* need to worry about adding >16-bit integers and overflowing */
            break;

        default:
            break;
        }

        if(two_nums_type_match(p_ev_data, &stbp->running_data, size_worry) == TWO_INTS)
        {
            switch(stbp->function_id)
            {
            case RPN_FNF_DAVG:
            case RPN_FNF_DSUM:
                /* only dealing with individual narrower integer types here but SKS shows this may eventually overflow WORD16 */
                if(!stbp->count)
                    ev_data_set_integer(&stbp->running_data, p_ev_data->arg.integer);
                else
                    ev_data_set_integer(&stbp->running_data, p_ev_data->arg.integer + stbp->running_data.arg.integer);
                trace_1(TRACE_MODULE_EVAL, "DSUM result now int: %d", stbp->running_data.arg.integer);
                int_done = 1;
                break;

            case RPN_FNF_DMAX:
                if(!stbp->count || (p_ev_data->arg.integer > stbp->running_data.arg.integer))
                    ev_data_set_integer(&stbp->running_data, p_ev_data->arg.integer);
                int_done = 1;
                break;

            case RPN_FNF_DMIN:
                if(!stbp->count || (p_ev_data->arg.integer < stbp->running_data.arg.integer))
                    ev_data_set_integer(&stbp->running_data, p_ev_data->arg.integer);
                int_done = 1;
                break;
            }
        }

        if(!int_done)
        {
            switch(stbp->function_id)
            {
            case RPN_FNF_DAVG:
            case RPN_FNF_DSUM:
                if(stbp->running_data.did_num == RPN_DAT_REAL)
                {
                    if(!stbp->count)
                        stbp->running_data.arg.fp  = p_ev_data->arg.fp;
                    else
                        stbp->running_data.arg.fp += p_ev_data->arg.fp;
                    trace_1(TRACE_MODULE_EVAL, "DSUM result now float: %g", stbp->running_data.arg.fp);
                }
                else
                    array_range_proc_item_add(stbp, p_ev_data);
                break;

            case RPN_FNF_DMAX:
                if(!stbp->count || (p_ev_data->arg.fp > stbp->running_data.arg.fp))
                    stbp->running_data.arg.fp = p_ev_data->arg.fp;
                break;

            case RPN_FNF_DMIN:
                if(!stbp->count || (p_ev_data->arg.fp < stbp->running_data.arg.fp))
                    stbp->running_data.arg.fp = p_ev_data->arg.fp;
                break;

            case RPN_FNF_DSTD:
            case RPN_FNF_DSTDP:
            case RPN_FNF_DVAR:
            case RPN_FNF_DVARP:
                {
                F64 x2 = p_ev_data->arg.fp * p_ev_data->arg.fp;

                if(!stbp->count)
                {
                    stbp->temp = x2;
                    stbp->running_data.arg.fp = p_ev_data->arg.fp;
                }
                else
                {
                    stbp->temp += x2;
                    stbp->running_data.arg.fp += p_ev_data->arg.fp;
                }

                break;
                }

            case RPN_FNF_NPV:
                npv_item(p_ev_data, &stbp->count, &stbp->parm, &stbp->temp, &stbp->running_data.arg.fp);
                stbp->count -= 1;
                break;

            case RPN_FNF_MIRR:
                if(p_ev_data->arg.fp >= 0)
                    npv_item(p_ev_data, &stbp->count1, &stbp->parm1, &stbp->temp1, &stbp->result1);
                else
                    npv_item(p_ev_data, &stbp->count,  &stbp->parm,  &stbp->temp,  &stbp->running_data.arg.fp);
                stbp->count -= 1;
                break;

            case RPN_FNF_DCOUNT:
            default:
                break;
            }
        }

        stbp->count   += 1;
        stbp->count_a += 1;
        break;
        }

    /* cope with dates and times within a SUM by calling the add routine */
    case RPN_DAT_DATE:
        {
        stbp->count_a += 1;

        if(stbp->function_id == RPN_FNF_DSUM)
        {
            array_range_proc_item_add(stbp, p_ev_data);
            stbp->count += 1;
        }

        break;
        }

    case RPN_DAT_BLANK:
        break;

    case RPN_DAT_STRING:
        if(ss_string_is_blank(p_ev_data))
            break;

    /* FALLTHRU */

    default:
        stbp->count_a += 1;
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
    P_STAT_BLOCK stbp,
    P_EV_DATA p_ev_data)
{
    if(!stbp->count)
        stbp->running_data = *p_ev_data;
    else
    {
        P_EV_DATA args[2];
        EV_DATA result_data;
        EV_SLR dummy_slr;

        args[0] = p_ev_data;
        args[1] = &stbp->running_data;

        c_add(args, 2, &result_data, &dummy_slr);

        stbp->running_data = result_data;
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
        if(tracing(TRACE_MODULE_EVAL))
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
    else if(tracing(TRACE_MODULE_EVAL))
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
                while(range_scan_element(&lkbp->rsb,
                                         &element,
                                         EM_REA | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_INT) != RPN_FRM_END)
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

        while(array_scan_element(&asb, &element,
                                 EM_REA | EM_STR | EM_DAT | EM_AR0 | EM_BLK | EM_INT) != RPN_FRM_END)
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

                if(match < 0 && lkbp->match > 0 || match > 0 && lkbp->match < 0)
                    res = -1;
                }
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
    P_STAT_BLOCK stbp,
    S32 function_id,
    F64 parm,
    F64 parm1)
{
    EV_IDNO id;

    switch(function_id)
    {
    case DBASE_DAVG:
        id = RPN_FNF_DAVG;
        break;
    case DBASE_DCOUNT:
        id = RPN_FNF_DCOUNT;
        break;
    case DBASE_DCOUNTA:
        id = RPN_FNF_DCOUNTA;
        break;
    case DBASE_DMAX:
        id = RPN_FNF_DMAX;
        break;
    case DBASE_DMIN:
        id = RPN_FNF_DMIN;
        break;
    case DBASE_DSTD:
        id = RPN_FNF_DSTD;
        break;
    case DBASE_DSTDP:
        id = RPN_FNF_DSTDP;
        break;
    case DBASE_DSUM:
        id = RPN_FNF_DSUM;
        break;
    case DBASE_DVAR:
        id = RPN_FNF_DVAR;
        break;
    case DBASE_DVARP:
        id = RPN_FNF_DVARP;
        break;
    default:
        id = (EV_IDNO) function_id;
        break;
    }

    stbp->function_id = id;
    stbp->count       = 0;
    stbp->count_a     = 0;
    stbp->count1      = 0;
    stbp->temp        = 0;
    stbp->temp1       = 0;
    stbp->parm        = parm;
    stbp->parm1       = parm1;
    stbp->result1     = 0;

    /* configure for those functions which can process integers */
    switch(id)
    {
    case RPN_FNF_DMAX:
    case RPN_FNF_DMIN:
    case RPN_FNF_DSUM:
    case RPN_FNF_DAVG:
        stbp->running_data.did_num     = RPN_DAT_WORD8; /* start with narrowest integer type */
        stbp->running_data.arg.integer = 0;
        break;

    default:
        ev_data_set_real(&stbp->running_data, 0.0);
        break;
    }
}

/* end of ev_exec.c */
