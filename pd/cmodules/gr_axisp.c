/* gr_axisp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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
* convert an axes,axis index pair into an external axes number
*
******************************************************************************/

_Check_return_
extern GR_EAXES_NO
gr_axes_external_from_idx(
    PC_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx)
{
    assert(cp);

    if(cp->d3.bits.use)
        return((GR_EAXES_NO) ((axes_idx * 3) + axis_idx + 1));

    return((GR_EAXES_NO) ((axes_idx * 2) + axis_idx + 1));
}

/******************************************************************************
*
* convert an external axes number into an axes,axis index pair
*
******************************************************************************/

/*ncr*/
extern GR_AXIS_IDX
gr_axes_idx_from_external(
    PC_GR_CHART cp,
    GR_EAXES_NO eaxes_no,
    _OutRef_    P_GR_AXES_IDX p_axes_idx)
{
    assert(cp);

    myassert0x(eaxes_no != 0, "external axes 0");

    eaxes_no -= 1;

    if(cp->d3.bits.use)
    {
        myassert1x(eaxes_no < 6, "external axes id %d > 6", eaxes_no);
        *p_axes_idx = eaxes_no / 3;
        return(eaxes_no % 3);
    }

    myassert1x(eaxes_no < 4, "external axes id %d > 4", eaxes_no);
    *p_axes_idx = eaxes_no / 2;
    return(eaxes_no % 2);
}

/******************************************************************************
*
* return the axes index of a series index
*
******************************************************************************/

_Check_return_
extern GR_AXES_IDX
gr_axes_idx_from_series_idx(
    PC_GR_CHART   cp,
    GR_SERIES_IDX series_idx)
{
    if(series_idx < cp->axes[0].series.end_idx)
        return(0);

    return(1);
}

/******************************************************************************
*
* return the axes ptr of a series index
*
******************************************************************************/

_Check_return_
extern P_GR_AXES
gr_axesp_from_series_idx(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx)
{
    GR_AXES_IDX axes_idx;

    if(series_idx < cp->axes[0].series.end_idx)
        axes_idx = 0;
    else
        axes_idx = 1;

    return(&cp->axes[axes_idx]);
}

_Check_return_
extern P_GR_AXIS
gr_axisp_from_external(
    P_GR_CHART cp,
    GR_EAXES_NO eaxes_no)
{
    GR_AXES_IDX axes_idx;
    GR_AXIS_IDX axis_idx = gr_axes_idx_from_external(cp, eaxes_no, &axes_idx);

    return(&cp->axes[axes_idx].axis[axis_idx]);
}

static void
gr_numtopowstr(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer /*out*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     F64 value_in,
    _InVal_     F64 base)
{
    F64 value = value_in;
    F64 lnz, mantissa, exponent;

    if(value == 0.0)
    {
        xstrkpy(buffer, elemof_buffer, "0");
        return;
    }

    if(value < 0.0)
    {
        value = fabs(value);
        *buffer++ = '-';
    }

    lnz = log(value) / log(base);

    mantissa = _splitlognum(lnz, &exponent);

    if(mantissa != 0.0) /* log(1.0) == 0.0 */
    {
        mantissa = pow(base, mantissa);

        /* use ANSI 'times' character */
        (void) xsnprintf(buffer, elemof_buffer, "%g" "\xD7" "%g" "^" "%g", mantissa, base, exponent);
    }
    else
    {
        (void) xsnprintf(buffer, elemof_buffer,             "%g" "^" "%g",           base, exponent);
    }
}

/******************************************************************************
*
* helper routines for looping along axes for grids, labels and ticks
*
******************************************************************************/

typedef struct GR_AXIS_ITERATOR
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

    lnz = _splitlognum(lnz, &iterp->exponent);

    iterp->mantissa = pow(iterp->base, lnz);

    iterp->iter = iterp->mantissa * pow(iterp->base, iterp->exponent);
}

_Check_return_
static BOOL
gr_axis_iterator_first(
    PC_GR_AXIS p_axis,
    P_GR_AXIS_ITERATOR iterp /*out*/)
{
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

    return(TRUE);
}

_Check_return_
static BOOL
gr_axis_iterator_next(
    PC_GR_AXIS p_axis,
    P_GR_AXIS_ITERATOR iterp /*inout*/,
    _InVal_     BOOL major)
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

        /* SKS after PD 4.12 26mar92 - correct for very small FP rounding errors (wouldn't loop up to 3.0 in 0.1 steps) */
        if(fabs(iterp->iter - p_axis->current.max) / iterp->step < 0.000244140625) /* 2^-12 */
        {
            iterp->iter = p_axis->current.max;
            return(TRUE);
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

_Check_return_
static STATUS
gr_axis_addin_category_grids(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_grids)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_grids ? &p_axis->major : &p_axis->minor;
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   gridStart;
    GR_POINT_NO      point;
    S32              step;
    GR_LINESTYLE     linestyle;
    BOOL doing_line_chart = (cp->axes[0].charttype == GR_CHARTTYPE_LINE);
    BOOL draw_main;
    STATUS res;

    if(!p_axis_ticks->bits.grid)
        return(1);

    step = (S32) p_axis_ticks->current;
    if(!step)
        return(1);

    if(front_phase)
    {
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
        case GR_AXIS_POSITION_ARF_AUTO:
            return(1);

        case GR_AXIS_POSITION_ARF_REAR:
            return(1);

        case GR_AXIS_POSITION_ARF_FRONT:
            draw_main = TRUE;
            break;
        }
    }
    else
    {
#if 1
        /* I now argue that bringing the axis to the front
         * shouldn't affect rear grid drawing state
        */
        draw_main = TRUE;
#else
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
        case GR_AXIS_POSITION_ARF_AUTO:
            draw_main = TRUE;
            break;

        case GR_AXIS_POSITION_ARF_REAR:
            draw_main = TRUE;
            break;

        case GR_AXIS_POSITION_ARF_FRONT:
            return(1);
        }
