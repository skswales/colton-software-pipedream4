/* ss_const.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC April 1992 / August 1993; SKS derived from Fireworkz for PipeDream use */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

#define SECS_IN_24 (60 * 60 * 24)

/******************************************************************************
*
* set error type into data element
*
******************************************************************************/

/*ncr*/
extern S32
ev_data_set_error(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     STATUS error)
{
    zero_struct_ptr(p_ev_data);
    p_ev_data->did_num = RPN_DAT_ERROR;
    p_ev_data->arg.ev_error.status = error;
    p_ev_data->arg.ev_error.type = ERROR_NORMAL;
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
extern F64
real_floor(
    _InVal_     F64 f64)
{
    F64 floor_value;

    /*if(global_preferences.ss_calc_additional_rounding)*/
    {
        int exponent;
        F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */
        const int mantissa_digits_minus_n = (DBL_MANT_DIG - 3);
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;

        if(exponent >= 0) /* no need to do more for negative exponents here */
        {
            const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);
            const F64 adjusted_value = f64 + rounding_value;

            /* adjusted result */
            floor_value = floor(adjusted_value);
            return(floor_value);
        }
    }

    /* standard result */
    floor_value = floor(f64);
    return(floor_value);
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
extern F64
real_trunc(
    _InVal_     F64 f64)
{
    F64 trunc_value;

    /*if(global_preferences.ss_calc_additional_rounding)*/
    {
        int exponent;
        F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */
        const int mantissa_digits_minus_n = (DBL_MANT_DIG - 3);
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;

        if(exponent >= 0) /* no need to do more for negative exponents here */
        {
            const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);
            const F64 adjusted_value = f64 + rounding_value;

            /* adjusted result */
            (void) modf(adjusted_value, &trunc_value);
            return(trunc_value);
        }
    }

    /* standard result */
    (void) modf(f64, &trunc_value);
    return(trunc_value);
}

/******************************************************************************
*
* force conversion of fp to integer
*
******************************************************************************/

/*ncr*/
extern STATUS
real_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    F64 floor_value;
    S32 s32;

    assert(RPN_DAT_REAL == p_ev_data->did_num);
    if(RPN_DAT_REAL != p_ev_data->did_num)
        return(STATUS_OK); /* no conversion */

    if(p_ev_data->arg.fp > (F64) S32_MAX)
    {
        ev_data_set_integer(p_ev_data, S32_MAX);
        return(STATUS_OK); /* out of range */
    }
    else if(p_ev_data->arg.fp < (F64) -S32_MAX) /* NB NOT S32_MIN */
    {
        ev_data_set_integer(p_ev_data, -S32_MAX);
        return(STATUS_OK); /* out of range */
    }
    else
    {
        floor_value = floor(p_ev_data->arg.fp);

        s32 = (S32) floor_value;

        if(s32 == S32_MIN)
        {
            ev_data_set_integer(p_ev_data, -S32_MAX);
            return(STATUS_OK); /* out of range */
        }
    }

    ev_data_set_integer(p_ev_data, s32);
    return(STATUS_DONE); /* converted OK */
}

/******************************************************************************
*
* try to make an integer from an fp
*
******************************************************************************/

/*ncr*/
extern BOOL
real_to_integer_try(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    F64 floor_value;

    if(p_ev_data->did_num != RPN_DAT_REAL)
        return(FALSE);

    floor_value = floor(p_ev_data->arg.fp);

    if( (floor_value >  S32_MAX) ||
        (floor_value < -S32_MAX) ) /* NB NOT S32_MIN */
        return(FALSE);

    if(floor_value == p_ev_data->arg.fp)
    {
        S32 s32 = (S32) floor_value;
        ev_data_set_integer(p_ev_data, s32);
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
    _InoutRef_  P_EV_DATA p_ev_data)
{
    S32 ix, iy;

    for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
        for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
            ss_data_free_resources(ss_array_element_index_wr(p_ev_data, ix, iy));

    p_ev_data->did_num = RPN_DAT_BLANK;

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ev_data->arg.ev_array.elements));
}

/******************************************************************************
*
* make a duplicate of an array
*
******************************************************************************/

extern STATUS
ss_array_dup(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    STATUS status = STATUS_OK;

    p_ev_data->did_num = RPN_TMP_ARRAY;
    p_ev_data->arg.ev_array.x_size = 0;
    p_ev_data->arg.ev_array.y_size = 0;
    p_ev_data->arg.ev_array.elements = NULL;

    if(status_fail(status = array_copy(p_ev_data, p_ev_data_in)))
        ss_array_free(p_ev_data);

    if(status_fail(status))
        return(ev_data_set_error(p_ev_data, status));

    return(STATUS_OK);
}

