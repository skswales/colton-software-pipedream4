/* gr_blgal.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module to handle bar, line and scatter chart galleries */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"    /* includes dbox.h */
#endif

#ifndef __cs_template_h
#include "cs-template.h"/* includes template.h */
#endif

#ifndef                 __tristate_h
#include "cmodules/riscos/tristate.h" /* no includes */
#endif

/*
internal functions
*/

#if RISCOS

typedef struct GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE
{
    P_GR_CHART      cp;
    HOST_WND        window_handle;
    const int *     icon_handles;
    GR_CHARTTYPE    charttype;
    P_GR_CHARTEDITOR cep;
    S32             reflect_modify;

    GR_CHART_OBJID  modifying_id;
    GR_AXIS_IDX     modifying_axes_idx;
    GR_SERIES_IDX   modifying_series_idx;

    struct GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE_BITS
    {
        UBF tristate_cumulative  : 2;                                               /* UNUSED in scatch */
        UBF tristate_point_vary  : 2;
        UBF tristate_fill_axis   : 2; /* UNUSED in barch */                         /* UNUSED in scatch */
        UBF tristate_best_fit    : 2;
        UBF tristate_join_lines  : 2; /* UNUSED in barch */ /* UNUSED in linech */
        UBF tristate_stacked     : 2;                                               /* UNUSED in scatch */
        UBF tristate_pictures    : 2;
        UBF tristate_categ_x_val : 2; /* UNUSED in barch */ /* UNUSED in linech */
    }
    bits;

    GR_BARCHSTYLE      barch;
    S32                barch_modified;      /* must be separate S32 not bitfield */

    GR_LINECHSTYLE     linech;
    S32                linech_modified;     /* must be separate S32 not bitfield */

    GR_BARLINECHSTYLE  barlinech;
    S32                barlinech_modified;  /* must be separate S32 not bitfield */

    GR_PIECHDISPLSTYLE piechdispl;
    S32                piechdispl_modified; /* must be separate S32 not bitfield */

    GR_PIECHLABELSTYLE piechlabel;
    S32                piechlabel_modified; /* must be separate S32 not bitfield */

    GR_SCATCHSTYLE     scatch;
    S32                scatch_modified;     /* must be separate S32 not bitfield */

    struct GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE_PIE_XTRA
    {
        S32 anticlockwise;
        F64 start_heading_degrees;
    }
    pie_xtra;
    S32 pie_xtra_modified;
}
GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE, * P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE;

enum GR_CHARTEDIT_GALLERY_BARLINESCATCH_ICONS
{
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_WIDTH = 0,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_2D_OVERLAP,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_STACK_PICT,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_WIDTH,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_2D_SHIFT,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_BARLINE_3D_DEPTH,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_SCAT_WIDTH,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_ON,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_TURN,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_DROOP,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_REMOVE_OVERLAY,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_CUMULATIVE,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_POINT_VARY,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_FILL_AXIS,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_BEST_FIT,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_JOIN_LINES,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_STACKED,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_PICTURES,
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_CATEG_X_VAL,

    GR_CHARTEDIT_GALLERY_BARLINESCATCH_N_ICONS
};

#if 0
static void
gr_chartedit_gallery_barlinescatch_convert(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*const*/,
    GR_SERIES_TYPE sertype);
#endif

#if 0
static void
gr_chartedit_gallery_barlinescatch_decode(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*inout*/);
#endif

#if 0
static void
gr_chartedit_gallery_barlinescatch_encode(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*const*/);
#endif

#if 0
static void
gr_chartedit_gallery_barlinescatch_getsel(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*out*/);
#endif

#if 0
static S32
gr_chartedit_gallery_barlinescatch_get_pictures(
    P_GR_CHART      cp,
    GR_CHART_OBJID modifying_id,
    GR_CHARTTYPE   charttype);
#endif

#if 0
static void
gr_chartedit_gallery_barlinescatch_kill_pictures(
    P_GR_CHART      cp,
    GR_CHART_OBJID modifying_id);
#endif

#if 0
static S32
gr_chartedit_gallery_barlinescatch_init(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state,
    P_GR_CHARTEDITOR cep,
    dbox *          p_dbox,
    P_U8            dboxname);
#endif

#if 0
static void
gr_chartedit_gallery_barlinescatch_process(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*inout*/,
    dbox_field f);
#endif

static void
gr_chartedit_gallery_undo_pie_bodges(
    P_GR_CHART cp);

#endif

/* ------------------------------------------------------------------------- */

#if RISCOS

/*
useful doubles to point the finger at
*/

static const F64 double_m100 = -100.0;
static const F64 double_1    =    1.0;
static const F64 double_100  =  100.0;

/******************************************************************************
*
* for all series in the chart, allocate pictures to them if not already got them
*
******************************************************************************/

static S32
gr_chartedit_gallery_barlinescatch_get_fillpattern_using_point(
    P_GR_CHART      cp,
    GR_CHART_OBJID id,
    S32            file_id)
{
    GR_ESERIES_NO   eseries_no;
    GR_POINT_NO     epoint_no;
    GR_SERIES_IDX   series_idx;
    GR_FILLSTYLE    fillstyle;
    P_U8            currname;
    char            leafname[BUF_MAX_GENLEAFNAME + MAX_GENLEAFNAME + 1 /* sep_ch */];
    char            filename[BUF_MAX_PATHSTRING];
    IMAGE_CACHE_HANDLE cah;
    S32             using_default;
    S32             res;

    switch(id.name)
    {
    default:
        assert(0);
        return(0);

    case GR_CHART_OBJNAME_POINT:
        break;
    }

    epoint_no  = id.subno;
    eseries_no = id.no;

    series_idx = gr_series_idx_from_external(cp, eseries_no);

    /* look up point's actual non-inherited style; look up SxPy file if no picture of its own */
    using_default = gr_point_fillstyle_query(cp, series_idx, gr_point_key_from_external(epoint_no), &fillstyle);

    if(!using_default && (fillstyle.pattern != GR_FILL_PATTERN_NONE))
        return(1);

    currname = file_is_rooted(cp->core.currentfilename)
                            ? cp->core.currentfilename
                            : NULL;

    /* first try looking in any SxPx directory on the path (or relative) */
    (void) xsnprintf(leafname, elemof32(leafname), string_lookup(file_id + 2), eseries_no, epoint_no); /* "markers.S1Px" */
    {
    const U32 len = strlen(leafname);
    (void) xsnprintf(leafname + len, elemof32(leafname) - len, string_lookup(file_id + 3), epoint_no); /* "markers.S1Px.P2" */
    } /*block*/

    res = (NULL != currname)
        ? file_find_on_path_or_relative(filename, elemof32(filename), file_get_search_path(), leafname, currname)
        : file_find_on_path(filename, elemof32(filename), file_get_search_path(), leafname);

    if(res <= 0)
    {
        (void) xsnprintf(leafname, elemof32(leafname), string_lookup(file_id + 1), eseries_no, epoint_no); /* "markers.S1P2" */

        res = (NULL != currname)
            ? file_find_on_path_or_relative(filename, elemof32(filename), file_get_search_path(), leafname, currname)
            : file_find_on_path(filename, elemof32(filename), file_get_search_path(), leafname);
    }

    if(res <= 0)
        return(0);

    res = image_cache_entry_ensure(&cah, filename);

    if(res <= 0)
        return(0);

    fillstyle.pattern = (GR_FILL_PATTERN_HANDLE) cah;

    fillstyle.bits.notsolid = 1;
    fillstyle.bits.pattern  = 1;

    /* if loaded from Pictures, recolouring is off by default */
    fillstyle.bits.norecolour = (file_id == GR_CHART_MSG_FILES_SERIES_PICTURE);

    /* if loaded from Pictures, isotropic is off by default */
    fillstyle.bits.isotropic  = (file_id != GR_CHART_MSG_FILES_SERIES_PICTURE);

    fillstyle.fg.manual = 1;

    return(gr_chart_objid_fillstyle_set(cp, id, &fillstyle));
}

