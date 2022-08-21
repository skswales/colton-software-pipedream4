/* gr_chart.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Charting module interface */

/* SKS May 1991 */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

/*
internal functions
*/

_Check_return_
static STATUS
gr_chart_build(
    PC_GR_CHART_HANDLE chp);

_Check_return_
static STATUS
gr_chart_clone(
    PC_GR_CHART_HANDLE dst_chp,
    PC_GR_CHART_HANDLE src_chp,
    S32               non_core_too);

static void
gr_chart_clone_noncore_pict_lose_refs(
    PC_GR_CHART_HANDLE chp);

static GR_DATASOURCE_HANDLE
gr_datasource_insert(
    P_GR_CHART           cp,
    gr_chart_travelproc proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out,
    P_GR_INT_HANDLE     p_int_handle_after /*const*/);

static GR_DATASOURCE_NO
gr_series_n_datasources_req(
    P_GR_CHART cp,
    GR_SERIES_IDX series_idx);

/* --------------------------------------------------------------------------------- */

#ifndef GR_SERIES_DESC_INCR
#define GR_SERIES_DESC_INCR 4
#endif

/* no GR_SERIES_DESC_DECR - series descriptor never shrinks */

#ifndef GR_DATASOURCES_DESC_INCR
#define GR_DATASOURCES_DESC_INCR 4
#endif

#ifndef GR_DATASOURCES_DESC_DECR
#define GR_DATASOURCES_DESC_DECR 5
#endif

static BOOL
gr_chart_initialised = FALSE;

/*
a list of charts
*/

static NLISTS_BLK gr_charts =
{
    NULL,
    sizeof32(GR_CHART),
    sizeof32(GR_CHART) * 8
};

/*
exported variables
*/

const GR_CHART_OBJID
gr_chart_objid_anon = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_ANON);

const GR_CHART_OBJID
gr_chart_objid_chart = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_CHART);

const GR_CHART_OBJID
gr_chart_objid_legend = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_LEGEND);

/*
preferred defaults - exported solely for check in chtIO
*/

/*const-to-them*/ GR_CHART_HANDLE
gr_chart_preferred_ch = NULL;

/******************************************************************************
*
* add a data source to a chart
*
******************************************************************************/

static /*const*/ GR_INT_HANDLE gr_datasource_handle_none  = GR_DATASOURCE_HANDLE_NONE;
static /*const*/ GR_INT_HANDLE gr_datasource_handle_start = GR_DATASOURCE_HANDLE_START;
static /*const*/ GR_INT_HANDLE gr_datasource_handle_texts = GR_DATASOURCE_HANDLE_TEXTS;

_Check_return_
extern BOOL
gr_chart_add(
    P_GR_CHART_HANDLE    chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out)
{
    return(gr_chart_insert(chp, proc, ext_handle, p_int_handle_out, &gr_datasource_handle_none));
}

/******************************************************************************
*
* add a datasource as the labels datasource
* even with multiple axes there is only one
* if punter tries to add more then let him
* but its really ignored by us (eg add range)
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_add_labels(
    P_GR_CHART_HANDLE    chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out)

{
    P_GR_CHART cp;

    cp = gr_chart_cp_from_ch(*chp);

    if(cp->core.category_datasource.dsh != GR_DATASOURCE_HANDLE_NONE)
    {
        myassert0x(0, "gr_chart_add_labels: chart already has labels");
        *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;
    }
    else
        (void) gr_datasource_insert(cp, proc, ext_handle,
                                    p_int_handle_out, &gr_datasource_handle_start);

    return(1);
}

/******************************************************************************
*
* add some live text to the chart
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_add_text(
    P_GR_CHART_HANDLE    chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out)
{
    return(gr_chart_insert(chp, proc, ext_handle, p_int_handle_out, &gr_datasource_handle_texts));
}

/******************************************************************************
*
* use external handle to dial up reference to underlying chart structure
*
******************************************************************************/

extern P_GR_CHART
gr_chart_cp_from_ch(
    GR_CHART_HANDLE ch)
{
    LIST_ITEMNO key = (LIST_ITEMNO) ch;
    P_GR_CHART cp = NULL;

    if(ch)
    {
        cp = collect_goto_item(GR_CHART, &gr_charts.lbr, key);
        myassert1x(cp != NULL, "gr_chart_cp_from_ch: failed to find chart handle &%p", ch);
    }

    return(cp);
}

/******************************************************************************
*
* damage a datasource
*
******************************************************************************/

extern void
gr_chart_damage(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE   p_int_handle /*const*/)
{
    GR_CHART_HANDLE  ch;
    P_GR_CHART       cp;
    P_GR_DATASOURCE  dsp;
    GR_AXES_IDX      axes_idx;
    GR_SERIES_IDX    series_idx;
    P_GR_SERIES      serp;

    trace_4(TRACE_MODULE_GR_CHART, "gr_chart_damage(&%p->&%p, &%p->&%p)]n",
            report_ptr_cast(chp), report_ptr_cast(chp ? *chp : NULL), report_ptr_cast(p_int_handle), report_ptr_cast(*p_int_handle));

    ch = *chp;
    if(!ch)
        return;

    cp = gr_chart_cp_from_ch(ch);

    cp->core.modified = 1;

    /* look it up */
    gr_chart_dsp_from_dsh(cp, &dsp, *p_int_handle);

    if(dsp)
    {
        * (int *) &dsp->valid = 0;

        if(dsp == &cp->core.category_datasource)
        {
            for(axes_idx = 0; axes_idx < cp->axes_idx_max; ++axes_idx)
                for(series_idx = cp->axes[axes_idx].series.stt_idx;
                    series_idx < cp->axes[axes_idx].series.end_idx;
                    series_idx++)
                {
                    /* all series in chart depend on this! */
                    serp = getserp(cp, series_idx);

                    * (int *) &serp->valid = 0;
                }
        }
        else
        {
            switch(dsp->id.name)
            {
            case GR_CHART_OBJNAME_TEXT:
                /* internal reformat of text object needed */
                break;

            case GR_CHART_OBJNAME_SERIES:
                {
                series_idx = gr_series_idx_from_external(cp, dsp->id.no);

                serp = getserp(cp, series_idx);

                * (int *) &serp->valid = 0;
                }
                break;

            default:
                assert(0);
            case GR_CHART_OBJNAME_ANON:
                /* SKS after 4.12 30mar92 - note that damages can come through to a datasource even when not yet assigned to a series */
                break;
            }
        }
    }
}

extern void
gr_chart_diagram(
    PC_GR_CHART_HANDLE chp,
    _OutRef_    P_P_GR_DIAG dcpp)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(*chp);

    *dcpp = cp->core.p_gr_diag;
}

/******************************************************************************
*
* propogate damage from datasources and
* user modifications into chart, both in
* editing window (if present) and to disc
* (if present) for client reload
*
******************************************************************************/

extern void
gr_chart_diagram_ensure(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART       cp;
    P_GR_CHARTEDITOR cep;
    S32             requires_building;
    S32             res;

    cp = gr_chart_cp_from_ch(*chp);
    if(!cp)
        return;

    requires_building = cp->core.modified || !cp->core.p_gr_diag || !cp->core.p_gr_diag->gr_riscdiag.draw_diag.length;

    cep = cp->core.ceh ? gr_chartedit_cep_from_ceh(cp->core.ceh) : NULL;

    /* clear down current chart and selection, redrawing editor window sometime */
    if(cep)
    {
        gr_chartedit_selection_kill_repr(cep);

        gr_chartedit_diagram_update_later(cep);
    }

    gr_diag_diagram_dispose(&cp->core.p_gr_diag);

    if(status_fail(res = gr_chart_build(&cp->core.ch)))
    {
        gr_chartedit_winge(res);
    }
    else
    {   /* only save out if built ok: do we know where to save this? */
        if(file_is_rooted(cp->core.currentfilename))
            if(status_fail(res = gr_chart_save_chart_without_dialog(chp, NULL)))
                gr_chart_save_chart_winge(GR_CHART_MSG_CHARTSAVE_ERROR, res);
    }

    if(cep)
    {
        gr_chartedit_set_scales(cep);

        if(cp->core.p_gr_diag)
            gr_chartedit_diagram_update_later(cep);

        if(cep->selection.id.name != GR_CHART_OBJNAME_ANON)
            /* test here too as it's called from callback procs of dialogs too! */
            if(!cep->selection.temp)
                gr_chartedit_selection_make_repr(cep);
    }
}

/******************************************************************************
*
* dispose of a chart
*
******************************************************************************/

static void
gr_chart_dispose_core(
    P_GR_CHART cp)
{
    gr_diag_diagram_dispose(&cp->core.p_gr_diag);

    al_ptr_dispose(P_P_ANY_PEDANTIC(&cp->core.datasources.mh));

    str_clr(&cp->core.currentfilename);
    str_clr(&cp->core.currentdrawname);

    zero_struct(cp->core);
}

static void
gr_chart_dispose_noncore(
    P_GR_CHART cp)
{
    S32          plotidx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;

    /* lose refs to pictures */
    gr_fillstyle_pict_delete(&cp->chart.areastyle);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyle_pict_delete(&cp->plotarea.area[plotidx].areastyle);

    gr_fillstyle_pict_delete(&cp->legend.areastyle);

    /* delete text and text styles from chart */
    gr_chart_list_delete(cp, GR_LIST_CHART_TEXT);
    gr_chart_list_delete(cp, GR_LIST_CHART_TEXT_TEXTSTYLE);

    for(series_idx = 0; series_idx < cp->series.n_defined; ++series_idx)
    {
        /* delete series pictures */
        serp = getserp(cp, series_idx);
        gr_fillstyle_pict_delete(&serp->style.pdrop_fill);
        gr_fillstyle_pict_delete(&serp->style.point_fill);

        /* delete all point info from this series */
        gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_FILLSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_LINESTYLE);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_FILLSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_LINESTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_TEXTSTYLE);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_BARCHSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_BARLINECHSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_LINECHSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_PIECHDISPLSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_PIECHLABELSTYLE);
        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_SCATCHSTYLE);
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&cp->series.mh));

    assert(offsetof(GR_CHART, core) == 0); /* else we'd need another memset of noncore info */
    memset32(PtrAddBytes(P_BYTE, &cp->core, sizeof32(cp->core)), 0, sizeof32(*cp) - sizeof32(cp->core));
}

extern void
gr_chart_dispose(
    P_GR_CHART_HANDLE chp /*inout*/)
{
    GR_CHART_HANDLE ch;
    P_GR_CHART       cp;
    LIST_ITEMNO     key;

    trace_2(TRACE_MODULE_GR_CHART, "gr_chart_dispose(&%p->&%p)]n", report_ptr_cast(chp), report_ptr_cast(chp ? *chp : NULL));

    ch = *chp;
    if(!ch)
        return;
    *chp = NULL;

    /* dispose of substructure (any dependent editors must be dealt with separately) */
    cp = gr_chart_cp_from_ch(ch);
    myassert1x(!cp->core.ceh, "chart editor for chart &%p is still open!", ch);

    gr_chart_dispose_noncore(cp);

    gr_chart_dispose_core(cp);

    /* reconvert ch explicitly for subtract */
    key = (LIST_ITEMNO) ch;

    trace_1(TRACE_MODULE_GR_CHART, "gr_chart_dispose: collect_subtract_entry %d from gr_charts list", key);
    collect_subtract_entry(&gr_charts.lbr, key);
}

/******************************************************************************
*
* initialisation for the chart module and friends
*
******************************************************************************/

static void
gr_chart_initialise(void)
{
    /* enumerate marker and fill files and add cache entries (but don't actually cache) */

    gr_chart_initialised = 1;
}

/******************************************************************************
*
* insert a data source in a chart AFTER a given entry
*
******************************************************************************/

extern BOOL
gr_chart_insert(
    P_GR_CHART_HANDLE    chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out,
    P_GR_INT_HANDLE     p_int_handle_after /*const*/)
{
    P_GR_CHART            cp;
    GR_DATASOURCE_HANDLE dsh;

    *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;

    assert(*chp);
    if(!*chp)
        return(0);

    cp = gr_chart_cp_from_ch(*chp);

    dsh = gr_datasource_insert(cp, proc, ext_handle, p_int_handle_out, p_int_handle_after);

    return(dsh != GR_DATASOURCE_HANDLE_NONE);
}