/******************************************************************************
*
* return pointer to array element data
*
******************************************************************************/

_Check_return_
_Ret_/*maybenull*/
extern P_EV_DATA
ss_array_element_index_wr(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert((ix < p_ev_data->arg.ev_array.x_size) && (iy < p_ev_data->arg.ev_array.y_size));

    if((ix >= p_ev_data->arg.ev_array.x_size) || (iy >= p_ev_data->arg.ev_array.y_size))
        return(NULL);

    assert(NULL != p_ev_data->arg.ev_array.elements);

    return(p_ev_data->arg.ev_array.elements + (iy * p_ev_data->arg.ev_array.x_size) + ix);
}

/******************************************************************************
*
* return pointer to array element data
*
******************************************************************************/

_Check_return_
_Ret_/*maybenull*/
extern PC_EV_DATA
ss_array_element_index_borrow(
    _InRef_     PC_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert((ix < p_ev_data->arg.ev_array.x_size) && (iy < p_ev_data->arg.ev_array.y_size));

    if((ix >= p_ev_data->arg.ev_array.x_size) || (iy >= p_ev_data->arg.ev_array.y_size))
        return(NULL);

    assert(NULL != p_ev_data->arg.ev_array.elements);

    return(p_ev_data->arg.ev_array.elements + (iy * p_ev_data->arg.ev_array.x_size) + ix);
}

/******************************************************************************
*
* ensure that a given array element exists
*
******************************************************************************/

extern STATUS
ss_array_element_make(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 old_xs = p_ev_data->arg.ev_array.x_size;
    const S32 old_ys = p_ev_data->arg.ev_array.y_size;
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
    P_EV_DATA newp;

    if(NULL == (newp = al_ptr_realloc_elem(EV_DATA, p_ev_data->arg.ev_array.elements, new_size, &status)))
        return(status);

    trace_1(TRACE_MODULE_EVAL, TEXT("array realloced, now: %d entries"), new_xs * new_ys);

    p_ev_data->arg.ev_array.elements = newp;
    } /*block*/

    { /* set all new array elements to blank */
    P_EV_DATA p_ev_data_s, p_ev_data_e, p_ev_data_t;

    /* set up new sizes */
    p_ev_data->arg.ev_array.x_size = new_xs;
    p_ev_data->arg.ev_array.y_size = new_ys;

    if(old_xs < 1 || old_ys < 1)
        p_ev_data_s = ss_array_element_index_wr(p_ev_data, 0, 0);
    else
        p_ev_data_s = ss_array_element_index_wr(p_ev_data, old_xs - 1, old_ys - 1) + 1;

    /* use new stored size to get pointer to end of new array */
    p_ev_data_e = ss_array_element_index_wr(p_ev_data, new_xs - 1, new_ys - 1);

    for(p_ev_data_t = p_ev_data_s; p_ev_data_t <= p_ev_data_e; ++p_ev_data_t)
        p_ev_data_t->did_num = RPN_DAT_BLANK;

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
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    assert((p_ev_data_src->did_num == RPN_TMP_ARRAY) || (p_ev_data_src->did_num == RPN_RES_ARRAY));

    *p_ev_data = *ss_array_element_index_borrow(p_ev_data_src, ix, iy);
    /*p_ev_data->local_data = 0;*/
    if(RPN_TMP_STRING == p_ev_data->did_num)
        p_ev_data->did_num = RPN_DAT_STRING;
}

/******************************************************************************
*
* make an array of a given size
*
******************************************************************************/

extern STATUS
ss_array_make(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size)
{
    STATUS status = STATUS_OK;

    p_ev_data->did_num = RPN_TMP_ARRAY;
    p_ev_data->arg.ev_array.x_size = 0;
    p_ev_data->arg.ev_array.y_size = 0;
    p_ev_data->arg.ev_array.elements = NULL;

    if(ss_array_element_make(p_ev_data, x_size - 1, y_size - 1) < 0)
    {
        ss_array_free(p_ev_data);
        status = status_nomem();
    }

    if(status_fail(status))
        return(ev_data_set_error(p_ev_data, status));

    return(STATUS_OK);
}

/******************************************************************************
*
* copies input data item to out, setting equal type numbers for equivalent types
*
******************************************************************************/

