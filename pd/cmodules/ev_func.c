/* ev_func.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Less common (was split) semantic routines for evaluator */

/* JAD September 1994 split */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "cmodules/mathxtr3.h"

#include "cmodules/numform.h"

#include "version_x.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

#define PI                  (3.1415926535898)

static BOOL
uniform_distribution_seeded = FALSE;

/******************************************************************************
*
* age(date1, date2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_age)
{
    EV_DATE ev_date;

    exec_func_ignore_parms();

    ev_date.date = args[0]->arg.date.date - args[1]->arg.date.date;
    ev_date.time = args[0]->arg.date.time - args[1]->arg.date.time;
    ss_date_normalise(&ev_date);

    {
    S32 year, month, day;

    ss_dateval_to_ymd(&ev_date, &year, &month, &day);

    p_ev_data_res->arg.fp = (F64) year + (F64) month * 0.01;
    p_ev_data_res->did_num = RPN_DAT_REAL;
    } /*block*/
}

/******************************************************************************
*
* bin(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin)
{
    exec_func_ignore_parms();

    {
    S32 xs_0, ys_0, xs_1, ys_1;

    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    /* make result array */
    if(status_ok(ss_array_make(p_ev_data_res, 1, ys_1 + 1)))
    {

    { /* clear result array to 0 integers */
        S32 y;
        for(y = 0; y < ys_1 + 1; ++y)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);
            p_ev_data->did_num = RPN_DAT_WORD32;
            p_ev_data->arg.integer = 0;
        }
    } /*block*/

    { /* put each item into a bin */
        S32 y;
        for(y = 0; y < ys_0; ++y)
        {
            EV_DATA ev_data;

            switch(array_range_index(&ev_data, args[0], 0, y, EM_REA | EM_INT | EM_DAT | EM_STR))
            {
            case RPN_DAT_REAL:
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
            case RPN_DAT_DATE:
            case RPN_DAT_STRING:
                {
                S32 bin_y, bin_out_y = ys_1;

                for(bin_y = 0; bin_y < ys_1; ++bin_y)
                {
                    EV_DATA ev_data_bin;
                    S32 res;
                    array_range_mono_index(&ev_data_bin, args[1], bin_y, EM_REA | EM_INT | EM_DAT | EM_STR);
                    res = ss_data_compare(&ev_data, &ev_data_bin);
                    ss_data_free_resources(&ev_data_bin);
                    if(res <= 0)
                    {
                        bin_out_y = bin_y;
                        break;
                    }
                }

                ss_array_element_index_wr(p_ev_data_res, 0, bin_out_y)->arg.integer += 1;
                break;
                }

            default:
                break;
            }

            ss_data_free_resources(&ev_data);
        }
    } /*block*/

    }
    } /*block*/
}

/******************************************************************************
*
* char(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_char)
{
    exec_func_ignore_parms();

    if((U32) args[0]->arg.integer >= 256U)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        U8 ch = (U8) args[0]->arg.integer;
        status_assert(str_hlp_make_uchars(p_ev_data_res, &ch, 1));
    }
}

/******************************************************************************
*
* choose(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_choose)
{
    exec_func_ignore_parms();

    if((args[0]->arg.integer < 1))
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
        {
        look_block lkb;
        S32 res, i;

        lookup_block_init(&lkb,
                          NULL,
                          LOOKUP_CHOOSE,
                          (S32) args[0]->arg.integer,
                          0);

        for(i = 1, res = 0; i < nargs && !res; ++i)
            {
            do
                res = lookup_array_range_proc(&lkb, args[i]);
            while(res < 0);
            }

        if(res)
            ss_data_resource_copy(p_ev_data_res, &lkb.result);
        else
            data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        }
}

/******************************************************************************
*
* code("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_code)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.integer = (S32) *(args[0]->arg.string.data);
    p_ev_data_res->did_num     = RPN_DAT_WORD8;
}

/******************************************************************************
*
* col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    exec_func_ignore_parms();

    if(!nargs)
        p_ev_data_res->arg.integer = (S32) p_cur_slr->col + 1;
    else if(args[0]->did_num == RPN_DAT_SLR)
        p_ev_data_res->arg.integer = (S32) args[0]->arg.slr.col + 1;
    else if(args[0]->did_num == RPN_DAT_RANGE)
        p_ev_data_res->arg.integer = (S32) args[0]->arg.range.s.col + 1;
    else
    {
        data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
    }

    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* number of columns in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_cols)
{
    exec_func_ignore_parms();

    if(!nargs)
        p_ev_data_res->arg.integer = (S32) ev_numcol(p_cur_slr);
    else
    {
        S32 xs, ys;
        array_range_sizes(args[0], &xs, &ys);
        p_ev_data_res->arg.integer = xs;
    }

    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* command(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_command)
{
    exec_func_ignore_parms();

    {
    STATUS status = STATUS_OK;
    P_STACK_ENTRY p_stack_entry = NULL;
    P_STACK_ENTRY p_stack_entry_i = &stack_base[stack_offset];
    ARRAY_HANDLE h_commands;
    P_U8 p_u8;

    /* command context in custom function must be caller, not the function itself */
    while((p_stack_entry_i = stack_back_search(p_stack_entry_i - stack_base, EXECUTING_MACRO)) != NULL)
        p_stack_entry = p_stack_entry_i;

    if(NULL != (p_u8 = al_array_alloc(&h_commands, U8, args[0]->arg.string.size, &array_init_block_u8, &status)))
    {
        EV_DOCNO ev_docno = (EV_DOCNO) (p_stack_entry ? p_stack_entry->slr.docno : p_cur_slr->docno);
        void_memcpy32(p_u8, args[0]->arg.string.data, args[0]->arg.string.size);
        command_array_handle_execute(p_docu_from_docno(ev_docno), h_commands);
        al_array_dispose(&h_commands);
    }

    if(status_fail(status))
        data_set_error(p_ev_data_res, status);
    } /*block*/
}

#endif

