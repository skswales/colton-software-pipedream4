/* gr_chari.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Internal header file for the gr_chart module */

/* SKS May 1991 */

/*
required includes
*/

#ifndef          __bezier_h
#include "cmodules/bezier.h"
#endif

#ifndef          __collect_h
#include "cmodules/collect.h"
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

#ifndef          __mathxtr2_h
#include "cmodules/mathxtr2.h"
#endif

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

#ifndef __stringlk_h
#include "stringlk.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#if RISCOS

typedef struct GR_RISCDIAG_TAGSTRIP_INFO * P_GR_RISCDIAG_TAGSTRIP_INFO;

typedef S32 (* gr_riscdiag_tagstrip_proc) (
    P_ANY handle,
    P_GR_RISCDIAG_TAGSTRIP_INFO p_info);

#define gr_riscdiag_tagstrip_proto(_e_s, _p_proc_gr_riscdiag_tagstrip) \
_e_s S32 _p_proc_gr_riscdiag_tagstrip( \
    P_ANY handle, \
    P_GR_RISCDIAG_TAGSTRIP_INFO p_info)

#endif


/*
chart object 'names'
*/

typedef enum GR_CHART_OBJID_NAME
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

typedef struct GR_CHART_OBJID
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

#define GR_CHART_OBJID_INIT(name, has_no, has_subno, no, subno) \
{ \
    (name), 0, (has_no), (has_subno), (no), (subno) \
}

#define GR_CHART_OBJID_INIT_NAME(name) \
    GR_CHART_OBJID_INIT(name, 0, 0, 0, 0)

#define GR_CHART_OBJID_INIT_NAME_NO(name, no) \
    GR_CHART_OBJID_INIT(name, 1, 0, no, 0)

#define GR_CHART_OBJID_INIT_NAME_NO_SUBNO(name, no, subno) \
    GR_CHART_OBJID_INIT(name, 1, 1, no, subno)

#define gr_chart_objid_clear(p_id) \
    * (U32 *) (p_id) = 0

#define BUF_MAX_GR_CHART_OBJID_REPR 32

#define GR_CHART_OBJID_SUBNO_MAX U16_MAX

/*
internal exports from gr_diag.c -- diagram building
*/

/*
internally exported structure from gr_diag.c
*/

typedef struct GR_DIAG_PROCESS_T
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

typedef struct GR_DIAG_DIAGHEADER
{
    GR_BOX bbox;

    TCHARZ szCreatorName[12 + 4]; /* must fit PDreamCharts and CH_NULL */

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
#define GR_DIAG_OBJTYPE_QUADRILATERAL 8U

typedef U32 GR_DIAG_OBJTYPE;

#define GR_DIAG_OBJHDR_DEF    \
    GR_DIAG_OBJTYPE    tag;   \
    U32                n_bytes; \
    GR_BOX             bbox;  \
    GR_DIAG_OBJID_T    objid; \
    GR_DIAG_OFFSET     sys_off /* offset in system-dependent representation of corresponding object */

typedef struct GR_DIAG_OBJHDR
{
    GR_DIAG_OBJHDR_DEF;
}
GR_DIAG_OBJHDR;

/*
groups are simply encapulators
*/

typedef struct GR_DIAG_OBJGROUP
{
    GR_DIAG_OBJHDR_DEF;
  /*SB_U8Z name[12];*/
}
GR_DIAG_OBJGROUP;

/*
objects with position
*/

#define GR_DIAG_POSOBJHDR_DEF \
    GR_DIAG_OBJHDR_DEF; \
    GR_POINT pos

typedef struct GR_DIAG_POSOBJHDR
{
    GR_DIAG_POSOBJHDR_DEF;
}
GR_DIAG_POSOBJHDR;

typedef struct GR_DIAG_OBJLINE
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          offset;
    GR_LINESTYLE      linestyle;
}
GR_DIAG_OBJLINE;

typedef struct GR_DIAG_OBJRECTANGLE
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_SIZE           size;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJRECTANGLE;

typedef struct GR_DIAG_OBJPIESECTOR
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_COORD          radius;
    F64               alpha, beta;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;

    GR_POINT          p0, p1;
}
GR_DIAG_OBJPIESECTOR;

typedef struct GR_DIAG_OBJPICTURE
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_SIZE           size;
    IMAGE_CACHE_HANDLE picture;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJPICTURE;

typedef struct GR_DIAG_OBJQUADRILATERAL
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_POINT          offset1;
    GR_POINT          offset2;
    GR_POINT          offset3;
    GR_LINESTYLE      linestyle;
    GR_FILLSTYLE      fillstyle;
}
GR_DIAG_OBJQUADRILATERAL;

typedef struct GR_DIAG_OBJTEXT
{
    GR_DIAG_POSOBJHDR_DEF;

    GR_SIZE           size;
    GR_TEXTSTYLE      textstyle;

    /* data stored in here */
}
GR_DIAG_OBJTEXT;

typedef union P_GR_DIAG_OBJECT
{
    P_BYTE                     p_byte;

    GR_DIAG_OBJHDR *           hdr;

    GR_DIAG_OBJGROUP *         group;
    GR_DIAG_OBJLINE *          line;
    GR_DIAG_OBJRECTANGLE *     rect;
    GR_DIAG_OBJPIESECTOR *     pie;
    GR_DIAG_OBJPICTURE *       pict;
    GR_DIAG_OBJQUADRILATERAL * quad;
    GR_DIAG_OBJTEXT *          text;
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
    _InoutRef_  P_GR_DIAG p_gr_diag,
    GR_DIAG_PROCESS_T process);

extern GR_DIAG_OFFSET
gr_diag_diagram_search(
    P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OBJID_T objid);

extern U32
gr_diag_group_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    PC_GR_DIAG_OFFSET pGroupStart);

_Check_return_
extern STATUS
gr_diag_group_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    _InVal_     GR_DIAG_OBJID_T objid);

_Check_return_
extern STATUS
gr_diag_line_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle);

extern U32
gr_diag_object_base_size(
    GR_DIAG_OBJTYPE objectType);

extern S32
gr_diag_object_correlate_between(
    P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size,
    /*out*/ P_GR_DIAG_OFFSET pHitObject /*[]*/,
    S32 recursionLimit,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in);

extern U32
gr_diag_object_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
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
    _InVal_     GR_DIAG_OBJID_T objid,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObjHdr,
    _InVal_     GR_DIAG_OBJTYPE objectType,
    _InVal_     U32 extraBytes);

