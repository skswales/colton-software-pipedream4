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
#ifndef __newhlist_h
#include "handlist.h"
#endif
#endif

/*
exported types
*/

typedef struct _nlists_blk
{
    P_LIST_BLKREF lbr;

    /* tuning parameters to use with auto-nlist_init() */
    S32         maxitemsize;
    S32         maxpoolsize;
}
NLISTS_BLK, * P_NLISTS_BLK;

/*
external functions
*/

extern P_ANY
collect_add_entry(
    P_NLISTS_BLK lbrp,
    S32 size,
    /*inout*/ P_LIST_ITEMNO key);

extern void
collect_compress(
    P_NLISTS_BLK lbrp);

extern S32
collect_copy(
    P_NLISTS_BLK new_lbrp,
    P_NLISTS_BLK old_lbrp);

extern void
collect_delete(
    P_NLISTS_BLK lbrp);

extern void
collect_delete_entry(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO  key);

extern P_ANY
collect_first(
    P_NLISTS_BLK lbrp,
    /*out*/ P_LIST_ITEMNO key);

extern P_ANY
collect_first_from(
    P_NLISTS_BLK  lbrp,
    /*inout*/ P_LIST_ITEMNO key);

extern P_ANY
collect_insert_entry(
    P_NLISTS_BLK lbrp,
    S32 size,
    PC_LIST_ITEMNO key);

extern P_ANY
collect_next(
    P_NLISTS_BLK  lbrp,
    /*inout*/ P_LIST_ITEMNO key);

extern P_ANY
collect_prev(
    P_NLISTS_BLK lbrp,
    /*inout*/ P_LIST_ITEMNO key);

extern P_ANY
collect_search(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO key);

extern S32
collect_subtract_entry(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO key);

#endif /* __collect_h */

/* end of collect.h */