/******************************************************************************
*
* cterm(interest, fv, pv)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cterm)
{
    F64 rate, fv, pv;
    BOOL fv_negative = 0; /* SKS - allow it to work out debts like Fireworkz */
    BOOL pv_negative = 0;
    exec_func_ignore_parms();

    rate = args[0]->arg.fp + 1.;
    fv = args[1]->arg.fp;
    pv = args[2]->arg.fp;

    if(fv < 0.0)
    {
        fv = fabs(fv);
        fv_negative = 1;
    }

    if(pv < 0.0)
    {
        pv = fabs(pv);
        pv_negative = 1;
    }

    if(fv_negative == pv_negative)
    {
        p_ev_data_res->arg.fp = (log(fv) - log(pv)) / log(rate);
        p_ev_data_res->did_num = RPN_DAT_REAL;
    }
    else
        data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SIGNS);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* currentcell
*
******************************************************************************/

PROC_EXEC_PROTO(c_currentcell)
{
    exec_func_ignore_parms();

    ev_current_cell(&p_ev_data_res->arg.slr);
    p_ev_data_res->did_num = RPN_DAT_SLR;
}

#endif

/******************************************************************************
*
* date(year, month, day)
*
******************************************************************************/

PROC_EXEC_PROTO(c_date)
{
    S32 our_year;

    exec_func_ignore_parms();

    our_year = (S32) args[0]->arg.integer;

    if(our_year < 100)
        our_year = sliding_window_year(our_year);

    if(ss_ymd_to_dateval(&p_ev_data_res->arg.date, our_year - 1, (S32) args[1]->arg.integer - 1, (S32) args[2]->arg.integer - 1) < 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        p_ev_data_res->did_num = RPN_DAT_DATE;
        p_ev_data_res->arg.date.time = 0;
    }
}

/******************************************************************************
*
* convert text into a date
* datevalue(text)
*
******************************************************************************/

PROC_EXEC_PROTO(c_datevalue)
{
    U8Z buffer[256];
    U32 len;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, elemof32(buffer)-1); /* SKS for 1.30 */
    void_memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || !p_ev_data_res->arg.date.date)
        data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
}

/******************************************************************************
*
* return day number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_day)
{
    exec_func_ignore_parms();

    if(args[0]->arg.date.date)
    {
        S32 year, month, day;

        ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);

        p_ev_data_res->arg.integer = (S32) day + 1;
        p_ev_data_res->did_num     = RPN_DAT_WORD8;
    }
    else
        data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
}

static const PC_USTR
ss_day_names[7] =
{
    "sunday",
    "monday",
    "tuesday",
    "wednesday",
    "thursday",
    "friday",
    "saturday"
};

/******************************************************************************
*
* dayname(n | date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dayname)
{
    S32 day = 1;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
        case RPN_DAT_DATE:
            day = ((args[0]->arg.date.date + 1) % 7) + 1;
            break;

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            day = MAX(1, args[0]->arg.integer);
            break;

        default: default_unhandled(); break;
    }

    day = (day - 1) % 7;

    if(status_ok(str_hlp_make_ustr(p_ev_data_res, ss_day_names[day])))
        *(p_ev_data_res->arg.string.data) = (U8) toupper(*(p_ev_data_res->arg.string.data));
}

/******************************************************************************
*
* ddb(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ddb)
{
    F64 cost, salvage, value;
    S32 life, period;

    exec_func_ignore_parms();

    value = cost = args[0]->arg.fp;
    salvage      = args[1]->arg.fp;
    life         = (S32) args[2]->arg.fp;
    period       = (S32) args[3]->arg.fp;

    if(cost < 0.0     ||
       salvage > cost ||
       life < 1       ||
       period < 1     ||
       period > life)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        S32 i;
        F64 cur_period;

        cur_period = 0;
        for(i = 0; i < period; ++i)
        {
            cur_period = (value * 2) / life;
            if(value - cur_period < salvage)
                cur_period = value - salvage;
            value -= cur_period;
        }

        p_ev_data_res->arg.fp = cur_period;
        p_ev_data_res->did_num = RPN_DAT_REAL;
    }
}

/******************************************************************************
*
* deref(slr)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deref)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_SLR)
        ev_slr_deref(p_ev_data_res, &args[0]->arg.slr, TRUE);
    else
        ss_data_resource_copy(p_ev_data_res, args[0]);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* double_click
*
******************************************************************************/

PROC_EXEC_PROTO(c_doubleclick)
{
    exec_func_ignore_parms();

    ev_double_click(&p_ev_data_res->arg.slr, p_cur_slr);
    if(p_ev_data_res->arg.slr.docno != DOCNO_NONE)
        p_ev_data_res->did_num = RPN_DAT_SLR;
    else
        data_set_error(p_ev_data_res, EVAL_ERR_NA);
}

#endif

/******************************************************************************
*
* exact("text1", "text2")
*
******************************************************************************/

PROC_EXEC_PROTO(c_exact)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.integer = 0;

    if(args[0]->arg.string.size == args[1]->arg.string.size)
        if(0 == memcmp(args[0]->arg.string.data, args[1]->arg.string.data, (size_t) args[0]->arg.string.size))
            p_ev_data_res->arg.integer = 1;

    p_ev_data_res->did_num = RPN_DAT_WORD8;
}

/******************************************************************************
*
* find("find_text", "in_text" {, start_n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_find)
{
    S32 start_n;

    exec_func_ignore_parms();

    if(nargs == 3)
        start_n = (S32) args[2]->arg.integer;
    else
        start_n = 1;

    if(start_n <= 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        S32 find_len = args[1]->arg.string.size;
        P_U8 ptr;

        start_n -= 1; /* SKS 12apr95 could have caused exception with find("str", "") as find_len == 0 */

        if( start_n > find_len)
            start_n = find_len;

        if(NULL == (ptr = (P_U8) memstr32(args[1]->arg.string.data + start_n, find_len - start_n, args[0]->arg.string.data, args[0]->arg.string.size))) /*strstr replacement*/
            p_ev_data_res->arg.integer = 0;
        else
            p_ev_data_res->arg.integer = (S32) (ptr - args[1]->arg.string.data) + 1;

        p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
    }
}

/******************************************************************************
*
* flip(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_flip)
{
    exec_func_ignore_parms();

    {
    S32 xs, ys, ys_half, y, y_swap;

    ss_data_resource_copy(p_ev_data_res, args[0]);
    data_ensure_constant(p_ev_data_res);

    if(p_ev_data_res->did_num == RPN_TMP_ARRAY)
    {
        array_range_sizes(p_ev_data_res, &xs, &ys);
        ys_half = ys / 2;
        y_swap = ys - 1;
        for(y = 0; y < ys_half; ++y, y_swap -= 2)
        {
            S32 x;
            for(x = 0; x < xs; ++x)
            {
                EV_DATA temp;
                temp = *ss_array_element_index_borrow(p_ev_data_res, x, y + y_swap);
                *ss_array_element_index_wr(p_ev_data_res, x, y + y_swap) =
                *ss_array_element_index_borrow(p_ev_data_res, x, y);
                *ss_array_element_index_wr(p_ev_data_res, x, y) = temp;
            }
        }
    }
    } /*block*/
}