extern BOOL
gr_diag_object_next(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObjHdr);

extern void
gr_diag_object_reset_bbox_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_    P_GR_BOX pBox,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process);

extern GR_DIAG_OFFSET
gr_diag_object_search_between(
    P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_diag_parallelogram_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_piesector_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InVal_     GR_COORD radius,
    _InRef_     PC_F64 alpha,
    _InRef_     PC_F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_quadrilateral_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_POINT pOffset3,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_rectangle_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_scaled_picture_add(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    IMAGE_CACHE_HANDLE picture,
    _InRef_     PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_diag_text_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR szText,
    _InRef_     PC_GR_TEXTSTYLE textstyle);

/*
internally exported functions as macros from gr_diag.c
*/

#define gr_diag_getoffptr(__base_type, p_gr_diag, offset) ( (__base_type *) \
    array_ptr(&(p_gr_diag)->handle, BYTE, (offset)) )

#define gr_diag_query_offset(p_gr_diag) \
    array_elements32(&(p_gr_diag)->handle)

/*
end of internal exports from gr_diag.c
*/

/*
*/

#if RISCOS
#define SYSCHARWIDTH_OS     16
#define SYSCHARHEIGHT_OS    32
#define SYSCHARWIDTH_PIXIT  gr_pixit_from_riscos(SYSCHARWIDTH_OS)
#define SYSCHARHEIGHT_PIXIT gr_pixit_from_riscos(SYSCHARHEIGHT_OS)
#endif

typedef /*unsigned*/ int GR_DATASOURCE_NO;

typedef struct GR_MINMAX_NUMBER
{
    GR_CHART_NUMBER min, max;
}
GR_MINMAX_NUMBER;

/*
an index into the series table of a chart
*/
typedef /*unsigned*/ int GR_SERIES_IDX;
/*
an array of data sources is held on a per chart basis
*/

#define GR_DATASOURCE_HANDLE GR_INT_HANDLE /* abstract, unique non-repeating number */

#define GR_DATASOURCE_HANDLE_NONE    ((GR_INT_HANDLE) 0)
#define GR_DATASOURCE_HANDLE_START   ((GR_INT_HANDLE) 1) /* used for internal label adding */
#define GR_DATASOURCE_HANDLE_TEXTS   ((GR_INT_HANDLE) 2) /* used for external texts adding */

typedef struct GR_DATASOURCE
{
    GR_DATASOURCE_HANDLE dsh;        /* unique non-repeating number */

    gr_chart_travelproc  ext_proc;   /* callback to owner */
    P_ANY                ext_handle; /* using this handle */

    GR_CHART_OBJID       id;         /* object using this data source */

    /* validity bits for cached items */
    struct GR_DATASOURCE_VALID
    {
        UBF n_items : 1;
    }
    valid;

    /* cached items */
    struct GR_DATASOURCE_CACHE
    {
        GR_CHART_ITEMNO n_items;
    }
    cache;
}
GR_DATASOURCE, * P_GR_DATASOURCE, ** P_P_GR_DATASOURCE; typedef const GR_DATASOURCE * PC_GR_DATASOURCE;

/*
a clutch of datasources for plotting porpoises
*/

typedef struct GR_DATASOURCE_FOURSOME
{
    GR_DATASOURCE_HANDLE value_x, value_y;
    GR_DATASOURCE_HANDLE error_x, error_y;
}
GR_DATASOURCE_FOURSOME, * P_GR_DATASOURCE_FOURSOME;

/*
style options for bar charts
*/

