/* ev_uref.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Routines that provide the evaluator uref service:
 * this updates references in the tree and in compiled expressions
 */

/* MRJC February 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

static S32
match_slr_e(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp);

static void
rng_set_span(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InRef_     PC_UREF_PARM upp,
    _OutRef_    P_S32 colspan,
    _OutRef_    P_S32 rowspan);

static S32
uref_cdoc(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_EV_SLR to);

static S32
uref_swap(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_EV_SLR slrp1,
    _InRef_     PC_EV_SLR slrp2);

static S32
uref_swapslot(
    _OutRef_    P_EV_SLR ref,
    _InRef_     PC_EV_SLR slrp);

static S32
uref_uref(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp,
    S32 copy);

/******************************************************************************
*
* check if the given range matches the uref in operation; if it does then update it
*
* <0 range matched and went bad - delete entry
* =0 range didn't match
* >0 range matched and was updated OK
*
******************************************************************************/

extern S32
ev_match_rng(
    _InoutRef_  P_EV_RANGE p_ev_range,
    _InRef_     PC_UREF_PARM upp)
{
    S32 res;

    res = DEP_NONE;
    switch(upp->action)
        {
        case UREF_ADJUST:
        case UREF_COPY:
            ev_match_slr(&p_ev_range->s, upp);
            match_slr_e (&p_ev_range->e, upp);
            res = DEP_UPDATE;
            break;

        case UREF_UREF:
            if(!(p_ev_range->s.flags & SLR_ALL_REF))
                {
                S32 colspan, rowspan, res_s, res_e, do_uref;

                rng_set_span(p_ev_range, upp, &colspan, &rowspan);

                /* if all cols and rows spanned, we can move anywhere;
                   if all cols spanned and only moving up or down, OK;
                   if all rows spanned and olny moving left or right, OK
                */
                do_uref = 0;
                if(colspan && rowspan)
                    do_uref = 1;
                else if(colspan && upp->slr3.row && !upp->slr3.col)
                    do_uref = 1;
                else if(rowspan && upp->slr3.col && !upp->slr3.row)
                    do_uref = 1;

                res_s = res_e = DEP_NONE;
                if(do_uref)
                    {
                    res_s = ev_match_slr(&p_ev_range->s, upp);
                    res_e = match_slr_e (&p_ev_range->e, upp);
                    }

                if(res_s == DEP_DELETE ||
                   res_e == DEP_DELETE)
                    res = DEP_DELETE;
                else if(res_s != DEP_NONE ||
                        res_e != DEP_NONE)
                    res = DEP_UPDATE;

                /* if the area affected and the range intersect
                 * at all, then the dependents must be told
                 */
                if(res == DEP_NONE)
                    {
                    if((upp->slr1.docno == p_ev_range->s.docno) &&
                       (upp->slr2.col >  p_ev_range->s.col) &&
                       (upp->slr1.col <  p_ev_range->e.col) &&
                       (upp->slr2.row >  p_ev_range->s.row) &&
                       (upp->slr1.row <  p_ev_range->e.row) )
                        res = DEP_INFORM;
                    }
                }
            break;

        case UREF_REPLACE:
        case UREF_DELETE:
            if(!(p_ev_range->s.flags & SLR_ALL_REF))
                {
                S32 colspan, rowspan;

                rng_set_span(p_ev_range, upp, &colspan, &rowspan);

                /* range completely deleted ? */
                if(colspan && rowspan)
                    {
                    if(upp->action != UREF_REPLACE)
                        {
                        p_ev_range->s.flags |= SLR_BAD_REF;
                        p_ev_range->e.flags |= SLR_BAD_REF;
                        }
                    res = DEP_DELETE;
                    break;
                    }

                if(colspan)
                    {
                    if(ev_match_slr(&p_ev_range->s, upp) == DEP_DELETE)
                        {
                        if(upp->action != UREF_REPLACE)
                            {
                            p_ev_range->s.row = upp->slr2.row;
                            p_ev_range->s.flags &= ~SLR_BAD_REF;
                            }
                        res = DEP_UPDATE;
                        }
                    else if(match_slr_e(&p_ev_range->e, upp) == DEP_DELETE)
                        {
                        if(upp->action != UREF_REPLACE)
                            {
                            p_ev_range->e.row = upp->slr1.row;
                            p_ev_range->e.flags &= ~SLR_BAD_REF;
                            }
                        res = DEP_UPDATE;
                        }
                    }
                else if(rowspan)
                    {
                    if(ev_match_slr(&p_ev_range->s, upp) == DEP_DELETE)
                        {
                        if(upp->action != UREF_REPLACE)
                            {
                            p_ev_range->s.col = upp->slr2.col;
                            p_ev_range->s.flags &= ~SLR_BAD_REF;
                            }
                        res = DEP_UPDATE;
                        }
                    else if(match_slr_e(&p_ev_range->e, upp) == DEP_DELETE)
                        {
                        if(upp->action != UREF_REPLACE)
                            {
                            p_ev_range->e.col = upp->slr1.col;
                            p_ev_range->e.flags &= ~SLR_BAD_REF;
                            }
                        res = DEP_UPDATE;
                        }
                    }
                }

            /* if the area affected and the range intersect
             * at all, then the dependents must be told
             */
            if(res == DEP_NONE)
                {
                if((upp->slr1.docno == p_ev_range->s.docno) &&
                   (upp->slr2.col >  p_ev_range->s.col) &&
                   (upp->slr1.col <  p_ev_range->e.col) &&
                   (upp->slr2.row >  p_ev_range->s.row) &&
                   (upp->slr1.row <  p_ev_range->e.row) )
                    res = DEP_INFORM;
                }

            break;

        case UREF_SWAP:
            if(!(p_ev_range->s.flags & SLR_ALL_REF))
                {
                S32 colspan, rowspan, res_s, res_e;

                rng_set_span(p_ev_range, upp, &colspan, &rowspan);

                /* slr1/2 contain row numbers, not an area */
                if(p_ev_range->e.row == p_ev_range->s.row + 1 &&
                   (p_ev_range->s.row == upp->slr1.row ||
                    p_ev_range->s.row == upp->slr2.row))
                    rowspan = 1;

                /* check if the range is being moved wholesale */
                if(colspan && rowspan)
                    {
                    res_s = ev_match_slr(&p_ev_range->s, upp);
                    res_e = match_slr_e (&p_ev_range->e, upp);

                    if(res_s == DEP_DELETE || res_e == DEP_DELETE)
                        res = DEP_DELETE;
                    else
                        res = DEP_UPDATE;
                    }
                else
                    {
                    /* tell people who have ranges affected at all */
                    EV_RANGE rng;

                    rng.s = upp->slr1;
                    rng.e = upp->slr2;
                    rng.e.row = upp->slr1.row;

                    if(ev_range_overlap(p_ev_range, &rng))
                        res = DEP_INFORM;
                    else
                        {
                        rng.s.row = rng.e.row = upp->slr2.row;
                        if(ev_range_overlap(p_ev_range, &rng))
                            res = DEP_INFORM;
                        }
                    }
                }
            else
                res = DEP_INFORM;

            break;

        case UREF_CHANGEDOC:
            if(p_ev_range->s.docno == upp->slr2.docno)
                {
                uref_cdoc(&p_ev_range->s, &upp->slr1);
                res = uref_cdoc(&p_ev_range->e, &upp->slr1);
                }
            break;

        case UREF_REDRAW:
        case UREF_CHANGE:
            if(ev_slr_in_range(p_ev_range, &upp->slr1))
                res = DEP_INFORM;
            break;

        /* (range may be a slot after all) */
        case UREF_SWAPSLOT:
            if(!(p_ev_range->s.flags & SLR_ALL_REF))
                {
                /* check if the whole range can be moved */
                if(p_ev_range->e.col == p_ev_range->s.col + 1 &&
                   p_ev_range->e.row == p_ev_range->s.row + 1)
                    {
                    if(slr_equal(&p_ev_range->s, &upp->slr1))
                        {
                        p_ev_range->s = upp->slr2;
                        res = DEP_UPDATE;
                        }
                    else if(slr_equal(&p_ev_range->s, &upp->slr2))
                        {
                        p_ev_range->s = upp->slr1;
                        res = DEP_UPDATE;
                        }

                    if(res == DEP_UPDATE)
                        {
                        p_ev_range->e = p_ev_range->s;
                        p_ev_range->e.col += 1;
                        p_ev_range->e.row += 1;
                        }
                    }
                }

            /* tell people who have ranges affected */
            if(res != DEP_UPDATE &&
               (ev_slr_in_range(p_ev_range, &upp->slr1) ||
                ev_slr_in_range(p_ev_range, &upp->slr2)))
                res = DEP_INFORM;
            break;

        case UREF_CLOSE:
            if(p_ev_range->s.docno == upp->slr1.docno)
                res = DEP_DELETE;
            break;

        case UREF_RENAME:
            if(p_ev_range->s.docno == upp->slr1.docno)
                res = DEP_INFORM;
            break;
        }

    return(res);
}