#endif
    }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major_grids);

    status_return(res = gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point  = 0;
        point <= total_n_points;
        point += step)
    {
        GR_COORD x_pos, y_pos;
        GR_BOX line_box;

        x_pos  = gr_categ_pos(cp, point);
        y_pos  = 0;

        x_pos += cp->plotarea.posn.x;
        y_pos += cp->plotarea.posn.y;

        /* line charts require category ticks centring in slot or so they say */
        if(doing_line_chart)
            x_pos += cp->barlinech.cache.cat_group_width / 2;

        if(draw_main)
        {
            /* vertical grid line up the entire y span */
            line_box.x0 = x_pos;
            line_box.y0 = y_pos;

            line_box.x1 = line_box.x0;
            line_box.y1 = line_box.y0 + cp->plotarea.size.y;

            if(cp->d3.bits.use)
            {   /* map together to front or back of 3-D world */
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x1, cp, front_phase);
            }

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            if(cp->d3.bits.use && front_phase)
            {
                /* diagonal grid lines across the top too ONLY DURING THE FRONT PHASE */
                line_box.x0 = x_pos;
                line_box.y0 = y_pos + cp->plotarea.size.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
            }
        }

        if(cp->d3.bits.use && !front_phase)
        {
            /* diagonal grid lines across the midplane and floor too ONLY DURING THE REAR PHASE */
            line_box.x0 = x_pos;
            line_box.y0 = y_pos;

            /* one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            if((axis_ypos != 0) && (axis_ypos != cp->plotarea.size.y))
            {
                /* grid line across middle (simply shift) */
                line_box.y0 += axis_ypos;
                line_box.y1 += axis_ypos;

                status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
            }
        }
    }

    gr_chart_group_end(cp, &gridStart);

    status_return(res);

    return(1);
}

_Check_return_
static BOOL
gr_axis_ticksizes(
    P_GR_CHART       cp,
    _InRef_     PC_GR_AXIS_TICKS p_axis_ticks,
    GR_COORD *      p_top    /*out*/, /* or p_right */
    GR_COORD *      p_bottom /*out*/) /* or p_left  */
{
    GR_COORD ticksize;

    ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    switch(p_axis_ticks->bits.tick)
    {
    case GR_AXIS_TICK_POSITION_NONE:
        *p_top    = 0;
        *p_bottom = 0;
        return(FALSE);

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

    return(TRUE);
}

/*
category axis - labels
*/

_Check_return_
static STATUS
gr_axis_addin_category_labels(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase)
{
/*  const BOOL major = TRUE; */
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = &p_axis->major;
    GR_DIAG_OFFSET   labelStart;
    GR_POINT_NO      point;
    S32              step;
    GR_COORD         ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    GR_MILLIPOINT    gwidth_mp;
    GR_MILLIPOINT    available_width_mp;
    GR_COORD         available_width_px;
    STATUS res;

    step = (S32) p_axis_ticks->current;
    if(!step)
        return(1);

    status_return(res = gr_chart_group_new(cp, &labelStart, *p_id));

    consume_bool(gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_top, &ticksize_bottom));

    gr_chart_objid_textstyle_query(cp, *p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    gwidth_mp = cp->barlinech.cache.cat_group_width * GR_MILLIPOINTS_PER_PIXIT;

    available_width_px = (cp->barlinech.cache.cat_group_width * step); /* text covers 1..step categories */
    available_width_mp = available_width_px * GR_MILLIPOINTS_PER_PIXIT;

    for(point  = 0;
        point <  total_n_points; /* not <= as never have category label for end tick! */
        point += step)
    {
        GR_COORD x_pos;
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        x_pos = gr_categ_pos(cp, point);

        text_box.x0  = x_pos;
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

        switch(p_axis->bits.lzr /*bzt for category axis*/)
        {
        case GR_AXIS_POSITION_BZT_TOP:
            text_box.y0 += (3 * ticksize_top) / 2;
            text_box.y0 += (1 * textstyle.height) / 4; /* st. descenders don't crash into ticks */
            break;

        default:
            text_box.y0 -= (3 * ticksize_bottom) / 2;
            text_box.y0 -= (3 * textstyle.height) / 4; /* st. ascenders don't crash into ticks */
            break;
        }

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point_front_or_back((P_GR_POINT) &text_box.x0, cp, front_phase);

        text_box.x1 = text_box.x0 + available_width_mp; /* text covers 1..step categories */
        text_box.y1 = text_box.y0 + textstyle.height;

        status_break(res = gr_chart_text_new(cp, *p_id, &text_box, cv.data.text, &textstyle));
    }

    gr_riscdiag_font_dispose(&f);

    gr_chart_group_end(cp, &labelStart);

    return(res);
}

/*
category axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_category_ticks(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_POINT_NO      total_n_points,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_ticks)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_ticks ? &p_axis->major : &p_axis->minor;
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   tickStart;
    GR_POINT_NO      point;
    S32              step;
    GR_COORD         ticksize, ticksize_top, ticksize_bottom;
    GR_LINESTYLE     linestyle;
    S32              doing_line_chart = (cp->axes[0].charttype == GR_CHARTTYPE_LINE);
    STATUS res;

    step = (S32) p_axis_ticks->current;
    if(!step)
        return(1);

    if(!gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_top, &ticksize_bottom))
        return(1);

    if(!major_ticks)
    {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
    }

    axis_ypos -= ticksize_bottom;
    ticksize   = ticksize_top + ticksize_bottom;

    id = *p_id;

    gr_chart_objid_from_axis_tick(&id, major_ticks);

    status_return(res = gr_chart_group_new(cp, &tickStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point  = 0;
        point <= total_n_points;
        point += step)
    {
        GR_COORD x_pos;
        GR_BOX line_box;

        x_pos = gr_categ_pos(cp, point);

        /* line charts require category ticks centring in slot or so they say */
        if(doing_line_chart)
            x_pos += cp->barlinech.cache.cat_group_width / 2;

        line_box.x0  = x_pos;
        line_box.y0  = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + ticksize;

        status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
    }

    gr_chart_group_end(cp, &tickStart);

    status_return(res);

    return(1);
}

