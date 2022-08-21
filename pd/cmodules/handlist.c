/* handlist.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* A generalised list manager based on a handle type heap system
 * with movable blocks of memory
 */

/* MRJC January 1989 */

#include "common/gflags.h"

#include <limits.h>

/* local header file */
#include "handlist.h"

#define FILLSIZE (sizeof(union _list_guts))

typedef struct handle_block_entry
{
    P_ANY pointer;
}
* hand_ent_p;

#if defined SPARSE_ENDJUMPER_DEBUG
#define ENDJUMPERSIZE 8
#else
#define ENDJUMPERSIZE 0
#endif

/*
internal functions
*/

static P_LIST_ITEM
addafter(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust);

static P_LIST_ITEM
addbefore(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust);

static P_LIST_ITEM
allocitem(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust);

static P_U8
allocpool(
    P_LIST_BLOCK lp,
    S32 poolix,
    OFF_TYPE size);

static P_LIST_ITEM
convofftoptr(
    P_LIST_BLOCK lp,
    S32 poolix,
    OFF_TYPE off);

static void
deallocitem(
    P_LIST_BLOCK lp,
    P_LIST_ITEM it);

static void
deallocpool(
    P_LIST_BLOCK lp,
    S32 poolix);

static P_LIST_ITEM
fillafter(
    P_LIST_BLOCK lp,
    LIST_ITEMNO itemfill,
    LIST_ITEMNO adjust);

static P_LIST_ITEM
fillbefore(
    P_LIST_BLOCK lp,
    LIST_ITEMNO itemfill,
    LIST_ITEMNO adjust);

static pooldp
getpooldp(
    P_LIST_BLOCK lp,
    S32 poolix);

static P_U8
getpoolptr(
    pooldp pdp);

static P_LIST_ITEM
reallocitem(
    P_LIST_BLOCK lp,
    P_LIST_ITEM it,
    S32 newsize);

static P_U8
reallocpool(
    P_LIST_BLOCK lp,
    OFF_TYPE size,
    S32 ix);

static P_U8
splitpool(
    P_LIST_BLOCK lp,
    LIST_ITEMNO adjust);

static void
updatepoolitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO change);

#if defined(SPARSE_VALIDATE_DEBUG) && TRACE_ALLOWED

static void
_validatepool(
    P_LIST_BLOCK lp,
    S32 recache);

static void
validatepools(void);
#else
#define                _validatepool(lp, recache)
#define                validatepools()
#endif
#define                validatepool(lp) _validatepool(lp, 0)

/*
static data
*/

static P_LIST_BLOCK firstblock = NULL; /* first LIST_BLOCK in chain */

static const ARRAY_INIT_BLOCK handlist_array_init_block =
aib_init(4 /*size increment*/, sizeof(BYTE) /*element size*/, FALSE /*clear to zero*/);

/******************************************************************************
*
* allocate some memory and return a handle
*
******************************************************************************/

extern ARRAY_HANDLE
list_allochandle(
    S32 bytes)
{
    ARRAY_HANDLE newhandle;

    trace_1(TRACE_MODULE_LIST, "list_allochandle(%d)\n", bytes);

    validatepools();

    if(!bytes)
        {
        trace_2(TRACE_MODULE_LIST, "allocated handle: %d, size: %d\n", 0, 0);
        return(0);
        }

    newhandle = 0;

    do  {
        ARRAY_HANDLE array_handle;
        STATUS status;

        if(NULL != al_array_alloc(&array_handle, P_U8, bytes, &handlist_array_init_block, &status))
            {
            newhandle = (ARRAY_HANDLE) array_handle;
            trace_2(TRACE_MODULE_LIST, "allocated handle: %d, size: %d\n", newhandle, bytes);
            return(newhandle);
            }

        trace_0(TRACE_MODULE_LIST, "*************** AL_ARRAY_ALLOC FAILED **************\n");
        }
    while(list_unlockpools() || list_freepoolspace(-1));

    trace_2(TRACE_MODULE_LIST, "allocated handle: %d, size: %d\n", 0, 0);
    return(0);
}

/******************************************************************************
*
* allocate a block of memory, returning a pointer; the memory is locked in place
*
******************************************************************************/

extern P_ANY
list_allocptr_p1(
    S32 bytes)
{
    return(list_allocptr(bytes + 1 /*for NULLCH*/));
}

extern P_ANY
list_allocptr(
    S32 bytes)
{
    STATUS status;

    return(_al_ptr_alloc(bytes, &status));
}

/******************************************************************************
*
* Create a item for a particular list and item of a given size
*
* the size must include any extra space required apart from the item
* overhead itself: a size of 0 means a blank item; a size of 1 leaves space
* for a delimiter.
*
******************************************************************************/

#define FILLSET 1

extern P_LIST_ITEM
list_createitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    S32 size,
    S32 fill)
{
    LIST_ITEMNO itemcur, newfill;
    P_LIST_ITEM newitem, it;
    S32 poolsave;
    OFF_TYPE offsetsave;

    validatepools();

    if((OFF_TYPE) size >= lp->maxitemsize)
        return(NULL);

    it = NULL;

    newitem = list_gotoitem(lp, item);

    if(!newitem)
        {
        #ifdef SPARSE_DEBUG
        trace_4(TRACE_MODULE_LIST, "list_createitem *: lp: %x, lp->itemc: %d, item: %d, atitem: %d\n",
                (S32) lp, lp->itemc, (S32) item, (S32) list_atitem(lp));
        #endif

        it = list_igotoitem(lp, (itemcur = list_atitem(lp)));

        if(it && (it->flags & LIST_FILL))
            {
            /* save parameters for address reconstruction */
            poolsave = lp->pooldix;
            offsetsave = lp->itemc;

            if((itemcur + it->i.itemfill) > item)
                {
                /* check for creating hole */
                if(!size && !fill)
                    return(list_igotoitem(lp, itemcur));

                /* need to split filler block; newfill becomes
                the size of the lower filler block */
                if((newfill = (itemcur + it->i.itemfill -
                                item - FILLSET)) != 0)
                    {
                    /* adjust higher filler for new filler */
                    it->i.itemfill -= newfill;

                    if(!fillafter(lp, newfill, newfill))
                        {
                        /* replace earlier state */
                        convofftoptr(lp,
                                     poolsave,
                                     offsetsave)->i.itemfill += newfill;
                        return(NULL);
                        }

                    /* re-load higher fill pointer */
                    it = list_igotoitem(lp, itemcur);
                    }

                /* if filler is only 1 big, cause
                new item to overwrite it */
                if(it->i.itemfill == 1)
                    {
                    newitem = it;
                    it = NULL;
                    }
                /* otherwise make filler 1 smaller
                anticipating item being created */
                else
                    it->i.itemfill -= FILLSET;
                }
            else
                /* found a trailing filler - need to enlarge */
                {
                it->i.itemfill += item - (itemcur + it->i.itemfill);
                it = NULL;

                /* update numitem record */
                if(item >= lp->numitem)
                    {
                    lp->numitem = item + 1;
                    #ifdef SPARSE_DEBUG
                    trace_2(TRACE_MODULE_LIST, "list_createitem 1 lp: %x, numitem: %d\n",
                            (S32) lp, lp->numitem);
                    #endif
                    }
                }
            }
        /* adding past end of list - filler item needed */
        else if((newfill = (it ? item - (itemcur + list_leapnext(it))
                               : item - itemcur)) > 0)
            {
            if(!fillafter(lp, newfill, (LIST_ITEMNO) 0))
                return(NULL);
            it = NULL;

            /* update numitem record */
            if(item >= lp->numitem)
                {
                #if 1
                /* in case where addafter of actual item fails (below), and we
                 * added a trailing filler to an existing list, numitem was 1
                 * too big
                 * MRJC 23.9.91
                 */
                lp->numitem = itemcur + newfill + 1;
                #else
                lp->numitem = item + 1;
                #endif

                #ifdef SPARSE_DEBUG
                trace_2(TRACE_MODULE_LIST, "list_createitem 2 lp: %x, numitem: %d\n",
                        (S32) lp, lp->numitem);
                #endif
                }
            }
        else
            it = NULL;
        }

    if(fill || !size)
        {
        fill = TRUE;
        size = FILLSIZE;
        }

    if(!newitem)
        {
        /* allocate actual item */
        if((newitem = addafter(lp, size, (LIST_ITEMNO) FILLSET)) == NULL)
            {
            /* correct higher filler since we couldn't insert */
            if(it)
                convofftoptr(lp, poolsave, offsetsave)->i.itemfill += FILLSET;
            return(NULL);
            }

        /* update numitem record */
        if(item >= lp->numitem)
            {
            lp->numitem = item + 1;
            #ifdef SPARSE_DEBUG
            trace_2(TRACE_MODULE_LIST, "list_createitem 3 lp: %x, numitem: %d\n",
                    (S32) lp, lp->numitem);
            #endif
            }
        }
    else
        {
        /* item exists - ensure the same size */
        if((newitem = reallocitem(lp, newitem, size)) == NULL)
            return(NULL);
        }

    /* set filler flag if it is a filler or a hole */
    if(fill)
        {
        newitem->flags |= LIST_FILL;
        newitem->i.itemfill = FILLSET;
        }
    else
        newitem->flags &= ~LIST_FILL;

    return(newitem);
}

