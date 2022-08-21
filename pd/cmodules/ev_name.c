/* ev_name.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that manages the list of named resources for the evaluator */

/* MRJC January 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

static void
custom_change_id(
    EV_NAMEID id_to,
    EV_NAMEID id_from);

PROC_QSORT_PROTO(static, customid_lookcomp, EV_CUSTOM);

static void
name_change_id(
    EV_NAMEID id_to,
    EV_NAMEID id_from);

PROC_QSORT_PROTO(static, nameid_lookcomp, EV_NAME);

/*
resource lists
*/

DEPTABLE custom_def = DEPTABLE_INIT;

static EV_NAMEID next_custom_key = 0;

#define MACDEFINC 2

DEPTABLE names_def = DEPTABLE_INIT;

static EV_NAMEID next_name_key  = 0;

#define NAMDEFINC 5

/******************************************************************************
*
* when two documents merge via the change document mechanism,
* this routine is called to sort out name and custom definitions
*
******************************************************************************/

extern void
change_doc_mac_nam(
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from)
{
    P_EV_CUSTOM p_ev_custom;
    P_EV_NAME p_ev_name;

    while((p_ev_custom = custom_def.ptr) != NULL)
    {
        EV_NAMEID i;

        for(i = 0; i < custom_def.next; ++i, ++p_ev_custom)
        {
            if(p_ev_custom->flags & TRF_TOBEDEL)
                continue;

            if( (p_ev_custom->owner.docno == docno_from) &&
                (p_ev_custom->flags & TRF_UNDEFINED) )
            {
                EV_NAMEID new_num = find_custom_in_list(docno_to, p_ev_custom->id);

                if(new_num >= 0)
                {
                    P_EV_CUSTOM t_p_ev_custom = custom_ptr_must(new_num);

                    custom_change_id(t_p_ev_custom->key, p_ev_custom->key);
                    p_ev_custom->flags |= TRF_TOBEDEL;
                    custom_def.flags |= TRF_TOBEDEL;
                    custom_def.mindel = MIN(custom_def.mindel, i);
                    ev_todo_add_custom_dependents(t_p_ev_custom->key);

                    trace_2(TRACE_MODULE_EVAL,
                            "change_doc_mac_nam changed custom: %d, to: %d",
                            p_ev_custom->key, t_p_ev_custom->key);
                    /* re-loop size the world has moved under our feet */
                    break;
                }
            }
        }

        if(i >= custom_def.next)
            break;
    }

    while((p_ev_name = names_def.ptr) != NULL)
    {
        EV_NAMEID i;

        for(i = 0; i < names_def.next; ++i, ++p_ev_name)
        {
            if(p_ev_name->flags & TRF_TOBEDEL)
                continue;

            if( (p_ev_name->owner.docno == docno_from) &&
                (p_ev_name->flags & TRF_UNDEFINED) )
            {
                EV_NAMEID new_num = find_name_in_list(docno_to, p_ev_name->id);

                if(new_num >= 0)
                {
                    P_EV_NAME t_p_ev_name = name_ptr_must(new_num);

                    name_change_id(t_p_ev_name->key, p_ev_name->key);
                    p_ev_name->flags |= TRF_TOBEDEL;
                    names_def.flags |= TRF_TOBEDEL;
                    names_def.mindel = MIN(names_def.mindel, i);
                    ev_todo_add_name_dependents(t_p_ev_name->key);

                    trace_2(TRACE_MODULE_EVAL,
                            "change_doc_mac_nam changed name: %d, to: %d",
                            p_ev_name->key, t_p_ev_name->key);
                    /* re-loop size the world has moved under our feet */
                    break;
                }
            }
        }

        if(i >= names_def.next)
            break;
    }
}

/******************************************************************************
*
* given a document and a custom funcion, ensure
* this [document]custom combination exists in the
* resource table
*
* --out--
* <0  new custom function couldn't be inserted
* >=0 entry number of custom function
*
******************************************************************************/

