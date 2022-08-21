/* numform.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/******************************************************************************
*
* formats data in an ev_data structure
*
* James A. Dixon 07-Jan-1992; SKS April 1993 major hack
*
* The master copy is in Fireworkz - but note the small differences in this one!
*
******************************************************************************/

#include "common/gflags.h"

/*#include "ob_skel/flags.h"*/

#include "cmodules/numform.h"

#include "cmodules/ev_eval.h"
#include "cmodules/ev_evali.h"

/*
internal structure
*/

#define SECTION_MAX 200
#define BUF_SECTION_MAX 201

#define LIST_SEPARATOR ','

#define ROMAN_MAX 3999

typedef struct _NUMFORM_INFO
{
    S32 type; /* conversion type being performed */

    P_QUICK_UBLOCK p_quick_ublock;

    EV_DATA ev_data;

    struct _NUMFORM_INFO_DATETIME
    {
        S32 day, month, year;

        S32 hour, minute, second;
    }
    datetime;

    STATUS negative;
    STATUS zero;

    S32 integer_places_format;
    S32 decimal_places_format;
    S32 exponent_places_format;

    P_USTR ustr_integer_section;
    S32 sizeof_integer_section;
    S32 integer_places_actual;

    P_USTR ustr_decimal_section;
    S32 decimal_places_actual;
    S32 decimal_section_spaces;

    P_USTR ustr_exponent_section;
    S32 exponent_places_actual;
    S32 exponent_section_spaces;
    U8 exponent_sign_actual;
    U8 _spare[3];

    P_USTR ustr_engineering_section;

    P_USTR ustr_lookup_section;
    P_USTR ustr_style_name;

    U8  decimal_pt;
    U8  engineering;
    U8  exponential;
    U8  exponent_sign;
    U8  roman;
    U8  spreadsheet;
    U8  thousands_sep;
    U8  wildcard_match;

    U8  integer_section_result_output;

    U8  decimal_section_has_hash;
    U8  decimal_section_has_zero;

    U8  base_basechar;
    S32 base;
}
NUMFORM_INFO, * P_NUMFORM_INFO;

/*
internal procedures
*/

static STATUS
numform_output_date_fields(
    P_NUMFORM_INFO p_numform_info,
    PC_NUMFORM_CONTEXT p_numform_context,
    _InoutRef_  P_PC_USTR p_ustr_numform_section);

static STATUS
numform_output_number_fields(
    P_NUMFORM_INFO p_numform_info,
    _InVal_     U8 ch);

static void
numform_section_extract_datetime(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_datetime);

static void
numform_section_extract_numeric(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_numeric,
    _Out_writes_z_(elemof_num_buf) P_USTR p_num_buf,
    _InVal_     U32 elemof_num_buf);

static void
numform_section_extract_texterror(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_texterror);

_Check_return_
_Ret_ /*opt_*/
static PC_USTR
numform_section_scan_next(
    _In_z_      PC_USTR ustr_section);

/*
internal variables
*/

static U8 numform_output_datetime_last_field = NULLCH; /* Last field output */

static P_U8
convert_digit_roman(
    P_U8 p_num_buf,
    _InVal_     U8 ch,
    PC_U8Z p_ones_fives_tens)
{
    U8 ones = p_ones_fives_tens[0];

    if(ch == '6' || ch == '7' || ch == '8')
        *p_num_buf++ = p_ones_fives_tens[1];

    switch(ch)
    {
    default: default_unhandled();
#if CHECKING
    case '0':
#endif
        break;

    case '3':
    case '8':
        *p_num_buf++ = ones;

        /*FALLTHRU*/

    case '2':
    case '7':
        *p_num_buf++ = ones;

        /*FALLTHRU*/

    case '1':
    case '6':
        *p_num_buf++ = ones;
        break;

    case '4':
        *p_num_buf++ = ones;

        /*FALLTHRU*/

    case '5':
        *p_num_buf++ = p_ones_fives_tens[1];
        break;

    case '9':
        *p_num_buf++ = ones;
        *p_num_buf++ = p_ones_fives_tens[2];
        break;
    }

    return(p_num_buf);
}

/******************************************************************************
*
* Output engineering number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_engineering(
    P_NUMFORM_INFO p_numform_info)
{
    F64 work_value = (p_numform_info->ev_data.did_num == RPN_DAT_REAL) ? p_numform_info->ev_data.arg.fp : p_numform_info->ev_data.arg.integer;
    S32 remainder;
    S32 decimal_places;
    P_USTR ustr;

    if(work_value < DBL_MIN)
        remainder = 0;
    else
    {
        F64 log10_work_value = log10(work_value);
        F64 f64_exponent;
        F64 mantissa = modf(log10_work_value, &f64_exponent);
        S32 exponent = (S32) f64_exponent;

        /* allow for logs of numbers (especially those not in same base as FP representation) becoming imprecise */