/******************************************************************************
*
* formula(ref)
*
******************************************************************************/

PROC_EXEC_PROTO(c_formula_text)
{
    exec_func_ignore_parms();

    {
    P_EV_SLOT p_ev_slot;

    if(ev_travel(&p_ev_slot, &args[0]->arg.slr) > 0)
    {
        U8Z buffer[4096];
        EV_DOCNO docno = args[0]->arg.slr.docno;
        EV_OPTBLOCK optblock;

        ev_set_options(&optblock, docno);
        optblock.lf = optblock.cr = 0;

        ev_decode_slot(docno, buffer, p_ev_slot, &optblock);

        status_assert(str_hlp_make_ustr(p_ev_data_res, buffer));
    }
    } /*block*/
}

/******************************************************************************
*
* fv(payment, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fv)
{
    exec_func_ignore_parms();

    /* pmt * ((1 + interest) ^ n - 1 / interest */
    p_ev_data_res->arg.fp = args[0]->arg.fp * (pow(args[1]->arg.fp + 1.0, args[2]->arg.fp) - 1.0) / args[1]->arg.fp;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* grand({mean{,sd}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_grand)
{
    F64 s = 1.0;
    F64 m = 0.0;
    F64 r;

    exec_func_ignore_parms();

    switch(nargs)
    {
    case 2:
        if((s = args[1]->arg.fp) < 0.0)
            s = -s;

        /*FALLTHRU*/

    case 1:
        m = args[0]->arg.fp;
        break;

    default:
        break;
    }

    if(!uniform_distribution_seeded)
    {
        if((nargs >= 2) && (args[1]->arg.fp < 0.0))
            uniform_distribution_seed((unsigned int) -args[1]->arg.fp);
        else
            uniform_distribution_seed((unsigned int) time(NULL));

        uniform_distribution_seeded = TRUE;
    }

    r  = normal_distribution();

    r *= s;
    r += m;

    p_ev_data_res->arg.fp  = r;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* return hour number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_hour)
{
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date, &hour, &minute, &second);

    p_ev_data_res->arg.integer = (S32) hour;
    p_ev_data_res->did_num     = RPN_DAT_WORD8;
}

/******************************************************************************
*
* index(array, number, number {, xsize, ysize})
*
* returns slr if it can
*
******************************************************************************/

PROC_EXEC_PROTO(c_index)
{
    S32 x, y, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    x_size_out = y_size_out = 1;

    array_range_sizes(args[0], &x_size_in, &y_size_in);

    x = (S32) args[1]->arg.integer - 1;
    y = (S32) args[2]->arg.integer - 1;

    /* get size out parameters */
    if(nargs == 5)
        {
        x_size_out = (S32) args[3]->arg.integer;
        x_size_out = MAX(1, x_size_out);
        y_size_out = (S32) args[4]->arg.integer;
        y_size_out = MAX(1, y_size_out);
        }

    /* check it's all in range */
    if(x < 0                           ||
       y < 0                           ||
       x + x_size_out - 1 >= x_size_in ||
       y + y_size_out - 1 >= y_size_in)
        data_set_error(p_ev_data_res, EVAL_ERR_BAD_INDEX);
    else
        {
        if(x_size_out == 1 && y_size_out == 1)
            {
            EV_DATA temp;

            array_range_index(&temp, args[0], x, y, EM_ANY);
            ss_data_resource_copy(p_ev_data_res, &temp);
            }
        else
            {
            if(status_ok(ss_array_make(p_ev_data_res, x_size_out, y_size_out)))
                {
                S32 x_in, y_in, x_out, y_out;

                for(y_in = y, y_out = 0; y_out < y_size_out; ++y_in, ++y_out)
                    for(x_in = x, x_out = 0; x_out < x_size_out; ++x_in, ++x_out)
                        {
                        EV_DATA temp;
                        array_range_index(&temp, args[0], x_in, y_in, EM_ANY);
                        ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, x_out, y_out), &temp);
                        }
                }
            }
        }
}

/******************************************************************************
*
* string join(string1, string2 {, stringn})
*
******************************************************************************/

PROC_EXEC_PROTO(c_join)
{
    STATUS status;
    S32 i;
    S32 len = 0;
    P_U8 p_u8;

    exec_func_ignore_parms();

    for(i = 0; i < nargs; ++i)
        len += args[i]->arg.string.size;

    if(0 == len)
        return;

    if(NULL == (p_u8 = al_ptr_alloc_bytes(U8, len + 1, &status)))
        data_set_error(p_ev_data_res, status);
    else
    {
        p_ev_data_res->arg.string.data = p_u8;
        p_ev_data_res->arg.string.size = len;
        p_ev_data_res->did_num = RPN_TMP_STRING;

        for(i = 0; i < nargs; ++i)
        {
            U32 arglen = args[i]->arg.string.size;
            void_memcpy32(p_u8, args[i]->arg.string.data, arglen);
            p_u8 += arglen;
        }

        *p_u8 = NULLCH;
    }
}

/******************************************************************************
*
* string left(string, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_left)
{
    S32 n = (S32) args[1]->arg.integer;

    exec_func_ignore_parms();

    if(n <= 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        n = MIN(n, args[0]->arg.string.size);
        status_assert(str_hlp_make_uchars(p_ev_data_res, args[0]->arg.string.data, n));
    }
}

/******************************************************************************
*
* real length(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_length)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.integer = args[0]->arg.string.size;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* listcount(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_listcount)
{
    exec_func_ignore_parms();

    {
    STATUS status = STATUS_OK;
    EV_DATA ev_data_temp_array;

    ev_data_temp_array.did_num = RPN_DAT_BLANK;

    for(;;)
    {
        S32 /*xs, ys,*/ n_unique = 0, y = 0;

        ss_data_resource_copy(&ev_data_temp_array, args[0]);
        data_ensure_constant(&ev_data_temp_array);

        if(ev_data_temp_array.did_num == RPN_DAT_ERROR)
        {
            ss_data_free_resources(p_ev_data_res);
            *p_ev_data_res = ev_data_temp_array;
            return;
        }

        status_break(status = array_sort(&ev_data_temp_array, 0));

