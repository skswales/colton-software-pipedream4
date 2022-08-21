/* funclist.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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

typedef struct FUNCLIST_HANDLER
{
    funclist_proc proc;     /* procedure to call */
    P_ANY          handle;   /* handle to call with */
    S32           tag;      /* tag for caller */
    S32           priority; /* priority added at */

    /* extra data may follow */
}
FUNCLIST_HANDLER, * P_FUNCLIST_HANDLER;

/******************************************************************************
*
* add a procedure to a function list. caller gets back the proc,handle,tag
* on a first/next enumeration. priority determines the insertion order, where
* zero priority uses a default value. identical priority additions add the
* new procedure to the end of that block i.e. still before any lower priority
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
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    funclist_proc proc,
    P_ANY handle,
    _OutRef_    P_LIST_ITEMNO itemnop,
    S32 tag,
    S32 priority,
    _InVal_     U32 extradata)
{
    P_FUNCLIST_HANDLER hp;
    NLISTS_BLK new_nlb;
    STATUS status;

    if(!priority)
        priority = FUNCLIST_DEFAULT_PRIORITY;

    trace_3(TRACE_MODULE_FUNCLIST,
            "funclist_add(&%p, (&%p,&%p))", report_ptr_cast(p_p_list_block), report_procedure_name(report_proc_cast(*proc)), report_ptr_cast(handle));

    /* search for handler on list */

    if(NULL != *p_p_list_block)
    {
        for(hp = collect_first(FUNCLIST_HANDLER, p_p_list_block, itemnop);
            hp;
            hp = collect_next( FUNCLIST_HANDLER, p_p_list_block, itemnop))
        {
            if(hp->priority < priority)
            {
                /* found place in list to add before */
                break;
            }

            if((hp->proc == proc)  &&  (hp->handle == handle))
            {
                trace_2(TRACE_MODULE_FUNCLIST, "funclist_proc (&%p,&%p) already on list, retagging", report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));
                hp->tag = tag;
                return(STATUS_OK);
            }
        }

        /* if we ran out of list, add to end */
        *itemnop = list_numitem(*p_p_list_block);
    }
    else
    {
        /* add to start */
        *itemnop = 0;
    }

    myassert1x(extradata <= 256, "extradata %u too big for current pool initialiser", extradata);

    new_nlb.lbr = *p_p_list_block;
    new_nlb.maxitemsize = (sizeof32(*hp) + 256);
    new_nlb.maxpoolsize = new_nlb.maxitemsize * 8;

    if(NULL == (hp = collect_insert_entry_bytes(FUNCLIST_HANDLER, &new_nlb, sizeof32(*hp) + extradata, *itemnop, &status)))
        return(status);

    *p_p_list_block = new_nlb.lbr;

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
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _Out_       funclist_proc * proc,
    _OutRef_    P_P_ANY handle,
    _OutRef_    P_LIST_ITEMNO itemnop,
    _InVal_     BOOL ascending)
{
    P_FUNCLIST_HANDLER hp;
    S32                tag;

    trace_2(TRACE_MODULE_FUNCLIST,
            "funclist_first(&%p, %s)", report_ptr_cast(p_p_list_block), report_boolstring(ascending));

    *itemnop = list_numitem(*p_p_list_block);

    if(*itemnop)
    {
        if(ascending)
            *itemnop = 0;
        else
            if(*itemnop)
                --(*itemnop);

        if((hp = collect_first_from(FUNCLIST_HANDLER, p_p_list_block, itemnop)) != NULL)
        {
            trace_3(TRACE_MODULE_FUNCLIST,
                    " yields (&%p,&%p) %d", report_procedure_name(report_proc_cast(hp->proc)), report_ptr_cast(hp->handle), hp->tag);
            *proc   = hp->proc;
            *handle = hp->handle;
            tag     = hp->tag;

            return(tag);
        }
    }

    *proc = funclist_proc_none;
    *handle = NULL;
    trace_0(TRACE_MODULE_FUNCLIST, " yields NONE");
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
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _Out_       funclist_proc * proc,
    _OutRef_    P_P_ANY handle,
    _InoutRef_  P_LIST_ITEMNO itemnop,
    _InVal_     BOOL ascending)
{
    P_FUNCLIST_HANDLER hp;
    S32                tag;

    trace_2(TRACE_MODULE_FUNCLIST,
            "funclist_next(&%p, %s)", report_ptr_cast(p_p_list_block), report_boolstring(ascending));

    if(ascending)
        hp = collect_next(FUNCLIST_HANDLER, p_p_list_block, itemnop);
    else
        hp = collect_prev(FUNCLIST_HANDLER, p_p_list_block, itemnop);

    if(NULL != hp)
    {
        trace_3(TRACE_MODULE_FUNCLIST,
                " yields (&%p,&%p) %d", report_procedure_name(report_proc_cast(hp->proc)), report_ptr_cast(hp->handle), hp->tag);
        *proc   = hp->proc;
        *handle = hp->handle;
        tag     = hp->tag;

        return(tag);
    }

    *proc = funclist_proc_none;
    *handle = NULL;
    trace_0(TRACE_MODULE_FUNCLIST, " yields NONE");
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
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO itemno,
    _Out_writes_bytes_(nbytes) P_ANY dest,
    _InVal_     U32 offset,
    _InVal_     U32 nbytes)
{
    P_FUNCLIST_HANDLER hp;

    trace_5(TRACE_MODULE_FUNCLIST,
            "funclist_readdata_ip(&%p, %u): offset %u nbytes %u to &%p",
            report_ptr_cast(p_p_list_block), itemno, offset, nbytes, report_ptr_cast(dest));

    /* got position of handler on list */
    if((hp = collect_goto_item(FUNCLIST_HANDLER, p_p_list_block, itemno)) != NULL)
    {
        trace_1(TRACE_MODULE_FUNCLIST, " from &%p", PtrAddBytes(PC_BYTE, (hp + 1), offset));
        memcpy32(dest, PtrAddBytes(PC_BYTE, (hp + 1), offset), nbytes);
        return(1);
    }

    trace_0(TRACE_MODULE_FUNCLIST, " -- not found");
    return(0);
}