typedef struct GR_BARCHSTYLE
{
    struct GR_BARCHSTYLE_BITS
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

typedef struct GR_LINECHSTYLE
{
    struct GR_LINECHSTYLE_BITS
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

typedef struct GR_BARLINECHSTYLE
{
    struct GR_BARLINECHSTYLE_BITS
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

typedef struct GR_PIECHDISPLSTYLE
{
    struct GR_PIECHDISPLSTYLE_BITS
    {
        UBF manual                   : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

        UBF reserved                 : sizeof(U16)*8 - 1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

    F64 radial_displacement;
}
GR_PIECHDISPLSTYLE, * P_GR_PIECHDISPLSTYLE; typedef const GR_PIECHDISPLSTYLE * PC_GR_PIECHDISPLSTYLE;

typedef struct GR_PIECHLABELSTYLE
{
    struct GR_PIECHLABELSTYLE_BITS
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

typedef struct GR_SCATCHSTYLE
{
    struct GR_SCATCHSTYLE_BITS
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

typedef enum GR_CHARTTYPE
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

typedef enum GR_SERIES_TYPE
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

typedef struct GR_SERIES
{
    /* cloning of series descriptors starts here */

    GR_SERIES_TYPE sertype;         /* determine number of sources needed and how used */
    GR_CHARTTYPE   charttype;       /* base type */

    struct GR_SERIES_BITS
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

    struct GR_SERIES_STYLE
    {
        F64                pie_start_heading_degrees;

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

    struct GR_SERIES_INTERNAL_BITS
    {
        UBF point_vary_manual_pie_bodge : 1;

        UBF descriptor_ok               : 1;

        /* not exported, size irrelevant */
    }
    internal_bits;

#define GR_SERIES_MAX_DATASOURCES 4
    struct GR_SERIES_DATASOURCES
    {
        GR_DATASOURCE_NO     n;     /* how many data sources are contributing to this series */
        GR_DATASOURCE_NO     n_req; /* how many data sources are required to contribute to this series */
        GR_DATASOURCE_HANDLE dsh[GR_SERIES_MAX_DATASOURCES]; /* no more than this data sources per series! */
    }
    datasources;

    struct GR_SERIES_LBR
    {
        P_LIST_BLOCK pdrop_fill;
        P_LIST_BLOCK pdrop_line;

        P_LIST_BLOCK point_fill;
        P_LIST_BLOCK point_line;
        P_LIST_BLOCK point_text;

        P_LIST_BLOCK point_barch;
        P_LIST_BLOCK point_barlinech;
        P_LIST_BLOCK point_linech;
        P_LIST_BLOCK point_piechdispl;
        P_LIST_BLOCK point_piechlabel;
        P_LIST_BLOCK point_scatch;
    }
    lbr;

    /* validity bits for cached items */
    struct GR_SERIES_VALID
    {
        UBF n_items_total : 1;

        UBF limits  : 1;

        /* not exported, size irrelevant */
    }
    valid;

    /* cached items */
    struct GR_SERIES_CACHE
    {
        /* number of items contributing to the series */
        GR_CHART_ITEMNO  n_items_total;

        /* actual min/max limits from this series in its given
         * plotstyle e.g. taking error bars, log axes etc. into account
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

#define GR_AXIS_POSITION_LZR_ZERO   0 /* zero or closest edge of plotted area */
#define GR_AXIS_POSITION_BZT_ZERO   GR_AXIS_POSITION_LZR_ZERO
#define GR_AXIS_POSITION_LZR_LEFT   1 /* left(bottom) edge of plotted area */
#define GR_AXIS_POSITION_BZT_BOTTOM GR_AXIS_POSITION_LZR_LEFT
#define GR_AXIS_POSITION_LZR_RIGHT  2 /* right(top) edge of plotted area */
#define GR_AXIS_POSITION_BZT_TOP    GR_AXIS_POSITION_LZR_RIGHT

#define GR_AXIS_POSITION_LZR_BITS   2 /* one spare entry */

#define GR_AXIS_POSITION_ARF_AUTO   0 /* deduce position from lzr in 3-D */
#define GR_AXIS_POSITION_ARF_REAR   1
#define GR_AXIS_POSITION_ARF_FRONT  2

#define GR_AXIS_POSITION_ARF_BITS   2 /* one spare entry */

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

typedef struct GR_AXIS_TICKS
{
    struct GR_AXIS_TICKS_BITS
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

    struct GR_AXIS_TICKS_STYLE
    {
        GR_LINESTYLE grid;
        GR_LINESTYLE tick;
    }
    style;
}
GR_AXIS_TICKS, * P_GR_AXIS_TICKS; typedef const GR_AXIS_TICKS * PC_GR_AXIS_TICKS;

typedef struct GR_AXIS
{
    GR_MINMAX_NUMBER punter;  /* punter specified min/max for this axis */
    GR_MINMAX_NUMBER actual;  /* actual min/max for the assoc data */
    GR_MINMAX_NUMBER current;

    GR_CHART_NUMBER  current_span; /* current.max - current.min */
    GR_CHART_NUMBER  zero_frac;    /* how far up the axis is the zero line? */
    GR_CHART_NUMBER  current_frac; /* major span as a fraction of the current span */

    struct GR_AXIS_BITS
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

    struct GR_AXIS_STYLE
    {
        GR_LINESTYLE axis;
        GR_TEXTSTYLE label;
    }
    style;

    GR_AXIS_TICKS major, minor;
}
GR_AXIS, * P_GR_AXIS; typedef const GR_AXIS * PC_GR_AXIS;

enum GR_CHART_AXISTICKS
{
    GR_CHART_AXISTICK_MAJOR = 1,
    GR_CHART_AXISTICK_MINOR
};

/*
descriptor for an axes set and how series belonging to it are plotted
*/

typedef struct GR_AXES
{
    GR_SERIES_TYPE sertype;         /* default series type for series creation on these axes */
    GR_CHARTTYPE   charttype;       /* default chart type ... */

    struct GR_AXES_BITS
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

    struct GR_AXES_SERIES
    {
        /* SKS after PD 4.12 26mar92 - what series no this axes set should start at (-ve -> user forced, +ve auto) */
        S32 start_series;

        S32 stt_idx; /* what series_idx this axes set starts at */
        S32 end_idx;
    }
    series;

    struct GR_AXES_STYLE
    {
        GR_BARCHSTYLE      barch;
        GR_BARLINECHSTYLE  barlinech;
        GR_LINECHSTYLE     linech;
        GR_PIECHDISPLSTYLE piechdispl;
        GR_PIECHLABELSTYLE piechlabel;
        GR_SCATCHSTYLE     scatch;
    }
    style;

    struct GR_AXES_CACHE
    {
        S32 /*GR_SERIES_NO*/ n_contrib_bars;
        S32 /*GR_SERIES_NO*/ n_contrib_lines;
        S32 /*GR_SERIES_NO*/ n_series; /* for loading and shaving */
    }
    cache;

#define X_AXIS_IDX 0
#define Y_AXIS_IDX 1
#define Z_AXIS_IDX 2
    GR_AXIS axis[3]; /* x, y, z */
}
GR_AXES, * P_GR_AXES; typedef const GR_AXES * PC_GR_AXES;

typedef S32 GR_AXIS_IDX;
typedef S32 GR_AXES_IDX; typedef GR_AXES_IDX * P_GR_AXES_IDX;

enum GR_CHART_PLOTAREAS
{
    GR_CHART_PLOTAREA_REAR = 0,
    GR_CHART_PLOTAREA_WALL,
    GR_CHART_PLOTAREA_FLOOR,

    GR_CHART_N_PLOTAREAS
};

/*
a chart
*/

typedef struct GR_CHART
{
    struct GR_CHART_CORE /* the core of the chart - preserve over clone */
    {
        GR_CHART_HANDLE     ch;              /* the handle under which this structure is exported */

        GR_CHARTEDIT_HANDLE ceh;             /* the handle of the single chart editor that is allowed on this chart */

        char *              currentfilename; /* full pathname of where (if anywhere) this file is stored */
        char *              currentdrawname; /* full pathname of where (if anywhere) this file was stored as a Draw file */
        GR_DATASOURCE       category_datasource;

        P_ANY               ext_handle;      /* a handle which the creator of the chart passed us */

        struct GR_CHART_DATASOURCES
        {
            GR_DATASOURCE_NO n;        /* number in use */
            GR_DATASOURCE_NO n_alloc;  /* how many data sources handle has been allocated for */
            GR_DATASOURCE *  mh;       /* ptr to data source descriptor array */
        }
        datasources;

        struct GR_CHART_LAYOUT
        {
            GR_COORD width;
            GR_COORD height;

            struct GR_CHART_LAYOUT_MARGINS
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

        struct GR_CHART_EDITSAVE
        {
#if RISCOS
            struct GR_CHART_EDITSAVE_OPEN
            {
                BBox visible_area;
                GDI_COORD scroll_x, scroll_y;
            }
            open;
#else
            char foo;
#endif
        }
        editsave;
    }
    core;

    struct GR_CHART_CHART           /* attributes of 'chart' object */
    {
        GR_FILLSTYLE areastyle;
        GR_LINESTYLE borderstyle;
    }
    chart;

    struct GR_CHART_PLOTAREA        /* attributes of 'plotarea' object */
    {
        GR_POINT posn;              /* offset of plotted area in chart */
        GR_POINT size;              /* how big the plotted area is */

        struct GR_CHART_PLOTAREA_AREA
        {
            GR_FILLSTYLE areastyle;
            GR_LINESTYLE borderstyle;
        }
        area[GR_CHART_N_PLOTAREAS];
    }
    plotarea;

    struct GR_CHART_LEGEND          /* attributes of 'legend' object */
    {
        struct GR_CHART_LEGEND_BITS
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

    struct GR_CHART_SERIES
    {
        GR_SERIES_IDX n;
        GR_SERIES_IDX n_alloc;   /* how many series handle has been allocated for */
        GR_SERIES_IDX n_defined; /* how many are defined */
        P_GR_SERIES   mh;       /* ptr to series descriptor array */

        GR_ESERIES_NO overlay_start; /* 0 -> auto */
    }
    series;

#define GR_AXES_IDX_MAX 1
    GR_AXES_IDX       axes_idx_max; /* how many axes sets are in use (-1 as there's always at least one) */
    GR_AXES           axes[GR_AXES_IDX_MAX + 1]; /* two independent sets of axes you can plot on (these hold series descriptors) */

    struct GR_CHART_TEXT
    {
        P_LIST_BLOCK    lbr; /* list of gr_texts */

        struct GR_CHART_TEXT_STYLE
        {
            P_LIST_BLOCK    lbr; /* list of attributes for the gr_texts */

            GR_TEXTSTYLE    base; /* base text style for text objects */
        }
        style;
    }
    text;

    struct GR_CHART_BITS
    {
        UBF realloc_series : 1; /* need to reallocate ds to series */

        /* not exported, size irrelevant */
    }
    bits;

    struct GR_CHART_D3
    {
        struct GR_CHART_D3_BITS
        {
            UBF on       : 1; /* 3-D embellishment? applies to whole chart */
            UBF use      : 1; /* whether 3-D is in use, e.g. pie & scat turn off */

            UBF reserved : sizeof(U32)*8 - 1;
        }
        bits; /* U32 for sys indep expansion */

        F64 droop;            /* angle about the horizontal x-axis, bringing top into view */
        F64 turn;             /* angle about the vertical y-axis, bringing side into view */

        /* validity bits for cached items */
        struct GR_CHART_D3_VALID
        {
            UBF vector : 1;

            /* not exported, size irrelevant */
        }
        valid;

        /* cached items */
        struct GR_CHART_D3_CACHE
        {
            GR_POINT vector;            /* how much a point is displaced by per series in depth */

            GR_POINT vector_full;       /* full displacement from front to back */
        }
        cache;
    }
    d3;

    struct GR_CHART_CACHE
    {
        S32 /*GR_SERIES_NO*/ n_contrib_series; /* how many series are contributing to this chart from all axes */
        S32 /*GR_SERIES_NO*/ n_contrib_bars;
        S32 /*GR_SERIES_NO*/ n_contrib_lines;
    }
    cache;

    struct GR_CHART_BARCH
    {
        F64 slot_overlap_percentage;

        struct GR_CHART_BARCH_CACHE
        {
            GR_COORD slot_width;
            GR_COORD slot_shift;
        }
        cache;
    }
    barch;

    struct GR_CHART_BARLINECH
    {
        struct GR_CHART_BARLINECH_CACHE
        {
            GR_COORD cat_group_width;
            GR_COORD zeropoint_y;
        }
        cache;
    }
    barlinech;

    struct GR_CHART_LINECH
    {
        F64 slot_shift_percentage;

        struct GR_CHART_LINECH_CACHE
        {
            GR_COORD slot_start;
            GR_COORD slot_shift;
        }
        cache;
    }
    linech;

    GR_SERIES_IDX pie_series_idx;
    }
GR_CHART, * P_GR_CHART; typedef const GR_CHART * PC_GR_CHART;

typedef struct GR_TEXT
{
    GR_BOX box;

    struct GR_TEXT_BITS
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

#define gr_text_search_key(p_gr_chart, key) \
    collect_goto_item(GR_TEXT, &(p_gr_chart)->text.lbr, (key))

typedef union P_GR_TEXT_GUTS
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

typedef struct GR_BARLINESCATCH_SERIES_CACHE
{
    P_GR_CHART              cp;
    GR_AXES_IDX             axes_idx;
    GR_SERIES_IDX           series_idx;
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

typedef struct GR_BARLINESCATCH_LINEST_STATE
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

_Check_return_
static inline STATUS
gr_chart_group_new(
    P_GR_CHART cp,
    P_GR_DIAG_OFFSET groupStart,
    _InVal_     GR_CHART_OBJID id)
{
    status_return(gr_diag_group_new(cp->core.p_gr_diag, groupStart, id));

    return(STATUS_DONE);
}

static inline void
gr_chart_group_end(
    P_GR_CHART cp,
    PC_GR_DIAG_OFFSET pGroupStart)
{
    (void) gr_diag_group_end(cp->core.p_gr_diag, pGroupStart);
}

_Check_return_
static inline STATUS
gr_chart_line_new(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    GR_POINT pos, offset;

    pos.x = pBox->x0;
    pos.y = pBox->y0;

    offset.x = pBox->x1 - pBox->x0;
    offset.y = pBox->y1 - pBox->y0;

    status_return(gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &pos, &offset, linestyle));

    return(STATUS_DONE);
}

_Check_return_
static inline STATUS
gr_chart_rectangle_new(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    status_return(gr_diag_rectangle_new(cp->core.p_gr_diag, NULL, id, pBox, linestyle, fillstyle));

    return(STATUS_DONE);
}

_Check_return_
static inline STATUS
gr_chart_scaled_picture_add(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX box,
    PC_GR_FILLSTYLE style)
{
    status_return(gr_diag_scaled_picture_add(cp->core.p_gr_diag, NULL, id, box, style ? (IMAGE_CACHE_HANDLE) style->pattern : IMAGE_CACHE_HANDLE_NONE, style));

    return(STATUS_DONE);
}

_Check_return_
static inline STATUS
gr_chart_text_new(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR szText,
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    status_return(gr_diag_text_new(cp->core.p_gr_diag, NULL, id, pBox, szText, textstyle));

    return(STATUS_DONE);
}

/*
internally exported functions from gr_chart.c
*/

_Check_return_
extern STATUS
gr_chart_add_series(
    P_GR_CHART  cp,
    GR_AXES_IDX axes_idx,
    S32        init);

extern P_GR_CHART
gr_chart_cp_from_ch(
    GR_CHART_HANDLE ch);

/*ncr*/
extern BOOL
gr_chart_dsp_from_dsh(
    P_GR_CHART cp,
    _OutRef_    P_P_GR_DATASOURCE dspp,
    GR_DATASOURCE_HANDLE dsh);

_Check_return_
extern STATUS
gr_chart_legend_addin(
    P_GR_CHART cp);

extern void
gr_chart_object_name_from_id(
    /*out*/ P_U8 szName,
    _InVal_ U32 elemof_buffer,
    _InVal_     GR_CHART_OBJID id);

extern PC_U8Z
gr_chart_object_name_from_id_quick(
    _InVal_     GR_CHART_OBJID id);

extern void
gr_chart_objid_find_parent(
    _InoutRef_  P_GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_realloc_series(
    P_GR_CHART cp);

/*ncr*/
extern S32
gr_chart_subtract_datasource_using_dsh(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh);

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
    GR_SERIES_IDX series_idx,
    GR_DATASOURCE_NO ds);

extern void
gr_travel_series_label(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    /*out*/ P_GR_CHART_VALUE pValue);

extern GR_CHART_ITEMNO
gr_travel_series_n_items(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_DATASOURCE_NO ds);

extern GR_CHART_ITEMNO
gr_travel_series_n_items_total(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx);

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
get a series descriptor pointer from series_idx on a chart
*/

#define getserp(cp, series_idx) \
    ((cp)->series.mh + (series_idx))

/*
end of internal exports from gr_chart.c
*/

/*
internally exported functions from gr_chtIO.c
*/

_Check_return_
extern STATUS
gr_fillstyle_make_key_for_save(
    PC_GR_FILLSTYLE style);

_Check_return_
extern STATUS
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

typedef struct GR_CHARTEDITOR
{
    GR_CHARTEDIT_HANDLE      ceh;           /* the handle under which this structure is exported */

    GR_CHART_HANDLE          ch;            /* which chart is being edited */

    gr_chartedit_notify_proc notifyproc;
    P_ANY                    notifyhandle;

    GR_POINT                size;         /* oh yes, this is needed */

    struct GR_CHARTEDITOR_SELECTION
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

    struct GR_CHARTEDITOR_RISCOS
    {
        WimpWindowWithBitset * window_template; /* a template_copy_new'ed window definition (keep for indirected data) */
        HOST_WND      window_handle;       /* Window Manager handle of chart editing window */

/* margins around chart in display */
#define GR_CHARTEDIT_DISPLAY_LM_OS 8
#define GR_CHARTEDIT_DISPLAY_RM_OS 8
#define GR_CHARTEDIT_DISPLAY_BM_OS 8
#define GR_CHARTEDIT_DISPLAY_TM_OS 8

        GR_OSUNIT     heading_size;        /* total y size at the top of the window that is taken up by the icons */

        F64           scale_from_diag;     /* last scale factor used */
        GR_SCALE_PAIR scale_from_diag16;   /* and as a pair of 16.16 fixed nos. */
        GR_SCALE_PAIR scale_from_screen16; /* and the inverse. */

        GDI_COORD     init_scroll_x, init_scroll_y;

        S32           diagram_off_x, diagram_off_y; /* work area offsets of diagram origin */

        S32           processing_fontselector;
    }
    riscos;
#elif WINDOWS
    /* WINDOWS specifics */

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
    _InVal_     STATUS err);

extern void
gr_chartedit_warning(
    P_GR_CHART cp,
    _InVal_     STATUS err);

#if RISCOS

extern unsigned int
gr_chartedit_riscos_correlate(
    P_GR_CHARTEDITOR cep,
    PC_GR_POINT point,
    _OutRef_    P_GR_CHART_OBJID id,
    /*out*/ P_GR_DIAG_OFFSET hitObject /*[]*/,
    size_t hitObjectDepth,
    BOOL adjust_clicked);

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

/*ncr*/
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

_Check_return_
extern STATUS
gr_text_addin(
    P_GR_CHART cp);

extern void
gr_text_delete(
    P_GR_CHART cp,
    LIST_ITEMNO key);

extern LIST_ITEMNO
gr_text_key_for_new(
    P_GR_CHART cp);

_Check_return_
extern STATUS
gr_text_new(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    PC_U8 szText,
    PC_GR_POINT point);

extern void
gr_text_box_query(
    P_GR_CHART cp,
    LIST_ITEMNO key,
    _OutRef_    P_GR_BOX p_box);

_Check_return_
extern STATUS
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
    const BBox * const dragboxp);

extern S32
gr_chartedit_selection_fillstyle_edit(
    P_GR_CHARTEDITOR cep);

extern S32
gr_chartedit_selection_textstyle_edit(
    P_GR_CHARTEDITOR cep);

extern void
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

_Check_return_
extern STATUS
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

_Check_return_
extern GR_EAXES_NO
gr_axes_external_from_idx(
    PC_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx);

/*ncr*/
extern GR_AXIS_IDX
gr_axes_idx_from_external(
    PC_GR_CHART cp,
    GR_EAXES_NO eaxes_no,
    _OutRef_    P_GR_AXES_IDX p_axes_idx);

_Check_return_
extern GR_AXES_IDX
gr_axes_idx_from_series_idx(
    PC_GR_CHART cp,
    GR_SERIES_IDX series_idx);

_Check_return_
extern P_GR_AXES
gr_axesp_from_series_idx(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx);

_Check_return_
extern P_GR_AXIS
gr_axisp_from_external(
    P_GR_CHART cp,
    GR_EAXES_NO eaxes_no);

_Check_return_
extern STATUS
gr_axis_addin_category(
    P_GR_CHART cp,
    GR_POINT_NO total_n_points,
    _InVal_     BOOL front_phase);

_Check_return_
extern STATUS
gr_axis_addin_value_x(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase);

_Check_return_
extern STATUS
gr_axis_addin_value_y(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase);

extern void
gr_axis_form_category(
    P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points);

extern void
gr_axis_form_value(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx);

extern void
gr_chartedit_selection_axis_process(
    P_GR_CHARTEDITOR cep);

_Check_return_
extern F64
gr_lin_major(
    F64 span);

/*ncr*/
extern F64
_splitlognum(
    _InRef_     PC_F64 logval,
    _OutRef_    P_F64 exponent /*out*/);

/*
end of internal exports from P_GR_AXIS.c
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

_Check_return_
extern GR_COORD
gr_categ_pos(
    PC_GR_CHART cp,
    GR_POINT_NO point);

extern void
gr_map_point(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    S32 plotindex);

extern void
gr_map_point_front_or_back(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    _InVal_     BOOL front);

extern void
gr_map_box_front_and_back(
    _InoutRef_  P_GR_BOX xbox,
    _InRef_     PC_GR_CHART cp);

extern void
gr_point_partial_z_shift(
    /*out*/ P_GR_POINT xpoint,
    PC_GR_POINT apoint,
    P_GR_CHART cp,
    const F64 * z_frac_p);

_Check_return_
extern GR_COORD
gr_value_pos(
    PC_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx,
    const GR_CHART_NUMBER * value);

_Check_return_
extern GR_COORD
gr_value_pos_rel(
    PC_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx,
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

_Check_return_
extern STATUS
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

_Check_return_
extern STATUS
gr_barlinescatch_best_fit_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    GR_CHARTTYPE charttype);

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc, client_handle, colID, row);

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc_cumulative, client_handle, colID, row);

PROC_LINEST_DATA_PUT_PROTO(extern, gr_barlinescatch_linest_putproc, client_handle, colID, row, value);

extern void
gr_barlinescatch_get_datasources(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    /*out*/ P_GR_DATASOURCE_FOURSOME dsh);

_Check_return_
extern STATUS
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

typedef struct GR_RISCDIAG_PROCESS_T
{
    UBF recurse:1;    /* recurse into group objects */
    UBF recompute:1;  /* ensure object bboxes recomputed if possible (some object sizes defined by their bbox) */

    UBF reserved:32-1-1;
}
GR_RISCDIAG_PROCESS_T;

typedef struct GR_RISCDIAG_RISCOS_FONTLIST_ENTRY
{
    SBCHARZ szHostFontName[64];
}
GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, * P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY; typedef const GR_RISCDIAG_RISCOS_FONTLIST_ENTRY * PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY;

/*
rendering information
*/

typedef struct GR_RISCDIAG_RENDERINFO
{
    GR_POINT       origin;   /* database origin wants to be moved to here in output space */
    GR_SCALE_PAIR  scale16;
    S32            action;
    GR_BOX         cliprect; /* output space clipping rectangle */
}
GR_RISCDIAG_RENDERINFO; typedef const GR_RISCDIAG_RENDERINFO * PC_GR_RISCDIAG_RENDERINFO;

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
_Ret_writes_maybenull_(size)
extern P_BYTE
_gr_riscdiag_ensure(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _In_        U32 size,
    _OutRef_    P_STATUS p_status);

#define gr_riscdiag_ensure(__base_type, p_gr_riscdiag, size, p_status) ( \
    (__base_type *) _gr_riscdiag_ensure(p_gr_riscdiag, size, p_status) )

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_stt(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in);

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_end(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET endObject_in);

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
    _In_z_      PC_SBSTR szCreatorName);

_Check_return_
extern STATUS
gr_riscdiag_diagram_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PC_SBSTR szCreatorName,
    _InVal_     ARRAY_HANDLE array_handleR);

extern void
gr_riscdiag_diagram_reset_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    GR_RISCDIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_riscdiag_diagram_save(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PC_U8Z filename);

_Check_return_
extern STATUS
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

typedef union P_DRAW_OBJECT
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

_Check_return_
extern BOOL
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

_Check_return_
extern BOOL
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
    _In_opt_z_  PC_SBSTR pGroupName);

/*
RISC OS Draw font table object
*/

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_        DRAW_DIAG_OFFSET fontListR,
    PC_U8 szFontName);

extern PC_SBSTR
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
gr_riscdiag_riscDraw_from_millipoint_xformer;

/*
internally exported functions as macros from gr_rdiag.c
*/

#define gr_riscdiag_getoffptr(__base_type, p_gr_riscdiag, offset) ( \
    PtrAddBytes(__base_type *, (p_gr_riscdiag)->draw_diag.data, offset) )

#define gr_riscdiag_query_offset(p_gr_riscdiag) ( \
    (p_gr_riscdiag)->draw_diag.length )

#define DRAW_OBJHDR(__base_type, pObject, field) \
    PtrAddBytes(__base_type *, pObject, offsetof32(DRAW_OBJECT_HEADER, field))

#define DRAW_OBJBOX(pObject, field) \
    PtrAddBytes(P_S32, pObject, offsetof32(DRAW_OBJECT_HEADER, bbox) + offsetof32(DRAW_BOX, field))

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
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pPathStart,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLE fillstyle,
    _InRef_opt_ PC_DRAW_PATH_STYLE pathstyle,
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
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
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
gr_riscdiag_line_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle);