/******************************************************************************
*
* modify this chart and save if possible
*
******************************************************************************/

extern void
gr_chart_modify_and_rebuild(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART cp;

    cp = gr_chart_cp_from_ch(*chp);
    assert(cp);
    if(!cp)
        return;

    cp->core.modified = 1;

    gr_chart_diagram_ensure(chp);
}

/******************************************************************************
*
* ask chart for its filename if any
*
******************************************************************************/

extern void
gr_chart_name_query(
    PC_GR_CHART_HANDLE chp,
    _Out_writes_(bufsiz) P_U8Z szName,
    _InVal_     U32 bufsiz)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(*chp);

    xstrkpy(szName, bufsiz, cp->core.currentfilename);
}

_Check_return_
extern STATUS
gr_chart_name_set(
    PC_GR_CHART_HANDLE chp,
    _In_opt_z_  PC_U8Z szName)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(*chp);

    return((str_set(&cp->core.currentfilename, szName) >= 0) ? 1 : status_nomem());
}

/******************************************************************************
*
* create a chart
*
******************************************************************************/

static const struct GR_CHART_LAYOUT
gr_chart_layout_default = /* only approx. A8 */
{
    (GR_COORD) (4.00 * GR_PIXITS_PER_INCH), /* w */
    (GR_COORD) (3.00 * GR_PIXITS_PER_INCH), /* h */

    { (GR_PIXITS_PER_INCH * 4) / 10, /* LM */
      (GR_PIXITS_PER_INCH * 3) / 10, /* BM */
      (GR_PIXITS_PER_INCH * 4) / 10, /* RM */
      (GR_PIXITS_PER_INCH * 3) / 10  /* TM */ },

    /* dependent fields filled in by user of this structure */
    { 0, 0 }
};

_Check_return_
extern STATUS
gr_chart_new(
    P_GR_CHART_HANDLE chp /*out*/,
    P_ANY            ext_handle,
    S32              new_untitled)
{
    static LIST_ITEMNO cpkey_gen = 0x32461000; /* NB. not tbs! */

    static U32         nextUntitledNumber = 1;

    GR_CHART_HANDLE ch;
    P_GR_CHART      cp;
    LIST_ITEMNO     key;
    GR_AXES_IDX     axes_idx;
    GR_AXIS_IDX     axis_idx;
    STATUS status;

    if(!gr_chart_initialised)
        gr_chart_initialise();

    *chp = 0;

    /* add to list of charts */
    key  = cpkey_gen++;

    if(NULL == (cp = collect_add_entry_elem(GR_CHART, &gr_charts, &key, &status)))
        return(status);

    /* convert ch explicitly */
    ch = (GR_CHART_HANDLE) key;

    *chp = ch;

    /* empty the descriptor */
    zero_struct_ptr(cp);

    cp->core.ch = ch;

    cp->core.ext_handle = ext_handle;

    /* NB. there is only one of these per chart */
    cp->core.category_datasource.dsh = GR_DATASOURCE_HANDLE_NONE;

    /* initial size of chart (roughly about A8 landscape) */
    cp->core.layout = gr_chart_layout_default;

    cp->core.layout.size.x = cp->core.layout.width  - (cp->core.layout.margins.left   + cp->core.layout.margins.right);
    cp->core.layout.size.y = cp->core.layout.height - (cp->core.layout.margins.bottom + cp->core.layout.margins.top  );

    /* SKS after 4.12 25mar92 - must reallocate series to chart on first gr_chart_build() even if no datasources have been added */
    cp->bits.realloc_series = 1;

    cp->legend.bits.on = 1;

    cp->d3.bits.on     = 1;
    cp->d3.droop       = 10.0; /* DEGREES */
    cp->d3.turn        = 10.0;

    cp->barch.slot_overlap_percentage = 0.0; /* no overlap */
    cp->linech.slot_shift_percentage  = 0.0; /* complete overlap */

    /* normal axes start out life as bar */
    cp->axes[0].sertype   = GR_CHART_SERIES_PLAIN;
    cp->axes[0].charttype = GR_CHARTTYPE_BAR;

    /* overlay axes always starts out life as line */
    cp->axes[1].sertype   = GR_CHART_SERIES_PLAIN;
    cp->axes[1].charttype = GR_CHARTTYPE_LINE;

    for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
    {
        P_GR_AXES p_axes = &cp->axes[axes_idx];

        p_axes->style.barch.slot_width_percentage     = 100.0; /* fill slots widthways */
        p_axes->style.barlinech.slot_depth_percentage =  75.0;
        p_axes->style.linech.slot_width_percentage    =  20.0; /* fractionally fill slots widthways */
        p_axes->style.piechdispl.radial_displacement  =   0.0;
        p_axes->style.scatch.width_percentage         =  20.0;

        /* ensure options saved */
        p_axes->style.barch.bits.manual               = 1;
        p_axes->style.barlinech.bits.manual           = 1;
        p_axes->style.linech.bits.manual              = 1;
        p_axes->style.piechdispl.bits.manual          = 1;
        p_axes->style.piechlabel.bits.manual          = 1;
        p_axes->style.scatch.bits.manual              = 1;

        for(axis_idx = 0; axis_idx < 3; ++axis_idx)
        {
            const P_GR_AXIS p_axis = &p_axes->axis[axis_idx];

            p_axis->bits.incl_zero = 1; /* this is RJM's fault */

            p_axis->punter.min      =  0.0;
            p_axis->punter.max      = 10.0;

            p_axis->major.punter    =  1.0;
            p_axis->major.bits.tick = GR_AXIS_TICK_POSITION_FULL;

            p_axis->minor.punter    =  0.2;
            p_axis->minor.bits.tick = GR_AXIS_TICK_POSITION_NONE;
        }
    }

    cp->axes[1].axis[X_AXIS_IDX].bits.lzr = GR_AXIS_POSITION_BZT_TOP; /* bzt for category or x-axis */
    cp->axes[1].axis[Y_AXIS_IDX].bits.lzr = GR_AXIS_POSITION_LZR_RIGHT;

    gr_colour_set_BLACK(  cp->chart.borderstyle.fg);
                          cp->chart.borderstyle.fg.manual = 1;
    gr_colour_set_NONE(   cp->chart.areastyle.fg);
                          cp->chart.areastyle.fg.manual = 1;
                          cp->chart.areastyle.bits.norecolour = 1;

    gr_colour_set_MIDGRAY(cp->plotarea.area[0].borderstyle.fg);
                          cp->plotarea.area[0].borderstyle.fg.manual = 1;
    gr_colour_set_VLTGRAY(cp->plotarea.area[0].areastyle.fg);
                          cp->plotarea.area[0].areastyle.fg.manual = 1;
                          cp->plotarea.area[0].areastyle.bits.norecolour = 1;

    gr_colour_set_MIDGRAY(cp->plotarea.area[1].borderstyle.fg);
                          cp->plotarea.area[1].borderstyle.fg.manual = 1;
    gr_colour_set_LTGRAY( cp->plotarea.area[1].areastyle.fg);
                          cp->plotarea.area[1].areastyle.fg.manual = 1;

    gr_colour_set_MIDGRAY(cp->plotarea.area[2].borderstyle.fg);
                          cp->plotarea.area[2].borderstyle.fg.manual = 1;
    gr_colour_set_LTGRAY( cp->plotarea.area[2].areastyle.fg);
                          cp->plotarea.area[2].areastyle.fg.manual = 1;

    gr_colour_set_BLACK(  cp->legend.borderstyle.fg);
                          cp->legend.borderstyle.fg.manual = 1;
    gr_colour_set_WHITE(  cp->legend.areastyle.fg);
                          cp->legend.areastyle.fg.manual = 1;
                          cp->legend.areastyle.bits.norecolour = 1;

    gr_colour_set_BLACK(  cp->text.style.base.fg);
                          cp->text.style.base.fg.manual = 1;
    gr_colour_set_NONE(   cp->text.style.base.bg);
    cp->text.style.base.width  = (GR_COORD) (atof(string_lookup(GR_CHART_MSG_DEFAULT_FONTWIDTH )) * GR_PIXITS_PER_POINT);
    cp->text.style.base.height = (GR_COORD) (atof(string_lookup(GR_CHART_MSG_DEFAULT_FONTHEIGHT)) * GR_PIXITS_PER_POINT);
    strcpy(cp->text.style.base.szFontName, string_lookup(GR_CHART_MSG_DEFAULT_FONTNAME));

    {
    char filename[BUF_MAX_GENLEAFNAME];

    *filename = NULLCH;

    if(new_untitled)
        (void) xsnprintf(filename, elemof32(filename),
                string_lookup(GR_CHART_MSG_DEFAULT_CHARTZD),
                nextUntitledNumber++);

    #ifdef GR_CHART_SAVES_ONLY_DRAWFILE
    status_consume(str_set(&cp->core.currentdrawname, filename));
    #endif

    status_consume(str_set(&cp->core.currentfilename, filename));
    }

    return(1);
}

_Check_return_
extern U32
gr_chart_order_query(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE   p_int_handle /*const*/)
{
    P_GR_CHART            cp;
    GR_DATASOURCE_HANDLE dsh;
    P_GR_DATASOURCE       dsp;

    cp = gr_chart_cp_from_ch(*chp);
    if(cp == NULL)
        return(0);

    /* look it up */
    dsh = *p_int_handle;

    gr_chart_dsp_from_dsh(cp, &dsp, dsh);

    if(dsp == NULL)
        return(0);

    if(dsp == &cp->core.category_datasource)
        return(1);
    else
        switch(dsp->id.name)
        {
        case GR_CHART_OBJNAME_TEXT:
            {
            /* SKS after 4.12 27mar92 - needed for live text reload mechanism */
            LIST_ITEMNO key;
            P_GR_TEXT t;

            for(t = collect_first(GR_TEXT, &cp->text.lbr, &key);
                t;
                t = collect_next( GR_TEXT, &cp->text.lbr, &key))
            {
                P_GR_TEXT_GUTS gutsp;

                if(!t->bits.live_text)
                    continue;

                gutsp.mp = (t + 1);
                if(dsp == gutsp.dsp) /* dsp_from_dsh yielded this ptr */
                    return(key);
            }

            assert(0);
            return(0);
            }

        default:
            assert(0);

        case GR_CHART_OBJNAME_SERIES:
            {
            GR_SERIES_IDX series_idx;
            P_GR_SERIES   serp;

            series_idx = gr_series_idx_from_external(cp, dsp->id.no);

            serp = getserp(cp, series_idx);

            assert(serp->datasources.dsh[dsp->id.subno] == dsh);

            /* invert some sort of ordered key to pass back */
            return((dsp->id.no + 1) * 0x100 + dsp->id.subno);
            }
        }
}

_Check_return_
extern BOOL
gr_chart_query_exists(
    /*out*/ P_GR_CHART_HANDLE chp,
    /*out*/ P_P_ANY p_ext_handle,
    _In_z_      PC_U8Z szName)
{
    LIST_ITEMNO key;
    P_GR_CHART   cp;

    for(cp = collect_first(GR_CHART, &gr_charts.lbr, &key);
        cp;
        cp = collect_next( GR_CHART, &gr_charts.lbr, &key))
    {
        if(cp->core.currentfilename)
        {
            if(0 == _stricmp(cp->core.currentfilename, szName))
            {
                if(chp)
                    *chp = cp->core.ch;
                if(p_ext_handle)
                    *p_ext_handle = cp->core.ext_handle;
                return(1);
            }
        }
    }

    return(0);
}

/******************************************************************************
*
* ask chart whether category labels ought to be provided (they are optional even so)
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_query_labelling(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART cp;

    if(chp)
    {
        cp = gr_chart_cp_from_ch(*chp);

        return(cp->axes[0].charttype != GR_CHARTTYPE_SCAT);
    }

    /* return preferred state if no chart to enquire about */

    if(gr_chart_preferred_ch)
    {
        cp = gr_chart_cp_from_ch(gr_chart_preferred_ch);

        return(cp->axes[0].charttype != GR_CHARTTYPE_SCAT);
    }

    /* hardwired state is bar chart so, yes, we will be labelling */

    return(1);
}

