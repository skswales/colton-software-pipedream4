/* lists.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that handles lists */

/* MRJC March 1989 */

#include "common/gflags.h"

#include "datafmt.h"

/*
internal functions
*/

_Check_return_
static LIST_ITEMNO
searchkey(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key,
    _OutRef_    P_P_LIST_ITEM itp);

/******************************************************************************
*
* add definition to list
*
* --out--
*   -ve, NULL: error
*     0, NULL: zero size (up to caller if this is an error)
*   +ve, xxxx: added
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_LIST
add_list_entry(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 size,
    _OutRef_    P_STATUS resp)
{
    P_LIST_ITEM it;

    trace_3(TRACE_APP_PD4, "add_list_entry(" PTR_XTFMT ", %d, " PTR_XTFMT ")", report_ptr_cast(list), size, report_ptr_cast(resp));

    *resp = STATUS_OK;

    if(0 == size)
        return(NULL);

    /* allocate new list if pointer is null */
    if(NULL == *list)
    {
        STATUS status;

        if(NULL == (*list = al_ptr_alloc_elem(LIST_BLOCK, 1, &status)))
        {
            *resp = status;
            return(NULL);
        }

        list_init(*list, 400, 2000);

        list_register(*list);
    }

    it = list_createitem(*list,
                         list_numitem(*list),
                         (sizeof(LIST) - 1) + size,
                         FALSE);

    if(it)
    {
        *resp = STATUS_DONE;

        return((P_LIST) it->i.inside);
    }

    *resp = status_nomem();

    return(NULL);
}

/******************************************************************************
*
* add definition to list; the body must be a string
*
* --out--
*   -ve: error
*   +ve: added (or string empty)
*
******************************************************************************/

_Check_return_
extern STATUS
add_to_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key,
    _In_opt_z_  PC_U8Z str)
{
    U32 n_bytes;
    P_LIST lpt;
    STATUS status;

    trace_3(TRACE_APP_PD4, "add_to_list(" PTR_XTFMT ", %d, %s)", report_ptr_cast(list), key, trace_string(str));

    if(!str)
        str = NULLSTR;

    n_bytes = strlen32p1(str);

    if(NULL != (lpt = add_list_entry(list, n_bytes, &status)))
    {
        lpt->key = key;
        memcpy32(lpt->value, str, n_bytes);
    }

    return(status);
}

/******************************************************************************
*
* if there is an entry in the list for the given key, delete it.
* It looks through the list until it finds the entry, holding the pointer
* to the current list element. On finding the entry it makes the pointer
* point to the following element and deletes the entry.
*
******************************************************************************/

/*ncr*/
extern BOOL
delete_from_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key)
{
    LIST_ITEMNO item;
    P_LIST_ITEM it;

    if((item = searchkey(list, key, &it)) < 0)
        return(FALSE);

    list_deleteitems(*list, item, (LIST_ITEMNO) 1);

    return(TRUE);
}

/******************************************************************************
*
* delete a list altogether and free storage
*
******************************************************************************/

extern void
delete_list(
    _InoutRef_  P_P_LIST_BLOCK list)
{
    P_LIST_BLOCK lp = *list;

    if(NULL == lp)
        return;

    list_free(lp);

    list_deregister(lp);

    al_ptr_dispose(P_P_ANY_PEDANTIC(list));
}

/******************************************************************************
*
*  make a copy of a list
*  NB. dst must be initialised
*
******************************************************************************/

_Check_return_
extern STATUS
duplicate_list(
    _InoutRef_  P_P_LIST_BLOCK dst,
    _InoutRef_  P_P_LIST_BLOCK src)
{
    LIST_ITEMNO item, nitems;
    P_LIST_ITEM it;
    P_LIST s_lptr, d_lptr;
    S32 res;

    delete_list(dst);

    if(*src)
    {
        item = 0;
        nitems = list_numitem(*src);

        while(item < nitems)
        {
            it = list_gotoitem(*src, item);

            if(it)
            {
                s_lptr = (P_LIST) it->i.inside;

                if(NULL == (d_lptr = add_list_entry(dst, strlen((char *) s_lptr->value) + 1, &res)))
                {
                    delete_list(dst);
                    return(res);
                }

                /* new item creation may have moved source */
                it = list_gotoitem(*src, item);
                s_lptr = (P_LIST) it->i.inside;

                d_lptr->key = s_lptr->key;
                strcpy((char *) d_lptr->value, (char *) s_lptr->value);
            }

            ++item;
        }
    }

    return(1);
}

/******************************************************************************
*
* initialise list for sequence and
* return the first element in the list
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_ANY
list_first(
    _InoutRef_  P_P_LIST_BLOCK list)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = 0;

    if(NULL == *list)
        return(NULL);

    it = list_initseq(*list, &item);

    return(it ? it->i.inside : NULL);
}

/******************************************************************************
*
* return the next element in the list
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_ANY
list_next(
    _InoutRef_  P_P_LIST_BLOCK list)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = list_atitem(*list);

    if(NULL == *list)
        return(NULL);

    it = list_nextseq(*list, &item);

    return(it ? it->i.inside : NULL);
}

/******************************************************************************
*
* search the given list for the given target,
* returning a pointer to the element
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_LIST
search_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key)
{
    LIST_ITEMNO item;
    P_LIST_ITEM it;

    if((item = searchkey(list, key, &it)) < 0)
        return(NULL);

    return((P_LIST) it->i.inside);
}

/******************************************************************************
*
* search list for key; return item number
* SKS made it return item pointer iff item number valid
*
******************************************************************************/

_Check_return_
static LIST_ITEMNO
searchkey(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InVal_     S32 key,
    _OutRef_    P_P_LIST_ITEM itp)
{
    LIST_ITEMNO i;
    P_LIST_ITEM it;

    trace_2(TRACE_APP_PD4, "searchkey(" PTR_XTFMT ", %d)", report_ptr_cast(list), key);

    *itp = NULL;

    for(i = 0; i < list_numitem(*list); i++)
    {
        it = list_gotoitem(*list, i);
        if(!it)
            continue;

        trace_3(TRACE_APP_PD4, "comparing item %d key %d with key %d",
                i, ((P_LIST) it->i.inside)->key, key);

        if(((P_LIST) it->i.inside)->key == key)
        {
            trace_1(TRACE_APP_PD4, "key matched at item %d", i);
            *itp = it;
            return(i);
        }
    }

    trace_0(TRACE_APP_PD4, "key not found in list");

    return(STATUS_FAIL);
}

/* end of lists.c */
