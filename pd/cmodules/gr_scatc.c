/* gr_scatc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module to handle scatter charts */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

extern void
gr_barlinescatch_get_datasources(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx,
    P_GR_DATASOURCE_FOURSOME dsh /*out*/)
{
    P_GR_SERIES       serp;
    GR_DATASOURCE_NO ds;

    serp = getserp(cp, series_idx);

    ds = 0;

    switch(serp->sertype)
    {
    case GR_CHART_SERIES_PLAIN:
    case GR_CHART_SERIES_PLAIN_ERROR1:
    case GR_CHART_SERIES_PLAIN_ERROR2:
        dsh->value_x = cp->core.category_datasource.dsh;
        break;

    default:
        dsh->value_x = gr_travel_series_dsh_from_ds(cp, series_idx, ds++);
        break;
    }

    dsh->value_y = gr_travel_series_dsh_from_ds(cp, series_idx, ds++);

    dsh->error_x = GR_DATASOURCE_HANDLE_NONE;
    dsh->error_y = GR_DATASOURCE_HANDLE_NONE;

    switch(serp->sertype)
    {
    case GR_CHART_SERIES_POINT_ERROR1:
    case GR_CHART_SERIES_PLAIN_ERROR1:
        dsh->error_y = gr_travel_series_dsh_from_ds(cp, series_idx, ds++);
        break;

    case GR_CHART_SERIES_POINT_ERROR2:
    case GR_CHART_SERIES_PLAIN_ERROR2:
        dsh->error_x = gr_travel_series_dsh_from_ds(cp, series_idx, ds++);
        dsh->error_y = gr_travel_series_dsh_from_ds(cp, series_idx, ds++);
        break;

    default:
        break;
    }
}

static void
gr_actualise_series_point(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx,
    S32          plain)
{
    P_GR_SERIES serp = getserp(cp, series_idx);

    UNREFERENCED_PARAMETER(plain);

    if(1 /*!serp->valid.limits*/)
    {
        GR_CHART_ITEMNO        n_items, item;
        GR_DATASOURCE_FOURSOME dsh;
        GR_CHART_NUMPAIR       value;
        PC_GR_AXES p_axes = gr_axesp_from_series_idx(cp, series_idx);
        PC_GR_AXIS x_axisp = &p_axes->axis[X_AXIS_IDX];
        PC_GR_AXIS y_axisp = &p_axes->axis[Y_AXIS_IDX];

        serp->cache.limits_x.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_x.max = -GR_CHART_NUMBER_MAX;

        serp->cache.limits_y.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_y.max = -GR_CHART_NUMBER_MAX;

        gr_barlinescatch_get_datasources(cp, series_idx, &dsh);

        n_items = gr_travel_dsh_n_items(cp, dsh.value_x);

        for(item = 0; item < n_items; ++item)
        {
            if(!gr_travel_dsh_valof(cp, dsh.value_x, item, &value.x))
                continue;

            if(!gr_travel_dsh_valof(cp, dsh.value_y, item, &value.y))
                continue;

            /* points are never stacked or accumulated! */

            if(x_axisp->bits.log_scale)
                if(value.x <= 0.0)
                    continue;

            if(y_axisp->bits.log_scale)
                if(value.y <= 0.0)
                    continue;

            serp->cache.limits_x.min = MIN(serp->cache.limits_x.min, value.x);
            serp->cache.limits_x.max = MAX(serp->cache.limits_x.max, value.x);

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, value.y);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, value.y);
        }

        serp->valid.limits = 1;
    }
}