static S32
gr_chartedit_gallery_barlinescatch_get_fillpattern_using_series(
    P_GR_CHART      cp,
    GR_CHART_OBJID id,
    S32            file_id,
    S32            msg_id)
{
    GR_ESERIES_NO   eseries_no;
    GR_SERIES_IDX   series_idx;
    GR_FILLSTYLE    fillstyle;
    P_U8            currname;
    char            leafname[BUF_MAX_GENLEAFNAME + MAX_GENLEAFNAME + 1 /* sep_ch */];
    char            filename[BUF_MAX_PATHSTRING];
    IMAGE_CACHE_HANDLE cah;
    S32             res;

    switch(id.name)
    {
    default:
        assert(0);
        return(1);

    case GR_CHART_OBJNAME_POINT:
        if(status_fail(res = gr_chartedit_gallery_barlinescatch_get_fillpattern_using_point(cp, id, file_id)))
            return(0);

        if(status_done(res))
            return(res);

        /* deliberate drop thru ... */

    case GR_CHART_OBJNAME_SERIES:
        eseries_no = id.no;

        series_idx = gr_series_idx_from_external(cp, eseries_no);

        gr_chart_objid_fillstyle_query(cp, id, &fillstyle);

        if(fillstyle.pattern == GR_FILL_PATTERN_NONE)
        {
            /* keep it if already set, just twiddle bits */

            currname = file_is_rooted(cp->core.currentfilename)
                                    ? cp->core.currentfilename
                                    : NULL;

            (void) xsnprintf(leafname, elemof32(leafname), string_lookup(file_id), eseries_no); /* "markers.S1 */

            res = (NULL != currname)
                ? file_find_on_path_or_relative(filename, elemof32(filename), file_get_search_path(), leafname, currname)
                : file_find_on_path(filename, elemof32(filename), file_get_search_path(), leafname);

            if(res <= 0)
            {
                messagef(string_lookup(msg_id), eseries_no, eseries_no);

                /* use series 1 markers */
                (void) xsnprintf(leafname, elemof32(leafname), string_lookup(file_id), 1);

                res = (NULL != currname)
                    ? file_find_on_path_or_relative(filename, elemof32(filename), file_get_search_path(), leafname, currname)
                    : file_find_on_path(filename, elemof32(filename), file_get_search_path(), leafname);

                if(res <= 0)
                    return(0);
            }

            res = image_cache_entry_ensure(&cah, filename);

            if(res <= 0)
                return(0);

            fillstyle.pattern = (GR_FILL_PATTERN_HANDLE) cah;
        }
        break;
    }

    fillstyle.bits.notsolid = 1;
    fillstyle.bits.pattern  = 1;

    /* if loaded from Pictures, recolouring is off by default */
    fillstyle.bits.norecolour = (file_id == GR_CHART_MSG_FILES_SERIES_PICTURE);

    /* if loaded from Pictures, isotropic is off by default */
    fillstyle.bits.isotropic  = (file_id != GR_CHART_MSG_FILES_SERIES_PICTURE);

    fillstyle.fg.manual = 1;

    status_return(gr_chart_objid_fillstyle_set(cp, id, &fillstyle));

    switch(id.name)
    {
    default:
        break;

    case GR_CHART_OBJNAME_SERIES:
        {
        /* if vary by point try looking up SxPy files */
        P_GR_SERIES serp = getserp(cp, series_idx);

        if(serp->bits.point_vary_manual
                        ? serp->bits.point_vary
                        : gr_axesp_from_series_idx(cp, series_idx)->bits.point_vary)
        {
            GR_CHART_OBJID point_id;
            GR_POINT_NO    point, n_points;

            point_id           = id;
            point_id.name      = GR_CHART_OBJNAME_POINT;
            point_id.has_subno = 1;

            n_points = gr_travel_series_n_items_total(cp, series_idx);

            for(point = 1; point < n_points; ++point)
            {
                point_id.subno = (U16) gr_point_external_from_key(point);

                res = gr_chartedit_gallery_barlinescatch_get_fillpattern_using_point(cp, point_id, file_id);

                if(res <= 0)
                    break;
            }
        }
        break;
        }
    }

    return(1);
}

static S32
gr_chartedit_gallery_barlinescatch_get_pictures(
    P_GR_CHART      cp,
    GR_CHART_OBJID modifying_id,
    GR_CHARTTYPE   charttype)
{
    S32 is_bar  = (charttype == GR_CHARTTYPE_BAR);
    S32 file_id = is_bar ? GR_CHART_MSG_FILES_SERIES_PICTURE : GR_CHART_MSG_FILES_SERIES_MARKER;
    S32 msg_id  = is_bar ? GR_CHART_MSG_WINGE_SERIES_PICTURE : GR_CHART_MSG_WINGE_SERIES_MARKER;

    switch(modifying_id.name)
    {
    case GR_CHART_OBJNAME_AXIS:
        {
        GR_AXES_IDX     axes_idx;
        GR_SERIES_IDX   series_idx;
        GR_CHART_OBJID id;
        S32            res;

        (void) gr_axes_idx_from_external(cp, modifying_id.no, &axes_idx);

        for(series_idx = cp->axes[axes_idx].series.stt_idx;
            series_idx < cp->axes[axes_idx].series.end_idx;
            series_idx++)
        {
            gr_chart_objid_from_series_idx(cp, series_idx, &id);
            if(0 >= (res =
                gr_chartedit_gallery_barlinescatch_get_fillpattern_using_series(
                    cp, id, file_id, msg_id)))
                    return(res);
        }
        return(1);
        }

    default:
        #ifndef NDEBUG
        assert(0);
        return(1);
        #endif
    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
        return(gr_chartedit_gallery_barlinescatch_get_fillpattern_using_series(
                    cp, modifying_id, file_id, msg_id));
    }
}

static void
gr_chartedit_gallery_barlinescatch_kill_pictures(
    P_GR_CHART      cp,
    GR_CHART_OBJID modifying_id)
{
    GR_FILLSTYLE fillstyle;

    switch(modifying_id.name)
    {
    case GR_CHART_OBJNAME_AXIS:
        {
        GR_AXES_IDX    axes_idx;
        GR_SERIES_IDX  series_idx;
        GR_CHART_OBJID id;

        (void) gr_axes_idx_from_external(cp, modifying_id.no, &axes_idx);

        for(series_idx = cp->axes[axes_idx].series.stt_idx;
            series_idx < cp->axes[axes_idx].series.end_idx;
            series_idx++)
        {
            gr_chart_objid_from_series_idx(cp, series_idx, &id);

            gr_chartedit_gallery_barlinescatch_kill_pictures(cp, id);
        }
        break;
        }

    default:
    #ifndef NDEBUG
        assert(0);
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
    #endif
        gr_chart_objid_fillstyle_query(cp, modifying_id, &fillstyle);

        if(fillstyle.pattern != GR_FILL_PATTERN_NONE)
        {
            fillstyle.bits.pattern = 0;
            fillstyle.pattern      = GR_FILL_PATTERN_NONE;
            fillstyle.fg.manual    = 1;
            status_consume(gr_chart_objid_fillstyle_set(cp, modifying_id, &fillstyle));
        }
        break;
    }
}

static void
gr_chartedit_gallery_scatch_lines_onoff(
    P_GR_CHART      cp,
    GR_CHART_OBJID id,
    S32            lines_on)
{
    GR_SCATCHSTYLE scatchstyle;
    S32            lines_off = !lines_on;

    gr_chart_objid_scatchstyle_query(cp, id, &scatchstyle);

    if(scatchstyle.bits.lines_off != lines_off)
    {
        scatchstyle.bits.lines_off = lines_off;

        scatchstyle.bits.manual = 1;
        status_consume(gr_chart_objid_scatchstyle_set(cp, id, &scatchstyle));
    }
}

/******************************************************************************
*
* general gallery processing
*
******************************************************************************/

static const F64 pct_min_limit =   0.0;
static const F64 pct_max_limit = 100.0;
static const S32 pct_decplaces =     1;
static const F64 pct_bumpvalue =   1.0;

static const F64 turn_min_limit  =  0.0;
static const F64 turn_max_limit  = 90.0;
static const S32 turn_decplaces  =    1;

static const F64 droop_min_limit =  0.0;
static const F64 droop_max_limit = 75.0;
static const S32 droop_decplaces =    1;

static const F64 pct_radial_dsp_increment =    1.0;
static const F64 pct_radial_dsp_min_limit =    0.0;
static const F64 pct_radial_dsp_max_limit = 1000.0;
static const S32 pct_radial_dsp_decplaces =      1;

