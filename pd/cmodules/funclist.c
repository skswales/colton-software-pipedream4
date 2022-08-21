/* funclist.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module to handle lists of functions */

/* SKS October 1990 */

#include "common/gflags.h"

#ifndef __funclist_h
#include "funclist.h"
#endif

#ifndef __handlist_h
#include "handlist.h"
#endif

#ifndef          __collect_h
#include "cmodules/collect.h"
#endif

#define TRACE_MODULE_FUNCLIST 0

/*
internal structure
*/

typedef struct _FUNCLIST_HANDLER
{
    funclist_proc proc;     /* procedure to call */
    P_ANY          handle;   /* handle to call with */
    S32           tag;      /* tag for caller */
    S32           priority; /* priority added at */

    /* extra data may follow */
}
* P_FUNCLIST_HANDLER;

/******************************************************************************
*
* add a procedure to a function list. caller gets back the proc,handle,tag
* on a first/next enumeration. priority determines the insertion order, where
* zero priority uses a default value. identical priority additions add the
* new procedure to the end of that block ie. still before any lower priority
* entries. caller can ask for store to be allocated with the proc,handle to
* be read from/written to by funclist_readdata_ip/writedata_ip later.
* currently can't change priority with this call, use remove/add again.
*
* NB. don't add procedures with non-zero tag unless you want to make life hard
*
******************************************************************************/

_Check_return_
extern STATUS
funclist_add(
    P_P_LIST_BLKREF lbrp,
    funclist_proc proc,
    P_ANY handle,
    /*out*/ P_LIST_ITEMNO itemnop,
    S32 tag,
    S32 priority,
    size_t extradata)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;
    LIST_ITEMNO        itemno;

    if(!priority)
        priority = FUNCLIST_DEFAULT_PRIORITY;

    if(!itemnop)
        itemnop = &itemno;

    trace_3(TRACE_MODULE_FUNCLIST,
            "funclist_add(&%p, (&%p,&%p))\n", report_ptr_cast(*lbrp), report_procedure_name(report_proc_cast(*proc)), report_ptr_cast(handle));

    /* search for handler on list */

    if(*lbrp)
        {
        for(hp = collect_first(nlbrp, itemnop); hp; hp = collect_next(nlbrp, itemnop))
            {
            if(hp->priority < priority)
                {
                /* found place in list to add before */
                break;
                }

            if((hp->proc == proc)  &&  (hp->handle == handle))
                {
                trace_2(TRACE_MODULE_FUNCLIST, "funclist_proc (&%p,&%p) already on list, retagging\n", report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));
                hp->tag = tag;
                return(STATUS_OK);
                }
            }

        /* if we ran out of list, add to end */
        *itemnop = nlist_numitem(*lbrp);
        }
    else
        {
        trace_0(TRACE_MODULE_FUNCLIST, "funclist_add: no list, so allocblkref\n");
        /* allocate new list if null list */
        if(!nlist_allocblkref(lbrp))
            return(status_nomem());
        else
            {
            /* retune temporarily */
            myassert1x(extradata <= 256, "extradata %s too big for current pool initialiser", extradata);
            list_deregister((P_LIST_BLOCK) *lbrp);
            list_init(      (P_LIST_BLOCK) *lbrp, (sizeof(*hp) + 256), (sizeof(*hp) + 256) * 8);
            list_register(  (P_LIST_BLOCK) *lbrp);
            }

        /* add to start */
        *itemnop = 0;
        }

    if((hp = collect_insert_entry(nlbrp, sizeof(*hp) + extradata, itemnop)) == NULL)
        {
        /* remove P_LIST_BLKREF if failed to add first block */
        collect_compress(nlbrp);
        return(status_nomem());
        }

    hp->proc     = proc;
    hp->handle   = handle;
    hp->tag      = tag;
    hp->priority = priority;

    return(STATUS_OK);
}

/******************************************************************************
*
* obtain the first procedure in a function list
*
* --out--
*
*      0    no procedures on list
*   <> 0    proc,handle,tag returned
*
******************************************************************************/

extern S32
funclist_first(
    P_P_LIST_BLKREF lbrp,
    /*out*/ funclist_proc * proc,
    /*out*/ P_P_ANY handle,
    /*out*/ P_LIST_ITEMNO itemnop,
    S32 ascending)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;
    S32                tag;

    trace_2(TRACE_MODULE_FUNCLIST,
            "funclist_first(&%p, %s)", report_ptr_cast(*lbrp), trace_boolstring(ascending));

    if(*lbrp)
        {
        *itemnop = nlist_numitem(*lbrp);

        if(itemnop)
            {
            if(ascending)
                *itemnop = 0;
            else
                if(*itemnop)
                    --(*itemnop);

            if((hp = collect_first_from(nlbrp, itemnop)) != NULL)
                {
                trace_3(TRACE_MODULE_FUNCLIST,
                        " yields (&%p,&%p) %d\n", report_procedure_name(report_proc_cast(hp->proc)), report_ptr_cast(hp->handle), hp->tag);
                *proc   = hp->proc;
                *handle = hp->handle;
                tag     = hp->tag;

                return(tag);
                }
            }
        else
            trace_0(TRACE_MODULE_FUNCLIST,
                    " yields NONE -- found nothing on list\n");
        }
    else
        trace_0(TRACE_MODULE_FUNCLIST, " yields NONE -- found no list\n");

    *proc = funclist_proc_none;
    return(0);
}

