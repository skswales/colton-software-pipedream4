/* gr_scale.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Scaling procedures for graphics modules */

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
#include "cs-wimptx.h"
#endif

/*
internal procedures
*/

typedef const struct _GR_STYLE_COMMON_BLK * P_GR_STYLE_COMMON_BLK; /* NB. these are never modified */

static P_ANY
gr_point_list_search(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    GR_LIST_ID list_id);

static S32
gr_point_list_set(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_ANY style,
    GR_LIST_ID list_id);

/******************************************************************************
*
* round to ± infinity at multiples of a given number
* (occasionally need to round to real pixels etc.)
* (oh, for function overloading...)
*
******************************************************************************/

extern GR_OSUNIT
gr_os_from_pixit_floor(
    GR_COORD a)
{
    S32 b = GR_PIXITS_PER_RISCOS;
    return( (GR_OSUNIT) ((a >= 0) ? a : (a - b + 1)) / b );
}

extern GR_OSUNIT
gr_round_os_to_ceil(
    GR_OSUNIT a,
    S32 b)
{
    return(             ((a <= 0) ? a : (a + b - 1)) / b );
}

extern GR_OSUNIT
gr_round_os_to_floor(
    GR_OSUNIT a,
    S32 b)
{
    return(             ((a >= 0) ? a : (a - b + 1)) / b );
}

extern GR_COORD
gr_round_pixit_to_ceil(
    GR_COORD a,
    S32 b)
{
    return( (GR_COORD)  ((a <= 0) ? a : (a + b - 1)) / b );
}

extern GR_COORD
gr_round_pixit_to_floor(
    GR_COORD a,
    S32 b)
{
    return( (GR_COORD)  ((a >= 0) ? a : (a - b + 1)) / b );
}

extern GR_COORD
(gr_pixit_from_riscos) (GR_OSUNIT a)
{
    return((GR_COORD) a * GR_PIXITS_PER_RISCOS);
}

/* -------------------------------------------------------------------------------- */

extern void
gr_chart_objid_from_axes(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name   = GR_CHART_OBJNAME_AXIS;
    id->has_no = 1;
    id->no     = gr_axes_external_from_ix(cp, axes, axis);
}

/*
converts from GR_CHART_OBJNAME_AXIS(n) to appropriate major/minor tick/grid object
*/

extern void
gr_chart_objid_from_axis_grid(
     _InoutRef_  P_GR_CHART_OBJID id,
    BOOL major)
{
    id->name      = GR_CHART_OBJNAME_AXISGRID;
    id->has_subno = 1;
    id->subno     = major ? 1 : 2;
}

extern void
gr_chart_objid_from_axis_tick(
    _InoutRef_  P_GR_CHART_OBJID id,
    BOOL major)
{
    id->name      = GR_CHART_OBJNAME_AXISTICK;
    id->has_subno = 1;
    id->subno     = major ? 1 : 2;
}

extern void
gr_chart_objid_from_seridx(
    P_GR_CHART cp,
    GR_SERIES_NO seridx,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name   = GR_CHART_OBJNAME_SERIES;
    id->has_no = 1;
    id->no     = gr_series_external_from_ix(cp, seridx);
}

extern void
gr_chart_objid_from_series(
    GR_SERIES_NO series,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name   = GR_CHART_OBJNAME_SERIES;
    id->has_no = 1;
    id->no     = series;
}

extern void
gr_chart_objid_from_text(
    LIST_ITEMNO key,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name   = GR_CHART_OBJNAME_TEXT;
    id->has_no = 1;
    id->no     = (U8) key;
}

/* -------------------------------------------------------------------------------- */

typedef struct _GR_STYLE_COMMON_BLK
{
    NLISTS_BLK nlb;

    U16 offset_of_lbr;

    U16 style_size;

    void (* rerefproc) (
        P_GR_CHART    cp,
        GR_SERIES_IX seridx,
        S32          add);

    #if TRACE_ALLOWED
    PC_U8     list_name;
    #endif
}
GR_STYLE_COMMON_BLK;

#if TRACE_ALLOWED
#define TRACE_STYLE_CLASS(name) , name
#else
#define TRACE_STYLE_CLASS(name) /* nothing */
#endif

static const GR_STYLE_COMMON_BLK
gr_chart_text_common_blk =
{
    { NULL, sizeof(GR_TEXT) + 1024, 16 * (sizeof(GR_TEXT) + 1024) },
    offsetof(GR_CHART, text) + offsetof(struct _gr_chart_text, lbr),
    sizeof(GR_TEXT), NULL
    TRACE_STYLE_CLASS("text object")
};

static const GR_STYLE_COMMON_BLK
gr_chart_text_textstyle_common_blk =
{
    { NULL, sizeof(GR_TEXTSTYLE), 16 * sizeof(GR_TEXTSTYLE) },
    offsetof(GR_CHART, text) + offsetof(struct _gr_chart_text, style) + offsetof(struct _gr_chart_text_style, lbr),
    sizeof(GR_TEXTSTYLE), NULL
    TRACE_STYLE_CLASS("text object textstyle")
};