static const F64 pie_heading_increment    =    1.0;
static const F64 pie_heading_min_limit    =    0.0;
static const F64 pie_heading_max_limit    =  360.0;
static const S32 pie_heading_decplaces    =      1;

static void
gr_chartedit_gallery_barlinescatch_convert(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*const*/,
    GR_SERIES_TYPE sertype)
{
    P_GR_CHART    cp;
    P_GR_AXES     p_axes;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;
    S32          sertype_modified = 0;

    cp = state->cp;

    state->reflect_modify = 1;

    switch(state->modifying_id.name)
    {
    case GR_CHART_OBJNAME_AXIS:
        p_axes = &cp->axes[state->modifying_axes_idx];

        /* make all series on selected axes part of a xxx chart */
        p_axes->charttype = state->charttype;
        p_axes->sertype   = sertype;

        /* ensure all series currently on these axes get blotted */
        for(series_idx = p_axes->series.stt_idx;
            series_idx < p_axes->series.end_idx;
            series_idx++)
        {
            serp = getserp(cp, series_idx);

            serp->charttype = GR_CHARTTYPE_NONE;

            serp->sertype = sertype;
            * (int *) &serp->valid = 0;
            sertype_modified = 1;
        }
        break;

    default:
        assert(0);
    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
        serp = getserp(cp, state->modifying_series_idx);

        serp->charttype = state->charttype;

        serp->sertype = sertype;
        * (int *) &serp->valid = 0;
        sertype_modified = 1;
        break;
    }

    if(sertype_modified)
    {
        /* kill current pictures prior to reshuffle */
        if(state->bits.tristate_pictures == RISCOS_TRISTATE_OFF)
            gr_chartedit_gallery_barlinescatch_kill_pictures(cp, state->modifying_id);

        /* need to reshuffle and reallocate */
        gr_chart_realloc_series(cp);
    }
}

static void
gr_chartedit_gallery_barlinescatch_decode(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*inout*/)
{
    P_GR_CHART cp = state->cp;
    const HOST_WND window_handle = state->window_handle;
    const int * const icon_handles = state->icon_handles;
    int icon_handle;
    STATUS res;

    if(NULL != icon_handles)
    {
        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_WIDTH];
        if(icon_handle)
            winf_checkdouble(window_handle, icon_handle,
                             &state->barch.slot_width_percentage,
                             &state->barch_modified,
                             &pct_min_limit, &pct_max_limit, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_2D_OVERLAP];
        if(icon_handle && !cp->d3.bits.on)
            winf_checkdouble(window_handle, icon_handle,
                             &cp->barch.slot_overlap_percentage,
                             NULL,
                             &double_m100, &double_100, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_WIDTH];
        if(icon_handle)
            winf_checkdouble(window_handle, icon_handle,
                             &state->linech.slot_width_percentage,
                             &state->linech_modified,
                             &pct_min_limit, &pct_max_limit, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_2D_SHIFT];
        if(icon_handle && !cp->d3.bits.on)
            winf_checkdouble(window_handle, icon_handle,
                             &cp->linech.slot_shift_percentage,
                             NULL,
                             &double_m100, &double_100, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BARLINE_3D_DEPTH];
        if(icon_handle && cp->d3.bits.on)
            winf_checkdouble(window_handle, icon_handle,
                             &state->barlinech.slot_depth_percentage,
                             &state->barlinech_modified,
                             &pct_min_limit, &pct_max_limit, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_SCAT_WIDTH];
        if(icon_handle)
            winf_checkdouble(window_handle, icon_handle,
                             &state->scatch.width_percentage,
                             &state->scatch_modified,
                             &pct_min_limit, &pct_max_limit, pct_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_TURN];
        if(icon_handle && cp->d3.bits.on)
            winf_checkdouble(window_handle, icon_handle,
                             &cp->d3.turn,
                             NULL,
                             &turn_min_limit, &turn_max_limit, turn_decplaces);

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_DROOP];
        if(icon_handle && cp->d3.bits.on)
            winf_checkdouble(window_handle, icon_handle,
                             &cp->d3.droop,
                             NULL,
                             &droop_min_limit, &droop_max_limit, droop_decplaces);
    }
    else if(state->charttype == GR_CHARTTYPE_PIE)
    {
        icon_handle = GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL;
        if(icon_handle)
            winf_checkdouble(window_handle, icon_handle,
                             &state->piechdispl.radial_displacement,
                             &state->piechdispl_modified,
                             &pct_radial_dsp_min_limit,
                             &pct_radial_dsp_max_limit,
                              pct_radial_dsp_decplaces);

        icon_handle = GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL;
        if(icon_handle)
            winf_checkdouble(window_handle, icon_handle,
                             &state->pie_xtra.start_heading_degrees,
                             &state->pie_xtra_modified,
                             &pie_heading_min_limit,
                             &pie_heading_max_limit,
                              pie_heading_decplaces);
    }

    if(state->reflect_modify)
    {
        state->reflect_modify = 0; /* no point doing it again unless subsequently changed */

        if(state->bits.tristate_cumulative != RISCOS_TRISTATE_DONT_CARE)
        {
            S32          cumulative;
            P_GR_AXES     p_axes;
            GR_SERIES_IDX series_idx;
            P_GR_SERIES   serp;

            cumulative = (state->bits.tristate_cumulative == RISCOS_TRISTATE_ON);

            state->bits.tristate_cumulative = RISCOS_TRISTATE_DONT_CARE;

            switch(state->modifying_id.name)
            {
            case GR_CHART_OBJNAME_AXIS:
                p_axes = &cp->axes[state->modifying_axes_idx];

                p_axes->bits.cumulative = cumulative;

                /* blow away serp->valid.use_xxx */
                for(series_idx = p_axes->series.stt_idx;
                    series_idx < p_axes->series.end_idx;
                    series_idx++)
                {
                    serp = getserp(cp, series_idx);

                    /* blow away serp->valid.use_xxx */
                    * (int *) &serp->valid = 0;
                }
                break;

            default:
                serp = getserp(cp, state->modifying_series_idx);

                /* blow away serp->valid.use_xxx */
                * (int *) &serp->valid = 0;

                serp->bits.cumulative        = cumulative;
                serp->bits.cumulative_manual = 1;
                break;
            }
        }

        if(state->bits.tristate_point_vary != RISCOS_TRISTATE_DONT_CARE)
        {
            S32        point_vary;
            P_GR_SERIES serp;

            point_vary = (state->bits.tristate_point_vary == RISCOS_TRISTATE_ON);

            state->bits.tristate_point_vary = RISCOS_TRISTATE_DONT_CARE;

            switch(state->modifying_id.name)
            {
            case GR_CHART_OBJNAME_AXIS:
                cp->axes[state->modifying_axes_idx].bits.point_vary = point_vary;
                break;

            default:
                serp = getserp(cp, state->modifying_series_idx);

                serp->bits.point_vary        = point_vary;
                serp->bits.point_vary_manual = 1;
                break;
            }
        }

        if(state->bits.tristate_fill_axis != RISCOS_TRISTATE_DONT_CARE)
        {
            S32        fill_axis;
            P_GR_SERIES serp;

            fill_axis = (state->bits.tristate_fill_axis == RISCOS_TRISTATE_ON);

            state->bits.tristate_fill_axis = RISCOS_TRISTATE_DONT_CARE;

            switch(state->modifying_id.name)
            {
            case GR_CHART_OBJNAME_AXIS:
                cp->axes[state->modifying_axes_idx].bits.fill_axis = fill_axis;
                break;

            default:
                serp = getserp(cp, state->modifying_series_idx);

                serp->bits.fill_axis        = fill_axis;
                serp->bits.fill_axis_manual = 1;
                break;
            }
        }

        if(state->bits.tristate_best_fit != RISCOS_TRISTATE_DONT_CARE)
        {
            S32        best_fit;
            P_GR_SERIES serp;

            best_fit = (state->bits.tristate_best_fit == RISCOS_TRISTATE_ON);

            state->bits.tristate_best_fit = RISCOS_TRISTATE_DONT_CARE;

            switch(state->modifying_id.name)
            {
            case GR_CHART_OBJNAME_AXIS:
                cp->axes[state->modifying_axes_idx].bits.best_fit = best_fit;
                break;

            default:
                serp = getserp(cp, state->modifying_series_idx);

                serp->bits.best_fit        = best_fit;
                serp->bits.best_fit_manual = 1;
                break;
            }
        }

        if(state->bits.tristate_join_lines != RISCOS_TRISTATE_DONT_CARE)
        {
            S32 join_lines;

            join_lines = (state->bits.tristate_join_lines == RISCOS_TRISTATE_ON);

            state->bits.tristate_join_lines = RISCOS_TRISTATE_DONT_CARE;

            gr_chartedit_gallery_scatch_lines_onoff(cp, state->modifying_id, join_lines);
        }

        if(state->bits.tristate_stacked != RISCOS_TRISTATE_DONT_CARE)
        {
            S32          stacked;
            P_GR_AXES     p_axes;
            GR_SERIES_IDX series_idx;
            P_GR_SERIES   serp;

            stacked = (state->bits.tristate_stacked == RISCOS_TRISTATE_ON);

            state->bits.tristate_stacked = RISCOS_TRISTATE_DONT_CARE;

            p_axes = &cp->axes[state->modifying_axes_idx];

            p_axes->bits.stacked = stacked;

            /* blow away serp->valid.use_xxx */
            for(series_idx = p_axes->series.stt_idx;
                series_idx < p_axes->series.end_idx;
                series_idx++)
            {
                serp = getserp(cp, series_idx);

                * (int *) &serp->valid = 0;
            }
        }

        if(state->bits.tristate_pictures != RISCOS_TRISTATE_DONT_CARE)
        {
            S32 pictures;

            pictures = (state->bits.tristate_pictures == RISCOS_TRISTATE_ON);

            state->bits.tristate_pictures = RISCOS_TRISTATE_DONT_CARE;

            if(pictures)
                gr_chartedit_gallery_barlinescatch_get_pictures(cp, state->modifying_id, state->charttype);
            else
                gr_chartedit_gallery_barlinescatch_kill_pictures(cp, state->modifying_id);
        }

        if(state->bits.tristate_categ_x_val != RISCOS_TRISTATE_DONT_CARE)
        {
            S32 categ_x_val;

            categ_x_val = (state->bits.tristate_categ_x_val == RISCOS_TRISTATE_ON);

            state->bits.tristate_categ_x_val = RISCOS_TRISTATE_DONT_CARE;

            if(categ_x_val)
                ;
            else
                ;
        }

        res = 1;

        for(;;)
        {
            if(state->barch_modified)
            {
                state->barch_modified = 0;
                state->barch.bits.manual = 1;
                status_break(res = gr_chart_objid_barchstyle_set(cp, state->modifying_id, &state->barch));
            }

            if(state->linech_modified)
            {
                state->linech_modified = 0;
                state->linech.bits.manual = 1;
                status_break(res = gr_chart_objid_linechstyle_set(cp, state->modifying_id, &state->linech));
            }

            if(state->barlinech_modified)
            {
                state->barlinech_modified = 0;
                state->barlinech.bits.manual = 1;
                status_break(res = gr_chart_objid_barlinechstyle_set(cp, state->modifying_id, &state->barlinech));
            }

            if(state->piechdispl_modified)
            {
                state->piechdispl_modified = 0;
                state->piechdispl.bits.manual = 1;
                status_break(res = gr_chart_objid_piechdisplstyle_set(cp, state->modifying_id, &state->piechdispl));
            }

            if(state->piechlabel_modified)
            {
                state->piechlabel_modified = 0;
                state->piechlabel.bits.manual = 1;
                status_break(res = gr_chart_objid_piechlabelstyle_set(cp, state->modifying_id, &state->piechlabel));
            }

            if(state->scatch_modified)
            {
                state->scatch_modified = 0;
                state->scatch.bits.manual = 1;
                status_break(res = gr_chart_objid_scatchstyle_set(cp, state->modifying_id, &state->scatch));
            }

            if(state->pie_xtra_modified)
            {
                /* currently bash one series */
                P_GR_SERIES serp;

                state->pie_xtra_modified = 0;

                serp = getserp(cp, state->modifying_series_idx);

                serp->bits.pie_anticlockwise = state->pie_xtra.anticlockwise;
                serp->style.pie_start_heading_degrees = state->pie_xtra.start_heading_degrees;
            }

            break;
        }

        if(status_fail(res))
            gr_chartedit_winge(res);

        gr_chart_modify_and_rebuild(&state->cep->ch);
    }
}

