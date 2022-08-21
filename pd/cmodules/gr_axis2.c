/* gr_axis2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Chart axis handling */

/* SKS November 1991 */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

/* NB This is a copy of that in gr_axisp.c !!! */

/* allow for logs of numbers (especially those not in same base as FP representation) becoming imprecise */
#define LOG_SIG_EPS 1E-8

static F64
splitlognum(
    PC_F64 logval,
    /*out*/ P_F64 exponent)
{
    F64 mantissa;

    mantissa = modf(*logval, exponent);

    /*
    NB. consider numbers such as log10(0.2) ~ -0.698 = (-1.0 exp) + (0.30103 man)
    */
    if(mantissa < 0.0)
    {
        mantissa  += 1.0;
        *exponent -= 1.0;
    }

    /* watch for logs going awry */
    if(mantissa < LOG_SIG_EPS)
        /* keep rounded down */
        mantissa   = 0.0;

    else if((1.0 - mantissa) < LOG_SIG_EPS)
        /* round up */
    {
        mantissa   = 0.0;
        *exponent += 1.0;
    }

    return(mantissa);
}

/******************************************************************************
*
* given punter limits, flags and actual values for this
* category axis, form a current minmax and set up cached vars
*
******************************************************************************/

extern void
gr_axis_form_category(
    P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points)
{
    GR_AXES_IDX axes_idx = 0;
    GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    P_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    P_GR_AXIS_TICKS ticksp;

    /* how far up the interval is the zero line? */
    /* answer: categories thinks zero is at left */
    p_axis->zero_frac = 0.0;

    /* category axis major ticks */
    ticksp = &p_axis->major;

    ticksp->current = gr_lin_major(total_n_points - 0.0);
    /* forbid sub-category ticks */
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
            {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
            }
        }

    /* category axis minor ticks */
    ticksp = &p_axis->minor;

    ticksp->current = gr_lin_major(2.0 * p_axis->major.current);
    /* forbid sub-category ticks */
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
            {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
            }
        }
}

/******************************************************************************
*
* given punter limits, flags and actual values for this
* value axis, form a current minmax and set up cached vars
*
******************************************************************************/

