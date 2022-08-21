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
* REAL age(date1, date2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_age)
{
    F64 age_result;
    EV_DATE ev_date;
    S32 year, month, day;

    exec_func_ignore_parms();

    ev_date.date = args[0]->arg.date.date - args[1]->arg.date.date;
    ev_date.time = args[0]->arg.date.time - args[1]->arg.date.time;
    ss_date_normalise(&ev_date);

    ss_dateval_to_ymd(&ev_date.date, &year, &month, &day);

    age_result = (F64) year + (F64) month * 0.01;

    ev_data_set_real(p_ev_data_res, age_result);
}

/******************************************************************************
*
* bin(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin)
{
    S32 xs_0, ys_0, xs_1, ys_1;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    /* make result array */
    if(status_ok(ss_array_make(p_ev_data_res, 1, ys_1 + 1)))
    {
        { /* clear result array to zero as widest integers */
        S32 y;
        for(y = 0; y < ys_1 + 1; ++y)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);
            ev_data_set_WORD32(p_ev_data, 0);
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
}

/******************************************************************************
*
* STRING char(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_char)
{
    S32 n = args[0]->arg.integer;
    U8 ch = (U8) n;

    exec_func_ignore_parms();

    if((U32) n >= 256U) /* Some users are known to put highlight characters in so don't get too picky */
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    status_assert(ss_string_make_uchars(p_ev_data_res, &ch, 1));
}

/******************************************************************************
*
* THING choose(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_choose)
{
    look_block lkb;
    S32 res, i;

    exec_func_ignore_parms();

    if((args[0]->arg.integer < 1) || (args[0]->arg.integer >= nargs))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    lookup_block_init(&lkb,
                      NULL,
                      LOOKUP_CHOOSE,
                      args[0]->arg.integer,
                      0);

    for(i = 1, res = 0; i < nargs && !res; ++i)
    {
        do
            res = lookup_array_range_proc(&lkb, args[i]);
        while(res < 0);
    }

    if(0 == res)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    status_assert(ss_data_resource_copy(p_ev_data_res, &lkb.result_data));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING clean("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_clean)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    U32 o_idx = 0;
    S32 i_idx;

    exec_func_ignore_parms();

    /* clean the string of unprintable chracters during transfer */
    for(i_idx = 0; i_idx < args[0]->arg.string.size; ++i_idx)
    {
        U8 u8 = args[0]->arg.string.data[i_idx];

        if(!t5_isprint(u8))
            continue;

        buffer[o_idx++] = u8;

        if(o_idx == elemof32(buffer))
            break;
    }

    status_assert(ss_string_make_uchars(p_ev_data_res, buffer, o_idx));
}

#endif

/******************************************************************************
*
* INTEGER code("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_code)
{
    S32 code_result;
    
    exec_func_ignore_parms();

    if(0 == args[0]->arg.string.size)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    code_result = (S32) *(args[0]->arg.string.data);

    ev_data_set_integer(p_ev_data_res, code_result);
}

/******************************************************************************
*
* INTEGER col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    S32 col_result;

    exec_func_ignore_parms();

    if(0 == nargs)
        col_result = (S32) p_cur_slr->col + 1;
    else if(args[0]->did_num == RPN_DAT_SLR)
        col_result = (S32) args[0]->arg.slr.col + 1;
    else if(args[0]->did_num == RPN_DAT_RANGE)
        col_result = (S32) args[0]->arg.range.s.col + 1;
    else
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
    }

    ev_data_set_integer(p_ev_data_res, col_result);
}

/******************************************************************************
*
* INTEGER number of columns in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_cols)
{
    S32 cols_result;

    exec_func_ignore_parms();

    if(0 == nargs)
        cols_result = (S32) ev_numcol(p_cur_slr);
    else
    {
        S32 xs, ys;
        array_range_sizes(args[0], &xs, &ys);
        cols_result = xs;
    }

    ev_data_set_integer(p_ev_data_res, cols_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* command(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_command)
{
    STATUS status = STATUS_OK;
    P_STACK_ENTRY p_stack_entry = NULL;
    P_STACK_ENTRY p_stack_entry_i = &stack_base[stack_offset];
    ARRAY_HANDLE h_commands;
    P_U8 p_u8;

    exec_func_ignore_parms();

    /* command context in custom function must be topmost caller, not the function itself */
    while((p_stack_entry_i = stack_back_search(PtrDiffElemU32(p_stack_entry_i, stack_base), EXECUTING_MACRO)) != NULL)
        p_stack_entry = p_stack_entry_i;

    if(NULL != (p_u8 = al_array_alloc(&h_commands, U8, args[0]->arg.string.size, &array_init_block_u8, &status)))
    {
        EV_DOCNO ev_docno = (EV_DOCNO) (p_stack_entry ? p_stack_entry->slr.docno : p_cur_slr->docno);
        memcpy32(p_u8, args[0]->arg.string.data, args[0]->arg.string.size);
        status_consume(command_array_handle_execute(ev_docno, h_commands)); /* error already reported */
        al_array_dispose(&h_commands);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

#endif

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

#if 0 /* just for diff minimization */

/******************************************************************************
*
* SLR current_cell
*
******************************************************************************/

PROC_EXEC_PROTO(c_current_cell)
{
    exec_func_ignore_parms();

    ev_current_cell(&p_ev_data_res->arg.slr);
    p_ev_data_res->did_num = RPN_DAT_SLR;
}

#endif

/******************************************************************************
*
* DATE date(year, month, day)
*
******************************************************************************/

PROC_EXEC_PROTO(c_date)
{
    S32 year = args[0]->arg.integer;
    S32 our_year = year;
    S32 month = args[1]->arg.integer;
    S32 day = args[2]->arg.integer;

    exec_func_ignore_parms();

    if(our_year < 100)
        our_year = sliding_window_year(our_year);

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.date.time = EV_TIME_INVALID;

    if(ss_ymd_to_dateval(&p_ev_data_res->arg.date.date, our_year - 1, month - 1, day - 1) < 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }
}

/******************************************************************************
*
* DATE datevalue(text) - convert text into a date
*
******************************************************************************/

PROC_EXEC_PROTO(c_datevalue)
{
    U8Z buffer[256];
    U32 len;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, elemof32(buffer)-1); /* SKS for 1.30 */
    memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || (EV_DATE_INVALID == p_ev_data_res->arg.date.date))
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
}

