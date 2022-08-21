/* collect.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that handles new lists (was nlists.c) with collect-style interface */

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

_Check_return_
_Ret_writes_maybenull_(size)
extern P_BYTE
_collect_add_entry(
    _InoutRef_  P_NLISTS_BLK nlbrp,
    S32 size,
    _InoutRef_opt_ P_LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status)
{
    P_P_LIST_BLOCK p_p_list_block = &nlbrp->lbr;
    LIST_ITEMNO item;
    P_LIST_ITEM it;

    *p_status = STATUS_OK;

    /* if bad or empty item, make 1 */
    if(size <= 0)
        size = 1;

    myassert2x(key && ((S32) *key >= -1), "collect_add_entry key(&%p, %ld) missing or negative and not -1", p_p_list_block, key ? *key : 0);

    /* allocate new LIST_BLOCK if needed */
    if(NULL == *p_p_list_block)
        if(status_fail(*p_status = collect_alloc_list_block(p_p_list_block, nlbrp->maxitemsize, nlbrp->maxpoolsize)))
            return(NULL);

    if(key && *key >= 0)
        item = *key;
    else
        item = list_numitem(*p_p_list_block);

    if(NULL == (it = list_createitem(*p_p_list_block, item, size, FALSE)))
    {
        *p_status = status_nomem();
        return(NULL);
    }

    if(key)
        *key = item;

    return(list_itemcontents(void, it));
}

_Check_return_
extern STATUS
collect_alloc_list_block(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block,
    _In_        S32 maxitemsize,
    _In_        S32 maxpoolsize)
{
    STATUS status;

    if((maxitemsize == maxpoolsize)  &&  (maxitemsize == 0))
    {   /* use initialisers a la PD3 lists.c */
        maxitemsize = 400;
        maxpoolsize = 2000;
    }

    if(NULL == (*p_p_list_block = al_ptr_alloc_elem(LIST_BLOCK, 1, &status)))
        return(status);

    /* ensure new list block gets sensible defaults */
    list_init(*p_p_list_block, maxitemsize, maxpoolsize);

    list_register(*p_p_list_block);

    return(STATUS_OK);
}

/******************************************************************************
*
* compress a list, coalescing fillers and removing if empty
*
******************************************************************************/

extern void
collect_compress(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block)
{
    if(NULL == *p_p_list_block)
        return;

    /* ensure any fillers get zapped */
    list_garbagecollect(*p_p_list_block);

    list_packlist(*p_p_list_block, -1);

    if(0 != list_numitem(*p_p_list_block))
        return;

    collect_delete(p_p_list_block);
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

_Check_return_
extern STATUS
collect_copy(
    _InoutRef_  P_NLISTS_BLK new_nlbrp,
    _InRef_     P_P_LIST_BLOCK old_p_p_list_block)
{
    LIST_ITEMNO key = 0;

    if(NULL != collect_first(BYTE, old_p_p_list_block, &key))
    {
        do  {
            S32 entry_size = list_entsize(*old_p_p_list_block, list_atitem(*old_p_p_list_block));
            STATUS status;
            PC_ANY old_entp;
            P_ANY new_entp;

            if(NULL == (new_entp = _collect_add_entry(new_nlbrp, entry_size, &key, &status)))
            {
                collect_delete(&new_nlbrp->lbr);
                return(status);
            }

            /* listed memory may move */
            old_entp = _list_gotoitemcontents(*old_p_p_list_block, list_atitem(*old_p_p_list_block));
            assert(NULL != old_entp);
            memcpy32(new_entp, old_entp, entry_size);
        }
        while(collect_next(BYTE, old_p_p_list_block, &key));
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* delete a list altogether and free storage
*
******************************************************************************/

extern void
collect_delete(
    _InoutRef_  P_P_LIST_BLOCK p_p_list_block)
{
    list_free(*p_p_list_block);

    list_deregister(*p_p_list_block);

    al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_list_block));
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
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO key)
{
    myassert2x(((S32) key >= 0), "collect_delete_entry key(&%p, %ld) negative", p_p_list_block, key);

    if(NULL == *p_p_list_block)
        return;

    if(NULL == list_gotoitem(*p_p_list_block, key))
        return;

    list_deleteitems(*p_p_list_block, key, (LIST_ITEMNO) 1);
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

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first(
    _InRef_     P_LIST_BLOCK p_list_block,
    _OutRef_    P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = 0;

    it = list_initseq(p_list_block, &item);

    if(p_key)
        *p_key = item;

    return(it ? list_itemcontents(void, it) : NULL);
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

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first_from(
    _InRef_     P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    assert(p_key);
    item = *p_key;
    myassert1x((S32) item >= 0, "collect_first_from key %ld negative", item);

    it = list_initseq(p_list_block, &item);

    *p_key = item;

    return(it ? list_itemcontents(void, it) : NULL);
}

/******************************************************************************
*
* insert an entry in the list at the given item number
* items with a keynumber greater than the given key
* are incremented by one
*
******************************************************************************/

_Check_return_
_Ret_writes_to_maybenull_(size, 0)
extern P_BYTE
_collect_insert_entry(
    _InoutRef_  P_NLISTS_BLK nlbrp,
    S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status)
{
    P_P_LIST_BLOCK p_p_list_block = &nlbrp->lbr;
    P_LIST_ITEM it;

    *p_status = STATUS_OK;

    /* if bad or empty item, make 1 */
    if(size <= 0)
        size = 1;

    myassert2x(((S32) key >= 0), "collect_insert_entry key(&%p, %ld) negative", p_p_list_block, key);

    /* allocate new LIST_BLOCK if needed */
    if(NULL == *p_p_list_block)
        if(status_fail(*p_status = collect_alloc_list_block(p_p_list_block, nlbrp->maxpoolsize, nlbrp->maxpoolsize)))
            return(NULL);

    /* insert an item at this position if not off end of list */
    if(key < list_numitem(*p_p_list_block))
        if(status_fail(*p_status = list_insertitems(*p_p_list_block, key, 1)))
            return(NULL);

    /* create an item at the given position */
    if(NULL == (it = list_createitem(*p_p_list_block, key, size, FALSE)))
    {
        *p_status = status_nomem();
        return(NULL);
    }

    return(list_itemcontents(void, it));
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

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_next(
    _InRef_     P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    assert(p_key);
    item = *p_key;
    myassert1x((S32) item >= 0, "collect_next key %ld negative", item);

    if(NULL == p_list_block)
        return(NULL); /* like PD list_next() */

    it = list_nextseq(p_list_block, &item);

    *p_key = item;

    return(it ? list_itemcontents(void, it) : NULL);
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

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_prev(
    _InRef_     P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    assert(p_key);
    item = *p_key;
    myassert1x((S32) item >= 0, "collect_prev key %ld negative", item);

    it = list_prevseq(p_list_block, &item);

    *p_key = item;

    return(it ? list_itemcontents(void, it) : NULL);
}

/******************************************************************************
*
* remove an item from the list; items with key numbers
* greater than the removed item remain unaffected
* (complementary to collect_add_entry)
*
******************************************************************************/

extern void
collect_subtract_entry(
    _InRef_     P_P_LIST_BLOCK p_p_list_block,
    _InVal_     LIST_ITEMNO key)
{
    myassert2x(((S32) key >= 0), "collect_subtract_entry key(&%p, %ld) negative", *p_p_list_block, key);

    if(NULL == *p_p_list_block)
        return;

    /* create filler entry */
    if(NULL == list_createitem(*p_p_list_block, key, 0, FALSE))
        assert("failed to create filler");
}

/* end of collect.c */
