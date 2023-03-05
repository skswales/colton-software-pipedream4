/* ev_func.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Less common (was split) semantic routines for evaluator */

/* JAD September 1994 split */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "cmodules/mathxtr3.h"

#include "cmodules/numform.h"

#include "version_x.h"

#include "cs-kernel.h" /*C:*/

#include "swis.h" /*C:*/

/******************************************************************************
*
* Lookup and reference functions
*
******************************************************************************/

#if 0 /* just for diff minimization */

/******************************************************************************
*
* STRING address(row:integer, col:integer {, abs:integer {, a1:Logical {, ext_doc:string}}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_address)
{
    STATUS status = STATUS_OK;
    const ROW row = ss_data_get_integer(args[0]) - 1;
    const COL col = ss_data_get_integer(args[1]) - 1;
    BOOL abs_row = TRUE;
    BOOL abs_col = TRUE;
    BOOL a1 = TRUE;
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    exec_func_ignore_parms();

    if(n_args > 2)
    {
        switch(ss_data_get_integer(args[2]))
        {
        default:
            break;

        case 2:
            abs_col = FALSE;
            break;

        case 3:
            abs_row = FALSE;
            break;

        case 4:
            abs_row = FALSE;
            abs_col = FALSE;
            break;
        }
    }

    if(n_args > 3)
        a1 = ss_data_get_logical(args[3]);

    if(n_args > 4)
    {   /* prepend external reference */
        status = quick_ublock_uchars_add(&quick_ublock, ss_data_get_string(args[4]), ss_data_get_string_size(args[4]));
    }

    if(a1)
    {   /* A1 */
        if(status_ok(status))
            if(abs_col)
                status = quick_ublock_a7char_add(&quick_ublock, CH_DOLLAR_SIGN);

        if(status_ok(status))
        {
            U32 len = xtos_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), col, TRUE /*upper_case_slr*/);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }

        if(status_ok(status))
            if(abs_row)
                status = quick_ublock_a7char_add(&quick_ublock, CH_DOLLAR_SIGN);

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), S32_FMT, (S32) row + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }
    }
    else
    {   /* R1C1 */
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, 'R');

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), abs_row ? S32_FMT : "[" S32_FMT "]", (S32) row + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }

        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, 'C');

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), abs_col ? S32_FMT : "[" S32_FMT "]", (S32) col + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }
    }

    if(status_ok(status))
        status = ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    exec_func_status_return(p_ss_data_res, status);
}

#endif

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

    if(ss_data_get_integer(args[0]) < 1) /* NB Fireworkz' CHOOSE is different */
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    lookup_block_init(&lkb,
                      NULL,
                      LOOKUP_CHOOSE,
                      ss_data_get_integer(args[0]),
                      0);

    for(i = 1, res = 0; i < n_args && !res; ++i)
    {
        do
            res = lookup_array_range_proc(&lkb, args[i]);
        while(res < 0);
    }

    if(0 == res)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    status_assert(ss_data_resource_copy(p_ss_data_res, &lkb.result_data));
}

/******************************************************************************
*
* INTEGER col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    EV_COL col = p_cur_slr->col;
    S32 col_result;

    exec_func_ignore_parms();

    if(0 != n_args)
    {
        if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
            col = (args[0]->arg.slr).col;
        else if(DATA_ID_RANGE == ss_data_get_data_id(args[0]))
            col = (args[0]->arg.range.s).col;
        else
            exec_func_status_return(p_ss_data_res, EVAL_ERR_UNEXARRAY);
    }

    col_result = (S32) col + 1;

    ss_data_set_integer_fn(p_ss_data_res, col_result);
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
        cols_result = (S32) ev_numcol(ev_slr_docno(p_cur_slr));
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        cols_result = (S32) x_size;
    }

    ss_data_set_integer_fn(p_ss_data_res, cols_result);
}

/******************************************************************************
*
* SLR|other index(array, x:number, y:number {, x_size:number, y_size:number})
*
* returns SLR if it can
*
******************************************************************************/

