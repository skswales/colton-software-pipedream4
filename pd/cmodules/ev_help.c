/* ev_help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines for processing various data and types */

/* MRJC May 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

/*
internal functions
*/

_Check_return_
static STATUS
array_element_copy(
    _InoutRef_  P_EV_DATA p_ev_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_EV_DATA p_ev_data_from);

static void
data_ensure_constant_sub(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 array);

/******************************************************************************
*
* given a data item and a set of data allowable flags,
* convert as many different data items as possible into acceptable types
*
* --out--
* <0  error returned
* >=0 rpn number of data
*
******************************************************************************/

_Check_return_
extern S32
arg_normalise(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    /* what have we currently got? */
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
        {
        if(type_flags & EM_REA)
            break; /* preferred */

        if(type_flags & EM_INT)
        {   /* try to obtain integer value from this real arg */
            if(status_done(real_to_integer_force(p_ev_data)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }

#if 0 /* just for diff minimization */
        if(type_flags & EM_DAT)
        {   /* try to obtain date value from this real arg */
            if(status_ok(ss_serial_number_to_date(&p_ev_data->arg.ev_date, p_ev_data->arg.fp)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }
#endif

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXNUMBER));
        }

  /*case RPN_DAT_BOOL8:*/
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        if(type_flags & EM_INT)
            break; /* preferred */

        if(type_flags & EM_REA)
        {   /* promote this integer arg to real value */
            ev_data_set_real(p_ev_data, (F64) p_ev_data->arg.integer);
            break;
        }

#if 0 /* just for diff minimization */
        if(type_flags & EM_DAT)
        {   /* try to obtain date value from this integer arg */
            if(status_ok(ss_serial_number_to_date(&p_ev_data->arg.ev_date, (F64) p_ev_data->arg.integer)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }
#endif

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXNUMBER));
        }

    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        {
        if(type_flags & EM_STR)
            break; /* preferred */

        if(p_ev_data->did_num == RPN_TMP_STRING)
        {
            trace_1(TRACE_MODULE_EVAL, "arg_normalise freeing string: %s", p_ev_data->arg.string.uchars);
            str_clr(&p_ev_data->arg.string_wr.uchars);
        }
        /* else string arg is not owned by us so don't free it */

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXSTRING));
        }

    case RPN_DAT_RANGE:
        {
        if(type_flags & EM_ARY)
            break; /* preferred */

        if((NULL != p_max_x) && (NULL != p_max_y))
        {
            S32 x_size, y_size;
            array_range_sizes(p_ev_data, &x_size, &y_size);

            *p_max_x = MAX(*p_max_x, (S32) x_size);
            *p_max_y = MAX(*p_max_y, (S32) y_size);
            break;
        }

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXARRAY));
        }

    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        {
        if(type_flags & EM_AR0)
        {
            EV_DATA temp = *ss_array_element_index_borrow(p_ev_data, 0, 0);

            if(p_ev_data->did_num == RPN_TMP_ARRAY)
                ss_array_free(p_ev_data);

            *p_ev_data = temp;
            return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y));
        }

        if(type_flags & EM_ARY)
            break; /* preferred */

        if((NULL != p_max_x) && (NULL != p_max_y))
        {
            S32 x_size, y_size;
            array_range_sizes(p_ev_data, &x_size, &y_size);
            *p_max_x = MAX(*p_max_x, (S32) x_size);
            *p_max_y = MAX(*p_max_y, (S32) y_size);
            break;
        }

        if(RPN_TMP_ARRAY == p_ev_data->did_num)
            ss_array_free(p_ev_data);
        /* else array arg is not owned by us so don't free it */

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXARRAY));
        }

    case RPN_DAT_DATE:
        {
        if(type_flags & EM_DAT)
            break; /* preferred */

#if 0 /* just for diff minimization */
        /* coerce dates to Excel-compatible serial number if a number is acceptable */
        if(type_flags & EM_REA)
        {
            ev_data_set_real(p_ev_data, ss_date_to_serial_number(&p_ev_data->arg.ev_date));
            break;
        }

        if(type_flags & EM_INT)
        {   /* for EM_INT args ignore any time component */
            if(EV_DATE_NULL != p_ev_data->arg.ev_date.date)
                ev_data_set_integer(p_ev_data, ss_dateval_to_serial_number(&p_ev_data->arg.ev_date.date));
            else
                ev_data_set_integer(p_ev_data, 0); /* this is a pure time value */

            break;
        }
#endif

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXDATE));
        }

    case RPN_DAT_BLANK:
        {
        if(type_flags & EM_BLK)
            break; /* preferred */

        if(type_flags & EM_STR)
        {   /* map blank arg to empty string */
            p_ev_data->arg.string.uchars = "";
            p_ev_data->arg.string.size = 0;
            p_ev_data->did_num = RPN_DAT_STRING;
            break;
        }

        /* map blank arg to zero and retry */
        ev_data_set_real(p_ev_data, 0.0);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y));
        }

    case RPN_DAT_ERROR:
        {
        if(type_flags & EM_ERR)
            break; /* preferred */

        return(p_ev_data->arg.ev_error.status);
        }

    case RPN_DAT_SLR:
        {
        if(type_flags & EM_SLR)
            break; /* preferred */

        /* function doesn't want SLRs, so dereference them and retry */
        ev_slr_deref(p_ev_data, &p_ev_data->arg.slr, TRUE);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y));
        }

    case RPN_DAT_NAME:
        {
        /* no function handles NAME args, so dereference them and retry */
        name_deref(p_ev_data, p_ev_data->arg.nameid);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y));
        }

    case RPN_FRM_COND:
        {
        if(type_flags & EM_CDX)
            break; /* preferred */

        return(ev_data_set_error(p_ev_data, EVAL_ERR_BADEXPR));
        }
    }

    return(p_ev_data->did_num);
}

