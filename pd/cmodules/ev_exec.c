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
* unary plus
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
* unary minus
*
******************************************************************************/

PROC_EXEC_PROTO(c_umi)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_REAL)
        ev_data_set_real(p_ev_data_res, -args[0]->arg.fp);
    else
        ev_data_set_integer(p_ev_data_res, -args[0]->arg.integer);
}

/******************************************************************************
*
* unary not
*
******************************************************************************/

PROC_EXEC_PROTO(c_not)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (args[0]->arg.integer == 0 ? TRUE : FALSE));
}

/******************************************************************************
*
* binary and
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
* binary multiplication
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
* binary addition
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

                    if(args[1]->arg.date.date)
                        {
                        p_ev_data_res->arg.date.date = args[1]->arg.date.date + args[0]->arg.integer;
                        p_ev_data_res->arg.date.time = args[1]->arg.date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.date.date = 0;
                        p_ev_data_res->arg.date.time = args[1]->arg.date.time + args[0]->arg.integer;
                        ss_date_normalise(&p_ev_data_res->arg.date);
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

                    if(args[0]->arg.date.date)
                        {
                        p_ev_data_res->arg.date.date = args[0]->arg.date.date + args[1]->arg.integer;
                        p_ev_data_res->arg.date.time = args[0]->arg.date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.date.date = 0;
                        p_ev_data_res->arg.date.time = args[0]->arg.date.time + args[1]->arg.integer;
                        ss_date_normalise(&p_ev_data_res->arg.date);
                        }
                    break;

                /* date + date */
                case RPN_DAT_DATE:
                    p_ev_data_res->did_num = RPN_DAT_DATE;
                    p_ev_data_res->arg.date.date = args[0]->arg.date.date + args[1]->arg.date.date;
                    p_ev_data_res->arg.date.time = args[0]->arg.date.time + args[1]->arg.date.time;
                    ss_date_normalise(&p_ev_data_res->arg.date);
                    break;
                }

            break;
        }
}

/******************************************************************************
*
* binary subtraction
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

                    if(args[0]->arg.date.date)
                        {
                        p_ev_data_res->arg.date.date = args[0]->arg.date.date - args[1]->arg.integer;
                        p_ev_data_res->arg.date.time = args[0]->arg.date.time;
                        }
                    else
                        {
                        p_ev_data_res->arg.date.date = 0;
                        p_ev_data_res->arg.date.time = args[0]->arg.date.time - args[1]->arg.integer;
                        ss_date_normalise(&p_ev_data_res->arg.date);
                        }
                    break;

                /* date - date */
                case RPN_DAT_DATE:
                    p_ev_data_res->did_num = RPN_DAT_DATE;
                    p_ev_data_res->arg.date.date = args[0]->arg.date.date - args[1]->arg.date.date;
                    p_ev_data_res->arg.date.time = args[0]->arg.date.time - args[1]->arg.date.time;

                    if(p_ev_data_res->arg.date.date && p_ev_data_res->arg.date.time)
                        {
                        ss_date_normalise(&p_ev_data_res->arg.date);
                        }
                    else
                        {
                        ev_data_set_integer(p_ev_data_res,
                                            (p_ev_data_res->arg.date.date
                                                 ? p_ev_data_res->arg.date.date
                                                 : p_ev_data_res->arg.date.time));
                        }

                    break;
                }

            break;
        }
}

/******************************************************************************
*
* binary division
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
* res = a ^ b
*
******************************************************************************/

PROC_EXEC_PROTO(c_power)
{
    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, pow(args[0]->arg.fp, args[1]->arg.fp));

    if(errno /* == EDOM */) /* SKS 15may97 test for 1/odd number */
    {
        if(args[1]->arg.fp <= 1.0)
            {
            F64 test_f64 = 1.0 / args[1]->arg.fp;
            const S32 power_of_two = 4194304; /*4*1024*1024*/
            const S32 mask = power_of_two - 1;
            S32 test_s32 = (S32) (test_f64 * power_of_two);
            S32 after_decimal_point = test_s32 & mask;

            if((test_s32 & power_of_two) == 0)
                after_decimal_point = mask - after_decimal_point;

            if(after_decimal_point < 2 /*arbitrary sloppy*/)
                p_ev_data_res->arg.fp = -(pow(-(args[0]->arg.fp), args[1]->arg.fp));
            else
                ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            }
    }
    else
        real_to_integer_try(p_ev_data_res);
}