static const F64
pie_start_headings[8] =
{
      0.0,  45.0,  90.0, 135.0,
    180.0, 225.0, 270.0, 315.0
};

static void
gr_chartedit_gallery_barlinescatch_encode(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*const*/)
{
    P_GR_CHART cp = state->cp;
    const HOST_WND window_handle = state->window_handle;
    const int * const icon_handles = state->icon_handles;
    int icon_handle;
    void (* unfadeproc2d) (_HwndRef_ HOST_WND window_handle, _InVal_ int icon_handle);
    void (* unfadeproc3d) (_HwndRef_ HOST_WND window_handle, _InVal_ int icon_handle);
    void (* unfadeprocRO) (_HwndRef_ HOST_WND window_handle, _InVal_ int icon_handle);

    /* on if 2-D */
    unfadeproc2d = cp->d3.bits.on ? winf_fadefield : winf_unfadefield;

    /* on if 3-D */
    unfadeproc3d = cp->d3.bits.on ? winf_unfadefield : winf_fadefield;

    /* on iff overlay present */
    unfadeprocRO = (cp->axes_idx_max > 0) ? winf_unfadefield : winf_fadefield;

    if(state->charttype == GR_CHARTTYPE_PIE)
    {
        U32 pie_idx;

        winf_setdouble(window_handle, GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL,
                       &state->piechdispl.radial_displacement, pct_radial_dsp_decplaces);

        winf_setonoff(window_handle, GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_ANTICLOCKWISE,
                      state->pie_xtra.anticlockwise);

        winf_setdouble(window_handle, GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL,
                       &state->pie_xtra.start_heading_degrees, pie_heading_decplaces);

        for(pie_idx = 0; pie_idx < elemof32(pie_start_headings); ++pie_idx)
            winf_setonoff(window_handle, pie_idx + GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_000,
                          (state->pie_xtra.start_heading_degrees == pie_start_headings[pie_idx]));

        return;
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_WIDTH];
    if(icon_handle)
        winf_setdouble(window_handle, icon_handle, &state->barch.slot_width_percentage, pct_decplaces);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_2D_OVERLAP];
    if(icon_handle)
    {
        (* unfadeproc2d) (window_handle, icon_handle - 2); /* triplet field */
        (* unfadeproc2d) (window_handle, icon_handle - 1);
        (* unfadeproc2d) (window_handle, icon_handle);

        winf_setdouble(window_handle, icon_handle, &cp->barch.slot_overlap_percentage, pct_decplaces);
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_STACK_PICT];
    if(icon_handle)
        winf_setonoff(window_handle, icon_handle, state->barch.bits.pictures_stacked);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_WIDTH];
    if(icon_handle)
        winf_setdouble(window_handle, icon_handle, &state->linech.slot_width_percentage, pct_decplaces);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_2D_SHIFT];
    if(icon_handle)
    {
        (* unfadeproc2d) (window_handle, icon_handle - 2); /* triplet field */
        (* unfadeproc2d) (window_handle, icon_handle - 1);
        (* unfadeproc2d) (window_handle, icon_handle);

        winf_setdouble(window_handle, icon_handle, &cp->linech.slot_shift_percentage, pct_decplaces);
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BARLINE_3D_DEPTH];
    if(icon_handle)
    {
        (* unfadeproc3d) (window_handle, icon_handle - 2); /* triplet field */
        (* unfadeproc3d) (window_handle, icon_handle - 1);
        (* unfadeproc3d) (window_handle, icon_handle);

        winf_setdouble(window_handle, icon_handle, &state->barlinech.slot_depth_percentage, pct_decplaces);
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_SCAT_WIDTH];
    if(icon_handle)
        winf_setdouble(window_handle, icon_handle, &state->scatch.width_percentage, pct_decplaces);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_ON];
    if(icon_handle)
        winf_setonoff(window_handle, icon_handle, cp->d3.bits.on);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_TURN];
    if(icon_handle)
    {
        (* unfadeproc3d) (window_handle, icon_handle - 2); /* triplet field */
        (* unfadeproc3d) (window_handle, icon_handle - 1);
        (* unfadeproc3d) (window_handle, icon_handle);

        winf_setdouble(window_handle, icon_handle, &cp->d3.turn, turn_decplaces);
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_DROOP];
    if(icon_handle)
    {
        (* unfadeproc3d) (window_handle, icon_handle - 2); /* triplet field */
        (* unfadeproc3d) (window_handle, icon_handle - 1);
        (* unfadeproc3d) (window_handle, icon_handle);

        winf_setdouble(window_handle, icon_handle, &cp->d3.droop, droop_decplaces);
    }

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_REMOVE_OVERLAY];
    if(icon_handle)
        (* unfadeprocRO) (window_handle, icon_handle);

    /* tristate section */

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_CUMULATIVE];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_cumulative);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_POINT_VARY];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_point_vary);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_FILL_AXIS];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_fill_axis);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BEST_FIT];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_best_fit);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_JOIN_LINES];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_join_lines);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_STACKED];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_stacked);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_PICTURES];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_pictures);

    icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_CATEG_X_VAL];
    if(icon_handle)
        riscos_tristate_set(window_handle, icon_handle, state->bits.tristate_categ_x_val);
}

