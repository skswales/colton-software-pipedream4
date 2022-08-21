/* ev_tree.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Tree management routines for evaluator */

/* MRJC January 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

PROC_QSORT_PROTO(static, slrcomp, SLR_USE);

PROC_QSORT_PROTO(static, todocomp, TODO_ENTRY);

static void
todo_add_name_deps_of_slr(
    _InRef_     PC_EV_SLR slrp,
    S32 all_doc);

static S32
todo_add_slr(
    _InRef_     PC_EV_SLR slrp,
    S32 sort);

static void
tree_sort_customs(void);

static void
tree_sort_names(void);

static void
tree_sort_ranges(
    _InVal_     EV_DOCNO docno);

static void
tree_sort_slrs(
    _InVal_     EV_DOCNO docno);

static void
tree_sort_todo(void);

/*
name use table
*/

DEPTABLE namtab = {0, 0, 0, 0, 0, 0};

/*
custom function use table
*/

DEPTABLE custom_use_table = {0, 0, 0, 0, 0, 0};

/*
to do table
*/

DEPTABLE todotab = {0, 0, 0, 0, 0, 0};

/*
flags about all trees
*/

EV_FLAGS tree_flags = TRF_BLOWN;

/*
external dependency handles
*/

static S32 next_ext_dep_han = 0;

/*
block size increments
*/

#define EXTBLKINC   5
#define MACBLKINC  10
#define NAMBLKINC  10
#define SLRBLKINC 100
#define RNGBLKINC  20
#define TDOBLKINC 250

/******************************************************************************
*
* add a use to the list of custom uses
*
******************************************************************************/

static S32
add_custom_use(
    P_EV_GRUB grubp)
{
    P_CUSTOM_USE mep;
    EV_TRENT mix;

    if(dep_table_check_add_one(&custom_use_table,
                               sizeof32(CUSTOM_USE),
                               MACBLKINC) < 0)
        return(-1);

    mix = custom_use_table.next;
    mep = tree_macptr(mix);
    __assume(mep);
    /* create a space to insert */
    void_memmove32(mep + 1,
                   mep,
                   (custom_use_table.next - mix) * sizeof32(CUSTOM_USE));
    ++custom_use_table.next;

    /* copy in new dependency */
    mep->custom_to = grubp->data.arg.nameid;
    mep->byslr    = grubp->slr;
    mep->flags    = 0;
    mep->byoffset = grubp->byoffset;

    tree_flags |= TRF_BLOWN;

    return(0);
}

/******************************************************************************
*
* add a use to the list of name uses
*
******************************************************************************/

static S32
add_namuse(
    P_EV_GRUB grubp)
{
    P_NAME_USE nep;
    EV_TRENT nix;

    if(dep_table_check_add_one(&namtab,
                               sizeof32(NAME_USE),
                               NAMBLKINC) < 0)
        return(-1);

    nix = namtab.next;
    nep = tree_namptr(nix);
    __assume(nep);
    /* create a space to insert */
    void_memmove32(nep + 1,
                   nep,
                   (namtab.next - nix) * sizeof32(NAME_USE));
    ++namtab.next;

    /* copy in new dependency */
    nep->nameto   = grubp->data.arg.nameid;
    nep->byslr    = grubp->slr;
    nep->flags    = 0;
    nep->byoffset = grubp->byoffset;

    tree_flags |= TRF_BLOWN;

    return(0);
}

/******************************************************************************
*
* ensure we can add an entry
* to a dependency table
*
******************************************************************************/

extern S32
dep_table_check_add_one(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size)
{
    /* ensure we have space to add the dependency */
    if(dpp->next + 1 >= dpp->size)
        {
        P_ANY newblkp;
        EV_TRENT expand_size;

        expand_size = MAX(inc_size, dpp->size / 8);

        if((newblkp = list_reallocptr(dpp->ptr, ent_size * (dpp->size + (S32) expand_size))) == NULL &&
           expand_size > inc_size)
            newblkp = list_reallocptr(dpp->ptr, ent_size * (dpp->size + (S32) expand_size));

        if(!newblkp)
            return(-1);

        dpp->ptr   = newblkp;
        dpp->size += expand_size;

        trace_3(TRACE_MODULE_EVAL,
                "dependency table " PTR_XTFMT " realloced, now: %d entries, %d bytes\n",
                report_ptr_cast(dpp), dpp->size,
                ent_size * dpp->size);
        }

    return(0);
}

/******************************************************************************
*
* free some memory from a
* dependency table if sensible
*
******************************************************************************/

static void
dep_table_check_free_some(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size)
{
    /* free the handle altogether */
    if(dpp->size && !dpp->next)
        {
        list_disposeptr(&dpp->ptr);
        dpp->size = 0;
        }
    /* free some memory */
    else if(dpp->size - dpp->next >= inc_size)
        {
        P_ANY newptr;

        if((newptr = list_reallocptr(dpp->ptr,
                                     (dpp->next + (S32) inc_size) * ent_size)) != NULL)
            {
            dpp->ptr  = newptr;
            dpp->size = dpp->next + inc_size;
            }
        }
}

/******************************************************************************
*
* add an entry to the external dependency list
*
******************************************************************************/

