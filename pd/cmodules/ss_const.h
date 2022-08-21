/* ss_const.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Data definitions */

/* MRJC April 1992 */

/* SKS derived from Fireworkz for PipeDream use */

#ifndef __ss_const_h
#define __ss_const_h

#define LEAP_YEAR_ACTUAL(year) ( \
    (((year    ) % 4  ) ? 0 : 1) && \
    (((year    ) % 100) ? 1 : (((year    ) % 400) ? 0 : 1)) )

/* maximum size of symbol */
#define EV_INTNAMLEN        25
#define BUF_EV_INTNAMLEN    (EV_INTNAMLEN + 1)
#define EV_LONGNAMLEN       (1/*[*/ + MAX_PATHSTRING/*rooted-doc*/ + 1/*]*/ + EV_MAX_RANGE_LEN + 1/*CH_NULL*/) /* was 200 */
#define BUF_EV_LONGNAMLEN   (EV_LONGNAMLEN + 1)

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
#define EV_DOCNO_BITS       8

typedef int                 EV_COL;
#define EV_COL_BITS         32
#define EV_COL_SBF          SBF

typedef int                 EV_ROW;
#define EV_ROW_BITS         32

typedef unsigned char       EV_IDNO; typedef EV_IDNO * P_EV_IDNO;

#define EV_MAX_COL          0x07FFFFFF
#define EV_MAX_ROW          0x07FFFFFF
#define EV_MAX_COL_LETS     3           /* maximum number of column letters */

#define EV_MAX_ARRAY_ELES   0x08000000  /* maximum number of array elements */

#define EV_MAX_SLR_LEN      (1/*%*/ + 1/*$*/ + 4/*cols*/ + 1/*$*/ + 10/*rows*/)
#define BUF_EV_MAX_SLR_LEN  (EV_MAX_SLR_LEN + 1)

#define EV_MAX_RANGE_LEN    (EV_MAX_SLR_LEN + 1/*:*/ + EV_MAX_SLR_LEN)
#define BUF_EV_MAX_RANGE_LEN (EV_MAX_RANGE_LEN + 1)

typedef unsigned char       EV_FLAGS;
#define EV_FLAGS_BITS       8
#define EV_FLAGS_INIT       0

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

#define EV_SLR_INIT { 0 /*...*/ } /* this aggregate initialiser gives poor code on ARM Norcroft */

#define ev_slr_init(p_ev_slr) \
    zero_struct_ptr(p_ev_slr)

#define ev_slr_docno(p_ev_slr) ( \
    (p_ev_slr)->docno )

#define ev_slr_col(p_ev_slr) ( \
    (p_ev_slr)->col )

#define ev_slr_row(p_ev_slr) ( \
    (p_ev_slr)->row )

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

#define ev_range_init(p_ev_range) \
    zero_struct_ptr(p_ev_range)

/*
packed range type
*/

#define PACKED_RNGSIZE (sizeof(EV_DOCNO)     +  \
                        sizeof(EV_COL)   * 2 +  \
                        sizeof(EV_ROW)   * 2 +  \
                        sizeof(EV_FLAGS) * 2)

/*
date/time type
*/

typedef S32 SS_DATE_DATE; typedef SS_DATE_DATE * P_SS_DATE_DATE; typedef const SS_DATE_DATE * PC_SS_DATE_DATE;
#define SS_DATE_NULL 0 /*S32_MIN*/

typedef S32 SS_DATE_TIME; typedef SS_DATE_TIME * P_SS_DATE_TIME; typedef const SS_DATE_TIME * PC_SS_DATE_TIME;
#define SS_TIME_NULL 0 /*S32_MIN*/

typedef struct SS_DATE
{
    SS_DATE_DATE date;
    SS_DATE_TIME time;
}
SS_DATE, * P_SS_DATE; typedef const SS_DATE * PC_SS_DATE;

static inline void
ss_date_init(
    _OutRef_    P_SS_DATE p_ss_date)
{
    p_ss_date->date = SS_DATE_NULL;
    p_ss_date->time = SS_TIME_NULL;
}

/*
packed date type
*/

#define PACKED_DATESIZE sizeof(S32) + \
                        sizeof(S32)

/*
evaluator error type
*/

