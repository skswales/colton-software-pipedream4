/* ss_const.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Data definitions */

/* MRJC April 1992; SKS derived from Fireworkz for PipeDream use */

#ifndef __ss_const_h
#define __ss_const_h

#define LEAP_YEAR_ACTUAL(year) ( \
    (((year) % 4)   ? 0 : 1) &&                     \
    (((year) % 100) ? 1 : (((year) % 400) ? 0 : 1)) )

/* maximum size of symbol */
#define EV_INTNAMLEN        25
#define BUF_EV_INTNAMLEN    (EV_INTNAMLEN + 1)
#define EV_LONGNAMLEN       (1/*[*/ + MAX_PATHSTRING/*rooted-doc*/ + 1/*]*/ + EV_MAX_RANGE_LEN + 1/*NULLCH*/) /* was 200 */
#define BUF_EV_LONGNAMLEN   (1 + EV_LONGNAMLEN)

/* maximum size of a string value  -
*  this is also the maximum string length
*  of a calculated result of whatever type
*/
#define EV_MAX_STRING_LEN   200
#define BUF_EV_MAX_STRING_LEN (EV_MAX_STRING_LEN + 1)

/* maximum size of expressions in input and output forms */
#define EV_MAX_IN_LEN       1000        /* (textual form) */
#define BUF_EV_MAX_IN_LEN   (EV_MAX_IN_LEN + 1)
#define EV_MAX_OUT_LEN      1000        /* (compiled RPN form) */
#define BUF_EV_MAX_OUT_LEN  (EV_MAX_OUT_LEN + 1)

#if RISCOS || WINDOWS

typedef U8                  EV_DOCNO; typedef EV_DOCNO * P_EV_DOCNO; /* NB same as DOCNO */
#define DOCNO_SIZE          8

typedef int                 EV_COL;
#define EV_COL_BITS         32
#define EV_COL_SBF          SBF

typedef int                 EV_ROW;
#define EV_ROW_BITS         32

typedef unsigned char       EV_IDNO; typedef EV_IDNO * P_EV_IDNO;

#define EV_MAX_COL          0x7FFFFFF
#define EV_MAX_ROW          0x7FFFFFF
#define EV_MAX_COL_LETS     3           /* maximum number of column letters */

#define EV_MAX_ARRAY_ELES   0x8000000   /* maximum number of array elements */

#define EV_MAX_SLR_LEN      (1/*%*/ + 1/*$*/ + 4/*cols*/ + 1/*$*/ + 10/*rows*/)
#define BUF_EV_MAX_SLR_LEN  (EV_MAX_SLR_LEN + 1)

#define EV_MAX_RANGE_LEN    (EV_MAX_SLR_LEN + 1/*:*/ + EV_MAX_SLR_LEN)
#define BUF_EV_MAX_RANGE_LEN (EV_MAX_RANGE_LEN + 1)

typedef unsigned char       EV_FLAGS;
#define FLAGS_SIZE          8

typedef int                 EV_NAMEID; typedef EV_NAMEID * P_EV_NAMEID;

#else
#error Needs sizeof things defining
#endif

/*
evaluator data types
*/

/*
slr type for evaluator
*/

typedef struct EV_SLR
{
    EV_COL      col;
    EV_ROW      row;
    EV_DOCNO    docno;
    EV_FLAGS    flags;
}
EV_SLR, * P_EV_SLR; typedef const EV_SLR * PC_EV_SLR;

/*
packed slr type
*/

#define PACKED_SLRSIZE (sizeof(EV_DOCNO) +  \
                        sizeof(EV_COL)  +  \
                        sizeof(EV_ROW)  +  \
                        sizeof(EV_FLAGS))

/*
range type
*/

typedef struct EV_RANGE
{
    EV_SLR s;
    EV_SLR e;
}
EV_RANGE, * P_EV_RANGE; typedef const EV_RANGE * PC_EV_RANGE;

/*
packed range type
*/

#define PACKED_RNGSIZE (sizeof(EV_DOCNO)     +  \
                        sizeof(EV_COL)   * 2 +  \
                        sizeof(EV_ROW)   * 2 +  \
                        sizeof(EV_FLAGS) * 2)

/*
array structure
*/

typedef struct EV_ARRAY
{
    S32 x_size;                 /* x dimension of array */
    S32 y_size;                 /* y dimension of array */
    struct EV_DATA * elements;
}
EV_ARRAY, * P_EV_ARRAY; typedef const EV_ARRAY * PC_EV_ARRAY;

/*
date/time type
*/

typedef S32 EV_DATE_DATE; typedef EV_DATE_DATE * P_EV_DATE_DATE; typedef const EV_DATE_DATE * PC_EV_DATE_DATE;
#define EV_DATE_INVALID 0 /*S32_MIN*/

typedef S32 EV_DATE_TIME; typedef EV_DATE_TIME * P_EV_DATE_TIME; typedef const EV_DATE_TIME * PC_EV_DATE_TIME;
#define EV_TIME_INVALID 0 /*S32_MIN*/

typedef struct EV_DATE
{
    EV_DATE_DATE date;
    EV_DATE_TIME time;
}
EV_DATE, * P_EV_DATE; typedef const EV_DATE * PC_EV_DATE;

static inline void
ev_date_init(
    _OutRef_    P_EV_DATE p_ev_date)
{
    p_ev_date->date = EV_DATE_INVALID;
    p_ev_date->time = EV_TIME_INVALID;
}

/*
packed date type
*/

#define PACKED_DATESIZE sizeof(S32) + \
                        sizeof(S32)

/*
evaluator error type
*/