extern S32
ev_add_extdependency(
    U32 exthandle,
    S32 int_handle_in,
    P_S32 int_handle_out,
    ev_urefct proc,
    _InRef_     PC_EV_RANGE rngto)
{
    P_SS_DOC p_ss_doc;
    extentp eep;

    if((p_ss_doc = ev_p_ss_doc_from_docno(rngto->s.docno)) == NULL)
        return(-1);

    if(dep_table_check_add_one(&p_ss_doc->exttab,
                               sizeof32(struct extentry),
                               EXTBLKINC) < 0)
        return(-1);

    eep = tree_extptr(p_ss_doc, p_ss_doc->exttab.next++);
    __assume(eep);

    /* copy in new dependency */
    eep->refto     = *rngto;
    eep->exthandle = exthandle;
    eep->flags     = 0;
    eep->proc      = proc;

    if(int_handle_out)
        eep->inthandle = *int_handle_out = next_ext_dep_han++;
    else
        eep->inthandle = int_handle_in;

    tree_flags |= TRF_BLOWN;

    return(0);
}

/******************************************************************************
*
* add all the dependencies for an
* expression slot into the tree
*
******************************************************************************/

extern S32
ev_add_exp_slot_to_tree(
    P_EV_SLOT p_ev_slot,
    _InRef_     PC_EV_SLR slrp)
{
    struct ev_grub_state grubb;
    EV_FLAGS custom_flags, names_flags;
    S32 res;

    /* push state of names and custom check use flags, then
     * clear them; we don't want to delete definitions that
     * may have uses that we are about to add
     */
    custom_flags     = custom_def.flags & TRF_CHECKUSE;
    custom_def.flags &= ~TRF_CHECKUSE;

    names_flags      = names_def.flags & TRF_CHECKUSE;
    names_def.flags &= ~TRF_CHECKUSE;

    grub_init(&grubb, slrp);

    res = 0;
    while(res >= 0 && grub_next(p_ev_slot, &grubb) != RPN_FRM_END)
        {
        switch(grubb.data.did_num)
            {
            case RPN_DAT_SLR:
                if(ev_add_slrdependency(&grubb) < 0)
                    res = -1;
                break;

            case RPN_DAT_RANGE:
                if(ev_add_rngdependency(&grubb) < 0)
                    res = -1;
                break;

            /* record use of a name */
            case RPN_DAT_NAME:
                if(add_namuse(&grubb) < 0)
                    res = -1;
                break;

            /* custom function call */
            case RPN_FNM_CUSTOMCALL:
                if(add_custom_use(&grubb) < 0)
                    res = -1;
                break;

            /* custom function definitions */
            case RPN_FNM_FUNCTION:
                {
                P_EV_CUSTOM p_ev_custom;
                EV_NAMEID custom_num;

                if((custom_num = custom_def_find(grubb.data.arg.nameid)) >= 0)
                    if((p_ev_custom = custom_ptr(custom_num)) != NULL)
                        {
                        P_SS_DOC p_ss_doc;

                        p_ev_custom->owner  = *slrp;
                        p_ev_custom->flags &= ~TRF_UNDEFINED;

                        /* mark document as a custom function sheet */
                        if((p_ss_doc = ev_p_ss_doc_from_docno(slrp->docno)) != NULL)
                            p_ss_doc->flags |= DCF_CUSTOM;

                        ev_todo_add_custom_dependents(grubb.data.arg.nameid);
                        }
                break;
                }
            }
        }

    /* restore flags */
    custom_def.flags |= custom_flags;
    names_def.flags |= names_flags;

    return(res);
}

/******************************************************************************
*
* add a dependency to the list of range dependencies
*
******************************************************************************/

extern S32
ev_add_rngdependency(
    P_EV_GRUB grubp)
{
    P_SS_DOC p_ss_doc;
    P_RANGE_USE rep;
    EV_TRENT rix;
    RANGE_USE target;

    if((p_ss_doc = ev_p_ss_doc_from_docno(grubp->data.arg.range.s.docno)) == NULL)
        return(-1);

    if(dep_table_check_add_one(&p_ss_doc->range_table,
                               sizeof32(RANGE_USE),
                               RNGBLKINC) < 0)
        return(-1);

    target.refto = grubp->data.arg.range;

    rix = p_ss_doc->range_table.next;
    rep = tree_rngptr(p_ss_doc, rix);
    __assume(rep);
    /* create a space to insert */
    void_memmove32(rep + 1,
                   rep,
                   (p_ss_doc->range_table.next - rix) * sizeof32(RANGE_USE));
    ++p_ss_doc->range_table.next;

    /* copy in new dependency */
    rep->refto    = grubp->data.arg.range;
    rep->byslr    = grubp->slr;
    rep->byoffset = grubp->byoffset;
    rep->flags    = 0;
    rep->visited  = 0;

    tree_flags |= TRF_BLOWN;

    return(0);
}

/******************************************************************************
*
* add a dependency to the list of slr dependencies
*
******************************************************************************/

