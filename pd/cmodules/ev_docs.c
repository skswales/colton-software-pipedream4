/* ev_docs.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Routines that manage the inter-document links for the evaluator */

/* MRJC February 1991 */

#include "common/gflags.h"

#include "datafmt.h"

#include "handlist.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

static void
doc_free(
    _InVal_     EV_DOCNO docno);

static void
doc_free_table(
    P_DEPTABLE dpp);

static void
ensure_doc_in_list(
    P_EV_DOCNO p_docno_array,
    _InoutRef_  P_S32 count,
    _InVal_     EV_DOCNO docno);

static void
ensure_refs_to_name_in_list(
    P_EV_DOCNO p_docno_array,
    P_S32 count,
    EV_NAMEID name);

/*
document table variables
*/

static S32 doc_reuse_hold_count = 0;

/******************************************************************************
*
* take the list of documents in memory and find their handles
*
******************************************************************************/

static S32
doc_bind_docno(
    _InVal_     EV_DOCNO docno_in,
    _OutRef_    P_EV_DOCNO p_mapped_docno)
{
    P_SS_DOC p_ss_doc_in = ev_p_ss_doc_from_docno_must(docno_in);
    S32 res = NAME_NONE;
    EV_DOCNO docno;

    *p_mapped_docno = DOCNO_NONE;

    if(NULL == p_ss_doc_in->docu_name.leaf_name)
        {
        assert(p_ss_doc_in->is_docu_thunk);
        return(NAME_NONE);
        }

    for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
        {
        P_SS_DOC p_ss_doc;

        if(docno_void_entry(docno))
            continue;

        p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

        if(NULL == p_ss_doc->docu_name.leaf_name)
            {
            assert(p_ss_doc->is_docu_thunk);
            continue;
            }

        /* do wholenames match? */
        if(0 == name_compare(&p_ss_doc->docu_name, &p_ss_doc_in->docu_name))
            {
            assert(docno_in == docno);
            *p_mapped_docno = docno;
            return(NAME_OK);
            }

        /* leafnames can match iff both are 'sloppy' */
        if(0 == _stricmp(p_ss_doc->docu_name.leaf_name, p_ss_doc_in->docu_name.leaf_name))
            {
            if((!p_ss_doc->docu_name.flags.path_name_supplied) &&
               (!p_ss_doc_in->docu_name.flags.path_name_supplied))
                {
                if(NAME_NONE == res)
                    {
                    *p_mapped_docno = docno; /* remember the first match anyhow */
                    res = NAME_OK;
                    }
                else
                    {
                    res = NAME_MANY;
                    }
                /* break; keep going in case we get a precise match */
                }
            }
         }

     return(res);
}

static void
doc_bind_docnos(
    _InVal_     EV_DOCNO new_docno)
{
    EV_DOCNO docno;

    reportf("doc_bind_docnos(new_docno=%u)", new_docno);

    for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
    {
        P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);
        DOCNO mapped_docno;
        S32 res;

#if CHECKING
        if(docno == new_docno)
            assert(p_ss_doc);
#endif
        if(NULL == p_ss_doc)
            continue;

        if(!p_ss_doc->is_docu_thunk)
            { /* fully loaded documents are easy */
            mapped_docno = docno;
            res = NAME_OK;
            }
        else
            res = doc_bind_docno(docno, &mapped_docno);

        switch(res)
        {
        case NAME_OK:
            reportf("bind docno: %u, thunk=%s, mapped_docno: %u, path(%s) leaf(%s) %s\n",
                    docno, report_boolstring(p_ss_doc->is_docu_thunk), mapped_docno,
                    report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name), report_boolstring(p_ss_doc->docu_name.flags.path_name_supplied));
            break;

        case NAME_MANY:
            reportf("bind docno: %u, thunk=%s, REFS_MANY, path(%s) leaf(%s) %s",
                    docno, report_boolstring(p_ss_doc->is_docu_thunk),
                    report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name), report_boolstring(p_ss_doc->docu_name.flags.path_name_supplied));
            break;

        case NAME_NONE:
            reportf("bind docno: %u, thunk=%s, REFS_NONE, path(%s) leaf(%s) %s",
                    docno, report_boolstring(p_ss_doc->is_docu_thunk),
                    report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name), report_boolstring(p_ss_doc->docu_name.flags.path_name_supplied));

            if(docno == new_docno)
                reportf("********* docu thunk not freed as it is being created");
            else if(!docno_void_entry(docno))
                reportf("********* docu thunk not freed as !docno_void_entry(%u); slr: %d, rng: %d, ext: %d, nam: %d, mac: %d\n",
                        docno,
                        p_ss_doc->slr_table.next,
                        p_ss_doc->range_table.next,
                        p_ss_doc->exttab.next,
                        p_ss_doc->nam_ref_count,
                        p_ss_doc->custom_ref_count);
            else
                {
                reportf("********* doc to be freed: docno %u", docno);
                doc_free(docno);
                continue;
                }

            assert(p_ss_doc->is_docu_thunk);
            break;
        }

        /* mark for document recalc if there's a change */
        if(mapped_docno != docno)
            ev_todo_add_doc_dependents(docno);
    }

    /* try to release some stack */
    stack_release_check();

    reportf("%s: names: %d, customs: %d, name uses: %d, custom uses: %d\n",
            __func__, names_def.next, custom_def.next, namtab.next, custom_use_table.next);
}