/******************************************************************************
*
* ask chart whether category labels have been provided
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_query_labels(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART cp;

    if(chp)
    {
        cp = gr_chart_cp_from_ch(*chp);

        return(cp->core.category_datasource.dsh != GR_DATASOURCE_HANDLE_NONE);
    }

    return(0);
}

/******************************************************************************
*
* remove a data source from a chart
*
******************************************************************************/

/*ncr*/
extern S32
gr_chart_subtract(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE   p_int_handle /*inout*/)
{
    P_GR_CHART            cp;
    GR_DATASOURCE_HANDLE dsh;

    cp = gr_chart_cp_from_ch(*chp);

    if(!p_int_handle)
        return(0);

    dsh = *p_int_handle;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
        return(0);

    *p_int_handle = GR_DATASOURCE_HANDLE_NONE;

    return(gr_chart_subtract_datasource_using_dsh(cp, dsh));
}

/******************************************************************************
*
* allow clients to reregister
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_chart_update_handle(
    PC_GR_CHART_HANDLE chp,
    P_ANY             ext_handle,
    P_GR_INT_HANDLE   p_int_handle /*const*/)
{
    GR_CHART_HANDLE ch;
    P_GR_CHART       cp;
    P_GR_DATASOURCE  dsp;

    trace_4(TRACE_MODULE_GR_CHART, "gr_chart_update_handle(&%p->&%p, &%p->&%p)]n",
            report_ptr_cast(chp), report_ptr_cast(chp ? *chp : NULL), report_ptr_cast(p_int_handle), report_ptr_cast(*p_int_handle));

    ch = *chp;
    if(!ch)
        return(0);

    cp = gr_chart_cp_from_ch(ch);

    /* look it up */
    gr_chart_dsp_from_dsh(cp, &dsp, *p_int_handle);

    if(dsp)
        dsp->ext_handle = ext_handle;

    return(dsp != NULL);
}

/******************************************************************************
*
* insert a data source in a chart AFTER a given entry
*
******************************************************************************/

static GR_DATASOURCE_HANDLE
gr_datasource_insert(
    P_GR_CHART           cp,
    gr_chart_travelproc ext_proc,
    P_ANY               ext_handle,
    P_GR_INT_HANDLE     p_int_handle_out,
    P_GR_INT_HANDLE     p_int_handle_after /*const*/)
{
    P_GR_DATASOURCE  i_dsp;
    P_GR_DATASOURCE  dsp;
    GR_DATASOURCE_NO before_ds;
    P_ANY            mh;
    GR_CHART_OBJID   id;

    /*
    an internal handle for uniquely binding data sources to series
    */
    static GR_INT_HANDLE gr_datasource_handle_gen = (GR_INT_HANDLE) 0x02233220;

    *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;

    if(*p_int_handle_after == GR_DATASOURCE_HANDLE_START)
    {
        /* replace the category data source */
        dsp = &cp->core.category_datasource;
        id  = gr_chart_objid_chart;
    }
    else if(*p_int_handle_after == GR_DATASOURCE_HANDLE_TEXTS)
    {
        /* create new text object */
        LIST_ITEMNO   key;
        P_GR_TEXT      t;
        P_GR_TEXT_GUTS gutsp;
        S32           res;

        key = gr_text_key_for_new(cp);

        if(status_fail(res = gr_text_new(cp, key, NULL, NULL)))
            return(0);

        t = gr_text_search_key(cp, key);
        assert(t);

        t->bits.live_text = 1;

        gutsp.mp = (t + 1);
        dsp = gutsp.dsp;

        gr_chart_objid_from_text(key, &id);
    }
    else
    {
        if(*p_int_handle_after == GR_DATASOURCE_HANDLE_NONE)
            /* insert AT the end */
            before_ds = cp->core.datasources.n;
        else if(*p_int_handle_after == cp->core.category_datasource.dsh)
            /* if inserting after (non null) category data source then insert BEFORE first real data source */
            before_ds = 0;
        else
        {
            /* find the entry to insert BEFORE */
            gr_chart_dsp_from_dsh(cp, &dsp, *p_int_handle_after);
            if(!dsp)
                return(0);

            i_dsp = cp->core.datasources.mh;
            before_ds = (dsp - i_dsp) + 1;
        }

        if(cp->core.datasources.n == cp->core.datasources.n_alloc)
        {
            const U32 n_alloc = cp->core.datasources.n_alloc + GR_DATASOURCES_DESC_INCR;
            STATUS status;

            if(NULL == (mh = _al_ptr_realloc(cp->core.datasources.mh, n_alloc * sizeof32(GR_DATASOURCE), &status)))
                return(0);

            cp->core.datasources.mh      = mh;
            cp->core.datasources.n_alloc = n_alloc;
        }

        dsp = cp->core.datasources.mh;

        /* remember we are now adding BEFORE the given element */
        dsp += before_ds;

        /* move rest of descriptors up to make way */
        memmove32(dsp + 1, /* UP */
                  dsp,
                  sizeof32(*dsp) * (cp->core.datasources.n - before_ds));

        ++cp->core.datasources.n;

        cp->bits.realloc_series = 1;

        /* SKS after 4.12 30mar92 - not yet allocated to series */
        id = gr_chart_objid_anon;
    }

    /* no prior inheritance; clear out descriptor */
    zero_struct_ptr(dsp);

    /* invent a unique non-repeating handle for this datasource */
    gr_datasource_handle_gen = (GR_INT_HANDLE) ((P_U8) gr_datasource_handle_gen + 1);

    dsp->dsh        = gr_datasource_handle_gen;
    dsp->ext_proc   = ext_proc;
    dsp->ext_handle = ext_handle;
    dsp->id         = id;

    *p_int_handle_out = dsp->dsh;

    trace_4(TRACE_MODULE_GR_CHART,
            "gr_datasource_insert(&%p, (&%p,&%p), &%p",
            report_ptr_cast(cp), report_procedure_name(report_proc_cast(ext_proc)), report_ptr_cast(ext_handle), report_ptr_cast(dsp->dsh));
    return(dsp->dsh);
}

/******************************************************************************
*
* remove a data source from a chart
*
******************************************************************************/

/*ncr*/
extern S32
gr_chart_subtract_datasource_using_dsh(
    P_GR_CHART            cp,
    GR_DATASOURCE_HANDLE dsh)
{
    P_GR_DATASOURCE dsp;
    P_GR_DATASOURCE i_dsp;

    /* look it up */
    gr_chart_dsp_from_dsh(cp, &dsp, dsh);

    if(dsp == NULL)
        return(0);

    if(dsp == &cp->core.category_datasource)
        cp->core.category_datasource.dsh = GR_DATASOURCE_HANDLE_NONE;
    else
        switch(dsp->id.name)
        {
        default:
            assert(0);

        case GR_CHART_OBJNAME_ANON:
            /* unused datasource */
            break;

        case GR_CHART_OBJNAME_TEXT:
            /* prevent recall during deletion */
            dsp->dsh = GR_DATASOURCE_HANDLE_NONE;

            gr_text_delete(cp, dsp->id.no);
            break;

        case GR_CHART_OBJNAME_SERIES:
            {
            GR_CHART_OBJID   id;
            GR_SERIES_IDX     series_idx;
            P_GR_SERIES       serp;
            GR_DATASOURCE_NO ds;

            dsp->dsh = GR_DATASOURCE_HANDLE_NONE;

            /* kill client dep */
            gr_travel_dsp(cp, dsp, 0, NULL);

            cp->bits.realloc_series = 1;

            id = dsp->id; /* rembember what series and ds we were used by */

            --cp->core.datasources.n;

            i_dsp = cp->core.datasources.mh;

            /* compact down datasource descriptor array */
            memmove32(dsp, /* DOWN */
                      dsp + 1,
                      sizeof32(*dsp) * (cp->core.datasources.n - (dsp - i_dsp)) );
                                             /* max poss. # */  /* # preceding */

            /* free up some space if worthwhile */
            if( !cp->core.datasources.n  ||
                (cp->core.datasources.n + GR_DATASOURCES_DESC_DECR <= cp->core.datasources.n_alloc))
            {
                STATUS status;
                P_ANY mh = _al_ptr_realloc(cp->core.datasources.mh, cp->core.datasources.n * sizeof32(GR_DATASOURCE), &status);

                /* yes, we can reallocate to zero elements! */
                assert(mh || !cp->core.datasources.n);
                status_assert(status);

                cp->core.datasources.mh      = mh;
                cp->core.datasources.n_alloc = mh ? cp->core.datasources.n : 0;
            }

            /* remove from use in a series */
            series_idx = gr_series_idx_from_external(cp, id.no);

            serp = getserp(cp, series_idx);

            * (int *) &serp->valid = 0;

            assert(serp->datasources.dsh[id.subno] == dsh);
            serp->datasources.dsh[id.subno] = GR_DATASOURCE_HANDLE_NONE;

            /* loop over datasources in this series and see if it needs removing */
            for(ds = 0; ds < GR_SERIES_MAX_DATASOURCES; ++ds)
                if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
                    break;

            if(ds == GR_SERIES_MAX_DATASOURCES)
            {
                P_GR_AXES p_axes;

                /* decrement number of series on these axes and swap
                 * deleted series descriptor out to end of this set
                 * if not already there
                */
                p_axes = gr_axesp_from_series_idx(cp, series_idx);

                if(series_idx != --p_axes->series.end_idx)
                {
                    P_GR_SERIES last_serp;

                    last_serp = getserp(cp, p_axes->series.end_idx);

                    memswap32(serp, last_serp, sizeof32(*serp));
                }
            }
            break;
            }
        }

    return(1);
}

/******************************************************************************
*
* reallocate the data sources we have now amongst the series for this chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_add_series(
    P_GR_CHART cp,
    GR_AXES_IDX axes_idx,
    S32 init)
{
    P_GR_SERIES serp;
    GR_SERIES_IDX series_idx;

    /* add after this one */
    series_idx = cp->axes[axes_idx].series.end_idx;

    /* descriptor doesn't already exist? */
    if(series_idx >= ((axes_idx == 0) ? cp->axes[1].series.stt_idx : cp->series.n_defined))
    {
        if(cp->series.n_defined >= cp->series.n_alloc)
        {
            /* grow series descriptor for these axes to accomodate new series */
            const U32 n_alloc = cp->series.n_alloc + GR_SERIES_DESC_INCR;
            STATUS status;
            P_ANY mh;

            if(NULL == (mh = _al_ptr_realloc(cp->series.mh, n_alloc * sizeof32(*serp), &status)))
                return(status);

            cp->series.mh      = mh;
            cp->series.n_alloc = n_alloc;
        }

        if(axes_idx == 0)
        {
            /* have to move existing defined descriptors from higher axes up (maybe 0) */
            serp = getserp(cp, cp->axes[1].series.stt_idx);

            memmove32(serp + 1 /* UP */,
                      serp,
                      sizeof32(*serp) * (cp->series.n_defined - cp->axes[1].series.stt_idx));

            /* overlay axes start higher up now (even during single axes set preparation) - move together */
            ++cp->axes[1].series.stt_idx;
            ++cp->axes[1].series.end_idx;
        }
        else
            serp = getserp(cp, cp->series.n_defined);

        /* zap this newly created descriptor */
        zero_struct_ptr(serp);

        ++cp->series.n_defined;
    }

    /* if not clearly specified the initialise this new
     * element from a lower friend, if available, or defaults
    */
    serp = getserp(cp, series_idx);

    if(!serp->internal_bits.descriptor_ok && init) /* won't be set if brand new */
    {
        if(cp->axes[axes_idx].series.stt_idx == cp->axes[axes_idx].series.end_idx)
        {
            /* first addition to axes set */
            serp->sertype   = cp->axes[axes_idx].sertype;
            serp->charttype = cp->axes[axes_idx].charttype;
        }
        else
        {
            /* clone some aspects of its friend */
            memcpy32(serp, serp-1, offsetof(GR_SERIES, GR_SERIES_CLONE_END));

            /* dup picture refs */
            gr_fillstyle_ref_add(&serp->style.pdrop_fill);
            gr_fillstyle_ref_add(&serp->style.point_fill);
        }
    }

    serp->datasources.n_req = gr_series_n_datasources_req(cp, series_idx);

    cp->axes[axes_idx].cache.n_series = ++cp->axes[axes_idx].series.end_idx - cp->axes[axes_idx].series.stt_idx;

    cp->series.n = cp->axes[0].cache.n_series + cp->axes[1].cache.n_series;

    return(1);
}

