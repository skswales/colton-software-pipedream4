/* pd123.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Set of routines to convert between PipeDream files and Lotus 1-2-3 (rel 2) files */

/* MRJC February 1988 */

#if defined(UTIL_PTL) || defined(UTIL_LTP)
#define RELEASED 1
#ifndef CHECKING
#define CHECKING 0
#endif
#endif

/* target machine type - mutually exclusive options */

#if !defined(RISCOS) && !defined(WINDOWS)
#define RISCOS  1
#define WINDOWS 0
#endif

#include "cmodules/coltsoft/defineos.h"

#include "cmodules/coltsoft/ansi.h"

#include "cmodules/coltsoft/coltsoft.h"

/* external header */
#include "pd123.h"

/* local header file */
#include "pd123_i.h"

#if RISCOS
#include "cs-kernel.h"
#endif

#define TYPE_MATCH 0
#define WIDTH_MATCH 1
#define NEXT_ROW 2

#define P_DATE 1
#define P_TEXT 2

#define MAXSTACK 100

/*
function declarations
*/

extern S32
islotusfile(
    FILE *filein);

extern S32
readlotus(
    FILE *filein,
    FILE *fileout,
    S32 PD_type,
    S32 (* counterproc)(S32));

extern S32
pd123__flookup(
    const char *);

extern S32
pd123__foutc(
    S32,
    FILE *);

extern S32
pd123__ichlotus(
    S32);

extern S32
pd123__olookup(
    const char *);

extern S32
pd123__searchdefo(
    const char *);

/*
exported variables
*/

extern S32    pd123__curpd;                 /* current PipeDream level */
extern S32    pd123__errexp;                /* had error in expression */
extern FILE * pd123__fin;
extern FILE * pd123__fout;
extern S32    pd123__maxcol;
extern S32    pd123__poorfunc;              /* had a function that would't convert */

/*
internal functions
*/

static S32
addplusn(
    S32,
    S32);

static S32
checkdate(
    P_S32 ,
    long);

static S32
chksym(void);

static S32
compopr(
    const void * arg1,
    const void * arg2);

static uchar *
convconst(
    S32, uchar *, S32,
    S32);

static void
convslr(
    uchar *, P_S32 , uchar *, S32, S32, P_S32 ,
    P_S32 );

static S32
fachoose(
    P_S32 , uchar *, P_S32 ,
    P_S32 );

static S32
fadate(
    P_S32 , uchar *, P_S32 ,
    P_S32 );

static S32
fadateval(
    P_S32 , uchar *, P_S32 ,
    P_S32 );

static S32
faindex(
    P_S32 , uchar *, P_S32 ,
    P_S32 );

static uchar *
findrec(
    S32, S32, S32,
    S32);

static S32
foutcr(
    FILE *);

static S32
fptostr(
    char *,
    F64);

static void
freestk(void);

static S32
lotusich(S32);

static S32
outcon(
    const char *);

static S32
outdecplc(int);

static S32
outstr(
    const char *);

static S32
readrange(void);

static F64
lts_readdouble(
    const char *);

static U16
lts_readuword16(
    const char *);

static S16
lts_readword16(
    const char *);

static S32
scnsym(void);

static S32
startopt(optp);

static S32
wrcalcmode(optp);

static S32
wrcalcord(optp);

static S32
wrdecplc(optp);

static S32
wrheadfoot(optp);

static S32
writedate(long);

static S32
writeexp(
    uchar *, S32,
    S32);

static S32
writeformat(
    uchar *,
    P_S32 );

static S32
writeoptions(void);

static S32
writepcol(
    S32, S32,
    S32);

static S32
wrmar(
    optp,
    U16);

static S32
wrmargins(optp);

static S32
wrminbrk(optp);

static S32
wrsetmar(optp);

static S32
wrtextn(optp);

static S32
wrthousands(optp);

static S32
wrwrapmo(optp);

static S32
lotus_xtos(
    uchar *,
    S32);

/******************************************************************************
*
* table of characters for conversion between
* ISO 8859-1 character set and Lotus LICS character set
*
* the ISO character is first, followed by the corresponding LICS character
*
******************************************************************************/

static const unsigned char iso_lics[] =
{
    /* conversions are for reversibility */
#if 0
    0x79,   0xDD,   /* Y diaresis in LICS (lossy: Y here) */
#endif

#if RISCOS
    /* Acorn Latin-1 group */
    0x8D,   0xB8,   /* trademark */
    0x94,   0xA4,   /* upper left double quote  */
    0x9A,   0xD7,   /* OE */
    0x9B,   0xF7,   /* oe */
    0x96,   0xB4,   /* lower left double quote */
#endif

    /* ISO 8859-1 group */
    0xA0,   0x9A,   /* hard space */
    0xA4,   0xA8,   /* currency symbol */
    0xA8,   0x83,   /* diaresis */
    0xB4,   0x81,   /* acute accent */
    0xB9,   0x95,   /* superscript 1 */
    0xF7,   0xAF,   /* divide by */
    0xFF,   0xFD    /* y diaresis */
};

#define host_lics iso_lics

/******************************************************************************
*
* table of PipeDream options
* and LOTUS equivalents
*
* they are ordered as they appear in the
* 123 file to avoid a lot of searching
*
******************************************************************************/

static const OPTDEF optqv[] =
{
    { "AM",  L_CALCMODE,  0,  0xFF,   wrcalcmode },
    { "RC", L_CALCORDER,  0,     0,   wrcalcord },
    { "DP",   L_WINDOW1,  4,     2,   wrdecplc },
    { "MB",   L_WINDOW1,  4,     0,   wrminbrk },
    { "TH",   L_WINDOW1,  4,     0,   wrthousands },
    { "FO",    L_FOOTER,  0,     0,   wrheadfoot },
    { "HE",    L_HEADER,  0,     0,   wrheadfoot },
    { "BM",   L_MARGINS,  8,     8,   wrmargins },
    { "LM",   L_MARGINS,  0,     0,   wrmargins },
    { "PL",   L_MARGINS,  4,    66,   wrmargins },
    { "TM",   L_MARGINS,  6,     0,   wrmargins },
    { "FM",           0,  0,     2,   wrsetmar },
    { "HM",           0,  0,     2,   wrsetmar },
    { "WR",           0,  0,   'N',   wrwrapmo },
    { "TN",           0,  0,   'N',   wrtextn },
};

/******************************************************************************
*
* table of lotus operators and PipeDream equivalents
*
******************************************************************************/