/******************************************************************************
*
* deallocate a block of memory, given a handle
*
******************************************************************************/

extern void
list_deallochandle(
    ARRAY_HANDLE mh)
{
    ARRAY_HANDLE array_handle = (ARRAY_HANDLE) mh;

    al_array_dispose(&array_handle);
}

/******************************************************************************
*
* deallocate a block of memory, given a pointer to a handle
*
******************************************************************************/

extern void
list_disposehandle(
    P_ARRAY_HANDLE mhp)
{
    ARRAY_HANDLE mh = *mhp;

    if(mh)
        {
        *mhp = 0;
        list_deallochandle(mh);
        }
}

/******************************************************************************
*
* deallocate a locked block of memory, given a pointer to the memory
*
******************************************************************************/

extern void
list_deallocptr(
    P_ANY mempnt)
{
    al_ptr_free(mempnt);
}

/******************************************************************************
*
* delete an item from the list
*
******************************************************************************/

extern void
list_deleteitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    LIST_ITEMNO numdel)
{
    P_LIST_ITEM it;
    LIST_ITEMNO removed;

    validatepools();

    do  {
        if((it = list_igotoitem(lp, item)) == NULL)
            return;

        if((it->flags & LIST_FILL) && (list_leapnext(it) > numdel))
            {
            it->i.itemfill -= numdel;
            updatepoolitems(lp, -numdel);
            return;
            }

        /* give up if we're about to delete the wrong item */
        if(list_atitem(lp) != item)
            return;

        removed = list_leapnext(it);
        numdel -= removed;
        deallocitem(lp, it);
        updatepoolitems(lp, -removed);
        }
    while(numdel);
}

/******************************************************************************
*
* take list block off chain
*
******************************************************************************/

extern void
list_deregister(
    P_LIST_BLOCK lp)
{
    P_LIST_BLOCK lpt, oldlpt;

    validatepools();

    for(lpt = firstblock, oldlpt = NULL;
        lpt;
        oldlpt = lpt, lpt = lpt->nextblock)
        if(lpt == lp)
            break;

    if(!lpt)
        return;

    if(oldlpt)
        oldlpt->nextblock = lpt->nextblock;
    else
        firstblock = lpt->nextblock;

    lpt->nextblock = NULL;

    #ifdef SPARSE_REGISTER_DEBUG
    {
    P_LIST_BLOCK lpt;

    trace_1(TRACE_MODULE_LIST, "list_deregister: %x\n", (S32) lp);

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        trace_1(TRACE_MODULE_LIST, "lp: %x registered\n", (S32) lpt);

    }
    #endif
}

/******************************************************************************
*
* ensure we have a item break at the specified position:
* insert or split filler if necessary
*
******************************************************************************/

extern S32
list_ensureitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item)
{
    if(list_gotoitem(lp, item))
        return(0);

    if(!list_createitem(lp, item, FILLSIZE, TRUE))
        return(-1);

    return(0);
}

/******************************************************************************
*
* return the size in bytes of an entry
* the number returned by be bigger than
* the size originally allocated since it
* may have been rounded up for alignment
*
******************************************************************************/

extern S32
list_entsize(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item)
{
    P_LIST_ITEM it;

    if((it = list_gotoitem(lp, item)) == NULL)
        return(-1);

    if(it->offsetn)
        return(it->offsetn - LIST_ITEMOVH);

    return((S32) (lp->poold->poolfree -
                   (((P_U8) it) - lp->poold->pool) - LIST_ITEMOVH));
}

/******************************************************************************
*
* free a whole list
*
******************************************************************************/

extern void
list_free(
    P_LIST_BLOCK lp)
{
    S32 i;

    validatepools();

    if(lp)
        for(i = lp->pooldblkfree - 1; i >= 0; --i)
            deallocpool(lp, i);
}

/******************************************************************************
*
* release any free space distributed in the pools
*
******************************************************************************/

extern S32
list_freepoolspace(
    S32 needed)
{
    P_LIST_BLOCK lp;
    S32 space_found;

    trace_1(TRACE_MODULE_LIST, "list_freepoolspace(%d)\n", needed);

    validatepools();

    if(needed < 0)
        needed = INT_MAX;

    for(lp = firstblock, space_found = 0;
        lp && (space_found < needed);
        lp = lp->nextblock)
            space_found += list_packlist(lp, needed);

    trace_2(TRACE_MODULE_LIST, "list_freepoolspace needed: %d, found: %d\n",
            needed, space_found);

    return(space_found);
}

/******************************************************************************
*
* garbage collect for a list, removing
* adjacent filler blocks
*
******************************************************************************/

extern S32
list_garbagecollect(
    P_LIST_BLOCK lp)
{
    LIST_ITEMNO item, nextitem;
    P_LIST_ITEM it;
    LIST_ITEMNO fill;
    S32 res;

    validatepools();

    for(item = 0, res = 0; item < list_numitem(lp); ++item)
        {
        it = list_igotoitem(lp, item);

        if(it && (it->flags & LIST_FILL) && (item == list_atitem(lp)))
            {
            nextitem = item + list_leapnext(it);
            it = list_igotoitem(lp, nextitem);

            if(it &&
               (it->flags & LIST_FILL) &&
               ((nextitem == list_atitem(lp)) ||
                (nextitem == list_numitem(lp)))
              )
                {
                fill = it->i.itemfill;
                deallocitem(lp, it);
                updatepoolitems(lp, -fill);
                if( ((it = list_igotoitem(lp, item)) != NULL) &&
                    (it->flags & LIST_FILL))
                    {
                    it->i.itemfill += fill;
                    updatepoolitems(lp, fill);
                    }

                res += 1;

                #ifdef SPARSE_DEBUG
                trace_0(TRACE_MODULE_LIST, "Filler recovered\n");
                #endif
                }
            }
        }

    validatepools();

    trace_1(TRACE_MODULE_LIST, "%d fillers recovered\n", res);
    return(res);
}

/******************************************************************************
*
* given handle, return pointer to object
*
******************************************************************************/

extern P_ANY
list_getptr(
    ARRAY_HANDLE hand)
{
    ARRAY_HANDLE array_handle = (ARRAY_HANDLE) hand;

    return(array_base(&array_handle, void));
}

/******************************************************************************
*
* travel to a particular item
*
* due to the structure used, where a item may conceptually exist,
* but an explicit entry in the list chain does not exist, travel()
* will return a NULL pointer for a item that exists but has no entry
* of its own in the structure
*
* the function list_atitem() must then be called to return the
* actual item that was achieved
*
******************************************************************************/

