/* collect.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that handles new lists (was nlists.c) */

/* SKS February 1991 derived from lists.c / 2012 convergence with Fireworkz terms */

#include "common/gflags.h"

#include "cmodules/collect.h"

/******************************************************************************
*
* add entry to list
* existing entry is overwritten if exists
* key number returned in * key
* use of key optional, NULL -> don't care
* use *key of -1 to add to end of list
*
* --out--
*
*      -1   entry not added - out of memory
*       0   entry not added - size was zero
*       1   entry added
*
******************************************************************************/

extern P_ANY
collect_add_entry(
    P_NLISTS_BLK lbrp,
    S32 size,
    /*inout*/ P_LIST_ITEMNO key)
{
    P_LIST_ITEM it;
    S32 res = 1;

    /* if empty item, make 1 */
    if(!size)
        size = 1;

    myassert2x(key && ((S32) *key >= -1), "collect_add_entry key(&%p, %ld) missing or negative and not -1", key, key ? *key : 0);

    /* allocate new list if null list */
    if(!lbrp->lbr)
        {
        if(!nlist_allocblkref(&lbrp->lbr))
            {
            it  = NULL;
            res = -1;
            }
        else
            {
            /* a little initialisation here temporarily till tuning sorted */
            S32 maxitemsize = lbrp->maxitemsize;
            S32 maxpoolsize = lbrp->maxpoolsize;
            if((maxitemsize == maxpoolsize)  &&  (maxitemsize == 0))
                {
                /* use initialisers a la PD3 lists.c */
                maxitemsize = 400;
                maxpoolsize = 2000;
                }

            /* call to retune pool */
            list_deregister((P_LIST_BLOCK) lbrp->lbr);
            list_init(      (P_LIST_BLOCK) lbrp->lbr, maxitemsize, maxpoolsize);
            list_register(  (P_LIST_BLOCK) lbrp->lbr);
            }
        }

    /* if failed, return NULL */
    if(lbrp->lbr)
        {
        LIST_ITEMNO item;

        if(key && *key >= 0)
            item = *key;
        else
            item = nlist_numitem(lbrp->lbr);

        if(nlist_createitem(lbrp->lbr, item, &it, size, FALSE))
            {
            if(key)
                *key = item;
            }
        else
            res = -1;
        }

    /* SKS after 4.12 24mar92 - whoops, this will have caused much evil */
    return(it ? list_itemcontents(it) : it);
}

/******************************************************************************
*
* compress a list, coalescing fillers and removing if empty
*
******************************************************************************/

extern void
collect_compress(
    P_NLISTS_BLK lbrp)
{
    if(lbrp->lbr)
        {
        /* ensure any fillers get zapped */
        nlist_garbagecollect(lbrp->lbr);

        nlist_packlist(lbrp->lbr, -1);

        if(!nlist_numitem(lbrp->lbr))
            collect_delete(lbrp);
        }
}

/******************************************************************************
*
* copy one list into another
*
* --out--
*
*      -1   out of memory - intermediate result cleaned up
*       1   list copied
*
******************************************************************************/

extern S32
collect_copy(
    P_NLISTS_BLK new_lbrp,
    P_NLISTS_BLK old_lbrp)
{
    LIST_ITEMNO  key;
    P_ANY        old_entp;
    list_itempos old_itpos;
    P_ANY        new_entp;
    S32          entry_size;

    key = 0;

    if(collect_first(old_lbrp, &key))
        do  {
            entry_size = nlist_entsize(old_lbrp->lbr, nlist_atitem(old_lbrp->lbr));

            if((new_entp = collect_add_entry(new_lbrp, entry_size, &key)) == NULL)
                {
                collect_delete(new_lbrp);
                return(-1);
                }

            /* reload after alloc since memory may move (a ce moment) */
            nlist_gotoitemcontents(old_lbrp->lbr, nlist_atitem(old_lbrp->lbr), &old_entp, &old_itpos);

            void_memcpy32(new_entp, old_entp, entry_size);
            }
        while(collect_next(old_lbrp, &key));

    return(1);
}

/******************************************************************************
*
* delete a list altogether and free storage
*
******************************************************************************/

extern void
collect_delete(
    /*inout*/ P_NLISTS_BLK lbrp)
{
    nlist_free(lbrp->lbr);

    nlist_disposeblkref(&lbrp->lbr);
}

/******************************************************************************
*
* delete an item from the list; key numbers greater
* than the given item are decremented by one
* (complementary to collect_insert_entry)
*
******************************************************************************/

extern void
collect_delete_entry(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO key)
{
    P_LIST_ITEM   it;
    list_itempos itpos;

    myassert2x(key && ((S32) *key >= 0), "collect_delete_entry key(&%p, %ld) missing or negative", key, key ? *key : 0);

    if(lbrp->lbr)
        if(nlist_gotoitem(lbrp->lbr, *key, &it, &itpos))
            nlist_deleteitems(lbrp->lbr, *key, (LIST_ITEMNO) 1);
}

/******************************************************************************
*
* initialise list for sequence and
* return the very first element in the list
*
* --out--
*
*   0   nothing on list
*
******************************************************************************/

extern P_ANY
collect_first(
    P_NLISTS_BLK lbrp,
    /*out*/ P_LIST_ITEMNO key)
{
    P_LIST_ITEM    it;
    LIST_ITEMNO   item;
    list_itempos  itpos;
    S32           res;

    item = 0;

    res = nlist_initseq(lbrp->lbr, &item, &it, &itpos);

    if(key)
        *key = item;

    return(it ? list_itemcontents(it) : NULL);
}

