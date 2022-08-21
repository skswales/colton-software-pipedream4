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

extern void
real_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    F64 floor_value;
    S32 s32;

    assert(RPN_DAT_REAL == p_ev_data->did_num);

    floor_value = floor(p_ev_data->arg.fp);

    if(floor_value > (F64) S32_MAX)
        s32 = S32_MAX;
    else if(floor_value < (F64) -S32_MAX) /* NB NOT S32_MIN */
        s32 = -S32_MAX;
    else
        s32 = (S32) floor_value;

    ev_data_set_integer(p_ev_data, s32);
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
* given a date which may have a time field
* past the number of seconds in a day, convert
* spurious seconds into days
*
******************************************************************************/

extern void
ss_date_normalise(
    _InoutRef_  P_EV_DATE p_ev_date)
{
    if(EV_DATE_INVALID == p_ev_date->date)
    {   /* allow hours >= 24 etc */
        return;
    }

    if((p_ev_date->time >= SECS_IN_24) || (p_ev_date->time < 0))
    {
        S32 days = p_ev_date->time / SECS_IN_24;

        p_ev_date->date += days;
        p_ev_date->time -= days * SECS_IN_24;

        if(p_ev_date->time < 0)
        {
            p_ev_date->date -= 1;
            p_ev_date->time += SECS_IN_24;
        }
    }
}

/******************************************************************************
*
* calculate actual years, months and days from a date value
*
******************************************************************************/

#define DAYS_IN_400 ((S32) (365 * 400 + 400 / 4 - 400 / 100 + 400 / 400))
#define DAYS_IN_100 ((S32) (365 * 100 + 100 / 4 - 100 / 100))
#define DAYS_IN_4   ((S32) (365 * 4   + 4   / 4))
#define DAYS_IN_1   ((S32) (365 * 1))