/******************************************************************************
*
* suss the selection to use for modifying a chart style
*
******************************************************************************/

static void
gr_chartedit_gallery_barlinescatch_getsel(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*out*/)
{
    P_GR_CHART cp;

    cp = state->cp;

    /* ensure temporary selection not used */
    state->modifying_id = state->cep->selection.temp
                                    ? gr_chart_objid_anon
                                    : state->cep->selection.id;

    switch(state->modifying_id.name)
    {
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_DROPSER:
        state->modifying_id.name = GR_CHART_OBJNAME_SERIES;
        goto setup_series_or_point;

    case GR_CHART_OBJNAME_DROPPOINT:
        state->modifying_id.name = GR_CHART_OBJNAME_POINT;

    /* deliberate drop thru ... */

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
    setup_series_or_point:;
        state->modifying_series_idx = gr_series_idx_from_external(cp, state->modifying_id.no);
        state->modifying_axes_idx   = gr_axes_idx_from_series_idx(cp, state->modifying_series_idx);
        if(state->charttype == GR_CHARTTYPE_PIE)
            cp->pie_series_idx = state->modifying_series_idx;
        break;

    default:
        /* if unsure, modify first axes set */
        gr_chart_objid_from_axes_idx(cp, 0, 0, &state->modifying_id);
        goto setup_axis;

    case GR_CHART_OBJNAME_AXISTICK:
    case GR_CHART_OBJNAME_AXISGRID:
        state->modifying_id.name      = GR_CHART_OBJNAME_AXIS;
        state->modifying_id.has_subno = 0;
        state->modifying_id.subno     = 0;

    /* deliberate drop thru ... */

    case GR_CHART_OBJNAME_AXIS:
    setup_axis:;
        (void) gr_axes_idx_from_external(cp, state->modifying_id.no, &state->modifying_axes_idx);

        if(state->charttype == GR_CHARTTYPE_PIE)
        {
            /* if unsure, modify Series 1 */
            gr_chart_objid_from_series_idx(cp, 0, &state->modifying_id);
            goto setup_series_or_point;
        }
        break;
    }

    /* read current state */

    if(state->charttype == GR_CHARTTYPE_BAR)
        gr_chart_objid_barchstyle_query(     cp, state->modifying_id, &state->barch);

    if(state->charttype == GR_CHARTTYPE_LINE)
        gr_chart_objid_linechstyle_query(    cp, state->modifying_id, &state->linech);

    if((state->charttype == GR_CHARTTYPE_BAR) || (state->charttype == GR_CHARTTYPE_LINE))
        gr_chart_objid_barlinechstyle_query( cp, state->modifying_id, &state->barlinech);

    if(state->charttype == GR_CHARTTYPE_PIE)
    {
        /* currently read one series */
        P_GR_SERIES serp;

        gr_chart_objid_piechdisplstyle_query(cp, state->modifying_id, &state->piechdispl);
        gr_chart_objid_piechlabelstyle_query(cp, state->modifying_id, &state->piechlabel);

        serp = getserp(cp, state->modifying_series_idx);

        state->pie_xtra.anticlockwise = serp->bits.pie_anticlockwise;
        state->pie_xtra.start_heading_degrees = serp->style.pie_start_heading_degrees;
    }

    if(state->charttype == GR_CHARTTYPE_SCAT)
        gr_chart_objid_scatchstyle_query(    cp, state->modifying_id, &state->scatch);

    state->barch_modified           = 0;
    state->linech_modified          = 0;
    state->barlinech_modified       = 0;
    state->piechdispl_modified      = 0;
    state->piechlabel_modified      = 0;
    state->pie_xtra_modified        = 0;
    state->scatch_modified          = 0;

    /* tristate on/off bits all reset */
    * (int *) &state->bits          = 0;
    state->bits.tristate_cumulative  = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_point_vary  = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_fill_axis   = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_best_fit    = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_join_lines  = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_stacked     = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_pictures    = RISCOS_TRISTATE_DONT_CARE;
    state->bits.tristate_categ_x_val = RISCOS_TRISTATE_DONT_CARE;
}

static S32
gr_chartedit_gallery_barlinescatch_init(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state,
    P_GR_CHARTEDITOR cep,
    dbox *          p_dbox,
    P_U8            dboxname)
{
    dbox d;
    char * errorp;

    if(*p_dbox)
    {
        const HOST_WND window_handle = dbox_window_handle(*p_dbox);
        WimpGetWindowStateBlock window_state;

        if( !WrapOsErrorReporting_IsError(tbl_wimp_get_window_state_x(window_handle, &window_state)) )
        {   /* send it an Open_Window_Request message */
            wimp_eventstr e;
            memcpy32(&e.data.o, &window_state, sizeof32(WimpOpenWindowRequestEvent));
            e.data.o.behind = (wimp_w) -1; /* force this window to the front */
            void_WrapOsErrorReporting(wimp_send_message_to_window(Wimp_EOpenWindow, &e.data, window_handle, -1));
        } /*block*/

        return(0);
    }

    *p_dbox = d = chart_dbox_new_new(dboxname, &errorp);

    if(!d)
    {
        message_output(errorp ? errorp : string_lookup(STATUS_NOMEM));
        return(0);
    }

    dbox_show(d);

    state->window_handle = dbox_window_handle(d);
    state->cep = cep;
    state->cp  = gr_chart_cp_from_ch(cep->ch);

    state->reflect_modify = 0;

    return(1);
}

