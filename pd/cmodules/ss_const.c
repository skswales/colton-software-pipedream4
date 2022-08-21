/* ss_const.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC April 1992 / August 1993 */

/* SKS derived from Fireworkz for PipeDream use */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

#define SECS_IN_24 (60 * 60 * 24)

const SS_DATA
ss_data_real_zero =
{
#if defined(SS_DATA_HAS_EV_IDNO_IN_TOP_16_BITS)
    0, 0, DATA_ID_REAL,
#else
    DATA_ID_REAL,
#endif
    { 0.0 }
};

/******************************************************************************
*
* set error type into data element
*
******************************************************************************/

/*ncr*/
extern S32
ss_data_set_error(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     STATUS error)
{
    zero_struct_ptr_fn(p_ss_data);
    ss_data_set_data_id(p_ss_data, DATA_ID_ERROR);
    p_ss_data->arg.ss_error.status = error;
    p_ss_data->arg.ss_error.type = ERROR_NORMAL;
    return(error);
}

/******************************************************************************
*
* floor() of real with possible ickle rounding
*
* rounding performed like real_trunc()
* so ((0.06-0.04)/0.01) coerced to integer is 2 not 1 etc.
*
******************************************************************************/

/* rounds at the given significant place before floor-ing */

_Check_return_
static F64
real_floor_try_additional_rounding(
    _InVal_     F64 f64)
{
    F64 adjusted_value = f64; /* value to be floor()ed, unless tweaked */
    int exponent;
    const F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */

    if(exponent >= 0) /* no need to do more for negative exponents here */
    {
        const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;
        const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);

        adjusted_value = f64 + rounding_value; /* adjusted result */
    }

    return(floor(adjusted_value));
}

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64)
{
    /* standard result */
    F64 floor_value = floor(f64);

    /* first do the cheap step to see if we're already at an integer */
    if(floor_value == f64)
        return(f64);

    /* if(!global_preferences.ss_calc_additional_rounding) return(floor_value); */
    /* if not already an integer, and allowed, then do the more expensive bit */
    return(real_floor_try_additional_rounding(f64));
}

/******************************************************************************
*
* trunc() of real with possible ickle rounding
*
* SKS 06oct97 for INT() function try rounding an ickle bit
* so INT((0.06-0.04)/0.01) is 2 not 1
* and INT((0.06-0.02)/1E-6) is 20000 not 19999
* which is different to naive real_trunc()
*
******************************************************************************/

/* rounds at the given significant place before truncating */

_Check_return_
static F64
real_trunc_try_additional_rounding(
    _InVal_     F64 f64)
{
    F64 adjusted_value = f64; /* value to be trunc()ed, unless tweaked */
    int exponent;
    const F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */

    if(exponent >= 0) /* no need to do more for negative exponents here */
    {
        const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;
        const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);

        adjusted_value = f64 + rounding_value; /* adjusted result */
    }

    return(trunc(adjusted_value));
}

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64)
{
    /* standard result */
    F64 trunc_value = trunc(f64);

    /* first do the cheap step to see if we're already at an integer */
    if(trunc_value == f64)
        return(f64);

    /* if(!global_preferences.ss_calc_additional_rounding) return(trunc_value); */
    /* if not already an integer, and allowed, then do the more expensive bit */
    return(real_trunc_try_additional_rounding(f64));
}

/******************************************************************************
*
* force conversion of real to integer
*
* NB ss_data_real_to_integer_force uses real_floor() NOT real_trunc()
*
******************************************************************************/

/*ncr*/
extern STATUS
ss_data_real_to_integer_force(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    F64 f64;
    F64 floor_value;
    S32 s32;

    assert(ss_data_is_real(p_ss_data));
    if(DATA_ID_REAL != ss_data_get_data_id(p_ss_data))
        return(STATUS_OK); /* no conversion */

    f64 = ss_data_get_real(p_ss_data);

    floor_value = real_floor(f64);

    if(islessequal(fabs(floor_value), (F64) S32_MAX)) /* test rejects NaN */
    {
        s32 = (S32) floor_value;
        ss_data_set_integer(p_ss_data, s32);
        return(STATUS_DONE); /* converted OK */
    }

    ss_data_set_integer(p_ss_data, isless(floor_value, 0.0) ? -S32_MAX : S32_MAX);
    return(STATUS_OK); /* out of range */
}