static void
gr_actualise_series_point_error1(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx,
    S32          plain)
{
    P_GR_SERIES serp;

    serp = getserp(cp, series_idx);

    UNREFERENCED_PARAMETER(plain);

    if(1 /*!serp->valid.limits*/)
    {
        GR_CHART_ITEMNO        n_items, item;
        GR_DATASOURCE_FOURSOME dsh;
        GR_CHART_NUMPAIR       value, error, old_error;
        GR_CHART_NUMBER        valincerr;
        PC_GR_AXES p_axes = gr_axesp_from_series_idx(cp, series_idx);
        PC_GR_AXIS x_axisp = &p_axes->axis[X_AXIS_IDX];
        PC_GR_AXIS y_axisp = &p_axes->axis[Y_AXIS_IDX];

        serp->cache.limits_x.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_x.max = -GR_CHART_NUMBER_MAX;

        serp->cache.limits_y.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_y.max = -GR_CHART_NUMBER_MAX;

        gr_barlinescatch_get_datasources(cp, series_idx, &dsh);

        n_items = gr_travel_dsh_n_items(cp, dsh.value_x);

        old_error.y = 0.0;

        for(item = 0; item < n_items; ++item)
        {
            if(!gr_travel_dsh_valof(cp, dsh.value_x, item, &value.x))
                continue;

            if(!gr_travel_dsh_valof(cp, dsh.value_y, item, &value.y))
                continue;

            /* points are never stacked or accumulated! */

            if(x_axisp->bits.log_scale)
                if(value.x <= 0.0)
                    continue;

            if(y_axisp->bits.log_scale)
                if(value.y <= 0.0)
                    continue;

            serp->cache.limits_x.min = MIN(serp->cache.limits_x.min, value.x);
            serp->cache.limits_x.max = MAX(serp->cache.limits_x.max, value.x);

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, value.y);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, value.y);

            if(gr_travel_dsh_valof(cp, dsh.error_y, item, &error.y))
            {
                error.y = fabs(error.y);
                old_error.y = error.y;
            }
            else
                error.y = old_error.y;

            /* error bars off top and bottom */
            valincerr = value.y + error.y;
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, valincerr);

            valincerr = value.y - error.y;

            if(!y_axisp->bits.log_scale || (valincerr > 0.0))
                serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, valincerr);
        }

        serp->valid.limits = 1;
    }
}

static void
gr_actualise_series_point_error2(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx,
    S32          plain)
{
    P_GR_SERIES serp;

    serp = getserp(cp, series_idx);

    UNREFERENCED_PARAMETER(plain);

    if(1 /*!serp->valid.limits*/)
    {
        GR_CHART_ITEMNO        n_items, item;
        GR_DATASOURCE_FOURSOME dsh;
        GR_CHART_NUMPAIR       value, error, old_error;
        GR_CHART_NUMBER        valincerr;
        PC_GR_AXES p_axes = gr_axesp_from_series_idx(cp, series_idx);
        PC_GR_AXIS x_axisp = &p_axes->axis[X_AXIS_IDX];
        PC_GR_AXIS y_axisp = &p_axes->axis[Y_AXIS_IDX];

        serp->cache.limits_x.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_x.max = -GR_CHART_NUMBER_MAX;

        serp->cache.limits_y.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_y.max = -GR_CHART_NUMBER_MAX;

        gr_barlinescatch_get_datasources(cp, series_idx, &dsh);

        n_items = gr_travel_dsh_n_items(cp, dsh.value_x);

        old_error.x = 0.0;
        old_error.y = 0.0;

        for(item = 0; item < n_items; ++item)
        {
            if(!gr_travel_dsh_valof(cp, dsh.value_x, item, &value.x))
                continue;

            if(!gr_travel_dsh_valof(cp, dsh.value_y, item, &value.y))
                continue;

            /* points are never stacked or accumulated! */

            if(x_axisp->bits.log_scale)
                if(value.x <= 0.0)
                    continue;

            if(y_axisp->bits.log_scale)
                if(value.y <= 0.0)
                    continue;

            serp->cache.limits_x.min = MIN(serp->cache.limits_x.min, value.x);
            serp->cache.limits_x.max = MAX(serp->cache.limits_x.max, value.x);

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, value.y);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, value.y);

            if(gr_travel_dsh_valof(cp, dsh.error_x, item, &error.x))
            {
                error.x = fabs(error.x);
                old_error.x = error.x;
            }
            else
                error.x = old_error.x;

            if(gr_travel_dsh_valof(cp, dsh.error_y, item, &error.y))
            {
                error.y = fabs(error.y);
                old_error.y = error.y;
            }
            else
                error.y = old_error.y;

            /* error bars off left and right */
            valincerr = value.x + error.x;
            serp->cache.limits_x.max = MAX(serp->cache.limits_x.max, valincerr);

            valincerr = value.x - error.x;
            if(!x_axisp->bits.log_scale || (valincerr > 0.0))
                serp->cache.limits_x.min = MIN(serp->cache.limits_x.min, valincerr);

            /* error bars off top and bottom */
            valincerr = value.y + error.y;
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, valincerr);

            valincerr = value.y - error.y;
            if(!y_axisp->bits.log_scale || (valincerr > 0.0))
                serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, valincerr);
        }

        serp->valid.limits = 1;
    }
}