#define LOG_SIG_EPS 1E-8

        /* NB. consider numbers such as log10(0.2) ~ -0.698 = (-1.0 exp) + (0.30103 man) */
        if(mantissa < 0.0)
        {
            mantissa += 1.0;
            exponent -= 1;
        }

        /* watch for logs going awry */
        if(mantissa < LOG_SIG_EPS)
            /* keep rounded down */
            mantissa = 0.0;

        else if((1.0 - mantissa) < LOG_SIG_EPS)
            /* round up */
        {
            mantissa = 0.0;
            exponent += 1;
        }

        remainder = exponent % 3;

        if(remainder < 0)
            remainder += 3;

        exponent -= remainder;

        switch(exponent)
        {
        case +36: p_numform_info->ustr_engineering_section = "U"; break; /* wot is dis? */
        case +33: p_numform_info->ustr_engineering_section = "V"; break; /* vendeka */
        case +30: p_numform_info->ustr_engineering_section = "W"; break; /* wot is dis? */
        case +27: p_numform_info->ustr_engineering_section = "X"; break; /* xenna */
        case +24: p_numform_info->ustr_engineering_section = "Y"; break; /* yotta */
        case +21: p_numform_info->ustr_engineering_section = "Z"; break; /* zetta */
        case +18: p_numform_info->ustr_engineering_section = "E"; break; /* exa */
        case +15: p_numform_info->ustr_engineering_section = "P"; break; /* peta */
        case +12: p_numform_info->ustr_engineering_section = "T"; break; /* tera */
        case  +9: p_numform_info->ustr_engineering_section = "G"; break; /* giga */
        case  +6: p_numform_info->ustr_engineering_section = "M"; break; /* mega */
        case  +3: p_numform_info->ustr_engineering_section = "k"; break; /* kilo */
        case   0: break;
        case  -3: p_numform_info->ustr_engineering_section = "m"; break; /* milli */
        case  -6: p_numform_info->ustr_engineering_section = "u"; break; /* micro */
        case  -9: p_numform_info->ustr_engineering_section = "n"; break; /* nano */
        case -12: p_numform_info->ustr_engineering_section = "p"; break; /* pico */
        case -15: p_numform_info->ustr_engineering_section = "f"; break; /* femto */
        case -18: p_numform_info->ustr_engineering_section = "a"; break; /* atto */
        case -21: p_numform_info->ustr_engineering_section = "z"; break; /* zepto */
        case -24: p_numform_info->ustr_engineering_section = "y"; break; /* yocto */
        case -27: p_numform_info->ustr_engineering_section = "x"; break; /* xenno */
        case -30: p_numform_info->ustr_engineering_section = "w"; break; /* dunno, but it's there by induction */
        case -33: p_numform_info->ustr_engineering_section = "v"; break; /* vendeko */
        //case -36: p_numform_info->ustr_engineering_section = "uu"; break; /* dunno, but it's there by induction */

        default:
            {
            static UCHARZ exponent_buffer[16];
            (void) xusnprintf(exponent_buffer, sizeof32(exponent_buffer), "%c" S32_FMT, (p_numform_info->engineering == 'g') ? 'e': 'E', exponent);
            p_numform_info->ustr_engineering_section = exponent_buffer;
            break;
            }
        }

        work_value = pow(10.0, remainder + mantissa);
    }

    decimal_places = (p_numform_info->integer_places_format + p_numform_info->decimal_places_format - 1) /*- remainder*/;

    (void) xusnprintf(p_numform_info->ustr_integer_section, p_numform_info->sizeof_integer_section,
                      "%#.*f", (int) decimal_places, work_value);
    /* NB # flag forces f conversion to always have decimal point */
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform engfmt : %s"), trace_ustr(p_numform_info->ustr_integer_section));

    ustr = p_numform_info->ustr_integer_section;

    while(*ustr++ != '.')
    { /*EMPTY*/ }
    p_numform_info->ustr_decimal_section = ustr;

    *--ustr = NULLCH; /* terminate integer section */
    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    /* strip trailing zeroes behind dp */
    while(*++ustr)
    { /*EMPTY*/ }

    while(*--ustr == '0')
    { /*EMPTY*/ }
    *++ustr = NULLCH; /* terminate decimal section */
    p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform engfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            trace_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            trace_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

/******************************************************************************
*
* Output exponential number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_exponential(
    P_NUMFORM_INFO p_numform_info)
{
    P_USTR ustr;
    F64 work_value = (p_numform_info->ev_data.did_num == RPN_DAT_REAL) ? p_numform_info->ev_data.arg.fp : p_numform_info->ev_data.arg.integer;

    (void) xusnprintf(p_numform_info->ustr_integer_section, p_numform_info->sizeof_integer_section,
                      "%#.*e", (int) p_numform_info->decimal_places_format, work_value);
    /* NB # flag forces e conversion to always have decimal point */
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform expfmt : %s\n"), trace_ustr(p_numform_info->ustr_integer_section));

    ustr = p_numform_info->ustr_integer_section;

    while(*ustr++ != '.')
    { /*EMPTY*/ }
    p_numform_info->ustr_decimal_section = ustr;
    *--ustr = NULLCH; /* terminate integer section */
    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    assert(p_numform_info->integer_places_actual == 1);

    /* strip trailing zeroes behind dp and before 'e' */
    while(*++ustr != 'e')
    { /*EMPTY*/ }
    p_numform_info->ustr_exponent_section = ustr + 1; /* point to exp sign */

    while(*--ustr == '0')
    { /*EMPTY*/ }
    *++ustr = NULLCH; /* terminate decimal section, overwriting 'e' or a '0' */
    p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);

    /* strip sign and leading zero(es) from exponent */
    p_numform_info->exponent_sign_actual = *p_numform_info->ustr_exponent_section++;
    assert((p_numform_info->exponent_sign_actual == '+') || (p_numform_info->exponent_sign_actual == '-'));

    for(ustr = p_numform_info->ustr_exponent_section; *ustr == '0'; ++ustr)
        /* don't strip last zero? */
        if(!ustr[1])
            break;

    p_numform_info->ustr_exponent_section = ustr;

    p_numform_info->exponent_places_actual = ustrlen32(p_numform_info->ustr_exponent_section);

    trace_6(TRACE_MODULE_NUMFORM, TEXT("numform expfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            trace_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            trace_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual,
            trace_ustr(p_numform_info->ustr_exponent_section), p_numform_info->exponent_places_actual);
}

/******************************************************************************
*
* Output roman numerals to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_roman(
    P_NUMFORM_INFO p_numform_info)
{
    static const U8 roman_numerals_lc[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm', '-', '-' };
    static const U8 roman_numerals_uc[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M', '-', '-' };

    PC_U8 roman_numerals = (p_numform_info->roman == 'R') ? roman_numerals_uc : roman_numerals_lc;
    U8Z num_string[9];
    P_U8Z p_num_string = num_string;
    S32 digit = 0;
    P_USTR ustr;

    if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
        (void) xsnprintf(num_string, sizeof32(num_string),
                         "%.0f",  p_numform_info->ev_data.arg.fp); /* precision of 0 -> no decimal point */
    else
        (void) xsnprintf(num_string, sizeof32(num_string),
                         S32_FMT, p_numform_info->ev_data.arg.integer);

    while(*p_num_string++)
        ++digit;

    p_num_string = num_string;
    ustr = p_numform_info->ustr_integer_section;

    while(--digit >= 0)
        ustr = convert_digit_roman(ustr, *p_num_string++, &roman_numerals[digit * 2]);

    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = p_numform_info->ustr_integer_section + p_numform_info->integer_places_actual; /* point to NULLCH */

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform rmnfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            trace_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            trace_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

/******************************************************************************
*
* Output spreadsheet number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_spreadsheet(
    P_NUMFORM_INFO p_numform_info)
{
    S32 work_value;

    if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
        work_value = (S32) (p_numform_info->ev_data.arg.fp + 0.5);
    else
        work_value = p_numform_info->ev_data.arg.integer;

    p_numform_info->integer_places_actual = xtos_buf(p_numform_info->ustr_integer_section, p_numform_info->sizeof_integer_section, work_value - 1, (p_numform_info->spreadsheet == 'X'));

    p_numform_info->ustr_decimal_section = p_numform_info->ustr_integer_section + p_numform_info->integer_places_actual; /* point to NULLCH */
}

