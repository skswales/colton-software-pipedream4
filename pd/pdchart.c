/* pdchart.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* PipeDream to charting module interface */

/* SKS 30-Apr-1991 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __cs_event_h
#include "cs-event.h"
#endif

#ifndef __cs_menu_h
#include "cs-menu.h"
#endif

#ifndef __akbd_h
#include "akbd.h"
#endif

#include "version_x.h"

/*
need to know about ev_dec_range()
*/

#include "cmodules/ev_evali.h"

/*
local header
*/

#include "pdchart.h"

/*
callback functions
*/

null_event_proto(static, pdchart_null_handler);

image_cache_tagstrip_proto(extern, pdchart_tagstrip);

image_cache_tagstrip_proto(extern, pdchart_tagstrip_legacy);

gr_chart_travel_proto(static, pdchart_travel_for_input);

gr_chart_travel_proto(static, pdchart_travel_for_text_input);

PROC_UREF_PROTO(static, pdchart_uref_handler);

PROC_UREF_PROTO(static, pdchart_text_uref_handler);

_Check_return_
static STATUS
ChartEdit_notify_proc(
    P_ANY handle,
    GR_CHARTEDIT_HANDLE ceh,
    GR_CHARTEDIT_NOTIFY_TYPE ntype,
    P_ANY nextra);

/*
internal functions
*/

_Check_return_
static STATUS
pdchart_add_into(
    P_PDCHART_HEADER pdchart,
    PDCHART_PROCESS process,
    P_PDCHART_DEP itdep,
    EV_COL stt,
    EV_COL end);

static P_PDCHART_ELEMENT
pdchart_element_add(
    P_PDCHART_HEADER pdchart);

_Check_return_
static STATUS
pdchart_element_ensure(
    P_PDCHART_HEADER pdchart,
    U32 n_ranges);

static void
pdchart_element_subtract(
    P_PDCHART_HEADER pdchart,
    P_PDCHART_ELEMENT pdchartelem,
    S32 kill_to_gr);

static void
pdchart_extdependency_dispose(
    P_PDCHART_DEP itdep /*inout*/);

_Check_return_
static STATUS
pdchart_extdependency_new(
    P_PDCHART_DEP itdep /*inout*/);

static void
pdchart_init_shape_suss_holes_end(
    PDCHART_SHAPEDESC * const p_chart_shapedesc /*inout*/);

static void
pdchart_listed_dep_dispose(
    _InoutRef_  P_P_PDCHART_DEP p_itdep);

_Check_return_
static STATUS
pdchart_listed_dep_new(
    _OutRef_    P_P_PDCHART_DEP p_itdep,
    _InVal_     PDCHART_RANGE_TYPE type);

_Check_return_
static STATUS
pdchart_modify(
    P_PDCHART_HEADER pdchart);

static void
pdchart_select_something(
    S32 iff_just_one);

static void
pdchart_submenu_kill(void);

/* try to allocate pdcharts in multiples of this */
#define PDCHART_ELEMENT_GRANULAR 8

/*
the list of charts
*/

static NLISTS_BLK
pdchart_listed_data =
{
    NULL,
    sizeof(struct PDCHART_LISTED_DATA),
    sizeof(struct PDCHART_LISTED_DATA) * 32
};

#define pdchart_listed_data_search_key(key) \
    collect_goto_item(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, (key))

/*
the list of chart dependencies
*/

static NLISTS_BLK
pdchart_listed_deps =
{
    NULL,
    sizeof(struct PDCHART_DEP),
    sizeof(struct PDCHART_DEP) * 16
};

#define pdchart_listed_dep_search_key(key) \
    collect_goto_item(PDCHART_DEP, &pdchart_listed_deps.lbr, (key))

/*
the 'current' chart. PD functions operate on this
*/

static P_PDCHART_HEADER
pdchart_current = NULL;

/******************************************************************************
*
* add the given shape of data to the 'current' chart
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_add(
    P_PDCHART_HEADER pdchart,
    const PDCHART_SHAPEDESC * const p_chart_shapedesc,
    S32 initial)
{
    S32               this_initial      = initial;
  /*S32               maybe_add_labels  = FALSE; never used */
    S32               has_unused_labels = FALSE;
    PDCHART_SHAPEDESC chart_shapedesc;
    P_P_LIST_BLOCK    p_p_list_block;
    LIST_ITEMNO       stt_key, end_key;
    U32               n_ranges;
    STATUS res, res1;

    n_ranges = p_chart_shapedesc->n_ranges;

    /* only consider adding labels if we haven't already done so to this chart */
    if(p_chart_shapedesc->bits.label_first_range)
    {
        if(!gr_chart_query_labels(&pdchart->ch))
        {
            /*maybe_add_labels = TRUE*/ /*EMPTY*/
        }
        else
        {
            /* this is one range we won't have to bother adding then.
             * if it's the only one then do nothing more
            */
            if(!--n_ranges)
                return(1);

            has_unused_labels = TRUE;
        }
    }

    /* cater for the PipeDream side of the chart descriptor growing */
    if((res = pdchart_element_ensure(pdchart, n_ranges)) < 0)
        return(res);

    /* derive new blocks from the passed block & nz lists */
    chart_shapedesc = *p_chart_shapedesc;

    p_p_list_block = de_const_cast(P_P_LIST_BLOCK,
        (p_chart_shapedesc->bits.range_over_columns) ? &p_chart_shapedesc->nz_cols.lbr : &p_chart_shapedesc->nz_rows.lbr);

    end_key = (p_chart_shapedesc->bits.range_over_columns) ? p_chart_shapedesc->stt.col : p_chart_shapedesc->stt.row;

    if(has_unused_labels)
    {
        /* turn them off and skip them */
        chart_shapedesc.bits.label_first_range = 0;
        ++end_key;
    }

#if 1
    /* SKS after PD 4.12 25mar92 - now sussing seems good, allow additions to use deduced series labelling */
    chart_shapedesc.bits.label_first_item = p_chart_shapedesc->bits.label_first_item;
#else
    {
    /* look at what's already allocated in this chart for a
     * hint about series label usage from all these dependencies
     * i.e. find the first valid non-category labels range of the same orientation as us!
    */
    P_PDCHART_ELEMENT ep = pdchart->elem.base;
    U32 ix = 0;

    while(ep[ix].bits.label_first_range  ||
          (ep[ix].type != (p->bits.range_over_columns ? PDCHART_RANGE_COL : PDCHART_RANGE_ROW)))
        if(++ix >= pdchart->elem.n)
            goto keep;

    assert(  offsetof(struct PDCHART_ELEMENT, rng.col.stt_row) ==   offsetof(struct PDCHART_ELEMENT, rng.row.stt_col));
    assert(sizeofmemb(struct PDCHART_ELEMENT, rng.col.stt_row) == sizeofmemb(struct PDCHART_ELEMENT, rng.row.stt_col));
    if(ep->rng.col.stt_row ==  (p->bits.range_over_columns ? p->stt.row : p->stt.col))
        chart_shapedesc.bits.label_first_item = ep->bits.label_first_item;

    keep:;
    }
#endif

    for(;;)
    {
        P_PDCHART_DEP   itdep;
        PDCHART_PROCESS process;
        P_U8            entryp;

        /* find a contiguous range of columns/rows to add as a block */
        stt_key = end_key;

        if((entryp = collect_first_from(U8, p_p_list_block, &stt_key)) == NULL)
            /* no more ranges */
            break;

        if(stt_key /*found*/ < end_key /*desired start*/)
        {
            /* stt_key was in a hole and found the previous item, so do a next */
            if((entryp = collect_next(U8, p_p_list_block, &stt_key)) == NULL)
                break;
        }

        end_key = stt_key;

        /* cells in this range, find the end */

        do  {
            ++end_key;

            entryp = collect_goto_item(U8, p_p_list_block, end_key);
        }
        while(entryp);

        /* stt_key and end_key are new incl,excl range so bodge the dependency in a bit */

        /* create a new dependency block (at end of list) for all sub-ranges to refer to */
        res = pdchart_listed_dep_new(&itdep, (p_chart_shapedesc->bits.range_over_columns ? PDCHART_RANGE_COL : PDCHART_RANGE_ROW));

        status_return(res);

        /* need to be able to key back to chart */
        itdep->pdchartdatakey = pdchart->pdchartdatakey;

        itdep->bits.label_first_range = chart_shapedesc.bits.label_first_range;
        itdep->bits.label_first_item  = chart_shapedesc.bits.label_first_item;

        if(DOCNO_NONE != (itdep->rng.s.docno = chart_shapedesc.docno))
        {
            /* store whole range in there */
            itdep->rng.s.col = p_chart_shapedesc->bits.range_over_columns ? (EV_COL) stt_key : (EV_COL) chart_shapedesc.stt.col;
            itdep->rng.s.row = p_chart_shapedesc->bits.range_over_columns ? (EV_ROW) chart_shapedesc.stt.row : (EV_ROW) stt_key;
            itdep->rng.e.col = p_chart_shapedesc->bits.range_over_columns ? (EV_COL) end_key : (EV_COL) chart_shapedesc.end.col;
            itdep->rng.e.row = p_chart_shapedesc->bits.range_over_columns ? (EV_ROW) chart_shapedesc.end.row : (EV_ROW) end_key;

            /* add an external reference to this whole pdchart range */
            res = pdchart_extdependency_new(itdep);
        }
        else
            res = create_error(EVAL_ERR_CANTEXTREF);

        if(status_done(res))
        {
            zero_struct(process);
            process.initial   = this_initial;
            process.force_add = 1; /* all are adds from PD commands */

            res = pdchart_add_into(pdchart, process, itdep, (EV_COL) stt_key, (EV_COL) end_key);
        }

        this_initial = 0;     /* only the first block of the first add is initial */
      /*maybe_add_labels = 0;*/ /* only the first block of any add is searched for labels */

        chart_shapedesc.bits.label_first_range = 0;

        if(res <= 0)
        {
            pdchart_extdependency_dispose(itdep);

            pdchart_listed_dep_dispose(&itdep);

            /* I simply can't be bothered to roll back the previous changes from earlier times round this loop ... */
            break;
        }
    }

    res1 = pdchart_modify(pdchart);

    return(status_done(res) ? res : res1);
}

_Check_return_
static STATUS
pdchart_add_into(
    P_PDCHART_HEADER pdchart,
    PDCHART_PROCESS process,
    P_PDCHART_DEP itdep,
    EV_COL stt,
    EV_COL end)
{
    P_PDCHART_ELEMENT i_pdchartelem, pdchartelem;
    P_PDCHART_ELEMENT ep;
    BOOL range_over_columns = (itdep->type == PDCHART_RANGE_COL);
    EV_COL cur;
    STATUS res = 1;

    cur = stt;

    do  {
        pdchartelem = pdchart_element_add(pdchart);

        pdchartelem->type     = itdep->type;
        pdchartelem->itdepkey = itdep->itdepkey;

        pdchartelem->rng.col.docno = itdep->rng.s.docno; /* EV_DOCNO */
        pdchartelem->rng.col.col = cur;

        pdchartelem->rng.col.stt_row = (range_over_columns) ? itdep->rng.s.row : itdep->rng.s.col;
        pdchartelem->rng.col.end_row = (range_over_columns) ? itdep->rng.e.row : itdep->rng.e.col;

        /* category ranges only at start of dependency! */
        pdchartelem->bits.label_first_range = itdep->bits.label_first_range &&
                                                (cur == (range_over_columns ? itdep->rng.s.col : itdep->rng.s.row));

        pdchartelem->bits.label_first_item = itdep->bits.label_first_item;

        if(pdchartelem->bits.label_first_range)
        {
            res = gr_chart_add_labels(&pdchart->ch,
                                      pdchart_travel_for_input,
                                      pdchartelem,
                                      &pdchartelem->gr_int_handle);
        }
        else
        {
            P_PDCHART_ELEMENT epf;

            /* look for handle to insert after */
            epf = NULL;

            if(!process.force_add)
            {
                EV_COL try_this = cur - 1;

                if(0 != pdchart->elem.n)
                {
                    i_pdchartelem = &pdchart->elem.base[0];
                    ep = i_pdchartelem + pdchart->elem.n;

                    while(--ep >= i_pdchartelem)
                    {
                        if(ep->itdepkey == itdep->itdepkey)
                        {
                            switch(ep->type)
                            {
                            default:
                                myassert2x(0, "Found chart element of different type %d to dep type %d", ep->type, itdep->type);

                            case PDCHART_RANGE_NONE:
                                break;

                            case PDCHART_RANGE_COL_DEAD:
                                if(ep->rng.col.col == try_this)
                                    --try_this;
                                break;

                            case PDCHART_RANGE_ROW_DEAD:
                                if(ep->rng.row.row == try_this)
                                    --try_this;
                                break;

                            case PDCHART_RANGE_COL:
                                if(ep->rng.col.col == try_this)
                                {
                                    epf = ep;
                                    goto endwhile_add_after;
                                }
                                break;

                            case PDCHART_RANGE_ROW:
                                if(ep->rng.row.row == try_this)
                                {
                                    epf = ep;
                                    goto endwhile_add_after;
                                }
                                break;
                            }
                        }
                    }
                }
                endwhile_add_after:;

                myassert0x((epf != NULL) || process.initial, "Failed to find element to insert after");
            }

            if(epf)
                res = gr_chart_insert(&pdchart->ch,
                                      pdchart_travel_for_input,
                                      pdchartelem,
                                      &pdchartelem->gr_int_handle,
                                      &epf->gr_int_handle);
            else
                res = gr_chart_add(&pdchart->ch,
                                   pdchart_travel_for_input,
                                   pdchartelem,
                                   &pdchartelem->gr_int_handle);

            if(res < 0)
                /* failed to add, so remove allocation */
                pdchart_element_subtract(pdchart, pdchartelem, 0);
        }

        if(res < 0)
        {
            /* failed to add this col/row as range, so remove and loop back removing others we just added */

            i_pdchartelem = &pdchart->elem.base[0];

            while(--cur >= stt)
            {
                pdchartelem = i_pdchartelem + pdchart->elem.n;

                while(--pdchartelem >= i_pdchartelem)
                    if(pdchartelem->itdepkey == itdep->itdepkey)
                        if(cur == (range_over_columns ? pdchartelem->rng.col.col : pdchartelem->rng.row.row))
                        {
                            pdchart_element_subtract(pdchart, pdchartelem, 1);
                            break;
                        }
            }

            break;
        }
    }
    while(++cur < end);

    return(res);
}

_Check_return_
static STATUS
pdchart_add_range_for_load(
    P_PDCHART_HEADER pdchart,
    PDCHART_RANGE_TYPE type,
    _InRef_     PC_EV_RANGE rng,
    U16 bits)
{
    P_PDCHART_DEP   itdep;
    PDCHART_PROCESS process;
    STATUS res;

    /* create a new dependency block (at end of list) for all sub-ranges to refer to */
    if((res = pdchart_listed_dep_new(&itdep, type)) < 0)
        return(res);

    /* need to be able to key back to chart */
    itdep->pdchartdatakey = pdchart->pdchartdatakey;

    * (P_U16) &itdep->bits = bits;

    itdep->rng  = *rng;

    /* add an external reference to this whole pdchart range */
    res = pdchart_extdependency_new(itdep);

    if(status_done(res))
    {
        zero_struct(process);
        process.initial   = 1;
        process.force_add = 1; /* all are adds from reload */

        res = pdchart_add_into(pdchart, process, itdep,
                               ((type == PDCHART_RANGE_COL) ? itdep->rng.s.col : itdep->rng.s.row),
                               ((type == PDCHART_RANGE_COL) ? itdep->rng.e.col : itdep->rng.e.row));

        if(status_fail(res))
            pdchart_extdependency_dispose(itdep);
    }

    if(res <= 0)
    {
        pdchart_listed_dep_dispose(&itdep);

        return(res);
    }

    return(1);
}

/******************************************************************************
*
* allocate offset for a new element
*
******************************************************************************/

