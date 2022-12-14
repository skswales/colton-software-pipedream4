/* 123pd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Program to convert a PipeDream file into a Lotus 1-2-3 (rel 2) .WK1 file */

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

/*
function declarations
*/

static S32
compileexp(
    uchar *,
    S32,
    S32);

static void
lexpr(void);

static void
alterm(void);

static void
blterm(void);

static void
clterm(void);

static void
dlterm(void);

static void
elterm(void);

static void
flterm(void);

static void
glterm(void);

static void
lterm(void);

static S32
procfunc(oprp);

static void
mochoose(
    P_S32,
    S32);

static void
moindex(
    P_S32,
    S32);

static void
element(void);

static S32
dolotus(void);

static void
ebout(
    uchar byt);

static void
edout(F64);

static void
euwout(
    U16 wrd);

static void
ewout(
    S16 wrd);

static S32
findcolumns(void);

static S16
makeabr(
    U16 ref,
    S32 csl);

static S32
nxtsym(void);

static uchar *
procdcp(
    uchar *,
    uchar *,
    P_S32);

static uchar *
procpct(
    uchar *,
    uchar *,
    P_S32);

static uchar *
searchcon(
    uchar *startp,
    uchar *endp,
    const char *conid);

static uchar *
searchopt(
    const char *);

static S32
reccon(
    uchar *,
    P_F64,
    P_S32,
    P_S32,
    P_S32,
    P_S32);

#ifdef UTIL_PTL
static S32
showrow(S32);
#endif

extern S32
pd123__scnslr(
    uchar *,
    U16 *,
    U16 *);

static S32
lotus_stox(
    uchar *,
    P_S32);

static S32
wrlcalcmode(void);

static S32
wrlcalcord(void);

static S32
wrcols(void);

static S32
wrcolws(void);

static S32
wrfooter(void);

static S32
wrgraph(void);

static S32
wrheader(void);

static S32
wrlheadfoot(
    uchar *);

static S32
wrhidvec(void);

static S32
writecolrow(
    S32,
    S32);

static S32
writeins(lfip);

static S32
writelabel(
    uchar *,
    S32,
    S32,
    S32);

static S32
writeldate(
    S32,
    S32,
    S32,
    S32,
    S32);

static S32
writelformat(
    S32,
    S32);

extern S32
writelotus(
    FILE *filein,
    FILE *fileout,
    S32 PD_type,
    S32 (* counterproc)(S32));

static S32
writeslot(
    uchar *,
    S32,
    S32,
    U16,
    S32);

static S32
writesslot(
    S32,
    S32,
    S32,
    S32,
    U16,
    S32);

static S32
lts_writedouble(F64);

static S32
lts_writeuword(U16);

static S32
lts_writeword(S16);

static S32
wrlmar(
    const char *);

static S32
wrlmargins(void);

static S32
wrwindow1(void);

static int
u_strnicmp(
    PC_U8 a,
    PC_U8 b,
    size_t n);

/******************************************************************************
*
* lotus file structure
*
******************************************************************************/

#define NOPT 0xFF

static const LFINS lfstruct[] =
{
    { L_BOF,            2, "\x6\x4",          2,    NOPT,       NULL },
    { L_RANGE,          8, "",                0,    NOPT,       NULL },
    { L_CPI,            6, "",                0,    NOPT,       NULL },
    { L_CALCCOUNT,      1, "\x1",             1,    NOPT,       NULL },
    { L_CALCMODE,       1, "",                0,    NOPT,wrlcalcmode },
    { L_CALCORDER,      1, "",                0,    NOPT, wrlcalcord },
    { L_SPLIT,          1, "",                0,    NOPT,       NULL },
    { L_SYNC,           1, "",                0,    NOPT,       NULL },
    { L_WINDOW1,       32, "",                0,    NOPT,  wrwindow1 },
    { L_COLW1,          3, "",                0,    NOPT,    wrcolws },
    { L_HIDVEC1,       32, "",                0,    NOPT,   wrhidvec },
    { L_CURSORW12,      1, "",                0,    NOPT,       NULL },
    { L_TABLE,         25, "\xFF\xFF\x0\x0",  4,       1,       NULL },
    { L_QRANGE,        25, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_PRANGE,         8, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_UNFORMATTED,    1, "",                0,    NOPT,       NULL },
    { L_FRANGE,         8, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_SRANGE,         8, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_KRANGE,         9, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_KRANGE2,        9, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_RRANGES,       25, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_MATRIXRANGES,  40, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_HRANGE,        16, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_PARSERANGES,   16, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_PROTEC,         1, "",                0,    NOPT,       NULL },
    { L_FOOTER,       242, "",                0,    NOPT,   wrfooter },
    { L_HEADER,       242, "",                0,    NOPT,   wrheader },
    { L_SETUP,         40, "",                0,    NOPT,       NULL },
    { L_MARGINS,       10, "",                0,    NOPT, wrlmargins },
    { L_LABELFMT,       1, "\x27",            1,    NOPT,       NULL },
    { L_TITLES,        16, "\xFF\xFF\x0\x0",  4,       0,       NULL },
    { L_GRAPH,        439, "",                4,       0,    wrgraph },
    { L_FORMULA,        0, "",                0,    NOPT,     wrcols },
    { L_EOF,            0, "",                0,    NOPT,       NULL }
};

/*
table of argument modifier functions
*/

static void (* argmod[])(P_S32 , S32) =
{
    mochoose,
    NULL,
    moindex,
    NULL,
};

/*
table of valid constructs for cells
*/

#define BIT_BRK 0x0001
#define BIT_CEN 0x0002
#define BIT_DCN 0x0004
#define BIT_DCF 0x0008
#define BIT_LFT 0x0010
#define BIT_CUR 0x0020
#define BIT_RYT 0x0040
#define BIT_EXP 0x0080

typedef const struct CONSTR *conp;

static const struct CONSTR
{
    const char * conid;
    U16 mask;
    uchar * (* proccons)(uchar *, uchar *, P_S32 );
}
constab[] =
{
    { "V",    BIT_EXP,    NULL },
    { "DF",   BIT_DCF,    NULL },
    { "R",    BIT_RYT,    NULL },
    { "C",    BIT_CEN,    NULL },
    { "B",    BIT_BRK,    NULL },
    { "D",    BIT_DCN,    procdcp },
    { "LCR",        0,    NULL },
    { "LC",   BIT_CUR,    NULL },
    { "L",    BIT_LFT,    NULL },
    { "TC",         0,    NULL },
    { "PC",         0,    procpct },
    { "F",          0,    NULL },
    { "H",          0,    NULL },
    { "JL",         0,    NULL },
    { "JR",         0,    NULL },
    { "P",          0,    NULL }
};

/* pd constants */
#define PD_REAL 1
#define PD_DATE 2
#define PD_INTEGER 3

/* arrays of column information */
static uchar *colend[LOTUS_MAXCOL];
static uchar *colcur[LOTUS_MAXCOL];
static S32 colwid[LOTUS_MAXCOL];
uchar pd123__hidvec[32];