static void
c_index_common(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA array_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     S32 x_size_out,
    _InVal_     S32 y_size_out)
{
    if((x_size_out == 1) && (y_size_out == 1))
    {
        SS_DATA temp_data;
        (void) array_range_index(&temp_data, array_data, ix, iy, EM_ANY);
        status_assert(ss_data_resource_copy(p_ss_data_out, &temp_data));
        return;
    }

    if(status_ok(ss_array_make(p_ss_data_out, x_size_out, y_size_out)))
    {
        S32 ix_in, iy_in, ix_out, iy_out;

        for(iy_in = iy, iy_out = 0; iy_out < y_size_out; ++iy_in, ++iy_out)
        {
            for(ix_in = ix, ix_out = 0; ix_out < x_size_out; ++ix_in, ++ix_out)
            {
                SS_DATA temp_data;
                (void) array_range_index(&temp_data, array_data, ix_in, iy_in, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ss_data_out, ix_out, iy_out), &temp_data));
            }
        }
    }
}

#if 1 /* new c_index from Fireworkz */
PROC_EXEC_PROTO(c_index)
{
    const PC_SS_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB Fireworkz and PipeDream INDEX() has x, y args */
    if(0 == ss_data_get_integer(args[1]))
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = ss_data_get_integer(args[1]) - 1;
        x_size_out = 1;
    }

    if(0 == ss_data_get_integer(args[2]))
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = ss_data_get_integer(args[2]) - 1;
        y_size_out = 1;
    }

    /* get size out parameters */
    if(n_args > 4)
    {
        if(0 == ss_data_get_integer(args[3]))
        {   /* zero x_size -> all of row starting from column index x */
            x_size_out = x_size_in - ix;
        }
        else
            x_size_out = MAX(1, ss_data_get_integer(args[3]));

        if(0 == ss_data_get_integer(args[4]))
        {   /* zero y_size -> all of column starting from row index y */
            y_size_out = y_size_in - iy;
        }
        else
            y_size_out = MAX(1, ss_data_get_integer(args[4]));
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_INDEX);
    }

    c_index_common(p_ss_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

#else
PROC_EXEC_PROTO(c_index)
{
    const PC_SS_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    x_size_out = y_size_out = 1;

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB Fireworkz and PipeDream INDEX() has x, y args */
    ix = ss_data_get_integer(args[1]) - 1;
    iy = ss_data_get_integer(args[2]) - 1;

    /* get size out parameters */
    if(n_args > 4)
    {
        x_size_out = ss_data_get_integer(args[3]);
        x_size_out = MAX(1, x_size_out);
        y_size_out = ss_data_get_integer(args[4]);
        y_size_out = MAX(1, y_size_out);
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_INDEX);
    }

    c_index_common(p_ss_data_res, array_data, ix, iy, x_size_out, y_size_out);
}
#endif

#if 0 /* just for diff minimization */