*p_ev_data_res = ev_data_temp_array;
ev_data_temp_array.did_num = RPN_DAT_BLANK;

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ev_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/******************************************************************************
*
* lower("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_lower)
{
    exec_func_ignore_parms();

    if(status_ok(str_hlp_dup(p_ev_data_res, args[0])))
    {
        S32 x;
        S32 len = p_ev_data_res->arg.string.size;
        P_U8 ptr = p_ev_data_res->arg.string.data;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) tolower(*ptr);
    }
}

/*
flatten an x by y array into a 1 by x*y array
*/

static void
ss_data_array_flatten(
    P_EV_DATA p_ev_data)
{
    if(p_ev_data->did_num == RPN_TMP_ARRAY)
    {
        /* this is just so easy given the current array representation */
        p_ev_data->arg.arrayp->y_size *= p_ev_data->arg.arrayp->x_size;
        p_ev_data->arg.arrayp->x_size = 1;
    }
}

/******************************************************************************
*
* median(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_median)
{
    exec_func_ignore_parms();

    {
    STATUS status = STATUS_OK;
    EV_DATA ev_data_temp_array;

    ev_data_temp_array.did_num = RPN_DAT_BLANK;

    for(;;)
    {
        S32 xs, ys, ys_half;
        F64 median;

        /* find median by sorting a copy of the source */
        ss_data_resource_copy(&ev_data_temp_array, args[0]);
        data_ensure_constant(&ev_data_temp_array);

        if(ev_data_temp_array.did_num == RPN_DAT_ERROR)
        {
            ss_data_free_resources(p_ev_data_res);
            *p_ev_data_res = ev_data_temp_array;
            return;
        }

        ss_data_array_flatten(&ev_data_temp_array); /* SKS 04jun96 - flatten so we can median() on horizontal and rectangular ranges */

        status_break(status = array_sort(&ev_data_temp_array, 0));

        array_range_sizes(&ev_data_temp_array, &xs, &ys);
        ys_half = ys / 2;

        {
        EV_DATA ev_data;
        array_range_index(&ev_data, &ev_data_temp_array, 0, ys_half, EM_REA);
        median = ev_data.arg.fp;
        } /*block*/

        if(0 == (ys & 1))
        {
            /* if there are an even number of elements, the median is the mean of the two middle ones */
            EV_DATA ev_data;
            array_range_index(&ev_data, &ev_data_temp_array, 0, ys_half - 1, EM_REA);
            median = (median + ev_data.arg.fp) / 2.0;
        }

        p_ev_data_res->arg.fp = median;
        p_ev_data_res->did_num = RPN_DAT_REAL;

        fp_to_integer_try(p_ev_data_res);

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ev_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/******************************************************************************
*
* string mid(string, start, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mid)
{
    exec_func_ignore_parms();

    if((args[1]->arg.integer <= 0) || (args[2]->arg.integer < 0))
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        S32 len = args[0]->arg.string.size;
        S32 start = (S32) args[1]->arg.integer - 1; /* ensured that this will not go -ve above */
        S32 n = args[2]->arg.integer;
        start = MIN(start, len);
        n = MIN(n, len - start);
        status_assert(str_hlp_make_uchars(p_ev_data_res, args[0]->arg.string.data + start, n));
    }
}

/******************************************************************************
*
* return minute number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_minute)
{
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date, &hour, &minute, &second);

    p_ev_data_res->arg.integer = (S32) minute;
    p_ev_data_res->did_num     = RPN_DAT_WORD8;
}

/******************************************************************************
*
* return month number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_month)
{
    exec_func_ignore_parms();

    if(args[0]->arg.date.date)
        {
        S32 year, month, day;

        ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);

        p_ev_data_res->arg.integer = (S32) month + 1;
        p_ev_data_res->did_num     = RPN_DAT_WORD8;
        }
    else
        data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
}

/******************************************************************************
*
* monthdays(date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_monthdays)
{
    S32 monthdays;

    exec_func_ignore_parms();

    {
    S32 year, month, day;

    ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);

    monthdays = ev_days[month];

    if((month == 1) && LEAP_YEAR(year))
        monthdays += 1;
    } /*block*/

    p_ev_data_res->did_num = RPN_DAT_WORD8;
    p_ev_data_res->arg.integer = (S32) monthdays;
}

static const PC_USTR
ss_month_names[12] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

/******************************************************************************
*
* monthname(n | date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_monthname)
{
    S32 month = 0;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
        {
        case RPN_DAT_DATE:
            {
            S32 year, day;
            ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);
            break;
            }

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            month = MAX(1, args[0]->arg.integer);
            month = (month - 1) % 12;
            break;

        default: default_unhandled(); break;
        }

    if(status_ok(str_hlp_make_ustr(p_ev_data_res, ss_month_names[month])))
        *(p_ev_data_res->arg.string.data) = (U8) toupper(*(p_ev_data_res->arg.string.data));
}

/******************************************************************************
*
* now
*
******************************************************************************/

PROC_EXEC_PROTO(c_now)
{
    exec_func_ignore_parms();

    ss_local_time_as_ev_date(&p_ev_data_res->arg.date);
    p_ev_data_res->did_num = RPN_DAT_DATE;
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* page(ref, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    exec_func_ignore_parms();

    {
    S32 xy = nargs < 2 ? 1 : args[1]->arg.integer;
    STATUS status = ev_page_slr(&args[0]->arg.slr, xy);

    if(status_fail(status))
    {
        data_set_error(p_ev_data_res, status);
        return;
    }

    p_ev_data_res->arg.integer = status + 1;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
    } /*block*/
}

/******************************************************************************
*
* pages(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pages)
{
    exec_func_ignore_parms();

    {
    S32 xy = nargs < 1 ? 1 : args[0]->arg.integer;
    STATUS status = ev_page_last(p_cur_slr, xy);

    if(status_fail(status))
    {
        data_set_error(p_ev_data_res, status);
        return;
    }

    p_ev_data_res->arg.integer = status;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
    } /*block*/
}

#endif