static void
gr_axis_form_value_log(
    P_GR_AXES p_axes,
    P_GR_AXIS p_axis,
    _InVal_     BOOL is_y_axis)
{
    P_GR_AXIS_TICKS ticksp;
    F64 lna;

    p_axis->current = p_axis->actual;

    if(p_axis->current.min > p_axis->current.max)
        { /* SKS 28 Dec 2012 sort out potential exception (zombie) when no data on this axis */
        p_axis->current.min = 1.0;
        p_axis->current.max = 2.0;
        }

    if(!p_axis->bits.manual) /* SKS 25 Nov 1996 ensure punter gets updated from actual on first chart build */
        p_axis->punter = p_axis->current;

    if(!p_axes->bits.stacked_pct || !is_y_axis)
        if(p_axis->bits.manual)
            {
            if((p_axis->punter.min > 0.0) && (p_axis->punter.max > 0.0))
                { /* do sort just in case... */
                p_axis->current.min = MIN(p_axis->punter.min, p_axis->punter.max);
                p_axis->current.max = MAX(p_axis->punter.max, p_axis->punter.min);
                }
            }

    if(p_axis->bits.incl_zero)
        {
        /* in log scaling mode this 'obviously' means an exponent of zero! */
        p_axis->current.min = MIN(p_axis->current.min, 1.0);
        p_axis->current.max = MAX(p_axis->current.max, 1.0);
        }

    /* account for stupid log graphs */
    if( p_axis->current.min < 10.0*DBL_MIN)
        p_axis->current.min = 10.0*DBL_MIN;

    /* account for stupid log graphs */
    if( p_axis->current.max < 10.0*DBL_MIN)
        p_axis->current.max = 10.0*DBL_MIN;

    /* value axis major ticks (multiplier between ticks) */
    ticksp = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    /* my guesses are always in base 10 */
    ticksp->current = gr_lin_major(log10(p_axis->current.max) - log10(p_axis->current.min));
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;
    p_axis->bits.log_base = 10;
    ticksp->current = pow(p_axis->bits.log_base, ticksp->current);

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 1.0) /* must be multiplying by something significant each time */
            ticksp->current = 0.0; /* off */
        else
            {
            /* look at punter value and split into base and exponent step adder */
            F64 test, exponent;

            /* first try and split punter value up as base 10 animacule */
            test = log10(ticksp->punter);

            test = splitlognum(&test, &exponent);

            /* was number close enough to a power of the base? */
            if(test < LOG_SIG_EPS)
                {
                test = log10(ticksp->current);

                (void) splitlognum(&test, &test);

                test = ticksp->punter / test;

                if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                    {
                    /* use punter value unless it is silly or we area doing autocalc */
                    ticksp->current = ticksp->punter;
                    }
                }
            else
                {
                /* try base 2 */
                test = log(ticksp->punter) / log(2.0);

                test = splitlognum(&test, &exponent);

                /* was number close enough to a power of the base? */
                if(test < LOG_SIG_EPS)
                    {
                    ticksp->current = ticksp->punter;
                    p_axis->bits.log_base = 2;
                    }
                }
            }
        }

    lna = log(p_axis->bits.log_base);

    /* Round down the current.min to a multiple of the major value towards an exponent of -infinity */
    if(!p_axis->bits.manual || (p_axis->current.min != p_axis->punter.min))
        {
        F64 ftest;

        /* compute log-to-the-base-of-major-multiplier of the current.min */
        ftest = log(p_axis->current.min);

        ftest = ftest / lna;

        ftest = floor(ftest);

        p_axis->current.min = pow(p_axis->bits.log_base, ftest); /* could underflow */

        if( p_axis->current.min < DBL_MIN)
            p_axis->current.min = pow(10.0, DBL_MIN_10_EXP); /* may look bad for non-base 10 but who cares */
        }

    /* account for stupid graphs */
    if( p_axis->current.max <= p_axis->current.min)
        p_axis->current.max  = p_axis->current.min * (ticksp->current ? ticksp->current : 2.0);

    /* Round up the current.max to a multiple of the major value towards an exponent of +infinity */
    if(!p_axis->bits.manual || (p_axis->current.max != p_axis->punter.max))
        {
        F64 ctest;

        /* compute log-to-the-base-of-major-multiplier of the current.max */
        ctest = log(p_axis->current.max);

        ctest = ctest / lna;

        ctest = ceil(ctest);

        p_axis->current.max = pow(p_axis->bits.log_base, ctest);
        }

    /* cache the spanned interval NOW as major scaling might have modified the endpoints! */
    p_axis->current_span = (log(p_axis->current.max) - log(p_axis->current.min)) / lna;

    p_axis->zero_frac = 0.0; /* no real zero so say at bottom */

    /* value axis minor ticks (mantissa increment between ticks) */
    ticksp = &p_axis->minor;

    ticksp->current = 1.0;

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
            {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
            }
        }
}

static void
gr_axis_form_value_lin(
    P_GR_AXIS p_axis)
{
    P_GR_AXIS_TICKS ticksp;
    F64 major_interval;

    p_axis->current = p_axis->actual;

    if(p_axis->current.min > p_axis->current.max)
        { /* SKS 28 Dec 2012 sort out potential exception (zombie) when no data on this axis */
        p_axis->current.min = 0.0;
        p_axis->current.max = 1.0;
        }

    if(!p_axis->bits.manual) /* SKS 25 Nov 1996 ensure punter gets updated from actual on first chart build */
        p_axis->punter = p_axis->current;

    if(p_axis->bits.manual)
        {
        /* SKS 25 Nov 1996 don't override a punter selection even if it's really silly (but do sort just in case...) */
        p_axis->current.min = MIN(p_axis->punter.min, p_axis->punter.max);
        p_axis->current.max = MAX(p_axis->punter.max, p_axis->punter.min);
        }

    if(p_axis->bits.incl_zero)
        {
        p_axis->current.min = MIN(p_axis->current.min, 0.0);
        p_axis->current.max = MAX(p_axis->current.max, 0.0);
        }

    /* value axis major ticks */
    ticksp = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    ticksp->current = gr_lin_major(p_axis->current.max - p_axis->current.min);
    major_interval  = ticksp->current;

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
            {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                {
                /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
                major_interval  = ticksp->current;
                }
            }
        }
#if 1
    /* SKS after 4.12 26mar92 - in auto mode only: rjm reckons this is what punters would like and I agree */
    else
        {
        if(ticksp->current == 0.5)
            {
            ticksp->current = 1.0;
            major_interval  = ticksp->current;
            }
        }