_Check_return_
extern STATUS
gr_riscdiag_piesector_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
    PC_DRAW_POINT pPos,
    DRAW_COORD radius,
    PC_F64 alpha,
    PC_F64 beta,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_riscdiag_quadrilateral_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset1,
    _InRef_     PC_DRAW_POINT pOffset2,
    _InRef_     PC_DRAW_POINT pOffset3,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle);

_Check_return_
extern STATUS
gr_riscdiag_rectangle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLE fillstyle);

/*
scaled diagram
*/

_Check_return_
extern STATUS
gr_riscdiag_scaled_diagram_add(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_GR_DIAG_OFFSET pPictStart,
    PC_DRAW_BOX pBox,
    P_DRAW_DIAG diag,
    PC_GR_FILLSTYLE fillstyle);

extern void
gr_riscdiag_shift_diagram(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    PC_DRAW_POINT pShiftBy);

static inline void
draw_box_from_gr_box(
    _OutRef_    P_DRAW_BOX p_draw_box,
    _InRef_     PC_GR_BOX p_gr_box)
{
    gr_box_xform((P_GR_BOX) p_draw_box, p_gr_box, &gr_riscdiag_riscDraw_from_pixit_xformer);
}

static inline void
draw_point_from_gr_point(
    _OutRef_    P_DRAW_POINT p_draw_point,
    _InRef_     PC_GR_POINT p_gr_point)
{
    gr_point_xform((P_GR_POINT) p_draw_point, p_gr_point, &gr_riscdiag_riscDraw_from_pixit_xformer);
}