static const GR_STYLE_COMMON_BLK
gr_pdrop_fillstyle_common_blk =
{
    { NULL, sizeof(GR_FILLSTYLE), 16 * sizeof(GR_FILLSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, pdrop_fill),
    sizeof(GR_FILLSTYLE), gr_pdrop_list_fillstyle_reref
    TRACE_STYLE_CLASS("drop fillstyle")
};

static const GR_STYLE_COMMON_BLK
gr_pdrop_linestyle_common_blk =
{
    { NULL, sizeof(GR_LINESTYLE), 16 * sizeof(GR_LINESTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, pdrop_line),
    sizeof(GR_LINESTYLE), NULL
    TRACE_STYLE_CLASS("drop linestyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_fillstyle_common_blk =
{
    { NULL, sizeof(GR_FILLSTYLE), 16 * sizeof(GR_FILLSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_fill),
    sizeof(GR_FILLSTYLE), gr_point_list_fillstyle_reref
    TRACE_STYLE_CLASS("fillstyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_linestyle_common_blk =
{
    { NULL, sizeof(GR_LINESTYLE), 16 * sizeof(GR_LINESTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_line),
    sizeof(GR_LINESTYLE), NULL
    TRACE_STYLE_CLASS("linestyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_textstyle_common_blk =
{
    { NULL, sizeof(GR_TEXTSTYLE), 16 * sizeof(GR_TEXTSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_text),
    sizeof(GR_TEXTSTYLE), NULL
    TRACE_STYLE_CLASS("textstyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_barchstyle_common_blk =
{
    { NULL, sizeof(GR_BARCHSTYLE), 16 * sizeof(GR_BARCHSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_barch),
    sizeof(GR_BARCHSTYLE), NULL
    TRACE_STYLE_CLASS("bar chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_barlinechstyle_common_blk =
{
    { NULL, sizeof(GR_BARLINECHSTYLE), 16 * sizeof(GR_BARLINECHSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_barlinech),
    sizeof(GR_BARLINECHSTYLE), NULL
    TRACE_STYLE_CLASS("bar & line chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_linechstyle_common_blk =
{
    { NULL, sizeof(GR_LINECHSTYLE), 16 * sizeof(GR_LINECHSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_linech),
    sizeof(GR_LINECHSTYLE), NULL
    TRACE_STYLE_CLASS("line chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_piechdisplstyle_common_blk =
{
    { NULL, sizeof(GR_PIECHDISPLSTYLE), 16 * sizeof(GR_PIECHDISPLSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_piechdispl),
    sizeof(GR_PIECHDISPLSTYLE), NULL
    TRACE_STYLE_CLASS("pie chart displ")
};

static const GR_STYLE_COMMON_BLK
gr_point_piechlabelstyle_common_blk =
{
    { NULL, sizeof(GR_PIECHLABELSTYLE), 16 * sizeof(GR_PIECHLABELSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_piechlabel),
    sizeof(GR_PIECHLABELSTYLE), NULL
    TRACE_STYLE_CLASS("pie chart label")
};

static const GR_STYLE_COMMON_BLK
gr_point_scatchstyle_common_blk =
{
    { NULL, sizeof(GR_SCATCHSTYLE), 16 * sizeof(GR_SCATCHSTYLE) },
    offsetof(GR_SERIES, lbr) + offsetof(struct _gr_series_lbr, point_scatch),
    sizeof(GR_SCATCHSTYLE), NULL
    TRACE_STYLE_CLASS("scatter chart")
};

/*
array tying GR_LIST_IDs to GR_STYLE_COMMON_BLKs
*/

static const P_GR_STYLE_COMMON_BLK
gr_style_common_blks[GR_LIST_N_IDS] =
{
    &gr_chart_text_common_blk,
    &gr_chart_text_textstyle_common_blk,

    &gr_pdrop_fillstyle_common_blk,
    &gr_pdrop_linestyle_common_blk,

    &gr_point_fillstyle_common_blk,
    &gr_point_linestyle_common_blk,
    &gr_point_textstyle_common_blk,

    &gr_point_barchstyle_common_blk,
    &gr_point_barlinechstyle_common_blk,
    &gr_point_linechstyle_common_blk,
    &gr_point_piechdisplstyle_common_blk,
    &gr_point_piechlabelstyle_common_blk,
    &gr_point_scatchstyle_common_blk
};

/******************************************************************************
*
* create an entry on a list to store style data
* associated with a data point and store it
*
******************************************************************************/

static S32
common_list_set(
    P_NLISTS_BLK lbrp,
    LIST_ITEMNO item,
    PC_ANY style,
    P_GR_STYLE_COMMON_BLK cbp)
{
    list_itempos itpos; /* <<< delete me soon! */
    P_LIST_ITEM it;
    P_ANY pt;

    assert(lbrp);

    /* allocate new list if null list */
    if(!lbrp->lbr)
        {
        if(!nlist_allocblkref(&lbrp->lbr))
            return(create_error(GR_CHART_ERR_NOMEM));

        /* retune temporarily */
        list_deregister((P_LIST_BLOCK) lbrp->lbr);
        list_init(      (P_LIST_BLOCK) lbrp->lbr, cbp->nlb.maxitemsize, cbp->nlb.maxpoolsize);
        list_register(  (P_LIST_BLOCK) lbrp->lbr);
        }

    if(!nlist_gotoitem(lbrp->lbr, item, &it, &itpos))
        if(!nlist_createitem(lbrp->lbr, item, &it, cbp->nlb.maxitemsize, FALSE))
            return(create_error(GR_CHART_ERR_NOMEM));

    pt = list_itemcontents(it);

    if(style)
        void_memcpy32(pt, style, cbp->style_size);

    return(1);
}

static void
common_list_fillstyle_reref(
    P_NLISTS_BLK lbrp,
    S32 add)
{
    LIST_ITEMNO key;
    P_GR_FILLSTYLE pt;

    for(pt = collect_first(lbrp, &key); pt; pt = collect_next(lbrp, &key))
        {
        /* add/lose ref to particular picture. DOES NOT destroy stored pattern handle */
        gr_cache_ref((/*const*/ P_GR_CACHE_HANDLE) &pt->pattern, add);
        }
}

/******************************************************************************
*
* lists rooted in the chart
*
******************************************************************************/

#define gr_chart_list_getlbrp(cp, cbp) ((P_NLISTS_BLK) ((P_U8) (cp) + (cbp)->offset_of_lbr))

extern S32
gr_chart_list_delete(
    P_GR_CHART cp,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    P_NLISTS_BLK lbrp;

    lbrp = gr_chart_list_getlbrp(cp, cbp);

    trace_2(TRACE_MODULE_GR_CHART,
            "gr_chart_list_delete %s list &%p\n",
            cbp->list_name, report_ptr_cast(lbrp));

    assert(!cbp->rerefproc);

    collect_delete(lbrp);

    return(1);
}

extern S32
gr_chart_list_duplic(
    P_GR_CHART cp,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    NLISTS_BLK   new_lbr = cbp->nlb;
    P_NLISTS_BLK lbrp;
    S32          res;

    lbrp = gr_chart_list_getlbrp(cp, cbp);

    trace_2(TRACE_MODULE_GR_CHART,
            "gr_chart_list_duplic %s list &%p\n",
            cbp->list_name, report_ptr_cast(lbrp));

    new_lbr.lbr = NULL;

    res = collect_copy(&new_lbr, lbrp);

    lbrp->lbr = new_lbr.lbr;

    assert(!cbp->rerefproc);

    return(res);
}

#if 0
extern P_ANY
gr_chart_list_first(
    P_GR_CHART cp,
    _OutRef_    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(collect_first(gr_chart_list_getlbrp(cp, cbp), p_key));
}

extern P_ANY
gr_chart_list_next(
    P_GR_CHART cp,
    /*inout*/ P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(collect_next(gr_chart_list_getlbrp(cp, cbp), p_key));
}
#endif

static P_ANY
gr_chart_list_search(
    P_GR_CHART cp,
    LIST_ITEMNO item,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(collect_search(gr_chart_list_getlbrp(cp, cbp), &item));
}

static S32
gr_chart_list_set(
    P_GR_CHART cp,
    LIST_ITEMNO item,
    PC_ANY style,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(common_list_set(gr_chart_list_getlbrp(cp, cbp), item, style, cbp));
}

/******************************************************************************
*
* query/set the general chart style associated with an object
*
******************************************************************************/

typedef struct _GR_CHART_OBJID_CHARTSTYLE_DESC
{
    U16 style_size;
    U16 axes_offset;
    U16 ser_offset;
    U16 list_id;     /* GR_LIST_ID */
}
GR_CHART_OBJID_CHARTSTYLE_DESC; typedef const GR_CHART_OBJID_CHARTSTYLE_DESC * P_GR_CHART_OBJID_CHARTSTYLE_DESC;

/*
bar chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_barchstyle_desc =
{
                                                               sizeof(GR_BARCHSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         barch),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_barch),
                                                           GR_LIST_POINT_BARCHSTYLE
};

/*
bar & line chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_barlinechstyle_desc =
{
                                                               sizeof(GR_BARLINECHSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         barlinech),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_barlinech),
                                                           GR_LIST_POINT_BARLINECHSTYLE
};

/*
line chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_linechstyle_desc =
{
                                                               sizeof(GR_LINECHSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         linech),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_linech),
                                                           GR_LIST_POINT_LINECHSTYLE
};

/*
pie chart styles
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_piechdisplstyle_desc =
{
                                                               sizeof(GR_PIECHDISPLSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         piechdispl),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_piechdispl),
                                                           GR_LIST_POINT_PIECHDISPLSTYLE
};

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_piechlabelstyle_desc =
{
                                                               sizeof(GR_PIECHLABELSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         piechlabel),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_piechlabel),
                                                           GR_LIST_POINT_PIECHLABELSTYLE
};

/*
scatter chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_scatchstyle_desc =
{
                                                               sizeof(GR_SCATCHSTYLE),
    offsetof(GR_AXES,   style) + offsetof(struct _gr_axes_style,         scatch),
    offsetof(GR_SERIES, style) + offsetof(struct _gr_series_style, point_scatch),
                                                           GR_LIST_POINT_SCATCHSTYLE
};

/******************************************************************************
*
* query objid
*
******************************************************************************/

static S32
gr_chart_objid_chartstyle_query(
    PC_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    /*out*/ P_ANY style,
    P_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    P_U8         bpt /*const*/;
    GR_AXES_NO   axes;
    GR_SERIES_IX seridx;
    S32          using_default = 0;

    assert(cp);
    assert(style);
    assert(desc);

    switch(id->name)
        {
        default:
            /* use style of first axes */
            axes = 0;
            goto lookup_axes_style;

        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            (void) gr_axes_ix_from_external(cp, id->no, &axes);

        lookup_axes_style:;
            bpt = (P_U8) &cp->axes[axes] + desc->axes_offset;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_BESTFITSER:
            seridx = gr_series_ix_from_external(cp, id->no);

        lookup_series_style:;
            {
            bpt = (P_U8) getserp(cp, seridx) + desc->ser_offset;

            if((*bpt & 1) == 0 /*!pt->bits.manual*/)
                {
                axes = gr_axes_ix_from_seridx(cp, seridx);
                using_default = 1;
                goto lookup_axes_style;
                }
            }
            break;

        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            seridx = gr_series_ix_from_external(cp, id->no);

            bpt = gr_point_list_search(cp, seridx, gr_point_key_from_external(id->subno), (GR_LIST_ID) desc->list_id);

            if(!bpt)
                {
                using_default = 1;
                goto lookup_series_style; /* with seridx valid */
                }

            break;
        }

    void_memcpy32(style, bpt, desc->style_size);

    return(using_default);
}

/******************************************************************************
*
* set objid
*
******************************************************************************/

static S32
gr_chart_objid_chartstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_ANY style,
    P_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    P_U8         bpt;
    GR_AXES_NO   axes;
    GR_SERIES_IX seridx;
    S32          res = 1;

    assert(cp);
    assert(style);
    assert(desc);

    switch(id->name)
        {
        default:
            /* set style of first axes */
            axes = 0;
            goto set_axes_style;

        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            (void) gr_axes_ix_from_external(cp, id->no, &axes);

        set_axes_style:;

            /* set all series on axes to auto and remove deviant points */
            for(seridx = cp->axes[axes].series.stt_idx;
                seridx < cp->axes[axes].series.end_idx;
                seridx++)
                {
                /* remove deviant point data from this series */
                gr_point_list_delete(cp, seridx, (GR_LIST_ID) desc->list_id);

                /* set series to auto */
                bpt = (P_U8) getserp(cp, seridx) + desc->ser_offset;

                *bpt &= ~1; /*serp->style.point_xxxch.bits.manual = 0;*/
                }

            bpt = (P_U8) &cp->axes[axes] + desc->axes_offset;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_BESTFITSER:
            seridx = gr_series_ix_from_external(cp, id->no);

            /* remove deviant point data from this series */
            gr_point_list_delete(cp, seridx, (GR_LIST_ID) desc->list_id);

            bpt = (P_U8) getserp(cp, seridx) + desc->ser_offset;
            break;

        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_point_list_set(cp,
                                     gr_series_ix_from_external(cp, id->no),
                                     gr_point_key_from_external(id->subno),
                                     style,
                                     (GR_LIST_ID) desc->list_id));
        }

    if(bpt)
        void_memcpy32(bpt, style, desc->style_size);

    return(res);
}

/******************************************************************************
*
* query/set the bar, bar & line, line, scatter chart style associated with an object
*
******************************************************************************/

extern S32
gr_chart_objid_barchstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_BARCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_barchstyle_desc));
}

extern S32
gr_chart_objid_barchstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_BARCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_barchstyle_desc));
}

extern S32
gr_chart_objid_barlinechstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_BARLINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_barlinechstyle_desc));
}

extern S32
gr_chart_objid_barlinechstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_BARLINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_barlinechstyle_desc));
}

extern S32
gr_chart_objid_linechstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_LINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_linechstyle_desc));
}

extern S32
gr_chart_objid_linechstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_LINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_linechstyle_desc));
}

extern S32
gr_chart_objid_piechdisplstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHDISPLSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_piechdisplstyle_desc));
}

extern S32
gr_chart_objid_piechdisplstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_PIECHDISPLSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_piechdisplstyle_desc));
}

