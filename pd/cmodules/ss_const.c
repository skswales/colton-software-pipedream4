/* ss_const.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* MRJC April 1992 / August 1993; SKS derived from Fireworkz for PipeDream use */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

/******************************************************************************
*
* set error type into data element
*
******************************************************************************/

extern S32
data_set_error(
    _OutRef_    P_EV_DATA p_ev_data,
    S32 error)
{
    zero_struct_ptr(p_ev_data);
    p_ev_data->did_num        = RPN_DAT_ERROR;
    p_ev_data->arg.error.num  = error;
    p_ev_data->arg.error.type = ERROR_NORMAL;
    return(error);
}

/******************************************************************************
*
* force conversion of fp to integer
*
******************************************************************************/

extern EV_IDNO
fp_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(p_ev_data->arg.fp > S32_MAX)
        p_ev_data->arg.integer = S32_MAX;
    else if(p_ev_data->arg.fp < S32_MIN)
        p_ev_data->arg.integer = S32_MIN;
    else
        p_ev_data->arg.integer = (S32) p_ev_data->arg.fp;

    return(p_ev_data->did_num = ev_integer_size(p_ev_data->arg.integer));
}

/******************************************************************************
*
* try to make an integer from an fp
*
******************************************************************************/

extern EV_IDNO
fp_to_integer_try(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if((p_ev_data->did_num == RPN_DAT_REAL)
    && (p_ev_data->arg.fp <= S32_MAX)
    && (p_ev_data->arg.fp >= S32_MIN)
       #if 0
    && (fabs(floor(p_ev_data->arg.fp) - p_ev_data->arg.fp) <= DBL_EPSILON)
       #else
    && (floor(p_ev_data->arg.fp) == p_ev_data->arg.fp)
       #endif
       )
        {
        p_ev_data->arg.integer = (S32) p_ev_data->arg.fp;
        return(p_ev_data->did_num = ev_integer_size(p_ev_data->arg.integer));
        }

    return(p_ev_data->did_num);
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
    S32 x, y;

    for(y = 0; y < p_ev_data->arg.arrayp->y_size; ++y)
        for(x = 0; x < p_ev_data->arg.arrayp->x_size; ++x)
            ss_data_free_resources(ss_array_element_index_wr(p_ev_data, x, y));

    list_disposeptr(P_P_ANY_PEDANTIC(&p_ev_data->arg.arrayp));
    p_ev_data->did_num = RPN_DAT_BLANK;
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
    S32 err;

    p_ev_data->did_num = RPN_DAT_BLANK;

    if((p_ev_data->arg.arrayp = list_allocptr(EV_ARRAY_OVH)) == 0)
        return(status_nomem());

    p_ev_data->did_num = RPN_TMP_ARRAY;
    p_ev_data->arg.arrayp->x_size = 0;
    p_ev_data->arg.arrayp->y_size = 0;

    if((err = array_copy(p_ev_data, p_ev_data_in)) < 0)
        {
        ss_array_free(p_ev_data);
        data_set_error(p_ev_data, err);
        return(err);
        }

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
    _InVal_     S32 x,
    _InVal_     S32 y)
{
    assert(x < p_ev_data->arg.arrayp->x_size && y < p_ev_data->arg.arrayp->y_size);

    if(x >= p_ev_data->arg.arrayp->x_size || y >= p_ev_data->arg.arrayp->y_size)
        return(NULL);

    return(&(p_ev_data->arg.arrayp->data[(y * p_ev_data->arg.arrayp->x_size) + x]).ev_data);
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
    _InVal_     S32 x,
    _InVal_     S32 y)
{
    assert(x < p_ev_data->arg.arrayp->x_size && y < p_ev_data->arg.arrayp->y_size);

    if(x >= p_ev_data->arg.arrayp->x_size || y >= p_ev_data->arg.arrayp->y_size)
        return(NULL);

    return(&(p_ev_data->arg.arrayp->data[(y * p_ev_data->arg.arrayp->x_size) + x]).ev_data);
}

/******************************************************************************
*
* ensure that a given array element exists
*
******************************************************************************/

