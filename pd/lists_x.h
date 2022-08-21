/* lists_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* External definitions for lists in PipeDream */

/* MRJC March 1989 */

/*
data definition
*/

typedef struct _list
{
    S32 key;
    uchar value[1];
}
LIST;

/*
function declarations
*/

extern LIST *
add_list_entry(
    P_P_LIST_BLOCK list,
    S32 size,
    P_S32 resp);

extern S32
add_to_list(
    P_P_LIST_BLOCK list,
    S32 key,
    const uchar *str,
    P_S32 resp);

extern BOOL
delete_from_list(
    P_P_LIST_BLOCK list,
    S32 key);

extern void
delete_list(
    P_P_LIST_BLOCK);

extern S32
duplicate_list(
    P_P_LIST_BLOCK dst,
    P_P_LIST_BLOCK src);

extern LIST *
search_list(
    P_P_LIST_BLOCK first_one,
    S32 target);

extern LIST *
first_in_list(
    P_P_LIST_BLOCK);

extern LIST *
next_in_list(
    P_P_LIST_BLOCK);

extern P_ANY
list_first(
    P_P_LIST_BLOCK list,
    P_LIST_ITEMNO key);

extern P_ANY
list_next(
    P_P_LIST_BLOCK list,
    P_LIST_ITEMNO key);

/* end of lists_x.h */