const OPRDEF
pd123__opreqv[] =
{
    { LF_CONST,   LO_CONST,  0,      0, "",            0 },
    { LF_SLR,     LO_CONST,  0,      0, "",            0 },
    { LF_RANGE,   LO_CONST,  0,      0, "",            0 },
    { LF_END,     LO_END,    0,      0, "",            0 },
    { LF_BRACKETS,LO_BRACKETS,0,     0, "",            0 },
    { LF_INTEGER, LO_CONST,  0,      0, "",            0 },
    { LF_STRING,  LO_CONST,  0,      0, "",            0 },

    { LF_UMINUS,  LO_UNARY,  1,  PD_VP, "-",           0 },
    { LF_PLUS,    LO_BINARY, 2,  PD_VP, "+",           0 },
    { LF_MINUS,   LO_BINARY, 2,  PD_VP, "-",           0 },
    { LF_TIMES,   LO_BINARY, 2,  PD_VP, "*",           0 },
    { LF_DIVIDE,  LO_BINARY, 2,  PD_VP, "/",           0 },
    { LF_POWER,   LO_BINARY, 2,  PD_VP, "^",           0 },
    { LF_EQUALS,  LO_BINARY, 2,  PD_VP, "=",           0 },
    { LF_NOTEQUAL,LO_BINARY, 2,  PD_VP, "<>",          0 },
    { LF_LTEQUAL, LO_BINARY, 2,  PD_VP, "<=",          0 },
    { LF_GTEQUAL, LO_BINARY, 2,  PD_VP, ">=",          0 },
    { LF_LT,      LO_BINARY, 2,  PD_VP, "<",           0 },
    { LF_GT,      LO_BINARY, 2,  PD_VP, ">",           0 },
    { LF_AND,     LO_BINARY, 2,  PD_VP, "&",           0 },
    { LF_OR,      LO_BINARY, 2,  PD_VP, "|",           0 },
    { LF_NOT,     LO_UNARY,  1,  PD_VP, "!",           0 },
    { LF_UPLUS,   LO_UNARY,  1,  PD_VP, "+",           0 },

    { LF_NA,      LO_FUNC,   0,      0, ".na",         0 },
    { LF_ERR,     LO_FUNC,   0,      0, ".err",        0 },

    { LF_ABS,     LO_FUNC,   1,  PD_VP, "abs",         0 },
    { LF_INT,     LO_FUNC,   1,  PD_VP, "int",         0 },
    { LF_SQRT,    LO_FUNC,   1,  PD_VP, "sqr",         0 }, /* 123:SQRT */
    { LF_LOG,     LO_FUNC,   1,  PD_VP, "log",         0 }, /* 123 won't support second optional parameter */
    { LF_LN,      LO_FUNC,   1,  PD_VP, "ln",          0 },
    { LF_PI,      LO_FUNC,   0,  PD_VP, "pi",          0 },
    { LF_SIN,     LO_FUNC,   1,  PD_VP, "sin",         0 },
    { LF_COS,     LO_FUNC,   1,  PD_VP, "cos",         0 },
    { LF_TAN,     LO_FUNC,   1,  PD_VP, "tan",         0 },
    { LF_ATAN2,   LO_FUNC,   1,   PD_3, "atan_2",      0 }, /* 123:ATAN2 */
    { LF_ATAN,    LO_FUNC,   1,  PD_VP, "atn",         0 },
    { LF_ASIN,    LO_FUNC,   1,  PD_VP, "asn",         0 }, /* 123:ASIN */
    { LF_ACOS,    LO_FUNC,   1,  PD_VP, "acs",         0 }, /* 123:ACOS */
    { LF_EXP,     LO_FUNC,   1,  PD_VP, "exp",         0 },
    { LF_MOD,     LO_FUNC,   2,  PD_PC, "mod",         0 },
    { LF_CHOOSE,  LO_FUNC,  -1,  PD_VP, "choose",      FU_CHOOSE },
    { LF_ISNA,    LO_FUNC,   1,      0, ".isna",       0 },
    { LF_ISERR,   LO_FUNC,   1,      0, ".iserr",      0 },
    { LF_FALSE,   LO_FUNC,   0,  PD_VP, "0",           0 }, /* 123:FALSE */
    { LF_TRUE,    LO_FUNC,   1,  PD_VP, "1",           0 }, /* 123:TRUE */
    { LF_RAND,    LO_FUNC,   0,   PD_3, "rand",        0 },
    { LF_DATE,    LO_FUNC,   3,  PD_VP, "datef",       FU_DATE },
    { LF_TODAY,   LO_FUNC,   0,  PD_PC, "date",        0 },
    { LF_PMT,     LO_FUNC,   3,  PD_PC, "pmt",         0 },
    { LF_PV,      LO_FUNC,   3,  PD_PC, "pv",          0 },
    { LF_FV,      LO_FUNC,   3,  PD_PC, "fv",          0 },
    { LF_IF,      LO_FUNC,   3,  PD_VP, "if",          0 },
    { LF_DAY,     LO_FUNC,   1,  PD_VP, "day",         0 },
    { LF_MONTH,   LO_FUNC,   1,  PD_VP, "month",       0 },
    { LF_YEAR,    LO_FUNC,   1,  PD_VP, "year",        0 },
    { LF_ROUND,   LO_FUNC,   2,   PD_3, "round",       0 },
    { LF_TIME,    LO_FUNC,   3,      0, "time",        0 },
    { LF_HOUR,    LO_FUNC,   1,      0, "hour",        0 },
    { LF_MINUTE,  LO_FUNC,   1,      0, "minute",      0 },
    { LF_SECOND,  LO_FUNC,   1,      0, "second",      0 },
    { LF_ISN,     LO_FUNC,   1,      0, ".isnumber",   0 }, /* 123:ISNUMBER */
    { LF_ISS,     LO_FUNC,   1,      0, ".isstring",   0 }, /* 123:ISSTRING */
    { LF_LENGTH,  LO_FUNC,   1,      0, "length",      0 },
    { LF_VALUE,   LO_FUNC,   1,      0, "value",       0 },
    { LF_FIXED,   LO_FUNC,   1,      0, "fixed",       0 }, /* ??? */
    { LF_MID,     LO_FUNC,   3,      0, "mid",         0 },
    { LF_CHR,     LO_FUNC,   1,      0, "char",        0 },
    { LF_ASCII,   LO_FUNC,   1,      0, "code",        0 },
    { LF_FIND,    LO_FUNC,   3,      0, "find",        0 },
    { LF_DATEVALUE,LO_FUNC,  1,  PD_VP, "datevalue",   FU_DATEVAL },
    { LF_TIMEVALUE,LO_FUNC,  1,      0, "timevalue",   0 },
    { LF_CELLPOINTER,LO_FUNC,1,      0, ".cellpointer",0 },
    { LF_SUM,     LO_FUNC,  -1,  PD_VP, "sum",         0 },
    { LF_AVG,     LO_FUNC,  -1,  PD_PC, "avg",         0 },
    { LF_CNT,     LO_FUNC,  -1,  PD_VP, "counta",      0 }, /* 123:COUNT (maps to COUNTA according to Excel) */
    { LF_MIN,     LO_FUNC,  -1,  PD_VP, "min",         0 },
    { LF_MAX,     LO_FUNC,  -1,  PD_VP, "max",         0 },
    { LF_VLOOKUP, LO_FUNC,   3,  PD_PC, "vlookup",     0 },
    { LF_NPV,     LO_FUNC,   2,  PD_PC, "npv",         0 },
    { LF_VAR,     LO_FUNC,  -1,  PD_PC, "varp",        0 }, /* 123:VAR (maps to VARP according to Excel) */
    { LF_STD,     LO_FUNC,  -1,  PD_PC, "stdp",        0 }, /* 123:STD (maps to STDEVP according to Excel) */
    { LF_IRR,     LO_FUNC,   2,  PD_PC, "irr",         0 },
    { LF_HLOOKUP, LO_FUNC,   3,  PD_PC, "hlookup",     0 },
    { LF_DSUM,    LO_FUNC,   3,      0, "dsum",        0 },
    { LF_DAVG,    LO_FUNC,   3,      0, "davg",        0 },
    { LF_DCNT,    LO_FUNC,   3,      0, "dcounta",     0 },
    { LF_DMIN,    LO_FUNC,   3,      0, "dmin",        0 },
    { LF_DMAX,    LO_FUNC,   3,      0, "dmax",        0 },
    { LF_DVAR,    LO_FUNC,   3,      0, "dvarp",       0 }, /* 123:DVAR (maps to DVARP according to Excel) */
    { LF_DSTD,    LO_FUNC,   3,      0, "dstdp",       0 }, /* 123:DSTD (maps to DSTDEVP according to Excel) */
    { LF_INDEX,   LO_FUNC,   3,  PD_VP, "index",       FU_INDEX },
    { LF_COLS,    LO_FUNC,   1,      0, "cols",        0 },
    { LF_ROWS,    LO_FUNC,   1,      0, "rows",        0 },
    { LF_REPEAT,  LO_FUNC,   2,      0, "rept",        0 }, /* 123:REPEAT */
    { LF_UPPER,   LO_FUNC,   1,      0, "upper",       0 },
    { LF_LOWER,   LO_FUNC,   1,      0, "lower",       0 },
    { LF_LEFT,    LO_FUNC,   2,      0, "left",        0 },
    { LF_RIGHT,   LO_FUNC,   2,      0, "right",       0 },
    { LF_REPLACE, LO_FUNC,   4,      0, "replace",     0 },
    { LF_PROPER,  LO_FUNC,   1,      0, "proper",      0 },
    { LF_CELL,    LO_FUNC,   2,      0, ".cell",       0 },
    { LF_TRIM,    LO_FUNC,   1,      0, "trim",        0 },
    { LF_CLEAN,   LO_FUNC,   1,      0, ".clean",      0 },
    { LF_S,       LO_FUNC,   1,      0, ".t",          0 }, /* 123:"S" */
    { LF_V,       LO_FUNC,   1,      0, ".n",          0 }, /* 123:"V" now "N"? */
    { LF_STREQ,   LO_FUNC,   2,      0, "exact",       0 },
    { LF_CALL,    LO_FUNC,   1,      0, ".call",       0 },
    { LF_INDIRECT,LO_FUNC,   1,      0, ".indirect",   0 }, /* end of 1985 documentation */

    { LF_RATE,    LO_FUNC,   3,  PD_PC, "rate",        0 },
    { LF_TERM,    LO_FUNC,   3,  PD_PC, "term",        0 },
    { LF_CTERM,   LO_FUNC,   3,  PD_PC, "cterm",       0 },
    { LF_SLN,     LO_FUNC,   3,  PD_PC, "sln",         0 },
    { LF_SOY,     LO_FUNC,   4,  PD_PC, "syd",         0 },
    { LF_DDB,     LO_FUNC,   4,  PD_PC, "ddb",         0 }
};