extern EV_NAMEID
ensure_custom_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR custom_name)
{
    EV_NAMEID customid;
    EV_NAMEID new_entry;
    P_EV_CUSTOM p_ev_custom;

    if((customid = find_custom_in_list(owner_docno, custom_name)) >= 0)
        return(customid);

    if(dep_table_check_add_one(&custom_def,
                               sizeof(EV_CUSTOM),
                               MACDEFINC) < 0)
        return(-1);

    new_entry = custom_def.next++;

    p_ev_custom = custom_ptr_must(new_entry);

    p_ev_custom->key = next_custom_key++;
    p_ev_custom->owner.docno = owner_docno;
    /* this stops ev_uref removing a custom reference
     * from the table when it doesn't yet belong anywhere
    */
    p_ev_custom->owner.col = EV_MAX_COL - 1;
    p_ev_custom->owner.row = EV_MAX_ROW - 1;
    p_ev_custom->flags     = TRF_UNDEFINED;
    strcpy(p_ev_custom->id, custom_name);

    ev_p_ss_doc_from_docno_must(p_ev_custom->owner.docno)->custom_ref_count += 1;

    return(new_entry);
}

/******************************************************************************
*
* given a document and a name, ensure
* this [document]name combination exists in the
* resource table
*
* --out--
* <0  new name couldn't be inserted
* >=0 entry number of name
*
******************************************************************************/

extern EV_NAMEID
ensure_name_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR name)
{
    EV_NAMEID name_num;
    EV_NAMEID new_entry;
    P_EV_NAME p_ev_name;

    if((name_num = find_name_in_list(owner_docno, name)) >= 0)
        return(name_num);

    if(dep_table_check_add_one(&names_def,
                               sizeof(EV_NAME),
                               NAMDEFINC) < 0)
        return(-1);

    new_entry = names_def.next++;

    p_ev_name = name_ptr_must(new_entry);

    p_ev_name->key         = next_name_key++;
    p_ev_name->owner.docno = owner_docno;
    /* col, row need to be large enough not to be caught by accident
     */
    p_ev_name->owner.col   = EV_MAX_COL - 1;
    p_ev_name->owner.row   = EV_MAX_ROW - 1;
    p_ev_name->def_data.did_num = RPN_DAT_BLANK;
    p_ev_name->flags       = TRF_UNDEFINED;
    p_ev_name->visited     = 0;
    strcpy(p_ev_name->id, name);

    ev_p_ss_doc_from_docno_must(p_ev_name->owner.docno)->nam_ref_count += 1;

    return(new_entry);
}

/******************************************************************************
*
* switch custom deletions off
*
******************************************************************************/

extern void
ev_custom_del_hold(void)
{
    custom_def.flags |= TRF_DELHOLD;
}

/******************************************************************************
*
* allow custom function deletions again
*
******************************************************************************/

extern void
ev_custom_del_release(void)
{
    custom_def.flags &= ~TRF_DELHOLD;
}

/******************************************************************************
*
* switch name deletions off
*
******************************************************************************/

extern void
ev_name_del_hold(void)
{
    names_def.flags |= TRF_DELHOLD;
}

/******************************************************************************
*
* allow name deletions again
*
******************************************************************************/

extern void
ev_name_del_release(void)
{
    names_def.flags &= ~TRF_DELHOLD;
}

/******************************************************************************
*
* define a name with the given argument
*
* --in--
* set undefine to make the name
* 'undefined'
*
* --out--
* < 0 error
* >=0 OK
*
******************************************************************************/

_Check_return_
extern S32
ev_name_make(
    P_U8 name_text,
    _InVal_     EV_DOCNO doc_from,
    P_U8 name_def_text,
    _InRef_     PC_EV_OPTBLOCK p_optblock,
    S32 undefine)
{
    S32 res;

    /* check name of name is OK */
    if(ident_validate(name_text) < 0)
        return(create_error(EVAL_ERR_BADIDENT));

    res = 0;
    if(undefine)
    {
        EV_NAMEID name_num = find_name_in_list(doc_from, name_text);

        if(name_num >= 0)
        {
            P_EV_NAME p_ev_name = name_ptr_must(name_num);

            ev_todo_add_name_dependents(p_ev_name->key);

            ss_data_free_resources(&p_ev_name->def_data);

            p_ev_name->flags |= TRF_UNDEFINED;
        }
        else
            res = create_error(EVAL_ERR_NAMEUNDEF);
    }
    else
    {
        EV_DATA data;

        if((res = ss_recog_constant(&data, doc_from, name_def_text, p_optblock, 1)) > 0)
        {
            EV_NAMEID name_key;

            res = name_make(&name_key, doc_from, name_text, &data);

            /* name_make has copied away resources */
            ss_data_free_resources(&data);
        }
        else if(!res)
            res = create_error(EVAL_ERR_NA);
    }

    return(res);
}