/* global default decimal places */
static S32 decplc = 2;

/* end of options page */
static uchar *optend;

/* PipeDream file details */
static uchar *pdf;
static uchar *pdend;
static size_t pdsize;

/* expression output buffer */
static uchar expbuf[512];
static uchar *expop;
static uchar *exppos;
static S32 ecol;
static S32 erow;
struct SYMBOL pd123__csym;

/* symbol checker macro */
#define chknxs() ((pd123__csym.symno != SYM_BLANK) ? pd123__csym.symno : nxtsym())

/* current lotus instruction */
static lfip curlfi = NULL;

/* counter routine */
static S32 (* counter)(S32);

/******************************************************************************
*
* main loop for PipeDream to Lotus 1-2-3
* this main procedure is compiled if the constant
* UTIL_PTL is defined:
*
* UTIL_LTP creates stand-alone utility for Lotus 1-2-3 to PipeDream
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus 1-2-3
* INT_LPTPL creates internal code for converter using function lp_lptpl
*
******************************************************************************/

#ifdef UTIL_PTL

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
#if CROSS_COMPILE
util_ptl_main(
    int argc,
    char **argv)
#else
main(
    int argc,
    char **argv)
#endif
{
    S32 err;
    const char * infile;
    const char * outfile;

    /* banner */
    puts("PipeDream to Lotus 1-2-3 converter: (C) Colton Software 1988-2022");

    /* argument checking */
    if(argc < 3)
    {
        fprintf(stderr, "Syntax: %s <infile> <outfile>\n", argv[0]);
        return(EXIT_FAILURE);
    }

    infile = *++argv;
    outfile = *++argv;

    if((pd123__fin = fopen(infile, "rb")) == NULL)
    {
        fprintf(stderr, "Can't open %s for reading\n", infile);
        exit(EXIT_FAILURE);
    }

    if(0 == (pdsize = read_file_size(pd123__fin)))
    {
        fprintf(stderr, "Unable to read length of %s\n", infile);
        return(EXIT_FAILURE);
    }

    if((pdf = malloc(pdsize)) == NULL)
    {
        fprintf(stderr, "Not enough memory to load %s\n", infile);
        return(EXIT_FAILURE);
    }

    /* read in PipeDream file */
    fread(pdf, 1, pdsize, pd123__fin);
    pdend = pdf + pdsize;
    fclose(pd123__fin);

    if((pd123__fout = fopen(outfile, "wb")) == NULL)
    {
        fprintf(stderr, "Can't open %s for writing\n", outfile);
        exit(EXIT_FAILURE);
    }

    /* set pd level */
    pd123__curpd = PD_4; //PD_Z88;
    counter = showrow;

    /* write out Lotus file */
    err = dolotus();
    printf("\n");

    switch(err)
    {
    case PD123_ERR_MEM:
        fprintf(stderr, "Out of memory\n");
        break;
    case PD123_ERR_FILE:
        perror("File error");
        break;
    case PD123_ERR_BADFILE:
        fprintf(stderr, "Bad PipeDream file\n");
        break;
    case PD123_ERR_BIGFILE:
        fprintf(stderr, "Too many rows or columns for Lotus\n");
        break;
    default:
        break;
    }

    fclose(pd123__fout);
    free(pdf);

    if(err)
    {
        remove(outfile);
        puts("Conversion failed");
        return(EXIT_FAILURE);
    }

    if(pd123__errexp)
        fprintf(stderr, "%d bad expressions found\n", pd123__errexp);

    {
    _kernel_osfile_block fileblk;
    fileblk.load /*r2*/ = 0xDB0 /*FILETYPE_LOTUS123*/;
    (void) _kernel_osfile(18 /*SetType*/, outfile, &fileblk);
    } /*block*/

    puts("Conversion complete");
    return(EXIT_SUCCESS);
}
#endif