typedef struct SS_ERROR
{
    SBF         status : 16; /* packed to keep SS_CONSTANT size down */
    UBF         spare : 8-2;
    UBF         type  : 2;
    UBF         docno : 8; /* packed to keep SS_CONSTANT size down */
    EV_COL      col;
    EV_ROW      row;
}
SS_ERROR, * P_SS_ERROR;

#define ERROR_NORMAL 0
#define ERROR_CUSTOM 1
#define ERROR_PROPAGATED 2

/*
array structure
*/

typedef struct SS_ARRAY
{
    S32 x_size;                 /* x dimension of array */
    S32 y_size;                 /* y dimension of array */
    struct SS_DATA * elements;
}
SS_ARRAY, * P_SS_ARRAY; typedef const SS_ARRAY * PC_SS_ARRAY;

/*
string data (CH_NULL terminated in PipeDream but size field helps)
*/

typedef struct SS_STRING
{
    U32 size;
    P_USTR uchars;
}
SS_STRING, * P_SS_STRING;

typedef struct SS_STRINGC
{
    U32 size;
    PC_USTR uchars;
}
SS_STRINGC, * P_SS_STRINGC; typedef const SS_STRINGC * PC_SS_STRINGC;

/*
* evaluator constant type (external mixed data)
* see the dependent SS_DATA type below
*/

typedef union SS_CONSTANT
{
    F64             fp;             /* floating point */
    S32             integer;        /* integer */
    U32             logical_integer; /* alias: shadows integer */
    SS_STRING       string_wr;      /* string constant (writeable) */
    SS_STRINGC      string;         /* string constant (const) */
    SS_ARRAY        ss_array;       /* array */
    SS_DATE         ss_date;        /* date */
    SS_ERROR        ss_error;       /* error */
}
SS_CONSTANT, * P_SS_CONSTANT;

/*
evaluator mixed data type
*/

typedef union SS_DATA_ARG
{
    F64             fp;             /* floating point */
    S32             integer;        /* integer values */
    bool            logical_bool;
    U32             logical_integer; /* alias: shadows integer */
    SS_STRING       string_wr;      /* string (writeable) */
    SS_STRINGC      string;         /* string (const) */
    SS_ARRAY        ss_array;       /* array */
    SS_DATE         ss_date;        /* date */
    SS_ERROR        ss_error;       /* error */

    SS_CONSTANT     ss_constant;    /* all the above, as copied by ss_data_to_result_convert() */

    EV_SLR          slr;            /* cell reference */
    EV_RANGE        range;          /* range */
    EV_NAMEID       nameid;         /* id of named resource */
    S16             cond_pos;       /* position of conditional expression */
}
SS_DATA_ARG;

typedef struct SS_DATA
{
    EV_IDNO data_id;                /* type of result in union DATA_ID_, SYM_, RPN_*/

    SS_DATA_ARG arg;
}
SS_DATA, * P_SS_DATA, ** P_P_SS_DATA; typedef const SS_DATA * PC_SS_DATA;

typedef struct SS_RECOG_CONTEXT
{
    U8 ui_flag; /* 0 -> canonical file representations only, 1 -> extra parsing for UI */
    U8 alternate_function_flag; /* 1 -> bother trying alternate function name encode/decode */
    U8 function_arg_sep;
    U8 array_col_sep;
    U8 array_row_sep;
    U8 thousands_char;
    U8 decimal_point_char;
    U8 list_sep_char;
    U8 date_sep_char;
    U8 alternate_date_sep_char;
    U8 time_sep_char;
    U8 _spare4;
    U8 _spare5;
    U8 _spare6;
    U8 _spare7;
    U8 _spare8;
}
SS_RECOG_CONTEXT, * P_SS_RECOG_CONTEXT;

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

#define ev_slr_equal(pc_ev_slr1, pc_ev_slr2) ( \
    (pc_ev_slr1)->docno == (pc_ev_slr2)->docno && \
    (pc_ev_slr1)->col == (pc_ev_slr2)->col && \
    (pc_ev_slr1)->row == (pc_ev_slr2)->row )

