/* vsload.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Code to interpret a ViewSheet file */

/* MRJC May 1989 */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "xstring.h"

/* local header file */
#include "vsload.h"

#ifndef uchar
#define uchar char
#endif

struct VSFILEHEADER
{
    uchar maxrow;
    uchar scmode;
    uchar curwin;
    uchar scrwin[120];
    uchar prnwin[100];
    uchar stracc[14];
    uchar prttab[64];
    uchar rowpn1;         /* this because the ARM can't byte align ANYTHING */
    uchar rowpn2;
    uchar rtbpn1;
    uchar rtbpn2;
    uchar ctbpn1;
    uchar ctbpn2;
    uchar sltpn1;
    uchar sltpn2;
    uchar frepn1;
    uchar frepn2;
    uchar fileid;
};

struct ROWTABENTRY
{
    uchar colsinrow;
    uchar offtoco1;
    uchar offtoco2;
};

#define VS_MAXSLOTLEN 255

/*
internal functions
*/

static S32
vsxtos(
    char *string,
    S32 x);

/*
static data
*/

static struct VSFILEHEADER * vsp    = NULL;
static char *                outbuf = NULL;

struct VSFUNC
{
    const char * name;
    char   flags;
};

#define BRACKET 0
#define NO_SUM 1

static const struct VSFUNC
vsfuncs[] =
{
    { "abs(",     BRACKET },
    { "acs(",     BRACKET },
    { "asn(",     BRACKET },
    { "atn(",     BRACKET },
    { "average(",  NO_SUM },
    { "choose(",  BRACKET },
    { "col",      BRACKET },
    { "cos(",     BRACKET },
    { "deg(",     BRACKET },
    { "exp(",     BRACKET },
    { "if(",      BRACKET },
    { "int(",     BRACKET },
    { "ln(",      BRACKET },
    { "log(",     BRACKET },
    { "lookup(",   NO_SUM },
    { "max(",      NO_SUM },
    { "min(",      NO_SUM },
    { "pi",       BRACKET },
    { "rad(",     BRACKET },
    { "read(",    BRACKET },
    { "row",      BRACKET },
    { "sgn(",     BRACKET },
    { "sin(",     BRACKET },
    { "sqr(",     BRACKET },
    { "tan(",     BRACKET },
    { "write(",   BRACKET },
};

/******************************************************************************
*
* say whether a file is a ViewSheet file or not
*
******************************************************************************/

_Check_return_
extern BOOL
vsload_fileheader_isvsfile(
    _In_reads_(size) PC_U8 ptr,
    _InVal_     U32 size)
{
    const struct VSFILEHEADER * p_vsfh;

    if(size >= sizeof32(*p_vsfh))
    {
        p_vsfh = (const struct VSFILEHEADER *) ptr;

        trace_3(TRACE_APP_PD4, "vsload_fileheader_isvsfile curwin: %d, fileid: %d, scmode: %d",
                p_vsfh->curwin, p_vsfh->fileid, p_vsfh->scmode);

        if( (p_vsfh->curwin <= 108)  &&
            (p_vsfh->fileid == 0xDD) )
            return(1);
    }

    return(0);
}

#ifdef UNUSED

_Check_return_
extern STATUS
vsload_isvsfile(
    FILE_HANDLE fin)
{
    U8 vsfh[sizeof(struct VSFILEHEADER)];
    filepos_t pos;
    S32 bytes_read;
    STATUS res;

    file_getpos(fin, &pos);

    /* loop for structure */
    res = FALSE;

    for(;;)
    {
        /* read in ViewSheet file header */
        if(file_seek(fin, 0, SEEK_SET) < 0)
        {
            res = create_error(VSLOAD_ERR_CANTREAD);
            break;
        }

        if((bytes_read = file_read(&vsfh, 1, sizeof(vsfh), fin)) < 0)
        {
            res = create_error(VSLOAD_ERR_CANTREAD);
            break;
        }

        res = vsload_fileheader_isvsfile(vsfh, bytes_read);

        break; /* always */
    }

    /* restore file */
    file_setpos(fin, &pos);

    return(res);
}

#endif

/******************************************************************************
*
* load the ViewSheet file into memory ready for action
*
******************************************************************************/

_Check_return_
extern STATUS
vsload_loadvsfile(
    FILE_HANDLE fin)
{
    S32 vsfsize;
    STATUS res;

    /* read file size */
    if((vsfsize = file_length(fin)) <= 0)
        return(create_error(VSLOAD_ERR_CANTREAD));

    /* allocate memory to receive it */
    if(NULL == (vsp = al_ptr_alloc_bytes(struct VSFILEHEADER *, vsfsize, &res)))
        return(res);

    /* loop for structure */
    for(;;)
    {
        /* read in the file */
        if(file_seek(fin, 0, SEEK_SET) < 0)
        {
            res = create_error(VSLOAD_ERR_CANTREAD);
            break;
        }

        if(file_read(vsp, 1, vsfsize, fin) < 0)
        {
            res = create_error(VSLOAD_ERR_CANTREAD);
            break;
        }

        if(NULL == (outbuf = al_ptr_alloc_bytes(char *, (S32) VS_MAXSLOTLEN + 1, &res)))
            break;

        res = vsp->maxrow;

        break; /* always */
    }

    if(res < 0)
        vsload_fileend();

    return(res);
}

/******************************************************************************
*
* decode a ViewSheet cell into plain text
*
******************************************************************************/

#define SLR_START 1