extern S32
gr_chart_objid_piechlabelstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHLABELSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_piechlabelstyle_desc));
}

extern S32
gr_chart_objid_piechlabelstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_PIECHLABELSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_piechlabelstyle_desc));
}

extern S32
gr_chart_objid_scatchstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_SCATCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_scatchstyle_desc));
}

extern S32
gr_chart_objid_scatchstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_SCATCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_scatchstyle_desc));
}

/******************************************************************************
*
* query/set the fill style associated with an object
*
******************************************************************************/

#define GR_FILLSTYLE_N_DEFAULTS 9

static GR_FILLSTYLE gr_fillstyle_defaults[GR_FILLSTYLE_N_DEFAULTS]; /* starts up all zeroes */

static P_GR_FILLSTYLE  /*const*/
gr_fillstyle_default(
    GR_CHART_ITEMNO item)
{
    static int fill_init = FALSE;

    static const int fill_with_wimp[GR_FILLSTYLE_N_DEFAULTS] =
    {
        3  /* mid grey */,

        /* this vvv will be the first one used */
        11 /* simply red    */,
        9  /* mellow yellow */,
        14 /* orange        */,
        15 /* light blue    */,
        10 /* light green   */,
        12 /* fresh cream   */,
        13 /* dark green    */,
        8  /* dark blue     */
    };

    if(!fill_init)
        {
        S32  i;
        P_GR_FILLSTYLE pt = gr_fillstyle_defaults;

        fill_init = TRUE;

        for(i = 0; i < GR_FILLSTYLE_N_DEFAULTS; ++i, ++pt)
            {
            /* NB. you get stuck with the palette entries for the startup mode */
            * (int *) &pt->fg = wimpt_RGB_for_wimpcolour(fill_with_wimp[i]);
            pt->fg.reserved = 0;
            pt->fg.manual   = 0;
            pt->fg.visible  = 1;
            }
        }

    assert(item >= 0);

    return(&gr_fillstyle_defaults[item % GR_FILLSTYLE_N_DEFAULTS]);
}