/******************************************************************************
*
* copy one array into another; the target array is not shrunk, only expanded
*
* this task is not as simple as appears at first sight since the array may
* contain handle based resources e.g. strings which need copies making
*
******************************************************************************/

extern STATUS
array_copy(
    P_EV_DATA p_ev_data_to,
    _InRef_     PC_EV_DATA p_ev_data_from)
{
    const S32 x_from = p_ev_data_from->arg.ev_array.x_size;
    const S32 y_from = p_ev_data_from->arg.ev_array.y_size;
    STATUS res = STATUS_OK;
    S32 ix, iy;

    if( (p_ev_data_to->arg.ev_array.x_size < x_from) ||
        (p_ev_data_to->arg.ev_array.y_size < y_from) )
        if(ss_array_element_make(p_ev_data_to, x_from - 1, y_from - 1) < 0)
            return(status_nomem());

    for(iy = 0; iy < y_from; ++iy)
        for(ix = 0; ix < x_from; ++ix)
        {
            PC_EV_DATA elep = ss_array_element_index_borrow(p_ev_data_from, ix, iy);

            if((res = array_element_copy(p_ev_data_to, ix, iy, elep)) < 0)
            {
                iy = y_from;
                break;
            }
        }

    return(res);
}

/******************************************************************************
*
* copy data element at p_ev_data_from into
* array element given by p_ev_data_array, x, y
*
******************************************************************************/

_Check_return_
static STATUS
array_element_copy(
    _InoutRef_  P_EV_DATA p_ev_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_EV_DATA p_ev_data_from)
{
    const P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_array, ix, iy);

    switch(p_ev_data_from->did_num)
    {
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        return(ss_string_dup(p_ev_data, p_ev_data_from));

    default:
        *p_ev_data = *p_ev_data_from;
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* expand an existing array or data element
* to the given x,y size
*
* note that max_x, max_y are sizes not indices;
* i.e. to store element 0,2 sizes must be 1,3
*
******************************************************************************/

extern S32
array_expand(
    P_EV_DATA p_ev_data,
    S32 max_x,
    S32 max_y)
{
    EV_DATA temp;
    S32 old_x, old_y, res, x, y;

    res = 0;
    if(p_ev_data->did_num == RPN_TMP_ARRAY ||
       p_ev_data->did_num == RPN_RES_ARRAY)
    {
        old_x = p_ev_data->arg.ev_array.x_size;
        old_y = p_ev_data->arg.ev_array.y_size;
    }
    else if(p_ev_data->did_num == RPN_DAT_RANGE)
    {
        old_x = ev_slr_col(&p_ev_data->arg.range.e) - ev_slr_col(&p_ev_data->arg.range.s);
        old_y = ev_slr_row(&p_ev_data->arg.range.e) - ev_slr_row(&p_ev_data->arg.range.s);
    }
    else
    {
        temp = *p_ev_data;
        old_x = old_y = 0;
    }

    /* if array/range is already big enough, give up */
    if(max_x <= old_x && max_y <= old_y)
        return(0);

    /* if we have to expand a constant array, we
     * make a copy of it first so's not to alter
     * the user's typed data
    */
    if(p_ev_data->did_num != RPN_TMP_ARRAY)
    {
        EV_DATA new_array;

        status_return(ss_array_make(&new_array, max_x, max_y));

        /* we got an array already ? */
        if(p_ev_data->did_num == RPN_RES_ARRAY)
            if((res = array_copy(&new_array, p_ev_data)) < 0)
                return(res);

        /* we can forget about any source array since it
         * it is owned by the rpn and will be freed with that
        */
        *p_ev_data = new_array;
    }
    /* ensure target array element exists */
    else if(ss_array_element_make(p_ev_data, max_x - 1, max_y - 1) < 0)
        return(status_nomem());

    if((old_x == 1 && old_y == 1) ||
       (old_x == 0 && old_y == 0))
    {
        if(old_x)
            temp = *ss_array_element_index_borrow(p_ev_data, 0, 0);

        /* replicate across and down */
        for(y = 0; y < max_y; ++y)
            for(x = 0; x < max_x; ++x)
            {
                if(old_x && (!x && !y))
                    continue;

                if((res = array_element_copy(p_ev_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
    }
    else if(max_x > old_x && old_y == max_y)
    {
        /* replicate across */
        for(y = 0; y < max_y; ++y)
        {
            temp = *ss_array_element_index_borrow(p_ev_data, old_x - 1, y);

            for(x = 1; x < max_x; ++x)
                if((res = array_element_copy(p_ev_data, x, y, &temp)) < 0)
                    goto end_expand;
        }
    }
    else if(max_y > old_y && old_x == max_x)
    {
        /* replicate down */
        for(x = 0; x < max_x; ++x)
        {
            temp = *ss_array_element_index_borrow(p_ev_data, x, old_y - 1);

            for(y = 1; y < max_y; ++y)
                if((res = array_element_copy(p_ev_data, x, y, &temp)) < 0)
                    goto end_expand;
        }
    }
    else
    {
        /* fill new elements with blanks */
        temp.did_num = RPN_DAT_BLANK;

        for(y = 0; y < old_y; ++y)
            for(x = old_x; x < max_x; ++x)
                if((res = array_element_copy(p_ev_data, x, y, &temp)) < 0)
                    goto end_expand;

        for(y = old_y; y < max_y; ++y)
            for(x = 0; x < max_x; ++x)
                if((res = array_element_copy(p_ev_data, x, y, &temp)) < 0)
                    goto end_expand;
    }

end_expand:

    return(res);
}

/******************************************************************************
*
* given a pointer to a data item and an x,y offset,
* return the relevant data item in an array or range;
* if the subscripts are too large, a subscript error
* is produced
*
******************************************************************************/

/*ncr*/
extern EV_IDNO
array_range_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ev_data_in, &x_size, &y_size);

    if( ((U32) ix >= (U32) x_size) ||
        ((U32) iy >= (U32) y_size) ) /* caters for negative indexing */
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_SUBSCRIPT);
        return(p_ev_data_out->did_num);
    }

    switch(p_ev_data_in->did_num)
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        ss_array_element_read(p_ev_data_out, p_ev_data_in, ix, iy);
        break;

    case RPN_DAT_RANGE:
        p_ev_data_out->arg.slr = p_ev_data_in->arg.range.s;
        p_ev_data_out->arg.slr.col += (EV_COL) ix;
        p_ev_data_out->arg.slr.row += (EV_ROW) iy;
        p_ev_data_out->did_num = RPN_DAT_SLR;
        break;

    default:
        ev_data_set_error(p_ev_data_out, create_error(EVAL_ERR_SUBSCRIPT));
        break;
    }

    status_assert(arg_normalise(p_ev_data_out, types, NULL, NULL));

    return(p_ev_data_out->did_num);
}

