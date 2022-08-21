/* gr_chari.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Internal header file for the gr_chart module */

/* SKS May 1991 */

/*
required includes
*/

#ifndef          __bezier_h
#include "cmodules/bezier.h"
#endif

#ifndef          __funclist_h
#include "cmodules/funclist.h"
#endif

#ifndef          __gr_coord_h
#include "cmodules/gr_coord.h"
#endif

#ifndef          __handlist_h
#include "cmodules/handlist.h"
#endif

#ifndef          __mathnums_h
#include "cmodules/mathnums.h"
#endif

#ifndef          __mathxtr2_h
#include "cmodules/mathxtr2.h"
#endif

#ifndef          __collect_h
#include "cmodules/collect.h"
#endif

#ifndef __stringlk_h
#include "stringlk.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#if RISCOS

typedef struct _GR_RISCDIAG_TAGSTRIP_INFO * P_GR_RISCDIAG_TAGSTRIP_INFO;

typedef S32 (* gr_riscdiag_tagstrip_proc) (
    P_ANY handle,
    P_GR_RISCDIAG_TAGSTRIP_INFO p_info);

#define gr_riscdiag_tagstrip_proto(_e_s, _p_proc_gr_riscdiag_tagstrip) \
_e_s S32 _p_proc_gr_riscdiag_tagstrip( \
    P_ANY handle, \
    P_GR_RISCDIAG_TAGSTRIP_INFO p_info)

#endif

/*
*/

#if RISCOS
#define SYSCHARWIDTH_OS     16
#define SYSCHARHEIGHT_OS    32
#define SYSCHARWIDTH_PIXIT  gr_pixit_from_riscos(SYSCHARWIDTH_OS)
#define SYSCHARHEIGHT_PIXIT gr_pixit_from_riscos(SYSCHARHEIGHT_OS)
#endif

typedef /*unsigned*/ int GR_DATASOURCE_NO;
typedef /*unsigned*/ int GR_AXES_NO; typedef GR_AXES_NO * P_GR_AXES_NO;

typedef struct _gr_minmax_number
{
    GR_CHART_NUMBER min, max;
}
GR_MINMAX_NUMBER;

/*
an index into the series table of a chart
*/
typedef /*unsigned*/ int GR_SERIES_IX;

/*
chart object 'names'
*/

typedef enum
{
    GR_CHART_OBJNAME_ANON       = 0,

    GR_CHART_OBJNAME_CHART      = 1, /* only one of these per chart */
    GR_CHART_OBJNAME_PLOTAREA   = 2, /* now 0..2 */
    GR_CHART_OBJNAME_LEGEND     = 3, /* only one of these per chart */

    GR_CHART_OBJNAME_TEXT       = 4, /* as many as you want */

    GR_CHART_OBJNAME_SERIES     = 5,
    GR_CHART_OBJNAME_POINT      = 6,

    GR_CHART_OBJNAME_DROPSER    = 7,
    GR_CHART_OBJNAME_DROPPOINT  = 8,

    GR_CHART_OBJNAME_AXIS       = 9, /* 0..5 */
    GR_CHART_OBJNAME_AXISGRID   = 10,
    GR_CHART_OBJNAME_AXISTICK   = 11,

    GR_CHART_OBJNAME_BESTFITSER = 12

    /* if it gets more than 15 here, increase below bitfield size */
}
GR_CHART_OBJID_NAME;

/*
this is a 32-bit id
*/

typedef struct _GR_CHART_OBJID
{
    UBF name      : 4; /*GR_CHART_OBJID_NAME :  C can't use enum as synonym for int in bitfields*/

    UBF reserved  : (sizeof(U8)*8-4-1-1); /* pack up to char boundary after has_xxx bits */

    UBF has_no    : 1;
    UBF has_subno : 1;

    UBF no        : 8;

#if WINDOWS
    UBF           : 0; /* align at word boundary */
#endif

    UBF subno     : 16;
}
GR_CHART_OBJID, * P_GR_CHART_OBJID; typedef const GR_CHART_OBJID * PC_GR_CHART_OBJID;

#define gr_chart_objid_clear(p_id) * (U32 *) (p_id) = 0

#define BUF_MAX_GR_CHART_OBJID_REPR 32

#define GR_CHART_OBJID_SUBNO_MAX U16_MAX

/*
an array of data sources is held on a per chart basis
*/

#define GR_DATASOURCE_HANDLE GR_INT_HANDLE /* abstract, unique non-repeating number */

#define GR_DATASOURCE_HANDLE_NONE    ((GR_INT_HANDLE) 0)
#define GR_DATASOURCE_HANDLE_START   ((GR_INT_HANDLE) 1) /* used for internal label adding */
#define GR_DATASOURCE_HANDLE_TEXTS   ((GR_INT_HANDLE) 2) /* used for external texts adding */

typedef struct _gr_datasource
{
    GR_DATASOURCE_HANDLE dsh;        /* unique non-repeating number */

    gr_chart_travelproc  ext_proc;   /* callback to owner */
    P_ANY                ext_handle; /* using this handle */

    GR_CHART_OBJID       id;         /* object using this data source */

    /* validity bits for cached items */
    struct _gr_datasource_valid
    {
        UBF n_items : 1;
    }
    valid;

    /* cached items */
    struct _gr_datasource_cache
    {
        GR_CHART_ITEMNO n_items;
    }
    cache;
}
GR_DATASOURCE, * P_GR_DATASOURCE, ** P_P_GR_DATASOURCE; typedef const GR_DATASOURCE * PC_GR_DATASOURCE;

/*
a clutch of datasources for plotting porpoises
*/

typedef struct _GR_DATASOURCE_FOURSOME
{
    GR_DATASOURCE_HANDLE value_x, value_y;
    GR_DATASOURCE_HANDLE error_x, error_y;
}
GR_DATASOURCE_FOURSOME, * P_GR_DATASOURCE_FOURSOME;

/*
style options for bar charts
*/