extern P_LIST_ITEM
list_gotoitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item)
{
    LIST_ITEMNO i, t;
    P_LIST_ITEM it;
    P_U8 pp;
    pooldp pdp;

    validatepools();

    if(!lp)
        return(NULL);

    #if TRACE_ALLOWED
    /* check list block registration */
    {
    P_LIST_BLOCK lpt;

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        if(lpt == lp)
            break;

    if(!lpt)
        {
        trace_1(TRACE_MODULE_LIST, "lp: %x not registered\n", (S32) lp);

        for(lpt = firstblock; lpt; lpt = lpt->nextblock)
            trace_1(TRACE_MODULE_LIST, "lp: %x found\n", (S32) lpt);
        }
    }
    #endif

    /* load pool descriptor pointer */
    if((pdp = lp->poold) == NULL)
        if((pdp = getpooldp(lp, lp->pooldix)) == NULL)
            return(NULL);

    /* get to correct item */
    if((pp = pdp->pool) == NULL)
        if((pp = getpoolptr(pdp)) == NULL)
            return(NULL);

    it = (P_LIST_ITEM) (pp + lp->itemc);

    if((i = lp->item) == item)
        {
        #ifdef SPARSE_DEBUG
        if(!lp->itemc && (item != pdp->poolitem))
            trace_1(TRACE_MODULE_LIST, "*** mismatch: item: %d\n", (S32) item);
        #endif

        return(!(it->flags & LIST_FILL) ? it : NULL);
        }

    /* skip backwards to pool if necessary */
    if(item < pdp->poolitem)
        {
        do
            {
            --lp->pooldix;
            --lp->poold;
            --pdp;
            }
        while(item < pdp->poolitem);

        i = pdp->poolitem;
        pp = getpoolptr(pdp);
        it = (P_LIST_ITEM) pp;
        }

    if(item > i)
        {
        do
            {
            /* go down list */
            while(it->offsetn)
                {
                if((i + (t = list_leapnext(it))) > item)
                    goto there;
                it = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);
                if((i += t) == item)
                    goto there;
                }

            if((lp->pooldix + 1 < lp->pooldblkfree) &&
               (item >= (pdp + 1)->poolitem))
                {
                do
                    {
                    ++lp->pooldix;
                    ++lp->poold;
                    ++pdp;
                    }
                while((lp->pooldix + 1 < lp->pooldblkfree) &&
                      (item >= (pdp + 1)->poolitem));

                i = pdp->poolitem;
                pp = getpoolptr(pdp);
                it = (P_LIST_ITEM) pp;
                }
            else
                break;
            }
        while(TRUE);
        }
    else if(item < i)
        /* go up the list */
        {
        do
            it = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        while((i -= list_leapnext(it)) > item);
        }

there:

    lp->item = i;
    lp->itemc = ((P_U8) it) - pp;
    return((i == item) && !(it->flags & LIST_FILL) ? it : NULL);
}

/******************************************************************************
*
* calculate how much free space is distributed in the pools
*
******************************************************************************/

extern S32
list_howmuchpoolspace(void)
{
    P_LIST_BLOCK lp;
    pooldp pdp;
    S32 res;
    S32 i;

    validatepools();

    for(lp = firstblock, res = 0; lp; lp = lp->nextblock)
        for(i = 0, pdp = getpooldp(lp, 0);
            pdp && i < lp->pooldblkfree;
            ++i, ++pdp)
            if(pdp->poolh)
                res += pdp->poolsize - pdp->poolfree;

    trace_1(TRACE_MODULE_LIST, "list_howmuchpoolspace: %d bytes\n", res);

    return(res);
}

/******************************************************************************
*
* internal gotoitem
*
* igotoitem only returns a null pointer if there is no item at all in
* the structure; it may not get to the item specified, in which case
* it returns the nearest item BEFORE the one asked for. this may be
* a filler item, of course. this routine is for use by the internal
* structure management only; generally list_gotoitem() is the one to use
*
******************************************************************************/

extern P_LIST_ITEM
list_igotoitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item)
{
    P_LIST_ITEM it;

    return((it = list_gotoitem(lp, item)) != NULL
                 ? it
                 : lp ? convofftoptr(lp, lp->pooldix, lp->itemc)
                      : NULL);
}

/******************************************************************************
*
* initialise a list block
*
******************************************************************************/

extern void
list_init(
    P_LIST_BLOCK lp,
    S32 maxitemsize,
    S32 maxpoolsize)
{
    lp->itemc = 0;
    lp->pooldblkh = 0;
    lp->pooldix = 0;
    lp->pooldblksize = 0;
    lp->pooldblkfree = 0;
    lp->item = 0;
    lp->numitem = 0;
    lp->poold = lp->pooldblkp = NULL;
    lp->nextblock = NULL;

    lp->maxitemsize = ((maxitemsize / SIZEOF_ALIGN_T) + 1) *
                        SIZEOF_ALIGN_T + LIST_ITEMOVH;
    lp->maxitemsize = MIN(MAX_POOL, lp->maxitemsize);
    lp->maxpoolsize = MIN(MAX_POOL, maxpoolsize);
    lp->poolsizeinc = MAX(lp->maxpoolsize / 4, lp->maxitemsize);
}

/******************************************************************************
*
* initialise a sequence
*
******************************************************************************/

extern P_LIST_ITEM
list_initseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop)
{
    P_LIST_ITEM it;

    #ifdef SPARSE_SEQ_DEBUG
    trace_3(TRACE_MODULE_LIST, "list_initseq(&%p, &%p (current %d))\n", lp, itemnop, *itemnop);
    #endif

    if((it = list_igotoitem(lp, *itemnop)) == NULL)
        return(NULL);

    if(!(it->flags & LIST_FILL))
        {
        *itemnop = list_atitem(lp);
        return(it);
        }

    return(list_nextseq(lp, itemnop));
}

/******************************************************************************
*
* insert items into a list
*
******************************************************************************/

extern S32
list_insertitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    LIST_ITEMNO numins)
{
    P_LIST_ITEM it, psl;

    /* if there's no item there, do nothing */
    if(((it = list_igotoitem(lp, item)) == NULL) || (item >= list_numitem(lp)))
        return(0);

    /* if we have a filler item either side,
     * add to the filler item to insert
    */
    if((it->flags & LIST_FILL))
        {
        it->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(0);
        }

    psl = (item == 0) ? NULL : list_igotoitem(lp, item - 1);
    if(psl && ((psl->flags & LIST_FILL)))
        {
        psl->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(0);
        }

    /* create filler at insert position */
    list_igotoitem(lp, item);
    if(!fillbefore(lp, numins, (LIST_ITEMNO) 0))
        return(-1);
    updatepoolitems(lp, numins);

    return(0);
}

/******************************************************************************
*
* get next item in sequence
*
******************************************************************************/

extern P_LIST_ITEM
list_nextseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop)
{
    P_LIST_ITEM it;
    P_U8 pp;
    pooldp pdp;

    #ifdef SPARSE_SEQ_DEBUG
    trace_4(TRACE_MODULE_LIST, "list_nextseq(&%p, &%p (current %d)): numitem = %d\n", lp, itemnop, *itemnop, list_numitem(lp));
    #endif

    validatepools();

    /* get current slot pointer */
    if( (lp->item != *itemnop)      ||
        ((pdp = lp->poold) == NULL) ||
        ((pp = pdp->pool) == NULL)  )
        {
        #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
        if(!pdp || !pp)
            trace_0(TRACE_MODULE_LIST, "list_nextseq: reloading pdp/pp\n");
        else
            trace_2(TRACE_MODULE_LIST, "list_nextseq: lp->item %d != *itemnop %d\n", lp->item, *itemnop);
        #endif

        if((it = list_igotoitem(lp, *itemnop)) == NULL)
            return(NULL);
        }
    else
        it = (P_LIST_ITEM) (pp + lp->itemc);

    #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
    trace_1(TRACE_MODULE_LIST, "list_nextseq: it = &%p\n", it);
    #endif

    /* skip over fillers to next real row */
    do  {
        /* work out next row boundary and move to it */
        *itemnop = lp->item + list_leapnext(it);
        #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
        trace_1(TRACE_MODULE_LIST, "list_nextseq: *itemnop := %d\n", *itemnop);
        #endif
        if(*itemnop >= list_numitem(lp))
            return(NULL);
        it = list_igotoitem(lp, *itemnop);
        }
    while(it->flags & LIST_FILL);

    #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
    trace_1(TRACE_MODULE_LIST, "list_nextseq: returning it = &%p\n", it);
    #endif

    return(it);
}