/******************************************************************************
*
* pmt(principal, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pmt)
{
    exec_func_ignore_parms();

    /* prin * int / (1-(int+1)^(-n)) */
    p_ev_data_res->arg.fp = args[0]->arg.fp * args[1]->arg.fp / (1.0 - pow(args[1]->arg.fp + 1.0, - args[2]->arg.fp));
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

#if 0 /* just for diff minimisation */

/******************************************************************************
*
* pmt_xls(interest, term, principal)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pmt_xls)
{
    exec_func_ignore_parms();

    /* - prin * int / (1-(int+1)^(-n)) */
    p_ev_data_res->arg.fp = - (args[2]->arg.fp * args[0]->arg.fp / (1.0 - pow(args[0]->arg.fp + 1.0, - args[1]->arg.fp)));
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

#endif

/******************************************************************************
*
* proper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_proper)
{
    exec_func_ignore_parms();

    if(status_ok(str_hlp_dup(p_ev_data_res, args[0])))
    {
        size_t x;
        size_t len = (size_t) p_ev_data_res->arg.string.size;
        P_U8 ptr = p_ev_data_res->arg.string.data;
        size_t offset_in_word = 0;

        for(x = 0; x < len; ++x, ++ptr)
        {
            if((*ptr == ' ') || (*ptr == '-') /* SKS 27aug97 Barnes-Wallis */)
                offset_in_word = 0;
            else
            {
                if((0 == offset_in_word)
                || ((2 == offset_in_word) && ('\'' == ptr[-1]) && ('O' == ptr[-2])))
                    /* SKS 17jul97 proper case O'Connor (but not A'chir?) */
                    *ptr = (U8) toupper(*ptr);
                else
                    *ptr = (U8) tolower(*ptr);

                ++offset_in_word;
            }
        }
    }
}

/******************************************************************************
*
* pv(payment, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pv)
{
    exec_func_ignore_parms();

    /* pmt * (1-(1+int)^(-n) / int */
    p_ev_data_res->arg.fp = args[0]->arg.fp * (1.0 - pow(1.0 + args[1]->arg.fp, - args[2]->arg.fp)) / args[1]->arg.fp;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* rand({n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rand)
{
    exec_func_ignore_parms();

    if(!uniform_distribution_seeded)
    {
        if((0 != nargs) && (args[0]->arg.fp != 0.0))
            uniform_distribution_seed((unsigned int) args[0]->arg.fp);
        else
            uniform_distribution_seed((unsigned int) time(NULL));

        uniform_distribution_seeded = TRUE;
    }

    p_ev_data_res->arg.fp = uniform_distribution();
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* rank(array{,spearflag})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank)
{
    BOOL spearman_correct = FALSE;

    exec_func_ignore_parms();

    if(nargs == 2)
        spearman_correct = (0 != args[1]->arg.integer);

    {
    S32 xs, ys;

    array_range_sizes(args[0], &xs, &ys);

    /* make result array */
    if(status_ok(ss_array_make(p_ev_data_res, 2, ys)))
    {
        S32 y;

        for(y = 0; y < ys; ++y)
        {
            S32 iy, position = 1, equal = 1;
            EV_DATA ev_data;

            array_range_index(&ev_data, args[0], 0, y, EM_CONST);
            for(iy = 0; iy < ys; ++iy)
            {
                if(iy != y)
                {
                    EV_DATA ev_data_t;
                    S32 res;
                    array_range_index(&ev_data_t, args[0], 0, iy, EM_CONST);
                    res = ss_data_compare(&ev_data, &ev_data_t);
                    if(res < 0)
                        position += 1;
                    else if(res == 0)
                        equal += 1;
                    ss_data_free_resources(&ev_data_t);
                }
            }

            {
            P_EV_DATA p_ev_data;
            
            p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);

            if(spearman_correct) /* SKS 12apr95 make suitable for passing to spearman with equal values */
            {
                p_ev_data->arg.fp = position + (equal - 1.0) / 2.0;
                p_ev_data->did_num = RPN_DAT_REAL;
            }
            else
            {
                p_ev_data->arg.integer = position;
                p_ev_data->did_num = ev_integer_size(p_ev_data[0].arg.integer);
            }

            p_ev_data = ss_array_element_index_wr(p_ev_data_res, 1, y);
            p_ev_data->arg.integer = equal;
            p_ev_data->did_num = ev_integer_size(p_ev_data->arg.integer);
            } /*block*/

            ss_data_free_resources(&ev_data);
        }
    }
    } /*block*/
}

/******************************************************************************
*
* rate(fv, pv, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rate)
{
    exec_func_ignore_parms();

    /*  (fv / pv) ^ (1/n) -1  */
    p_ev_data_res->arg.fp = pow((args[0]->arg.fp / args[1]->arg.fp), 1.0 / args[2]->arg.fp) - 1.0;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* rept("text", n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rept)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    const S32 len = args[0]->arg.string.size;
    S32 n = args[1]->arg.integer;

    exec_func_ignore_parms();

    if( n > (S32) (elemof32(buffer)-1) / len)
        n = (S32) (elemof32(buffer)-1) / len;

    if(n <= 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
    {
        P_U8 p_u8 = buffer;
        S32 x;

        for(x = 0; x < n; ++x)
        {
            void_memcpy32(p_u8, args[0]->arg.string.data, len);
            p_u8 += len;
        }

        status_assert(str_hlp_make_uchars(p_ev_data_res, buffer, n * len));
    }
}

/******************************************************************************
*
* replace("text", start_n, chars_n, "with_text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_replace)
{
    STATUS status = STATUS_OK;
    const S32 len = args[0]->arg.string.size;
    S32 start_n_chars = args[1]->arg.integer - 1; /* one-based API */
    S32 excise_n_chars = args[2]->arg.integer;
    QUICK_BLOCK_WITH_BUFFER(quick_block_result, BUF_EV_MAX_STRING_LEN);
    quick_block_with_buffer_setup(quick_block_result);

    exec_func_ignore_parms();

    if((start_n_chars < 0) || (excise_n_chars < 0))
        status = EVAL_ERR_ARGRANGE;
    else
    {
        if( start_n_chars > len)
            start_n_chars = len;

        if(0 != start_n_chars)
            status = quick_block_bytes_add(&quick_block_result, args[0]->arg.string.data, start_n_chars);

        /* NB even if 0 == excise_n_chars; that's just an insert operation */
        if(status_ok(status))
            status = quick_block_bytes_add(&quick_block_result, args[3]->arg.string.data, args[3]->arg.string.size);

        if(status_ok(status) && ((start_n_chars + excise_n_chars) < len))
        {
            S32 end_n_chars = len - (start_n_chars + excise_n_chars);

            status = quick_block_bytes_add(&quick_block_result, args[0]->arg.string.data + (start_n_chars + excise_n_chars), end_n_chars);
        }

        if(status_ok(status))
            status_assert(str_hlp_make_uchars(p_ev_data_res, quick_block_ptrc(&quick_block_result), quick_block_bytes(&quick_block_result)));
    }

    if(status_fail(status))
        data_set_error(p_ev_data_res, status);
}