extern S32
gr_chart_objid_fillstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLE style)
{
    PC_GR_FILLSTYLE pt;
    S32 using_default = 0;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            pt = &cp->chart.areastyle;
            break;

        case GR_CHART_OBJNAME_PLOTAREA:
            assert(id->no < GR_CHART_N_PLOTAREAS);
            pt = &cp->plotarea.area[(id->no < GR_CHART_N_PLOTAREAS) ? id->no : 0].areastyle;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.areastyle;
            break;

        case GR_CHART_OBJNAME_SERIES:
            pt = &gr_seriesp_from_external(cp, id->no)->style.point_fill;

            if(!pt->fg.manual)
                {
                using_default = 1;
                pt = gr_fillstyle_default(id->no);
                }
            break;

        case GR_CHART_OBJNAME_DROPSER:
            pt = &gr_seriesp_from_external(cp, id->no)->style.pdrop_fill;

            if(!pt->fg.manual)
                {
                using_default = 1;
                pt = gr_fillstyle_default(id->no);
                }
            break;

        case GR_CHART_OBJNAME_POINT:
            return(gr_point_fillstyle_query(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_pdrop_fillstyle_query(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        default:
            myassert1x(0, "gr_chart_objid_fillstyle_query of %s", gr_chart_object_name_from_id_quick(id));

            using_default = 1;
            pt = gr_fillstyle_default(0);
            break;
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_chart_objid_fillstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_FILLSTYLE style)
{
    P_GR_FILLSTYLE pt;
    GR_SERIES_IX seridx;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            pt = &cp->chart.areastyle;
            break;

        case GR_CHART_OBJNAME_PLOTAREA:
            assert(id->no < GR_CHART_N_PLOTAREAS);
            if(id->no >= GR_CHART_N_PLOTAREAS)
                return(1);

            pt = &cp->plotarea.area[id->no].areastyle;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.areastyle;
            break;

        case GR_CHART_OBJNAME_SERIES:
            seridx = gr_series_ix_from_external(cp, id->no);

            gr_point_list_delete(cp, seridx, GR_LIST_POINT_FILLSTYLE);

            pt = &getserp(cp, seridx)->style.point_fill;
            break;

        case GR_CHART_OBJNAME_DROPSER:
            seridx = gr_series_ix_from_external(cp, id->no);

            gr_point_list_delete(cp, seridx, GR_LIST_PDROP_FILLSTYLE);

            pt = &getserp(cp, seridx)->style.pdrop_fill;
            break;

        case GR_CHART_OBJNAME_POINT:
            return(gr_point_fillstyle_set(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_pdrop_fillstyle_set(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        default:
            myassert1x(0, "gr_chart_objid_fillstyle_set of %s", gr_chart_object_name_from_id_quick(id));
            return(1);
        }

    gr_fillstyle_pict_change(pt, &style->pattern);

    *pt = *style;

        /* check validity of style bits */
    if(pt->pattern == GR_FILL_PATTERN_NONE)
        {
        /* ensure solid if no pattern to use */
        if(pt->bits.notsolid)
            pt->bits.notsolid = 0;
        }
    else
        {
        /* if there's a pattern to use, use it */
        if(!pt->bits.pattern)
            pt->bits.pattern = 1;
        }

    return(1);
}

/******************************************************************************
*
* query/set the line style associated with an object
*
******************************************************************************/

static GR_LINESTYLE gr_linestyle_defaults; /* starts up all zeroes */

static P_GR_LINESTYLE  /*const*/
gr_linestyle_default(void)
{
    static int init = FALSE;

    if(!init)
        {
        init = TRUE;

        gr_colour_set_BLACK(gr_linestyle_defaults.fg);
                            gr_linestyle_defaults.fg.manual = 0;

        /* leave rest alone. thin solid lines ok */
        }

    return(&gr_linestyle_defaults);
}

extern S32
gr_chart_objid_linestyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_LINESTYLE style)
{
    PC_GR_LINESTYLE pt;
    P_GR_AXIS p_axis;
    S32 using_default = 0;
    S32 linewidth_divisor = 1;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            pt = &cp->chart.borderstyle;
            break;

        case GR_CHART_OBJNAME_PLOTAREA:
            assert(id->no < GR_CHART_N_PLOTAREAS);
            pt = &cp->plotarea.area[(id->no < GR_CHART_N_PLOTAREAS) ? id->no : 0].borderstyle;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.borderstyle;
            break;

        case GR_CHART_OBJNAME_AXIS:
            pt = &gr_axisp_from_external(cp, id->no)->style.axis;
            break;

        case GR_CHART_OBJNAME_AXISGRID:
            p_axis = gr_axisp_from_external(cp, id->no);

            switch(id->subno)
                {
                case GR_CHART_AXISTICK_MINOR:
                    pt = &p_axis->minor.style.grid;
                    if(pt->fg.manual)
                        break;

                    using_default      = 1;
                    linewidth_divisor *= 2;

                /* deliberate drop thru ... */

                default:
                case GR_CHART_AXISTICK_MAJOR:
                    pt = &p_axis->major.style.grid;
                    if(pt->fg.manual)
                        break;

                    using_default      = 1;
                    linewidth_divisor *= 16;

                    pt = &p_axis->style.axis;
                    break;
                }
            break;

        case GR_CHART_OBJNAME_AXISTICK:
            p_axis = gr_axisp_from_external(cp, id->no);

            switch(id->subno)
                {
                case GR_CHART_AXISTICK_MINOR:
                    pt = &p_axis->minor.style.tick;
                    if(pt->fg.manual)
                        break;

                    using_default      = 1;
                    linewidth_divisor *= 2;

                /* deliberate drop thru ... */

                default:
                case GR_CHART_AXISTICK_MAJOR:
                    pt = &p_axis->major.style.tick;
                    if(pt->fg.manual)
                        break;

                    using_default      = 1;
                    linewidth_divisor *= 8;

                    pt = &p_axis->style.axis;
                    break;
                }
            break;

        case GR_CHART_OBJNAME_SERIES:
            pt = &gr_seriesp_from_external(cp, id->no)->style.point_line;
            break;

        case GR_CHART_OBJNAME_DROPSER:
            pt = &gr_seriesp_from_external(cp, id->no)->style.pdrop_line;
            break;

        case GR_CHART_OBJNAME_BESTFITSER:
            pt = &gr_seriesp_from_external(cp, id->no)->style.bestfit_line;
            break;

        case GR_CHART_OBJNAME_POINT:
            return(gr_point_linestyle_query(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_pdrop_linestyle_query(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        default:
            myassert1x(0, "gr_chart_objid_linestyle_query of %s", gr_chart_object_name_from_id_quick(id));
            pt = gr_linestyle_default();
            break;
        }

    if(!pt->fg.manual)
        {
        pt = gr_linestyle_default();

        using_default = 1;
        }

    *style = *pt;

    if(linewidth_divisor != 1)
        style->width /= linewidth_divisor; /* principally for axis major/minor derivations */

    return(using_default);
}

extern S32
gr_chart_objid_linestyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_LINESTYLE style)
{
    P_GR_LINESTYLE pt;
    P_GR_AXIS p_axis;
    GR_SERIES_IX  seridx;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            pt = &cp->chart.borderstyle;
            break;

        case GR_CHART_OBJNAME_PLOTAREA:
            if(id->no >= GR_CHART_N_PLOTAREAS)
                return(1);

            pt = &cp->plotarea.area[id->no].borderstyle;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.borderstyle;
            break;

        case GR_CHART_OBJNAME_AXIS:
            pt = &gr_axisp_from_external(cp, id->no)->style.axis;
            break;

        case GR_CHART_OBJNAME_AXISGRID:
            p_axis = gr_axisp_from_external(cp, id->no);

            switch(id->subno)
                {
                case GR_CHART_AXISTICK_MINOR:
                    pt = &p_axis->minor.style.grid;
                    break;

                case GR_CHART_AXISTICK_MAJOR:
                    pt = &p_axis->major.style.grid;
                    break;

                default:
                    return(1);
                }
            break;

        case GR_CHART_OBJNAME_AXISTICK:
            p_axis = gr_axisp_from_external(cp, id->no);

            switch(id->subno)
                {
                case GR_CHART_AXISTICK_MINOR:
                    pt = &p_axis->minor.style.tick;
                    break;

                case GR_CHART_AXISTICK_MAJOR:
                    pt = &p_axis->major.style.tick;
                    break;

                default:
                    return(1);
                }
            break;

        case GR_CHART_OBJNAME_SERIES:
            seridx = gr_series_ix_from_external(cp, id->no);

            gr_point_list_delete(cp, seridx, GR_LIST_POINT_LINESTYLE);

            pt = &getserp(cp, seridx)->style.point_line;
            break;

        case GR_CHART_OBJNAME_DROPSER:
            seridx = gr_series_ix_from_external(cp, id->no);

            gr_point_list_delete(cp, seridx, GR_LIST_PDROP_LINESTYLE);

            pt = &getserp(cp, seridx)->style.pdrop_line;
            break;

        case GR_CHART_OBJNAME_BESTFITSER:
            pt = &gr_seriesp_from_external(cp, id->no)->style.bestfit_line;
            break;

        case GR_CHART_OBJNAME_POINT:
            return(gr_point_linestyle_set(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_pdrop_linestyle_set(cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        default:
            myassert1x(0, "gr_chart_objid_linestyle_set of %s", gr_chart_object_name_from_id_quick(id));
            return(0);
        }

    *pt = *style;

    return(1);
}

/******************************************************************************
*
* query/set the text style associated with an object
*
******************************************************************************/

extern S32
gr_chart_objid_textstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_TEXTSTYLE style)
{
    PC_GR_TEXTSTYLE pt;
    S32 using_default = 0;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            /* return the base style for the chart */
            pt = &cp->text.style.base;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.textstyle;
            break;

        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            pt = &gr_axisp_from_external(cp, id->no)->style.label;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_BESTFITSER:
            pt = &gr_seriesp_from_external(cp, id->no)->style.point_text;
            break;

        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_point_textstyle_query(
                            cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_TEXT:
            pt = gr_chart_list_search(cp, id->no, GR_LIST_CHART_TEXT_TEXTSTYLE);
            break;

        default:
            myassert1x(0, "gr_chart_objid_textstyle_query of %s", gr_chart_object_name_from_id_quick(id));
            pt = NULL;
            break;
    }

    if(!pt || !pt->fg.manual)
        {
        using_default = 1;
        pt = &cp->text.style.base;
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_chart_objid_textstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_TEXTSTYLE style)
{
    P_GR_TEXTSTYLE pt;
    GR_SERIES_IX seridx;
    S32 res = 1;

    assert(cp);

    switch(id->name)
        {
        case GR_CHART_OBJNAME_CHART:
            /* set the base style for the chart */
            pt = &cp->text.style.base;
            break;

        case GR_CHART_OBJNAME_LEGEND:
            pt = &cp->legend.textstyle;
            break;

        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            pt = &gr_axisp_from_external(cp, id->no)->style.label;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_BESTFITSER:
            seridx = gr_series_ix_from_external(cp, id->no);

            gr_point_list_delete(cp, seridx, GR_LIST_POINT_TEXTSTYLE);

            pt = &getserp(cp, seridx)->style.point_text;
            break;

        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            return(gr_point_textstyle_set(
                            cp,
                            gr_series_ix_from_external(cp, id->no),
                            gr_point_key_from_external(id->subno),
                            style));

        case GR_CHART_OBJNAME_TEXT:
            return(gr_chart_list_set(cp, id->no, style, GR_LIST_CHART_TEXT_TEXTSTYLE));

        default:
            myassert1x(0, "gr_chart_objid_textstyle_set of %s", gr_chart_object_name_from_id_quick(id));
            return(0);
        }

    *pt = *style;

    return(res);
}

/******************************************************************************
*
* change/delete pictures in fillstyles
*
******************************************************************************/

extern void
gr_fillstyle_pict_change(
    /*inout*/ P_GR_FILLSTYLE style,
    const GR_FILL_PATTERN_HANDLE * newpict)
{
    gr_cache_reref((          P_GR_CACHE_HANDLE) &style->pattern,
                   (/*const*/ P_GR_CACHE_HANDLE) newpict);
}

extern void
gr_fillstyle_pict_delete(
    /*inout*/ P_GR_FILLSTYLE style)
{
    gr_fillstyle_pict_change(style, NULL);
}

/******************************************************************************
*
* add/lose refs to pictures in fillstyles
*
******************************************************************************/

extern void
gr_fillstyle_ref_add(
    PC_GR_FILLSTYLE style)
{
    gr_cache_ref((/*const*/ P_GR_CACHE_HANDLE) &style->pattern, 1);
}

extern void
gr_fillstyle_ref_lose(
    PC_GR_FILLSTYLE style)
{
    gr_cache_ref((/*const*/ P_GR_CACHE_HANDLE) &style->pattern, 0);
}

#define gr_point_list_getlbrp(serp, cbp) ((P_NLISTS_BLK) ((P_U8) (serp) + (cbp)->offset_of_lbr))

static S32
gr_pdrop_point_common_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style,
    GR_LIST_ID list_id)
{
    P_GR_FILLSTYLE pt;
    S32 res;

    assert(cp);

    /* see if style already exists */
    pt = gr_point_list_search(cp, seridx, point, list_id);

    if(pt)
        {
        if(!style)
            {
            /* delete this style item */
            P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
            LIST_ITEMNO item;

            /* lose the picture ref */
            gr_fillstyle_ref_lose(pt);

            item = point;

            /* don't mangle following entry numbering */
            collect_subtract_entry(gr_point_list_getlbrp(getserp(cp, seridx), cbp), &item);

            pt = NULL;
            }
        else
            {
            /* change the picture ref */
            if(pt->pattern != style->pattern)
                {
                gr_fillstyle_ref_lose(pt);
                *pt = *style;
                gr_fillstyle_ref_add(pt);
                }
            else
                *pt = *style;
            }

        res = 1;
        }
    else
        {
        if(style)
            {
            if((res = gr_point_list_set(cp, seridx, point, style, list_id)) < 0)
                return(res);

            pt = gr_point_list_search(cp, seridx, point, list_id);
            assert(pt);

            /* add a picture ref */
            gr_fillstyle_ref_add(pt);
            }
        else
            /* not present, who cares? */
            res = 1;
        }

    if(pt)
        {
        /* check validity of style bits */
        if(pt->pattern == GR_FILL_PATTERN_NONE)
            {
            if(pt->bits.notsolid)
                pt->bits.notsolid = 0;
            }
        else
            {
            if(!pt->bits.pattern)
                pt->bits.pattern = 1;
            }
        }

    return(res);
}

/******************************************************************************
*
* query/set the fill style associated with a data pdrop
*
******************************************************************************/

extern S32
gr_pdrop_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style)
{
    PC_GR_FILLSTYLE pt;
    S32 using_default = 0;

    pt = gr_point_list_search(cp, seridx, point, GR_LIST_PDROP_FILLSTYLE);

    if(!pt)
        {
        using_default = 1;

        pt = &getserp(cp, seridx)->style.pdrop_fill;

        if(!pt->fg.manual)
            {
            /* use point-derived style ... */
            (void) gr_point_fillstyle_query(cp, seridx, point, style);

            /* ... but still treat as defaulting from here */
            return(using_default);
            }
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_pdrop_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style)
{
    return(gr_pdrop_point_common_fillstyle_set(cp, seridx, point, style, GR_LIST_PDROP_FILLSTYLE));
}

/******************************************************************************
*
* query/set the line style associated with a data pdrop
*
******************************************************************************/

extern S32
gr_pdrop_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style)
{
    PC_GR_LINESTYLE pt;
    S32 using_default = 0;

    pt = gr_point_list_search(cp, seridx, point, GR_LIST_PDROP_LINESTYLE);

    if(!pt)
        {
        using_default = 1;

        pt = &getserp(cp, seridx)->style.pdrop_line;

        if(!pt->fg.manual)
            {
            /* use point-derived style ... */
            (void) gr_point_linestyle_query(cp, seridx, point, style);

            /* ... but still treat as defaulting from here */
            return(using_default);
            }
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_pdrop_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_PDROP_LINESTYLE));
}

/******************************************************************************
*
* point lists are rooted in series descriptors
*
******************************************************************************/

extern S32
gr_point_list_delete(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    P_NLISTS_BLK lbrp;

    lbrp = gr_point_list_getlbrp(getserp(cp, seridx), cbp);

    trace_3(TRACE_MODULE_GR_CHART,
            "gr_point_list_delete seridx %u %s list &%p\n",
            seridx, cbp->list_name, report_ptr_cast(lbrp));

    /* remove refs before delete */
    if(cbp->rerefproc)
        (* cbp->rerefproc) (cp, seridx, 0);

    collect_delete(lbrp);

    return(1);
}

extern S32
gr_point_list_duplic(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    NLISTS_BLK   new_lbr = cbp->nlb;
    P_NLISTS_BLK lbrp;
    S32          res;

    lbrp = gr_point_list_getlbrp(getserp(cp, seridx), cbp);

    trace_3(TRACE_MODULE_GR_CHART,
            "gr_point_list_duplic seridx %u %s list &%p\n",
            seridx, cbp->list_name, report_ptr_cast(lbrp));

    new_lbr.lbr = NULL;

    res = collect_copy(&new_lbr, lbrp);

    lbrp->lbr = new_lbr.lbr;

    /* add refs after successful duplic */
    if(res > 0)
        if(cbp->rerefproc)
            (* cbp->rerefproc) (cp, seridx, 1);

    return(res);
}

extern S32
gr_point_list_fillstyle_enum_for_save(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    P_NLISTS_BLK lbrp;
    LIST_ITEMNO key;
    P_GR_FILLSTYLE pt;

    lbrp = gr_point_list_getlbrp(getserp(cp, seridx), cbp);

    trace_3(TRACE_MODULE_GR_CHART,
            "gr_point_list_fillstyle_enum_for_save seridx %u %s list &%p\n",
            seridx, cbp->list_name, report_ptr_cast(lbrp));

    for(pt = collect_first(lbrp, &key); pt; pt = collect_next(lbrp, &key))
        {
        S32 cres;

        if((cres = gr_fillstyle_make_key_for_save(pt)) < 0) /* 0 is valid, no pict */
            return(cres);
        }

    return(1);
}

extern void
gr_pdrop_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    S32 add)
{
    assert(cp);

    common_list_fillstyle_reref((P_NLISTS_BLK) &getserp(cp, seridx)->lbr.pdrop_fill, add);
}

extern void
gr_point_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    S32 add)
{
    assert(cp);

    common_list_fillstyle_reref((P_NLISTS_BLK) &getserp(cp, seridx)->lbr.point_fill, add);
}

extern P_ANY
gr_point_list_first(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    _OutRef_    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(collect_first(gr_point_list_getlbrp(getserp(cp, seridx), cbp), p_key));
}

extern P_ANY
gr_point_list_next(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    _InoutRef_  P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

    return(collect_next(gr_point_list_getlbrp(getserp(cp, seridx), cbp), p_key));
}

static P_ANY
gr_point_list_search(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    LIST_ITEMNO item;

    item = point;

    return(collect_search(gr_point_list_getlbrp(getserp(cp, seridx), cbp), &item));
}

static S32
gr_point_list_set(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_ANY style,
    GR_LIST_ID list_id)
{
    P_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    LIST_ITEMNO item;

    item = point;

    return(common_list_set(gr_point_list_getlbrp(getserp(cp, seridx), cbp), item, style, cbp));
}

/******************************************************************************
*
* query/set the fill style associated with a data point
*
******************************************************************************/

extern S32
gr_point_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style)
{
    PC_GR_FILLSTYLE pt;
    P_GR_SERIES serp;
    S32 using_default = 0;

    pt = gr_point_list_search(cp, seridx, point, GR_LIST_POINT_FILLSTYLE);

    if(!pt)
        {
        using_default = 1;

        serp = getserp(cp, seridx);

        /* use series-derived style unless varying by point */
        if(serp->bits.point_vary_manual
                    ? serp->bits.point_vary
                    : gr_axesp_from_seridx(cp, seridx)->bits.point_vary)
            pt = gr_fillstyle_default(gr_point_external_from_key(point));
        else
            {
            pt = &serp->style.point_fill;

            if(!pt->fg.manual)
                pt = gr_fillstyle_default(gr_series_external_from_ix(cp, seridx));
            }
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_point_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style)
{
    return(gr_pdrop_point_common_fillstyle_set(cp, seridx, point, style, GR_LIST_POINT_FILLSTYLE));
}

/******************************************************************************
*
* query/set the line style associated with a data point
*
******************************************************************************/

extern S32
gr_point_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style)
{
    PC_GR_LINESTYLE pt;
    S32 using_default = 0;

    pt = gr_point_list_search(cp, seridx, point, GR_LIST_POINT_LINESTYLE);

    if(!pt)
        {
        using_default = 1;

        /* use series-derived style */
        pt = &getserp(cp, seridx)->style.point_line;

        if(!pt->fg.manual)
            pt = gr_linestyle_default();
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_point_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_LINESTYLE));
}