static void
gr_chartedit_gallery_barlinescatch_process(
    P_GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state /*inout*/,
    dbox_field f)
{
    const int hit_icon_handle = dbox_field_to_icon_handle(d, f);
    P_GR_CHART cp = state->cp;
    const HOST_WND window_handle = state->window_handle;
    const int * const icon_handles = state->icon_handles;
    int icon_handle;

    /* adjusters all modify but rebuild at end */

    if(f == dbox_OK)
    {
        state->reflect_modify = 1;
        return;
    }

    if(NULL != icon_handles)
    {
        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_WIDTH];
        if(icon_handle &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->barch.slot_width_percentage,
                            &double_1,
                            &pct_min_limit, &pct_max_limit,
                             pct_decplaces))
        {
            state->barch_modified = 1;
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_2D_OVERLAP];
        if(icon_handle && !cp->d3.bits.on &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &cp->barch.slot_overlap_percentage,
                            &double_1,
                            &double_m100, &double_100,
                             pct_decplaces))
            return;

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BAR_STACK_PICT];
        if(icon_handle && (icon_handle == f))
        {
            state->barch.bits.pictures_stacked = winf_getonoff(window_handle, f);
            state->barch_modified = 1;

            /* if changed then assume we want pictures too */
            if(state->bits.tristate_pictures == RISCOS_TRISTATE_DONT_CARE)
                state->bits.tristate_pictures = RISCOS_TRISTATE_ON;

            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_WIDTH];
        if(icon_handle &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->linech.slot_width_percentage,
                            &double_1,
                            &pct_min_limit, &pct_max_limit,
                             pct_decplaces))
        {
            state->linech_modified = 1;
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_LINE_2D_SHIFT];
        if(icon_handle && !cp->d3.bits.on &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &cp->linech.slot_shift_percentage,
                            &double_1,
                            &double_m100, &double_100,
                             pct_decplaces))
            return;

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BARLINE_3D_DEPTH];
        if(icon_handle && cp->d3.bits.on &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->barlinech.slot_depth_percentage,
                            &double_1,
                            &pct_min_limit, &pct_max_limit,
                             pct_decplaces))
        {
            state->barlinech_modified = 1;
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_SCAT_WIDTH];
        if(icon_handle &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->scatch.width_percentage,
                            &pct_bumpvalue,
                            &pct_min_limit, &pct_max_limit,
                             pct_decplaces))
        {
            state->scatch_modified = 1;
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_ON];
        if(icon_handle && (icon_handle == f))
        {
            cp->d3.bits.on = winf_getonoff(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_TURN];
        if(icon_handle && cp->d3.bits.on &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &cp->d3.turn,
                            &double_1,
                            &turn_min_limit, &turn_max_limit,
                             turn_decplaces))
            return;

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_3D_DROOP];
        if(icon_handle && cp->d3.bits.on &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &cp->d3.droop,
                            &double_1,
                            &droop_min_limit, &droop_max_limit,
                             droop_decplaces))
            return;

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_REMOVE_OVERLAY];
        if(icon_handle && (icon_handle == f))
        {
            if(cp->axes_idx_max > 0)
            {
                cp->axes_idx_max = 0;
                cp->bits.realloc_series = 1;

                state->reflect_modify = 1;
            }
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_CUMULATIVE];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_cumulative = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_POINT_VARY];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_point_vary = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_FILL_AXIS];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_fill_axis = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_BEST_FIT];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_best_fit = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_JOIN_LINES];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_join_lines = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_STACKED];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_stacked = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_PICTURES];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_pictures = riscos_tristate_hit(window_handle, f);
            return;
        }

        icon_handle = icon_handles[GR_CHARTEDIT_GALLERY_BARLINESCATCH_CATEG_X_VAL];
        if(icon_handle && (icon_handle == f))
        {
            state->bits.tristate_categ_x_val = riscos_tristate_hit(window_handle, f);
            return;
        }
    }
    else if(state->charttype == GR_CHARTTYPE_PIE)
    {
        icon_handle = GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_ANTICLOCKWISE;
        if(icon_handle && (icon_handle == f))
        {
            state->pie_xtra.anticlockwise = winf_getonoff(window_handle, f);
            state->pie_xtra_modified = 1;
            return;
        }

        if( (f >= GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_000) &&
            (f <= GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_315) )
        {
            state->pie_xtra.start_heading_degrees = pie_start_headings[f - GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_000];
            state->pie_xtra_modified = 1;

            /* reflect into icon as this is checked for value */
            winf_setdouble(window_handle, GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL,
                           &state->pie_xtra.start_heading_degrees, pie_heading_decplaces);

            return;
        }

        icon_handle = GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL;
        if(icon_handle &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->pie_xtra.start_heading_degrees,
                            &pie_heading_increment,
                            &pie_heading_min_limit,
                            &pie_heading_max_limit,
                             pie_heading_decplaces))
        {
            state->pie_xtra_modified = 1;
            return;
        }

        icon_handle = GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL;
        if(icon_handle &&
            winf_bumpdouble(window_handle, hit_icon_handle, icon_handle,
                            &state->piechdispl.radial_displacement,
                            &pct_radial_dsp_increment,
                            &pct_radial_dsp_min_limit,
                            &pct_radial_dsp_max_limit,
                             pct_radial_dsp_decplaces))
        {
            state->piechdispl_modified = 1;
            return;
        }
    }

    assert(0);
}

/******************************************************************************
*
* encode bar gallery from current state
*
******************************************************************************/

static const int
gr_chartedit_gallery_icons_barch[GR_CHARTEDIT_GALLERY_BARLINESCATCH_N_ICONS] =
{
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BAR_WIDTH,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_2D_OVERLAP,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACK_PICT,

    0,
    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_DEPTH,

    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_ON,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_TURN,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_DROOP,

    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_REMOVE_OVERLAY,

    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CUMULATIVE,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_POINT_VARY,
    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BEST_FIT,
    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACKED,
    GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_PICTURES,
    0
};

/******************************************************************************
*
* process bar gallery dialog box
*
******************************************************************************/

extern void
gr_chartedit_gallery_bar_process(
    P_GR_CHARTEDITOR cep)
{
    static dbox d = NULL;

    P_GR_CHART cp;
    dbox_field f;
    BOOL ok_hit, pict_hit, persist;
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state;

    state.icon_handles = gr_chartedit_gallery_icons_barch;
    state.charttype = GR_CHARTTYPE_BAR;

    if(!gr_chartedit_gallery_barlinescatch_init(&state, cep, &d, GR_CHARTEDIT_TEM_GALLERY_BAR))
        return;

    cp = state.cp;

    for(;;)
    {
        gr_chartedit_gallery_barlinescatch_getsel(&state);

        for(;;)
        {
            gr_chartedit_gallery_barlinescatch_encode(&state);

            f = dbox_fillin(d);

            if(GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CANCEL == f)
                f = dbox_CLOSE;

            ok_hit = (f == dbox_OK);

            pict_hit = (f >= GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST)  &&
                       (f <= GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_LAST );

            if(f == dbox_CLOSE)
                break;

            /* maybe set base axes types to bar, first hit on anything converts */

            if(ok_hit || pict_hit)
            {
                if(state.modifying_id.name == GR_CHART_OBJNAME_AXIS)
                {
                    if( cp->axes[state.modifying_axes_idx].charttype != state.charttype)
                    {
                        cp->axes[state.modifying_axes_idx].charttype  = state.charttype;

                        /* set up grid state on first conversion */
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].major.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_NONE;

                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].major.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_NONE;

                        gr_chartedit_gallery_undo_pie_bodges(cp);

                        /* fake default pict_hit if OK clicked */
                        if(ok_hit)
                        {
                            f = GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST;
                            pict_hit = 1;
                        }
                    }
                }
            }

            if(pict_hit)
            {
                GR_SERIES_TYPE sertype;

                sertype                           = GR_CHART_SERIES_PLAIN;
                state.bits.tristate_cumulative    = RISCOS_TRISTATE_OFF;
                state.bits.tristate_point_vary    = RISCOS_TRISTATE_OFF;
                state.bits.tristate_stacked       = RISCOS_TRISTATE_OFF;
                state.bits.tristate_pictures      = RISCOS_TRISTATE_OFF;
                cp->axes[state.modifying_axes_idx].bits.stacked_pct = 0;
                state.barch.bits.pictures_stacked = 1;

                switch(f - GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST)
                {
                default:
                case 0:
                    break;

                case 1:
                    sertype                           = GR_CHART_SERIES_PLAIN_ERROR1;
                    break;

                case 2:
                    state.bits.tristate_point_vary    = RISCOS_TRISTATE_ON;
                    break;

                case 3:
                    break;

                case 4:
                    state.bits.tristate_pictures      = RISCOS_TRISTATE_ON;
                    state.barch.bits.pictures_stacked = 1;
                    state.barch_modified              = 1;
                    break;

                case 5:
                    state.bits.tristate_pictures      = RISCOS_TRISTATE_ON;
                    state.barch.bits.pictures_stacked = 0;
                    state.barch_modified              = 1;
                    break;

                case 6:
                    state.bits.tristate_stacked       = RISCOS_TRISTATE_ON;
                    break;

                case 7:
                    state.bits.tristate_stacked       = RISCOS_TRISTATE_ON;
                    cp->axes[state.modifying_axes_idx].bits.stacked_pct = 1;
                    break;
                }

                gr_chartedit_gallery_barlinescatch_convert(&state, sertype);
            }
            else
                gr_chartedit_gallery_barlinescatch_process(&state, f);

            gr_chartedit_gallery_barlinescatch_decode(&state);

            if(ok_hit || pict_hit)
                break;
        }

        /* hits on pictures not caused by OK faking pict_hit go loopy */
        if(!ok_hit && pict_hit)
            continue;

        persist = ok_hit ? dbox_persist() : FALSE;

        if(!persist)
            break;
    }

    dbox_dispose(&d);
}

/******************************************************************************
*
* encode line gallery from current state
*
******************************************************************************/

