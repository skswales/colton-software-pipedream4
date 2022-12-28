/* ev_help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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

#include "cs-kernel.h" /*C:*/

#include "swis.h" /*C:*/

/*
internal functions
*/

_Check_return_
static STATUS
array_element_copy(
    _InoutRef_  P_SS_DATA p_ss_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_SS_DATA p_ss_data_from);

static void
data_ensure_constant_sub(
    _InoutRef_  P_SS_DATA p_ss_data,
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

/* try to obtain integer value from this real arg */

_Check_return_
static S32
arg_normalise_real_as_integer(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    if(status_done(ss_data_real_to_integer_force(p_ss_data)))
        return(ss_data_get_data_id(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_ARGRANGE));
}

/* try to obtain date value from this real arg */

_Check_return_
static S32
arg_normalise_real_as_date(
    _InoutRef_  P_SS_DATA p_ss_data)
{
#if 0 /* just for diff minimization */
    if(status_ok(ss_serial_number_to_date(&p_ss_data->arg.ss_date, ss_data_get_real(p_ss_data))))
        return(DATA_ID_DATE);

    return(ss_data_set_error(p_ss_data, EVAL_ERR_ARGRANGE));
#else
    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
#endif
}

_Check_return_
static S32
arg_normalise_real(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_REA)
        return(DATA_ID_REAL); /* preferred */

    if(type_flags & EM_INT)
        /* try to obtain integer value from this real arg */
        return(arg_normalise_real_as_integer(p_ss_data));

    if(type_flags & EM_DAT)
        /* try to obtain date value from this real arg */
        return(arg_normalise_real_as_date(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
}

/* promote this integer arg to real value */

_Check_return_
static S32
arg_normalise_integer_as_real(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    return(ss_data_set_real_rid(p_ss_data, (F64) ss_data_get_integer(p_ss_data)));
}

/* try to obtain date value from this integer arg */

_Check_return_
static S32
arg_normalise_integer_as_date(
    _InoutRef_  P_SS_DATA p_ss_data)
{
#if 0 /* just for diff minimization */
    if(status_ok(ss_serial_number_to_date(&p_ss_data->arg.ss_date, (F64) ss_data_get_integer(p_ss_data))))
        return(DATA_ID_DATE);

    return(ss_data_set_error(p_ss_data, EVAL_ERR_ARGRANGE));
#else
    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
#endif
}

_Check_return_
static S32
arg_normalise_integer(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_INT)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */

    if(type_flags & EM_REA)
        /* promote this integer arg to real value */
        return(arg_normalise_integer_as_real(p_ss_data));

    if(type_flags & EM_DAT)
        /* try to obtain date value from this integer arg */
        return(arg_normalise_integer_as_date(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
}

/* coerce dates to Excel-compatible serial number if a number is acceptable */

_Check_return_
static S32
arg_normalise_date_as_real(
    _InoutRef_  P_SS_DATA p_ss_data)
{
#if 0 /* just for diff minimization */
    return(ss_data_set_real_rid(p_ss_data, ss_date_to_serial_number(&p_ss_data->arg.ss_date)));
#else
    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXDATE));
#endif
}

_Check_return_
static S32
arg_normalise_date_as_integer(
    _InoutRef_  P_SS_DATA p_ss_data)
{
#if 0 /* just for diff minimization */
    if(SS_DATE_NULL != p_ss_data->arg.ss_date.date)
        return(ss_data_set_integer_rid(p_ss_data, ss_dateval_to_serial_number(p_ss_data->arg.ss_date.date)));
    else
        return(ss_data_set_integer(p_ss_data, 0)); /* this is a pure time value */
#else
    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXDATE));
#endif
}

_Check_return_
static S32
arg_normalise_date(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_DAT)
        return(DATA_ID_DATE); /* preferred */

    if(type_flags & EM_REA)
        return(arg_normalise_date_as_real(p_ss_data));

    if(type_flags & EM_INT)
        /* for EM_INT args ignore any time component */
        return(arg_normalise_date_as_integer(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXDATE));
}

_Check_return_
static S32
arg_normalise_error(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_ERR)
        return(DATA_ID_ERROR); /* preferred */

    return(p_ss_data->arg.ss_error.status);
}