typedef struct _gr_barchstyle
{
    struct _gr_series_barchstyle_bits
    {
        UBF manual                   : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF pictures_stacked         : 1;
        UBF pictures_stacked_clipper : 1;

        UBF reserved                 : sizeof(U16)*8 - 2*1 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 slot_width_percentage;
}
GR_BARCHSTYLE, * P_GR_BARCHSTYLE; typedef const GR_BARCHSTYLE * PC_GR_BARCHSTYLE;

/*
style options for line charts
*/

typedef struct _gr_linechstyle
{
    struct _gr_series_linechstyle_bits
    {
        UBF manual    : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF reserved  : sizeof(U16)*8 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 slot_width_percentage;
}
GR_LINECHSTYLE, * P_GR_LINECHSTYLE; typedef const GR_LINECHSTYLE * PC_GR_LINECHSTYLE;

/*
style options common to both bar and line charts
*/

typedef struct _gr_barlinechstyle
{
    struct _gr_series_barlinechstyle_bits
    {
        UBF manual    : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF label_cat : 1;
        UBF label_val : 1;
        UBF label_pct : 1;

        UBF reserved  : sizeof(U16)*8 - 3*1 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 slot_depth_percentage; /* 3D only */
}
GR_BARLINECHSTYLE, * P_GR_BARLINECHSTYLE; typedef const GR_BARLINECHSTYLE * PC_GR_BARLINECHSTYLE;

/*
style options for pie charts
*/

typedef struct _gr_piechdiscplstyle
{
    struct _gr_series_piechdisplstyle_bits
    {
        UBF manual                   : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF reserved                 : sizeof(U16)*8 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 radial_displacement;
}
GR_PIECHDISPLSTYLE, * P_GR_PIECHDISPLSTYLE; typedef const GR_PIECHDISPLSTYLE * PC_GR_PIECHDISPLSTYLE;

typedef struct _gr_piechlabelstyle
{
    struct _gr_series_piechlabelstyle_bits
    {
        UBF manual                   : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF label_leg : 1;
        UBF label_val : 1;
        UBF label_pct : 1;

        UBF reserved                 : sizeof(U16)*8 - 3*1 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */
}
GR_PIECHLABELSTYLE, * P_GR_PIECHLABELSTYLE; typedef const GR_PIECHLABELSTYLE * PC_GR_PIECHLABELSTYLE;

/*
style options for scatter charts
*/

typedef struct _gr_scatchstyle
{
    struct _gr_series_scatchstyle_bits
    {
        UBF manual    : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF label_xval : 1;
        UBF label_yval : 1;
        UBF label__pct : 1; /* meaningless for scatter */

        UBF lines_off : 1;

        UBF reserved  : sizeof(U16)*8 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 width_percentage;
}
GR_SCATCHSTYLE, * P_GR_SCATCHSTYLE; typedef const GR_SCATCHSTYLE * PC_GR_SCATCHSTYLE;

/*
base plot style for axes/series
*/

typedef enum
{
    GR_CHARTTYPE_NONE = 0,

    GR_CHARTTYPE_PIE,
    GR_CHARTTYPE_BAR,
    GR_CHARTTYPE_LINE,
    GR_CHARTTYPE_SCAT
}
GR_CHARTTYPE;

/*
series (one or more per axes set)
*/

/*
type of a series
*/

typedef enum
{
    GR_CHART_SERIES_PLAIN  = 0,    /* requires 1 value source */
    GR_CHART_SERIES_PLAIN_ERROR1,  /* requires 1 value source and 1 error source */
    GR_CHART_SERIES_PLAIN_ERROR2,  /* requires 1 value source and 2 error sources */

    GR_CHART_SERIES_POINT,         /* requires 2 value sources */
    GR_CHART_SERIES_POINT_ERROR1,  /* requires 2 value sources and 1 error source */
    GR_CHART_SERIES_POINT_ERROR2   /* requires 2 value sources and 2 error sources */

#if 0
    GR_CHART_SERIES_HILOCLOSE,     /* requires 3 value sources */
    GR_CHART_SERIES_HILOOPENCLOSE  /* requires 4 value sources */
#endif
}
GR_SERIES_TYPE;

typedef struct _gr_series
{
    /* cloning of series descriptors starts here */

    GR_SERIES_TYPE sertype;         /* determine number of sources needed and how used */
    GR_CHARTTYPE   charttype;       /* base type */

    struct _gr_series_bits
    {
        UBF cumulative        : 1; /* s(n) = sum(s(i)...s(1)) for s(i) >= 0 */
        UBF cumulative_manual : 1; /* whether derived from axes default or set by punter */

        UBF point_vary        : 1; /* automatic variation between points in the same series */
        UBF point_vary_manual : 1; /* whether derived from axes default or set by punter */

        UBF best_fit          : 1;
        UBF best_fit_manual   : 1;

        UBF fill_axis         : 1; /* fill between line and axis */
        UBF fill_axis_manual  : 1; /* whether derived from axes default or set by punter */

        UBF pie_anticlockwise : 1;

        UBF reserved          : sizeof(U32)*8 - 1 - 4*2;
    }
    bits; /* U32 for sys indep expansion */

    struct _gr_series_style
    {
        F64                pie_start_heading;

        GR_FILLSTYLE       pdrop_fill;
        GR_LINESTYLE       pdrop_line;

        GR_FILLSTYLE       point_fill;
        GR_LINESTYLE       point_line;
        GR_TEXTSTYLE       point_text;

        GR_BARCHSTYLE      point_barch;
        GR_BARLINECHSTYLE  point_barlinech;
        GR_LINECHSTYLE     point_linech;
        GR_PIECHDISPLSTYLE point_piechdispl;
        GR_PIECHLABELSTYLE point_piechlabel;
        GR_SCATCHSTYLE     point_scatch;

        GR_LINESTYLE       bestfit_line;
    }
    style;

    /* cloning stops here. NB. things must refer to offsetof(GR_SERIES, GR_SERIES_CLONE_END) */

#define GR_SERIES_CLONE_END internal_bits

    struct _gr_series_internal_bits
    {
        UBF point_vary_manual_pie_bodge : 1;

        UBF descriptor_ok               : 1;

        /* not exported, size irrelevant */
    }
    internal_bits;

#define GR_SERIES_MAX_DATASOURCES 4
    struct _gr_series_datasources
    {
        GR_DATASOURCE_NO     n;     /* how many data sources are contributing to this series */
        GR_DATASOURCE_NO     n_req; /* how many data sources are required to contribute to this series */
        GR_DATASOURCE_HANDLE dsh[GR_SERIES_MAX_DATASOURCES]; /* no more than this data sources per series! */
    }
    datasources;

    struct _gr_series_lbr
    {
        P_LIST_BLKREF pdrop_fill;
        P_LIST_BLKREF pdrop_line;

        P_LIST_BLKREF point_fill;
        P_LIST_BLKREF point_line;
        P_LIST_BLKREF point_text;

        P_LIST_BLKREF point_barch;
        P_LIST_BLKREF point_barlinech;
        P_LIST_BLKREF point_linech;
        P_LIST_BLKREF point_piechdispl;
        P_LIST_BLKREF point_piechlabel;
        P_LIST_BLKREF point_scatch;
    }
    lbr;

    /* validity bits for cached items */
    struct _gr_series_valid
    {
        UBF n_items_total : 1;

        UBF limits  : 1;

        /* not exported, size irrelevant */
    }
    valid;

    /* cached items */
    struct _gr_series_cache
    {
        /* number of items contributing to the series */
        GR_CHART_ITEMNO  n_items_total;

        /* actual min/max limits from this series in its given
         * plotstyle eg. taking error bars, log axes etc. into account
        */
        GR_MINMAX_NUMBER limits_x;
        GR_MINMAX_NUMBER limits_y;

        F64 best_fit_c;
        F64 best_fit_m;
    }
    cache;
}
GR_SERIES, * P_GR_SERIES; typedef const GR_SERIES * PC_GR_SERIES;

/*
an 'external' point numbering type - starts at 1
*/
typedef /*unsigned*/ int GR_POINT_NO;

/*
descriptor for an individual axis
*/

/*
where an axis is drawn
*/

#define GR_AXIS_POSITION_ZERO     0 /* zero or closest edge of plotted area */
#define GR_AXIS_POSITION_LEFT     1 /* left(bottom) edge of plotted area */
#define GR_AXIS_POSITION_BOTTOM   GR_AXIS_POSITION_LEFT
#define GR_AXIS_POSITION_RIGHT    2 /* right(top) edge of plotted area */
#define GR_AXIS_POSITION_TOP      GR_AXIS_POSITION_RIGHT

#define GR_AXIS_POSITION_LZR_BITS 2 /* one spare entry */

#define GR_AXIS_POSITION_AUTO     0 /* deduce position from lzr in 3d */
#define GR_AXIS_POSITION_REAR     1
#define GR_AXIS_POSITION_FRONT    2

#define GR_AXIS_POSITION_ARF_BITS 2 /* one spare entry */

/*
where ticks are drawn on the axis
*/

#define GR_AXIS_TICK_POSITION_FULL        0 /* fully across axis */
#define GR_AXIS_TICK_POSITION_NONE        1 /* unticked */
#define GR_AXIS_TICK_POSITION_HALF_LEFT   2 /* to left(bottom) of axis */
#define GR_AXIS_TICK_POSITION_HALF_BOTTOM GR_AXIS_TICK_POSITION_HALF_LEFT
#define GR_AXIS_TICK_POSITION_HALF_RIGHT  3 /* to right(top) of axis */
#define GR_AXIS_TICK_POSITION_HALF_TOP    GR_AXIS_TICK_POSITION_HALF_RIGHT

#define GR_AXIS_TICK_SHAPE_BITS           3 /* several spare entries */

typedef struct _gr_axis_ticks
{
    struct _gr_axis_ticks_bits
    {
        UBF decimals : 10; /* for use in decimal places rounding of value labels */

        UBF manual   : 1;  /* if punter has specified values */
        UBF grid     : 1;

        UBF tick     : 3;  /*GR_AXIS_TICK_SHAPE_BITS*/ /* must be big enough to fit a GR_AXIS_TICK_SHAPE */

        UBF reserved : sizeof(U32)*8 - 10 - 2*1 - 3;
    }
    bits; /* U32 for sys indep expansion */

    GR_CHART_NUMBER punter;
    GR_CHART_NUMBER current;

    struct _gr_axis_ticks_style
    {
        GR_LINESTYLE grid;
        GR_LINESTYLE tick;
    }
    style;
}
GR_AXIS_TICKS, * P_GR_AXIS_TICKS; typedef const GR_AXIS_TICKS * PC_GR_AXIS_TICKS;

typedef struct _gr_axis
{
    GR_MINMAX_NUMBER punter;  /* punter specified min/max for this axis */
    GR_MINMAX_NUMBER actual;  /* actual min/max for the assoc data */
    GR_MINMAX_NUMBER current;

    GR_CHART_NUMBER  current_span; /* current.max - current.min */
    GR_CHART_NUMBER  zero_frac;    /* how far up the axis is the zero line? */
    GR_CHART_NUMBER  current_frac; /* major span as a fraction of the current span */

    struct _gr_axis_bits
    {
        UBF manual       : 1; /* if punter has specified values */
        UBF incl_zero    : 1; /* axis scaling forced to include zero */
        UBF log_scale    : 1; /* log scaled? */
        UBF pow_label    : 1; /* pow labelled? */

        UBF log_base     : 4;

        UBF lzr          : 2; /*GR_AXIS_POSITION_LZR_BITS*/ /* must be big enough to fit a GR_AXIS_POSITION_LZR */
        UBF arf          : 2; /*GR_AXIS_POSITION_ARF_BITS*/ /* must be big enough to fit a GR_AXIS_POSITION_ARF */

        UBF reverse      : 1; /* min/max reversal */

        UBF reserved_for_trunc_stagger_state : 3; /* big enough for a stagger of 8; 0 -> truncated; 1 -> cycle of 2 etc.*/

        UBF reserved     : sizeof(U32)*8 - 1 - 3 - 2*2 - 1*4 - 4*1;
    }
    bits; /* U32 for sys indep expansion */

    struct _gr_axis_style
    {
        GR_LINESTYLE axis;
        GR_TEXTSTYLE label;
    }
    style;

    GR_AXIS_TICKS major, minor;
}
GR_AXIS, * P_GR_AXIS; typedef const GR_AXIS * PC_GR_AXIS;

enum
{
    GR_CHART_AXISTICK_MAJOR = 1,
    GR_CHART_AXISTICK_MINOR
};

/*
descriptor for an axes set and how series belonging to it are plotted
*/

typedef struct _gr_axes
{
    GR_SERIES_TYPE sertype;         /* default series type for series creation on these axes */
    GR_CHARTTYPE   charttype;       /* default chart type ... */

    struct _gr_axes_bits
    {
        UBF cumulative  : 1;        /* default series cumulative state */
        UBF point_vary  : 1;        /* default series vary style by point state */
        UBF best_fit    : 1;        /* default series line of best fit state */
        UBF fill_axis   : 1;        /* default series fill to axis state */

        UBF stacked     : 1;        /* series plotted on these axes (bars & lines only) are stacked */
        UBF stacked_pct : 1;        /* series plotted on these axes (mostly as bars) are 100% stacked */

        UBF reserved    : sizeof(U32)*8 - 6;
    }
    bits; /* U32 for sys indep expansion */

    struct _gr_axes_series
    {
        /* SKS after 4.12 26mar92 - what series no this axes set should start at (-ve -> user forced, +ve auto) */
        S32 start_series;

        S32 stt_idx; /* what seridx this axes set starts at */
        S32 end_idx;
    }
    series;

    struct _gr_axes_style
    {
        GR_BARCHSTYLE      barch;
        GR_BARLINECHSTYLE  barlinech;
        GR_LINECHSTYLE     linech;
        GR_PIECHDISPLSTYLE piechdispl;
        GR_PIECHLABELSTYLE piechlabel;
        GR_SCATCHSTYLE     scatch;
    }
    style;

    struct _gr_axes_cache
    {
        GR_SERIES_NO n_contrib_bars;
        GR_SERIES_NO n_contrib_lines;
        GR_SERIES_NO n_series; /* for loading and shaving */
    }
    cache;

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
    GR_AXIS axis[3]; /* x, y, z */
}
GR_AXES, * P_GR_AXES; typedef const struct _gr_axes * PC_GR_AXES;

enum
{
    GR_CHART_PLOTAREA_REAR = 0,
    GR_CHART_PLOTAREA_WALL,
    GR_CHART_PLOTAREA_FLOOR,

    GR_CHART_N_PLOTAREAS
};

/*
a chart
*/

typedef struct _gr_chart
{
    struct _gr_chart_core /* the core of the chart - preserve over clone */
    {
        GR_CHART_HANDLE     ch;              /* the handle under which this structure is exported */

        GR_CHARTEDIT_HANDLE ceh;             /* the handle of the single chart editor that is allowed on this chart */

        char *              currentfilename; /* full pathname of where (if anywhere) this file is stored */
        char *              currentdrawname; /* full pathname of where (if anywhere) this file was stored as a Draw file */
        GR_DATASOURCE       category_datasource;

        P_ANY               ext_handle;      /* a handle which the creator of the chart passed us */

        struct _gr_chart_datasources
        {
            GR_DATASOURCE_NO n;        /* number in use */
            GR_DATASOURCE_NO n_alloc;  /* how many data sources handle has been allocated for */
            GR_DATASOURCE *  mh;       /* ptr to data source descriptor array */
        }
        datasources;

        struct _gr_chart_layout
        {
            GR_COORD width;
            GR_COORD height;

            struct _gr_chart_layout_margins
            {
                GR_COORD left;
                GR_COORD bottom;
                GR_COORD right;
                GR_COORD top;
            }
            margins;

            GR_POINT size;              /* x = width - (margin.left + margin.right) etc. never -ve */
        }
        layout;

        P_GR_DIAG p_gr_diag; /* the diagram constructed by this chart */

        S32 modified;

        struct _gr_chart_core_editsave
        {
#if RISCOS
            wimp_box  open_box;
            GR_OSUNIT open_scx, open_scy;
#else
            char foo;
#endif
        }
        editsave;
    }
    core;

    struct _gr_chart_chart           /* attributes of 'chart' object */
    {
        GR_FILLSTYLE areastyle;
        GR_LINESTYLE borderstyle;
    }
    chart;

    struct _gr_chart_plotarea        /* attributes of 'plotarea' object */
    {
        GR_POINT posn;              /* offset of plotted area in chart */
        GR_POINT size;              /* how big the plotted area is */

        struct _gr_chart_plotarea_area
        {
            GR_FILLSTYLE areastyle;
            GR_LINESTYLE borderstyle;
        }
        area[GR_CHART_N_PLOTAREAS];
    }
    plotarea;

    struct _gr_chart_legend          /* attributes of 'legend' object */
    {
        struct _gr_chart_legend_bits
        {
            UBF on       : 1;
            UBF in_rows  : 1;
            UBF manual   : 1; /* manually repositioned */

            UBF reserved : sizeof(U32)*8 - 3*1;
        }
        bits; /* U32 for sys indep expansion */

#if RISCOS
#define GR_CHART_LEGEND_HANG_LEFT gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_HANG_DOWN gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_LM gr_pixit_from_riscos(12)
#define GR_CHART_LEGEND_RM gr_pixit_from_riscos(12)
#define GR_CHART_LEGEND_BM gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_TM gr_pixit_from_riscos(8)
#endif
        GR_POINT posn;         /* offset in chart */
        GR_POINT size;         /* computed size */

        GR_FILLSTYLE   areastyle;
        GR_LINESTYLE   borderstyle;

        GR_TEXTSTYLE   textstyle;
    }
    legend;

    struct _gr_chart_series
    {
        GR_SERIES_IX n;
        GR_SERIES_IX n_alloc;   /* how many series handle has been allocated for */
        GR_SERIES_IX n_defined; /* how many are defined */
        P_GR_SERIES   mh;       /* ptr to series descriptor array */

        GR_SERIES_NO overlay_start; /* 0 -> auto */
    }
    series;

#define GR_AXES_MAX 1
    GR_AXES_NO        axes_max;              /* how many axes sets are in use (-1 as there's always at least one) */
    GR_AXES           axes[GR_AXES_MAX + 1]; /* two independent sets of axes you can plot on (these hold series descriptors) */

    struct _gr_chart_text
    {
        P_LIST_BLKREF    lbr; /* list of gr_texts */

        struct _gr_chart_text_style
        {
            P_LIST_BLKREF    lbr; /* list of attributes for the gr_texts */

            GR_TEXTSTYLE   base; /* base text style for text objects */
        }
        style;
    }
    text;

    struct _gr_chart_bits
    {
        UBF realloc_series : 1; /* need to reallocate ds to series */

        /* not exported, size irrelevant */
    }
    bits;

    struct _gr_chart_d3
    {
        struct _gr_chart_d3_bits
        {
            UBF on       : 1; /* 3-D embellishment? applies to whole chart */
            UBF use      : 1; /* whether 3D is in use, eg pie & scat turn off */

            UBF reserved : sizeof(U32)*8 - 1;
        }
        bits; /* U32 for sys indep expansion */

        F64 pitch;            /* angle pitched about a horizontal axis, bringing top into view */
        F64 roll;             /* angle pitched about the vertical axis, bringing side into view */

        /* validity bits for cached items */
        struct _gr_chart_d3_valid
        {
            UBF vector : 1;

            /* not exported, size irrelevant */
        }
        valid;

        /* cached items */
        struct _gr_chart_d3_cache
        {
            GR_POINT vector;            /* how much a point is displaced by per series in depth */

            GR_POINT vector_full;       /* full displacement from front to back */
        }
        cache;
    }
    d3;

    struct _gr_chart_cache
    {
        GR_SERIES_NO n_contrib_series; /* how many series are contributing to this chart from all axes */
        GR_SERIES_NO n_contrib_bars;
        GR_SERIES_NO n_contrib_lines;
    }
    cache;

    struct _gr_chart_barch
    {
        F64 slot_overlap_percentage;

        struct _gr_chart_barch_cache
        {
            GR_COORD slot_width;
            GR_COORD slot_shift;
        }
        cache;
    }
    barch;

    struct _gr_chart_barlinech
    {
        struct _gr_chart_barlinech_cache
        {
            GR_COORD cat_group_width;
            GR_COORD zeropoint_y;
        }
        cache;
    }
    barlinech;

    struct _gr_chart_linech
    {
        F64 slot_shift_percentage;

        struct _gr_chart_line_cache
        {
            GR_COORD slot_start;
            GR_COORD slot_shift;
        }
        cache;
    }
    linech;

    GR_SERIES_IX pie_seridx;
    }
GR_CHART, * P_GR_CHART; typedef const GR_CHART * PC_GR_CHART;

typedef struct _gr_text
{
    GR_BOX box;

    struct _gr_text_bits
    {
        UBF being_edited : 1;  /* use during discard to destroy any editors of text objects */
        UBF unused       : 1;
        UBF live_text    : 1;

        UBF reserved     : sizeof(U16)*8 - 3*1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    /* actual text or datasource follows */
}
GR_TEXT, * P_GR_TEXT; typedef const GR_TEXT * PC_GR_TEXT;

typedef union _gr_text_gutsp
{
    P_GR_DATASOURCE dsp;
    P_U8            textp;
    P_ANY           mp;
}
P_GR_TEXT_GUTS;

/*
structure used for caching series data as we go across a chart

also used for line of best fit data
*/

typedef struct _GR_BARLINESCATCH_SERIES_CACHE
{
    P_GR_CHART              cp;
    GR_AXES_NO              axes;
    GR_SERIES_IX            seridx;
    GR_POINT_NO             n_points;

    S32                     barindex;
    S32                     lineindex;
    S32                     plotindex;

    S32                     cumulative;
    S32                     best_fit;
    S32                     fill_axis;

    GR_CHART_OBJID          serid;
    GR_CHART_OBJID          drop_serid;
    GR_CHART_OBJID          point_id;
    GR_CHART_OBJID          pdrop_id;

    GR_DATASOURCE_FOURSOME  dsh;

    /* cumulative series cache */
    GR_CHART_NUMPAIR        old_value; /* reset each pass */
    GR_CHART_NUMPAIR        old_error; /* reset each pass */

    /* ribbon-can-be-drawn-back_to cache */
    S32                     had_first; /* reset each pass */

    GR_POINT                old_valpoint;
    GR_POINT                old_botpoint;
    GR_POINT                old_toppoint;

    F64                     slot_depth_percentage; /* to place best fit line at front of all points */
}
GR_BARLINESCATCH_SERIES_CACHE, * P_GR_BARLINESCATCH_SERIES_CACHE;

/*
structure used for callback from linest()
*/

typedef struct _GR_BARLINESCATCH_LINEST_STATE
{
    GR_DATASOURCE_FOURSOME dsh;
    P_GR_CHART             cp;
    F64                    y_cum; /* cumulative y */
    S32                    x_log;
    S32                    y_log;

    /* out */
    F64                    a[2]; /* just m and c please */
}
GR_BARLINESCATCH_LINEST_STATE, * P_GR_BARLINESCATCH_LINEST_STATE;

/*
internally exported functions from gr_chart.c
*/

extern S32
gr_chart_add_series(
    P_GR_CHART  cp,
    GR_AXES_NO axes,
    S32        init);

extern P_GR_CHART
gr_chart_cp_from_ch(
    GR_CHART_HANDLE ch);

extern S32
gr_chart_dsp_from_dsh(
    P_GR_CHART cp,
    _OutRef_    P_P_GR_DATASOURCE dspp,
    GR_DATASOURCE_HANDLE dsh);

extern S32
gr_chart_legend_addin(
    P_GR_CHART cp);

extern void
gr_chart_object_name_from_id(
    /*out*/ P_U8 szName,
    _InVal_ U32 elemof_buffer,
    PC_GR_CHART_OBJID id);

extern PC_U8
gr_chart_object_name_from_id_quick(
    PC_GR_CHART_OBJID id);

extern S32
gr_chart_objid_find_parent(
    _InoutRef_  P_GR_CHART_OBJID id);

extern S32
gr_chart_realloc_series(
    P_GR_CHART cp);

extern S32
gr_chart_set_n_axes(
    P_GR_CHART  cp,
    GR_AXES_NO axes);

extern S32
gr_chart_subtract_datasource_using_dsh(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh);

extern S32
gr_chart_group_new(
    P_GR_CHART cp,
    P_GR_DIAG_OFFSET  groupStart,
    PC_GR_CHART_OBJID id);

extern S32
gr_chart_scaled_picture_add(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_BOX box,
    PC_GR_FILLSTYLE style);

extern GR_CHART_ITEMNO
gr_travel_axes_n_items_total(
    P_GR_CHART cp,
    GR_AXES_NO axes);

extern void
gr_travel_categ_label(
    P_GR_CHART cp,
    GR_CHART_ITEMNO item,
    /*out*/ P_GR_CHART_VALUE pValue);

extern S32
gr_travel_dsh(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO item,
    /*out*/ P_GR_CHART_VALUE pValue);

extern void
gr_travel_dsh_label(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO item,
    /*out*/ P_GR_CHART_VALUE pValue);

extern GR_CHART_ITEMNO
gr_travel_dsh_n_items(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh);

extern S32
gr_travel_dsh_valof(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO item,
    _OutRef_    P_F64 value);

extern S32
gr_travel_dsp(
    P_GR_CHART cp,
    P_GR_DATASOURCE dsp,
    GR_CHART_ITEMNO item,
    /*out*/ P_GR_CHART_VALUE pValue);

extern GR_DATASOURCE_HANDLE
gr_travel_series_dsh_from_ds(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_DATASOURCE_NO ds);

extern void
gr_travel_series_label(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    /*out*/ P_GR_CHART_VALUE pValue);

extern GR_CHART_ITEMNO
gr_travel_series_n_items(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_DATASOURCE_NO ds);

extern GR_CHART_ITEMNO
gr_travel_series_n_items_total(
    P_GR_CHART cp,
    GR_SERIES_IX seridx);

/*
internally exported variables from gr_chart.c
*/

extern /*const-to-you*/ GR_CHART_HANDLE
gr_chart_preferred_ch;

extern const GR_CHART_OBJID
gr_chart_objid_anon;

extern const GR_CHART_OBJID
gr_chart_objid_chart;

extern const GR_CHART_OBJID
gr_chart_objid_legend;

/*
get a series descriptor pointer from seridx on a chart
*/

#define getserp(cp, seridx) \
    ((cp)->series.mh + (seridx))

/*
end of internal exports from gr_chart.c
*/

/*
internally exported functions from gr_chtIO.c
*/

extern S32
gr_fillstyle_make_key_for_save(
    PC_GR_FILLSTYLE style);

extern S32
gr_chart_save_internal(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 filename /*const*/);

extern void
gr_chart_save_chart_winge(
    S32 error_id,
    S32 res);

/*
end of internal exports from gr_chtIO.c
*/

/*
internal exports from gr_editc.c
*/

/*
a chart editor structure
*/

typedef struct _gr_charteditor
{
    GR_CHARTEDIT_HANDLE      ceh;           /* the handle under which this structure is exported */

    GR_CHART_HANDLE          ch;            /* which chart is being edited */

    gr_chartedit_notify_proc notifyproc;
    P_ANY                    notifyhandle;

    GR_POINT                size;         /* oh yes, this is needed */

    struct _gr_chartedit_selection
    {
        GR_CHART_OBJID           id;        /* which object got selected? */

        P_GR_DIAG                p_gr_diag; /* the diagram representing the current selection */

        BOOL                     temp;      /* temporary selection for MENU? */

        GR_DIAG_OFFSET           hitObject; /* offset of selection in diagram */

        GR_BOX                   box;       /* a box which can be used to represent the selection */
    }
    selection;

#if RISCOS
    /* RISC OS specifics */

    struct _gr_chartedit_riscos
    {
        wimp_wind * ww;        /* a template_copy'ed window definition (keep for indirected data) */
        wimp_w      w;         /* wimp handle of chart editing window */

/* margins around chart in display */
#define GR_CHARTEDIT_DISPLAY_LM_OS 8
#define GR_CHARTEDIT_DISPLAY_RM_OS 8
#define GR_CHARTEDIT_DISPLAY_BM_OS 8
#define GR_CHARTEDIT_DISPLAY_TM_OS 8

        GR_OSUNIT     heading_size; /* total y size at the top of the window that is taken up by the icons */

        F64           scale_from_diag;     /* last scale factor used */
        GR_SCALE_PAIR scale_from_diag16;   /* and as a pair of 16.16 fixed nos. */
        GR_SCALE_PAIR scale_from_screen16; /* and the inverse. */

        S32           init_scx, init_scy;

        S32           diagram_off_x, diagram_off_y; /* work area offsets of diagram origin */

        S32           processing_fontselector;
    }
    riscos;
#elif WINDOWS
    /* WINDOWS specifics */

    struct _gr_chartedit_windows
    {
        char foo;
    }
    windows;
#endif

}
GR_CHARTEDITOR, * P_GR_CHARTEDITOR; /* data stored in deep space */

/*
internally exported functions from gr_editc.c
*/

extern P_GR_CHARTEDITOR
gr_chartedit_cep_from_ceh(
    GR_CHARTEDIT_HANDLE ceh);

extern void
gr_chartedit_diagram_update_later(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_set_scales(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_selection_clear(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_selection_clear_if_temp(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_selection_kill_repr(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_selection_make_repr(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_setwintitle(
    P_GR_CHART cp);

extern void
gr_chartedit_winge(
    S32 err);

extern void
gr_chartedit_warning(
    P_GR_CHART cp,
    S32 err);

#if RISCOS

extern unsigned int
gr_chartedit_riscos_correlate(
    P_GR_CHARTEDITOR cep,
    PC_GR_POINT point,
    _OutRef_    P_GR_CHART_OBJID id,
    /*out*/ P_GR_DIAG_OFFSET hitObject /*[]*/,
    size_t hitObjectDepth,
    BOOL adjustclicked);

extern void
gr_chartedit_riscos_point_from_abs(
    P_GR_CHART cp,
    _OutRef_    P_GR_POINT point,
    GR_OSUNIT x,
    GR_OSUNIT y);

#endif /* RISCOS */

/*
end of internal exports from gr_editc.c
*/

/*
internal exports from gr_editm.c
*/

/*
internally exported functions from gr_editm.c
*/

extern void
gr_chartedit_menu_finalise(
    P_GR_CHARTEDITOR cep);

extern S32
gr_chartedit_menu_initialise(
    P_GR_CHARTEDITOR cep);

/*
internally exported variables from gr_editm.c
*/

/*
end of internal exports from gr_editm.c
*/

/*
internal exports from gr_editt.c
*/

/*
internally exported functions from gr_editt.c
*/

extern S32
gr_text_addin(
    P_GR_CHART cp);

extern void
gr_text_delete(
    P_GR_CHART cp,
    LIST_ITEMNO key);

extern LIST_ITEMNO
gr_text_key_for_new(
    P_GR_CHART cp);

extern S32
gr_text_new(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    PC_U8 szText,
    PC_GR_POINT point);

extern void
gr_text_box_query(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    /*out*/ P_GR_BOX p_box);

extern S32
gr_text_box_set(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    _InRef_     PC_GR_BOX p_box);

extern void
gr_chartedit_fontselect_kill(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_selected_object_drag_start(
    P_GR_CHARTEDITOR cep,
    PC_GR_POINT point,
    PC_GR_POINT workareaoff);

extern void
gr_chartedit_selected_object_drag_complete(
    P_GR_CHARTEDITOR cep,
    const wimp_box * dragboxp);

extern S32
gr_chartedit_selection_fillstyle_edit(
    P_GR_CHARTEDITOR cep);

extern S32
gr_chartedit_selection_textstyle_edit(
    P_GR_CHARTEDITOR cep);

extern S32
gr_chartedit_selection_text_delete(
    P_GR_CHARTEDITOR cep);

extern S32
gr_chartedit_selection_text_edit(
    P_GR_CHARTEDITOR cep,
    BOOL submenurequest);

extern void
gr_chartedit_text_editor_kill(
    P_GR_CHART cp,
    LIST_ITEMNO key);

extern S32
gr_chartedit_text_new_and_edit(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    BOOL submenu,
    PC_U8 text,
    P_GR_POINT point);

/*
internally exported variables from gr_editt.c
*/

extern const NLISTS_BLK
gr_text_nlists_blk_proto;

/*
end of internal exports from gr_editt.c
*/

/*
internal exports from P_GR_AXIS.c -- axis processing
*/

extern GR_AXES_NO
gr_axes_external_from_ix(
    PC_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis);

extern GR_AXIS_NO
gr_axes_ix_from_external(
    PC_GR_CHART cp,
    GR_AXES_NO eaxes,
    _OutRef_    P_GR_AXES_NO axes);

extern GR_AXES_NO
gr_axes_ix_from_seridx(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx);

extern P_GR_AXES
gr_axesp_from_seridx(
    P_GR_CHART cp,
    GR_SERIES_IX seridx);

extern P_GR_AXIS
gr_axisp_from_external(
    P_GR_CHART cp,
    GR_AXES_NO eaxes);

extern S32
gr_axis_addin_category(
    P_GR_CHART cp,
    GR_POINT_NO total_n_points,
    S32 front);

extern S32
gr_axis_addin_value_x(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    S32 front);

extern S32
gr_axis_addin_value_y(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    S32 front);

extern void
gr_axis_form_category(
    P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points);

extern void
gr_axis_form_value(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis);

extern void
gr_chartedit_selection_axis_process(
    P_GR_CHARTEDITOR cep);

extern F64
gr_lin_major(
    F64 span);

/*
end of internal exports from P_GR_AXIS.c
*/

/*
internal exports from gr_diag.c -- diagram building
*/

/*
internally exported structure from gr_diag.c
*/

typedef struct _gr_diag_process_t
{
    UBF recurse : 1;          /* recurse into group objects */
    UBF recompute : 1;        /* ensure object bboxes recomputed */
    UBF severe_recompute : 1; /* go to system level and recompute too */
    UBF find_children : 1;    /* return children of the id being searched for */

    UBF reserved : 8*sizeof(U16)-1-1-1-1;
}
GR_DIAG_PROCESS_T;

/*
an id stored in the diagram
*/

#define GR_DIAG_OBJID_T GR_CHART_OBJID

/*
diagram
*/

typedef struct _GR_DIAG_DIAGHEADER
{
    GR_BOX bbox;

    TCHARZ szCreatorName[12 + 4]; /* must fit PDreamCharts and NULLCH */

    /* followed immediately by objects */
}
GR_DIAG_DIAGHEADER, * P_GR_DIAG_DIAGHEADER;

/*
objects
*/

/*
object 'names'
*/

#define GR_DIAG_OBJECT_NONE  ((GR_DIAG_OFFSET) 0U)
#define GR_DIAG_OBJECT_FIRST ((GR_DIAG_OFFSET) 1U)
#define GR_DIAG_OBJECT_LAST  ((GR_DIAG_OFFSET) 0xFFFFFFFFU)

#define GR_DIAG_OBJTYPE_NONE          0U
#define GR_DIAG_OBJTYPE_GROUP         1U
#define GR_DIAG_OBJTYPE_TEXT          2U
#define GR_DIAG_OBJTYPE_LINE          3U
#define GR_DIAG_OBJTYPE_RECTANGLE     4U
#define GR_DIAG_OBJTYPE_PIESECTOR     5U
#define GR_DIAG_OBJTYPE_was_MARKER    6U
#define GR_DIAG_OBJTYPE_PICTURE       7U
#define GR_DIAG_OBJTYPE_PARALLELOGRAM 8U
#define GR_DIAG_OBJTYPE_TRAPEZOID     9U

typedef U32 GR_DIAG_OBJTYPE;

#define GR_DIAG_OBJHDR_DEF    \
    GR_DIAG_OBJTYPE    tag;   \
    U32                size;  \
    GR_BOX             bbox;  \
    GR_DIAG_OBJID_T    objid; \
    GR_DIAG_OFFSET     sys_off /* offset in system-dependent representation of corresponding object */

typedef struct _gr_diag_objhdr
{
    GR_DIAG_OBJHDR_DEF;
}
GR_DIAG_OBJHDR;

/*
groups are simply encapulators
*/

typedef struct _gr_diag_objgroup
{
    GR_DIAG_OBJHDR_DEF;
    L1_U8Z name[12];
}
GR_DIAG_OBJGROUP;

/*
objects with position
*/

#define GR_DIAG_POSOBJHDR_DEF \
    GR_DIAG_OBJHDR_DEF; \
    GR_POINT pos

typedef struct _gr_diag_posobjhdr
{
    GR_DIAG_POSOBJHDR_DEF;
}
GR_DIAG_POSOBJHDR;

typedef struct _gr_diag_objline
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          d;
    GR_LINESTYLE      linestyle;
}
GR_DIAG_OBJLINE;

typedef struct _gr_diag_objrectangle
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          wid_hei;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJRECTANGLE;

typedef struct _gr_diag_objparallelogram
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          wid_hei1;
    GR_POINT          wid_hei2;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJPARALLELOGRAM;

typedef struct _gr_diag_objtrapezoid
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          wid_hei1;
    GR_POINT          wid_hei2;
    GR_POINT          wid_hei3;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJTRAPEZOID;

typedef struct _gr_diag_objpiesector
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_COORD          radius;
    F64               alpha, beta;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;

    GR_POINT          p0, p1;
}
GR_DIAG_OBJPIESECTOR;

typedef struct _gr_diag_objpicture
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          wid_hei;
    GR_CACHE_HANDLE   picture;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJPICTURE;

typedef struct _gr_diag_objtext
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          wid_hei;
    GR_TEXTSTYLE      textstyle;

    /* data stored in here */
}
GR_DIAG_OBJTEXT;

typedef union _gr_diag_objectp
{
    P_BYTE                    p_byte;

    GR_DIAG_OBJHDR *          hdr;

    GR_DIAG_OBJGROUP *        group;
    GR_DIAG_OBJLINE *         line;
    GR_DIAG_OBJRECTANGLE *    rect;
    GR_DIAG_OBJPARALLELOGRAM *para;
    GR_DIAG_OBJTRAPEZOID     *trap;
    GR_DIAG_OBJPIESECTOR *    pie;
    GR_DIAG_OBJPICTURE *      pict;
    GR_DIAG_OBJTEXT *         text;
}
P_GR_DIAG_OBJECT, * P_P_GR_DIAG_OBJECT;

/*
internally exported functions from gr_diag.c
*/

extern S32
gr_diag_diagram_correlate(
    P_GR_DIAG p_gr_diag,
    PC_GR_POINT point,
    PC_GR_POINT semimajor,
    /*out*/ P_GR_DIAG_OFFSET pHitObject /*[]*/,
    S32 recursionLimit);

extern void
gr_diag_diagram_dispose(
    _InoutRef_  P_P_GR_DIAG dcpp);

extern GR_DIAG_OFFSET
gr_diag_diagram_end(
    P_GR_DIAG p_gr_diag);

_Check_return_
_Ret_maybenull_
extern P_GR_DIAG
gr_diag_diagram_new(
    _In_z_      PCTSTR szCreatorName,
    _OutRef_    P_STATUS p_status);

extern void
gr_diag_diagram_reset_bbox(
    P_GR_DIAG p_gr_diag,
    GR_DIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_diag_diagram_save(
    P_GR_DIAG p_gr_diag,
    PC_U8 filename);

extern GR_DIAG_OFFSET
gr_diag_diagram_search(
    P_GR_DIAG p_gr_diag,
    GR_DIAG_OBJID_T objid);

extern U32
gr_diag_group_end(
    P_GR_DIAG p_gr_diag,
    PC_GR_DIAG_OFFSET pGroupStart);

_Check_return_
extern STATUS
gr_diag_group_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    GR_DIAG_OBJID_T objid,
    PC_U8 szGroupName);

_Check_return_
extern STATUS
gr_diag_line_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_GR_LINESTYLE linestyle);

extern U32
gr_diag_object_base_size(
    GR_DIAG_OBJTYPE objectType);

extern S32
gr_diag_object_correlate_between(
    P_GR_DIAG p_gr_diag,
    PC_GR_POINT point,
    PC_GR_POINT semimajor,
    /*out*/ P_GR_DIAG_OFFSET pHitObject /*[]*/,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    S32 recursionLimit);

extern U32
gr_diag_object_end(
    P_GR_DIAG p_gr_diag,
    PC_GR_DIAG_OFFSET pObjectStart);

extern BOOL
gr_diag_object_first(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InoutRef_  P_GR_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObjHdr);

_Check_return_
extern STATUS
gr_diag_object_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_opt_ P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObjHdr,
    GR_DIAG_OBJTYPE objectType,
    _InVal_     U32 extraBytes);

extern BOOL
gr_diag_object_next(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObjHdr);

extern void
gr_diag_object_reset_bbox_between(
    P_GR_DIAG p_gr_diag,
    _OutRef_    P_GR_BOX pBox,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process);

extern GR_DIAG_OFFSET
gr_diag_object_search_between(
    P_GR_DIAG p_gr_diag,
    GR_DIAG_OBJID_T objid,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_diag_parallelogram_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pOrigin,
    PC_GR_POINT ps1,
    PC_GR_POINT ps2,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_piesector_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pPos,
    GR_COORD radius,
    PC_F64 alpha,
    PC_F64 beta,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_rectangle_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_scaled_picture_add(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    GR_CACHE_HANDLE picture,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_text_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_USTR szText,
    PC_GR_TEXTSTYLE textstyle);

_Check_return_
extern STATUS
gr_diag_trapezoid_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pOrigin,
    PC_GR_POINT ps1,
    PC_GR_POINT ps2,
    PC_GR_POINT ps3,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

/*
internally exported functions as macros from gr_diag.c
*/

#define gr_diag_getoffptr(__base_type, p_gr_diag, offset) \
    array_ptr(&(p_gr_diag)->handle, __base_type, (offset))

#define gr_diag_query_offset(p_gr_diag) \
    array_elements32(&(p_gr_diag)->handle)

/*
end of internal exports from gr_diag.c
*/

/*
internal exports from gr_barch.c
*/

/*
internally exported functions from gr_barch.c
*/

_Check_return_
extern STATUS
gr_barlinechart_addin(
    P_GR_CHART cp);

extern GR_COORD
gr_categ_pos(
    PC_GR_CHART cp,
    GR_POINT_NO point);

extern void
gr_chart_d3_cache_vector(
    P_GR_CHART cp);

extern void
gr_map_point(
    /*inout*/ P_GR_POINT xpoint,
    P_GR_CHART cp,
    S32 plotindex);

extern void
gr_map_box_front_and_back(
    /*inout*/ P_GR_BOX xbox,
    P_GR_CHART cp);

extern void
gr_point_partial_z_shift(
    /*out*/ P_GR_POINT xpoint,
    PC_GR_POINT apoint,
    P_GR_CHART cp,
    const F64 * z_frac_p);

extern GR_COORD
gr_value_pos(
    PC_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis,
    const GR_CHART_NUMBER * value);

extern GR_COORD
gr_value_pos_rel(
    PC_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis,
    const GR_CHART_NUMBER * value);

/*
end of internal exports from gr_barch.c
*/

/*
internal exports from gr_blgal.c
*/

/*
internally exported functions from gr_blgal.c
*/

#if RISCOS

extern void
gr_chartedit_gallery_bar_process(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_gallery_line_process(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_gallery_pie_process(
    P_GR_CHARTEDITOR cep);

extern void
gr_chartedit_gallery_scat_process(
    P_GR_CHARTEDITOR cep);

#endif

/*
end of internal exports from gr_blgal.c
*/

/*
internal exports from gr_piesg.c
*/

/*
internally exported functions from gr_piesg.c
*/

extern S32
gr_pie_addin(
    P_GR_CHART cp);

/*
end of internal exports from gr_piesg.c
*/

/*
internal exports from gr_scatc.c
*/

/*
internally exported functions from gr_scatc.c
*/

extern S32
gr_barlinescatch_best_fit_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    GR_CHARTTYPE charttype);

extern F64
gr_barlinescatch_linest_getproc(
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row);

extern F64
gr_barlinescatch_linest_getproc_cumulative(
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row);

extern S32
gr_barlinescatch_linest_putproc(
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row,
    _InRef_     PC_F64 value);

extern void
gr_barlinescatch_get_datasources(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    /*out*/ P_GR_DATASOURCE_FOURSOME dsh);

extern S32
gr_scatterchart_addin(
    P_GR_CHART cp);

/*
end of internal exports from gr_scatc.c
*/

/*
internal exports from gr_rdiag.c -- RISC OS Draw file creation
*/

/*
internally exported structure from gr_rdiag.c
*/

typedef struct _gr_riscdiag_process_t
{
    UBF recurse:1;    /* recurse into group objects */
    UBF recompute:1;  /* ensure object bboxes recomputed if possible (some object sizes defined by their bbox) */

    UBF reserved:32-1-1;
}
GR_RISCDIAG_PROCESS_T;

typedef struct _GR_RISCDIAG_RISCOS_FONTLIST_ENTRY
{
    L1_U8Z szHostFontName[64];
}
GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, * P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY; typedef const GR_RISCDIAG_RISCOS_FONTLIST_ENTRY * PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY;

/*
rendering information
*/

typedef struct _gr_riscdiag_renderinfo
{
    GR_POINT       origin;   /* database origin wants to be moved to here in output space */
    GR_SCALE_PAIR  scale16;
    S32            action;
    GR_BOX         cliprect; /* output space clipping rectangle */
}
gr_riscdiag_renderinfo;

typedef const gr_riscdiag_renderinfo * gr_riscdiag_renderinfocp;

/*
RISC OS Draw object 'names'
*/

#define GR_RISCDIAG_OBJECT_NONE  ((DRAW_DIAG_OFFSET) 0U)
#define GR_RISCDIAG_OBJECT_FIRST ((DRAW_DIAG_OFFSET) 1U)
#define GR_RISCDIAG_OBJECT_LAST  ((DRAW_DIAG_OFFSET) 0xFFFFFFFFU)

/*
internally exported functions from gr_rdiag.c
*/

/*
RISC OS Draw styles
*/

extern GR_COLOUR
gr_colour_from_riscDraw(
    DRAW_COLOUR riscDraw);

extern DRAW_COLOUR
gr_colour_to_riscDraw(
    GR_COLOUR colour);

/*
RISC OS Draw allocation
*/

_Check_return_
_Ret_writes_bytes_maybenull_(size)
extern P_BYTE
_gr_riscdiag_ensure(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _In_        U32 size,
    _OutRef_    P_STATUS p_status);

#define gr_riscdiag_ensure(__base_type, p_gr_riscdiag, size, p_status) ( \
    (__base_type *) _gr_riscdiag_ensure(p_gr_riscdiag, size, p_status) )

/*
RISC OS Draw diagram
*/

extern void
gr_riscdiag_diagram_delete(
    P_GR_RISCDIAG p_gr_riscdiag);

extern U32
gr_riscdiag_diagram_end(
    P_GR_RISCDIAG p_gr_riscdiag);

extern void
gr_riscdiag_diagram_init(
    _OutRef_    P_DRAW_FILE_HEADER pDrawFileHdr,
    _In_z_      PC_L1STR szCreatorName);

_Check_return_
extern STATUS
gr_riscdiag_diagram_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PC_L1STR szCreatorName,
    _InVal_     ARRAY_HANDLE array_handleR);

extern void
gr_riscdiag_diagram_reset_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    GR_RISCDIAG_PROCESS_T process);

extern S32
gr_riscdiag_diagram_save(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_U8 filename);

extern S32
gr_riscdiag_diagram_save_into(
    P_GR_RISCDIAG p_gr_riscdiag,
    FILE_HANDLE f);

extern void
gr_riscdiag_diagram_setup_from_draw_diag(
    _OutRef_    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG p_draw_diag); /* flex data stolen */

/*
RISC OS Draw objects
*/

typedef union _draw_objectp
{
    P_BYTE p_byte;

    DRAW_OBJECT_HEADER * hdr;

    DRAW_OBJECT_PATH * path;
}
P_DRAW_OBJECT, * P_P_DRAW_OBJECT;

extern U32
gr_riscdiag_object_base_size(
    U32 type);

extern U32
gr_riscdiag_object_end(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_GR_DIAG_OFFSET pObjectStart);

extern S32
gr_riscdiag_object_first(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InoutRef_  P_DRAW_DIAG_OFFSET pEndObject,
    /*out*/ P_P_DRAW_OBJECT ppObject,
    S32 recurse);

extern P_BYTE
gr_riscdiag_object_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
    U32 type,
    U32 extraBytes,
    _OutRef_    P_STATUS p_status);

extern S32
gr_riscdiag_object_next(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InRef_     PC_GR_DIAG_OFFSET pEndObject,
    /*inout*/ P_P_DRAW_OBJECT ppObject,
    S32 recurse);

extern void
gr_riscdiag_object_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart);

extern void
gr_riscdiag_object_reset_bbox_between(
    P_GR_RISCDIAG p_gr_riscdiag,
    /*out*/ P_DRAW_BOX pBox,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in,
    GR_RISCDIAG_PROCESS_T process);

/*
RISC OS Draw group objects
*/

extern U32
gr_riscdiag_group_end(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_GR_DIAG_OFFSET pGroupStart);

_Check_return_
extern STATUS
gr_riscdiag_group_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pGroupStart,
    _In_opt_z_  PC_L1STR pGroupName);

/*
RISC OS Draw font table object
*/

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_        DRAW_DIAG_OFFSET fontListR,
    PC_U8 szFontName);

extern PC_L1STR
gr_riscdiag_fontlist_name(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_        DRAW_DIAG_OFFSET fontListR,
    DRAW_FONT_REF16 fontRef);

_Check_return_
extern STATUS
gr_riscdiag_fontlistR_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_fontlist_scan(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in);

/*
internally exported variables from gr_rdiag.c
*/

/*
transforms
*/

extern const GR_XFORMMATRIX
gr_riscdiag_pixit_from_riscDraw_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscos_from_riscDraw_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_pixit_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_mp_xformer;

/*
internally exported functions as macros from gr_rdiag.c
*/

#define gr_riscdiag_getoffptr(__base_type, p_gr_riscdiag, offset) ( \
    PtrAddBytes(__base_type *, (p_gr_riscdiag)->draw_diag.data, offset) )

#define gr_riscdiag_query_offset(p_gr_riscdiag) ( \
    (p_gr_riscdiag)->draw_diag.length )

/*
end of internal exports from gr_rdiag.c
*/

/*
internal exports from gr_rdia2.c
*/

/*
internally exported functions from gr_rdia2.c
*/

/*
RISC OS Draw path objects
*/

_Check_return_
_Ret_writes_bytes_maybenull_(extraBytes)
extern P_ANY /* -> path guts */
gr_riscdiag_path_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pPathStart,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLE fillstyle,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status);

extern P_ANY
gr_riscdiag_path_query_guts(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart);

extern void
gr_riscdiag_path_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart);