/******************************************************************************
*
* alter document numbers in sheets and tree
* when a document number changes
*
******************************************************************************/

static void
doc_change(
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from)
{
    P_SS_DOC docep_to;
    P_SS_DOC docep_from;
    UREF_PARM urefb;

    trace_2(TRACE_MODULE_EVAL | TRACE_OUT, "doc_change docto: %d, docfrom: %d\n", docno_to, docno_from);

    /* change document numbers */
    urefb.slr1.docno = docno_to;
    urefb.slr2.docno = docno_from;
    urefb.action   = UREF_CHANGEDOC;

    ev_uref(&urefb);

    /* merge trees of old into trees of new */
    if((docep_from = ev_p_ss_doc_from_docno(docno_from)) != NULL)
        {
        EV_TRENT i;
        extentp eep;
        P_RANGE_USE rep;
        P_SLR_USE sep;

        for(i = 0, eep = tree_extptr(docep_from, 0); i < docep_from->exttab.next; ++i, ++eep)
            if(doc_move_extref(docep_from, docno_from, eep, i) < 0)
                break;

        for(i = 0, rep = tree_rngptr(docep_from, 0); i < docep_from->range_table.next; ++i, ++rep)
            if(doc_move_rngref(docep_from, docno_from, rep, i) < 0)
                break;

        for(i = 0, sep = tree_slrptr(docep_from, 0); i < docep_from->slr_table.next; ++i, ++sep)
            if(doc_move_slrref(docep_from, docno_from, sep, i) < 0)
                break;

        /* transfer dependency counts */
        if((docep_to = ev_p_ss_doc_from_docno(docno_to)) != NULL)
            {
            docep_to->nam_ref_count += docep_from->nam_ref_count;
            docep_from->nam_ref_count = 0;
            docep_to->custom_ref_count += docep_from->custom_ref_count;
            docep_from->custom_ref_count = 0;
            }
        }

    /* blow away old document entry */
    doc_free(docno_from);
}

_Check_return_
extern EV_DOCNO
ev_doc_establish_doc_from_docu_name(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    EV_DOCNO docno;
    P_SS_DOC p_ss_doc;

    /* either use given doc, or get a new one */
    if(DOCNO_NONE != (docno = docno_find_name(p_docu_name, DOCNO_NONE, FALSE)))
        {
        /*trace_1(TRACE_MODULE_EVAL, "matched existing docno: %u\n");*/
        return(docno);
        }

    reportf("ev_doc_establish_doc_from_docu_name(path(%s) leaf(%s) %s)\n",
            report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied));

    /* create a new docu thunk */
    if(NULL != (p_ss_doc = ev_create_new_ss_doc(p_docu_name, &docno)))
        reportf("ev_doc_establish_doc_from_docu_name(path(%s) leaf(%s) %s) established new docno: %u",
                report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied),
                docno);

    return(docno);
}

/******************************************************************************
*
* search document table for an entry that matches the name supplied
* but avoiding matching the given document
*
******************************************************************************/