/******************************************************************************
*
* reverse("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_reverse)
{
    exec_func_ignore_parms();

    if(status_ok(str_hlp_dup(p_ev_data_res, args[0])))
    {
        S32 x;
        S32 half_len = p_ev_data_res->arg.string.size / 2;
        P_U8 ptr_lo = p_ev_data_res->arg.string.data;
        P_U8 ptr_hi = p_ev_data_res->arg.string.data + p_ev_data_res->arg.string.size;

        for(x = 0; x < half_len; ++x)
        {
            U8 c = *--ptr_hi; /* SKS 04jul96 this was complete twaddle before */
            *ptr_hi = *ptr_lo;
            *ptr_lo++ = c;
        }
    }
}

/******************************************************************************
*
* string right(string, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_right)
{
    exec_func_ignore_parms();

    if(args[1]->arg.integer <= 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
        {
        S32 len = args[0]->arg.string.size;
        S32 n = (S32) args[1]->arg.integer; /* not -ve */
        n = MIN(n, len);
        status_assert(str_hlp_make_uchars(p_ev_data_res, args[0]->arg.string.data + len - n, n));
        }
}

/******************************************************************************
*
* round(n, decimals)
*
******************************************************************************/

PROC_EXEC_PROTO(c_round)
{
    BOOL neg_flag = 0;
    S32 places;
    F64 temp, base;

    exec_func_ignore_parms();

    if(nargs == 2)
        places = MIN(15, args[1]->arg.integer);
    else
        places = 2;

    temp = args[0]->arg.fp;

    if(args[0]->arg.fp < 0.0)
    {
        neg_flag = 1;
        temp = -temp;
    }

    base = pow(10.0, (F64) places);
    temp = temp * base;
    temp = floor(temp + 0.5); /* ok 'cos we've got a positive number here */
    temp = temp / base;

    p_ev_data_res->arg.fp  = neg_flag ? -temp : temp;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* row / row(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_row)
{
    exec_func_ignore_parms();

    if(!nargs)
        p_ev_data_res->arg.integer = (S32) p_cur_slr->row + 1;
    else if(args[0]->did_num == RPN_DAT_SLR)
        p_ev_data_res->arg.integer = (S32) args[0]->arg.slr.row + 1;
    else if(args[0]->did_num == RPN_DAT_RANGE)
        p_ev_data_res->arg.integer = (S32) args[0]->arg.range.s.row + 1;
    else
        {
        data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
        }

    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* number of rows in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_rows)
{
    exec_func_ignore_parms();

    if(!nargs)
        p_ev_data_res->arg.integer = (S32) ev_numrow(p_cur_slr);
    else
    {
        S32 xs, ys;
        array_range_sizes(args[0], &xs, &ys);
        p_ev_data_res->arg.integer = ys;
    }

    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* return second number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_second)
{
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date, &hour, &minute, &second);

    p_ev_data_res->arg.integer = (S32) second;
    p_ev_data_res->did_num     = RPN_DAT_WORD8;
}

/******************************************************************************
*
* define and set value of a name
*
* set_name("name", value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_setname)
{
    S32 res;
    EV_NAMEID name_key;

    exec_func_ignore_parms();

    if((res = name_make(&name_key, p_cur_slr->docno, args[0]->arg.string.data, args[1])) < 0)
        data_set_error(p_ev_data_res, res);
    else
    {
        EV_NAMEID name_num = name_def_find(name_key);
        ss_data_resource_copy(p_ev_data_res, &name_ptr(name_num)->def);
    }
}

/******************************************************************************
*
* sln(cost, salvage, life)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sln)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp = (args[0]->arg.fp - args[1]->arg.fp) / args[2]->arg.fp;
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* sort(array, col)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sort)
{
    exec_func_ignore_parms();

    ss_data_resource_copy(p_ev_data_res, args[0]);
    data_ensure_constant(p_ev_data_res);

    {
    STATUS status;
    if(status_fail(status = array_sort(p_ev_data_res, (nargs < 2) ? 0 : (args[1]->arg.integer - 1))))
    {
        ss_data_free_resources(p_ev_data_res);
        data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/******************************************************************************
*
* spearman(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_spearman)
{
    exec_func_ignore_parms();

    {
    S32 xs_0, ys_0, xs_1, ys_1, y, ys;

    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    ys = MAX(ys_0, ys_1);

    {
    F64 sum_d_squared = 0;
    S32 n_counted = 0;

    for(y = 0; y < ys; ++y)
    {
        EV_DATA ev_data_0;
        EV_DATA ev_data_1;
        EV_IDNO ev_idno_0 = array_range_index(&ev_data_0, args[0], 0, y, EM_REA);
        EV_IDNO ev_idno_1 = array_range_index(&ev_data_1, args[1], 0, y, EM_REA);

        if(ev_idno_0 == RPN_DAT_REAL && ev_idno_1 == RPN_DAT_REAL
           &&
           ev_data_0.arg.fp != 0. && ev_data_1.arg.fp != 0.)
        {
            F64 d = ev_data_1.arg.fp - ev_data_0.arg.fp;
            n_counted += 1;
            sum_d_squared += d * d;
        }

        ss_data_free_resources(&ev_data_0);
        ss_data_free_resources(&ev_data_1);
    }

    if(n_counted)
    {
        p_ev_data_res->arg.fp = (1. - (6. * sum_d_squared) / ((F64) n_counted * ((F64) n_counted * (F64) n_counted - 1.)) );
        p_ev_data_res->did_num = RPN_DAT_REAL;
    }
    else
        data_set_error(p_ev_data_res, EVAL_ERR_NO_VALID_DATA);
    } /*block*/

    } /*block*/
}