_Check_return_
extern STATUS
gr_chart_realloc_series(
    P_GR_CHART cp)
{
    GR_CHART_OBJID   id;
    GR_SERIES_IDX    series_idx;
    P_GR_SERIES      serp;
    GR_DATASOURCE_NO ds;
    GR_AXES_IDX      axes_idx;
    GR_SERIES_IDX    old_n_series_0, old_n_series_1;
    P_GR_DATASOURCE  dsp, last_dsp;

    cp->bits.realloc_series = 0;

    gr_chart_objid_from_series_no(1, &id);

    /* clear out data source allocation of all axes but preserve series attributes
     * and as we'll be reshuffling data sources between series destroy series caches
    */
    for(series_idx = 0;
        series_idx < cp->series.n_defined;
        series_idx++)
    {
        serp = getserp(cp, series_idx);

        * (int *) &serp->valid = 0;

        serp->datasources.n = 0;

        for(ds = 0; ds < GR_SERIES_MAX_DATASOURCES; ++ds)
            serp->datasources.dsh[ds] = GR_DATASOURCE_HANDLE_NONE;
    }

    /* ensure first axes set always start at zero! */
    assert(cp->axes[0].series.stt_idx == 0);

    /* remember where we had got to (relatively speaking) */
    old_n_series_0 = cp->axes[0].series.end_idx;
    old_n_series_1 = cp->axes[1].series.end_idx - cp->axes[1].series.stt_idx;

    /* 'remove' series from axes sets */
    for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
    {
        cp->axes[axes_idx].series.end_idx = cp->axes[axes_idx].series.stt_idx;
        cp->axes[axes_idx].cache.n_series = 0;
    }

    cp->series.n = 0;

    /* loop over existing series on main axis set refilling
     * them from the data source array or padding with NONE
    */

    series_idx = 0;

    dsp      = cp->core.datasources.mh;
    last_dsp = dsp + cp->core.datasources.n;

#if 1
    /* SKS after 4.12 25mar92 - must allocate at least one series even if this would be completely unfilled */
    do
#else
    while(dsp < last_dsp)
#endif
        {
        /* whether to bomb the new descriptor or leave as is: vvv */
        status_return(gr_chart_add_series(cp, 0, (series_idx >= old_n_series_0)));

        serp = getserp(cp, series_idx);

        /* pour as many datasources in as it will take */
        do  {
            GR_DATASOURCE_HANDLE dsh;

            if(dsp == last_dsp)
                dsh = GR_DATASOURCE_HANDLE_NONE;
            else
            {
                id.no       = gr_series_external_from_idx(cp, series_idx);
                id.subno    = serp->datasources.n;

                dsh         = dsp->dsh;
                (dsp++)->id = id;
            }

            serp->datasources.dsh[serp->datasources.n++] = dsh;
        }
        while(serp->datasources.n < serp->datasources.n_req);

        ++series_idx;
    }
#if 1
    while(dsp < last_dsp);
#endif

    /* if only one axes set then all series are on that */
    if(cp->axes_idx_max > 0)
    {
        /* loop stripping data sources and therefore series from main axes till a respectable balance is achieved */
        while((cp->axes[0].cache.n_series - cp->axes[1].cache.n_series) > 1)
        {
            GR_SERIES_IDX n_series_strip;
            GR_SERIES_IDX new_seridx;

            /* find first free descriptor off end of overlay axes */
            new_seridx = cp->axes[1].series.end_idx;

            /* concoct new series for overlay axes */
            status_return(gr_chart_add_series(cp, 1, ((new_seridx - cp->axes[1].series.end_idx) >= old_n_series_1)));

            /* count number of series we'd have to fully move over from main axes set
             * to satisfy this one new series on the overlay axes
             * note that we never have partially filled series on the overlay axes at this stage
             * all partially filled series are kept on the main axes
            */
            n_series_strip = 0;
            series_idx = cp->axes[0].series.end_idx;
            ds     = 0;
            while(--series_idx >= cp->axes[0].series.stt_idx)
            {
                serp = getserp(cp, series_idx);

                ds += serp->datasources.n;

                if(ds <= serp->datasources.n_req)
                    ++n_series_strip;

                if(ds >= serp->datasources.n_req)
                    break;
            }

            serp = getserp(cp, new_seridx);

            /* if couldn't fully satisfy request from series on main axes, stop */
            /* if number moved from main axes would swing the balance to the overlay side, stop */
            if( (ds < serp->datasources.n_req)                                               ||
                (cp->axes[1].cache.n_series > (cp->axes[0].cache.n_series - n_series_strip)) )
            {
                /* apologies go to the allocator - remove this descriptor and stop */
                --cp->axes[1].series.end_idx;
                --cp->axes[1].cache.n_series;
                --cp->series.n;
                break;
            }
            else
            {
                GR_SERIES_IDX     stop_at_seridx;
                GR_SERIES_IDX     src_seridx;
                GR_SERIES_IDX     dst_seridx;
                GR_DATASOURCE_NO src_ds;
                GR_DATASOURCE_NO dst_ds;
                P_GR_SERIES       src_serp;
                P_GR_SERIES       dst_serp;

                /* strip from some ds point in this series in the main axes set */
                stop_at_seridx = cp->axes[0].series.end_idx - n_series_strip;

                /* loop moving data sources upstream through the overlay descriptors! */
                src_seridx = cp->axes[1].series.end_idx - 1;
                dst_seridx = cp->axes[1].series.end_idx;
                src_ds     = 0;
                dst_ds     = 0;

                dsp = last_dsp;

                for(;;)
                {
                    /* step back one ds; when run out, step back one descriptor,
                     * possibly jumping over the mid-axis gap
                    */
                    if(dst_ds == 0)
                    {
                        if(dst_seridx == cp->axes[1].series.stt_idx)
                            dst_seridx = cp->axes[0].series.end_idx;

                        --dst_seridx;

                        dst_serp = getserp(cp, dst_seridx);
                        dst_ds   = dst_serp->datasources.n_req;
                    }

                    --dst_ds;

                    if(src_ds == 0)
                    {
                        if(src_seridx == cp->axes[1].series.stt_idx)
                            src_seridx = cp->axes[0].series.end_idx;

                        --src_seridx;

                        src_serp = getserp(cp, src_seridx);
                        src_ds   = src_serp->datasources.n_req;
                    }

                    --src_ds;

                    /* move over single datasource per loop */
                    dst_serp = getserp(cp, dst_seridx);
                    src_serp = getserp(cp, src_seridx);
                    dst_serp->datasources.dsh[dst_ds] = src_serp->datasources.dsh[src_ds];
                    dst_serp->datasources.n++;
                    src_serp->datasources.dsh[src_ds] = GR_DATASOURCE_HANDLE_NONE;
                    src_serp->datasources.n--;

                    if(dst_ds)
                        /* keep looping if dst not satisfied, esp. within last strip series */
                        continue;

                    if(src_seridx == stop_at_seridx)
                        break;
                }
            }

            /* possibly several series removed from the main axes set */
            cp->axes[0].series.end_idx -= n_series_strip;
            cp->axes[0].cache.n_series -= n_series_strip;
            cp->series.n               -= n_series_strip;

            /* one more series added to the overlay axes set by allocator */
        }
    }

    /* SKS after 4.12 26mar92 - added overlay reloading helper stuff */
    cp->axes[0].series.start_series = 0;
    if(cp->axes_idx_max > 0)
    {
        if(cp->axes[1].series.start_series >= 0) /* auto allocation */
        {
            if(cp->axes[1].cache.n_series != 0)
                cp->axes[1].series.start_series = gr_series_external_from_idx(cp, cp->axes[1].series.stt_idx);
            else
                cp->axes[1].series.start_series = 0;
        }
    }
    else
        cp->axes[1].series.start_series = 0;

#if 0 /* SKS after 4.12 26mar92 - removed, this should only be set on loading and cloning */
    /* all descriptors on main axes set are now ok
    */
    for(series_idx = cp->axes[0].series.stt_idx;
        series_idx < cp->axes[0].series.end_idx;
        series_idx++)
    {
        serp = getserp(cp, series_idx);

        serp->internal_bits.descriptor_ok = 1;
    }
#endif

    /* can now go back over the datasources and reassign series ownership:
     * note that all series on main axes will be correctly assigned still
    */
    for(series_idx = cp->axes[1].series.stt_idx;
        series_idx < cp->axes[1].series.end_idx;
        series_idx++)
    {
        serp = getserp(cp, series_idx);

#if 0 /* SKS after 4.12 26mar92 - removed, this should only be set on loading and cloning */
        serp->internal_bits.descriptor_ok = 1;
#endif

        id.no = gr_series_external_from_idx(cp, series_idx);

        for(ds = 0; ds < serp->datasources.n_req; ++ds)
        {
            if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
            {
                gr_chart_dsp_from_dsh(cp, &dsp, serp->datasources.dsh[ds]);
                assert(dsp);
                id.subno = ds;
                dsp->id  = id;
            }
        }
    }

    return(1);
}

/******************************************************************************
*
* how many data sources does a series of this type require?
*
******************************************************************************/

static GR_DATASOURCE_NO
gr_series_n_datasources_req(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx)
{
    GR_DATASOURCE_NO n_req;
    P_GR_SERIES       serp;

    serp = getserp(cp, series_idx);

    switch(serp->sertype)
    {
    default:
#if CHECKING
        myassert1x(0, "gr_series_datasources.n_req(type = %d unknown)", serp->sertype);
    case GR_CHART_SERIES_PLAIN:
#endif
        n_req = 1;
        break;

    case GR_CHART_SERIES_POINT:
    case GR_CHART_SERIES_PLAIN_ERROR1:
        n_req = 2;
        break;

    case GR_CHART_SERIES_POINT_ERROR1:
    case GR_CHART_SERIES_PLAIN_ERROR2:
    /*case GR_CHART_SERIES_HILOCLOSE:*/
        n_req = 3;
        break;

    case GR_CHART_SERIES_POINT_ERROR2:
    /*case GR_CHART_SERIES_HILOOPENCLOSE:*/
        n_req = 4;
        break;
    }

    return(n_req);
}

/******************************************************************************
*
* add in any bar or line chart components to the chart
*
******************************************************************************/

static void
gr_chart_cache_n_contrib(
    P_GR_CHART cp)
{
    GR_AXES_IDX   axes_idx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;

    cp->cache.n_contrib_series = 0;
    cp->cache.n_contrib_bars   = 0;
    cp->cache.n_contrib_lines  = 0;

    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        P_GR_AXES p_axes = &cp->axes[axes_idx];

        /* only add one effective series holder, one bar place or line
         * place per set of stacked bars on each axes set
        */
        BOOL series_adding = TRUE;
        BOOL bar_adding    = TRUE;
        BOOL line_adding   = TRUE;

        p_axes->cache.n_contrib_bars  = 0;
        p_axes->cache.n_contrib_lines = 0;

        switch(p_axes->charttype)
        {
        case GR_CHARTTYPE_PIE:
            cp->cache.n_contrib_series = 1;
            break;

        default:
        case GR_CHARTTYPE_SCAT:
            /* normally using all series */
            cp->cache.n_contrib_series += p_axes->series.end_idx;
            cp->cache.n_contrib_series -= p_axes->series.stt_idx;
            break;

        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            for(series_idx = p_axes->series.stt_idx;
                series_idx < p_axes->series.end_idx;
                series_idx++)
            {
                serp = getserp(cp, series_idx);

                switch(serp->charttype)
                {
                default:
                    /* not bar or line, skip */
                    continue;

                case GR_CHARTTYPE_NONE:
                    if(p_axes->charttype == GR_CHARTTYPE_LINE)
                        goto add_line;

                    /* else deliberate drop thru ... */

                case GR_CHARTTYPE_BAR:
                    if(series_adding)
                        ++cp->cache.n_contrib_series;

                    if(bar_adding)
                        ++p_axes->cache.n_contrib_bars;

                    if(p_axes->bits.stacked)
                        series_adding = bar_adding = FALSE;
                    break;

                case GR_CHARTTYPE_LINE:
                add_line:;
                    if(series_adding)
                        ++cp->cache.n_contrib_series;

                    if(line_adding)
                        ++p_axes->cache.n_contrib_lines;

                    if(p_axes->bits.stacked)
                        series_adding = line_adding = FALSE;
                    break;
                }
            }
        }

        cp->cache.n_contrib_bars  += p_axes->cache.n_contrib_bars;
        cp->cache.n_contrib_lines += p_axes->cache.n_contrib_lines;
    }
}