/******************************************************************************
*
* extract an element of array or range using single index (across rows first)
*
******************************************************************************/

extern void
array_range_mono_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ev_data_in, &x_size, &y_size);

    if((0 == x_size) || (0 == y_size))
    {
        ev_data_set_error(p_ev_data_out, create_error(EVAL_ERR_SUBSCRIPT));
        return;
    }

    {
    const S32 iy  =       mono_ix / x_size;
    const S32 ix  = mono_ix - (iy * x_size);

    array_range_index(p_ev_data_out, p_ev_data_in, ix, iy, types);
    } /*block*/
}

/******************************************************************************
*
* return x and y sizes of arrays or ranges
*
******************************************************************************/

extern void
array_range_sizes(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size)
{
    switch(p_ev_data_in->did_num)
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        {
        *p_x_size = p_ev_data_in->arg.ev_array.x_size;
        *p_y_size = p_ev_data_in->arg.ev_array.y_size;
        break;
        }

    case RPN_DAT_RANGE:
        {
        *p_x_size = ev_slr_col(&p_ev_data_in->arg.range.e) - ev_slr_col(&p_ev_data_in->arg.range.s);
        *p_y_size = ev_slr_row(&p_ev_data_in->arg.range.e) - ev_slr_row(&p_ev_data_in->arg.range.s);
        break;
        }

    default:
        *p_x_size = *p_y_size = 0;
        break;
    }
}

/******************************************************************************
*
* return normalised array elements
* call array_scan_init to start process
*
******************************************************************************/

extern S32
array_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK asbp,
    P_EV_DATA p_ev_data,
    EV_TYPE type_flags)
{
    if(asbp->x_pos >= asbp->p_ev_data->arg.ev_array.x_size)
    {
        asbp->x_pos  = 0;
        asbp->y_pos += 1;

        if(asbp->y_pos >= asbp->p_ev_data->arg.ev_array.y_size)
            return(RPN_FRM_END);
    }

    *p_ev_data = *ss_array_element_index_borrow(asbp->p_ev_data, asbp->x_pos, asbp->y_pos);

    asbp->x_pos += 1;

    return(arg_normalise(p_ev_data, type_flags, NULL, NULL));
}

/******************************************************************************
*
* start array scanning
*
******************************************************************************/

extern S32
array_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK asbp,
    P_EV_DATA p_ev_data)
{
    asbp->p_ev_data = p_ev_data;

    asbp->x_pos = asbp->y_pos = 0;

    return(p_ev_data->arg.ev_array.x_size * p_ev_data->arg.ev_array.y_size);
}

/******************************************************************************
*
* locals for array sorting
*
******************************************************************************/

static U32 x_index_static;

PROC_QSORT_PROTO(static, proc_array_sort, EV_DATA)
{
    PC_EV_DATA p_ev_data_row_1 = (PC_EV_DATA) _arg1;
    PC_EV_DATA p_ev_data_row_2 = (PC_EV_DATA) _arg2;

    PC_EV_DATA p_ev_data_1 = p_ev_data_row_1 + x_index_static;
    PC_EV_DATA p_ev_data_2 = p_ev_data_row_2 + x_index_static;

    return((int) ss_data_compare(p_ev_data_1, p_ev_data_2));
}

/******************************************************************************
*
* sort an array
*
******************************************************************************/