/******************************************************************************
*
* table of function addresses
* for argument modifying
*
******************************************************************************/

static S32 (* argfuddle[])(P_S32 , uchar *, P_S32 , P_S32 ) =
{
    fachoose,
    fadate,
    faindex,
    fadateval,
};

/* input and output files */
FILE *pd123__fin;
FILE *pd123__fout;

S32 pd123__maxcol;

/* lotus file headers */
static const char lfhead[4]   = { '\x0', '\x0', '\x2', '\x0' };
static const char lfh123[2]   = { '\x4', '\x4' };
static const char lfh123_2[2] = { '\x6', '\x4' };

/* Global parameters for lotus file */
static S32 sc, ec, sr, er;
static S32 defcwid;
static S32 foundeof;

/* array which contains the lotus file */
static uchar *lotusf;

/* current position in lotus file */
static uchar *curpos;

/* RPN recogniser variables */
static uchar *termp;              /* scanner index */
static S32 cursym;                /* current symbol */
static uchar *argstk[MAXSTACK];   /* stack of arguments */
static S32 argsp;                 /* argument stack pointer */
static oprp curopr;               /* current symbol structure */

/* days in the month */
static const signed char days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* PipeDream level */
S32 pd123__errexp;                /* had error in expression */
S32 pd123__poorfunc;              /* had a function that would't convert */
S32 pd123__curpd;                 /* current PipeDream level */

/******************************************************************************
*
* main loop for Lotus 1-2-3 to PipeDream
* this main procedure is compiled if the constant
* UTIL_LTP is defined:
*
* UTIL_LTP creates stand-alone utility for Lotus 1-2-3 to PipeDream
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus 1-2-3
* INT_LPTPL creates internal code for converter using function lp_lptpl
*
******************************************************************************/

#ifdef UTIL_LTP

static size_t
read_file_size(FILE * f)
{
    size_t n_bytes;

    if(fseek(f, 0L, SEEK_END))
        return(0);

    n_bytes = (size_t) ftell(f);

    if(fseek(f, 0L, SEEK_SET))
        return(0);

    return(n_bytes);
}