/*
RISC OS Draw sprite objects
*/

extern void
gr_riscdiag_sprite_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET spriteStart);

/*
RISC OS Draw string objects
*/

_Check_return_
extern STATUS
gr_riscdiag_string_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pTextStart,
    PC_DRAW_POINT point,
    PC_U8 szText,
    PC_U8 szFontName,
    GR_COORD fsizex,
    GR_COORD fsizey,
    GR_COLOUR fg,
    GR_COLOUR bg);

extern void
gr_riscdiag_string_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET textStart);

/*
RISC OS Draw pseudo objects
*/

_Check_return_
extern STATUS
gr_riscdiag_parallelogram_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pRectStart,
    PC_DRAW_POINT pOrigin,
    PC_DRAW_POINT pSize1,
    PC_DRAW_POINT pSize2,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_riscdiag_piesector_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pPieStart,
    PC_DRAW_POINT pOrigin,
    DRAW_COORD radius,
    PC_F64 alpha,
    PC_F64 beta,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_riscdiag_rectangle_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pRectStart,
    PC_DRAW_BOX pBox,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_riscdiag_scaled_diagram_add(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pPictStart,
    PC_DRAW_BOX pBox,
    P_DRAW_DIAG diag,
    PC_GR_FILLSTYLE fillstyle);

