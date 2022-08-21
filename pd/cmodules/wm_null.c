/* wm_null.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Idle processing */

/* SKS October 1990 */

#include "common/gflags.h"

#include "wm_event.h"

#include "funclist.h"

static
struct _NULL_EVENT_STATICS
{
    MONOTIMEDIFF max_slice;

    P_LIST_BLOCK event_list;
}
null_ =
{
    MONOTIME_VALUE(250),

    NULL
};

/*
data stored for us by funclist
*/

typedef struct _null_funclist_extradata
{
    NULL_EVENT_RETURN_CODE req; /* stores whether handler had null requests or not */
}
null_funclist_extradata;

/******************************************************************************
*
* call background null procedures - simple round-robin scheduler with each
* process requested to time out after a certain period, where the scheduler
* will time out for a process if it returns having taken too long. In the
* case of any timeout, subsequent procedures will have to wait for the next
* background null event. null processors can be on the list but refuse to be
* called by saying no on their being queried.
*
* NB. background null event processors may be reentered if they do any event
* processing, so watch out!
*
* --out--
*
*   NULL_EVENT_COMPLETED
*   NULL_EVENT_TIMED_OUT
*
******************************************************************************/

extern NULL_EVENT_RETURN_CODE
Null_DoEvent(void)
{
    /* everything we need to restart the null list */
    static LIST_ITEMNO   item = -1;
    static funclist_proc proc;
    static P_ANY         handle;
    static S32           tag;

    MONOTIME initialTime;
    NULL_EVENT_RETURN_CODE res = NULL_EVENT_COMPLETED;
    NULL_EVENT_RETURN_CODE req;
    NULL_EVENT_BLOCK null_event_block;

    initialTime = monotime();

    do  {
        /* require to start at head of list? */
        if(item == -1)
            tag = funclist_first(&null_.event_list,
                                 &proc, &handle, &item, TRUE);
        else
            {
            /* can we call this handler? */
            funclist_readdata_ip(&null_.event_list,
                                 item, &req,
                                 offsetof(null_funclist_extradata, req),
                                 sizeof(req));

            /* call the handler if required */
            if(req == NULL_EVENTS_REQUIRED)
                {
                P_PROC_NULL_EVENT np = (P_PROC_NULL_EVENT) proc;

                null_event_block.rc = NULL_EVENT;
                null_event_block.client_handle = handle;
                null_event_block.initial_time = initialTime;
                null_event_block.max_slice = null_.max_slice;

                res = (* np) (&null_event_block);
                }

            /* obtain next callee */
            tag = funclist_next( &null_.event_list,
                                 &proc, &handle, &item, TRUE);

            /* if handler not called, loop with this new one */
            if(req != NULL_EVENTS_REQUIRED)
                continue;

            if(res != NULL_EVENT_COMPLETED)
                {
                /* "null event proc &%p,&%p returned bad value %d", proc, handle, res); */
                break;
                }

            /* did this process take longer than allowed? */
            if(monotime_diff(initialTime) >= null_.max_slice)
                {
                res = NULL_EVENT_TIMED_OUT;
                break;
                }
            }
        }
    while(tag);

    /* if ended null list set up to restart next time */
    if(!tag)
        item = -1;

    return(res);
}

/******************************************************************************
*
* call query procedures in reverse order of registration to determine
* whether or not background null events are required. If one procedure
* replies in the affirmative, the others are still called to allow them
* to state whether they would NOT like events themselves.
*
* --out--
*
*   NULL_EVENTS_OFF
*   NULL_EVENTS_REQUIRED
*
******************************************************************************/

extern NULL_EVENT_RETURN_CODE
Null_DoQuery(void)
{
    LIST_ITEMNO            item;
    funclist_proc          proc;
    S32                    tag;
    NULL_EVENT_RETURN_CODE res = NULL_EVENTS_OFF;
    NULL_EVENT_RETURN_CODE res2;
    NULL_EVENT_BLOCK null_event_block;

    for(tag = funclist_first(&null_.event_list,
                             &proc, &null_event_block.client_handle, &item, FALSE);
        tag;
        tag = funclist_next( &null_.event_list,
                             &proc, &null_event_block.client_handle, &item, FALSE))
        {
        P_PROC_NULL_EVENT np = (P_PROC_NULL_EVENT) proc;

        null_event_block.rc = NULL_QUERY;

        /* query this handler */
        res2 = (* np) (&null_event_block);

        if(res2 == NULL_EVENTS_REQUIRED)
            res = NULL_EVENTS_REQUIRED;

        /* remember handler request for interlock */
        funclist_writedata_ip(&null_.event_list,
                              item, &res2,
                              offsetof(null_funclist_extradata, req),
                              sizeof(res2));
        }

    return(res);
}

/******************************************************************************
*
* add/remove procedure to be called each background null event
*
******************************************************************************/

_Check_return_
extern STATUS
Null_EventHandler(
    P_PROC_NULL_EVENT proc,
    P_ANY client_handle,
    _InVal_     BOOL add,
    S32 priority)
{
    if(add)
        {
        LIST_ITEMNO item;
        NULL_EVENT_RETURN_CODE res2 = NULL_EVENTS_REQUIRED;

        status_return(
            funclist_add(&null_.event_list,
                         (funclist_proc) proc, client_handle,
                         &item,
                         1 /*non-zero tag*/,
                         priority,
                         sizeof(null_funclist_extradata)));

        /* ensure that if events are given before querying this one responds safely */
        funclist_writedata_ip(&null_.event_list,
                              item, &res2,
                              offsetof(null_funclist_extradata, req),
                              sizeof(res2));
        }
    else
        {
        funclist_remove(&null_.event_list,
                        (funclist_proc) proc, client_handle);
        }

    return(STATUS_OK);
}

/* end of wm_null.c */