/******************************************************************************
*
* check if the given slr matches the uref in operation; if it does then update it
*
* returns DEP codes to indicate action required
*
******************************************************************************/

extern S32
ev_match_slr(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp)
{
    S32 res = DEP_NONE;

    switch(upp->action)
        {
        case UREF_COPY:
            res = uref_uref(ref, upp, TRUE);
            break;

        case UREF_ADJUST:
            if(ref->docno == upp->slr2.docno)
                res = uref_uref(ref, upp, FALSE);
            else if(ref->docno == DOCNO_NONE) /* <<< how would we represent this in the new scheme? */
                {
                ref->docno = upp->slr3.docno;
                ref->flags |= SLR_BAD_REF;
                res = DEP_UPDATE;
                }
            break;

        case UREF_UREF:
            if(ref->row >= upp->slr1.row &&
               ref->col >= upp->slr1.col &&
               ref->row <  upp->slr2.row &&
               ref->col <  upp->slr2.col &&
               (ref->docno == upp->slr2.docno))
                res = uref_uref(ref, upp, FALSE);
            break;

        case UREF_DELETE:
        case UREF_REPLACE:
            if(ref->row >= upp->slr1.row &&
               ref->col >= upp->slr1.col &&
               ref->row <  upp->slr2.row &&
               ref->col <  upp->slr2.col &&
               (ref->docno == upp->slr2.docno))
                 {
                 if(upp->action != UREF_REPLACE)
                     ref->flags |= SLR_BAD_REF;
                 res = DEP_DELETE;
                 }
             break;

        case UREF_SWAP:
            if((ref->row == upp->slr1.row  ||
                ref->row == upp->slr2.row) &&
               (ref->col >= upp->slr1.col  &&
                ref->col <  upp->slr2.col) &&
               (ref->docno == upp->slr2.docno)
              )
                res = uref_swap(ref, &upp->slr1, &upp->slr2);
            break;

        case UREF_CHANGEDOC:
            if(ref->docno == upp->slr2.docno)
                res = uref_cdoc(ref, &upp->slr1);
            break;

        case UREF_REDRAW:
        case UREF_CHANGE:
            if(slr_equal(ref, &upp->slr1))
                res = DEP_INFORM;
            break;

        case UREF_SWAPSLOT:
            if(slr_equal(ref, &upp->slr1))
                res = uref_swapslot(ref, &upp->slr2);
            else if(slr_equal(ref, &upp->slr2))
                res = uref_swapslot(ref, &upp->slr1);
            break;

        case UREF_CLOSE:
            if(ref->docno == upp->slr1.docno)
                res = DEP_DELETE;
            break;

        case UREF_RENAME:
            if(ref->docno == upp->slr1.docno)
                res = DEP_INFORM;
            break;
        }

    return(res);
}