static P_PDCHART_ELEMENT
pdchart_element_add(
    P_PDCHART_HEADER pdchart)
{
    P_PDCHART_ELEMENT stt_pdchartelem, end_pdchartelem, pdchartelem;

    /* try to find a hole if present */
    stt_pdchartelem = &pdchart->elem.base[0];
    end_pdchartelem = stt_pdchartelem + pdchart->elem.n;

    if(0 != pdchart->elem.n)
    {
        pdchartelem = end_pdchartelem;

        while(--pdchartelem >= stt_pdchartelem)
            if(pdchartelem->type == PDCHART_RANGE_NONE)
                return(pdchartelem);
    }

    ++pdchart->elem.n;

    /* allocate at end unless holey */
    return(end_pdchartelem);
}

/******************************************************************************
*
* dispose of a chart
*
******************************************************************************/

static void
pdchart_dispose(
    P_P_PDCHART_HEADER p_pdchart /*inout*/)
{
    P_PDCHART_HEADER  pdchart;
    P_PDCHART_ELEMENT i_pdchartelem, pdchartelem;
    P_PDCHART_DEP     itdep;

    pdchart = *p_pdchart;
    if(!pdchart)
        return;
    *p_pdchart = NULL;

    pdchart_submenu_kill();

    /* kill the chart editor window if any */
    gr_chartedit_dispose(&pdchart->ceh);

    /* loop over data sources, removing deps */
    if(pdchart->elem.base)
    {
        if(0 != pdchart->elem.n)
        {
            i_pdchartelem = &pdchart->elem.base[0];
            pdchartelem = i_pdchartelem + pdchart->elem.n;

            while(--pdchartelem >= i_pdchartelem)
            {
                /* remove extref and dep for this range if not already done */
                if((itdep = pdchart_listed_dep_search_key(pdchartelem->itdepkey)) != NULL)
                {
                    pdchart_extdependency_dispose(itdep);

                    pdchart_listed_dep_dispose(&itdep);
                }

                /* being pedantic one could remove data sources individually ... */
                #if TRACE_ALLOWED
                pdchart_element_subtract(pdchart, pdchartelem, 1);
                #endif
                /* ... but it's far quicker to leave that to the chart module */
            }
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(&pdchart->elem.base));
    }

    /* remove any cache entry that may have been this chart's Draw file */
    {
    U8 filename[BUF_MAX_PATHSTRING];
    IMAGE_CACHE_HANDLE cah;

    gr_chart_name_query(&pdchart->ch, filename, sizeof32(filename) - 1);

    if(image_cache_entry_query(&cah, filename))
    {
        image_cache_ref(&cah, 1);
        image_cache_ref(&cah, 0); /* autokill will remove this entry from the cache if refs go to zero, else it's referenced elsewhere */
    }
    } /*block*/

    /* can kill the chart and its baggage in one (quick) fell swoop */
    gr_chart_dispose(&pdchart->ch);

    /* kill the pdchart list entry */
    collect_subtract_entry(&pdchart_listed_data.lbr, pdchart->pdchartdatakey);

    /* were we 'current'? find another gullible chart if so */
    if(pdchart_current == pdchart)
        pdchart_select_something(0 /*any will do*/);

    /* some twerp may have set off a background process to kill this chart so bash that in */
    /* or else you can imagine the consequences as it accesses ... */
    if(pdchart->recalc.state != PDCHART_UNMODIFIED)
        Null_EventHandlerRemove(pdchart_null_handler, pdchart);

    al_ptr_dispose(P_P_ANY_PEDANTIC(&pdchart));
}

/******************************************************************************
*
* kill external dependency for chart
*
******************************************************************************/

static void
pdchart_extdependency_dispose(
    P_PDCHART_DEP itdep /*inout*/)
{
    EV_DOCNO docno;

    if((docno = itdep->rng.s.docno) != DOCNO_NONE)
    {
        itdep->rng.s.docno = DOCNO_NONE;

        ev_del_extdependency(docno, itdep->ev_int_handle);
    }
}

/******************************************************************************
*
* add external dependency for chart
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_extdependency_new(
    P_PDCHART_DEP itdep /*inout*/)
{
    STATUS res;

    /* little fixups for cleaner results */
    itdep->rng.e.docno = itdep->rng.s.docno;

    itdep->rng.s.flags = itdep->rng.e.flags = 0;

    res = ev_add_extdependency(itdep->itdepkey,
                               0,
                               &itdep->ev_int_handle,
                               ((itdep->type == PDCHART_RANGE_TXT) ? pdchart_text_uref_handler : pdchart_uref_handler),
                               &itdep->rng);

    if(status_fail(res))
    {
        itdep->rng.s.docno = DOCNO_NONE;
        return(res);
    }

    /* NB. res == 0 is good from ev_add_extdependency() */
    return(1);
}

/******************************************************************************
*
* try to get a numeric value out of this!
* 0 -> failed, expand as string in caller
*
******************************************************************************/

_Check_return_
static BOOL
pdchart_extract_numeric_result(
    _InRef_     PC_EV_RESULT p_ev_result,
    _OutRef_opt_ P_GR_CHART_VALUE val /*out. NULL->nopoke*/)
{
    switch(p_ev_result->data_id)
    {
    case DATA_ID_REAL:
        if(val)
        {
            val->type        = GR_CHART_VALUE_NUMBER;
            val->data.number = p_ev_result->arg.fp;
        }
        return(1);

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        if(val)
        {
            val->type        = GR_CHART_VALUE_NUMBER;
            val->data.number = (F64) p_ev_result->arg.integer;
        }
        return(1);

    case RPN_RES_ARRAY:
        {
        BOOL res;
        EV_RESULT temp_ev_result;
        SS_DATA temp_data;

        /* go via SS_DATA */
        ev_result_to_data_convert(&temp_data, p_ev_result);

        ss_data_to_result_convert(&temp_ev_result, ss_array_element_index_borrow(&temp_data, 0, 0));

        /* recurse to convert result */
        res = pdchart_extract_numeric_result(&temp_ev_result, val);

        ev_result_free_resources(&temp_ev_result);

        return(res);
        }

    default:
        return(0);
    }
}

/******************************************************************************
*
* grow pdchart descriptor as necessary to take more elements
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_element_ensure(
    P_PDCHART_HEADER pdchart,
    U32 n_ranges)
{
    P_PDCHART_ELEMENT i_pdchartelem, pdchartelem;
    U32               n_ranges_total;

    /* nuffink to allocate */
    if(!n_ranges)
        return(1);

    /* count number of holes present. don't reuse dead entries - they're there for a purpose! */
    if(0 != pdchart->elem.n)
    {
        i_pdchartelem = &pdchart->elem.base[0];
        pdchartelem = i_pdchartelem + pdchart->elem.n;

        while(--pdchartelem >= i_pdchartelem)
        {
            if(pdchartelem->type == PDCHART_RANGE_NONE)
                if(n_ranges > 0)
                    if(--n_ranges == 0)
                        /* holes can take the load */
                        return(1);
        }
    }

    /* cater for the PipeDream side of the chart descriptor growing */
    n_ranges_total = pdchart->elem.n + n_ranges;

    if(n_ranges_total > pdchart->elem.n_alloc)
    {
        P_PDCHART_ELEMENT t_base;
        U32 delta;
        STATUS status;

        /* round up to next multiple of n elements */
        n_ranges_total = round_up(n_ranges_total, PDCHART_ELEMENT_GRANULAR);

        /* number of new elements being allocated */
        delta = n_ranges_total - pdchart->elem.n_alloc;

        if(NULL == (t_base = al_ptr_realloc_elem(PDCHART_ELEMENT, pdchart->elem.base, n_ranges_total, &status)))
            return(status);

        pdchart->elem.base = t_base;

        /* clear out new element allocation */
        memset32(&pdchart->elem.base[pdchart->elem.n_alloc], 0, delta * sizeof32(*pdchartelem));

        /* then update our note of size */
        pdchart->elem.n_alloc = n_ranges_total;

        /* update references to the currently allocated chart elements (stored for us by gr_chart.c) */

        if(0 != pdchart->elem.n)
        {
            i_pdchartelem = &pdchart->elem.base[0];
            pdchartelem = i_pdchartelem + pdchart->elem.n;

            while(--pdchartelem >= i_pdchartelem)
                if(pdchartelem->type != PDCHART_RANGE_NONE)
                    gr_chart_update_handle(&pdchart->ch, pdchartelem, &pdchartelem->gr_int_handle);
        }
    }

    return(1);
}

/******************************************************************************
*
* remove element
*
******************************************************************************/

static void
pdchart_element_subtract(
    P_PDCHART_HEADER pdchart,
    P_PDCHART_ELEMENT pdchartelem,
    S32 kill_to_gr)
{
    assert(pdchartelem->type != PDCHART_RANGE_NONE);
    assert(pdchart->elem.n);

    if(kill_to_gr && pdchartelem->gr_int_handle) /* DEAD ROWs and COLs will have NULL handles */
        gr_chart_subtract(&pdchart->ch, &pdchartelem->gr_int_handle);

    zero_struct_ptr(pdchartelem);

    /* try shrinking descriptor usage after that clearout */
    pdchartelem = &pdchart->elem.base[pdchart->elem.n];

    while(pdchart->elem.n)
    {
        if((--pdchartelem)->type != PDCHART_RANGE_NONE)
            break;

        /* removing at end: shrink end back as far as possible */
        --pdchart->elem.n;
    }
}

/******************************************************************************
*
* set up chart shape from marked block in sheet
*
******************************************************************************/

static STATUS
pdchart_init_shape_suss_holes(
    PDCHART_SHAPEDESC * const p_chart_shapedesc)
{
    SLR end;
    TRAVERSE_BLOCK traverse_blk;
    P_CELL sl;
    STATUS status = STATUS_OK;

    p_chart_shapedesc->min.col = LARGEST_COL_POSSIBLE;
    p_chart_shapedesc->min.row = LARGEST_ROW_POSSIBLE;
    p_chart_shapedesc->max.col = 0;
    p_chart_shapedesc->max.row = 0;

    p_chart_shapedesc->nz_cols.lbr = NULL;
    p_chart_shapedesc->nz_cols.maxitemsize = 1;
    p_chart_shapedesc->nz_cols.maxpoolsize = 16;

    p_chart_shapedesc->nz_rows.lbr = NULL;
    p_chart_shapedesc->nz_rows.maxitemsize = 1;
    p_chart_shapedesc->nz_rows.maxpoolsize = 16;

    end.col = p_chart_shapedesc->end.col - 1; /* make these incl again for PipeDream block routines */
    end.row = p_chart_shapedesc->end.row - 1;

    traverse_block_init(&traverse_blk, p_chart_shapedesc->docno, &p_chart_shapedesc->stt, &end, TRAVERSE_DOWN_COLUMNS);

    while((sl = traverse_block_next_slot(&traverse_blk)) != NULL)
    {
        /* if there's a cell, mark the col & row it's in */
        const COL col = traverse_block_cur_col(&traverse_blk);
        const ROW row = traverse_block_cur_row(&traverse_blk);
        const S32 is_number_cell = (sl->type == SL_NUMBER);
        P_NLISTS_BLK nlbrp;
        LIST_ITEMNO key;
        P_U8 entryp;

        if(sl->type == SL_PAGE)
            continue;

        p_chart_shapedesc->min.col = MIN(p_chart_shapedesc->min.col, col);
        p_chart_shapedesc->min.row = MIN(p_chart_shapedesc->min.row, row);
        p_chart_shapedesc->max.col = MAX(p_chart_shapedesc->max.col, col);
        p_chart_shapedesc->max.row = MAX(p_chart_shapedesc->max.row, row);

        /* this column is not blank */
        nlbrp = &p_chart_shapedesc->nz_cols;
        key = (LIST_ITEMNO) col;

        if((entryp = collect_goto_item(U8, &nlbrp->lbr, key)) == NULL)
        {
            if(NULL == (entryp = collect_add_entry_elem(U8, nlbrp, &key, &status)))
                break;
            *entryp = is_number_cell;
        }
        else if(!*entryp)
            *entryp = is_number_cell; /* numbers in cells force their way in */

        /* this row is not blank */
        nlbrp = &p_chart_shapedesc->nz_rows;
        key = (LIST_ITEMNO) row;

        if((entryp = collect_goto_item(U8, &nlbrp->lbr, key)) == NULL)
        {
            if(NULL == (entryp = collect_add_entry_elem(U8, nlbrp, &key, &status)))
                break;
            *entryp = is_number_cell;
        }
        else if(!*entryp)
            *entryp = is_number_cell;
    }

    p_chart_shapedesc->bits.something = 1;

    if(status < 0)
    {
        p_chart_shapedesc->bits.something = 0;

        pdchart_init_shape_suss_holes_end(p_chart_shapedesc);
    }
    else
    {
        /* adjust the end points */
        if(p_chart_shapedesc->min.col < p_chart_shapedesc->max.col + 1)
        {
            p_chart_shapedesc->stt.col = p_chart_shapedesc->min.col;
            p_chart_shapedesc->end.col = p_chart_shapedesc->max.col + 1; /*excl*/
        }
        else
            p_chart_shapedesc->bits.something = 0;

        if(p_chart_shapedesc->min.row < p_chart_shapedesc->max.row + 1)
        {
            p_chart_shapedesc->stt.row = p_chart_shapedesc->min.row;
            p_chart_shapedesc->end.row = p_chart_shapedesc->max.row + 1; /*excl*/
        }
        else
            p_chart_shapedesc->bits.something = 0;
    }

    return(status);
}

static void
pdchart_init_shape_suss_holes_end(
    PDCHART_SHAPEDESC * const p_chart_shapedesc /*inout*/)
{
    collect_delete(&p_chart_shapedesc->nz_cols.lbr);
    collect_delete(&p_chart_shapedesc->nz_rows.lbr);
}