/******************************************************************************
*
* category axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_category(
    P_GR_CHART cp,
    GR_POINT_NO total_n_points,
    _InVal_     BOOL front_phase)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    GR_DIAG_OFFSET   axisStart;
    GR_CHART_OBJID   id;
    GR_COORD         axis_ypos;
    BOOL draw_main;
    STATUS res;

    switch(p_axis->bits.lzr /*bzt for category axis*/)
    {
    case GR_AXIS_POSITION_BZT_TOP:
        axis_ypos = cp->plotarea.size.y;
        break;

    case GR_AXIS_POSITION_BZT_ZERO:
        axis_ypos = (GR_COORD) ((F64) cp->plotarea.size.y *
                                cp->axes[axes_idx].axis[Y_AXIS_IDX /*NB*/].zero_frac);
        break;

    default:
        assert(0);
    case GR_AXIS_POSITION_BZT_BOTTOM:
        axis_ypos = 0;
        break;
    }

    switch(p_axis->bits.arf)
    {
    default: default_unhandled();
    case GR_AXIS_POSITION_ARF_AUTO:
        if(cp->d3.bits.use)
            if(axis_ypos != cp->plotarea.size.y)
                goto atfront;

    case GR_AXIS_POSITION_ARF_REAR:
        draw_main = !front_phase;
        break;

    case GR_AXIS_POSITION_ARF_FRONT:
    atfront:;
        draw_main = front_phase;
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(res = gr_chart_group_new(cp, &axisStart, id));

    for(;;) /* loop for structure */
    {
        /* minor grids */
        status_break(res = gr_axis_addin_category_grids(cp, &id, total_n_points, axis_ypos, front_phase, FALSE));

        /* major grids */
        status_break(res = gr_axis_addin_category_grids(cp, &id, total_n_points, axis_ypos, front_phase, TRUE));

        if(draw_main)
        {
            GR_BOX line_box;
            GR_LINESTYLE linestyle;

            /* axis line */
            line_box.x0  = 0;
            line_box.y0  = axis_ypos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            /* map to front or back of 3-D world */
            if(cp->d3.bits.use)
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);

            line_box.x1  = line_box.x0 + cp->plotarea.size.x;
            line_box.y1  = line_box.y0;

            gr_chart_objid_linestyle_query(cp, id, &linestyle);

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            /* minor ticks */
            status_break(res = gr_axis_addin_category_ticks(cp, &id, total_n_points, axis_ypos, front_phase, FALSE));

            /* major ticks */
            status_break(res = gr_axis_addin_category_ticks(cp, &id, total_n_points, axis_ypos, front_phase, TRUE));

            /* labels */
            status_break(res = gr_axis_addin_category_labels(cp, &id, total_n_points, axis_ypos, front_phase));
        }

        res = 1;
        break; /* out of loop for structure */
    }

    gr_chart_group_end(cp, &axisStart);

    return(res);
}

/******************************************************************************
*
* value x-axis & y-axis
*
******************************************************************************/

/*
value x-axis - major and minor grids
*/

_Check_return_
static STATUS
gr_axis_addin_value_grids_x(
    P_GR_CHART       cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_grids)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_grids ? &p_axis->major : &p_axis->minor;
    GR_CHART_OBJID   id;
    GR_AXIS_ITERATOR iterator;
    GR_DIAG_OFFSET   gridStart;
    GR_LINESTYLE     linestyle;
    STATUS res;
    BOOL loop;

    UNREFERENCED_PARAMETER_InVal_(axis_ypos);

    if(!p_axis_ticks->bits.grid)
        return(1);

    iterator.step = p_axis_ticks->current;
    if(!iterator.step)
        return(1);

    if(front_phase)
    {
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_ARF_AUTO:
        case GR_AXIS_POSITION_ARF_REAR:
#endif
            return(1);

        case GR_AXIS_POSITION_ARF_FRONT:
            break;
        }
    }
    else
    {
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_ARF_AUTO:
        case GR_AXIS_POSITION_ARF_REAR:
#endif
            break;

        case GR_AXIS_POSITION_ARF_FRONT:
            return(1);
        }
    }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major_grids);

    status_return(res = gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, &iterator, major_grids))
    {
        GR_COORD x_pos;
        GR_BOX line_box;

        x_pos = gr_value_pos(cp, axes_idx, axis_idx, &iterator.iter);

        /* vertical grid line across entire y span */
        line_box.x0  = x_pos;
        line_box.y0  = 0;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + cp->plotarea.size.y;

        status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
    }

    gr_chart_group_end(cp, &gridStart);

    status_return(res);

    return(1);
}

/*
value y-axis - major and minor grids
*/

_Check_return_
static STATUS
gr_axis_addin_value_grids_y(
    P_GR_CHART       cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_xpos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_grids)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_grids ? &p_axis->major : &p_axis->minor;
    GR_CHART_OBJID   id;
    GR_DIAG_OFFSET   gridStart;
    GR_AXIS_ITERATOR iterator;
    GR_LINESTYLE     linestyle;
    BOOL draw_main;
    STATUS res;
    BOOL loop;

    if(!p_axis_ticks->bits.grid)
        return(1);

    iterator.step = p_axis_ticks->current;
    if(!iterator.step)
        return(1);

    if(front_phase)
    {
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
        case GR_AXIS_POSITION_ARF_AUTO:
            return(1);

        case GR_AXIS_POSITION_ARF_REAR:
            return(1);

        case GR_AXIS_POSITION_ARF_FRONT:
            draw_main = TRUE;
            break;
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
        default: default_unhandled();
        case GR_AXIS_POSITION_ARF_AUTO:
            draw_main = TRUE;
            break;

        case GR_AXIS_POSITION_ARF_REAR:
            draw_main = TRUE;
            break;

        case GR_AXIS_POSITION_ARF_FRONT:
            return(1);
        }
