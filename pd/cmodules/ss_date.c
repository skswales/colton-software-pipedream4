/* ss_date.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC April 1992 / August 1993; SKS derived from Fireworkz for PipeDream use */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

#include <time.h> /* for struct tm */

#define SECS_IN_24 ((S32) 60 * (S32) 60 * (S32) 24)
#define FP_SECS_IN_24 (60.0 * 60.0 * 24.0)

/*
days in the month
*/

const S32
ev_days_in_month[12] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const S32
ev_days_in_month_leap[12] =
{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/******************************************************************************
*
* Conversion to / from dateval
*
******************************************************************************/

/******************************************************************************
*
* Convert a date value to a largely Excel-compatible serial number
*
******************************************************************************/

/* Noting that Excel doesn't correctly handle 1900 as a non-leap year
 * and can't represent dates earlier than 1900 at all
 * we start conversion from 01-Jan-1901 which is 367 in Excel files (NB with 1904 mode turned off)
 * In fact this supports compatible dates back to 01-Mar-1900 which is 61 in Excel file
 */

#define BASE_DATE_SERIAL_NUMBER 367 /* Excel serial number of 01-Jan-1901 */

static SS_DATE_DATE
get_base_dateval(void)
{
    static SS_DATE_DATE g_base_dateval = 0;

    if(0 == g_base_dateval)
    {
        (void) ss_ymd_to_dateval(&g_base_dateval, 1901, 1, 1); /* expect 693960 */
        assert(693960 == g_base_dateval);
    }

    return(g_base_dateval);
}

_Check_return_
extern S32
ss_dateval_to_serial_number(
    _InVal_     SS_DATE_DATE ss_date_date)
{
    const SS_DATE_DATE base_dateval = get_base_dateval();
    S32 days_from_base;
    S32 serial_number;

    if(SS_DATE_NULL == ss_date_date)
        return(S32_MIN);

    /* there exists a small range of very large negative datevals that won't fit in an serial number */
    days_from_base = ss_date_date - base_dateval; /* Subtract Fireworkz dateval of base date */

    serial_number = days_from_base + BASE_DATE_SERIAL_NUMBER; /* Add Excel serial number of base date */

    return(serial_number);
}

/* reverse conversion */

_Check_return_ _Success_(status_ok(return))
extern STATUS
ss_serial_number_to_dateval(
    _OutRef_    P_SS_DATE_DATE p_ss_date_date,
    _InRef_     F64 serial_number)
{
    const SS_DATE_DATE base_dateval = get_base_dateval();
    S32 days_from_base;
    SS_DATA ss_data;

    ss_data_set_real(&ss_data, serial_number);

    if(!ss_data_real_to_integer_try(&ss_data))
        return(EVAL_ERR_ARGRANGE);

    /* there exists a small (693960-367) range of very large serial numbers (>2146790054) that won't fit in a dateval */
    assert(base_dateval >= BASE_DATE_SERIAL_NUMBER);
    if(ss_data_get_integer(&ss_data) > S32_MAX - (base_dateval - BASE_DATE_SERIAL_NUMBER))
        return(EVAL_ERR_ARGRANGE);

    days_from_base = /*serial_number*/ ss_data_get_integer(&ss_data) - BASE_DATE_SERIAL_NUMBER; /* Subtract Excel serial number of base date */

    *p_ss_date_date = days_from_base + base_dateval; /* Add Fireworkz dateval of base date */

    return(STATUS_OK);
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
    _InVal_     SS_DATE_DATE ss_date_date,
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day)
{
    S32 date = ss_date_date;
    S32 adjyear = 0;

    /* Fireworkz serial number zero == basis date 1.1.0001 */
    *p_year = 1;

    if(SS_DATE_NULL == date)
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
     * 31.12.1600,2000,2400 etc. will fail, so we may as well unroll the loop...
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
    SS_DATE_DATE test_date;
    ss_ymd_to_dateval(&test_date, *p_year, *p_month, *p_day);
    assert(test_date == ss_date_date);
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
    _OutRef_    P_SS_DATE_DATE p_ss_date_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day)
{
    /* convert certain actual values (1..31,1..n,0001..9999) to index values */
    S32 month_index = month - 1;
    S32 adjdate = 0;

    *p_ss_date_date = 0;

    /* transfer excess months to the years component */
    if(month_index < 0)
    {
        year += (month_index - 12) / 12;
        month_index = 12 + (month_index % 12); /* 12 + (-11,-10,..,-1,0) => 0..11 : month_index now positive */
        month = month_index + 1; /* reduces month into 1..12 */
        UNREFERENCED_LOCAL_VARIABLE(month); /* useful for debug */
    }
    else if(month_index > 11)
    {
        year += (month_index / 12);
        month_index = (month_index % 12); /* reduce month_index into 0..11 */
        month = month_index + 1; /* reduces month into 1..12 */
        UNREFERENCED_LOCAL_VARIABLE(month);
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
            *p_ss_date_date = SS_DATE_NULL;
            return(-1);
        }
#else
        /* fixed bias covers all BCE dates in the Holocene */
        adjdate = (DAYS_IN_400 * 25);
        year += (400 * 25);

        if(year <= 0)
        {
            *p_ss_date_date = SS_DATE_NULL;
            return(-1);
        }
#endif
    }

    /* get number of days between basis date 1.1.0001 and the start of this year - i.e. consider *previous* years */
    assert(year >= 1);
    {
        const S32 year_minus_1 = year - 1;

        *p_ss_date_date = (year_minus_1 * 365)
                        + (year_minus_1 / 4)
                        - (year_minus_1 / 100)
                        + (year_minus_1 / 400);

        if(*p_ss_date_date < 0)
        {
            *p_ss_date_date = SS_DATE_NULL;
            return(-1);
        }
    }

    /* get number of days between the start of this year and the start of this month - i.e. consider *previous* months */
    {
    const PC_S32 p_days_in_month = (LEAP_YEAR_ACTUAL(year) ? ev_days_in_month_leap : ev_days_in_month);
    S32 i;

    for(i = 0; i < month_index; ++i)
    {
        *p_ss_date_date += p_days_in_month[i];
    }
    } /*block*/

    /* get number of days between the start of this month and the start of this day - i.e. consider *previous* days */
    *p_ss_date_date += (day - 1);

    if(adjdate)
        *p_ss_date_date -= adjdate;

    return(0);
}