/******************************************************************************
*
* evaluator uref service
*
* the slr parameters have the following meaning:
*
* UREF_UREF
*   slr1, slr2 are tl, br of block being moved
*   slr3 has column and row +/- offsets
*
* UREF_DELETE
*   slr1, slr2 are tl, br of block being removed
*
* UREF_SWAP
*   slr1, slr2 contain row1, row2 to be swapped
*   slr1, slr2 contain column start and column end
*
* UREF_CHANGEDOC
*   slr1 contains target document number
*   slr2 contains source document number
*
* UREF_CHANGE
*   slr1 contains slot changed
*
* UREF_REPLACE
*   as UREF_DELETE; references to replaced slots
*   are not marked bad
*
* UREF_COPY
*   slr2 contains: source document
*   slr3 contains: target document
*                  column and row +/- offsets
*   should NEVER come in main EV_UREF entry, only
*   local calls to ev_match_slr/ev_match_rng
*   used in replicate/copy so that absolute bits
*   are be checked when updating a slot reference
*
* UREF_ADJUST
*   as UREF_COPY but no checking of absolute bits;
*   should NEVER come in main EV_UREF entry, only
*   local calls to ev_match_slr/ev_match_rng
*   result document number is checked for validity
*
* UREF_SWAPSLOT
*   slr1, slr2 contain slots to be swapped
*
* UREF_CLOSE
*   document slr1.docno is going away
*
* UREF_REDRAW
*   request for redraw; slr1 contains slot
*
* MRJC added 26.3.92
* UREF_RENAME
*   slr1.docno is being told about rename
*   slr2.docno is being renamed
*
******************************************************************************/