_Check_return_
static STATUS
pdchart_init_shape_from_marked_block(
    PDCHART_SHAPEDESC * const p_chart_shapedesc /*out*/,
    P_PDCHART_HEADER pdchart /*const, maybe NULL*/)
{
    P_CELL       sl;
    P_EV_RESULT  p_ev_result;
    STATUS res;
    COL          cols_tx_n;
    ROW          rows_tx_n;
    LIST_ITEMNO  key;
    P_P_LIST_BLOCK p_p_list_block;
    P_U8         entryp;

    init_marked_block();

    p_chart_shapedesc->docno = blk_docno;

    ++end_bl.col; /* convert end from incl to excl */
    ++end_bl.row;

    /* SKS after PD 4.12 24mar92 - made forced recalc so that yet-to-be-recalced numbers appear as numbers not text */
    ev_recalc_all();

    #if TRACE_ALLOWED
    * (int *) &p_chart_shapedesc->bits = 0;
    #endif

    /* read the options */
    p_chart_shapedesc->bits.range_over_manual  = (d_chart_options[0].option != 'A');
    p_chart_shapedesc->bits.range_over_columns = (d_chart_options[0].option == 'C');

    /* SKS after PD 4.12 24mar92 - merged from_marked_block and from_range as one was only ever called as the tail of t'other */

    p_chart_shapedesc->stt.col = start_bl.col;
    p_chart_shapedesc->stt.row = start_bl.row;
    p_chart_shapedesc->end.col = end_bl.col; /*excl*/
    p_chart_shapedesc->end.row = end_bl.row; /*excl*/

    status_return(res = pdchart_init_shape_suss_holes(p_chart_shapedesc));

    p_chart_shapedesc->n.col = p_chart_shapedesc->end.col - p_chart_shapedesc->stt.col;
    p_chart_shapedesc->n.row = p_chart_shapedesc->end.row - p_chart_shapedesc->stt.row;

    p_chart_shapedesc->nz_n.col = 0;
    p_chart_shapedesc->nz_n.row = 0;

    /* count the number of cols actually used */
    p_p_list_block = &p_chart_shapedesc->nz_cols.lbr;
    cols_tx_n = 0;

    for(entryp = collect_first(U8, p_p_list_block, &key);
        entryp;
        entryp = collect_next( U8, p_p_list_block, &key))
    {
        ++p_chart_shapedesc->nz_n.col;

        if(!*entryp)
            ++cols_tx_n;
    }

    /* count the number of rows actually used */
    p_p_list_block = &p_chart_shapedesc->nz_rows.lbr;
    rows_tx_n = 0;

    for(entryp = collect_first(U8, p_p_list_block, &key);
        entryp;
        entryp = collect_next( U8, p_p_list_block, &key))
    {
        ++p_chart_shapedesc->nz_n.row;

        if(!*entryp)
            ++rows_tx_n;
    }

    p_chart_shapedesc->bits.number_top_left = 0;
    p_chart_shapedesc->bits.label_top_left  = 0;
    p_chart_shapedesc->bits.number_left_col = 0;
    p_chart_shapedesc->bits.label_left_col  = 0;
    p_chart_shapedesc->bits.number_top_row  = 0;
    p_chart_shapedesc->bits.label_top_row   = 0;

    /* what is at the top left? */
    sl = travel_externally(p_chart_shapedesc->docno, p_chart_shapedesc->stt.col, p_chart_shapedesc->stt.row);

    if(sl)
    {
        switch(result_extract(sl, &p_ev_result))
        {
        case SL_PAGE:
            break;

        case SL_NUMBER:
            if(pdchart_extract_numeric_result(p_ev_result, NULL))
            {
                p_chart_shapedesc->bits.number_top_left = 1;
                break;
            }

        /* else deliberate drop thru ... */

        default:
            p_chart_shapedesc->bits.label_top_left = 1;
            break;
        }
    }

    { /* see whether left column is a label set. can skip top left as that's been covered */
    ROW row = p_chart_shapedesc->stt.row + 1;

    if(row < p_chart_shapedesc->end.row)
    {
        do {
            sl = travel_externally(p_chart_shapedesc->docno, p_chart_shapedesc->stt.col, row);

            if(sl)
                switch(result_extract(sl, &p_ev_result))
                {
                case SL_PAGE:
                    break;

                case SL_NUMBER:
                    if(pdchart_extract_numeric_result(p_ev_result, NULL))
                    {
                        p_chart_shapedesc->bits.number_left_col = 1;
                        break;
                    }

                /* else deliberate drop thru ... */

                default:
                    p_chart_shapedesc->bits.label_left_col = 1;
                    break;
                }
        }
        while(++row < p_chart_shapedesc->end.row); /* in ideal case one would be able to zip down a sparse list... */
    }
    } /*block*/

    { /* see whether top row is a label set. can skip top left as that's been covered */
    COL col = p_chart_shapedesc->stt.col + 1;

    if(col < p_chart_shapedesc->end.col)
    {
        do  {
            sl = travel_externally(p_chart_shapedesc->docno, col, p_chart_shapedesc->stt.row);

            if(sl)
                switch(result_extract(sl, &p_ev_result))
                {
                case SL_PAGE:
                    break;

                case SL_NUMBER:
                    if(pdchart_extract_numeric_result(p_ev_result, NULL))
                    {
                        p_chart_shapedesc->bits.number_top_row = 1;
                        break;
                    }

                /* else deliberate drop thru ... */

                default:
                    p_chart_shapedesc->bits.label_top_row = 1;
                    break;
                }
        }
        while(++col < p_chart_shapedesc->end.col); /* in ideal case one would be able to zip down a sparse list... (but not in PipeDream) */
    }
    } /*block*/

    /* SKS after PD 4.12 24mar92 -  more care needed with top left corner for predictability */
    if(p_chart_shapedesc->bits.label_top_left)
    {
        if(p_chart_shapedesc->bits.range_over_manual && !p_chart_shapedesc->bits.range_over_columns)
        {
            /* if we are definitely arranging across rows then do opposite to the below case */
            if(!p_chart_shapedesc->bits.label_left_col)
            {
                if((p_chart_shapedesc->nz_n.row > 1) && p_chart_shapedesc->bits.number_left_col)
                    p_chart_shapedesc->bits.label_top_row  = 1;
                else
                    p_chart_shapedesc->bits.label_left_col = 1;
            }
        }
        else
        {
            /* give the label at top left to the top row only if it already has labels,
             * otherwise give to the left column if more than one column and otherwise empty top row
             * (else adding a column with a series label is impossible)
             * this makes adding to chart with series headings work as before (sort of)
            */
            if(!p_chart_shapedesc->bits.label_top_row)
            {
                if((p_chart_shapedesc->nz_n.col > 1) && p_chart_shapedesc->bits.number_top_row)
                    p_chart_shapedesc->bits.label_left_col = 1;
                else
                    p_chart_shapedesc->bits.label_top_row  = 1;
            }
        }
    }

    /* can sort bits out now (and soon probably remove as irrelevant) */
    if(p_chart_shapedesc->bits.label_top_left)
        p_chart_shapedesc->bits.number_top_left = 0;

    if(p_chart_shapedesc->bits.label_left_col)
        p_chart_shapedesc->bits.number_left_col = 0;

    if(p_chart_shapedesc->bits.label_top_row)
        p_chart_shapedesc->bits.number_top_row = 0;

    if(!p_chart_shapedesc->bits.range_over_manual)
    {
        /* SKS after PD 4.12 24mar92 - change test to be more careful and predictable */
        S32 datacols = p_chart_shapedesc->nz_n.col;
        S32 datarows = p_chart_shapedesc->nz_n.row;
        if(p_chart_shapedesc->bits.label_left_col)
            --datacols;
        if(p_chart_shapedesc->bits.label_top_row)
            --datarows;
        if(datacols != datarows)
            p_chart_shapedesc->bits.range_over_columns = (datacols < datarows);
        else if(p_chart_shapedesc->bits.label_top_row)
            p_chart_shapedesc->bits.range_over_columns = TRUE;
        else if(p_chart_shapedesc->bits.label_left_col)
            p_chart_shapedesc->bits.range_over_columns = FALSE;
        else
            p_chart_shapedesc->bits.range_over_columns = TRUE;
    }

    p_chart_shapedesc->bits.label_first_range = (p_chart_shapedesc->bits.range_over_columns)
                                                    ? p_chart_shapedesc->bits.label_left_col
                                                    : p_chart_shapedesc->bits.label_top_row;

    p_chart_shapedesc->bits.label_first_item  = (p_chart_shapedesc->bits.range_over_columns)
                                                    ? p_chart_shapedesc->bits.label_top_row
                                                    : p_chart_shapedesc->bits.label_left_col;

    p_p_list_block = (p_chart_shapedesc->bits.range_over_columns)
                        ? &p_chart_shapedesc->nz_cols.lbr
                        : &p_chart_shapedesc->nz_rows.lbr;

    if(p_chart_shapedesc->bits.label_first_range)
    {
        /* protect range from deletion */
        if(p_chart_shapedesc->bits.range_over_columns)
        {
            --cols_tx_n;
            key = p_chart_shapedesc->stt.col;
        }
        else
        {
            --rows_tx_n;
            key = p_chart_shapedesc->stt.row;
        }

        entryp = collect_goto_item(U8, p_p_list_block, key);
        assert(entryp);
        *entryp = 1; /* protect */
    }

    /* remove superfluous non-numeric ranges now */
    p_chart_shapedesc->nz_n.col -= cols_tx_n;
    p_chart_shapedesc->nz_n.row -= rows_tx_n;

    p_chart_shapedesc->n_ranges = (p_chart_shapedesc->bits.range_over_columns)
                                    ? p_chart_shapedesc->nz_n.col
                                    : p_chart_shapedesc->nz_n.row;

    key = (p_chart_shapedesc->bits.range_over_columns)
                    ? p_chart_shapedesc->stt.col
                    : p_chart_shapedesc->stt.row;

    for(entryp = collect_first_from(U8, p_p_list_block, &key);
        entryp;
        entryp = collect_next(U8, p_p_list_block, &key))
    {
        if(!*entryp)
            collect_subtract_entry(p_p_list_block, key);
    }

    /* SKS after PD 4.12 24mar92 - see whether there is any data left after that! */
    if(p_chart_shapedesc->n_ranges == 0)
        return(create_error(ERR_CHARTNONUMERICDATA));

    if((p_chart_shapedesc->n_ranges == 1) && p_chart_shapedesc->bits.label_first_range)
    {
        /* go interactive */
        switch(riscdialog_query_YN("There is no numeric data in the marked block.", "Do you want to continue?"))
        {
        case riscdialog_query_YES:
            /* punter has given us the go-ahead */
            break;

        default: default_unhandled();
#if CHECKING
        case riscdialog_query_NO:
        case riscdialog_query_CANCEL:
#endif
            /* abandon operation */
            pdchart_init_shape_suss_holes_end(p_chart_shapedesc);
            return(0);
        }
    }

    /* if no category labels found and the chart would prefer to
     * have them then ask the punter whether he knows better

     * SKS after PD 4.12 24mar92 - rather different to what has gone before...
    */
    if(0 && !p_chart_shapedesc->bits.label_first_range)
    {
        /* if adding to a chart see if already added else ask preferred chart whether it wants them */
        S32 wants_labels = 0;

        if(NULL == pdchart)
        {
            wants_labels = gr_chart_query_labelling(NULL);
        }
        else
        {
            if(gr_chart_query_labelling(&pdchart->ch))
                wants_labels = !gr_chart_query_labels(&pdchart->ch);
        }

        if(wants_labels)
        {
            /* go interactive */
            char statement_buffer[128];

            consume_int(xsnprintf(statement_buffer, elemof32(statement_buffer),
                                  "Use %snumeric %s as category labels?",
                                  (p_chart_shapedesc->n_ranges > 1)            ? "first " : "",
                                  (p_chart_shapedesc->bits.range_over_columns) ? "column" : "row"));

            switch(riscdialog_query_YN(statement_buffer, " ")) /* NB the question cannot be NULL */
            {
            case riscdialog_query_YES:
                p_chart_shapedesc->bits.label_first_range = 1;
                break;

            case riscdialog_query_CANCEL:
                /* abandon operation */
                pdchart_init_shape_suss_holes_end(p_chart_shapedesc);
                return(0);

            default: default_unhandled();
            case riscdialog_query_NO:
                break;
            }
        }
    }

    return(1);
}

/******************************************************************************
*
* deallocate a dependency block
*
******************************************************************************/

static void
pdchart_listed_dep_dispose(
    _InoutRef_  P_P_PDCHART_DEP p_itdep)
{
    P_PDCHART_DEP itdep = *p_itdep;
    LIST_ITEMNO itdepkey = itdep->itdepkey;

    *p_itdep = NULL;

    collect_subtract_entry(&pdchart_listed_deps.lbr, itdepkey);
}

/******************************************************************************
*
* allocate a new dependency block
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_listed_dep_new(
    _OutRef_    P_P_PDCHART_DEP p_itdep,
    _InVal_     PDCHART_RANGE_TYPE type)
{
    static LIST_ITEMNO itdepkey_gen = 0x42224000; /* NB. not tbs */

    LIST_ITEMNO itdepkey;
    P_PDCHART_DEP itdep;
    STATUS status;

    /* create a new block for all sub-ranges to refer to */
    itdepkey = itdepkey_gen++;

    if(NULL == (*p_itdep = itdep = collect_add_entry_elem(PDCHART_DEP, &pdchart_listed_deps, &itdepkey, &status)))
        return(status);

    zero_struct_ptr(itdep);

    itdep->type = type;

    /* need to be able to key back to this entry */
    itdep->itdepkey = itdepkey;

    return(1);
}

/******************************************************************************
*
* prepare for modifying a chart sometime when we get idle time
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_modify(
    P_PDCHART_HEADER pdchart)
{
    STATUS status;

#if 0
    pdchart->recalc.last_mod_time = monotime();
#endif

    /* mark chart for recalc if not already done so */
    if(pdchart->recalc.state == PDCHART_UNMODIFIED)
    {
        pdchart->recalc.state = PDCHART_MODIFIED;

        /* and claim some nulls, monitoring till we seem to have
         * recalced areas of interest in this chart
        */
        if(status_fail(status = Null_EventHandlerAdd(pdchart_null_handler, pdchart, 0)))
        {
            /* fail pathetically */
            pdchart->recalc.state = PDCHART_UNMODIFIED;
            return(status);
        }
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* prepare a new chart for adding to, suggested size given so no realloc initially
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_new(
    P_P_PDCHART_HEADER p_pdchart /*out*/,
    U32 n_ranges,
    S32 use_preferred,
    S32 new_untitled)
{
    static LIST_ITEMNO    pdchartdatakey_gen = 0x52834000; /* NB. starting at a non-zero position, not tbs! */

    P_PDCHART_HEADER      pdchart;
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;
    S32                   res;

    /* allocate header */
    if(NULL == (pdchart = *p_pdchart = al_ptr_calloc_elem(PDCHART_HEADER, 1, &res)))
        return(res);

    pdchart_submenu_kill();

    /* clear out allocation */
    zero_struct_ptr(pdchart);

    pdchart->recalc.state = PDCHART_UNMODIFIED;

    if(status_done(res = pdchart_element_ensure(pdchart, n_ranges /*may be 0*/)))
    {
        /* subsequent failure irrelevant to monotonic handle generator */
        pdchartdatakey = pdchartdatakey_gen++;

        if(NULL != (pdchartdata = collect_add_entry_elem(PDCHART_LISTED_DATA, &pdchart_listed_data, &pdchartdatakey, &res)))
        {
            /* merely store pointer to pdchart header on list */
            pdchartdata->pdchart = pdchart;

            /* store key in pdchart header */
            pdchart->pdchartdatakey = pdchartdatakey;

            if((res = (use_preferred
                            ? gr_chart_preferred_new(&pdchart->ch, (P_ANY) pdchartdatakey)
                            : gr_chart_new(&pdchart->ch, (P_ANY) pdchartdatakey, new_untitled))) < 0)
                collect_subtract_entry(&pdchart_listed_data.lbr, pdchartdatakey);
        }

        if(res < 0)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&pdchart->elem.base));
    }

    if(res < 0)
    {
        al_ptr_dispose(P_P_ANY_PEDANTIC(p_pdchart));
        return(res);
    }

    return(1);
}

/******************************************************************************
*
* call-back from null engine to start charts recalculation
*
******************************************************************************/

null_event_proto(static, pdchart_null_handler)
{
    P_PDCHART_HEADER pdchart = (P_PDCHART_HEADER) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        /* SKS after PD 4.11 13jan92 - added auto/manual update */
        if(d_progvars[OR_AC].option != 'A')
            return(NULL_EVENTS_OFF);

        return((pdchart->recalc.state == PDCHART_UNMODIFIED)
                       ? NULL_EVENTS_OFF
                       : NULL_EVENTS_REQUIRED);

    case NULL_EVENT:
        /* SKS after PD 4.11 13jan92 - added auto/manual update */
        if(d_progvars[OR_AC].option != 'A')
            return(NULL_EVENT_COMPLETED);

        switch(pdchart->recalc.state)
        {
        default:
            assert(0);

        case PDCHART_MODIFIED:
            pdchart->recalc.state = PDCHART_AWAITING_RECALC;

        /* deliberate drop thru ... */

        case PDCHART_AWAITING_RECALC:
#if 1
            if(!ev_todo_check())
#else
            if((p_null_event_block->initial_time - pdchart->recalc.last_mod_time) >= MONOTIME_VALUE(250))
#endif
                {
                trace_2(TRACE_MODULE_GR_CHART,
                        "pdchart: chart " PTR_XTFMT "," PTR_XTFMT " has now waited a respectable time since last modification",
                        report_ptr_cast(pdchart), report_ptr_cast(pdchart->ch));

                /* kill off null process to this chart */
                pdchart->recalc.state = PDCHART_UNMODIFIED;

                Null_EventHandlerRemove(pdchart_null_handler, p_null_event_block->client_handle);

                /* ask chart to rebuild if it hasn't done so since given time */
                gr_chart_diagram_ensure(&pdchart->ch);
                }
            break;
        }

        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

/******************************************************************************
*
* add a dynamic text object to the 'current' chart. works for either PD cmd or reload
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_text_add(
    P_PDCHART_HEADER pdchart,
    _InRef_     PC_EV_RANGE rng)
{
    P_PDCHART_DEP     itdep;
    P_PDCHART_ELEMENT pdchartelem;
    STATUS res;

    /* create a new dependency block (at end of list) for this cell to refer to */
    if((res = pdchart_listed_dep_new(&itdep, PDCHART_RANGE_TXT)) < 0)
        return(res);

    /* need to be able to key back to chart */
    itdep->pdchartdatakey = pdchart->pdchartdatakey;

    /* store whole range in there */
    itdep->rng = *rng;

    /* add an external reference to this whole pdchart range */
    if(status_done(res = pdchart_extdependency_new(itdep)))
    {
        pdchartelem = pdchart_element_add(pdchart);

        pdchartelem->type     = itdep->type;
        pdchartelem->itdepkey = itdep->itdepkey;

        pdchartelem->rng.txt.docno = itdep->rng.s.docno; /* EV_DOCNO */
        pdchartelem->rng.txt.col = itdep->rng.s.col;
        pdchartelem->rng.txt.row = itdep->rng.s.row;

        if(status_fail(res = gr_chart_add_text(&pdchart->ch, pdchart_travel_for_text_input, pdchartelem, &pdchartelem->gr_int_handle)))
        {
            pdchart_element_subtract(pdchart, pdchartelem, 0);
        }

        if(status_fail(res))
            pdchart_extdependency_dispose(itdep);
    }

    if(status_fail(res))
    {
        pdchart_listed_dep_dispose(&itdep);
        return(res);
    }

    status_return(pdchart_modify(pdchart));

    return(1);
}