/******************************************************************************
*
* obtain the next procedure in a function list
*
* --out--
*
*      0    no more procedures on list
*   <> 0    proc,handle,tag returned
*
******************************************************************************/

extern S32
funclist_next(
    P_P_LIST_BLKREF lbrp,
    /*out*/ funclist_proc * proc,
    /*out*/ P_P_ANY handle,
    /*inout*/ P_LIST_ITEMNO itemnop,
    S32 ascending)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;
    S32                tag;

    trace_2(TRACE_MODULE_FUNCLIST,
            "funclist_next(&%p, %s)", report_ptr_cast(*lbrp), trace_boolstring(ascending));

    if(*lbrp)
        {
        if((hp = (ascending ? collect_next : collect_prev) (nlbrp, itemnop)) != NULL)
            {
            trace_3(TRACE_MODULE_FUNCLIST,
                    " yields (&%p,&%p) %d\n", report_procedure_name(report_proc_cast(hp->proc)), report_ptr_cast(hp->handle), hp->tag);
            *proc   = hp->proc;
            *handle = hp->handle;
            tag     = hp->tag;

            return(tag);
            }
        }

    *proc = funclist_proc_none;
    trace_0(TRACE_MODULE_FUNCLIST, " yields NONE\n");
    return(0);
}

/******************************************************************************
*
* read data stored with a procedure on a function list
*
* --out--
*
*      0    proc,handle not on list
*   <> 0    read nBytes from offset in stored data to given location
*
******************************************************************************/

extern S32
funclist_readdata_ip(
    P_P_LIST_BLKREF lbrp,
    P_LIST_ITEMNO itemnop,
    /*out*/ P_ANY dest,
    size_t offset,
    size_t nbytes)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;

    trace_5(TRACE_MODULE_FUNCLIST,
            "funclist_readdata_ip(&%p, %u): offset %u nbytes %u to &%p",
            report_ptr_cast(*lbrp), *itemnop, offset, nbytes, report_ptr_cast(dest));

    /* got position of handler on list */
    if((hp = collect_search(nlbrp, itemnop)) != NULL)
        {
        trace_1(TRACE_MODULE_FUNCLIST, " from &%p\n", PtrAddBytes(PC_BYTE, (hp + 1), offset));
        void_memcpy32(dest, PtrAddBytes(PC_BYTE, (hp + 1), offset), nbytes);

        return(1);
        }
    else
        trace_0(TRACE_MODULE_FUNCLIST, " -- not found\n");

    return(0);
}

/******************************************************************************
*
* remove a procedure from a function list
*
******************************************************************************/

extern void
funclist_remove(
    P_P_LIST_BLKREF lbrp,
    funclist_proc proc,
    P_ANY handle)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;
    LIST_ITEMNO        itemno;

    trace_3(TRACE_MODULE_FUNCLIST,
            "funclist_remove(&%p, (&%p,&%p))\n", report_ptr_cast(*lbrp), report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));

    /* search for handler on list */

    for(hp = collect_first(nlbrp, &itemno); hp; hp = collect_next(nlbrp, &itemno))
        {
        if((hp->proc == proc) && (hp->handle == handle))
            {
            itemno = nlist_atitem(*lbrp);

            (void) collect_delete_entry(nlbrp, &itemno);

            collect_compress(nlbrp);

            return;
            }
        }
}

/******************************************************************************
*
* set data stored with a procedure on a function list
*
* --out--
*
*      0    proc,handle not on list
*   <> 0    set nbytes from offset in stored data from given location
*
******************************************************************************/

extern S32
funclist_writedata_ip(
    P_P_LIST_BLKREF lbrp,
    P_LIST_ITEMNO itemnop,
    PC_ANY from,
    size_t offset,
    size_t nbytes)
{
    P_NLISTS_BLK       nlbrp = (P_NLISTS_BLK) lbrp;
    P_FUNCLIST_HANDLER hp;

    trace_5(TRACE_MODULE_FUNCLIST,
            "funclist_writedata_ip(&%p, %u): offset %u nbytes %u from &%p",
            report_ptr_cast(*lbrp), *itemnop, offset, nbytes, report_ptr_cast(from));

    /* got position of handler on list */
    if((hp = collect_search(nlbrp, itemnop)) != NULL)
        {
        trace_1(TRACE_MODULE_FUNCLIST, " to &%p\n", PtrAddBytes(P_BYTE, (hp + 1), offset));
        void_memcpy32(PtrAddBytes(P_BYTE, (hp + 1), offset), from, nbytes);

        return(1);
        }
    else
        trace_0(TRACE_MODULE_FUNCLIST, " -- not found\n");

    return(0);
}

/* end of funclist.c */