/******************************************************************************
*
* function to write a lotus file called from elsewhere
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
writelotus(
    FILE *inf,
    FILE *outf,
    S32 level,
    S32 (* counterproc)(S32))
{
    S32 err;
    #ifdef MS_HUGE
    uchar *op;
    #endif

    /* save file pointers */
    pd123__fin = inf;
    pd123__fout = outf;

    /* set level of conversion */
    pd123__curpd = level;

    /* set counter routine */
    counter = counterproc;

    if(fseek(pd123__fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    pdsize = (size_t) ftell(pd123__fin);
    if(fseek(pd123__fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);

    if((pdf = malloc(pdsize)) == NULL)
        return(PD123_ERR_MEM);

    /* read in PipeDream file */
    if(fread(pdf, 1, pdsize, pd123__fin) != pdsize)
        return(PD123_ERR_FILE);
    pdend = pdf + pdsize;

    /* write out lotus file */
    err = dolotus();

    free(pdf);

    if(!err && (pd123__errexp || pd123__poorfunc))
        err = PD123_ERR_EXP;

    return(err);
}
#endif

/******************************************************************************
*
* compile PipeDream expression into RPN for Lotus
*
******************************************************************************/

static S32
compileexp(
    uchar *slot,
    S32 col,
    S32 row)
{
    pd123__csym.symno = SYM_BLANK;
    pd123__csym.scandate = 0;
    expop = expbuf;
    exppos = slot;
    ecol = col;
    erow = row;

    lexpr();
    if(pd123__csym.symno != LF_END)
        return(0);
    ebout(LF_END);
    return(expop - expbuf);
}

/******************************************************************************
*
* expression recogniser works by recursive descent
* recognise |
*
******************************************************************************/

static void
lexpr(void)
{
    alterm();
    while(chknxs() == LF_OR)
    {
        pd123__csym.symno = SYM_BLANK;
        alterm();
        ebout(LF_OR);
    }
    return;
}

/******************************************************************************
*
* recognise &
*
******************************************************************************/

static void
alterm(void)
{
    blterm();
    while(chknxs() == LF_AND)
    {
        pd123__csym.symno = SYM_BLANK;
        blterm();
        ebout(LF_AND);
    }
    return;
}

/******************************************************************************
*
* recognise =, <>, <, >, <=, >=
*
******************************************************************************/

static void
blterm(void)
{
    S32 nxsym;

    clterm();
    for(;;)
    {
        switch(nxsym = chknxs())
        {
        case LF_EQUALS:
        case LF_NOTEQUAL:
        case LF_LT:
        case LF_GT:
        case LF_LTEQUAL:
        case LF_GTEQUAL:
            pd123__csym.symno = SYM_BLANK;
            break;
        default:
            return;
        }
        clterm();
        ebout((uchar) nxsym);
    }
}

/******************************************************************************
*
* recognise +, -
*
******************************************************************************/

static void
clterm(void)
{
    S32 nxsym;

    dlterm();
    for(;;)
    {
        switch(nxsym = chknxs())
        {
        case LF_PLUS:
        case LF_MINUS:
            pd123__csym.symno = SYM_BLANK;
            break;
        default:
            return;
        }
        dlterm();
        ebout((uchar) nxsym);
    }
}

/******************************************************************************
*
* recognise *, /
*
******************************************************************************/

static void
dlterm(void)
{
    S32 nxsym;

    elterm();
    for(;;)
    {
    switch(nxsym = chknxs())
        {
        case LF_TIMES:
        case LF_DIVIDE:
            pd123__csym.symno = SYM_BLANK;
            break;
        default:
            return;
        }
        elterm();
        ebout((uchar) nxsym);
    }
}

/******************************************************************************
*
* recognise ^
*
******************************************************************************/

static void
elterm(void)
{
    flterm();
    while(chknxs() == LF_POWER)
    {
        pd123__csym.symno = SYM_BLANK;
        flterm();
        ebout(LF_POWER);
    }
    return;
}

/******************************************************************************
*
* recognise unary +, -, !
*
******************************************************************************/

static void
flterm(void)
{
    switch(chknxs())
    {
    case LF_PLUS:
        pd123__csym.symno = SYM_BLANK;
        flterm();
        ebout(LF_UPLUS);
        return;
    case LF_MINUS:
        pd123__csym.symno = SYM_BLANK;
        flterm();
        ebout(LF_UMINUS);
        return;
    case LF_NOT:
        pd123__csym.symno = SYM_BLANK;
        flterm();
        ebout(LF_NOT);
        return;
    default:
        glterm();
        return;
    }
}

/******************************************************************************
*
* recognise lterm or brackets
*
******************************************************************************/

static void
glterm(void)
{
    if(chknxs() == SYM_OBRACKET)
    {
        pd123__csym.symno = SYM_BLANK;
        lexpr();
        if(chknxs() != SYM_CBRACKET)
        {
            pd123__csym.symno = SYM_BAD;
            return;
        }
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_BRACKETS);
    }
    else
    {
        lterm();
    }
}

/******************************************************************************
*
* recognise constants and functions
*
******************************************************************************/

static void
lterm(void)
{
    S32 nxsym;
    uchar *c;

    switch(nxsym = chknxs())
    {
    case LF_CONST:
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_CONST);
        edout(pd123__csym.fpval);
        return;

    case LF_SLR:
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_SLR);
        ewout(makeabr(pd123__csym.stcol, ecol));
        ewout(makeabr(pd123__csym.strow, erow));
        return;

    case LF_INTEGER:
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_INTEGER);
        ewout((S16) pd123__csym.fpval);
        return;

    case LF_STRING:
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_STRING);
        c = pd123__csym.stringp;
        while(*c)
            ebout((uchar) pd123__ichlotus((S32) *c++));
        ebout(CH_NULL);
        return;

    case SYM_FUNC:
        pd123__csym.symno = SYM_BLANK;
        switch(pd123__opreqv[pd123__csym.ixf].n_args)
        {
        /* zero argument functions */
        case 0:
            ebout(pd123__opreqv[pd123__csym.ixf].fno);
            return;

        /* variable argument functions */
        case -1:
            {
            S32 narg, fno;
            oprp funp = &pd123__opreqv[pd123__csym.ixf];

            fno = funp->fno;
            narg = procfunc(funp);
            ebout((uchar) fno);
            ebout((uchar) narg);
            return;
            }

        /* fixed argument functions */
        default:
            {
            S32 fno;
            oprp funp = &pd123__opreqv[pd123__csym.ixf];

            fno = funp->fno;
            procfunc(funp);
            ebout((uchar) fno);
            return;
            }
        }
    }
}

/******************************************************************************
*
* process a function with arguments
*
******************************************************************************/

static S32
procfunc(
    oprp funp)
{
    S32 narg = 0;
    void (* amodp)(P_S32 , S32) = NULL;

    if(funp->argix)
        amodp = argmod[funp->argix - 1];

    if(chknxs() != SYM_OBRACKET)
    {
        pd123__csym.symno = SYM_BAD;
        return(0);
    }

    do
    {
        /* call argument modifier */
        if(amodp)
            (*amodp)(&narg, 0);
        pd123__csym.symno = SYM_BLANK;
        element();
        if(amodp)
            (*amodp)(&narg, 1);
        ++narg;
    }
    while(chknxs() == SYM_COMMA);

    if(chknxs() != SYM_CBRACKET)
    {
        pd123__csym.symno = SYM_BAD;
        return(narg);
    }

    if((funp->n_args >= 0) && (funp->n_args != (uchar) narg))
        return(pd123__csym.symno = SYM_BAD);

    pd123__csym.symno = SYM_BLANK;
    return(narg);
}

/******************************************************************************
*
* modify choose arguments
*
******************************************************************************/

static void
mochoose(
    P_S32 narg,
    S32 prepost)
{
    /* get to after first argument */
    if((*narg != 0) || !prepost)
        return;

    /* add minus 1 to argument */
    ebout(LF_INTEGER);
    euwout(1);
    ebout(LF_MINUS);
}

/******************************************************************************
*
* modify index arguments
*
******************************************************************************/

static void
moindex(
    P_S32 narg,
    S32 prepost)
{
    switch(*narg)
    {
    case 0:
        if(!prepost)
        {
            ++(*narg);
            ebout(LF_RANGE);
            ewout(makeabr(0, ecol));
            ewout(makeabr(0, erow));
            ewout(makeabr(LOTUS_MAXCOL - 1, ecol));
            ewout(makeabr(LOTUS_MAXROW - 1, erow));
        }
        break;

    case 1:
    case 2:
        if(prepost)
        {
            /* add minus 1 to argument */
            ebout(LF_INTEGER);
            euwout(1);
            ebout(LF_MINUS);
        }
        break;

    default:
        break;
    }
}

/******************************************************************************
*
* recognise an element of a list of function arguments
*
******************************************************************************/

static void
element(void)
{
    if(chknxs() == LF_RANGE)
    {
        pd123__csym.symno = SYM_BLANK;
        ebout(LF_RANGE);
        ewout(makeabr(pd123__csym.stcol, ecol));
        ewout(makeabr(pd123__csym.strow, erow));
        ewout(makeabr(pd123__csym.encol, ecol));
        ewout(makeabr(pd123__csym.enrow, erow));
        return;
    }

    lexpr();
}

/******************************************************************************
*
* use the master lotus structure table to
* write out the lotus file
*
******************************************************************************/

