/* gr_axisp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Chart axis handling - plotting and dialog boxing */

/* SKS November 1991 */

/* SKS 15 Jan 1997 Allow punters to override axis min/max */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

/******************************************************************************
*
* convert an axes,axidx pair into an external axes number
*
******************************************************************************/

extern GR_AXES_NO
gr_axes_external_from_ix(
    PC_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis)
{
    assert(cp);

    if(cp->d3.bits.use)
        return((axes * 3) + axis + 1);

    return((axes * 2) + axis + 1);
}

/******************************************************************************
*
* convert an external axes number into an axes,axidx pair
*
******************************************************************************/

extern GR_AXIS_NO
gr_axes_ix_from_external(
    PC_GR_CHART cp,
    GR_AXES_NO eaxes,
    _OutRef_    P_GR_AXES_NO axes)
{
    assert(cp);

    myassert0x(eaxes != 0, "external axes 0");

    eaxes -= 1;

    if(cp->d3.bits.use)
        {
        myassert1x(eaxes < 6, "external axes id %d > 6", eaxes);
        *axes = eaxes / 3;
        return(eaxes % 3);
        }

    myassert1x(eaxes < 4, "external axes id > 4", eaxes);
    *axes = eaxes / 2;
    return(eaxes % 2);
}

/******************************************************************************
*
* return the axes index of a seridx
*
******************************************************************************/

extern GR_AXES_NO
gr_axes_ix_from_seridx(
    PC_GR_CHART   cp,
    GR_SERIES_IX seridx)
{
    if(seridx < cp->axes[0].series.end_idx)
        return(0);

    return(1);
}

/******************************************************************************
*
* return the axes ptr of a seridx
*
******************************************************************************/

extern P_GR_AXES
gr_axesp_from_seridx(
    P_GR_CHART    cp,
    GR_SERIES_IX seridx)
{
    GR_AXES_NO axes;

    if(seridx < cp->axes[0].series.end_idx)
        axes = 0;
    else
        axes = 1;

    return(&cp->axes[axes]);
}

extern P_GR_AXIS
gr_axisp_from_external(
    P_GR_CHART    cp,
    GR_AXES_NO   eaxes)
{
    GR_AXES_NO axes;
    GR_AXIS_NO axis;

    axis = gr_axes_ix_from_external(cp, eaxes, &axes);

    return(&cp->axes[axes].axis[axis]);
}

/* allow for logs of numbers (especially those not in same base as FP representation) becoming imprecise */
#define LOG_SIG_EPS 1E-8

static F64
splitlognum(
    const F64 * logval,
    P_F64 exponent /*out*/)
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

static S32
gr_numtopowstr(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_        PC_F64 evalue,
    _In_        PC_F64 base)
{
    F64 value = *evalue;
    F64 lnz, mantissa, exponent;

    if(value == 0.0)
        {
        void_strkpy(buffer, elemof_buffer, "0");
        return(1);
        }

    if(value < 0.0)
        {
        value = fabs(value);
        *buffer++ = '-';
        }

    lnz = log(value) / log(*base);

    mantissa = splitlognum(&lnz, &exponent);

    if(mantissa != 0.0) /* log(1.0) == 0.0 */
        {
        mantissa = pow(*base, mantissa);

        /* use ANSI 'times' character */
        (void) xsnprintf(buffer, elemof_buffer, "%g" "\xD7" "%g" "^" "%g", mantissa, *base, exponent);
        }
    else
        (void) xsnprintf(buffer, elemof_buffer,             "%g" "^" "%g",           *base, exponent);

    return(1);
}

/******************************************************************************
*
* helper routines for looping along axes for grids, labels and ticks
*
******************************************************************************/

typedef struct _gr_axis_iterator
{
    F64 iter;
    F64 step;

    F64 mantissa; /* for log scaling */
    F64 exponent;
    F64 base;
}
GR_AXIS_ITERATOR, * P_GR_AXIS_ITERATOR;

static void
gr_axis_iterator_renormalise_log(
    /*inout*/ P_GR_AXIS_ITERATOR iterp)
{
    F64 lna, lnz;

    lna = log(iterp->base);
    lnz = log(iterp->iter);
    lnz = lnz / lna;

    lnz = splitlognum(&lnz, &iterp->exponent);

    iterp->mantissa = pow(iterp->base, lnz);

    iterp->iter = iterp->mantissa * pow(iterp->base, iterp->exponent);
}

static S32
gr_axis_iterator_first(
    PC_GR_AXIS p_axis,
    S32 major,
    P_GR_AXIS_ITERATOR iterp /*out*/)
{
    IGNOREPARM(major);

    iterp->iter = p_axis->current.min;

    if(p_axis->bits.log_scale)
        {
        iterp->base = p_axis->bits.log_base;

        gr_axis_iterator_renormalise_log(iterp);
        }
    else
        {
        iterp->base = 10.0; /* in case we use pow_label when not log_scale */
        }

    return(1);
}

static S32
gr_axis_iterator_next(
    PC_GR_AXIS p_axis,
    S32 major,
    P_GR_AXIS_ITERATOR iterp /*inout*/)
{
    if(p_axis->bits.log_scale)
        {
        if(major)
            iterp->mantissa *= iterp->step; /* use step as multiplier */
        else
            iterp->mantissa += iterp->step; /* use step as adder */

        iterp->iter = iterp->mantissa * pow(iterp->base, iterp->exponent);

        if(iterp->mantissa >= iterp->base)
            gr_axis_iterator_renormalise_log(iterp);
        }
    else
        {
        iterp->iter += iterp->step;

        /* SKS after 4.12 26mar92 - correct for very small FP rounding errors (wouldn't loop up to 3.0 in 0.1 steps) */
        if(fabs(iterp->iter - p_axis->current.max) / iterp->step < 0.000244140625) /* 2^-12 */
            {
            iterp->iter = p_axis->current.max;
            return(1);
            }
        }

    return(iterp->iter <= p_axis->current.max);
}

/* an empirically derived aesthetic ratio. argue with SKS */
#define TICKLEN_FRAC 64

/******************************************************************************
*
* category axis
*
******************************************************************************/

/*
category axis - major and minor grids
*/

static S32
gr_axis_addin_category_grids(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    S32              front,
    BOOL             major)
{
    const GR_AXES_NO axes   = 0;
    const GR_AXIS_NO axis   = X_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   gridStart;
    GR_POINT_NO      point;
    S32              step;
    GR_LINESTYLE     linestyle;
    S32              draw_main;
    S32              doing_line_chart = (cp->axes[0].charttype == GR_CHARTTYPE_LINE);
    S32              res;

    if(!ticksp->bits.grid)
        return(1);

    step = (S32) ticksp->current;
    if(!step)
        return(1);

    if(front)
        {
        switch(p_axis->bits.arf)
            {
            default:
            case GR_AXIS_POSITION_AUTO:
                return(1);

            case GR_AXIS_POSITION_FRONT:
                draw_main = 1;
                break;

            case GR_AXIS_POSITION_REAR:
                return(1);
            }
        }
    else
        {
#if 1
        /* I now argue that bringing the axis to the front
         * shouldn't affect rear grid drawing state
        */
        draw_main = 1;
#else
        switch(p_axis->bits.arf)
            {
            default:
            case GR_AXIS_POSITION_AUTO:
                draw_main = 1;
                break;

            case GR_AXIS_POSITION_FRONT:
                return(1);

            case GR_AXIS_POSITION_REAR:
                draw_main = 1;
                break;
            }
#endif
        }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major);

    if((res = gr_chart_group_new(cp, &gridStart, &id)) < 0)
        return(res);

    gr_chart_objid_linestyle_query(cp, &id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point  = 0;
        point <= total_n_points;
        point += step)
        {
        GR_COORD xpos, ypos;
        GR_BOX line_box;

        xpos  = gr_categ_pos(cp, point);
        ypos  = 0;

        xpos += cp->plotarea.posn.x;
        ypos += cp->plotarea.posn.y;

        /* line charts require category ticks centering in slot or so they say */
        if(doing_line_chart)
            xpos += cp->barlinech.cache.cat_group_width / 2;

        if(draw_main)
            {
            /* vertical grid line up the entire y span */
            line_box.x0 = xpos;
            line_box.y0 = ypos;

            line_box.x1 = line_box.x0;
            line_box.y1 = line_box.y0 + cp->plotarea.size.y;

            /* map together to front or back of 3d world */
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);
            gr_map_point((P_GR_POINT) &line_box.x1, cp, fob);

            if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                break;

            if(cp->d3.bits.use && front)
                {
                /* diagonal grid lines across the top too ONLY DURING THE FRONT PHASE */
                line_box.x0 = xpos;
                line_box.y0 = ypos + cp->plotarea.size.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                    break;
                }
            }

        if(cp->d3.bits.use && !front)
            {
            /* diagonal grid lines across the midplane and floor too ONLY DURING THE REAR PHASE */
            line_box.x0 = xpos;
            line_box.y0 = ypos;

            /* one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                break;

            if((axis_ypos != 0) && (axis_ypos != cp->plotarea.size.y))
                {
                /* grid line across middle (simply shift) */
                line_box.y0 += axis_ypos;
                line_box.y1 += axis_ypos;

                if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                    break;
                }
            }
        }

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &gridStart);

    return(1);
}