/******************************************************************************
*
* INTEGER return day number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_day)
{
    S32 day_result;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_INVALID == args[0]->arg.date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day);

    day_result = day + 1;

    ev_data_set_integer(p_ev_data_res, day_result);
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
* STRING dayname(n | date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dayname)
{
    S32 day = 1;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        if(EV_DATE_INVALID == args[0]->arg.date.date)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
            return;
        }
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

    if(status_ok(ss_string_make_ustr(p_ev_data_res, ss_day_names[day])))
        *(p_ev_data_res->arg.string_wr.data) = (U8) toupper(*(p_ev_data_res->arg.string.data));
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
* deref(slr)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deref)
{
    exec_func_ignore_parms();

    if(args[0]->did_num == RPN_DAT_SLR)
        ev_slr_deref(p_ev_data_res, &args[0]->arg.slr, TRUE);
    else
        status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING dollar(number {, decimals})
*
* a bit like our old STRING() function but with a currency symbol and thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dollar)
{
    STATUS status;
    S32 decimal_places = 2;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(nargs > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    round_common(args, nargs, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses nargs, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    status = quick_ublock_ustr_add(&quick_ublock_format, USTR_TEXT("£")); /* FIXME */

    if(status_ok(status))
        status = quick_ublock_ustr_add(&quick_ublock_format, USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_u8_add(&quick_ublock_format, '.');

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_u8_add(&quick_ublock_format, '#');

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ev_data_res);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* SLR double_click
*
******************************************************************************/

PROC_EXEC_PROTO(c_doubleclick)
{
    exec_func_ignore_parms();

    ev_double_click(&p_ev_data_res->arg.slr, p_cur_slr);

    if(p_ev_data_res->arg.slr.docno == DOCNO_NONE)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NA);
        return;
    }

    p_ev_data_res->did_num = RPN_DAT_SLR;
}

/******************************************************************************
*
* NUMBER even(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_even)
{
    BOOL negative = FALSE;
    F64 f64 = args[0]->arg.fp;
    F64 even_result;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negative = TRUE;
    }

    even_result = 2.0 * ceil(f64 / 2.0);

    /* always round away from zero (Excel) */
    if(negative)
        even_result = -even_result;

    ev_data_set_real(p_ev_data_res, even_result);
    real_to_integer_try(p_ev_data_res);
}

#endif

/******************************************************************************
*
* BOOLEAN exact("text1", "text2")
*
******************************************************************************/

PROC_EXEC_PROTO(c_exact)
{
    BOOL exact_result = FALSE;

    exec_func_ignore_parms();

    if(args[0]->arg.string.size == args[1]->arg.string.size)
        if(0 == memcmp32(args[0]->arg.string.data, args[1]->arg.string.data, (U32) args[0]->arg.string.size))
            exact_result = TRUE;

    ev_data_set_boolean(p_ev_data_res, exact_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* BOOLEAN false
*
******************************************************************************/

PROC_EXEC_PROTO(c_false)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, FALSE);
}

#endif

/******************************************************************************
*
* INTEGER find("find_text", "in_text" {, start_n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_find)
{
    S32 find_result = 0;
    S32 find_len = args[1]->arg.string.size;
    S32 start_n = 1;
    P_U8 ptr;

    exec_func_ignore_parms();

    if(nargs > 2)
        start_n = args[2]->arg.integer;

    if(start_n <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    start_n -= 1; /* SKS 12apr95 could have caused exception with find("str", "") as find_len == 0 */

    if( start_n > find_len)
        start_n = find_len;

    if(NULL != (ptr = (P_U8) memstr32(args[1]->arg.string.data + start_n, find_len - start_n, args[0]->arg.string.data, args[0]->arg.string.size))) /*strstr replacement*/
        find_result = 1 + (S32) (ptr - args[1]->arg.string.data);

    ev_data_set_integer(p_ev_data_res, find_result);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING fixed(number {, decimals {, no_commas}})
