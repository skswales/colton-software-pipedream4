/* funclist.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module to handle lists of functions */

/* SKS October 1990 */

#ifndef __funclist_h
#define __funclist_h

/*
requires
*/

#ifndef __collect_h
#include "collect.h"
#endif

/*
structure
*/

/*
a type that can store the largest function pointer
*/

typedef U32 (* funclist_proc)(U32);

/*
value returned on enumeration end
*/

#define funclist_proc_none ((funclist_proc) 0L)

/*
higher numbers come to front of list: 0 passed to add uses this default
*/

#define FUNCLIST_DEFAULT_PRIORITY 0x4000

/*
exported functions
*/

_Check_return_
extern STATUS
funclist_add(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    funclist_proc proc,
    P_ANY handle,
    _OutRef_    P_LIST_ITEMNO itemnop,
    S32 tag,
    S32 priority,
    _InVal_     U32 extradata);

extern S32
funclist_first(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _Out_       funclist_proc * proc,
    _OutRef_    P_P_ANY handle,
    _OutRef_    P_LIST_ITEMNO itemnop,
    _InVal_     BOOL ascending);

extern S32
funclist_next(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _Out_       funclist_proc * proc,
    _OutRef_    P_P_ANY handle,
    _InoutRef_  P_LIST_ITEMNO itemnop,
    _InVal_     BOOL ascending);

extern S32
funclist_readdata_ip(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO itemno,
    _Out_writes_bytes_(nbytes) P_ANY dest,
    _InVal_     U32 offset,
    _InVal_     U32 nbytes);

extern void
funclist_remove(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    funclist_proc proc,
    P_ANY handle);

extern S32
funclist_writedata_ip(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO itemno,
    _In_reads_bytes_(nbytes) PC_ANY from,
    _InVal_     U32 offset,
    _InVal_     U32 nbytes);

#endif /* __funclist_h */

/* end of funclist.h */