static S32
gr_axis_ticksizes(
    P_GR_CHART       cp,
    PC_GR_AXIS_TICKS ticksp,
    GR_COORD *      p_top    /*out*/, /* or p_right */
    GR_COORD *      p_bottom /*out*/) /* or p_left  */
{
    GR_COORD ticksize;

    ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    switch(ticksp->bits.tick)
        {
        case GR_AXIS_TICK_POSITION_NONE:
            *p_top    = 0;
            *p_bottom = 0;
            return(0);

        case GR_AXIS_TICK_POSITION_HALF_TOP:
            *p_top    = ticksize;
            *p_bottom = 0;
            break;

        case GR_AXIS_TICK_POSITION_HALF_BOTTOM:
            *p_top    = 0;
            *p_bottom = ticksize;
            break;

        default:
            assert(0);
        case GR_AXIS_TICK_POSITION_FULL:
            *p_top    = ticksize;
            *p_bottom = ticksize;
            break;
        }

    return(1);
}

/*
category axis - labels
*/

static S32
gr_axis_addin_category_labels(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    S32              front)
{
/*  const S32        major  = 1; */
    const GR_AXES_NO axes   = 0;
    const GR_AXIS_NO axis   = X_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET   labelStart;
    GR_POINT_NO      point;
    S32              step;
    GR_COORD         ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    GR_MILLIPOINT    gwidth_mp;
    GR_MILLIPOINT    available_width_mp;
    GR_COORD         available_width_px;
    S32              res;

    step = (S32) ticksp->current;
    if(!step)
        return(1);

    if((res = gr_chart_group_new(cp, &labelStart, p_id)) < 0)
        return(res);

    gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom);

    gr_chart_objid_textstyle_query(cp, p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    gwidth_mp = cp->barlinech.cache.cat_group_width * GR_MILLIPOINTS_PER_PIXIT;

    available_width_px = (cp->barlinech.cache.cat_group_width * step); /* text covers 1..step categories */
    available_width_mp = available_width_px * GR_MILLIPOINTS_PER_PIXIT;

    for(point  = 0;
        point <  total_n_points; /* not <= as never have category label for end tick! */
        point += step)
        {
        GR_COORD xpos;
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        xpos = gr_categ_pos(cp, point);

        text_box.x0  = xpos;
        text_box.y0  = axis_ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        gr_travel_categ_label(cp, point, &cv);

        if(!strlen(cv.data.text))
            continue;

        swidth_mp = gr_font_stringwidth(f, cv.data.text);

        /* always centre within one cat group else left justify */
        if(swidth_mp < gwidth_mp)
            {
            text_box.x0 += (gwidth_mp - swidth_mp) / GR_MILLIPOINTS_PER_PIXIT / 2;
            }
        else
            {
            /* truncate the little bugger to AVAILABLE WIDTH */
            swidth_mp = available_width_mp;

            gr_font_truncate(f, cv.data.text, (int *) &swidth_mp);
            }

        swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

        switch(p_axis->bits.lzr)
            {
            case GR_AXIS_POSITION_TOP:
                text_box.y0 += (3 * ticksize_top) / 2;
                text_box.y0 += (1 * textstyle.height) / 4; /* st. descenders don't crash into ticks */
                break;

            default:
                text_box.y0 -= (3 * ticksize_bottom) / 2;
                text_box.y0 -= (3 * textstyle.height) / 4; /* st. ascenders don't crash into ticks */
                break;
            }

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &text_box.x0, cp, fob);

        text_box.x1 = text_box.x0 + available_width_mp; /* text covers 1..step categories */
        text_box.y1 = text_box.y0 + textstyle.height;

        if((res = gr_diag_text_new(p_gr_diag, NULL, *p_id, &text_box, cv.data.text, &textstyle)) < 0)
            break;
        }

    gr_riscdiag_font_dispose(&f);

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &labelStart);

    return(1);
}

/*
category axis - major and minor ticks
*/

static S32
gr_axis_addin_category_ticks(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    S32              front,
    S32              major)
{
    const GR_AXES_NO axes   = 0;
    const GR_AXIS_NO axis   = X_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   tickStart;
    GR_POINT_NO      point;
    S32              step;
    GR_COORD         ticksize, ticksize_top, ticksize_bottom;
    GR_LINESTYLE     linestyle;
    S32              doing_line_chart = (cp->axes[0].charttype == GR_CHARTTYPE_LINE);
    S32              res;

    step = (S32) ticksp->current;
    if(!step)
        return(1);

    if(!gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom))
        return(1);

    if(!major)
        {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
        }

    axis_ypos -= ticksize_bottom;
    ticksize   = ticksize_top + ticksize_bottom;

    id = *p_id;

    gr_chart_objid_from_axis_tick(&id, major);

    if((res = gr_chart_group_new(cp, &tickStart, &id)) < 0)
        return(res);

    gr_chart_objid_linestyle_query(cp, &id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point  = 0;
        point <= total_n_points;
        point += step)
        {
        GR_COORD xpos;
        GR_BOX line_box;

        xpos = gr_categ_pos(cp, point);

        /* line charts require category ticks centering in slot or so they say */
        if(doing_line_chart)
            xpos += cp->barlinech.cache.cat_group_width / 2;

        line_box.x0  = xpos;
        line_box.y0  = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + ticksize;

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            break;
        }

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &tickStart);

    return(1);
}

/******************************************************************************
*
* category axis
*
******************************************************************************/