/******************************************************************************
*
* free pool space in a given list upto the amount requested
*
******************************************************************************/

extern S32
list_packlist(
    P_LIST_BLOCK lp,
    S32 needed)
{
    pooldp pdp;
    S32 i;
    S32 space_found;

    validatepools();

    if(needed < 0L)
        needed = INT_MAX;

    trace_2(TRACE_MODULE_LIST, "list_packlist(&%p, %d)\n", report_ptr_cast(lp), needed);

    if(!lp)
        return(0);

    for(i = 0, space_found = 0, pdp = getpooldp(lp, 0);
        pdp && i < lp->pooldblkfree && space_found < needed;
        ++i, ++pdp)
        {
        trace_4(TRACE_MODULE_LIST, "list_packlist: consider pdp &%p, poolh %d, size %d, free %d\n",
                report_ptr_cast(pdp), pdp->poolh, pdp->poolsize, pdp->poolfree);

        if(pdp->poolh  &&  (pdp->poolsize > pdp->poolfree))
            {
            space_found += (S32) pdp->poolsize - pdp->poolfree;
            reallocpool(lp, pdp->poolfree, i);
            /* reload pointer after dealloc */
            pdp = getpooldp(lp, i);
            }
        }

    /* pack descriptor if needed;
     * +1 added to dblkfree; see comment in allocpool
     * MRJC 23.9.91
     */
    if(space_found < needed &&
       lp->pooldblksize > lp->pooldblkfree + 1)
        {
        ARRAY_HANDLE newpdh;

        /* unlock descriptor block for realloc */
        if(lp->pooldblkp)
            lp->pooldblkp = lp->poold = NULL;

        newpdh = list_reallochandle(lp->pooldblkh, sizeof(struct POOLDESC) * ((S32) lp->pooldblkfree + 1));
        space_found += ((S32) lp->pooldblksize - lp->pooldblkfree - 1) * sizeof(struct POOLDESC);
        lp->pooldblksize = lp->pooldblkfree + 1;
        lp->pooldblkh = newpdh;
        }

    trace_2(TRACE_MODULE_LIST, "list_packlist: found %d, in lp &%p\n", space_found, report_ptr_cast(lp));

    return(space_found);
}

/******************************************************************************
*
* get next item in sequence
*
******************************************************************************/

extern P_LIST_ITEM
list_prevseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    validatepools();

    if((item = *itemnop) != 0)
        {
        do
            {
            it = list_igotoitem(lp, item - 1);
            item = list_atitem(lp);
            }
        while((it->flags & LIST_FILL) && item);

        *itemnop = item;

        if(it->flags & LIST_FILL)
            return(NULL);

        return(it);
        }
    else
        return(NULL);
}

/******************************************************************************
*
* reallocate some memory given a handle
*
******************************************************************************/

extern ARRAY_HANDLE
list_reallochandle(
    ARRAY_HANDLE hand,
    S32 bytes)
{
    trace_2(TRACE_MODULE_LIST, "list_reallochandle(%d, %d)\n", hand, bytes);

    validatepools();

    if(!hand)
        return(list_allochandle(bytes));

    if(!bytes)
        {
        trace_2(TRACE_MODULE_LIST, "list_reallochandle: %d, size: %d\n", 0, 0);
        list_deallochandle(hand);
        return(0);
        }

    do  {
        ARRAY_HANDLE array_handle = (ARRAY_HANDLE) hand;
        S32 curr_bytes = array_elements32(&array_handle);
        STATUS status;

        if(curr_bytes == bytes)
            return(hand);

        if(curr_bytes > bytes)
        {
            al_array_shrink_by(&array_handle, bytes - curr_bytes);
            return(hand);
        }

        if(NULL != al_array_extend_by(&array_handle, BYTE, bytes - curr_bytes, &handlist_array_init_block, &status))
            {
            trace_2(TRACE_MODULE_LIST, "list_reallochandle: %d, size: %d\n", hand, bytes);
            return(hand);
            }

        trace_0(TRACE_MODULE_LIST, "************** AL_ARRAY_REALLOC FAILED **************\n");
        status_assert(status);
        }
    while(list_unlockpools() || list_freepoolspace(-1));

    trace_2(TRACE_MODULE_LIST, "list_reallochandle: %d, size: %d\n", 0, 0);
    return(0);
}

/******************************************************************************
*
* change the size of an existing block of memory,
* given a pointer to the memory
*
******************************************************************************/

extern P_ANY
list_reallocptr(
    P_ANY mempnt,
    S32 bytes)
{
    STATUS status;

    return(_al_ptr_realloc(mempnt, bytes, &status));
}

/******************************************************************************
*
* add list block to chain
*
******************************************************************************/

extern void
list_register(
    P_LIST_BLOCK lp)
{
    validatepools();
    validatepool(lp);

    lp->nextblock = firstblock;
    firstblock = lp;

    #ifdef SPARSE_REGISTER_DEBUG
    {
    P_LIST_BLOCK lpt;

    trace_1(TRACE_MODULE_LIST, "list_register: %x\n", (S32) lp);

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        trace_1(TRACE_MODULE_LIST, "lp: %x registered\n", (S32) lpt);

    }
    #endif
}

/******************************************************************************
*
* unlock any pools which are locked so that
* the allocator can do any dreaded shuffling
*
* --out--
* flag indicates whether any pools were locked
*
******************************************************************************/

extern S32
list_unlockpools(void)
{
    P_LIST_BLOCK lp;
    pooldp pdp;
    S32 res, i, desc_was_locked;

    validatepools();

    trace_0(TRACE_MODULE_LIST, "Unlock pools\n");

    /* unlock all pools and pool descriptors */

    for(lp = firstblock, res = 0; lp; lp = lp->nextblock)
        {
        #ifdef SPARSE_DEBUG
        trace_1(TRACE_MODULE_LIST, "Unlock lp: %x\n", (S32) lp);
        #endif

        desc_was_locked = (lp->pooldblkp != NULL);
        pdp = getpooldp(lp, 0);
        if(pdp)
            {
            #ifdef SPARSE_DEBUG
            trace_1(TRACE_MODULE_LIST, "Unlock pooldblkp: %x\n", (S32) lp->pooldblkp);
            #endif

            for(i = 0; i < lp->pooldblkfree; ++i, ++pdp)
                {
                #ifdef SPARSE_DEBUG
                if(pdp->pool)
                    trace_2(TRACE_MODULE_LIST, "Discarding pool ptr for: %d, pool %p\n",
                            pdp - lp->pooldblkp, pdp->pool);
                #endif

                if(pdp->pool)
                    {
                    pdp->pool = NULL;
                    res = 1;
                    }
                }

            /* only say descriptor block unlocked if it was locked to start with */
            if(desc_was_locked)
                res = 1;
            }

        lp->pooldblkp = lp->poold = NULL;
        }

    /* 'unlocking' blocks on RISCOS certainly won't
     * allow an allocation to succeed
     */
    #if RISCOS
    res = 0;
    #endif

    return(res);
}

/******************************************************************************
*
* add in item to list after current position
*
******************************************************************************/

static P_LIST_ITEM
addafter(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;
    OFF_TYPE olditemc;
    LIST_ITEMNO olditem;

    olditemc = -1;
    if(lp->poold && lp->poold->pool)
        {
        olditemc = lp->itemc;
        olditem = lp->item;

        it = (P_LIST_ITEM) (((P_U8) lp->poold->pool) + lp->itemc);
        if(it->offsetn)
            lp->itemc += it->offsetn;
        else
            lp->itemc = lp->poold->poolfree;
        lp->item += list_leapnext(it);
        }

    it = addbefore(lp, size, adjust);

    if(!it  &&  (olditemc != -1))
        {
        lp->itemc = olditemc;
        lp->item = olditem;
        }

    return(it);
}

/******************************************************************************
*
* add in item to list before current position
*
******************************************************************************/

static P_LIST_ITEM
addbefore(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    it = allocitem(lp, size, adjust);

    if(it)
        it->flags = 0;

    return(it);
}