int
main(
    int argc,
    char **argv)
{
    S32 col, err;
    size_t length;
    const char * infile;
    const char * outfile;

    /* banner */
    puts("Lotus 1-2-3 to PipeDream converter: (C) 1988-2023 Colton Software");

    /* argument checking */
    if(argc < 3)
    {
        fprintf(stderr, "Syntax: %s <infile> <outfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    infile = *++argv;
    outfile = *++argv;

    if((pd123__fin = fopen(infile, "rb")) == NULL)
    {
        fprintf(stderr, "Can't open %s for reading\n", infile);
        exit(EXIT_FAILURE);
    }

    if(0 == (length = read_file_size(pd123__fin)))
    {
        fprintf(stderr, "Unable to read length of %s\n", infile);
        return(EXIT_FAILURE);
    }

    if(0 == (lotusf = malloc(length)))
    {
        printf("Not enough memory to load %s\n", infile);
        exit(EXIT_FAILURE);
    }

    /* read in Lotus 1-2-3 file */
    fread(lotusf, 1, length, pd123__fin);
    fclose(pd123__fin);

    if((pd123__fout = fopen(outfile, "wb")) == NULL)
    {
        fprintf(stderr, "Can't open %s for writing\n", outfile);
        exit(EXIT_FAILURE);
    }

    /* initialise variables */
    curpos = NULL;
    pd123__errexp = 0;
    pd123__poorfunc = 0;
    pd123__curpd = PD_Z88;

    /* read in range of 123 file */
    if(0 == (err = readrange()))
    {
        /* write out options to PipeDream file */
        err = writeoptions();

        /* write out columns */
        if(!err)
        {
            for(col = sc; (col <= ec) && !(foundeof && (col > pd123__maxcol)); ++col)
            {
                uchar scol[10];
                S32 cr;

                if(0 != (err = writepcol(col, sr, er)))
                    break;
                cr = lotus_xtos(scol, col);
                scol[cr] = CH_NULL;
                printf("\nColumn: %s", scol);
            }
            puts("\n");
        }
    }

    switch(err)
    {
    case PD123_ERR_MEM:
        fprintf(stderr, "Out of memory\n");
        break;
    case PD123_ERR_FILE:
        perror("File error");
        break;
    case PD123_ERR_BADFILE:
        fprintf(stderr, "Bad Lotus 1-2-3 file\n");
        break;
    default:
        break;
    }

    fclose(pd123__fout);
    free(lotusf);

    if(err)
    {
        remove(outfile);
        puts("Conversion failed");
        return(EXIT_FAILURE);
    }

    if(pd123__errexp)
        fprintf(stderr, "%d bad expressions found\n", pd123__errexp);
    if(pd123__poorfunc)
        fprintf(stderr, "Unable to convert %d Lotus 1-2-3 functions to PipeDream\n", pd123__poorfunc);

    {
    _kernel_osfile_block fileblk;
    fileblk.load /*r2*/ = 0xDDE /*FILETYPE_PIPEDREAM*/;
    (void) _kernel_osfile(18 /*SetType*/, outfile, &fileblk);
    } /*block*/

    puts("Conversion complete");
    return(EXIT_SUCCESS);
}

#endif

/******************************************************************************
*
* function to read a lotus file called from elsewhere
*
* --in--
* fin and fout are file pointers to open channels
* level specifies the level of the PipeDream file
* routine is the address of a routine to call to show activity
* this returns non-zero if an abort is required
*
******************************************************************************/

#ifdef INT_LPTPL
extern S32
readlotus(
    FILE *inf,
    FILE *outf,
    S32 level,
    S32 (* counterproc)(S32))
{
    S32 col, err;
    size_t length;

    /* save file pointers */
    pd123__fin = inf;
    pd123__fout = outf;

    /* set level of conversion */
    pd123__curpd = level;

    if(fseek(pd123__fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    length = (size_t) ftell(pd123__fin);
    if(fseek(pd123__fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);

    if((lotusf = malloc(length)) == NULL)
        return(PD123_ERR_MEM);

    /* read in LOTUS file */
    fread(lotusf, 1, length, pd123__fin);

    /* initialise variables */
    curpos = NULL;
    pd123__errexp = 0;
    pd123__poorfunc = 0;

    /* read in range of 123 file */
    if((err = readrange()) == 0)
    {
        /* write out options to PipeDream file */
        err = writeoptions();

        /* write out columns */
        if(!err)
            for(col = sc; (col <= ec) && !(foundeof && (col > pd123__maxcol)); ++col)
            {
                if((err = writepcol(col, sr, er)) != 0)
                    break;

                /* call activity routine and check for abort */
                if(counterproc)
                    if((*counterproc)( (((S32) col - sc) * 100) / ((S32) pd123__maxcol + 1) ))
                        break;
            }
    }

    free(lotusf);

    if(!err && (pd123__errexp || pd123__poorfunc))
        err = PD123_ERR_EXP;

    return(err);
}
#endif

/******************************************************************************
*
* function to add "+n" to an argument
*
******************************************************************************/

static S32
addplusn(
    S32 arg,
    S32 n)
{
    char nstr[10];
    char *tstr;
    const char *cele;
    S32 lcele, ln;

    /* add +1 to argument */
    cele = argstk[argsp - arg];
    lcele = strlen(cele);
    ln = sprintf(nstr, "+%d", n);
    if((tstr = malloc(lcele + ln + 1)) == NULL)
        return(PD123_ERR_MEM);
    strcpy(tstr, cele);
    strncpy(tstr + lcele, nstr, ln);
    tstr[lcele + ln] = CH_NULL;
    argstk[argsp - arg] = tstr;
    free((void *) cele);
    return(0);
}

/******************************************************************************
*
* handle special formats for dates and things
*
* if no more output for a cell is required,
* specflg is set to zero
*
******************************************************************************/

static S32
checkdate(
    P_S32 specflg,
    long value)
{
    switch(*specflg)
    {
    case 0:
        if(fprintf(pd123__fout, "%%V%%") < 0)
            return(PD123_ERR_FILE);
        break;
    case P_TEXT:
        break;
    case P_DATE:
        *specflg = 0;
        if(fprintf(pd123__fout, "%%V%%") < 0)
            return(PD123_ERR_FILE);
        return(writedate(value));
    }
    *specflg = -1;
    return(0);
}

/******************************************************************************
*
* symbol lookahead
*
******************************************************************************/

static S32
chksym(void)
{
    cursym = *termp;
    curopr = (struct OPRDEF *) bsearch(&cursym,
                                       pd123__opreqv,
                                       elemof(pd123__opreqv),
                                       sizeof(struct OPRDEF), compopr);
    return(cursym);
}

/******************************************************************************
*
* compare operators
*
******************************************************************************/

static int
compopr(
    const void * arg1,
    const void * arg2)
{
    uchar ch1 = * (const uchar *) arg1;
    uchar ch2 = * (const uchar *) arg2;

    if(ch1 < ch2)
        return(-1);

    if(ch1 == ch2)
        return(0);

    return(1);
}

/******************************************************************************
*
* convert current constant to a string
*
* string is allocated with malloc
* and must be freed when not needed
*
******************************************************************************/

static uchar *
convconst(
    S32 opr,
    uchar *arg, S32 col,
    S32 row)
{
    S32 reslen = 0;
    char resstr[256];
    uchar *res;
    uchar *c;

    switch(opr)
    {
    case LF_CONST:
        reslen = fptostr(resstr, lts_readdouble(arg));
        break;

    case LF_SLR:
        convslr(resstr, &reslen, arg, col, row, NULL, NULL);
        break;

    case LF_RANGE:
        {
        S32 cc = 0, cr = 0;

        convslr(resstr, &reslen, arg, col, row, &cc, &cr);
        convslr(resstr, &reslen, arg + 4, col, row, &cc, &cr);
        break;
        }

    case LF_INTEGER:
        {
        S32 ival;

        ival = (S32) lts_readword16(arg);
        reslen = sprintf(resstr, "%d", ival);
        break;
        }

    case LF_STRING:
        c = resstr;
        *c++ = '"';
        for(reslen = 0; *arg; reslen++)
            *c++ = (uchar) lotusich((S32) *arg++);
        *c++= '"';
        reslen += 2;
        break;
    }

    resstr[reslen] = CH_NULL;
    if((res = malloc(reslen + 1)) != NULL)
        strncpy(res, resstr, reslen + 1);
    return(res);
}

/******************************************************************************
*
* convert SLR
*
******************************************************************************/

static void
convslr(
    uchar *resstr,
    P_S32 reslen,
    uchar *arg,
    S32 tcol,
    S32 trow,
    P_S32 cc,
    P_S32 cr)
{
    S32 col_dollar = 0;
    S32 row_dollar = 0;
    S32 tlen = 0;
    S32 col, row;
    uchar tstr[50];

    col = lts_readuword16(arg);
    arg += 2;
    col_dollar = ((col & 0x8000) == 0);
    col &= 0x00FF;
    if(!col_dollar)
        col = tcol + ((col << (32 - 8)) >> (32 - 8)); /* sign-extend for relative cell reference */

    row = lts_readuword16(arg);
    arg += 2;
    row_dollar = ((row & 0x8000) == 0);
    row &= 0x1FFF;
    if(!row_dollar)
        row = trow + ((row << (32 - 13)) >> (32 - 13)); /* sign-extend for relative cell reference */
    ++row;

    if(col_dollar)
    {
        *(tstr + tlen) = '$';
        ++tlen;
    }
    tlen += lotus_xtos(tstr + tlen, col);

    if(row_dollar)
        *(tstr + tlen++) = '$';
    tlen += sprintf(tstr + tlen, "%d", row);

    /* save actual addresses for comparison */
    if(!*reslen && cc)
        *cc = col;
    if(!*reslen && cr)
        *cr = row;

    /* if on second ref., check for highest and swap */
    if(*reslen && ((cc && (col < *cc)) || (cr && (row < *cr))))
    {
        memmove32(resstr + tlen, resstr, *reslen);
        strncpy(resstr, tstr, tlen);
    }
    else
    {
        /* add second string */
        strncpy(resstr + *reslen, tstr, tlen);
    }
    *reslen += tlen;
}

/******************************************************************************
*
* fuddle the arguments to choose
*
******************************************************************************/

static S32
fachoose(
    P_S32 narg,
    uchar *argsep,
    P_S32 nobrk,
    P_S32 noname)
{
    UNREFERENCED_PARAMETER(argsep);
    UNREFERENCED_PARAMETER(nobrk);
    UNREFERENCED_PARAMETER(noname);

    /* add +1 to first argument */
    return(addplusn(*narg, 1));
}

/******************************************************************************
*
* fuddle the arguments to the date function
*
******************************************************************************/

static S32
fadate(
    P_S32 narg,
    uchar *argsep,
    P_S32 nobrk,
    P_S32 noname)
{
    uchar *temp;
    S32 tmp, i;

    UNREFERENCED_PARAMETER(narg);

    /* check that we have three literal numbers */
    for(i = 0; i < 3; ++i)
        if(sscanf(argstk[argsp - i - 1], "%d", &tmp) != 1)
            return(0);

    /* swap year and day */
    temp = argstk[argsp - 1];
    argstk[argsp - 1] = argstk[argsp - 3];
    argstk[argsp - 3] = temp;

    /* return dot separator */
    *noname = 1;
    *nobrk = 1;
    *argsep = '.';
    return(0);
}

/******************************************************************************
*
* fuddle the arguments to the dateval function
*
******************************************************************************/

static S32
fadateval(
    P_S32 narg,
    uchar *argsep,
    P_S32 nobrk,
    P_S32 noname)
{
    char tstr[25];
    uchar *nele;
    S32 day, mon, yr, cr;

    UNREFERENCED_PARAMETER(narg);
    UNREFERENCED_PARAMETER(argsep);

    /* check that we have a literal string */
    if(sscanf(argstk[argsp - 1], "\"%d.%d.%d.\"", &day, &mon, &yr) != 3)
        return(0);

    cr = sprintf(tstr, "%d.%d.%d", day, mon, yr);
    tstr[cr] = CH_NULL;

    /* get rid of first argument */
    if((nele = malloc(strlen(tstr) + 1)) == NULL)
        return(PD123_ERR_MEM);

    free(argstk[argsp - 1]);
    strcpy(nele, tstr);
    argstk[argsp - 1] = nele;
    *noname = 1;
    *nobrk = 1;
    return(0);
}

/******************************************************************************
*
* fuddle the arguments to the index function
*
******************************************************************************/

static S32
faindex(
    P_S32 narg,
    uchar *argsep,
    P_S32 nobrk,
    P_S32 noname)
{
    S32 err;
    U16 col = 0, row = 0;

    UNREFERENCED_PARAMETER(argsep);
    UNREFERENCED_PARAMETER(nobrk);
    UNREFERENCED_PARAMETER(noname);

    /* read the first slr in the first argument */
    pd123__scnslr(argstk[argsp - 3], &col, &row);

    /* get rid of first argument */
    free(argstk[argsp - 3]);

    /* shift up the other two arguments */
    argstk[argsp - 3] = argstk[argsp - 2];
    argstk[argsp - 2] = argstk[argsp - 1];
    --argsp;

    /* decrement argument count */
    *narg -= 1;

    /* adjust remaining arguments */
    if((err = addplusn(2, row + 1)) != 0)
        return(err);
    return(addplusn(1, col + 1));
}

/******************************************************************************
*
* locate a record of a given type in the lotus file
*
* --in--
* type contains type of record to locate
* flag indicates type of search:
*   0 specific type
*   1 column width record
*   2 next row after a given row
*
* --out--
* pointer to record body
* NULL if record not found
*
******************************************************************************/

static uchar *
findrec(
    S32 type,
    S32 aflag,
    S32 col,
    S32 row)
{
    uchar *startpos, *atpos, *datapos;
    U16 opcode, length;

    /* load start of file */
    if(!curpos)
        curpos = lotusf;

    /* remember start position */
    startpos = curpos;

    //printf("findrec(type=0x%04X, aflag=%d, col=%d, row=%d): curpos=0x%x\n", type, aflag, col, row, curpos-lotusf);

    /* search for required opcode */
    do
    {
        /* read opcode */
        atpos = curpos;
        opcode = lts_readuword16(atpos);
        atpos += 2;
        length = lts_readuword16(atpos);
        atpos += 2;
        //printf("  @0x%x opcode=0x%04X, length=0x%x\n", (atpos-4)-lotusf, opcode, length);

        switch(aflag)
        {
        case TYPE_MATCH:
            if(opcode == type)
            {
                //printf("  MATCH\n");
                return(atpos);
            }
            break;

        case WIDTH_MATCH:
            if(opcode == type)
            {
                S32 c;

                datapos = atpos;

                c = (S32) lts_readuword16(atpos);
                atpos += 2;
                if(col == c)
                {
                    //printf("  WIDTH_MATCH for c=%d\n", c);
                    return(datapos);
                }
            }
            break;

        case NEXT_ROW:
            if(opcode == L_INTEGER ||
               opcode == L_NUMBER ||
               opcode == L_LABEL ||
               opcode == L_FORMULA)
            {
                S32 c, r;

                /* skip format byte */
                datapos = atpos++;
                c = (S32) lts_readuword16(atpos);
                atpos += 2;
                r = (S32) lts_readuword16(atpos);
                atpos += 2;

                /* set maximum column found */
                pd123__maxcol = MAX(c, pd123__maxcol);
                if((c == col) && (r > row))
                {
                    //printf("  NEXT_ROW for c=%d, r=%d\n", c, r);
                    return(datapos);
                }
            }
            break;
        }

        /* advance to next record */
        if(opcode == L_EOF)
        {
            if(aflag == NEXT_ROW)
            {
                foundeof = 1;
                //puts("EOF for NEXT_ROW");
                return(NULL);
            }

            curpos = lotusf; /* wrap to start and keep trying */
            //if(curpos != startpos) puts("EOF - wrap to start");
        }
        else
        {
            curpos += length + 4;
        }
    }
    while(curpos != startpos);

    //puts("Not found in whole file");
    return(NULL);
}

/******************************************************************************
*
* lookup function in master table
*
******************************************************************************/

extern S32
pd123__flookup(
    const char *func)
{
    U32 i;

    for(i = 0; i < elemof32(pd123__opreqv); ++i)
    {
        const oprp op = &pd123__opreqv[i];

        if(op->ftype != LO_FUNC)
            continue;

        if(0 == strcmp(func, op->pdeqv))
        {
            pd123__csym.ixf = i;
            pd123__csym.symno = op->fno;
            return(strlen(op->pdeqv));
        }
    }

    return(pd123__csym.symno = SYM_BAD);
}

/******************************************************************************
*
* output character to file and check for errors properly
*
******************************************************************************/

extern S32
pd123__foutc(
    S32 ch,
    FILE *file)
{
    if(putc(ch, file) != EOF)
        return(0);

    if(ferror(file))
        return(PD123_ERR_FILE);

    return(0);
}

/******************************************************************************
*
* output CRLF to file and check for errors
*
******************************************************************************/

static S32
foutcr(
    FILE *file)
{
    S32 err;

    if((err = pd123__foutc(CR, file)) != 0)
        return(err);

    return(pd123__foutc(LF, file));
}

/******************************************************************************
*
* convert floating point number to string for PipeDream
*
******************************************************************************/

static S32
fptostr(
    char *resstr,
    F64 fpval)
{
    char *exp, *exps, sign;
    S32 reslen;

    reslen = sprintf(resstr, "%.15g", fpval);
    resstr[reslen] = CH_NULL;

    /* search for exponent and remove leading zeros because they confuse the Z88; remove the + for good measure */
    if((exp = strstr(resstr, "e")) != NULL)
    {
        sign = *(++exp);
        exps = exp;
        if(sign == '-')
        {
            ++exp;
            ++exps;
        }
        if(sign == '+')
            ++exp;
        while(*exp == '0')
            ++exp;
        strncpy(exps, exp, reslen - (exp - resstr));
        reslen = reslen - (exp - exps);
        resstr[reslen] = CH_NULL;
    }

    return(reslen);
}

/******************************************************************************
*
* free all elements on stack
*
******************************************************************************/

static void
freestk(void)
{
    __assume(argsp < elemof32(argstk));
    while(argsp)
        free(argstk[--argsp]);
}

/******************************************************************************
*
* convert international character to lotus character
*
******************************************************************************/

extern S32
pd123__ichlotus(
    S32 ch)
{
    U32 i;

    if((ch < 32) || (ch > 127))
        for(i = 0; i < elemof32(host_lics); i += 2)
            if(host_lics[i] == (S32) ch)
                return(host_lics[i + 1]);

    return(ch);
}

/******************************************************************************
*
* is this a lotus file?
*
******************************************************************************/

#define HEADLEN 6

extern S32
islotusfile(
    FILE *filein)
{
    char buffer[HEADLEN];
    fpos_t fpos;
    S32 res = FALSE;

    if(fgetpos(filein, &fpos))
        return(PD123_ERR_FILE);

    if(fseek(filein, 0L, SEEK_SET))
        res = PD123_ERR_FILE;

    if((res >= 0)  &&  (fread(buffer, 1, HEADLEN, filein) < HEADLEN))
        res = PD123_ERR_FILE;

    /* check start of file */
    if((res >= 0)  &&  (0 == memcmp(buffer, lfhead, 4)))
        if((0 == memcmp(buffer+ 4, lfh123, 2))  ||  (0 == memcmp(buffer + 4, lfh123_2, 2)))
            res = TRUE;

    if(fsetpos(filein, &fpos)  &&  (res >= 0))
        res = PD123_ERR_FILE;

    return(res);
}

/******************************************************************************
*
* convert lotus character to international character
*
******************************************************************************/

static S32
lotusich(
    S32 ch)
{
    U32 i;

    if((ch < 32) || (ch > 127))
        for(i = 0; i < elemof32(host_lics); i += 2)
            if(host_lics[i + 1] == (S32) ch)
                return(host_lics[i]);

    return(ch);
}

/******************************************************************************
*
* lookup operator in master table
*
* --out--
* returns size in characters of operator
*
******************************************************************************/

extern S32
pd123__olookup(
    const char *opr)
{
    U32 i;

    for(i = 0; i < elemof32(pd123__opreqv); ++i)
    {
        oprp op = &pd123__opreqv[i];

        if((op->ftype != LO_BINARY) && (op->fno != LF_NOT))
            continue;

        if(0 == strncmp(opr, op->pdeqv, strlen(op->pdeqv)))
        {
            pd123__csym.ixf = i;
            pd123__csym.symno = op->fno;
            return(strlen(op->pdeqv));
        }
    }

    return(pd123__csym.symno = SYM_BAD);
}

/******************************************************************************
*
* output construct to file
*
* --in--
* pointer to construct text
*
******************************************************************************/

static S32
outcon(
    const char *cons)
{
    S32 err;

    if((err = pd123__foutc((S32) '%', pd123__fout)) != 0)
        return(err);

    if((err = outstr(cons)) != 0)
        return(err);

    if((err = pd123__foutc((S32) '%', pd123__fout)) != 0)
        return(err);

    return(0);
}

/******************************************************************************
*
* write decimal place construct to output
*
******************************************************************************/

static S32
outdecplc(
    S32 decplc)
{
    S32 err;

    if((err = outstr("%D")) != 0)
        return(err);
    if(fprintf(pd123__fout, "%d", decplc) < 0)
        return(PD123_ERR_FILE);
    return(pd123__foutc('%', pd123__fout));
}

/******************************************************************************
*
* output string to file
*
******************************************************************************/

static S32
outstr(
    const char *str)
{
    S32 err;

    while(*str)
        if((err = pd123__foutc((S32) *str++, pd123__fout)) != 0)
            return(err);
    return(0);
}

/******************************************************************************
*
* read LOTUS file limits
*
******************************************************************************/

static S32
readrange(void)
{
    uchar *rec;
    S32 i;

    /* check start of file */
    if(0 != memcmp(lotusf, lfhead, 4))
        return(PD123_ERR_BADFILE);
    if(0 != memcmp(lotusf + 4, lfh123, 2) && memcmp(lotusf + 4, lfh123_2, 2))
        return(PD123_ERR_BADFILE);

    if((rec = findrec(L_RANGE, TYPE_MATCH, 0, 0)) != NULL)
    {
        sc = (S32) lts_readuword16(rec);
        rec += 2;
        sr = (S32) lts_readuword16(rec);
        rec += 2;
        ec = (S32) lts_readuword16(rec);
        rec += 2;
        er = (S32) lts_readuword16(rec);
        rec += 2;
    }
    else
    {
        return(PD123_ERR_BADFILE);
    }

    if((sc == -1) || (ec == -1) || (ec == 0) || (er == 0))
    {
        sc = sr = 0;
        ec = LOTUS_MAXCOL - 1;
        er = LOTUS_MAXROW - 1;
    }

    pd123__maxcol = -1;
    foundeof = 0;

    /* read default column width */
    if((rec = findrec(L_WINDOW1, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += 6;
        defcwid = (S32) lts_readuword16(rec);
        rec += 2;
        //printf("L_WINDOW1 found: Default column width = %d\n", defcwid);
    }
    else
    {   /* Schema2 converter doesn't output this? */
        defcwid = 12; // now same as PipeDream default - was zero, and PipeDream doesn't like all zeros!
        //printf("L_WINDOW1 not found: Default column width = %d\n", defcwid);
    }

    /* read hidden column vector */
    if((rec = findrec(L_HIDVEC1, TYPE_MATCH, 0, 0)) != NULL)
    {
        for(i = 0; i < 32; ++i)
            pd123__hidvec[i] = *rec++;
    }
    else
    {
        for(i = 0; i < 32; ++i)
            pd123__hidvec[i] = 0;
    }
    return(0);
}

/******************************************************************************
*
* read a double from memory
*
******************************************************************************/

static F64
lts_readdouble(
    const char *arg)
{
    /* this for the ARM <-> 8087 */
    U32 i;
    union LTS_READDOUBLE_U
    {
        F64 fpval;
        char fpbytes[8];
    } fp;

    for(i = 4; i < 8; ++i)
        fp.fpbytes[i] = *arg++;

    for(i = 0; i < 4; ++i)
        fp.fpbytes[i] = *arg++;

    return(fp.fpval);
}

/******************************************************************************
*
* read an unsigned word from memory
*
******************************************************************************/

static U16
lts_readuword16(
    const char *arg)
{
    /* this for the ARM */
    union LTS_READUWORD16_U
    {
        U16 uword;
        char bytes[2];
    } uw;

    uw.bytes[0] = *arg++;
    uw.bytes[1] = *arg++;

    return(uw.uword);
}

/******************************************************************************
*
* read a signed word from memory
*
******************************************************************************/

static S16
lts_readword16(
    const char *arg)
{
    /* this for the ARM */
    union LTS_READWORD16_U
    {
        S16 word;
        char bytes[2];
    } w;

    w.bytes[0] = *arg++;
    w.bytes[1] = *arg++;

    return(w.word);
}

/******************************************************************************
*
* RPN scanner
*
******************************************************************************/

static S32
scnsym(void)
{
    if(cursym != -1)
    {
        /* work out how to skip symbol */
        ++termp;
        switch(curopr->ftype)
        {
        case LO_CONST:
            switch(cursym)
            {
            case LF_CONST:
                termp += 8;
                break;
            case LF_SLR:
                termp += 4;
                break;
            case LF_RANGE:
                termp += 8;
                break;
            case LF_INTEGER:
                termp += 2;
                break;
            case LF_STRING:
                while(*termp++)
                    ;
                break;
            default:
                break;
            }
            break;

        case LO_FUNC:
            /* skip argument count */
            if(curopr->n_args == -1)
                ++termp;
            break;

        default:
            break;
        }
    }
    return(chksym());
}

/******************************************************************************
*
* search default table for option
*
******************************************************************************/

extern S32
pd123__searchdefo(
    const char *optid)
{
    U32 i;

    for(i = 0; i < elemof32(optqv); ++i)
        if(0 == strcmp(optid, optqv[i].optstr))
            return((S32) optqv[i].deflt);

    return(0);
}

/******************************************************************************
*
* write out option start and id
*
******************************************************************************/

static S32
startopt(
    optp op)
{
    S32 err;

    if((err = outcon("OP")) != 0)
        return(err);
    if(fprintf(pd123__fout, "%s", op->optstr) < 0)
        return(PD123_ERR_FILE);
    return(0);
}

/******************************************************************************
*
* write out calculation mode
*
******************************************************************************/

static S32
wrcalcmode(
    optp op)
{
    uchar *rec;
    S32 err;
    uchar cm;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        cm = *(rec + op->offset);
        if(!cm)
        {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = pd123__foutc('M', pd123__fout)) != 0)
                return(err);
            if((err = foutcr(pd123__fout)) != 0)
                return(err);
        }
    }
    return(0);
}

/******************************************************************************
*
* write out calculation order
*
******************************************************************************/

static S32
wrcalcord(
    optp op)
{
    uchar *rec, calc;
    char calco;
    S32 err;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        calc = *(rec + op->offset);
        if((err = startopt(op)) != 0)
            return(err);
        if(pd123__curpd >= PD_3)
        {
            switch(calc)
            {
            case 0xFF:
                calco = 'R';
                break;
            case 1:
                calco = 'C';
                break;
            default:
            case 0:
                calco = 'N';
                break;
            }
            if((err = pd123__foutc(calco, pd123__fout)) != 0)
                return(err);
        }
        else if((err = pd123__foutc(((calc == 0xFF) ? 'R' : 'C'), pd123__fout)) != 0)
            return(err);
        if((err = foutcr(pd123__fout)) != 0)
            return(err);
    }
    return(0);
}

/******************************************************************************
*
* write out decimal places
*
******************************************************************************/

static S32
wrdecplc(
    optp op)
{
    uchar *rec;
    S32 err, decplc;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += op->offset;

        if((*rec & L_FMTTYPE) != L_SPECL)
        {
            decplc = (S32) (*rec & L_DECPLC);
            if(decplc != op->deflt)
            {
                if((err = startopt(op)) != 0)
                    return(err);
                if(fprintf(pd123__fout, "%d", decplc) < 0)
                    return(PD123_ERR_FILE);
                if((err = foutcr(pd123__fout)) != 0)
                    return(err);
            }
        }
        else
        {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = pd123__foutc('F', pd123__fout)) != 0)
                return(err);
            if((err = foutcr(pd123__fout)) != 0)
                return(err);
        }
    }
    return(0);
}