/******************************************************************************
*
* lookup value in table
*
******************************************************************************/

static void
convert_number_lookup(
    P_NUMFORM_INFO p_numform_info,
    P_USTR p_buffer)
{
    U8 ch;
    S32 index = 0;
    P_USTR ustr_string = p_numform_info->ustr_lookup_section;
    P_USTR ustr_last_string = ustr_string;
    S32 work_value;

    if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
        work_value = (S32) (p_numform_info->ev_data.arg.fp + 0.5);
    else
        work_value = p_numform_info->ev_data.arg.integer;

    while(index < work_value && (ch = *ustr_string++) != '}' && (ch != NULLCH))
    {
        if(ch == LIST_SEPARATOR)
        {
            index++;
            ustr_last_string = ustr_string;
        }
        if(ch == '[') /* skip style names */
        {
            while((ch = *ustr_string++) != ']' && ch != '}' && (ch != NULLCH))
            { /*EMPTY*/ }
            if(ch != ']')
                ustr_string--; /* point back to terminator */
        }
    }
    ustr_string = p_numform_info->ustr_integer_section;
    while((ch = *ustr_last_string++) != LIST_SEPARATOR && ch != '}' && (ch != NULLCH))
    {
        if(ch == '[')
        {
            p_numform_info->ustr_style_name = ustr_last_string;
            while((ch = *ustr_last_string++) != ']' && ch!='}' && (ch != NULLCH) && ch != LIST_SEPARATOR)
            { /*EMPTY*/ }
            if(ch != ']')
                ustr_last_string--;
        }
        else
            *ustr_string++ = ch;
    }
    *ustr_string = NULLCH;

    p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = p_numform_info->ustr_integer_section + p_numform_info->integer_places_actual; /* point to NULLCH */

    if(!*p_buffer)
    {
        *p_buffer++ = '#';
        *p_buffer = NULLCH;
    }
}

static U8
base_character(
    _InVal_     S32 value,
    _InVal_     U8 base_char)
{
    if(value < 9)
        return((U8) (value + '0'));

    return((U8) (base_char + value - 10));
}

/******************************************************************************
*
* Output standard number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_standard(
    P_NUMFORM_INFO p_numform_info)
{
    if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
    {
        P_USTR ustr;

        (void) xusnprintf(p_numform_info->ustr_integer_section, p_numform_info->sizeof_integer_section,
                          "%#.*f", (int) p_numform_info->decimal_places_format, p_numform_info->ev_data.arg.fp);
        /* NB # flag forces f conversion to always have decimal point */
        trace_1(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt : %s\n"), trace_ustr(p_numform_info->ustr_integer_section));

        if(*p_numform_info->ustr_integer_section == '0')
            p_numform_info->ustr_integer_section++;

        ustr = p_numform_info->ustr_integer_section;

        while(*ustr++ != '.')
        { /*EMPTY*/ }
        p_numform_info->ustr_decimal_section = ustr;
        *--ustr = NULLCH; /* terminate integer section */
        p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

        /* strip trailing zeroes behind dp */
        while(*++ustr)
        { /*EMPTY*/ }

        while(*--ustr == '0')
        { /*EMPTY*/ }
        ++ustr;

        while(ustr - p_numform_info->ustr_decimal_section > p_numform_info->decimal_places_format)
            --ustr; /* converted too many places */

        *ustr = NULLCH; /* terminate decimal section */
        p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);
    }
    else
    {
        (void) xusnprintf(p_numform_info->ustr_integer_section, p_numform_info->sizeof_integer_section,
                          S32_FMT, p_numform_info->ev_data.arg.integer);
        trace_1(TRACE_MODULE_NUMFORM, TEXT("numform intfmt : %s\n"), trace_ustr(p_numform_info->ustr_integer_section));

        if(*p_numform_info->ustr_integer_section == '0')
            p_numform_info->ustr_integer_section++;

        p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

        p_numform_info->ustr_decimal_section = p_numform_info->ustr_integer_section + p_numform_info->integer_places_actual; /* point to NULLCH */
    }

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            trace_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            trace_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