/******************************************************************************
*
* allocate memory for a item
*
******************************************************************************/

static P_LIST_ITEM
allocitem(
    P_LIST_BLOCK lp,
    S32 size,
    LIST_ITEMNO adjust)
{
    pooldp pdp;
    OFF_TYPE newsl;
    P_LIST_ITEM it, nextsl, prevsl;

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    /* create a pool if we have none */
    if(!lp->poold)
        {
        if(!allocpool(lp, lp->pooldix, lp->poolsizeinc))
            return(NULL);
        else
            if((pdp = lp->poold) == NULL)
                pdp = getpooldp(lp, lp->pooldix);
        }

    size += LIST_ITEMOVH;

    /* round up size */
    if(size & (SIZEOF_ALIGN_T - 1))
        size += SIZEOF_ALIGN_T;
/*  size &= (0 - SIZEOF_ALIGN_T);*/
    size &= ~(SIZEOF_ALIGN_T - 1); /* SKS 11jul91 */

    /* is there room in the pool ? */
    if(pdp->poolfree + size > pdp->poolsize)
        {
        #ifdef SPARSE_DEBUG
        trace_0(TRACE_MODULE_LIST, "allocitem - no room in pool\n");
        #endif

        /* check if pool must be split */
        if(pdp->poolsize + size >= lp->maxpoolsize)
            {
            #ifdef SPARSE_DEBUG
            trace_0(TRACE_MODULE_LIST, "allocitem - splitting pool\n");
            #endif

            if(!splitpool(lp, adjust))
                return(NULL);
            }
        else if(!reallocpool(lp,
                             pdp->poolsize + lp->poolsizeinc,
                             lp->pooldix))
            return(NULL);

        /* reload pool descriptor pointer */
        pdp = getpooldp(lp, lp->pooldix);
        }

    #ifdef SPARSE_DEBUG
    trace_2(TRACE_MODULE_LIST, "allocitem: pdp: %x, pdp->pool: %x\n",
            (S32) pdp, (S32) pdp->pool);
    #endif

    /* insert space into pool */
    newsl = lp->itemc;
    it = (P_LIST_ITEM) (pdp->pool + newsl);
    nextsl = (P_LIST_ITEM) (pdp->pool + lp->itemc + size);

    /* is there a item after this one ? */
    if(pdp->poolfree - newsl)
        {
        void_memmove32(nextsl, it, pdp->poolfree - newsl);
        prevsl = nextsl->offsetp ? (P_LIST_ITEM) (((P_U8) it) - nextsl->offsetp)
                                 : NULL;
        nextsl->offsetp = it->offsetn = size;
        }
    else
        {
        it->offsetn = 0;
        if(newsl)
            {
            /* find previous item, painfully, by working down */
            prevsl = (P_LIST_ITEM) pdp->pool;
            while(prevsl->offsetn)
                prevsl = (P_LIST_ITEM) (((P_U8) prevsl) + prevsl->offsetn);
            }
        else
            prevsl = NULL;
        }

    /* is there a item before this one ? */
    if(prevsl)
        it->offsetp = prevsl->offsetn = ((P_U8) it) - ((P_U8) prevsl);
    else
        it->offsetp = 0;

    /* update free space in pool */
    pdp->poolfree += size;

    return(it);
}

/******************************************************************************
*
* allocate a memory pool, leaving it locked
*
******************************************************************************/

static P_U8
allocpool(
    P_LIST_BLOCK lp,
    S32 poolix,
    OFF_TYPE size)
{
    pooldp pdp, pdpi;
    S32 i;
    ARRAY_HANDLE poolh;

    /* check that pool descriptor block is large enough */
    if(lp->pooldblkfree >= lp->pooldblksize)
        {
        if(!lp->pooldblkh)
            {
            lp->pooldblkh = list_allochandle(sizeof(struct POOLDESC) * (S32) POOLDBLKSIZEINC);
            if(!lp->pooldblkh)
                return(NULL);

            lp->pooldblksize = POOLDBLKSIZEINC;

            #ifdef SPARSE_DEBUG
            trace_1(TRACE_MODULE_LIST, "allocpool allocated dblkh: %d\n", lp->pooldblkh);
            #endif
            }
        else
            {
            ARRAY_HANDLE newpdh;

            /* unlock descriptor block for realloc */
            if(lp->pooldblkp)
                {
                lp->pooldblkp = lp->poold = NULL;
                }

            newpdh = list_reallochandle(lp->pooldblkh, sizeof(struct POOLDESC) * ((S32) lp->pooldblksize + POOLDBLKSIZEINC));
            if(!newpdh)
                return(NULL);

            #ifdef SPARSE_DEBUG
            trace_1(TRACE_MODULE_LIST, "allocpool reallocated dblkh: %d\n", lp->pooldblkh);
            #endif

            lp->pooldblksize += POOLDBLKSIZEINC;
            lp->pooldblkh = newpdh;
            }
        }

    #if 1
    /* packlist now never frees the dblkfree+1 entry; not only could
     * it free the entry we had just allocated but thrashing could occur:
     * expand dblk; poolh allochandle fails; caller calls packlist, freeing dblk;
     * recalls allocpool which expands dblk etc etc
     * MRJC 23.9.91
     */
    if((poolh = list_allochandle((S32) size)) == 0)
        return(NULL);

    pdp = getpooldp(lp, lp->pooldblkfree++);
    pdp->poolh = 0;
    pdp->poolsize = 0;
    pdp->poolitem = 0;
    pdp->poolfree = 0;
    pdp->pool = NULL;

    #else
    /* ensure that shrinker never frees the entry we need;
     * create dummy entry so that packer does not try to discard
    */
    pdp = getpooldp(lp, lp->pooldblkfree++);
    pdp->poolh = 0;
    pdp->poolsize = 0;
    pdp->poolitem = 0;
    pdp->poolfree = 0;
    pdp->pool = NULL;

    /* allocate a pool */
    poolh = list_allochandle((S32) size);
    if(!poolh)
        {
        --lp->pooldblkfree;
        return(NULL);
        }
    #endif

    /* load address of descriptor block;
     * also loads lp->pooldblkp
    */
    pdp = getpooldp(lp, poolix);

    /* insert new pool into pool descriptor block */
    for(i = lp->pooldblkfree - 1, pdpi = &lp->pooldblkp[i];
        i > poolix; --i, --pdpi)
            *pdpi = *(pdpi - 1);

    if((lp->pooldix >= poolix) &&
       (lp->pooldix != lp->pooldblkfree - 1))
        {
        ++lp->pooldix;
        if(lp->poold)
            ++lp->poold;
        }

    pdp->poolh = poolh;
    pdp->poolsize = size;
    pdp->poolitem = 0;
    pdp->poolfree = 0;
    pdp->pool = NULL;

    return(getpoolptr(pdp));
}

/******************************************************************************
*
* extract item pointer from list block
*
******************************************************************************/

static P_LIST_ITEM
convofftoptr(
    P_LIST_BLOCK lp,
    S32 poolix,
    OFF_TYPE off)
{
    pooldp pdp;
    P_U8 pp;

    if((pdp = getpooldp(lp, poolix)) == NULL)
        return(NULL);

    if((pp = pdp->pool) == NULL)
        if((pp = getpoolptr(pdp)) == NULL)
            return(NULL);

    return((P_LIST_ITEM) (pp + off));
}

/******************************************************************************
*
* deallocate an item
*
******************************************************************************/