#define ev_slr_compare(pc_ev_slr1, pc_ev_slr2) ( \
    (pc_ev_slr1)->docno < (pc_ev_slr2)->docno \
    ? -1 \
    : (pc_ev_slr1)->docno > (pc_ev_slr2)->docno \
    ? 1 \
    : (pc_ev_slr1)->row < (pc_ev_slr2)->row \
    ? -1 \
    : (pc_ev_slr1)->row > (pc_ev_slr2)->row \
    ? 1 \
    : (pc_ev_slr1)->col < (pc_ev_slr2)->col \
    ? -1 \
    : (pc_ev_slr1)->col > (pc_ev_slr2)->col \
    ? 1 \
    : 0 )

static inline void
f64_copy_words(
    _OutRef_    P_F64 p_f64_out,
    _InRef_     PC_F64 p_f64_in)
{
    P_U32 p_u32_out = (P_U32) p_f64_out;
    PC_U32 p_u32_in = (PC_U32) p_f64_in;
    p_u32_out[0] = p_u32_in[0];
    p_u32_out[1] = p_u32_in[1];
}

#if RISCOS || 0
#define f64_copy(lval, f64) f64_copy_words(&(lval), &(f64))
#else
#define f64_copy(lval, f64) (lval) = (f64)
#endif

/*
ss_const.c external functions
*/

/*ncr*/
extern S32
ss_data_set_error(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     STATUS error);

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64);

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64);

/*ncr*/
extern STATUS
ss_data_real_to_integer_force(
    _InoutRef_  P_SS_DATA p_ss_data);

/*ncr*/
extern BOOL
ss_data_real_to_integer_try(
    _InoutRef_  P_SS_DATA p_ss_data);

extern void
ss_array_free(
    _InoutRef_  P_SS_DATA p_ss_data);

extern STATUS
ss_array_dup(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_in);

_Check_return_
_Ret_
extern PC_SS_DATA
ss_array_element_index_borrow(
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

_Check_return_
_Ret_
extern P_SS_DATA
ss_array_element_index_wr(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern STATUS
ss_array_element_make(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern void
ss_array_element_read(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern STATUS
ss_array_make(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size);

_Check_return_
extern bool
ss_data_get_logical(
    _InRef_     PC_SS_DATA p_ss_data);

extern void
ss_data_set_logical(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     bool logical);

_Check_return_
extern F64
ss_data_get_number(
    _InRef_     PC_SS_DATA p_ss_data);

extern S32
ss_data_compare(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2);

extern void
ss_data_free_resources(
    _InoutRef_  P_SS_DATA p_ss_data);

extern S32
ss_data_resource_copy(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in);

_Check_return_
extern STATUS
ss_string_allocate(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_/*Val_*/ U32 len);

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_SS_DATA p_ss_data);

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_src);

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _In_        U32 uchars_n);

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR ustr);

_Check_return_
extern U32
ss_string_skip_leading_whitespace(
    _InRef_     PC_SS_DATA p_ss_data);

_Check_return_
extern U32
ss_string_skip_leading_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n);

_Check_return_
extern U32
ss_string_skip_internal_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n,
    _InVal_     U32 uchars_idx);

_Check_return_
extern U32
ss_string_skip_trailing_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n);

_Check_return_
static inline PC_UCHARS
ss_string_trim_leading_whitespace_uchars(
    _In_reads_(*p_uchars_n) PC_UCHARS uchars,
    _InoutRef_  P_U32 p_uchars_n)
{
    const U32 wss = ss_string_skip_internal_whitespace_uchars(uchars, *p_uchars_n, 0U);
    *p_uchars_n -= wss;
    return(PtrAddBytes(PC_UCHARS, uchars, wss));
}

_Check_return_
static inline PC_UCHARS
ss_string_trim_trailing_whitespace_uchars(
    _In_reads_(*p_uchars_n) PC_UCHARS uchars,
    _InoutRef_  P_U32 p_uchars_n)
{
    const U32 wss = ss_string_skip_trailing_whitespace_uchars(uchars, *p_uchars_n);
    *p_uchars_n -= wss;
    return(uchars);
}

extern S32
two_nums_type_match(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2,
    _InVal_     BOOL size_worry);

_Check_return_
extern F64
ui_strtod(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr);

_Check_return_
extern S32
ui_strtol(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_P_USTR p_ustr,
    _In_        int radix);

enum two_nums_type_match_result
{
    TWO_INTS,
    TWO_REALS,
    TWO_MIXED
};

extern const SS_DATA
ss_data_real_zero;

/*
ss_date.c
*/

