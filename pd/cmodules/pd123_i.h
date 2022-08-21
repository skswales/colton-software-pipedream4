/* pd123_i.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Local header file for PipeDream/LOTUS converters */

/* MRJC March 1988 */

#include <limits.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef huge
#define huge
#endif

#ifndef uchar
#define uchar char
#endif

#include "typepack.h"

/******************************************************************************
*
* code selection
*
* UTIL_LTP creates stand-alone utility for Lotus to PipeDream
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus
* INT_LPTPL creates internal code for converter using function lp_lptpl
*
******************************************************************************/

/* LOTUS opcode types */
#define L_BOF 0
#define L_EOF 1
#define L_CALCMODE 2
#define L_CALCORDER 3
#define L_SPLIT 4
#define L_SYNC 5
#define L_RANGE 6
#define L_WINDOW1 7
#define L_COLW1 8
#define L_INTEGER 0xD
#define L_NUMBER 0xE
#define L_LABEL 0xF
#define L_FORMULA 0x10
#define L_TABLE 0x18
#define L_QRANGE 0x19
#define L_PRANGE 0x1A
#define L_SRANGE 0x1B
#define L_FRANGE 0x1C
#define L_KRANGE 0x1D
#define L_HRANGE 0x20
#define L_KRANGE2 0x23
#define L_PROTEC 0x24
#define L_FOOTER 0x25
#define L_HEADER 0x26
#define L_SETUP 0x27
#define L_MARGINS 0x28
#define L_LABELFMT 0x29
#define L_TITLES 0x2A
#define L_GRAPH 0x2D
#define L_CALCCOUNT 0x2F
#define L_UNFORMATTED 0x30
#define L_CURSORW12 0x31
#define L_HIDVEC1 0x64
#define L_PARSERANGES 0x66
#define L_RRANGES 0x67
#define L_MATRIXRANGES 0x69
#define L_CPI 0x96

/* mask to get format type */
#define L_FMTTYPE 0x70

/* mask to get decimal places */
#define L_DECPLC 0xF

/* protection bit */
#define L_PROT 0x80

/* format types */
#define L_FIXED (0 << 4)
#define L_SCIFI (1 << 4)
#define L_CURCY (2 << 4)
#define L_PERCT (3 << 4)
#define L_COMMA (4 << 4)
#define L_SPECL (7 << 4)

/* sub formats for special */
#define L_BARGRAPH 0
#define L_GENFMT 1
#define L_DDMMYY 2
#define L_DDMM 3
#define L_MMYY 4
#define L_TEXT 5
#define L_HIDDEN 6
#define L_DATETIME 7
#define L_DATETIMES 8
#define L_DATEINT1 9
#define L_DATEINT2 10
#define L_TIMEINT1 11
#define L_TIMEINT2 12
#define L_DEFAULT 15

/*
lotus formula opcodes
*/

/* constants */
#define LF_CONST 0
#define LF_SLR 1
#define LF_RANGE 2
#define LF_END 3
#define LF_BRACKETS 4
#define LF_INTEGER 5
#define LF_STRING 6

/* operators */
#define LF_UMINUS 8
#define LF_PLUS 9
#define LF_MINUS 10
#define LF_TIMES 11
#define LF_DIVIDE 12
#define LF_POWER 13
#define LF_EQUALS 14
#define LF_NOTEQUAL 15
#define LF_LTEQUAL 16
#define LF_GTEQUAL 17
#define LF_LT 18
#define LF_GT 19
#define LF_AND 20
#define LF_OR 21
#define LF_NOT 22
#define LF_UPLUS 23

/* functions */
#define LF_NA 31
#define LF_ERR 32
#define LF_ABS 33
#define LF_INT 34
#define LF_SQRT 35
#define LF_LOG 36
#define LF_LN 37
#define LF_PI 38
#define LF_SIN 39
#define LF_COS 40
#define LF_TAN 41
#define LF_ATAN2 42
#define LF_ATAN 43
#define LF_ASIN 44
#define LF_ACOS 45
#define LF_EXP 46
#define LF_MOD 47
#define LF_CHOOSE 48
#define LF_ISNA 49
#define LF_ISERR 50
#define LF_FALSE 51
#define LF_TRUE 52
#define LF_RAND 53
#define LF_DATE 54
#define LF_TODAY 55
#define LF_PMT 56
#define LF_PV 57
#define LF_FV 58
#define LF_IF 59
#define LF_DAY 60
#define LF_MONTH 61
#define LF_YEAR 62
#define LF_ROUND 63
#define LF_TIME 64
#define LF_HOUR 65
#define LF_MINUTE 66
#define LF_SECOND 67
#define LF_ISN 68
#define LF_ISS 69
#define LF_LENGTH 70
#define LF_VALUE 71
#define LF_FIXED 72
#define LF_MID 73
#define LF_CHR 74
#define LF_ASCII 75
#define LF_FIND 76
#define LF_DATEVALUE 77
#define LF_TIMEVALUE 78
#define LF_CELLPOINTER 79

