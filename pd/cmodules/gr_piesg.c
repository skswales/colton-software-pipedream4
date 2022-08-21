/* gr_piesg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module to handle pie charts */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

static F64 /*radians*/
reduce_into_range(
    _In_        F64 alpha)
{
    while(alpha > _pi)
        alpha -= _two_pi;

    while(alpha < -_pi)
        alpha += _two_pi;

    return(alpha);
}

static F64 /*radians*/
conv_heading_to_angle(
    _InVal_     F64 heading_degrees_in)
{
    F64 heading_degrees = heading_degrees_in;
    F64 heading_radians = heading_degrees * _radians_per_degree;
    F64 alpha;

    while(heading_radians < 0.0)
        heading_radians += _two_pi;

    /* if heading <= 90 or >= 270 it's +ve between 0 and pi */
    /* if heading > 90 and < 270 it's -ve between 0 and pi */
    alpha = _pi_div_two - heading_radians;

    alpha = reduce_into_range(alpha);

    trace_2(TRACE_MODULE_GR_CHART, "conv_heading_to_angle(%g degrees) yields %g radians", heading_degrees_in, alpha);

    return(alpha);
}

/******************************************************************************
*
* construct a pie chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_pie_addin(
    P_GR_CHART cp)
{
    GR_SERIES_IDX        series_idx;
    P_GR_SERIES          serp;
    GR_CHART_OBJID       serid;
    GR_POINT origin, thisOrigin;
    GR_PIECHDISPLSTYLE   piechdisplstyle;
    GR_PIECHLABELSTYLE   piechlabelstyle;
    F64                  pct_radial_disp_max, point_radial_displacement, base_radial_displacement;
    GR_COORD             radius;
    GR_POINT_NO          point, n_points;
    GR_CHART_NUMBER      total, value;
    GR_CHART_OBJID       id;
    GR_DIAG_OFFSET       pieStart;
    GR_DATASOURCE_HANDLE dsh;
    F64                  alpha, beta, bisector;
    GR_LINESTYLE         linestyle;
    GR_FILLSTYLE         fillstyle;
    GR_TEXTSTYLE         textstyle;
    S32                  numval, labelling;
    STATUS res = STATUS_DONE;

    series_idx = cp->pie_series_idx;
    serp = getserp(cp, series_idx);

    /* always centre in plot area */
    origin.x = cp->plotarea.posn.x + cp->plotarea.size.x / 2;
    origin.y = cp->plotarea.posn.y + cp->plotarea.size.y / 2;

    /* query point 1 for base textstyle for pie */
    gr_point_textstyle_query(cp, series_idx, 1, &textstyle);

    /* use about 90% of the available area */
    radius  = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / 2;
    if(radius > textstyle.height)
        radius -= textstyle.height;
    radius *= 90;
    radius /= 100;

    /* note that there is no point using gr_travel_total_n_items as
     * i)  there is only one series being plotted
     * ii) if there were more category labels than values in S1
     *     then the values would all be zero anyhow
    */
    dsh = gr_travel_series_dsh_from_ds(cp, series_idx, 0);

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
        n_points = 0;
    else
        n_points = gr_travel_dsh_n_items(cp, dsh);

    if(!n_points)
    {
        gr_chartedit_warning(cp, create_error(GR_CHART_ERR_NOT_ENOUGH_INPUT));
        return(res);
    }

    /* find how we are going to slice up the pie */
    total = 0.0;
    pct_radial_disp_max = 0.0;

    for(point = 0; point < n_points; ++point)
    {
        if(gr_travel_dsh_valof(cp, dsh, point, &value))
        {
            if(value > 0.0)
            {
                total += value;
                gr_point_piechdisplstyle_query(cp, series_idx, point, &piechdisplstyle);
                pct_radial_disp_max = MAX(pct_radial_disp_max, piechdisplstyle.radial_displacement);
            }
            else
                gr_chartedit_warning(cp, create_error(GR_CHART_ERR_NEGATIVE_OR_ZERO_IGNORED));
        }
    }

    if(total == 0.0)
    {
        gr_chartedit_warning(cp, create_error(GR_CHART_ERR_NOT_ENOUGH_INPUT));
        return(res);
    }

    base_radial_displacement = serp->style.point_piechdispl.radial_displacement;

    pct_radial_disp_max += base_radial_displacement;

    gr_chart_objid_from_series_idx(cp, series_idx, &serid);

    status_return(res = gr_chart_group_new(cp, &pieStart, serid));

    id           = serid;
    id.name      = GR_CHART_OBJNAME_POINT;
    id.has_subno = 1;

    alpha = conv_heading_to_angle(serp->style.pie_start_heading_degrees);

    /* reduce radius correspondingly */
    radius = (GR_COORD) (radius / (1.0 + pct_radial_disp_max / 100.0));

    for(point = 0; point < n_points; ++point)
    {
        id.subno = (U16) gr_point_external_from_key(point);

        numval = gr_travel_dsh_valof(cp, dsh, point, &value);

        if(numval && (value > 0.0))
        {
            GR_DIAG_OFFSET pointStart = 0;
            F64 fraction = value / total;
            F64 theta = _two_pi * fraction;

            if(serp->bits.pie_anticlockwise)
                beta = alpha + theta;
            else
                beta = alpha - theta;

            bisector = (alpha + beta) / 2;

            gr_point_fillstyle_query(cp, series_idx, point, &fillstyle);
            gr_point_linestyle_query(cp, series_idx, point, &linestyle);

            gr_point_piechdisplstyle_query(cp, series_idx, point, &piechdisplstyle);
            gr_point_piechlabelstyle_query(cp, series_idx, point, &piechlabelstyle);

            point_radial_displacement = piechdisplstyle.radial_displacement + base_radial_displacement;

            labelling = piechlabelstyle.bits.label_leg | piechlabelstyle.bits.label_val | piechlabelstyle.bits.label_pct;

            thisOrigin = origin;

            if(point_radial_displacement != 0.0)
            {
                thisOrigin.x += (GR_COORD) (radius * point_radial_displacement / 100.0 * cos(bisector));
                thisOrigin.y += (GR_COORD) (radius * point_radial_displacement / 100.0 * sin(bisector));
            }

            if(labelling)
                status_break(res = gr_chart_group_new(cp, &pointStart, id));

            status_break(res = gr_diag_piesector_new(cp->core.p_gr_diag, NULL, id, &thisOrigin, radius, (alpha < beta) ? alpha : beta, (alpha < beta) ? beta : alpha, &linestyle, &fillstyle));

            res = STATUS_DONE;

            if(labelling)
            {
                S32            decimals  = -1;
                S32            eformat   = FALSE;
                PC_U8Z         t_trailer = NULL;
                GR_CHART_VALUE cv;
                HOST_FONT      f;
                GR_MILLIPOINT  swidth_mp;
                GR_COORD       swidth_px;

                gr_point_textstyle_query(cp, series_idx, point, &textstyle);

                f = gr_riscdiag_font_from_textstyle(&textstyle);

                if(piechlabelstyle.bits.label_pct | piechlabelstyle.bits.label_val)
                {
                    /* convert value into %ge value */
                    if(piechlabelstyle.bits.label_pct)
                    {
                        value *= 100.0; /* care with order else 14.0 -> 0.14 -> ~14.0 */
                        value /= total; /* ALWAYS in 0.0 - 100.0 */
                        decimals = ((S32) value == value) ? 0 : 2;
                        t_trailer  = "%";
                    }
                    else
                    {
                        if(fabs(value) >= U32_MAX)
                            eformat = TRUE;
                        else if(fabs(value) >= 1.0)
                            decimals = ((S32) value == value) ? 0 : 2;
                    }

                    gr_numtostr(cv.data.text, elemof32(cv.data.text),
                                value,
                                eformat,
                                decimals,
                                CH_NULL /* dp_ch */,
                                ','     /* ths_ch */);

                    if(NULL != t_trailer)
                        xstrkat(cv.data.text, elemof32(cv.data.text), t_trailer);
                }
                else
                    gr_travel_categ_label(cp, point, &cv);

                swidth_mp = gr_font_stringwidth(f, cv.data.text);

                gr_riscdiag_font_dispose(&f);

                swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                if(swidth_px)
                {
                    GR_BOX text_box;
                    GR_COORD     width  = swidth_px;
                    GR_COORD     height = textstyle.height;

                    /* move further out relative to actual centre of sector rather than centre of pie */
                    text_box.x0 = thisOrigin.x + (GR_COORD) (radius * 1.10 * cos(bisector));
                    text_box.y0 = thisOrigin.y + (GR_COORD) (radius * 1.10 * sin(bisector));

                    /* shift slightly down to allow for fonts being higher above baseline than deep below it */
                    text_box.y0 -= height / 4;

                    /* place differently according to whether on right or left of centre vertical */
                    if(fabs(reduce_into_range(bisector)) <= _pi_div_two)
                    {
                        text_box.x1 = text_box.x0 + width;
                    }
                    else
                    {
                        /* move start point further out to left */
                        text_box.x1 = text_box.x0;
                        text_box.x0 = text_box.x1 - width;
                    }

                    text_box.y1 = text_box.y0 + (height * 12) / 10;

                    status_break(res = gr_chart_text_new(cp, id, &text_box, cv.data.text, &textstyle));
                }
            }

            if(pointStart)
                gr_chart_group_end(cp, &pointStart);

            /* use this angle as the next segment's start angle */
            alpha = reduce_into_range(beta);
        }
    }

    gr_chart_group_end(cp, &pieStart);

    return(res);
}

/* end of gr_piesg.c */