_Check_return_
static S32
arg_normalise_string(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_STR)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */

    if(RPN_TMP_STRING == ss_data_get_data_id(p_ss_data))
    {
        trace_1(TRACE_MODULE_EVAL, "arg_normalise freeing string: %s", ss_data_get_string(p_ss_data));
        str_clr(&p_ss_data->arg.string_wr.uchars);
    }
    /* else string arg is not owned by us so don't free it */

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXSTRING));
}

_Check_return_
static S32
arg_normalise_blank(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    if(type_flags & EM_BLK)
        return(DATA_ID_BLANK); /* preferred */

    if(type_flags & EM_STR)
    {   /* map blank arg to empty string */
        p_ss_data->arg.string.uchars = "";
        p_ss_data->arg.string.size = 0;
        ss_data_set_data_id(p_ss_data, RPN_DAT_STRING);
        return(RPN_DAT_STRING);
    }

    /* map blank arg to zero and retry */
    ss_data_set_integer(p_ss_data, 0);

    return(arg_normalise(p_ss_data, type_flags, p_max_x, p_max_y));
}

_Check_return_
static S32
arg_normalise_array(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    if(type_flags & EM_AR0)
    {
        SS_DATA ss_data_temp = *ss_array_element_index_borrow(p_ss_data, 0, 0);

        if(RPN_TMP_ARRAY == ss_data_get_data_id(p_ss_data))
            ss_array_free(p_ss_data);

        *p_ss_data = ss_data_temp;

        return(arg_normalise(p_ss_data, type_flags, p_max_x, p_max_y));
    }

    if(type_flags & EM_ARY)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */

    if( (NULL != p_max_x) && (NULL != p_max_y) )
    {
        S32 x_size, y_size;

        array_range_sizes(p_ss_data, &x_size, &y_size);

        *p_max_x = MAX(*p_max_x, x_size);
        *p_max_y = MAX(*p_max_y, y_size);
        return(ss_data_get_data_id(p_ss_data));
    }

    if(RPN_TMP_ARRAY == ss_data_get_data_id(p_ss_data))
        ss_array_free(p_ss_data);
    /* else array arg is not owned by us so don't free it */

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXARRAY));
}

_Check_return_
static S32
arg_normalise_range(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    if(type_flags & EM_ARY)
        return(DATA_ID_RANGE); /* preferred */

    if( (NULL != p_max_x) && (NULL != p_max_y) )
    {
        S32 x_size, y_size;
        array_range_sizes(p_ss_data, &x_size, &y_size);

        *p_max_x = MAX(*p_max_x, x_size);
        *p_max_y = MAX(*p_max_y, y_size);
        return(DATA_ID_RANGE);
    }

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXARRAY));
}

_Check_return_
static S32
arg_normalise_slr(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    if(type_flags & EM_SLR)
        return(DATA_ID_SLR); /* preferred */

    /* function doesn't want SLRs, so dereference them and retry */
    ev_slr_deref(p_ss_data, &p_ss_data->arg.slr, TRUE);

    return(arg_normalise(p_ss_data, type_flags, p_max_x, p_max_y));
}

_Check_return_
static S32
arg_normalise_name(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    /* no function handles NAME args, so dereference them and retry */
    name_deref(p_ss_data, p_ss_data->arg.nameid);

    return(arg_normalise(p_ss_data, type_flags, p_max_x, p_max_y));
}

_Check_return_
static S32
arg_normalise_frm_cond(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_CDX)
        return(RPN_FRM_COND); /* preferred */

    return(ss_data_set_error(p_ss_data, EVAL_ERR_BADEXPR));
}