PROC_EXEC_PROTO(c_odf_index)
{
    const PC_SS_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB OpenDocument INDEX() has row, column args */
    if(0 == ss_data_get_integer(args[1]))
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = ss_data_get_integer(args[1]) - 1;
        y_size_out = 1;
    }

    if(0 == ss_data_get_integer(args[2]))
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = ss_data_get_integer(args[2]) - 1;
        x_size_out = 1;
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_INDEX);
    }

    c_index_common(p_ss_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

#endif

/******************************************************************************
*
* INTEGER row / row(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_row)
{
    EV_ROW row = p_cur_slr->row;
    S32 row_result;

    exec_func_ignore_parms();

    if(0 != n_args)
    {
        if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
            row = (args[0]->arg.slr).row;
        else if(DATA_ID_RANGE == ss_data_get_data_id(args[0]))
            row = (args[0]->arg.range.s).row;
        else
            exec_func_status_return(p_ss_data_res, EVAL_ERR_UNEXARRAY);
    }

    row_result = (S32) row + 1;

    ss_data_set_integer_fn(p_ss_data_res, row_result);
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
        rows_result = (S32) ev_numrow(ev_slr_docno(p_cur_slr));
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        rows_result = (S32) y_size;
    }

    ss_data_set_integer_fn(p_ss_data_res, rows_result);
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
    while(NULL != (p_stack_entry_i = stack_back_search(PtrDiffElemU32(p_stack_entry_i, stack_base), EXECUTING_MACRO)))
        p_stack_entry = p_stack_entry_i;

    if(NULL != (p_u8 = al_array_alloc_U8(&h_commands, ss_data_get_string_size(args[0]), &array_init_block_u8, &status)))
    {
        EV_DOCNO ev_docno = p_stack_entry ? ev_slr_docno(&p_stack_entry->slr) : ev_slr_docno(p_cur_slr);
        memcpy32(p_u8, ss_data_get_string(args[0]), ss_data_get_string_size(args[0]));
        status_consume(command_array_handle_execute((DOCNO) ev_docno, h_commands)); /* error already reported */
        al_array_dispose(&h_commands);
    }

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* SLR current_cell
*
******************************************************************************/

PROC_EXEC_PROTO(c_current_cell)
{
    exec_func_ignore_parms();

    ev_current_cell(&p_ss_data_res->arg.slr);
    ss_data_set_data_id(p_ss_data_res, DATA_ID_SLR);
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

    if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
        ev_slr_deref(p_ss_data_res, &args[0]->arg.slr, TRUE);
    else
        status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* SLR double_click
*
******************************************************************************/

PROC_EXEC_PROTO(c_doubleclick)
{
    exec_func_ignore_parms();

    ev_double_click(&p_ss_data_res->arg.slr, p_cur_slr);

    if(DOCNO_NONE == ev_slr_docno(&p_ss_data_res->arg.slr))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NA);

    ss_data_set_data_id(p_ss_data_res, DATA_ID_SLR);
}

/******************************************************************************
*
* NUMBER even(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_even)
{
    BOOL negate_result = FALSE;
    F64 f64 = ss_data_get_real(args[0]);
    F64 even_result;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = TRUE;
    }

    even_result = 2.0 * ceil(f64 * 0.5);

    /* always round away from zero (Excel) */
    if(negate_result)
        even_result = -even_result;

    ss_data_set_real_try_integer(p_ss_data_res, even_result);
}

/******************************************************************************
*
* LOGICAL false
*
******************************************************************************/

PROC_EXEC_PROTO(c_false)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, FALSE);
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
    S32 ix, iy;
    S32 y_swap;

    exec_func_ignore_parms();

    status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
    data_ensure_constant(p_ss_data_res);

    if(ss_data_is_error(p_ss_data_res))
        return;

    if(RPN_TMP_ARRAY == ss_data_get_data_id(p_ss_data_res))
    {
        array_range_sizes(p_ss_data_res, &x_size, &y_size);
        y_half = y_size / 2;
        y_swap = y_size - 1;
        for(iy = 0; iy < y_half; ++iy, y_swap -= 2)
        {
            for(ix = 0; ix < x_size; ++ix)
            {
                SS_DATA temp_data;
                temp_data = *ss_array_element_index_borrow(p_ss_data_res, ix, iy + y_swap);
                *ss_array_element_index_wr(p_ss_data_res, ix, iy + y_swap) =
                *ss_array_element_index_borrow(p_ss_data_res, ix, iy);
                *ss_array_element_index_wr(p_ss_data_res, ix, iy) = temp_data;
            }
        }
    }
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* LOGICAL isxxx(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_isblank)
{
    BOOL isblank_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_BLANK:
        isblank_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isblank_result);
}