/******************************************************************************
*
* write out header and footer
*
******************************************************************************/

static S32
wrheadfoot(
    optp op)
{
    uchar *rec;
    S32 err;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += op->offset;
        if(*rec)
        {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = pd123__foutc('|', pd123__fout)) != 0)
                return(err);
            while(*rec)
            {
                /* check for lotus page number */
                if(*rec == '#')
                {
                    if((err = outstr("@P@")) != 0)
                        return(err);
                    ++rec;
                }
                /* check for lotus date */
                else if(*rec == '@')
                {
                    if((err = outstr("@D@")) != 0)
                        return(err);
                    ++rec;
                }
                else if((err = pd123__foutc(lotusich((S32) *rec++), pd123__fout)) != 0)
                    return(err);
            }
            if((err = foutcr(pd123__fout)) != 0)
                return(err);
            }
        }
    return(0);
}

/******************************************************************************
*
* write out a date to PD
*
******************************************************************************/

static S32
writedate(
    long dateno)
{
    S32 month, day, lasta, leap;
    long dayno;
    S32 year;

    if(dateno > 73049)
    {
        day = month = year = 99;
    }
    else
    {
        dayno = year = month = day = 0;
        year = -1;
        leap = 0;
        do  {
            ++year;
            leap = (year & 3) ? 0 : 1;
            month = 0;
            do  {
                lasta = days[month];
                if(leap && (month == 1))
                    ++lasta;
                dayno += lasta;
                ++month;
            }
            while((dayno < dateno)  &&  (month < 12));
        }
        while(dayno < dateno);

        day = (S32) (dateno - (dayno - lasta));
        year %= 100;
    }

    if(fprintf(pd123__fout, "%d.%d.%d", day, month, year) < 0)
        return(PD123_ERR_FILE);
    return(0);
}