_Check_return_
extern EV_DOCNO
docno_find_name(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO docno_avoid,
    _InVal_     BOOL avoid_thunks)
{
    EV_DOCNO res = DOCNO_NONE;
    EV_DOCNO docno;
    P_SS_DOC p_ss_doc;
    char wholename1[BUF_EV_LONGNAMLEN];
    char wholename2[BUF_EV_LONGNAMLEN];

    name_make_wholename(p_docu_name, wholename1, elemof32(wholename1));

    for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
        {
        if(docno_avoid == docno)
            continue;

        if(docno_void_entry(docno))
            continue;

        p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

        if(avoid_thunks)
            if(p_ss_doc->is_docu_thunk)
                continue;

        if(NULL == p_ss_doc->docu_name.leaf_name)
            continue;

        /* do wholenames match? */
        name_make_wholename(&p_ss_doc->docu_name, wholename2, elemof32(wholename2));

        /*if(0 == name_compare(&p_ss_doc->docu_name, p_docu_name))*/
        if(0 == _stricmp(wholename1, wholename2))
            return(docno);

        /* leafnames can match iff both are 'sloppy' */
        if(0 == _stricmp(p_ss_doc->docu_name.leaf_name, p_docu_name->leaf_name))
            {
            if((!p_ss_doc->docu_name.flags.path_name_supplied) &&
               (!p_docu_name->flags.path_name_supplied))
                {
                if(DOCNO_NONE == res)
                    res = docno;
                /* break; keep going in case we get a precise match */
                }
            }
        }

    return(res);
}

/******************************************************************************
*
* remove a document from the table and free its resources
*
******************************************************************************/

static void
doc_free(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

    assert(p_ss_doc != NULL);

    if(p_ss_doc != NULL)
        {
        reportf("********* freeing docno: %u, path(%s) leaf(%s)\n",
                docno, report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name));

        ev_tree_close(docno);

        doc_free_table(&p_ss_doc->exttab);
        doc_free_table(&p_ss_doc->range_table);
        doc_free_table(&p_ss_doc->slr_table);

        ev_destroy_ss_doc(docno);
        }
}

/******************************************************************************
*
* free a dependency table
*
******************************************************************************/

static void
doc_free_table(
    P_DEPTABLE dpp)
{
    list_disposeptr(&dpp->ptr);
    dpp->next  = 0;
    dpp->size  = 0;
    dpp->flags = 0;
}

/******************************************************************************
*
* --in--
* doc points to document number
* --out--
* doc_array is array[DOCNO_MAX] to contain list of dependents
* returns number of dependent documents found
*
******************************************************************************/