extern void
ev_uref(
    _InRef_     PC_UREF_PARM upp)
{
    trace_0(TRACE_MODULE_UREF, "ev_uref in -- ");

    #if TRACE_ALLOWED
        switch(upp->action)
            {
            case UREF_CHANGE:
                trace_0(TRACE_MODULE_UREF, "UREF_CHANGE ");
                break;
            case UREF_UREF:
                trace_0(TRACE_MODULE_UREF, "UREF_UREF ");
                break;
            case UREF_DELETE:
                trace_0(TRACE_MODULE_UREF, "UREF_DELETE ");
                break;
            case UREF_SWAP:
                trace_0(TRACE_MODULE_UREF, "UREF_SWAP ");
                break;
            case UREF_CHANGEDOC:
                trace_0(TRACE_MODULE_UREF, "UREF_CHANGEDOC ");
                break;
            case UREF_REPLACE:
                trace_0(TRACE_MODULE_UREF, "UREF_REPLACE ");
                break;
            case UREF_SWAPSLOT:
                trace_0(TRACE_MODULE_UREF, "UREF_SWAPSLOT ");
                break;
            case UREF_CLOSE:
                trace_0(TRACE_MODULE_UREF, "UREF_CLOSE ");
                break;
            case UREF_REDRAW:
                trace_0(TRACE_MODULE_UREF, "UREF_REDRAW ");
                break;
            case UREF_RENAME:
                trace_0(TRACE_MODULE_UREF, "UREF_RENAME ");
                break;
            default:
                assert(0);
                break;
            }
    #endif

    /* blow up the evaluator when things move under its feet */
    switch(upp->action)
        {
        case UREF_CHANGE:
        case UREF_REDRAW:
            break;
        case UREF_REPLACE:
        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
        case UREF_RENAME:
            stack_zap(upp);
            break;
        }

    /* look through the todo list */
    switch(upp->action)
        {
        P_TODO_ENTRY todop;

        case UREF_REDRAW:
        case UREF_CHANGE:
        case UREF_REPLACE:
        case UREF_RENAME:
            break;

        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            if((todop = todo_ptr(0)) != NULL)
                {
                EV_TRENT tix;

                for(tix = 0; tix < todotab.next; ++tix, ++todop)
                    {
                    S32 res;

                    if(todop->flags & TRF_TOBEDEL)
                        continue;

                    if((res = ev_match_slr(&todop->slr, upp)) == DEP_DELETE)
                        {
                        todop->flags  |= TRF_TOBEDEL;
                        todotab.flags |= TRF_TOBEDEL;
                        todotab.mindel = MIN(todotab.mindel, tix);
                        }
                    else if(res != DEP_INFORM)
                        todotab.sorted = 0;
                    }
                }
        }

    if(0 != docu_array_size)
        {
        EV_DOCNO docno;

        /* loop over documents */
        for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
            {
            P_SS_DOC p_ss_doc;
            extentp eep;
            P_SLR_USE sep;
            P_RANGE_USE rep;

            if(docno_void_entry(docno))
                continue;

            p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

            /* look through external dependencies */
            switch(upp->action)
                {
                case UREF_UREF:
                case UREF_DELETE:
                case UREF_SWAP:
                case UREF_CHANGEDOC:
                case UREF_CHANGE:
                case UREF_REDRAW:
                case UREF_REPLACE:
                case UREF_SWAPSLOT:
                case UREF_CLOSE:
                case UREF_RENAME:
                    if((eep = tree_extptr(p_ss_doc, 0)) != NULL)
                        {
                        EV_TRENT i;

                        for(i = 0; i < p_ss_doc->exttab.next; ++i, ++eep)
                            {
                            S32 res;

                            if(eep->flags & TRF_TOBEDEL)
                                continue;

                            if((res = ev_match_rng(&eep->refto, upp)) != DEP_NONE)
                                {
                                (eep->proc)(&eep->refto,
                                            upp,
                                            eep->exthandle,
                                            eep->inthandle,
                                            res);

                                /* refs to deleted area must be removed */
                                if(res == DEP_DELETE && upp->action != UREF_REPLACE)
                                    {
                                    eep->flags          |= TRF_TOBEDEL;
                                    p_ss_doc->exttab.flags |= TRF_TOBEDEL;
                                    p_ss_doc->exttab.mindel = MIN(p_ss_doc->exttab.mindel, i);
                                    continue;
                                    }
                                else
                                    doc_move_extref(p_ss_doc, docno, eep, i);
                                }
                            }
                        }
                    break;
                }

            /* look through the range tree */
            switch(upp->action)
                {
                case UREF_CHANGE:
                case UREF_REDRAW:
                case UREF_RENAME:
                    break;

                case UREF_UREF:
                case UREF_DELETE:
                case UREF_SWAP:
                case UREF_CHANGEDOC:
                case UREF_REPLACE:
                case UREF_SWAPSLOT:
                case UREF_CLOSE:
                    if((rep = tree_rngptr(p_ss_doc, 0)) != NULL)
                        {
                        EV_TRENT i;

                        for(i = 0; i < p_ss_doc->range_table.next; ++i, ++rep)
                            {
                            S32 res;

                            if(rep->flags & TRF_TOBEDEL)
                                continue;

                            /* refs contained by deleted area must be removed */
                            if(ev_match_slr(&rep->byslr, upp) == DEP_DELETE)
                                {
                                rep->flags          |= TRF_TOBEDEL;
                                p_ss_doc->range_table.flags |= TRF_TOBEDEL;
                                p_ss_doc->range_table.mindel = MIN(p_ss_doc->range_table.mindel, i);
                                continue;
                                }

                            /* if reference to matched, find the reference
                            in the compiled string and update that */
                            if((res = ev_match_rng(&rep->refto, upp)) != DEP_NONE)
                                {
                                P_EV_SLOT p_ev_slot;
                                S32 travel_res;

                                /* mark for recalc */
                                ev_todo_add_slr(&rep->byslr, TODO_SORT);

                                if(res != DEP_INFORM)
                                    {
                                    if(rep->byoffset >= 0 &&
                                       (travel_res = ev_travel(&p_ev_slot, &rep->byslr)) != 0)
                                        {
                                        /* internal format slots */
                                        if(travel_res > 0)
                                            {
                                            EV_RANGE temp_rng;

                                            read_range(  &temp_rng,
                                                         p_ev_slot->rpn.var.rpn_str +
                                                         rep->byoffset);
                                            ev_match_rng(&temp_rng, upp);
                                            write_rng   (&temp_rng,
                                                         p_ev_slot->rpn.var.rpn_str +
                                                         rep->byoffset);
                                            }
                                        /* external format slots */
                                        else
                                            ev_ext_uref(&rep->byslr,
                                                        rep->byoffset,
                                                        upp);
                                        }

                                    doc_move_rngref(p_ss_doc, docno, rep, i);

                                    p_ss_doc->range_table.sorted = 0;
                                    }
                                }
                            }
                        }
                    break;
                }

            /* look through slr tree */
            switch(upp->action)
                {
                case UREF_CHANGE:
                case UREF_REDRAW:
                case UREF_RENAME:
                    break;

                case UREF_UREF:
                case UREF_DELETE:
                case UREF_SWAP:
                case UREF_CHANGEDOC:
                case UREF_REPLACE:
                case UREF_SWAPSLOT:
                case UREF_CLOSE:
                    if((sep = tree_slrptr(p_ss_doc, 0)) != NULL)
                        {
                        EV_TRENT i;

                        for(i = 0; i < p_ss_doc->slr_table.next; ++i, ++sep)
                            {
                            S32 res;

                            if(sep->flags & TRF_TOBEDEL)
                                continue;

                            /* refs contained by deleted area
                             * must be removed
                            */
                            if(ev_match_slr(&sep->byslr, upp) == DEP_DELETE)
                                {
                                sep->flags          |= TRF_TOBEDEL;
                                p_ss_doc->slr_table.flags |= TRF_TOBEDEL;
                                p_ss_doc->slr_table.mindel = MIN(p_ss_doc->slr_table.mindel, i);
                                continue;
                                }

                            /* if ref_to matched, find the reference
                            in the compiled string and update that */
                            if((res = ev_match_slr(&sep->refto, upp)) != DEP_NONE)
                                {
                                P_EV_SLOT p_ev_slot;
                                S32 travel_res;

                                /* mark for recalc */
                                ev_todo_add_slr(&sep->byslr, TODO_SORT);

                                if(res != DEP_INFORM)
                                    {
                                    if(sep->byoffset >= 0 &&
                                       (travel_res = ev_travel(&p_ev_slot, &sep->byslr)) != 0)
                                        {
                                        /* internal format slots */
                                        if(travel_res > 0)
                                            {
                                            EV_SLR temp_slr;

                                            read_slr    (&temp_slr,
                                                         p_ev_slot->rpn.var.rpn_str +
                                                         sep->byoffset);
                                            ev_match_slr(&temp_slr, upp);
                                            write_slr   (&temp_slr,
                                                         p_ev_slot->rpn.var.rpn_str +
                                                         sep->byoffset);
                                            }
                                        /* external format slots */
                                        else
                                            ev_ext_uref(&sep->byslr,
                                                        sep->byoffset,
                                                        upp);
                                        }

                                    doc_move_slrref(p_ss_doc, docno, sep, i);

                                    p_ss_doc->slr_table.sorted = 0;
                                    }
                                }
                            }
                        }
                    break;
                }
            }
        }

    if(upp->action == UREF_CHANGEDOC)
        change_doc_mac_nam(upp->slr1.docno, upp->slr2.docno);

    /* look through the name use table */
    switch(upp->action)
        {
        P_NAME_USE nep;

        case UREF_CHANGE:
        case UREF_REDRAW:
        case UREF_RENAME:
            break;

        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_REPLACE:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            if((nep = tree_namptr(0)) != NULL)
                {
                EV_TRENT i;
                S32 check_use, un_sort;

                name_list_sort();
                for(i = 0, check_use = un_sort = 0; i < namtab.next; ++i, ++nep)
                    {
                    S32 res;

                    if(nep->flags & TRF_TOBEDEL)
                        continue;

                    /* refs contained by deleted area must be removed */
                    if((res = ev_match_slr(&nep->byslr, upp)) == DEP_DELETE)
                        {
                        nep->flags   |= TRF_TOBEDEL;
                        namtab.flags |= TRF_TOBEDEL;
                        namtab.mindel = MIN(namtab.mindel, i);
                        check_use = 1;
                        }
                    else if(res == DEP_UPDATE)
                        un_sort = 1;
                    }

                /* set these flags at end to avoid sorts
                 * in the middle of our fuddle
                 */
                if(check_use)
                    names_def.flags |= TRF_CHECKUSE;
                if(un_sort)
                    namtab.sorted = 0;
                }
            break;
        }

    /* look through name definition table */
    switch(upp->action)
        {
        P_EV_NAME p_ev_name;

        case UREF_CHANGE:
        case UREF_REDRAW:
        case UREF_RENAME:
            break;

        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_REPLACE:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            if((p_ev_name = name_ptr(0)) != NULL)
                {
                EV_NAMEID i;

                for(i = 0; i < names_def.next; ++i, ++p_ev_name)
                    {
                    if(p_ev_name->flags & TRF_TOBEDEL)
                        continue;

                    /* check for name definition in the area */
                    if(ev_match_slr(&p_ev_name->owner, upp) == DEP_DELETE)
                        {
                        /* delete name definition
                         * if there are no dependents
                         */
                        name_free_resources(p_ev_name);

                        if(!ev_todo_add_name_dependents(p_ev_name->key))
                            {
                            ev_p_ss_doc_from_docno_must(p_ev_name->owner.docno)->nam_ref_count -= 1;

                            p_ev_name->flags |= TRF_TOBEDEL;

                            names_def.flags |= TRF_TOBEDEL;
                            names_def.mindel = MIN(names_def.mindel, i);

                            trace_1(TRACE_MODULE_UREF, "uref deleting name: %s", p_ev_name->id);
                            }

                        /* it's definitely undefined now, anyway */
                        p_ev_name->flags |= TRF_UNDEFINED;
                        }
                    else
                        {
                        /* check for the definition of the name
                         * itself being in the uref area
                         */
                        switch(p_ev_name->def_data.did_num)
                            {
                            case RPN_DAT_SLR:
                                if(ev_match_slr(&p_ev_name->def_data.arg.slr, upp) != DEP_NONE)
                                    ev_todo_add_name_dependents(p_ev_name->key);
                                break;
                            case RPN_DAT_RANGE:
                                if(ev_match_rng(&p_ev_name->def_data.arg.range, upp) != DEP_NONE)
                                    ev_todo_add_name_dependents(p_ev_name->key);
                                break;
                            }
                        }
                    }
                }
            break;
        }

    /* look through the custom use table */
    switch(upp->action)
        {
        P_CUSTOM_USE mep;

        case UREF_CHANGE:
        case UREF_REDRAW:
        case UREF_RENAME:
            break;

        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_REPLACE:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            if((mep = tree_macptr(0)) != NULL)
                {
                EV_TRENT i;
                S32 check_use, un_sort;

                custom_list_sort();
                for(i = 0, check_use = un_sort = 0; i < custom_use_table.next; ++i, ++mep)
                    {
                    S32 res;

                    if(mep->flags & TRF_TOBEDEL)
                        continue;

                    /* custom uses contained by deleted area must be removed */
                    if((res = ev_match_slr(&mep->byslr, upp)) == DEP_DELETE)
                        {
                        mep->flags   |= TRF_TOBEDEL;
                        custom_use_table.flags |= TRF_TOBEDEL;
                        custom_use_table.mindel = MIN(custom_use_table.mindel, i);
                        check_use = 1;
                        }
                    else if(res == DEP_UPDATE)
                        un_sort = 1;
                    }

                /* set these flags at end to avoid sorts
                 * in the middle of our fuddle
                 */
                if(check_use)
                    custom_def.flags |= TRF_CHECKUSE;
                if(un_sort)
                    custom_use_table.sorted = 0;
                }
            break;
        }

    /* update custom definition table */
    switch(upp->action)
        {
        P_EV_CUSTOM p_ev_custom;

        case UREF_CHANGE:
        case UREF_REDRAW:
        case UREF_RENAME:
            break;

        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_CHANGEDOC:
        case UREF_REPLACE:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            if((p_ev_custom = custom_ptr(0)) != NULL)
                {
                EV_NAMEID i;

                for(i = 0; i < custom_def.next; ++i, ++p_ev_custom)
                    {
                    if(p_ev_custom->flags & TRF_TOBEDEL)
                        continue;

                    /* check for custom definition in the area */
                    if(ev_match_slr(&p_ev_custom->owner, upp) == DEP_DELETE)
                        {
                        /* delete custom table entry
                         * if there are no dependents
                        */
                        if(!ev_todo_add_custom_dependents(p_ev_custom->key))
                            {
                            ev_p_ss_doc_from_docno_must(p_ev_custom->owner.docno)->custom_ref_count -= 1;
                            p_ev_custom->flags  |= TRF_TOBEDEL;
                            custom_def.flags |= TRF_TOBEDEL;
                            custom_def.mindel = MIN(custom_def.mindel, i);

                            trace_1(TRACE_MODULE_UREF, "uref deleting custom: %s", p_ev_custom->id);
                            }

                        /* it's definitely undefined now, anyway */
                        p_ev_custom->owner.col = EV_MAX_COL - 1;
                        p_ev_custom->owner.row = EV_MAX_ROW - 1;
                        p_ev_custom->flags |= TRF_UNDEFINED;
                        }
                    }
                }
            break;
        }

    trace_0(TRACE_MODULE_UREF, " -- out");
}