extern void
gr_riscdiag_shift_diagram(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_DRAW_POINT pShiftBy);

_Check_return_
extern STATUS
gr_riscdiag_trapezoid_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pRectStart,
    PC_DRAW_POINT pOrigin,
    PC_DRAW_POINT pSize1,
    PC_DRAW_POINT pSize2,
    PC_DRAW_POINT pSize3,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

/*
RISC OS Draw tagged objects
*/

extern void
gr_riscdiag_diagram_tagged_object_strip(
    P_GR_RISCDIAG p_gr_riscdiag,
    gr_riscdiag_tagstrip_proc proc,
    P_ANY handle);

extern S32
gr_riscdiag_wackytag_save_start(
    FILE_HANDLE f,
    /*out*/ filepos_t * pos);

extern S32
gr_riscdiag_wackytag_save_end(
    FILE_HANDLE f,
    const filepos_t * pos);
/*
RISC OS Draw JPEG objects
*/

extern void
gr_riscdiag_jpeg_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET jpegStart);

/*
misc
*/

extern HOST_FONT
gr_riscdiag_font_from_textstyle(
    PC_GR_TEXTSTYLE style);

extern void
gr_riscdiag_font_dispose(
    /*inout*/ HOST_FONT * fp);

/*
end of internal exports from gr_rdia2.c
*/