/******************************************************************************
*
* query/set the text style associated with a data point
*
******************************************************************************/

extern S32
gr_point_textstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_TEXTSTYLE style)
{
    PC_GR_TEXTSTYLE pt;
    S32           using_default = 0;

    pt = gr_point_list_search(cp, seridx, point, GR_LIST_POINT_TEXTSTYLE);

    if(!pt)
        {
        using_default = 1;

        /* use series-derived style */
        pt = &getserp(cp, seridx)->style.point_text;

        if(!pt->fg.manual)
            pt = &cp->text.style.base;
        }

    *style = *pt;

    return(using_default);
}

extern S32
gr_point_textstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_TEXTSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_TEXTSTYLE));
}

/******************************************************************************
*
* query chart style for point
*
******************************************************************************/

static S32
gr_point_chartstyle_query(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    /*out*/ P_ANY style,
    P_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    P_U8       bpt /*const*/;
    GR_AXES_NO axes;
    S32        using_default = 0;

    assert(cp);
    assert(style);
    assert(desc);

    bpt = gr_point_list_search(cp, seridx, point, (GR_LIST_ID) desc->list_id);

    if(!bpt)
        {
        using_default = 1;

        /* use series style */
        bpt = (P_U8) getserp(cp, seridx) + desc->ser_offset;

        if((*bpt & 1) == 0 /*!pt->bits.manual*/)
            {
            /* use axes style */
            axes = gr_axes_ix_from_seridx(cp, seridx);

            bpt = (P_U8) &cp->axes[axes] + desc->axes_offset;
            }
        }

    void_memcpy32(style, bpt, desc->style_size);

    return(using_default);
}