/******************************************************************************
*
* try to make an integer from a real if appropriate
*
******************************************************************************/

/*ncr*/
extern BOOL
ss_data_real_to_integer_try(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    F64 f64;
    F64 floor_value;
    S32 s32;

    if(DATA_ID_REAL != ss_data_get_data_id(p_ss_data))
        return(FALSE);

    f64 = ss_data_get_real(p_ss_data);

    floor_value = real_floor(f64);

    if(floor_value != ss_data_get_real(p_ss_data)/*f64*/) /* reloading saves SFM/LFN/STF ! */
        return(FALSE); /* unmodified */

    if(islessequal(fabs(floor_value), (F64) S32_MAX))
    {
        s32 = (S32) floor_value;
        ss_data_set_integer(p_ss_data, s32);
        return(TRUE); /* converted OK */
    }

    return(FALSE); /* unmodified */
}

/******************************************************************************
*
* free an array - we have to check each element for strings which themselves need deallocating
*
******************************************************************************/

extern void
ss_array_free(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    S32 ix, iy;

    for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
        for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
            ss_data_free_resources(ss_array_element_index_wr(p_ss_data, ix, iy));

    ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ss_data->arg.ss_array.elements));
}

/******************************************************************************
*
* make a duplicate of an array
*
******************************************************************************/

extern STATUS
ss_array_dup(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    STATUS status = STATUS_OK;

    ss_data_set_data_id(p_ss_data, RPN_TMP_ARRAY);
    p_ss_data->arg.ss_array.x_size = 0;
    p_ss_data->arg.ss_array.y_size = 0;
    p_ss_data->arg.ss_array.elements = NULL;

    if(status_fail(status = array_copy(p_ss_data, p_ss_data_in)))
        ss_array_free(p_ss_data);

    if(status_fail(status))
        return(ss_data_set_error(p_ss_data, status));

    return(STATUS_OK);
}

/******************************************************************************
*
* return pointer to array element data
*
******************************************************************************/

_Check_return_
_Ret_/*maybenull*/
extern P_SS_DATA
ss_array_element_index_wr(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert((ix < p_ss_data->arg.ss_array.x_size) && (iy < p_ss_data->arg.ss_array.y_size));

    if((ix >= p_ss_data->arg.ss_array.x_size) || (iy >= p_ss_data->arg.ss_array.y_size))
        return(NULL);

    assert(NULL != p_ss_data->arg.ss_array.elements);

    return(p_ss_data->arg.ss_array.elements + (iy * p_ss_data->arg.ss_array.x_size) + ix);
}

/******************************************************************************
*
* return pointer to array element data
*
******************************************************************************/

_Check_return_
_Ret_/*maybenull*/
extern PC_SS_DATA
ss_array_element_index_borrow(
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert((ix < p_ss_data->arg.ss_array.x_size) && (iy < p_ss_data->arg.ss_array.y_size));

    if((ix >= p_ss_data->arg.ss_array.x_size) || (iy >= p_ss_data->arg.ss_array.y_size))
        return(NULL);

    assert(NULL != p_ss_data->arg.ss_array.elements);

    return(p_ss_data->arg.ss_array.elements + (iy * p_ss_data->arg.ss_array.x_size) + ix);
}

/******************************************************************************
*
* ensure that a given array element exists
*
******************************************************************************/