/******************************************************************************
*
* scatter
*
******************************************************************************/

_Check_return_
static STATUS
gr_scatterchart_series_addin(
    P_GR_CHART    cp,
    GR_AXES_IDX   axes_idx,
    GR_SERIES_IDX series_idx)
{
    const P_GR_SERIES serp = getserp(cp, series_idx);
    GR_DATASOURCE_FOURSOME dsh;
    GR_CHART_OBJID         serid;
    GR_CHART_ITEMNO        n_points;
    GR_POINT_NO            point;
    GR_CHART_NUMPAIR       value;
    GR_CHART_NUMPAIR       error, old_error;
    GR_SCATCHSTYLE         scatchstyle;
    GR_LINESTYLE           linestyle;
    GR_FILLSTYLE           fillstyle;
    GR_POINT valpoint;
    GR_POINT old_valpoint = { 0, 0 }; /* keep dataflower happy */
    GR_COORD               pict_halfsize;
    GR_COORD               tbar_halfsize;
    GR_DIAG_OFFSET         lineStart, pointStart;
    STATUS res;
    S32                    best_fit;
    S32                    point_pass;

    gr_chart_objid_from_series_idx(cp, series_idx, &serid);

    status_return(res = gr_chart_group_new(cp, &lineStart, serid));

    gr_barlinescatch_get_datasources(cp, series_idx, &dsh);

    n_points = gr_travel_dsh_n_items(cp, dsh.value_x);

    best_fit = serp->bits.best_fit_manual
                        ? serp->bits.best_fit
                        : gr_axesp_from_series_idx(cp, series_idx)->bits.best_fit;

    for(point_pass = 0; point_pass <= 1; ++point_pass)
    {
        GR_DIAG_OFFSET linepassStart;
        GR_CHART_OBJID id;
        S32            had_first;

        status_break(res = gr_chart_group_new(cp, &linepassStart, serid));

        id           = serid;
        id.name      = GR_CHART_OBJNAME_POINT;
        id.has_subno = 1;

        old_error.x = 0.0;
        old_error.y = 0.0;

        had_first = 0;

        for(point = 0; point < n_points; ++point)
        {
            id.subno = (U16) gr_point_external_from_key(point);

            if(!gr_travel_dsh_valof(cp, dsh.value_x, point, &value.x))
            {
                /* lose line between points if x blank */
                had_first = 0;
                continue;
            }

            if(!gr_travel_dsh_valof(cp, dsh.value_y, point, &value.y))
            {
                /* lose line between points if y blank */
                had_first = 0;
                continue;
            }

            if(dsh.error_x != GR_DATASOURCE_HANDLE_NONE)
            {
                if(gr_travel_dsh_valof(cp, dsh.error_x, point, &error.x))
                {
                    error.x = fabs(error.x);
                    old_error.x = error.x;
                }
                else
                    error.x = old_error.x; /* SKS 22jan92 says blanks in error record maintain previous error values */
            }
            else
                error.x = 0;

            if(dsh.error_y != GR_DATASOURCE_HANDLE_NONE)
            {
                if(gr_travel_dsh_valof(cp, dsh.error_y, point, &error.y))
                {
                    error.y = fabs(error.y);
                    old_error.y = error.y;
                }
                else
                    error.y = old_error.y; /* SKS 22jan92 says blanks in error record maintain previous error values */
            }
            else
                error.y = 0;

            /* NB. do value & error cumulation before deciding to wimp out */

            /* can't plot zero or -ve x values on log chart */
            if(cp->axes[axes_idx].axis[X_AXIS_IDX].bits.log_scale)
                if(value.x <= 0.0)
                {
                    /* lose line between points if 'blank' */
                    had_first = 0;
                    continue;
                }

            /* can't plot zero or -ve y values on log chart */
            if(cp->axes[axes_idx].axis[Y_AXIS_IDX].bits.log_scale)
                if(value.y <= 0.0)
                {
                    /* lose line between points if 'blank' */
                    had_first = 0;
                    continue;
                }

            status_break(res = gr_chart_group_new(cp, &pointStart, id));

            gr_point_fillstyle_query(cp, series_idx, point, &fillstyle);
            gr_point_linestyle_query(cp, series_idx, point, &linestyle);

            gr_point_scatchstyle_query(cp, series_idx, point, &scatchstyle);

            valpoint.x = gr_value_pos(cp, axes_idx, X_AXIS_IDX, &value.x);
            valpoint.y = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &value.y);

            /* map into absolute plot area */
            valpoint.x += cp->plotarea.posn.x;
            valpoint.y += cp->plotarea.posn.y;

            pict_halfsize = MAX(cp->plotarea.size.x, cp->plotarea.size.y) / 8; /* arbitrary constant */
            pict_halfsize = (GR_COORD) ((F64) pict_halfsize * scatchstyle.width_percentage / 100.0 / 2.0);
            tbar_halfsize = pict_halfsize / 2;

            /* x error tits plotted below point? */
            if(error.x && !point_pass)
            {
                GR_CHART_NUMBER valincerr;
                GR_BOX    err_box;
                GR_COORD        errsize, right_side;

                /* horizontal part of H */
                if(cp->axes[axes_idx].axis[X_AXIS_IDX].bits.log_scale)
                {
                    valincerr   = value.x - error.x;
                    err_box.x0  = gr_value_pos(cp, axes_idx, X_AXIS_IDX, &valincerr);
                    valincerr   = value.x + error.x;
                    err_box.x1  = gr_value_pos(cp, axes_idx, X_AXIS_IDX, &valincerr);

                    /* map into absolute plot area */
                    err_box.x0 += cp->plotarea.posn.x;
                    err_box.x1 += cp->plotarea.posn.x;
                }
                else
                {
                    errsize     = gr_value_pos_rel(cp, axes_idx, X_AXIS_IDX, &error.x);

                    err_box.x0  = valpoint.x - errsize;
                    err_box.x1  = valpoint.x + errsize;
                }

                err_box.y0 = err_box.y1 = valpoint.y;

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));

                /* vertical part of H at left */
                right_side = err_box.x1;
                err_box.x1 = err_box.x0;

                err_box.y1 = err_box.y0 + tbar_halfsize;
                err_box.y0 = err_box.y0 - tbar_halfsize;

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));

                /* vertical part of H at right */
                err_box.x0 = err_box.x1 = right_side;

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));
            }

            /* y error tits plotted below point? */
            if(error.y && !point_pass)
            {
                GR_CHART_NUMBER valincerr;
                GR_BOX    err_box;
                GR_COORD        errsize, top_side;

                /* vertical part of I */
                err_box.x0 = err_box.x1 = valpoint.x;

                if(cp->axes[axes_idx].axis[Y_AXIS_IDX].bits.log_scale)
                {
                    valincerr  = value.y - error.y;
                    err_box.y0 = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &valincerr);
                    valincerr  = value.y + error.y;
                    err_box.y1 = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &valincerr);

                    /* map into absolute plot area */
                    err_box.y0 += cp->plotarea.posn.y;
                    err_box.y1 += cp->plotarea.posn.y;
                }
                else
                {
                    errsize    = gr_value_pos_rel(cp, axes_idx, Y_AXIS_IDX, &error.y);

                    err_box.y0 = valpoint.y - errsize;
                    err_box.y1 = valpoint.y + errsize;
                }

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));

                /* horizontal part of I at bottom */
                err_box.x1 = err_box.x0 + tbar_halfsize;
                err_box.x0 = err_box.x0 - tbar_halfsize;

                top_side   = err_box.y1;
                err_box.y1 = err_box.y0;

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));

                /* horizontal part of I at top */
                err_box.y0 = err_box.y1 = top_side;

                status_break(res = gr_chart_line_new(cp, id, &err_box, &linestyle));
            }

            if(!had_first)
                had_first = 1;
            else
            {
                if(!point_pass && !scatchstyle.bits.lines_off)
                {
                    /* the line joins the actual points */
                    GR_BOX line_box;

                    line_box.x0 = old_valpoint.x;
                    line_box.y0 = old_valpoint.y;
                    line_box.x1 =     valpoint.x;
                    line_box.y1 =     valpoint.y;

                    status_break(res = gr_chart_line_new(cp, id, &line_box, &linestyle));
                }
            }

            /* leave some debris behind to be picked up next time */
            old_valpoint = valpoint;

            /* pretty picture at front? */
            if(fillstyle.pattern && point_pass)
            {
                GR_BOX pict_box;

                pict_box.x0 = valpoint.x - pict_halfsize;
                pict_box.y0 = valpoint.y - pict_halfsize;
                pict_box.x1 = valpoint.x + pict_halfsize;
                pict_box.y1 = valpoint.y + pict_halfsize;

                status_break(res = gr_chart_scaled_picture_add(cp, id, &pict_box, &fillstyle));
            }

            gr_chart_group_end(cp, &pointStart);
        }

        status_break(res);

        gr_chart_group_end(cp, &linepassStart);

        if(point_pass == 0)
        {
            if(best_fit)
            {
                GR_BARLINESCATCH_SERIES_CACHE   single_series_cache;
                P_GR_BARLINESCATCH_SERIES_CACHE lcp;

                lcp = &single_series_cache;

                lcp->cp     = cp;
                lcp->axes_idx   = axes_idx;
                lcp->series_idx = series_idx;

                lcp->serid  = serid;

                status_break(res = gr_barlinescatch_best_fit_addin(lcp, GR_CHARTTYPE_SCAT));
            }
        }

        /* end of point_pass loop */
    }

    gr_chart_group_end(cp, &lineStart);

    status_return(res);

    return(1);
}