/******************************************************************************
*
* call match_slr for a range end-point
*
******************************************************************************/

static S32
match_slr_e(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp)
{
    S32 res;
    ref->col -= 1;
    ref->row -= 1;

    res = ev_match_slr(ref, upp);

    ref->col += 1;
    ref->row += 1;
    return(res);
}

/******************************************************************************
*
* set spam flags for a range
*
******************************************************************************/

static void
rng_set_span(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InRef_     PC_UREF_PARM upp,
    _OutRef_    P_S32 colspan,
    _OutRef_    P_S32 rowspan)
{
    *colspan = *rowspan = 0;

    if(p_ev_range->s.docno != upp->slr2.docno)
        return;

    /* does the moved area span all the rows in the range ? */
    if(upp->slr1.row <= p_ev_range->s.row &&
       upp->slr2.row >= p_ev_range->e.row)
        *rowspan = 1;

    /* does the moved area span all the cols in the range ? */
    if(upp->slr1.col <= p_ev_range->s.col &&
       upp->slr2.col >= p_ev_range->e.col)
        *colspan = 1;
}

/******************************************************************************
*
* update reference when document
* number changes
*
******************************************************************************/

static S32
uref_cdoc(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_EV_SLR to)
{
    ref->docno = to->docno;
    return(DEP_UPDATE);
}