/*
RISC OS Draw tagged objects
*/

extern void
gr_riscdiag_diagram_tagged_object_strip(
    P_GR_RISCDIAG p_gr_riscdiag,
    gr_riscdiag_tagstrip_proc proc,
    P_ANY handle);

_Check_return_
extern STATUS
gr_riscdiag_wackytag_save_start(
    FILE_HANDLE file_handle,
    _OutRef_    P_DRAW_DIAG_OFFSET p_offset);

_Check_return_
extern STATUS
gr_riscdiag_wackytag_save_end(
    FILE_HANDLE file_handle,
    _InRef_     PC_DRAW_DIAG_OFFSET p_offset);

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

typedef enum GR_LIST_ID
{
    GR_LIST_CHART_TEXT = 0,
    GR_LIST_CHART_TEXT_TEXTSTYLE,

    GR_LIST_PDROP_FILLSTYLE,
    GR_LIST_PDROP_LINESTYLE,

    /* SKS after PD 4.12 25mar92 - these were horribly wrong */
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
internal exports from gr_style.c
*/

/*
internally exported functions from gr_style.c
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

extern void
gr_chart_list_delete(
    P_GR_CHART cp,
    GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_chart_list_duplic(
    P_GR_CHART cp,
    GR_LIST_ID list_id);

#if 0
extern P_ANY
gr_chart_list_first(
    P_GR_CHART cp,
    _OutRef_    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

extern P_ANY
gr_chart_list_next(
    P_GR_CHART cp,
    _InoutRef_  P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);
#endif

extern void
gr_chart_objid_from_axes_idx(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx,
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
gr_chart_objid_from_series_idx(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_series_no(
    GR_ESERIES_NO series_no,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_text(
    LIST_ITEMNO key,
    _OutRef_    P_GR_CHART_OBJID id);

/*ncr*/
extern BOOL
gr_chart_objid_fillstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_fillstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_FILLSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_linestyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINESTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_linestyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_LINESTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_textstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_TEXTSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_textstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_TEXTSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_barchstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARCHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_barchstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_BARCHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_barlinechstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARLINECHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_barlinechstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_BARLINECHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_linechstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINECHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_linechstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_LINECHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_piechdisplstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_piechdisplstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_PIECHDISPLSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_piechlabelstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_piechlabelstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    PC_GR_PIECHLABELSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_scatchstyle_query(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_SCATCHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_scatchstyle_set(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
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

/*ncr*/
extern BOOL
gr_pdrop_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style);

_Check_return_
extern STATUS
gr_pdrop_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style);

/*ncr*/
extern BOOL
gr_pdrop_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style);