/******************************************************************************
*
* string(n{, x})
*
******************************************************************************/

PROC_EXEC_PROTO(c_string)
{
    S32 places;
    U8Z buffer[BUF_EV_MAX_STRING_LEN];

    exec_func_ignore_parms();

    c_round(args, nargs, p_ev_data_res, p_cur_slr);

    if(nargs == 2)
        places = (S32) args[1]->arg.integer;
    else
        places = 2;

    places = MAX(places,   0);
    places = MIN(places, 100);

    (void) xsnprintf(buffer, elemof32(buffer), "%.*f", (int) places, p_ev_data_res->arg.fp);

    status_assert(str_hlp_make_ustr(p_ev_data_res, buffer));
}

/******************************************************************************
*
* syd(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_syd)
{
    exec_func_ignore_parms();

    /* (c-s) * (n-p+1) / (n*(n+1)/2) */
    p_ev_data_res->arg.fp = ((args[0]->arg.fp -  args[1]->arg.fp) * (args[2]->arg.fp -  args[3]->arg.fp + 1.0)) / ((args[2]->arg.fp * (args[2]->arg.fp + 1.0) / 2.0));
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* term(payment, interest, fv)
*
******************************************************************************/

PROC_EXEC_PROTO(c_term)
{
    exec_func_ignore_parms();

    /* ln(1+(fv * int/pmt)) / ln(1+int)   */
    p_ev_data_res->arg.fp = log(1.0 + (args[2]->arg.fp * args[1]->arg.fp / args[0]->arg.fp)) / log(1.0 + args[1]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

_Check_return_
static STATUS
ev_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_EV_DATA p_ev_data)
{
    NUMFORM_PARMS numform_parms;

    numform_parms.ustr_numform_numeric =
    numform_parms.ustr_numform_datetime =
    numform_parms.ustr_numform_texterror = ustr;

    numform_parms.p_numform_context = P_DATA_NONE; /* use default */

    return(numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms));
}

/******************************************************************************
*
* text(n, numform_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_text)
{
    exec_func_ignore_parms();

    {
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    if(status_ok(quick_ublock_uchars_add(&quick_ublock_format, args[1]->arg.string.data, args[1]->arg.string.size)))
        if(status_ok(quick_ublock_nullch_add(&quick_ublock_format)))
            status_assert(ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]));

    status_assert(str_hlp_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
    } /*block*/
}

/******************************************************************************
*
* time(hour, minute, second)
*
******************************************************************************/

PROC_EXEC_PROTO(c_time)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.date.date = 0;

    if(ss_hms_to_timeval(&p_ev_data_res->arg.date,
                         (S32) args[0]->arg.integer,
                         (S32) args[1]->arg.integer,
                         (S32) args[2]->arg.integer) < 0)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
    else
        {
        ss_date_normalise(&p_ev_data_res->arg.date);
        p_ev_data_res->did_num = RPN_DAT_DATE;
        }
}

/******************************************************************************
*
* convert text to time value
*
* timevalue(text)
*
******************************************************************************/

PROC_EXEC_PROTO(c_timevalue)
{
    U8Z buffer[256];
    U32 len;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, sizeof32(buffer)-1);
    void_memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || p_ev_data_res->arg.date.date)
        data_set_error(p_ev_data_res, EVAL_ERR_BADTIME);
}

/******************************************************************************
*
* return today's date
*
******************************************************************************/

PROC_EXEC_PROTO(c_today)
{
    exec_func_ignore_parms();

    c_now(args, nargs, p_ev_data_res, p_cur_slr);
    p_ev_data_res->arg.date.time = 0;
}

/******************************************************************************
*
* trim("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_trim)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    S32 gap;
    P_U8 s_ptr, e_ptr, i_ptr, o_ptr;

    exec_func_ignore_parms();

    void_memcpy32(buffer, args[0]->arg.string.data, args[0]->arg.string.size);

    s_ptr = buffer;
    e_ptr = buffer + args[0]->arg.string.size;

    while(*s_ptr == ' ')
        ++s_ptr;

    while((e_ptr > s_ptr) && (e_ptr[-1] == ' '))
        --e_ptr;

    *e_ptr = NULLCH;

    /* crunge multiple spaces */
    for(i_ptr = o_ptr = s_ptr, gap = 0; i_ptr <= e_ptr; ++i_ptr)
        {
        if(*i_ptr == ' ')
            {
            if(gap)
                continue;
            else
                gap = 1;
            }
        else
            gap = 0;

        *o_ptr++ = *i_ptr;
        }

    status_assert(str_hlp_make_ustr(p_ev_data_res, s_ptr));
}

/******************************************************************************
*
* return type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_type)
{
    EV_TYPE type;
    PC_USTR type_str;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
        {
        case RPN_DAT_REAL:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            type = EM_REA;
            break;

        case RPN_DAT_SLR:
            type = EM_SLR;
            break;

        case RPN_DAT_STRING:
        case RPN_TMP_STRING:
        case RPN_RES_STRING:
            type = EM_STR;
            break;

        case RPN_DAT_DATE:
            type = EM_DAT;
            break;

        case RPN_DAT_RANGE:
        case RPN_TMP_ARRAY:
        case RPN_RES_ARRAY:
            type = EM_ARY;
            break;

        case RPN_DAT_BLANK:
            type = EM_BLK;
            break;

        default:
        case RPN_DAT_ERROR:
            type = EM_ERR;
            break;
        }

    type_str = type_from_flags(type);

    status_assert(str_hlp_make_ustr(p_ev_data_res, type_str));
}

/******************************************************************************
*
* upper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_upper)
{
    exec_func_ignore_parms();

    if(status_ok(str_hlp_dup(p_ev_data_res, args[0])))
    {
        S32 x;
        S32 len = p_ev_data_res->arg.string.size;
        P_U8 ptr = p_ev_data_res->arg.string.data;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) toupper(*ptr);
    }
}

/******************************************************************************
*
* value("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_value)
{
    U8Z buffer[256];
    U32 len;
    P_U8 ptr;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, sizeof32(buffer)-1);
    void_memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    p_ev_data_res->arg.fp = strtod(buffer, &ptr);

    if(ptr == buffer)
        p_ev_data_res->arg.fp = 0.0; /* SKS 08sep97 now behaves as documented */

    p_ev_data_res->did_num = RPN_DAT_REAL;

    fp_to_integer_try(p_ev_data_res);
}