extern S32
ev_add_slrdependency(
    P_EV_GRUB grubp)
{
    P_SS_DOC p_ss_doc;
    P_SLR_USE sep;
    EV_TRENT six;

    if((p_ss_doc = ev_p_ss_doc_from_docno(grubp->data.arg.slr.docno)) == NULL)
        return(-1);

    if(dep_table_check_add_one(&p_ss_doc->slr_table,
                               sizeof32(SLR_USE),
                               SLRBLKINC) < 0)
        return(-1);

    six = p_ss_doc->slr_table.next;
    sep = tree_slrptr(p_ss_doc, six);
    __assume(sep);
    /* create a space to insert */
    void_memmove32(sep + 1,
                   sep,
                   (p_ss_doc->slr_table.next - six) * sizeof32(SLR_USE));
    ++p_ss_doc->slr_table.next;

    /* copy in new dependency */
    sep->refto    = grubp->data.arg.slr;
    sep->byslr    = grubp->slr;
    sep->byoffset = grubp->byoffset;
    sep->flags    = 0;

    tree_flags |= TRF_BLOWN;

    return(0);
}

/******************************************************************************
*
* remove an external dependency from the table
*
******************************************************************************/

extern void
ev_del_extdependency(
    _InVal_     EV_DOCNO docno,
    S32 handle_in)
{
    P_SS_DOC p_ss_doc;
    extentp eep;
    EV_TRENT eix;

    if((p_ss_doc = ev_p_ss_doc_from_docno(docno)) == NULL)
        return;

    eix = 0;
    if((eep = tree_extptr(p_ss_doc, 0)) != NULL)
        {
        while(eix < p_ss_doc->exttab.next)
            {
            if(eep->inthandle == handle_in)
                {
                eep->flags |= TRF_TOBEDEL;
                p_ss_doc->exttab.flags |= TRF_TOBEDEL;
                p_ss_doc->exttab.mindel = MIN(p_ss_doc->exttab.mindel, eix);
                break;
                }

            ++eep;
            ++eix;
            }
        }

    tree_flags |= TRF_BLOWN;
}

/******************************************************************************
*
* initialise slot dependent/supporter getter
*
******************************************************************************/

extern void
ev_enum_dep_sup_init(
    _OutRef_    P_EV_DEPSUP dsp,
    _InRef_     PC_EV_SLR slrp,
    S32 category,
    S32 get_deps)
{
    dsp->item_no = 0;
    dsp->slr = *slrp;
    dsp->category = category;
    dsp->get_deps = get_deps;
    dsp->tab_pos = 0;
}

/******************************************************************************
*
* return supporting or dependent
* slots/ranges/names for a slot
*
* --in--
* item_no <  0 - get next dep/sup
* item_no >= 0 - get specific dep/sup
*
* --out--
* < 0 error (end)
* >=0 entry returned
*
******************************************************************************/

extern S32
ev_enum_dep_sup_get(
    P_EV_DEPSUP dsp,
    P_S32 item_no,
    P_EV_DATA p_ev_data)
{
    S32 res, item_get;

    if(*item_no < 0)
        {
        item_get = dsp->item_no++;
        *item_no = item_get;
        }
    else
        item_get = *item_no;

    res = -1;

    if(dsp->get_deps)
        {
        /* look for things dependent upon this slot */
        switch(dsp->category)
            {
            case EV_DEPSUP_SLR:
                {
                EV_TRENT six;

                if((six = search_for_slrdependent(&dsp->slr)) >= 0)
                    {
                    P_SS_DOC p_ss_doc;
                    P_SLR_USE sep;

                    p_ss_doc = ev_p_ss_doc_from_docno(dsp->slr.docno);
                    six  += item_get;
                    sep   = tree_slrptr(p_ss_doc, six);
                    __assume(sep);

                    if(six < p_ss_doc->slr_table.next         &&
                       !(sep->flags & TRF_TOBEDEL)      &&
                       slr_equal(&dsp->slr, &sep->refto))
                        {
                        p_ev_data->did_num = RPN_DAT_SLR;
                        p_ev_data->arg.slr = sep->byslr;
                        res = 1;
                        }
                    }
                break;
                }

            case EV_DEPSUP_RANGE:
                {
                P_SS_DOC p_ss_doc;
                P_RANGE_USE rep;

                p_ss_doc = ev_p_ss_doc_from_docno(dsp->slr.docno);

                /* look thru range dependencies */
                if((rep = tree_rngptr(p_ss_doc, 0)) != NULL)
                    {
                    EV_TRENT rix;

                    for(rix = 0; rix < p_ss_doc->range_table.next; ++rix, ++rep)
                        if(!(rep->flags & TRF_TOBEDEL))
                            if(ev_slr_in_range(&rep->refto, &dsp->slr) && !item_get--)
                                {
                                p_ev_data->did_num = RPN_DAT_SLR;
                                p_ev_data->arg.slr = rep->byslr;
                                res = 1;
                                }
                    }
                break;
                }

            case EV_DEPSUP_NAME:
                {
                P_EV_NAME p_ev_name;

                /* look for name references */
                if((p_ev_name = name_ptr(0)) != NULL)
                    {
                    EV_TRENT i;

                    for(i = 0; i < names_def.next; ++i, ++p_ev_name)
                        if(!(p_ev_name->flags & TRF_TOBEDEL))
                            {
                            S32 got_ref;

                            got_ref = 0;
                            switch(p_ev_name->def.did_num)
                                {
                                case RPN_DAT_SLR:
                                    if(slr_equal(&p_ev_name->def.arg.slr, &dsp->slr))
                                        got_ref = 1;
                                    break;

                                case RPN_DAT_RANGE:
                                    if(ev_slr_in_range(&p_ev_name->def.arg.range, &dsp->slr))
                                        got_ref = 1;
                                    break;
                                }

                            if(got_ref && !item_get--)
                                {
                                p_ev_data->did_num = RPN_DAT_NAME;
                                p_ev_data->arg.nameid = p_ev_name->key;
                                res = 1;
                                }
                            }
                    }

                break;
                }
            }
        }
    else
        {
        P_EV_SLOT p_ev_slot;

        /* grub about for supporters of this slot */
        if(ev_travel(&p_ev_slot, &dsp->slr) > 0)
            {
            struct ev_grub_state grubb;

            grub_init(&grubb, &dsp->slr);

            while(res < 0 && grub_next(p_ev_slot, &grubb) != RPN_FRM_END)
                {
                switch(grubb.data.did_num)
                    {
                    case RPN_DAT_SLR:
                        if(dsp->category == EV_DEPSUP_SLR)
                            if(!item_get--)
                                {
                                *p_ev_data = grubb.data;
                                res = 1;
                                }
                        break;
                    case RPN_DAT_RANGE:
                        if(dsp->category == EV_DEPSUP_RANGE)
                            if(!item_get--)
                                {
                                *p_ev_data = grubb.data;
                                res = 1;
                                }
                        break;
                    case RPN_DAT_NAME:
                        if(dsp->category == EV_DEPSUP_NAME)
                            if(!item_get--)
                                {
                                *p_ev_data = grubb.data;
                                res = 1;
                                }
                        break;
                    }
                }
            }
        }

    return(res);
}