_Check_return_
extern STATUS
gr_pdrop_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style);

#define gr_point_external_from_key(key)        ((GR_POINT_NO) (key)        + 1)

#define gr_point_key_from_external(extPointID) ((LIST_ITEMNO) (extPointID) - 1)

extern void
gr_point_list_delete(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_point_list_duplic(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_point_list_fillstyle_enum_for_save(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_LIST_ID list_id);

extern void
gr_pdrop_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    S32 add);

extern void
gr_point_list_fillstyle_reref(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    S32 add);

extern P_ANY
gr_point_list_first(
    PC_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    _OutRef_    P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

extern P_ANY
gr_point_list_next(
    PC_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    _InoutRef_  P_LIST_ITEMNO p_key,
    GR_LIST_ID list_id);

/*ncr*/
extern BOOL
gr_point_fillstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLE style);

_Check_return_
extern STATUS
gr_point_fillstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_FILLSTYLE style);

/*ncr*/
extern BOOL
gr_point_linestyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style);

_Check_return_
extern STATUS
gr_point_linestyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_LINESTYLE style);

/*ncr*/
extern BOOL
gr_point_textstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_TEXTSTYLE style);

_Check_return_
extern STATUS
gr_point_textstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_TEXTSTYLE style);

/*ncr*/
extern BOOL
gr_point_barchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARCHSTYLE style);