#endif
    }

    id = *p_id;

    gr_chart_objid_from_axis_grid(&id, major_grids);

    status_return(res = gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, &iterator, major_grids))
    {
        GR_COORD y_pos;
        GR_BOX line_box;

        y_pos = gr_value_pos(cp, axes_idx, axis_idx, &iterator.iter);

        if(draw_main)
        {
            /* horizontal grid line across entire x span */
            line_box.x0  = 0;
            line_box.y0  = y_pos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            line_box.x1  = line_box.x0 + cp->plotarea.size.x;
            line_box.y1  = line_box.y0;

            if(cp->d3.bits.use)
            {   /* map together to front or back of 3-D world */
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x1, cp, front_phase);
            }

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            if(cp->d3.bits.use && front_phase)
            {
                /* grid lines across front of right hand side ONLY DURING THE FRONT PHASE **/

                line_box.x0  = cp->plotarea.size.x;
                line_box.y0  = y_pos;

                line_box.x0 += cp->plotarea.posn.x;
                line_box.y0 += cp->plotarea.posn.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
            }
        }

        if(cp->d3.bits.use && !front_phase)
        {
            /* grid lines across midplane and side wall ONLY DURING THE REAR PHASE */
            line_box.x0  = 0;
            line_box.y0  = y_pos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            /* put one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            if((axis_xpos != 0) && (axis_xpos != cp->plotarea.size.x))
            {
                /* grid line across middle (simply shift) */
                line_box.x0 += axis_xpos;
                line_box.x1 += axis_xpos;

                status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
            }
        }
    }

    gr_chart_group_end(cp, &gridStart);

    status_return(res);

    return(1);
}

/*
value x-axis - labels next to ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_labels_x(
    P_GR_CHART        cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase)
{
    const BOOL major = TRUE;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = &p_axis->major;
    GR_DIAG_OFFSET   labelStart;
    GR_AXIS_ITERATOR iterator;
    GR_COORD         major_width_px;
    GR_COORD         last_x1 = S32_MIN;
    GR_COORD         ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    STATUS res;
    BOOL loop;

    UNREFERENCED_PARAMETER_InVal_(front_phase);

    iterator.step = p_axis_ticks->current;
    if(!iterator.step)
        return(1);

    major_width_px = (GR_COORD) ((F64) p_axis->current_frac * cp->plotarea.size.x);

    consume_bool(gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_top, &ticksize_bottom));

    status_return(res = gr_chart_group_new(cp, &labelStart, *p_id));

    gr_chart_objid_textstyle_query(cp, *p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    for(loop = gr_axis_iterator_first(p_axis, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, &iterator, major))
    {
        GR_COORD x_pos;
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        x_pos = gr_value_pos(cp, axes_idx, axis_idx, &iterator.iter);

        text_box.x0  = x_pos;
        text_box.y0  = axis_ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        if(p_axis->bits.pow_label)
            gr_numtopowstr(cv.data.text, elemof32(cv.data.text),
                           iterator.iter,
                           iterator.base);
        else
        {
            S32 decimals = p_axis_ticks->bits.decimals;
            S32 eformat  = (fabs(iterator.iter) >= U32_MAX);

            gr_numtostr(cv.data.text, elemof32(cv.data.text),
                        iterator.iter,
                        eformat,
                        decimals,
                        CH_NULL /* dp_ch */,
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

        switch(p_axis->bits.lzr /*bzt for x-axis*/)
        {
        case GR_AXIS_POSITION_BZT_TOP:
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

        status_break(res = gr_chart_text_new(cp, *p_id, &text_box, cv.data.text, &textstyle));
    }

    gr_riscdiag_font_dispose(&f);

    gr_chart_group_end(cp, &labelStart);

    return(res);
}

/*
value y-axis - labels next to ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_labels_y(
    P_GR_CHART       cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_xpos,
    _InVal_     BOOL front_phase)
{
    const BOOL major_ticks = TRUE;
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = &p_axis->major;
    GR_DIAG_OFFSET   labelStart;
    GR_AXIS_ITERATOR iterator;
    GR_COORD         last_y1 = S32_MIN;
    GR_COORD         ticksize_left, ticksize_right, spacewidth_px;
    GR_TEXTSTYLE     textstyle;
    HOST_FONT        f;
    STATUS res;
    BOOL loop;

    iterator.step = p_axis_ticks->current;
    if(!iterator.step)
        return(1);

    consume_bool(gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_right, &ticksize_left)); /* NB. order! */

    status_return(res = gr_chart_group_new(cp, &labelStart, *p_id));

    gr_chart_objid_textstyle_query(cp, *p_id, &textstyle);

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    /* allow room for a space at whichever side of the tick we are adding at */
    {
    GR_MILLIPOINT swidth_mp = gr_font_stringwidth(f, " ");

    spacewidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;
    }

    for(loop = gr_axis_iterator_first(p_axis, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, &iterator, major_ticks))
    {
        GR_BOX text_box;
        GR_COORD y_pos;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        GR_COORD swidth_px;

        y_pos = gr_value_pos(cp, axes_idx, axis_idx, &iterator.iter);

        text_box.x0  = axis_xpos;
        text_box.y0  = y_pos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        text_box.y0 -= (1 * textstyle.height) / 4; /* a vague attempt to centre the number on the tick */

        if(text_box.y0 < last_y1)
            continue;

        if(p_axis->bits.pow_label)
            gr_numtopowstr(cv.data.text, elemof32(cv.data.text),
                           iterator.iter,
                           iterator.base);
        else
        {
            S32 decimals = p_axis_ticks->bits.decimals;
            S32 eformat  = (fabs(iterator.iter) >= U32_MAX);

            gr_numtostr(cv.data.text, elemof32(cv.data.text),
                        iterator.iter,
                        eformat,
                        decimals,
                        CH_NULL /* dp_ch */,
                        ','     /* ths_ch */);
        }

        swidth_mp = gr_font_stringwidth(f, cv.data.text);

        if(!swidth_mp)
            continue;

        swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

        /* if axis at right then left just else right just */
        if(p_axis->bits.lzr == GR_AXIS_POSITION_LZR_RIGHT)
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

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point_front_or_back((P_GR_POINT) &text_box.x0, cp, front_phase);

        text_box.x1 = text_box.x0 + swidth_px;
        text_box.y1 = text_box.y0 + textstyle.height;

        /* use standard 120% line spacing as guide */
        last_y1 = text_box.y1 + (2 * textstyle.height) / 10;

        status_break(res = gr_chart_text_new(cp, *p_id, &text_box, cv.data.text, &textstyle));
    }

    gr_riscdiag_font_dispose(&f);

    gr_chart_group_end(cp, &labelStart);

    return(res);
}