/*
DO NOT CHANGE THESE ENUMS UNLESS FULL RECOMPILATION DONE

KEEP CONSISTENT WITH gr_style_common_blks[] in GR_SCALE.C
*/

typedef enum
{
    GR_LIST_CHART_TEXT = 0,
    GR_LIST_CHART_TEXT_TEXTSTYLE,

    GR_LIST_PDROP_FILLSTYLE,
    GR_LIST_PDROP_LINESTYLE,

    /* SKS after 4.12 25mar92 - these were horribly wrong */
    GR_LIST_POINT_FILLSTYLE,
    GR_LIST_POINT_LINESTYLE,
    GR_LIST_POINT_TEXTSTYLE,

    GR_LIST_POINT_BARCHSTYLE,
    GR_LIST_POINT_BARLINECHSTYLE,
    GR_LIST_POINT_LINECHSTYLE,
    GR_LIST_POINT_PIECHDISPLSTYLE,
    GR_LIST_POINT_PIECHLABELSTYLE,
    GR_LIST_POINT_SCATCHSTYLE,

    GR_LIST_N_IDS
}
GR_LIST_ID;

/*
internal exports from gr_scale.c
*/

/*
internally exported functions from gr_scale.c
*/

extern GR_COORD
(gr_pixit_from_riscos) (
    GR_OSUNIT a);