extern const S32
ev_days_in_month[];

extern const S32
ev_days_in_month_leap[];

/*
ss_date.c external functions
*/

/* conversion to / from dateval */

_Check_return_
extern S32
ss_dateval_to_serial_number(
    _InVal_     SS_DATE_DATE ss_date_date);

_Check_return_ _Success_(status_ok(return))
extern STATUS
ss_serial_number_to_dateval(
    _OutRef_    P_SS_DATE_DATE p_ss_date_date,
    _InRef_     F64 serial_number);

_Check_return_
extern STATUS
ss_dateval_to_ymd(
    _InVal_     SS_DATE_DATE ss_date_date,
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day);

/*ncr*/
extern S32
ss_ymd_to_dateval(
    _OutRef_    P_SS_DATE_DATE p_ss_date_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day);

/* conversion to / from timeval */

_Check_return_
extern F64
ss_timeval_to_serial_fraction(
    _InVal_     SS_DATE_TIME ss_date_time);

extern void
ss_serial_fraction_to_timeval(
    _OutRef_    P_SS_DATE_TIME p_ss_date_time,
    _InVal_     F64 serial_fraction);

_Check_return_
extern STATUS
ss_timeval_to_hms(
    _InVal_     SS_DATE_TIME ss_date_time,
    _OutRef_    P_S32 hours,
    _OutRef_    P_S32 minutes,
    _OutRef_    P_S32 seconds);

/*ncr*/
extern S32
ss_hms_to_timeval(
    _OutRef_    P_SS_DATE_TIME p_ss_date_time,
    _InVal_     S32 hours,
    _InVal_     S32 minutes,
    _InVal_     S32 seconds);

/* date processing */

_Check_return_
extern S32
ss_date_compare(
    _InRef_     PC_SS_DATE p_ss_date_1,
    _InRef_     PC_SS_DATE p_ss_date_2);

extern void
ss_date_normalise(
    _InoutRef_  P_SS_DATE p_ss_date);

_Check_return_
extern F64
ss_date_to_serial_number(
    _InRef_     PC_SS_DATE p_ss_date);

_Check_return_ _Success_(status_ok(return))
extern STATUS
ss_serial_number_to_date(
    _OutRef_    P_SS_DATE p_ss_date,
    _InVal_     F64 serial_number);

extern void
ss_local_time_as_ymd_hms(
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_hours,
    _OutRef_    P_S32 p_minutes,
    _OutRef_    P_S32 p_seconds);

extern void
ss_date_set_from_local_time(
    _OutRef_    P_SS_DATE p_ss_date);

_Check_return_
extern S32
sliding_window_year(
    _In_        S32 year);

_Check_return_ _Success_(status_ok(return))
extern S32
ss_recog_date_time(
    _InoutRef_  P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str,
    _InVal_     bool american_date);

_Check_return_ _Success_(status_ok(return))
extern STATUS
ss_recog_date_time_as_t5( /* diff minimization - PipeDream has more args */
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str);

_Check_return_
extern S32
ss_date_decode(
    P_U8 op_buf,
    _InRef_     PC_SS_DATE p_ss_date,
    _InVal_     bool american_date);

_Check_return_
extern STATUS
ss_date_decode_as_t5( /* diff minimization - PipeDream has more args */
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_SS_DATE p_ss_date);

/******************************************************************************
*
* add, subtract or multiply two 32-bit signed integers, 
* checking for overflow and also returning
* a signed 64-bit result that the caller may consult
* e.g. to promote to fp
*
******************************************************************************/

typedef struct INT64_WITH_INT32_OVERFLOW
{
    int64_t int64_result;
    bool    f_overflow;
}
INT64_WITH_INT32_OVERFLOW, * P_INT64_WITH_INT32_OVERFLOW;

_Check_return_
extern int32_t
int32_add_check_overflow(
    _In_        const int32_t addend_a,
    _In_        const int32_t addend_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow);

_Check_return_
extern int32_t
int32_subtract_check_overflow(
    _In_        const int32_t minuend,
    _In_        const int32_t subtrahend,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow);

_Check_return_
extern int32_t
int32_multiply_check_overflow(
    _In_        const int32_t multiplicand_a,
    _In_        const int32_t multiplicand_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow);

#endif /* __ss_const_h */

/* end of ss_const.h */