_Check_return_
static STATUS
gr_scatterchart_axes_addin(
    P_GR_CHART cp,
    _InVal_     BOOL front_phase)
{
    GR_DIAG_OFFSET axesGroupStart;
    GR_AXES_IDX axes_idx;
    STATUS res;

    status_return(res = gr_chart_group_new(cp, &axesGroupStart, gr_chart_objid_anon));

    axes_idx = cp->axes_idx_max;

    do  {
        status_break(res = gr_axis_addin_value_x(cp, axes_idx, front_phase));

        status_break(res = gr_axis_addin_value_y(cp, axes_idx, front_phase));
    }
    while(axes_idx-- > 0);

    gr_chart_group_end(cp, &axesGroupStart);

    status_return(res);

    return(1);
}

_Check_return_
extern STATUS
gr_scatterchart_addin(
    P_GR_CHART cp)
{
    GR_CHART_ITEMNO total_n_points, n_points;
    GR_AXES_IDX     axes_idx;
    GR_SERIES_IDX   series_idx;
    P_GR_SERIES     serp;
    STATUS res;

    total_n_points = 0; /* category datasource ignored */

    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        P_GR_AXES p_axes = &cp->axes[axes_idx];
        P_GR_AXIS xaxisp = &p_axes->axis[X_AXIS_IDX];
        P_GR_AXIS yaxisp = &p_axes->axis[Y_AXIS_IDX];

        xaxisp->actual.min = +GR_CHART_NUMBER_MAX;
        xaxisp->actual.max = -GR_CHART_NUMBER_MAX;

        yaxisp->actual.min = +GR_CHART_NUMBER_MAX;
        yaxisp->actual.max = -GR_CHART_NUMBER_MAX;

        for(series_idx = p_axes->series.stt_idx;
            series_idx < p_axes->series.end_idx;
            series_idx++)
        {
            serp = getserp(cp, series_idx);

            n_points = gr_travel_series_n_items_total(cp, series_idx);

            total_n_points = MAX(total_n_points, n_points);

            switch(serp->sertype)
            {
            case GR_CHART_SERIES_POINT_ERROR1:
            case GR_CHART_SERIES_PLAIN_ERROR1:
                gr_actualise_series_point_error1(cp, series_idx, (serp->sertype == GR_CHART_SERIES_PLAIN_ERROR1));
                break;

            case GR_CHART_SERIES_POINT_ERROR2:
            case GR_CHART_SERIES_PLAIN_ERROR2:
                gr_actualise_series_point_error2(cp, series_idx, (serp->sertype == GR_CHART_SERIES_PLAIN_ERROR2));
                break;

            default:
                gr_actualise_series_point(cp, series_idx, (serp->sertype == GR_CHART_SERIES_PLAIN));
                break;
            }

            xaxisp->actual.min = MIN(xaxisp->actual.min, serp->cache.limits_x.min);
            xaxisp->actual.max = MAX(xaxisp->actual.max, serp->cache.limits_x.max);

            yaxisp->actual.min = MIN(yaxisp->actual.min, serp->cache.limits_y.min);
            yaxisp->actual.max = MAX(yaxisp->actual.max, serp->cache.limits_y.max);
        }
    }

    /* loop over axes again after total_n_points accumulated,
     * sussing best fit lines and forming x-axis & y-axis for each axes set
    */
    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        P_GR_AXES p_axes = &cp->axes[axes_idx];

        for(series_idx = p_axes->series.stt_idx;
            series_idx < p_axes->series.end_idx;
            series_idx++)
        {
            serp = getserp(cp, series_idx);

            if(serp->bits.best_fit_manual
                        ? serp->bits.best_fit
                        : p_axes->bits.best_fit)
            {
                /* derive best fit line parameters */
                GR_BARLINESCATCH_LINEST_STATE state;

                gr_barlinescatch_get_datasources(cp, series_idx, &state.dsh);

                state.cp    = cp;
                state.y_cum = 0.0;
                state.x_log = (p_axes->axis[X_AXIS_IDX].bits.log_scale != 0);
                state.y_log = (p_axes->axis[Y_AXIS_IDX].bits.log_scale != 0);

                /* initialise to be on the safe side */
                serp->cache.best_fit_c = 0.0;
                serp->cache.best_fit_m = 1.0;

                status_consume(
                    linest(gr_barlinescatch_linest_getproc,
                        gr_barlinescatch_linest_putproc,
                        (CLIENT_HANDLE) &state,
                        1, /* independent x variables */
                        (U32) total_n_points));

                /* store m and c away for fuschia reference */
                serp->cache.best_fit_c = state.a[0];
                serp->cache.best_fit_m = state.a[1];
            }
        }

        gr_axis_form_value(cp, axes_idx, X_AXIS_IDX);
        gr_axis_form_value(cp, axes_idx, Y_AXIS_IDX);
    }

    /* add in rear axes and gridlines */
    status_return(res = gr_scatterchart_axes_addin(cp, FALSE));

    /* add in data on axes - always plot from the back to the front */
    axes_idx = cp->axes_idx_max;

    do {
        const P_GR_AXES p_axes = &cp->axes[axes_idx];

        series_idx = p_axes->series.end_idx;

        while(series_idx > p_axes->series.stt_idx)
        {
            --series_idx;

            status_break(res = gr_scatterchart_series_addin(cp, axes_idx, series_idx));
        }

        status_break(res);
    }
    while(axes_idx-- > 0);

    status_return(res);

    /* add in front axes and gridlines */
    status_return(gr_scatterchart_axes_addin(cp, TRUE));

    return(1);
}