/******************************************************************************
*
* res = a|b
*
******************************************************************************/

PROC_EXEC_PROTO(c_or)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, ((args[0]->arg.integer != 0) || (args[1]->arg.integer != 0)));
}

/******************************************************************************
*
* res = a==b
*
******************************************************************************/

PROC_EXEC_PROTO(c_eq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) == 0));
}

/******************************************************************************
*
* res = a>b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gt)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) > 0));
}

/******************************************************************************
*
* res = a>=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gteq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) >= 0));
}

/******************************************************************************
*
* res = a<b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lt)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) < 0));
}

/******************************************************************************
*
* res = a<=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lteq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) <= 0));
}

/******************************************************************************
*
* res = a!=b
*
******************************************************************************/

PROC_EXEC_PROTO(c_neq)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, (ss_data_compare(args[0], args[1]) != 0));
}

/******************************************************************************
*
* if(num, true_res, false_res)
*
******************************************************************************/

PROC_EXEC_PROTO(c_if)
{
    exec_func_ignore_parms();

    if(args[0]->arg.boolean)
        ss_data_resource_copy(p_ev_data_res, args[1]);
    else if(nargs > 2)
        ss_data_resource_copy(p_ev_data_res, args[2]);
    else
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NA);
}

/*-------------------------------------------------------------------------*/

/******************************************************************************
*
* set the value of a slot
*
******************************************************************************/