/******************************************************************************
*
* internal traveller - returns zip for invalid handle requests
*
******************************************************************************/

gr_chart_travel_proto(static, gr_chart_empty_traveller)
{
    IGNOREPARM(handle);
    IGNOREPARM(ch);
    IGNOREPARM(item);
    val->type = GR_CHART_VALUE_NONE;
    return(0);
}

static /*const*/ GR_DATASOURCE
gr_chart_empty_traveller_datasource =
{
    GR_DATASOURCE_HANDLE_NONE,

    gr_chart_empty_traveller,
    NULL
};

/******************************************************************************
*
* find a data source descriptor from its handle
* this could do with being fast so make it a list?
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_chart_dsp_from_dsh(
    P_GR_CHART cp,
    _OutRef_    P_P_GR_DATASOURCE dspp,
    GR_DATASOURCE_HANDLE dsh)
{
    P_GR_DATASOURCE dsp;
    P_GR_DATASOURCE last_dsp;
    P_GR_TEXT t;
    LIST_ITEMNO key;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
    {
        *dspp = &gr_chart_empty_traveller_datasource;
        return(1);
    }

    dsp = &cp->core.category_datasource;

    if(dsp->dsh == dsh)
    {
        *dspp = dsp;
        return(1);
    }

    dsp = cp->core.datasources.mh;

    for(last_dsp = dsp + cp->core.datasources.n; dsp < last_dsp; ++dsp)
    {
        if(dsp->dsh == dsh)
        {
            *dspp = dsp;
            return(1);
        }
    }

    for(t = collect_first(GR_TEXT, &cp->text.lbr, &key);
        t;
        t = collect_next( GR_TEXT, &cp->text.lbr, &key))
    {
        P_GR_TEXT_GUTS gutsp;

        if(!t->bits.live_text)
            continue;

        gutsp.mp = (t + 1);
        dsp = gutsp.dsp;

        if(dsp->dsh == dsh)
        {
            *dspp = dsp;
            return(1);
        }
    }

    *dspp = NULL;
    assert(0);
    return(0);
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this datasource and
* ask him about it.
*
******************************************************************************/