static void
convert_number_base(
    P_NUMFORM_INFO p_numform_info)
{
    S32 work_value;
    U8Z buffer[1000]; /* should cover all numbers! */
    P_U8Z p_buffer = buffer;
    P_USTR ustr_next = p_numform_info->ustr_integer_section;

    if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
        work_value = (S32) (p_numform_info->ev_data.arg.fp + 0.5);
    else
        work_value = p_numform_info->ev_data.arg.integer;

    *p_buffer++ = NULLCH;
    if(work_value == 0)
        *p_buffer++ = '0';
    while(work_value>0)
    {
        *p_buffer++ = base_character(work_value % p_numform_info->base, p_numform_info->base_basechar);
        work_value /= p_numform_info->base;
    }
    p_buffer--;
    while(*p_buffer)
    {
        *ustr_next++ = *p_buffer;
        p_buffer--;
    }
    *ustr_next = NULLCH;

    if(*p_numform_info->ustr_integer_section == '0')
        p_numform_info->ustr_integer_section++;

    p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = p_numform_info->ustr_integer_section + p_numform_info->integer_places_actual; /* point to NULLCH */

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            trace_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            trace_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

/******************************************************************************

Recognised Symbols:

\    - next character literal
".." - literal text
;    - Section separator
-:() - quotes unnecessary (also '.' and ',' in non-numeric fields)
#    - Digit, nothing if absent
0    - Digit, zero if absent
_    - Digit, space if absent
.    - decimal point indicator (next char)
,    - thousands sep indicator (next char)
eE   - exponent (followed by +, - or nothing)
<    - shift left (* 1000)
>    - shift right (/ 1000)
%    - % char and *100
gG   - Engineering notation (afpnum.kMGTPE)
rR   - Roman numerals (if < 4999)
xX   - Spreadsheet (ie  1 -> A  etc.)
hH   - hours
nN   - minutes
sS   - seconds
dD   - days
mM   - months, unless immediately after hH, then minutes
yY   - years
@    - Text / error
[]   - encloses a style name
{}   - lookup list, starting from 0
bB   - base number
******************************************************************************/

static const NUMFORM_CONTEXT
default_numform_context =
{
    { "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december" },

    {
/*    0     1     2     3     4     5     6     7     8     9 */
          "st", "nd", "rd", "th", "th", "th", "th", "th", "th",
    "th", "th", "th", "th", "th", "th", "th", "th", "th", "th",
    "th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th",
    "th", "st"
    }
};

_Check_return_
extern STATUS
numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _InoutRef_opt_ P_QUICK_TBLOCK p_quick_tblock_style,
    _InRef_     PC_EV_DATA p_ev_data,
    _InRef_     PC_NUMFORM_PARMS p_numform_parms)
{
    PC_NUMFORM_CONTEXT p_numform_context = p_numform_parms->p_numform_context;
    NUMFORM_INFO numform_info;
    UCHARZ own_numform[BUF_SECTION_MAX]; /* buffer into which numform section is extracted */
    PC_USTR ustr_numform_section;
    UCHARZ num_ubuf[1 /*sign*/ + (1+DBL_MAX_10_EXP) /*digits*/ + 1 /*paranoia*/ + 1 /*NULLCH*/]; /* Contains the number to be output */
    STATUS status;
    U8 ch;

    if(IS_P_DATA_NONE(p_numform_context))
        p_numform_context = &default_numform_context;

    zero_struct(numform_info);

    numform_info.p_quick_ublock = p_quick_ublock;
    numform_info.ustr_style_name = NULL;

    /* Get a hard copy of the ev_data */
    numform_info.ev_data = *p_ev_data;

    /* just show array tl */
    if(numform_info.ev_data.did_num == RPN_TMP_ARRAY)
        ss_array_element_read(&numform_info.ev_data, &numform_info.ev_data, 0, 0);

    /* note conversion type */
    numform_info.type = numform_info.ev_data.did_num;

    /*numform_info.negative = 0; already clear */
    /*numform_info.zero = 0;*/

    own_numform[0] = NULLCH;

    switch(numform_info.ev_data.did_num)
    {
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        numform_info.type = RPN_DAT_REAL;

        if(numform_info.ev_data.arg.integer < 0)
        {
            numform_info.ev_data.arg.integer = 0 - numform_info.ev_data.arg.integer;
            numform_info.negative = 1;
        }
        else
            numform_info.zero = (numform_info.ev_data.arg.integer == 0);

        if(p_numform_parms->ustr_numform_numeric)
            numform_section_extract_numeric(&numform_info, own_numform, sizeof32(own_numform), p_numform_parms->ustr_numform_numeric, num_ubuf, sizeof32(num_ubuf));
        break;

    case RPN_DAT_REAL:
        if(numform_info.ev_data.arg.fp < 0.0)
        {
            numform_info.ev_data.arg.fp = 0.0 - numform_info.ev_data.arg.fp;
            numform_info.negative = 1;
        }
        else if(numform_info.ev_data.arg.fp == 0.0)
        {
            numform_info.ev_data.arg.fp = 0.0; /* NB We DO get -0.0 !!! which screws up rest of printing code ... */
            numform_info.zero = 1;
        }

        if(p_numform_parms->ustr_numform_numeric)
            numform_section_extract_numeric(&numform_info, own_numform, sizeof32(own_numform), p_numform_parms->ustr_numform_numeric, num_ubuf, sizeof32(num_ubuf));
        break;

    case RPN_DAT_DATE:
        numform_output_datetime_last_field = NULLCH;

        ss_dateval_to_ymd(&numform_info.ev_data.arg.date, &numform_info.datetime.year, &numform_info.datetime.month,  &numform_info.datetime.day);
        ss_timeval_to_hms(&numform_info.ev_data.arg.date, &numform_info.datetime.hour, &numform_info.datetime.minute, &numform_info.datetime.second);

        if(p_numform_parms->ustr_numform_datetime)
            numform_section_extract_datetime(&numform_info, own_numform, sizeof32(own_numform), p_numform_parms->ustr_numform_datetime);
        break;

    default:
#if CHECKING
    /*case RPN_DAT_NEXT_NUMBER:*/
        assert0();

        /*FALLTHRU*/

    case RPN_DAT_STRING:
    case RPN_DAT_ERROR:
    case RPN_DAT_BLANK:
#endif
        numform_info.type = RPN_DAT_STRING;

        if(!IS_P_DATA_NONE(p_numform_parms->ustr_numform_texterror))
            numform_section_extract_texterror(&numform_info, own_numform, sizeof32(own_numform), p_numform_parms->ustr_numform_texterror);
        break;

    case RPN_TMP_ARRAY:
        return(create_error(EVAL_ERR_UNEXARRAY));
    }

    status = STATUS_OK;

    ustr_numform_section = own_numform;

    /* loop outputting chars from data under control of format */
    while(NULLCH != (ch = *ustr_numform_section++))
    {
        switch(ch)
        {
        case '\\':
            /* next char literal */
            ch = *ustr_numform_section++;
            break;

        case '"':
            /* literal string */
            while(((ch = *ustr_numform_section++) != '"') && ch)
                status_break(status = quick_ublock_u8_add(numform_info.p_quick_ublock, ch));

            ch = NULLCH;
            break;

        /* Any characters to be recognised as literal text without backslash or double quotes */
        case ',':
        case ':':
        case '(':
        case ')':
        case '-':
            break;

        default:
            switch(numform_info.type)
            {
            default: default_unhandled();
#if CHECKING
            case RPN_DAT_REAL:
#endif
                switch(ch)
                {
                case '.':
                case '_':
                case '0':
                case '#':
                    if(numform_info.negative)
                    {
                        /* for ALL formats we put the - sign in as needed just prior to the first bit of numeric output */
                        numform_info.negative = 0;

                        if(numform_info.wildcard_match)
                            if(status_fail(status = quick_ublock_u8_add(numform_info.p_quick_ublock, '-')))
                            {
                                ch = NULLCH;
                                break;
                            }
                    }

                    status = numform_output_number_fields(&numform_info, ch);

                    /* nasty bodge to cope with engineering format giving us more after the decimal places than we bargained for */
                    if(numform_info.ustr_engineering_section && !numform_info.decimal_places_format && status_ok(status))
                    {
                        status = quick_ublock_ustr_add(numform_info.p_quick_ublock, numform_info.ustr_engineering_section);
                        numform_info.ustr_engineering_section = NULL;
                    }

                    ch = NULLCH;
                    break;

                default:
                    break;
                }
                break;

            case RPN_DAT_DATE:
                ch = NULLCH;
                --ustr_numform_section;

                status = numform_output_date_fields(&numform_info, p_numform_context, &ustr_numform_section);
                break;

            case RPN_DAT_STRING:
                if(ch == '@')
                {
                    ch = NULLCH;

                    if(numform_info.ev_data.did_num == RPN_DAT_STRING)
                        status = quick_ublock_uchars_add(numform_info.p_quick_ublock, numform_info.ev_data.arg.string.data, numform_info.ev_data.arg.string.size);
#if 0
                    else if(numform_info.ev_data.did_num == RPN_DAT_ERROR)
                        status = quick_ublock_ustr_add(numform_info.p_quick_ublock, resource_lookup_ustr(-numform_info.ev_data.arg.error.num));
#endif
                }
                break;
            }

            break;
        }

        if(ch)
            status = quick_ublock_u8_add(numform_info.p_quick_ublock, ch);

        status_break(status);
    }

    if(status_ok(status))
        status = quick_ublock_nullch_add(numform_info.p_quick_ublock);

    /* get pointer to style name if existing */
    if((P_QUICK_TBLOCK_NONE != p_quick_tblock_style) && (NULL != numform_info.ustr_style_name))
    {
        PC_USTR ustr_style_name = numform_info.ustr_style_name;

        /* terminated by ']' hopefully */
        while(((ch = *ustr_style_name++) != ']') && (ch != NULLCH))
            status_return(quick_tblock_tchar_add(p_quick_tblock_style, ch));

        status = quick_tblock_nullch_add(p_quick_tblock_style);
    }

    return(status);
}