/******************************************************************************
*
* update reference when rows are swapped
*
******************************************************************************/

static S32
uref_swap(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_EV_SLR slrp1,
    _InRef_     PC_EV_SLR slrp2)
{
    if(ref->row == slrp1->row)
        ref->row = slrp2->row;
    else
        ref->row = slrp1->row;

    return(DEP_UPDATE);
}

/******************************************************************************
*
* update reference when slots are swapped
*
******************************************************************************/

static S32
uref_swapslot(
    _OutRef_    P_EV_SLR ref,
    _InRef_     PC_EV_SLR slrp)
{
    *ref = *slrp;
    return(DEP_UPDATE);
}

/******************************************************************************
*
* update reference for a normal uref
*
******************************************************************************/

static S32
uref_uref(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp,
    S32 copy)
{
    if(!copy || !(ref->flags & SLR_ABS_COL))
        ref->col += upp->slr3.col;

    if(!copy || !(ref->flags & SLR_ABS_ROW))
        ref->row += upp->slr3.row;

    if(ref->docno == upp->slr2.docno)
        ref->docno = upp->slr3.docno;

    ref->flags |= upp->slr3.flags;

    if(ref->col < 0)
        {
        ref->col = 0;
        ref->flags |= SLR_BAD_REF;
        return(DEP_DELETE);
        }

    if(ref->row < 0)
        {
        ref->row = 0;
        ref->flags |= SLR_BAD_REF;
        return(DEP_DELETE);
        }

    return(DEP_UPDATE);
}

/* end of ev_uref.c */