static S32
poke_slot(
    _InRef_     PC_EV_SLR slrp,
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR cur_slrp)
{
    P_EV_SLOT p_ev_slot;
    S32 res;

    /* can't overwrite formula slots */
    if(ev_travel(&p_ev_slot, slrp) > 0 && p_ev_slot->parms.type != EVS_CON_DATA)
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
        EV_RESULT result;

        ev_result_free_resources(&p_ev_slot->result);

        /* make sure we have a copy of data */
        ss_data_resource_copy(&temp, p_ev_data);

        ev_data_to_result_convert(&result, &temp);

        if((res = ev_make_slot(slrp, &result)) < 0)
            ev_result_free_resources(&result);
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
* set the value of a slot
* set_value(slr/range, value)
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
            rsblock rsb;
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

                    /* poke slot duplicates resources,
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

/*-------------------------------------------------------------------------*/

/*
range-ey functions
*/

static void
args_array_range_proc(
    P_EV_DATA args[EV_MAX_ARGS],
    S32 nargs,
    P_EV_DATA p_ev_data_res,
    EV_IDNO function_id);

static void
array_range_proc(
    stat_blockp stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_array(
    stat_blockp stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    stat_blockp stbp);

static void
array_range_proc_item(
    stat_blockp stbp,
    P_EV_DATA p_ev_data);

static void
array_range_proc_item_add(
    stat_blockp stbp,
    P_EV_DATA p_ev_data);

static S32
lookup_array_range_proc_array(
    look_blockp lkbp,
    P_EV_DATA p_ev_data);

static S32
lookup_array_range_proc_item(
    look_blockp lkbp,
    P_EV_DATA p_ev_data);

static EV_IDNO
npv_calc(
    P_EV_DATA p_ev_data_res,
    P_F64 intp,
    P_EV_DATA array);

static void
npv_item(
    P_EV_DATA p_ev_data,
    P_S32 countp,
    P_F64 parmp,
    P_F64 tempp,
    P_F64 resp);

/******************************************************************************
*
* avg(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_avg)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DAVG);
}

/******************************************************************************
*
* count(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_count)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DCOUNT);
}

/******************************************************************************
*
* error(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_error)
{
    exec_func_ignore_parms();

    ev_data_set_error(p_ev_data_res, create_error(EVAL_ERR_NOTIMPLEMENTED));
}

/******************************************************************************
*
* irr(guess, array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_irr)
{
    F64 lastnpv, lastguess, guess;
    S32 i;

    exec_func_ignore_parms();

    guess     = args[0]->arg.fp;
    lastguess = guess / 2;
    lastnpv   = 1.0;

    for(i = 0; i < 20; ++i)
        {
        F64 temp;
        EV_DATA npv;

        if(npv_calc(&npv, &guess, args[1]) != RPN_DAT_REAL)
            return;

        if(fabs(npv.arg.fp) < .0000001)
            break;

        temp = guess;
        guess -=  npv.arg.fp * (guess - lastguess) /
                 (npv.arg.fp - lastnpv);

        lastnpv   = npv.arg.fp;
        lastguess = temp;
        }

    if(i >= 20)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_IRR);
    else
        ev_data_set_real(p_ev_data_res, guess);
}

/******************************************************************************
*
* max(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_max)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DMAX);
}

/******************************************************************************
*
* min(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_min)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DMIN);
}

/******************************************************************************
*
* mirr(values_array, finance_rate, reinvest_rate
*
******************************************************************************/

PROC_EXEC_PROTO(c_mirr)
{
    stat_block stb;

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

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DSTD);
}

/******************************************************************************
*
* stdp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_stdp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DSTDP);
}

/******************************************************************************
*
* sum(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DSUM);
}

/******************************************************************************
*
* var(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_var)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DVAR);
}

/******************************************************************************
*
* varp(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_varp)
{
    exec_func_ignore_parms();

    args_array_range_proc(args, nargs, p_ev_data_res, RPN_FNF_DVARP);
}

/*-------------------------------------------------------------------------*/

/******************************************************************************
*
* process the arguments of the statistical functions
*
******************************************************************************/

static void
args_array_range_proc(
    P_EV_DATA args[EV_MAX_ARGS],
    S32 nargs,
    P_EV_DATA p_ev_data_res,
    EV_IDNO function_id)
{
    S32 i;
    stat_block stb;

    stat_block_init(&stb, function_id, 0, 0);

    for(i = 0; i < nargs; ++i)
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
    stat_blockp stbp,
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
            rsblock rsb;

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
    stat_blockp stbp,
    P_EV_DATA p_ev_data)
{
    asblock asb;

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
* helper routine for std/var(p) which accounts for small errors
* in our friendly floating point
*/

static F64
array_range_proc_stdvar_help(
    stat_blockp stbp,
    BOOL n_1)
{
    F64 n_sum_x_2 = stbp->temp * (F64) stbp->count;
    F64 sum_2 = stbp->result_data.arg.fp * stbp->result_data.arg.fp;
    F64 res = n_sum_x_2 - sum_2;

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
    stat_blockp stbp)
{
    ev_data_set_real(p_ev_data, 0.0);

    switch(stbp->function_id)
    {
    case RPN_FNF_DAVG:
        if(stbp->count)
        {
            if(stbp->result_data.did_num != RPN_DAT_REAL)
                ev_data_set_real(p_ev_data, (F64) stbp->result_data.arg.integer / (F64) stbp->count);
            else
                ev_data_set_real(p_ev_data, stbp->result_data.arg.fp / (F64) stbp->count);
        }
        break;

    case RPN_FNF_DMAX:
    case RPN_FNF_DMIN:
    case RPN_FNF_DSUM:
        if(stbp->count)
        {
            switch(stbp->result_data.did_num)
            {
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
                ev_data_set_integer(p_ev_data, stbp->result_data.arg.integer);
                break;

            default:
                *p_ev_data = stbp->result_data;
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
            ev_data_set_real(p_ev_data, stbp->result_data.arg.fp);
        break;

    case RPN_FNF_MIRR:
        if(stbp->count_a > 1)
        {
            ev_data_set_real(p_ev_data,
                             pow(-stbp->result1
                                * pow(stbp->parm1, (F64) stbp->count1)
                                / (stbp->result_data.arg.fp * stbp->parm),
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
    stat_blockp stbp,
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

        if(two_nums_type_match(p_ev_data, &stbp->result_data, size_worry) == TWO_INTS)
        {
            switch(stbp->function_id)
            {
            case RPN_FNF_DAVG:
            case RPN_FNF_DSUM:
                /* only dealing with individual narrower integer types here but SKS shows this may eventually overflow WORD16 */
                if(!stbp->count)
                    ev_data_set_integer(&stbp->result_data, p_ev_data->arg.integer);
                else
                    ev_data_set_integer(&stbp->result_data, p_ev_data->arg.integer + stbp->result_data.arg.integer);
                trace_1(TRACE_MODULE_EVAL, "DSUM result now int: %d", stbp->result_data.arg.integer);
                int_done = 1;
                break;

            case RPN_FNF_DMAX:
                if(!stbp->count || (p_ev_data->arg.integer > stbp->result_data.arg.integer))
                    ev_data_set_integer(&stbp->result_data, p_ev_data->arg.integer);
                int_done = 1;
                break;

            case RPN_FNF_DMIN:
                if(!stbp->count || (p_ev_data->arg.integer < stbp->result_data.arg.integer))
                    ev_data_set_integer(&stbp->result_data, p_ev_data->arg.integer);
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
                if(stbp->result_data.did_num == RPN_DAT_REAL)
                {
                    if(!stbp->count)
                        stbp->result_data.arg.fp  = p_ev_data->arg.fp;
                    else
                        stbp->result_data.arg.fp += p_ev_data->arg.fp;
                    trace_1(TRACE_MODULE_EVAL, "DSUM result now float: %g", stbp->result_data.arg.fp);
                }
                else
                    array_range_proc_item_add(stbp, p_ev_data);
                break;

            case RPN_FNF_DMAX:
                if(!stbp->count || (p_ev_data->arg.fp > stbp->result_data.arg.fp))
                    stbp->result_data.arg.fp = p_ev_data->arg.fp;
                break;

            case RPN_FNF_DMIN:
                if(!stbp->count || (p_ev_data->arg.fp < stbp->result_data.arg.fp))
                    stbp->result_data.arg.fp = p_ev_data->arg.fp;
                break;

            case RPN_FNF_DSTD:
            case RPN_FNF_DSTDP:
            case RPN_FNF_DVAR:
            case RPN_FNF_DVARP:
                {
                F64 x2 = p_ev_data->arg.fp * p_ev_data->arg.fp;

                if(!stbp->count)
                {
                    stbp->temp               = x2;
                    stbp->result_data.arg.fp = p_ev_data->arg.fp;
                }
                else
                {
                    stbp->temp               += x2;
                    stbp->result_data.arg.fp += p_ev_data->arg.fp;
                }

                break;
                }

            case RPN_FNF_NPV:
                npv_item(p_ev_data, &stbp->count, &stbp->parm, &stbp->temp, &stbp->result_data.arg.fp);
                stbp->count -= 1;
                break;

            case RPN_FNF_MIRR:
                if(p_ev_data->arg.fp >= 0)
                    npv_item(p_ev_data, &stbp->count1, &stbp->parm1, &stbp->temp1, &stbp->result1);
                else
                    npv_item(p_ev_data, &stbp->count,  &stbp->parm,  &stbp->temp,  &stbp->result_data.arg.fp);
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
    stat_blockp stbp,
    P_EV_DATA p_ev_data)
{
    if(!stbp->count)
        stbp->result_data = *p_ev_data;
    else
    {
        P_EV_DATA args[2];
        EV_DATA result_data;
        EV_SLR dummy_slr;

        args[0] = p_ev_data;
        args[1] = &stbp->result_data;

        c_add(args, 2, &result_data, &dummy_slr);

        stbp->result_data = result_data;
    }
}

/******************************************************************************
*
* call back for database function processing
*
******************************************************************************/

extern void
dbase_sub_function(
    stack_dbasep sdbp,
    P_EV_DATA cond_flagp)
{
    EV_SLR cur_slot;

    cur_slot      = sdbp->dbase_rng.s;
    cur_slot.col += sdbp->offset.col;
    cur_slot.row += sdbp->offset.row;

    /* work out state of condition */
    if((arg_normalise(cond_flagp, EM_REA, NULL, NULL) == RPN_DAT_REAL) &&
       (cond_flagp->arg.fp != 0.0))
        {
        EV_DATA data;

#if TRACE_ALLOWED
        if(tracing(TRACE_MODULE_EVAL))
            {
            char buffer[BUF_EV_LONGNAMLEN];
            ev_trace_slr(buffer, elemof32(buffer), "DBASE processing slot: $$", &cur_slot);
            trace_0(TRACE_MODULE_EVAL, buffer);
            }
#endif

        ev_slr_deref(&data, &cur_slot, FALSE);
        array_range_proc(sdbp->stbp, &data);
        }
#if TRACE_ALLOWED
    else if(tracing(TRACE_MODULE_EVAL))
        {
        char buffer[BUF_EV_LONGNAMLEN];
        ev_trace_slr(buffer, elemof32(buffer), "DBASE failed slot: $$", &cur_slot);
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
    stack_dbasep sdbp)
{
    array_range_proc_finish(p_ev_data, sdbp->stbp);
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
    look_blockp lkbp,
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
    look_blockp lkbp,
    P_EV_DATA p_ev_data)
{
    S32 res = 0;
    asblock asb;

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
    look_blockp lkbp,
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
    look_blockp lkbp,
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
    stack_lkp slkp)
{
    switch(slkp->lkbp->lookup_id)
        {
        case LOOKUP_LOOKUP:
            {
            EV_DATA data;

            array_range_mono_index(&data,
                                   &slkp->arg2,
                                   slkp->lkbp->count - 1,
                                   EM_REA | EM_SLR | EM_STR | EM_DAT | EM_BLK | EM_INT);
            /* MRJC 27.3.92 */
            ss_data_resource_copy(p_ev_data_res, &data);
            break;
            }

        case LOOKUP_HLOOKUP:
            {
            if(slkp->lkbp->result_data.did_num != RPN_DAT_SLR)
                ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
            else
                {
                ss_data_resource_copy(p_ev_data_res, &slkp->lkbp->result_data);
                p_ev_data_res->arg.slr.row += (EV_ROW) slkp->arg2.arg.integer;
                }
            break;
            }

        case LOOKUP_VLOOKUP:
            {
            if(slkp->lkbp->result_data.did_num != RPN_DAT_SLR)
                ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
            else
                {
                ss_data_resource_copy(p_ev_data_res, &slkp->lkbp->result_data);
                p_ev_data_res->arg.slr.col += (EV_COL) slkp->arg2.arg.integer;
                }
            break;
            }

        case LOOKUP_MATCH:
            if(slkp->lkbp->in_array)
                ev_data_set_integer(p_ev_data_res, slkp->lkbp->count);
            else
                ss_data_resource_copy(p_ev_data_res, &slkp->lkbp->result_data);
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
    stat_block stb;

    stat_block_init(&stb, RPN_FNF_NPV, *intp + 1, 0);

    array_range_proc(&stb, array);

    array_range_proc_finish(p_ev_data, &stb);

    return(p_ev_data->did_num);
}

/******************************************************************************
*
* process an individual item
* for npv/mirr
*
******************************************************************************/

static void
npv_item(
    P_EV_DATA p_ev_data,
    P_S32 countp,
    P_F64 parmp,
    P_F64 tempp,
    P_F64 resp)
{
    if(!*countp)
    {
        *tempp  = *parmp;                   /* interest */
        *resp   = p_ev_data->arg.fp / *parmp;   /* interest */
    }
    else
    {
        *tempp *= *parmp;                   /* interest */
        *resp  += p_ev_data->arg.fp / *tempp;
    }

    *countp += 1;
}

/******************************************************************************
*
* initialise stats data block
*
******************************************************************************/

extern void
stat_block_init(
    stat_blockp stbp,
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
        stbp->result_data.did_num     = RPN_DAT_WORD8; /* start with narrowest integer type */
        stbp->result_data.arg.integer = 0;
        break;

    default:
        ev_data_set_real(&stbp->result_data, 0.0);
        break;
    }
}

/* end of ev_exec.c */