/******************************************************************************
*
* Conversion to / from timeval
*
******************************************************************************/

/******************************************************************************
*
* return Excel-compatible fraction of a day
*
******************************************************************************/

_Check_return_
extern F64
ss_timeval_to_serial_fraction(
    _InVal_     SS_DATE_TIME ss_date_time)
{
    if(SS_TIME_NULL == ss_date_time)
        return(0.0);

    return((F64) ss_date_time / FP_SECS_IN_24);
}

/* reverse conversion */

extern void
ss_serial_fraction_to_timeval(
    _OutRef_    P_SS_DATE_TIME p_ss_date_time,
    _InVal_     F64 serial_fraction)
{
    *p_ss_date_time = (SS_DATE_TIME) (serial_fraction * FP_SECS_IN_24);
}

/******************************************************************************
*
* work out hours minutes and seconds from a time value
*
******************************************************************************/

_Check_return_
extern STATUS
ss_timeval_to_hms(
    _InVal_     SS_DATE_TIME ss_date_time,
    _OutRef_    P_S32 p_hours,
    _OutRef_    P_S32 p_minutes,
    _OutRef_    P_S32 p_seconds)
{
    S32 time = ss_date_time;

    if(SS_TIME_NULL == time)
    {
        *p_hours = *p_minutes = *p_seconds = 0;
#if SS_TIME_NULL != 0
        return(EVAL_ERR_NOTIME);
#else
        return(STATUS_OK);
#endif
    }

    *p_hours   = (S32) (time / 3600);
    time      -= (S32) *p_hours * 3600;

    *p_minutes = (S32) (time / 60);
    time      -= *p_minutes * 60;

    *p_seconds = (S32) time;

    return(STATUS_OK);
}