extern S32
gr_axis_addin_category(
    P_GR_CHART cp,
    GR_POINT_NO total_n_points,
    S32 front)
{
    const GR_AXES_NO axes  = 0;
    const GR_AXIS_NO axis  = X_AXIS;
    PC_GR_AXIS       p_axis = &cp->axes[axes].axis[axis];
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob   = front ? 0 : cp->cache.n_contrib_series;
    GR_DIAG_OFFSET   axisStart;
    GR_CHART_OBJID   id;
    GR_COORD         axis_ypos;
    S32              draw_main;
    S32              res;

    switch(p_axis->bits.lzr)
        {
        case GR_AXIS_POSITION_TOP:
            axis_ypos = cp->plotarea.size.y;
            break;

        case GR_AXIS_POSITION_ZERO:
            axis_ypos = (GR_COORD) ((F64) cp->plotarea.size.y *
                                    cp->axes[axes].axis[Y_AXIS /*NB*/].zero_frac);
            break;

        default:
            assert(0);
        case GR_AXIS_POSITION_BOTTOM:
            axis_ypos = 0;
            break;
        }

    switch(p_axis->bits.arf)
        {
        case GR_AXIS_POSITION_FRONT:
        atfront:;
            draw_main = (front == 1);
            break;

        default:
        case GR_AXIS_POSITION_AUTO:
            if(cp->d3.bits.use)
                if(axis_ypos != cp->plotarea.size.y)
                    goto atfront;

        case GR_AXIS_POSITION_REAR:
            draw_main = (front == 0);
            break;
        }

    gr_chart_objid_from_axes(cp, axes, axis, &id);

    if((res = gr_chart_group_new(cp, &axisStart, &id)) < 0)
        return(res);

    /* minor grids */
    if((res = gr_axis_addin_category_grids(cp, &id, total_n_points, axis_ypos, front, 0)) < 0)
        return(res);

    /* major grids */
    if((res = gr_axis_addin_category_grids(cp, &id, total_n_points, axis_ypos, front, 1)) < 0)
        return(res);

    if(draw_main)
        {
        GR_BOX line_box;
        GR_LINESTYLE linestyle;

        /* axis line */
        line_box.x0  = 0;
        line_box.y0  = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0 + cp->plotarea.size.x;
        line_box.y1  = line_box.y0;

        gr_chart_objid_linestyle_query(cp, &id, &linestyle);

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            return(res);

        /* minor ticks */
        if((res = gr_axis_addin_category_ticks(cp, &id, total_n_points, axis_ypos, front, 0)) < 0)
            return(res);

        /* major ticks */
        if((res = gr_axis_addin_category_ticks(cp, &id, total_n_points, axis_ypos, front, 1)) < 0)
            return(res);

        /* labels */
        if((res = gr_axis_addin_category_labels(cp, &id, total_n_points, axis_ypos, front)) < 0)
            return(res);
        }

    gr_diag_group_end(p_gr_diag, &axisStart);

    return(res);
}

/******************************************************************************
*
* value X & Y axes
*
******************************************************************************/

/*
value X axis - major and minor grids
*/

static S32
gr_axis_addin_value_grids_x(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_ypos,
    S32              front,
    S32              major)
{
    const GR_AXIS_NO axis   = X_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_CHART_OBJID   id;
    GR_AXIS_ITERATOR iterator;
    GR_DIAG_OFFSET   gridStart;
    GR_LINESTYLE     linestyle;
    S32              res, loop;

    IGNOREPARM_InVal_(axis_ypos);

    if(!ticksp->bits.grid)
        return(1);

    iterator.step = ticksp->current;
    if(!iterator.step)
        return(1);

    if(front)
        {
        switch(p_axis->bits.arf)
            {
            case GR_AXIS_POSITION_FRONT:
                break;

            default:
            case GR_AXIS_POSITION_AUTO:
            case GR_AXIS_POSITION_REAR:
                return(1);
            }
        }
    else
        {
        switch(p_axis->bits.arf)
            {
            case GR_AXIS_POSITION_FRONT:
                return(1);

            default:
            case GR_AXIS_POSITION_AUTO:
            case GR_AXIS_POSITION_REAR:
                break;
            }
        }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major);

    if((res = gr_chart_group_new(cp, &gridStart, &id)) < 0)
        return(res);

    gr_chart_objid_linestyle_query(cp, &id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &iterator))
        {
        GR_COORD xpos;
        GR_BOX line_box;

        xpos = gr_value_pos(cp, axes, axis, &iterator.iter);

        /* vertical grid line across entire y span */
        line_box.x0  = xpos;
        line_box.y0  = 0;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + cp->plotarea.size.y;

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            break;
        }

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &gridStart);

    return(1);
}

/*
value Y axis - major and minor grids
*/

static S32
gr_axis_addin_value_grids_y(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_xpos,
    S32              front,
    S32              major)
{
    const GR_AXIS_NO axis   = Y_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   gridStart;
    GR_AXIS_ITERATOR iterator;
    GR_LINESTYLE     linestyle;
    S32              draw_main;
    S32              res, loop;

    if(!ticksp->bits.grid)
        return(1);

    iterator.step = ticksp->current;
    if(!iterator.step)
        return(1);

    if(front)
        {
        switch(p_axis->bits.arf)
            {
            default:
            case GR_AXIS_POSITION_AUTO:
                return(1);

            case GR_AXIS_POSITION_FRONT:
                draw_main = 1;
                break;

            case GR_AXIS_POSITION_REAR:
                return(1);
            }
        }
    else
        {
#if 1
        /* I now argue that bringing the axis to the front
         * shouldn't affect rear grid drawing state
        */
        draw_main = 1;
#else
        switch(p_axis->bits.arf)
            {
            default:
            case GR_AXIS_POSITION_AUTO:
                draw_main = 1;
                break;

            case GR_AXIS_POSITION_FRONT:
                return(1);

            case GR_AXIS_POSITION_REAR:
                draw_main = 1;
                break;
            }
#endif
        }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major);

    if((res = gr_chart_group_new(cp, &gridStart, &id)) < 0)
        return(res);

    gr_chart_objid_linestyle_query(cp, &id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &iterator))
        {
        GR_COORD ypos;
        GR_BOX line_box;

        ypos = gr_value_pos(cp, axes, axis, &iterator.iter);

        if(draw_main)
            {
            /* horizontal grid line across entire x span */
            line_box.x0  = 0;
            line_box.y0  = ypos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            line_box.x1  = line_box.x0 + cp->plotarea.size.x;
            line_box.y1  = line_box.y0;

            /* map together to front or back of 3d world */
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);
            gr_map_point((P_GR_POINT) &line_box.x1, cp, fob);

            if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                break;

            if(cp->d3.bits.use && front)
                {
                /* grid lines across front of right hand side ONLY DURING THE FRONT PHASE **/

                line_box.x0  = cp->plotarea.size.x;
                line_box.y0  = ypos;

                line_box.x0 += cp->plotarea.posn.x;
                line_box.y0 += cp->plotarea.posn.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                    break;
                }
            }

        if(cp->d3.bits.use && !front)
            {
            /* grid lines across midplane and side wall ONLY DURING THE REAR PHASE */
            line_box.x0  = 0;
            line_box.y0  = ypos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            /* put one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                break;

            if((axis_xpos != 0) && (axis_xpos != cp->plotarea.size.x))
                {
                /* grid line across middle (simply shift) */
                line_box.x0 += axis_xpos;
                line_box.x1 += axis_xpos;

                if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
                    break;
                }
            }
        }

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &gridStart);

    return(1);
}

/*
value X axis - labels next to ticks
*/