static S32
dolotus(void)
{
    S32 err;
    U32 i;

    if(!findcolumns())
        return(PD123_ERR_BADFILE);

    if(pd123__maxcol >= LOTUS_MAXCOL)
        return(PD123_ERR_BIGFILE);

    for(i = 0; i < elemof32(lfstruct); ++i)
    {
        curlfi = &lfstruct[i];

        if(curlfi->writefunc)
        {
            /* call special function for this instruction */
            if((err = (*curlfi->writefunc)()) != 0)
                return(err);
        }
        else
        {
            /* use table to write out default data */
            if((err = writeins(curlfi)) != 0)
                return(err);
            if(curlfi->length)
            {
                S32 len = (S32) curlfi->length;
                S32 dlen = (S32) curlfi->dlen;
                const uchar *dp = curlfi->sdata;

                /* check for a pattern to be output */
                if(curlfi->pattern == NOPT)
                {
                    /* output data */
                    while(dlen--)
                    {
                        if((err = pd123__foutc((S32) *dp++, pd123__fout)) != 0)
                            return(err);
                        --len;
                    }
                }
                else
                {
                    S32 tlen = curlfi->pattern;

                    /* output leading nulls before pattern */
                    while(tlen--)
                    {
                        if((err = pd123__foutc(0, pd123__fout)) != 0)
                            return(err);
                        --len;
                    }

                    /* output as many patterns as possible */
                    while(dlen <= len)
                    {
                        tlen = dlen;
                        while(tlen--)
                        {
                            if((err = pd123__foutc((S32) *dp++, pd123__fout)) != 0)
                                return(err);
                            --len;
                        }
                        dp = curlfi->sdata;
                    }
                }

                /* pad with trailing nulls */
                while(len--)
                    if((err = pd123__foutc(0, pd123__fout)) != 0)
                        return(err);
            }
        }
    }
    return(0);
}

/******************************************************************************
*
* output byte to compiled expression
*
******************************************************************************/

static void
ebout(
    uchar byt)
{
    *expop++ = byt;
}

/******************************************************************************
*
* output double to compiled expression
*
******************************************************************************/

static void
edout(
    F64 fpval)
{
#if RISCOS

    /* this for the ARM <-> 8087 */
    union EDOUT_U
    {
        F64 fpval;
        uchar fpbytes[sizeof(F64)];
    } fp;
    U32 i;

    fp.fpval = fpval;

    for(i = 4; i < sizeof(F64); ++i)
        *expop++ = fp.fpbytes[i];

    for(i = 0; i < 4; ++i)
        *expop++ = fp.fpbytes[i];

#elif WINDOWS

    *(((P_F64) expop)++) = fpval;

#endif /* OS */
}

/******************************************************************************
*
* output unsigned word to compiled expression
*
******************************************************************************/

static void
euwout(
    U16 wrd)
{
#if RISCOS

    /* this for the ARM */
    union EUWOUT_U
    {
        U16 uword;
        uchar uwbytes[sizeof(U16)];
    } uw;

    uw.uword = wrd;
    *expop++ = uw.uwbytes[0];
    *expop++ = uw.uwbytes[1];

#elif WINDOWS

    *(((U16 *) expop)++) = wrd;

#endif /* OS */
}

/******************************************************************************
*
* output signed word to compiled expression
*
******************************************************************************/

static void
ewout(
    S16 wrd)
{
#if RISCOS

    /* this for the ARM */
    union EWOUT_U
    {
        S16 word;
        uchar wbytes[sizeof(S16)];
    } w;

    w.word = wrd;
    *expop++ = w.wbytes[0];
    *expop++ = w.wbytes[1];

#elif WINDOWS

    *(((S16 *) expop)++) = wrd;

#endif /* OS */
}

/******************************************************************************
*
* search loaded PipeDream file for column constructs
* and note their positions in the array
*
******************************************************************************/

static S32
findcolumns(void)
{
    uchar *fp = pdf;
    S32 i, lastcol = -1;

    pd123__maxcol = 0;
    for(i = 0; i < LOTUS_MAXCOL; ++i)
    {
        colcur[i] = NULL;
        colwid[i] = 0;
    }

    while((fp = searchcon(fp, pdend, "CO:")) != 0)
    {
        S32 col, cw, ww, cr1, cr2;
        uchar *tp = fp, *fpend = fp - 4;
        uchar tstr[12];

        i = 0;
        while(i < 12)
            tstr[i++] = *tp++;

        cr1 = lotus_stox(tstr, &col);

        i = sscanf(tstr + cr1, ",%d,%d%n", &cw, &ww, &cr2);
        if((i == 2) && (*(fp + cr1 + cr2) == '%'))
        {
            fp += cr1 + cr2 + 1;
            pd123__maxcol = col + 1;
            colcur[col] = fp;
            colwid[col] = cw;
            if(lastcol != -1)
                colend[lastcol] = fpend;
            lastcol = col;
        }
    }

    if(lastcol != -1)
        colend[lastcol] = pdend;
    return(pd123__maxcol);
}

/******************************************************************************
*
* make a lotus form absolute/relative cell reference
*
******************************************************************************/

static S16
makeabr(
    U16 ref,
    S32 csl)
{
    return(ref & 0x8000 ? (S16) (ref & 0x3FFF)
                        : ((S16) (((S32) ref - csl)) & 0x3FFF)
                        | 0x8000);
}

/******************************************************************************
*
* scan a symbol from the expression
*
******************************************************************************/

static S32
nxtsym(void)
 {
    uchar cc = *exppos;
    uchar fstr[25], *fs;
    S32 cr, co = 0, ts;

    /* special date scanning */
    switch(pd123__csym.scandate--)
    {
    case 7:
        return(pd123__csym.symno = SYM_OBRACKET);
    case 6:
        pd123__csym.fpval = (F64) pd123__csym.yr;
        return(pd123__csym.symno = LF_INTEGER);
    case 5:
        return(pd123__csym.symno = SYM_COMMA);
    case 4:
        pd123__csym.fpval = (F64) pd123__csym.mon;
        return(pd123__csym.symno = LF_INTEGER);
    case 3:
        return(pd123__csym.symno = SYM_COMMA);
    case 2:
        pd123__csym.fpval = (F64) pd123__csym.day;
        return(pd123__csym.symno = LF_INTEGER);
    case 1:
        return(pd123__csym.symno = SYM_CBRACKET);

    default:
        break;
    }

    /* check for end of expression */
    if(!cc)
        return(pd123__csym.symno = LF_END);

    /* check for constant */
    if(isdigit(cc) || (cc == '.'))
    {
        if((ts = reccon(exppos, &pd123__csym.fpval, &pd123__csym.day,
                                             &pd123__csym.mon,
                                             &pd123__csym.yr, &cr)) != 0)
        {
            exppos += cr;
            switch(ts)
            {
            case PD_DATE:
                pd123__csym.scandate = 7;
                pd123__flookup("datef");
                pd123__csym.symno = SYM_FUNC;
                break;
            case PD_INTEGER:
                pd123__csym.symno = LF_INTEGER;
                break;
            case PD_REAL:
                pd123__csym.symno = LF_CONST;
                break;
            }
            return(pd123__csym.symno);
        }
    }

    /* check for cell reference/range */
    if(isalpha(cc) || (cc == '$'))
    {
        if((cr = pd123__scnslr(exppos, &pd123__csym.stcol, &pd123__csym.strow)) != 0)
        {
            exppos += cr;
            /* check for another SLR to make range */
            if((cr = pd123__scnslr(exppos, &pd123__csym.encol, &pd123__csym.enrow)) != 0)
            {
                exppos += cr;
                return(pd123__csym.symno = LF_RANGE);
            }

            return(pd123__csym.symno = LF_SLR);
        }
    }

    /* check for function */
    if(isalpha(cc))
    {
        fs = fstr;
        do
        {
            *fs++ = tolower(cc);
            ++co;
            cc = *(++exppos);
        }
        while((isalpha(cc) || isdigit(cc)) && (co < 24));

        *fs = CH_NULL;
        pd123__flookup(fstr);
        if(pd123__csym.symno != SYM_BAD)
            return(pd123__csym.symno = SYM_FUNC);
        return(SYM_BAD);
    }

    /* check for string */
    if((cc == '"') || (cc == '\''))
    {
        fs = exppos + 1;
        while(*fs && (*fs != cc))
            ++fs;
        if(*fs != cc)
            return(SYM_BAD);
        *fs = CH_NULL;
        pd123__csym.stringp = exppos + 1;
        exppos = fs + 1;
        return(pd123__csym.symno = LF_STRING);
    }

    /* check for special operators */
    switch(cc)
    {
    case '(':
        ++exppos;
        return(pd123__csym.symno = SYM_OBRACKET);
    case ')':
        ++exppos;
        return(pd123__csym.symno = SYM_CBRACKET);
    case ',':
        ++exppos;
        return(pd123__csym.symno = SYM_COMMA);
    default:
        break;
    }

    /* check for operator */
    exppos += pd123__olookup(exppos);
    return(pd123__csym.symno);
}