extern STATUS
ss_array_element_make(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 old_xs = p_ss_data->arg.ss_array.x_size;
    const S32 old_ys = p_ss_data->arg.ss_array.y_size;
    S32 new_xs;
    S32 new_ys;
    S32 new_size;

    if((ix < old_xs) && (iy < old_ys))
        return(STATUS_OK);

#if CHECKING
    /* SKS 22nov94 trap uncatered for resizing, whereby we'd have to delaminate the current array due to x growth with > 1 row */
    if(old_xs && old_ys)
        if(ix > old_xs)
            assert(old_ys <= 1);
#endif

    /* calculate number of extra elements needed */
    new_xs = MAX(ix + 1, old_xs);
    new_ys = MAX(iy + 1, old_ys);
    new_size = new_xs * new_ys;

    /* check not too many elements */
    if(new_size >= EV_MAX_ARRAY_ELES)
        return(-1);

    {
    STATUS status;
    P_SS_DATA newp;

    if(NULL == (newp = al_ptr_realloc_elem(SS_DATA, p_ss_data->arg.ss_array.elements, new_size, &status)))
        return(status);

    trace_1(TRACE_MODULE_EVAL, TEXT("array realloced, now: %d entries"), new_xs * new_ys);

    p_ss_data->arg.ss_array.elements = newp;
    } /*block*/

    { /* set all new array elements to blank */
    P_SS_DATA p_ss_data_s, p_ss_data_e, p_ss_data_t;

    /* set up new sizes */
    p_ss_data->arg.ss_array.x_size = new_xs;
    p_ss_data->arg.ss_array.y_size = new_ys;

    if(old_xs < 1 || old_ys < 1)
        p_ss_data_s = ss_array_element_index_wr(p_ss_data, 0, 0);
    else
        p_ss_data_s = ss_array_element_index_wr(p_ss_data, old_xs - 1, old_ys - 1) + 1;

    /* use new stored size to get pointer to end of new array */
    p_ss_data_e = ss_array_element_index_wr(p_ss_data, new_xs - 1, new_ys - 1);

    for(p_ss_data_t = p_ss_data_s; p_ss_data_t <= p_ss_data_e; ++p_ss_data_t)
        p_ss_data_t->data_id = DATA_ID_BLANK;

    return(STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* read array element data
*
******************************************************************************/

extern void
ss_array_element_read(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert( (ss_data_get_data_id(p_ss_data_src) == RPN_TMP_ARRAY) || (ss_data_get_data_id(p_ss_data_src) == RPN_RES_ARRAY) );

    *p_ss_data = *ss_array_element_index_borrow(p_ss_data_src, ix, iy);
    /*p_ss_data->local_data = 0;*/
    if(RPN_TMP_STRING == ss_data_get_data_id(p_ss_data))
        ss_data_set_data_id(p_ss_data, RPN_DAT_STRING);
}

/******************************************************************************
*
* make an array of a given size
*
******************************************************************************/

extern STATUS
ss_array_make(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size)
{
    STATUS status = STATUS_OK;

    ss_data_set_data_id(p_ss_data, RPN_TMP_ARRAY);
    p_ss_data->arg.ss_array.x_size = 0;
    p_ss_data->arg.ss_array.y_size = 0;
    p_ss_data->arg.ss_array.elements = NULL;

    if(ss_array_element_make(p_ss_data, x_size - 1, y_size - 1) < 0)
    {
        ss_array_free(p_ss_data);
        status = status_nomem();
    }

    if(status_fail(status))
        return(ss_data_set_error(p_ss_data, status));

    return(STATUS_OK);
}

/******************************************************************************
*
* sets equal type numbers for equivalent types
*
******************************************************************************/

static void
type_equate_non_number(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case RPN_RES_ARRAY:
        /* flatten ARRAY types */
        ss_data_set_data_id(p_ss_data, RPN_TMP_ARRAY);
        return;

    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        if(!ss_string_is_blank(p_ss_data))
        {   /* flatten STRING types */
            ss_data_set_data_id(p_ss_data, RPN_DAT_STRING);
            return;
        }

        /*FALLTHRU*/

    case DATA_ID_BLANK:
        ss_data_set_WORD32(p_ss_data, 0); /* blanks_equal_zero in PipeDream */
        return;

    default:
        return;
    }
}

static inline void
type_equate(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    /* deal with number cases quickest (look at ARM DecAOF output in context before tweaking at all!) */
    const EV_IDNO data_id = ss_data_get_data_id(p_ss_data);

    switch(data_id)
    {
    case DATA_ID_REAL:
        return;

    case DATA_ID_WORD32:
        return;
    }

    switch(data_id)
    {
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
        ss_data_set_data_id(p_ss_data, DATA_ID_WORD32); /* NB all integers returned from type_equate() are promoted to widest type */
        return;

    default:
        type_equate_non_number(p_ss_data);
        return;
    }
}

/******************************************************************************
*
* compare two results
*
* --out--
* <0 p_ss_data_1 < p_ss_data_2
* =0 p_ss_data_1 = p_ss_data_2
* >0 p_ss_data_1 > p_ss_data_2
*
******************************************************************************/

static S32
ss_data_compare_data_ids(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    S32 res;

    if(ss_data_get_data_id(p_ss_data_1) < ss_data_get_data_id(p_ss_data_2))
        res = -1;
    else if(ss_data_get_data_id(p_ss_data_1) > ss_data_get_data_id(p_ss_data_2))
        res = 1;
    else
        res = 0;

    return(res);
}

_Check_return_
static S32
ss_data_compare_reals_unordered(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    F64 f64_1 = ss_data_get_real(p_ss_data_1);
    F64 f64_2 = ss_data_get_real(p_ss_data_2);

    /* This will stick NaNs up at the top of a sort ("To +infinity and beyond...") */
    if(isnan(f64_2))
    {
        if(isnan(f64_1))
            return(0);

        /* else */
            return(-1); /* f1 '<' f2 */
    }

    /* if(isnan(f64_1)) */
        return(1); /* f1 '>' f2 */
}

_Check_return_
static S32
ss_data_compare_reals(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    /* NB Do not muck about with this without watching the generated code ... */

    /* NaNs, being unordered, are never equal to each other or anything else */
    if(!isunordered(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2)))
    {
        if(ss_data_get_real(p_ss_data_1) == ss_data_get_real(p_ss_data_2))
            return(0);

        /* Handle +/-inf */
        if(isless(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2)))
            return(-1);

        /* else if(isgreater(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2))) */
            return(1);
    }

    return(ss_data_compare_reals_unordered(p_ss_data_1, p_ss_data_2));
}