#endif

    /* remember for decimal places rounding of values shown on axis */
    {
    F64 log10_major, exponent;
    int    decimals;

    log10_major = log10(major_interval);
    log10_major = splitlognum(&log10_major, &exponent);
    decimals    = (int) floor(exponent);
    p_axis->major.bits.decimals = (decimals < 0) ? -decimals : 0;
    }

    /* Round down the current.min to a multiple of the major value towards -infinity */
    if(!p_axis->bits.manual || (p_axis->current.min != p_axis->punter.min))
        {
        F64 ftest;

        ftest = floor(p_axis->current.min / major_interval);

        p_axis->current.min = ftest * major_interval;
        }

    /* account for stupid graphs */
    if( p_axis->current.max <= p_axis->current.min)
        p_axis->current.max  = p_axis->current.min + major_interval;

    /* Round up the current.max to a multiple of the major value towards +infinity */
    if(!p_axis->bits.manual || (p_axis->current.max != p_axis->punter.max))
        {
        F64 ctest;

        ctest = ceil( p_axis->current.max / major_interval);

        p_axis->current.max = ctest * major_interval;
        }

    /* cache the spanned interval NOW as major scaling might have modified the endpoints! */
    p_axis->current_span = p_axis->current.max - p_axis->current.min;

    /* how far up the interval is the zero line? */
    p_axis->zero_frac = (0.0 - p_axis->current.min) / p_axis->current_span;

    /* this is limited to either end of the interval */
    if( p_axis->zero_frac < 0.0)
        p_axis->zero_frac = 0.0;
    if( p_axis->zero_frac > 1.0)
        p_axis->zero_frac = 1.0;

    /* major tick interval as a fraction of the current span */
    p_axis->current_frac = major_interval / p_axis->current_span;

    /* value axis minor ticks */
    ticksp = &p_axis->minor;

    ticksp->current = gr_lin_major(2.0 * major_interval);

    if(ticksp->bits.manual)
        {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
            {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= 1.0/10.0) && (test <= 10.0/1.0))
                /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
            }
        }
}

extern void
gr_axis_form_value(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx)
{
    P_GR_AXES p_axes = &cp->axes[axes_idx];
    P_GR_AXIS p_axis = &p_axes->axis[axis_idx];

    if(p_axis->bits.log_scale)
        gr_axis_form_value_log(p_axes, p_axis, Y_AXIS_IDX == axis_idx);
    else
        gr_axis_form_value_lin(p_axis);
}

/******************************************************************************
*
* guess what would be a good major mark interval
*
******************************************************************************/

/* helper routine needed to stop MSC8.00 compiler barfing on release */

static BOOL
gr_lin_major_test(
    PC_F64 p_mantissa,
    PC_F64 p_cutoff,
    /*inout*/ P_F64 p_test,
    PC_F64 p_divisor)
{
    if(*p_mantissa < *p_cutoff)
    {
        *p_test = *p_test / *p_divisor;
        return(TRUE);
    }

    return(FALSE);
}

/* eg.
 *   99 -> 1, .9956 -> 10 so leave 10
 *   49 -> 1, .6902 -> 10 but make 5
 *   19 -> 1, .2788 -> 10 but make 2
*/
extern F64
gr_lin_major(
    F64 span)
{
    static const F64 cutoff[3] =
    {
        0.11394 /* approx. log10(1.2 + 0.1)*/,
        0.39794 /* approx. log10(2.4 + 0.1)*/,
        0.78533 /* approx. log10(6.0 + 0.1)*/
    };
    static const F64 divisor[3] =
    {
        10.0/1,
        10.0/2,
        10.0/5
    };
    F64 test, mantissa, exponent;

    if(span == 0.0)
        return(1.0);

    test = log10(fabs(span));

    mantissa = splitlognum(&test, &exponent);

    test = pow(10.0, exponent);

    if(gr_lin_major_test(&mantissa, &cutoff[0], &test, &divisor[0]))
        return(test);

    if(gr_lin_major_test(&mantissa, &cutoff[1], &test, &divisor[1]))
        return(test);

    if(gr_lin_major_test(&mantissa, &cutoff[2], &test, &divisor[2]))
        return(test);

    return(test);
}

/* end of gr_axis2.c */