static S32
gr_axis_addin_value_labels_x(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_ypos,
    S32              front)
{
    const S32        major  = 1;
    const GR_AXIS_NO axis   = X_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_DIAG_OFFSET   labelStart;
    GR_AXIS_ITERATOR iterator;
    GR_COORD         major_width_px;
    GR_COORD         last_x1 = S32_MIN;
    GR_COORD         ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    S32              res, loop;

    IGNOREPARM_InVal_(front);

    iterator.step = ticksp->current;
    if(!iterator.step)
        return(1);

    major_width_px = (GR_COORD) ((F64) p_axis->current_frac * cp->plotarea.size.x);

    if((res = gr_chart_group_new(cp, &labelStart, p_id)) < 0)
        return(res);

    gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom);

    gr_chart_objid_textstyle_query(cp, p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &iterator))
        {
        GR_COORD xpos;
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        xpos = gr_value_pos(cp, axes, axis, &iterator.iter);

        text_box.x0  = xpos;
        text_box.y0  = axis_ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        if(p_axis->bits.pow_label)
            gr_numtopowstr(cv.data.text, elemof32(cv.data.text),
                           &iterator.iter,
                           &iterator.base);
        else
            {
            S32 decimals = ticksp->bits.decimals;
            S32 eformat  = (fabs(iterator.iter) >= U32_MAX);

            gr_numtostr(cv.data.text, elemof32(cv.data.text),
                        &iterator.iter,
                        eformat,
                        decimals,
                        NULLCH /* dp_ch */,
                        ','    /* ths_ch */);
            }

        swidth_mp = gr_font_stringwidth(f, cv.data.text);

        if(!swidth_mp)
            continue;

        swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

        /* always centre within one major group at tick point */
        text_box.x0 -= swidth_px / 2;

        if(text_box.x0 < last_x1)
            continue;

        switch(p_axis->bits.lzr)
            {
            case GR_AXIS_POSITION_TOP:
                text_box.y0 += (3 * ticksize_top) / 2;
                text_box.y0 += (1 * textstyle.height) / 4; /* st. descenders don't crash into ticks */
                break;

            default:
                text_box.y0 -= (3 * ticksize_bottom) / 2;
                text_box.y0 -= (3 * textstyle.height) / 4; /* st. ascenders don't crash into ticks */
                break;
            }

        text_box.x1 = text_box.x0 + swidth_px;
        text_box.y1 = text_box.y0 + textstyle.height;

        last_x1 = text_box.x1 + (textstyle.height / 8);

        if((res = gr_diag_text_new(p_gr_diag, NULL, *p_id, &text_box, cv.data.text, &textstyle)) < 0)
            break;
        }

    gr_riscdiag_font_dispose(&f);

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &labelStart);

    return(1);
}

/*
value Y axis - labels next to ticks
*/

static S32
gr_axis_addin_value_labels_y(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_xpos,
    S32              front)
{
    const S32        major  = 1;
    const GR_AXIS_NO axis   = Y_AXIS;
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET   labelStart;
    GR_AXIS_ITERATOR iterator;
    GR_COORD         last_y1 = S32_MIN;
    GR_COORD         ticksize_left, ticksize_right, spacewidth_px;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    S32              res, loop;

    iterator.step = ticksp->current;
    if(!iterator.step)
        return(1);

    gr_axis_ticksizes(cp, ticksp, &ticksize_right, &ticksize_left); /* NB. order! */

    if((res = gr_chart_group_new(cp, &labelStart, p_id)) < 0)
        return(res);

    gr_chart_objid_textstyle_query(cp, p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    /* allow room for a space at whichever side of the tick we are adding at */
    {
    GR_MILLIPOINT swidth_mp = gr_font_stringwidth(f, " ");

    spacewidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;
    }

    for(loop = gr_axis_iterator_first(p_axis, major, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &iterator))
        {
        GR_BOX text_box;
        GR_COORD ypos;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        ypos = gr_value_pos(cp, axes, axis, &iterator.iter);

        text_box.x0  = axis_xpos;
        text_box.y0  = ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        text_box.y0 -= (1 * textstyle.height) / 4; /* a vague attempt to centre then number on the tick */

        if(text_box.y0 < last_y1)
            continue;

        if(p_axis->bits.pow_label)
            gr_numtopowstr(cv.data.text, elemof32(cv.data.text),
                           &iterator.iter,
                           &iterator.base);
        else
            {
            S32 decimals = ticksp->bits.decimals;
            S32 eformat  = (fabs(iterator.iter) >= U32_MAX);

            gr_numtostr(cv.data.text, elemof32(cv.data.text),
                        &iterator.iter,
                        eformat,
                        decimals,
                        NULLCH /* dp_ch */,
                        ','    /* ths_ch */);
            }

        swidth_mp = gr_font_stringwidth(f, cv.data.text);

        if(!swidth_mp)
            continue;

        swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

        /* if axis at right then left just else right just */
        if(p_axis->bits.lzr == GR_AXIS_POSITION_RIGHT)
            {
            text_box.x0 += ticksize_right;
            text_box.x0 += spacewidth_px;
            }
        else
            {
            text_box.x0 -= ticksize_left;
            text_box.x0 -= spacewidth_px;
            text_box.x0 -= swidth_px;
            }

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &text_box.x0, cp, fob);

        text_box.x1 = text_box.x0 + swidth_px;
        text_box.y1 = text_box.y0 + textstyle.height;

        /* use standard 120% line spacing as guide */
        last_y1 = text_box.y1 + (2 * textstyle.height) / 10;

        if((res = gr_diag_text_new(p_gr_diag, NULL, *p_id, &text_box, cv.data.text, &textstyle)) < 0)
            break;
        }

    gr_riscdiag_font_dispose(&f);

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &labelStart);

    return(1);
}

/*
value X & Y axis - major and minor ticks
*/

static S32
gr_axis_addin_value_ticks(
    P_GR_CHART        cp,
    GR_AXIS_NO       axes,
    GR_AXIS_NO       axis,
    P_GR_CHART_OBJID p_id /*const*/,
    P_GR_POINT axis_pos,
    S32              front,
    S32              major,
    GR_COORD         ticksize,
    S32              doing_x)
{
    PC_GR_AXIS       p_axis  = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob    = (front ? 0 : cp->cache.n_contrib_series);
    GR_CHART_OBJID   id;
    GR_AXIS_ITERATOR iterator;
    GR_DIAG_OFFSET   tickStart;
    GR_LINESTYLE     linestyle;
    S32              res, loop;

    iterator.step = ticksp->current;
    if(!iterator.step)
        return(1);

    id = *p_id;

    gr_chart_objid_from_axis_tick(&id, major);

    if((res = gr_chart_group_new(cp, &tickStart, &id)) < 0)
        return(res);

    gr_chart_objid_linestyle_query(cp, &id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &iterator))
        {
        GR_COORD pos;
        GR_BOX line_box;

        pos = gr_value_pos(cp, axes, axis, &iterator.iter);

        line_box.x0  = doing_x ? pos         : axis_pos->x;
        line_box.y0  = doing_x ? axis_pos->y : pos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0;

        if(doing_x)
            /* tick is vertical */
            line_box.y1 += ticksize;
        else
            /* tick is horizontal */
            line_box.x1 += ticksize;

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            break;
        }

    if(res < 0)
        return(res);

    gr_diag_group_end(p_gr_diag, &tickStart);

    return(1);
}

/*
value X axis - major and minor ticks
*/

static S32
gr_axis_addin_value_ticks_x(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_ypos,
    S32              front,
    S32              major)
{
    const GR_AXIS_NO axis = X_AXIS;
    PC_GR_AXIS p_axis = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_COORD ticksize, ticksize_top, ticksize_bottom;

    ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    if(!gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom))
        return(1);

    if(!major)
        {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
        }

    axis_pos.y = axis_ypos - ticksize_bottom;
    ticksize   = ticksize_top + ticksize_bottom;

    return(gr_axis_addin_value_ticks(cp, axes, axis, p_id, &axis_pos, front, major, ticksize, TRUE /*doing_x*/));
}

/*
value Y axis - major and minor ticks
*/

static S32
gr_axis_addin_value_ticks_y(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_NO       axes,
    GR_COORD         axis_xpos,
    S32              front,
    S32              major)
{
    const GR_AXIS_NO axis = Y_AXIS;
    PC_GR_AXIS p_axis = &cp->axes[axes].axis[axis];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_COORD ticksize, ticksize_left, ticksize_right;

    if(!gr_axis_ticksizes(cp, ticksp, &ticksize_right, &ticksize_left)) /* NB. order! */
        return(1);

    if(!major)
        {
        ticksize_left  /= 2;
        ticksize_right /= 2;
        }

    axis_pos.x = axis_xpos - ticksize_left;
    ticksize   = ticksize_left + ticksize_right;

    return(gr_axis_addin_value_ticks(cp, axes, axis, p_id, &axis_pos, front, major, ticksize, FALSE /*doing_x*/));
}

