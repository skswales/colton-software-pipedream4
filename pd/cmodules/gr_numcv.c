/* gr_numcv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Numeric conversion */

/* 17/01/99 SKS Don't try to put commas in E format numbers!!! */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#define DOT_CH '.'

#if RISCOS
#ifndef SYSCHARWIDTH_OS
#define SYSCHARWIDTH_OS 16
#endif
#define SYSCHARWIDTH_MP (SYSCHARWIDTH_OS * GR_MILLIPOINTS_PER_RISCOS)  /* 6400 */
#define SYSCHARWIDTH_GR SYSCHARWIDTH_MP
#endif

#if RISCOS

#ifndef __cs_font_h
#include "cs-font.h"
#endif

/* sugaring function for system font use */

extern S32
gr_font_stringwidth(
    HOST_FONT f,
    PC_U8 str)
{
    font_string fs;
    S32 res;

    if(f)
    {
        fs.s     = (char *) str;
        fs.x     = INT_MAX;
        fs.y     = INT_MAX;
        fs.split = -1;
        fs.term  = INT_MAX;

        if((NULL == font_setfont(f)) &&
           (NULL == font_strwidth(&fs)) )
        {
            res = fs.x;
            return(res);
        }
    }

    res = strlen(str) * SYSCHARWIDTH_MP;

    return(res);
}

/******************************************************************************
*
* truncate a fonty string to a given width (millipoints)
* returning the new truncated width
*
******************************************************************************/

extern os_error *
gr_font_truncate(
    HOST_FONT f,
    char * str /*poked*/,
    int * width_mpp /*inout*/)
{
    font_string fs;
    os_error * e;

    if(!f)
    {
        S32 nchars       = strlen(str);
        S32 nchars_limit = *width_mpp / SYSCHARWIDTH_MP; /* rounding down */
        fs.term = MIN(nchars, nchars_limit);
        fs.x    = SYSCHARWIDTH_MP * fs.term;

        e = NULL;
    }
    else if(NULL == (e = font_SetFont(f)))
    {
        fs.s     = str;
        fs.x     = *width_mpp;
        fs.y     = INT_MAX;
        fs.split = -1;
        fs.term  = INT_MAX;

        e = font_strwidth(&fs);
    }

    if(NULL == e)
    {
        str[fs.term] = '\0';
        trace_2(TRACE_RISCOS_HOST, "gr_font_truncate new width: %d, str: \"%s\"", fs.x, str);
        *width_mpp = fs.x;
    }
    else
    {
        trace_1(TRACE_RISCOS_HOST, "gr_font_truncate error: %s", e->errmess);
        *width_mpp = 0;
    }

    return(e);
}

#endif /* RISCOS */

/******************************************************************************
*
* gr_numtostr outputs the given number into array
* if eformat is FALSE it outputs number in long form
* if eformat is TRUE  it outputs number in e format
*
******************************************************************************/

extern void
gr_numtostr(
    _Out_writes_z_(elemof_buffer) P_U8Z array,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_F64 pValue,
    S32 eformat,
    S32 decimals,
    char decimal_point_ch /* NULLCH -> default (.)    */,
    char thousands_sep_ch /* NULLCH -> default (none) */)
{
    F64 value = *pValue;
    P_U8 nextprint, numptr;
    P_U8 eptr, tptr, dotptr;
    S32 negative;
    S32 inc;

    if(value < 0.0)
    {
        value = fabs(value);
        negative = TRUE;
    }
    else
        negative = FALSE;

    if(!decimal_point_ch)
        decimal_point_ch = '.';

    /* prepare for output */
    nextprint = array;

    /* output number */
    if(negative)
        *nextprint++ = '-';

    /* note start of actual numeric conversion */
    numptr = nextprint;

    if(decimals < 0)
        (void) xsnprintf(nextprint, elemof_buffer - (nextprint - array), "%g", value);
    else
        (void) xsnprintf(nextprint, elemof_buffer - (nextprint - array), eformat ? "%.*e" : "%.*f", decimals, value);

    /* work out thousands separator */
    if(thousands_sep_ch && !eformat /*SKS 17.01.99*/)
    {
        S32 beforedot;

        /* count number of digits before decimal point/end of number */
        for(beforedot = 0, dotptr = nextprint;
            *dotptr && *dotptr != DOT_CH;
            ++dotptr)
            ++beforedot;

        if(*dotptr == DOT_CH)
            *dotptr++ = decimal_point_ch;
        else
            dotptr = NULL;

        while(beforedot > 3)
        {
            inc = ((beforedot - 1) % 3) + 1;
            nextprint += inc;

            memmove32(nextprint + 1, nextprint, strlen32p1(nextprint) /*for NULLCH*/);
            *nextprint++ = thousands_sep_ch;

            /* keep dot ptr updated */
            if(dotptr)
                ++dotptr;

            beforedot -= inc;
        }
    }
    else
    {
        /* replace (or at least find) decimal point
         * if not doing thousands
        */
        dotptr = strchr(numptr, DOT_CH);

        if(dotptr)
        {
            *dotptr++ = decimal_point_ch;
            nextprint = dotptr;
        }
    }

    /* find end of string */
    while(*nextprint++)
        ;
    --nextprint;

    /* get rid of superfluous E padding */
    eptr = strchr(numptr, 'e');

    if(eptr)
    {
        /* remove leading + from exponent */
        if(*++eptr == '+')
        {
            --nextprint;
            memmove32(eptr, eptr+1, strlen32p1(eptr+1) /*for NULLCH*/);
        }
        else if(*eptr == '-')
            eptr++;

        /* remove leading zeros from exponent */
        tptr = eptr;
        while((*tptr++ == '0')  &&  *tptr)
            ;
        --tptr;

        if(eptr != tptr)
        {
            nextprint -= tptr - eptr;
            memmove32(eptr, tptr, strlen32p1(tptr) /*for NULLCH*/);
        }
    }
}

/* end of gr_numcv.c */