extern GR_OSUNIT
gr_os_from_pixit_floor(
    GR_COORD a);

extern GR_OSUNIT
gr_round_os_to_ceil(
    GR_OSUNIT a,
    S32 b);

extern GR_OSUNIT
gr_round_os_to_floor(
    GR_OSUNIT a,
    S32 b);

extern GR_COORD
gr_round_pixit_to_ceil(
    GR_COORD a,
    S32 b);

extern GR_COORD
gr_round_pixit_to_floor(
    GR_COORD a,
    S32 b);

extern F64
gr_lin_major(
    F64 span);

extern S32
gr_chart_list_delete(
    P_GR_CHART cp,
    GR_LIST_ID list_id);

extern S32
gr_chart_list_duplic(
    P_GR_CHART cp,
    GR_LIST_ID list_id);

#if 0
extern P_ANY
gr_chart_list_first(
    P_GR_CHART cp,
    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

extern P_ANY
gr_chart_list_next(
    P_GR_CHART cp,
    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);
#endif

extern void
gr_chart_objid_from_axes(
    P_GR_CHART cp,
    GR_AXES_NO axes,
    GR_AXIS_NO axis,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_axis_grid(
    _InoutRef_  P_GR_CHART_OBJID id,
    BOOL major);

extern void
gr_chart_objid_from_axis_tick(
    _InoutRef_  P_GR_CHART_OBJID id,
    BOOL major);

extern void
gr_chart_objid_from_seridx(
    P_GR_CHART cp,
    GR_SERIES_NO seridx,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_series(
    GR_SERIES_NO series,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_text(
    LIST_ITEMNO key,
    _OutRef_    P_GR_CHART_OBJID id);

extern S32
gr_chart_objid_fillstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLE style);

extern S32
gr_chart_objid_fillstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_FILLSTYLE style);

extern S32
gr_chart_objid_linestyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_LINESTYLE style);

extern S32
gr_chart_objid_linestyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_LINESTYLE style);