PROC_EXEC_PROTO(c_iserr)
{
    BOOL iserr_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_ERROR:
        iserr_result = (EVAL_ERR_ODF_NA != args[0]->arg.ss_error.status);
        iserr_result = TRUE; /* No #N/A to discriminate on in PipeDream */
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, iserr_result);
}

PROC_EXEC_PROTO(c_iserror)
{
    BOOL iserror_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_ERROR:
        iserror_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, iserror_result);
}

static void
iseven_isodd_calc(
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA arg0,
    _InVal_     BOOL test_iseven)
{
    BOOL is_even = FALSE;

    switch(ss_data_get_data_id(arg0))
    {
    case DATA_ID_REAL:
        {
        F64 f64 = ss_data_get_real(arg0);
        if(f64 < 0.0)
            f64 = -f64;
        f64 = floor(f64); /* NB truncate (Excel) */
        is_even = (f64 == (2.0 * floor(f64 * 0.5))); /* exactly divisible by two? */
        ss_data_set_logical(p_ss_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

    case DATA_ID_LOGICAL: /* more useful? */
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        {
        S32 s32 = ss_data_get_integer(arg0);
        if(s32 < 0)
            s32 = -s32;
        is_even = (0 == (s32 & 1)); /* bottom bit clear -> number is even */
        ss_data_set_logical(p_ss_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

#if 0 /* more pedantic? */
    case DATA_ID_LOGICAL:
        ss_data_set_error(p_ss_data_out, EVAL_ERR_UNEXNUMBER);
        break;
#endif

    default: default_unhandled();
        ss_data_set_logical(p_ss_data_out, FALSE);
        break;
    }
}

PROC_EXEC_PROTO(c_iseven)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ss_data_res, args[0], TRUE /*->test_ISEVEN*/);
}

PROC_EXEC_PROTO(c_islogical)
{
    BOOL islogical_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_LOGICAL:
        islogical_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, islogical_result);
}

PROC_EXEC_PROTO(c_isna)
{
    BOOL isna_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
#if 0 /* There is no #N/A error in PipeDream */
    case DATA_ID_ERROR:
        isna_result = (EVAL_ERR_ODF_NA == args[0]->arg.ss_error.status);
        break;
#endif

    case DATA_ID_BLANK:
        isna_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isna_result);
}

PROC_EXEC_PROTO(c_isnontext)
{
    BOOL isnontext_result = TRUE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_STRING:
        isnontext_result = FALSE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isnontext_result);
}

PROC_EXEC_PROTO(c_isnumber)
{
    BOOL isnumber_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_REAL:
    /*case DATA_ID_LOGICAL:*/ /* indeed! that's a LOGICAL for Excel */
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
        isnumber_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isnumber_result);
}

PROC_EXEC_PROTO(c_isodd)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ss_data_res, args[0], FALSE /*->test_ISODD*/);
}

PROC_EXEC_PROTO(c_isref)
{
    BOOL isref_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_SLR:
    case DATA_ID_RANGE:
        isref_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isref_result);
}

PROC_EXEC_PROTO(c_istext)
{
    BOOL istext_result = FALSE;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_STRING:
        istext_result = TRUE;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, istext_result);
}

#endif

#if 0 /* just for diff minimization */

/******************************************************************************
*
* ERROR na
*
******************************************************************************/

PROC_EXEC_PROTO(c_na)
{
    exec_func_ignore_parms();

    exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NA);
}

/******************************************************************************
*
* NUMBER odd(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odd)
{
    BOOL negate_result = FALSE;
    F64 f64 = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = TRUE;
    }

    f64 = (2.0 * ceil((f64 + 1.0) * 0.5)) - 1.0;

    /* always round away from zero (Excel) */
    if(negate_result)
        f64 = -f64;

    ss_data_set_real_try_integer(p_ss_data_res, f64);
}