extern S32
gr_travel_dsh(
    P_GR_CHART            cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO      item,
    P_GR_CHART_VALUE      pValue /*out*/)
{
    P_GR_DATASOURCE dsp;
    S32            res;

    pValue->type = GR_CHART_VALUE_NONE;

    if(!gr_chart_dsp_from_dsh(cp, &dsp, dsh))
        return(0);

    myassert3x(dsp->ext_proc != NULL, "gr_travel about to call NULL travel proc: chart &%p dsh &%p item %d", cp, dsh, item);

    res = (* dsp->ext_proc) (dsp->ext_handle, cp->core.ch, item, pValue);

    if(res <= 0)
        pValue->type = GR_CHART_VALUE_NONE;

    return(res);
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this datasource and
* ask him what the label is
*
******************************************************************************/

extern void
gr_travel_dsh_label(
    P_GR_CHART            cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO      item,
    P_GR_CHART_VALUE      pValue /*out*/)
{
    gr_travel_dsh(cp, dsh, item, pValue);

    switch(pValue->type)
    {
    case GR_CHART_VALUE_NONE:
        break;

    case GR_CHART_VALUE_TEXT:
        /* got label */
        break;

    case GR_CHART_VALUE_NUMBER:
        {
        S32 decimals = -1;
        S32 eformat  = FALSE;

        pValue->type = GR_CHART_VALUE_TEXT;

        if(fabs(pValue->data.number) >= U32_MAX)
            eformat = TRUE;
        else if(fabs(pValue->data.number) >= 1.0)
            decimals = ((S32) pValue->data.number == pValue->data.number) ? 0 : 2;

        gr_numtostr(pValue->data.text, elemof32(pValue->data.text),
                    &pValue->data.number,
                    eformat,
                    decimals,
                    NULLCH /* dp_ch */,
                    ','    /* ths_ch */);
        }
        break;

    default:
        pValue->type = GR_CHART_VALUE_NONE;
        break;
    }
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this datasource and
* ask him how many items there are in it.
*
******************************************************************************/

extern GR_CHART_ITEMNO
gr_travel_dsh_n_items(
    P_GR_CHART            cp,
    GR_DATASOURCE_HANDLE dsh)
{
    GR_CHART_VALUE value;
    P_GR_DATASOURCE dsp;

    if(!gr_chart_dsp_from_dsh(cp, &dsp, dsh))
        return(0);

    if(dsp->valid.n_items)
        return(dsp->cache.n_items);

    gr_travel_dsp(cp, dsp, GR_CHART_ITEMNO_N_ITEMS, &value);

    if(value.type != GR_CHART_VALUE_N_ITEMS)
        return(0);

    dsp->cache.n_items = value.data.n_items;
    dsp->valid.n_items = 1;
    return(dsp->cache.n_items);
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this datasource and
* ask him about its value, returning a number (zero if he returns a non-number).
*
******************************************************************************/

extern S32
gr_travel_dsh_valof(
    P_GR_CHART cp,
    GR_DATASOURCE_HANDLE dsh,
    GR_CHART_ITEMNO item,
    _OutRef_    P_F64 evalue)
{
    GR_CHART_VALUE value;

    gr_travel_dsh(cp, dsh, item, &value);

    if(value.type != GR_CHART_VALUE_NUMBER)
    {
        *evalue = 0.0;
        return(0);
    }

    *evalue = value.data.number;
    return(1);
}

extern S32
gr_travel_dsp(
    P_GR_CHART        cp,
    P_GR_DATASOURCE   dsp,
    GR_CHART_ITEMNO  item,
    P_GR_CHART_VALUE  pValue /*out*/)
{
    S32 res;

    /* pValue == NULL special value to tell client we no longer want to access this item */
    if(pValue)
        pValue->type = GR_CHART_VALUE_NONE;

    assert(dsp);

    myassert3x(dsp->ext_proc != NULL, "gr_travel about to call NULL travel proc: chart &%p dsp &%p item %d", cp, dsp, item);

    res = (* dsp->ext_proc) (dsp->ext_handle, cp->core.ch, item, pValue);

    if((res <= 0) && pValue)
        pValue->type = GR_CHART_VALUE_NONE;

    return(res);
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this chart and
* ask him what the category label is, inventing one if he can't
*
******************************************************************************/

extern void
gr_travel_categ_label(
    P_GR_CHART       cp,
    GR_CHART_ITEMNO item,
    P_GR_CHART_VALUE pValue /*out*/)
{
    gr_travel_dsh_label(cp, cp->core.category_datasource.dsh, item, pValue);

    if(pValue->type != GR_CHART_VALUE_TEXT)
    {
        /* invent a category label, based on item number */
        pValue->type = GR_CHART_VALUE_TEXT;
        (void) sprintf(pValue->data.text, "%u", item + 1);
    }
}

/******************************************************************************
*
* obtain the datasource handle for a ds in the given series
* unused series have DATASOURCE_NONE set anyway
*
******************************************************************************/

extern GR_DATASOURCE_HANDLE
gr_travel_series_dsh_from_ds(
    P_GR_CHART        cp,
    GR_SERIES_IDX     series_idx,
    GR_DATASOURCE_NO ds)
{
    P_GR_SERIES serp;

    assert(series_idx < cp->series.n_defined);

    serp = getserp(cp, series_idx);

    return(serp->datasources.dsh[ds]);
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this chart/axes/series and
* ask him what the label is, inventing one if he can't
*
******************************************************************************/

extern void
gr_travel_series_label(
    P_GR_CHART       cp,
    GR_SERIES_IDX    series_idx,
    P_GR_CHART_VALUE pValue /*out*/)
{
    GR_DATASOURCE_NO ds;
    P_GR_SERIES       serp;

    assert(series_idx < cp->series.n_defined);

    serp = getserp(cp, series_idx);

#if 1
    /* SKS after 4.12 01apr92 - why had nobody spotted this before? Get series label from Y value where possible */
    ds = 1;
    if(serp->datasources.dsh[ds] == GR_DATASOURCE_HANDLE_NONE)
        --ds;
#else
    ds = 0; /* always get series label from ds 0 in a series */
#endif

    gr_travel_dsh_label(cp, serp->datasources.dsh[ds], GR_CHART_ITEMNO_LABEL, pValue);

    if(pValue->type != GR_CHART_VALUE_TEXT)
    {
        /* invent a label, based on series number */
        pValue->type = GR_CHART_VALUE_TEXT;
        (void) sprintf(pValue->data.text, "S%u", gr_series_external_from_idx(cp, series_idx));
    }
}

/******************************************************************************
*
* pay a visit to the owner of the data we are using in this chart/axes/series/ds and
* ask him how many items there are in it.
*
******************************************************************************/

extern GR_CHART_ITEMNO
gr_travel_series_n_items(
    P_GR_CHART        cp,
    GR_SERIES_IDX     series_idx,
    GR_DATASOURCE_NO ds)
{
    P_GR_SERIES serp;

    assert(series_idx < cp->series.n_defined);

    serp = getserp(cp, series_idx);

    return(gr_travel_dsh_n_items(cp, serp->datasources.dsh[ds]));
}

/* scan all ds belonging to this series */

extern GR_CHART_ITEMNO
gr_travel_series_n_items_total(
    P_GR_CHART    cp,
    GR_SERIES_IDX series_idx)
{
    GR_CHART_ITEMNO  curnum, maxnum;
    GR_DATASOURCE_NO ds;
    P_GR_SERIES       serp;

    assert(series_idx < cp->series.n_defined);

    serp = getserp(cp, series_idx);

    if(serp->valid.n_items_total)
        return(serp->cache.n_items_total);

    maxnum = 0;
    for(ds = 0; ds < serp->datasources.n; ++ds)
    {
        curnum = gr_travel_dsh_n_items(cp, serp->datasources.dsh[ds]);
        if(maxnum < curnum)
            maxnum = curnum;
    }
    serp->cache.n_items_total = maxnum;
    serp->valid.n_items_total = 1;

    return(maxnum);
}

/******************************************************************************
*
* build the representation of this chart for display etc.
*
******************************************************************************/

/* add in actual chart area iff patterned or coloured */

_Check_return_
static STATUS
gr_chart_chart_addin(
    P_GR_CHART cp)
{
    GR_CHART_OBJID id = gr_chart_objid_chart;
    GR_DIAG_OFFSET chartGroupStart;
    GR_BOX chartbox;
    GR_FILLSTYLE fillstyle;
    GR_LINESTYLE linestyle;

    status_return(gr_chart_group_new(cp, &chartGroupStart, id));

    chartbox.x0 = 0;
    chartbox.y0 = 0;
    chartbox.x1 = chartbox.x0 + cp->core.layout.width;
    chartbox.y1 = chartbox.y0 + cp->core.layout.height;

    gr_chart_objid_fillstyle_query(cp, id, &fillstyle);
    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(!fillstyle.bits.notsolid)
        if(linestyle.fg.visible || fillstyle.fg.visible)
            status_return(gr_chart_rectangle_new(cp, id, &chartbox, &linestyle, &fillstyle));

    if(fillstyle.bits.pattern)
        status_return(gr_chart_scaled_picture_add(cp, id, &chartbox, &fillstyle));

    gr_chart_group_end(cp, &chartGroupStart);

    return(1);
}

/* add in plot area */

_Check_return_
static STATUS
gr_chart_plotarea_addin(
    P_GR_CHART cp)
{
    struct GR_CHART_PLOTAREA * plotarea = &cp->plotarea;
    GR_CHART_OBJID id;
    GR_BOX plotareabox;
    GR_DIAG_OFFSET plotGroupStart;
    GR_FILLSTYLE fillstyle;
    GR_LINESTYLE linestyle;
    S32 res = 1;

    /* SKS after 4.12 30mar92 - now individual plotareas don't inherit from each other, divorce selection grouping */
    status_return(res = gr_chart_group_new(cp, &plotGroupStart, gr_chart_objid_anon));

    gr_chart_objid_clear(&id);
    id.name   = GR_CHART_OBJNAME_PLOTAREA;
    id.has_no = 1;
    id.no     = GR_CHART_PLOTAREA_REAR;

    plotareabox.x0 = plotarea->posn.x;
    plotareabox.y0 = plotarea->posn.y;
    plotareabox.x1 = plotarea->posn.x + plotarea->size.x;
    plotareabox.y1 = plotarea->posn.y + plotarea->size.y;

    /* NB. never display wall and floor of box for pies or scatters! */

    gr_chart_objid_fillstyle_query(cp, id, &fillstyle);
    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(!fillstyle.bits.notsolid)
        status_return(gr_chart_rectangle_new(cp, id, &plotareabox, &linestyle, &fillstyle));

    if(fillstyle.bits.pattern)
        status_return(gr_chart_scaled_picture_add(cp, id, &plotareabox, &fillstyle));

    if(cp->d3.bits.use)
    {
        GR_POINT origin, ps1, ps2, ps3;

        /* wall */
        id.no = GR_CHART_PLOTAREA_WALL;

        gr_chart_objid_fillstyle_query(cp, id, &fillstyle);
        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        origin.x = cp->plotarea.posn.x;
        origin.y = cp->plotarea.posn.y;

        ps1.x    = cp->d3.cache.vector_full.x; /* -ve */
        ps1.y    = cp->d3.cache.vector_full.y;

        ps2.x    = ps1.x;
        ps2.y    = ps1.y + plotarea->size.y;

        ps3.x    = 0;
        ps3.y    = plotarea->size.y;

        if(ps1.x)
            if(!fillstyle.bits.notsolid)
                status_return(gr_diag_quadrilateral_new(cp->core.p_gr_diag, NULL, id, &origin, &ps1, &ps2, &ps3, &linestyle, &fillstyle));

        /* floor */
        id.no = GR_CHART_PLOTAREA_FLOOR;

        gr_chart_objid_fillstyle_query(cp, id, &fillstyle);
        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        ps2.x = ps1.x + plotarea->size.x;
        ps2.y = ps1.y;

        ps3.x = plotarea->size.x;
        ps3.y = 0;

        if(ps2.y)
            if(!fillstyle.bits.notsolid)
                status_return(gr_diag_quadrilateral_new(cp->core.p_gr_diag, NULL, id, &origin, &ps1, &ps2, &ps3, &linestyle, &fillstyle));
    }

    gr_chart_group_end(cp, &plotGroupStart);

    return(1);
}

/******************************************************************************
*
* add in legend
*
******************************************************************************/

static void
gr_chart_legend_boxes_prepare(
    PC_GR_POINT curpt,
    P_GR_BOX    rect_box,
    P_GR_BOX    line_box,
    P_GR_BOX    pict_box,
    P_GR_BOX    text_box,
    PC_GR_TEXTSTYLE   textstyle,
    GR_COORD         swidth_px)
{
    rect_box->x0 = curpt->x;
    rect_box->y0 = curpt->y;
    rect_box->x1 = rect_box->x0 + ((textstyle->height * 7) / 8);
    rect_box->y1 = rect_box->y0 + ((textstyle->height * 6) / 8);

    if(line_box)
    {
        line_box->x0 = rect_box->x0 - (rect_box->x1 - rect_box->x0) / 4;
        line_box->y0 =                (rect_box->y0 + rect_box->y1) / 2;
        line_box->x1 = rect_box->x1 + (rect_box->x1 - rect_box->x0) / 4;
        line_box->y1 = line_box->y0;
    }

    /* embed point marker in a smaller box */
    if(pict_box)
    {
        pict_box->x0 = rect_box->x0 + (rect_box->x1 - rect_box->x0) / 8;
        pict_box->y0 = rect_box->y0 + (rect_box->y1 - rect_box->y0) / 8;
        pict_box->x1 = rect_box->x1 - (rect_box->x1 - rect_box->x0) / 8;
        pict_box->y1 = rect_box->y1 - (rect_box->y1 - rect_box->y0) / 8;
    }

    text_box->x0 = rect_box->x1 + (3 * textstyle->height / 4);
    text_box->y0 = curpt->y;
    text_box->x1 = text_box->x0 + swidth_px;
    text_box->y1 = text_box->y0 + textstyle->height;
}

_Check_return_
static S32 /*0,1,2*/
gr_chart_legend_boxes_fitq(
    S32             in_rows,
    PC_GR_BOX  legendbox,
    P_GR_POINT curpt,
    P_GR_POINT maxpt   /*inout*/,
    GR_COORD        linespacing,
    P_GR_BOX   text_box,
    GR_COORD *      maxlegx /*inout*/)
{
    S32 res = 1; /* normal return */

    if(in_rows)
    {
        /* see if legend will fit in remainder of row */
        if(text_box->x1 > legendbox->x1)
        {
            /* drop to start of next row */
            curpt->x = legendbox->x0;

            text_box->y0 -= curpt->y;
            curpt->y     -= linespacing;
            text_box->y0 += curpt->y;

            if(text_box->y0 < legendbox->y0)
                return(0);

            res = 2; /* recompute boxes on return */
        }

        /* record our maximum travel in the -ve y direction */
        maxpt->y = MIN(maxpt->y, text_box->y0);
    }
    else
    {
        /* see if legend will fit in remainder of column */
        if(text_box->y0 < legendbox->y0)
        {
            /* move to top of next column */
            curpt->y = legendbox->y1;

            text_box->x1 -= curpt->x;
            curpt->x      = *maxlegx + linespacing;
            text_box->x1 += curpt->x;

            /* reset maxlegx for this new column */
            *maxlegx = legendbox->x0;

            if(text_box->x1 > legendbox->x1)
                return(0);

            res = 2; /* recompute boxes on return */
        }

        /* record our maximum travel in the +ve x direction */
        maxpt->x = MAX(maxpt->x, text_box->x1);
    }

    return(res);
}

_Check_return_
extern STATUS
gr_chart_legend_addin(
    P_GR_CHART cp)
{
    struct GR_CHART_LEGEND * legend = &cp->legend;
    GR_BOX legendbox;
    GR_BOX legend_margins;
    GR_DIAG_OFFSET legendGroupStart;
    GR_CHART_OBJID legend_id = gr_chart_objid_legend;
    GR_CHART_OBJID intern_id;
    GR_CHARTTYPE   charttype;
    GR_AXES_IDX    axes_idx;
    P_GR_AXES      p_axes;
    GR_SERIES_IDX  series_idx;
    P_GR_SERIES    serp;
    GR_POINT curpt, maxpt;
    GR_COORD       maxlegx;
    GR_TEXTSTYLE   textstyle;
    GR_COORD       linespacing;
    GR_LINESTYLE   linestyle;
    GR_FILLSTYLE   fillstyle;
    S32            n_for_legend;
    GR_BOX rect_box, line_box, pict_box, text_box;
    GR_CHART_VALUE cv;
    HOST_FONT      f;
    GR_MILLIPOINT  swidth_mp;
    GR_COORD       swidth_px;
    S32            res;
    S32            pass_out;

    if(!legend->bits.on)
        return(1);

    gr_chart_objid_fillstyle_query(cp, legend_id, &fillstyle);
    gr_chart_objid_linestyle_query(cp, legend_id, &linestyle);
    gr_chart_objid_textstyle_query(cp, legend_id, &textstyle);

    linespacing = (textstyle.height * 12) / 10;

    charttype = cp->axes[0].charttype;

    switch(charttype)
    {
    case GR_CHARTTYPE_PIE:
        axes_idx = 0;
        series_idx = cp->pie_series_idx;
        n_for_legend = gr_travel_series_n_items_total(cp, series_idx);
        break;

    default:
        n_for_legend = 0;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            n_for_legend += cp->axes[axes_idx].series.end_idx;
            n_for_legend -= cp->axes[axes_idx].series.stt_idx;
        }
        break;
    }

    /* if nothing to legend, return */
    if(!n_for_legend)
        return(1);

    legend_margins.x0 = MAX(GR_CHART_LEGEND_LM,      linespacing  / 2);
    legend_margins.y0 = MAX(GR_CHART_LEGEND_BM, (2 * linespacing) / 4);
    legend_margins.x1 = MAX(GR_CHART_LEGEND_RM,      linespacing  / 2);
    legend_margins.y1 = MAX(GR_CHART_LEGEND_TM, (5 * linespacing) / 4); /* account for baseline offset here */

    if(legend->bits.manual)
    {
        legendbox.x0 = legend->posn.x;
        legendbox.y0 = legend->posn.y;
        legendbox.x1 = legend->posn.x + legend->size.x;
        legendbox.y1 = legend->posn.y + legend->size.y;

        /* take margins off box */
        legendbox.x0 += legend_margins.x0;
        legendbox.y0 += legend_margins.y0;
        legendbox.x1 -= legend_margins.x1;
        legendbox.y1 -= legend_margins.y1;

        /* ensure always 'reasonable' size even if punter has set wally */
        legendbox.x1 = MAX(legendbox.x1, legendbox.x0 + 3 * linespacing);
        legendbox.y0 = MIN(legendbox.y0, legendbox.y1 - 2 * linespacing);
    }
    else
    {
        if(legend->bits.in_rows)
        {
            /* legend at bottom, goes right across rows */
            GR_COORD x_size;

            /* ensure always 'reasonable' size even if punter has set wally */
            x_size = cp->core.layout.size.x;
            x_size = MAX(x_size, 4 * linespacing);
            x_size = MIN(x_size, cp->core.layout.width);

            legendbox.x0 = cp->core.layout.margins.left;
            legendbox.x1 = legendbox.x0 + x_size;

            legendbox.y1 = 0;

            /* ensure box down by this amount */
            legendbox.y1 -= GR_CHART_LEGEND_TM;

            /* apply margin (and baseline offset) to top edge */
            legendbox.y1 -= legend_margins.y1;

            legendbox.y0 = S32_MIN; /* can grow to be as deep as it needs to be */
        }
        else
        {
            /* legend at right, goes down in columns */
            GR_COORD estdepth, y_size;

            legendbox.x0 = cp->core.layout.width;
            legendbox.x1 = S32_MAX; /* can grow to be as wide as it needs to be */

            /* ensure always 'reasonable' size even if punter has set wally */
            y_size = cp->core.layout.size.y;
            y_size = MAX(y_size, linespacing);
            y_size = MIN(y_size, cp->core.layout.height);

            /* have a guess at how big the legend ought to be, and centre if possible */
            estdepth = n_for_legend * linespacing;
            estdepth = MIN(estdepth, y_size);

            legendbox.y0 = (cp->core.layout.margins.bottom + y_size - estdepth) / 2;
            legendbox.y1 = legendbox.y0 + estdepth;

            /* ensure box out by this amount */
            legendbox.x0 += GR_CHART_LEGEND_LM;

            /* apply margin to left hand edge */
            legendbox.x0 += legend_margins.x0;

            /* NB. no output movement on legendbox.y1 */
        }
    }

    /* record furthest positions we get to */
    maxpt.x = legendbox.x0;
    maxpt.y = legendbox.y1;

    f = gr_riscdiag_font_from_textstyle(&textstyle);

    res = 1;

    for(pass_out = 0; pass_out <= 1; ++pass_out)
    {
        if(pass_out && !legend->bits.manual)
        {
            /* now we can use the info gathered on pass 1 to make a reasonable legend box */
            if(legend->bits.in_rows)
            {
                legendbox.y0 = maxpt.y;
            }
            else
            {
                legendbox.x1 = maxpt.x;
            }
        }

        legend->posn.x = legendbox.x0;
        legend->posn.y = legendbox.y0;
        legend->size.x = legendbox.x1 - legendbox.x0;
        legend->size.y = legendbox.y1 - legendbox.y0;

        /* recompute outer box from inner box and margins (including baseline offset) */
        legend->posn.x -= legend_margins.x0;
        legend->posn.y -= legend_margins.y0;
        legend->size.x += legend_margins.x0 + legend_margins.x1;
        legend->size.y += legend_margins.y0 + legend_margins.y1;

        maxlegx = legendbox.x0;

        curpt.x = legendbox.x0;
        curpt.y = legendbox.y1;

        if(pass_out)
        {
            GR_BOX legend_outer_rect;

            status_break(res = gr_chart_group_new(cp, &legendGroupStart, legend_id));

            legend_outer_rect.x0 = legend->posn.x;
            legend_outer_rect.y0 = legend->posn.y;
            legend_outer_rect.x1 = legend->posn.x + legend->size.x;
            legend_outer_rect.y1 = legend->posn.y + legend->size.y;

            if(!fillstyle.bits.notsolid)
                status_break(res = gr_chart_rectangle_new(cp, legend_id, &legend_outer_rect, &linestyle, &fillstyle));

            if(fillstyle.bits.pattern)
                status_break(res = gr_chart_scaled_picture_add(cp, legend_id, &legend_outer_rect, &fillstyle));
        }

        switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
            {
            /* loop plotting a rectangle of colour and category label for each category */
            GR_CHART_ITEMNO point;

            gr_chart_objid_clear(&intern_id);
            intern_id.name      = GR_CHART_OBJNAME_POINT;
            intern_id.no        = gr_series_external_from_idx(cp, cp->pie_series_idx);
            intern_id.has_no    = 1;
            intern_id.has_subno = 1;

            for(point = 0; point < n_for_legend; ++point)
            {
                gr_travel_categ_label(cp, point, &cv);

                swidth_mp = gr_font_stringwidth(f, cv.data.text);
                swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                gr_chart_legend_boxes_prepare(&curpt, &rect_box, NULL, NULL, &text_box, &textstyle, swidth_px);

                res = gr_chart_legend_boxes_fitq(legend->bits.in_rows, &legendbox, &curpt, &maxpt, linespacing, &text_box, &maxlegx);

                if(res == 0)
                    break;

                maxlegx = MAX(maxlegx, text_box.x1);

                if(pass_out)
                {
                    if(res == 2)
                        gr_chart_legend_boxes_prepare(&curpt, &rect_box, NULL, NULL, &text_box, &textstyle, swidth_px);

                    intern_id.subno = (U16) gr_point_external_from_key(point);

                    gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);
                    gr_chart_objid_fillstyle_query(cp, intern_id, &fillstyle);

                    status_break(res = gr_chart_rectangle_new(cp, legend_id, &rect_box, &linestyle, &fillstyle));

                    if(swidth_px)
                    {
                        status_break(res = gr_chart_text_new(cp, gr_chart_objid_legend, &text_box, cv.data.text, &textstyle));
                        res = STATUS_DONE;
                    }
                }

                if(legend->bits.in_rows)
                    curpt.x  = text_box.x1 + (linespacing / 2);
                else
                    curpt.y -= linespacing;
            }
            break;
            }

        default:
            /* loop plotting the point marker and series label for each series */

            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
            {
                p_axes = &cp->axes[axes_idx];

                for(series_idx = p_axes->series.stt_idx;
                    series_idx < p_axes->series.end_idx;
                    series_idx++)
                {
                    /* loop plotting a rectangle of colour and category label for each category */

                    gr_travel_series_label(cp, series_idx, &cv);

                    swidth_mp = gr_font_stringwidth(f, cv.data.text);
                    swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                    gr_chart_legend_boxes_prepare(&curpt, &rect_box, &line_box, &pict_box, &text_box, &textstyle, swidth_px);

                    res = gr_chart_legend_boxes_fitq(legend->bits.in_rows, &legendbox, &curpt, &maxpt, linespacing, &text_box, &maxlegx);

                    if(res == 0)
                        break;

                    maxlegx = MAX(maxlegx, text_box.x1);

                    if(pass_out)
                    {
                        S32 linestyle_using_default;

                        if(res == 2)
                            gr_chart_legend_boxes_prepare(&curpt, &rect_box, &line_box, &pict_box, &text_box, &textstyle, swidth_px);

                        gr_chart_objid_from_series_no(gr_series_external_from_idx(cp, series_idx), &intern_id);

                        linestyle_using_default =
                        gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);
                        gr_chart_objid_fillstyle_query(cp, intern_id, &fillstyle);

                        serp = getserp(cp, series_idx);

                        charttype = (serp->charttype != GR_CHARTTYPE_NONE)
                                                ? serp->charttype
                                                : p_axes->charttype;

                        if(charttype == GR_CHARTTYPE_BAR)
                        {
                            if(!fillstyle.bits.notsolid)
                                status_break(res = gr_chart_rectangle_new(cp, legend_id, &rect_box, &linestyle, &fillstyle));

                            if(fillstyle.bits.pattern)
                                status_break(res = gr_chart_scaled_picture_add(cp, legend_id, &rect_box, &fillstyle));
                        }
                        else
                        {
                            if(cp->d3.bits.use && (charttype == GR_CHARTTYPE_LINE))
                            {
                                /* draw a section of ribbon through the box */
                                line_box.y0 -= (rect_box.x1 - rect_box.x0) / 4;
                                line_box.y1 += (rect_box.x1 - rect_box.x0) / 4;

                                if(!fillstyle.bits.notsolid)
                                    status_break(res = gr_chart_rectangle_new(cp, gr_chart_objid_legend, &line_box, &linestyle, &fillstyle));
                            }
                            else
                            {
                                /* draw a centred line through the box */

                                /* BODGE: in 2-D line chart mode derive line chart line colour from fill colour */
                                if(linestyle_using_default && (charttype == GR_CHARTTYPE_LINE))
                                    linestyle.fg = fillstyle.fg;

                                status_break(res = gr_chart_line_new(cp, legend_id, &line_box, &linestyle));
                            }

                            if(fillstyle.bits.pattern)
                                status_break(res = gr_chart_scaled_picture_add(cp, legend_id, &pict_box, &fillstyle));
                        }

                        if(swidth_px)
                        {
                            status_break(res = gr_chart_text_new(cp, legend_id, &text_box, cv.data.text, &textstyle));
                            res = STATUS_DONE;
                        }
                    }

                    if(legend->bits.in_rows)
                        curpt.x  = text_box.x1 + linespacing / 2;
                    else
                        curpt.y -= linespacing;
                }
            }
            break;
        }

        if(pass_out)
            gr_chart_group_end(cp, &legendGroupStart);

        status_break(res);
    }

    gr_riscdiag_font_dispose(&f);

    return(res);
}

static void
gr_chart_init_for_build(
    P_GR_CHART cp)
{
    GR_BOX plotarea;
    S32 /*GR_SERIES_NO*/ n_series_deep;
    GR_POINT new_posn;

    /* reflect 3-D usability */
    switch(cp->axes[0].charttype)
    {
    case GR_CHARTTYPE_PIE:
    case GR_CHARTTYPE_SCAT:
        cp->d3.bits.use = 0;
        break;

    default:
        assert(0);
    case GR_CHARTTYPE_BAR:
    case GR_CHARTTYPE_LINE:
        cp->d3.bits.use = cp->d3.bits.on;
        break;
    }

    /* given the chart size, work out where the plot area ought to be */

    /* first guess is within the chart margins */
    plotarea.x0 = cp->core.layout.margins.left;
    plotarea.y0 = cp->core.layout.margins.bottom;
    plotarea.x1 = cp->core.layout.width  - cp->core.layout.margins.right;
    plotarea.y1 = cp->core.layout.height - cp->core.layout.margins.top;

    gr_chart_cache_n_contrib(cp);

    /* affected by stacking */
    n_series_deep = cp->cache.n_contrib_series ? cp->cache.n_contrib_series : 1;

    if(cp->d3.bits.use)
    {
        F64 sin_t = sin(cp->d3.turn  * _radians_per_degree);
        F64 cos_t = cos(cp->d3.turn  * _radians_per_degree);
        F64 sin_d = sin(cp->d3.droop * _radians_per_degree);
        F64 cos_d = cos(cp->d3.droop * _radians_per_degree);
        /* NB. the following is now an int 'cos unsigneds promote ints
         * to unsigned and crap on the division result for -ve numbers
        */
        GR_POINT size;

        /* given current allowable size, work out how to share it out */
        size.x = plotarea.x1 - plotarea.x0;
        size.x = (GR_COORD) ((size.x * cos_t) / (cos_t + sin_t));
        size.y = plotarea.y1 - plotarea.y0;
        size.y = (GR_COORD) ((size.y * cos_d) / (cos_d + sin_d));

        new_posn.x = plotarea.x1 - size.x;
        new_posn.y = plotarea.y1 - size.y;
    }
    else
    {
        /* make null 3-D vectors */
        new_posn.x = plotarea.x0;
        new_posn.y = plotarea.y0;
    }

    cp->d3.cache.vector_full.x = plotarea.x0 - new_posn.x; /* 0 or -ve */
    cp->d3.cache.vector_full.y = plotarea.y0 - new_posn.y; /* 0 or -ve */

    cp->d3.cache.vector.x      = cp->d3.cache.vector_full.x / n_series_deep;
    cp->d3.cache.vector.y      = cp->d3.cache.vector_full.y / n_series_deep;

    cp->d3.valid.vector        = 1;

    plotarea.x0 = new_posn.x;
    plotarea.y0 = new_posn.y;

    cp->plotarea.posn.x = plotarea.x0;
    cp->plotarea.posn.y = plotarea.y0;
    cp->plotarea.size.x = plotarea.x1 - plotarea.x0;
    cp->plotarea.size.y = plotarea.y1 - plotarea.y0;
}

#include <signal.h>

typedef void (__cdecl * P_PROC_SIGNAL) (
    _In_        int sig);

#include <setjmp.h>

static jmp_buf gr_chart_jmp_buf;

static void
gr_chart_signal_handler(int sig)
{
    longjmp(gr_chart_jmp_buf, sig);
}

_Check_return_
static STATUS
gr_chart_build(
    PC_GR_CHART_HANDLE  chp)
{
    P_GR_CHART cp;
    P_GR_DIAG p_gr_diag;
    S32 res;
    P_PROC_SIGNAL oldfpe;

    oldfpe = signal(SIGFPE, gr_chart_signal_handler);

    if(0 != setjmp(gr_chart_jmp_buf))
    {
        reportf("*** gr_chart_build: setjmp returned from signal handler ***");
        (void) signal(SIGFPE, oldfpe);
        return(create_error(GR_CHART_ERR_EXCEPTION));
    }

    cp = gr_chart_cp_from_ch(*chp);

    /* blow some cached info */
    * (int *) &cp->d3.valid = 0;

    if(cp->bits.realloc_series)
        gr_chart_realloc_series(cp);

    /* reset warning status in editor */
    gr_chartedit_warning(cp, STATUS_OK);

    gr_chart_init_for_build(cp);

    /* loop for structure */
    for(;;)
    {
        cp->core.p_gr_diag = p_gr_diag = gr_diag_diagram_new(GR_CHART_CREATORNAME, &res);

        if(NULL == p_gr_diag)
            break;

        { /* create some hierarchy to help correlator */
        GR_DIAG_OFFSET bgGroupStart;

        status_break(res = gr_chart_group_new(cp, &bgGroupStart, gr_chart_objid_anon));

        /* add in actual chart area */
        status_break(res = gr_chart_chart_addin(cp));

        /* add in plot area */
        status_break(res = gr_chart_plotarea_addin(cp));

        gr_chart_group_end(cp, &bgGroupStart);
        } /*block*/

        switch(cp->axes[0].charttype)
        {
        case GR_CHARTTYPE_PIE:
            /* loop over categories plotting sectors */
            res = gr_pie_addin(cp);
            break;

        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            res = gr_barlinechart_addin(cp);
            break;

        default:
            assert(0);
        case GR_CHARTTYPE_SCAT:
            res = gr_scatterchart_addin(cp);
            break;
        }

        if(res < 0)
            break;

        { /* create some more hierarchy to help correlator */
        GR_DIAG_OFFSET fgGroupStart;

        status_break(res = gr_chart_group_new(cp, &fgGroupStart, gr_chart_objid_anon));

        /* add in legend */
        status_break(res = gr_chart_legend_addin(cp));

        status_break(res = gr_text_addin(cp));

        gr_chart_group_end(cp, &fgGroupStart);
        } /*block*/

        gr_diag_diagram_end(p_gr_diag);

        /* end of loop for structure */
        break;
    }

    (void) signal(SIGFPE, oldfpe);

    return(res);
}

extern void
gr_chart_object_name_from_id(
    P_U8Z szName /*out*/,
    _InVal_ U32 elemof_buffer,
    _InVal_     GR_CHART_OBJID id)
{
    P_U8Z out = szName;

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
        assert(elemof_buffer);
        if(elemof_buffer)
            *out = NULLCH;
        break;

    case GR_CHART_OBJNAME_CHART:
        xstrkpy(out, elemof_buffer, "Chart");
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        if(id.no == 1)
            xstrkpy(out, elemof_buffer, "Wall");
        else if(id.no == 2)
            xstrkpy(out, elemof_buffer, "Floor");
        else
            xstrkpy(out, elemof_buffer, "Plot area");
        break;

    case GR_CHART_OBJNAME_LEGEND:
        xstrkpy(out, elemof_buffer, "Legend");
        break;

    case GR_CHART_OBJNAME_TEXT:
        (void) xsnprintf(out, elemof_buffer, "Text " U16_FMT, id.no /*.text*/);
        break;

    case GR_CHART_OBJNAME_SERIES:
        (void) xsnprintf(out, elemof_buffer, "Series " U16_FMT, id.no /*.series*/);
        break;

    case GR_CHART_OBJNAME_POINT:
        (void) xsnprintf(out, elemof_buffer, "S" U16_FMT "P" U16_FMT, id.no /*.series*/, id.subno);
        break;

    case GR_CHART_OBJNAME_DROPSER:
        (void) xsnprintf(out, elemof_buffer, "Drop S" U16_FMT, id.no /*.series*/);
        break;

    case GR_CHART_OBJNAME_DROPPOINT:
        (void) xsnprintf(out, elemof_buffer, "Drop S" U16_FMT "P" U16_FMT, id.no /*.series*/, id.subno);
        break;

    case GR_CHART_OBJNAME_AXIS:
        (void) xsnprintf(out, elemof_buffer, "Axis " U16_FMT, id.no /*.axis*/);
        break;

    case GR_CHART_OBJNAME_AXISGRID:
        out += xsnprintf(out, elemof_buffer, "Grid " U16_FMT, id.no /*.axis*/);
        if(id.subno > 1)
            (void) xsnprintf(out, elemof_buffer - (out - szName), "-" U16_FMT, id.subno);
        break;

    case GR_CHART_OBJNAME_AXISTICK:
        out += xsnprintf(out, elemof_buffer, "Tick " U16_FMT, id.no /*.axis*/);
        if(id.subno > 1)
            (void) xsnprintf(out, elemof_buffer - (out - szName), "-" U16_FMT, id.subno);
        break;

    case GR_CHART_OBJNAME_BESTFITSER:
        (void) xsnprintf(out, elemof_buffer, "Best fit " U16_FMT, id.no /*.series*/);
        break;

    default:
        (void) xsnprintf(out, elemof_buffer,
                         id.has_subno
                             ? "O%u(%u.%u)"
                             :
                         id.has_no
                             ? "O%u(%u)"
                             : "O%u",
                         id.name, id.no, id.subno);
        break;
    }
}