_Check_return_
static inline S32
ss_data_compare_integers(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    if(p_ss_data_1->arg.integer == p_ss_data_2->arg.integer)
        return(0);

    if(p_ss_data_1->arg.integer < p_ss_data_2->arg.integer)
        return(-1);

    /* if(p_ss_data_1->arg.integer > p_ss_data_2->arg.integer) */
        return(1);
}

static inline S32
ss_data_compare_dates(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    return(ss_date_compare(&p_ss_data_1->arg.ss_date, &p_ss_data_2->arg.ss_date));
}

static S32
ss_data_compare_errors(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    S32 res;

    if(p_ss_data_1->arg.ss_error.status == p_ss_data_2->arg.ss_error.status)
        res = 0;
    else if(p_ss_data_1->arg.ss_error.status < p_ss_data_2->arg.ss_error.status)
        res = -1;
    else /* if(p_ss_data_1->arg.ss_error.status > p_ss_data_2->arg.ss_error.status) */
        res = 1;

    return(res);
}

static S32
ss_data_compare_strings(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    S32 res;

    res = stricmp_wild(ss_data_get_string(p_ss_data_1), ss_data_get_string(p_ss_data_2));

    return(res);
}

static S32
ss_data_compare_blanks(void)
{
    return(0);
}

static S32
ss_data_compare_arrays(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    S32 ix, iy;
    S32 res = 0;

    if(p_ss_data_1->arg.ss_array.y_size < p_ss_data_2->arg.ss_array.y_size)
        return(-1);
    else if(p_ss_data_1->arg.ss_array.y_size > p_ss_data_2->arg.ss_array.y_size)
        return(1);
    else if(p_ss_data_1->arg.ss_array.x_size < p_ss_data_2->arg.ss_array.x_size)
        return(-1);
    else if(p_ss_data_1->arg.ss_array.x_size > p_ss_data_2->arg.ss_array.x_size)
        return(1);

    else
    {   /* arrays are the same size; compare each element */
        for(iy = 0; iy < p_ss_data_1->arg.ss_array.y_size; ++iy)
        {
            for(ix = 0; ix < p_ss_data_1->arg.ss_array.x_size; ++ix)
            {
                res = ss_data_compare(ss_array_element_index_borrow(p_ss_data_1, ix, iy),
                                      ss_array_element_index_borrow(p_ss_data_2, ix, iy));
                if(0 != res)
                    return(res);
            }
        }
    }

    return(res);
}