/******************************************************************************
*
* mark all dependents of the given docno for recalc; bump serial number first
*
******************************************************************************/

extern void
ev_todo_add_doc_dependents(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc;
    P_EV_CUSTOM p_ev_custom;
    EV_SLR slr;

    ++ev_serial_num;

    if((p_ss_doc = ev_p_ss_doc_from_docno(docno)) != NULL)
        {
        P_RANGE_USE rep;
        P_SLR_USE sep;

        /* look thru range dependencies */
        if((rep = tree_rngptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT rix;

            for(rix = 0; rix < p_ss_doc->range_table.next; ++rix, ++rep)
                if(!(rep->flags & TRF_TOBEDEL))
                    todo_add_slr(&rep->byslr, TODO_SORT);
            }

        /* add slr dependencies */
        if((sep = tree_slrptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT six;

            for(six = 0; six < p_ss_doc->slr_table.next; ++six, ++sep)
                if(!(sep->flags & TRF_TOBEDEL))
                    todo_add_slr(&sep->byslr, TODO_SORT);
            }
        }

    /* add dependents of names referencing this document */
    slr.docno = docno;
    todo_add_name_deps_of_slr(&slr, TRUE);

    /* look for custom function references */
    if((p_ev_custom = custom_ptr(0)) != NULL)
        {
        EV_NAMEID i;

        for(i = 0; i < custom_def.next; ++i, ++p_ev_custom)
            if(!(p_ev_custom->flags & (TRF_TOBEDEL | TRF_UNDEFINED)))
                if(p_ev_custom->owner.docno == docno)
                    todo_add_custom_dependents(p_ev_custom->key);
        }
}

/******************************************************************************
*
* add a slot's dependents to the todo list bumping the serial number first
*
******************************************************************************/

extern void
ev_todo_add_dependents(
    _InRef_     PC_EV_SLR slrp)
{
    ++ev_serial_num;
    todo_add_dependents(slrp);
}

/******************************************************************************
*
* add dependents of a custom function to the todo list
*
******************************************************************************/

extern S32
ev_todo_add_custom_dependents(
    EV_NAMEID customid)
{
    ++ev_serial_num;
    return(todo_add_custom_dependents(customid));
}

/******************************************************************************
*
* add dependents of a name to the todo list
*
******************************************************************************/

extern S32
ev_todo_add_name_dependents(
    EV_NAMEID nameid)
{
    ++ev_serial_num;
    return(todo_add_name_dependents(nameid));
}

/******************************************************************************
*
* add slr to todo list, but bump recalc serial number first
*
******************************************************************************/

extern S32
ev_todo_add_slr(
    _InRef_     PC_EV_SLR slrp,
    S32 sort)
{
    ++ev_serial_num;
    return(todo_add_slr(slrp, sort));
}

/******************************************************************************
*
* document is about to vanish - tell its dependents,
* but leave references to it intact
*
******************************************************************************/

extern void
ev_tree_close(
    _InVal_     EV_DOCNO docno)
{
    UREF_PARM urefb;

    urefb.slr1.docno = docno;

    urefb.action= UREF_CLOSE;

    ev_uref(&urefb);
}

/******************************************************************************
*
* routine for bsearch to compare two custom use ids
*
******************************************************************************/

PROC_QSORT_PROTO(static, custom_comp, CUSTOM_USE)
{
    P_CUSTOM_USE c1 = (P_CUSTOM_USE) arg1;
    P_CUSTOM_USE c2 = (P_CUSTOM_USE) arg2;

    /* NB no current_p_docu global register furtling required */

    if(c1->custom_to > c2->custom_to)
        return(1);
    if(c1->custom_to < c2->custom_to)
        return(-1);

    return(0);
}

/******************************************************************************
*
* routine for bsearch to compare two nameids
*
******************************************************************************/

PROC_QSORT_PROTO(extern, namcomp, NAME_USE)
{
    P_NAME_USE name1 = (P_NAME_USE) arg1;
    P_NAME_USE name2 = (P_NAME_USE) arg2;

    /* NB no current_p_docu global register furtling required */

    if(name1->nameto > name2->nameto)
        return(1);
    if(name1->nameto < name2->nameto)
        return(-1);

    return(0);
}

/******************************************************************************
*
* routine for bsearch to compare two range table entries
*
******************************************************************************/

PROC_QSORT_PROTO(static, rngcomp, RANGE_USE)
{
    P_RANGE_USE rng1 = (P_RANGE_USE) arg1;
    P_RANGE_USE rng2 = (P_RANGE_USE) arg2;

    /* NB no current_p_docu global register furtling required */

    if(rng1->refto.s.docno > rng2->refto.s.docno)
        return(1);
    if(rng1->refto.s.docno < rng2->refto.s.docno)
        return(-1);

    if(rng1->refto.s.col > rng2->refto.s.col)
        return(1);
    if(rng1->refto.s.col < rng2->refto.s.col)
        return(-1);

    if(rng1->refto.s.row > rng2->refto.s.row)
        return(1);
    if(rng1->refto.s.row < rng2->refto.s.row)
        return(-1);

    if(rng1->refto.e.col > rng2->refto.e.col)
        return(1);
    if(rng1->refto.e.col < rng2->refto.e.col)
        return(-1);

    if(rng1->refto.e.row > rng2->refto.e.row)
        return(1);
    if(rng1->refto.e.row < rng2->refto.e.row)
        return(-1);

    return(0);
}

/******************************************************************************
*
* search the custom function use table for
* a reference to a given custom function
*
******************************************************************************/

extern EV_TRENT
search_for_custom_use(
    EV_NAMEID customid)
{
    P_CUSTOM_USE smep, mep;
    EV_TRENT res;
    CUSTOM_USE target;

    tree_sort_customs();

    if((smep = tree_macptr(0)) == NULL)
        return(-1);

    target.custom_to = customid;

    /* search for reference */
    if((mep = bsearch(&target,
                      smep,
                      custom_use_table.sorted,
                      sizeof(CUSTOM_USE),
                      custom_comp)) != 0)
        {
        EV_TRENT mix;

        /* step back to start of all names the same */
        for(mix = mep - smep;
            mix > 0 && !custom_comp(mep - 1, &target);
            --mep, --mix);

        res = mix;
        }
    else
        res = -1;

    return(res);
}

/******************************************************************************
*
* search the name tree for a reference to a given name
*
******************************************************************************/

extern EV_TRENT
search_for_name_use(
    EV_NAMEID nameid)
{
    P_NAME_USE snep, nep;
    EV_TRENT res;
    NAME_USE target;

    tree_sort_names();

    if((snep = tree_namptr(0)) == NULL)
        return(-1);

    target.nameto = nameid;

    /* search for reference */
    if((nep = bsearch(&target,
                      snep,
                      namtab.sorted,
                      sizeof(NAME_USE),
                      namcomp)) != 0)
        {
        EV_TRENT nix;

        /* step back to start of all names the same */
        for(nix = nep - snep;
            nix > 0 && !namcomp(nep - 1, &target);
            --nep, --nix);

        res = nix;
        }
    else
        res = -1;

    return(res);
}

/******************************************************************************
*
* search for a given specific range reference
*
******************************************************************************/

extern EV_TRENT
search_for_rng_ref(
    _InRef_     PC_EV_RANGE p_ev_range)
{
    P_SS_DOC p_ss_doc;
    P_RANGE_USE srep;
    EV_TRENT res;

    tree_sort_ranges(p_ev_range->s.docno);

    if((p_ss_doc = ev_p_ss_doc_from_docno(p_ev_range->s.docno)) == NULL)
        return(-1);

    res = -1;
    if((srep = tree_rngptr(p_ss_doc, 0)) != NULL)
        {
        RANGE_USE target;
        P_RANGE_USE rep;

        target.refto = *p_ev_range;

        /* search for reference */
        if((rep = bsearch(&target,
                          srep,
                          p_ss_doc->range_table.sorted,
                          sizeof(RANGE_USE),
                          rngcomp)) != NULL)
            {
            EV_TRENT rix;

            /* step back to start of all ranges the same */
            for(rix = rep - srep;
                rix > 0 && !rngcomp(rep - 1, &target);
                --rep, --rix);
            res = rix;
            }
        }
    return(res);
}

/******************************************************************************
*
* search the table of slot references for a reference to the given slot
*
******************************************************************************/

extern EV_TRENT
search_for_slrdependent(
    _InRef_     PC_EV_SLR slrp)
{
    P_SS_DOC p_ss_doc;
    P_SLR_USE ssep;
    EV_TRENT res;

    tree_sort_slrs(slrp->docno);

    if((p_ss_doc = ev_p_ss_doc_from_docno(slrp->docno)) == NULL)
        return(-1);

    res = -1;
    if((ssep = tree_slrptr(p_ss_doc, 0)) != NULL)
        {
        SLR_USE target;
        P_SLR_USE sep;

        target.refto = *slrp;

        /* search for reference */
        if((sep = bsearch(&target,
                          ssep,
                          p_ss_doc->slr_table.sorted,
                          sizeof(SLR_USE),
                          slrcomp)) != NULL)
            {
            EV_TRENT six;

            /* step back to start of all refs the same */
            for(six = sep - ssep;
                six > 0 && !slrcomp(sep - 1, &target);
                --sep, --six);
            res = six;
            }
        }

    return(res);
}

/******************************************************************************
*
* routine for bsearch to compare two slots
*
******************************************************************************/

PROC_QSORT_PROTO(static, slrcomp, SLR_USE)
{
    P_SLR_USE slr1 = (P_SLR_USE) arg1;
    P_SLR_USE slr2 = (P_SLR_USE) arg2;

    /* NB no current_p_docu global register furtling required */

    if(slr1->refto.row > slr2->refto.row)
        return(1);
    if(slr1->refto.row < slr2->refto.row)
        return(-1);

    if(slr1->refto.col > slr2->refto.col)
        return(1);
    if(slr1->refto.col < slr2->refto.col)
        return(-1);

    if(slr1->refto.docno > slr2->refto.docno)
        return(1);
    if(slr1->refto.docno < slr2->refto.docno)
        return(-1);

    return(0);
}

/******************************************************************************
*
* add the dependents of a slot to the todo list
*
******************************************************************************/

extern void
todo_add_dependents(
    _InRef_     PC_EV_SLR slrp)
{
    P_SS_DOC p_ss_doc;

    if((p_ss_doc = ev_p_ss_doc_from_docno(slrp->docno)) != NULL)
        {
        P_RANGE_USE rep;
        P_SLR_USE sep;
        EV_TRENT six;

        /* look thru range dependencies */
        if((rep = tree_rngptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT rix;

            for(rix = 0; rix < p_ss_doc->range_table.next; ++rix, ++rep)
                if(!(rep->flags & TRF_TOBEDEL))
                    if(ev_slr_in_range(&rep->refto, slrp))
                        todo_add_slr(&rep->byslr, TODO_SORT);
            }

        /* find slr dependents */
        six = search_for_slrdependent(slrp);

        if(six >= 0)
            {
            sep = tree_slrptr(p_ss_doc, six);
            __assume(sep);

            while(six < p_ss_doc->slr_table.next &&
                  slr_equal(&sep->refto, slrp))
                {
                if(!(sep->flags & TRF_TOBEDEL))
                    todo_add_slr(&sep->byslr, TODO_SORT);

                ++sep;
                ++six;
                }
            }
        }

    /* look for name references */
    todo_add_name_deps_of_slr(slrp, FALSE);
}

/******************************************************************************
*
* mark all dependents (users of a custom function) for recalc
*
******************************************************************************/

extern S32
todo_add_custom_dependents(
    EV_NAMEID customid)
{
    EV_NAMEID mix;
    S32 found_use;

    found_use = 0;
    if((mix = search_for_custom_use(customid)) >= 0)
        {
        P_CUSTOM_USE mep;

        if((mep = tree_macptr(mix)) != NULL)
            {
            CUSTOM_USE key;

            key.custom_to = customid;

            while(mix < custom_use_table.next && !custom_comp(mep, &key))
                {
                if(!(mep->flags & TRF_TOBEDEL))
                    todo_add_slr(&mep->byslr, TODO_SORT);

                ++mep;
                ++mix;
                }
            }
        found_use = 1;
        }

    return(found_use);
}

/******************************************************************************
*
* mark all dependents (users of a name) for recalc
*
******************************************************************************/

extern S32
todo_add_name_dependents(
    EV_NAMEID nameid)
{
    EV_NAMEID nix;
    S32 found_use;

    found_use = 0;
    if((nix = search_for_name_use(nameid)) >= 0)
        {
        P_NAME_USE nep;

        if((nep = tree_namptr(nix)) != NULL)
            {
            NAME_USE key;

            key.nameto = nameid;

            while(nix < namtab.next && !namcomp(nep, &key))
                {
                if(!(nep->flags & TRF_TOBEDEL))
                    todo_add_slr(&nep->byslr, TODO_SORT);

                ++nep;
                ++nix;
                }
            }
        found_use = 1;
        }

    return(found_use);
}

/******************************************************************************
*
* look for names which refer to a given slot
* or document and add their dependents to be done
*
* set all_doc to match all slots in a document
*
******************************************************************************/

static void
todo_add_name_deps_of_slr(
    _InRef_     PC_EV_SLR slrp,
    S32 all_doc)
{
    P_EV_NAME p_ev_name;

    /* look for name references */
    if((p_ev_name = name_ptr(0)) != NULL)
        {
        EV_NAMEID i;

        for(i = 0; i < names_def.next; ++i, ++p_ev_name)
            if(!(p_ev_name->flags & TRF_TOBEDEL))
                {
                S32 got_ref;

                got_ref = 0;
                switch(p_ev_name->def.did_num)
                    {
                    case RPN_DAT_SLR:
                        if(all_doc)
                            {
                            if(p_ev_name->def.arg.slr.docno == slrp->docno)
                                got_ref = 1;
                            }
                        else if(slr_equal(&p_ev_name->def.arg.slr, slrp))
                            got_ref = 1;
                        break;

                    case RPN_DAT_RANGE:
                        if(all_doc)
                            {
                            if(p_ev_name->def.arg.range.s.docno == slrp->docno)
                                got_ref = 1;
                            }
                        if(ev_slr_in_range(&p_ev_name->def.arg.range, slrp))
                            got_ref = 1;
                        break;
                    }

                /* mark all uses of name */
                if(got_ref)
                    todo_add_name_dependents(p_ev_name->key);
                }
        }
}

/******************************************************************************
*
* ensure that a slot is in the to do list
*
* --in--
* sort = 0 no sort, no duplicate check
* sort = 1    sort,    duplicate check
*
******************************************************************************/

static S32
todo_add_slr(
    _InRef_     PC_EV_SLR slrp,
    S32 sort)
{
    if(!doc_check_custom(slrp->docno))
        {
        P_TODO_ENTRY todop;

        if(!sort || (todop = todo_has_slr(slrp)) == NULL)
            {
            if(dep_table_check_add_one(&todotab,
                                       sizeof(TODO_ENTRY),
                                       TDOBLKINC) < 0)
                return(-1);

            /* find place to insert, keeping order */
            if(sort && todotab.sorted == todotab.next && todotab.next)
                {
                TODO_ENTRY target;
                P_TODO_ENTRY todops, todope;

                target.slr = *slrp;
                todops = todo_ptr(0);
                todope = todo_ptr(todotab.next);

                todop = todope;
                do  {
                    --todop;
                    if(todocomp(&target, todop) > 0)
                        {
                        ++todop;
                        break;
                        }
                    }
                while(todop > todops);

                /* make space for new entry */
                void_memmove32(todop + 1, todop, PtrDiffBytesU32(todope, todop));
                ++todotab.next;
                ++todotab.sorted;
                }
            else
                todop = todo_ptr(todotab.next++);

            /* copy in new entry */
            __assume(todop);
            todop->slr     = *slrp;
            todop->flags   = 0;

#if TRACE_ALLOWED
            if(tracing(TRACE_MODULE_EVAL))
                {
                char buffer[BUF_EV_LONGNAMLEN];
                ev_trace_slr(buffer, elemof32(buffer), "<todo_add_slr> added $$, serial_num=%ld, todo=%ld\n", slrp);
                trace_2(TRACE_MODULE_EVAL, buffer, ev_serial_num, todotab.next);
                }
#endif
            }

        todop->flags  &= ~TRF_TOBEDEL;
        }

    return(0);
}

/******************************************************************************
*
* routine for bsearch to compare two todo entries
*
******************************************************************************/

PROC_QSORT_PROTO(static, todocomp, TODO_ENTRY)
{
    P_TODO_ENTRY slr1 = (P_TODO_ENTRY) arg1;
    P_TODO_ENTRY slr2 = (P_TODO_ENTRY) arg2;

    /* NB no current_p_docu global register furtling required */

    if(slr1->slr.docno > slr2->slr.docno)
        return(1);
    if(slr1->slr.docno < slr2->slr.docno)
        return(-1);

    if(slr1->slr.row > slr2->slr.row)
        return(-1);
    if(slr1->slr.row < slr2->slr.row)
        return(1);

    if(slr1->slr.col > slr2->slr.col)
        return(-1);
    if(slr1->slr.col < slr2->slr.col)
        return(1);

    return(0);
}

/******************************************************************************
*
* get a slot to calc from the to do list
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
extern S32
todo_get_slr(
    _OutRef_    P_EV_SLR slrp)
{
    S32 got_todo;

    got_todo = 0;

    if(todotab.next)
        tree_sort_todo();

    if(todotab.next)
        {
        P_TODO_ENTRY todop, todops;

        for(todops = todo_ptr(0), todop = todo_ptr(todotab.next - 1); todop >= todops; --todop)
            {
            if(todop->flags & TRF_TOBEDEL)
                continue;

            /* don't leave custom function sheet todos behind */
            if(doc_check_custom(todop->slr.docno))
                {
                todop->flags  |= TRF_TOBEDEL;
                todotab.flags |= TRF_TOBEDEL;
                todotab.mindel = MIN(todotab.mindel, todop - todops);
                continue;
                }

            *slrp = todop->slr;
            got_todo = 1;
            break;
            }
        }

    return(got_todo);
}