/******************************************************************************
*
* search internal resource list for a custom function
*
* --out--
* <0  custom function not found
* >=0 entry number of custom function
*
******************************************************************************/

_Check_return_
extern EV_NAMEID
find_custom_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR custom_name)
{
    EV_NAMEID i;
    P_EV_CUSTOM p_ev_custom;

    if((p_ev_custom = custom_def.ptr) == NULL)
        return(-1);

    for(i = 0; i < custom_def.next; ++i, ++p_ev_custom)
    {
        if(p_ev_custom->flags & TRF_TOBEDEL)
            continue;

        if(owner_docno != p_ev_custom->owner.docno)
            continue;

        if(0 == _stricmp(p_ev_custom->id, custom_name))
            return(i);
    }

    return(-1);
}

/******************************************************************************
*
* search name definition table for a name
*
* --out--
* <0  name not found
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern EV_NAMEID
find_name_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR name)
{
    EV_NAMEID i;
    P_EV_NAME p_ev_name;

    if((p_ev_name = names_def.ptr) == NULL)
        return(-1);

    for(i = 0; i < names_def.next; ++i, ++p_ev_name)
    {
        if(p_ev_name->flags & TRF_TOBEDEL)
            continue;

        if(owner_docno != p_ev_name->owner.docno)
            continue;

        if(0 == _stricmp(p_ev_name->id, name))
            return(i);
    }

    return(-1);
}

/******************************************************************************
*
* change use of a given custom id to another
*
******************************************************************************/

static void
custom_change_id(
    EV_NAMEID id_to,
    EV_NAMEID id_from)
{
    EV_TRENT i;
    P_CUSTOM_USE mep;

    if((mep = tree_macptr(0)) == NULL)
        return;

    for(i = 0; i < custom_use_deptable.next; ++i, ++mep)
    {
        if(mep->flags & TRF_TOBEDEL)
            continue;

        if(mep->custom_to == id_from)
        {
            P_EV_CELL p_ev_cell;

            if(ev_travel(&p_ev_cell, &mep->byslr) > 0)
                write_nameid(id_to, p_ev_cell->rpn.var.rpn_str + mep->byoffset);

            mep->custom_to = id_to;
            custom_use_deptable.sorted = 0;
        }
    }
}

/******************************************************************************
*
* lookup custom id in custom table
*
* --out--
* <0  custom not in table
* >=0 entry number of custom
*
******************************************************************************/

_Check_return_
extern EV_NAMEID
custom_def_find(
    EV_NAMEID key)
{
    EV_CUSTOM temp;
    P_EV_CUSTOM p_ev_custom, custom_listp;

    custom_list_sort();

    temp.key = key;
    if((custom_listp = custom_def.ptr) == NULL)
        return(-1);

    if((p_ev_custom = bsearch(&temp,
                         custom_listp,
                         custom_def.sorted,
                         sizeof(EV_CUSTOM),
                         customid_lookcomp)
       ) == NULL
      )
        return(-1);

    return(p_ev_custom - custom_listp);
}

/******************************************************************************
*
* sort custom definition table
*
******************************************************************************/