*
* a bit like our old STRING() function but with thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fixed)
{
    STATUS status;
    S32 decimal_places = 2;
    BOOL no_commas = FALSE;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(nargs > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    if(nargs > 2)
        no_commas = (0 != args[2]->arg.integer);

    round_common(args, nargs, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses nargs, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    status = quick_ublock_ustr_add(&quick_ublock_format, no_commas ? USTR_TEXT("0") : USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_u8_add(&quick_ublock_format, '.');

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_u8_add(&quick_ublock_format, '#');

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ev_data_res);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

#endif

/******************************************************************************
*
* ARRAY flip(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_flip)
{
    S32 xs, ys, ys_half, y, y_swap;

    exec_func_ignore_parms();

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
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
                EV_DATA temp_data;
                temp_data = *ss_array_element_index_borrow(p_ev_data_res, x, y + y_swap);
                *ss_array_element_index_wr(p_ev_data_res, x, y + y_swap) =
                *ss_array_element_index_borrow(p_ev_data_res, x, y);
                *ss_array_element_index_wr(p_ev_data_res, x, y) = temp_data;
            }
        }
    }
}

/******************************************************************************
*
* STRING formula_text(ref)
*
******************************************************************************/

PROC_EXEC_PROTO(c_formula_text)
{
    P_EV_SLOT p_ev_slot;

    exec_func_ignore_parms();

    if(ev_travel(&p_ev_slot, &args[0]->arg.slr) > 0)
    {
        U8Z buffer[4096];
        EV_DOCNO docno = args[0]->arg.slr.docno;
        EV_OPTBLOCK optblock;

        ev_set_options(&optblock, docno);
        optblock.lf = optblock.cr = 0;

        ev_decode_slot(docno, buffer, p_ev_slot, &optblock);

        status_assert(ss_string_make_ustr(p_ev_data_res, buffer));
    }
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
* REAL grand({mean {, sd}})
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
        if((nargs > 1) && (args[1]->arg.fp < 0.0))
            uniform_distribution_seed((unsigned int) -args[1]->arg.fp);
        else
            uniform_distribution_seed((unsigned int) time(NULL));

        uniform_distribution_seeded = TRUE;
    }

    r  = normal_distribution();

    r *= s;
    r += m;

    ev_data_set_real(p_ev_data_res, r);
}

/******************************************************************************
*
* INTEGER return hour number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_hour)
{
    S32 hour_result;
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date.time, &hour, &minute, &second);

    hour_result = hour;

    ev_data_set_integer(p_ev_data_res, hour_result);
}

/******************************************************************************
*
* SLR|other index(array, number, number {, xsize, ysize})
*
* returns SLR if it can
*
******************************************************************************/

PROC_EXEC_PROTO(c_index)
{
    S32 x, y, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    x_size_out = y_size_out = 1;

    array_range_sizes(args[0], &x_size_in, &y_size_in);

    x = args[1]->arg.integer - 1;
    y = args[2]->arg.integer - 1;

    /* get size out parameters */
    if(nargs > 4)
    {
        x_size_out = args[3]->arg.integer;
        x_size_out = MAX(1, x_size_out);
        y_size_out = args[4]->arg.integer;
        y_size_out = MAX(1, y_size_out);
    }

    /* check it's all in range */
    if(x < 0                           ||
       y < 0                           ||
       x + x_size_out - 1 >= x_size_in ||
       y + y_size_out - 1 >= y_size_in)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_INDEX);
        return;
    }

    if(x_size_out == 1 && y_size_out == 1)
    {
        EV_DATA temp_data;
        array_range_index(&temp_data, args[0], x, y, EM_ANY);
        status_assert(ss_data_resource_copy(p_ev_data_res, &temp_data));
        return;
    }

    if(status_ok(ss_array_make(p_ev_data_res, x_size_out, y_size_out)))
    {
        S32 x_in, y_in, x_out, y_out;

        for(y_in = y, y_out = 0; y_out < y_size_out; ++y_in, ++y_out)
        {
            for(x_in = x, x_out = 0; x_out < x_size_out; ++x_in, ++x_out)
            {
                EV_DATA temp_data;
                array_range_index(&temp_data, args[0], x_in, y_in, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, x_out, y_out), &temp_data));
            }
        }
    }
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* BOOLEAN isxxx(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_isblank)
{
    BOOL isblank_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_BLANK:
        isblank_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isblank_result);
}

PROC_EXEC_PROTO(c_iserr)
{
    BOOL iserr_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_ERROR:
        iserr_result = TRUE; /* No #N/A to discriminate on here */
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, iserr_result);
}

PROC_EXEC_PROTO(c_iserror)
{
    BOOL iserror_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_ERROR:
        iserror_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, iserror_result);
}

static void
iseven_isodd_common(
    _InoutRef_  P_EV_DATA arg0,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     BOOL test_iseven)
{
    BOOL is_even = FALSE;

    switch(arg0->did_num)
    {
    case RPN_DAT_REAL:
        {
        F64 f64 = arg0->arg.fp;
        if(f64 < 0.0)
            f64 = -f64;
        f64 = floor(f64); /* NB truncate (Excel) */
        is_even = (f64 == (2.0 * floor(f64 / 2.0))); /* exactly divisible by two? */
        ev_data_set_boolean(p_ev_data_res, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

    case RPN_DAT_BOOL8: /* more useful? */
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        S32 s32 = arg0->arg.integer;
        if(s32 < 0)
            s32 = -s32;
        is_even = (0 == (s32 & 1)); /* bottom bit clear -> number is even */
        ev_data_set_boolean(p_ev_data_res, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

#if 0 /* more pedantic? */
    case RPN_DAT_BOOL8:
        ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXNUMBER);
        break;
#endif

    default: default_unhandled();
        ev_data_set_boolean(p_ev_data_res, FALSE);
        break;
    }
}

PROC_EXEC_PROTO(c_iseven)
{
    exec_func_ignore_parms();

    iseven_isodd_common(args[0], p_ev_data_res, TRUE /*->test_ISEVEN*/);
}

PROC_EXEC_PROTO(c_islogical)
{
    BOOL islogical_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_BOOL8:
        islogical_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, islogical_result);
}

PROC_EXEC_PROTO(c_isna)
{
    BOOL isna_result = FALSE;

    exec_func_ignore_parms();

    /* There is no #N/A error in Fireworkz */
    switch(args[0]->did_num)
    {
    case RPN_DAT_BLANK:
        isna_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isna_result);
}

PROC_EXEC_PROTO(c_isnontext)
{
    BOOL isnontext_result = TRUE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_STRING:
        isnontext_result = FALSE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isnontext_result);
}