_Check_return_
extern S32
arg_normalise(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y)
{
    /* what have we currently got? */
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        return(arg_normalise_real(p_ss_data, type_flags));

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(arg_normalise_integer(p_ss_data, type_flags));

    case DATA_ID_DATE:
        return(arg_normalise_date(p_ss_data, type_flags));

    case DATA_ID_BLANK:
        return(arg_normalise_blank(p_ss_data, type_flags, p_max_x, p_max_y));

    case DATA_ID_ERROR:
        return(arg_normalise_error(p_ss_data, type_flags));

    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        return(arg_normalise_string(p_ss_data, type_flags));

    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        return(arg_normalise_array(p_ss_data, type_flags, p_max_x, p_max_y));

    case DATA_ID_SLR:
        return(arg_normalise_slr(p_ss_data, type_flags, p_max_x, p_max_y));

    case DATA_ID_RANGE:
        return(arg_normalise_range(p_ss_data, type_flags, p_max_x, p_max_y));

    case DATA_ID_NAME:
        return(arg_normalise_name(p_ss_data, type_flags, p_max_x, p_max_y));

    case RPN_FRM_COND:
        return(arg_normalise_frm_cond(p_ss_data, type_flags));

    default:
        return(ss_data_get_data_id(p_ss_data));
    }
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
    _InoutRef_  P_SS_DATA p_ss_data_to,
    _InRef_     PC_SS_DATA p_ss_data_from)
{
    const S32 x_from = p_ss_data_from->arg.ss_array.x_size;
    const S32 y_from = p_ss_data_from->arg.ss_array.y_size;
    STATUS res = STATUS_OK;
    S32 ix, iy;

    if( (p_ss_data_to->arg.ss_array.x_size < x_from) ||
        (p_ss_data_to->arg.ss_array.y_size < y_from) )
        if(ss_array_element_make(p_ss_data_to, x_from - 1, y_from - 1) < 0)
            return(status_nomem());

    for(iy = 0; iy < y_from; ++iy)
        for(ix = 0; ix < x_from; ++ix)
        {
            PC_SS_DATA elep = ss_array_element_index_borrow(p_ss_data_from, ix, iy);

            if((res = array_element_copy(p_ss_data_to, ix, iy, elep)) < 0)
            {
                iy = y_from;
                break;
            }
        }

    return(res);
}

/******************************************************************************
*
* copy data element at p_ss_data_from into
* array element given by p_ss_data_array, x, y
*
******************************************************************************/