extern STATUS
array_sort(
    P_EV_DATA p_ev_data,
    _InVal_     U32 x_index)
{
    STATUS status = STATUS_OK;

    assert((p_ev_data->did_num == RPN_RES_ARRAY) || (p_ev_data->did_num == RPN_TMP_ARRAY));

    {
    U32 row_size = (U32) p_ev_data->arg.ev_array.x_size * sizeof32(EV_DATA);

    if(x_index < (U32) p_ev_data->arg.ev_array.x_size)
    {
        x_index_static = x_index;
        qsort(p_ev_data->arg.ev_array.elements, (size_t) p_ev_data->arg.ev_array.y_size, row_size, proc_array_sort);
    }
    else
        status = create_error(EVAL_ERR_OUTOFRANGE);
    } /*block*/

    return(status);
}

/******************************************************************************
*
* replace an rpn atom with an SLR when
* processing conditional RPN strings
*
******************************************************************************/

static void
cond_rpn_range_adjust(
    _InoutRef_  P_EV_RANGE p_ev_range,
    P_U8 *skip_seen,
    P_U8 *out_pos,
    S32 bytes_adjust,
    _InRef_     PC_EV_SLR target_slrp,
    BOOL abs_col,
    BOOL abs_row)
{
    if(abs_col)
        p_ev_range->s.col  = ev_slr_col(target_slrp);
    else
        p_ev_range->s.col += ev_slr_col(target_slrp);

    if(abs_row)
        p_ev_range->s.row  = ev_slr_row(target_slrp);
    else
        p_ev_range->s.row += ev_slr_row(target_slrp);

    *(*out_pos)++ = RPN_DAT_SLR;
    *out_pos += write_slr(&p_ev_range->s, *out_pos);

    /* when we alter an atom size we must adjust all skips
     * that skip over the atom !
     */
    if(*skip_seen)
    {
        RPNSTATE rpnbs;

        rpnbs.pos = *skip_seen;
        rpn_check(&rpnbs);

        while(rpnbs.pos < *out_pos)
        {
            if(rpnbs.num == RPN_FRM_SKIPTRUE ||
               rpnbs.num == RPN_FRM_SKIPFALSE)
            {
                S16 skip_dist = readval_S16(rpnbs.pos + 2);
                if(rpnbs.pos + skip_dist + bytes_adjust > *out_pos)
                {
                    writeval_S16(rpnbs.pos + 2, skip_dist + bytes_adjust);
                    *skip_seen = de_const_cast(P_U8, rpnbs.pos); /* hmm */
                }
                else
                    *skip_seen = NULL;
            }

            rpn_skip(&rpnbs);
        }
    }
}

/******************************************************************************
*
* ensure we own all the data
*
******************************************************************************/

static void
data_ensure_owner(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    switch(p_ev_data_out->did_num = p_ev_data_in->did_num)
    {
    case RPN_RES_STRING:
    case RPN_DAT_STRING:
        ss_data_resource_copy(p_ev_data_out, p_ev_data_in);
        p_ev_data_out->did_num = RPN_TMP_STRING;
        break;

    case RPN_RES_ARRAY:
        ss_data_resource_copy(p_ev_data_out, p_ev_data_in);
        p_ev_data_out->did_num = RPN_TMP_ARRAY;
        break;

    default:
        *p_ev_data_out = *p_ev_data_in;
        break;
    }
}

/******************************************************************************
*
* make a constant array from a range
*
******************************************************************************/

static void
data_constant_array_from_range(
    P_EV_DATA p_ev_data)
{
    if(p_ev_data->did_num == RPN_DAT_RANGE)
    {
        EV_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ev_data, &x_size, &y_size);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 ix, iy;
            EV_SLR slr;

            zero_struct(slr);
            slr.docno = p_ev_data->arg.range.s.docno;

            for(iy = 0, slr.row = p_ev_data->arg.range.s.row;
                iy < y_size;
                ++iy, ++slr.row)
            {
                for(ix = 0, slr.col = p_ev_data->arg.range.s.col;
                    ix < x_size;
                    ++ix, ++slr.col)
                {
                    P_EV_DATA p_ev_data_i = ss_array_element_index_wr(&array_out, ix, iy);
                    ev_slr_deref(p_ev_data_i, &slr, TRUE);
                    data_ensure_constant_sub(p_ev_data_i, TRUE);
                }
            }
        }

        *p_ev_data = array_out;
    }
}

/******************************************************************************
*
* limit the types in a data item to those that are allowed in a result
*
******************************************************************************/

extern void
data_ensure_constant(
    P_EV_DATA p_ev_data)
{
    data_ensure_constant_sub(p_ev_data, FALSE);
}

static void
data_ensure_constant_sub(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 array)
{
    switch(p_ev_data->did_num)
    {
    /* these types need no attention */
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
    case RPN_DAT_BLANK:
    case RPN_DAT_ERROR:
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        break;

    /* scan array and remove embedded cockroaches */
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        if(array)
            ev_data_set_error(p_ev_data, EVAL_ERR_NESTEDARRAY);
        else
        {
            S32 ix, iy;

            for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
                for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
                    data_ensure_constant_sub(ss_array_element_index_wr(p_ev_data, ix, iy), TRUE);
        }
        break;

    case RPN_DAT_SLR:
        {
        ev_slr_deref(p_ev_data, &p_ev_data->arg.slr, TRUE);
        data_ensure_constant_sub(p_ev_data, array);
        break;
        }

    case RPN_DAT_NAME:
        name_deref(p_ev_data, p_ev_data->arg.nameid);
        data_ensure_constant_sub(p_ev_data, array);
        break;

    /* range is converted to array of constants */
    case RPN_DAT_RANGE:
        if(array)
            ev_data_set_error(p_ev_data, EVAL_ERR_UNEXRANGE);
        else
            data_constant_array_from_range(p_ev_data);
        break;

    /* duff result types */
    default:
        ev_data_set_error(p_ev_data, EVAL_ERR_INTERNAL);
        break;
    }
}

