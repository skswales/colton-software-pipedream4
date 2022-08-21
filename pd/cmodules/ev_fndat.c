/* ev_fndat.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Date and time function routines for evaluator */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "version_x.h"

#include "kernel.h" /*C:*/

#include "swis.h" /*C:*/

/******************************************************************************
*
* Date and time functions
*
******************************************************************************/

/******************************************************************************
*
* REAL age(date1, date2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_age)
{
    F64 age_result;
    STATUS status;
    EV_DATE ev_date;
    S32 year, month, day;

    exec_func_ignore_parms();

    if((EV_DATE_NULL != args[0]->arg.ev_date.date) && (EV_DATE_NULL != args[1]->arg.ev_date.date))
    {
        ev_date.date = args[0]->arg.ev_date.date - args[1]->arg.ev_date.date;
        ev_date.time = args[0]->arg.ev_date.time - args[1]->arg.ev_date.time;
        ss_date_normalise(&ev_date);
    }
    else
    {
        ev_date_init(&ev_date);
    }

    if(status_fail(status = ss_dateval_to_ymd(&ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    age_result = ((F64) year - 1) + ((F64) month - 1) * 0.01;

    ev_data_set_real(p_ev_data_res, age_result);
}

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

#if (RELEASED || 1) /* you may set the one to zero temporarily for testing Fireworkz serial numbers for years < 0100 in Debug build */
    if((our_year >= 0) && (our_year < 100))
        our_year = sliding_window_year(our_year);
#endif

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date.time = EV_TIME_NULL;

    if(ss_ymd_to_dateval(&p_ev_data_res->arg.ev_date.date, our_year, month, day) < 0)
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
    memcpy32(buffer, args[0]->arg.string.uchars, len);
    buffer[len] = CH_NULL;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || (EV_DATE_NULL == p_ev_data_res->arg.ev_date.date))
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
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    day_result = day;

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
        if(EV_DATE_NULL == args[0]->arg.ev_date.date)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
            return;
        }
        day = ((args[0]->arg.ev_date.date + 1) % 7) + 1;
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
        *(p_ev_data_res->arg.string_wr.uchars) = (U8) toupper(*(p_ev_data_res->arg.string.uchars));
}

/******************************************************************************
*
* INTEGER return hour number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_hour)
{
    S32 hour_result;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds);

    hour_result = hours;

    ev_data_set_integer(p_ev_data_res, hour_result);
}

/******************************************************************************
*
* INTEGER return minute number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_minute)
{
    S32 minute_result;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds);

    minute_result = minutes;

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
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    month_result = month;

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
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    monthdays_result = (LEAP_YEAR_ACTUAL(year) ? ev_days_in_month_leap[month - 1] : ev_days_in_month[month - 1]);

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
    S32 month_idx = 0;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        {
        STATUS status;
        S32 year, month, day;
        if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
        {
            ev_data_set_error(p_ev_data_res, status);
            return;
        }
        month_idx = month - 1;
        break;
        }

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        month_idx = MAX(0, args[0]->arg.integer - 1);
        month_idx = month_idx % 12;
        break;

    default: default_unhandled(); break;
    }

    if(status_ok(ss_string_make_ustr(p_ev_data_res, ss_month_names[month_idx])))
        *(p_ev_data_res->arg.string_wr.uchars) = (U8) toupper(*(p_ev_data_res->arg.string.uchars));
}

/******************************************************************************
*
* DATE now
*
******************************************************************************/

PROC_EXEC_PROTO(c_now)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    ss_local_time_as_ev_date(&p_ev_data_res->arg.ev_date);
}

/******************************************************************************
*
* INTEGER return second number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_second)
{
    S32 second_result;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds);

    second_result = seconds;

    ev_data_set_integer(p_ev_data_res, second_result);
}

/******************************************************************************
*
* DATE time(hours, minutes, seconds)
*
******************************************************************************/

PROC_EXEC_PROTO(c_time)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date.date = EV_DATE_NULL;

    if(ss_hms_to_timeval(&p_ev_data_res->arg.ev_date.time,
                         args[0]->arg.integer,
                         args[1]->arg.integer,
                         args[2]->arg.integer) < 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    ss_date_normalise(&p_ev_data_res->arg.ev_date);
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
    memcpy32(buffer, args[0]->arg.string.uchars, len);
    buffer[len] = CH_NULL;

    if((ss_recog_date_time(p_ev_data_res, buffer, 0) < 0) || (EV_DATE_NULL != p_ev_data_res->arg.ev_date.date))
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

    c_now(args, n_args, p_ev_data_res, p_cur_slr);

    if(RPN_DAT_DATE == p_ev_data_res->did_num)
        p_ev_data_res->arg.ev_date.time = EV_TIME_NULL;
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

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    weekday = (args[0]->arg.ev_date.date + 1) % 7;

    ev_data_set_integer(p_ev_data_res, weekday + 1);
}

/******************************************************************************
*
* INTEGER return the week number for a date
*
******************************************************************************/

#if RISCOS

typedef struct FIVEBYTE
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
    STATUS status;
    S32 year, month, day;
    RISCOS_TIME_ORDINALS time_ordinals;
    _kernel_swi_regs rs;

    if(status_fail(status = ss_dateval_to_ymd(p_ev_date_date, &year, &month, &day)))
    {
        zero_struct_ptr(p_fivebyte);
        return(status);
    }

    zero_struct(time_ordinals);
    time_ordinals.year = year;
    time_ordinals.month = month;
    time_ordinals.day = day;

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

    if(status_ok(status = fivebytetime_from_date(&fivebyte, &args[0]->arg.ev_date.date)))
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
        tm.tm_year = (int) (year - 1900);
        tm.tm_mon = (int) (month - 1);
        tm.tm_mday = (int) day;

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
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    year_result = year;

    ev_data_set_integer(p_ev_data_res, year_result);
}

/* end of ev_fndat.c */