/******************************************************************************
*
* write out lotus expression to pd
*
******************************************************************************/

static S32
writeexp(
    uchar *ep, S32 col,
    S32 row)
{
    S32 err, oldsp, nobrk;

    /* set scanner index */
    termp = ep;
    argsp = nobrk = 0;

    chksym();
    for(;;)
    {
        switch(curopr->ftype)
        {
        case LO_CONST:
            if((argstk[argsp++] =
                        convconst(cursym, termp + 1, col, row)) == 0)
                return(PD123_ERR_MEM);
            if(argsp == MAXSTACK)
                return(PD123_ERR_EXP);
            break;

        case LO_END:
            break;

        case LO_BRACKETS:
            {
            if(!nobrk)
            {
                uchar *nele, *oele, *c;

                if(!argsp)
                {
                    freestk();
                    return(PD123_ERR_EXP);
                }

                oele = argstk[--argsp];

                /* add two for brackets and null */
                if((nele = malloc(strlen(oele) + 2 + 1)) == NULL)
                    return(PD123_ERR_MEM);

                c = nele;
                *c++ = '(';
                strcpy(c, oele);
                c += strlen(oele);
                *c++ = ')';
                *c++ = CH_NULL;
                free(oele);
                argstk[argsp++] = nele;
            }
            nobrk = 0;
            break;
            }

        case LO_UNARY:
            {
            uchar *nele, *oele, *c;
            S32 addlen;

            if(!argsp)
            {
                freestk();
                return(PD123_ERR_EXP);
            }

            oele = argstk[--argsp];
            addlen = strlen(curopr->pdeqv);
            /* extra for new operator and null */
            if((nele = malloc(strlen(oele) + addlen + 1)) == NULL)
                return(PD123_ERR_MEM);

            c = nele;
            strcpy(c, curopr->pdeqv);
            c += addlen;
            strcpy(c, oele);
            c += strlen(oele);
            *c++ = CH_NULL;
            free(oele);
            argstk[argsp++] = nele;
            if(!curopr->ltpok || (pd123__curpd < curopr->ltpok))
                ++pd123__poorfunc;
            break;
            }

        case LO_BINARY:
            {
            uchar *ele1, *ele2, *nele, *c;
            S32 addlen, lele1, lele2;

            if(argsp < 2)
            {
                freestk();
                return(PD123_ERR_EXP);
            }

            ele2 = argstk[--argsp];
            ele1 = argstk[--argsp];
            lele1 = strlen(ele1);
            lele2 = strlen(ele2);
            addlen = strlen(curopr->pdeqv);
            /* two arguments, operator and null */
            if((nele = malloc(lele1 + lele2 + addlen + 1)) == NULL)
                return(PD123_ERR_MEM);

            c = nele;
            strcpy(c, ele1);
            c += lele1;
            strcpy(c, curopr->pdeqv);
            c += addlen;
            strcpy(c, ele2);
            c += lele2;
            *c++ = CH_NULL;
            free(ele1);
            free(ele2);
            argstk[argsp++] = nele;
            if(!curopr->ltpok || (pd123__curpd < curopr->ltpok))
                ++pd123__poorfunc;
            break;
            }

        case LO_FUNC:
            {
            S32 narg, i, tlen, argc, noname = 0;
            uchar *nele, *c;
            uchar argsep = ',';

            /* work out number of arguments */
            if((narg = curopr->n_args) == -1)
                narg = (S32) *(termp + 1);

            if(argsp < narg)
            {
                freestk();
                return(PD123_ERR_EXP);
            }

            /* call argument fuddler */
            if(curopr->argix)
                if((err = (*argfuddle[curopr->argix - 1])(&narg,
                                                          &argsep,
                                                          &nobrk,
                                                          &noname)) != 0)
                    return(err);

            tlen = 0;
            if(narg)
            {
                /* add up the length of all the arguments */
                for(i = 1; i <= narg; ++i)
                    tlen += strlen(argstk[argsp - i]);
                /* add in space for commas and function brackets */
                tlen += narg - 1;
                if(!nobrk)
                    tlen += 2;
            }

            /* add length of name, null */
            if(!noname)
                tlen += strlen(curopr->pdeqv);
            ++tlen;
            if((nele = malloc(tlen)) == NULL)
                return(PD123_ERR_MEM);

            c = nele;
            if(!noname)
            {
                strcpy(c, curopr->pdeqv);
                c += strlen(curopr->pdeqv);
            }
            argc = narg;
            if(narg)
            {
                if(!nobrk)
                    *c++ = '(';
                while(argc)
                {
                    uchar *carg = argstk[argsp - (argc--)];
                    strcpy(c, carg);
                    c += strlen(carg);
                    if(argc)
                        *c++ = argsep;
                    free(carg);
                }
                if(!nobrk)
                    *c++ = ')';
            }
            *c++ = CH_NULL;
            argsp -= narg;
            __assume(argsp < elemof32(argstk));
            argstk[argsp++] = nele;
            if(!curopr->ltpok || (pd123__curpd < curopr->ltpok))
                ++pd123__poorfunc;
            nobrk = 0;
            break;
            }
        }

        if(cursym == LF_END)
            break;
        scnsym();
    }

    err = outstr(argstk[0]);
    oldsp = argsp;
    freestk();
    return(err ? err : (oldsp == 1) ? 0 : PD123_ERR_EXP);
}