/******************************************************************************
*
* say whether data is an array or not
*
******************************************************************************/

_Check_return_
extern BOOL
data_is_array_range(
    _InRef_     PC_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_RANGE:
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        return(TRUE);

    default:
        return(FALSE);
    }
}

/******************************************************************************
*
* limit the types in a data item to those that are allowed in a result
*
* handle based resources must be owned by the structure i.e. be TMP versions
*
******************************************************************************/

extern void
data_limit_types(
    P_EV_DATA p_ev_data,
    S32 array)
{
    switch(p_ev_data->did_num)
    {
    /* these types need no attention */
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
    case RPN_DAT_BLANK:
    case RPN_DAT_ERROR:
    case RPN_TMP_STRING:
        break;

    /* scan array and remove embedded cockroaches */
    case RPN_TMP_ARRAY:
        if(array)
            ev_data_set_error(p_ev_data, create_error(EVAL_ERR_NESTEDARRAY));
        else
        {
            S32 ix, iy;

            for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
                for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
                    data_limit_types(ss_array_element_index_wr(p_ev_data, ix, iy), TRUE);
        }
        break;

    case RPN_DAT_SLR:
        {
        EV_DATA temp;

        ev_slr_deref(&temp, &p_ev_data->arg.slr, TRUE);
        data_ensure_owner(p_ev_data, &temp);
        data_limit_types(p_ev_data, array);
        break;
        }

    case RPN_DAT_NAME:
        name_deref(p_ev_data, p_ev_data->arg.nameid);
        data_limit_types(p_ev_data, array);
        break;

    /* range is converted to array of constants */
    case RPN_DAT_RANGE:
        if(array)
            ev_data_set_error(p_ev_data, create_error(EVAL_ERR_UNEXRANGE));
        else
        {
            EV_DATA array_out;
            S32 x_size, y_size;

            array_range_sizes(p_ev_data, &x_size, &y_size);

            if(status_fail(ss_array_make(&array_out, x_size, y_size)))
                ev_data_set_error(p_ev_data, status_nomem());
            else
            {
                S32 ix, iy;
                EV_SLR slr;

                slr.docno = p_ev_data->arg.range.s.docno;
                slr.flags = 0;

                for(iy = 0, slr.row = p_ev_data->arg.range.s.row;
                    iy < y_size;
                    ++iy, ++slr.row)
                {
                    for(ix = 0, slr.col = p_ev_data->arg.range.s.col;
                        ix < x_size;
                        ++ix, ++slr.col)
                    {
                        EV_DATA temp;
                        P_EV_DATA p_ev_data_a;

                        ev_slr_deref(&temp, &slr, FALSE);
                        p_ev_data_a = ss_array_element_index_wr(&array_out, ix, iy);
                        data_ensure_owner(p_ev_data_a, &temp);
                        data_limit_types(p_ev_data_a, TRUE);
                    }
                }
            }

            *p_ev_data = array_out;
        }
        break;

    /* duff result types */
    default:
        ev_data_set_error(p_ev_data, create_error(EVAL_ERR_INTERNAL));
        break;
    }
}

/******************************************************************************
*
* convert a data item to a result type
*
******************************************************************************/

extern void
ev_data_to_result_convert(
    _OutRef_    P_EV_RESULT p_ev_result,
    _InRef_     PC_EV_DATA p_ev_data)
{
    EV_DATA temp;

    data_ensure_owner(&temp, p_ev_data);
    data_limit_types(&temp, FALSE);

    switch(p_ev_result->did_num = temp.did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
    case RPN_DAT_BLANK:
    case RPN_DAT_ERROR:
        p_ev_result->arg = temp.arg.ev_constant;
        break;

        /* mutate temp types to result types */

    case RPN_TMP_STRING:
        p_ev_result->did_num = RPN_RES_STRING;
        p_ev_result->arg.string = temp.arg.string;
        break;

    case RPN_TMP_ARRAY:
        {
        S32 ix, iy;

        for(iy = 0; iy < temp.arg.ev_array.y_size; ++iy)
            for(ix = 0; ix < temp.arg.ev_array.x_size; ++ix)
            {
                P_EV_DATA p_ev_data_a;

                p_ev_data_a = ss_array_element_index_wr(&temp, ix, iy);
                if(p_ev_data_a->did_num == RPN_TMP_STRING)
                    p_ev_data_a->did_num = RPN_RES_STRING;
            }

        p_ev_result->did_num = RPN_RES_ARRAY;
        p_ev_result->arg.ev_array = temp.arg.ev_array;
        break;
        }

    default:
        p_ev_result->did_num = RPN_DAT_ERROR;
        p_ev_result->arg.ev_error.status  = create_error(EVAL_ERR_INTERNAL);
        p_ev_result->arg.ev_error.type = ERROR_NORMAL;
        break;
    }
}

/******************************************************************************
*
* is this a formula cell ??
*
******************************************************************************/