extern void
custom_list_sort(void)
{
    /* check through custom table to see if the deletion of a
     * custom use has rendered the definition record useless
     */
    if((custom_def.flags & TRF_CHECKUSE) && !(custom_def.flags & TRF_DELHOLD))
    {
        P_EV_CUSTOM p_ev_custom;

        if((p_ev_custom = custom_def.ptr) != NULL)
        {
            EV_NAMEID i;

            for(i = 0; i < custom_def.next; ++i, ++p_ev_custom)
            {
                if(p_ev_custom->flags & TRF_TOBEDEL)
                    continue;

                /* custom reference must be undefined before we can delete it */
                if(!(p_ev_custom->flags & TRF_UNDEFINED))
                    continue;

                if(search_for_custom_use(p_ev_custom->key) < 0)
                {
                    P_SS_DOC p_ss_doc;

                    if(NULL != (p_ss_doc = ev_p_ss_doc_from_docno(p_ev_custom->owner.docno)))
                        p_ss_doc->custom_ref_count -= 1;

                    p_ev_custom->flags |= TRF_TOBEDEL;
                    custom_def.flags |= TRF_TOBEDEL;
                    custom_def.mindel = MIN(custom_def.mindel, i);

                    trace_1(TRACE_MODULE_EVAL,
                            "custom_list_sort deleting custom: %s",
                            p_ev_custom->id);
                }
            }
        }

        custom_def.flags &= ~TRF_CHECKUSE;
    }

    tree_sort(&custom_def,
              sizeof(EV_CUSTOM),
              MACDEFINC,
              offsetof(EV_CUSTOM, flags),
              customid_lookcomp);
}

/******************************************************************************
*
* compare custom ids
*
******************************************************************************/

PROC_QSORT_PROTO(static, customid_lookcomp, EV_CUSTOM)
{
    /* NB no current_p_docu global register furtling required */

    if(((P_EV_CUSTOM) _arg1)->key == ((P_EV_CUSTOM) _arg2)->key)
        return(0);

    return(((P_EV_CUSTOM) _arg1)->key > ((P_EV_CUSTOM) _arg2)->key ? 1 : -1);
}

/******************************************************************************
*
* given a name id, update the target
* document's name reference count
*
******************************************************************************/

static void
nam_ref_count_update(
    P_EV_NAME p_ev_name,
    S32 update)
{
    switch(p_ev_name->def_data.did_num)
    {
    case RPN_DAT_SLR:
        ev_p_ss_doc_from_docno_must(p_ev_name->def_data.arg.slr.docno)->nam_ref_count += update;
        trace_2(TRACE_MODULE_EVAL,
                "nam_ref_count_update doc: %d, ref now: %d",
                p_ev_name->def_data.arg.slr.docno, ev_p_ss_doc_from_docno_must(p_ev_name->def_data.arg.slr.docno)->nam_ref_count);
        break;

    case RPN_DAT_RANGE:
        ev_p_ss_doc_from_docno_must(p_ev_name->def_data.arg.range.s.docno)->nam_ref_count += update;
        trace_2(TRACE_MODULE_EVAL,
                "nam_ref_count_update doc: %d, ref now: %d",
                p_ev_name->def_data.arg.range.s.docno, ev_p_ss_doc_from_docno_must(p_ev_name->def_data.arg.range.s.docno)->nam_ref_count);
        break;
    }
}

/******************************************************************************
*
* change use of a given name id to another
*
******************************************************************************/

static void
name_change_id(
    EV_NAMEID id_to,
    EV_NAMEID id_from)
{
    EV_TRENT i;
    P_NAME_USE nep;

    if((nep = tree_namptr(0)) == NULL)
        return;

    for(i = 0; i < namtab.next; ++i, ++nep)
    {
        if(nep->flags & TRF_TOBEDEL)
            continue;

        if(nep->nameto == id_from)
        {
            P_EV_CELL p_ev_cell;

            if(ev_travel(&p_ev_cell, &nep->byslr) > 0)
                write_nameid(id_to, p_ev_cell->rpn.var.rpn_str + nep->byoffset);

            nep->nameto = id_to;
            namtab.sorted = 0;
        }
    }
}