_Check_return_
extern STATUS
gr_point_barchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_BARCHSTYLE style);

/*ncr*/
extern BOOL
gr_point_barlinechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_BARLINECHSTYLE style);

_Check_return_
extern STATUS
gr_point_barlinechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_BARLINECHSTYLE style);

/*ncr*/
extern BOOL
gr_point_linechstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_LINECHSTYLE style);

_Check_return_
extern STATUS
gr_point_linechstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_LINECHSTYLE style);

/*ncr*/
extern BOOL
gr_point_piechdisplstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

_Check_return_
extern STATUS
gr_point_piechdisplstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_PIECHDISPLSTYLE style);

/*ncr*/
extern BOOL
gr_point_piechlabelstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

_Check_return_
extern STATUS
gr_point_piechlabelstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_PIECHLABELSTYLE style);

/*ncr*/
extern BOOL
gr_point_scatchstyle_query(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    _OutRef_    P_GR_SCATCHSTYLE style);

_Check_return_
extern STATUS
gr_point_scatchstyle_set(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx,
    GR_POINT_NO point,
    PC_GR_SCATCHSTYLE style);

extern GR_ESERIES_NO
gr_series_external_from_idx(
    PC_GR_CHART cp,
    GR_SERIES_IDX series_idx);

extern GR_SERIES_IDX
gr_series_idx_from_external(
    PC_GR_CHART cp,
    GR_ESERIES_NO eseries_no);

extern P_GR_SERIES
gr_seriesp_from_external(
    PC_GR_CHART cp,
    GR_ESERIES_NO eseries_no);

/*
end of internal exports from gr_style.c
*/

/*
number to string conversion
*/

_Check_return_
extern S32
gr_font_stringwidth(
    HOST_FONT f,
    PC_U8 str);

extern _kernel_oserror *
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
    char decimal_point_ch,  /* CH_NULL -> default (.)    */
    char thousands_sep_ch); /* CH_NULL -> default (none) */

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

#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_WIDTH          ( 8)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_HEIGHT         (13)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_TOP    (18)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_TOP     (20)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_LEFT   (25)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_LEFT    (27)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_RIGHT  (32)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_RIGHT   (34)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_PCT_MARGIN_BOTTOM (39)
#define GR_CHARTEDIT_TEM_OPTIONS_ICON_IN_MARGIN_BOTTOM  (41)