static S32
numform_numeric_section_copy_and_parse(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_section)
{
    S32 buffer_remaining = elemof_buffer;
    S32 shift_value = 0;
    U8 ch;

    p_numform_info->integer_places_format  = 0;
    p_numform_info->decimal_places_format  = 0;
    p_numform_info->exponent_places_format = 0;

    p_numform_info->integer_places_actual  = 0;
    p_numform_info->decimal_places_actual  = 0;
    p_numform_info->exponent_places_actual = 0;

    p_numform_info->exponent_sign          = '+';
    p_numform_info->thousands_sep          = 0;
    p_numform_info->decimal_pt             = 0;

    p_numform_info->engineering            = 0;
    p_numform_info->exponential            = 0;
    p_numform_info->roman                  = 0;
    p_numform_info->spreadsheet            = 0;
    p_numform_info->base                   = 0;

    p_numform_info->decimal_section_has_hash = 0;
    p_numform_info->decimal_section_has_zero = 0;
    p_numform_info->decimal_section_spaces   = 0;

    p_numform_info->integer_section_result_output = 0;

    p_numform_info->ustr_engineering_section = NULL;
    p_numform_info->ustr_lookup_section = NULL;
    p_numform_info->ustr_style_name = NULL;

    do  {
        switch(ch = *ustr_numform_section++)
        {
        case '\\':
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;
            if((ch = *ustr_numform_section++) != NULLCH)
            {
                if(buffer_remaining <= 0) break;
                --buffer_remaining;
                *p_buffer++ = ch;
            }
            else
            {
                ch = 1;
                --ustr_numform_section;
            }
            break;

        case '{':
            p_numform_info->ustr_lookup_section = (P_U8) ustr_numform_section;
            while((ch = *ustr_numform_section++) != '}' && (ch != NULLCH))
            { /*EMPTY*/ }
            if(ch == NULLCH)
            {
                ch = 1;
                ustr_numform_section--;
            }
            break;

        case '[':
            p_numform_info->ustr_style_name = (P_U8) ustr_numform_section;
            while((ch = *ustr_numform_section++) != ']' && ch != NULLCH)
            { /*EMPTY*/ }
            if(ch == NULLCH)
            {
                ch = 1;
                ustr_numform_section--;
            }
            break;

        case '"':
            do  {
                if(buffer_remaining <= 0) break;
                --buffer_remaining;
                *p_buffer++ = ch;
                ch = *ustr_numform_section++;
            }
            while(ch && (ch != '"'));

            if(ch)
            {
                if(buffer_remaining <= 0) break;
                --buffer_remaining;
                *p_buffer++ = ch;
            }
            else
                --ustr_numform_section;
            break;

        case '.':
        case ',':
            {
            P_U8 p_char = (ch == '.')
                        ? &p_numform_info->decimal_pt
                        : &p_numform_info->thousands_sep;

            if(!*p_char)
            {
                if(ch == '.')
                {
                    if(buffer_remaining <= 0) break;
                    --buffer_remaining;
                    *p_buffer++ = ch;
                }

                switch(*ustr_numform_section)
                {
                case NULLCH:
                case '_':
                case '0':
                case '#':
                case '"':
                    /* dot/comma followed by one of these chars specifies default decimal point/thousands sep char */
                    *p_char = ch;
                    break;

                default:
                    /* dot/comma followed by char specifies decimal point/thousands sep char */
                    *p_char = *ustr_numform_section++;
                    break;
                }
            }

            break;
            }

        case '_':
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_section_spaces += 1;
            else if(p_numform_info->decimal_pt)
                p_numform_info->decimal_section_spaces += 1;
            else
                p_numform_info->integer_places_format++;

            break;

        case '0':
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_places_format++;
            else if(p_numform_info->decimal_pt)
            {
                p_numform_info->decimal_places_format++;
                p_numform_info->decimal_section_has_zero = 1;
            }
            else
                p_numform_info->integer_places_format++;

            break;

        case '#':
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_places_format++;
            else if(p_numform_info->decimal_pt)
            {
                p_numform_info->decimal_places_format++;
                p_numform_info->decimal_section_has_hash = 1;
            }
            else
                p_numform_info->integer_places_format++;

            break;

        case 'e':
        case 'E':
            p_numform_info->exponential = ch;

            ch = *ustr_numform_section++;
            if((ch == '+') || (ch == '-'))
                p_numform_info->exponent_sign = ch;
            else
                --ustr_numform_section;
            break;

        case 'b':
        case 'B':
            {
            p_numform_info->base_basechar = (U8) (ch - 1);

            if(!isdigit(*ustr_numform_section))
                p_numform_info->base = 0;  /* turn conversion off again */
            else
            {
                p_numform_info->base = fast_strtoul(ustr_numform_section, (P_USTR *) &ustr_numform_section);

                if((p_numform_info->base < 2) || (p_numform_info->base > 36))
                    p_numform_info->base = 0;
            }
            break;
            }

        case 'g':
        case 'G':
            p_numform_info->engineering = ch;
            break;

        case 'r':
        case 'R':
            p_numform_info->roman = ch;
            break;

        case 'x':
        case 'X':
            p_numform_info->spreadsheet = ch;
            break;

        case '<': /* SKS 19dec94 removed dross code */
            shift_value += 3;
            break;

        case '>':
            shift_value -= 3;
            break;

        case '%':
            shift_value += 2;
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;
            break;

        case ';':
            ch = NULLCH;

            /*FALLTHRU*/

        default:
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;
            break;
        }
    }
    while(ch);

    if(!p_numform_info->decimal_pt)
        p_numform_info->decimal_pt = '.';

    trace_6(TRACE_MODULE_NUMFORM, TEXT("%c ths; ") S32_TFMT TEXT(" ip; %c dpc; ") S32_TFMT TEXT(" dp; ") S32_TFMT TEXT(" ep; %c es\n"),
            p_numform_info->thousands_sep ? p_numform_info->thousands_sep : ' ',
            p_numform_info->integer_places_format,
            p_numform_info->decimal_pt,
            p_numform_info->decimal_places_format,
            p_numform_info->exponent_places_format,
            p_numform_info->exponent_sign ? p_numform_info->exponent_sign : ' ');

    return(shift_value);
}