static void
deallocitem(
    P_LIST_BLOCK lp,
    P_LIST_ITEM it)
{
    OFF_TYPE size, itoff;
    P_U8 pp;
    P_LIST_ITEM nextsl, prevsl;
    pooldp pdp;
    LIST_ITEMNO trailfill;

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    if((pp = pdp->pool) == NULL)
        pp = getpoolptr(pdp);

    /* work out size */
    itoff = ((P_U8) it) - pp;
    if(it->offsetn)
        size = it->offsetn;
    else
        {
        if(it->offsetp)
            size = pdp->poolfree - itoff;
        else
            {
            deallocpool(lp, lp->pooldix);
            return;
            }
        }

    /* sort out item links */
    if(it->offsetn)
        {
        nextsl = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);

        /* check for special case of trailing filler slot */
        if(!it->offsetp &&
           !nextsl->offsetn &&
           (nextsl->flags & LIST_FILL) &&
           ((lp->pooldix + 1) == lp->pooldblkfree))
            {
            trailfill = list_leapnext(nextsl);
            deallocpool(lp, lp->pooldix);
            lp->numitem -= trailfill;

            #ifdef SPARSE_DEBUG
            trace_2(TRACE_MODULE_LIST, "deallocitem lp: %x, numitem: %d\n",
                    (S32) lp, lp->numitem);
            #endif

            return;
            }

        nextsl->offsetp = it->offsetp;
        }
    else
        {
        /* step back to previous item so we don't hang in space */
        prevsl = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        lp->itemc -= it->offsetp;
        lp->item -= list_leapnext(prevsl);
        prevsl->offsetn = 0;
        }

    void_memmove32(it, ((P_U8) it) + size, pdp->poolfree - size - itoff);
    pdp->poolfree -= size;
}

/******************************************************************************
*
* deallocate pool and remove from pool descriptor block
*
******************************************************************************/

static void
deallocpool(
    P_LIST_BLOCK lp,
    S32 poolix)
{
    pooldp pdp;
    S32 i;

    pdp = getpooldp(lp, poolix);

    /* the pool is dead, kill the pool */
    list_deallochandle(pdp->poolh);

    /* remove descriptor */
    for(i = poolix; i < lp->pooldblkfree - 1; ++i, ++pdp)
        *pdp = *(pdp + 1);

    if(!--lp->pooldblkfree)
        {
        /* deallocate pool descriptor block */
        list_deallochandle(lp->pooldblkh);

        lp->pooldblkh = 0;
        lp->pooldblksize = 0;
        lp->pooldblkfree = 0;
        lp->pooldix = 0;
        lp->itemc = 0;
        lp->item = lp->numitem = 0;
        lp->pooldblkp = lp->poold = NULL;
        }
    else if(lp->pooldix >= lp->pooldblkfree)
        {
        /* make sure not pointing past the end */
        --lp->pooldix;
        --lp->poold;
        lp->item = getpooldp(lp, lp->pooldix)->poolitem;
        lp->itemc = 0;
        }

    return;
}

/******************************************************************************
*
* create filler item after current position in list
*
******************************************************************************/

static P_LIST_ITEM
fillafter(
    P_LIST_BLOCK lp,
    LIST_ITEMNO itemfill,
    LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    it = addafter(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->flags |= LIST_FILL;
    it->i.itemfill = itemfill;
    return(it);
}

/******************************************************************************
*
* create filler item in list before specified item
*
******************************************************************************/

static P_LIST_ITEM
fillbefore(
    P_LIST_BLOCK lp,
    LIST_ITEMNO itemfill,
    LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    it = addbefore(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->flags |= LIST_FILL;
    it->i.itemfill = itemfill;
    return(it);
}

/******************************************************************************
*
* get pointer to descriptor block
*
******************************************************************************/

static pooldp
getpooldp(
    P_LIST_BLOCK lp,
    S32 poolix)
{
    pooldp pdp;

    if(!lp->pooldblkp)
        {
        if(!lp->pooldblkh)
            return(NULL);
        else
            {
            lp->pooldblkp = list_getptr(lp->pooldblkh);

            #ifdef SPARSE_DEBUG
            trace_1(TRACE_MODULE_LIST, "loaded new pooldblkp: %x\n", (S32) lp->pooldblkp);
            #endif
            }
        }

    pdp = (poolix >= lp->pooldblkfree) ? NULL : lp->pooldblkp + poolix;

    /* set temporary pointer save */
    if(poolix == lp->pooldix)
        lp->poold = pdp;

    return(pdp);
}

/******************************************************************************
*
* return the address of a pool of memory
*
******************************************************************************/

static P_U8
getpoolptr(
    pooldp pdp)
{
    if(!pdp->pool)
        {
        if(!pdp->poolh)
            return(NULL);

        pdp->pool = (P_U8) list_getptr(pdp->poolh);

        #ifdef SPARSE_DEBUG
        trace_1(TRACE_MODULE_LIST, "loading poolptr %p\n", pdp->pool);
        #endif
        }

    return(pdp->pool);
}

/******************************************************************************
*
* reallocate memory for a item
*
******************************************************************************/

static P_LIST_ITEM
reallocitem(
    P_LIST_BLOCK lp,
    P_LIST_ITEM it,
    S32 newsize)
{
    P_U8 pp;
    pooldp pdp;
    OFF_TYPE extraspace, itemend, oldsize;

    /* check for a delete */
    if(!newsize)
        {
        deallocitem(lp, it);
        return(NULL);
        }

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    /* create a pool if we have none */
    if(!pdp)
        return(allocitem(lp, newsize, (LIST_ITEMNO) 0));

    lp->itemc = ((P_U8) it) - pdp->pool;

    /* round up size */
    newsize += LIST_ITEMOVH;
    if(newsize & (SIZEOF_ALIGN_T - 1))
        newsize += SIZEOF_ALIGN_T;
/*  newsize &= (0 - SIZEOF_ALIGN_T);*/
    newsize &= ~(SIZEOF_ALIGN_T - 1);

    /* work out old size */
    if(it->offsetn)
        oldsize = it->offsetn;
    else
        oldsize = pdp->poolfree - lp->itemc;

    extraspace = it ? newsize - oldsize : newsize;

    /* is there room in the pool ? */
    if(pdp->poolfree + extraspace >= pdp->poolsize)
        {
        /* check if pool must be split */
        if(pdp->poolsize + extraspace >= lp->maxpoolsize)
            {
            if(!splitpool(lp, (LIST_ITEMNO) 0))
                return(NULL);
            }
        else if(!reallocpool(lp,
                             pdp->poolsize + lp->poolsizeinc,
                             lp->pooldix))
            return(NULL);

        /* re-load descriptor pointer */
        if((pdp = lp->poold) == NULL)
            pdp = getpooldp(lp, lp->pooldix);
        }

    /* adjust pool for space needed */
    if((pp = pdp->pool) == NULL)
        pp = getpoolptr(pdp);

    itemend = lp->itemc + oldsize;
    void_memmove32(pp + itemend + extraspace, pp + itemend, pdp->poolfree - itemend);
    pdp->poolfree += extraspace;

    /* store new item size in item, & update link in next item */
    it = (P_LIST_ITEM) (pp + lp->itemc);
    if(it->offsetn)
        {
        it->offsetn = (LINK_TYPE) (oldsize + extraspace);
        ((P_LIST_ITEM) (((P_U8) it) + it->offsetn))->offsetp =
                            (LINK_TYPE) (oldsize + extraspace);
        }

    return(it);
}

/******************************************************************************
*
* reallocate a pool of memory
*
******************************************************************************/

static P_U8
reallocpool(
    P_LIST_BLOCK lp,
    OFF_TYPE size,
    S32 ix)
{
    ARRAY_HANDLE mh;
    pooldp pdp;

    pdp = getpooldp(lp, ix);

    #ifdef SPARSE_DEBUG
    trace_2(TRACE_MODULE_LIST, "reallocpool pdp: %x, pdp->pool: %x\n",
            (S32) pdp, (S32) pdp->pool);
    #endif

    if(pdp->pool)
        {
        pdp->pool = NULL;
        }

    #ifdef SPARSE_DEBUG
    trace_2(TRACE_MODULE_LIST, "reallocpool * pdp->pool: %d, pooldblkh: %d\n",
            pdp->pool, lp->pooldblkh);
    #endif

    mh = list_reallochandle(pdp->poolh, (S32) size);
    if(!mh)
        return(NULL);

    pdp = getpooldp(lp, ix);
    pdp->poolh = mh;
    pdp->poolsize = size;

    /* clear pool pointer again; freepoolspace may reload the pool pointer
     * under our feet, leaving it pointing to the old pool !
     * MC 23.9.91
     */
    pdp->pool = NULL;

    #ifdef SPARSE_DEBUG
    trace_2(TRACE_MODULE_LIST, "reallocpool *** poolh: %d, pdp->pool: %x\n",
            pdp->poolh, (S32) pdp->pool);
    trace_2(TRACE_MODULE_LIST, "reallocpool *** pdp: %x, pool: %x\n",
            (S32) pdp, (S32) getpoolptr(pdp));
    #endif

    return(getpoolptr(pdp));
}

/******************************************************************************
*
* split a memory pool
*
******************************************************************************/

static P_U8
splitpool(
    P_LIST_BLOCK lp,
    LIST_ITEMNO adjust)
{
    P_U8 newpp;
    S32 justadd;
    OFF_TYPE newsize;
    pooldp pdp, newpdp;

    #ifdef SPARSE_DEBUG
    trace_0(TRACE_MODULE_LIST, "*** splitting pool\n");
    #endif

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    #ifdef SPARSE_DEBUG
    trace_1(TRACE_MODULE_LIST, "Existing pdp: %x\n", (S32) pdp);
    #endif

    /* if adding to the end of the pool, don't split,
    just add a new pool onto the end */
    justadd = lp->itemc >= pdp->poolfree;
    if(justadd)
        {
        /* free space in old pool for efficiency */
        reallocpool(lp, pdp->poolfree, lp->pooldix);
        newsize = lp->poolsizeinc;
        }
    else
        newsize = pdp->poolsize / 2 + lp->maxitemsize;

    /* allocate new pool */
    if((newpp = allocpool(lp, lp->pooldix + 1, newsize)) == NULL)
        return(NULL);

    #ifdef SPARSE_DEBUG
    trace_1(TRACE_MODULE_LIST, "Newpp: %x\n", (S32) newpp);
    #endif

    /* re-load pointer */
    pdp = getpooldp(lp, lp->pooldix);

    #ifdef SPARSE_DEBUG
    trace_1(TRACE_MODULE_LIST, "Pdp after newpp alloced: %x\n", (S32) pdp);
    #endif

    /* load pointer for new pool */
    newpdp = getpooldp(lp, lp->pooldix + 1);

    #ifdef SPARSE_DEBUG
    trace_1(TRACE_MODULE_LIST, "Newpdp: %x\n", (S32) newpdp);
    #endif

    /* are we adding the item to the end of the pool? */
    if(!justadd)
        {
        P_U8 pp;
        P_LIST_ITEM it, psl;
        LIST_ITEMNO itemc;
        OFF_TYPE offset, split, bytes;

        #ifdef SPARSE_DEBUG
        trace_2(TRACE_MODULE_LIST, "Existing pdp->pool: %x, poolix: %d\n",
                (S32) pdp->pool, lp->pooldix);
        #endif

        /* get address of current pool */
        if((pp = pdp->pool) == NULL)
            pp = getpoolptr(pdp);

        #ifdef SPARSE_DEBUG
        trace_1(TRACE_MODULE_LIST, "Existing pp: %x\n", (S32) pp);
        #endif

        /* find a place to split */
        itemc = pdp->poolitem;
        it = (P_LIST_ITEM) pp;
        offset = 0;
        split = pdp->poolsize / 2;

        while(offset < split)
            {
            itemc += list_leapnext(it);
            offset += it->offsetn;
            it = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);
            if(adjust && itemc > lp->item)
                {
                itemc += adjust;
                #ifdef SPARSE_DEBUG
                trace_2(TRACE_MODULE_LIST, "***** splitpool adding adjust of: %d, new itemc: %d *****\n",
                        adjust, itemc);
                #endif
                adjust = 0;
                }
            }

        #ifdef SPARSE_DEBUG
        trace_2(TRACE_MODULE_LIST, "Offset: %d, Split: %d\n", offset, split);
        #endif

        /* copy second half into new pool */
        psl = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        psl->offsetn = it->offsetp = 0;
        bytes = pdp->poolfree - (((P_U8) it) - pp);

        #ifdef SPARSE_DEBUG
        trace_3(TRACE_MODULE_LIST, "Newpp: %x, it: %x, bytes: %d\n", (S32) newpp, (S32) it, bytes);
        #endif

        void_memmove32(newpp, it, bytes);

        /* update descriptor blocks */
        pdp->poolfree = ((P_U8) it) - pp;

        newpdp->poolitem = itemc;
        newpdp->poolfree = bytes;

        /* if item in next block, update item offset and pool descriptor */
        if(lp->itemc >= pdp->poolfree)
            {
            lp->itemc -= pdp->poolfree;
            ++lp->pooldix;
            lp->poold = NULL;
            }
        }
    else
        {
        lp->itemc = 0;
        ++lp->pooldix;
        lp->poold = NULL;
        newpdp->poolitem = lp->item;
        }

    /* re-load pointers */
    pdp = getpooldp(lp, lp->pooldix);
    return(getpoolptr(pdp));
}

/******************************************************************************
*
* update the item counts in pools below the current pool after an insert or a delete
*
******************************************************************************/

static void
updatepoolitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO change)
{
    pooldp pdp;
    S32 i;

    if((pdp = lp->poold) == NULL)
        if((pdp = getpooldp(lp, lp->pooldix)) == NULL)
            return;

    for(i = lp->pooldix; i < lp->pooldblkfree; ++i, ++pdp)
        if(pdp->poolitem > lp->item)
            pdp->poolitem += change;

    if(lp->numitem)
        {
        lp->numitem += change;

        #ifdef SPARSE_DEBUG
        trace_2(TRACE_MODULE_LIST, "updatepoolitems lp: %x, numitem: %d\n",
                (S32) lp, lp->numitem);
        #endif
        }
}

