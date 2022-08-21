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

typedef S32     LIST_ITEMNO;               /* define to hold biggest item no. */
typedef LIST_ITEMNO * P_LIST_ITEMNO; typedef const LIST_ITEMNO * PC_LIST_ITEMNO;

typedef S32     OFF_TYPE;                  /* type for pool sizes and offsets */
typedef U16     LINK_TYPE;                 /* type for inter-item links */
#define MAX_POOL (32767 - LIST_ITEMOVH)    /* maximum size of pool and item */

/*
memory handle definition
*/

typedef struct _LIST_BLOCK LIST_BLOCK, * P_LIST_BLOCK, ** P_P_LIST_BLOCK;

#ifndef __newhlist_h
/* allow list_block ptrs to be used for now */
typedef struct _LIST_BLKREF * P_LIST_BLKREF, ** P_P_LIST_BLKREF;
#endif

typedef struct LISTITEMPOS list_itempos, * list_itemposp;

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
reference to an item position so that it can be unlocked
*/

struct LISTITEMPOS
{
    S32         lock;           /* whether item has a lock on the pool */
    P_LIST_BLKREF lbr;            /* list it belongs to */
    ARRAY_HANDLE     poolh;          /* handle of pool which is locked */
    LIST_ITEMNO item;           /* item number */
    LINK_TYPE   itemc;          /* offset of item in pool */
};

/*
structure of entry in pool block array
*/

struct POOLDESC
{
    ARRAY_HANDLE     poolh;
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

    ARRAY_HANDLE pooldblkh;          /* handle of block of pool descriptors */
    pooldp pooldblkp;           /* pointer to block of pool descriptors */
    S32 pooldix;                /* number of current pool descriptor */
    S32 pooldblksize;           /* size of block of pool descriptors */
    S32 pooldblkfree;           /* first free in block of pool descriptors */

    LIST_ITEMNO numitem;        /* number of items in this list */
    OFF_TYPE maxitemsize;       /* maximum size of an item */
    OFF_TYPE maxpoolsize;       /* maximum size of a pool */
    OFF_TYPE poolsizeinc;       /* pool size increment */

    P_LIST_BLOCK nextblock;           /* link to next list block */
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

/* full-event clients */
typedef S32 (* list_full_event_proctp) (
    S32 bytes);

#define list_full_event_proto(_e_s, _p_proc_list_full_event) \
_e_s S32 _p_proc_list_full_event( \
    S32 bytes)

/*
flags definition
*/

#define LIST_FILL            0x01             /* filler item */
#define LIST_APPLICATION_BIT 0x80             /* reserved for application use */

/*
function declarations
*/

extern ARRAY_HANDLE
list_allochandle(
    S32 bytes);

extern P_ANY
list_allocptr(
    S32 bytes);

extern P_ANY
list_allocptr_p1(
    S32 bytes);

extern P_LIST_ITEM
list_createitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    S32 size,
    S32 fill);

extern void
list_deallochandle(
    ARRAY_HANDLE mh);

extern void
list_deallocptr(
    P_ANY mempnt);

extern void
list_deleteitems(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item,
    LIST_ITEMNO numdel);

extern void
list_deregister(
    P_LIST_BLOCK lp);

extern void
list_disposehandle(
    P_ARRAY_HANDLE mhp);

static __forceinline void
list_disposeptr(
    _Inout_opt_ P_P_ANY p_p_any)
{
    if(NULL != p_p_any)
    {
        P_ANY p_any = *p_p_any;

        if(NULL != p_any)
        {
            *p_p_any = NULL;
            list_deallocptr(p_any);
        }
    }
}

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

extern P_ANY
list_getptr(
    ARRAY_HANDLE hand);

extern S32
list_howmuchpoolspace(void);

extern P_LIST_ITEM
list_gotoitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item);

extern P_LIST_ITEM
list_igotoitem(
    P_LIST_BLOCK lp,
    LIST_ITEMNO item);

extern void
list_init(
    P_LIST_BLOCK lp,
    S32 maxitemsize,
    S32 maxpoolsize);

extern P_LIST_ITEM
list_initseq(
    P_LIST_BLOCK lp,
    P_LIST_ITEMNO itemnop);

extern S32
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

extern ARRAY_HANDLE
list_reallochandle(
    ARRAY_HANDLE hand,
    S32 bytes);

extern P_ANY
list_reallocptr(
    P_ANY mempnt,
    S32 bytes);

extern void
list_register(
    P_LIST_BLOCK lp);

extern S32
list_register_full_event_client(
    list_full_event_proctp proc);

extern S32
list_unlockpools(void);

/*
functions as macros
*/

#define list_atitem(lp)                ((lp) ? (lp)->item : 0)
#define list_itemcontents(it)          ((P_ANY) ((it)->i.inside))
#define list_itemunalignedcontents(it) ((P_ANY) (((U32) (it)->i.inside) - LIST_ALIGNOVH))
#define list_leapnext(it)              (((it)->flags & LIST_FILL) \
                                                ? (it)->i.itemfill   \
                                                : 1)
#define list_numitem(lp)               ((lp) ? (lp)->numitem : 0)

#ifndef __newhlist_h

/* fake newhlist calls until real implementation comes in */

extern S32
nlist_allocblkref(
    P_P_LIST_BLKREF lbrp /*out*/);

extern LIST_ITEMNO
nlist_atitem(
    P_LIST_BLKREF lbr);

extern S32
nlist_createitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_P_LIST_ITEM itp,
    S32 size,
    S32 fill);

extern void
nlist_deleteitems(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    LIST_ITEMNO numdel);

extern void
nlist_disposeblkref(
    /*inout*/ P_P_LIST_BLKREF lbrp);

extern S32
nlist_ensureitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item);

extern S32
nlist_entsize(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item);

extern void
nlist_free(
    P_LIST_BLKREF lbr);

extern S32
nlist_garbagecollect(
    P_LIST_BLKREF lbr);

extern S32
nlist_gotoitem(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_P_LIST_ITEM itp,
    /*out*/ list_itemposp itposp);

extern S32
nlist_gotoitemcontents(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    /*out*/ P_ANY * mpp,
    /*out*/ list_itemposp itposp);

extern void
nlist_init(
    P_LIST_BLKREF lbr,
    S32 maxitemsize,
    S32 maxpoolsize);

extern S32
nlist_initseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*out*/ list_itemposp itposp);

extern S32
nlist_insertitems(
    P_LIST_BLKREF lbr,
    LIST_ITEMNO item,
    LIST_ITEMNO numins);

extern S32
nlist_nextseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*inout*/ list_itemposp itposp);

extern LIST_ITEMNO
nlist_numitem(
    P_LIST_BLKREF lbr);

extern S32
nlist_packlist(
    P_LIST_BLKREF lbr,
    S32 needed);

extern S32
nlist_prevseq(
    P_LIST_BLKREF lbr,
    /*inout*/ P_LIST_ITEMNO itemnop,
    /*out*/ P_P_LIST_ITEM itp,
    /*inout*/ list_itemposp itposp);

#endif /* __newhlist_h */

#endif /* __handlist_h */

/* end of handlist.h */
