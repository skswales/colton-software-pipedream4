/* lists.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that handles lists */

/* MRJC March 1989 */

#include "common/gflags.h"

#include "datafmt.h"

/*
internal functions
*/

static LIST_ITEMNO
searchkey(
    P_P_LIST_BLOCK list,
    S32 key,
    P_LIST_ITEM *itp);

/******************************************************************************
*
* add definition to list
*
* --out--
*   -ve, NULL: error
*     0, NULL: no room
*   +ve, xxxx: added
*
******************************************************************************/

extern LIST *
add_list_entry(
    P_P_LIST_BLOCK list,
    S32 size,
    P_S32 resp)
{
    P_LIST_ITEM it;
    S32 res = 0;

    trace_3(TRACE_APP_PD4, "add_list_entry(" PTR_XTFMT ", %d, " PTR_XTFMT ")\n", report_ptr_cast(list), size, report_ptr_cast(resp));

    if(resp)
        *resp = res;

    if(!size)
        return(NULL);

    /* allocate new list if pointer is null */
    if(!*list)
        {
        if((*list = list_allocptr(sizeof(LIST_BLOCK))) == NULL)
            {
            if(resp)
                *resp = status_nomem();
            return(NULL);
            }

        list_init(*list, 400, 2000);

        list_register(*list);
        }

    it = list_createitem(*list,
                         list_numitem(*list),
                         sizeof(LIST) - 1 + size,
                         FALSE);

    if(it)
        {
        if(resp)
            *resp = 1;

        return((LIST *) it->i.inside);
        }

    if(resp)
        *resp = res;

    return(NULL);
}

/******************************************************************************
*
* add definition to list; the body must be a string
*
* --out--
*   -ve: error
*     0: no room
*   +ve: added (or string empty)
*
******************************************************************************/

extern S32
add_to_list(
    P_P_LIST_BLOCK list,
    S32 key,
    PC_U8 str,
    P_S32 resp)
{
    LIST *lpt;
    S32 res;

    trace_4(TRACE_APP_PD4, "add_to_list(" PTR_XTFMT ", %d, %s, " PTR_XTFMT ")\n", report_ptr_cast(list), key, trace_string(str), report_ptr_cast(resp));

    if(!str)
        str = NULLSTR;

    lpt = add_list_entry(list, strlen(str) + 1, resp);
    if(lpt)
        {
        lpt->key = key;
        strcpy((char *) lpt->value, str);
        res = 1;
        }
    else
        res = resp ? *resp : 0;

    return(res);
}

/******************************************************************************
*
* if there is an entry in the list for the given key, delete it.
* It looks through the list until it finds the entry, holding the pointer
* to the current list element. On finding the entry it makes the pointer
* point to the following element and deletes the entry.
*
******************************************************************************/

extern BOOL
delete_from_list(
    P_P_LIST_BLOCK list,
    S32 key)
{
    LIST_ITEMNO item;

    if(*list)
        {
        item = searchkey(list, key, NULL);

        if(item >= 0)
            {
            list_deleteitems(*list, item, (LIST_ITEMNO) 1);
            return(TRUE);
            }
        }

    return(FALSE);
}

/******************************************************************************
*
* delete a list altogether and free storage
*
******************************************************************************/

extern void
delete_list(
    P_P_LIST_BLOCK list)
{
    P_LIST_BLOCK lp;

    if((lp = *list) != NULL)
        {
        list_free(lp);
        list_deregister(lp);
        list_disposeptr((void **) list);
        }
}

/******************************************************************************
*
*  make a copy of a list
*  NB. dst must be initialised
*
******************************************************************************/

extern S32
duplicate_list(
    P_P_LIST_BLOCK dst,
    P_P_LIST_BLOCK src)
{
    LIST_ITEMNO item, nitems;
    P_LIST_ITEM it;
    LIST *s_lptr, *d_lptr;
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
                s_lptr = (LIST *) it->i.inside;

                d_lptr = add_list_entry(dst, strlen((char *) s_lptr->value) + 1, &res);
                if(!d_lptr)
                    {
                    delete_list(dst);
                    return(res);
                    }

                /* new item creation may have moved source */
                it = list_gotoitem(*src, item);
                s_lptr = (LIST *) it->i.inside;

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

extern LIST *
first_in_list(
    P_P_LIST_BLOCK list)
{
    return(list_first(list, NULL));
}

/******************************************************************************
*
* return the next element in the list
*
******************************************************************************/

extern LIST *
next_in_list(
    P_P_LIST_BLOCK list)
{
    return(list_next(list, NULL));
}

/******************************************************************************
*
* initialise list for sequence and
* return the first element in the list
*
******************************************************************************/

extern P_ANY
list_first(
    P_P_LIST_BLOCK list,
    P_LIST_ITEMNO key)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    if(!*list)
        return(NULL);

    if(key)
        item = *key;
    else
        item = 0;

    it = list_initseq(*list, &item);

    if(key)
        *key = item;

    return(it ? it->i.inside : NULL);
}

/******************************************************************************
*
* return the next element in the list
*
******************************************************************************/

extern P_ANY
list_next(
    P_P_LIST_BLOCK list,
    P_LIST_ITEMNO key)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    if(!*list)
        return(NULL);

    if(key)
        item = *key;
    else
        item = list_atitem(*list);

    it = list_nextseq(*list, &item);

    if(key)
        *key = item;

    return(it ? it->i.inside : NULL);
}

/******************************************************************************
*
* search the given list for the given target,
* returning a pointer to the element
*
******************************************************************************/

extern LIST *
search_list(
    P_P_LIST_BLOCK list,
    S32 key)
{
    LIST_ITEMNO item;
    P_LIST_ITEM  it;

    if(!*list)
        return(NULL);

    item = searchkey(list, key, &it);
    if(item < 0)
        return(NULL);

    return((LIST *) it->i.inside);
}

/******************************************************************************
*
* search list for key; return item number
* SKS made it return item pointer iff item number valid
*
******************************************************************************/

static LIST_ITEMNO
searchkey(
    P_P_LIST_BLOCK list,
    S32 key,
    P_LIST_ITEM *itp)
{
    LIST_ITEMNO i;
    P_LIST_ITEM it;

    trace_2(TRACE_APP_PD4, "searchkey(" PTR_XTFMT ", %d)\n", report_ptr_cast(list), key);

    for(i = 0; i < list_numitem(*list); i++)
        {
        it = list_gotoitem(*list, i);
        if(!it)
            continue;

        trace_3(TRACE_APP_PD4, "comparing item %d key %d with key %d\n",
                i, ((LIST *) it->i.inside)->key, key);

        if(((LIST *) it->i.inside)->key == key)
            {
            trace_1(TRACE_APP_PD4, "key matched at item %d\n", i);
            if(itp)
                *itp = it;
            return(i);
            }
        }

    trace_0(TRACE_APP_PD4, "key not found in list\n");

    return(STATUS_FAIL);
}

/* end of lists.c */