_Check_return_
extern STATUS
ss_dateval_to_ymd(
    _InRef_     PC_EV_DATE_DATE p_ev_date_date,
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day)
{
    S32 date = *p_ev_date_date;
    S32 adjyear = 0;

    /* Fireworkz serial number zero == basis date 1.1.0001 */
    *p_year = 1;

    if(EV_DATE_INVALID == date)
    {
        *p_month = 1;
        *p_day = 1;
        return(EVAL_ERR_NODATE);
    }

    if(date < 0)
    {
        /* cater for BCE dates by
         * adding a bias which is a multiple of 400 years
         * and adjusting for that at the end
         */
#if 1
        while(date < 0)
        {
            date += (DAYS_IN_400 * 1);
            adjyear += (400 * 1);
        }

        date += (DAYS_IN_400 * 1); /* add another multiple to be safe */
        adjyear += (400 * 1);
#else
        /* fixed bias covers all BCE dates in the Holocene */
        date += (DAYS_IN_400 * 25);
        adjyear = (400 * 25);

        if(date < 0)
        {
            *p_month = 1;
            *p_day = 1;
            return(EVAL_ERR_NODATE);
        }
#endif
    }

    /* repeated subtraction is good for ARM */
    while(date >= DAYS_IN_400)
    {
        date -= DAYS_IN_400;
        *p_year += 400; /* yields one of 0401,0801,1201,1601,2001,2401,2801... */
    }

    /* NB only do this loop up to three times otherwise we will get caught
     * by the last year in the cycle being a leap year so
     * 31.12.1600,2000,2400 etc will fail, so we may as well unroll the loop...
     */
    if(date >= DAYS_IN_100)
    {
        date -= DAYS_IN_100;
        *p_year += 100;
    }
    if(date >= DAYS_IN_100)
    {
        date -= DAYS_IN_100;
        *p_year += 100;
    }
    if(date >= DAYS_IN_100)
    {
        date -= DAYS_IN_100;
        *p_year += 100;
    }

    while(date >= DAYS_IN_4)
    {
        date -= DAYS_IN_4;
        *p_year += 4; /* yields one of 0405,... */
    }

    while(date >= DAYS_IN_1)
    {
        if(LEAP_YEAR_ACTUAL(*p_year))
        {
            assert(date == DAYS_IN_1); /* given the start year being 0001, any leap year, if present, must be at the end of a cycle */
            if(date == DAYS_IN_1)
                break;

            date -= (DAYS_IN_1 + 1);
        }
        else
        {
            date -= DAYS_IN_1;
        }

        *p_year += 1;
    }

    {
    const PC_S32 p_days_in_month = (LEAP_YEAR_ACTUAL(*p_year) ? ev_days_in_month_leap : ev_days_in_month);
    S32 month_index;
    S32 days_left = (S32) date;

    for(month_index = 0; month_index < 11; ++month_index)
    {
        S32 monthdays = p_days_in_month[month_index];

        if(monthdays > days_left)
            break;

        days_left -= monthdays;
    }

    /* convert index values to actual values (1..31,1..n,0001..9999) */
    *p_month = (month_index + 1);

    *p_day = (days_left + 1);
    } /*block*/

#if 1
    if(adjyear)
        *p_year -= adjyear;
#endif

#if CHECKING && 1 /* verify that the conversion is reversible */
    {
    EV_DATE_DATE test_date;
    ss_ymd_to_dateval(&test_date, *p_year, *p_month, *p_day);
    assert(test_date == *p_ev_date_date);
    } /*block*/
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* convert actual day, month, year values (1..31, 1..n, 0001...9999) to a dateval
*
* Fireworkz serial number zero == basis date 1.1.0001
*
******************************************************************************/

/*ncr*/
extern S32
ss_ymd_to_dateval(
    _OutRef_    P_EV_DATE_DATE p_ev_date_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day)
{
    /* convert certain actual values (1..31,1..n,0001..9999) to index values */
    S32 month_index = month - 1;
    S32 adjdate = 0;

    *p_ev_date_date = 0;

    /* transfer excess months to the years component */
    if(month_index < 0)
    {
        year += (month_index - 12) / 12;
        month_index = 12 + (month_index % 12); /* 12 + (-11,-10,..,-1,0) => 0..11 : month_index now positive */
        month = month_index + 1; /* reduces month into 1..12 */
    }
    else if(month_index > 11)
    {
        year += (month_index / 12);
        month_index = (month_index % 12); /* reduce month_index into 0..11 */
        month = month_index + 1; /* reduces month into 1..12 */
    }

    if(year <= 0)
    {
        /* cater for BCE dates by
         * adding a bias which is a multiple of 400 years
         * and adjusting for that at the end
         */
#if 1
        while(year <= 0)
        {
            adjdate += (DAYS_IN_400 * 1);
            year += (400 * 1);
        }

        adjdate += (DAYS_IN_400 * 1); /* add another multiple to be safe */
        year += (400 * 1);

        if(adjdate <= 0)
        {
            *p_ev_date_date = EV_DATE_INVALID;
            return(-1);
        }
#else
        /* fixed bias covers all BCE dates in the Holocene */
        adjdate = (DAYS_IN_400 * 25);
        year += (400 * 25);

        if(year <= 0)
        {
            *p_ev_date_date = EV_DATE_INVALID;
            return(-1);
        }
#endif
    }

    /* get number of days between basis date 1.1.0001 and the start of this year - i.e. consider *previous* years */
    assert(year >= 1);
    {
        const S32 year_minus_1 = year - 1;

        *p_ev_date_date = (year_minus_1 * 365)
                        + (year_minus_1 / 4)
                        - (year_minus_1 / 100)
                        + (year_minus_1 / 400);

        if(*p_ev_date_date < 0)
        {
            *p_ev_date_date = EV_DATE_INVALID;
            return(-1);
        }
    }

    /* get number of days between the start of this year and the start of this month - i.e. consider *previous* months */
    {
    const PC_S32 p_days_in_month = (LEAP_YEAR_ACTUAL(year) ? ev_days_in_month_leap : ev_days_in_month);
    S32 i;

    for(i = 0; i < month_index; ++i)
    {
        *p_ev_date_date += p_days_in_month[i];
    }
    } /*block*/

    /* get number of days between the start of this month and the start of this day - i.e. consider *previous* days */
    *p_ev_date_date += (day - 1);

    if(adjdate)
        *p_ev_date_date -= adjdate;

    return(0);
}

/******************************************************************************
*
* convert an hours, minutes and seconds to a timeval
*
******************************************************************************/

/*ncr*/
extern S32
ss_hms_to_timeval(
    _OutRef_    P_EV_DATE_TIME p_ev_date_time,
    _InVal_     S32 hour,
    _InVal_     S32 minute,
    _InVal_     S32 second)
{
    /* at least check hour is in range */
    if((S32) labs((long) hour) >= S32_MAX / 3600)
    {
        *p_ev_date_time = EV_TIME_INVALID;
        return(-1);
    }

    *p_ev_date_time = ((S32) hour * 3600) + (minute * 60) + second;

    return(0);
}

/******************************************************************************
*
* read the current time as actual values
*
******************************************************************************/

extern void
ss_local_time(
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_hour,
    _OutRef_    P_S32 p_minute,
    _OutRef_    P_S32 p_second)
{
#if RISCOS

    /* localtime doesn't work well on RISCOS */
    RISCOS_TIME_ORDINALS time_ordinals;
    _kernel_swi_regs rs;
    U8Z buffer[8];

    rs.r[0] = 14;
    rs.r[1] = (int) buffer;
    buffer[0] = 3 /*OSWord_ReadUTC*//*1*/;
    (void) _kernel_swi(OS_Word, &rs, &rs);

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) buffer;
    rs.r[2] = (int) &time_ordinals;

    if(NULL == _kernel_swi(Territory_ConvertTimeToOrdinals, &rs, &rs))
    {
        *p_year = time_ordinals.year;
        *p_month = time_ordinals.month;
        *p_day = time_ordinals.day;
        *p_hour = time_ordinals.hours;
        *p_minute = time_ordinals.minutes;
        *p_second = time_ordinals.seconds;
    }
    else
    {
        time_t today_time = time(NULL);
        struct tm * split_timep = localtime(&today_time);

        *p_year = split_timep->tm_year + (S32) 1900;
        *p_month = split_timep->tm_mon + (S32) 1;
        *p_day = split_timep->tm_mday;
        *p_hour = split_timep->tm_hour;
        *p_minute = split_timep->tm_min;
        *p_second = split_timep->tm_sec;
    }

#elif WINDOWS

    SYSTEMTIME systemtime;

    GetLocalTime(&systemtime);

    *p_year = systemtime.wYear;
    *p_month = systemtime.wMonth;
    *p_day = systemtime.wDay;
    *p_hour = systemtime.wHour;
    *p_minute = systemtime.wMinute;
    *p_second = systemtime.wSecond;

#else

    time_t today_time = time(NULL);
    struct tm * split_timep = localtime(&today_time);

    *p_year = split_timep->tm_year + 1900;
    *p_month = split_timep->tm_mon + 1;
    *p_day = split_timep->tm_mday;
    *p_hour = split_timep->tm_hour;
    *p_minute = split_timep->tm_min;
    *p_second = split_timep->tm_sec;

#endif /* OS */
}