/*
value x-axis & y-axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks(
    P_GR_CHART       cp,
    GR_AXES_IDX      axes_idx,
    GR_AXIS_IDX      axis_idx,
    P_GR_CHART_OBJID p_id /*const*/,
    P_GR_POINT       axis_pos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_ticks,
    GR_COORD         ticksize,
    S32              doing_x)
{
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_ticks ? &p_axis->major : &p_axis->minor;
    GR_CHART_OBJID   id;
    GR_AXIS_ITERATOR iterator;
    GR_DIAG_OFFSET   tickStart;
    GR_LINESTYLE     linestyle;
    STATUS res;
    BOOL loop;

    iterator.step = p_axis_ticks->current;
    if(!iterator.step)
        return(1);

    id = *p_id;

    gr_chart_objid_from_axis_tick(&id, major_ticks);

    status_return(res = gr_chart_group_new(cp, &tickStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, &iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, &iterator, major_ticks))
    {
        GR_COORD pos;
        GR_BOX line_box;

        pos = gr_value_pos(cp, axes_idx, axis_idx, &iterator.iter);

        line_box.x0  = doing_x ? pos         : axis_pos->x;
        line_box.y0  = doing_x ? axis_pos->y : pos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0;

        if(doing_x)
            /* tick is vertical */
            line_box.y1 += ticksize;
        else
            /* tick is horizontal */
            line_box.x1 += ticksize;

        status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
    }

    gr_chart_group_end(cp, &tickStart);

    status_return(res);

    return(1);
}

/*
value x-axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks_x(
    P_GR_CHART       cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_ypos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_ticks)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_ticks ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_COORD ticksize, ticksize_top, ticksize_bottom;

    ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    if(!gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_top, &ticksize_bottom))
        return(1);

    if(!major_ticks)
    {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
    }

    axis_pos.y = axis_ypos - ticksize_bottom;
    ticksize   = ticksize_top + ticksize_bottom;

    return(gr_axis_addin_value_ticks(cp, axes_idx, axis_idx, p_id, &axis_pos, front_phase, major_ticks, ticksize, TRUE /*doing_x*/));
}

/*
value y-axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks_y(
    P_GR_CHART       cp,
    P_GR_CHART_OBJID p_id /*const*/,
    GR_AXES_IDX      axes_idx,
    GR_COORD         axis_xpos,
    _InVal_     BOOL front_phase,
    _InVal_     BOOL major_ticks)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    const PC_GR_AXIS_TICKS p_axis_ticks = major_ticks ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_COORD ticksize, ticksize_left, ticksize_right;

    if(!gr_axis_ticksizes(cp, p_axis_ticks, &ticksize_right, &ticksize_left)) /* NB. order! */
        return(1);

    if(!major_ticks)
    {
        ticksize_left  /= 2;
        ticksize_right /= 2;
    }

    axis_pos.x = axis_xpos - ticksize_left;
    ticksize   = ticksize_left + ticksize_right;

    return(gr_axis_addin_value_ticks(cp, axes_idx, axis_idx, p_id, &axis_pos, front_phase, major_ticks, ticksize, FALSE /*doing_x*/));
}

/******************************************************************************
*
* value x-axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_value_x(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    GR_CHART_OBJID   id;
    GR_COORD         axis_ypos;
    GR_DIAG_OFFSET   axisStart;
    BOOL draw_main;
    STATUS res;

    switch(p_axis->bits.lzr /*bzt for x-axis*/)
    {
    case GR_AXIS_POSITION_BZT_TOP:
        axis_ypos = cp->plotarea.size.y;
        break;

    case GR_AXIS_POSITION_BZT_ZERO:
        axis_ypos = (GR_COORD) ((F64) cp->plotarea.size.y *
                                cp->axes[axes_idx].axis[Y_AXIS_IDX /*NB*/].zero_frac);
        break;

    default:
        assert(0);
    case GR_AXIS_POSITION_BZT_BOTTOM:
        axis_ypos = 0;
        break;
    }

    /* NB. in 2-D AUTO and REAR synonymous */

    /* no 3-D to consider here */

    switch(p_axis->bits.arf)
    {
    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_ARF_AUTO:
    case GR_AXIS_POSITION_ARF_REAR:
#endif
        draw_main = !front_phase;
        break;

    case GR_AXIS_POSITION_ARF_FRONT:
        draw_main = front_phase;
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(res = gr_chart_group_new(cp, &axisStart, id));

    for(;;) /* loop for structure */
    {
        /* minor grids */
        status_break(res = gr_axis_addin_value_grids_x(cp, &id, axes_idx, axis_ypos, front_phase, FALSE));

        /* major grids */
        status_break(res = gr_axis_addin_value_grids_x(cp, &id, axes_idx, axis_ypos, front_phase, TRUE));

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

            gr_chart_objid_linestyle_query(cp, id, &linestyle);

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            /* minor ticks */
            status_break(res = gr_axis_addin_value_ticks_x(cp, &id, axes_idx, axis_ypos, front_phase, FALSE));

            /* major ticks */
            status_break(res = gr_axis_addin_value_ticks_x(cp, &id, axes_idx, axis_ypos, front_phase, TRUE));

            /* labels */
            status_break(res = gr_axis_addin_value_labels_x(cp, &id, axes_idx, axis_ypos, front_phase));
        }

        res = 1;
        break; /* out of loop for structure */
    }

    gr_chart_group_end(cp, &axisStart);

    status_return(res);

    return(1);
}

