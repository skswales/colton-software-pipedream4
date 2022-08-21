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

/******************************************************************************
*
* Lookup & refererence functions
*
******************************************************************************/

/******************************************************************************
*
* THING choose(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_choose)
{
    LOOKUP_BLOCK lkb;
    S32 res, i;

    exec_func_ignore_parms();

    if((args[0]->arg.integer < 1) || (args[0]->arg.integer >= n_args))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    lookup_block_init(&lkb,
                      NULL,
                      LOOKUP_CHOOSE,
                      args[0]->arg.integer,
                      0);

    for(i = 1, res = 0; i < n_args && !res; ++i)
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

/******************************************************************************
*
* INTEGER col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    S32 col_result;

    exec_func_ignore_parms();

    if(0 == n_args)
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

    if(0 == n_args)
        cols_result = (S32) ev_numcol(p_cur_slr);
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        cols_result = (S32) x_size;
    }

    ev_data_set_integer(p_ev_data_res, cols_result);
}

/******************************************************************************
*
* SLR|other index(array, number, number {, x_size, y_size})
*
* returns SLR if it can
*
******************************************************************************/

PROC_EXEC_PROTO(c_index)
{
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    x_size_out = y_size_out = 1;

    array_range_sizes(args[0], &x_size_in, &y_size_in);

    /* NB Fireworkz and PipeDream INDEX() has x, y args */
    ix = args[1]->arg.integer - 1;
    iy = args[2]->arg.integer - 1;

    /* get size out parameters */
    if(n_args > 4)
    {
        x_size_out = args[3]->arg.integer;
        x_size_out = MAX(1, x_size_out);
        y_size_out = args[4]->arg.integer;
        y_size_out = MAX(1, y_size_out);
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_INDEX);
        return;
    }

    if((x_size_out == 1) && (y_size_out == 1))
    {
        EV_DATA temp_data;
        array_range_index(&temp_data, args[0], ix, iy, EM_ANY);
        status_assert(ss_data_resource_copy(p_ev_data_res, &temp_data));
        return;
    }

    if(status_ok(ss_array_make(p_ev_data_res, x_size_out, y_size_out)))
    {
        S32 ix_in, iy_in, ix_out, iy_out;

        for(iy_in = iy, iy_out = 0; iy_out < y_size_out; ++iy_in, ++iy_out)
        {
            for(ix_in = ix, ix_out = 0; ix_out < x_size_out; ++ix_in, ++ix_out)
            {
                EV_DATA temp_data;
                array_range_index(&temp_data, args[0], ix_in, iy_in, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, ix_out, iy_out), &temp_data));
            }
        }
    }
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

    if(0 == n_args)
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

    if(0 == n_args)
        rows_result = (S32) ev_numrow(p_cur_slr);
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        rows_result = (S32) y_size;
    }

    ev_data_set_integer(p_ev_data_res, rows_result);
}

/******************************************************************************
*
* Miscellaneous functions
*
******************************************************************************/

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
        memcpy32(p_u8, args[0]->arg.string.uchars, args[0]->arg.string.size);
        status_consume(command_array_handle_execute(ev_docno, h_commands)); /* error already reported */
        al_array_dispose(&h_commands);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

#endif

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

    ev_data_set_real_ti(p_ev_data_res, even_result);
}

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
* ARRAY flip(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_flip)
{
    S32 x_size, y_size;
    S32 y_half;
    S32 y_swap;
    S32 ix, iy;

    exec_func_ignore_parms();

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
    data_ensure_constant(p_ev_data_res);

    if(p_ev_data_res->did_num == RPN_TMP_ARRAY)
    {
        array_range_sizes(p_ev_data_res, &x_size, &y_size);
        y_half = y_size / 2;
        y_swap = y_size - 1;
        for(iy = 0; iy < y_half; ++iy, y_swap -= 2)
        {
            for(ix = 0; ix < x_size; ++ix)
            {
                EV_DATA temp_data;
                temp_data = *ss_array_element_index_borrow(p_ev_data_res, ix, iy + y_swap);
                *ss_array_element_index_wr(p_ev_data_res, ix, iy + y_swap) =
                *ss_array_element_index_borrow(p_ev_data_res, ix, iy);
                *ss_array_element_index_wr(p_ev_data_res, ix, iy) = temp_data;
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

    ev_data_set_real_ti(p_ev_data_res, f64);
}

/******************************************************************************
*
* INTEGER page(ref, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    S32 page_result;
    BOOL xy = (n_args < 2) ? TRUE : (0 != args[1]->arg.integer);
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
    BOOL xy = (n_args < 1) ? TRUE : (0 != args[0]->arg.integer);
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

    if((res = name_make(&name_key, p_cur_slr->docno, args[0]->arg.string.uchars, args[1])) < 0)
    {
        ev_data_set_error(p_ev_data_res, res);
        return;
    }

    name_num = name_def_find(name_key);
    assert(name_num >= 0);

    status_assert(ss_data_resource_copy(p_ev_data_res, &name_ptr_must(name_num)->def_data));
}

/******************************************************************************
*
* ARRAY sort(array {, col})
*
******************************************************************************/

PROC_EXEC_PROTO(c_sort)
{
    U32 x_index = 0;
    STATUS status;

    exec_func_ignore_parms();

    if(n_args > 1)
        x_index = (U32) args[1]->arg.integer; /* array_sort() does range checking */ /* NB NOT -1 - SORT() is zero-based */

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
    data_ensure_constant(p_ev_data_res);

    if(RPN_DAT_ERROR == p_ev_data_res->did_num)
        return;

    if(status_fail(status = array_sort(p_ev_data_res, x_index)))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
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
* REAL return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, strtod(applicationversion, NULL));
}

/* end of ev_func.c */