extern S32
gr_chart_objid_textstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_TEXTSTYLE style);

extern S32
gr_chart_objid_textstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_TEXTSTYLE style);

extern S32
gr_chart_objid_barchstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_BARCHSTYLE style);

extern S32
gr_chart_objid_barchstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_BARCHSTYLE style);

extern S32
gr_chart_objid_barlinechstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_BARLINECHSTYLE style);

extern S32
gr_chart_objid_barlinechstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_BARLINECHSTYLE style);

extern S32
gr_chart_objid_linechstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_LINECHSTYLE style);

extern S32
gr_chart_objid_linechstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_LINECHSTYLE style);

extern S32
gr_chart_objid_piechdisplstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

extern S32
gr_chart_objid_piechdisplstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_PIECHDISPLSTYLE style);

extern S32
gr_chart_objid_piechlabelstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

extern S32
gr_chart_objid_piechlabelstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_PIECHLABELSTYLE style);

extern S32
gr_chart_objid_scatchstyle_query(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    _OutRef_    P_GR_SCATCHSTYLE style);

extern S32
gr_chart_objid_scatchstyle_set(
    P_GR_CHART cp,
    PC_GR_CHART_OBJID id,
    PC_GR_SCATCHSTYLE style);

extern void
gr_fillstyle_pict_change(
    /*inout*/ P_GR_FILLSTYLE style,
    const GR_FILL_PATTERN_HANDLE * newpict);

extern void
gr_fillstyle_pict_delete(
    /*inout*/ P_GR_FILLSTYLE style);

extern void
gr_fillstyle_ref_add(
    PC_GR_FILLSTYLE style);

extern void
gr_fillstyle_ref_lose(
    PC_GR_FILLSTYLE style);

extern S32
gr_pdrop_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style);

extern S32
gr_pdrop_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style);

extern S32
gr_pdrop_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style);

extern S32
gr_pdrop_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style);

#define gr_point_external_from_key(key)        ((GR_POINT_NO) (key)        + 1)

#define gr_point_key_from_external(extPointID) ((LIST_ITEMNO) (extPointID) - 1)

extern S32
gr_point_list_delete(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id);

extern S32
gr_point_list_duplic(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id);

extern S32
gr_point_list_fillstyle_enum_for_save(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_LIST_ID list_id);

extern void
gr_pdrop_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    S32 add);

extern void
gr_point_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    S32 add);

extern P_ANY
gr_point_list_first(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    _OutRef_    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

extern P_ANY
gr_point_list_next(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx,
    _InoutRef_  P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

extern S32
gr_point_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style);

extern S32
gr_point_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style);

extern S32
gr_point_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style);

extern S32
gr_point_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style);

extern S32
gr_point_textstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_TEXTSTYLE style);

extern S32
gr_point_textstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_TEXTSTYLE style);

extern S32
gr_point_barchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARCHSTYLE style);

extern S32
gr_point_barchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_BARCHSTYLE style);

extern S32
gr_point_barlinechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARLINECHSTYLE style);

extern S32
gr_point_barlinechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_BARLINECHSTYLE style);

extern S32
gr_point_linechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINECHSTYLE style);

extern S32
gr_point_linechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_LINECHSTYLE style);

extern S32
gr_point_piechdisplstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

extern S32
gr_point_piechdisplstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_PIECHDISPLSTYLE style);

extern S32
gr_point_piechlabelstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

extern S32
gr_point_piechlabelstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_PIECHLABELSTYLE style);

extern S32
gr_point_scatchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    _OutRef_    P_GR_SCATCHSTYLE style);

extern S32
gr_point_scatchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IX seridx,
    GR_POINT_NO point,
    PC_GR_SCATCHSTYLE style);