/******************************************************************************
*
* convert a number of hours, minutes and seconds to a timeval
*
******************************************************************************/

/*ncr*/
extern S32
ss_hms_to_timeval(
    _OutRef_    P_SS_DATE_TIME p_ss_date_time,
    _InVal_     S32 hours,
    _InVal_     S32 minutes,
    _InVal_     S32 seconds)
{
    /* check hours is in range */
    if((S32) labs((long) hours) >= S32_MAX / 3600)
    {
        *p_ss_date_time = SS_TIME_NULL;
        return(-1);
    }

    /* check minutes is in range */
    if((S32) labs((long) minutes) >= S32_MAX / 60)
    {
        *p_ss_date_time = SS_TIME_NULL;
        return(-1);
    }

    *p_ss_date_time = ((S32) hours * 3600) + (minutes * 60) + seconds;

    return(0);
}

/******************************************************************************
*
* compare two date/time velues
*
* --out--
* -1 p_ss_date_1 < p_ss_date_2
*  0 p_ss_date_1 = p_ss_date_2
* +1 p_ss_date_1 > p_ss_date_2
*
******************************************************************************/

_Check_return_
extern S32
ss_date_compare(
    _InRef_     PC_SS_DATE p_ss_date_1,
    _InRef_     PC_SS_DATE p_ss_date_2)
{
    S32 res;

    if(p_ss_date_1->date < p_ss_date_2->date)
        res = -1;
    else if(p_ss_date_1->date > p_ss_date_2->date)
        res = 1;
    else
    {   /* date components are equal; compare time components */
        if(p_ss_date_1->time < p_ss_date_2->time)
            res = -1;
        else if(p_ss_date_1->time > p_ss_date_2->time)
            res = 1;
        else
            res = 0;
    }

    return(res);
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
    _InoutRef_  P_SS_DATE p_ss_date)
{
    if(SS_DATE_NULL == p_ss_date->date)
    {   /* allow hours >= 24 etc. */
        return;
    }

    if(SS_TIME_NULL == p_ss_date->time)
        return;

    if((p_ss_date->time >= SECS_IN_24) || (p_ss_date->time < 0))
    {
        S32 days = p_ss_date->time / SECS_IN_24;

        p_ss_date->date += days;
        p_ss_date->time -= days * SECS_IN_24;

        if(p_ss_date->time < 0)
        {
            p_ss_date->date -= 1;
            p_ss_date->time += SECS_IN_24;
        }
    }
}

/******************************************************************************
*
* Convert a date and time value to a largely Excel-compatible serial number
*
******************************************************************************/

_Check_return_
extern F64
ss_date_to_serial_number(
    _InRef_     PC_SS_DATE p_ss_date)
{
    F64 f64 = 0.0;

    if(SS_DATE_NULL != p_ss_date->date)
    {   /* Convert the date component to a largely Excel-compatible serial number */
        S32 serial_number = ss_dateval_to_serial_number(p_ss_date->date);

        /* Whilst Excel can't represent dates earlier than 1900 at all (and doesn't correctly handle 1900 as non-leap year), that doesn't stop us from handling them in Fireworkz */
        f64 = (F64) serial_number;
    }

    if(SS_TIME_NULL != p_ss_date->time)
    {
        f64 += ss_timeval_to_serial_fraction(p_ss_date->time);
    }

    return(f64);
}

/* reverse conversion */

_Check_return_ _Success_(status_ok(return))
extern STATUS
ss_serial_number_to_date(
    _OutRef_    P_SS_DATE p_ss_date,
    _InVal_     F64 serial_number)
{
    F64 serial_integer;
    F64 serial_fraction = modf(serial_number, &serial_integer);

    ss_date_init(p_ss_date);

    /* happy in Fireworkz to have time-only date values */
    if(0.0 != serial_integer)
        status_return(ss_serial_number_to_dateval(&p_ss_date->date, serial_integer));

    if(0.0 != serial_fraction)
        ss_serial_fraction_to_timeval(&p_ss_date->time, serial_fraction);

    return(STATUS_OK);
}