extern S32
ss_data_compare(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    SS_DATA ss_data_1 = *p_ss_data_1;
    SS_DATA ss_data_2 = *p_ss_data_2;

    /* eliminate equivalent types */
    type_equate(&ss_data_1);
    type_equate(&ss_data_2);

    two_nums_type_match(&ss_data_1, &ss_data_2, FALSE);

    if(ss_data_get_data_id(&ss_data_1) != ss_data_get_data_id(&ss_data_2))
        return(ss_data_compare_data_ids(&ss_data_1, &ss_data_2));

    switch(ss_data_1.data_id)
    {
    case DATA_ID_REAL:
        return(ss_data_compare_reals(&ss_data_1, &ss_data_2));

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(ss_data_compare_integers(&ss_data_1, &ss_data_2));

    case DATA_ID_DATE:
        return(ss_data_compare_dates(&ss_data_1, &ss_data_2));

    case DATA_ID_ERROR:
        return(ss_data_compare_errors(&ss_data_1, &ss_data_2));

    case RPN_DAT_STRING:
        return(ss_data_compare_strings(&ss_data_1, &ss_data_2));

    case DATA_ID_BLANK:
        return(ss_data_compare_blanks());

    case RPN_TMP_ARRAY:
        return(ss_data_compare_arrays(&ss_data_1, &ss_data_2));

    default: default_unhandled();
        return(0);
    }
}

/******************************************************************************
*
* free resources owned by a data item
*
******************************************************************************/

extern void
ss_data_free_resources(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case RPN_TMP_STRING:
        str_clr(&p_ss_data->arg.string_wr.uchars);
        ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);
        break;

    case RPN_TMP_ARRAY:
        ss_array_free(p_ss_data);
        ss_data_set_data_id(p_ss_data, DATA_ID_BLANK);
        break;
    }
}

/******************************************************************************
*
* given a data item, make a copy of it so we can be sure we own it
*
******************************************************************************/

extern S32
ss_data_resource_copy(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    /* we get our own copy of handle based resources */
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        (void) ss_string_dup(p_ss_data_out, p_ss_data_in);
        break;

    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        (void) ss_array_dup(p_ss_data_out, p_ss_data_in);
        break;

    default:
        *p_ss_data_out = *p_ss_data_in;
        break;
    }

    return(ss_data_get_data_id(p_ss_data_out));
}

/******************************************************************************
*
* decode data (which must be a number type) as a Logical value
*
******************************************************************************/

_Check_return_
extern bool
ss_data_get_logical(
    _InRef_     PC_SS_DATA p_ss_data)
{
    /* deal with expected cases first - again, consult ARM DecAOF output before tweaking */
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(0 != ss_data_get_integer(p_ss_data));
    }

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        return(0.0 != ss_data_get_real(p_ss_data));

#if 0 /* diff minimization */
    case DATA_ID_LOGICAL:
        assert((p_ss_data->arg.logical_integer == 0U /*false*/) || (p_ss_data->arg.logical_integer == 1U /*true*/)); /* verify full width */
        return(p_ss_data->arg.logical_bool);
#endif


    default: default_unhandled();
        assert(ss_data_is_number(p_ss_data));
        return(FALSE);
    }
}