/******************************************************************************
*
* initialise list for sequence and
* return the first element in the list
* starting at the given key (may be previous
* element if key is in a hole or off end)
*
* --out--
*
*   0   nothing on list
*
******************************************************************************/

extern P_ANY
collect_first_from(
    P_NLISTS_BLK lbrp,
    /*inout*/ P_LIST_ITEMNO key)
{
    P_LIST_ITEM    it;
    LIST_ITEMNO   item;
    list_itempos  itpos;
    S32           res;

    if(key)
        {
        item = *key;

        myassert1x((S32) item >= 0, "collect_first key %ld negative", item);
        }
    else
        item = 0;

    res = nlist_initseq(lbrp->lbr, &item, &it, &itpos);

    if(key)
        *key = item;

    return(it ? list_itemcontents(it) : NULL);
}

/******************************************************************************
*
* insert an entry in the list at the given item number
* items with a keynumber greater than the given key
* are incremented by one
*
******************************************************************************/

extern P_ANY
collect_insert_entry(
    P_NLISTS_BLK lbrp,
    S32 size,
    PC_LIST_ITEMNO key)
{
    P_LIST_ITEM it;
    S32 res = 1;

    if(!size)
        size = 1;

    myassert2x(key && ((S32) *key >= 0), "collect_insert_entry key(&%p, %ld) missing or negative", key, key ? *key : 0);

    /* allocate new list if null list */
    if(!lbrp->lbr)
        {
        if(!nlist_allocblkref(&lbrp->lbr /* pass tuning parms thru sometime */))
            {
            it  = NULL;
            res = -1;
            }
        else
            {
            /* a little initialisation here temporarily till tuning sorted */
            S32 maxitemsize = lbrp->maxpoolsize;
            S32 maxpoolsize = lbrp->maxpoolsize;
            if((maxitemsize == maxpoolsize)  &&  (maxitemsize == 0))
                {
                /* use initialisers a la PD3 lists.c */
                maxitemsize = 400;
                maxpoolsize = 2000;
                }

            /* call to retune pool */
            list_deregister((P_LIST_BLOCK) lbrp->lbr);
            list_init(      (P_LIST_BLOCK) lbrp->lbr, maxitemsize, maxpoolsize);
            list_register(  (P_LIST_BLOCK) lbrp->lbr);
            }
        }

    /* if failed, return NULL */
    if(lbrp->lbr)
        {
        /* insert an item at this position if not off end of list */
        if(*key < nlist_numitem(lbrp->lbr))
            if((res = nlist_insertitems(lbrp->lbr, *key, 1)) < 0)
                return(NULL);

        /* create an item at the given position */
        res = nlist_createitem(lbrp->lbr, *key, &it, size, FALSE)
                     ? 1
                     : -1;
        }

    return(it ? list_itemcontents(it) : NULL);
}

/******************************************************************************
*
* return the next element in the list
*
* --out--
*
*   0   nothing more on list
*
******************************************************************************/

extern P_ANY
collect_next(
    P_NLISTS_BLK lbrp,
    /*inout*/ P_LIST_ITEMNO key)
{
    P_LIST_ITEM    it;
    LIST_ITEMNO   item;
    list_itempos  itpos;
    S32           res;

    if(key)
        {
        item = *key;

        myassert1x((S32) item >= 0, "collect_next key %ld negative", item);
        }
    else
        item = nlist_atitem(lbrp->lbr);

    res = nlist_nextseq(lbrp->lbr, &item, &it, &itpos);

    if(key)
        *key = item;

    return(it ? list_itemcontents(it) : NULL);
}

/******************************************************************************
*
* return the previous element in the list
*
* --out--
*
*   0   nothing more on list
*
******************************************************************************/

extern P_ANY
collect_prev(
    P_NLISTS_BLK lbrp,
    /*inout*/ P_LIST_ITEMNO key)
{
    P_LIST_ITEM    it;
    LIST_ITEMNO   item;
    list_itempos  itpos;
    S32           res;

    if(key)
        {
        item = *key;

        myassert1x((S32) item >= 0, "collect_prev key %ld negative", item);
        }
    else
        item = nlist_atitem(lbrp->lbr);

    res = nlist_prevseq(lbrp->lbr, &item, &it, &itpos);

    if(key)
        *key = item;

    return(it ? list_itemcontents(it) : NULL);
}

/******************************************************************************
*
* search the given list for the given target,
* returning a pointer to the element
*
* --out--
*
*   0   entry not found
*
******************************************************************************/

extern P_ANY
collect_search(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO key)
{
    P_ANY        mp;
    list_itempos itpos;
    S32          res;

    myassert2x(key && ((S32) *key >= 0), "collect_search key(&%p, %ld) missing or negative", key, key ? *key : 0);

    res = nlist_gotoitemcontents(lbrp->lbr, *key, &mp, &itpos);

    return(mp);
}

/******************************************************************************
*
* remove an item from the list; items with key numbers
* greater than the removed item remain unaffected
* (complementary to collect_add_entry)
*
******************************************************************************/

extern S32
collect_subtract_entry(
    P_NLISTS_BLK lbrp,
    PC_LIST_ITEMNO key)
{
    P_LIST_ITEM it;

    myassert2x(key && ((S32) *key >= 0), "collect_subtract_entry key(&%p, %ld) missing or negative", key, key ? *key : 0);

    /* create filler entry */
    if(lbrp->lbr)
        if(!nlist_createitem(lbrp->lbr, *key, &it, 0, FALSE))
            return(-1);

    return(0);
}

/* end of collect.c */