_Check_return_
extern STATUS
gr_barlinescatch_best_fit_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    GR_CHARTTYPE charttype)
{
    P_GR_CHART     cp;
    GR_AXES_IDX    axes_idx;
    GR_SERIES_IDX  series_idx;
    P_GR_SERIES    serp;
    S32            x_log, y_log, categories, negative_slope;
    GR_POINT_NO    x0, x1;
    F64            fpx0, fpx1;
    F64            fpy0, fpy1;
    F64            fpx;
    GR_BOX line_box;
    GR_POINT zsv;
    F64 z_frac;
    GR_CHART_OBJID id;
    GR_LINESTYLE linestyle;

    if(!lcp->best_fit)
        return(1);

    cp     = lcp->cp;
    axes_idx   = lcp->axes_idx;
    series_idx = lcp->series_idx;

    serp = getserp(cp, series_idx);

    x_log = (cp->axes[axes_idx].axis[X_AXIS_IDX].bits.log_scale != 0);
    y_log = (cp->axes[axes_idx].axis[Y_AXIS_IDX].bits.log_scale != 0);

    /* work out if line is steep enough to intersect top or bottom */
    fpy0 = cp->axes[axes_idx].axis[Y_AXIS_IDX].current.min;
    fpy1 = cp->axes[axes_idx].axis[Y_AXIS_IDX].current.max;

    if(y_log)
    {
        fpy0 = log(fpy0);
        fpy1 = log(fpy1);
    }

    categories = (charttype != GR_CHARTTYPE_SCAT);

    if(categories)
    {
        /* NEVER use log on category axis */
        x_log = 0;

        fpx0 = 1;
        fpx1 = lcp->n_points;
    }
    else
    {
        fpx0 = cp->axes[axes_idx].axis[X_AXIS_IDX].current.min;
        fpx1 = cp->axes[axes_idx].axis[X_AXIS_IDX].current.max;
    }

    if(x_log)
    {
        fpx0 = log(fpx0);
        fpx1 = log(fpx1);
    }

    negative_slope = (serp->cache.best_fit_m < 0.0);

    if(serp->cache.best_fit_m != 0.0)
    {
        fpx = (fpy0 - serp->cache.best_fit_c) / serp->cache.best_fit_m;

        if((fpx > fpx0) && (fpx < fpx1))
        {
            if(negative_slope)
                fpx1 = categories ? floor(fpx) : fpx;
            else
                fpx0 = categories ? ceil( fpx) : fpx;
        }

        fpx = (fpy1 - serp->cache.best_fit_c) / serp->cache.best_fit_m;

        if((fpx > fpx0) && (fpx < fpx1))
        {
            if(negative_slope)
                fpx0 = categories ? ceil( fpx) : fpx;
            else
                fpx1 = categories ? floor(fpx) : fpx;
        }
    }

    /* SKS now believes this to be correct in all 4 cases */
    fpy0 = fpx0 * serp->cache.best_fit_m + serp->cache.best_fit_c;
    fpy1 = fpx1 * serp->cache.best_fit_m + serp->cache.best_fit_c;

    if(y_log)
    {
        fpy0 = exp(fpy0);
        fpy1 = exp(fpy1);
    }

    if(x_log)
    {
        fpx0 = exp(fpx0);
        fpx1 = exp(fpx1);
    }

    if(fpx0 > fpx1)
        return(1);

    if(categories)
    {
        /* place in right category groups */
        x0 = (GR_POINT_NO) fpx0;
        x1 = (GR_POINT_NO) fpx1;

        x0 = MAX(x0, 1);
        x1 = MIN(x1, lcp->n_points);

        line_box.x0 = (int) gr_categ_pos(cp, x0 - 1);
        line_box.x1 = (int) gr_categ_pos(cp, x1 - 1);
    }
    else
    {
        /* must do after converting values back */
        line_box.x0 = gr_value_pos(cp, axes_idx, X_AXIS_IDX, &fpx0);
        line_box.x1 = gr_value_pos(cp, axes_idx, X_AXIS_IDX, &fpx1);
    }

    switch(charttype)
    {
    case GR_CHARTTYPE_BAR:
        /* shift bar slots along group according to overlap (hi overlap -> low shift) */
        line_box.x0 += (int) (cp->barch.cache.slot_shift * lcp->barindex);
        line_box.x1 += (int) (cp->barch.cache.slot_shift * lcp->barindex);

        /* centre points on bars in their slots */
        line_box.x0 += (int) ((cp->barch.cache.slot_width - 0 /*zero bar_width*/) / 2);
        line_box.x1 += (int) ((cp->barch.cache.slot_width - 0 /*zero bar_width*/) / 2);
        break;

    case GR_CHARTTYPE_LINE:
        /* shift points on line along group according to overlap (hi overlap -> low shift) */
        line_box.x0 += (int) (cp->linech.cache.slot_shift * lcp->lineindex);
        line_box.x1 += (int) (cp->linech.cache.slot_shift * lcp->lineindex);

        /* shift points on line to right offsets in group (normally 1/2 way along) */
        line_box.x0 += (int) cp->linech.cache.slot_start;
        line_box.x1 += (int) cp->linech.cache.slot_start;
        break;

    default:
        break;
    }

    line_box.y0 = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &fpy0);
    line_box.y1 = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &fpy1);

    /* map together into 3-D world */
    if(cp->d3.bits.use)
    {
        gr_map_point((P_GR_POINT) &line_box.x0, cp, lcp->plotindex);
        gr_map_point((P_GR_POINT) &line_box.x1, cp, lcp->plotindex);
    }

    /* map into absolute plot area */
    line_box.x0 += cp->plotarea.posn.x;
    line_box.y0 += cp->plotarea.posn.y;
    line_box.x1 += cp->plotarea.posn.x;
    line_box.y1 += cp->plotarea.posn.y;

    /* introduce (small) z shift for non-100% depth centring */
    z_frac = (100.0 - lcp->slot_depth_percentage) / 100.0 / 2.0; /* only half you twerp! */

    gr_point_partial_z_shift(&zsv, NULL, cp, &z_frac);

    line_box.x0 += zsv.x;
    line_box.y0 += zsv.y;
    line_box.x1 += zsv.x;
    line_box.y1 += zsv.y;

    id      = lcp->serid;
    id.name = GR_CHART_OBJNAME_BESTFITSER;

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    return(gr_chart_line_new(cp, id, &line_box, &linestyle));
}

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc, client_handle, colID, row)
{
    P_GR_BARLINESCATCH_LINEST_STATE state = (P_GR_BARLINESCATCH_LINEST_STATE) client_handle;
    F64 value;
    GR_DATASOURCE_HANDLE dsh;

    switch(colID)
    {
    case LINEST_A_COLOFF:
        return(0.0);

    case LINEST_Y_COLOFF:
        dsh = state->dsh.value_y;

        gr_travel_dsh_valof(state->cp, dsh, (GR_CHART_ITEMNO) row, &value);

        if(state->y_log)
            value = (value > 0.0) ? log(value) : DBL_MIN;
        break;

    default:
        /* only one x var here */
        assert(0);

    case LINEST_X_COLOFF:
        dsh = state->dsh.value_x;

        if(dsh == GR_DATASOURCE_HANDLE_NONE)
        {
            /* category */
            return(row + 1.0); /* invent sequence of numbers for category starting at 1.0 */
        }

        gr_travel_dsh_valof(state->cp, dsh, (GR_CHART_ITEMNO) row, &value);

        if(state->x_log)
            value = (value > 0.0) ? log(value) : DBL_MIN;
        break;
    }

    return(value);
}