/******************************************************************************
*
* read the current time as actual values
*
******************************************************************************/

extern void
ss_local_time_as_ymd_hms(
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_hours,
    _OutRef_    P_S32 p_minutes,
    _OutRef_    P_S32 p_seconds)
{
#if RISCOS

    /* localtime doesn't work well on RISCOS */
    RISCOS_TIME_ORDINALS time_ordinals;
    _kernel_swi_regs rs;
    TCHARZ buffer[8];

    rs.r[0] = 14;
    rs.r[1] = (int) buffer;
    buffer[0] = 3 /*OSWord_ReadUTC*//*1*/;
    void_WrapOsErrorChecking(_kernel_swi(OS_Word, &rs, &rs));

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) buffer;
    rs.r[2] = (int) &time_ordinals;

    if(NULL == WrapOsErrorChecking(_kernel_swi(Territory_ConvertTimeToOrdinals, &rs, &rs)))
    {
        *p_year = time_ordinals.year;
        *p_month = time_ordinals.month;
        *p_day = time_ordinals.day;

        *p_hours = time_ordinals.hours;
        *p_minutes = time_ordinals.minutes;
        *p_seconds = time_ordinals.seconds;
    }
    else
    {
        time_t today_time = time(NULL);
        struct tm * split_timep = localtime(&today_time);

        *p_year = split_timep->tm_year + (S32) 1900;
        *p_month = split_timep->tm_mon + (S32) 1;
        *p_day = split_timep->tm_mday;

        *p_hours = split_timep->tm_hour;
        *p_minutes = split_timep->tm_min;
        *p_seconds = split_timep->tm_sec;
    }

#elif WINDOWS

    SYSTEMTIME systemtime;

    GetLocalTime(&systemtime);

    *p_year = systemtime.wYear;
    *p_month = systemtime.wMonth;
    *p_day = systemtime.wDay;

    *p_hours = systemtime.wHour;
    *p_minutes = systemtime.wMinute;
    *p_seconds = systemtime.wSecond;

#else

    time_t today_time = time(NULL);
    struct tm * split_timep = localtime(&today_time);

    *p_year = split_timep->tm_year + 1900;
    *p_month = split_timep->tm_mon + 1;
    *p_day = split_timep->tm_mday;

    *p_hours = split_timep->tm_hour;
    *p_minutes = split_timep->tm_min;
    *p_seconds = split_timep->tm_sec;

#endif /* OS */
}

extern void
ss_date_set_from_local_time(
    _OutRef_    P_SS_DATE p_ss_date)
{
    S32 year, month, day;
    S32 hours, minutes, seconds;
    ss_local_time_as_ymd_hms(&year, &month, &day, &hours, &minutes, &seconds);
    (void) ss_ymd_to_dateval(&p_ss_date->date, year, month, day);
    (void) ss_hms_to_timeval(&p_ss_date->time, hours, minutes, seconds);
}