/******************************************************************************
*
* value y-axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_value_y(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    const PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    GR_CHART_OBJID   id;
    GR_COORD         axis_xpos;
    GR_DIAG_OFFSET   axisStart;
    BOOL draw_main;
    S32 res;

    switch(p_axis->bits.lzr)
    {
    case GR_AXIS_POSITION_LZR_RIGHT:
        axis_xpos = cp->plotarea.size.x;
        break;

    case GR_AXIS_POSITION_LZR_ZERO:
        axis_xpos = (GR_COORD) ((F64) cp->plotarea.size.x *
                                cp->axes[axes_idx].axis[X_AXIS_IDX /*NB*/].zero_frac);
        break;

    default:
        assert(0);
    case GR_AXIS_POSITION_LZR_LEFT:
        axis_xpos = 0;
        break;
    }

    /* NB. in 2-D AUTO and REAR synonymous */

    switch(p_axis->bits.arf)
    {
    default: default_unhandled();
    case GR_AXIS_POSITION_ARF_AUTO:
        if(cp->d3.bits.use)
            if(axis_xpos != cp->plotarea.size.x)
                goto atfront;

    case GR_AXIS_POSITION_ARF_REAR:
        draw_main = !front_phase;
        break;

    case GR_AXIS_POSITION_ARF_FRONT:
    atfront:;
        draw_main = front_phase;
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(res = gr_chart_group_new(cp, &axisStart, id));

    for(;;) /* loop for structure */
    {
        /* minor grids */
        status_break(res = gr_axis_addin_value_grids_y(cp, &id, axes_idx, axis_xpos, front_phase, FALSE));

        /* major grids */
        status_break(res = gr_axis_addin_value_grids_y(cp, &id, axes_idx, axis_xpos, front_phase, TRUE));

        if(draw_main)
        {
            /* axis line */
            GR_BOX line_box;
            GR_LINESTYLE linestyle;

            line_box.x0 = axis_xpos;
            line_box.y0 = 0;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            /* map to front or back of 3-D world */
            if(cp->d3.bits.use)
                gr_map_point_front_or_back((P_GR_POINT) &line_box.x0, cp, front_phase);

            line_box.x1 = line_box.x0;
            line_box.y1 = line_box.y0 + cp->plotarea.size.y;

            gr_chart_objid_linestyle_query(cp, id, &linestyle);

            status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));

            /* minor ticks */
            status_break(res = gr_axis_addin_value_ticks_y(cp, &id, axes_idx, axis_xpos, front_phase, FALSE));

            /* major ticks */
            status_break(res = gr_axis_addin_value_ticks_y(cp, &id, axes_idx, axis_xpos, front_phase, TRUE));

            /* labels */
            status_break(res = gr_axis_addin_value_labels_y(cp, &id, axes_idx, axis_xpos, front_phase));
        }

        res = 1;
        break; /* out of loop for structure */
    }

    gr_chart_group_end(cp, &axisStart);

    status_return(res);

    return(1);
}

/******************************************************************************
*
* edit selected category axis attributes
*
******************************************************************************/

typedef struct GR_CHARTEDIT_CAT_AXIS_STATE
{
    HOST_WND window_handle;

    struct GR_CHARTEDIT_CAT_AXIS_STATE_AXIS
    {
        UBF bzt : GR_AXIS_POSITION_LZR_BITS;
        UBF arf : GR_AXIS_POSITION_ARF_BITS;
    }
    axis;

    struct GR_CHARTEDIT_CAT_AXIS_STATE_TICKS
    {
        S32 val;
        S32 delta;
        S32 manual;
        S32 grid;

        UBF tick : GR_AXIS_TICK_SHAPE_BITS;
    }
    major, minor;
}
GR_CHARTEDIT_CAT_AXIS_STATE;

static const UBF /*GR_AXIS_POSITION_LZR*/
gr_chartedit_selection_axis_lzrs[] =
{
    GR_AXIS_POSITION_LZR_LEFT,
    GR_AXIS_POSITION_LZR_ZERO,
    GR_AXIS_POSITION_LZR_RIGHT
};

#define gr_chartedit_selection_cat_axis_bzts gr_chartedit_selection_axis_lzrs

static const UBF /*GR_AXIS_POSITION_ARF*/
gr_chartedit_selection_axis_arfs[] =
{
    GR_AXIS_POSITION_ARF_FRONT,
    GR_AXIS_POSITION_ARF_AUTO,
    GR_AXIS_POSITION_ARF_REAR
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
    const GR_CHARTEDIT_CAT_AXIS_STATE * const state)
{
    const struct GR_CHARTEDIT_CAT_AXIS_STATE_TICKS * smp;
    const HOST_WND window_handle = state->window_handle;
    U32 i;

    /* bzt */
    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_bzts); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM,
                      (state->axis.bzt == gr_chartedit_selection_cat_axis_bzts[i]));

    /* arf */
    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_arfs); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT,
                      (state->axis.arf == gr_chartedit_selection_cat_axis_arfs[i]));

    /* major */
    smp = &state->major;

    winf_setonoffpair(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_AUTO, !smp->manual);
    winf_setint(      window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,   smp->val);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_ticks); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE,
                      (smp->tick == gr_chartedit_selection_cat_axis_ticks[i]));

    /* minor */
    smp = &state->minor;

    winf_setonoffpair(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO, !smp->manual);
    winf_setint(      window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,   smp->val);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_cat_axis_ticks); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE,
                      (smp->tick == gr_chartedit_selection_cat_axis_ticks[i]));
}