/******************************************************************************
*
* Checks the next character in the passed numform_section for any
* characters relevant to time / date field processing
*
******************************************************************************/

static STATUS
numform_output_date_fields(
    P_NUMFORM_INFO p_numform_info,
    PC_NUMFORM_CONTEXT p_numform_context,
    _InoutRef_  P_PC_USTR p_ustr_numform_section)
{
    PC_USTR ustr_numform_section = *p_ustr_numform_section;
    U32 count = 0;
    UCHARZ tem_ubuf[BUF_SECTION_MAX];
    U8 ch, section_ch;
    P_USTR ustr_format = P_USTR_NONE;
    S32 arg = 0; /* keep dataflower happy */

    STATUS status = STATUS_OK;

    switch(section_ch = *ustr_numform_section++)
    {
    default:
        *p_ustr_numform_section = ustr_numform_section;
        return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, section_ch)); /* don't set last_field */

    case 'h':
    case 'H':
        do { ++count; } while(((ch = *ustr_numform_section++) == 'h') || (ch == 'H'));
        --ustr_numform_section;

        ustr_format = (count == 1) ? S32_FMT : "%02" S32_FMT_POSTFIX;
        arg = p_numform_info->datetime.hour;
        break;

    case 'n':
    case 'N':
        do { ++count; } while(((ch = *ustr_numform_section++) == 'n') || (ch == 'N'));
        --ustr_numform_section;

        ustr_format =  (count == 1) ? S32_FMT : "%02" S32_FMT_POSTFIX;
        arg = p_numform_info->datetime.minute;
        break;

    case 's':
    case 'S':
        do { ++count; } while(((ch = *ustr_numform_section++) == 's') || (ch == 'S'));
        --ustr_numform_section;

        ustr_format = (count == 1) ? S32_FMT : "%02" S32_FMT_POSTFIX;
        arg = p_numform_info->datetime.second;
        break;

    case 'd':
    case 'D':
        {
        PC_USTR ustr_suffix = "";

        do { ++count; } while(((ch = *ustr_numform_section++) == 'd') || (ch == 'D'));
        --ustr_numform_section;

        if(count > 2)
        {
            PC_USTR ustr = p_numform_context->day_endings[p_numform_info->datetime.day];

            if(NULL != ustr)
                ustr_suffix = ustr;
        }

        arg = p_numform_info->datetime.day + 1;

        (void) xusnprintf(tem_ubuf, sizeof32(tem_ubuf),
                          ((count == 1) || (count == 3))
                          ? "" S32_FMT "%s"
                          : "%02" S32_FMT_POSTFIX "%s",
                          arg, ustr_suffix);
        status_break(status = quick_ublock_ustr_add(p_numform_info->p_quick_ublock, tem_ubuf));
        break;
        }

    case 'm':
    case 'M':
        do { ++count; } while(((ch = *ustr_numform_section++) == 'm') || (ch == 'M'));
        --ustr_numform_section;

        if((numform_output_datetime_last_field == 'h') || (numform_output_datetime_last_field == 'H'))
        {
            /* minutes, not months iff immediately after hours */
            ustr_format = (count == 1) ? S32_FMT : "%02" S32_FMT_POSTFIX;
            arg = p_numform_info->datetime.minute;
            break;
        }

        if((count == 1) || (count == 2))
        {
            ustr_format = (count == 1) ? S32_FMT : "%02" S32_FMT_POSTFIX;
            arg = p_numform_info->datetime.month + 1;
        }
        else
        {
            PC_USTR ustr_month_name = p_numform_context->month_names[p_numform_info->datetime.month];
            U32 len = ustr_month_name ? ustrlen32(ustr_month_name) : 0;
            U32 i;

            /* restrict output to just first 3 chars of the monthname */
            if(count == 3)
                len = MIN(len, count);

            ustr_numform_section -= count;

            for(i = 0; i < len; ++i)
            {
                ch = ustr_month_name[i];

                /* uppercase corresponding chars of the monthname, using last specifier char if overflow */
                if(ustr_numform_section[MIN(i, count)] == 'M')
                    ch = (U8) toupper(ch);

                status_break(status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, ch));
            }

            ustr_numform_section += count;
        }
        break;

    case 'y':
    case 'Y':
        {
        BOOL adbc;
        BOOL bc_date;

        do { ++count; } while(((ch = *ustr_numform_section++) == 'y') || (ch == 'Y'));

        adbc = ((ch == 'a') || (ch == 'A'));
        if(!adbc)
           --ustr_numform_section;

        arg = p_numform_info->datetime.year + 1;

        if(arg <= 0) /* NB. year 0 -> 1BC etc */
        {
            bc_date = TRUE;
            arg = 1 - arg;
        }
        else
            bc_date = FALSE;

        switch(count)
        {
        case 0:
        case 1:
            arg -= (arg / 100) * 100;
            ustr_format = S32_FMT;
            break;

        case 2:
            arg -= (arg / 100) * 100;
            ustr_format = "%02" S32_FMT_POSTFIX;
            break;

        case 4:
            if(adbc)
                ustr_format = bc_date ? "%04" S32_FMT_POSTFIX "BC" : "%04" S32_FMT_POSTFIX "AD";
            else
                ustr_format = "%04" S32_FMT_POSTFIX;
            break;

        default:
            if(adbc)
                ustr_format = bc_date ? S32_FMT "BC" : S32_FMT "AD";
            else
                ustr_format = S32_FMT;
            break;
        }

        break;
        }
    }

    numform_output_datetime_last_field = section_ch;

    if(P_USTR_NONE != ustr_format)
    {
        (void) xusnprintf(tem_ubuf, sizeof32(tem_ubuf), ustr_format, arg);
        status = quick_ublock_ustr_add(p_numform_info->p_quick_ublock, tem_ubuf);
    }

    *p_ustr_numform_section = ustr_numform_section;
    return(status);
}