/******************************************************************************
*
* value X axis
*
******************************************************************************/

extern S32
gr_axis_addin_value_x(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    S32 front)
{
    const GR_AXIS_NO axis  = X_AXIS;
    PC_GR_AXIS       p_axis = &cp->axes[axes].axis[axis];
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_CHART_OBJID   id;
    GR_COORD         axis_ypos;
    S32              draw_main;
    GR_DIAG_OFFSET   axisStart;
    S32              res;

    switch(p_axis->bits.lzr)
        {
        case GR_AXIS_POSITION_TOP:
            axis_ypos = cp->plotarea.size.y;
            break;

        case GR_AXIS_POSITION_ZERO:
            axis_ypos = (GR_COORD) ((F64) cp->plotarea.size.y *
                                    cp->axes[axes].axis[Y_AXIS /*NB*/].zero_frac);
            break;

        default:
            assert(0);
        case GR_AXIS_POSITION_LEFT:
            axis_ypos = 0;
            break;
        }

    /* NB. in 2d AUTO and REAR synonymous */

    /* no 3d to consider here */

    switch(p_axis->bits.arf)
        {
        case GR_AXIS_POSITION_FRONT:
            draw_main = front;
            break;

        default:
        case GR_AXIS_POSITION_AUTO:
        case GR_AXIS_POSITION_REAR:
            draw_main = !front;
            break;
        }

    gr_chart_objid_from_axes(cp, axes, axis, &id);

    if((res = gr_chart_group_new(cp, &axisStart, &id)) < 0)
        return(res);

    /* minor ticks */
    if((res = gr_axis_addin_value_grids_x(cp, &id, axes, axis_ypos, front, 0)) < 0)
        return(res);

    /* major grids */
    if((res = gr_axis_addin_value_grids_x(cp, &id, axes, axis_ypos, front, 1)) < 0)
        return(res);

    if(draw_main)
        {
        /* horizontal main axis line */
        GR_BOX line_box;
        GR_LINESTYLE linestyle;

        line_box.x0 = 0;
        line_box.y0 = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        line_box.x1 = line_box.x0 + cp->plotarea.size.x;
        line_box.y1 = line_box.y0;

        gr_chart_objid_linestyle_query(cp, &id, &linestyle);

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            return(res);

        /* minor ticks */
        if((res = gr_axis_addin_value_ticks_x(cp, &id, axes, axis_ypos, front, 0)) < 0)
            return(res);

        /* major ticks */
        if((res = gr_axis_addin_value_ticks_x(cp, &id, axes, axis_ypos, front, 1)) < 0)
            return(res);

        /* labels */
        if((res = gr_axis_addin_value_labels_x(cp, &id, axes, axis_ypos, front)) < 0)
            return(res);
        }

    gr_diag_group_end(p_gr_diag, &axisStart);

    return(1);
}

/******************************************************************************
*
* value Y axis
*
******************************************************************************/

extern S32
gr_axis_addin_value_y(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    S32 front)
{
    const GR_AXIS_NO axis  = Y_AXIS;
    PC_GR_AXIS       p_axis = &cp->axes[axes].axis[axis];
    P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_SERIES_NO     fob   = (front ? 0 : cp->cache.n_contrib_series);
    GR_CHART_OBJID   id;
    GR_COORD         axis_xpos;
    GR_DIAG_OFFSET   axisStart;
    S32              draw_main;
    S32              res;

    switch(p_axis->bits.lzr)
        {
        case GR_AXIS_POSITION_RIGHT:
            axis_xpos = cp->plotarea.size.x;
            break;

        case GR_AXIS_POSITION_ZERO:
            axis_xpos = (GR_COORD) ((F64) cp->plotarea.size.x *
                                    cp->axes[axes].axis[X_AXIS /*NB*/].zero_frac);
            break;

        default:
            assert(0);
        case GR_AXIS_POSITION_LEFT:
            axis_xpos = 0;
            break;
        }

    /* NB. in 2d AUTO and REAR synonymous */

    switch(p_axis->bits.arf)
        {
        case GR_AXIS_POSITION_FRONT:
        atfront:;
            draw_main = (front == 1);
            break;

        default:
        case GR_AXIS_POSITION_AUTO:
            if(cp->d3.bits.use)
                if(axis_xpos != cp->plotarea.size.x)
                    goto atfront;

        case GR_AXIS_POSITION_REAR:
            draw_main = (front == 0);
            break;
        }

    gr_chart_objid_from_axes(cp, axes, axis, &id);

    if((res = gr_chart_group_new(cp, &axisStart, &id)) < 0)
        return(res);

    /* minor ticks */
    if((res = gr_axis_addin_value_grids_y(cp, &id, axes, axis_xpos, front, 0)) < 0)
        return(res);

    /* major grids */
    if((res = gr_axis_addin_value_grids_y(cp, &id, axes, axis_xpos, front, 1)) < 0)
        return(res);

    if(draw_main)
        {
        /* axis line */
        GR_BOX line_box;
        GR_LINESTYLE linestyle;

        line_box.x0 = axis_xpos;
        line_box.y0 = 0;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3d world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1 = line_box.x0;
        line_box.y1 = line_box.y0 + cp->plotarea.size.y;

        gr_chart_objid_linestyle_query(cp, &id, &linestyle);

        if((res = gr_diag_line_new(p_gr_diag, NULL, id, &line_box, &linestyle)) < 0)
            return(res);

        /* minor ticks */
        if((res = gr_axis_addin_value_ticks_y(cp, &id, axes, axis_xpos, front, 0)) < 0)
            return(res);

        /* major ticks */
        if((res = gr_axis_addin_value_ticks_y(cp, &id, axes, axis_xpos, front, 1)) < 0)
            return(res);

        /* labels */
        if((res = gr_axis_addin_value_labels_y(cp, &id, axes, axis_xpos, front)) < 0)
            return(res);
        }

    gr_diag_group_end(p_gr_diag, &axisStart);

    return(1);
}

/******************************************************************************
*
* edit selected category axis attributes
*
******************************************************************************/

typedef struct _gr_chartedit_cat_axis_state
{
    wimp_w w;

    struct _gr_chartedit_cat_axis_state_axis
        {
        UBF lzr : GR_AXIS_POSITION_LZR_BITS;
        UBF arf : GR_AXIS_POSITION_ARF_BITS;
        }
    axis;

    struct _gr_chartedit_cat_axis_state_ticks
        {
        S32 val;
        S32 delta;
        S32 manual;
        S32 grid;

        UBF tick : GR_AXIS_TICK_SHAPE_BITS;
        }
    major, minor;
}
gr_chartedit_cat_axis_state;

static const UBF /*GR_AXIS_POSITION_LZR*/
gr_chartedit_selection_axis_lzrs[] =
{
    GR_AXIS_POSITION_LEFT,
    GR_AXIS_POSITION_ZERO,
    GR_AXIS_POSITION_RIGHT
};

#define gr_chartedit_selection_cat_axis_lzrs gr_chartedit_selection_axis_lzrs

static const UBF /*GR_AXIS_POSITION_ARF*/
gr_chartedit_selection_axis_arfs[] =
{
    GR_AXIS_POSITION_FRONT,
    GR_AXIS_POSITION_AUTO,
    GR_AXIS_POSITION_REAR
};

#define gr_chartedit_selection_cat_axis_arfs gr_chartedit_selection_axis_arfs