_Check_return_
extern S32 /* actual year */
sliding_window_year(
    _In_        S32 year)
{
    S32 modified_year = year;
    S32 local_year, local_century;
    S32 month, day;
    S32 hours, minutes, seconds;

    ss_local_time_as_ymd_hms(&local_year, &month, &day, &hours, &minutes, &seconds);

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
* convert date/time value to a string
*
******************************************************************************/

_Check_return_
extern S32
ss_date_decode(
    P_U8 op_buf,
    _InRef_     PC_SS_DATE p_ss_date,
    _InVal_     bool american_date)
{
    SS_DATE ss_date = *p_ss_date;
    S32 len = 0;

#if 0 /* just for diff minimization */
    if((SS_DATE_NULL == ss_date.date) && (SS_TIME_NULL == ss_date.time))
        return(ERR_DATE_TIME_INVALID);
#endif

    if(SS_DATE_NULL != ss_date.date)
    {
        S32 year, month, day;

        if(ss_dateval_to_ymd(ss_date.date, &year, &month, &day) < 0) /* year may be -ve without any special case handling here */
        {
            len += sprintf(op_buf + len,
                            "%s.%s.%s",
                            "**",
                            "**",
                            "****");
        }
        else if(american_date)
        {
            len += sprintf(op_buf + len,
                            "%"   S32_FMT_POSTFIX "."
                            "%"   S32_FMT_POSTFIX "."
                            "%.4" S32_FMT_POSTFIX,
                            month, day, year);
        }
        else
        {
            len += sprintf(op_buf + len,
                            "%.2" S32_FMT_POSTFIX "."
                            "%.2" S32_FMT_POSTFIX "."
                            "%.4" S32_FMT_POSTFIX,
                            day, month, year);
        }

        /* separate time from date */
        if(SS_TIME_NULL != ss_date.time)
            op_buf[len++] = CH_SPACE;
    }

    if((SS_DATE_NULL == ss_date.date) || (SS_TIME_NULL != ss_date.time))
    {
        S32 hours, minutes, seconds;
        BOOL negative = FALSE;

        if(ss_date.time < 0)
        {
            ss_date.time = -ss_date.time;
            negative = TRUE;
        }

        status_assert(ss_timeval_to_hms(ss_date.time, &hours, &minutes, &seconds));

        if(negative)
            op_buf[len++] = CH_MINUS_SIGN__BASIC; /* apply minus sign in front of most significant part of output */

        len += sprintf(op_buf + len,
                        "%.2" S32_FMT_POSTFIX ":"
                        "%.2" S32_FMT_POSTFIX ":"
                        "%.2" S32_FMT_POSTFIX,
                        hours, minutes, seconds);
    }

    return(len);
}

/******************************************************************************
*
* read a three part date value
*
******************************************************************************/

_Check_return_
static U32
recog_dmy_date(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_year,
    _InVal_     bool american_date)
{
    PC_USTR pos = spos;
    PC_USTR epos;
    S32 scan_val;
    U32 scanned;
    U8 date_sep_char = '.';
    BOOL negative_year = FALSE;

    *p_day = *p_month = *p_year = 0;

    while(' ' == *pos++)
    { /*EMPTY*/ }
    if(CH_NULL == *--pos)
        return(0);

    /* scan day */
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val = (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);

    if(sbchar_isdigit(PtrGetByte(pos)))
    {   /* only one or two digits allowed in a number of days */
        scan_val *= 10;
        scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
        ustr_IncByte(pos);
    }

    if((scan_val < 1) || (scan_val > (american_date ? 12 : 31)))
        return(0);

    if(PtrGetByte(pos) == date_sep_char)
        ustr_IncByte(pos);
    else
    {
        if(CH_NULL == (date_sep_char = '/' /*alternate_date_sep_char*/))
            return(0);
        if(PtrGetByte(pos) == date_sep_char)
            ustr_IncByte(pos);
        else
            return(0);
    }

    *p_day = scan_val;

    /* scan month */
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val = (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);

    if(sbchar_isdigit(PtrGetByte(pos)))
    {   /* only one or two digits allowed in a month */
        scan_val *= 10;
        scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
        ustr_IncByte(pos);
    }

    if((scan_val < 1) || (scan_val > (american_date ? 31 : 12)))
        return(0);

    if(PtrGetByte(pos) != date_sep_char) /* ensure same date sep char matched */
        return(0);

    ustr_IncByte(pos);

    if(american_date)
    {
        *p_month = *p_day;
        *p_day = scan_val;
    }
    else
        *p_month = scan_val;

    /* scan year */
    if(CH_MINUS_SIGN__BASIC == PtrGetByte(pos)) /* SKS 20191203 allow negative years here */
    {
        negative_year = TRUE;
        ustr_IncByte(pos);
    }
    else if(CH_PLUS_SIGN == PtrGetByte(pos))
        ustr_IncByte(pos);
    scan_val = (S32) fast_ustrtoul(pos, &epos);
    scanned = PtrDiffBytesU32(epos, pos);
    if(0 == scanned)
        return(0);
    pos = epos;

    if(scan_val < 0)
        return(0); /* overflow in number */

    if((scan_val >= 0) && (scan_val < 100) && (scanned <= 2)) /* SKS 20191203 allows you to enter 0095 as a date for completeness (and 095 too). */
        scan_val = sliding_window_year(scan_val);

    if(negative_year)
        scan_val = -scan_val;

    *p_year = scan_val;

    return(PtrDiffBytesU32(pos, spos));
}

/*
ISO 8601 extended format: YYYY-MM-DD
*/

_Check_return_
static U32
recog_iso_date(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_year)
{
    PC_USTR pos = spos;
    PC_USTR epos;
    S32 scan_val;
    U32 scanned;
    U8 date_sep_char = CH_HYPHEN_MINUS;
    BOOL negative_year = FALSE;
    BOOL signed_year = FALSE;

    *p_day = *p_month = *p_year = 0;

    /* scan year */
    if(CH_MINUS_SIGN__BASIC == PtrGetByte(pos))
    {
        negative_year = TRUE;
        signed_year = TRUE;
        ustr_IncByte(pos);
    }
    else if(CH_PLUS_SIGN == PtrGetByte(pos))
    {
        signed_year = TRUE;
        ustr_IncByte(pos);
    }
    scan_val = (S32) fast_ustrtoul(pos, &epos);
    scanned = PtrDiffBytesU32(epos, pos);
    /* four digits required in an ISO year unless signed, in which case we will accept more */
    if((4 != scanned) && (!signed_year || (scanned < 5)))
        return(0);
    pos = epos;

    if(scan_val < 0)
        return(0); /* overflow in number */

    if(negative_year)
        scan_val = -scan_val;

    /* ensure that we are now pointing to date sep char */
    if(PtrGetByte(pos) != date_sep_char)
        return(0);
    else
        ustr_IncByte(pos);

    *p_year = scan_val;

    /* scan month */
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val = (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);

    /* two digits are required in an ISO month */
#if 1
    /* some CSV files are seen to only have one */
    if(sbchar_isdigit(PtrGetByte(pos)))
    {
        scan_val *= 10;
        scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
        ustr_IncByte(pos);
    }
    else if(PtrGetByte(pos) != date_sep_char)
        return(0);
#else
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val *= 10;
    scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);
#endif

    if((scan_val < 1) || (scan_val > 12))
        return(0);

    if(PtrGetByte(pos) != date_sep_char) /* ensure same date sep char matched */
        return(0);

    ustr_IncByte(pos);

    *p_month = scan_val;

    /* scan day */
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val = (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);

    /* two digits are required in an ISO day */
#if 1
    /* some CSV files are seen to only have one */
    if(sbchar_isdigit(PtrGetByte(pos)))
    {
        scan_val *= 10;
        scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
        ustr_IncByte(pos);
    }
#else
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val *= 10;
    scan_val += (S32) PtrGetByte(pos) - CH_DIGIT_ZERO;
    ustr_IncByte(pos);
#endif

    if((scan_val < 1) || (scan_val > 31))
        return(0);

    *p_day = scan_val;

    return(PtrDiffBytesU32(pos, spos));
}

