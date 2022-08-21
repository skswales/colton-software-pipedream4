/* funclist.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module to handle lists of functions */

/* SKS October 1990 */

#ifndef __funclist_h
#define __funclist_h

/*
requires
*/

#ifndef __handlist_h
#include "handlist.h"
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
    P_P_LIST_BLKREF lbrp,
    funclist_proc proc,
    P_ANY handle,
    /*out*/ P_LIST_ITEMNO itemnop,
    S32 tag,
    S32 priority,
    size_t extradata);

extern S32
funclist_first(
    P_P_LIST_BLKREF lbrp,
    /*out*/ funclist_proc * proc,
    /*out*/ P_P_ANY handle,
    /*out*/ P_LIST_ITEMNO itemnop,
    S32 ascending);

extern S32
funclist_next(
    P_P_LIST_BLKREF lbrp,
    /*out*/ funclist_proc * proc,
    /*out*/ P_P_ANY handle,
    /*inout*/ P_LIST_ITEMNO itemnop,
    S32 ascending);

extern S32
funclist_readdata_ip(
    P_P_LIST_BLKREF lbrp,
    P_LIST_ITEMNO itemnop,
    /*out*/ P_ANY dest,
    size_t offset,
    size_t nbytes);

extern void
funclist_remove(
    P_P_LIST_BLKREF lbrp,
    funclist_proc proc,
    P_ANY handle);

extern S32
funclist_writedata_ip(
    P_P_LIST_BLKREF lbrp,
    P_LIST_ITEMNO itemnop,
    PC_ANY from,
    size_t offset,
    size_t nbytes);

/*
macros
*/

#define funclist_numprocs(lbrp) \
    nlist_numitem(*(lbrp))

#endif /* __funclist_h */

/* end of funclist.h */