/******************************************************************************
*
* is the slot already in the to do list ?
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_TODO_ENTRY
todo_has_slr(
    _InRef_     PC_EV_SLR slrp)
{
    P_TODO_ENTRY found = NULL;
    P_TODO_ENTRY todop;

    if(todotab.sorted < todotab.next)
        tree_sort_todo();

    if((todop = todo_ptr(0)) != NULL)
        {
        TODO_ENTRY target;

        target.slr = *slrp;

        found = bsearch(&target,
                        todop,
                        todotab.sorted,
                        sizeof(TODO_ENTRY),
                        todocomp);
        }

    return(found);
}

/******************************************************************************
*
* remove all occurrences of a slot from the todo list
*
******************************************************************************/

extern void
todo_remove_slr(
    _InRef_     PC_EV_SLR slrp)
{
    P_TODO_ENTRY todop;

    if((todop = todo_has_slr(slrp)) != NULL)
        {
        todop->flags  |= TRF_TOBEDEL;
        todotab.flags |= TRF_TOBEDEL;
        todotab.mindel = MIN(todotab.mindel, todop - todo_ptr(0));

#if TRACE_ALLOWED
        if(tracing(TRACE_MODULE_EVAL))
            {
            char buffer[BUF_EV_LONGNAMLEN];
            ev_trace_slr(buffer, elemof32(buffer), "<todo_remove_slr> removed $$, todo=%ld\n", slrp);
            trace_1(TRACE_MODULE_EVAL, buffer, todotab.next);
            }
#endif
        }
}