static S32
doc_get_dependent_docs(
    P_EV_DOCNO p_docno,
    P_EV_DOCNO p_docno_array)
{
    P_SS_DOC p_ss_doc;
    S32 count = 0;

    if((p_ss_doc = ev_p_ss_doc_from_docno(*p_docno)) != NULL)
        {
        P_SLR_USE sep;
        P_RANGE_USE rep;
        P_CUSTOM_USE mep;
        P_EV_NAME p_ev_name;

        /* MRJC 27.3.92 - ensure we are dependent on ourselves */
        ensure_doc_in_list(p_docno_array, &count, *p_docno);

        if((sep = tree_slrptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT i;

            for(i = 0; i < p_ss_doc->slr_table.next; ++i, ++sep)
                if(!(sep->flags & TRF_TOBEDEL))
                    ensure_doc_in_list(p_docno_array, &count, sep->byslr.docno);
            }

        if((rep = tree_rngptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT i;

            for(i = 0; i < p_ss_doc->range_table.next; ++i, ++rep)
                if(!(rep->flags & TRF_TOBEDEL))
                    ensure_doc_in_list(p_docno_array, &count, rep->byslr.docno);
            }

        if(p_ss_doc->nam_ref_count && (p_ev_name = name_ptr(0)) != NULL)
            {
            EV_NAMEID i;

            for(i = 0; i < names_def.next; ++i, ++p_ev_name)
                {
                if(p_ev_name->flags & TRF_TOBEDEL)
                    continue;

                switch(p_ev_name->def.did_num)
                    {
                    case RPN_DAT_SLR:
                        /* if name refers to this document */
                        if(p_ev_name->def.arg.slr.docno == *p_docno)
                            ensure_refs_to_name_in_list(p_docno_array,
                                                        &count,
                                                        p_ev_name->key);
                        break;

                    case RPN_DAT_RANGE:
                        /* if name refers to this document */
                        if(p_ev_name->def.arg.range.s.docno == *p_docno)
                            ensure_refs_to_name_in_list(p_docno_array,
                                                        &count,
                                                        p_ev_name->key);
                        break;
                    }
                }
            }

        /* find all the sheets which use one of our custom functions */
        if((mep = tree_macptr(0)) != NULL)
            {
            EV_TRENT i;

            for(i = 0; i < custom_use_table.next; ++i, ++mep)
                if(!(mep->flags & TRF_TOBEDEL))
                    {
                    EV_NAMEID custom_num;

                    if((custom_num = custom_def_find(mep->custom_to)) >= 0)
                        {
                        P_EV_CUSTOM p_ev_custom;

                        if((p_ev_custom = custom_ptr(custom_num)) != NULL)
                            {
                            if(p_ev_custom->owner.docno == *p_docno)
                                ensure_doc_in_list(p_docno_array,
                                                   &count,
                                                   mep->byslr.docno);
                            }
                        }
                    }
            }
        }

    return(count);
}

/******************************************************************************
*
* move an external reference to another document's tree
*
******************************************************************************/

extern S32
doc_move_extref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    extentp eep,
    EV_TRENT eix)
{
    S32 res;

    if(!(eep->flags & TRF_TOBEDEL) && (eep->refto.s.docno != docno_from))
        {
        if((res = ev_add_extdependency(eep->exthandle,
                                       eep->inthandle,
                                       NULL,
                                       eep->proc,
                                       &eep->refto)) < 0)
            {
            P_SS_DOC docep_to;
            if(NULL != (docep_to = ev_p_ss_doc_from_docno(eep->refto.s.docno)))
                docep_to->flags |= DCF_TREEDAMAGED;
            }

        eep->flags |= TRF_TOBEDEL;
        docep_from->exttab.flags |= TRF_TOBEDEL;
        docep_from->exttab.mindel = MIN(docep_from->exttab.mindel, eix);
        }
    else
        res = 0;

    return(res);
}

/******************************************************************************
*
* move a range reference to another document's tree
*
******************************************************************************/

extern S32
doc_move_rngref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    P_RANGE_USE rep,
    EV_TRENT rix)
{
    S32 res;

    if(!(rep->flags & TRF_TOBEDEL) && (rep->refto.s.docno != docno_from))
        {
        struct ev_grub_state grubb;

        grubb.slr = rep->byslr;
        grubb.data.arg.range = rep->refto;
        grubb.data.did_num = RPN_DAT_RANGE;
        grubb.byoffset = rep->byoffset;

        if((res = ev_add_rngdependency(&grubb)) < 0)
            {
            P_SS_DOC docep_to;
            if(NULL != (docep_to = ev_p_ss_doc_from_docno(rep->refto.s.docno)))
                docep_to->flags |= DCF_TREEDAMAGED;
            }

        rep->flags |= TRF_TOBEDEL;
        docep_from->range_table.flags |= TRF_TOBEDEL;
        docep_from->range_table.mindel = MIN(docep_from->range_table.mindel, rix);
        }
    else
        res = 0;

    return(res);
}

/******************************************************************************
*
* move a slot reference to another document's tree
*
******************************************************************************/

extern S32
doc_move_slrref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    P_SLR_USE sep,
    EV_TRENT six)
{
    S32 res;

    if(!(sep->flags & TRF_TOBEDEL) && (sep->refto.docno != docno_from))
        {
        struct ev_grub_state grubb;

        grubb.slr          = sep->byslr;
        grubb.data.arg.slr = sep->refto;
        grubb.data.did_num = RPN_DAT_SLR;
        grubb.byoffset     = sep->byoffset;

        if((res = ev_add_slrdependency(&grubb)) < 0)
            {
            P_SS_DOC docep_to;
            if(NULL != (docep_to = ev_p_ss_doc_from_docno(sep->refto.docno)))
                docep_to->flags |= DCF_TREEDAMAGED;
            }

        sep->flags |= TRF_TOBEDEL;
        docep_from->slr_table.flags |= TRF_TOBEDEL;
        docep_from->slr_table.mindel = MIN(docep_from->slr_table.mindel, six);
        }
    else
        res = 0;

    return(res);
}

