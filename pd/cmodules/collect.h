/* collect.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* MRJC December 1991 */

#ifndef __collect_h
#define __collect_h

/*
requires
*/

#ifndef __handlist_h
#include "handlist.h"
#endif

/*
exported types
*/

typedef struct _nlists_blk
{
    P_LIST_BLOCK lbr;

    /* tuning parameters to use with auto list_init() */
    S32 maxitemsize;
    S32 maxpoolsize;
}
NLISTS_BLK, * P_NLISTS_BLK;

/*
external functions
*/

extern P_ANY
collect_add_entry(
    _InoutRef_  P_NLISTS_BLK nlbrp,
    S32 size,
    _InoutRef_opt_ P_LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status);

_Check_return_
extern STATUS
collect_alloc_list_block(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    _In_        S32 maxitemsize,
    _In_        S32 maxpoolsize);

extern void
collect_compress(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block);

_Check_return_
extern STATUS
collect_copy(
    _InoutRef_  P_NLISTS_BLK new_nlbrp,
    _InRef_     P_P_LIST_BLOCK old_p_p_list_block);

extern void
collect_delete(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block);

extern void
collect_delete_entry(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO key);

extern P_ANY
collect_first(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _OutRef_opt_ P_LIST_ITEMNO key);

extern P_ANY
collect_first_from(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InoutRef_opt_ P_LIST_ITEMNO key);

extern P_ANY
collect_insert_entry(
    _InoutRef_  P_NLISTS_BLK nlbrp,
    S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status);

extern P_ANY
collect_next(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InoutRef_opt_ P_LIST_ITEMNO key);

extern P_ANY
collect_prev(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InoutRef_opt_ P_LIST_ITEMNO key);

#define collect_search(p_p_list_block, key) \
    _list_gotoitemcontents(*p_p_list_block, key)

extern void
collect_subtract_entry(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO key);

#endif /* __collect_h */

/* end of collect.h */