/******************************************************************************
*
* remove any entries that are to be deleted from a dependency table
*
******************************************************************************/

static S32
tree_remove_tobedel(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size,
    S32 flag_offset)
{
    S32 blown = 0;

    /* first free any deleted entries */
    if(dpp->next && (dpp->flags & TRF_TOBEDEL))
        {
        P_U8 sep, iep, oep;

        if(dpp->ptr)
            {
            EV_TRENT i, old_next;

            old_next = dpp->next;
            sep = (P_U8) dpp->ptr;

            for(i = dpp->mindel, iep = oep = sep + (i * ent_size);
                i < dpp->next;
                ++i, iep += ent_size)
                if(!(*((EV_FLAGS *)(iep + flag_offset)) & TRF_TOBEDEL))
                    {
                    if(oep != iep)
                        void_memcpy32(oep, iep, ent_size);

                    oep += ent_size;
                    }

            dpp->mindel = dpp->next = (oep - sep) / ent_size;
            dpp->flags &= ~TRF_TOBEDEL;

            dpp->sorted -= old_next - dpp->next;

            blown = 1;
            dep_table_check_free_some(dpp, ent_size, inc_size);
            }
        }

    return(blown);
}

/******************************************************************************
*
* generic routine to sort trees
*
******************************************************************************/