/******************************************************************************/

#if defined(SPARSE_VALIDATE_DEBUG) && TRACE_ALLOWED

/******************************************************************************
*
* attempt to validate the pool structure
*
******************************************************************************/

static void
valfatal(
    int rc,
    P_ANY p1,
    P_ANY p2)
{
    char *ptr;

    switch(rc)
        {
        case 1:
            ptr = "lp &%p < al_start &%p";
            break;

        case 2:
            ptr = "lp &%p >= al_end &%p";
            break;

        case 3:
            ptr = "lp->pooldblkp &%p < fl_start &%p";
            break;

        case 4:
            ptr = "lp->pooldblkp &%p >= fl_end &%p";
            break;

        case 5:
            ptr = "!lp->pooldblkh but lp->pooldblkp &%p";
            break;

        case 6:
            ptr = "lp->pooldblkh %d not in handleblock (size %d)";
            break;

        case 7:
            ptr = "lp->pooldblkp &%p not same as handleblock[i] &%p";
            break;

        case 8:
            ptr = "handleblock[i] &%p < fl_start &%p";
            break;

        case 9:
            ptr = "handleblock[i] &%p >= fl_end &%p";
            break;

        case 10:
            ptr = "!pdp->poolh but pdp->pool &%p";
            break;

        case 11:
            ptr = "pdp->poolh %d not in handleblock (size %d)";
            break;

        case 12:
            ptr = "pdp->pool &%p not same as handleblock[i] &%p";
            break;

        case 13:
            ptr = "pdp->pool &%p < fl_start &%p";
            break;

        case 14:
            ptr = "pdp->pool &%p >= fl_end &%p";
            break;

        default:
            ptr = "??? &%p &%p rc = %d";
            break;
        }

    reportf(ptr, p1, p2, rc);
}

static S32 validate_enabled = FALSE;

static P_ANY fl_start = (P_ANY) (     32*1024);
static P_ANY fl_end   = (P_ANY) (32*1024*1024);
static P_ANY al_start = (P_ANY) (     32*1024);
static P_ANY al_end   = (P_ANY) (32*1024*1024);

static void
cachebits(void)
{
    /* release version may not have either of these */
    /*alloc_limits(&al_start, &al_end, NULL);*/
    /*flex_limits (&fl_start, &fl_end, NULL, NULL);*/
}