PROC_EXEC_PROTO(c_isnumber)
{
    BOOL isnumber_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
    /*case RPN_DAT_BOOL8:*/ /* indeed! that's a LOGICAL for Excel */
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
        isnumber_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isnumber_result);
}

PROC_EXEC_PROTO(c_isodd)
{
    exec_func_ignore_parms();

    iseven_isodd_common(args[0], p_ev_data_res, FALSE /*->test_ISODD*/);
}

PROC_EXEC_PROTO(c_isref)
{
    BOOL isref_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_SLR:
    case RPN_DAT_RANGE:
        isref_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isref_result);
}

PROC_EXEC_PROTO(c_istext)
{
    BOOL istext_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_STRING:
        istext_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, istext_result);
}

#endif

/******************************************************************************
*
* STRING join(string1 {, stringn ...})
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

    if(NULL == (p_u8 = al_ptr_alloc_bytes(U8, len + 1/*NULLCH*/, &status)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    p_ev_data_res->arg.string.data = p_u8;
    p_ev_data_res->arg.string.size = len;
    p_ev_data_res->did_num = RPN_TMP_STRING;

    for(i = 0; i < nargs; ++i)
    {
        U32 arglen = args[i]->arg.string.size;
        memcpy32(p_u8, args[i]->arg.string.data, arglen);
        p_u8 += arglen;
    }

    *p_u8 = NULLCH;
}

/******************************************************************************
*
* STRING left(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_left)
{
    const S32 len = args[0]->arg.string.size;
    S32 n = 1;

    exec_func_ignore_parms();

    if(nargs > 1)
    {
        if((n = args[1]->arg.integer) < 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.data, n));
}

/******************************************************************************
*
* INTEGER length(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_length)
{
    S32 length_result = args[0]->arg.string.size;

    exec_func_ignore_parms();
    
    ev_data_set_integer(p_ev_data_res, length_result);
}

/******************************************************************************
*
* ARRAY listcount(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_listcount)
{
    /* never got this to work in PipeDream */
    exec_func_ignore_parms();
}

/******************************************************************************
*
* STRING lower("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_lower)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.data;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) tolower(*ptr);
    }
}

/*
flatten an x by y array into a 1 by x*y array
*/

static void
ss_data_array_flatten(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(p_ev_data->did_num == RPN_TMP_ARRAY)
    {
        /* this is just so easy given the current array representation */
        p_ev_data->arg.array.y_size *= p_ev_data->arg.array.x_size;
        p_ev_data->arg.array.x_size = 1;
    }
}

/******************************************************************************
*
* NUMBER median(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_median)
{
    STATUS status = STATUS_OK;
    EV_DATA ev_data_temp_array;

    exec_func_ignore_parms();

    ev_data_set_blank(&ev_data_temp_array);

    for(;;)
    {
        S32 xs, ys, ys_half;
        F64 median;

        /* find median by sorting a copy of the source */
        status_assert(ss_data_resource_copy(&ev_data_temp_array, args[0]));
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

        ev_data_set_real(p_ev_data_res, median);
        real_to_integer_try(p_ev_data_res);

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ev_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
* STRING mid(string, start, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mid)
{
    S32 len = args[0]->arg.string.size;
    S32 start = args[1]->arg.integer - 1; /* ensure that this will not go -ve */
    S32 n = args[2]->arg.integer;

    exec_func_ignore_parms();

    if((args[1]->arg.integer <= 0) || (n < 0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    start = MIN(start, len);
    n = MIN(n, len - start);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.data + start, n));
}

/******************************************************************************
*
* INTEGER return minute number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_minute)
{
    S32 minute_result;
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date.time, &hour, &minute, &second);

    minute_result = minute;

    ev_data_set_integer(p_ev_data_res, minute_result);
}

/******************************************************************************
*
* INTEGER return month number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_month)
{
    S32 month_result;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_INVALID == args[0]->arg.date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day);

    month_result = month + 1;

    ev_data_set_integer(p_ev_data_res, month_result);
}

/******************************************************************************
*
* INTEGER monthdays(date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_monthdays)
{
    S32 monthdays_result;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_INVALID == args[0]->arg.date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day);

    monthdays_result = ev_days[month];

    if((month == 1) && LEAP_YEAR(year))
        monthdays_result += 1;

    ev_data_set_integer(p_ev_data_res, monthdays_result);
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
* STRING monthname(n | date)
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
        ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day);
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

    if(status_ok(ss_string_make_ustr(p_ev_data_res, ss_month_names[month])))
        *(p_ev_data_res->arg.string_wr.data) = (U8) toupper(*(p_ev_data_res->arg.string.data));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* VALUE n
*
******************************************************************************/