extern void
ss_local_time_as_ev_date(
    _OutRef_    P_EV_DATE p_ev_date)
{
    S32 year, month, day, hour, minute, second;
    ss_local_time(&year, &month, &day, &hour, &minute, &second);
    (void) ss_ymd_to_dateval(&p_ev_date->date, year, month, day);
    (void) ss_hms_to_timeval(&p_ev_date->time, hour, minute, second);
}

_Check_return_
extern S32 /* actual year */
sliding_window_year(
    _In_        S32 year)
{
    S32 modified_year = year;
    S32 local_year, local_century;
    S32 month, day, hour, minute, second;

    ss_local_time(&local_year, &month, &day, &hour, &minute, &second);

    local_century = (local_year / 100) * 100;

    local_year = local_year - local_century;

#if 1
    /* default to a year within the sliding window of time about 'now' */
    if(modified_year > (local_year + 30))
        modified_year -= 100;
#else
    /* default to current century */
#endif

    modified_year += local_century;

    return(modified_year);
}

/******************************************************************************
*
* work out hours minutes and seconds from a time value
*
******************************************************************************/

_Check_return_
extern STATUS
ss_timeval_to_hms(
    _InRef_     PC_EV_DATE_TIME p_ev_date_time,
    _OutRef_    P_S32 p_hour,
    _OutRef_    P_S32 p_minute,
    _OutRef_    P_S32 p_second)
{
    S32 time = *p_ev_date_time;

#if EV_TIME_INVALID != 0
    if(EV_TIME_INVALID == time)
    {
        *p_hour = *p_minute = *p_second = 0;
        return(EVAL_ERR_NOTIME);
    }
#endif

    *p_hour   = (S32) (time / 3600);
    time     -= (S32) *p_hour * 3600;

    *p_minute = (S32) (time / 60);
    time     -= *p_minute * 60;

    *p_second = (S32) time;

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
        { /* SKS 27oct96 optimise empty strings */
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

/*
days in the month
*/

const S32
ev_days_in_month[12] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const S32
ev_days_in_month_leap[12] =
{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* end of ss_const.c */