static const int
gr_chartedit_gallery_icons_linech[GR_CHARTEDIT_GALLERY_BARLINESCATCH_N_ICONS] =
{
    0,
    0,
    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_WIDTH,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_2D_SHIFT,

    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_DEPTH,

    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_ON,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_TURN,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_DROOP,

    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_REMOVE_OVERLAY,

    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CUMULATIVE,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_VARY,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FILL_AXIS,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_BEST_FIT,
    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_STACKED,
    GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_PICTURES,
    0
};

/******************************************************************************
*
* process line gallery dialog box
*
******************************************************************************/

extern void
gr_chartedit_gallery_line_process(
    P_GR_CHARTEDITOR cep)
{
    static dbox d = NULL;

    P_GR_CHART cp;
    dbox_field f;
    BOOL ok_hit, pict_hit, persist;
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state;

    state.icon_handles = gr_chartedit_gallery_icons_linech;
    state.charttype = GR_CHARTTYPE_LINE;

    if(!gr_chartedit_gallery_barlinescatch_init(&state, cep, &d, GR_CHARTEDIT_TEM_GALLERY_LINE))
        return;

    cp = state.cp;

    for(;;)
    {
        gr_chartedit_gallery_barlinescatch_getsel(&state);

        for(;;)
        {
            gr_chartedit_gallery_barlinescatch_encode(&state);

            f = dbox_fillin(d);

            if(GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CANCEL == f)
                f = dbox_CLOSE;

            ok_hit = (f == dbox_OK);

            pict_hit = (f >= GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FIRST)  &&
                       (f <= GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_LAST );

            if(f == dbox_CLOSE)
                break;

            /* maybe set base axes types to line, first hit on anything converts */

            if(ok_hit || pict_hit)
            {
                if(state.modifying_id.name == GR_CHART_OBJNAME_AXIS)
                {
                    if( cp->axes[state.modifying_axes_idx].charttype != state.charttype)
                    {
                        cp->axes[state.modifying_axes_idx].charttype  = state.charttype;

                        /* set up grid state on first conversion */
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].major.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[X_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_NONE;

                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].major.bits.grid = 1;
                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[state.modifying_axes_idx].axis[Y_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_HALF_RIGHT;

                        gr_chartedit_gallery_undo_pie_bodges(cp);

                        /* fake default pict_hit if OK clicked */
                        if(ok_hit)
                        {
                            f = GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FIRST;
                            pict_hit = 1;
                        }
                    }
                }
            }

            if(pict_hit)
            {
                GR_SERIES_TYPE sertype;

                sertype                       = GR_CHART_SERIES_PLAIN;
                state.bits.tristate_fill_axis = RISCOS_TRISTATE_OFF;
                state.bits.tristate_stacked   = RISCOS_TRISTATE_OFF;
                state.bits.tristate_pictures  = RISCOS_TRISTATE_OFF;

                switch(f - GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FIRST)
                {
                default:
                case 0:
                    break;

                case 1:
                    sertype                       = GR_CHART_SERIES_PLAIN_ERROR1;
                    break;

                case 2:
                    state.bits.tristate_fill_axis = RISCOS_TRISTATE_ON;
                    break;

                case 3:
                    state.bits.tristate_fill_axis = RISCOS_TRISTATE_ON;
                    state.bits.tristate_stacked   = RISCOS_TRISTATE_ON;
                    break;

                case 4:
                    state.bits.tristate_pictures  = RISCOS_TRISTATE_ON;
                    break;

                case 5:
                    sertype                       = GR_CHART_SERIES_PLAIN_ERROR1;
                    state.bits.tristate_pictures  = RISCOS_TRISTATE_ON;
                    break;

                case 6:
                    state.bits.tristate_fill_axis = RISCOS_TRISTATE_ON;
                    state.bits.tristate_pictures  = RISCOS_TRISTATE_ON;
                    break;

                case 7:
                    state.bits.tristate_fill_axis = RISCOS_TRISTATE_ON;
                    state.bits.tristate_stacked   = RISCOS_TRISTATE_ON;
                    state.bits.tristate_pictures  = RISCOS_TRISTATE_ON;
                    break;
                }

                gr_chartedit_gallery_barlinescatch_convert(&state, sertype);
            }
            else
                gr_chartedit_gallery_barlinescatch_process(&state, f);

            gr_chartedit_gallery_barlinescatch_decode(&state);

            if(ok_hit || pict_hit)
                break;
        }

        /* hits on pictures not caused by OK faking pict_hit go loopy */
        if(!ok_hit && pict_hit)
            continue;

        persist = ok_hit ? dbox_persist() : FALSE;

        if(!persist)
            break;
    }

    dbox_dispose(&d);
}

/******************************************************************************
*
* encode scat gallery from current state
*
******************************************************************************/

static const int
gr_chartedit_gallery_icons_scatch[GR_CHARTEDIT_GALLERY_BARLINESCATCH_N_ICONS] =
{
    0,
    0,
    0,

    0,
    0,

    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_WIDTH,

    0,
    0,
    0,

    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_REMOVE_OVERLAY,

    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_VARY,
    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_BEST_FIT,
    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_JOIN_LINES,
    0,
    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_PICTURES,
    GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CATEG_X_VAL
};

/******************************************************************************
*
* process scatter gallery dialog box
*
******************************************************************************/