/******************************************************************************
*
* read a two or three part time value
*
******************************************************************************/

_Check_return_
static U32
recog_time(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 p_hours,
    _OutRef_    P_S32 p_minutes,
    _OutRef_    P_S32 p_seconds)
{
    PC_USTR pos = spos;
    PC_USTR epos;
    S32 scan_val;
    U32 scanned;
    const U8 time_sep_char = ':';
    BOOL negative_hours = FALSE;

    *p_hours = *p_minutes = *p_seconds = 0;

    while(' ' == *pos++)
    { /*EMPTY*/ }
    if(CH_NULL == *--pos)
        return(0);

    /* scan hours part */
    if(CH_MINUS_SIGN__BASIC == PtrGetByte(pos))
    {
        ustr_IncByte(pos);
        negative_hours = TRUE;
    }
    else if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    scan_val = (S32) fast_ustrtoul(pos, &epos);
    scanned = PtrDiffBytesU32(epos, pos);
    if(0 == scanned)
        return(0);
    pos = epos;

    if(scan_val < 0)
        return(0); /* overflow in number */

    if(PtrGetByte(pos) != time_sep_char) /* must have minutes part to be recognisable as time */
        return(0);
    ustr_IncByte(pos);

    if(negative_hours)
        scan_val = -scan_val;

    *p_hours = scan_val;

    /* scan minutes part */
    scan_val = (S32) fast_ustrtoul(pos, &epos);
    scanned = PtrDiffBytesU32(epos, pos);
    if(0 == scanned)
        return(0);
    pos = epos;

    if(scan_val < 0)
        return(0); /* overflow in number */

    *p_minutes = scan_val;

    /* scan seconds part */
    if(PtrGetByte(pos) == time_sep_char) /* seconds are optional */
    {
        ustr_IncByte(pos);

        scan_val = (S32) fast_ustrtoul(pos, &epos);
        scanned = PtrDiffBytesU32(epos, pos);
        if(0 == scanned)
            return(0);
        pos = epos;

        if(scan_val < 0)
            return(0); /* overflow in number */

        *p_seconds = scan_val;
    }

    return(PtrDiffBytesU32(pos, spos));
}