/******************************************************************************
*
* INTEGER page(ref, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    S32 page_result;
    BOOL xy = (n_args < 2) ? TRUE : (0 != ss_data_get_integer(args[1]));
    STATUS status = ev_page_slr(&args[0]->arg.slr, xy);

    exec_func_ignore_parms();

    exec_func_status_return(p_ss_data_res, status);

    page_result = (S32) status + 1;

    ss_data_set_integer_fn(p_ss_data_res, page_result);
}

/******************************************************************************
*
* INTEGER pages(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pages)
{
    S32 pages_result;
    BOOL xy = (n_args < 1) ? TRUE : (0 != ss_data_get_integer(args[0]));
    STATUS status = ev_page_last(ev_slr_docno(p_cur_slr), xy);

    exec_func_ignore_parms();

    exec_func_status_return(p_ss_data_res, status);

    pages_result = (S32) status;

    ss_data_set_integer_fn(p_ss_data_res, pages_result);
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

    res = name_make(&name_key, ev_slr_docno(p_cur_slr), ss_data_get_string(args[0]), args[1]);
    exec_func_status_return(p_ss_data_res, res);

    name_num = name_def_find(name_key);
    assert(name_num >= 0);

    status_assert(ss_data_resource_copy(p_ss_data_res, &name_ptr_must(name_num)->def_data));
}

/******************************************************************************
*
* ARRAY sort(array {, column_index})
*
******************************************************************************/

PROC_EXEC_PROTO(c_sort)
{
    U32 x_index = 0;
    STATUS status;

    exec_func_ignore_parms();

    if(n_args > 1)
        x_index = (U32) ss_data_get_integer(args[1]); /* array_sort() does range checking */ /* NB NOT -1 - SORT() is zero-based */

    status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
    data_ensure_constant(p_ss_data_res);

    if(ss_data_is_error(p_ss_data_res))
        return;

    if(status_fail(status = array_sort(p_ss_data_res, x_index)))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* LOGICAL true
*
******************************************************************************/

PROC_EXEC_PROTO(c_true)
{
    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, TRUE);
}

#endif

/******************************************************************************
*
* STRING return type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_type)
{
    EV_TYPE type_flags;
    PC_A7STR a7str_type;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    default: default_unhandled();
#if CHECKING
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
#endif
        type_flags = EM_REA;
        break;

    case DATA_ID_DATE:
        type_flags = EM_DAT;
        break;

    case DATA_ID_BLANK:
        type_flags = EM_BLK;
        break;

    case DATA_ID_ERROR:
        type_flags = EM_ERR;
        break;

    case DATA_ID_SLR:
        type_flags = EM_SLR;
        break;

    case DATA_ID_RANGE:
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        type_flags = EM_ARY;
        break;

    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        type_flags = EM_STR;
        break;
    }

    a7str_type = type_name_from_type_flags(type_flags);
    PTR_ASSERT(a7str_type);

    status_assert(ss_string_make_ustr(p_ss_data_res, (PC_USTR) a7str_type)); /* U is superset of A7 */
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* NUMBER return OpenDocument / Microsoft Excel compatible type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_type)
{
    S32 odf_type_result;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    default: default_unhandled();
#if CHECKING
    case DATA_ID_REAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE: /* Excel stores these as real numbers; we can convert if required */
    case DATA_ID_BLANK: /* yup */
#endif
        odf_type_result = 1;
        break;

    case DATA_ID_STRING:
        odf_type_result = 2;
        break;

    case DATA_ID_LOGICAL:
        odf_type_result = 4;
        break;

    case DATA_ID_ERROR:
        odf_type_result = 16;
        break;

    case DATA_ID_RANGE:
    case RPN_DAT_ARRAY:
    case RPN_DAT_FIELD:
        odf_type_result = 64;
        break;
    }

    ss_data_set_integer_fn(p_ss_data_res, odf_type_result);
}

#endif

/******************************************************************************
*
* REAL return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_data_set_real(p_ss_data_res, strtod(applicationversion, NULL));
}

/* end of ev_func.c */