static STATUS
numform_output_number_fields(
    P_NUMFORM_INFO p_numform_info,
    _InVal_     U8 ch)
{
    STATUS status = STATUS_OK;

    if(ch == '.')
    {
        U8 ch_out = p_numform_info->decimal_pt;

        /* supress output of decimal point char sometimes st. 2.0 output under 0.## will give 2, but under 0. will give 2. */
        if(p_numform_info->decimal_places_actual == 0)
            if(p_numform_info->decimal_section_has_hash)
                if(!p_numform_info->decimal_section_has_zero && !p_numform_info->decimal_section_spaces)
                    ch_out = NULLCH;

        /* ie a format such as .### will display just the fractional part of a number */
        p_numform_info->integer_places_actual = 0;

        if(ch_out)
            status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, ch_out));

        return(status);
    }

    /* Integer length overflows or matches specifier */
    while(p_numform_info->integer_places_format < p_numform_info->integer_places_actual)
    {
        status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, *p_numform_info->ustr_integer_section++));
        p_numform_info->integer_places_actual--;
        p_numform_info->integer_section_result_output = 2;

        if(p_numform_info->thousands_sep && (p_numform_info->integer_places_actual > 0))
            if(p_numform_info->integer_places_actual % 3 == 0)
                status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, p_numform_info->thousands_sep));
    }

    if(p_numform_info->integer_places_format > 0)
    {
        if(p_numform_info->integer_places_format > p_numform_info->integer_places_actual)
        {
            /* Specifier overflows number, so pad up */
            U8 ch_out;

            switch(ch)
            {
            case '_':
                ch_out = ' ';
                if(!p_numform_info->integer_section_result_output)
                    p_numform_info->integer_section_result_output = 1; /* put spaces at thousand positions between padding spaces */
                break;

            case '0':
                ch_out = '0';
                p_numform_info->integer_section_result_output = 2; /* put thousands separators between padding zeroes */
                break;

            default:
                ch_out = '0';
                if(!p_numform_info->integer_section_result_output)
                    ch_out = NULLCH;
                break;
            }

            if(ch_out)
                 status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, ch_out));
        }
        else
        {
            assert(p_numform_info->integer_places_format == p_numform_info->integer_places_actual);
            status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, *p_numform_info->ustr_integer_section++));
            p_numform_info->integer_places_actual--;

            p_numform_info->integer_section_result_output = 2;
        }

        p_numform_info->integer_places_format--;

        if(p_numform_info->thousands_sep && (p_numform_info->integer_places_format > 0) && p_numform_info->integer_section_result_output)
            if(p_numform_info->integer_places_format % 3 == 0)
            {
                U8 th_ch = ' ';
                if(p_numform_info->integer_section_result_output != 1)
                    th_ch = p_numform_info->thousands_sep;
                status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, th_ch);
            }

        return(status);
    }

    /* got any trailing spaces in the decimal section? */
    if('_' == ch)
        if(p_numform_info->decimal_section_spaces)
        {
            status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, ' ');
            p_numform_info->decimal_section_spaces--;
            return(status);
        }

    if(p_numform_info->decimal_places_format > 0)
    {
        if(p_numform_info->decimal_places_actual > 0)
        {
            status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, *p_numform_info->ustr_decimal_section++);
            p_numform_info->decimal_places_actual--;
        }
        else
        {
            /* run out of digits for the format - pad with zeroes as appropriate */
            switch(ch)
            {
            case '0':
                status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, '0');
                break;

            default:
                break;
            }
        }

        p_numform_info->decimal_places_format--;

        return(status);
    }

    if(p_numform_info->exponent_sign)
    {
        status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, p_numform_info->exponential));

        if( (p_numform_info->exponent_sign_actual == '-') ||
            (p_numform_info->exponent_sign_actual == '+' && p_numform_info->exponent_sign == '+'))
            status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, p_numform_info->exponent_sign_actual));

        p_numform_info->exponent_sign = NULLCH;
    }

    /* Exponent length overflows or matches specifier */
    while(p_numform_info->exponent_places_format < p_numform_info->exponent_places_actual)
    {
        assert(isdigit(*p_numform_info->ustr_exponent_section));
        status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, *p_numform_info->ustr_exponent_section++));
        p_numform_info->exponent_places_actual--;
    }

    /* got any trailing spaces in the exponent section? */
    if('_' == ch)
        if(p_numform_info->exponent_section_spaces)
        {
            status = quick_ublock_u8_add(p_numform_info->p_quick_ublock, ' ');
            p_numform_info->exponent_section_spaces--;
            return(status);
        }

    if(p_numform_info->exponent_places_format > 0)
    {
        if(p_numform_info->exponent_places_format > p_numform_info->exponent_places_actual)
        {
            /* Specifier overflows number */
            switch(ch)
            {
            case '0':
                status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, '0'));
                break;

            default:
                break;
            }
        }
        else
        {
            assert(p_numform_info->exponent_places_format == p_numform_info->exponent_places_actual);
            assert(isdigit(*p_numform_info->ustr_exponent_section));
            status_return(quick_ublock_u8_add(p_numform_info->p_quick_ublock, *p_numform_info->ustr_exponent_section++));
            p_numform_info->exponent_places_actual--;
        }

        p_numform_info->exponent_places_format--;
    }

    return(status);
}

static void
numform_section_copy(
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_section)
{
    S32 buffer_remaining = elemof_buffer;
    U8 ch;

    do  {
        switch(ch = *ustr_section++)
        {
        case '\\':
            /* next character literal */
            if(buffer_remaining <= 0) break;
            --buffer_remaining;
            *p_buffer++ = ch;
            ch = *ustr_section++;
            break;

        case '"':
            do  {
                if(buffer_remaining <= 0) break;
                --buffer_remaining;
                *p_buffer++ = ch;
                ch = *ustr_section++;
            }
            while(ch && (ch != '"'));
            break;

        case ';':
            ch = NULLCH;
            break;

        default:
            break;
        }

        if(buffer_remaining <= 0) break;
        --buffer_remaining;
        *p_buffer++ = ch;
    }
    while(ch);
}

/* 2 section format is for time_only_am,time_only_pm */
/* 3 section format is for generic,date_only,time_only */
/* 4 section format is for generic,date_only,time_only_am,time_only_pm */

/*           1 section  2 sections  3 sections  4 sections  */
/* date_only    1st         1st         2nd         2nd     */
/* time_only    1st         1/2         3rd         3/4     */
/* both         1st         1st         1st         1st     */