extern S32
tree_sort(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size,
    S32 flag_offset,
    sort_proctp sort_proc)
{
    S32 blown;

    blown = tree_remove_tobedel(dpp, ent_size, inc_size, flag_offset);

    if(dpp->sorted < dpp->next)
        {
        if(dpp->ptr)
            {
            trace_0(TRACE_MODULE_EVAL, "** sort **");
            qsort((P_U8) dpp->ptr, dpp->next, ent_size, sort_proc);

            dpp->sorted = dpp->next;
            blown = 1;
            }
        }

    return(blown);
}

/******************************************************************************
*
* sort all the trees
*
******************************************************************************/

extern void
tree_sort_all(void)
{
    EV_DOCNO docno;

    if(0 == docu_array_size)
        return;

    for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
        {
        P_SS_DOC p_ss_doc;

        if(docno_void_entry(docno))
            continue;

        p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

        tree_remove_tobedel(&p_ss_doc->exttab,
                            sizeof(struct extentry),
                            EXTBLKINC,
                            offsetof(struct extentry, flags));

        tree_sort_ranges(docno);
        tree_sort_slrs(docno);
        }

    tree_sort_customs();
    tree_sort_names();

    custom_list_sort();
    name_list_sort();

    tree_sort_todo();
}

/******************************************************************************
*
* sort custom use table
*
******************************************************************************/