extern S32
gr_point_barchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARCHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_barchstyle_desc));
}

extern S32
gr_point_barchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_BARCHSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_BARCHSTYLE));
}

extern S32
gr_point_barlinechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARLINECHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_barlinechstyle_desc));
}

extern S32
gr_point_barlinechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_BARLINECHSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_BARLINECHSTYLE));
}

extern S32
gr_point_linechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINECHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_linechstyle_desc));
}

extern S32
gr_point_linechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINECHSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_LINECHSTYLE));
}

extern S32
gr_point_piechdisplstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHDISPLSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_piechdisplstyle_desc));
}

extern S32
gr_point_piechdisplstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_PIECHDISPLSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_PIECHDISPLSTYLE));
}

extern S32
gr_point_piechlabelstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHLABELSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_piechlabelstyle_desc));
}

extern S32
gr_point_piechlabelstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_PIECHLABELSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_PIECHLABELSTYLE));
}

extern S32
gr_point_scatchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_SCATCHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, seridx, point, style, &gr_chart_objid_scatchstyle_desc));
}

extern S32
gr_point_scatchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_SCATCHSTYLE style)
{
    return(gr_point_list_set(cp, seridx, point, style, GR_LIST_POINT_SCATCHSTYLE));
}

/******************************************************************************
*
* convert a seridx into an external series number
*
******************************************************************************/