static void
type_equate(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    *p_ev_data_out = *p_ev_data_in;

    switch(p_ev_data_out->did_num)
    {
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        p_ev_data_out->did_num = RPN_DAT_WORD32; /* NB all integers returned from type_equate() are promoted to widest type */
        break;

    case RPN_RES_ARRAY:
        p_ev_data_out->did_num = RPN_TMP_ARRAY;
        break;

    case RPN_DAT_STRING:
    case RPN_RES_STRING:
    case RPN_TMP_STRING:
        if(ss_string_is_blank(p_ev_data_out))
            ev_data_set_WORD32(p_ev_data_out, 0);
        else
            p_ev_data_out->did_num = RPN_DAT_STRING;
        break;

    case RPN_DAT_BLANK:
        ev_data_set_WORD32(p_ev_data_out, 0);
        break;
    }
}

/******************************************************************************
*
* compare two results
*
* --out--
* <0 p_ev_data1 < p_ev_data2
* =0 p_ev_data1 = p_ev_data2
* >0 p_ev_data1 > p_ev_data2
*
******************************************************************************/

extern S32
ss_data_compare(
    _InRef_     PC_EV_DATA p_ev_data1,
    _InRef_     PC_EV_DATA p_ev_data2)
{
    EV_DATA data1, data2;
    S32 res = 0;

    /* eliminate equivalent types */
    type_equate(&data1, p_ev_data1);
    type_equate(&data2, p_ev_data2);
    two_nums_type_match(&data1, &data2, 0);

    if(data1.did_num != data2.did_num)
    {
        if(data1.did_num < data2.did_num)
            res = -1;
        else
            res = 1;
    }
    else
    {
        switch(data1.did_num)
        {
        case RPN_DAT_REAL:
            if(data1.arg.fp < data2.arg.fp)
                res = -1;
            else if(data1.arg.fp == data2.arg.fp)
                res = 0;
            else
                res = 1;
            break;

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            if(data1.arg.integer < data2.arg.integer)
                res = -1;
            else if(data1.arg.integer == data2.arg.integer)
                res = 0;
            else
                res = 1;
            break;

        case RPN_DAT_STRING:
            res = stricmp_wild(data1.arg.string.uchars, data2.arg.string.uchars);
            break;

        case RPN_TMP_ARRAY:
            {
            if(data1.arg.ev_array.y_size < data2.arg.ev_array.y_size)
                res = -1;
            else if(data1.arg.ev_array.y_size > data2.arg.ev_array.y_size)
                res = 1;
            else if(data1.arg.ev_array.x_size < data2.arg.ev_array.x_size)
                res = -1;
            else if(data1.arg.ev_array.x_size > data2.arg.ev_array.x_size)
                res = 1;

            if(!res)
            {
                S32 ix, iy;

                for(iy = 0; iy < data1.arg.ev_array.y_size; ++iy)
                {
                    for(ix = 0; ix < data1.arg.ev_array.x_size; ++ix)
                    {
                        if((res = ss_data_compare(ss_array_element_index_borrow(&data1, ix, iy),
                                                  ss_array_element_index_borrow(&data2, ix, iy))) != 0)
                            goto end_array_comp;
                    }
                }

                end_array_comp:
                    ;
            }
            break;
            }

        case RPN_DAT_DATE:
            if(data1.arg.ev_date.date < data2.arg.ev_date.date)
                res = -1;
            else if(data1.arg.ev_date.date == data2.arg.ev_date.date)
            {
                if(data1.arg.ev_date.time < data2.arg.ev_date.time)
                    res = -1;
                else if(data1.arg.ev_date.time == data2.arg.ev_date.time)
                    res = 0;
                else
                    res = 1;
            }
            else
                res = 1;
            break;

        case RPN_DAT_ERROR:
            if(data1.arg.ev_error.status < data2.arg.ev_error.status)
                res = -1;
            else if(data1.arg.ev_error.status == data2.arg.ev_error.status)
                res = 0;
            else
                res = 1;
            break;

        case RPN_DAT_BLANK:
            res = 0;
            break;
        }
    }

    return(res);
}

/******************************************************************************
*
* free resources owned by a data item
*
******************************************************************************/

extern void
ss_data_free_resources(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_TMP_STRING:
        str_clr(&p_ev_data->arg.string_wr.uchars);
        p_ev_data->did_num = RPN_DAT_BLANK;
        break;

    case RPN_TMP_ARRAY:
        ss_array_free(p_ev_data);
        p_ev_data->did_num = RPN_DAT_BLANK;
        break;
    }
}