PROC_EXEC_PROTO(c_n)
{
    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        ev_data_set_integer(p_ev_data_res, ss_dateval_to_serial_number(&args[0]->arg.date.date));
        break;

    case RPN_DAT_STRING:
        /* can't discriminate here between strings and text cells */
    default:
        status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
        break;
    }
}

#endif

/******************************************************************************
*
* DATE now
*
******************************************************************************/

PROC_EXEC_PROTO(c_now)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    ss_local_time_as_ev_date(&p_ev_data_res->arg.date);
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* NUMBER odd(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odd)
{
    BOOL negative = FALSE;
    F64 f64 = args[0]->arg.fp;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negative = TRUE;
    }

    f64 = (2.0 * ceil((f64 + 1.0) / 2.0)) - 1.0;

    /* always round away from zero (Excel) */
    if(negative)
        f64 = -f64;

    ev_data_set_real(p_ev_data_res, f64);
    real_to_integer_try(p_ev_data_res);
}

/******************************************************************************
*
* INTEGER page(ref, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    S32 page_result;
    BOOL xy = (nargs < 2) ? TRUE : (0 != args[1]->arg.integer);
    STATUS status = ev_page_slr(&args[0]->arg.slr, xy);

    exec_func_ignore_parms();

    if(status_fail(status))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    page_result = (S32) status + 1;

    ev_data_set_integer(p_ev_data_res, page_result);
}

/******************************************************************************
*
* INTEGER pages(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pages)
{
    S32 pages_result;
    BOOL xy = (nargs < 1) ? TRUE : (0 != args[0]->arg.integer);
    STATUS status = ev_page_last(p_cur_slr, xy);

    exec_func_ignore_parms();

    if(status_fail(status))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    pages_result = (S32) status;

    ev_data_set_integer(p_ev_data_res, pages_result);
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
* STRING proper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_proper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.data;
        U32 offset_in_word = 0;

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
* REAL rand({n})
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

    ev_data_set_real(p_ev_data_res, uniform_distribution());
}

/******************************************************************************
*
* ARRAY rank(array {, spearflag})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank)
{
    BOOL spearman_correct = FALSE;
    S32 xs, ys;

    exec_func_ignore_parms();

    if(nargs > 1)
        spearman_correct = (0 != args[1]->arg.integer);

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
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);

            if(spearman_correct) /* SKS 12apr95 make suitable for passing to spearman with equal values */
                ev_data_set_real(p_ev_data, position + (equal - 1.0) / 2.0);
            else
                ev_data_set_integer(p_ev_data, position);

            p_ev_data = ss_array_element_index_wr(p_ev_data_res, 1, y);
            ev_data_set_integer(p_ev_data, equal);
            } /*block*/

            ss_data_free_resources(&ev_data);
        }
    }
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
* STRING rept("text", n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rept)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    const S32 len = args[0]->arg.string.size;
    S32 n = args[1]->arg.integer;
    P_U8 p_u8 = buffer;
    S32 x;

    exec_func_ignore_parms();

    if( n > (S32) (elemof32(buffer)-1) / len)
        n = (S32) (elemof32(buffer)-1) / len;

    if(n <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    for(x = 0; x < n; ++x)
    {
        memcpy32(p_u8, args[0]->arg.string.data, len);
        p_u8 += len;
    }

    status_assert(ss_string_make_uchars(p_ev_data_res, buffer, n * len));
}