extern GR_SERIES_NO
gr_series_external_from_ix(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx)
{
    if(cp->axes_max > 0)
        {
        if(seridx >= cp->axes[1].series.stt_idx)
            {
            /* paper over the crack */
            seridx -= cp->axes[1].series.stt_idx;
            seridx += cp->axes[0].series.end_idx;
            }
        }

    return(seridx + 1);
}

/******************************************************************************
*
* convert an external series number into a seridx
*
******************************************************************************/

extern GR_SERIES_IX
gr_series_ix_from_external(
    PC_GR_CHART cp,
    GR_SERIES_NO eseries)
{
    GR_SERIES_IX seridx;

    assert(eseries != 0);

    seridx = eseries - 1;

    if(seridx < cp->axes[0].series.end_idx)
        return(seridx);

    if(cp->axes_max > 0)
        {
        seridx -= cp->axes[0].series.end_idx; /* jump over the gap */
        seridx += cp->axes[1].series.stt_idx;
        }

    return(seridx);
}

extern P_GR_SERIES
gr_seriesp_from_external(
    PC_GR_CHART cp,
    GR_SERIES_NO eseries)
{
    GR_SERIES_IX seridx;

    seridx = gr_series_ix_from_external(cp, eseries);

    return(getserp(cp, seridx));
}

/* end of gr_scale.c */