static void
_validatepool(
    P_LIST_BLOCK lp,
    S32 recache)
{
    /* validate all pools and pool descriptors */
    P_ANY ptr;
    pooldp pdp;
    S32 i;
    S32 desc_was_locked, pool_was_locked;

    if(!validate_enabled)
        return;

    trace_1(TRACE_MODULE_LIST, "Validating lp &%p", lp);

    if(recache)
        cachebits();

    if((P_U8) lp < (P_U8) al_start)
        /* bad lp */
        valfatal(1, lp, al_start);

    if((P_U8) lp >= (P_U8) al_end)
        /* bad lp */
        valfatal(2, lp, al_end);

    desc_was_locked = (lp->pooldblkp != NULL);

    trace_2(TRACE_MODULE_LIST, ": descriptor handle %d ptr &%p\n", lp->pooldblkh, lp->pooldblkp);

    if(lp->pooldblkh)
        {
        if((U32) lp->pooldblkh >= (U32) handlefree)
            /* not in table */
            valfatal(6, (P_ANY) lp->pooldblkh, (P_ANY) handlefree);

        if(lp->pooldblkp)
            if(lp->pooldblkp != (ptr = list_getptr(lp->pooldblkh)))
                /* cached ptr different to that in table */
                valfatal(7, lp->pooldblkp, ptr);
        }
    else
        {
        if(lp->pooldblkp)
            /* no handle but crap ptr */
            valfatal(5, lp->pooldblkp, NULL);
        }

    if(lp->pooldblkp)
        {
        if((P_U8) lp->pooldblkp < (P_U8) fl_start)
            valfatal(3, lp->pooldblkp, fl_start);

        if((P_U8) lp->pooldblkp >= (P_U8) fl_end)
            valfatal(4, lp->pooldblkp, fl_end);
        }

    pdp = lp->pooldblkp = list_getptr(lp->pooldblkh);

    if(pdp)
        {
        trace_2(TRACE_MODULE_LIST, "Validating pools in descriptor &%p: free %d\n", lp->pooldblkp, lp->pooldblkfree);

        for(i = 0; i < lp->pooldblkfree; ++i, ++pdp)
            {
            pool_was_locked = (pdp->pool != NULL);

            trace_5(TRACE_MODULE_LIST, "Validating pool (&%p[%d] = &%p): handle %d ptr &%p\n", lp->pooldblkp, i, pdp, pdp->poolh, pdp->pool);

            if(pdp->poolh)
                {
                if((U32) pdp->poolh >= (U32) handlefree)
                    /* not in table */
                    valfatal(10, (void *) pdp->poolh, (void *) handlefree);

                if(pdp->pool)
                    if(pdp->pool != (ptr = list_getptr(pdp->poolh)))
                        /* cached ptr different to that in table */
                        valfatal(11, pdp->pool, ptr);
                }
            else
                {
                if(pdp->pool)
                    /* no handle but crap ptr */
                    valfatal(12, pdp->pool, NULL);
                }

            if(pdp->pool)
                {
                if((P_U8) pdp->pool < (P_U8) fl_start)
                    valfatal(13, pdp->pool, fl_start);

                if((P_U8) pdp->pool >= (P_U8) fl_end)
                    valfatal(14, pdp->pool, fl_end);
                }

            /* would be really tedious to check contents! */

            if(!pool_was_locked)
                {
                pdp->pool = NULL;
                }
            }

        if(!desc_was_locked)
            {
            lp->pooldblkp = NULL;
            }
        }
}

static void
validatepools(void)
{
    S32 i;
    hand_ent_p hep;
    P_ANY ptr;
    P_LIST_BLOCK lp;

    if(!validate_enabled)
        return;

    trace_1(TRACE_MODULE_LIST, "Validate handleblock[0..%d]: ", handlefree);
    cachebits();

    for(i = 0; i < handlefree; ++i)
        {
        hep = handleblkp + i;
        ptr = hep->pointer;
        trace_2(TRACE_MODULE_LIST, "%d] = &%p  ", i, ptr);
        if(ptr)
            {
            if((P_U8) ptr < (P_U8) fl_start)
                valfatal(8, ptr, fl_start);

            if((P_U8) ptr >= (P_U8) fl_end)
                valfatal(9, ptr, fl_end);
            }
        }

    trace_0(TRACE_MODULE_LIST, "\n[Validate registered pools\n");

    for(lp = firstblock; lp; lp = lp->nextblock)
        _validatepool(lp, 0);
}

extern void
list_toggle_validate(void)
{
    validate_enabled = !validate_enabled;
}

#endif /* SPARSE_VALIDATE_DEBUG */

#ifndef __newhlist_ext

/* ****** fake newhlist calls until real implementation comes in ****** */

#define getlp(lbr) ((P_LIST_BLOCK) (lbr))

extern S32
nlist_allocblkref(
    /*out*/ P_P_LIST_BLKREF lbrp)
{
    P_LIST_BLKREF lbr;

    *lbrp = lbr = list_allocptr(sizeof(LIST_BLOCK));

    if(lbr)
        {
        /* ensure new list block gets sensible defaults */
        list_init(getlp(lbr), MAX_POOL, MAX_POOL);

        list_register(getlp(lbr));
        }

    return(lbr != NULL);
}

extern LIST_ITEMNO
nlist_atitem(
    P_LIST_BLKREF lbr)
{
    return(list_atitem(getlp(lbr)));
}

extern S32
nlist_createitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_P_LIST_ITEM itp,
    S32 size,
    S32 fill)
{
    return((*itp = list_createitem(getlp(lbr), item, size, fill)) != NULL);
}

extern void
nlist_deleteitems(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    LIST_ITEMNO numdel)
{
    list_deleteitems(getlp(lbr), item, numdel);
}

extern void
nlist_disposeblkref(
    /*inout*/ P_P_LIST_BLKREF lbrp)
{
    list_deregister(getlp(*lbrp));

    list_disposeptr(P_P_ANY_PEDANTIC(lbrp));
}

extern S32
nlist_ensureitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item)
{
    return(list_ensureitem(getlp(lbr), item));
}

extern S32
nlist_entsize(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item)
{
    return(list_entsize(getlp(lbr), item));
}

extern void
nlist_free(
    P_LIST_BLKREF lbr)
{
    list_free(getlp(lbr));
}

extern S32
nlist_garbagecollect(
    P_LIST_BLKREF lbr)
{
    return(list_garbagecollect(getlp(lbr)));
}

extern S32
nlist_gotoitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_P_LIST_ITEM itp,
    /*out*/ list_itemposp itposp)
{
    itposp->lbr = lbr;

    return((*itp = list_gotoitem(getlp(lbr), item)) != NULL);
}

extern S32
nlist_gotoitemcontents(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_ANY * mpp,
    /*out*/ list_itemposp itposp)
{
    P_LIST_ITEM it;

    nlist_gotoitem(lbr, item, &it, itposp);

    *mpp = it ? list_itemcontents(it) : NULL;

    return(*mpp != NULL);
}

extern void
nlist_init(
    P_LIST_BLKREF lbr,
    S32 maxitemsize,
    S32 maxpoolsize)
{
    list_init(getlp(lbr), maxitemsize, maxpoolsize);
}

extern S32
nlist_initseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*out*/ list_itemposp itposp)
{
    itposp->lbr = lbr;

    return((*itp = list_initseq(getlp(lbr), itemnop)) != NULL);
}

extern S32
nlist_insertitems(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    LIST_ITEMNO numins)
{
    return(list_insertitems(getlp(lbr), item, numins));
}

extern S32
nlist_nextseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*inout*/ list_itemposp itposp)
{
    itposp->lbr = lbr;

    return((*itp = list_nextseq(getlp(lbr), itemnop)) != NULL);
}

extern LIST_ITEMNO
nlist_numitem(
    P_LIST_BLKREF lbr)
{
    return(list_numitem(getlp(lbr)));
}

extern S32
nlist_packlist(
    P_LIST_BLKREF lbr,
    S32 needed)
{
    return(list_packlist(getlp(lbr), needed));
}

extern S32
nlist_prevseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*inout*/ list_itemposp itposp)
{
    itposp->lbr = lbr;

    return((*itp = list_prevseq(getlp(lbr), itemnop)) != NULL);
}

#endif /* __newhlist_ext */

/* end of handlist.c */
