/* lists_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External definitions for lists in PipeDream */

/* MRJC March 1989 */

/*
data definition
*/

typedef struct LIST
{
    S32 key;
    uchar value[1];
}
LIST, * P_LIST; typedef const LIST * PC_LIST;

/*
exported functions
*/

_Check_return_
_Ret_maybenull_
extern P_LIST
add_list_entry(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 size,
    _OutRef_    P_STATUS resp);

_Check_return_
extern STATUS
add_to_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key,
    _In_opt_z_  PC_U8Z str);

/*ncr*/
extern BOOL
delete_from_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key);

extern void
delete_list(
    _InoutRef_  P_P_LIST_BLOCK);

_Check_return_
extern STATUS
duplicate_list(
    _InoutRef_  P_P_LIST_BLOCK dst,
    _InoutRef_  P_P_LIST_BLOCK src);

_Check_return_
_Ret_maybenull_
extern P_LIST
search_list(
    _InoutRef_  P_P_LIST_BLOCK first_one,
    _InVal_     S32 target);

_Check_return_
_Ret_maybenull_
extern P_ANY
list_first(
    _InoutRef_  P_P_LIST_BLOCK list);

_Check_return_
_Ret_maybenull_
extern P_ANY
list_next(
    _InoutRef_  P_P_LIST_BLOCK list);

#define first_in_list(list) list_first(list)
#define next_in_list(list)  list_next(list)

/* end of lists_x.h */