/******************************************************************************
*
* process decimal places construct
*
******************************************************************************/

static uchar *
procdcp(
    uchar *constr,
    uchar *body,
    P_S32 dcp)
{
    consume_int(sscanf(body, "%d", dcp));
    return(constr);
}

/******************************************************************************
*
* process percent construct
*
******************************************************************************/

static uchar *
procpct(
    uchar *constr,
    uchar *body,
    P_S32 dcp)
{
    UNREFERENCED_PARAMETER(body);
    UNREFERENCED_PARAMETER(dcp);
    /* return pointer one past 1st percent
    to leave a percent character behind */
    return(constr + 1);
}

/******************************************************************************
*
* search PipeDream file for construct
*
******************************************************************************/

static uchar *
searchcon(
    uchar *startp,
    uchar *endp,
    const char *conid)
{
    const S32 conlen = strlen(conid);
    S32 tlen;
    const char *tp;
    uchar *ip;

    if(endp > startp)
    {
        do
        {
            if(*startp++ == '%')
            {
                tlen = conlen;
                tp = conid;
                ip = startp;

                do
                {
                    if(*ip != *tp)
                        if(toupper(*ip) != toupper(*tp))
                            break;
                    ++ip;
                    ++tp;
                }
                while(--tlen);

                if(!tlen)
                    return(ip);
            }
        }
        while(startp < endp);
    }

    return(NULL);
}

/******************************************************************************
*
* search PipeDream file for option
*
******************************************************************************/

static uchar *
searchopt(
    const char *optid)
{
    const S32 optlen = strlen(optid);
    uchar *curp = pdf, *endp = optend ? optend : pdend;
    uchar *oldp = pdf;
    S32 tlen;
    const char *tp;
    uchar *ip;

    while((curp = searchcon(curp, endp, "OP")) != 0)
    {
        if(*curp++ != '%')
            continue;

        tlen = optlen;
        tp = optid;
        ip = curp;
        do
        {
            if(*ip != *tp)
                if(toupper(*ip) != toupper(*tp))
                    break;
            ++ip;
            ++tp;
        }
        while(--tlen);

        if(!tlen)
            return(ip);

        oldp = curp;
    }

    /* set pointer to last option */
    optend = oldp;
    return(NULL);
}

/******************************************************************************
*
* recognise a constant and classify
*
* --out--
* 0 - no constant found
* PD_DATE found date - in day, mon, yr
* PD_INTEGER integer - in fpval
* PF_REAL double - in fpval
*
******************************************************************************/

static S32
reccon(
    uchar *slot,
    P_F64 fpval,
    P_S32 day,
    P_S32 mon,
    P_S32 yr,
    P_S32 cs)
{
    S32 res;
    uchar tstr[25], *tp;

    /* check for date without brackets */
    if((res = sscanf(slot, "%d.%d.%d%n", day, mon, yr, cs)) == 3)
    {
        #if WINDOWS
        /* check for sscanf bug */
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif
        return(PD_DATE);
    }

    /* check for date with brackets */
    if((res = sscanf(slot, "(%d.%d.%d)%n", day, mon, yr, cs)) == 3)
    {
        /* check for sscanf bug */
        #if WINDOWS
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif
        return(PD_DATE);
    }

    /* a fuddle cos sscanf doesn't get ".5", needs "0.5" */
    tp = tstr;
    if(*slot == '.')
        *tp++ = '0';
    strncpy(tp, slot, 25);
    if((res = sscanf(tstr, "%lf%n", fpval, cs)) > 0)
    {
        /* account for inserted zero */
        if(tp != tstr)
            --(*cs);

        /* check for sscanf bug */
        #if WINDOWS
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif

        if((floor(*fpval) == *fpval) &&
           (*fpval < 32767.) && (*fpval > -.32767))
            return(PD_INTEGER);
        else
            return(PD_REAL);
    }

    /* ensure that cs is zero - some sscanfs
    don't set it to zero if the scan nowt */
    *cs = 0;
    return(0);
}

/******************************************************************************
*
* show the current row number on the screen
*
******************************************************************************/

#ifdef UTIL_PTL

static S32
showrow(
    S32 row)
{
    printf("\rRow: %ld", (long) row);
    return(0);
}

#endif

/******************************************************************************
*
* scan a cell reference
*
******************************************************************************/

extern S32
pd123__scnslr(
    uchar *exppos_in,
    U16 *col,
    U16 *row)
{
    uchar *c = exppos_in;
    U16 absc = 0, absr = 0;
    S32 cr, res;
    S32 tc, tr;

    if(*c == '$')
    {
        ++c;
        absc = 0x8000;
    }

    if((cr = lotus_stox(c, &tc)) == 0)
        return(0);
    *col = (U16) (tc & 0xFF);

    c += cr;
    if(*c == '$')
    {
        ++c;
        absr = 0x8000;
    }

    if(!isdigit(*c))
        return(0);

    res = sscanf(c, "%d%n", &tr, &cr);
    if((res < 1) || !cr)
        return(0);

    *row = (U16) (tr & 0x3FFF);

    #if WINDOWS
    /* check for sscanf bug */
    if(!(*(c + cr - 1)))
        --cr;
    #endif

    --(*row);
    *col |= absc;
    *row |= absr;

    return(c - exppos_in + cr);
}