typedef struct EV_ERROR
{
    SBF         status : 16; /* packed to keep EV_CONSTANT size down */
    UBF         spare : 8-2;
    UBF         type  : 2;
    UBF         docno : 8; /* packed to keep EV_CONSTANT size down */
    EV_COL      col;
    EV_ROW      row;
}
EV_ERROR, * P_EV_ERROR;

#define ERROR_NORMAL 0
#define ERROR_CUSTOM 1
#define ERROR_PROPAGATED 2

/*
string data (CH_NULL terminated in PipeDream but size field helps)
*/

typedef struct EV_STRING
{
    U32 size;
    P_USTR uchars;
}
EV_STRING, * P_EV_STRING;

typedef struct EV_STRINGC
{
    U32 size;
    PC_USTR uchars;
}
EV_STRINGC, * P_EV_STRINGC;

/*
* evaluator constant type (external mixed data)
* see the dependent EV_DATA type below
*/

typedef union EV_CONSTANT
{
    F64             fp;             /* floating point */
    S32             integer;        /* integer */
    BOOL            boolean;        /* alias: shadows integer */
    EV_STRING       string_wr;      /* string constant (writeable) */
    EV_STRINGC      string;         /* string constant (const) */
    EV_ARRAY        ev_array;       /* array */
    EV_DATE         ev_date;        /* date */
    EV_ERROR        ev_error;       /* error */
}
EV_CONSTANT, * P_EV_CONSTANT;

/*
evaluator mixed data type
*/

typedef union EV_DATA_ARG
{
    F64             fp;             /* floating point */
    S32             integer;        /* integer values */
    BOOL            boolean;        /* alias: shadows integer */
    EV_STRING       string_wr;      /* string (writeable) */
    EV_STRINGC      string;         /* string (const) */
    EV_ARRAY        ev_array;       /* array */
    EV_DATE         ev_date;        /* date */
    EV_ERROR        ev_error;       /* error */

    EV_CONSTANT     ev_constant;    /* all the above, as copied by ev_data_to_result_convert() */

    EV_SLR          slr;            /* slot reference */
    EV_RANGE        range;          /* range */
    EV_NAMEID       nameid;         /* id of named resource */
    S16             cond_pos;       /* position of conditional expression */
}
EV_DATA_ARG;

typedef struct EV_DATA
{
    EV_IDNO did_num;            /* type of result in union SYM_, RPN_*/

    EV_DATA_ARG arg;
}
EV_DATA, * P_EV_DATA; typedef const EV_DATA * PC_EV_DATA;

#if RISCOS
typedef struct RISCOS_TIME_ORDINALS
{
    S32 centiseconds;
    S32 seconds;
    S32 minutes;
    S32 hours;
    S32 day;
    S32 month;
    S32 year;
    S32 day_of_week;
    S32 day_of_year;
}
RISCOS_TIME_ORDINALS;
#endif

/*
ss_const.c
*/

extern const S32 ev_days_in_month[];
extern const S32 ev_days_in_month_leap[];

/*
ss_const.c external functions
*/

/*ncr*/
extern S32
ev_data_set_error(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     STATUS error);

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64);

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64);

extern void
real_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data);

/*ncr*/
extern BOOL
real_to_integer_try(
    _InoutRef_  P_EV_DATA p_ev_data);

extern void
ss_array_free(
    _InoutRef_  P_EV_DATA p_ev_data);

extern STATUS
ss_array_dup(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_in);

_Check_return_
_Ret_
extern PC_EV_DATA
ss_array_element_index_borrow(
    _InRef_     PC_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

_Check_return_
_Ret_
extern P_EV_DATA
ss_array_element_index_wr(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern STATUS
ss_array_element_make(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern void
ss_array_element_read(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern STATUS
ss_array_make(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size);

extern S32
ss_data_compare(
    _InRef_     PC_EV_DATA p_ev_data_1,
    _InRef_     PC_EV_DATA p_ev_data_2);

extern void
ss_data_free_resources(
    _InoutRef_  P_EV_DATA p_ev_data);

extern S32
ss_data_resource_copy(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in);

extern void
ss_date_normalise(
    _InoutRef_  P_EV_DATE datep);

_Check_return_
extern STATUS
ss_dateval_to_ymd(
    _InRef_     PC_EV_DATE_DATE p_ev_date_date,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_year);

/*ncr*/
extern S32
ss_ymd_to_dateval(
    _OutRef_    P_EV_DATE_DATE p_ev_date_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day);

/*ncr*/
extern S32
ss_hms_to_timeval(
    _OutRef_    P_EV_DATE_TIME p_ev_date_time,
    _InVal_     S32 hour,
    _InVal_     S32 minute,
    _InVal_     S32 second);

extern void
ss_local_time(
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_hour,
    _OutRef_    P_S32 p_minute,
    _OutRef_    P_S32 p_second);

extern void
ss_local_time_as_ev_date(
    _OutRef_    P_EV_DATE p_ev_date);

_Check_return_
extern S32
sliding_window_year(
    _In_        S32 year);

_Check_return_
extern STATUS
ss_timeval_to_hms(
    _InRef_     PC_EV_DATE_TIME p_ev_date_time,
    _OutRef_    P_S32 p_hour,
    _OutRef_    P_S32 p_minute,
    _OutRef_    P_S32 p_second);

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src);

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _In_        U32 uchars_n);

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR ustr);

extern S32
two_nums_type_match(
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL size_worry);

enum TWO_NUM_TYPES
{
    TWO_INTS,
    TWO_REALS,
    TWO_MIXED
};

#endif /* __ss_const_h */

/* end of ss_const.h */