extern S32
ev_is_formula(
    P_EV_CELL p_ev_cell)
{
    switch(p_ev_cell->parms.type)
    {
    case EVS_CON_RPN:
    case EVS_VAR_RPN:
        return(1);

    case EVS_CON_DATA:
    default:
        return(0);
    }
}

/******************************************************************************
*
* make a copy of an evaluator cell, duplicating
* indirect resources where necessary
*
******************************************************************************/

extern S32
ev_exp_copy(
    P_EV_CELL out_slotp,
    P_EV_CELL in_slotp)
{
    P_U8 rpn_in, rpn_out;
    EV_DATA data_in, data_out;

    rpn_in = rpn_out = NULL;
    switch(in_slotp->parms.type)
    {
    case EVS_CON_DATA:
        break;
    case EVS_CON_RPN:
        rpn_in  =  in_slotp->rpn.con.rpn_str;
        rpn_out = out_slotp->rpn.con.rpn_str;
        break;
    case EVS_VAR_RPN:
        rpn_in  =  in_slotp->rpn.var.rpn_str;
        rpn_out = out_slotp->rpn.var.rpn_str;
        out_slotp->rpn.var.visited = 0;
        break;
    }

    out_slotp->parms      = in_slotp->parms;
    out_slotp->parms.circ = 0;

    /* copy rpn, duplicating static data where necessary */
    if(rpn_in)
        memcpy32(rpn_out, rpn_in, len_rpn(rpn_in));

    /* copy cell results */
    ev_result_to_data_convert(&data_in, &in_slotp->ev_result);
    ss_data_resource_copy(&data_out, &data_in);
    ev_data_to_result_convert(&out_slotp->ev_result, &data_out);

    return(0);
}

/******************************************************************************
*
* free any resources that are owned by an expression cell
*
******************************************************************************/

extern void
ev_exp_free_resources(
    _InoutRef_  P_EV_CELL p_ev_cell)
{
    /* free any results that are there */
    ev_result_free_resources(&p_ev_cell->ev_result);
}

/******************************************************************************
*
* take conditional rpn string and convert to
* rpn string for current offset, ready for eval_rpn
*
* col in target_slrp is an offset
* row in target_slrp is absolute
*
******************************************************************************/

extern S32
ev_proc_conditional_rpn(
    P_U8 rpn_out,
    P_U8 rpn_in,
    _InRef_     PC_EV_SLR target_slrp,
    BOOL abs_col,
    BOOL abs_row)
{
    RPNSTATE rpnb;
    P_U8 out_pos, skip_seen;
    EV_IDNO done;

    out_pos = rpn_out;
    rpnb.pos = rpn_in;
    skip_seen = NULL;
    rpn_check(&rpnb);

    do  {
        done = rpnb.num;

        switch(rpnb.num)
        {
        /* replace ranges with slr for current offset */
        case RPN_DAT_RANGE:
            {
            EV_RANGE rng;

            read_range(&rng, rpnb.pos + 1);

            rpn_skip(&rpnb);

            cond_rpn_range_adjust(&rng, &skip_seen, &out_pos, (S32) PACKED_SLRSIZE - (S32) PACKED_RNGSIZE,
                                  target_slrp, abs_col, abs_row);

            break;
            }

        /* check if a name indirects to a range */
        case RPN_DAT_NAME:
            {
            S32 name_processed = 0;
            EV_NAMEID nameid, name_num;

            read_nameid(&nameid, rpnb.pos + 1);

            name_num = name_def_find(nameid);

            if(name_num >= 0)
            {
                P_EV_NAME p_ev_name = name_ptr_must(name_num);

                if(!(p_ev_name->flags & TRF_UNDEFINED) &&
                   p_ev_name->def_data.did_num == RPN_DAT_RANGE)
                {
                    EV_RANGE rng = p_ev_name->def_data.arg.range;

                    rpn_skip(&rpnb);

                    cond_rpn_range_adjust(&rng, &skip_seen, &out_pos, PACKED_SLRSIZE - sizeof(EV_NAMEID),
                                          target_slrp, abs_col, abs_row);

                    name_processed = 1;
                }
            }

            if(!name_processed)
                goto copy_rpn;

            break;
            }

        /* net skips for adjustment */
        case RPN_FRM_SKIPTRUE:
        case RPN_FRM_SKIPFALSE:
            if(!skip_seen)
                skip_seen = out_pos;

        /* note fall thru ! */

        /* copy across the chunk of rpn unmodified */
        copy_rpn:
        default:
            {
            PC_U8 old_pos;
            S32 len;

            old_pos = rpnb.pos;
            rpn_skip(&rpnb);
            len = rpnb.pos - old_pos;
            memcpy32(out_pos, old_pos, len);
            out_pos += len;
            break;
            }
        }
    }
    while(done != RPN_FRM_END);

    return(out_pos - rpn_out);
}

/******************************************************************************
*
* free resources stored in an expression result
*
******************************************************************************/

extern S32
ev_result_free_resources(
    _InoutRef_  P_EV_RESULT p_ev_result)
{
    switch(p_ev_result->did_num)
    {
    case RPN_RES_STRING:
        trace_1(TRACE_MODULE_EVAL, "ev_result_free_resources freeing string: %s", p_ev_result->arg.string.uchars);
        str_clr(&p_ev_result->arg.string_wr.uchars);
        p_ev_result->did_num = RPN_DAT_BLANK;
        break;

    case RPN_RES_ARRAY:
        {
        EV_DATA ev_data;
        /* go via EV_DATA */
        ev_data.arg.ev_array = p_ev_result->arg.ev_array;
        ss_array_free(&ev_data);
        p_ev_result->did_num = RPN_DAT_BLANK;
        break;
        }
    }

    return(0);
}