/******************************************************************************
*
* return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp = strtod(applicationversion, NULL);
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* return the day of the week for a date
*
******************************************************************************/

PROC_EXEC_PROTO(c_weekday)
{
    S32 weekday;

    exec_func_ignore_parms();

    weekday = (args[0]->arg.date.date + 1) % 7;

    p_ev_data_res->arg.integer = weekday + 1;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* return the week number for a date
*
******************************************************************************/

#if RISCOS

static void
fivebytetime_from_date(
    unsigned char * utc /*[5]*/,
    P_EV_DATE p_ev_date)
{
    riscos_time_ordinals time_ordinals;
    _kernel_swi_regs rs;

    zero_struct(time_ordinals);
    ss_dateval_to_ymd(p_ev_date, &time_ordinals.year, &time_ordinals.month, &time_ordinals.day);

    time_ordinals.year += 1;
    time_ordinals.month += 1;
    time_ordinals.day += 1;

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) utc;
    rs.r[2] = (int) &time_ordinals;
    if(NULL != _kernel_swi(Territory_ConvertOrdinalsToTime, &rs, &rs))
        void_memset32(utc, 0, 5);
}

#endif

PROC_EXEC_PROTO(c_weeknumber)
{
    S32 weeknumber;
    U8Z buffer[32];

    exec_func_ignore_parms();

    {
#if RISCOS
    _kernel_swi_regs rs;
    unsigned char utc[5];

    fivebytetime_from_date(utc, &args[0]->arg.date);

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) utc;
    rs.r[2] = (int) buffer;
    rs.r[3] = sizeof32(buffer);
    rs.r[4] = (int) "%WK";
    if(NULL != _kernel_swi(Territory_ConvertDateAndTime, &rs, &rs))
        weeknumber = 0; /* a result of zero -> info not available */
    else
        weeknumber = (S32) fast_strtoul(buffer, NULL);
#else
    S32 year, month, day;
    struct tm tm;

    ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);

    zero_struct(tm);
    tm.tm_year = (int) (year + 1 - 1900);
    tm.tm_mon = (int) month;
    tm.tm_mday = (int) (day + 1);

    /* actually needs wday and yday setting up! */
    (void) mktime(&tm); /* normalise */

    strftime(buffer, elemof32(buffer), "%W", &tm);

    weeknumber = fast_strtoul(buffer, NULL);

    weeknumber += 1;
#endif
    } /*block*/

    p_ev_data_res->arg.integer = weeknumber;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* return year number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_year)
{
    exec_func_ignore_parms();

    if(!args[0]->arg.date.date)
    {
        data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    {
    S32 year, month, day;

    ss_dateval_to_ymd(&args[0]->arg.date, &year, &month, &day);

    p_ev_data_res->arg.integer = (S32) year + 1;
    p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
    } /*block*/
}

/* primitive number/trig functions moved here as per Fireworkz split*/

/******************************************************************************
*
* abs(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_abs)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = fabs(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* acos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acos)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = acos(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* asin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asin)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = asin(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* atan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = atan(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* no error cases */
}

/******************************************************************************
*
* cos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cos)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = cos(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* deg(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deg)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = args[0]->arg.fp * (180.0/PI);
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* exp(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_exp)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = exp(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_res->arg.fp == HUGE_VAL)
        data_set_error(p_ev_data_res, EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* int(number)
*
* NB truncates towards zero
*
******************************************************************************/

PROC_EXEC_PROTO(c_int)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_REAL)
        {
        /* SKS as per Fireworkz, try pre-rounding an ickle bit so INT((0.06-0.04)/0.01) is 2 not 1 */
        int exponent;
        F64 mantissa = frexp(args[0]->arg.fp, &exponent);
        F64 f64;
        if(mantissa > (+1.0 - 1E-14))
            mantissa = +1.0;
        else if(mantissa < (-1.0 + 1E-14))
            mantissa = -1.0;
        f64 = ldexp(mantissa, exponent);
        (void) modf(f64, &p_ev_data_res->arg.fp);
        p_ev_data_res->did_num = RPN_DAT_REAL;
        fp_to_integer_try(p_ev_data_res);
        }
    else
        {
        p_ev_data_res->arg.integer = args[0]->arg.integer;
        p_ev_data_res->did_num     = args[0]->did_num;
        }
}

/******************************************************************************
*
* ln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ln)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = log(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* log(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = log10(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    if(errno /* == EDOM, ERANGE */)
        data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* mod(a,b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1], FALSE))
        {
        case TWO_INTS:
            /* SKS after 4.11 03feb92 - gave FP error not zero if trap taken */
            if(args[1]->arg.integer == 0)
                data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
            else
                {
                p_ev_data_res->arg.integer = args[0]->arg.integer % args[1]->arg.integer;
                p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
                }
            break;

        case TWO_REALS:
            {
            errno = 0;

            p_ev_data_res->arg.fp  = fmod(args[0]->arg.fp, args[1]->arg.fp);
            p_ev_data_res->did_num = RPN_DAT_REAL;

            /* would have divided by zero? */
            if(errno /* == EDOM */)
                data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);

            break;
            }
        }
}

/* a little note from SKS:

 * it seems that fmod(a, b) guarantees the sign of the result
 * to be the same sign as a, whereas integer ops give an
 * implementation defined sign for both a / b and a % b.
*/

/******************************************************************************
*
* pi
*
******************************************************************************/

PROC_EXEC_PROTO(c_pi)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_REAL;
    p_ev_data_res->arg.fp  = PI;
}

/******************************************************************************
*
* rad(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rad)
{
    exec_func_ignore_parms();

    p_ev_data_res->arg.fp  = args[0]->arg.fp * (PI/180.0);
    p_ev_data_res->did_num = RPN_DAT_REAL;
}

/******************************************************************************
*
* sin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sin)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = sin(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* sqr(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sqr)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp = sqrt(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    if(errno /* == EDOM */)
        data_set_error(p_ev_data_res, EVAL_ERR_NEG_ROOT);
}

/******************************************************************************
*
* tan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tan)
{
    exec_func_ignore_parms();

    errno = 0;

    p_ev_data_res->arg.fp  = tan(args[0]->arg.fp);
    p_ev_data_res->did_num = RPN_DAT_REAL;

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/* end of ev_func.c */