extern void
gr_chartedit_gallery_scat_process(
    P_GR_CHARTEDITOR cep)
{
    static dbox d = NULL;

    P_GR_CHART cp;
    dbox_field f;
    BOOL ok_hit, pict_hit, persist;
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state;

    state.icon_handles = gr_chartedit_gallery_icons_scatch;
    state.charttype = GR_CHARTTYPE_SCAT;

    if(!gr_chartedit_gallery_barlinescatch_init(&state, cep, &d, GR_CHARTEDIT_TEM_GALLERY_SCAT))
        return;

    cp = state.cp;

    for(;;)
    {
        gr_chartedit_gallery_barlinescatch_getsel(&state);

        for(;;)
        {
            gr_chartedit_gallery_barlinescatch_encode(&state);

            f = dbox_fillin(d);

            if(GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CANCEL == f)
                f = dbox_CLOSE;

            ok_hit = (f == dbox_OK);

            pict_hit = (f >= GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_FIRST)  &&
                       (f <= GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_LAST );

            if(f == dbox_CLOSE)
                break;

            /* maybe set base CHART type to scat, first hit on anything converts */

            if(ok_hit || pict_hit)
            {
                if( cp->axes[0].charttype != state.charttype)
                {
                    cp->axes[0].charttype  = state.charttype;

                    /* set up grid state on first conversion */
                    cp->axes[0].axis[X_AXIS_IDX].major.bits.grid = 1;
                    cp->axes[0].axis[X_AXIS_IDX].minor.bits.grid = 0;
                    cp->axes[0].axis[X_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_HALF_TOP;

                    cp->axes[0].axis[Y_AXIS_IDX].major.bits.grid = 1;
                    cp->axes[0].axis[Y_AXIS_IDX].minor.bits.grid = 0;
                    cp->axes[0].axis[Y_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_HALF_RIGHT;

                    if(cp->axes_idx_max > 0)
                    {
                        cp->axes[1].axis[X_AXIS_IDX].major.bits.grid = 1;
                        cp->axes[1].axis[X_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[1].axis[X_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_HALF_BOTTOM;

                        cp->axes[1].axis[Y_AXIS_IDX].major.bits.grid = 1;
                        cp->axes[1].axis[Y_AXIS_IDX].minor.bits.grid = 0;
                        cp->axes[1].axis[Y_AXIS_IDX].minor.bits.tick = GR_AXIS_TICK_POSITION_HALF_LEFT;
                    }

                    gr_chartedit_gallery_undo_pie_bodges(cp);

                    /* fake default pict_hit if OK clicked */
                    if(ok_hit)
                    {
                        f = GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_FIRST;
                        pict_hit = 1;
                    }
                }
            }

            if(pict_hit)
            {
                GR_SERIES_TYPE sertype;

                sertype                        = GR_CHART_SERIES_POINT;
                state.bits.tristate_join_lines = RISCOS_TRISTATE_ON;
                state.bits.tristate_pictures   = RISCOS_TRISTATE_ON;

                switch(f - GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_FIRST)
                {
                default:
                case 0:
                    state.bits.tristate_pictures   = RISCOS_TRISTATE_OFF;
                    break;

                case 1:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR1;
                    state.bits.tristate_pictures   = RISCOS_TRISTATE_OFF;
                    break;

                case 2:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR2;
                    state.bits.tristate_pictures   = RISCOS_TRISTATE_OFF;
                    break;

                case 3:
                    state.bits.tristate_join_lines = RISCOS_TRISTATE_OFF;
                    break;

                case 4:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR1;
                    state.bits.tristate_join_lines = RISCOS_TRISTATE_OFF;
                    break;

                case 5:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR2;
                    state.bits.tristate_join_lines = RISCOS_TRISTATE_OFF;
                    break;

                case 6:
                    break;

                case 7:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR1;
                    break;

                case 8:
                    sertype                        = GR_CHART_SERIES_POINT_ERROR2;
                    break;
                }

                if(state.bits.tristate_categ_x_val == RISCOS_TRISTATE_ON)
                    sertype = (GR_SERIES_TYPE) ((sertype - GR_CHART_SERIES_POINT) + GR_CHART_SERIES_PLAIN);

                gr_chartedit_gallery_barlinescatch_convert(&state, sertype);
            }
            else
                gr_chartedit_gallery_barlinescatch_process(&state, f);

            gr_chartedit_gallery_barlinescatch_decode(&state);

            if(ok_hit || pict_hit)
                break;
        }

        /* hits on pictures not caused by OK faking pict_hit go loopy */
        if(!ok_hit && pict_hit)
            continue;

        persist = ok_hit ? dbox_persist() : FALSE;

        if(!persist)
            break;
    }

    dbox_dispose(&d);
}

#endif /* RISCOS */

#if RISCOS

/******************************************************************************
*
* process pie gallery dialog box
*
******************************************************************************/

extern void
gr_chartedit_gallery_pie_process(
    P_GR_CHARTEDITOR cep)
{
    static dbox d = NULL;

    dbox_field f;
    P_GR_CHART cp;
    BOOL ok_hit, pict_hit, persist;
    GR_CHARTEDIT_GALLERY_BARLINESCATCH_STATE state;

    state.icon_handles = NULL;
    state.charttype = GR_CHARTTYPE_PIE;

    if(!gr_chartedit_gallery_barlinescatch_init(&state, cep, &d, GR_CHARTEDIT_TEM_GALLERY_PIE))
        return;

    cp = state.cp;

    for(;;)
    {
        gr_chartedit_gallery_barlinescatch_getsel(&state);

        for(;;)
        {
            gr_chartedit_gallery_barlinescatch_encode(&state);

            f = dbox_fillin(d);

            if(GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_CANCEL == f)
                f = dbox_CLOSE;

            ok_hit = (f == dbox_OK);

            pict_hit = (f >= GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_FIRST)  &&
                       (f <= GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_LAST );

            if(f == dbox_CLOSE)
                break;

            /* maybe set base axes types to pie, first hit on anything converts */

            if(ok_hit || pict_hit)
            {
                if(cp->axes[0].charttype != GR_CHARTTYPE_PIE)
                {
                    P_GR_SERIES serp;

                    cp->axes[0].charttype = GR_CHARTTYPE_PIE;

                    /* this is what the book says ... */
                    if(cp->axes_idx_max > 0)
                    {
                        cp->axes_idx_max = 0;
                        cp->bits.realloc_series = 1;
                    }

                    serp = getserp(cp, state.modifying_series_idx);

                    /* normally pies vary by point unless punter insists */
                    if(!serp->bits.point_vary_manual)
                    {
                        serp->bits.point_vary_manual = 1;
                        serp->bits.point_vary        = 1;

                        serp->internal_bits.point_vary_manual_pie_bodge = 1;
                    }

                        /* fake default pict_hit if OK clicked */
                        if(ok_hit)
                        {
                            f = GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST;
                            pict_hit = 1;
                        }
                }
            }

            if(pict_hit)
            {
                switch(f - GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_FIRST)
                {
                case 0:
                    state.piechdispl.radial_displacement = 0.0;
                    winf_setdouble(state.window_handle, GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL, &state.piechdispl.radial_displacement, 0);
                    state.piechdispl_modified = 1;

                    state.piechlabel.bits.label_leg = 0;
                    state.piechlabel.bits.label_val = 0;
                    state.piechlabel.bits.label_pct = 0;

                    state.piechlabel_modified = 1;
                    break;

                case 1:
                    state.piechlabel.bits.label_leg = 0;
                    state.piechlabel.bits.label_val = 1;
                    state.piechlabel.bits.label_pct = 0;

                    state.piechlabel_modified = 1;
                    break;

                case 2:
                    state.piechlabel.bits.label_leg = 0;
                    state.piechlabel.bits.label_val = 0;
                    state.piechlabel.bits.label_pct = 1;

                    state.piechlabel_modified = 1;
                    break;

                case 3:
                    state.piechlabel.bits.label_leg = 1;
                    state.piechlabel.bits.label_val = 0;
                    state.piechlabel.bits.label_pct = 0;

                    state.piechlabel_modified = 1;
                    break;

                case 4:
                    {
                    /* explode a point and kill the rest if not just point selected - this is harder */
                    GR_CHART_OBJID tmp_id;
                    GR_PIECHDISPLSTYLE tmp_style;

                    if(state.modifying_id.name != GR_CHART_OBJNAME_POINT)
                    {
                        /* explode point 1 of whichever series if no point selected */
                        gr_chart_objid_from_series_idx(cp, state.modifying_series_idx, &tmp_id);
                        tmp_id.name      = GR_CHART_OBJNAME_POINT;
                        tmp_id.has_subno = 1;
                        tmp_id.subno     = 1;
                    }
                    else
                        tmp_id = state.modifying_id;

                    state.piechdispl_modified = 0;

                    gr_chart_objid_piechdisplstyle_query(cp, tmp_id, &tmp_style);
                    tmp_style.radial_displacement = state.piechdispl.radial_displacement;
                    status_consume(gr_chart_objid_piechdisplstyle_set(cp, tmp_id, &tmp_style));
                    break;
                    }

                case 5:
                    {
                    /* explode all - set series base value to whatever */
                    GR_CHART_OBJID tmp_id;
                    GR_PIECHDISPLSTYLE tmp_style;

                    state.piechdispl_modified = 0;

                    gr_chart_objid_from_series_idx(cp, state.modifying_series_idx, &tmp_id);

                    gr_chart_objid_piechdisplstyle_query(cp, tmp_id, &tmp_style);
                    tmp_style.radial_displacement = state.piechdispl.radial_displacement;
                    status_consume(gr_chart_objid_piechdisplstyle_set(cp, tmp_id, &tmp_style));
                    break;
                    }

                default:
                    assert(0);
                    break;
                }

                gr_chartedit_gallery_barlinescatch_convert(&state, GR_CHART_SERIES_PLAIN);
            }
            else
                gr_chartedit_gallery_barlinescatch_process(&state, f);

            gr_chartedit_gallery_barlinescatch_decode(&state);

            if(ok_hit || pict_hit)
                break;
        }

        /* hits on pictures not caused by OK faking pict_hit go loopy */
        if(!ok_hit && pict_hit)
            continue;

        persist = ok_hit ? dbox_persist() : FALSE;

        if(!persist)
            break;
    }

    dbox_dispose(&d);
}

#endif /* RISCOS */

/******************************************************************************
*
* undo the bodge we performed to make pies vary by point
*
******************************************************************************/

static void
gr_chartedit_gallery_undo_pie_bodges(
    P_GR_CHART cp)
{
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;

    for(series_idx = 0;
        series_idx < cp->series.n_defined;
        series_idx++)
    {
        serp = getserp(cp, series_idx);

        if(serp->internal_bits.point_vary_manual_pie_bodge)
        {
            serp->internal_bits.point_vary_manual_pie_bodge = 0;

            serp->bits.point_vary_manual = 0;
        }
    }
}

/******************************************************************************
*
* create a chart dialogue box, first ensuring the template file is loaded
*
******************************************************************************/

extern void * /*dbox*/
chart_dbox_new_new(const char * name, char ** errorp /*out*/)
{
    // reportf("chart_template_ensure()");
    if(chart_template_ensure())
        return(dbox_new_new(name, errorp));

    return(NULL);
}

/* end of gr_blgal.c */