extern STATUS
ss_array_element_make(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 x,
    _InVal_     S32 y)
{
    const S32 old_xs = p_ev_data->arg.arrayp->x_size;
    const S32 old_ys = p_ev_data->arg.arrayp->y_size;
    S32 new_xs;
    S32 new_ys;
    S32 new_size;

    if((x < old_xs) && (y < old_ys))
        return(STATUS_OK);

#if CHECKING
    /* SKS 22nov94 trap uncatered for resizing, whereby we'd have to delaminate the current array due to x growth with > 1 row */
    if(old_xs && old_ys)
        if(x > old_xs)
            assert(old_ys <= 1);
#endif

    /* calculate number of extra elements needed */
    new_xs = MAX(x + 1, old_xs);
    new_ys = MAX(y + 1, old_ys);
    new_size = (S32) new_xs * new_ys;

    /* check not too many elements */
    if(new_size >= EV_MAX_ARRAY_ELES)
        return(-1);

    {
    P_EV_ARRAY newp;
    S32 new_byte_size = (S32) sizeof(EV_DATA) * new_size;

    new_byte_size += EV_ARRAY_OVH;

    if((newp = list_reallocptr(p_ev_data->arg.arrayp, new_byte_size)) == NULL)
        return(-1);

    trace_2(TRACE_MODULE_EVAL,
            TEXT("array realloced, now: %d entries, %d bytes\n"),
            new_xs * new_ys,
            new_byte_size);

    p_ev_data->arg.arrayp = newp;
    } /*block*/

    { /* set all new array elements to blank */
    P_EV_DATA p_ev_data_s, p_ev_data_e, p_ev_data_t;

    /* set up new sizes */
    p_ev_data->arg.arrayp->x_size = new_xs;
    p_ev_data->arg.arrayp->y_size = new_ys;

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
    _InVal_     S32 x,
    _InVal_     S32 y)
{
    assert(p_ev_data_src->did_num == RPN_TMP_ARRAY);
    assert(x < p_ev_data_src->arg.arrayp->x_size);
    assert(y < p_ev_data_src->arg.arrayp->y_size);

    {
    S32 element = (y * p_ev_data_src->arg.arrayp->x_size) + x;
    PC_EV_ARRAY_ELEMENT p_ev_array_element = ((PC_EV_ARRAY_ELEMENT) &p_ev_data_src->arg.arrayp->data[0]) + element;
    *p_ev_data = p_ev_array_element->ev_data;
    /*p_ev_data->local_data = 0;*/
    } /*block*/
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

    p_ev_data->did_num = RPN_DAT_BLANK;

    if((p_ev_data->arg.arrayp = list_allocptr(EV_ARRAY_OVH)) == 0)
        status = status_nomem();
    else
        {
         p_ev_data->did_num = RPN_TMP_ARRAY;
         p_ev_data->arg.arrayp->x_size = 0;
         p_ev_data->arg.arrayp->y_size = 0;

        if(ss_array_element_make(p_ev_data, x_size - 1, y_size - 1) < 0)
            {
            ss_array_free(p_ev_data);
            status = status_nomem();
            }
        }

    if(status_fail(status))
        data_set_error(p_ev_data, status);

    return(status);
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
            p_ev_data_out->did_num = RPN_DAT_WORD32;
            break;

        case RPN_RES_ARRAY:
            p_ev_data_out->did_num = RPN_TMP_ARRAY;
            break;

        case RPN_DAT_STRING:
        case RPN_RES_STRING:
        case RPN_TMP_STRING:
            if(str_hlp_blank(p_ev_data_out))
                {
                p_ev_data_out->did_num = RPN_DAT_WORD32;
                p_ev_data_out->arg.integer = 0;
                }
            else
                p_ev_data_out->did_num = RPN_DAT_STRING;
            break;

        case RPN_DAT_BLANK:
            p_ev_data_out->did_num = RPN_DAT_WORD32;
            p_ev_data_out->arg.integer = 0;
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
                res = stricmp_wild(data1.arg.string.data, data2.arg.string.data);
                break;

            case RPN_TMP_ARRAY:
                {
                if(data1.arg.arrayp->y_size < data2.arg.arrayp->y_size)
                    res = -1;
                else if(data1.arg.arrayp->y_size > data2.arg.arrayp->y_size)
                    res = 1;
                else if(data1.arg.arrayp->x_size < data2.arg.arrayp->x_size)
                    res = -1;
                else if(data1.arg.arrayp->x_size > data2.arg.arrayp->x_size)
                    res = 1;

                if(!res)
                    {
                    S32 x, y;

                    for(y = 0; y < data1.arg.arrayp->y_size; ++y)
                        {
                        for(x = 0; x < data1.arg.arrayp->x_size; ++x)
                            {
                            if((res = ss_data_compare(ss_array_element_index_borrow(&data1, x, y),
                                                      ss_array_element_index_borrow(&data2, x, y))) != 0)
                                goto end_array_comp;
                            }
                        }

                    end_array_comp:
                        ;
                    }
                break;
                }

            case RPN_DAT_DATE:
                if(data1.arg.date.date < data2.arg.date.date)
                    res = -1;
                else if(data1.arg.date.date == data2.arg.date.date)
                    {
                    if(data1.arg.date.time < data2.arg.date.time)
                        res = -1;
                    else if(data1.arg.date.time == data2.arg.date.time)
                        res = 0;
                    else
                        res = 1;
                    }
                else
                    res = 1;
                break;

            case RPN_DAT_ERROR:
                if(data1.arg.error.num < data2.arg.error.num)
                    res = -1;
                else if(data1.arg.error.num == data2.arg.error.num)
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
            str_clr(&p_ev_data->arg.string.data);
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
            (void) str_hlp_dup(p_ev_data_out, p_ev_data_in);
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

#define SECS_IN_24 (60 * 60 * 24)

extern void
ss_date_normalise(
    _InoutRef_  P_EV_DATE p_ev_date)
{
    if(p_ev_date->date && (p_ev_date->time >= SECS_IN_24 || p_ev_date->time < 0))
    {
        S32 days;

        days = p_ev_date->time / SECS_IN_24;

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
* calculate years, months and days from a date value
*
******************************************************************************/

#define DAYS_IN_400 ((S32) (365 * 400 + 400 / 4 - 400 / 100 + 400 / 400))
#define DAYS_IN_100 ((S32) (365 * 100 + 100 / 4 - 100 / 100))
#define DAYS_IN_4   ((S32) (365 * 4   + 4   / 4))
#define DAYS_IN_1   ((S32) (365 * 1))

extern void
ss_dateval_to_ymd(
    _InRef_     PC_EV_DATE p_ev_date,
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day)
{
    S32 date = p_ev_date->date;

    *p_year = 0;

    assert(date>=0);

    if(date == 730484) /* 31.12.2000!!! */
    {
        /* SKS for 1.31/00. very sad fudge; NB 31.12.2400 will also fail */
        *p_year = 2000-1;
        *p_month = 12-1;
        *p_day = 31-1;
        return;
    }

    while(date >= DAYS_IN_400)
    {
        date  -= DAYS_IN_400;
        *p_year += 400;
    }

    while(date >= DAYS_IN_100)
    {
        date  -= DAYS_IN_100;
        *p_year += 100;
    }

    while(date >= DAYS_IN_4)
    {
        date  -= DAYS_IN_4;
        *p_year += 4;
    }

    while(date >= DAYS_IN_1)
    {
        if(date == DAYS_IN_1 && LEAP_YEAR(*p_year))
            break;

        date  -= DAYS_IN_1;
        *p_year += 1;
    }

    {
    S32 days_left = (S32) date;

    for(*p_month = 0; *p_month < 11; ++(*p_month))
        {
        S32 monthdays = (S32) ev_days[*p_month] + (*p_month == 1 && LEAP_YEAR(*p_year) ? 1 : 0);

        if(monthdays <= days_left)
            days_left -= monthdays;
        else
            break;
        }

    *p_day = days_left;
    } /*block*/
}

/******************************************************************************
*
* convert year, month, day to a dateval
*
******************************************************************************/

extern S32
ss_ymd_to_dateval(
    _InoutRef_  P_EV_DATE p_ev_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day)
{
    /* crunge up -ve months */
    if(month < 0)
    {
        year  += (month - 12) / 12;
        month  = 12 + (month % 12);
    }
    else if(month > 11)
    {
        year += month / 12;
        month = month % 12;
    }

    /* -ve years have no meaning here */
    if(year < 0)
        return(-1);

    /* get days in previous years */
    if(year)
    {
        p_ev_date->date = (U32) year * 365 +
                                year / 4   -
                                year / 100 +
                                year / 400;
    }
    else
        p_ev_date->date = 0;

    {
    S32 i;

    for(i = 0; i < month; ++i)
    {
        p_ev_date->date += ev_days[i];

        if((i == 1) && LEAP_YEAR(year))
            p_ev_date->date += 1;
    }
    } /*block*/

    p_ev_date->date += day;

    return(0);
}

/******************************************************************************
*
* convert an hours, minutes and seconds to a timeval
*
******************************************************************************/

extern S32
ss_hms_to_timeval(
    _InoutRef_  P_EV_DATE p_ev_date,
    _InVal_     S32 hour,
    _InVal_     S32 minute,
    _InVal_     S32 second)
{
    /* at least check hour is in range */
    if((S32) labs((long) hour) >= S32_MAX / 3600)
        return(-1);

    p_ev_date->time = ((S32) hour * 3600) + (minute * 60) + second;

    return(0);
}

/******************************************************************************
*
* read the current time
*
******************************************************************************/

extern void
ss_local_time(
    _OutRef_opt_ P_S32 p_year,
    _OutRef_opt_ P_S32 p_month,
    _OutRef_opt_ P_S32 p_day,
    _OutRef_opt_ P_S32 p_hour,
    _OutRef_opt_ P_S32 p_minute,
    _OutRef_opt_ P_S32 p_second)
{
#if RISCOS

    /* localtime doesn't work well on RISCOS */
    riscos_time_ordinals time_ordinals;
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
        if(NULL != p_year)
            *p_year = time_ordinals.year - 1;
        if(NULL != p_month)
            *p_month = time_ordinals.month - 1;
        if(NULL != p_day)
            *p_day = time_ordinals.day - 1;
        if(NULL != p_hour)
            *p_hour = time_ordinals.hours;
        if(NULL != p_minute)
            *p_minute = time_ordinals.minutes;
        if(NULL != p_second)
            *p_second = time_ordinals.seconds;
        }
    else
        {
        time_t today_time = time(NULL);
        struct tm * split_timep = localtime(&today_time);

        if(NULL != p_year)
            *p_year = split_timep->tm_year + (S32) 1900 - 1;
        if(NULL != p_month)
            *p_month = split_timep->tm_mon;
        if(NULL != p_day)
            *p_day = split_timep->tm_mday - (S32) 1;
        if(NULL != p_hour)
            *p_hour = split_timep->tm_hour;
        if(NULL != p_minute)
            *p_minute = split_timep->tm_min;
        if(NULL != p_second)
            *p_second = split_timep->tm_sec;
        }

#elif WINDOWS

    SYSTEMTIME systemtime;

    GetLocalTime(&systemtime);

    if(NULL != p_year)
        *p_year = systemtime.wYear - 1;
    if(NULL != p_month)
        *p_month = systemtime.wMonth - 1;
    if(NULL != p_day)
        *p_day = systemtime.wDay - 1;
    if(NULL != p_hour)
        *p_hour = systemtime.wHour;
    if(NULL != p_minute)
        *p_minute = systemtime.wMinute;
    if(NULL != p_second)
        *p_second = systemtime.wSecond;

#else

    time_t today_time = time(NULL);
    struct tm * split_timep = localtime(&today_time);

    if(NULL != p_year)
        *p_year = split_timep->tm_year + 1900 - 1;
    if(NULL != p_month)
        *p_month = split_timep->tm_mon;
    if(NULL != p_day)
        *p_day = split_timep->tm_mday - 1;
    if(NULL != p_hour)
        *p_hour = split_timep->tm_hour;
    if(NULL != p_minute)
        *p_minute = split_timep->tm_min;
    if(NULL != p_second)
        *p_second = split_timep->tm_sec;

#endif
}

extern void
ss_local_time_as_ev_date(
    _InoutRef_  P_EV_DATE p_ev_date)
{
    S32 year, month, day, hour, minute, second;
    ss_local_time(&year, &month, &day, &hour, &minute, &second);
    ss_ymd_to_dateval(p_ev_date, year, month, day);
    ss_hms_to_timeval(p_ev_date, hour, minute, second);
}

extern S32
sliding_window_year(
    _In_        S32 year)
{
    S32 local_year, local_century;

    ss_local_time(&local_year, NULL, NULL, NULL, NULL, NULL);

    local_year += 1; /* back to UI space */

    local_century = (local_year / 100) * 100;

    local_year = local_year - local_century;

#if 1
    /* default to a year within the sliding window of time about 'now' */
    if(year > (local_year + 30))
        year -= 100;
#else
    /* default to current century */
#endif

    year += local_century;

    return(year);
}

/******************************************************************************
*
* work out hours minutes and seconds from a time value
*
******************************************************************************/

extern void
ss_timeval_to_hms(
    _InRef_     PC_EV_DATE p_ev_date,
    _OutRef_    P_S32 p_hour,
    _OutRef_    P_S32 p_minute,
    _OutRef_    P_S32 p_second)
{
    S32 time;

    time      = p_ev_date->time;
    *p_hour   = (S32) (time / 3600);
    time     -= (S32) *p_hour * 3600;
    *p_minute = (S32) (time / 60);
    time     -= *p_minute * 60;
    *p_second = (S32) time;
}

/******************************************************************************
*
* is a string blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
str_hlp_blank(
    _InRef_     PC_EV_DATA p_ev_data)
{
    PC_U8 ptr = p_ev_data->arg.stringc.data;
    S32 len = p_ev_data->arg.stringc.size;

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
str_hlp_dup(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src)
{
    assert(p_ev_data_src);
    /*assert(p_ev_data_src->did_num == RPN_DAT_STRING);*/
    assert(p_ev_data);

    return(str_hlp_make_uchars(p_ev_data, p_ev_data_src->arg.string.data, p_ev_data_src->arg.string.size));
}

/******************************************************************************
*
* make a string
*
******************************************************************************/

_Check_return_
extern STATUS
str_hlp_make_uchars(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_reads_(len) PC_UCHARS uchars,
    _In_        S32 len)
{
    assert(p_ev_data);

    if(len < 0)
    {
        assert(uchars);
        len = strlen32(uchars);
    }

    len = MIN(len, EV_MAX_STRING_LEN); /* SKS 27oct96 limit in any case */

    /*if(0 != len)*/
        {
        STATUS status;
        if(NULL == (p_ev_data->arg.string.data = list_allocptr_p1(len)))
            {
            status = status_nomem();
            data_set_error(p_ev_data, status);
            return(status);
            }
        assert(uchars);
        void_memcpy32(p_ev_data->arg.string.data, uchars, len);
        p_ev_data->arg.string.data[len] = NULLCH;
        p_ev_data->did_num = RPN_TMP_STRING; /* copy is now owned by the caller */
        }
#if 0 /* can't do this in PipeDream - gets transferred to a result and then freed */
    else
        { /* SKS 27oct96 optimise empty strings */
        p_ev_data->arg.string.data = "";
        p_ev_data->did_num = RPN_DAT_STRING;
        }
#endif

    p_ev_data->arg.string.size = len;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
str_hlp_make_ustr(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR ustr)
{
    return(str_hlp_make_uchars(p_ev_data, ustr, ustrlen32(ustr)));
}

/******************************************************************************
*
* given two numbers, reals or integers of various
* sizes, convert them so they are both compatible
*
******************************************************************************/

enum two_num_action
{
    TN_NOP,
    TN_R1,
    TN_R2,
    TN_RB,
    TN_I,
    TN_MIX
};

enum two_num_index
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
        p_ev_data1->arg.fp = (F64) p_ev_data1->arg.integer;
        p_ev_data1->did_num = RPN_DAT_REAL;
        return(TWO_REALS);

    case TN_R2:
        p_ev_data2->arg.fp = (F64) p_ev_data2->arg.integer;
        p_ev_data2->did_num = RPN_DAT_REAL;
        return(TWO_REALS);

    case TN_RB:
        p_ev_data1->arg.fp = (F64) p_ev_data1->arg.integer;
        p_ev_data1->did_num = RPN_DAT_REAL;
        p_ev_data2->arg.fp = (F64) p_ev_data2->arg.integer;
        p_ev_data2->did_num = RPN_DAT_REAL;
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

signed char
ev_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* end of ss_const.c */