/******************************************************************************
*
* convert column string into column number
*
******************************************************************************/

static S32
lotus_stox(
    uchar *string,
    P_S32 col)
{
    S32 cr = 0, i, tcol;

    i = toupper(*string) - 'A';
    ++string;
    if((i >= 0) && (i <= 25))
    {
        tcol = i;
        cr = 1;
        i = toupper(*string) - 'A';
        ++string;
        if((i >= 0) && (i <= 25))
        {
            tcol = (tcol + 1) * 26 + i;
            cr = 2;
        }
        *col = tcol;
    }

    return(cr);
}

/******************************************************************************
*
* write out calculation mode
*
******************************************************************************/

static S32
wrlcalcmode(void)
{
    S32 err;
    uchar *optp;
    S32 ofmt;

    optp = searchopt("AM");
    if(optp)
        ofmt = (toupper(*optp) == 'M') ? 0 : 0xFF;
    else
        ofmt = pd123__searchdefo("AM");

    if((err = writeins(curlfi)) != 0)
        return(err);
    return(pd123__foutc(ofmt, pd123__fout));
}

/******************************************************************************
*
* write out calculation order
*
******************************************************************************/

static S32
wrlcalcord(void)
{
    S32 err, ofmt;
    uchar *optp;

    optp = searchopt("RC");
    if(optp)
    {
        switch(toupper(*optp))
        {
        case 'C':
            ofmt = 1;
            break;
        case 'R':
            ofmt = 0xFF;
            break;
        default:
        case 'N':
            ofmt = 0;
            break;
        }
    }
    else
        ofmt = pd123__searchdefo("RC");

    if((err = writeins(curlfi)) != 0)
        return(err);
    return(pd123__foutc(ofmt, pd123__fout));
}

/******************************************************************************
*
* write out all the column information
*
******************************************************************************/

static S32
wrcols(void)
{
    S32 err, row = 0, col, didacol;
    S32 dcp, slotbits;
    uchar *c, *constr;
    uchar slot[256], cc, *op;

    do  {
        didacol = 0;
        for(col = 0; col < pd123__maxcol; ++col)
        {
            if(((c = colcur[col]) != 0) && (c < colend[col]))
            {
                constr = NULL;
                op = slot;
                slotbits = dcp = 0;
                while(cc = *c++, (cc != LF) && (cc != CR))
                {
                    if(cc == '%')
                    {
                        /* look up a construct */
                        if(constr && (op - constr < 25))
                        {
                            conp pcons = NULL;
                            U32 i;

                            for(i = 0; i < elemof32(constab); ++i)
                            {
                                uchar *c1 = constr + 1;
                                const char *c2 = constab[i].conid;

                                while(isalpha(*c1) && (toupper(*c1) == *c2))
                                {
                                    ++c1;
                                    ++c2;
                                    if(!*c2)
                                    {
                                        pcons = &constab[i];
                                        break;
                                    }
                                }

                                *op = CH_NULL;
                                if(pcons)
                                {
                                    if(pcons->proccons)
                                        op = (*pcons->proccons)(constr,
                                                                c1,
                                                                &dcp);
                                    else
                                        op = constr;
                                    slotbits |= pcons->mask;
                                    break;
                                }
                            }

                            if(!pcons)
                                *op++ = cc;
                            constr = NULL;
                        }
                        else
                        {
                            constr = op;
                            *op++ = cc;
                        }
                    }
                    else
                    {
                        *op++ = cc;
                    }
                }

                *op++ = CH_NULL;
                if((cc == LF) && (*c == CR))
                    ++c;
                else if((cc == CR) && (*c == LF))
                    ++c;

                /* save position in column */
                colcur[col] = c;

                /* now write out cell contents */
                if(strlen(slot))
                    if((err = writeslot(slot, col, row, slotbits, dcp)) != 0)
                        return(err);
                didacol = 1;
            }
        }
        ++row;

        /* check for the file getting too big */
        if(row >= LOTUS_MAXROW)
            return(PD123_ERR_BIGFILE);

        if(counter)
            if((*counter)( ((colcur[0] - pdf) * (S32) 100) /
                           ((colend[0] - pdf) + (S32) 1) ))
                break;
    }
    while(didacol);
    return(0);
}

/******************************************************************************
*
* write out column width records
*
******************************************************************************/

static S32
wrcolws(void)
{
    S32 err, i, cwid;

    for(i = 0; i < pd123__maxcol; ++i)
    {
        cwid = colwid[i];

        /* weed out hidden cols and default widths */
        if(!cwid || (cwid == 9))
            continue;

        if((err = writeins(curlfi)) != 0)
            return(err);
        if((err = lts_writeuword(i)) != 0)
            return(err);
        if((err = pd123__foutc((uchar) cwid, pd123__fout)) != 0)
            return(err);
    }
    return(0);
}

/******************************************************************************
*
* write out footer
*
******************************************************************************/

static S32
wrfooter(void)
{
    return(wrlheadfoot("FO"));
}

/******************************************************************************
*
* write out graph record
*
******************************************************************************/