extern PC_U8Z
gr_chart_object_name_from_id_quick(
    _InVal_     GR_CHART_OBJID id)
{
    static U8Z name[BUF_MAX_GR_CHART_OBJID_REPR];

    gr_chart_object_name_from_id(name, elemof32(name), id);

    return(name);
}

/******************************************************************************
*
* find the 'parent' of an object for selection porpoises
*
******************************************************************************/

extern void
gr_chart_objid_find_parent(
     _InoutRef_  P_GR_CHART_OBJID p_id)
{
    GR_CHART_OBJID_NAME new_name = GR_CHART_OBJNAME_ANON;

    switch(p_id->name)
    {
    case GR_CHART_OBJNAME_POINT:
        new_name = GR_CHART_OBJNAME_SERIES;
        break;

    case GR_CHART_OBJNAME_DROPPOINT:
        new_name = GR_CHART_OBJNAME_DROPSER;
        break;

    /* note that AXISGRID and AXISTICK aren't considered as AXIS is easy to hit */

    default:
        break;
    }

    if(new_name == GR_CHART_OBJNAME_ANON)
        return;

    p_id->name      = new_name;
    p_id->has_subno = 0;
    p_id->subno     = 0; /* no longer just for debugging clarity, needed for diagram searches too */
}

_Check_return_
extern BOOL
gr_chart_query_modified(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART cp;

    cp = gr_chart_cp_from_ch(*chp);
    assert(cp);

    return(cp->core.modified);
}