/******************************************************************************
*
* STRING replace("text", start_n, chars_n, "with_text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_replace)
{
    STATUS status = STATUS_OK;
    const S32 len = args[0]->arg.string.size;
    S32 start_n_chars = args[1]->arg.integer - 1; /* one-based API */
    S32 excise_n_chars = args[2]->arg.integer;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, BUF_EV_MAX_STRING_LEN);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if((start_n_chars < 0) || (excise_n_chars < 0))
        status = EVAL_ERR_ARGRANGE;
    else
    {
        if( start_n_chars > len)
            start_n_chars = len;

        if(0 != start_n_chars)
            status = quick_ublock_uchars_add(&quick_ublock_result, args[0]->arg.string.data, start_n_chars);

        /* NB even if 0 == excise_n_chars; that's just an insert operation */
        if(status_ok(status))
            status = quick_ublock_uchars_add(&quick_ublock_result, args[3]->arg.string.data, args[3]->arg.string.size);

        if(status_ok(status) && ((start_n_chars + excise_n_chars) < len))
        {
            S32 end_n_chars = len - (start_n_chars + excise_n_chars);

            status = quick_ublock_uchars_add(&quick_ublock_result, args[0]->arg.string.data + (start_n_chars + excise_n_chars), end_n_chars);
        }

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ev_data_res, quick_ublock_uptrc(&quick_ublock_result), quick_ublock_bytes(&quick_ublock_result)));
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);

    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* STRING reverse("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_reverse)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 half_len = p_ev_data_res->arg.string_wr.size / 2;
        P_U8 ptr_lo = p_ev_data_res->arg.string_wr.data;
        P_U8 ptr_hi = p_ev_data_res->arg.string_wr.data + p_ev_data_res->arg.string_wr.size;

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
* STRING right(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_right)
{
    const S32 len = args[0]->arg.string.size;
    S32 n = 1;

    exec_func_ignore_parms();

    if(nargs > 1)
    {
        if((n = args[1]->arg.integer) < 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ev_data_res, args[0]->arg.string.data + len - n, n));
}

/******************************************************************************
*
* REAL round(n, decimals)
*
******************************************************************************/

static void
round_common(
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 nargs,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     U32 rpn_did_num)
{
    BOOL negative = FALSE;
    F64 f64, multiplier;

    {
        S32 decimal_places = 2;

        if(nargs > 1)
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

        multiplier = pow(10.0, (F64) decimal_places);
    }

    if(f64 < 0.0)
    {
        negative = TRUE;
        f64 = -f64;
    }

    f64 = f64 * multiplier;

    switch(rpn_did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_FNV_ROUND:
#endif
        f64 = floor(f64 + 0.5); /* got a positive number here */
        break;
    }

    f64 = f64 / multiplier;

    ev_data_set_real(p_ev_data_res, negative ? -f64 : f64);
}

PROC_EXEC_PROTO(c_round)
{
    exec_func_ignore_parms();

    round_common(args, nargs, p_ev_data_res, RPN_FNV_ROUND);

    real_to_integer_try(p_ev_data_res);
}

/******************************************************************************
*
* INTEGER row / row(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_row)
{
    S32 row_result;

    exec_func_ignore_parms();

    if(0 == nargs)
        row_result = (S32) p_cur_slr->row + 1;
    else if(args[0]->did_num == RPN_DAT_SLR)
        row_result = (S32) args[0]->arg.slr.row + 1;
    else if(args[0]->did_num == RPN_DAT_RANGE)
        row_result = (S32) args[0]->arg.range.s.row + 1;
    else
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
    }

    ev_data_set_integer(p_ev_data_res, row_result);
}

/******************************************************************************
*
* INTEGER number of rows in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_rows)
{
    S32 rows_result;

    exec_func_ignore_parms();

    if(0 == nargs)
        rows_result = (S32) ev_numrow(p_cur_slr);
    else
    {
        S32 xs, ys;
        array_range_sizes(args[0], &xs, &ys);
        rows_result = ys;
    }

    ev_data_set_integer(p_ev_data_res, rows_result);
}

/******************************************************************************
*
* INTEGER return second number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_second)
{
    S32 second_result;
    S32 hour, minute, second;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.date.time, &hour, &minute, &second);

    second_result = second;

    ev_data_set_integer(p_ev_data_res, second_result);
}

/******************************************************************************
*
* define and set value of a name
*
* VALUE set_name("name", value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_set_name)
{
    S32 res;
    EV_NAMEID name_key, name_num;

    exec_func_ignore_parms();

    if((res = name_make(&name_key, p_cur_slr->docno, args[0]->arg.string.data, args[1])) < 0)
    {
        ev_data_set_error(p_ev_data_res, res);
        return;
    }

    name_num = name_def_find(name_key);

    status_assert(ss_data_resource_copy(p_ev_data_res, &name_ptr(name_num)->def_data));
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
* ARRAY sort(array {, col})
*
******************************************************************************/

PROC_EXEC_PROTO(c_sort)
{
    S32 x_coord = 0;
    STATUS status;

    exec_func_ignore_parms();

    if(nargs > 1)
        x_coord = args[1]->arg.integer - 1; /* array_sort() does range checking */

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
    data_ensure_constant(p_ev_data_res);

    if(status_fail(status = array_sort(p_ev_data_res, x_coord)))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
* REAL spearman(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_spearman)
{
    F64 spearman_result;
    S32 xs_0, ys_0, xs_1, ys_1, y, ys;
    F64 sum_d_squared = 0.0;
    S32 n_counted = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    ys = MAX(ys_0, ys_1);

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

    if(0 == n_counted)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NO_VALID_DATA);
        return;
    }

    spearman_result = (1.0 - (6.0 * sum_d_squared) / ((F64) n_counted * ((F64) n_counted * (F64) n_counted - 1.0)));

    ev_data_set_real(p_ev_data_res, spearman_result);
}

/******************************************************************************
*
* STRING string(n {, x})
*
******************************************************************************/

