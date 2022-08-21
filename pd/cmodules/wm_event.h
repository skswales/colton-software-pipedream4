/* wm_event.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Centralised event acquisition and processing */

/* SKS October 1990 */

#ifndef __wm_event_h
#define __wm_event_h

/*
exported functions
*/

extern S32
wm_events_get(
    S32 fgNullsWanted);

/*
*** Null event processing ***
*/

/*
requires
*/

#ifndef __monotime_h
#include "monotime.h"
#endif

/*
structure
*/

#define NULL_QUERY 1
#define NULL_EVENT 2

#define NULL_EVENT_REASON_CODE int

#define NULL_EVENT_UNKNOWN    0x00

/* from null_events_do_query() */
#define NULL_EVENTS_OFF       0x10
#define NULL_EVENTS_REQUIRED  0x11

/* from null_events_do_events() */
#define NULL_EVENT_COMPLETED  0x20
#define NULL_EVENT_TIMED_OUT  0x21

#define NULL_EVENT_RETURN_CODE int

typedef struct NULL_EVENT_BLOCK
{
    NULL_EVENT_RETURN_CODE rc;      /*OUT*/

    P_ANY           client_handle;  /*IN*/
    MONOTIME        initial_time;   /*IN*/
    MONOTIMEDIFF    max_slice;      /*IN*/
}
NULL_EVENT_BLOCK, * P_NULL_EVENT_BLOCK;

typedef NULL_EVENT_RETURN_CODE (* P_PROC_NULL_EVENT) (
    _InoutRef_  P_NULL_EVENT_BLOCK p_null_event_block);

#define null_event_proto(_e_s, _p_proc_null_event) \
_e_s NULL_EVENT_RETURN_CODE _p_proc_null_event( \
    _InoutRef_  P_NULL_EVENT_BLOCK p_null_event_block)

/*
exported functions
*/

extern NULL_EVENT_RETURN_CODE
Null_DoEvent(void);

extern NULL_EVENT_RETURN_CODE
Null_DoQuery(void);

_Check_return_
extern STATUS
Null_EventHandlerAdd(
    P_PROC_NULL_EVENT proc,
    P_ANY client_handle,
    S32 priority /*0->default*/);

extern void
Null_EventHandlerRemove(
    P_PROC_NULL_EVENT proc,
    P_ANY client_handle);

#endif /* __wm_event_h */

/* end of wm_event.h */