/******************************************************************************
*
* convert result data into a data item
*
******************************************************************************/

extern void
ev_result_to_data_convert(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_RESULT p_ev_result)
{
    switch(p_ev_data->did_num = p_ev_result->did_num)
    {
    default: default_unhandled();
#if CHECKING
        ev_data_set_error(p_ev_data, create_error(EVAL_ERR_INTERNAL));
        break;

    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
    case RPN_DAT_BLANK:
    case RPN_DAT_ERROR:
    case RPN_RES_STRING: /* no mutation needed in this direction */
    case RPN_RES_ARRAY:
#endif
        p_ev_data->arg.ev_constant = p_ev_result->arg;
        break;
    }
}

/******************************************************************************
*
* dereference an slr
*
* p_ev_data can be the same data structure as contains the slr
*
******************************************************************************/

extern S32
ev_slr_deref(
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR slrp,
    S32 copy_ext)
{
    P_EV_CELL p_ev_cell;
    S32 res;

    if((slrp->flags & SLR_EXT_REF) && ev_doc_error(ev_slr_docno(slrp)))
    {
        ev_data_set_error(p_ev_data, ev_doc_error(ev_slr_docno(slrp)));
    }
    else if((res = ev_travel(&p_ev_cell, slrp)) != 0)
    {
        /* it's an external string */
        if(res < 0)
        {
            char buffer[BUF_EV_MAX_STRING_LEN];
            P_U8 ext_ptr;
            S32 ext_res;

            if((ext_res = ev_external_string(slrp, &ext_ptr, buffer, EV_MAX_STRING_LEN)) == 0)
                p_ev_data->did_num = RPN_DAT_BLANK;
            else
            {
                S32 err;
                P_U8 c;

                /* check if external is blank */
                for(c = ext_ptr; *c == ' '; ++c);

                if(*c)
                {
                    S32 len = strlen(ext_ptr);
                    if((ext_res > 0) || copy_ext)
                        err = ss_string_make_uchars(p_ev_data, ext_ptr, len);
                    else
                    {
                        p_ev_data->arg.string.uchars = ext_ptr;
                        p_ev_data->arg.string.size = len;
                        p_ev_data->did_num = RPN_RES_STRING;
                    }
                }
                else
                    p_ev_data->did_num = RPN_DAT_BLANK;
            }
        }
        /* it's an evaluator cell */
        else
        {
            ev_result_to_data_convert(p_ev_data, &p_ev_cell->ev_result);

            switch(p_ev_data->did_num)
            {
            case RPN_DAT_ERROR:
#if 1
                if(p_ev_data->arg.ev_error.type != ERROR_PROPAGATED)
                {
                    p_ev_data->arg.ev_error.type = ERROR_PROPAGATED; /* leaving underlying error */
                    p_ev_data->arg.ev_error.docno = ev_slr_docno(slrp);
                    p_ev_data->arg.ev_error.col = ev_slr_col(slrp);
                    p_ev_data->arg.ev_error.row = ev_slr_row(slrp);
                }
#else
                p_ev_data->arg.ev_error.num = create_error(EVAL_ERR_PROPAGATED);
#endif
                break;

            case RPN_TMP_STRING:
            case RPN_DAT_STRING:
            case RPN_RES_STRING:
                if(ss_string_is_blank(p_ev_data))
                    p_ev_data->did_num = RPN_DAT_BLANK;
                break;
            }
        }
    }
    else
        p_ev_data->did_num = RPN_DAT_BLANK;

    return(0);
}

#if TRACE_ALLOWED

extern void
ev_trace_slr(
    _Out_writes_z_(elemof_buffer) P_USTR string_out,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_U8 string_in,
    _InRef_     PC_EV_SLR slrp)
{
    P_U8 dollar;

    if((dollar = strstr(string_in, "$$")) != NULL)
    {
        EV_SLR slr = *slrp;
        char buffer[BUF_EV_LONGNAMLEN];
        xstrnkpy(buffer, elemof32(buffer), string_in, dollar - string_in);
        slr.flags |= SLR_EXT_REF;
        (void) ev_dec_slr(buffer + (dollar - string_in), slr.docno, &slr, FALSE);
        xstrkat(buffer, elemof32(buffer), dollar + 2);
        xstrkpy(string_out, elemof_buffer, buffer);
    }
    else
        xstrkpy(string_out, elemof_buffer, string_in);
}

#endif

/******************************************************************************
*
* dereference a name by copying its body into the supplied data structure
*
******************************************************************************/

extern void
name_deref(
    P_EV_DATA p_ev_data,
    EV_NAMEID nameid)
{
    EV_NAMEID name_num = name_def_find(nameid);
    BOOL got_def = FALSE;

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = name_ptr_must(name_num);

        if(!(p_ev_name->flags & TRF_UNDEFINED))
        {
            got_def = TRUE;
            status_assert(ss_data_resource_copy(p_ev_data, &p_ev_name->def_data));
        }
    }

    if(!got_def)
        ev_data_set_error(p_ev_data, EVAL_ERR_NAMEUNDEF);
}

/******************************************************************************
*
* return next slr in range without overscanning; scans row by row, then col by col
*
******************************************************************************/