/******************************************************************************
*
* remove a procedure from a function list
*
******************************************************************************/

extern void
funclist_remove(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    funclist_proc proc,
    P_ANY handle)
{
    P_FUNCLIST_HANDLER hp;
    LIST_ITEMNO        itemno;

    trace_3(TRACE_MODULE_FUNCLIST,
            "funclist_remove(&%p, (&%p,&%p))",
            report_ptr_cast(p_p_list_block), report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));

    /* search for handler on list */

    for(hp = collect_first(FUNCLIST_HANDLER, p_p_list_block, &itemno);
        hp;
        hp = collect_next( FUNCLIST_HANDLER, p_p_list_block, &itemno))
    {
        if((hp->proc == proc) && (hp->handle == handle))
        {
            itemno = list_atitem(*p_p_list_block);

            (void) collect_delete_entry(p_p_list_block, itemno);

            collect_compress(p_p_list_block);

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
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO itemno,
    _In_reads_bytes_(nbytes) PC_ANY from,
    _InVal_     U32 offset,
    _InVal_     U32 nbytes)
{
    P_FUNCLIST_HANDLER hp;

    trace_5(TRACE_MODULE_FUNCLIST,
            "funclist_writedata_ip(&%p, %u): offset %u nbytes %u from &%p",
            report_ptr_cast(p_p_list_block), itemno, offset, nbytes, report_ptr_cast(from));

    /* got position of handler on list */
    if((hp = collect_goto_item(FUNCLIST_HANDLER, p_p_list_block, itemno)) != NULL)
    {
        trace_1(TRACE_MODULE_FUNCLIST, " to &%p", PtrAddBytes(P_BYTE, (hp + 1), offset));
        memcpy32(PtrAddBytes(P_BYTE, (hp + 1), offset), from, nbytes);
        return(1);
    }

    trace_0(TRACE_MODULE_FUNCLIST, " -- not found");
    return(0);
}

/* end of funclist.c */