static void
numform_section_extract_datetime(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_datetime)
{
    BOOL has_date = (0 != p_numform_info->ev_data.arg.date.date);
    BOOL has_time = (0 != p_numform_info->ev_data.arg.date.time);
    BOOL date_only = has_date && !has_time;
    BOOL time_only = /*has_time &&*/ !has_date; /*SKS 10feb97 for 00:00:00 time with no date*/
    PC_USTR ustr_section = ustr_numform_datetime;

    if(time_only)
    {
        BOOL is_pm = (p_numform_info->datetime.hour >= 12);
        PC_USTR ustr_second_section = numform_section_scan_next(ustr_section);
        PC_USTR ustr_third_section = P_USTR_NONE;

        if(P_USTR_NONE != ustr_second_section)
            ustr_third_section = numform_section_scan_next(ustr_second_section);

        if(P_USTR_NONE != ustr_third_section)
        {
            ustr_section = ustr_third_section;

            /* if we have time >= 1200, look for an optional fourth section */
            if(is_pm)
            {
                PC_USTR ustr_fourth_section = numform_section_scan_next(ustr_third_section);

                if(P_USTR_NONE != ustr_fourth_section)
                {
                    if(p_numform_info->datetime.hour >= 12)
                        p_numform_info->datetime.hour -= 12;

                    if(p_numform_info->datetime.hour == 0) /* err, 0:30pm is silly SKS 07feb96 */
                        p_numform_info->datetime.hour = 12;

                    ustr_section = ustr_fourth_section;
                }
            }
        }
        else if(P_USTR_NONE != ustr_second_section) /* 2 section am/pm format? */
        {
            if(is_pm)
            {
                if(p_numform_info->datetime.hour >= 12)
                    p_numform_info->datetime.hour -= 12;

                if(p_numform_info->datetime.hour == 0) /* err, 0:30pm is silly SKS 07feb96 */
                    p_numform_info->datetime.hour = 12;

                ustr_section = ustr_second_section;
            }
        }
    }
    else if(date_only)
    {
        PC_USTR ustr_second_section = numform_section_scan_next(ustr_section);

        if(P_USTR_NONE != ustr_second_section)
        {
            PC_USTR ustr_third_section = numform_section_scan_next(ustr_second_section);

            if(P_USTR_NONE != ustr_third_section)
                ustr_section = ustr_second_section;
        }
    }

    numform_section_copy(p_buffer, elemof_buffer, ustr_section);
}

static void
numform_section_extract_numeric(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_numeric,
    _Out_writes_z_(elemof_num_buf) P_USTR p_num_buf,
    _InVal_     U32 elemof_num_buf)
{
    PC_USTR ustr_section = ustr_numform_numeric;
    PC_USTR ustr_next_section;
    S32 shift_value;

    p_numform_info->wildcard_match = 0;

    /* if we have a negative number, search for optional second section */
    if(p_numform_info->negative || p_numform_info->zero)
    {
        if(P_USTR_NONE == (ustr_next_section = numform_section_scan_next(ustr_section)))
            p_numform_info->wildcard_match = 1;
        else
        {
            /* if we have a zero number, search for optional third section */
            if(p_numform_info->zero)
            {
                if(P_USTR_NONE == (ustr_next_section = numform_section_scan_next(ustr_next_section)))
                    p_numform_info->wildcard_match = 1;
                else
                    ustr_section = ustr_next_section;
            }
            else
                ustr_section = ustr_next_section;
        }
    }

    trace_1(TRACE_MODULE_NUMFORM, TEXT("\nnumform buffer 1 %s\n"), trace_ustr(ustr_section));
    shift_value = numform_numeric_section_copy_and_parse(p_numform_info, p_buffer, elemof_buffer, ustr_section);
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform buffer 2 %s\n"), trace_ustr(p_buffer));

    if(shift_value)
    {
        F64 multiplier = pow(10.0, shift_value);
        F64 work_value = (p_numform_info->ev_data.did_num == RPN_DAT_REAL) ? p_numform_info->ev_data.arg.fp : p_numform_info->ev_data.arg.integer;
        p_numform_info->ev_data.did_num = RPN_DAT_REAL;
        p_numform_info->ev_data.arg.fp = work_value * multiplier;
    }

    /* bounds check argument for given output format */
    if(p_numform_info->roman)
    {
        if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
        {
            if(p_numform_info->ev_data.arg.fp > (F64) ROMAN_MAX)
                p_numform_info->roman = 0;
        }
        else if(p_numform_info->ev_data.arg.integer > ROMAN_MAX)
            p_numform_info->roman = 0;
    }
    else if(p_numform_info->spreadsheet)
    {
        if(p_numform_info->ev_data.did_num == RPN_DAT_REAL)
            if(p_numform_info->ev_data.arg.fp > (F64) S32_MAX)
                p_numform_info->spreadsheet = 0;

        /* all integer args are representable in spreadsheet format */
    }

    p_numform_info->ustr_integer_section = p_num_buf;
    p_numform_info->sizeof_integer_section = elemof_num_buf;

    if(p_numform_info->ustr_lookup_section)
        convert_number_lookup(p_numform_info, p_buffer /* if empty, supply a default */);
    else if(p_numform_info->engineering)
        convert_number_engineering(p_numform_info);
    else if(p_numform_info->exponential)
        convert_number_exponential(p_numform_info);
    else if(p_numform_info->roman)
        convert_number_roman(p_numform_info);
    else if(p_numform_info->spreadsheet)
        convert_number_spreadsheet(p_numform_info);
    else if(p_numform_info->base)
        convert_number_base(p_numform_info);
    else
        convert_number_standard(p_numform_info);
}

static void
numform_section_extract_texterror(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR p_buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_texterror)
{
    PC_USTR ustr_section = ustr_numform_texterror;
    PC_USTR ustr_next_section;

    /* if we have an RPN_DAT_ERROR, search for optional second section */
    if((p_numform_info->ev_data.did_num == RPN_DAT_ERROR) || (p_numform_info->ev_data.did_num == RPN_DAT_BLANK))
    {
        if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_section)))
        {
            /* if we have an RPN_DAT_ERROR, search for optional third section */
            if(p_numform_info->ev_data.did_num == RPN_DAT_BLANK)
            {
                if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_next_section)))
                    ustr_section = ustr_next_section;
            }
            else
                ustr_section = ustr_next_section;
        }
    }

    numform_section_copy(p_buffer, elemof_buffer, ustr_section);
}

_Check_return_
_Ret_ /*opt_*/
static PC_USTR
numform_section_scan_next(
    _In_z_      PC_USTR ustr_section)
{
    U8 ch;

    do  {
        switch(ch = *ustr_section++)
        {
        case '\\':
            /* next character literal */
            ch = *ustr_section++;
            break;

        case '"':
            do  {
                ch = *ustr_section++;
            }
            while(ch && (ch != '"'));
            break;

        case ';':
            return(ustr_section);

        default:
            break;
        }
    }
    while(ch);

    return(P_USTR_NONE);
}

/* end of numform.c */
