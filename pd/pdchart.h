/* pdchart.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS 30-Apr-1991 */

/*
private structure defs for pdchart.c
*/

#ifndef __pdchart_h
#define __pdchart_h

#ifndef __gr_chart_h
#include "gr_chart.h"
#endif

#ifndef __handlist_h
#include "handlist.h"
#endif

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

#include "pd_x.h"
#include "riscos_x.h"

#include "ridialog.h"

/* PipeDream side of charting things */

typedef ROW rowcolt; /* big enough to hold either */

typedef enum PDCHART_RANGE_TYPE
{
    PDCHART_RANGE_NONE = 0, /* unallocated hole */

    PDCHART_RANGE_COL,      /* rows in a column */
    PDCHART_RANGE_ROW,      /* columns in a row */

    PDCHART_RANGE_TXT,      /* text objects in chart dependent on sheet data */

    PDCHART_RANGE_COL_DEAD, /* was rows in a column */
    PDCHART_RANGE_ROW_DEAD  /* was columns in a row */
}
PDCHART_RANGE_TYPE;

/*
objects on the dependency list for a chart.

there is one of these per added range and each can
be referenced by many elements of the same chart
*/

typedef struct PDCHART_DEP
{
    PDCHART_RANGE_TYPE type;           /* whether COL or ROW or TXT */

    LIST_ITEMNO        itdepkey;       /* have to be able to search back using key */

    LIST_ITEMNO        pdchartdatakey; /* have to find parent chart using key as it's listed data */

    S32                ev_int_handle;  /* so the extdependency can be deleted */

    EV_RANGE             rng;

    struct PDCHART_DEP_BITS
    {
        int label_first_range : 1;       /* is first element of this dep a category label range? */
        int label_first_item  : 1;       /* are first items in this dep series labels? */

        int reserved          : sizeof(U16)*8 - 2*1;
    }
    bits;
}
PDCHART_DEP, * P_PDCHART_DEP, ** P_P_PDCHART_DEP;

/*
elements in an array belonging to a chart decriptor
*/

typedef struct PDCHART_ELEMENT
{
    PDCHART_RANGE_TYPE type;          /* whether COL or ROW or whatever */

    LIST_ITEMNO        itdepkey;      /* which dep block this element refers to */

    GR_INT_HANDLE      gr_int_handle; /* something to delete me by */

    struct PDCHART_ELEMENT_BITS
    {
        int label_first_range : 1;      /* entire range labels? */
        int label_first_item  : 1;      /* is first item a label? */

        int reserved          : sizeof(U16)*8 - 2*1;
    }
    bits;

    union PDCHART_ELEMENT_RNG
    {
        struct PDCHART_ELEMENT_RNG_COL
        {
            EV_DOCNO docno; /* something to remember the range by EVEN IF NO PD DOCUMENT EXISTS! */
            COL     col;
            ROW     stt_row;
            ROW     end_row;
        }
        col;

        struct PDCHART_ELEMENT_RNG_ROW
        {
            EV_DOCNO docno; /* something to remember the range by EVEN IF NO PD DOCUMENT EXISTS! */
            ROW     row;
            COL     stt_col;
            COL     end_col;
        }
        row;

        struct PDCHART_ELEMENT_RNG_TXT
        {
            EV_DOCNO docno; /* something to remember the range by EVEN IF NO PD DOCUMENT EXISTS! */
            COL     col;
            ROW     row;
        }
        txt;
    }
    rng;
}
PDCHART_ELEMENT, * P_PDCHART_ELEMENT, ** P_P_PDCHART_ELEMENT;

/*
this is in a fixed block
*/

typedef enum PDCHART_RECALC_STATES
{
    PDCHART_UNMODIFIED = 0,
    PDCHART_MODIFIED,
    PDCHART_AWAITING_RECALC
}
PDCHART_RECALC_STATES;

typedef struct PDCHART_HEADER
{
    LIST_ITEMNO         pdchartdatakey; /* our internal handle on this chart */

    GR_CHART_HANDLE     ch;             /* which chart we have made */
    GR_CHARTEDIT_HANDLE ceh;            /* if it has an editor window */

    struct PDCHART_HEADER_RECALC
    {
        PDCHART_RECALC_STATES state;
    }
    recalc;

    struct PDCHART_HEADER_ELEM
    {
        U32                n_alloc; /* number of elements (not bytes) allocated */
        U32                n;       /* number of elements (not bytes) used so far */
        P_PDCHART_ELEMENT  base;
    }
    elem;
}
PDCHART_HEADER, * P_PDCHART_HEADER, ** P_P_PDCHART_HEADER;

/*
a list of PipeDream charts (so they can be selected from)
*/

typedef struct PDCHART_LISTED_DATA
{
    P_PDCHART_HEADER pdchart; /* only need this to find the chart */
}
PDCHART_LISTED_DATA, * P_PDCHART_LISTED_DATA;

/*
a PipeDream shape descriptor for setting up a range to chart (only one at once so don't bitfield the bits)
*/

struct PDCHART_SHAPEDESC_BITS
{
/*  4.12-3  24-Mar-92   SKS changed block sussing fields from poss_label_ to number_
 *                          and defn_label_ to label_ for block sussing improvement
 *                          NB. this also inverts the sense of what was a poss_label_
*/
    U8 number_top_left;
    U8 label_top_left;

    U8 number_left_col;
    U8 label_left_col;

    U8 number_top_row;
    U8 label_top_row;

    U8 label_first_range;
    U8 label_first_item;

    U8 range_over_columns;
    U8 range_over_manual;

    U8 something;
};

 typedef struct PDCHART_SHAPEDESC
{
    DOCNO docno;
    SLR stt;
    SLR end; /*excl*/
    rowcolt n_ranges;

    struct PDCHART_SHAPEDESC_BITS bits;

    SLR min, max, n, nz_n;

    NLISTS_BLK nz_cols; /* list of non-empty columns in range */
    NLISTS_BLK nz_rows; /* list of non-empty rows in range */
}
PDCHART_SHAPEDESC;

typedef struct PDCHART_PROCESS
{
    int initial   : 1;
    int force_add : 1;

    int reserved  : sizeof(U16)*8 - 2*1;
}
PDCHART_PROCESS;

typedef struct PDCHART_SORT_ELEM
{
    P_PDCHART_ELEMENT ep;
    U32               gr_order_no;
}
PDCHART_SORT_ELEM, * P_PDCHART_SORT_ELEM;

#endif /* __pdchart_h */

/* end of pdchart.h */