extern void
ss_data_set_logical(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     bool logical)
{
    p_ss_data->arg.logical_integer = 0U; /* set a full width integer in case bool is smaller */
    p_ss_data->arg.logical_bool = logical;
    assert((p_ss_data->arg.logical_integer == 0U /*false*/) || (p_ss_data->arg.logical_integer == 1U /*true*/)); /* verify full width */
    ss_data_set_data_id(p_ss_data, DATA_ID_LOGICAL);
}

/******************************************************************************
*
* decode data (which must be a number type) as a REAL value
*
******************************************************************************/

_Check_return_
extern F64
ss_data_get_number(
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_number(p_ss_data));
    return(ss_data_is_real(p_ss_data) ? ss_data_get_real(p_ss_data) : (F64) ss_data_get_integer(p_ss_data));
}

/******************************************************************************
*
* allocate space for a string
*
* NB MUST cater for zero-length strings
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_allocate(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_/*Val_*/ U32 len)
{
    assert(p_ss_data);
    assert((S32) len >= 0);

    // len = MIN(len, EV_MAX_STRING_LEN); /* SKS 27oct96 limit in any case */

#if 0 /* can't do this in PipeDream - gets transferred to a result and then freed */
    /*if(0 == len)*/
    {   /* SKS 27oct96 optimise empty strings */
        p_ss_data->arg.string.uchars = "";
        ss_data_set_data_id(p_ss_data, DATA_ID_STRING);
    }
#endif
    {
        STATUS status;

        if(NULL == (p_ss_data->arg.string_wr.uchars = al_ptr_alloc_bytes(P_U8Z, len + 1/*CH_NULL*/, &status)))
            return(ss_data_set_error(p_ss_data, status));

        p_ss_data->arg.string_wr.uchars[len] = CH_NULL; /* keep whole string terminated (unlike Fireworkz) so that callers don't need to worry */

        ss_data_set_data_id(p_ss_data, RPN_TMP_STRING); /* copy is now owned by the caller */
    }

    p_ss_data->arg.string.size = len;

    return(STATUS_OK);
}

/******************************************************************************
*
* is a string blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_SS_DATA p_ss_data)
{
    PC_U8 ptr = ss_data_get_string(p_ss_data);
    S32 len = (S32) ss_data_get_string_size(p_ss_data);

    /*assert(DATA_ID_STRING);*/

    if(ptr != NULL)
        while(--len >= 0)
            if(' ' != *ptr++)
                return(0);

    return(1);
}

/******************************************************************************
*
* duplicate a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_src)
{
    PTR_ASSERT(p_ss_data_src);
    /*assert(ss_data_get_data_id(p_ss_data_src) == DATA_ID_STRING);*/
    PTR_ASSERT(p_ss_data);

    return(ss_string_make_uchars(p_ss_data, ss_data_get_string(p_ss_data_src), ss_data_get_string_size(p_ss_data_src)));
}

/******************************************************************************
*
* make a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _In_        U32 uchars_n)
{
    assert(p_ss_data);
    assert((S32) uchars_n >= 0);
    assert(uchars_n <= EV_MAX_STRING_LEN); /* sometimes we get copied willy-nilly into buffers! */

    status_return(ss_string_allocate(p_ss_data, uchars_n)); /* NB caters for zero-length strings */

    if(0 != uchars_n)
    {
        if(NULL == uchars)
            PtrPutByte(p_ss_data->arg.string_wr.uchars, CH_NULL); /* allows append (like ustr_set_n()) */
        else
            memcpy32(p_ss_data->arg.string_wr.uchars, uchars, uchars_n);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR ustr)
{
    return(ss_string_make_uchars(p_ss_data, ustr, ustrlen32(ustr)));
}

_Check_return_
extern U32
ss_string_skip_leading_whitespace(
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_string(p_ss_data));

    return(ss_string_skip_leading_whitespace_uchars(ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data)));
}

/* test character for 'space' in a spreadsheet text string - not necessarily same as Unicode classification, for instance */