static char *
vsdecodecell(
    char * cell_data)
{
    char * op;
    char * ip;
    char rangebuf[25];
    char bracstac[255];
    S32 slrcount, rangeix, bracix;

    slrcount = rangeix = 0;
    op = outbuf;
    ip = cell_data;

    bracstac[0] = BRACKET;
    bracix = 1;

    for(;;)
    {
        if(isalpha(*ip))
        {
            U32 i;

            for(i = 0; i < elemof32(vsfuncs); ++i)
                if(0 == C_strnicmp(vsfuncs[i].name, ip, strlen(vsfuncs[i].name)))
                    break;

            trace_1(TRACE_APP_PD4, "vsfuncs[i].name: %s]", vsfuncs[i].name);
            trace_1(TRACE_APP_PD4, "ip: %s]", ip);
            trace_1(TRACE_APP_PD4, "i: %d]", i);

            if(i < elemof(vsfuncs))
            {
                static char average_str[] = "average(";
                if(0 == C_strnicmp(average_str, ip, sizeof(average_str) - 1))
                {
                    strcpy(op, "avg(");
                    op += 4;
                }
                else
                {
                    strcpy(op, vsfuncs[i].name);
                    op += strlen(vsfuncs[i].name);
                    trace_0(TRACE_APP_PD4, "copied]");
                }

                ip += strlen(vsfuncs[i].name);

                bracstac[bracix++] = vsfuncs[i].flags;
                continue;
            }
        }
        else if(*ip == '(')
        {
            assert(bracix >= 1);
            bracstac[bracix] = bracstac[bracix - 1];
            bracix++;
        }
        else if(*ip == ')')
        {
            if(bracix)
                --bracix;
        }
        else if(*ip == SLR_START)
        {
            S32 col, row;

            col = (S32) *(++ip);
            row = (S32) *(++ip);
            rangeix += vsxtos(rangebuf + rangeix, col);
            rangeix += sprintf(rangebuf + rangeix, "%d", row + 1);
            rangebuf[rangeix] = CH_NULL;
            if(++slrcount == 2)
            {
                if(bracix && bracstac[bracix - 1] != NO_SUM)
                {
                    strcpy(op, "sum(");
                    op += 4;
                    strcpy(op, rangebuf);
                    op += rangeix;
                    strcpy(op, ")");
                    op += 1;
                    slrcount = rangeix = 0;
                }
            }

            ++ip;
            continue;
        }

        if(slrcount)
        {
            strcpy(op, rangebuf);
            op += rangeix;
            slrcount = rangeix = 0;
        }

        if(!(*op++ = *ip++))
            break;
    }

    return(outbuf);
}

/******************************************************************************
*
* free any resources used by the ViewSheet converter
*
******************************************************************************/

extern void
vsload_fileend(void)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(&vsp));

    al_ptr_dispose(P_P_ANY_PEDANTIC(&outbuf));
}

/******************************************************************************
*
* return a pointer to textual cell contents of a ViewSheet cell
*
******************************************************************************/

extern char *
vsload_travel(
    S32 col,
    S32 row,
    P_S32 type,
    P_S32 decp,
    P_S32 justright,
    P_S32 minus)
{
    char *vsdp, *rtbp, *cell_cont;
    struct ROWTABENTRY * rixp;
    U16 coloff;
    char * res;

    if(!vsp)
        return(NULL);

    if(row >= (S32) vsp->maxrow)
        return(NULL);

    /* calculate pointer to data area */
    vsdp = (char *) (vsp + 1);

    rtbp = vsdp + readval_S16(&vsp->rtbpn1);
    rixp = (struct ROWTABENTRY *) (rtbp + row * 3);
    if(col >= (S32) rixp->colsinrow)
        return(NULL);

    coloff = readval_U16(rtbp + readval_S16(&rixp->offtoco1) + 2 * col);

    /* check for null pointer */
    if(!coloff)
        return(NULL);

    /* work out type from top bit */
    if(coloff & 0x8000)
        *type = VS_TEXT;
    else
        *type = VS_NUMBER;

    coloff &= 0x7FFF;

    /* calculate final cell pointer */
    cell_cont = vsdp + readval_S16(&vsp->ctbpn1) + coloff;

    if(*type == VS_NUMBER)
    {
        char formatb;

        formatb = *(cell_cont + 5);
        if((formatb & 0x7F) == 0x7F)
        {
            /* default to FRM */
            *decp = -2;
            *minus = TRUE;
            *justright = TRUE;
        }
        else
        {
            if(formatb & 0x40)
                *decp = formatb & 0xF;
            else
                *decp = -2;

            if(formatb & 0x20)
                *justright = FALSE;
            else
                *justright = TRUE;

            if(formatb & 0x10)
                *minus = FALSE;
            else
                *minus = TRUE;
        }

        res = vsdecodecell(cell_cont + 6);
        return(res);
    }

    /* bash off right-justify label bit */
    if(*cell_cont & 0x80)
        *justright = TRUE;
    else
        *justright = FALSE;

    strcpy(outbuf, cell_cont);
    *outbuf &= 0x7F;

    return(outbuf);
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
vsxtos(
    char * string,
    S32 x)
{
    char * c = string;
    S32 digit1, digit2;

    digit2 = x / 26;
    digit1 = x - digit2 * 26;

    if(digit2)
        *c++ = (char) ((digit2 - 1) + (S32) 'A');

    *c++ = (char) (digit1 + (S32) 'A');
    return(c - string);
}

/* end of vsload.c */