static void
tree_sort_customs(void)
{
    if(!(tree_flags & TRF_LOCK))
        {
        if(tree_sort(&custom_use_table,
                     sizeof32(CUSTOM_USE),
                     MACBLKINC,
                     offsetof32(CUSTOM_USE, flags),
                     custom_comp))
            tree_flags |= TRF_BLOWN;
        }
}

/******************************************************************************
*
* sort name use tree
*
******************************************************************************/

static void
tree_sort_names(void)
{
    if(!(tree_flags & TRF_LOCK))
        {
        if(tree_sort(&namtab,
                     sizeof32(NAME_USE),
                     NAMBLKINC,
                     offsetof32(NAME_USE, flags),
                     namcomp))
            tree_flags |= TRF_BLOWN;
        }
}

/******************************************************************************
*
* sort a range tree
*
******************************************************************************/

static void
tree_sort_ranges(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc;

    if((p_ss_doc = ev_p_ss_doc_from_docno(docno)) == NULL)
        return;

    if(!(tree_flags & TRF_LOCK))
        {
        if(tree_sort(&p_ss_doc->range_table,
                     sizeof32(RANGE_USE),
                     RNGBLKINC,
                     offsetof32(RANGE_USE, flags),
                     rngcomp))
            tree_flags |= TRF_BLOWN;
        }
}

/******************************************************************************
*
* sort an slr tree
*
******************************************************************************/

static void
tree_sort_slrs(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc;

    if((p_ss_doc = ev_p_ss_doc_from_docno(docno)) == NULL)
        return;

    if(!(tree_flags & TRF_LOCK))
        {
        if(tree_sort(&p_ss_doc->slr_table,
                     sizeof32(SLR_USE),
                     SLRBLKINC,
                     offsetof32(SLR_USE, flags),
                     slrcomp))
            tree_flags |= TRF_BLOWN;
        }
}

/******************************************************************************
*
* sort the todo list
*
******************************************************************************/

static void
tree_sort_todo(void)
{
    trace_0(TRACE_MODULE_EVAL, "sort todo list\n");

    tree_sort(&todotab,
              sizeof32(TODO_ENTRY),
              TDOBLKINC,
              offsetof32(TODO_ENTRY, flags),
              todocomp);
}

/* end of ev_tree.c */