_Check_return_
static inline BOOL
ss_string_ucs4_is_space(
    _InVal_     UCS4 ucs4)
{
    if(ucs4 > CH_SPACE)
        return(FALSE);

    switch(ucs4)
    {  /* just these are as defined as 'space' by OpenFormula */
    case UCH_CHARACTER_TABULATION:
    case UCH_LINE_FEED:
    case UCH_CARRIAGE_RETURN:
    case UCH_SPACE:
        return(TRUE);

    default:
        return(FALSE);
    }
}

_Check_return_
extern U32
ss_string_skip_leading_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n)
{
    return(ss_string_skip_internal_whitespace_uchars(uchars, uchars_n, 0U));
}

_Check_return_
extern U32
ss_string_skip_internal_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n,
    _InVal_     U32 uchars_idx)
{
    U32 buf_idx = uchars_idx;
    U32 wss;

    assert( (0 == uchars_n) || PTR_NOT_NULL_OR_NONE(uchars) );

    while(buf_idx < uchars_n)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx);

        if(!ss_string_ucs4_is_space(u8))
            break;

        ++buf_idx;
    }

    wss = buf_idx - uchars_idx;
    return(wss);
}

_Check_return_
extern U32
ss_string_skip_trailing_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n)
{
    U32 buf_idx = uchars_n;
    U32 wss;

    assert( (0 == uchars_n) || PTR_NOT_NULL_OR_NONE(uchars) );

    while(0 != buf_idx)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx-1);

        if(!ss_string_ucs4_is_space(u8))
            break;

        --buf_idx;
    }

    wss = uchars_n - buf_idx;
    return(wss);
}

_Check_return_
extern F64
ui_strtod(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr)
{
    return(strtod((const char *) ustr, (char **) p_ustr));
}

_Check_return_
extern S32
ui_strtol(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_P_USTR p_ustr,
    _In_        int radix)
{
    return((S32) strtol((const char *) ustr, (char **) p_ustr, radix));
}

/******************************************************************************
*
* given two numbers, reals or integers of various
* sizes, convert them so they are both compatible
*
******************************************************************************/

enum TWO_NUM_ACTION
{
    TN_NOP,
    TN_R1,
    TN_R2,
    TN_RB,
    TN_I,
    TN_MIX
};

enum TWO_NUM_INDEX
{
    TN_REAL,
    TN_WORD8,
    TN_WORD16,
    TN_WORD32
};

static const U8
tn_worry[5][5] =
{ /*  REAL    L8      W16     W32     OTH */
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /* REAL */
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX }, /* L8 */
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX }, /* W16 */
    { TN_R2,  TN_RB,  TN_RB,  TN_RB,  TN_MIX }, /* W32 */
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /* OTH */
};

static const U8
tn_no_worry[5][5] =
{ /*  REAL    L8      W16     W32     OTH */
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /* REAL */
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX }, /* L8 */
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX }, /* W16 */
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX }, /* W32 */
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /* OTH */
};

extern S32
two_nums_type_match(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2,
    _InVal_     BOOL size_worry)
{
    const S32 did1 = MIN(4, ss_data_get_data_id(p_ss_data_1));
    const S32 did2 = MIN(4, ss_data_get_data_id(p_ss_data_2));
    const U8 (*p_tn_array)[5] = (size_worry) ? tn_worry : tn_no_worry;

    myassert0x(DATA_ID_REAL    == 0 &&
               DATA_ID_LOGICAL == 1 &&
               DATA_ID_WORD16  == 2 &&
               DATA_ID_WORD32  == 3,
               "two_nums_type_match RPN_DAT_types have moved");

    switch(p_tn_array[did2][did1])
    {
    case TN_NOP:
        return(TWO_REALS);

    case TN_I:
        return(TWO_INTS);
    }

    switch(p_tn_array[did2][did1])
    {
    case TN_R1:
        ss_data_set_real(p_ss_data_1, (F64) ss_data_get_integer(p_ss_data_1));
        return(TWO_REALS);

    case TN_R2:
        ss_data_set_real(p_ss_data_2, (F64) ss_data_get_integer(p_ss_data_2));
        return(TWO_REALS);

    case TN_RB:
        ss_data_set_real(p_ss_data_1, (F64) ss_data_get_integer(p_ss_data_1));
        ss_data_set_real(p_ss_data_2, (F64) ss_data_get_integer(p_ss_data_2));
        return(TWO_REALS);

    default:
#if CHECKING
        default_unhandled();
    case TN_MIX:
#endif
        return(TWO_MIXED);
    }
}