static void
gr_chartedit_selection_cat_axis_process(
    P_GR_CHARTEDITOR cep,
    GR_CHART_OBJID id)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    P_GR_CHART cp;
    BOOL ok, persist;
    GR_CHARTEDIT_CAT_AXIS_STATE state;
    GR_AXES_IDX     modifying_axes_idx;
    GR_AXIS_IDX     modifying_axis_idx;
    P_GR_AXES       p_axes;
    P_GR_AXIS       p_axis;
    P_GR_AXIS_TICKS mmp;
    struct GR_CHARTEDIT_CAT_AXIS_STATE_TICKS * smp;

    cp = gr_chart_cp_from_ch(cep->ch);

    modifying_axis_idx = gr_axes_idx_from_external(cp, id.no, &modifying_axes_idx);

    d = dbox_new_new(GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    state.window_handle = window_handle = dbox_window_handle(d);

    win_settitle(window_handle, de_const_cast(char *, gr_chart_object_name_from_id_quick(id)));

    do  {
        /* load current settings into structure */
        p_axes = &cp->axes[modifying_axes_idx];
        p_axis = &p_axes->axis[modifying_axis_idx];

        state.axis.bzt = p_axis->bits.lzr; /* bzt for category axis */
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
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM:
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_ZERO:
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_TOP:

            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT:
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_AUTO:
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_REAR:

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

                smp->manual = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_MANUAL);

                if(!smp->manual)
                    smp->val = (S32) mmp->current;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID:
                state.major.grid = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO:
            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL:
                smp = &state.minor;
                mmp = &p_axis->minor;

                smp->manual = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL);

                if(!smp->manual)
                    smp->val = (S32) mmp->current;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID:
                state.minor.grid = winf_getonoff(window_handle, f);
                break;

            default:
                smp = &state.major;

                if(winf_bumpint(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,
                                &smp->val,
                                smp->delta,
                                1, INT_MAX))
                {
                    state.major.manual = 1;
                    break;
                }

                smp = &state.minor;

                if(winf_bumpint(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,
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

            /* bzt */

            state.axis.bzt = gr_chartedit_selection_cat_axis_bzts[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_ZERO)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM];

            /* arf */

            state.axis.arf = gr_chartedit_selection_cat_axis_arfs[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_AUTO)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT];

            /* major */
            smp = &state.major;

            winf_checkint(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL,
                          &smp->val,
                          NULL,
                          1, INT_MAX);

            smp->tick = gr_chartedit_selection_cat_axis_ticks[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE];

            /* minor */
            smp = &state.minor;

            winf_checkint(window_handle, GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL,
                          &smp->val,
                          NULL,
                          1, INT_MAX);

            smp->tick = gr_chartedit_selection_cat_axis_ticks[
                                winf_whichonoff(window_handle,
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
            p_axes = &cp->axes[modifying_axes_idx];
            p_axis = &p_axes->axis[modifying_axis_idx];

            p_axis->bits.lzr = state.axis.bzt; /* bzt for category axis */
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

typedef struct GR_CHARTEDIT_AXIS_STATE
{
    HOST_WND window_handle;

    struct GR_CHARTEDIT_AXIS_STATE_AXIS
    {
        S32 manual, zero, log_scale, log_scale_modified, pow_label;
        F64 min, max, delta;

        UBF lzr : GR_AXIS_POSITION_LZR_BITS;
        UBF arf : GR_AXIS_POSITION_ARF_BITS;
    }
    axis;

    struct GR_CHARTEDIT_AXIS_STATE_TICKS
    {
        F64 val, delta;
        S32 manual, grid;

        UBF tick : GR_AXIS_TICK_SHAPE_BITS;
    }
    major, minor;

    struct GR_CHARTEDIT_AXIS_STATE_SERIES
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
GR_CHARTEDIT_AXIS_STATE;

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
    const GR_CHARTEDIT_AXIS_STATE * state)
{
    const struct GR_CHARTEDIT_AXIS_STATE_TICKS * smp;
    const HOST_WND window_handle = state->window_handle;
    U32 i;

    /* scale */
    winf_setonoffpair(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_AUTO,     !state->axis.manual);
    winf_setdouble(   window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,      &state->axis.min, INT_MAX);
    winf_setdouble(   window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,      &state->axis.max, INT_MAX);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO,      state->axis.zero);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE, state->axis.log_scale);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL, state->axis.pow_label);

    /* lzr */
    for(i = 0; i < elemof32(gr_chartedit_selection_axis_lzrs); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT,
                      (state->axis.lzr == gr_chartedit_selection_axis_lzrs[i]));

    /* arf */
    for(i = 0; i < elemof32(gr_chartedit_selection_axis_arfs); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT,
                      (state->axis.arf == gr_chartedit_selection_axis_arfs[i]));

    /* major */
    smp = &state->major;
    winf_setonoffpair(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO, !smp->manual);
    winf_setdouble(   window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,  &smp->val, INT_MAX);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_axis_ticks); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE,
                      (smp->tick == gr_chartedit_selection_axis_ticks[i]));

    /* minor */
    smp = &state->minor;
    winf_setonoffpair(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO, !smp->manual);
    winf_setdouble(   window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,  &smp->val, INT_MAX);
    winf_setonoff(    window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID,  smp->grid);

    for(i = 0; i < elemof32(gr_chartedit_selection_axis_ticks); i++)
        winf_setonoff(window_handle, i + GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE,
                      (smp->tick == gr_chartedit_selection_axis_ticks[i]));

    /* series defaults */
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE, state->series.cumulative);
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY, state->series.point_vary);
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT,   state->series.best_fit);
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS,  state->series.fill_axis);

    /* series immediate */
    winf_setonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED,    state->series.stacked);
}