static const UBF /*GR_AXIS_TICK_SHAPE*/
gr_chartedit_selection_axis_ticks[] =
{
    GR_AXIS_TICK_POSITION_NONE,
    GR_AXIS_TICK_POSITION_FULL,
    GR_AXIS_TICK_POSITION_HALF_RIGHT, /* == BOTTOM */
    GR_AXIS_TICK_POSITION_HALF_LEFT   /* == TOP */
};

#define gr_chartedit_selection_cat_axis_ticks gr_chartedit_selection_axis_ticks

/* encode icons from current structure (not necessarily reflected in chart structure) */

static void
gr_chartedit_selection_cat_axis_encode(
    const gr_chartedit_cat_axis_state * state)
{
    const struct _gr_chartedit_cat_axis_state_ticks * smp;
    wimp_w w = state->w;
    S32 i;

    /* lzr */
    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_lzrs); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM,
                     (state->axis.lzr == gr_chartedit_selection_cat_axis_lzrs[i]));

    /* arf */
    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_arfs); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT,
                 (state->axis.arf == gr_chartedit_selection_cat_axis_arfs[i]));

#if 0
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT);
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_AUTO);
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_REAR);
#endif

    /* major */
    smp = &state->major;

    win_setonoffpair(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_AUTO, !smp->manual);
    win_setint(      w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,   smp->val);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_ticks); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE,
                     (smp->tick == gr_chartedit_selection_cat_axis_ticks[i]));

    /* minor */
    smp = &state->minor;

    win_setonoffpair(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO, !smp->manual);
    win_setint(      w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,   smp->val);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_ticks); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE,
                     (smp->tick == gr_chartedit_selection_cat_axis_ticks[i]));
}

static void
gr_chartedit_selection_cat_axis_process(
    P_GR_CHARTEDITOR cep,
    GR_CHART_OBJID id)
{
    dbox           d;
    char * errorp;
    wimp_w         w;
    dbox_field     f;
    P_GR_CHART      cp;
    S32            ok, persist;
    gr_chartedit_cat_axis_state state;
    GR_AXES_NO     modifying_axes;
    GR_AXIS_NO     modifying_axis;
    P_GR_AXES       p_axes;
    P_GR_AXIS       p_axis;
    P_GR_AXIS_TICKS mmp;
    struct _gr_chartedit_cat_axis_state_ticks * smp;

    cp = gr_chart_cp_from_ch(cep->ch);

    modifying_axis = gr_axes_ix_from_external(cp, id.no, &modifying_axes);

    d = dbox_new_new(GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(GR_CHART_ERR_NOMEM));
        return;
    }

    dbox_show(d);

    state.w = w = dbox_syshandle(d);

    win_settitle_c(w, gr_chart_object_name_from_id_quick(&id));

    do  {
        /* load current settings into structure */
        p_axes = &cp->axes[modifying_axes];
        p_axis = &p_axes->axis[modifying_axis];

        state.axis.lzr = p_axis->bits.lzr;
        state.axis.arf = p_axis->bits.arf;

        mmp         = &p_axis->major;
        smp         = &state.major;
        smp->manual = mmp->bits.manual;
        smp->val    = (S32) (mmp->bits.manual ? mmp->punter : mmp->current);
        smp->grid   = mmp->bits.grid;
        smp->tick   = mmp->bits.tick;

        smp->delta  = (S32) gr_lin_major(smp->val);
        if(!smp->delta)
            smp->delta = 1;

        mmp         = &p_axis->minor;
        smp         = &state.minor;
        smp->manual = mmp->bits.manual;
        smp->val    = (S32) (mmp->bits.manual ? mmp->punter : mmp->current);
        smp->grid   = mmp->bits.grid;
        smp->tick   = mmp->bits.tick;

        smp->delta  = (S32) gr_lin_major(smp->val);
        if(!smp->delta)
            smp->delta = 1;

        for(;;)
            {
            gr_chartedit_selection_cat_axis_encode(&state);

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            /* adjusters all modify but rebuild at end */
            switch(f)
                {
                case dbox_OK:
                    break;

                #ifndef NDEBUG
                /* icons which can be clicked on for state but we don't want assert() to trap */
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ZERO:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_TOP:

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_REAR:

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_FULL:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_TOP:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_BOTTOM:

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_FULL:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_TOP:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_BOTTOM:
                    break;
                #endif

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_MANUAL:
                    smp = &state.major;
                    mmp = &p_axis->major;

                    smp->manual = win_getonoff(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_MANUAL);

                    if(!smp->manual)
                        smp->val = (S32) mmp->current;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID:
                    state.major.grid = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL:
                    smp = &state.minor;
                    mmp = &p_axis->minor;

                    smp->manual = win_getonoff(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL);

                    if(!smp->manual)
                        smp->val = (S32) mmp->current;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID:
                    state.minor.grid = win_getonoff(w, f);
                    break;

                default:
                    smp = &state.major;

                    if(win_bumpint(w, f, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,
                                   &smp->val,
                                   smp->delta,
                                   1, INT_MAX))
                        {
                        state.major.manual = 1;
                        break;
                        }

                    smp = &state.minor;

                    if(win_bumpint(w, f, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,
                                   &smp->val,
                                   smp->delta,
                                   1, state.major.val))
                        {
                        state.minor.manual = 1;
                        break;
                        }

                    assert(0);
                    break;
                }

            /* lzr */

            state.axis.lzr = gr_chartedit_selection_cat_axis_lzrs[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ZERO)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM];

            /* arf */

            state.axis.arf = gr_chartedit_selection_cat_axis_arfs[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_AUTO)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT];

            /* major */
            smp = &state.major;

            win_checkint(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,
                         &smp->val,
                         NULL,
                         1, INT_MAX);

            smp->tick = gr_chartedit_selection_cat_axis_ticks[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE];

            /* minor */
            smp = &state.minor;

            win_checkint(w, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,
                         &smp->val,
                         NULL,
                         1, INT_MAX);

            smp->tick = gr_chartedit_selection_cat_axis_ticks[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE];

            if(f == dbox_OK)
                break;
            }

        ok = (f == dbox_OK);

        if(ok)
            {
            /* set chart from modified structure */
            p_axes = &cp->axes[modifying_axes];
            p_axis = &p_axes->axis[modifying_axis];

            p_axis->bits.lzr = state.axis.lzr;
            p_axis->bits.arf = state.axis.arf;

            mmp                   = &p_axis->major;
            smp                   = &state.major;
            mmp->bits.manual      = smp->manual;
            if(mmp->bits.manual)
                mmp->punter       = (F64) smp->val;
            mmp->bits.tick        = smp->tick;
            mmp->bits.grid        = smp->grid;

            mmp                   = &p_axis->minor;
            smp                   = &state.minor;
            mmp->bits.manual      = smp->manual;
            if(mmp->bits.manual)
                mmp->punter       = (F64) smp->val;
            mmp->bits.tick        = smp->tick;
            mmp->bits.grid        = smp->grid;

            gr_chart_modify_and_rebuild(&cep->ch);
            }

        persist = ok ? dbox_persist() : FALSE;
        }
    while(persist);

    dbox_dispose(&d);
}

/******************************************************************************
*
* edit selected value axis attributes
*
******************************************************************************/

typedef struct _gr_chartedit_axis_state
    {
    wimp_w w;

    struct _gr_chartedit_axis_state_axis
        {
        S32 manual, zero, log_scale, log_scale_modified, pow_label;
        F64 min, max, delta;

        UBF lzr : GR_AXIS_POSITION_LZR_BITS;
        UBF arf : GR_AXIS_POSITION_ARF_BITS;
        }
    axis;

    struct _gr_chartedit_axis_state_ticks
        {
        F64 val, delta;
        S32 manual, grid;

        UBF tick : GR_AXIS_TICK_SHAPE_BITS;
        }
    major, minor;

    struct _gr_chartedit_axis_state_series
        {
        S32 cumulative;
        S32 cumulative_modified;

        S32 point_vary;

        S32 best_fit;

        S32 fill_axis;

        S32 stacked;
        S32 stacked_modified;
        }
    series;
    }