extern GR_SERIES_NO
gr_series_external_from_ix(
    PC_GR_CHART cp,
    GR_SERIES_IX seridx);

extern GR_SERIES_IX
gr_series_ix_from_external(
    PC_GR_CHART cp,
    GR_SERIES_NO eseries);

extern P_GR_SERIES
gr_seriesp_from_external(
    PC_GR_CHART cp,
    GR_SERIES_NO eseries);

/*
end of internal exports from gr_scale.c
*/

/*
number to string conversion
*/

extern S32
gr_font_stringwidth(
    HOST_FONT f,
    PC_U8 str);

extern os_error *
gr_font_truncate(
    HOST_FONT f,
    char * str /*poked*/,
    /*inout*/ int * width_mpp);

extern void
gr_numtostr(
    _Out_writes_z_(elemof_buffer) P_U8Z array,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_F64 pValue,
    S32 eformat,
    S32 decimals,
    char decimal_point_ch,  /* NULLCH -> default (.)    */
    char thousands_sep_ch); /* NULLCH -> default (none) */

/*
template names & important icons therein (keep consistent with &.CModules.*.tem.gr_chart)
*/

#define GR_CHARTEDIT_TEM "gr_editc"

#define GR_CHARTEDIT_TEM_ICON_BASE      0 /* used as baseline for sizing */
#define GR_CHARTEDIT_TEM_ICON_SELECTION 1 /* where selection id is displayed */
#define GR_CHARTEDIT_TEM_ICON_ZOOM      2 /* where zoom factor is displayed */

/*
options window
*/

#define GR_CHARTEDIT_TEM_OPTIONS "gr_ec_opt"

#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH          ((wimp_i)  8)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT         ((wimp_i) 13)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP    ((wimp_i) 18)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_TOP     ((wimp_i) 20)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT   ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_LEFT    ((wimp_i) 27)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT  ((wimp_i) 32)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_RIGHT   ((wimp_i) 34)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM ((wimp_i) 39)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_BOTTOM  ((wimp_i) 41)

/*
fill style
*/

#define GR_CHARTEDIT_TEM_FILLSTYLE "gr_ec_fstyl"

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_PICT ((wimp_i) 9)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_INC  ((wimp_i) 2)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_DEC  ((wimp_i) 3)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME ((wimp_i) 4)

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_SOLID     ((wimp_i) 5)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN   ((wimp_i) 6)

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_ISOTROPIC ((wimp_i) 7)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_RECOLOUR  ((wimp_i) 8)

/*
line style
*/

#define GR_CHARTEDIT_TEM_LINESTYLE "gr_ec_lstyl"

/* made harder by having display icons overlapped by radio buttons */

#define GR_CHARTEDIT_TEM_LINESTYLE_ICON_PAT_FIRST ((wimp_i) 10)
#define GR_CHARTEDIT_TEM_LINESTYLE_ICON_PAT_LAST  ((wimp_i) 14)

#define GR_CHARTEDIT_TEM_LINESTYLE_ICON_WID_FIRST ((wimp_i) 30)
#define GR_CHARTEDIT_TEM_LINESTYLE_ICON_WID_LAST  ((wimp_i) 36)

#define GR_CHARTEDIT_TEM_LINESTYLE_ICON_WID_USER  ((wimp_i) 37)

/*
category axis dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS "gr_ec_cax"

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BOTTOM            ((wimp_i)  7)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ZERO              ((wimp_i)  8)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_TOP               ((wimp_i)  9)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_FRONT          ((wimp_i) 10)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_AUTO           ((wimp_i) 11)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_3D_REAR           ((wimp_i) 12)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_AUTO             ((wimp_i) 13)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_MANUAL           ((wimp_i) 14)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL              ((wimp_i) 18)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID             ((wimp_i) 19)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE        ((wimp_i) 21)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_FULL        ((wimp_i) 22)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_TOP    ((wimp_i) 23)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_BOTTOM ((wimp_i) 24)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO             ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL           ((wimp_i) 26)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL              ((wimp_i) 30)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID             ((wimp_i) 31)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE        ((wimp_i) 33)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_FULL        ((wimp_i) 34)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_TOP    ((wimp_i) 35)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_BOTTOM ((wimp_i) 36)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_CANCEL                 ((wimp_i) 52)

/*
selection => axis => dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_AXIS "gr_ec_vax"

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LEFT              ((wimp_i)  7)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ZERO              ((wimp_i)  8)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_RIGHT             ((wimp_i)  9)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_FRONT          ((wimp_i) 10)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_AUTO           ((wimp_i) 11)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_3D_REAR           ((wimp_i) 12)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO             ((wimp_i) 13)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL           ((wimp_i) 14)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL              ((wimp_i) 18)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID             ((wimp_i) 19)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE        ((wimp_i) 21)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_FULL        ((wimp_i) 22)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_RIGHT  ((wimp_i) 23)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_LEFT   ((wimp_i) 24)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO             ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL           ((wimp_i) 26)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL              ((wimp_i) 30)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID             ((wimp_i) 31)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE        ((wimp_i) 33)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_FULL        ((wimp_i) 34)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_RIGHT  ((wimp_i) 35)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_LEFT   ((wimp_i) 36)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_AUTO             ((wimp_i) 41)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MANUAL           ((wimp_i) 42)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX              ((wimp_i) 46)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN              ((wimp_i) 50)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO             ((wimp_i) 51)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE        ((wimp_i) 52)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL        ((wimp_i) 53)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE      ((wimp_i) 54)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY      ((wimp_i) 55)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT        ((wimp_i) 56)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS       ((wimp_i) 57)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED         ((wimp_i) 58)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_CANCEL                 ((wimp_i) 59)

/*
selection => series => dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_SERIES "gr_ec_ser"

#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CUMULATIVE    ((wimp_i) 1)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_POINT_VARY    ((wimp_i) 2)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_BEST_FIT      ((wimp_i) 3)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_FILL_AXIS     ((wimp_i) 4)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_REMOVE_SERIES ((wimp_i) 5)

/*
text editor
*/

#define GR_CHART_TEM_TEXT_EDITOR "gr_editt"

/*
pie gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_PIE "gr_ec_pie"

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_FIRST  ((wimp_i) 1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_LAST   ((wimp_i) 8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL   ((wimp_i) 14)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_ANTICLOCKWISE ((wimp_i) 16)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_000   ((wimp_i) 18)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_045   ((wimp_i) 19)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_090   ((wimp_i) 20)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_135   ((wimp_i) 21)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_180   ((wimp_i) 22)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_225   ((wimp_i) 23)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_270   ((wimp_i) 24)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_315   ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL   ((wimp_i) 29)

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_CANCEL        ((wimp_i) 31)

/*
bar gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_BAR "gr_ec_bar"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST          ((wimp_i) 1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_LAST           ((wimp_i) 8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BAR_WIDTH      ((wimp_i) 13)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_2D_OVERLAP     ((wimp_i) 17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_DEPTH       ((wimp_i) 21)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_ON          ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_ROLL        ((wimp_i) 28)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_PITCH       ((wimp_i) 32)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_REMOVE_OVERLAY ((wimp_i) 34)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CUMULATIVE     ((wimp_i) 35)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_POINT_VARY     ((wimp_i) 36)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BEST_FIT       ((wimp_i) 37)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACK_PICT     ((wimp_i) 38)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_PICTURES       ((wimp_i) 39)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACKED        ((wimp_i) 40)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CANCEL         ((wimp_i) 41)

/*
line gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_LINE "gr_ec_line"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FIRST          ((wimp_i) 1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_LAST           ((wimp_i) 8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_WIDTH    ((wimp_i) 13)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_2D_SHIFT       ((wimp_i) 17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_DEPTH       ((wimp_i) 21)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_ON          ((wimp_i) 25)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_ROLL        ((wimp_i) 28)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_PITCH       ((wimp_i) 32)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_REMOVE_OVERLAY ((wimp_i) 34)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CUMULATIVE     ((wimp_i) 35)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_VARY     ((wimp_i) 36)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_BEST_FIT       ((wimp_i) 37)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FILL_AXIS      ((wimp_i) 38)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_PICTURES       ((wimp_i) 39)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_STACKED        ((wimp_i) 40)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CANCEL         ((wimp_i) 41)

/*
scatter gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_SCAT "gr_ec_scat"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_FIRST ((wimp_i) 1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_LAST  ((wimp_i) 9) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_WIDTH    ((wimp_i) 14)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_REMOVE_OVERLAY ((wimp_i) 16)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_VARY     ((wimp_i) 17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_BEST_FIT       ((wimp_i) 18)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_JOIN_LINES     ((wimp_i) 19)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_PICTURES       ((wimp_i) 20)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CATEG_X_VAL    ((wimp_i) 21)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CANCEL         ((wimp_i) 22)

/*
some useful macros
*/

#ifndef mo_no_selection
#define mo_no_selection 0
#endif

#ifndef round_up
/*
round up v if needed to the next r (r must be a power of 2)
*/

#define round_up(v, r) \
    (((v) & ((r) - 1)) \
            ? (((v) + ((r) - 1)) & ~((r) - 1)) \
            : (v))
#endif

/*
undebuggable bits
*/

extern unsigned int
gr_nodbg_bring_me_the_head_of_yuri_gagarin(
    P_GR_CHARTEDITOR cp,
    P_GR_CHART_OBJID id,
    P_GR_DIAG_OFFSET hitObject,
    GR_COORD x,
    GR_COORD y,
    BOOL adjustclicked);

extern S32
gr_nodbg_chart_save(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 save_filename /*const*/);

/* end of gr_chari.h */
