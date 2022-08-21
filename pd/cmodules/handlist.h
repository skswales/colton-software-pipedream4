/* handlist.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Defines the external interface to the handle based list manager */

/* MRJC January 1989 */

#ifndef __handlist_h
#define __handlist_h

/*
types
*/

/******************************************************************************

Maximum size of items and pools is 32k; this can be increased on machines
where the default integer type is more than 2 bytes by altering LINK_TYPE.
This adds extra overhead for every item stored by handlist - the overhead is
now 5 bytes; using 32 bit links would increase the overhead to 9 bytes.
OFF_TYPE must be large enough to hold 1 bit more information than MAX_POOL -
to allow for overflow during calculations and pool splitting

******************************************************************************/

/* define to hold biggest item no. */
typedef S32 LIST_ITEMNO; typedef LIST_ITEMNO * P_LIST_ITEMNO;
#define LIST_ITEMNO_TFMT S32_TFMT

#ifdef SHORT_INT /* Was for 16-bit Windows; left in as example of use */

typedef U16 OFF_TYPE;
typedef UBF LINK_TYPE;
#define MAX_POOL 32767

#else

typedef S32 OFF_TYPE;                   /* type for pool sizes and offsets */
typedef U16 LINK_TYPE;                  /* type for inter-item links */
#define MAX_POOL (32767 - LIST_ITEMOVH) /* maximum size of pool and item */

#endif /* SHORT_INT */

typedef struct _LIST_BLOCK LIST_BLOCK, * P_LIST_BLOCK, ** P_P_LIST_BLOCK;

typedef struct _LIST_ITEM LIST_ITEM, * P_LIST_ITEM, ** P_P_LIST_ITEM;

/*
block increment sizes
*/

#if WINDOWS
#define HANDLEBLOCKINC 100
#elif RISCOS
#   if !defined(SPARSE_OLD_HANDLEBLOCK)
#       define SPARSE_FLEX_HANDLEBLOCK
#   endif
#   if defined(SPARSE_ENDJUMPER_DEBUG) || defined(SPARSE_FLEX_HANDLEBLOCK)
#       define HANDLEBLOCKINC 10
#       define INITMAXHANDLEBLOCK 30
#   else
#       define HANDLEBLOCKINC 500
#       define INITMAXHANDLEBLOCK 300
#   endif
#endif

#define POOLDBLKSIZEINC 5

/*
data structures
*/

/*
structure of entry in pool block array
*/

struct POOLDESC
{
    ARRAY_HANDLE poolh;
    P_U8        pool;
    LIST_ITEMNO poolitem;
    OFF_TYPE    poolsize;
    OFF_TYPE    poolfree;
};

typedef struct POOLDESC *pooldp;    /* pool descriptor pointer */

struct _LIST_BLOCK
{
    OFF_TYPE itemc;             /* offset to current item */
    LIST_ITEMNO item;           /* item number of current pool */
    pooldp poold;               /* pointer to current pool descriptor */

    ARRAY_HANDLE pooldblkh;     /* handle of block of pool descriptors */
    pooldp pooldblkp;           /* pointer to block of pool descriptors */
    S32 pooldix;                /* number of current pool descriptor */
    S32 pooldblksize;           /* size of block of pool descriptors */
    S32 pooldblkfree;           /* first free in block of pool descriptors */

    LIST_ITEMNO numitem;        /* number of items in this list */
    OFF_TYPE maxitemsize;       /* maximum size of an item */
    OFF_TYPE maxpoolsize;       /* maximum size of a pool */
    OFF_TYPE poolsizeinc;       /* pool size increment */

    P_LIST_BLOCK nextblock;     /* link to next list block */
};

struct _LIST_ITEM
{
    LINK_TYPE offsetn;          /* offset to next item */
    LINK_TYPE offsetp;          /* offset to previous item */
    U8 flags;
    union _list_guts
    {
        LIST_ITEMNO itemfill;   /* fill count */
        U8 inside[1];           /* contents of the item */
    } i;
};

/* overhead per allocated item */
#define LIST_ITEMOVH   (sizeof(LIST_ITEM) - sizeof(union _list_guts))

/* overhead to get to an aligned point (can be subtracted from aligned contents pointer) */
#define LIST_ALIGNOVH  (LIST_ITEMOVH - (offsetof(LIST_ITEM, flags) + sizeof(((list_item *) 0)->flags)))

/*
flags definition
*/

#define LIST_FILL            0x01             /* filler item */
#define LIST_APPLICATION_BIT 0x80             /* reserved for application use */

/*
functions as macros
*/

#define list_atitem(lp) ( \
    (lp) ? (lp)->item : 0 )

#define list_itemcontents(__base_type, it) ( \
    (__base_type *) ((it)->i.inside) )

#define list_leapnext(it) ( \
    ((it)->flags & LIST_FILL) \
        ? (it)->i.itemfill   \
        : 1 )

#define list_numitem(lp) ( \
    (lp) ? (lp)->numitem : 0 )

/*
function declarations
*/

extern P_LIST_ITEM
list_createitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    S32 size,
    S32 fill);

extern void
list_deleteitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    LIST_ITEMNO numdel);

extern void
list_deregister(
    P_LIST_BLOCK lp);

extern S32
list_ensureitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item);

extern S32
list_entsize(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item);

extern void
list_free(
    P_LIST_BLOCK lp);

extern S32
list_freepoolspace(
    S32 needed);

extern S32
list_garbagecollect(
    P_LIST_BLOCK lp);

extern S32
list_howmuchpoolspace(void);

extern P_LIST_ITEM
list_gotoitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item);

/* return a pointer to the given item's contents */

_Check_return_
_Ret_maybenull_
static inline P_ANY
_list_gotoitemcontents(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM it = list_gotoitem(lp, item);

    return(it ? list_itemcontents(void, it) : NULL);
}

#define list_gotoitemcontents(__base_type, p_list_block, item) ( \
    ((__base_type *) _list_gotoitemcontents(p_list_block, item)) )

extern void
list_init(
    P_LIST_BLOCK lp,
    S32 maxitemsize,
    S32 maxpoolsize);

extern P_LIST_ITEM
list_initseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop);

_Check_return_
extern STATUS
list_insertitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    LIST_ITEMNO numins);

extern P_LIST_ITEM
list_nextseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop);

extern S32
list_packlist(
    P_LIST_BLOCK lp,
    S32 needed);

extern P_LIST_ITEM
list_prevseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop);

extern P_ANY
list_reallocptr(
    P_ANY mempnt,
    S32 bytes);

extern void
list_register(
    P_LIST_BLOCK lp);

extern S32
list_unlockpools(void);

#endif /* __handlist_h */

/* end of handlist.h */