extern void
gr_chartedit_selection_axis_process(
    P_GR_CHARTEDITOR cep)
{
    dbox d;
    char * errorp;
    HOST_WND window_handle;
    dbox_field f;
    P_GR_CHART cp;
    BOOL ok, persist;
    GR_CHART_OBJID id;
    GR_CHARTEDIT_AXIS_STATE state;
    GR_AXES_IDX     modifying_axes_idx;
    GR_AXIS_IDX     modifying_axis_idx;
    P_GR_AXES       p_axes;
    P_GR_AXIS       p_axis;
    P_GR_AXIS_TICKS mmp;
    struct GR_CHARTEDIT_AXIS_STATE_TICKS * smp;

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
        GR_SERIES_IDX series_idx;
        GR_AXES_IDX axes_idx;

        series_idx = gr_series_idx_from_external(cp, cep->selection.id.no);
        axes_idx = gr_axes_idx_from_series_idx(cp, series_idx);

        gr_chart_objid_clear(&id);
        id.name = GR_CHART_OBJNAME_AXIS;
        id.no   = gr_axes_external_from_idx(cp, axes_idx, Y_AXIS_IDX);
        }
        break;

    default:
        gr_chart_objid_clear(&id);
        id.name = GR_CHART_OBJNAME_AXIS;
        id.no   = gr_axes_external_from_idx(cp, 0, Y_AXIS_IDX);
        break;
    }

    modifying_axis_idx = gr_axes_idx_from_external(cp, id.no, &modifying_axes_idx);

    if( (cp->axes[modifying_axes_idx].charttype != GR_CHARTTYPE_SCAT) &&
        (cp->axes[0].charttype                  != GR_CHARTTYPE_SCAT) )
        if(modifying_axis_idx == 0)
        {
            gr_chartedit_selection_cat_axis_process(cep, id); /* give him a hand with id processing */
            return;
        }

    d = dbox_new_new(GR_CHARTEDIT_TEM_SELECTION_AXIS, &errorp);
    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return;
    }

    dbox_show(d);

    state.window_handle = window_handle = dbox_window_handle(d);

    win_settitle(window_handle, de_const_cast(char *, gr_chart_object_name_from_id_quick(id)));

    /* id not used after that */

    do  {
        /* load current settings into structure */
        p_axes = &cp->axes[modifying_axes_idx];
        p_axis = &p_axes->axis[modifying_axis_idx];

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
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_ZERO:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_RIGHT:

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_AUTO:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_REAR:

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
                state.axis.manual = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MANUAL);

                if(!state.axis.manual)
                {
                    state.axis.min = p_axis->current.min;
                    state.axis.max = p_axis->current.max;
                }
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO:
                state.axis.zero = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE:
                state.axis.log_scale = winf_getonoff(window_handle, f);
                state.axis.log_scale_modified = 1;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL:
                state.axis.pow_label = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL:
                smp = &state.major;
                mmp = &p_axis->major;

                smp->manual = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL);

                if(!smp->manual)
                    smp->val = mmp->current;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID:
                state.major.grid = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO:
            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL:
                smp = &state.minor;
                mmp = &p_axis->minor;

                smp->manual = winf_getonoff(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL);

                if(!smp->manual)
                    smp->val = mmp->current;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID:
                state.minor.grid = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE:
                state.series.cumulative = winf_getonoff(window_handle, f);
                state.series.cumulative_modified = 1;
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY:
                state.series.point_vary = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT:
                state.series.best_fit = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS:
                state.series.fill_axis = winf_getonoff(window_handle, f);
                break;

            case GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED:
                state.series.stacked = winf_getonoff(window_handle, f);
                state.series.stacked_modified = 1;
                break;

            default:
                if(winf_bumpdouble(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,
                                   &state.axis.max,
                                   &state.axis.delta,
                                   &state.axis.min, &dbl_max_limit,
                                   INT_MAX))
                {
                    state.major.delta = gr_lin_major(state.axis.max - state.axis.min);

                    state.axis.manual = 1;
                    break;
                }

                if(winf_bumpdouble(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,
                                   &state.axis.min,
                                   &state.axis.delta,
                                   state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                                   &state.axis.max,
                                   INT_MAX))
                {
                    state.major.delta = gr_lin_major(state.axis.max - state.axis.min);

                    state.axis.manual = 1;
                    break;
                }

                smp = &state.major;

                if(state.axis.log_scale)
                {
                    if(winf_adjustbumphit(&f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL))
                    {
                        const int dec_icon_handle = GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL - 1;
                        F64 dval;

                        dval = winf_getdouble(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL, &smp->val, INT_MAX);
                        dval = fabs(dval);

                        if(f == dec_icon_handle)
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
                    if(winf_bumpdouble(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,
                                       &smp->val,
                                       &smp->delta,
                                       &dbl_min_interval_limit,
                                       &dbl_max_interval_limit,
                                       INT_MAX))
                    {
                        state.minor.delta = gr_lin_major(2.0 * smp->val);

                        smp->manual = 1;
                        break;
                    }
                }

                smp = &state.minor;

                if(winf_bumpdouble(window_handle, f, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,
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

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX,
                             &state.axis.max,
                             NULL,
                             state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                             &dbl_max_limit,
                             INT_MAX);

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN,
                             &state.axis.min,
                             NULL,
                             state.axis.log_scale ? &dbl_log_min_limit : &dbl_min_limit,
                             &state.axis.max,
                             INT_MAX);

            /* lzr */

            state.axis.lzr = gr_chartedit_selection_axis_lzrs[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_ZERO)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT];

            /* arf */

            state.axis.arf = gr_chartedit_selection_axis_arfs[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT + 2,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_AUTO)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT];

            /* major */
            smp = &state.major;

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL,
                             &smp->val,
                             NULL,
                             &dbl_min_interval_limit,
                             state.axis.log_scale ? &dbl_log_major_max_interval_limit : &dbl_max_interval_limit,
                             INT_MAX);

            smp->tick = gr_chartedit_selection_axis_ticks[
                                winf_whichonoff(window_handle,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE + 3,
                                      GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_FULL)
                                    - GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE];

            /* minor */
            smp = &state.minor;

            winf_checkdouble(window_handle, GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL,
                             &smp->val,
                             NULL,
                             &dbl_min_interval_limit,
                             state.axis.log_scale ? &dbl_log_minor_max_interval_limit : &dbl_max_interval_limit,
                             INT_MAX);

            smp->tick = gr_chartedit_selection_axis_ticks[
                                winf_whichonoff(window_handle,
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
            GR_SERIES_IDX series_idx;
            P_GR_SERIES serp;

            /* set chart from modified structure */
            p_axes = &cp->axes[modifying_axes_idx];
            p_axis = &p_axes->axis[modifying_axis_idx];

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
            for(series_idx = cp->axes[modifying_axes_idx].series.stt_idx;
                series_idx < cp->axes[modifying_axes_idx].series.end_idx;
                series_idx++)
            {
                serp = getserp(cp, series_idx);

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