PROC_EXEC_PROTO(c_string)
{
    S32 decimal_places = 2;
    F64 rounded_value;
    U8Z buffer[BUF_EV_MAX_STRING_LEN];

    exec_func_ignore_parms();

    if(nargs > 1)
    {
        decimal_places = MIN(127, args[1]->arg.integer); /* can be large for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    round_common(args, nargs, p_ev_data_res, RPN_FNV_ROUND); /* ROUND: uses nargs, args[0] and {args[1]}, yields RPN_DAT_REAL (or an integer type) */

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
        rounded_value = p_ev_data_res->arg.fp;
    else
        rounded_value = (F64) p_ev_data_res->arg.integer; /* take care now we can get integer types back */

    (void) xsnprintf(buffer, elemof32(buffer), "%.*f", (int) decimal_places, rounded_value);

    status_assert(ss_string_make_ustr(p_ev_data_res, buffer));
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

PROC_EXEC_PROTO(c_term)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp ;
    F64 fv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    ev_data_set_real(p_ev_data_res, term_nper_common(payment, interest, fv));
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

    return(numform(p_quick_ublock, P_QUICK_UBLOCK_NONE, p_ev_data, &numform_parms));
}

/******************************************************************************
*
* STRING text(n, numform_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_text)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, args[1]->arg.string.data, args[1]->arg.string.size)))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        status_assert(ss_string_make_ustr(p_ev_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* DATE time(hour, minute, second)
*
******************************************************************************/

PROC_EXEC_PROTO(c_time)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.date.date = EV_DATE_INVALID;

    if(ss_hms_to_timeval(&p_ev_data_res->arg.date.time,
                         args[0]->arg.integer,
                         args[1]->arg.integer,
                         args[2]->arg.integer) < 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    ss_date_normalise(&p_ev_data_res->arg.date);
}

/******************************************************************************
*
* DATE timevalue(text) - convert text to time value
*
******************************************************************************/

PROC_EXEC_PROTO(c_timevalue)
{
    U8Z buffer[256];
    U32 len;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, sizeof32(buffer)-1);
    memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || (EV_DATE_INVALID != p_ev_data_res->arg.date.date))
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BADTIME);
}

/******************************************************************************
*
* DATE return today's date (without time component)
*
******************************************************************************/

PROC_EXEC_PROTO(c_today)
{
    exec_func_ignore_parms();

    c_now(args, nargs, p_ev_data_res, p_cur_slr);

    if(RPN_DAT_DATE == p_ev_data_res->did_num)
        p_ev_data_res->arg.date.time = EV_TIME_INVALID;
}

/******************************************************************************
*
* STRING trim("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_trim)
{
    U8Z buffer[BUF_EV_MAX_STRING_LEN];
    BOOL gap;
    P_U8 s_ptr, e_ptr, i_ptr, o_ptr;

    exec_func_ignore_parms();

    memcpy32(buffer, args[0]->arg.string.data, args[0]->arg.string.size);

    s_ptr = buffer;
    e_ptr = buffer + args[0]->arg.string.size;

    /* skip leading spaces */
    while(*s_ptr == ' ')
        ++s_ptr;

    /* skip trailing spaces */
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
                gap = TRUE;
        }
        else
            gap = FALSE;

        *o_ptr++ = *i_ptr;
    }

    status_assert(ss_string_make_ustr(p_ev_data_res, s_ptr));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* BOOLEAN true
*
******************************************************************************/

PROC_EXEC_PROTO(c_true)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, TRUE);
}

#endif

/******************************************************************************
*
* STRING return type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_type)
{
    EV_TYPE type;
    PC_USTR ustr_type;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
#endif
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

    case RPN_DAT_ERROR:
        type = EM_ERR;
        break;
    }

    ustr_type = type_from_flags(type);

    status_assert(ss_string_make_ustr(p_ev_data_res, ustr_type));
}

/******************************************************************************
*
* STRING upper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_upper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ev_data_res, args[0])))
    {
        U32 x;
        const U32 len = p_ev_data_res->arg.string_wr.size;
        P_U8 ptr = p_ev_data_res->arg.string_wr.data;

        for(x = 0; x < len; ++x, ++ptr)
            *ptr = (U8) toupper(*ptr);
    }
}

/******************************************************************************
*
* NUMBER value("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_value)
{
    U8Z buffer[256];
    U32 len;
    P_U8 ptr;

    exec_func_ignore_parms();

    len = MIN(args[0]->arg.string.size, sizeof32(buffer)-1);
    memcpy32(buffer, args[0]->arg.string.data, len);
    buffer[len] = NULLCH;

    ev_data_set_real(p_ev_data_res, strtod(buffer, &ptr));

    if(ptr == buffer)
        ev_data_set_integer(p_ev_data_res, 0); /* SKS 08sep97 now behaves as documented */
    else
        real_to_integer_try(p_ev_data_res);
}

/******************************************************************************
*
* REAL return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, strtod(applicationversion, NULL));
}

/******************************************************************************
*
* INTEGER return the day of the week for a date
*
******************************************************************************/

PROC_EXEC_PROTO(c_weekday)
{
    S32 weekday;

    exec_func_ignore_parms();

    if(EV_DATE_INVALID == args[0]->arg.date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    weekday = (args[0]->arg.date.date + 1) % 7;

    ev_data_set_integer(p_ev_data_res, weekday + 1);
}

/******************************************************************************
*
* INTEGER return the week number for a date
*
******************************************************************************/

#if RISCOS

typedef struct _FIVEBYTE
{
    U8 utc[5];
}
FIVEBYTE, * P_FIVEBYTE;

_Check_return_
static STATUS
fivebytetime_from_date(
    _OutRef_    P_FIVEBYTE p_fivebyte,
    _InRef_     PC_EV_DATE_DATE p_ev_date_date)
{
    S32 year, month, day;
    riscos_time_ordinals time_ordinals;
    _kernel_swi_regs rs;

    ss_dateval_to_ymd(p_ev_date_date, &year, &month, &day);

    zero_struct(time_ordinals);
    time_ordinals.year = year + 1;
    time_ordinals.month = month + 1;
    time_ordinals.day = day + 1;

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) &p_fivebyte->utc[0];
    rs.r[2] = (int) &time_ordinals;
    if(NULL != _kernel_swi(Territory_ConvertOrdinalsToTime, &rs, &rs))
        zero_struct_ptr(p_fivebyte);

    return(STATUS_OK);
}