/******************************************************************************
*
* remove dynamic text object
*
******************************************************************************/

_Check_return_
static STATUS
pdchart_text_subtract(
    P_PDCHART_HEADER pdchart,
    P_PDCHART_ELEMENT pdchartelem)
{
    P_PDCHART_DEP itdep;

    itdep = pdchart_listed_dep_search_key(pdchartelem->itdepkey);
    assert(itdep);

    /* remove this range */
    pdchart_extdependency_dispose(itdep);

    pdchart_listed_dep_dispose(&itdep);

    /* source cells going away: must kill use in chart */
    pdchart_element_subtract(pdchart, pdchartelem, 1);

    status_return(pdchart_modify(pdchart));

    return(1);
}

/******************************************************************************
*
* procedure exported to the chart module to get data from the sheet
*
******************************************************************************/

gr_chart_travel_proto(static, pdchart_travel_for_input)
{
    P_PDCHART_ELEMENT pdchartelem = handle;
    DOCNO docno;
    P_LIST_ITEM it;
    P_CELL sl;
    COL col;
    ROW row;
    P_EV_RESULT p_ev_result;

    UNREFERENCED_PARAMETER(ch);

    /* initial guess is nothing */
    if(val)
        val->type = GR_CHART_VALUE_NONE;

    /* determine actual cell to go to */
    switch(pdchartelem->type)
    {
    case PDCHART_RANGE_COL:
        {
        /* col as input source: travel to the nth row therein */

        if(!val)
        {
            /* client calling us to remove ourseleves */
            pdchartelem->type          = PDCHART_RANGE_COL_DEAD;
            pdchartelem->gr_int_handle = 0;
            return(1);
        }

        /* return size of range -- gets complicated with array handling:
         * need to construct an enumeration list mapping external chart
         * addresses to real sub-travel() addresses
         * or else limit such to only supply one array per range
        */
        if(item < 0)
        {
            switch(item)
            {
            case GR_CHART_ITEMNO_N_ITEMS:
                /* return size of range */
                {
                val->type         = GR_CHART_VALUE_N_ITEMS;
                val->data.n_items = (GR_CHART_ITEMNO) (
                                    pdchartelem->rng.col.end_row -
                                    pdchartelem->rng.col.stt_row);

                /* if range has a label then number of items is one less */
                if(pdchartelem->bits.label_first_item)
                    --val->data.n_items;
                return(1);
                }

            case GR_CHART_ITEMNO_LABEL:
                /* return label for range */
                {
                /* if range has label then it's the first item */
                if(pdchartelem->bits.label_first_item)
                    item = 0;
                else
                    return(0);
                }
                break;

            default:
                return(0);
            }
        }
        else
        {
            /* if range has label then skip the first item */
            if(pdchartelem->bits.label_first_item)
                item += 1;
        }

        col = pdchartelem->rng.col.col;
        row = pdchartelem->rng.col.stt_row + (ROW) item;

        if(row >= pdchartelem->rng.col.end_row)
            return(0);
        }
        break;

    default:
        myassert1x(pdchartelem->type == PDCHART_RANGE_NONE, "chart element type %d not COL or ROW or NONE", pdchartelem->type);

        /* deliberate drop thru ... */

    case PDCHART_RANGE_NONE:
        {
        if(item == GR_CHART_ITEMNO_N_ITEMS)
        {
            assert(val);
            if(NULL != val)
            {
                val->type         = GR_CHART_VALUE_N_ITEMS;
                val->data.n_items = 0;
            }
        }
        return(1);
        }

    case PDCHART_RANGE_ROW:
        {
        /* row as input source: travel to the nth column therein */

        if(!val)
        {
            /* client calling us to remove ourseleves */
            pdchartelem->type          = PDCHART_RANGE_ROW_DEAD;
            pdchartelem->gr_int_handle = 0;
            return(1);
        }

        if(item < 0)
        {
            switch(item)
            {
            case GR_CHART_ITEMNO_N_ITEMS:
                /* return size of range */
                {
                val->type         = GR_CHART_VALUE_N_ITEMS;
                val->data.n_items = pdchartelem->rng.row.end_col -
                                    pdchartelem->rng.row.stt_col;
                /* if range has a label then number of items is one less */
                if(pdchartelem->bits.label_first_item)
                    --val->data.n_items;
                return(1);
                }

            case GR_CHART_ITEMNO_LABEL:
                /* return label for range */
                {
                /* if range has label then it's the first item */
                if(pdchartelem->bits.label_first_item)
                    item = 0;
                else
                    return(0);
                }
                break;

            default:
                return(0);
            }
        }
        else
        {
            /* if range has label then skip the first item */
            if(pdchartelem->bits.label_first_item)
                item += 1;
        }

        col = pdchartelem->rng.row.stt_col + (COL) item;
        row = pdchartelem->rng.row.row;

        if(col >= pdchartelem->rng.row.end_col)
            return(0);
        }
        break;
    }

#ifndef OFFICIAL_PDCHART_TRAVEL
/* this is a hackers slightly more efficient version of pdchart_travel_for_input;
 * for the official version, see below
*/

    {
    P_DOCU p_docu;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(pdchartelem->rng.col.docno)))  ||  docu_is_thunk(p_docu))
    {
        assert(DOCNO_NONE != pdchartelem->rng.col.docno);
        assert(pdchartelem->rng.col.docno < DOCNO_MAX);
        return(0);
    }

    if(NULL == (it = list_gotoitem(x_indexcollb(p_docu, col), row)))
        return(0);
    } /*block*/

    sl = slot_contents(it);

    docno = pdchartelem->rng.col.docno;

#else

    docno = pdchartelem->rng.col.docno;

    sl = travel_externally(docno, col, row);
    if(!sl)
        return(0);

#endif

    switch(result_extract(sl, &p_ev_result))
    {
    case SL_NUMBER:
        if(pdchart_extract_numeric_result(p_ev_result, val))
            break;

        /* else deliberate drop thru ... */

    default:
        {
        char buffer[LIN_BUFSIZ];

        expand_cell_for_chart_export(docno, sl, buffer, LIN_BUFSIZ, row);

        val->type = GR_CHART_VALUE_TEXT;
        xstrkpy(val->data.text, elemof32(val->data.text), buffer);
        }
        break;
    }

    return(1);
}

gr_chart_travel_proto(static, pdchart_travel_for_text_input)
{
    P_PDCHART_ELEMENT pdchartelem = handle;
    DOCNO docno;
    P_CELL sl;
    ROW row;
    P_EV_RESULT p_ev_result;

    UNREFERENCED_PARAMETER(ch);
    UNREFERENCED_PARAMETER(item);

    if(!val)
    {
        /* client calling us to remove ourselves */
        P_PDCHART_DEP         itdep;
        P_PDCHART_LISTED_DATA pdchartdata;
        P_PDCHART_HEADER      pdchart;

        assert(pdchartelem->type == PDCHART_RANGE_TXT);

        /* boy is this ludicrous & circuitous ... */

        itdep = pdchart_listed_dep_search_key(pdchartelem->itdepkey);
        assert(itdep);

        pdchartdata = pdchart_listed_data_search_key(itdep->pdchartdatakey);
        assert(pdchartdata);

        pdchart = pdchartdata->pdchart; /* was your journey really necessary? */

        status_assert(pdchart_text_subtract(pdchart, pdchartelem));
        return(1);
    }

    /* initial guess is nothing */
    val->type = GR_CHART_VALUE_NONE;

    /* determine actual cell to go to */
    row = pdchartelem->rng.txt.row;

    /* use official means for texts */
    docno = pdchartelem->rng.txt.docno;

    if(NULL == (sl = travel_externally(docno, pdchartelem->rng.txt.col, row)))
        return(0);

    switch(result_extract(sl, &p_ev_result))
    {
    case SL_PAGE:
        return(0);

    default:
        {
        /* force to text form ALWAYS */
        char buffer[LIN_BUFSIZ];

        expand_cell_for_chart_export(docno, sl, buffer, LIN_BUFSIZ, row);

        val->type = GR_CHART_VALUE_TEXT;
        xstrkpy(val->data.text, elemof32(val->data.text), buffer);
        }
        break;
    }

    return(1);
}

/******************************************************************************
*
* send damage messages off to the chart for all elements in this dependency
*
******************************************************************************/

static void
pdchart_damage_chart_for_dep(
    P_PDCHART_HEADER pdchart,
    P_PDCHART_DEP itdep)
{
    P_PDCHART_ELEMENT i_pdchartelem, pdchartelem;
    S32 modify = 0;

    if(0 != pdchart->elem.n)
    {
        i_pdchartelem = &pdchart->elem.base[0];
        pdchartelem = i_pdchartelem + pdchart->elem.n;

        while(--pdchartelem >= i_pdchartelem)
            if(pdchartelem->itdepkey == itdep->itdepkey)
                switch(pdchartelem->type)
                {
                case PDCHART_RANGE_COL:
                case PDCHART_RANGE_ROW:
                case PDCHART_RANGE_TXT:
                    gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                    modify = 1;
                    break;

                default:
                    break;
                }
    }

    if(modify)
        status_assert(pdchart_modify(pdchart));
}

/******************************************************************************
*
* evaluator document number has changed
*
******************************************************************************/

static void
pdchart_uref_changedoc(
    P_PDCHART_HEADER pdchart,
    P_PDCHART_DEP itdep)
{
    P_PDCHART_ELEMENT i_pdchartelem, pdchartelem;

    itdep->rng.e.docno = itdep->rng.s.docno;

    if(0 != pdchart->elem.n)
    {
        i_pdchartelem = &pdchart->elem.base[0];
        pdchartelem = i_pdchartelem + pdchart->elem.n;

        while(--pdchartelem >= i_pdchartelem)
            if(pdchartelem->itdepkey == itdep->itdepkey)
                pdchartelem->rng.col.docno = itdep->rng.s.docno;
    }
}

/******************************************************************************
*
* call-back from evaluator uref to update references
*
******************************************************************************/