/******************************************************************************
*
* write out format details to PD
*
******************************************************************************/

static S32
writeformat(
    uchar *fmtp,
    P_S32 specflg)
{
    S32 err, decplc;

    /* numbers are always right aligned */
    if((err = outcon("R")) != 0)
        return(err);

    decplc = *fmtp & L_DECPLC;
    switch(*fmtp & L_FMTTYPE)
    {
    case L_CURCY:
        if((err = outcon("LC")) != 0)
            return(err);
        if((err = outcon("B")) != 0)
            return(err);
        if((err = outdecplc(decplc)) != 0)
            return(err);
        break;
    case L_PERCT:
        if((err = outcon("TC")) != 0)
            return(err);
        if((err = outdecplc(decplc)) != 0)
            return(err);
        break;
    default:
    case L_COMMA:
        if((err = outcon("B")) != 0)
            return(err);
        if((err = outdecplc(decplc)) != 0)
            return(err);
        break;
    case L_FIXED:
    case L_SCIFI:
        if((err = outdecplc(decplc)) != 0)
            return(err);
        break;
    case L_SPECL:
        switch(decplc)
        {
        /* general format */
        case L_GENFMT:
            if((err = outcon("DF")) != 0)
                return(err);
            break;
        /* dates */
        case L_DDMMYY:
        case L_DDMM:
        case L_MMYY:
        case L_DATETIME:
        case L_DATETIMES:
        case L_DATEINT1:
        case L_DATEINT2:
            *specflg = P_DATE;
            break;
        /* text */
        case L_TEXT:
            *specflg = P_TEXT;
            break;
        default:
            break;
        }
        break;
    }

    return(0);
}