extern S32
range_next(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InoutRef_  P_EV_SLR p_ev_slr_pos)
{
    if( (ev_slr_row(p_ev_slr_pos) >= ev_slr_row(&p_ev_range->e)) ||
        (ev_slr_row(p_ev_slr_pos) >= ev_numrow(ev_slr_docno(p_ev_slr_pos))) )
    {
        p_ev_slr_pos->row  = p_ev_range->s.row;
        p_ev_slr_pos->col += 1;

        /* hit the end of the range ? */
        if(ev_slr_col(p_ev_slr_pos) >= ev_slr_col(&p_ev_range->e))
            return(0);
    }
    else
        p_ev_slr_pos->row += 1;

    return(1);
}

/******************************************************************************
*
* initialise scanning of a range
*
******************************************************************************/

_Check_return_
extern S32
range_scan_init(
    _InRef_     PC_EV_RANGE p_ev_range,
    _OutRef_    P_RANGE_SCAN_BLOCK p_range_scan_block)
{
    S32 res = 0;

    p_range_scan_block->range = *p_ev_range;
    p_range_scan_block->col_size = ev_slr_col(&p_range_scan_block->range.e) - ev_slr_col(&p_range_scan_block->range.s);
    p_range_scan_block->row_size = ev_slr_row(&p_range_scan_block->range.e) - ev_slr_row(&p_range_scan_block->range.s);

    p_range_scan_block->pos = p_range_scan_block->range.s;
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    if(p_range_scan_block->range.s.flags & SLR_EXT_REF)
        res = ev_doc_error(p_range_scan_block->range.s.docno);

    return(res);
}

/******************************************************************************
*
* scan each element of a range
*
******************************************************************************/

_Check_return_
extern S32
range_scan_element(
    _InoutRef_  P_RANGE_SCAN_BLOCK p_range_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags)
{
    if(ev_slr_col(&p_range_scan_block->pos) >= ev_slr_col(&p_range_scan_block->range.e))
    {
        p_range_scan_block->pos.col  = p_range_scan_block->range.s.col; /* equivalent SBF */
        p_range_scan_block->pos.row += 1;

        if(ev_slr_row(&p_range_scan_block->pos) >= ev_slr_row(&p_range_scan_block->range.e))
        {
            CODE_ANALYSIS_ONLY(ev_data_set_blank(p_ev_data));
            return(RPN_FRM_END);
        }
    }

    ev_slr_deref(p_ev_data, &p_range_scan_block->pos, FALSE);
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    p_range_scan_block->pos.col += 1;

    return(arg_normalise(p_ev_data, type_flags, NULL, NULL));
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* add an offset to an slr when the slr is moved (maybe take account of abs bits)
*
******************************************************************************/

extern void
slr_offset_add(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _InRef_     PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope,
    _InVal_     BOOL use_abs,
    _InVal_     BOOL end_coord)
{
    if(end_coord)
    {
        p_ev_slr->col -= 1;
        p_ev_slr->row -= 1;
    }

    if((NULL == p_ev_range_scope) || ev_slr_in_range(p_ev_range_scope, p_ev_slr))
    {
        if(!use_abs || !p_ev_slr->abs_col)
            p_ev_slr->col += p_ev_slr_offset->col;

        if(!use_abs || !p_ev_slr->abs_row)
            p_ev_slr->row += p_ev_slr_offset->row;
    }

    if((NULL != p_ev_range_scope) && (ev_slr_docno(p_ev_slr) == ev_slr_docno(&p_ev_range_scope->s)))
        p_ev_slr->docno = p_ev_slr_offset->docno;

    if(end_coord)
    {
        p_ev_slr->col += 1;
        p_ev_slr->row += 1;
    }

    if(p_ev_slr->col < 0)
    {
        p_ev_slr->col = 0;
        p_ev_slr->bad_ref = 1;
    }

    if(p_ev_slr->row < 0)
    {
        p_ev_slr->row = 0;
        p_ev_slr->bad_ref = 1;
    }
}

/******************************************************************************
*
* p_ev_data_res = error iff either p_ev_data1 or p_ev_data2 are error
*
******************************************************************************/

_Check_return_ _Success_(return)
static inline BOOL
two_nums_propogate_errors(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    if(ev_data_is_error(p_ev_data1))
    {
        *p_ev_data_res = *p_ev_data1;
        return(TRUE);
    }

    if(ev_data_is_error(p_ev_data2))
    {
        *p_ev_data_res = *p_ev_data2;
        return(TRUE);
    }

    return(FALSE);
}

#endif

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 + p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_add_try(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer + p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp + p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 / p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_divide_try(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        if(0 == p_ev_data2->arg.integer)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        else
        {
            const div_t d = div((int) p_ev_data1->arg.integer, (int) p_ev_data2->arg.integer);

            if(0 == d.rem)
                ev_data_set_integer(p_ev_data_res, d.quot);
            else
                ev_data_set_real(p_ev_data_res, (F64) p_ev_data1->arg.integer / p_ev_data2->arg.integer);
        }

        did_op = TRUE;
        break;

    case TWO_REALS:
        if(0.0 == p_ev_data2->arg.fp)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        else
            ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp / p_ev_data2->arg.fp);

        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 * p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_multiply_try(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer * p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp * p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 - p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_subtract_try(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer - p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp - p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        break;
    }

    return(did_op);
}

/* end of ev_help.c */