/******************************************************************************
*
* given a data item, make a copy of
* it so we can be sure we own it
*
******************************************************************************/

extern S32
ss_data_resource_copy(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    /* we get our own copy of handle based resources */
    switch(p_ev_data_in->did_num)
    {
    case RPN_DAT_STRING:
    case RPN_TMP_STRING:
    case RPN_RES_STRING:
        (void) ss_string_dup(p_ev_data_out, p_ev_data_in);
        break;

    case RPN_TMP_ARRAY:
    case RPN_RES_ARRAY:
        (void) ss_array_dup(p_ev_data_out, p_ev_data_in);
        break;

    default:
        *p_ev_data_out = *p_ev_data_in;
        break;
    }

    return(p_ev_data_out->did_num);
}

/******************************************************************************
*
* is a string blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_EV_DATA p_ev_data)
{
    PC_U8 ptr = p_ev_data->arg.string.uchars;
    S32 len = (S32) p_ev_data->arg.string.size;

    /*assert(p_ev_data->did_num == RPN_DAT_STRING);*/

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
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src)
{
    assert(p_ev_data_src);
    /*assert(p_ev_data_src->did_num == RPN_DAT_STRING);*/
    assert(p_ev_data);

    return(ss_string_make_uchars(p_ev_data, p_ev_data_src->arg.string.uchars, p_ev_data_src->arg.string.size));
}

/******************************************************************************
*
* make a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _In_        U32 uchars_n)
{
    assert(p_ev_data);
    assert((S32) uchars_n >= 0);

    uchars_n = MIN(uchars_n, EV_MAX_STRING_LEN); /* SKS 27oct96 limit in any case */

    /*if(0 != uchars_n)*/
    {
        STATUS status;
        if(NULL == (p_ev_data->arg.string_wr.uchars = al_ptr_alloc_bytes(P_U8Z, uchars_n + 1/*NULLCH*/, &status)))
            return(ev_data_set_error(p_ev_data, status));
        if(NULL == uchars)
            PtrPutByte(p_ev_data->arg.string_wr.uchars, CH_NULL); /* allows append (like ustr_set_n()) */
        else
            memcpy32(p_ev_data->arg.string_wr.uchars, uchars, uchars_n);
        p_ev_data->arg.string_wr.uchars[uchars_n] = NULLCH;
        p_ev_data->did_num = RPN_TMP_STRING; /* copy is now owned by the caller */
    }
#if 0 /* can't do this in PipeDream - gets transferred to a result and then freed */
    else
    {   /* SKS 27oct96 optimise empty strings */
        p_ev_data->arg.string.uchars = "";
        p_ev_data->did_num = RPN_DAT_STRING;
    }
#endif

    p_ev_data->arg.string.size = uchars_n;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR ustr)
{
    return(ss_string_make_uchars(p_ev_data, ustr, ustrlen32(ustr)));
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
{
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_MIX },
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX },
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX },
    { TN_R2,  TN_RB,  TN_RB,  TN_RB,  TN_MIX },
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX },
};

static const U8
tn_no_worry[5][5] =
{
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_MIX },
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX },
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX },
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_MIX },
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX },
};

extern S32
two_nums_type_match(
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL size_worry)
{
    S32 did1 = MIN(4, p_ev_data1->did_num);
    S32 did2 = MIN(4, p_ev_data2->did_num);
    const U8 (*p_tn_array)[5] = (size_worry) ? tn_worry : tn_no_worry;

    myassert0x(RPN_DAT_REAL   == 0 &&
               RPN_DAT_WORD8  == 1 &&
               RPN_DAT_WORD16 == 2 &&
               RPN_DAT_WORD32 == 3,
               "two_nums_type_match RPN_DAT_types have moved");

    switch(p_tn_array[did2][did1])
    {
    case TN_NOP:
        return(TWO_REALS);

    case TN_R1:
        ev_data_set_real(p_ev_data1, (F64) p_ev_data1->arg.integer);
        return(TWO_REALS);

    case TN_R2:
        ev_data_set_real(p_ev_data2, (F64) p_ev_data2->arg.integer);
        return(TWO_REALS);

    case TN_RB:
        ev_data_set_real(p_ev_data1, (F64) p_ev_data1->arg.integer);
        ev_data_set_real(p_ev_data2, (F64) p_ev_data2->arg.integer);
        return(TWO_REALS);

    case TN_I:
        return(TWO_INTS);

    default:
    case TN_MIX:
        return(TWO_MIXED);
    }
}

/* end of ss_const.c */