/******************************************************************************
*
* prevent re-use of possibly void document
* entries temporarily (used during compilation)
*
******************************************************************************/

extern void
doc_reuse_hold(void)
{
    doc_reuse_hold_count += 1;
}

/******************************************************************************
*
* release document reuse
*
******************************************************************************/

extern void
doc_reuse_release(void)
{
    doc_reuse_hold_count -= 1;
}

/******************************************************************************
*
* work out whether this entry in document tree is worth keeping
*
* --out--
* TRUE  = can reuse entry
* FALSE = entry still required
*
******************************************************************************/

_Check_return_
extern BOOL /* TRUE=can reuse entry */
docno_void_entry(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

    if(NULL == p_ss_doc)
        return(TRUE);

    if(!p_ss_doc->is_docu_thunk)
        /* it is a fully loaded document */
        return(FALSE);

    /* it is a docu thunk */

    if(doc_reuse_hold_count && p_ss_doc->docu_name.leaf_name)
        return(FALSE);

    /* does this docu thunk have any dependents ? */
    if(p_ss_doc->exttab.ptr || p_ss_doc->range_table.ptr || p_ss_doc->slr_table.ptr || p_ss_doc->nam_ref_count || p_ss_doc->custom_ref_count)
        return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* given a pointer to a document list, ensure that
* a given document is in the list
*
******************************************************************************/

static void
ensure_doc_in_list(
    P_EV_DOCNO p_docno_array,
    _InoutRef_  P_S32 count,
    _InVal_     EV_DOCNO docno)
{
    S32 i;
    P_EV_DOCNO p_docno;

    for(i = 0, p_docno = p_docno_array; i < *count; ++i, ++p_docno)
        if(docno == *p_docno)
            return;

    *p_docno = docno;
    ++(*count);
}

/******************************************************************************
*
* step along all dependencies for a given
* name and put them in the document list
*
******************************************************************************/

static void
ensure_refs_to_name_in_list(
    P_EV_DOCNO p_docno_array,
    P_S32 count,
    EV_NAMEID name)
{
    EV_TRENT nix;

    if((nix = search_for_name_use(name)) >= 0)
        {
        P_NAME_USE nep;

        if((nep = tree_namptr(nix)) != NULL)
            {
            NAME_USE key;

            key.nameto = name;

            while(nix < namtab.next && !namcomp(nep, &key))
                {
                ensure_doc_in_list(p_docno_array, count, nep->byslr.docno);
                ++nep;
                ++nix;
                }
            }
        }
}

/******************************************************************************
*
* remove all traces of a file when it is closed and the handle becomes invalid
*
******************************************************************************/

extern void
ev_close_sheet(
    _InVal_     DOCNO docno)
{
    DOCU_NAME temp_docu_name;
    P_SS_DOC p_ss_doc;
    EV_DOCNO new_docno;
    UREF_PARM urefb;

    reportf("ev_close_sheet docno: %u", docno);

    if(docno == DOCNO_NONE)
        return;

    /* blow away the evaluator stack; frees any resources,
     * pending calculations and alert/input boxes
     */
    urefb.action = UREF_CLOSE;
    stack_zap(&urefb);

    /* mark all this document's tree entries for delete */
    ev_tree_close(docno);

    /* actually delete all those entries */
    tree_sort_all();

    /* save data for re-establish attempt - we have
     * to remove this instance so that docno_find_name doesn't
     * find us again
     */
    reportf("ev_close_sheet: try to find another home for the references for docno=%u", docno);

    p_ss_doc = ev_p_ss_doc_from_docno_must(docno);
    temp_docu_name = p_ss_doc->docu_name;

    p_ss_doc->docu_name.leaf_name = NULL;
    new_docno = docno_find_name(&temp_docu_name, DOCNO_NONE, FALSE);
    p_ss_doc->docu_name.leaf_name = temp_docu_name.leaf_name;

    if(DOCNO_NONE != new_docno)
        {
        reportf("found other home for the references, so doc_change(new_docno=%u)", new_docno);
        doc_change(new_docno, docno);
        }
    else
        {
        reportf("no other home for the references, hopefully will be doc_free()'d");
        /*doc_free(docno);*/
        doc_bind_docnos(DOCNO_NONE);
        }
}

/******************************************************************************
*
* read document name and return document
* number - creating entry in the document
* list if there is not one there already
*
* --out--
* number of characters read inc []
*
******************************************************************************/

extern S32
ev_doc_establish_doc_from_name(
    _In_z_      PC_USTR in_str,
    _InVal_     EV_DOCNO cur_docno,
    _OutRef_    P_EV_DOCNO p_docno)
{
    char name_given[BUF_EV_LONGNAMLEN];
    char path_name[BUF_EV_LONGNAMLEN];
    S32 count;
    DOCU_NAME docu_name;

    /* default if nothing read */
    *p_docno = cur_docno;

    /* read in name to temporary buffer */
    while(*in_str == ' ')
        ++in_str;

    if(0 == (count = recog_extref(name_given, elemof32(name_given), in_str, 0)))
        return(count);

    name_init(&docu_name);

    if(file_is_rooted(name_given))
        { /* this is rare */
        PTSTR leaf_p_ev_name = file_leafname(name_given);
        void_strnkpy(path_name, elemof32(path_name), name_given, leaf_p_ev_name - name_given);
        docu_name.path_name = path_name;
        docu_name.leaf_name = leaf_p_ev_name;
        docu_name.flags.path_name_supplied = 1;
        }
    else
        { /* establish a relative reference where possible - don't force path_name_supplied here */
        P_SS_DOC p_ss_doc;

        if(NULL != (p_ss_doc = ev_p_ss_doc_from_docno(cur_docno)))
            docu_name.path_name = p_ss_doc->docu_name.path_name;

        docu_name.leaf_name = name_given;
        }

    /* make sure the document is in the list,
     * and get a document number for it
     */
    if(DOCNO_NONE == (*p_docno = ev_doc_establish_doc_from_docu_name(&docu_name)))
        return(-1);

    return(count);
}

/******************************************************************************
*
* initialise the sequence of dependent sheets for the given docno
*
******************************************************************************/

extern S32
ev_doc_get_dep_docs_for_sheet(
    _InVal_     DOCNO docno,
    _OutRef_    P_EV_DOCNO p_docno,
    P_EV_DOCNO p_docno_array)
{
    if((*p_docno = docno) == DOCNO_NONE)
        return(0);

    return(doc_get_dependent_docs(p_docno, p_docno_array));
}

/******************************************************************************
*
* initialise the sequence of supporting sheets for the given docno
*
* --out--
* p_docno_array is array[DOCNO_MAX] to contain list of dependents
* returns number of dependent documents found
*
******************************************************************************/

extern S32
ev_doc_get_sup_docs_for_sheet(
    _InVal_     DOCNO docno,
    _OutRef_    P_EV_DOCNO p_docno,
    P_EV_DOCNO p_docno_array)
{
    S32 count = 0;
    EV_DOCNO i_docno;
    P_NAME_USE nep;
    P_CUSTOM_USE mep;

    if((*p_docno = docno) == DOCNO_NONE)
        return(0);

    ensure_doc_in_list(p_docno_array, &count, *p_docno);

    for(i_docno = DOCNO_FIRST; i_docno < docu_array_size; ++i_docno)
        {
        P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(i_docno);
        P_SLR_USE sep;
        P_RANGE_USE rep;
        BOOL this_doc = FALSE;

        if(NULL == p_ss_doc)
            continue;

        if((sep = tree_slrptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT i;

            for(i = 0; i < p_ss_doc->slr_table.next; ++i, ++sep)
                if(!(sep->flags & TRF_TOBEDEL) && (sep->byslr.docno == *p_docno))
                    {
                    ensure_doc_in_list(p_docno_array, &count, i_docno);
                    this_doc = TRUE;
                    break;
                    }
            }

        if(!this_doc && (rep = tree_rngptr(p_ss_doc, 0)) != NULL)
            {
            EV_TRENT i;

            for(i = 0; i < p_ss_doc->range_table.next; ++i, ++rep)
                if(!(rep->flags & TRF_TOBEDEL) && (rep->byslr.docno == *p_docno))
                    {
                    ensure_doc_in_list(p_docno_array, &count, i_docno);
                    this_doc = TRUE;
                    break;
                    }
            }
        }

    if((nep = tree_namptr(0)) != NULL)
        {
        EV_TRENT i;

        for(i = 0; i < namtab.next; ++i, ++nep)
            if(!(nep->flags & TRF_TOBEDEL) && (nep->byslr.docno == *p_docno))
                {
                EV_NAMEID name_num;

                if((name_num = name_def_find(nep->nameto)) >= 0)
                    {
                    P_EV_NAME p_ev_name;

                    if((p_ev_name = name_ptr(name_num)) != NULL)
                        {
                        switch(p_ev_name->def.did_num)
                            {
                            case RPN_DAT_SLR:
                                ensure_doc_in_list(p_docno_array,
                                                   &count,
                                                   p_ev_name->def.arg.slr.docno);
                                break;

                            case RPN_DAT_RANGE:
                                ensure_doc_in_list(p_docno_array,
                                                   &count,
                                                   p_ev_name->def.arg.range.s.docno);
                                break;
                            }

                        ensure_doc_in_list(p_docno_array,
                                           &count,
                                           p_ev_name->owner.docno);
                        }
                    }
                }
        }

    /* find all the supporting custom functions */
    if((mep = tree_macptr(0)) != NULL)
        {
        EV_TRENT i;

        for(i = 0; i < custom_use_table.next; ++i, ++mep)
            if(!(mep->flags & TRF_TOBEDEL) && (mep->byslr.docno == *p_docno))
                {
                EV_NAMEID custom_num;

                if((custom_num = custom_def_find(mep->custom_to)) >= 0)
                    {
                    P_EV_CUSTOM p_ev_custom;

                    if((p_ev_custom = custom_ptr(custom_num)) != NULL)
                        {
                        if(p_ev_custom->owner.docno != DOCNO_NONE)
                            ensure_doc_in_list(p_docno_array,
                                               &count,
                                               p_ev_custom->owner.docno);
                        }
                    }
                }
        }

    return(count);
}

/******************************************************************************
*
* external call to say about being a custom sheet
*
******************************************************************************/

extern S32
ev_doc_is_custom_sheet(
    _InVal_     DOCNO docno)
{
    if(docno == DOCNO_NONE)
        return(0);

    return(doc_check_custom(docno));
}

/******************************************************************************
*
* update sheet link information when a file is renamed
*
******************************************************************************/

extern void
ev_rename_sheet(
    _InRef_     PC_DOCU_NAME p_new_docu_name,
    _InVal_     DOCNO docno)
{
    P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

    reportf("ev_rename_sheet docno: %d", 
            docno);
    reportf("    from: path(%s) leaf(%s) %s",
            report_tstr(p_ss_doc->docu_name.path_name),
            report_tstr(p_ss_doc->docu_name.leaf_name),
            report_boolstring(p_ss_doc->docu_name.flags.path_name_supplied));
    reportf("    to:   path(%s) leaf(%s) %s",
            report_tstr(p_new_docu_name->path_name),
            report_tstr(p_new_docu_name->leaf_name),
            report_boolstring(p_new_docu_name->flags.path_name_supplied));

    /* check for rename to same name */
    if(0 != name_compare(p_new_docu_name, &p_ss_doc->docu_name))
        {
        UREF_PARM urefb;
        EV_DOCNO docno_array[DOCNO_MAX];
        EV_DOCNO cur_docno, new_docno;
        P_EV_DOCNO i_docno;
        S32 n_docs, i;

        /* MRJC 26.3.92 tell hangers on about renaming */
        n_docs = ev_doc_get_dep_docs_for_sheet(docno, &cur_docno, docno_array);

        for(i = 0, i_docno = docno_array; i < n_docs; ++i, ++i_docno)
            {
            urefb.action = UREF_RENAME;
            urefb.slr1.docno = *i_docno;
            urefb.slr2.docno = cur_docno;
            ev_uref(&urefb);
            }

        /* are there unresolved references with the new name ? */
        if(DOCNO_NONE != (new_docno = docno_find_name(p_new_docu_name, DOCNO_NONE, FALSE)))
            {
            P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno_must(new_docno);

            if(p_ss_doc->is_docu_thunk)
                doc_change(docno, new_docno);
            }

        /* copy in the new name */
        name_free(&p_ss_doc->docu_name);
        (void) name_dup(&p_ss_doc->docu_name, p_new_docu_name);

        /*doc_bind_docnos(DOCNO_NONE);*/
        }
    else
        reportf("-- same name");
}

/******************************************************************************
*
* write document name, given docno
*
* --out--
* number of characters written
*
******************************************************************************/

extern S32
ev_write_docname(
    P_U8 out_str,
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from)
{
    S32 len, path_len;
    P_SS_DOC docep_to, docep_from;
    char buffer[BUF_EV_LONGNAMLEN];
    P_U8 nam_str;

    if(docno_to == DOCNO_NONE)
        return(0);

    if((docep_to = ev_p_ss_doc_from_docno(docno_to)) == NULL)
        return(0);

    docep_from = ev_p_ss_doc_from_docno_must(docno_from);

    /* strip out a pathname common to target and source documents */
    nam_str = buffer;
    if(docep_to->docu_name.flags.path_name_supplied)
        {
        name_make_wholename(&docep_to->docu_name, buffer, elemof32(buffer));
        path_len = 0;
        if(docep_from->docu_name.path_name)
            {
            path_len = strlen(docep_from->docu_name.path_name);
            if(0 == _strnicmp(buffer, docep_from->docu_name.path_name, path_len))
                nam_str = buffer + path_len;
            }
        }
    else
        strcpy(buffer, docep_to->docu_name.leaf_name);

    len = 0;
    out_str[len++] = '[';
    strcpy(out_str + len, nam_str);
    len += strlen(nam_str);
    out_str[len++] = ']';
    out_str[len] = '\0';

    return(len);
}

/******************************************************************************
*
* compare two names
*
******************************************************************************/

_Check_return_
extern S32
name_compare(
    _InRef_     PC_DOCU_NAME p_docu_name_1,
    _InRef_     PC_DOCU_NAME p_docu_name_2)
{
    char wholename1[BUF_EV_LONGNAMLEN];
    char wholename2[BUF_EV_LONGNAMLEN];

    name_make_wholename(p_docu_name_1, wholename1, elemof32(wholename1));
    name_make_wholename(p_docu_name_2, wholename2, elemof32(wholename2));

    return(_stricmp(wholename1, wholename2));
}

/******************************************************************************
*
* make a copy of filename resources
*
******************************************************************************/

_Check_return_
extern S32
name_dup(
    _OutRef_    P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in)
{
    S32 res = 1;

    p_docu_name_out->flags = p_docu_name_in->flags;

    if(str_set(&p_docu_name_out->path_name, p_docu_name_in->path_name) >= 0)
        {
        if(str_set(&p_docu_name_out->leaf_name, p_docu_name_in->leaf_name) < 0)
            {
            str_clr(&p_docu_name_out->path_name);
            res = 0;
            }
        }
    else
        res = 0;

    return(res);
}

/******************************************************************************
*
* free a name block
*
******************************************************************************/

extern void
name_free(
    _InoutRef_  P_DOCU_NAME p_docu_name)
{
    str_clr(&p_docu_name->path_name);
    str_clr(&p_docu_name->leaf_name);
    p_docu_name->flags.path_name_supplied = 0;
}

/******************************************************************************
*
* make a whole name from its parts
*
******************************************************************************/

extern void
name_make_wholename(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer)
{
    assert(0 != elemof_buffer);
    buffer[0] = '\0';

    if(NULL != p_docu_name->path_name)
        void_strkpy(buffer, elemof_buffer, p_docu_name->path_name);

    assert(NULL != p_docu_name->leaf_name);
    void_strkat(buffer, elemof_buffer, p_docu_name->leaf_name);
}

/* end of ev_docs.c */