gr_chartedit_axis_state;

static const F64 dbl_min_limit     = -DBL_MAX;
static const F64 dbl_max_limit     = +DBL_MAX;

static const F64 dbl_min_interval_limit = 0.0; /* 0 -> off */
static const F64 dbl_max_interval_limit = +DBL_MAX;

static const F64 dbl_log_min_limit = 0.0; /*+DBL_MIN;*/ /* small number > 0 */

static const F64 dbl_log_major_delta              = +10.0;
static const F64 dbl_log_major_max_interval_limit = +1.0E+307; /* very large multiplier */

static const F64 dbl_log_minor_delta              = +1.0;
static const F64 dbl_log_minor_max_interval_limit = +1.0E+307; /* very large adder */

/* encode icons from current structure (not necessarily reflected in chart structure) */

static void
gr_chartedit_selection_axis_encode(
    const gr_chartedit_axis_state * state)
{
    const struct _gr_chartedit_axis_state_ticks * smp;
    wimp_w w = state->w;
    S32 i;

    /* scale */
    win_setonoffpair(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_AUTO,     !state->axis.manual);
    win_setdouble(   w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,      &state->axis.min, INT_MAX);
    win_setdouble(   w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,      &state->axis.max, INT_MAX);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO,      state->axis.zero);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE, state->axis.log_scale);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL, state->axis.pow_label);

    /* lzr */
    for(i = 0; i < elemof32(gr_chartedit_selection_axis_lzrs); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT,
                     (state->axis.lzr == gr_chartedit_selection_axis_lzrs[i]));

    /* arf */
    for(i = 0; i < elemof32(gr_chartedit_selection_axis_arfs); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT,
                 (state->axis.arf == gr_chartedit_selection_axis_arfs[i]));

#if 0
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT);
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_AUTO);
    (cp->d3.bits.use ? win_unfadefield : win_fadefield) (w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_REAR);
#endif

    /* major */
    smp = &state->major;
    win_setonoffpair(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO, !smp->manual);
    win_setdouble(   w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,  &smp->val, INT_MAX);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_axis_ticks); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE,
                     (smp->tick == gr_chartedit_selection_axis_ticks[i]));

    /* minor */
    smp = &state->minor;
    win_setonoffpair(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO, !smp->manual);
    win_setdouble(   w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,  &smp->val, INT_MAX);
    win_setonoff(    w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_axis_ticks); i++)
        win_setonoff(w, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE,
                     (smp->tick == gr_chartedit_selection_axis_ticks[i]));

    /* series defaults */
    win_setonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE, state->series.cumulative);
    win_setonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY, state->series.point_vary);
    win_setonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT,   state->series.best_fit);
    win_setonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS,  state->series.fill_axis);

    /* series immediate */
    win_setonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED,    state->series.stacked);
}