/******************************************************************************
*
* add, subtract or multiply two 32-bit signed integers, 
* checking for overflow and also returning
* a signed 64-bit result that the caller may consult
* e.g. to promote to fp
*
* N.B. before faffing with this, look at the generated code...
* ...it could be better, but is easy to make worse!
*
******************************************************************************/

_Check_return_
static inline int32_t
int32_from_int64_possible_overflow(
    _InoutRef_   P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    const int64_t int64 = p_int64_with_int32_overflow->int64_result;

    /* if both the top word and the MSB of the low word of the result
     * are all zeros (+ve) or all ones (-ve) then
     * the result still fits in 32-bit integer
     */

#if WINDOWS && (BYTE_ORDER == LITTLE_ENDIAN)
    /* try to stop Microsoft compiler generating a redundant generic shift by 32 call */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                (((const int32_t *) &int64)[1])  -  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }

    return((int64 < 0) ? INT32_MIN : INT32_MAX);
#elif RISCOS
    /* two instructions on ARM Norcroft - SUBS r0, r_hi, r_lo ASR #31; MOVNE r0, #1 */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                ((int32_t) (int64 >> 32))  -  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }
  
    /* just test sign bit of 64-bit result - single instruction TST r_hi on ARM Norcroft (compare does full subtraction) */
    return(((uint32_t) (int64 >> 32) & 0x80000000U) ? INT32_MIN : INT32_MAX);
#else
    /* portable version */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                ((int32_t) (int64 >> 32))  !=  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }

    return((int64 < 0) ? INT32_MIN : INT32_MAX);
#endif
}

_Check_return_
extern int32_t
int32_add_check_overflow(
    _In_        const int32_t addend_a,
    _In_        const int32_t addend_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
#if RISCOS
    /* NB contorted order to save register juggling on ARM Norcroft for arithmetic op */
    const int64_t int64 = (int64_t) addend_b + addend_a;
#else
    const int64_t int64 = (int64_t) addend_a + addend_b;
#endif

    p_int64_with_int32_overflow->int64_result = int64;

    return(int32_from_int64_possible_overflow(p_int64_with_int32_overflow));
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
extern int32_t
int32_subtract_check_overflow(
    _In_        const int32_t minuend,
    _In_        const int32_t subtrahend,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    const int64_t int64 = (int64_t) minuend - subtrahend;

    p_int64_with_int32_overflow->int64_result = int64;

    return(int32_from_int64_possible_overflow(p_int64_with_int32_overflow));
}

_Check_return_
extern int32_t
int32_multiply_check_overflow(
    _In_        const int32_t multiplicand_a,
    _In_        const int32_t multiplicand_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
#if RISCOS && !defined(NORCROFT_ARCH_M)
    /* NB contorted order to save register juggling on ARM Norcroft for function call */
    /* ARM Norcroft should generate a call to _ll_mullss: "Create a 64-bit number by multiplying two int32_t numbers" */
    const int64_t int64 = (int64_t) multiplicand_b * multiplicand_a;
#else
    /* Specify -arch 3M -Otime with ARM Norcroft to use SMULL instead of function call */
    const int64_t int64 = (int64_t) multiplicand_a * multiplicand_b;
#endif

    p_int64_with_int32_overflow->int64_result = int64;

    return(int32_from_int64_possible_overflow(p_int64_with_int32_overflow));
}

#endif /* UNUSED_KEEP_ALIVE */

/* end of ss_const.c */