/******************************************************************************
*
* try to recognise date or time
*
* --out--
* =0 no date or time found
* >0 # chars scanned
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern S32
ss_recog_date_time(
    _InoutRef_  P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str,
    _InVal_     bool american_date)
{
    S32 day, month, year;
    U32 date_scanned;
    S32 hours, minutes, seconds;
    U32 time_scanned;
    PC_USTR pos = in_str;

    ss_date_init(&p_ss_data->arg.ss_date);

    /* check for a date */
    date_scanned = recog_dmy_date(pos, &day, &month, &year, american_date);

    if((0 == date_scanned) && TRUE /* extra parsing allowed? */)
        date_scanned = recog_iso_date(pos, &day, &month, &year);

    if(0 != date_scanned)
    {
        S32 tres;

        tres = ss_ymd_to_dateval(&p_ss_data->arg.ss_date.date, year, month, day);

        if(tres >= 0)
        {
            ustr_IncBytes(pos, date_scanned);

            ss_data_set_data_id(p_ss_data, DATA_ID_DATE);

            if(CH_SPACE == PtrGetByte(pos))
            {
                ustr_SkipSpaces(pos);

                if(CH_NULL == PtrGetByte(pos))
                    return(PtrDiffBytesS32(pos, in_str)); /* just a date part */
            }
            else if('T' == PtrGetByte(pos)) /* ISO 8601 */
                ustr_IncByte(pos);

            /* go on to consider a time part */
        }
        else
            p_ss_data->arg.ss_date.date = SS_DATE_NULL;
    }

    /* check for a time */
    time_scanned = recog_time(pos, &hours, &minutes, &seconds);

    if(0 != time_scanned)
    {
        if(ss_hms_to_timeval(&p_ss_data->arg.ss_date.time, hours, minutes, seconds) >= 0)
        {
            ustr_IncBytes(pos, time_scanned);

            ss_data_set_data_id(p_ss_data, DATA_ID_DATE);
        }
        else
            p_ss_data->arg.ss_date.time = SS_TIME_NULL;

        ss_date_normalise(&p_ss_data->arg.ss_date);
    }

    return(PtrDiffBytesS32(pos, in_str));
}

/* end of ss_date.c */