/******************************************************************************
*
* read options page equivalents from LOTUS
* and write to output
*
******************************************************************************/

static S32
writeoptions(void)
{
    U32 count;
    S32 err;

    /* loop for each option */
    for(count = 0; count < elemof32(optqv); ++count)
        if((err = (*optqv[count].wropt)(&optqv[count])) != 0)
            return(err);

    return(0);
}

/******************************************************************************
*
* write out a column to PD
*
******************************************************************************/

static S32
writepcol(
    S32 col,
    S32 sro,
    S32 ero)
{
    S32 err, cw, row;
    uchar strcol[5];
    uchar *rec;

    /* write out construct */
    if((err = outstr("%CO:")) != 0)
        return(err);

    strcol[lotus_xtos(strcol, col)] = CH_NULL;
    if((err = outstr(strcol)) != 0)
        return(err);

    /* reset pointer to start */
    curpos = NULL;
    if((rec = findrec(L_COLW1, WIDTH_MATCH, col, 0)) != NULL)
    {
        rec += 2;
        cw = (S32) *rec;
    }
    else
    {
        cw = defcwid;
    }

    /* check hidden vector for an entry */
    if(pd123__hidvec[col >> 3] & (1 << (col & 7)))
        cw = 0;

    if(fprintf(pd123__fout, ",%d,72%%", cw) < 0)
        return(PD123_ERR_FILE);

    /* output all the rows */
    row = sro - 1;
    while(row <= ero)
    {
        U16 opcode;
        S32 count, oldrow;
        uchar *fmtp;

        if((rec = findrec(0, NEXT_ROW, col, row)) == NULL)
            break;

        /* read opcode */
        fmtp = rec;
        rec -= 4;
        opcode = lts_readuword16(rec);
        rec += 2;

        /* read row number */
        rec += 5;
        oldrow = row;
        row = (S32) lts_readuword16(rec);
        rec += 2;
        count = row - oldrow - 1;

        /* output blank rows to pd file */
        while(count--)
            if((err = foutcr(pd123__fout)) != 0)
                return(err);

        /* deal with different cell types */
        switch(opcode)
        {
        case L_INTEGER:
            {
            S32 specflg = 0;
            S32 intval;

            if((err = writeformat(fmtp, &specflg)) != 0)
                return(err);

            intval = (S32) lts_readword16(rec);
            rec += 2;

            if((err = checkdate(&specflg, (long) intval)) != 0)
                return(err);

            if(specflg)
                if(fprintf(pd123__fout, "%d", intval) < 0)
                    return(PD123_ERR_FILE);
            break;
            }

        case L_NUMBER:
            {
            S32 specflg = 0;
            F64 fpval;

            if((err = writeformat(fmtp, &specflg)) != 0)
                return(err);

            fpval = lts_readdouble(rec);

            if(fpval < LONG_MAX)
                if((err = checkdate(&specflg, (long) (fpval + .5))) != 0)
                    return(err);

            if(specflg)
                if(fprintf(pd123__fout, "%17g", fpval) < 0)
                    return(PD123_ERR_FILE);
            break;
            }

        case L_LABEL:
            {
            S32 rep = FALSE, width = cw;
            uchar *startlab;

            /* deal with label alignment byte */
            switch(*rec++)
            {
            case '\'':
                break;
            case '"':
                if((err = outcon("R")) != 0)
                    return(err);
                break;
            case '^':
                if((err = outcon("C")) != 0)
                    return(err);
                break;
            case '\\':
                rep = TRUE;
                break;
            }

            startlab = rec;
            do
            {
                while(*rec)
                {
                    if((err = pd123__foutc(lotusich((S32) *rec++), pd123__fout)) != 0)
                        return(err);
                    --width;
                }
                rec = startlab;
            }
            while(rep && (width > 0));

            break;
            }

        case L_FORMULA:
            {
            S32 specflg = 0;

            if((err = writeformat(fmtp, &specflg)) != 0)
                return(err);

            if((err = outcon("V")) != 0)
                return(err);

            if((err = writeexp(rec + 10, col, row)) != 0)
            {
                if(err != PD123_ERR_EXP)
                    return(err);
                else
                    pd123__errexp += 1;
            }

            break;
            }
        }

        if((err = foutcr(pd123__fout)) != 0)
            return(err);
    }
    return(0);
}

/******************************************************************************
*
* write out a margin value
*
******************************************************************************/

static S32
wrmar(
    optp op,
    U16 value)
{
    S32 err;

    if(value != op->deflt)
    {
        if((err = startopt(op)) != 0)
            return(err);
        if(fprintf(pd123__fout, "%d", value) < 0)
            return(PD123_ERR_FILE);
        if((err = foutcr(pd123__fout)) != 0)
            return(err);
    }
    return(0);
}

/******************************************************************************
*
* write out margin settings
*
******************************************************************************/

static S32
wrmargins(
    optp op)
{
    uchar *rec;
    U16 value;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += op->offset;
        value = *rec++;
        return(wrmar(op, value));
    }
    return(0);
}

/******************************************************************************
*
* write out minus and brackets
*
******************************************************************************/

static S32
wrminbrk(
    optp op)
{
    uchar *rec;
    S32 err;
    S32 minbrk;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += op->offset;
        minbrk = (*rec & L_FMTTYPE) == L_CURCY ? 1 : 0;
        if(minbrk)
        {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = pd123__foutc('B', pd123__fout)) != 0)
                return(err);
            if((err = foutcr(pd123__fout)) != 0)
                return(err);
        }
    }
    return(0);
}

/******************************************************************************
*
* write out set margin values
*
******************************************************************************/

static S32
wrsetmar(
    optp op)
{
    return(wrmar(op, 2));
}

/******************************************************************************
*
* write out text/numbers flag
*
******************************************************************************/

static S32
wrtextn(
    optp op)
{
    S32 err;

    if(pd123__curpd >= PD_PC)
    {
        if((err = startopt(op)) != 0)
            return(err);
        if((err = pd123__foutc((S32) op->deflt, pd123__fout)) != 0)
            return(err);
        return(foutcr(pd123__fout));
    }

    return(0);
}

/******************************************************************************
*
* write out thousands
*
******************************************************************************/

static S32
wrthousands(
    optp op)
{
    uchar *rec, thous;
    S32 err;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
    {
        rec += op->offset;
        thous = *rec & L_FMTTYPE;
        if(thous == L_COMMA || thous == L_CURCY)
        {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = pd123__foutc('1', pd123__fout)) != 0)
                return(err);
            if((err = foutcr(pd123__fout)) != 0)
                return(err);
        }
    }
    return(0);
}

/******************************************************************************
*
* write out wrap mode
*
******************************************************************************/

static S32
wrwrapmo(
    optp op)
{
    S32 err;

    if((err = startopt(op)) != 0)
        return(err);
    if((err = pd123__foutc((S32) op->deflt, pd123__fout)) != 0)
        return(err);
    return(foutcr(pd123__fout));
}

/******************************************************************************
*
* convert column to a string
*
* --out--
* length of resulting string
*
******************************************************************************/

static S32
lotus_xtos(
    uchar *string,
    S32 x)
{
    uchar *c = string;
    register S32 digit2;
    register S32 digit1;

    digit2 = x / 26;
    digit1 = x - digit2 * 26;

    if(digit2)
        *c++ = (uchar) ((digit2 - 1) + (S32) 'A');
    *c++ = (uchar) (digit1 + (S32) 'A');
    return(c - string);
}

/* end of pd123.c */