/******************************************************************************
*
* lookup id in named resource table
*
* --out--
* <0  name not in table
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern EV_NAMEID
name_def_find(
    EV_NAMEID key)
{
    EV_NAME temp;
    P_EV_NAME p_ev_name, name_listp;

    name_list_sort();

    temp.key = key;
    if((name_listp = names_def.ptr) == NULL)
        return(-1);

    if((p_ev_name =
            bsearch(&temp,
                    name_listp,
                    names_def.sorted,
                    sizeof(EV_NAME),
                    nameid_lookcomp)
       ) == NULL
      )
        return(-1);

    return(p_ev_name - name_listp);
}

/******************************************************************************
*
* free any resources that are
* owned by a name definition
*
******************************************************************************/

extern void
name_free_resources(
    P_EV_NAME p_ev_name)
{
    nam_ref_count_update(p_ev_name, -1);
    ss_data_free_resources(&p_ev_name->def_data);
    p_ev_name->def_data.did_num = RPN_DAT_BLANK;
}

/******************************************************************************
*
* sort name definition table
*
******************************************************************************/

extern void
name_list_sort(void)
{
    /* check through name table to see if the deletion of a
     * name use has rendered the definition record useless
     */
    if((names_def.flags & TRF_CHECKUSE) && !(names_def.flags & TRF_DELHOLD))
    {
        P_EV_NAME p_ev_name;

        if((p_ev_name = names_def.ptr) != NULL)
        {
            EV_NAMEID i;

            for(i = 0; i < names_def.next; ++i, ++p_ev_name)
            {
                if(p_ev_name->flags & TRF_TOBEDEL)
                    continue;

                /* name must be undefined before we can delete it */
                if(!(p_ev_name->flags & TRF_UNDEFINED))
                    continue;

                if(search_for_name_use(p_ev_name->key) < 0)
                {
                    P_SS_DOC p_ss_doc;

                    name_free_resources(p_ev_name);

                    if(NULL != (p_ss_doc = ev_p_ss_doc_from_docno(p_ev_name->owner.docno)))
                        p_ss_doc->nam_ref_count -= 1;

                    p_ev_name->flags |= TRF_TOBEDEL;
                    names_def.flags |= TRF_TOBEDEL;
                    names_def.mindel = MIN(names_def.mindel, i);

                    trace_1(TRACE_MODULE_EVAL, "name_list_sort deleting name: %s", p_ev_name->id);
                }
            }
        }

        names_def.flags &= ~TRF_CHECKUSE;
    }

    tree_sort(&names_def,
              sizeof(EV_NAME),
              NAMDEFINC,
              offsetof(EV_NAME, flags),
              nameid_lookcomp);
}

/******************************************************************************
*
* compare names
*
******************************************************************************/

PROC_QSORT_PROTO(static, nameid_lookcomp, EV_NAME)
{
    /* NB no current_p_docu global register furtling required */

    if(((P_EV_NAME) _arg1)->key == ((P_EV_NAME) _arg2)->key)
        return(0);

    return(((P_EV_NAME) _arg1)->key > ((P_EV_NAME) _arg2)->key ? 1 : -1);
}

/******************************************************************************
*
* make a name with a given value
*
******************************************************************************/

extern S32
name_make(
    EV_NAMEID *nameidp,
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_USTR name,
    P_EV_DATA p_ev_data_in)
{
    S32 res = 0;

    if(ident_validate(name) >= 0)
    {
        EV_NAMEID name_num = ensure_name_in_list(docno, name);

        if(name_num >= 0)
        {
            P_EV_NAME p_ev_name = name_ptr_must(name_num);

            /* free any resources owned by the name at the moment */
            name_free_resources(p_ev_name);

            ss_data_resource_copy(&p_ev_name->def_data, p_ev_data_in);

            /* dereference names and slrs inside arrays */
            if(p_ev_name->def_data.did_num == RPN_TMP_ARRAY)
                data_limit_types(&p_ev_name->def_data, FALSE);
            ev_todo_add_name_dependents(p_ev_name->key);
            p_ev_name->flags &= ~TRF_UNDEFINED;
            nam_ref_count_update(p_ev_name, 1);

            *nameidp = p_ev_name->key;
        }
        else
            res = status_nomem();
    }
    else
        res = create_error(EVAL_ERR_BADIDENT);

    return(res);
}

/* end of ev_name.c */