/******************************************************************************
*
* new chart using preferred state
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_new(
    P_GR_CHART_HANDLE chp /*out*/,
    P_ANY            ext_handle)
{
    status_return(gr_chart_new(chp, ext_handle, 1));

    if(!gr_chart_preferred_ch)
        return(1);

    return(gr_chart_clone(chp, &gr_chart_preferred_ch, 1 /*non-core info too*/));
}

/******************************************************************************
*
* query the existence of a preferred state
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_preferred_query(void)
{
    return(gr_chart_preferred_ch != 0);
}

/******************************************************************************
*
* save preferred state into a file
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_save(
    P_U8 filename /*const*/)
{
    if(gr_chart_preferred_ch)
        return(gr_chart_save_chart_without_dialog(&gr_chart_preferred_ch, filename));

    assert(0);
    return(1);
}

/******************************************************************************
*
* copy chart data as preferred state
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_set(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART pcp;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;
    STATUS res;

    if(gr_chart_preferred_ch)
        gr_chart_dispose(&gr_chart_preferred_ch);

    status_return(gr_chart_new(&gr_chart_preferred_ch, &gr_chart_preferred_ch /*internal creation*/, 0));

    if(status_fail(res = gr_chart_clone(&gr_chart_preferred_ch, chp, 1 /*non-core info too*/)))
    {
        gr_chart_dispose(&gr_chart_preferred_ch);
        return(res);
    }

    gr_chart_clone_noncore_pict_lose_refs(&gr_chart_preferred_ch);

    pcp = gr_chart_cp_from_ch(gr_chart_preferred_ch);

    /* run over all series in preferred world and give them the ok */
    for(series_idx = 0; series_idx < pcp->series.n_defined; ++series_idx)
    {
        serp = getserp(pcp, series_idx);

        serp->internal_bits.descriptor_ok = 1;
    }

    return(1);
}

/******************************************************************************
*
* reflect preferred state into this chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_use(
    PC_GR_CHART_HANDLE chp)
{
    if(!gr_chart_preferred_ch)
        return(1);

    return(gr_chart_clone(chp, &gr_chart_preferred_ch, 0 /*leave non-core info alone*/));
}

/*
helper routines for preferred state handling
*/

_Check_return_
static STATUS
gr_chart_clone(
    PC_GR_CHART_HANDLE dst_chp,
    PC_GR_CHART_HANDLE src_chp,
    S32               non_core_too)
{
    P_GR_CHART   dst_cp;
    S32          plotidx;
    GR_SERIES_IDX series_idx;
    GR_AXES_IDX  axes_idx;
    P_GR_SERIES  dst_serp;
    STATUS res = STATUS_OK;

    dst_cp = gr_chart_cp_from_ch(*dst_chp);

    /* blow away current non-core contents */
    gr_chart_dispose_noncore(dst_cp);

    {
    PC_GR_CHART src_cp = gr_chart_cp_from_ch(*src_chp); /* restrict scope */

    if(non_core_too)
    {
        dst_cp->core.layout   = src_cp->core.layout;
        dst_cp->core.editsave = src_cp->core.editsave;
    }

    /* replicate current non-core contents from source */
    assert(offsetof(GR_CHART, core) == 0); /* else we'd need another memcpy of noncore info */
    memcpy32(( P_BYTE) dst_cp  + sizeof32(dst_cp->core),
                  (PC_BYTE) src_cp  + sizeof32(dst_cp->core),
                  sizeof32(*dst_cp) - sizeof32(dst_cp->core));

    /* dup series descriptor, packing down */
    dst_cp->axes[0].series.end_idx -= dst_cp->axes[0].series.stt_idx; /* make relative */
    dst_cp->axes[0].series.stt_idx  = 0;
/*  dst_cp->axes[0].series.end_idx += dst_cp->axes[0].series.stt_idx; !* make abs again */

    dst_cp->axes[1].series.end_idx -= dst_cp->axes[1].series.stt_idx; /* make relative */
    dst_cp->axes[1].series.stt_idx  = dst_cp->axes[0].series.end_idx;
    dst_cp->axes[1].series.end_idx += dst_cp->axes[1].series.stt_idx; /* make abs again */

    dst_cp->series.n_alloc = 0;

    for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
    {
        dst_cp->axes[axes_idx].cache.n_series = dst_cp->axes[axes_idx].series.end_idx - dst_cp->axes[axes_idx].series.stt_idx;
        dst_cp->series.n_alloc += dst_cp->axes[axes_idx].cache.n_series;
    }

    dst_cp->series.n_defined = dst_cp->series.n_alloc;

    if(!dst_cp->series.n_alloc)
        dst_cp->series.mh = NULL;
    else
    {
        if(NULL == (dst_cp->series.mh = al_ptr_alloc_elem(GR_SERIES, (S32) dst_cp->series.n_alloc, &res)))
            return(res);

        /* copy over in two sections such that destination is packed */
        for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
        {
            P_GR_SERIES src_serp;

            src_serp = src_cp->series.mh;
            dst_serp = dst_cp->series.mh;

            src_serp += src_cp->axes[axes_idx].series.stt_idx;
            dst_serp += dst_cp->axes[axes_idx].series.stt_idx;

            memcpy32(dst_serp, src_serp, sizeof32(*dst_serp) *
                (dst_cp->axes[axes_idx].series.end_idx - dst_cp->axes[axes_idx].series.stt_idx));
        }
    }
    } /*block*/

    /* have to properly duplicate the lists */

    /* have to properly dup the picture refs */
    gr_fillstyle_ref_add(&dst_cp->chart.areastyle);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyle_ref_add(&dst_cp->plotarea.area[plotidx].areastyle);

    gr_fillstyle_ref_add(&dst_cp->legend.areastyle);

    /* dup text and text styles from this chart */
    status_accumulate(res, gr_chart_list_duplic(dst_cp, GR_LIST_CHART_TEXT));
    status_accumulate(res, gr_chart_list_duplic(dst_cp, GR_LIST_CHART_TEXT_TEXTSTYLE));

    for(series_idx = 0; series_idx < dst_cp->series.n_defined; ++series_idx)
    {
        /* dup refs to series pictures */
        dst_serp = getserp(dst_cp, series_idx);
        gr_fillstyle_ref_add(&dst_serp->style.pdrop_fill);
        gr_fillstyle_ref_add(&dst_serp->style.point_fill);

        /* dup all point info from this series - implicit dup of refs to point pictures */
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_PDROP_FILLSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_PDROP_LINESTYLE));

        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_FILLSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_LINESTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_TEXTSTYLE));

        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_BARCHSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_BARLINECHSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_LINECHSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_PIECHDISPLSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_PIECHLABELSTYLE));
        status_accumulate(res, gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_SCATCHSTYLE));
    }

    dst_cp->bits.realloc_series = 1;

    assert(status_done(res));

    /* if anything failed blow the new copied non-core contents away */
    /* but only when we have reached a consistent state with no stolen list_blkrefs etc */
    if(status_fail(res))
        gr_chart_dispose_noncore(dst_cp);

    return(res);
}

static void
gr_chart_clone_noncore_pict_lose_refs(
    PC_GR_CHART_HANDLE chp)
{
    P_GR_CHART    cp;
    S32          plotidx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;

    cp = gr_chart_cp_from_ch(*chp);

    /* have to properly dup the picture refs when really used */
    gr_fillstyle_ref_lose(&cp->chart.areastyle);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyle_ref_lose(&cp->plotarea.area[plotidx].areastyle);

    gr_fillstyle_ref_lose(&cp->legend.areastyle);

    for(series_idx = 0; series_idx < cp->series.n_defined; ++series_idx)
    {
        /* lose refs to series pictures */
        serp = getserp(cp, series_idx);
        gr_fillstyle_ref_lose(&serp->style.pdrop_fill);
        gr_fillstyle_ref_lose(&serp->style.point_fill);

        /* lose refs to point pictures */
        gr_pdrop_list_fillstyle_reref(cp, series_idx, 0 /*lose*/);
        gr_point_list_fillstyle_reref(cp, series_idx, 0 /*lose*/);
    }
}

/* end of gr_chart.c */