/*
(Robin) Reliant on linest() requesting y data from row = 0 upwards
*/

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc_cumulative, client_handle, colID, row)
{
    P_GR_BARLINESCATCH_LINEST_STATE state = (P_GR_BARLINESCATCH_LINEST_STATE) client_handle;
    F64 value;
    GR_DATASOURCE_HANDLE dsh;

    switch(colID)
    {
    case LINEST_A_COLOFF:
        return(0.0);

    case LINEST_Y_COLOFF:
        dsh = state->dsh.value_y;

        if(gr_travel_dsh_valof(state->cp, dsh, (GR_CHART_ITEMNO) row, &value))
            if(value > 0.0)
            {
                state->y_cum += value;
                value = state->y_cum;
            }

        if(state->y_log)
            value =  (value > 0.0) ? log(value) : DBL_MIN;
        break;

    default:
        /* only one x var here */
        assert(0);

    case LINEST_X_COLOFF:
        dsh = state->dsh.value_x;

        if(dsh == GR_DATASOURCE_HANDLE_NONE)
        {
            /* category */
            return(row + 1.0); /* invent sequence of numbers for category starting at 1.0 */
        }

        gr_travel_dsh_valof(state->cp, dsh, (GR_CHART_ITEMNO) row, &value);

        if(state->x_log)
            value = (value > 0.0) ? log(value) : DBL_MIN;
        break;
    }

    return(value);
}

PROC_LINEST_DATA_PUT_PROTO(extern, gr_barlinescatch_linest_putproc, client_handle, colID, row, value)
{
    P_GR_BARLINESCATCH_LINEST_STATE state = (P_GR_BARLINESCATCH_LINEST_STATE) client_handle;

    switch(colID)
    {
    case LINEST_A_COLOFF:
        state->a[row] = value;
        break;

    default: default_unhandled();
        break;
    }

    return(1);
}

/* end of gr_scatc.c */