static S32
wrgraph(void)
{
    static uchar part1[] = { '\xFF', '\xFF', '\x0', '\x0' };
    static uchar part2[] = { '\x04', '\x00', '\x0', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3' };
    static uchar part3[] = { '\x71', '\x71', '\x1', '\x0', '\x0', '\x0' };
    S32 l1 = sizeof(part1), l2 = sizeof(part2), l3 = sizeof(part3);
    S32 err, i, x;
    uchar *c;

    if((err = writeins(curlfi)) != 0)
        return(err);

    for(i = 0; i < 26; ++i)
        for(x = 0, c = part1; x < l1; ++x)
            if((err = pd123__foutc(*c++, pd123__fout)) != 0)
                return(err);

    for(i = 0, c = part2; i < l2; ++i)
        if((err = pd123__foutc(*c++, pd123__fout)) != 0)
            return(err);

    for(i = 0; i < 320; ++i)
        if((err = pd123__foutc(0, pd123__fout)) != 0)
            return(err);

    for(i = 0, c = part3; i < l3; ++i)
        if((err = pd123__foutc(*c++, pd123__fout)) != 0)
            return(err);

    return(0);
}

/******************************************************************************
*
* write out header
*
******************************************************************************/

static S32
wrheader(void)
{
    return(wrlheadfoot("HE"));
}

/******************************************************************************
*
* write out header or footer
*
******************************************************************************/

static S32
wrlheadfoot(
    uchar *optid)
{
    S32 err, len;
    uchar *optp;
    uchar delim, co, tstr[10];

    if((err = writeins(curlfi)) != 0)
        return(err);

    len = curlfi->length;
    optp = searchopt(optid);
    if(optp)
    {
        delim = *optp++;

        while((*optp != CR) && (*optp != LF) && (optp < pdend) && len)
        {
            co = *optp++;
            if(co == delim)
            {
                co = '|';
            }
            else if(co == '@')
            {
                tstr[0] = co;
                tstr[1] = *optp;
                tstr[2] = *(optp + 1);

                if(0 == u_strnicmp(tstr, "@D@", 3))
                {
                    co = '@';
                    optp += 2;
                }
                else if(0 == u_strnicmp(tstr, "@P@", 3))
                {
                    co = '#';
                    optp += 2;
                }
            }

            if((err = pd123__foutc((len--, pd123__ichlotus(co)), pd123__fout)) != 0)
                return(err);
        }
    }

    while(len--)
        if((err = pd123__foutc(0, pd123__fout)) != 0)
            return(err);

    return(0);
}

/******************************************************************************
*
* write out the hidden columns record
*
******************************************************************************/

static S32
wrhidvec(void)
{
    S32 err, i;

    if((err = writeins(curlfi)) != 0)
        return(err);
    for(i = 0; i < 32; ++i)
        if((err = pd123__foutc((uchar) pd123__hidvec[i], pd123__fout)) != 0)
            return(err);
    return(0);
}

/******************************************************************************
*
* write out column and row pair
*
******************************************************************************/

static S32
writecolrow(
    S32 col,
    S32 row)
{
    S32 err;

    if((err = lts_writeuword(col)) != 0)
        return(err);
    return(lts_writeuword(row));
}

/******************************************************************************
*
* write a double out to the lotus file
*
******************************************************************************/

static S32
lts_writedouble(
    F64 fpval)
{
    S32 err;
    U32 i;
    union LTS_WRITEDOUBLE_U
    {
        F64 fpval;
        char fpbytes[sizeof(F64)];
    } fp;

    fp.fpval = fpval;

#if RISCOS

    for(i = 4; i < sizeof(F64); ++i)
        if((err = pd123__foutc(fp.fpbytes[i], pd123__fout)) != 0)
            return(err);

    for(i = 0; i < 4; ++i)
        if((err = pd123__foutc(fp.fpbytes[i], pd123__fout)) != 0)
            return(err);

#elif WINDOWS

    for(i = 0; i < sizeof(F64); ++i)
        if((err = pd123__foutc(fp.fpbytes[i], pd123__fout)) != 0)
            return(err);

#endif /* OS */

    return(0);
}

/******************************************************************************
*
* write out a lotus structure instruction
*
******************************************************************************/

static S32
writeins(
    lfip curlfi_in)
{
    S32 err;

    if((err = lts_writeuword((U16) curlfi_in->opcode)) != 0)
        return(err);
    return((err = lts_writeuword((U16) curlfi_in->length)) != 0);
}

/******************************************************************************
*
* write out a cell as a label
*
******************************************************************************/

static S32
writelabel(
    uchar *slot,
    S32 col, S32 row,
    S32 mask)
{
    S32 err;
    uchar align;

    /* write out label */
    if((err = lts_writeuword(L_LABEL)) != 0)
        return(err);
    /* length: overhead + align + contents + null */
    if((err = lts_writeuword((U16) (5 + 1 + strlen(slot) + 1))) != 0)
        return(err);
    if((err = pd123__foutc(0xFF, pd123__fout)) != 0)
        return(err);
    if((err = writecolrow(col, row)) != 0)
        return(err);

    /* default left alignment */
    align = '\'';
    if(mask & BIT_RYT)
        align = '"';
    if(mask & BIT_CEN)
        align = '^';

    if((err = pd123__foutc((S32) align, pd123__fout)) != 0)
        return(err);

    while(*slot)
        if((err = pd123__foutc(pd123__ichlotus((S32) *slot++), pd123__fout)) != 0)
            return(err);

    return(pd123__foutc(0, pd123__fout));
}

/******************************************************************************
*
* write out a date
*
******************************************************************************/

static S32
writeldate(
    S32 day,
    S32 mon,
    S32 yr,
    S32 col,
    S32 row)
{
    S32 err, i;

    /* write out length and date format */
    if((err = lts_writeuword(L_FORMULA)) != 0)
        return(err);
    if((err = lts_writeuword(26)) != 0)
        return(err);
    if((err = pd123__foutc(L_PROT | L_SPECL | L_DDMMYY, pd123__fout)) != 0)
        return(err);
    if((err = writecolrow(col, row)) != 0)
        return(err);

    /* write out value */
    for(i = 0; i < 4; ++i)
        if((err = lts_writeuword(0)) != 0)
            return(err);

    /* write out @DATE(yr, mon, day) expression */
    if((err = lts_writeuword(11)) != 0)
        return(err);
    if((err = pd123__foutc(LF_INTEGER, pd123__fout)) != 0)
        return(err);
    if((err = lts_writeuword(yr)) != 0)
        return(err);
    if((err = pd123__foutc(LF_INTEGER, pd123__fout)) != 0)
        return(err);
    if((err = lts_writeuword(mon)) != 0)
        return(err);
    if((err = pd123__foutc(LF_INTEGER, pd123__fout)) != 0)
        return(err);
    if((err = lts_writeuword(day)) != 0)
        return(err);

    if((err = pd123__foutc(LF_DATE, pd123__fout)) != 0)
        return(err);
    if((err = pd123__foutc(LF_END, pd123__fout)) != 0)
        return(err);

    return(0);
}

/******************************************************************************
*
* write out lotus format byte given mask and dcp
*
******************************************************************************/

static S32
writelformat(
    S32 mask,
    S32 dcp)
{
    S32 ofmt = -1;

    /* ensure sensible value in dcp */
    if(!(mask & BIT_DCN))
    {
        if(decplc == -1)
            dcp = 2;
        else
            dcp = decplc;
    }

    /* priority: GENERAL>BRACKETS>DEC.PLACES */
    /* check for decimal places */
    if(mask & BIT_DCN)
        ofmt = L_FIXED | dcp;

    /* check for brackets needed */
    if(mask & BIT_BRK)
    {
        if(mask & BIT_CUR)
            ofmt = L_CURCY | dcp;
        else
            ofmt = L_COMMA | dcp;
    }

    /* check for general format */
    if(mask & BIT_DCF)
        ofmt = L_SPECL | L_GENFMT;

    if(ofmt == -1)
        ofmt = L_SPECL | L_DEFAULT;

    return(pd123__foutc(L_PROT | ofmt, pd123__fout));
}

/******************************************************************************
*
* write out cell to lotus file
*
******************************************************************************/

static S32
writeslot(
    uchar *slot,
    S32 col, S32 row, U16 mask,
    S32 dcp)
{
    S32 err, res, day, mon, yr, cs, len;
    uchar *ep;

    if(!(mask & BIT_EXP))
    {
        if((err = writelabel(slot, col, row, mask)) != 0)
            return(err);
    }
    else
    {
        F64 fpval;

        /* try to interpret the cell as an integer/float/date */
        res = reccon(slot, &fpval, &day, &mon, &yr, &cs);

        /* check characters were scanned and
        that we got to the end of the cell */
        if(!cs || (*(slot + cs)))
            res = 0;

        switch(res)
        {
        case PD_REAL:
            /* output floating value */
            if((err = writesslot(L_NUMBER, 13, col, row, mask, dcp)) != 0)
                return(err);

            return(lts_writedouble(fpval));
            break;

        case PD_INTEGER:
            /* output integer */
            if((err = writesslot(L_INTEGER, 7, col, row, mask, dcp)) != 0)
                return(err);

            return(lts_writeword((S16) fpval));
            break;

        case PD_DATE:
            return(writeldate(day, mon, yr, col, row));
            break;

        default:
            /* must be expression */
            if((len = compileexp(slot, col, row)) != 0)
            {
                if((err = writesslot(L_FORMULA, len + 15, col, row, mask, dcp)) != 0)
                    return(err);

                if((err = lts_writedouble(0.)) != 0)
                    return(err);
                ep = expbuf;
                lts_writeuword(len);
                while(len--)
                    if((err = pd123__foutc(*ep++, pd123__fout)) != 0)
                        return(err);
            }
            else
            {
                /* write out bad expression as a label */
                if((err = writelabel(slot, col, row, mask)) != 0)
                    return(err);
                ++pd123__errexp;
            }
            break;
        }
    }

    return(0);
}

/******************************************************************************
*
* write out start of cell
*
******************************************************************************/

static S32
writesslot(
    S32 opc,
    S32 len,
    S32 col,
    S32 row,
    U16 mask,
    S32 dcp)
{
    S32 err;

    if((err = lts_writeuword(opc)) != 0)
        return(err);
    if((err = lts_writeuword(len)) != 0)
        return(err);
    if((err = writelformat(mask, dcp)) != 0)
        return(err);
    return(writecolrow(col, row));
}

/******************************************************************************
*
* write an unsigned word out to the lotus file
*
******************************************************************************/

static S32
lts_writeuword(
    U16 aword)
{
    S32 err;
    union LTS_WRITEUWORD_U
    {
        U16 uword;
        uchar uwbytes[sizeof(U16)];
    } uw;

    uw.uword = aword;
    if((err = pd123__foutc(uw.uwbytes[0], pd123__fout)) == 0)
        err = pd123__foutc(uw.uwbytes[1], pd123__fout);
    return(err);
}

/******************************************************************************
*
* write a signed word out to the lotus file
*
******************************************************************************/

static S32
lts_writeword(
    S16 aword)
{
    S32 err;
    union LTS_WRITEWORD_U
    {
        S16 word;
        char wbytes[sizeof(S16)];
    } w;

    w.word = aword;
    if((err = pd123__foutc(w.wbytes[0], pd123__fout)) == 0)
        err = pd123__foutc(w.wbytes[1], pd123__fout);
    return(err);
}

/******************************************************************************
*
* write out a margin value
*
******************************************************************************/

static S32
wrlmar(
    const char *optid)
{
    S32 i;
    uchar *optp;
    S32 curv = 0;
    uchar tstr[5];

    optp = searchopt(optid);
    if(optp)
    {
        for(i = 0; i < 5; ++i)
            tstr[i] = *optp++;
        if(sscanf(tstr, "%d", &curv) < 1)
            optp = NULL;
    }

    if(!optp)
        curv = pd123__searchdefo(optid);

    return(lts_writeuword(curv));
}

/******************************************************************************
*
* write out the margins
*
******************************************************************************/

static S32
wrlmargins(void)
{
    S32 err;

    if((err = writeins(curlfi)) != 0)
        return(err);

    if((err = wrlmar("LM")) != 0)
        return(err);
    if((err = lts_writeuword(76)) != 0)
        return(err);
    if((err = wrlmar("PL")) != 0)
        return(err);
    if((err = wrlmar("TM")) != 0)
        return(err);
    return(wrlmar("BM"));
}

/******************************************************************************
*
* write out window1 information
*
******************************************************************************/

static S32
wrwindow1(void)
{
    S32 err, i;
    uchar *optp;
    S32 ofmt, widleft, ncols;

    if((err = writeins(curlfi)) != 0)
        return(err);
    /* current cursor column */
    if((err = lts_writeuword(0)) != 0)
        return(err);
    /* current cursor row */
    if((err = lts_writeuword(0)) != 0)
        return(err);

    /* work out format byte */
    optp = searchopt("DP");
    if(optp)
    {
        if(toupper(*optp) == 'F')
        {
            ofmt = L_SPECL | L_GENFMT;
            decplc = -1;
        }
        else
        {
            uchar *tp = optp;
            uchar tstr[12];

            for(i = 0; i < 12; ++i)
                tstr[i] = *tp++;

            consume_int(sscanf(tstr, "%d", &decplc));
            ofmt = decplc | L_FIXED;
            optp = searchopt("MB");
            if(optp && (toupper(*optp) == 'B'))
                ofmt = decplc | L_COMMA;
        }
    }
    else
    {
        ofmt = pd123__searchdefo("DP");
    }

    /* write out format byte */
    if((err = pd123__foutc(ofmt, pd123__fout)) != 0)
        return(err);
    /* write out padder */
    if((err = pd123__foutc(0, pd123__fout)) != 0)
        return(err);
    /* write out default column width */
    if((err = lts_writeuword(9)) != 0)
        return(err);

    /* calculate columns on screen and set up hidden vector */
    widleft = 76;
    for(i = ncols = 0; i < pd123__maxcol; ++i)
        if(!colwid[i])
        {
            pd123__hidvec[(i >> 3)] |= (1 << (i & 7));
        }
        else
        {
            if(widleft > 0)
            {
                ++ncols;
                widleft -= colwid[i];
            }
        }

    /* write out number of cols on screen */
    if((err = lts_writeuword(ncols)) != 0)
        return(err);
    /* number of rows */
    if((err = lts_writeuword(20)) != 0)
        return(err);

    /* col/row left/top, #title col/rows,
     * title col/row left/top
    */
    for(i = 0; i < 6; ++i)
        if((err = lts_writeuword(0)) != 0)
            return(err);

    /* border width column/row */
    if((err = lts_writeuword(4)) != 0)
        return(err);
    if((err = lts_writeuword(4)) != 0)
        return(err);

    /* window width */
    if((err = lts_writeuword(76)) != 0)
        return(err);
    /* padding */
    return(err = lts_writeuword(0));
}

/******************************************************************************
*
* case-insensitive lexical comparison of part of two strings
*
******************************************************************************/

static int
u_strnicmp(
    PC_U8 a,
    PC_U8 b,
    size_t n)
{
    int c1, c2;

    while(n-- > 0)
    {
        c1 = *a++;
        c2 = *b++;

        if(c1 == c2)
            continue;

        c1 = tolower(c1);
        c2 = tolower(c2);

        if(c1 != c2)
            return(c1 - c2);

        if(c1 == 0)
            return(0); /* strings ended together before limit */
    }

    /* strings are equal up to n characters */
    return(0);
}

/* end of 123pd.c */