/* variable argument functions */
#define LF_VARARG 80        /* start of arguments with variable args */
#define LF_SUM 80
#define LF_AVG 81
#define LF_CNT 82
#define LF_MIN 83
#define LF_MAX 84
#define LF_VLOOKUP 85
#define LF_NPV 86
#define LF_VAR 87
#define LF_STD 88
#define LF_IRR 89
#define LF_HLOOKUP 90
#define LF_DSUM 91
#define LF_DAVG 92
#define LF_DCNT 93
#define LF_DMIN 94
#define LF_DMAX 95
#define LF_DVAR 96
#define LF_DSTD 97
#define LF_INDEX 98
#define LF_COLS 99
#define LF_ROWS 100
#define LF_REPEAT 101
#define LF_UPPER 102
#define LF_LOWER 103
#define LF_LEFT 104
#define LF_RIGHT 105
#define LF_REPLACE 106
#define LF_PROPER 107
#define LF_CELL 108
#define LF_TRIM 109
#define LF_CLEAN 110
#define LF_S 111
#define LF_V 112
#define LF_STREQ 113
#define LF_CALL 114
#define LF_INDIRECT 115
#define LF_RATE 116
#define LF_TERM 117
#define LF_CTERM 118
#define LF_SLN 119
#define LF_SOY 120
#define LF_DDB 121
#define LF_AAFSTART 156
#define LF_AAFUNKNOWN 206
#define LF_AAFEND 255

#define LOTUS_MAXCOL 256
#define LOTUS_MAXROW 8192

/*
types of operators
*/

#define LO_UNARY 1
#define LO_BINARY 2
#define LO_FUNC 3
#define LO_CONST 4
#define LO_END 5
#define LO_BRACKETS 6

/*
table of PipeDream options page
constructs and equivalents
*/

typedef struct OPTDEF
{
	uchar *optstr;
	U16 opcode;
	U16 offset;
	U16 deflt;
	S32 (* wropt)();
}
* optp;

/*
table of LOTUS operators
and equivalents
*/

typedef struct OPRDEF
{
	uchar fno;
	S32 ftype;
	S32 nargs;
	S32 ltpok;
	uchar *pdeqv;
	S32 argix;
}
* oprp;

/*
index of argument modifiers
*/

#define FU_CHOOSE 1
#define FU_DATE 2
#define FU_INDEX 3
#define FU_DATEVAL 4

/*
scanner communication
*/

struct SYMBOL
{
	S32 symno;
	F64 fpval;
	U16 stcol;
	U16 strow;
	U16 encol;
	U16 enrow;
	uchar *stringp;
	S32 ixf;
	S32 scandate;
	S32 day;
	S32 mon;
	S32 yr;
};

/* special symbol constants */
#define SYM_BAD -1
#define SYM_BLANK -2
#define SYM_OBRACKET -3
#define SYM_CBRACKET -4
#define SYM_COMMA -5
#define SYM_FUNC -6

/*
table of lotus opcodes giving 1-2-3 file structure
*/

struct LFINS
{
	uchar opcode;
	S32 length;
	uchar *sdata;
	uchar dlen;
	uchar pattern;
	S32 (* writefunc)();
};

typedef struct LFINS *lfip;

/*
external definitions
*/

/* pd123.c */
extern S32           pd123__curpd;
extern S32           pd123__errexp;
extern FILE *        pd123__fin;
extern FILE *        pd123__fout;
extern S32           pd123__maxcol;
extern struct OPRDEF pd123__opreqv[];
extern S32           pd123__poorfunc;

extern S32
pd123__flookup(
    uchar *);

extern S32
pd123__foutc(
    S32,
    FILE *);

extern S32
pd123__ichlotus(
    S32);

extern S32
pd123__olookup(
    uchar *);

extern S32
pd123__searchdefo(
    uchar *);

/* 123pd.c */
extern uchar         pd123__hidvec[];
extern struct SYMBOL pd123__csym;

extern S32
pd123__scnslr(
    uchar *,
    U16 *,
    U16 *);

/* end of pd123.h */