#define GR_CHARTEDIT_TEM_OPTIONS_ICON_CANCEL            (43)

/*
fill style
*/

#define GR_CHARTEDIT_TEM_FILLSTYLE "gr_ec_fstyl"

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_PICT   (9)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_INC    (2)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_DEC    (3)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_DRAW_NAME   (4)

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_SOLID       (5)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_PATTERN     (6)

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_ISOTROPIC   (7)
#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_RECOLOUR    (8)

#define GR_CHARTEDIT_TEM_FILLSTYLE_ICON_CANCEL      (10)

/*
line pattern
*/

#define GR_CHARTEDIT_TEM_LINEPATTERN "gr_ec_lpat"

#define GR_CHARTEDIT_TEM_LINEPATTERN_ICON_CANCEL    ( 1)

/* made harder by having display icons overlapped by radio buttons */

#define GR_CHARTEDIT_TEM_LINEPATTERN_ICON_FIRST     ( 9)
#define GR_CHARTEDIT_TEM_LINEPATTERN_ICON_LAST      (13)

/*
line width
*/

#define GR_CHARTEDIT_TEM_LINEWIDTH "gr_ec_lwid"

#define GR_CHARTEDIT_TEM_LINEWIDTH_ICON_CANCEL  ( 1)

/* made harder by having display icons overlapped by radio buttons */

#define GR_CHARTEDIT_TEM_LINEWIDTH_ICON_FIRST   (18)
#define GR_CHARTEDIT_TEM_LINEWIDTH_ICON_LAST    (24)

#define GR_CHARTEDIT_TEM_LINEWIDTH_ICON_USER    (25)

/*
category axis dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS "gr_ec_cax"

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_BOTTOM        ( 7)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_ZERO          ( 8)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_BZT_TOP           ( 9)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_FRONT         (10)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_AUTO          (11)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_POSN_ARF_REAR          (12)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_AUTO             (13)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_MANUAL           (14)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_VAL              (18)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_GRID             (19)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_NONE        (21)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_FULL        (22)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_TOP    (23)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MAJOR_TICK_HALF_BOTTOM (24)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_AUTO             (25)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_MANUAL           (26)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_VAL              (30)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_GRID             (31)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_NONE        (33)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_FULL        (34)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_TOP    (35)
#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_MINOR_TICK_HALF_BOTTOM (36)

#define GR_CHARTEDIT_TEM_SELECTION_CAT_AXIS_ICON_CANCEL                 (37)

/*
selection => axis => dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_AXIS "gr_ec_vax"

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_LEFT          ( 7)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_ZERO          ( 8)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_LZR_RIGHT         ( 9)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_FRONT         (10)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_AUTO          (11)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_POSN_ARF_REAR          (12)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_AUTO             (13)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_MANUAL           (14)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_VAL              (18)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_GRID             (19)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_NONE        (21)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_FULL        (22)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_RIGHT  (23)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MAJOR_TICK_HALF_LEFT   (24)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_AUTO             (25)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_MANUAL           (26)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_VAL              (30)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_GRID             (31)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_NONE        (33)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_FULL        (34)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_RIGHT  (35)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_MINOR_TICK_HALF_LEFT   (36)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_AUTO             (41)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MANUAL           (42)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MAX              (46)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_MIN              (50)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_ZERO             (51)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_LOG_SCALE        (52)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SCALE_POW_LABEL        (53)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_CUMULATIVE      (54)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_POINT_VARY      (55)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_BEST_FIT        (56)
#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_FILL_AXIS       (57)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_SERIES_STACKED         (58)

#define GR_CHARTEDIT_TEM_SELECTION_AXIS_ICON_CANCEL                 (59)

/*
selection => series => dialog
*/

#define GR_CHARTEDIT_TEM_SELECTION_SERIES "gr_ec_ser"

#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CANCEL           (1)

#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_REMOVE_SERIES    (2)

#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_CUMULATIVE       (3)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_POINT_VARY       (4)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_BEST_FIT         (5)
#define GR_CHARTEDIT_TEM_SELECTION_SERIES_ICON_FILL_AXIS        (6)

/*
text editor
*/

#define GR_CHART_TEM_TEXT_EDITOR "gr_editt"

/*
pie gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_PIE "gr_ec_pie"

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_FIRST  (1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_LAST   (8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_VAL   (14)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_ANTICLOCKWISE (16)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_000   (18)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_045   (19)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_090   (20)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_135   (21)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_180   (22)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_225   (23)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_270   (24)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_HEADING_315   (25)
#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_EXPLODE_VAL   (29)

#define GR_CHARTEDIT_TEM_GALLERY_PIE_ICON_CANCEL        (31)

/*
bar gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_BAR "gr_ec_bar"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_FIRST          (1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_LAST           (8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BAR_WIDTH      (13)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_2D_OVERLAP     (17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_DEPTH       (21)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_ON          (25)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_TURN        (28)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_3D_DROOP       (32)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_REMOVE_OVERLAY (34)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CUMULATIVE     (35)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_POINT_VARY     (36)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_BEST_FIT       (37)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACK_PICT     (38)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_PICTURES       (39)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_STACKED        (40)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_BAR_CANCEL         (41)

/*
line gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_LINE "gr_ec_line"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FIRST          (1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_LAST           (8) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_WIDTH    (13)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_2D_SHIFT       (17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_DEPTH       (21)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_ON          (25)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_TURN        (28)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_3D_DROOP       (32)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_REMOVE_OVERLAY (34)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CUMULATIVE     (35)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_POINT_VARY     (36)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_BEST_FIT       (37)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_FILL_AXIS      (38)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_PICTURES       (39)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_STACKED        (40)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_LINE_CANCEL         (41)

/*
scatter gallery
*/

#define GR_CHARTEDIT_TEM_GALLERY_SCAT "gr_ec_scat"

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_FIRST (1) /* first picture in this gallery */
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_LAST  (9) /* last picture in this gallery */

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_WIDTH    (14)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_REMOVE_OVERLAY (16)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_POINT_VARY     (17)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_BEST_FIT       (18)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_JOIN_LINES     (19)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_PICTURES       (20)
#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CATEG_X_VAL    (21)

#define GR_CHARTEDIT_TEM_GALLERY_ICON_SCAT_CANCEL         (22)

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
    BOOL adjust_clicked);

_Check_return_
extern STATUS
gr_nodbg_chart_save(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 save_filename /*const*/);

/* end of gr_chari.h */