extern void
gr_chartedit_selection_axis_process(
    P_GR_CHARTEDITOR cep)
{
    dbox           d;
    char * errorp;
    wimp_w         w;
    dbox_field     f;
    P_GR_CHART      cp;
    S32            ok, persist;
    GR_CHART_OBJID id;
    gr_chartedit_axis_state state;
    GR_AXES_NO     modifying_axes;
    GR_AXIS_NO     modifying_axis;
    P_GR_AXES       p_axes;
    P_GR_AXIS       p_axis;
    P_GR_AXIS_TICKS mmp;
    struct _gr_chartedit_axis_state_ticks * smp;

    cp = gr_chart_cp_from_ch(cep->ch);

    id = cep->selection.id;

    switch(id.name)
        {
        case GR_CHART_OBJNAME_AXIS:
            break;

        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            id.name      = GR_CHART_OBJNAME_AXIS;
            id.has_subno = 0; /* clear the crud */
            id.subno     = 0;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_BESTFITSER:
        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            {
            GR_SERIES_IX seridx;
            GR_AXES_NO   axes;

            seridx = gr_series_ix_from_external(cp, cep->selection.id.no);
            axes   = gr_axes_ix_from_seridx(cp, seridx);

            gr_chart_objid_clear(&id);
            id.name = GR_CHART_OBJNAME_AXIS;
            id.no   = gr_axes_external_from_ix(cp, axes, Y_AXIS);
            }
            break;

        default:
            gr_chart_objid_clear(&id);
            id.name = GR_CHART_OBJNAME_AXIS;
            id.no   = gr_axes_external_from_ix(cp, 0, Y_AXIS);
            break;
        }

    modifying_axis = gr_axes_ix_from_external(cp, id.no, &modifying_axes);

    if( (cp->axes[modifying_axes].charttype != GR_CHARTTYPE_SCAT) &&
        (cp->axes[0].charttype              != GR_CHARTTYPE_SCAT))
        if(modifying_axis == 0)
            {
            gr_chartedit_selection_cat_axis_process(cep, id); /* give him a hand with id processing */
            return;
            }

    d = dbox_new_new(GR_CHARTEDIT_TEM_SELECTION_AXIS, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(GR_CHART_ERR_NOMEM));
        return;
    }

    dbox_show(d);

    state.w = w = dbox_syshandle(d);

    win_settitle_c(w, gr_chart_object_name_from_id_quick(&id));

    /* id not used after that */

    do  {
        /* load current settings into structure */
        p_axes = &cp->axes[modifying_axes];
        p_axis = &p_axes->axis[modifying_axis];

        state.axis.manual    = p_axis->bits.manual;
        state.axis.min       = p_axis->bits.manual ? p_axis->punter.min : p_axis->current.min;
        state.axis.max       = p_axis->bits.manual ? p_axis->punter.max : p_axis->current.max;
        state.axis.zero      = p_axis->bits.incl_zero;
        state.axis.log_scale = p_axis->bits.log_scale;
        state.axis.pow_label = p_axis->bits.pow_label;
        state.axis.lzr       = p_axis->bits.lzr;
        state.axis.arf       = p_axis->bits.arf;

        state.axis.log_scale_modified = 0;

        mmp         = &p_axis->major;
        smp         = &state.major;
        smp->manual = mmp->bits.manual;
        smp->val    = mmp->bits.manual ? mmp->punter : mmp->current;
        smp->grid   = mmp->bits.grid;
        smp->tick   = mmp->bits.tick;

        state.axis.delta = smp->val;

        if(state.axis.max == state.axis.min)
            state.axis.max += state.axis.delta;

        mmp         = &p_axis->minor;
        smp         = &state.minor;
        smp->manual = mmp->bits.manual;
        smp->val    = mmp->bits.manual ? mmp->punter : mmp->current;
        smp->grid   = mmp->bits.grid;
        smp->tick   = mmp->bits.tick;

        state.major.delta  = smp->val;

        smp->delta  = gr_lin_major(2.0 * smp->val);

        * (int *) &state.series = 0;
        state.series.cumulative = p_axes->bits.cumulative;
        state.series.point_vary = p_axes->bits.point_vary;
        state.series.best_fit   = p_axes->bits.best_fit;
        state.series.fill_axis  = p_axes->bits.fill_axis;
        state.series.stacked    = p_axes->bits.stacked;

        state.series.cumulative_modified = 0;
        state.series.stacked_modified    = 0;

        for(;;)
            {
            gr_chartedit_selection_axis_encode(&state);

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            if(f == GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_CANCEL)
            {
                f = dbox_CLOSE;
                break;
            }

            /* adjusters all modify but rebuild at end */
            switch(f)
                {
                case dbox_OK:
                    break;

                #ifndef NDEBUG
                /* icons which can be clicked on for state but we don't want assert() to trap */
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ZERO:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_RIGHT:

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_REAR:

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_FULL:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_LEFT:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_RIGHT:

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_FULL:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_LEFT:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_RIGHT:
                    break;
                #endif

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MANUAL:
                    state.axis.manual = win_getonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MANUAL);

                    if(!state.axis.manual)
                        {
                        state.axis.min = p_axis->current.min;
                        state.axis.max = p_axis->current.max;
                        }
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO:
                    state.axis.zero = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE:
                    state.axis.log_scale = win_getonoff(w, f);
                    state.axis.log_scale_modified = 1;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL:
                    state.axis.pow_label = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL:
                    smp = &state.major;
                    mmp = &p_axis->major;

                    smp->manual = win_getonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL);

                    if(!smp->manual)
                        smp->val = mmp->current;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID:
                    state.major.grid = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO:
                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL:
                    smp = &state.minor;
                    mmp = &p_axis->minor;

                    smp->manual = win_getonoff(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL);

                    if(!smp->manual)
                        smp->val = mmp->current;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID:
                    state.minor.grid = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE:
                    state.series.cumulative = win_getonoff(w, f);
                    state.series.cumulative_modified = 1;
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY:
                    state.series.point_vary = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT:
                    state.series.best_fit = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS:
                    state.series.fill_axis = win_getonoff(w, f);
                    break;

                case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED:
                    state.series.stacked = win_getonoff(w, f);
                    state.series.stacked_modified = 1;
                    break;

                default:
                    if(win_bumpdouble(w, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,
                                      &state.axis.max,
                                      &state.axis.delta,
                                      &state.axis.min, &dbl_max_limit, INT_MAX))
                        {
                        state.major.delta = gr_lin_major(state.axis.max - state.axis.min);

                        state.axis.manual = 1;
                        break;
                        }

                    if(win_bumpdouble(w, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,
                                      &state.axis.min,
                                      &state.axis.delta,
                                      state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                                      &state.axis.max, INT_MAX))
                        {
                        state.major.delta = gr_lin_major(state.axis.max - state.axis.min);

                        state.axis.manual = 1;
                        break;
                        }

                    smp = &state.major;

                    if(state.axis.log_scale)
                        {
                        if(win_adjustbumphit(&f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL))
                            {
                            wimp_i dec = GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL - 1;
                            F64 dval;

                            dval = win_getdouble(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL, &smp->val, INT_MAX);
                            dval = fabs(dval);

                            if(f == dec)
                                {
                                /* try not to cause exceptions */

                                if( dval <= dbl_min_interval_limit)
                                    dval  = dbl_min_interval_limit;
                                else
                                    dval /= dbl_log_major_delta;
                                }
                            else
                                {
                                assert(DBL_MAX_10_EXP - 1 == 307);

                                if( dval >= dbl_log_major_max_interval_limit)
                                    dval  = dbl_log_major_max_interval_limit;
                                else
                                    {
                                    /* start at one again if decremented to the limit value */
                                    if(dval == dbl_min_interval_limit)
                                        dval = 1.0;

                                    dval *= dbl_log_major_delta;
                                    }
                                }

                            smp->val = dval;

                            smp->manual = 1;
                            break;
                            }
                        }
                    else
                        {
                        if(win_bumpdouble(w, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,
                                          &smp->val,
                                          &smp->delta,
                                          &dbl_min_interval_limit,
                                          &dbl_max_interval_limit, INT_MAX))
                            {
                            state.minor.delta = gr_lin_major(2.0 * smp->val);

                            smp->manual = 1;
                            break;
                            }
                        }

                    smp = &state.minor;

                    if(win_bumpdouble(w, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,
                                      &smp->val,
                                      state.axis.log_scale ? &dbl_log_minor_delta : &smp->delta,
                                      &dbl_min_interval_limit,
                                      state.axis.log_scale ? &dbl_log_minor_max_interval_limit : &dbl_max_interval_limit,
                                      INT_MAX))
                        {
                        smp->manual = 1;
                        break;
                        }

                    assert(0);
                    break;
                }

            /* scale */

            win_checkdouble(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,
                            &state.axis.max,
                            NULL,
                            state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                            &dbl_max_limit, INT_MAX);

            win_checkdouble(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,
                            &state.axis.min,
                            NULL,
                            state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                            &state.axis.max, INT_MAX);

            /* lzr */

            state.axis.lzr = gr_chartedit_selection_axis_lzrs[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ZERO)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT];

            /* arf */

            state.axis.arf = gr_chartedit_selection_axis_arfs[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_AUTO)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT];

            /* major */
            smp = &state.major;

            win_checkdouble(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,
                            &smp->val,
                            NULL,
                            &dbl_min_interval_limit,
                            state.axis.log_scale ? &dbl_log_major_max_interval_limit : &dbl_max_interval_limit,
                            INT_MAX);

            smp->tick = gr_chartedit_selection_axis_ticks[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE];

            /* minor */
            smp = &state.minor;

            win_checkdouble(w, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,
                            &smp->val,
                            NULL,
                            &dbl_min_interval_limit,
                            state.axis.log_scale ? &dbl_log_minor_max_interval_limit : &dbl_max_interval_limit,
                            INT_MAX);

            smp->tick = gr_chartedit_selection_axis_ticks[win_whichonoff(w,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE];

            if(f == dbox_OK)
                break;
            }

        ok = (f == dbox_OK);

        if(ok)
            {
            GR_SERIES_IX seridx;
            P_GR_SERIES   serp;

            /* set chart from modified structure */
            p_axes = &cp->axes[modifying_axes];
            p_axis = &p_axes->axis[modifying_axis];

            p_axis->bits.manual     = state.axis.manual;
            if(p_axis->bits.manual)
                {
                p_axis->punter.min  = state.axis.min;
                p_axis->punter.max  = state.axis.max;
                }
            p_axis->bits.incl_zero  = state.axis.zero;
            p_axis->bits.log_scale  = state.axis.log_scale;
            p_axis->bits.pow_label  = state.axis.pow_label;
            p_axis->bits.lzr        = state.axis.lzr;
            p_axis->bits.arf        = state.axis.arf;

            mmp                    = &p_axis->major;
            smp                    = &state.major;
            mmp->bits.manual       = smp->manual;
            if(mmp->bits.manual)
                mmp->punter        = smp->val;
            mmp->bits.tick         = smp->tick;
            mmp->bits.grid         = smp->grid;

            mmp                    = &p_axis->minor;
            smp                    = &state.minor;
            mmp->bits.manual       = smp->manual;
            if(mmp->bits.manual)
                mmp->punter        = smp->val;
            mmp->bits.tick         = smp->tick;
            mmp->bits.grid         = smp->grid;

            p_axes->bits.cumulative = state.series.cumulative;
            p_axes->bits.point_vary = state.series.point_vary;
            p_axes->bits.best_fit   = state.series.best_fit;
            p_axes->bits.fill_axis  = state.series.fill_axis;
            p_axes->bits.stacked    = state.series.stacked;

            /* reflect into series on these axes */
            for(seridx = cp->axes[modifying_axes].series.stt_idx;
                seridx < cp->axes[modifying_axes].series.end_idx;
                seridx++)
                {
                serp = getserp(cp, seridx);

                if( state.series.stacked_modified     ||
                    state.series.cumulative_modified  ||
                    state.axis.log_scale_modified     )
                    * (int *) &serp->valid = 0;
                }

            gr_chart_modify_and_rebuild(&cep->ch);
            }

        persist = ok ? dbox_persist() : FALSE;
        }
    while(persist);

    dbox_dispose(&d);
}

/* end of gr_axisp.c */