PROC_UREF_PROTO(static, pdchart_uref_handler)
{
    LIST_ITEMNO           itdepkey = (LIST_ITEMNO) exthandle;
    P_PDCHART_DEP         itdep;
    P_PDCHART_LISTED_DATA pdchartdata;
    P_PDCHART_HEADER      pdchart;
    P_PDCHART_ELEMENT     i_pdchartelem, pdchartelem;
    S32                   stt_col, stt_row;
    S32                   end_col, end_row;
    S32                   modify = 0;

    UNREFERENCED_PARAMETER(inthandle);
    UNREFERENCED_PARAMETER_InRef_(at_rng);

    switch(upp->action)
    {
    case UREF_UREF:
    case UREF_DELETE:
    case UREF_SWAP:
    case UREF_CHANGEDOC:
    case UREF_CHANGE:
    /*case UREF_REPLACE:*/
    case UREF_COPY:
    case UREF_SWAPCELL:
    case UREF_CLOSE:
    case UREF_REDRAW:
    /*case UREF_ADJUST:*/
    case UREF_RENAME:
        break;

    default:
        assert(0);
    case UREF_REPLACE:
    case UREF_ADJUST:
        return;
    }

    itdep = pdchart_listed_dep_search_key(itdepkey);
    assert(itdep);

    pdchartdata = pdchart_listed_data_search_key(itdep->pdchartdatakey);
    assert(pdchartdata);

    pdchart = pdchartdata->pdchart;

    /* remember to reload these as needed */
    i_pdchartelem = &pdchart->elem.base[0];
    pdchartelem   = i_pdchartelem + pdchart->elem.n;

    switch(upp->action)
    {
    /* SKS after PD 4.12 26mar92 - added new rename document case */
    case UREF_RENAME:
        pdchart_damage_chart_for_dep(pdchart, itdep);
        break;

    case UREF_CHANGEDOC:
        trace_0(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_CHANGEDOC");
        assert(itdep->rng.s.docno == upp->slr2.docno);
        itdep->rng.s.docno = upp->slr1.docno;

        pdchart_uref_changedoc(pdchart, itdep);

        /* SKS after PD 4.12 26mar92 - this is needed for document unresolved/multiple -> resolved */
        modify = 1;
        break;

    case UREF_CLOSE:
        trace_0(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_CLOSE");

        /* REMOVE NEITHER THIS EXTDEPENDENCY OR OUR LISTED_DEP (needed for save) */

        /* source sheet going away: must bash chart to get it to request these still-live empties */
        pdchart_damage_chart_for_dep(pdchart, itdep);
        break;

    kill_ref:
        {
        switch(upp->action)
        {
        case UREF_CLOSE:
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_CLOSE");
            break;

        default:
            trace_1(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_%d killing ref", upp->action);
            break;
        }

        /* remove this range */
        pdchart_extdependency_dispose(itdep);

        pdchart_listed_dep_dispose(&itdep);

        /* source cells going away: must kill use in chart */
        if(pdchartelem != i_pdchartelem)
            while(--pdchartelem >= i_pdchartelem)
                if(pdchartelem->itdepkey == itdepkey)
                {
                    pdchart_element_subtract(pdchart, pdchartelem, 1);
                    modify = 1;
                }

#if 0
        /* possibility of killing chart too */

        /* SKS after PD 4.11 13jan92 - leave an empty chart. user must kill it */
        if(!pdchart->elem.n)
            pdchart_dispose(&pdchart);
#endif
        }
        break; /* end of UREF_CLOSE/kill_ref */

    case UREF_CHANGE:
    case UREF_SWAPCELL:
        {
#if TRACE_ALLOWED
        switch(upp->action)
        {
        case UREF_CHANGE:
            /* a single cell (upp->slr1) in the range has had a value change: recalc */
            trace_3(TRACE_MODULE_GR_CHART,
                    "pdchart_uref_handler: UREF_CHANGE for (%d,%d,%d)",
                    upp->slr1.docno, upp->slr1.col, upp->slr1.row);
            break;

        case UREF_SWAPCELL:
            /* a pair of cells (upp->slr1, upp->slr2) in the range have been swapped: recalc */
            trace_6(TRACE_MODULE_GR_CHART,
                    "pdchart_uref_handler: UREF_SWAPCELL for (%d,%d,%d), (%d,%d,%d)",
                    upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                    upp->slr2.docno, upp->slr2.col, upp->slr2.row);
            break;
        }
#endif

        if(pdchartelem != i_pdchartelem)
        {
            while(--pdchartelem >= i_pdchartelem)
            {
                if(pdchartelem->itdepkey == itdepkey)
                {
                    switch(pdchartelem->type)
                    {
                    case PDCHART_RANGE_COL:
                        if( (pdchartelem->rng.col.col == upp->slr1.col) || ((upp->action != UREF_CHANGE) &&
                            (pdchartelem->rng.col.col == upp->slr2.col)    ))
                        {
                            gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                            modify = 1;
                            goto endwhile_change;
                        }
                        break;

                    case PDCHART_RANGE_ROW:
                        if( (pdchartelem->rng.row.row == upp->slr1.row) || ((upp->action != UREF_CHANGE) &&
                            (pdchartelem->rng.col.col == upp->slr2.col)    )) /* should these be rows? */
                        {
                            gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                            modify = 1;
                            goto endwhile_change;
                        }
                        break;

                    default:
                        myassert1x(0, "chart element type %d not COL or ROW or NONE or DEAD", pdchartelem->type);

                    /* no need to worry about TXTs in the same pdchart; they'll be on different itdepkeys */

                    case PDCHART_RANGE_NONE:
                    case PDCHART_RANGE_COL_DEAD:
                    case PDCHART_RANGE_ROW_DEAD:
                        break;
                    }
                }
            }
        }
        endwhile_change:;
        }
        break; /* end of UREF_CHANGE/UREF_SWAPCELL */

    case UREF_SWAP:
        {
        trace_6(TRACE_MODULE_GR_CHART,
                "pdchart_uref_handler: UREF_SWAP for (%d,%d,%d), (%d,%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row);

        /* a pair of rows (upp->slr1, upp->slr2) in the range have been swapped: recalc */

        switch(itdep->type)
        {
        case PDCHART_RANGE_COL:
            /* damage all ranges across this dependency */
            pdchart_damage_chart_for_dep(pdchart, itdep);
            break;

        default:
            myassert1x(0, "chart dep type %d not COL or ROW", itdep->type);

            /* deliberate drop thru ... */

        case PDCHART_RANGE_ROW:
            /* damage just the two swapped rows and swap the refs */

            if(pdchartelem != i_pdchartelem)
            {
                while(--pdchartelem >= i_pdchartelem)
                {
                    if(pdchartelem->itdepkey == itdepkey)
                    {
                        if(pdchartelem->type == PDCHART_RANGE_ROW)
                        {
                            S32 hit = 0;

                            if(pdchartelem->rng.row.row == upp->slr1.row)
                            {
                                hit = 1;
                                pdchartelem->rng.row.row = upp->slr2.row;
                                }
                            else if(pdchartelem->rng.row.row == upp->slr2.row)
                            {
                                hit = 1;
                                pdchartelem->rng.row.row = upp->slr1.row;
                            }

                            if(hit)
                            {
                                gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                                modify = 1;
                            }
                        }
                    }
                }
            }
            break;
        }
        }
        break; /* end of UREF_SWAP */

    case UREF_UREF:
        {
        /* simple motion of deps and elements harmless; just update structures

         * if columns(rows) have been inserted/added into
         * a col(row)-based dep then insert new elements into chart

         * if rows(columns) have been inserted/added into
         * a col(row)-based element then recalc chart
        */
        S32 add;
        S32 res;

        trace_8(TRACE_MODULE_GR_CHART,
                "pdchart_uref_handler: UREF_UREF for (%d,%d,%d), (%d,%d,%d) by (%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row,
                                 upp->slr3.col, upp->slr3.row);

        /* SKS after PD 4.12 26mar92 - consider moving a block from one doc to another */
        if(itdep->rng.s.docno != upp->slr3.docno)
        {
            itdep->rng.s.docno = upp->slr3.docno;
            pdchart_uref_changedoc(pdchart, itdep);
            status_assert(pdchart_modify(pdchart));
        }

        if(status != DEP_UPDATE)
        {
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_UREF not DEP_UPDATE");
            modify = 1;
            break;
        }

        /* look for insertions while noting updrefs to dep block */

        stt_col = (itdep->rng.s.col >= upp->slr1.col)  &&
                  (itdep->rng.s.col <= upp->slr2.col);

        stt_row = (itdep->rng.s.row >= upp->slr1.row)  &&
                  (itdep->rng.s.row <= upp->slr2.row);

        end_col = (itdep->rng.e.col >= upp->slr1.col)  &&
                  (itdep->rng.e.col <= upp->slr2.col);

        end_row = (itdep->rng.e.row >= upp->slr1.row)  &&
                  (itdep->rng.e.row <= upp->slr2.row);

        /* updref our range */
        if(stt_col)
            itdep->rng.s.col += upp->slr3.col;
        if(stt_row)
            itdep->rng.s.row += upp->slr3.row;
        if(end_col)
            itdep->rng.e.col += upp->slr3.col;
        if(end_row)
            itdep->rng.e.row += upp->slr3.row;

        /* the range has been updrefed; modify elements correspondingly */

        if(pdchartelem != i_pdchartelem)
        {
            while(--pdchartelem >= i_pdchartelem)
            {
                if(pdchartelem->itdepkey == itdepkey)
                {
                    switch(pdchartelem->type)
                    {
                    case PDCHART_RANGE_COL:
                    case PDCHART_RANGE_COL_DEAD:
                        {
                        S32 elem_stt_row, elem_end_row;

                        /* column goes walkabout independently */
                        if( (pdchartelem->rng.col.col >= upp->slr1.col)  &&
                            (pdchartelem->rng.col.col <= upp->slr2.col)  )
                             pdchartelem->rng.col.col += upp->slr3.col;

                        /* check for row insertion */
                        elem_stt_row = (pdchartelem->rng.col.stt_row >= upp->slr1.row)  &&
                                       (pdchartelem->rng.col.stt_row <= upp->slr2.row);

                        elem_end_row = (pdchartelem->rng.col.end_row >= upp->slr1.row)  &&
                                       (pdchartelem->rng.col.end_row <= upp->slr2.row);

                        if(elem_stt_row)
                            pdchartelem->rng.col.stt_row += upp->slr3.row;
                        if(elem_end_row)
                            pdchartelem->rng.col.end_row += upp->slr3.row;

                        if(elem_stt_row != elem_end_row)
                            if(pdchartelem->type == PDCHART_RANGE_COL)
                            {
                                /* col-based live range has had row insertion: recalc */
                                gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                                modify = 1;
                            }
                        }
                        break;

                    default:
                        myassert1x(0, "chart element type %d not COL or ROW or NONE or DEAD", pdchartelem->type);

                        /* deliberate drop thru ... */

                    case PDCHART_RANGE_NONE:
                        break;

                    case PDCHART_RANGE_ROW:
                    case PDCHART_RANGE_ROW_DEAD:
                        {
                        S32 elem_stt_col, elem_end_col;

                        /* row goes walkabout independently */
                        if( (pdchartelem->rng.row.row >= upp->slr1.row)  &&
                            (pdchartelem->rng.row.row <= upp->slr2.row)  )
                             pdchartelem->rng.row.row += upp->slr3.row;

                        /* check for column insertion */
                        elem_stt_col = (pdchartelem->rng.row.stt_col >= upp->slr1.col)  &&
                                       (pdchartelem->rng.row.stt_col <= upp->slr2.col);

                        elem_end_col = (pdchartelem->rng.row.end_col >= upp->slr1.col)  &&
                                       (pdchartelem->rng.row.end_col <= upp->slr2.col);

                        if(elem_stt_col)
                            pdchartelem->rng.row.stt_col += upp->slr3.col;
                        if(elem_end_col)
                            pdchartelem->rng.row.end_col += upp->slr3.col;

                        if(elem_stt_col != elem_end_col)
                            if(pdchartelem->type == PDCHART_RANGE_ROW)
                            {
                                /* row-based live range has had col insertion: recalc */
                                gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                                modify = 1;
                            }
                        }
                        break;
                    } /*esac*/
                } /*fi*/
            }
        }

        /* all existing elements now processed; consider insertions into range */

        add = (itdep->type == PDCHART_RANGE_COL)
                            ? ((stt_col != end_col)  &&  end_col  &&  (upp->slr3.col > 0))
                            : ((stt_row != end_row)  &&  end_row  &&  (upp->slr3.row > 0));

        if(add)
        {
            PDCHART_PROCESS process;

            zero_struct(process);

            switch(itdep->type)
            {
            case PDCHART_RANGE_COL:
                /* have inserted upp->slr3.col column(s): supply insert(s) to chart */
                if(0 > (res = pdchart_element_ensure(pdchart, upp->slr3.col)))
                    break;

                /* insert into chart in current dependency block */
                res = pdchart_add_into(pdchart, process, itdep,
                                       upp->slr1.col,
                                       upp->slr1.col + upp->slr3.col /*excl*/);
                break;

            default:
                myassert1x(0, "chart dep type %d not COL or ROW", itdep->type);

                /* deliberate drop thru ... */

            case PDCHART_RANGE_ROW:
                /* have inserted upp->slr3.row row(s): supply insert(s) to chart */
                if(0 > (res = pdchart_element_ensure(pdchart, upp->slr3.row)))
                    break;

                /* insert into chart in current dependency block */
                res = pdchart_add_into(pdchart, process, itdep,
                                       upp->slr1.row,
                                       upp->slr1.row + upp->slr3.row /*excl*/);
                break;
            }

            if(res < 0)
                break;

            /* no need to free suss_holes resources as UREF insertions always insert
             * across the entire range and at the right row,col limits & no holes
            */

            /* get chart rebuilt sometime */
            modify = 1;
        }
        }
        break; /* end of UREF_UREF */

    case UREF_DELETE:
        {
        /* some (or all) of the cells in the range have been deleted - NOT NECESSARILY UPDATING RANGE

         * if columns(rows) have been deleted from a
         * col(row)-based dep then remove elements from chart

         * if cells in rows(columns) have been deleted from a
         * col(row)-based element then recalc chart
        */

        trace_6(TRACE_MODULE_GR_CHART,
                "pdchart_uref_handler: UREF_DELETE for (%d,%d,%d), (%d,%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row);

        if(status == DEP_DELETE)
        {
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_uref_handler: UREF_DELETE got DEP_DELETE");
            goto kill_ref;
        }

        /* look for removes in dep block */

        stt_col = (itdep->rng.s.col >= upp->slr1.col)  &&
                  (itdep->rng.s.col <= upp->slr2.col);

        stt_row = (itdep->rng.s.row >= upp->slr1.row)  &&
                  (itdep->rng.s.row <= upp->slr2.row);

        end_col = (itdep->rng.e.col >= upp->slr1.col)  &&
                  (itdep->rng.e.col <= upp->slr2.col);

        end_row = (itdep->rng.e.row >= upp->slr1.row)  &&
                  (itdep->rng.e.row <= upp->slr2.row);

        switch(itdep->type)
        {
        case PDCHART_RANGE_COL:
        case PDCHART_RANGE_COL_DEAD:
            {
            if(stt_row && end_row)
            {
                /* have wholly removed column(s): supply remove(s) to chart */
                COL col  = MIN(itdep->rng.e.col, upp->slr2.col);

                while(col > MAX(itdep->rng.s.col, upp->slr1.col))
                {
                    --col;

                    /* find handle to remove */
                    if(0 != pdchart->elem.n)
                    {
                        i_pdchartelem = &pdchart->elem.base[0];
                        pdchartelem   = i_pdchartelem + pdchart->elem.n;

                        while(--pdchartelem >= i_pdchartelem)
                        {
                            if(pdchartelem->itdepkey == itdepkey)
                            {
                                switch(pdchartelem->type)
                                {
                                default:
                                    myassert2x(0, "UREF_DELETE RANGE_COL found chart element of different type %d to dep type %d",
                                               pdchartelem->type, itdep->type);

                                case PDCHART_RANGE_NONE:
                                    break;

                                case PDCHART_RANGE_COL:
                                    if(pdchartelem->rng.col.col == col)
                                        modify = 1;

                                /* deliberate drop thru ... */

                                case PDCHART_RANGE_COL_DEAD:
                                    if(pdchartelem->rng.col.col == col)
                                    {
                                        pdchart_element_subtract(pdchart, pdchartelem, 1);
                                        goto endwhile_delete_col;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    endwhile_delete_col:;
                }
            }
            }
            break;

        default:
            myassert1x(0, "chart dep type %d not COL or ROW", itdep->type);

            /* deliberate drop thru ... */

        case PDCHART_RANGE_ROW:
        case PDCHART_RANGE_ROW_DEAD:
            {
            if(stt_col && end_col)
            {
                /* have wholly removed row(s): supply remove(s) to chart */
                ROW row  = MIN(itdep->rng.e.row, (ROW) upp->slr2.row);

                while(row > MAX(itdep->rng.s.row, (ROW) upp->slr1.row))
                {
                    --row;

                    /* find handle to remove */
                    if(0 != pdchart->elem.n)
                    {
                        i_pdchartelem = &pdchart->elem.base[0];
                        pdchartelem   = i_pdchartelem + pdchart->elem.n;

                        while(--pdchartelem >= i_pdchartelem)
                        {
                            if(pdchartelem->itdepkey == itdepkey)
                            {
                                switch(pdchartelem->type)
                                {
                                default:
                                    myassert2x(0, "UREF_DELETE RANGE_ROW found chart element of different type %d to dep type %d",
                                               pdchartelem->type, itdep->type);

                                case PDCHART_RANGE_NONE:
                                    break;

                                case PDCHART_RANGE_ROW:
                                    if(pdchartelem->rng.row.row == row)
                                        modify = 1;

                                /* deliberate drop thru ... */

                                case PDCHART_RANGE_ROW_DEAD:
                                    if(pdchartelem->rng.row.row == row)
                                    {
                                        pdchart_element_subtract(pdchart, pdchartelem, 1);
                                        goto endwhile_delete_row;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    endwhile_delete_row:;
                }
            }
            }
            break;
        }

        /* part of the range has been deleted; don't modify elements:
         * in the case of full block removal they're all gone anyway
         * and in the case of partial block removal just need chart updating
         */

        pdchartelem = i_pdchartelem + pdchart->elem.n;

        if(0 != pdchart->elem.n)
        {
            while(--pdchartelem >= i_pdchartelem)
            {
                if(pdchartelem->itdepkey == itdepkey)
                {
                    switch(pdchartelem->type)
                    {
                    case PDCHART_RANGE_COL:
                        {
                        if( (pdchartelem->rng.col.col >= upp->slr1.col)  &&
                            (pdchartelem->rng.col.col <= upp->slr2.col)  )
                        {
                            /* col-based range has had row deletion: recalc */
                            gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                            modify = 1;
                        }
                        break;
                        }

                    default:
                        myassert1x(0, "chart element type %d not COL or ROW or NONE", pdchartelem->type);

                        /* deliberate drop thru ... */

                    case PDCHART_RANGE_NONE:
                    case PDCHART_RANGE_COL_DEAD:
                    case PDCHART_RANGE_ROW_DEAD:
                        break;

                    case PDCHART_RANGE_ROW:
                        {
                        if( (pdchartelem->rng.row.row >= upp->slr1.row)  &&
                            (pdchartelem->rng.row.row <= upp->slr2.row)  )
                        {
                            /* row-based range has had col deletion: recalc */
                            gr_chart_damage(&pdchart->ch, &pdchartelem->gr_int_handle);
                            modify = 1;
                        }
                        break;
                        }
                    } /*esac*/
                }
            }
        }

        /* all elements now done */
        }
        break; /* end of UREF_DELETE */

    default:
        trace_6(TRACE_MODULE_GR_CHART,
                "pdchart_uref_handler: UREF_??? for (%d,%d,%d), (%d,%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row);
        break;
    }

    if(modify)
        status_assert(pdchart_modify(pdchart));
}

/******************************************************************************
*
* call-back from evaluator uref to update references to text objects
*
******************************************************************************/

PROC_UREF_PROTO(static, pdchart_text_uref_handler)
{
    LIST_ITEMNO           itdepkey = (LIST_ITEMNO) exthandle;
    P_PDCHART_DEP         itdep;
    P_PDCHART_LISTED_DATA pdchartdata;
    P_PDCHART_HEADER      pdchart;
    P_PDCHART_ELEMENT     i_pdchartelem, pdchartelem;
    S32                   modify = 0;

    UNREFERENCED_PARAMETER(inthandle);
    UNREFERENCED_PARAMETER_InRef_(at_rng);

    switch(upp->action)
    {
    case UREF_UREF:
    case UREF_DELETE:
    case UREF_SWAP:
    case UREF_CHANGEDOC:
    case UREF_CHANGE:
    /*case UREF_REPLACE:*/
    case UREF_COPY:
    case UREF_SWAPCELL:
    case UREF_CLOSE:
    case UREF_REDRAW:
    /*case UREF_ADJUST:*/
    case UREF_RENAME:
        break;

    default:
        assert(0);
    case UREF_REPLACE:
    case UREF_ADJUST:
        return;
    }

    itdep = pdchart_listed_dep_search_key(itdepkey);
    assert(itdep);

    pdchartdata = pdchart_listed_data_search_key(itdep->pdchartdatakey);
    assert(pdchartdata);

    pdchart = pdchartdata->pdchart;

    /* no realloc by this uref handler so we can common up here */
    i_pdchartelem = &pdchart->elem.base[0];
    pdchartelem   = i_pdchartelem + pdchart->elem.n;

    switch(upp->action)
    {
    /* SKS after PD 4.12 26mar92 - added new rename document case */
    case UREF_RENAME:
        pdchart_damage_chart_for_dep(pdchart, itdep);
        break;

    case UREF_CHANGEDOC:
        trace_0(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_CHANGEDOC");
        assert(itdep->rng.s.docno == upp->slr2.docno);
        itdep->rng.s.docno = upp->slr1.docno;

        pdchart_uref_changedoc(pdchart, itdep);

        /* SKS after PD 4.12 26mar92 - this is needed for document unresolved/multiple -> resolved */
        modify = 1;
        break;

    case UREF_CLOSE:
        trace_0(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_CLOSE");

        /* REMOVE NEITHER THIS EXTDEPENDENCY OR OUR LISTED_DEP (needed for save) */

        /* source sheet going away: must bash chart to get it to request this still-live empty cell */
        pdchart_damage_chart_for_dep(pdchart, itdep);
        break;

    kill_ref:
        {
#if TRACE_ALLOWED
        switch(upp->action)
        {
        case UREF_CLOSE:
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_CLOSE");
            break;

        default:
            trace_1(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_%d killing ref", upp->action);
            break;
        }
#endif /* TRACE_ALLOWED */

        /* remove this range */
        pdchart_extdependency_dispose(itdep);

        pdchart_listed_dep_dispose(&itdep);

        /* source cells going away: must kill use in chart and dep */
        if(pdchartelem != i_pdchartelem)
        {
            while(--pdchartelem >= i_pdchartelem)
            {
                if(pdchartelem->itdepkey == itdepkey)
                {
                    assert(pdchartelem->type == PDCHART_RANGE_TXT);
                    status_assert(pdchart_text_subtract(pdchart, pdchartelem));
                    modify = 1;
                    break; /* never more than one element per dependency */
                }
            }
        }
        }
        break; /* end of UREF_CLOSE/kill_ref */

    case UREF_CHANGE:
    case UREF_SWAPCELL:
    case UREF_SWAP:
        {
#if TRACE_ALLOWED
        switch(upp->action)
        {
        case UREF_CHANGE:
            /* a single cell (upp->slr1) in the range has had a value change: recalc */
            trace_3(TRACE_MODULE_GR_CHART,
                    "pdchart_text_uref_handler: UREF_CHANGE for (%d,%d,%d)",
                    upp->slr1.docno, upp->slr1.col, upp->slr1.row);
            break;

        case UREF_SWAPCELL:
            /* a pair of cells (upp->slr1, upp->slr2) in the range have been swapped: recalc */
            trace_6(TRACE_MODULE_GR_CHART,
                    "pdchart_text_uref_handler: UREF_SWAPCELL for (%d,%d,%d), (%d,%d,%d)",
                    upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                    upp->slr2.docno, upp->slr2.col, upp->slr2.row);
            break;

        case UREF_SWAP:
            /* a pair of rows (upp->slr1, upp->slr2) in the range have been swapped: recalc */
            trace_6(TRACE_MODULE_GR_CHART,
                    "pdchart_text_uref_handler: UREF_SWAP for (%d,%d,%d), (%d,%d,%d)",
                    upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                    upp->slr2.docno, upp->slr2.col, upp->slr2.row);
            break;
        }
#endif

        pdchart_damage_chart_for_dep(pdchart, itdep);
        }
        break;

    case UREF_UREF:
        {
        /* simple motion of text elements harmless; just update structures
        */
        trace_8(TRACE_MODULE_GR_CHART,
                "pdchart_text_uref_handler: UREF_UREF for (%d,%d,%d), (%d,%d,%d) by (%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row,
                                 upp->slr3.col, upp->slr3.row);

        /* SKS after PD 4.12 26mar92 - consider moving a block from one doc to another */
        if(itdep->rng.s.docno != upp->slr3.docno)
        {
            itdep->rng.s.docno = upp->slr3.docno;
            pdchart_uref_changedoc(pdchart, itdep);
            status_assert(pdchart_modify(pdchart));
        }

        if(status != DEP_UPDATE)
        {
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_UREF not DEP_UPDATE");
            modify = 1;
            break;
        }

        itdep->rng.s.col += upp->slr3.col;
        itdep->rng.s.row += upp->slr3.row;
        itdep->rng.e.col += upp->slr3.col;
        itdep->rng.e.row += upp->slr3.row;

        if(pdchartelem != i_pdchartelem)
        {
            while(--pdchartelem >= i_pdchartelem)
            {
                if(pdchartelem->itdepkey == itdepkey)
                {
                    pdchartelem->rng.txt.col += upp->slr3.col;
                    pdchartelem->rng.txt.row += upp->slr3.row;
                    break; /* never more than one element per dependency */
                }
            }
        }
        break; /* end of UREF_UREF */
        }

    case UREF_DELETE:
        {
        /* some (or all) of the cells in the range have been deleted - NOT NECESSARILY UPDATING RANGE
        */

        trace_6(TRACE_MODULE_GR_CHART,
                "pdchart_text_uref_handler: UREF_DELETE for (%d,%d,%d), (%d,%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row);

        if(status == DEP_DELETE)
        {
            trace_0(TRACE_MODULE_GR_CHART, "pdchart_text_uref_handler: UREF_DELETE got DEP_DELETE");
            goto kill_ref;
        }

        assert(0);
        }
        break; /* end of UREF_DELETE */

    default:
        trace_6(TRACE_MODULE_GR_CHART,
                "pdchart_text_uref_handler: UREF_??? for (%d,%d,%d), (%d,%d,%d)",
                upp->slr1.docno, upp->slr1.col, upp->slr1.row,
                upp->slr2.docno, upp->slr2.col, upp->slr2.row);

        break;
    }

    /* SKS after PD 4.12 26mar92 - best done here I feel */
    if(modify)
        status_assert(pdchart_modify(pdchart));
}

/******************************************************************************
*
* command function interface to rest of PipeDream
*
******************************************************************************/

extern void
ChartNew_fn(void)
{
    PDCHART_SHAPEDESC chart_shapedesc;
    P_PDCHART_HEADER pdchart;
    STATUS res;

    if(!MARKER_DEFINED())
    {
        reperr_null((blkstart.col != NO_COL)
                            ? create_error(ERR_NOBLOCKINDOC)
                            : create_error(ERR_NOBLOCK));
        return;
    }

    if(status_fail(res = pdchart_init_shape_from_marked_block(&chart_shapedesc, NULL)))
    {
        reperr_null(res);
        return;
    }

    /* cancelled chart creation */
    if(0 == res)
        return;

    if((res = pdchart_new(&pdchart, chart_shapedesc.n_ranges, 1, 1)) < 0)
    {
        pdchart_init_shape_suss_holes_end(&chart_shapedesc);
        reperr_null(res);
        return;
    }

    res = pdchart_add(pdchart, &chart_shapedesc, 1);

    pdchart_init_shape_suss_holes_end(&chart_shapedesc);

    if(status_fail(res))
    {
        pdchart_dispose(&pdchart);
        reperr_null(res);
        return;
    }

    res = gr_chartedit_new(&pdchart->ceh, pdchart->ch, ChartEdit_notify_proc, pdchart);

    if(status_fail(res))
    {
        pdchart_dispose(&pdchart);
        reperr_null(res);
        return;
    }

    /* select as current */
    pdchart_current = pdchart;
}

/******************************************************************************
*
* add the marked block as more data to be charted to the 'current' chart
*
******************************************************************************/

extern void
ChartAdd_fn(void)
{
    PDCHART_SHAPEDESC chart_shapedesc;
    STATUS res;

    /* SKS after PD 4.12 26mar92 */
    if(!pdchart_current)
        pdchart_select_something(1 /*iff only one*/);

    if(!pdchart_current)
    {
        reperr_null(ERR_NOSELCHART);
        return;
    }

    if(!MARKER_DEFINED())
    {
        reperr_null((blkstart.col != NO_COL)
                            ? create_error(ERR_NOBLOCKINDOC)
                            : create_error(ERR_NOBLOCK));
        return;
    }

    /* SKS after PD 4.11 07feb92 - made condition correct to allow single marks to add text (blkend.col may == NO_COL) */
    if(  (blkend.col == NO_COL) ||
        ((blkstart.col == blkend.col) && (blkstart.row == blkend.row)))
    {
        EV_RANGE rng;

        /* cater for the PipeDream side of the chart descriptor growing */
        if((res = pdchart_element_ensure(pdchart_current, 1)) > 0)
        {
            rng.s.docno = (EV_DOCNO) current_docno();

            rng.s.col = (EV_COL) blkstart.col;
            rng.s.row = (EV_ROW) blkstart.row;

            rng.e.col = rng.s.col + 1;
            rng.e.row = rng.s.row + 1;

            res = pdchart_text_add(pdchart_current, &rng);
        }
    }
    else
    {
        if(status_fail(res = pdchart_init_shape_from_marked_block(&chart_shapedesc, pdchart_current)))
        {
            reperr_null(res);
            return;
        }

        /* cancelled chart creation */
        if(0 == res)
            return;

        res = pdchart_add(pdchart_current, &chart_shapedesc, 0);

        pdchart_init_shape_suss_holes_end(&chart_shapedesc);
    }

    if(res < 0)
        reperr_null(res);
}

/******************************************************************************
*
* delete the 'current' chart
*
******************************************************************************/

extern void
ChartDelete_fn(void)
{
    U8  filename[BUF_MAX_PATHSTRING];
    S32 res;

    /* SKS after PD 4.12 26mar92 */
    if(!pdchart_current)
        pdchart_select_something(1 /*iff only one*/);

    if(!pdchart_current)
    {
        reperr_null(ERR_NOSELCHART);
        return;
    }

    /* convert the live Chart file to a dead Draw file if it's saved out already */
    gr_chart_name_query(&pdchart_current->ch, filename, sizeof32(filename)-1);
    if(file_is_rooted(filename))
    {
        res = gr_chart_save_draw_file_without_dialog(&pdchart_current->ch, filename);
        if(res < 0)
            reperr(res, filename);
    }

    pdchart_dispose(&pdchart_current);
}

static void
chartoptions_fn_core(void)
{
    /* all chart options are read from windvars when required */

    filealtered(TRUE);
}

extern void
ChartOptions_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_CHART_OPTIONS))
    {
        chartoptions_fn_core();

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* some user function will be needed (esp. for macro files)
 * to set a chart as 'current'
 */

extern void
ChartSelect_fn(void)
{
    /* nothing here but us comments */
}

/******************************************************************************
*
* the chart editor has called us back for something
*
******************************************************************************/

_Check_return_
static STATUS
ChartEdit_notify_proc(
    P_ANY handle,
    GR_CHARTEDIT_HANDLE ceh,
    GR_CHARTEDIT_NOTIFY_TYPE ntype,
    P_ANY nextra)
{
    P_PDCHART_HEADER pdchart = handle;
    STATUS res;

    UNREFERENCED_PARAMETER(ceh);
    UNREFERENCED_PARAMETER(nextra);

    switch(ntype)
    {
    case GR_CHARTEDIT_NOTIFY_RESIZEREQ:
        /* always allow resize ops */
        res = 1;
        break;

    case GR_CHARTEDIT_NOTIFY_CLOSEREQ:
        {
        char name_buffer[BUF_MAX_PATHSTRING];
        S32  nRefs;
        BOOL adjust_clicked = riscos_adjust_clicked();
        BOOL shift_pressed  = akbd_pollsh();
        BOOL just_opening   = (shift_pressed  &&  adjust_clicked);
        BOOL want_to_close  = TRUE;

        /* default is to allow window closure */
        res = 1;

        gr_chart_name_query(&pdchart->ch, name_buffer, sizeof32(name_buffer) - 1);

        /* if on disc ok, close editing window NOW, otherwise ask ... */
        if(!just_opening && !file_is_rooted(name_buffer))
        {
            char statement_buffer[LIN_BUFSIZ];

            consume_int(xsnprintf(statement_buffer, elemof32(statement_buffer), save_edited_chart_Zs_YN_S_STR, name_buffer));

            switch(riscdialog_query_YN(statement_buffer, save_edited_chart_YN_Q_STR))
            {
            case riscdialog_query_YES:
                { /* use a dialog box */
                res = gr_chart_save_chart_with_dialog(&pdchart->ch);

                /* test for unsafe receiver; don't close if sent off to Edit for instance */
                if(status_done(res))
                {
                    gr_chart_name_query(&pdchart->ch, name_buffer, sizeof32(name_buffer) - 1);

                    if(!file_is_rooted(name_buffer))
                        res = 0;
                }

                break;
                }

            case riscdialog_query_NO:
                res = 1;
                break;

            default: default_unhandled();
            case riscdialog_query_CANCEL:
                res = 0;
                break;
            }
        }

        if(adjust_clicked)
            filer_opendir(name_buffer);

        if(!just_opening && want_to_close && status_done(res))
        {
            /* destroy the editing window */
            gr_chartedit_dispose(&pdchart->ceh);

            /* may have been saved into a PipeDream window */
            nRefs = image_cache_refs(name_buffer);

            /* if there is no use of this chart elsewhere in PipeDream then kill completely */
            if(!nRefs)
                pdchart_dispose(&pdchart);
        }
        break;
        }

    default:
        /* pass back */
        res = gr_chartedit_notify_default(handle, ceh, ntype, nextra);
        break;
    }

    return(res);
}

_Check_return_
static STATUS
pdchart_show_editor(
    P_PDCHART_HEADER pdchart)
{
    GR_CHARTEDIT_HANDLE ceh;
    STATUS res;

    if(pdchart->ceh)
    {
        /* already editing this chart! - do NOT whinge */
        gr_chartedit_front(&pdchart->ceh);
        return(1);
    }

    res = gr_chartedit_new(&ceh, pdchart->ch, ChartEdit_notify_proc, pdchart);

    if(res <= 0)
    {
        return(res
                ? ((res != create_error(GR_CHART_ERR_ALREADY_EDITING)) ? res : 1)
                : status_nomem());
    }

    pdchart->ceh = ceh;

    return(1);
}

_Check_return_
extern STATUS
pdchart_show_editor_using_handle(
    P_ANY epdchartdatakey)
{
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;

    pdchartdatakey = (LIST_ITEMNO) epdchartdatakey;

    pdchartdata = pdchart_listed_data_search_key(pdchartdatakey);

    return(pdchart_show_editor(pdchartdata->pdchart));
}

extern void
ChartEdit_fn(void)
{
    STATUS res;

    /* SKS after PD 4.12 26mar92 */
    if(!pdchart_current)
        pdchart_select_something(0 /*any will do*/);

    if(!pdchart_current)
    {
        reperr_null(ERR_NOSELCHART);
        return;
    }

    if(status_fail(res = pdchart_show_editor(pdchart_current)))
        reperr_null(res);
}

/*
this is to just warn of any **unsaved** charts that depend on this document
other dependencies via charts are dealt with earlier
*/

extern BOOL
dependent_charts_warning(void)
{
    EV_DOCNO cur_docno = (EV_DOCNO) current_docno();
    enum RISCDIALOG_QUERY_DC_REPLY DC_res = riscdialog_query_DC_CANCEL;
    LIST_ITEMNO pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;

    for(pdchartdata = collect_first(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey);
        pdchartdata;
        pdchartdata = collect_next( PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey))
    {
        P_PDCHART_HEADER  pdchart;
        U8                filename[BUF_MAX_PATHSTRING];
        P_PDCHART_ELEMENT ep, last_ep;
        BOOL              saved_to_disc;
        S32               hit_doc_cur   = 0;
        S32               hit_doc_other = 0;

        pdchart = pdchartdata->pdchart;

        gr_chart_name_query(&pdchart->ch, filename, sizeof32(filename) - 1);

        saved_to_disc = file_is_rooted(filename);

        ep = &pdchart->elem.base[0];

        for(last_ep = ep + pdchart->elem.n; ep < last_ep; ++ep)
            switch(ep->type)
            {
            case PDCHART_RANGE_COL:
            case PDCHART_RANGE_ROW:
            case PDCHART_RANGE_TXT:
                if(cur_docno == ep->rng.col.docno)
                    hit_doc_cur = 1;
                else if(ep->rng.col.docno != DOCNO_NONE)
                    hit_doc_other = 1;
                break;

            default:
                break;
            }

        /* if it doesn't depend on us at all, loop */
        if(!hit_doc_cur)
            continue;

        if(!saved_to_disc)
        {
            /* only ask once for all such */
            if(DC_res == riscdialog_query_DC_CANCEL)
                DC_res = riscdialog_query_DC(close_dependent_charts_DC_S_STR,
                                             close_dependent_charts_DC_Q_STR);

            /* have mercy, the user went 'Cancel' */
            if(DC_res == riscdialog_query_DC_CANCEL)
                return(FALSE);

            /* next time round we won't ask */
        }

        /* if it depended just on us then kill it */
        if(!hit_doc_other)
            pdchart_dispose(&pdchart);
    }

    /* ok to kill this doc */
    return(TRUE);
}

/*
return the number of documents other than this one
that use charts that have dependencies into this sheet
*/

extern S32
pdchart_dependent_documents(
    _InVal_     EV_DOCNO cur_docno)
{
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;
    S32                   nDepDocs = 0;

    for(pdchartdata = collect_first(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey);
        pdchartdata;
        pdchartdata = collect_next( PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey))
    {
        P_PDCHART_HEADER  pdchart;
        P_PDCHART_ELEMENT ep, last_ep;

        pdchart = pdchartdata->pdchart;

        ep = &pdchart->elem.base[0];

        for(last_ep = ep + pdchart->elem.n; ep < last_ep; ++ep)
            switch(ep->type)
            {
            case PDCHART_RANGE_COL:
            case PDCHART_RANGE_ROW:
            case PDCHART_RANGE_TXT:
                if(cur_docno == ep->rng.col.docno)
                {
                    U8 filename[BUF_MAX_PATHSTRING];

                    gr_chart_name_query(&pdchart->ch, filename, sizeof32(filename) - 1);

                    if(file_is_rooted(filename))
                    {
                        IMAGE_CACHE_HANDLE cah;

                        if(image_cache_entry_query(&cah, filename))
                        {
                            /* loop over draw file references and see who
                             * (other than us) has a use of this chart
                            */
                            P_DRAW_FILE_REF dfrp;
                            LIST_ITEMNO key;

                            for(dfrp = collect_first(DRAW_FILE_REF, &draw_file_refs.lbr, &key);
                                dfrp;
                                dfrp = collect_next( DRAW_FILE_REF, &draw_file_refs.lbr, &key))
                                if(dfrp->draw_file_key == cah)
                                    if(dfrp->docno != cur_docno)
                                        ++nDepDocs;
                        }
                        /* else not in cache */
                    }
                    /* else unsaved, so won't be in cache */

                    goto next_pdchart;
                }
                break;

            default:
                break;
            }

        next_pdchart:;
    }

    return(nDepDocs);
}

/******************************************************************************
*
* clean up text and date cells for export
* a la send cell via hot link - common up!
*
******************************************************************************/

extern void
expand_cell_for_chart_export(
    _InVal_     DOCNO docno,
    P_CELL sl,
    char * buffer /*out*/,
    U32 elemof_buffer,
    ROW row)
{
    P_DOCU p_docu;
    char * ptr, * to, ch;
    S32 t_justify;
    S32 t_curpnm;

    t_justify   = sl->justify;
    sl->justify = J_LEFT;

    p_docu = p_docu_from_docno(docno);
    assert(NO_DOCUMENT != p_docu);
    assert(!docu_is_thunk(p_docu));

    t_curpnm = p_docu->Xcurpnm;
    p_docu->Xcurpnm = 0; /* we have really no idea what page a cell is on */

    (void) expand_cell(
                docno, sl, row, buffer, elemof_buffer /*fwidth*/,
                DEFAULT_EXPAND_REFS /*expand_refs*/,
                EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ |
                EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT /*!allow_fonty_result*/ /*expand_flags*/,
                TRUE /*cff*/);

    /* remove highlights & funny spaces etc. from plain non-fonty string */
    ptr = to = buffer;
    do  {
        ch = *ptr++;
        if(ch >= CH_SPACE)
            *to++ = ch;
    }
    while(CH_NULL != ch);
    *to = CH_NULL;

    p_docu->Xcurpnm = t_curpnm;

    sl->justify = t_justify;
}

/******************************************************************************
*
* attempt to load any documents that this chart
* has established external dependencies upon
*
******************************************************************************/

extern S32
pdchart_load_dependents(
    P_ANY epdchartdatakey,
    PC_U8 chartfilename)
{
#if 0 /* can't do this are we haven't got an established set of urefs */
    UNREFERENCED_PARAMETER(epdchartdatakey);

    (void) loadfile_recurse_load_supporting_documents(chartfilename);
#else
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;
#if 1
    LIST_ITEMNO           itdepkey;
    P_PDCHART_DEP         itdep;
#else
    P_PDCHART_HEADER      pdchart;
    P_PDCHART_ELEMENT     ep, last_ep;
#endif
    P_SS_DOC              p_ss_doc;

    UNREFERENCED_PARAMETER(chartfilename);

    pdchartdatakey = (LIST_ITEMNO) epdchartdatakey;

    pdchartdata = pdchart_listed_data_search_key(pdchartdatakey);

    assert(pdchartdata);
    if(!pdchartdata)
        return(1);

#if 1
    /* SKS after PD 4.12 26mar92 - more helpful on errors perchance - loop over deps of this chart not chart elements */
    for(itdep = collect_first(PDCHART_DEP, &pdchart_listed_deps.lbr, &itdepkey);
        itdep;
        itdep = collect_next( PDCHART_DEP, &pdchart_listed_deps.lbr, &itdepkey))
    {
        if(itdep->pdchartdatakey != pdchartdatakey)
            continue;
#else
    pdchart = pdchartdata->pdchart;

    ep = &pdchart->elem.base[0];

    for(last_ep = ep + pdchart->elem.n; ep < last_ep; ++ep)
    {
        /* what is the state of this dependency? */
        P_PDCHART_DEP itdep;

        itdep = pdchart_listed_deps_search_key(ep->itdepkey);
#endif

        p_ss_doc = ev_p_ss_doc_from_docno(itdep->rng.s.docno);
        assert(p_ss_doc);

        if(p_ss_doc  &&  p_ss_doc->is_docu_thunk)
        {
            /* not loaded yet so attempt to load it and its friends */
            DOCNO old_docno = current_docno();
            TCHARZ filename[BUF_MAX_PATHSTRING];
            LOAD_FILE_OPTIONS load_file_options;
            STATUS status;

            name_make_wholename(&p_ss_doc->docu_name, filename, elemof32(filename));

            if((status = find_filetype_option(filename, FILETYPE_UNDETERMINED)) > 0)
            {
                zero_struct(load_file_options);
                load_file_options.document_name = filename;
                load_file_options.filetype_option = (char) status;
                (void) loadfile_recurse(filename, &load_file_options); /* only BOOL anyway */
            }

            else if(status == 0)
            {
                status = create_error(FILE_ERR_NOTFOUND);

                messagef(string_lookup(status), filename);
            }

            select_document_using_docno(old_docno);
        }
    }
#endif

    return(1);
}

_Check_return_
extern STATUS
pdchart_preferred_save(
    P_U8 filename)
{
    if(gr_chart_preferred_query())
        return(gr_chart_preferred_save(filename));

    return(STATUS_OK);
}

/* SKS after PD 4.12 26mar92 - something like this was needed... */

static void
pdchart_select_something(
    S32 iff_only_one)
{
    P_PDCHART_LISTED_DATA pdchartdata;
    LIST_ITEMNO           pdchartdatakey;

    if(!iff_only_one /*|| (list_numitem(pdchart_listed_data) <= 1)*/)
    {
        if((pdchartdata = collect_first(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey)) != NULL)
            pdchart_current = pdchartdata->pdchart;
        else
            pdchart_current = NULL;
    }
}

/******************************************************************************
*
* 'Charts' submenu
*
******************************************************************************/

static menu          cl_submenu         = NULL;
static P_LIST_ITEMNO cl_submenu_array   = NULL;
static U32           cl_submenu_array_n = 0;

#define mo_cl_dynamic  1

#define cl_submenu_MAXWIDTH 96

/*
kill off the lists of Charts
*/

static void
pdchart_submenu_kill(void)
{
    menu_dispose(&cl_submenu, 0);
    cl_submenu = NULL;
}

extern BOOL
pdchart_submenu_maker(void)
{
    if(!event_is_menu_being_recreated())
    {
        trace_0(TRACE_APP_PD4, "dispose of old & then create new charts menu list");

        pdchart_submenu_kill();
    }

    if(!cl_submenu)
    {
        P_PDCHART_LISTED_DATA pdchartdata;
        LIST_ITEMNO           pdchartdatakey;
        U32                   i;
        U8                    cwd_buffer[BUF_MAX_PATHSTRING];
        size_t                cwd_len;

        i = 0;

        for(pdchartdata = collect_first(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey);
            pdchartdata;
            pdchartdata = collect_next( PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey))
            ++i;

        if(i == 0)
            return(1);

        /* enlarge array as needed */
        if(i > cl_submenu_array_n)
        {
            STATUS status;
            P_ANY ptr;

            if(NULL == (ptr = _al_ptr_realloc(cl_submenu_array,
                                              i * sizeof32(cl_submenu_array[0]), &status)))
                reperr_null(status);

            if(ptr)
            {
                cl_submenu_array   = ptr;
                cl_submenu_array_n = i;
            }
        }

        i = 0;

        /* note cwd for minimalist references to charts from current document */
        cwd_len = (is_current_document() && (NULL != file_get_cwd(cwd_buffer, elemof32(cwd_buffer), currentfilename)))
                        ? strlen(cwd_buffer)
                        : 0;

        for(pdchartdata = collect_first(PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey);
            pdchartdata;
            pdchartdata = collect_next( PDCHART_LISTED_DATA, &pdchart_listed_data.lbr, &pdchartdatakey))
        {
            U8Z buffer[BUF_MAX_PATHSTRING];
            P_U8 ptr;
            U8Z tempstring[BUF_MAX_PATHSTRING];
            U32 len;

            if(i >= cl_submenu_array_n)
                break;

            gr_chart_name_query(&pdchartdata->pdchart->ch, buffer, sizeof32(buffer)-1);
            ptr = buffer;

            if(cwd_len && (0 == C_strnicmp(cwd_buffer, ptr, cwd_len)))
                ptr += cwd_len;

            len = strlen32(ptr);

            if(len > cl_submenu_MAXWIDTH)
            {
                xstrkpy(tempstring, elemof32(tempstring), iw_menu_prefix);
                xstrkat(tempstring, elemof32(tempstring), ptr + (len - cl_submenu_MAXWIDTH) + strlen32(iw_menu_prefix));
            }
            else
                xstrkpy(tempstring, elemof32(tempstring), ptr);

            /* create/extend menu 'unparsed' cos buffer may contain ',' */
            if(cl_submenu == NULL)
            {
                if((cl_submenu = menu_new_unparsed(Chart_STR, tempstring)) != NULL)
                    cl_submenu_array[i++] = pdchartdatakey;
            }
            else
            {
                menu_extend_unparsed(&cl_submenu, tempstring);
                cl_submenu_array[i++] = pdchartdatakey;
            }
        }

        /* we cannot submenu here as it's all in flux due to short/long menuism */
    }

    return(1);
}

extern void
pdchart_submenu_show(
    BOOL submenurequest)
{
    S32 x, y;

    if(!cl_submenu)
        return;

    event_read_submenupos(&x, &y);

    (submenurequest
            ? event_create_submenu
            : event_create_menu)
        (menu_syshandle(cl_submenu), x, y);
}

extern void
pdchart_submenu_select_from(
    _InVal_     S32 hit)
{
    if(0 != hit)
        pdchart_select_using_handle((P_ANY) cl_submenu_array[hit - mo_cl_dynamic]);
}

/******************************************************************************
*
* select chart from an external handle
* called by numbers.c draw_cache_file
* on a successful non-preferred load
*
******************************************************************************/

extern void
pdchart_select_using_handle(
    P_ANY epdchartdatakey)
{
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;

    pdchartdatakey = (LIST_ITEMNO) epdchartdatakey;

    pdchartdata = pdchart_listed_data_search_key(pdchartdatakey);

    if(pdchartdata)
        pdchart_current = pdchartdata->pdchart;
    else
        pdchart_current = NULL;
}

/*
explicit callbacks
*/

_Check_return_
extern BOOL
gr_chart_preferred_get_name(
    P_U8 buffer,
    U32 bufsiz)
{
    /* Preferred should be saved in <Choices$Write>.PipeDream directory */
    (void) add_choices_write_prefix_to_name_using_dir(buffer, bufsiz, PREFERRED_FILE_STR, NULL);

    return(1);
}

/******************************************************************************
*
* loading & shaving
*
******************************************************************************/

/******************************************************************************
*
* ensure that a given 'document' has a document number
*
******************************************************************************/

_Check_return_
static EV_DOCNO
ev_docno_ensure_for_pdchart_bodge(void)
{
    TCHARZ path_buffer[BUF_EV_LONGNAMLEN];
    TCHARZ leaf_buffer[BUF_EV_LONGNAMLEN];
    DOCU_NAME docu_name;
    PCTSTR fullname = pdchart_docname_query();
    PCTSTR leafname = file_leafname(fullname);
    EV_DOCNO docno;

    name_init(&docu_name);

    xstrnkpy(path_buffer, elemof32(path_buffer), fullname, leafname - fullname);
    docu_name.path_name = path_buffer;

    xstrkpy(leaf_buffer, elemof32(leaf_buffer), leafname);
    docu_name.leaf_name = leaf_buffer;

    if(file_is_rooted(docu_name.path_name))
        docu_name.flags.path_name_supplied = 1;

    if(DOCNO_NONE != (docno = ev_doc_establish_doc_from_docu_name(&docu_name)))
        ev_p_ss_doc_from_docno_must(docno)->is_docu_thunk = FALSE; /* have to bodge this otherwise we get deleted when establishing our dependents */

    return(docno);
}

enum PDCHART_CONSTRUCT_TABLE_OFFSETS
{
    PDCHART_CON_RANGE_COL = 0,
    PDCHART_CON_RANGE_ROW,
    PDCHART_CON_RANGE_TXT,
    PDCHART_CON_VERSION,

    /* SKS after PD 4.12 27mar92 - needed for live text object ordering on reload */
    PDCHART_CON_RANGE_TXT_ORDER,

    PDCHART_CON_N_TABLE_OFFSETS
};

static GR_CONSTRUCT_TABLE_ENTRY
pdchart_construct_table[PDCHART_CON_N_TABLE_OFFSETS + 1 /*end marker*/] =
{
    gr_contab_entry("PCC,*,",  PDCHART_CON_RANGE_COL),
    gr_contab_entry("PCR,*,",  PDCHART_CON_RANGE_ROW),
    gr_contab_entry("PCT,*,",  PDCHART_CON_RANGE_TXT),
    gr_contab_entry("PCV,*,",  PDCHART_CON_VERSION),
    gr_contab_entry("PCTO,*,", PDCHART_CON_RANGE_TXT_ORDER),

    gr_contab_entry_last
};

static const PC_U8
pdchart_range_flags_fmt = "&" U16_XFMT ","; /* flags leadin sequence for ranges */

static P_PDCHART_HEADER
pdchart_load_save_pdchart;

static EV_DOCNO
pdchart_load_save_docno;

static U8
pdchart_load_save_docname[BUF_MAX_PATHSTRING];

static S32
pdchart_make_range_flags_for_save(
    P_U8 buffer /*out*/,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_U16 p_u16)
{
    return(xsnprintf(buffer, elemof_buffer, pdchart_range_flags_fmt, *p_u16));
}

static S32
pdchart_make_range_for_save(
    P_U8 buffer /*out*/,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_EV_RANGE rng)
{
    UNREFERENCED_PARAMETER_InVal_(elemof_buffer);
    return(ev_dec_range(buffer, pdchart_load_save_docno, rng, 1));
}

PROC_QSORT_PROTO(static, pdchart_sort_list, PDCHART_SORT_ELEM)
{
    P_PDCHART_SORT_ELEM slip1 = (P_PDCHART_SORT_ELEM) _arg1;
    P_PDCHART_SORT_ELEM slip2 = (P_PDCHART_SORT_ELEM) _arg2;

    /* NB no sb global register furtling required */

    return(slip1->gr_order_no > slip2->gr_order_no);
}

_Check_return_
static STATUS
pdchart_save_external_core(
    FILE_HANDLE f,
    GR_CHART_HANDLE ch,
    P_ANY ext_handle)
{
    P_PDCHART_LISTED_DATA pdchartdata;
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_HEADER      pdchart;
    P_PDCHART_ELEMENT     ep, last_ep;
    U32                   sort_ix;
    P_PDCHART_SORT_ELEM   sort_list;
    EV_RANGE rng;
    P_U16  p_bits;
    STATUS res;

    UNREFERENCED_PARAMETER(ch);

    pdchartdatakey = (LIST_ITEMNO) ext_handle;

    pdchartdata = pdchart_listed_data_search_key(pdchartdatakey);
    assert(pdchartdata);

    pdchart = pdchartdata->pdchart;

    /*
    build a list for sorting into usage order
    */

    if(NULL == (sort_list = al_ptr_alloc_elem(PDCHART_SORT_ELEM, pdchart->elem.n, &res)))
        return(res);

    sort_ix = 0;
    ep      = &pdchart->elem.base[0];

    for(last_ep = ep + pdchart->elem.n; ep < last_ep; ++ep)
        switch(ep->type)
        {
        default:
            continue;

        case PDCHART_RANGE_COL:
        case PDCHART_RANGE_ROW:
            sort_list[sort_ix].ep          = ep;
            sort_list[sort_ix].gr_order_no = gr_chart_order_query(&pdchart->ch, &ep->gr_int_handle);
            ++sort_ix;
            break;
        }

    res = 1;

    if(sort_ix)
    {
        U32 stt_ix;

        qsort(sort_list, sort_ix, sizeof(*sort_list), pdchart_sort_list);

        /* output contiguous ranges of things. this ignores the current state
         * of the chart wrt. DEAD_COLs and DEAD_ROWs etc. but what the hell
        */

        stt_ix = 0;

        do  {
            P_PDCHART_ELEMENT stt_ep;
            EV_COL           try_col;
            U32               cur_ix;

            stt_ep = sort_list[stt_ix].ep;
            try_col = stt_ep->rng.col.col;
            cur_ix = stt_ix;

            for(;;)
            {
                P_PDCHART_ELEMENT cur_ep;

                cur_ep = sort_list[++cur_ix].ep;

                ++try_col;

                if(cur_ix >= sort_ix)
                    break;

                if(cur_ep->type != stt_ep->type)
                    /* output what we've got so far then (stt_ix..cur_ix i.e.) */
                    break;

                if(cur_ep->rng.col.col != try_col)
                    /* output what we've got so far then (stt_ix..cur_ix i.e.) */
                    break;
            }

            /* output what we've got so far then (stt_ix..cur_ix i.e.) */
            {
            U8   buffer[LIN_BUFSIZ];
            P_U8 out;
            U16  contab_ix;
            P_PDCHART_DEP itdep;

            rng.s.docno = stt_ep->rng.col.docno;
            rng.e.docno = rng.s.docno;

            rng.s.flags = rng.e.flags = 0;

            if(stt_ep->type == PDCHART_RANGE_COL)
            {
                rng.s.col = stt_ep->rng.col.col;
                rng.s.row = stt_ep->rng.col.stt_row;
                rng.e.col = rng.s.col + (cur_ix - stt_ix);
                rng.e.row = stt_ep->rng.col.end_row;

                contab_ix = PDCHART_CON_RANGE_COL;
            }
            else
            {
                rng.s.col = stt_ep->rng.row.stt_col;
                rng.s.row = stt_ep->rng.row.row;
                rng.e.col = stt_ep->rng.row.end_col;
                rng.e.row = rng.s.row + (cur_ix - stt_ix);

                contab_ix = PDCHART_CON_RANGE_ROW;
            }

            itdep = pdchart_listed_dep_search_key(stt_ep->itdepkey);
            assert(itdep);
            p_bits = (P_U16) &itdep->bits;

            out  = buffer;
            out += pdchart_make_range_flags_for_save(out, elemof32(buffer), p_bits);
            /*out +=*/
            (void) pdchart_make_range_for_save(out, elemof32(buffer) - (out - buffer), &rng);

            status_break(res = gr_chart_construct_save_frag_stt(f, contab_ix));

            status_break(res = gr_chart_construct_save_frag_txt(f, buffer));

            status_break(res = gr_chart_construct_save_frag_end(f));
            }

            stt_ix = cur_ix;
        }
        while(stt_ix < sort_ix);
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&sort_list));

    status_return(res);

    /*
    live text objects - separately done for neatness
    */

    ep = &pdchart->elem.base[0];

    for(last_ep = ep + pdchart->elem.n; ep < last_ep; ++ep)
    {
        if(ep->type == PDCHART_RANGE_TXT)
        {
            U8 buffer[LIN_BUFSIZ];
            P_U8 out;
            P_PDCHART_DEP itdep;

            rng.s.docno = ep->rng.txt.docno;
            rng.s.col = ep->rng.txt.col;
            rng.s.row = ep->rng.txt.row;

            rng.e.docno = rng.s.docno;
            rng.e.col = rng.s.col + 1;
            rng.e.row = rng.s.row + 1;

            rng.s.flags = rng.e.flags = 0;

            itdep = pdchart_listed_dep_search_key(ep->itdepkey);
            assert(itdep);
            p_bits = (P_U16) &itdep->bits;

#if 1
            /* SKS after PD 4.12 27mar92 - needed for (compatible) live text object ordering on reload */
            consume_int(sprintf(buffer, "%d", gr_chart_order_query(&pdchart->ch, &ep->gr_int_handle)));

            status_break(res = gr_chart_construct_save_frag_stt(f, PDCHART_CON_RANGE_TXT_ORDER));

            status_break(res = gr_chart_construct_save_frag_txt(f, buffer));

            status_break(res = gr_chart_construct_save_frag_end(f));
#endif

            out  = buffer;
            out += pdchart_make_range_flags_for_save(out, elemof32(buffer), p_bits);
            /*out +=*/
            (void) pdchart_make_range_for_save(out, elemof32(buffer) - (out - buffer), &rng);

            status_break(res = gr_chart_construct_save_frag_stt(f, PDCHART_CON_RANGE_TXT));

            status_break(res = gr_chart_construct_save_frag_txt(f, buffer));

            status_break(res = gr_chart_construct_save_frag_end(f));
        }
    }

    status_return(res);

    return(1);
}

/******************************************************************************
*
* direct callback from chart module to save out any constructs from our side
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_save_external(
    GR_CHART_HANDLE ch,
    FILE_HANDLE f,
    P_U8 save_filename /*const*/,
    P_ANY ext_handle)
{
    STATUS res;

    /* set up as document name */
    xstrkpy(pdchart_load_save_docname, elemof32(pdchart_load_save_docname), save_filename);

    /* register our construct table with chart module */
    gr_chart_construct_table_register(pdchart_construct_table, PDCHART_CON_N_TABLE_OFFSETS);

    /* make the chart a document for relative extref saves */
    if(DOCNO_NONE == (pdchart_load_save_docno = ev_docno_ensure_for_pdchart_bodge()))
        return(create_error(status_nomem()));

    { /* SKS after PD 4.12 26mar92 - keep consistent with save_version_string */
    U8 array[LIN_BUFSIZ];

    xstrkpy(array, elemof32(array), applicationversion);
    xstrkat(array, elemof32(array), ", ");

    xstrkat(array, elemof32(array), !str_isblank(user_id()) ? user_id() : "Colton Software");
    if(!str_isblank(user_organ_id()))
    {
        xstrkat(array, elemof32(array), " - ");
        xstrkat(array, elemof32(array), user_organ_id());
    }
    xstrkat(array, elemof32(array), ", R");

    xstrkat(array, elemof32(array), registration_number());

    res = gr_chart_construct_save_txt(f, PDCHART_CON_VERSION, array);
    }

    if(status_done(res))
    {
        if(ext_handle)
            res = pdchart_save_external_core(f, ch, ext_handle);

        /* otherwise it's doing a preferred save; we don't have anything more to say */
    }

    /* kill name prior to close */
    pdchart_load_save_docname[0] = CH_NULL;

    ev_p_ss_doc_from_docno_must(pdchart_load_save_docno)->is_docu_thunk = TRUE; /* bodge back - we aren't a real document */

    ev_close_sheet((DOCNO) pdchart_load_save_docno);

    pdchart_load_save_docno = DOCNO_NONE;

    status_return(res);

    return(1);
}

/******************************************************************************
*
* reloading
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern char *
pdchart_docname_query(void)
{
    return(pdchart_load_save_docname);
}

static P_U8
pdchart_read_range_flags(
    P_U8 args,
    P_U16 p_u16 /*out*/)
{
    U8  format[64];
    int len;

    /* wot a bummer */
    xstrkpy(format, elemof32(format), pdchart_range_flags_fmt);
    xstrkat(format, elemof32(format), "%n");

    if(sscanf(args, format, p_u16, &len) < 1)
        return(NULL);

    return(args + len);
}

/******************************************************************************
*
* read a range out of the file
*
******************************************************************************/

static P_U8
pdchart_read_range(
    P_U8 args,
    _OutRef_    P_EV_RANGE rng /*out*/)
{
    P_U8 ptr = args;
    S32  len;

    len = ev_doc_establish_doc_from_name(ptr, pdchart_load_save_docno, &rng->s.docno);
    if(!len)
        return(NULL);

    ptr += len;

    rng->e.docno = rng->s.docno;

    ptr += stox(ptr, &rng->s.col);

    rng->s.row = (S32) strtol(ptr, &ptr, 10);
    --rng->s.row;

    rng->e.col = rng->s.col;
    rng->e.row = rng->s.row;

    len = stox(ptr, &rng->e.col);

    if(len)
    {
        ptr += len;

        rng->e.row = (S32) strtol(ptr, &ptr, 10);
        --rng->e.row;
    }

    ++rng->e.col;
    ++rng->e.row;

    return(ptr);
}

_Check_return_
extern STATUS
gr_ext_construct_load_this(
    GR_CHART_HANDLE ch,
    P_U8 args,
    U16 contab_ix)
{
    P_U8   ptr = args;
    U16    bits = 0;
    EV_RANGE rng;
    STATUS res;

    UNREFERENCED_PARAMETER(ch);

    zero_struct(rng);

    switch(contab_ix)
    {
    case PDCHART_CON_VERSION:
        /* ignore */
        break;

    case PDCHART_CON_RANGE_COL:
        ptr = pdchart_read_range_flags(ptr, &bits);
        if(ptr)
        {
            ptr = pdchart_read_range(ptr, &rng);
            if(ptr)
            {
                /* cater for the PipeDream side of the chart descriptor growing */
                res = pdchart_element_ensure(pdchart_load_save_pdchart, rng.e.col - rng.s.col);
                status_return(res);

                res = pdchart_add_range_for_load(pdchart_load_save_pdchart, PDCHART_RANGE_COL, &rng, bits);
                status_return(res);
            }
        }
        break;

    case PDCHART_CON_RANGE_ROW:
        ptr = pdchart_read_range_flags(ptr, &bits);
        if(ptr)
        {
            ptr = pdchart_read_range(ptr, &rng);
            if(ptr)
            {
                /* cater for the PipeDream side of the chart descriptor growing */
                res = pdchart_element_ensure(pdchart_load_save_pdchart, rng.e.row - rng.s.row);
                status_return(res);

                res = pdchart_add_range_for_load(pdchart_load_save_pdchart, PDCHART_RANGE_ROW, &rng, bits);
                status_return(res);
            }
        }
        break;

    case PDCHART_CON_RANGE_TXT:
        /* a live text object is the simplest thing for us to reload */
        ptr = pdchart_read_range_flags(ptr, &bits);
        if(ptr)
        {
            ptr = pdchart_read_range(ptr, &rng);
            if(ptr)
            {
                /* cater for the PipeDream side of the chart descriptor growing */
                res = pdchart_element_ensure(pdchart_load_save_pdchart, 1);
                status_return(res);

                res = pdchart_text_add(pdchart_load_save_pdchart, &rng);
                status_return(res);
            }
        }
        break;

#if 1
    /* SKS after PD 4.12 27mar92 - live text objects not as easy as originally claimed */
    case PDCHART_CON_RANGE_TXT_ORDER:
        {
        S32 order;

        if(sscanf(args, "%d", &order) < 1)
            return(1);

        gr_chart_text_order_set(&pdchart_load_save_pdchart->ch, order);
        }
        break;
#endif
    }

    return(1);
}

image_cache_tagstrip_proto(extern, pdchart_tagstrip)
{
    P_PDCHART_TAGSTRIP_INFO p_pdchart_tagstrip_info = (P_PDCHART_TAGSTRIP_INFO) handle;
    STATUS res;

    p_pdchart_tagstrip_info->pdchartdatakey = NULL;

    if((res = pdchart_new(&pdchart_load_save_pdchart, 0, 0, 0)) < 0)
    {
        reperr_null(res);
        return(1);
    }

    /* name needed for doc creation */
    image_cache_name_query(&p_image_cache_tagstrip_info->image_cache_handle, pdchart_load_save_docname, sizeof(pdchart_load_save_docname)-1);

    /* set the name up in the newly created chart */
    if(status_fail(res = gr_chart_name_set(&pdchart_load_save_pdchart->ch, pdchart_load_save_docname)))
        return(reperr_null(res), 1);

    /* register our construct table with chart module */
    gr_chart_construct_table_register(pdchart_construct_table, PDCHART_CON_N_TABLE_OFFSETS);

    if(DOCNO_NONE == (pdchart_load_save_docno = ev_docno_ensure_for_pdchart_bodge()))
        return(status_nomem());

    /* get chart module to process constructs for both it and us */
    res = gr_chart_construct_tagstrip_process(&pdchart_load_save_pdchart->ch, p_image_cache_tagstrip_info);

    p_pdchart_tagstrip_info->pdchartdatakey = (P_ANY) pdchart_load_save_pdchart->pdchartdatakey;

    return(res);
}

image_cache_tagstrip_proto(extern, pdchart_tagstrip_legacy)
{
    return(pdchart_tagstrip(p_image_cache_tagstrip_info, handle));
}

/******************************************************************************
*
* we must close our 'sheet' down after dependents have been loaded
*
******************************************************************************/

extern void
pdchart_load_ended(
    P_ANY epdchartdatakey,
    S32 loaded_preferred)
{
    LIST_ITEMNO           pdchartdatakey;
    P_PDCHART_LISTED_DATA pdchartdata;

    assert(pdchart_load_save_docno != DOCNO_NONE);

    /* kill name prior to close */
    pdchart_load_save_docname[0] = CH_NULL;

    ev_p_ss_doc_from_docno_must(pdchart_load_save_docno)->is_docu_thunk = TRUE; /* bodge back - we aren't a real document */

    ev_close_sheet((DOCNO) pdchart_load_save_docno);

    pdchart_load_save_docno = DOCNO_NONE;

    pdchartdatakey = (LIST_ITEMNO) epdchartdatakey;

    pdchartdata = pdchart_listed_data_search_key(pdchartdatakey);
    assert(pdchartdata);

    if(pdchartdata)
    {
        P_PDCHART_HEADER pdchart = pdchartdata->pdchart;

        if(loaded_preferred)
        {
            gr_chart_preferred_set(&pdchart->ch);

            pdchart_dispose(&pdchart);
        }
        else
        {
            /* ensure recalced sometime */
            status_assert(pdchart_modify(pdchart));
        }
    }
}

/*
required functions
*/

extern FILETYPE_RISC_OS
gr_chart_save_as_filetype(void)
{
    return((d_progvars[OR_CF].option) ? FILETYPE_PD_CHART : FILETYPE_PIPEDREAM);
}

/* end of pdchart.c */