_Check_return_
static STATUS
array_element_copy(
    _InoutRef_  P_SS_DATA p_ss_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_SS_DATA p_ss_data_from)
{
    const P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_array, ix, iy);

    switch(ss_data_get_data_id(p_ss_data_from))
    {
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        return(ss_string_dup(p_ss_data, p_ss_data_from));

    default:
        *p_ss_data = *p_ss_data_from;
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
    P_SS_DATA p_ss_data,
    S32 max_x,
    S32 max_y)
{
    SS_DATA temp;
    S32 old_x, old_y;
    S32 res = 0;
    S32 x, y;

    switch(ss_data_get_data_id(p_ss_data))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        old_x = p_ss_data->arg.ss_array.x_size;
        old_y = p_ss_data->arg.ss_array.y_size;
        break;

    case DATA_ID_RANGE:
        old_x = ev_slr_col(&p_ss_data->arg.range.e) - ev_slr_col(&p_ss_data->arg.range.s);
        old_y = ev_slr_row(&p_ss_data->arg.range.e) - ev_slr_row(&p_ss_data->arg.range.s);
        break;

    default:
        temp = *p_ss_data;
        old_x = old_y = 0;
        break;
    }

    /* if array/range is already big enough, give up */
    if( (max_x <= old_x) && (max_y <= old_y) )
        return(0);

    /* if we have to expand a constant array, we
     * make a copy of it first so's not to alter
     * the user's typed data
    */
    if(ss_data_get_data_id(p_ss_data) != RPN_TMP_ARRAY)
    {
        SS_DATA new_array;

        status_return(ss_array_make(&new_array, max_x, max_y));

        /* we got an array already ? */
        if(ss_data_get_data_id(p_ss_data) == RPN_RES_ARRAY)
            if((res = array_copy(&new_array, p_ss_data)) < 0)
                return(res);

        /* we can forget about any source array since it
         * it is owned by the rpn and will be freed with that
        */
        *p_ss_data = new_array;
    }
    /* ensure target array element exists */
    else if(ss_array_element_make(p_ss_data, max_x - 1, max_y - 1) < 0)
        return(status_nomem());

    if( (old_x == 1 && old_y == 1) ||
        (old_x == 0 && old_y == 0) )
    {
        if(old_x)
            temp = *ss_array_element_index_borrow(p_ss_data, 0, 0);

        /* replicate across and down */
        for(y = 0; y < max_y; ++y)
        {
            for(x = 0; x < max_x; ++x)
            {
                if(old_x && (!x && !y))
                    continue;

                if((res = array_element_copy(p_ss_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
        }
    }
    else if( (max_x > old_x) && (old_y == max_y) )
    {
        /* replicate across */
        for(y = 0; y < max_y; ++y)
        {
            temp = *ss_array_element_index_borrow(p_ss_data, old_x - 1, y);

            for(x = 1; x < max_x; ++x)
            {
                if((res = array_element_copy(p_ss_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
        }
    }
    else if( (max_y > old_y) && (old_x == max_x) )
    {
        /* replicate down */
        for(x = 0; x < max_x; ++x)
        {
            temp = *ss_array_element_index_borrow(p_ss_data, x, old_y - 1);

            for(y = 1; y < max_y; ++y)
            {
                if((res = array_element_copy(p_ss_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
        }
    }
    else
    {
        /* fill new elements with blanks */
        ss_data_set_data_id(&temp, DATA_ID_BLANK);

        for(y = 0; y < old_y; ++y)
        {
            for(x = old_x; x < max_x; ++x)
            {
                if((res = array_element_copy(p_ss_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
        }

        for(y = old_y; y < max_y; ++y)
        {
            for(x = 0; x < max_x; ++x)
            {
                if((res = array_element_copy(p_ss_data, x, y, &temp)) < 0)
                    goto end_expand;
            }
        }
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
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    if( ((U32) ix >= (U32) x_size) ||
        ((U32) iy >= (U32) y_size) ) /* caters for negative indexing */
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_SUBSCRIPT);
        return(ss_data_get_data_id(p_ss_data_out));
    }

    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        ss_array_element_read(p_ss_data_out, p_ss_data_in, ix, iy);
        break;

    case DATA_ID_RANGE:
        p_ss_data_out->arg.slr = p_ss_data_in->arg.range.s;
        p_ss_data_out->arg.slr.col += (EV_COL) ix;
        p_ss_data_out->arg.slr.row += (EV_ROW) iy;
        ss_data_set_data_id(p_ss_data_out, DATA_ID_SLR);
        break;

    default:
        ss_data_set_error(p_ss_data_out, create_error(EVAL_ERR_SUBSCRIPT));
        break;
    }

    status_assert(arg_normalise(p_ss_data_out, types, NULL, NULL));

    return(ss_data_get_data_id(p_ss_data_out));
}

/******************************************************************************
*
* extract an element of array or range using single index (across rows first)
*
******************************************************************************/

extern void
array_range_mono_index(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    if((0 == x_size) || (0 == y_size))
    {
        ss_data_set_error(p_ss_data_out, create_error(EVAL_ERR_SUBSCRIPT));
        return;
    }

    {
    const S32 iy  =       mono_ix / x_size;
    const S32 ix  = mono_ix - (iy * x_size);

    array_range_index(p_ss_data_out, p_ss_data_in, ix, iy, types);
    } /*block*/
}

/******************************************************************************
*
* return x and y sizes of arrays or ranges
*
******************************************************************************/

extern void
array_range_sizes(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size)
{
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        {
        *p_x_size = p_ss_data_in->arg.ss_array.x_size;
        *p_y_size = p_ss_data_in->arg.ss_array.y_size;
        break;
        }

    case DATA_ID_RANGE:
        {
        *p_x_size = ev_slr_col(&p_ss_data_in->arg.range.e) - ev_slr_col(&p_ss_data_in->arg.range.s);
        *p_y_size = ev_slr_row(&p_ss_data_in->arg.range.e) - ev_slr_row(&p_ss_data_in->arg.range.s);
        break;
        }

    default:
        *p_x_size = *p_y_size = 0;
        break;
    }
}

extern void
array_range_mono_size(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_mono_size)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    *p_mono_size = x_size * y_size;
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
    P_SS_DATA p_ss_data,
    EV_TYPE type_flags)
{
    if(asbp->x_pos >= asbp->p_ss_data->arg.ss_array.x_size)
    {
        asbp->x_pos  = 0;
        asbp->y_pos += 1;

        if(asbp->y_pos >= asbp->p_ss_data->arg.ss_array.y_size)
            return(RPN_FRM_END);
    }

    *p_ss_data = *ss_array_element_index_borrow(asbp->p_ss_data, asbp->x_pos, asbp->y_pos);

    asbp->x_pos += 1;

    return(arg_normalise(p_ss_data, type_flags, NULL, NULL));
}

/******************************************************************************
*
* start array scanning
*
******************************************************************************/

extern S32
array_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK asbp,
    P_SS_DATA p_ss_data)
{
    asbp->p_ss_data = p_ss_data;

    asbp->x_pos = asbp->y_pos = 0;

    return(p_ss_data->arg.ss_array.x_size * p_ss_data->arg.ss_array.y_size);
}

/******************************************************************************
*
* locals for array sorting
*
******************************************************************************/

static U32 x_index_static;

PROC_QSORT_PROTO(static, proc_array_sort, SS_DATA)
{
    PC_SS_DATA p_ss_data_row_1 = (PC_SS_DATA) _arg1;
    PC_SS_DATA p_ss_data_row_2 = (PC_SS_DATA) _arg2;

    PC_SS_DATA p_ss_data_1 = p_ss_data_row_1 + x_index_static;
    PC_SS_DATA p_ss_data_2 = p_ss_data_row_2 + x_index_static;

    return((int) ss_data_compare(p_ss_data_1, p_ss_data_2));
}

/******************************************************************************
*
* sort an array
*
******************************************************************************/

extern STATUS
array_sort(
    P_SS_DATA p_ss_data,
    _InVal_     U32 x_index)
{
    STATUS status = STATUS_OK;

    assert((ss_data_get_data_id(p_ss_data) == RPN_RES_ARRAY) || (ss_data_get_data_id(p_ss_data) == RPN_TMP_ARRAY));

    {
    U32 row_size = (U32) p_ss_data->arg.ss_array.x_size * sizeof32(SS_DATA);

    if(x_index < (U32) p_ss_data->arg.ss_array.x_size)
    {
        x_index_static = x_index;
        qsort(p_ss_data->arg.ss_array.elements, (size_t) p_ss_data->arg.ss_array.y_size, row_size, proc_array_sort);
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

    *(*out_pos)++ = DATA_ID_SLR;
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
                    writeval_S16(rpnbs.pos + 2, /*(S16)*/ (skip_dist + bytes_adjust));
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
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        /* optimise most common lightweight data items */
        p_ss_data_out->arg.integer = p_ss_data_in->arg.integer;
        ss_data_set_data_id(p_ss_data_out, ss_data_get_data_id(p_ss_data_in));
        break;
    }

    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case RPN_DAT_STRING:
    case RPN_RES_STRING:
        ss_data_resource_copy(p_ss_data_out, p_ss_data_in);
        ss_data_set_data_id(p_ss_data_out, RPN_TMP_STRING);
        break;

    case RPN_RES_ARRAY:
        ss_data_resource_copy(p_ss_data_out, p_ss_data_in);
        ss_data_set_data_id(p_ss_data_out, RPN_TMP_ARRAY);
        break;

    default:
        /* heave! */
        *p_ss_data_out = *p_ss_data_in;
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
    P_SS_DATA p_ss_data)
{
    if(DATA_ID_RANGE == ss_data_get_data_id(p_ss_data))
    {
        SS_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ss_data, &x_size, &y_size);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 ix, iy;
            EV_SLR slr;

            /*zero_struct_fn(slr);*/
            slr.docno = p_ss_data->arg.range.s.docno;
            slr.flags = 0;

            for(iy = 0, slr.row = p_ss_data->arg.range.s.row;
                iy < y_size;
                ++iy, ++slr.row)
            {
                for(ix = 0, slr.col = p_ss_data->arg.range.s.col;
                    ix < x_size;
                    ++ix, ++slr.col)
                {
                    P_SS_DATA p_ss_data_i = ss_array_element_index_wr(&array_out, ix, iy);
                    ev_slr_deref(p_ss_data_i, &slr, TRUE);
                    data_ensure_constant_sub(p_ss_data_i, TRUE);
                }
            }
        }

        *p_ss_data = array_out;
    }
}

/******************************************************************************
*
* limit the types in a data item to those that are allowed in a result
*
******************************************************************************/

extern void
data_ensure_constant(
    P_SS_DATA p_ss_data)
{
    data_ensure_constant_sub(p_ss_data, FALSE);
}

static void
data_ensure_constant_sub(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 array)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    /* these types need no attention */
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        break;

    /* scan array and remove embedded cockroaches */
    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        if(array)
            ss_data_set_error(p_ss_data, EVAL_ERR_NESTEDARRAY);
        else
        {
            S32 ix, iy;

            for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
                for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
                    data_ensure_constant_sub(ss_array_element_index_wr(p_ss_data, ix, iy), TRUE);
        }
        break;

    case DATA_ID_SLR:
        {
        ev_slr_deref(p_ss_data, &p_ss_data->arg.slr, TRUE);
        data_ensure_constant_sub(p_ss_data, array);
        break;
        }

    /* range is converted to array of constants */
    case DATA_ID_RANGE:
        if(array)
            ss_data_set_error(p_ss_data, EVAL_ERR_UNEXRANGE);
        else
            data_constant_array_from_range(p_ss_data);
        break;

    case DATA_ID_NAME:
        name_deref(p_ss_data, p_ss_data->arg.nameid);
        data_ensure_constant_sub(p_ss_data, array);
        break;

    /* duff result types */
    default:
        ss_data_set_error(p_ss_data, EVAL_ERR_INTERNAL);
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
    _InRef_     PC_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_RANGE:
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
    P_SS_DATA p_ss_data,
    S32 array)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    /* these types need no attention */
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
    case RPN_TMP_STRING:
        break;

    /* scan array and remove embedded cockroaches */
    case RPN_TMP_ARRAY:
        if(array)
            ss_data_set_error(p_ss_data, create_error(EVAL_ERR_NESTEDARRAY));
        else
        {
            S32 ix, iy;

            for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
                for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
                    data_limit_types(ss_array_element_index_wr(p_ss_data, ix, iy), TRUE);
        }
        break;

    case DATA_ID_SLR:
        {
        SS_DATA temp;

        ev_slr_deref(&temp, &p_ss_data->arg.slr, TRUE);
        data_ensure_owner(p_ss_data, &temp);
        data_limit_types(p_ss_data, array);
        break;
        }

    case DATA_ID_NAME:
        name_deref(p_ss_data, p_ss_data->arg.nameid);
        data_limit_types(p_ss_data, array);
        break;

    /* range is converted to array of constants */
    case DATA_ID_RANGE:
        if(array)
            ss_data_set_error(p_ss_data, create_error(EVAL_ERR_UNEXRANGE));
        else
        {
            SS_DATA array_out;
            S32 x_size, y_size;

            array_range_sizes(p_ss_data, &x_size, &y_size);

            if(status_fail(ss_array_make(&array_out, x_size, y_size)))
                ss_data_set_error(p_ss_data, status_nomem());
            else
            {
                S32 ix, iy;
                EV_SLR slr;

                slr.docno = p_ss_data->arg.range.s.docno;
                slr.flags = 0;

                for(iy = 0, slr.row = p_ss_data->arg.range.s.row;
                    iy < y_size;
                    ++iy, ++slr.row)
                {
                    for(ix = 0, slr.col = p_ss_data->arg.range.s.col;
                        ix < x_size;
                        ++ix, ++slr.col)
                    {
                        SS_DATA temp;
                        P_SS_DATA p_ss_data_a;

                        ev_slr_deref(&temp, &slr, FALSE);
                        p_ss_data_a = ss_array_element_index_wr(&array_out, ix, iy);
                        data_ensure_owner(p_ss_data_a, &temp);
                        data_limit_types(p_ss_data_a, TRUE);
                    }
                }
            }

            *p_ss_data = array_out;
        }
        break;

    /* duff result types */
    default:
        ss_data_set_error(p_ss_data, create_error(EVAL_ERR_INTERNAL));
        break;
    }
}

/******************************************************************************
*
* convert a data item to a result type
*
******************************************************************************/

static void
ss_data_to_result_convert_array(
    _OutRef_    P_EV_RESULT p_ev_result,
    _InRef_     P_SS_DATA p_ss_data_temp)
{
    S32 ix, iy;

    for(iy = 0; iy < p_ss_data_temp->arg.ss_array.y_size; ++iy)
    {
        for(ix = 0; ix < p_ss_data_temp->arg.ss_array.x_size; ++ix)
        {
            const P_SS_DATA p_ss_data_a = ss_array_element_index_wr(p_ss_data_temp, ix, iy);

            if(ss_data_get_data_id(p_ss_data_a) == RPN_TMP_STRING)
                ss_data_set_data_id(p_ss_data_a, RPN_RES_STRING);
        }
    }

    p_ev_result->data_id = RPN_RES_ARRAY;
    p_ev_result->arg.ss_array = p_ss_data_temp->arg.ss_array;
}

extern void
ss_data_to_result_convert(
    _OutRef_    P_EV_RESULT p_ev_result,
    _InRef_     PC_SS_DATA p_ss_data)
{
    SS_DATA ss_data_temp;

    data_ensure_owner(&ss_data_temp, p_ss_data);
    data_limit_types(&ss_data_temp, FALSE);

    p_ev_result->data_id = ss_data_temp.data_id;

    switch(p_ev_result->data_id)
    {
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_DATE:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
        p_ev_result->arg = ss_data_temp.arg.ss_constant;
        break;

    case DATA_ID_WORD32:
        /* minimise storage in result type */
        p_ev_result->data_id = ev_integer_size(ss_data_temp.arg.integer);
        p_ev_result->arg.integer = ss_data_temp.arg.integer;
        break;

        /* mutate temp types to result types */

    case RPN_TMP_STRING:
        p_ev_result->data_id = RPN_RES_STRING;
        p_ev_result->arg.string = ss_data_temp.arg.string;
        break;

    case RPN_TMP_ARRAY:
        ss_data_to_result_convert_array(p_ev_result, &ss_data_temp);
        break;

    default:
        p_ev_result->data_id = DATA_ID_ERROR;
        p_ev_result->arg.ss_error.status  = create_error(EVAL_ERR_INTERNAL);
        p_ev_result->arg.ss_error.type = ERROR_NORMAL;
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
    SS_DATA data_in, data_out;

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
    ss_data_to_result_convert(&out_slotp->ev_result, &data_out);

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
        case DATA_ID_RANGE:
            {
            EV_RANGE rng;

            read_range(&rng, rpnb.pos + 1);

            rpn_skip(&rpnb);

            cond_rpn_range_adjust(&rng, &skip_seen, &out_pos, (S32) PACKED_SLRSIZE - (S32) PACKED_RNGSIZE,
                                  target_slrp, abs_col, abs_row);

            break;
            }

        /* check if a name indirects to a range */
        case DATA_ID_NAME:
            {
            S32 name_processed = 0;
            EV_NAMEID nameid, name_num;

            read_nameid(&nameid, rpnb.pos + 1);

            name_num = name_def_find(nameid);

            if(name_num >= 0)
            {
                P_EV_NAME p_ev_name = name_ptr_must(name_num);

                if( !(p_ev_name->flags & TRF_UNDEFINED) &&
                    (p_ev_name->def_data.data_id == DATA_ID_RANGE) )
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
    switch(p_ev_result->data_id)
    {
    case RPN_RES_STRING:
        trace_1(TRACE_MODULE_EVAL, "ev_result_free_resources freeing string: %s", p_ev_result->arg.string.uchars);
        str_clr(&p_ev_result->arg.string_wr.uchars);
        p_ev_result->data_id = DATA_ID_BLANK;
        break;

    case RPN_RES_ARRAY:
        { /* go via SS_DATA */
        SS_DATA ss_data;
        ss_data.arg.ss_array = p_ev_result->arg.ss_array;
        ss_array_free(&ss_data);
        p_ev_result->data_id = DATA_ID_BLANK;
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
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_RESULT p_ev_result)
{
    p_ss_data->arg.ss_constant = p_ev_result->arg;

    ss_data_set_data_id(p_ss_data, p_ev_result->data_id);

#if CHECKING
    switch(ss_data_get_data_id(p_ss_data))
    {
    default:
#if CHECKING
        default_unhandled();
        /*FALLTHRU*/
        ss_data_set_error(p_ss_data, create_error(EVAL_ERR_INTERNAL));
        break;

    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
    case RPN_RES_STRING: /* no mutation needed in this direction */
    case RPN_RES_ARRAY:
#endif
        break;
    }
#endif
}

/******************************************************************************
*
* dereference an slr
*
* p_ss_data can be the same data structure as contains the slr
*
******************************************************************************/

extern S32
ev_slr_deref(
    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_SLR slrp,
    S32 copy_ext)
{
    P_EV_CELL p_ev_cell;
    S32 res;

    if((slrp->flags & SLR_EXT_REF) && ev_doc_error(ev_slr_docno(slrp)))
    {
        ss_data_set_error(p_ss_data, ev_doc_error(ev_slr_docno(slrp)));
    }
    else if((res = ev_travel(&p_ev_cell, slrp)) != 0)
    {
        /* it's an external string */
        if(res < 0)
        {
            char buffer[BUF_EV_MAX_STRING_LEN];
            P_U8 ext_ptr;
            S32 ext_res;

            if(0 == (ext_res = ev_external_string(slrp, &ext_ptr, buffer, EV_MAX_STRING_LEN)))
                ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);
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
                        err = ss_string_make_uchars(p_ss_data, ext_ptr, len);
                    else
                    {
                        p_ss_data->arg.string.uchars = ext_ptr;
                        p_ss_data->arg.string.size = len;
                        ss_data_set_data_id(p_ss_data, RPN_RES_STRING);
                    }
                }
                else
                    ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);
            }
        }
        /* it's an evaluator cell */
        else
        {
            ev_result_to_data_convert(p_ss_data, &p_ev_cell->ev_result);

            switch(ss_data_get_data_id(p_ss_data))
            {
            case DATA_ID_ERROR:
#if 1
                if(p_ss_data->arg.ss_error.type != ERROR_PROPAGATED)
                {
                    p_ss_data->arg.ss_error.type = ERROR_PROPAGATED; /* leaving underlying error */
                    p_ss_data->arg.ss_error.docno = ev_slr_docno(slrp);
                    p_ss_data->arg.ss_error.col = ev_slr_col(slrp);
                    p_ss_data->arg.ss_error.row = ev_slr_row(slrp);
                }
#else
                p_ss_data->arg.ss_error.num = create_error(EVAL_ERR_PROPAGATED);
#endif
                break;

            case RPN_DAT_STRING:
            case RPN_TMP_STRING:
            case RPN_RES_STRING:
                if(ss_string_is_blank(p_ss_data))
                    ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);
                break;
            }
        }
    }
    else
        ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);

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
        (void) ev_dec_slr(buffer + (dollar - string_in), slr.docno, &slr, false);
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
    P_SS_DATA p_ss_data,
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
            status_assert(ss_data_resource_copy(p_ss_data, &p_ev_name->def_data));
        }
    }

    if(!got_def)
        ss_data_set_error(p_ss_data, EVAL_ERR_NAMEUNDEF);
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
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(ev_slr_col(&p_range_scan_block->pos) >= ev_slr_col(&p_range_scan_block->range.e))
    {
        p_range_scan_block->pos.col  = p_range_scan_block->range.s.col; /* equivalent SBF */
        p_range_scan_block->pos.row += 1;

        if(ev_slr_row(&p_range_scan_block->pos) >= ev_slr_row(&p_range_scan_block->range.e))
        {
            CODE_ANALYSIS_ONLY(ss_data_set_blank(p_ss_data));
            return(RPN_FRM_END);
        }
    }

    ev_slr_deref(p_ss_data, &p_range_scan_block->pos, FALSE);
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    p_range_scan_block->pos.col += 1;

    return(arg_normalise(p_ss_data, type_flags, NULL, NULL));
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

#endif

/* end of ev_help.c */