#endif

PROC_EXEC_PROTO(c_weeknumber)
{
    S32 weeknumber_result;
    STATUS status;

    exec_func_ignore_parms();

    {
#if RISCOS
    FIVEBYTE fivebyte;

    if(status_ok(status = fivebytetime_from_date(&fivebyte, &args[0]->arg.date.date)))
    {
        U8Z buffer[32];
        _kernel_swi_regs rs;

        rs.r[0] = -1; /* use current territory */
        rs.r[1] = (int) &fivebyte.utc[0];
        rs.r[2] = (int) buffer;
        rs.r[3] = sizeof32(buffer);
        rs.r[4] = (int) "%WK";
        if(NULL != _kernel_swi(Territory_ConvertDateAndTime, &rs, &rs))
            weeknumber_result = 0; /* a result of zero -> info not available */
        else
            weeknumber_result = (S32) fast_strtoul(buffer, NULL);

        ev_data_set_integer(p_ev_data_res, weeknumber_result);
    }
    else
        ev_data_set_error(p_ev_data_res, status); /* bad/missing date */
#else
    S32 year, month, day;

    if(status_ok(status = ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day)))
    {
        U8Z buffer[32];
        struct tm tm;

        zero_struct(tm);
        tm.tm_year = (int) (year + 1 - 1900);
        tm.tm_mon = (int) month;
        tm.tm_mday = (int) (day + 1);

        /* actually needs wday and yday setting up! */
        (void) mktime(&tm); /* normalise */

        strftime(buffer, elemof32(buffer), "%W", &tm);

        weeknumber_result = fast_strtoul(buffer, NULL);

        weeknumber_result += 1;

        ev_data_set_integer(p_ev_data_res, weeknumber_result);
    }
    else
        ev_data_set_error(p_ev_data_res, status); /* bad/missing date */
#endif
    } /*block*/
}

/******************************************************************************
*
* INTEGER return year number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_year)
{
    S32 year_result;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_INVALID == args[0]->arg.date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    ss_dateval_to_ymd(&args[0]->arg.date.date, &year, &month, &day);

    year_result = year + 1;

    ev_data_set_integer(p_ev_data_res, year_result);
}

/* primitive number/trig functions moved here as per Fireworkz split*/

/******************************************************************************
*
* NUMBER abs(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_abs)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
        ev_data_set_real(p_ev_data_res, fabs(args[0]->arg.fp));
    else
        ev_data_set_integer(p_ev_data_res, abs(args[0]->arg.integer));
}

/******************************************************************************
*
* REAL acos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acos)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, acos(number));

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* REAL asin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asin)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, asin(number));

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* REAL atan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, atan(number));

    /* no error cases */
}

/******************************************************************************
*
* REAL cos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cos)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, cos(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* REAL deg(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deg)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, number * (180.0/PI));
}

/******************************************************************************
*
* REAL exp(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_exp)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, exp(number));

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_res->arg.fp == HUGE_VAL)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* NUMBER int(number)
*
* NB truncates towards zero
*
******************************************************************************/

PROC_EXEC_PROTO(c_int)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
    {
        /* SKS as per Fireworkz, try pre-rounding an ickle bit so INT((0.06-0.04)/0.01) is 2 not 1 */
        int exponent;
        F64 mantissa = frexp(args[0]->arg.fp, &exponent);
        F64 f64;
        F64 result;

        if(mantissa > (+1.0 - 1E-14))
            mantissa = +1.0;
        else if(mantissa < (-1.0 + 1E-14))
            mantissa = -1.0;
        f64 = ldexp(mantissa, exponent);

        (void) modf(f64, &result);

        ev_data_set_real(p_ev_data_res, result);
        real_to_integer_try(p_ev_data_res);
    }
    else
    {   /*NOP*/
        *p_ev_data_res = *(args[0]);
    }
}

/******************************************************************************
*
* REAL ln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ln)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, log(number));

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* REAL log(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, log10(number));

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER mod(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1], FALSE))
    {
    case TWO_INTS:
        {
        S32 s32_a = args[0]->arg.integer;
        S32 s32_b = args[1]->arg.integer;
        S32 s32_mod_result;

        /* SKS after 4.11 03feb92 - gave FP error not zero if trap taken */
        if(s32_b == 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
            return;
        }

        s32_mod_result = (s32_a % s32_b);

        ev_data_set_integer(p_ev_data_res, s32_mod_result);
        break;
        }

    case TWO_REALS:
        {
        F64 f64_a = args[0]->arg.fp;
        F64 f64_b = args[1]->arg.fp;
        F64 f64_mod_result;

        errno = 0;

        f64_mod_result = fmod(f64_a, f64_b);

        ev_data_set_real(p_ev_data_res, f64_mod_result);

        /* would have divided by zero? */
        if(errno /* == EDOM */)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);

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
* REAL pi
*
******************************************************************************/

PROC_EXEC_PROTO(c_pi)
{
    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, PI);
}

/******************************************************************************
*
* REAL rad(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rad)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, number * (PI/180.0));
}

/******************************************************************************
*
* REAL sin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sin)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, sin(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* REAL sqr(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sqr)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, sqrt(number));

    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NEG_ROOT);
}

/******************************************************************************
*
* REAL tan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tan)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, tan(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ACCURACY_LOST);
}

/* end of ev_func.c */
