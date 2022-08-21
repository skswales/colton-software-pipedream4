/* alloc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Allocation in an extensible flex block for RISC OS */

/* Stuart K. Swales 23-Aug-1989 */

#include "common/gflags.h"

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

#include "myassert.h"

#if RISCOS

/*
set consistent set of flags for alloc.h
*/

#if defined(VALIDATE_MAIN_ALLOCS) || defined(VALIDATE_FIXED_ALLOCS)
#if !defined(CHECK_ALLOCS)
#define  CHECK_ALLOCS
#endif
#endif

#if defined(TRACE_MAIN_ALLOCS) || defined(TRACE_FIXED_ALLOCS)
#if !defined(CHECK_ALLOCS)
#define  CHECK_ALLOCS
#endif
#if !defined(TRACE_ALLOCS)
#define  TRACE_ALLOCS
#endif
#endif

#if defined(CHECK_ALLOCS) || defined(TRACE_ALLOCS)
#undef   TRACE_ALLOWED
#define  TRACE_ALLOWED 1
#endif

/*
internal definitions
*/

#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_SWIS 1

#include "kernel.h" /*C:*/
#include "swis.h" /*C:*/

#include "os.h"

#ifndef __cs_flex_h
#include "cs-flex.h"
#endif

#define alloc_heap_fixed_size               0x00000001
#define alloc_heap_dont_compact             0x00000002
#define alloc_heap_compact_disabled         0x00000004

#define alloc_trace_off                     0x00000000
#define alloc_trace_on                      0x00000010

#define alloc_validate_off                  0x00000000
#define alloc_validate_on                   0x00000020

#define alloc_validate_disabled             0x00000000
#define alloc_validate_enabled              0x00000040

#define alloc_validate_heap_blocks          0x00000080

#define alloc_validate_heap_before_free     0x00010000 /* dispose, free */
#define alloc_validate_heap_before_alloc    0x00020000 /* calloc, malloc, realloc */
#define alloc_validate_heap_on_size         0x00040000 /* size */
#define alloc_validate_heap_after_free      0x00080000 /* dispose, free */
#define alloc_validate_heap_after_alloc     0x00100000 /* calloc, malloc */
#define alloc_validate_heap_after_realloc   0x00200000 /* realloc */

#define alloc_validate_block_before_free    0x01000000 /* dispose, free */
#define alloc_validate_block_before_realloc 0x02000000 /* realloc */
#define alloc_validate_block_on_size        0x04000000 /* size */
#define alloc_validate_block_after_alloc    0x08000000 /* calloc, malloc */
#define alloc_validate_block_after_realloc  0x10000000 /* realloc */

#define ALLOC_HEAP_FLAGS int

typedef struct _ALLOC_HEAP_DESC
{
    struct _RISCOS_HEAP * heap;
    U32                   minsize;
    U32                   increment;
    ALLOC_HEAP_FLAGS      flags;
}
ALLOC_HEAP_DESC, * P_ALLOC_HEAP_DESC;

/* Interface to RISC OS Heap Manager */

typedef enum _RISCOS_HEAPREASONCODES
{
    HeapReason_Init          = 0,
    HeapReason_Desc          = 1,
    HeapReason_Get           = 2,
    HeapReason_Free          = 3,
    HeapReason_ExtendBlock   = 4,
    HeapReason_ExtendHeap    = 5,
    HeapReason_ReadBlockSize = 6
}
RISCOS_HEAPREASONCODES;

typedef struct _RISCOS_HEAP
{
    U32 magic;  /* ID word */
    U32 free;   /* offset to first block on free list ***from this location*** */
    U32 hwm;    /* offset to first free location */
    U32 size;   /* size of heap, including header */

                /* rest of heap follows here ... */
}
RISCOS_HEAP, * P_RISCOS_HEAP;

typedef struct _RISCOS_USED_BLOCK
{
    U32 size;   /* rounded size of used block */

                /* data follows here ... */
}
RISCOS_USED_BLOCK, * P_RISCOS_USED_BLOCK;

/*
pointer to the block in which this object is allocated
*/
#define blockhdrp(core) ( \
    ((P_RISCOS_USED_BLOCK) (core)) - 1 )

/*
amount of core allocated to this object
*/
#define blocksize(core) ( \
    blockhdrp(core)->size - sizeof32(RISCOS_USED_BLOCK) )

typedef struct _RISCOS_FREE_BLOCK
{
    U32 free;   /* offset to next block on free list ***from this location*** */
    U32 size;   /* size of free block */

                /* free space follows here ... */
}
RISCOS_FREE_BLOCK, * P_RISCOS_FREE_BLOCK;

/*
RISC OS only maintains size field on used blocks
*/
#define HEAPMGR_OVERHEAD    sizeof32(RISCOS_USED_BLOCK)

/* Round to integral number of RISC OS heap manager granules
 * This size is given by the need to fit a freeblock header into
 * any space that might be freed or fragmented on allocation.
*/
#define round_heapmgr(n) ( \
    ((n) + (sizeof32(RISCOS_FREE_BLOCK)-1)) & ~(sizeof32(RISCOS_FREE_BLOCK)-1) )

#if defined(CHECK_ALLOCS)
#define FILL_BYTE 0x55
#define FILL_WORD ( \
    (((((FILL_BYTE << 8) + FILL_BYTE) << 8) + FILL_BYTE) << 8) + FILL_BYTE )
#if !defined(startguardsize)
#define startguardsize 0x10
#endif
#if !defined(endguardsize)
#define endguardsize   0x10
#endif
#else
#define startguardsize 0
#define endguardsize   0
#endif

#if defined(TRACE_ALLOCS)
#define alloc_trace_on_do(ahp)  { if((ahp)->flags & alloc_trace_on) trace_on();  }
#define alloc_trace_off_do(ahp) { if((ahp)->flags & alloc_trace_on) trace_off(); }
#else
#define alloc_trace_on_do(ahp)
#define alloc_trace_off_do(ahp)
#endif

#if !defined(ALLOC_NOISE_THRESHOLD)
#define  ALLOC_NOISE_THRESHOLD 1024
#endif

/*
internal functions
*/

static P_ANY
alloc__calloc(
    _InVal_     U32 num,
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp);

static void
alloc__dispose(
    P_P_ANY p_usrcore,
    P_ALLOC_HEAP_DESC ahp);

static void
alloc__free(
    P_ANY usrcore,
    P_ALLOC_HEAP_DESC ahp);

static P_ANY
alloc__malloc(
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp);

static P_ANY
alloc__realloc(
    P_ANY usrcore,
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp);

static U32
alloc__size(
    P_ANY usrcore,
    P_ALLOC_HEAP_DESC ahp);

static void
alloc__validate(
    P_ANY usrcore,
    const char * msg,
    P_ALLOC_HEAP_DESC ahp);

#if defined(FULL_ANSI)

static void
alloc__ini_dispose(
    P_P_ANY ap);

static U32
alloc__ini_size(
    P_ANY a);

static void
alloc__ini_validate(
    P_ANY a,
    const char * msg);

#endif

static P_ANY
alloc__main_calloc(
    _InVal_     U32 num,
    _InVal_     U32 size);

static void
alloc__main_dispose(
    P_P_ANY ap);

static void
alloc__main_free(
    P_ANY a);

static P_ANY
alloc__main_malloc(
    _InVal_     U32 size);

static P_ANY
alloc__main_realloc(
    P_ANY a,
    _InVal_     U32 size);

static U32
alloc__main_size(
    P_ANY a);

static void
alloc__main_validate(
    P_ANY a,
    const char * msg);

#if defined(REDIRECT_RISCOS_KERNEL_ALLOCS)

static void
alloc__riscos_kernel_free(
    void * a);

static void *
alloc__riscos_kernel_malloc(
    _In_        size_t size);

#endif

#if TRACE_ALLOWED

static void
alloc_validate_heap(
    P_ALLOC_HEAP_DESC ahp,
    const char * routine,
    _In_        int set_guards);

#endif

static P_ANY
riscos_ptr_alloc(
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp);

static void
riscos_ptr_free(
    RISCOS_USED_BLOCK * p_used,
    P_ALLOC_HEAP_DESC ahp);

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp);

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp);

#define P_HEAP_HWM(p_heap) ( \
    (P_U8) p_heap + p_heap->hwm ) /* P_U8 */

typedef union _P_RISCOS_HEAP_DATA
{
    P_U8 c;
    P_RISCOS_FREE_BLOCK f;
    P_RISCOS_USED_BLOCK u;
    P_ANY v;
}
P_RISCOS_HEAP_DATA;

typedef union _P_RISCOS_HEAP_FREE_DATA
{
    P_U8 c;
    P_RISCOS_FREE_BLOCK f;
    P_ANY v;
}
P_RISCOS_HEAP_FREE_DATA;

#define P_END_OF_FREE(p_free_block) ( \
    p_free_block.c + p_free_block.f->size ) /* P_U8 */

#define P_NEXT_FREE(p_free_block) ( \
    p_free_block.c + p_free_block.f->free ) /* P_U8 */

typedef union _P_RISCOS_HEAP_USED_DATA
{
    P_U8 c;
    P_RISCOS_USED_BLOCK u;
    P_ANY v;
}
P_RISCOS_HEAP_USED_DATA;

#define P_END_OF_USED(p_used_block) ( \
    p_used_block.c + p_used_block.u->size ) /* P_U8 */

#if defined(ALLOC_CLEAR_FREE)
#define CLEAR_FREE(p_free_block) \
    void_memset32(p_free_block.f + 1, 'x', p_free_block.f->size - sizeof32(*p_free_block.f));
#else
#define CLEAR_FREE(p_free_block)
#endif

struct _FLEX_USED_BLOCK
{
    void * anchor; U32 b;
};

#define RHM_SIZEOF_FLEX_USED_BLOCK \
    round_heapmgr(sizeof32(struct _FLEX_USED_BLOCK))

/* ----------------------------------------------------------------------- */

U32
g_dynamic_area_limit = 0;

static int
alloc_dynamic_area_handle = 0;

_Check_return_
extern int
alloc_dynamic_area_query(void)
{
    return(alloc_dynamic_area_handle);
}

/*
the main alloc heap
*/

static ALLOC_HEAP_DESC
alloc_main_heap_desc =
{
    NULL,    /* heap */
    0,       /* minsize */
    0x8000,  /* increment (32KB) */
#if defined(TRACE_MAIN_ALLOCS)
    alloc_trace_on |
#endif
#if defined(ALLOC_RELEASE_STORE_INFREQUENTLY)
    alloc_heap_compact_disabled |
#endif
#if defined(VALIDATE_MAIN_ALLOCS)
    alloc_validate_on |
    /*alloc_validate_heap_before_alloc |*/
    /*alloc_validate_heap_before_free |*/
    /*alloc_validate_heap_on_size |*/
    alloc_validate_block_before_realloc |
    alloc_validate_block_before_free |
    alloc_validate_block_on_size |
    alloc_validate_heap_blocks |
#endif
    0 /* flags */
};

U32
alloc_main_heap_minsize = 0x8000U - RHM_SIZEOF_FLEX_USED_BLOCK; /* (32KB minus flex block overhead) */

/*
the main heap function set
*/
#if defined(FULL_ANSI)
ALLOC_FUNCTION_SET
alloc_main =
{
    calloc,
    alloc__ini_dispose,
    free,
    malloc,
    realloc,
    alloc__ini_size,
    alloc__ini_validate
};
#else
ALLOC_FUNCTION_SET
alloc_main;
#endif

/*
the function set which alloc_main is redirected to on a main heap success
*/

static const ALLOC_FUNCTION_SET
alloc_main_redirected =
{
    alloc__main_calloc,
    alloc__main_dispose,
    alloc__main_free,
    alloc__main_malloc,
    alloc__main_realloc,
    alloc__main_size,
    alloc__main_validate
};

#if TRACE_ALLOWED && defined(USE_HEAP_SWIS)
static _kernel_oserror *
alloc_winge(
    _kernel_oserror * e,
    const char * routine)
{
    myassert2x(e == NULL, TEXT("alloc__%s error: %s"), routine, e->errmess);
    return(e);
}
#else
#define alloc_winge(e, r) (e)
#endif

static BOOL
alloc_initialise_heap(
    P_ALLOC_HEAP_DESC ahp)
{
#if defined(USE_HEAP_SWIS)
    _kernel_swi_regs rs;
#endif
    BOOL res;
    U32 new_size;
    P_RISCOS_HEAP heap;

    /* ensure minsize a multiple of heap granularity - it must already have space for the RISCOS_HEAP */
    ahp->minsize = round_heapmgr(ahp->minsize);
 reportf("ahp->minsize := %d", ahp->minsize);

    new_size = ahp->minsize;

    if(ahp->increment)
    {
        new_size += RHM_SIZEOF_FLEX_USED_BLOCK;
        new_size  = div_round_ceil_u(new_size, ahp->increment) * ahp->increment;
        new_size -= RHM_SIZEOF_FLEX_USED_BLOCK;
    }

    /* must be first fixed block at start of flex, or second block if the first is a fixed heap */
    if(!flex_alloc((flex_ptr) &ahp->heap, new_size))
        return(FALSE);

    /* once alloc is running in a fixed block at start of flex, we must stop the C runtime from moving us */
    /* NB if flex is running in a dynamic area, the C runtime is free to grow in any case */
    flex_set_budge(0);

    heap = ahp->heap;

    trace_4(TRACE_MODULE_ALLOC, TEXT("alloc_initialise_heap got heap at &%p->&%p, size %u,&%4.4x"), report_ptr_cast(ahp), report_ptr_cast(heap), new_size, new_size);

#if defined(USE_HEAP_SWIS)
    rs.r[0] = HeapReason_Init;
    rs.r[1] = (int) heap;
    /* no r2 */
    rs.r[3] = new_size;
    res = (NULL == alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "initialise_heap"));
#else
    /* do the job by hand */
    heap->magic = 0x70616548; /* ID word ("Heap") */
    heap->free = 0;
    heap->hwm = sizeof32(*heap);
    heap->size = new_size;

    res = TRUE;
#endif /* USE_HEAP_SWIS */

#if TRACE_ALLOWED && (PERSONAL_TRACE_FLAGS & TRACE_MODULE_ALLOC) && defined(TEST_REALLOC_RARE_PROCESSES)
    { /* perform some self-tests, especially on realloc processes 3,4,7 */
    U32 xs = sizeof32(RISCOS_USED_BLOCK)+startguardsize+endguardsize;
    P_ANY a = alloc__malloc(32-xs, ahp);
    P_ANY b = alloc__malloc(32-xs, ahp);
    P_ANY c = alloc__malloc(32-xs, ahp);
    P_ANY d = alloc__malloc(32-xs, ahp);
    P_ANY e = alloc__malloc(32-xs, ahp);
    P_ANY f = alloc__malloc(32-xs, ahp);
    P_ANY g = alloc__malloc(32-xs, ahp);
    U32 bs = heap->size-heap->hwm;
    P_ANY h = alloc__malloc(bs-xs, ahp);
    assert(heap->free == 0);
    assert(heap->size-heap->hwm == 0);

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 7"));
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    alloc__free(e, ahp);
    d = alloc__realloc(d, 3*32-xs, ahp); /* test realloc process 7 */

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 3"));
    alloc__free(g, ahp);
    h = alloc__realloc(h, bs-xs+32, ahp); /* test realloc process 3 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(f, ahp);
    alloc__free(h, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 8"));
    a = alloc__malloc(32-xs, ahp);
    b = alloc__malloc(32-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    d = alloc__malloc(32-xs, ahp);
    e = alloc__malloc(32-xs, ahp);
    f = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    alloc__free(e, ahp);
    d = alloc__realloc(d, 3*32-16-xs, ahp); /* test realloc process 8 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(f, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 9"));
    a = alloc__malloc(32-xs, ahp);
    b = alloc__malloc(32-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    d = alloc__malloc(32-xs, ahp);
    e = alloc__malloc(32-xs, ahp);
    f = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    d = alloc__realloc(d, 2*32-xs, ahp); /* test realloc process 9 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(e, ahp);
    alloc__free(f, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    bs = heap->size-heap->hwm - 64 - 32;
    a = alloc__malloc(64-xs, ahp);
    b = alloc__malloc(bs-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 14"));
    c = alloc__realloc(c, 64-xs, ahp); /* test realloc process 14 */
    alloc__free(b, ahp);
    alloc__free(c, ahp);

    a = alloc__malloc(64-xs, ahp);
    b = alloc__malloc(bs-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 4"));
    c = alloc__realloc(c, 32+4-xs, ahp); /* test realloc process 4 */
    alloc__free(b, ahp);
    alloc__free(c, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));
    } /*block*/
#endif

    return(res);
}

/******************************************************************************
*
* ensure that a block of given size can be allocated,
* the heap being extended as necessary
*
******************************************************************************/

#pragma no_check_stack

static int
alloc_needtoallocate(
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    U32 current_hwm, current_size;
    U32 spare, need, delta;

    current_hwm  = heap->hwm;
    current_size = heap->size;
    spare        = current_size - current_hwm;
    need         = round_heapmgr(size + HEAPMGR_OVERHEAD);

    trace_6(TRACE_MODULE_ALLOC,
            TEXT("alloc_needtoallocate(%u,&%4.4x): current_hwm = &%x, current_size = &%x, spare = %u,&%4.4x"),
            size, size, heap->hwm, heap->size, spare, spare);

    assert((int) need > 0);
    if(need <= spare)
        return(TRUE);

    /* must not wiggle fixed size heaps */
    if(ahp->flags & alloc_heap_fixed_size)
        return(FALSE);

    delta = need - spare;
    if(ahp->increment)
    {
    if(delta <= ahp->increment)
        delta = ahp->increment;
    else
    {
        delta = ahp->increment * (1U + (delta / ahp->increment));
        /*reportf("*** allocating big block - need=%d, spare=%d, delta=%d", need, spare, delta);*/
    }
    }

    trace_on();
    trace_4(TRACE_MODULE_ALLOC| TRACE_APP_MEMORY_USE, TEXT("extending heap (&%p->&%p) by %u,&%4.4x"), report_ptr_cast(ahp), report_ptr_cast(heap), delta, delta);
    /*alloc_validate_heap(ahp, "pre heap extension", 0);*/
    if(!flex_realloc((flex_ptr) &ahp->heap, current_size + delta))
    {
        trace_0(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("*** heap extension failed - return FALSE"));
        trace_off();
        return(FALSE);
    }

    heap->size = flex_size((flex_ptr) &ahp->heap);

    trace_4(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("heap (&%p->&%p) now size %u,&%4.4x"), report_ptr_cast(ahp), report_ptr_cast(heap), heap->size, heap->size);
    trace_off();
    return(TRUE);
}

#pragma check_stack

extern STATUS
alloc_ensure_froth(
    U32 froth_size)
{
    assert(froth_size >= 0x1000);

    froth_size -= RHM_SIZEOF_FLEX_USED_BLOCK;

    if(!alloc_needtoallocate(froth_size, &alloc_main_heap_desc))
        return(status_nomem());

    return(STATUS_OK);
}

/******************************************************************************
*
* release the free store at the top of the
* heap and flex area back to the free pool
*
******************************************************************************/

static void
alloc_freeextrastore(
    P_ALLOC_HEAP_DESC ahp)
{
    U32 current_hwm, current_size;
    U32 spare;

    /* must not wiggle fixed heaps */
    if(ahp->flags & alloc_heap_fixed_size)
        return;

    current_hwm = ahp->heap->hwm;

    /* don't let heap size fall too low */
    if(current_hwm <= ahp->minsize)
        current_size = ahp->minsize;
    else
        current_size = ahp->heap->size;

    spare = current_size - current_hwm;

    if((int) spare + flex_storefree() >= flex_granularity)
        {
        trace_0(TRACE_MODULE_ALLOC, "alloc_freeextrastore: contracting heap to free some space");

        if(!flex_realloc((flex_ptr) &ahp->heap, current_hwm))
            {
            trace_0(TRACE_MODULE_ALLOC, "*** heap contraction failed");
            return;
            }

        ahp->heap->size = flex_size((flex_ptr) &ahp->heap);

        trace_4(TRACE_MODULE_ALLOC,
                "heap (&%p->&%p) now contracted to size %u,&%4.4x\n",
                report_ptr_cast(ahp), report_ptr_cast(ahp->heap), ahp->heap->size, ahp->heap->size);
        }
}

/******************************************************************************
*
*  initialise allocators
*
******************************************************************************/

#if defined(USE_BOUND_LIBRARY)

extern void
__heap_checking_on_all_allocates(BOOL);

extern void
__heap_checking_on_all_deallocates(BOOL);

#endif /* USE_BOUND_LIBRARY */

_Check_return_
extern STATUS
alloc_init(void)
{
    STATUS status = STATUS_NOMEM;

#if defined(TRACE_ALLOCS)
    trace_on();
#endif

    trace_0(TRACE_MODULE_ALLOC, "alloc_init()");

#if defined(USE_BOUND_LIBRARY)
    __heap_checking_on_all_allocates(TRUE);
    __heap_checking_on_all_deallocates(TRUE);
#endif

    for(;;) /* loop for structure */
        {
        reportf("g_dynamic_area_limit: %d", g_dynamic_area_limit);
        if((alloc_dynamic_area_handle = flex_init(de_const_cast(char *, g_dynamic_area_name), 0, g_dynamic_area_limit)) < 0)
            break;

        flex_set_deferred_compaction(TRUE);

        if(alloc_main_heap_minsize)
            {
            alloc_main_heap_desc.minsize = alloc_main_heap_minsize;

            if(!alloc_initialise_heap(&alloc_main_heap_desc))
                break;

            /* redirect alloc main functions */
            alloc_main = alloc_main_redirected;

            #if defined(REDIRECT_KERNEL_ALLOCS)
            _kernel_register_allocs(alloc__kernel_malloc, alloc__kernel_free);
            #endif
            }

        status = STATUS_OK;

        break; /* always - loop only for structure */
        }

#if defined(TRACE_ALLOCS)
    trace_off();
#endif

    return(status);
}

/******************************************************************************
*
* release surplus memory
*
******************************************************************************/

extern void
alloc_tidy_up(void)
{
    alloc_freeextrastore(&alloc_main_heap_desc);
}

/******************************************************************************
*
* TRACE_ALLOWED functions
*
******************************************************************************/

#if TRACE_ALLOWED

extern void
alloc_traversefree(
    int which)
{
    P_ALLOC_HEAP_DESC ahp;

    IGNOREPARM(which);

    ahp = &alloc_main_heap_desc;

    alloc_validate_heap(ahp, "traversefree", 0);
}

#endif

/******************************************************************************
*
* heap validation functions
*
******************************************************************************/

#if !TRACE_ALLOWED
#else
static void
alloc_validate_block(
    P_ALLOC_HEAP_DESC ahp,
    P_ANY usrcore,
    const char * routine,
    _In_        int set_guards);
#endif

static void
alloc_validate_heap(
    P_ALLOC_HEAP_DESC ahp,
    const char * routine,
    _In_        int set_guards)
{
#if !TRACE_ALLOWED
    IGNOREPARM(ahp);
    IGNOREPARM(routine);
    IGNOREPARM(set_guards);
#else
    union
    {
        const char *              c;
        const RISCOS_FREE_BLOCK * f;
        const RISCOS_USED_BLOCK * u;
        const void *              v;
    } p;
    const char * freep;
    const char * hwmp;
    const char * endp;
    U32 current_hwm, current_size;
    const char * usrcore;
    U32 syssize, usrsize, offset;

    /* validate heap */

    p.v          = ahp->heap + 1;
    offset       = ahp->heap->free;
    freep        = offset ? ((char *) &ahp->heap->free + offset) : NULL;

    current_hwm  = ahp->heap->hwm;
    hwmp         = (char *) ahp->heap + current_hwm;

    current_size = ahp->heap->size;
    endp         = (char *) ahp->heap + current_size;

    trace_6(TRACE_MODULE_ALLOC,
            " heap (&%p->&%p) size &%4.4x,&%p hwm &%4.4x,&%p\n*** free/used blocks ***:\n",
            report_ptr_cast(ahp), report_ptr_cast(ahp->heap), current_size, report_ptr_cast(endp), current_hwm, report_ptr_cast(hwmp));

    myassert5x(current_hwm < INT_MAX, "alloc__%s: heap (&%p->&%p) has corrupt hwm %u,&%4.4x",
              routine, ahp, ahp->heap,
              current_hwm, current_hwm);

    myassert5x(current_size < INT_MAX, "alloc__%s: heap (&%p->&%p) has corrupt size %u,&%4.4x",
               routine, ahp, ahp->heap,
               current_size, current_size);

    myassert7x(current_hwm <= current_size, "alloc__%s: heap (&%p->&%p) has corrupt hwm %u,&%4.4x > size %u,&%4.4x",
               routine, ahp, ahp->heap,
               current_hwm, current_hwm,
               current_size, current_size);

    if(offset)
        myassert7x((offset <= current_hwm)  &&  (offset >= sizeof32(RISCOS_HEAP) - offsetof32(RISCOS_HEAP, size)),
                   "alloc__%s: heap (&%p->&%p) has corrupt initial free link %u,&%4.4x (hwm %u,&%4.4x)",
                   routine, ahp, ahp->heap,
                   offset, offset,
                   current_hwm, current_hwm);

    do  {
        if(p.c == hwmp)
            {
            trace_3(TRACE_MODULE_ALLOC,
                    "  (last free block &%p,%u,&%4.4x)\n",
                    p.c, endp - hwmp, endp - hwmp);

            p.c = endp;
            }
        else if(p.c == freep)
            {
            offset  = p.f->free;
            syssize = p.f->size;

            trace_3(TRACE_MODULE_ALLOC,
                    "  (free &%p,%u,&%4.4x)\n",
                    report_ptr_cast(p.f), syssize, syssize);

            if(offset)
                freep += offset;
            else
                freep = NULL;

            if(offset)
                myassert6x(((offset & 3) == 0)  &&  (freep >= p.c)  &&  (freep < hwmp),
                           "alloc__%s: heap (&%p->&%p) free block &%p has corrupt offset %u,&%4.4x",
                           routine, ahp, ahp->heap,
                           p.c, offset, offset);

            myassert6x(((syssize & 3) == 0)  &&  (p.c + syssize > p.c)  &&  (p.c + syssize <= hwmp),
                       "alloc__%s: heap (&%p->&%p) free block &%p has corrupt size %u,&%4.4x",
                       routine, ahp, ahp->heap,
                       p.c, syssize, syssize);

            p.c += syssize;
            }
        else
            {
            syssize = p.u->size;

            usrsize = syssize - sizeof32(*p.u) - (startguardsize + endguardsize);

            usrcore = (char *) (p.u + 1) + startguardsize;

            trace_6(TRACE_MODULE_ALLOC,
                    "  (used &%p &%p size %u,&%4.4x %u,&%4.4x)\n",
                    report_ptr_cast(p.u), report_ptr_cast(usrcore), syssize, syssize, usrsize, usrsize);

            myassert9x(((syssize & 3) == 0)  ||  (p.c + syssize > p.c)  &&  (p.c + syssize <= hwmp),
                       "alloc__%s: heap (&%p->&%p) used block &%p &%p has corrupt size %u,&%4.4x %u,&%4.4x",
                       routine, ahp, ahp->heap,
                       p.c, usrcore,
                       syssize, syssize,
                       usrsize, usrsize);

            if(ahp->flags & alloc_validate_heap_blocks)
                alloc_validate_block(ahp, de_const_cast(P_ANY, usrcore), routine, set_guards);

            p.c += syssize;
            }
        }
    while(p.c != endp);

    trace_0(TRACE_MODULE_ALLOC, "  -- heap validated");

#endif /* TRACE_ALLOWED */
}

static void
alloc_validate_block(
    P_ALLOC_HEAP_DESC ahp,
    P_ANY usrcore,
    const char * routine,
    _In_        int set_guards)
{
#if !TRACE_ALLOWED
    IGNOREPARM(ahp);
    IGNOREPARM(usrcore);
    IGNOREPARM(routine);
    IGNOREPARM(set_guards);
#else
    const char *              core;
    const RISCOS_USED_BLOCK * syscore;
    U32                       syssize, size, usrsize;
    int                       valid_size = 1;

    core    = usrcore;
    core   -= startguardsize;
    syscore = blockhdrp(core);

    syssize = syscore->size;
    size    = syssize - sizeof32(*syscore);
    usrsize = size - (startguardsize + endguardsize);

    if(!(syssize  &&  ((syssize & 3) == 0)  &&  (syssize < INT_MAX)))
        {
        myassert9x(0, "alloc__%s(&%p->&%p): object &%p &%p has corrupt size %u,&%4.4x %u,&%4.4x (can't check endguard)",
                    routine, ahp, !ahp ? NULL : ahp->heap,
                    syscore, usrcore,
                    syssize, syssize,
                    usrsize, usrsize);

        valid_size = 0;
        }

#if defined(CHECK_ALLOCS)
    {
    PC_S32 end32p;
    PC_S32 ptr32p;
    PC_S32 start32p;
    const char * end;
    const char * ptr;
    const char * start;

    end32p   = (P_S32 ) (core + startguardsize); /* give offsets relative to start of guts */
    ptr32p   = end32p;
    start32p = (P_S32 ) core;

    while(ptr32p > start32p)
        {
        --ptr32p;

        if(myassert(*ptr32p == FILL_WORD))
            myasserted("alloc__%s(&%p->&%p): object &%p &%p, size %u,&%4.4x %u,&%4.4x, fault at startguard offset %u,&%4.4x: &%8.8X != FILL_WORD &%8.8X",
                        routine, ahp, !ahp ? NULL : ahp->heap,
                        syscore, usrcore,
                        syssize, syssize,
                        usrsize, usrsize,
                        (end32p - ptr32p) * sizeof32(*ptr32p),
                        (end32p - ptr32p) * sizeof32(*ptr32p),
                        (int) *ptr32p, FILL_WORD);
        }

    if(valid_size)
        {
        end   = core + size;
        ptr   = end - endguardsize;
        start = ptr; /* give offsets relative to start of endguard */

        while(ptr < end)
            {
            if(myassert(*ptr == FILL_BYTE))
                myasserted("alloc__%s(&%p->&%p): object &%p &%p, size %u,&%4.4x %u,&%4.4x, fault at endguard offset %u,&%4.4x: &%2.2X != FILL_BYTE &%2.2X",
                            routine, ahp, !ahp ? NULL : ahp->heap,
                            syscore, usrcore,
                            syssize, syssize,
                            usrsize, usrsize,
                            (ptr - start) * sizeof32(*ptr),
                            (ptr - start) * sizeof32(*ptr),
                            *ptr, FILL_BYTE);

            ++ptr;
            }
        }
    }
#endif /* CHECK_ALLOCS */

#endif /* TRACE_ALLOWED */
}

#if defined(FULL_ANSI)

static void
alloc__ini_dispose(
    void ** ap)
{
    void * a;

    if(ap)
        {
        a = *ap;

        if(a)
            {
            *ap = NULL;

            free(a);
            }
        }
}

static U32
alloc__ini_size(
    const void * a)
{
    IGNOREPARM(a);
    myassert1x(a == (const void *) 1, "unable to yield size for block &%p", a);
    return(0);
}

static void
alloc__ini_validate(
    P_ANY a,
    const char * msg)
{
    IGNOREPARM(a);
    IGNOREPARM(msg);
}

#endif /* FULL_ANSI */

/******************************************************************************
*
* allocates space for an array of nmemb objects, each of whose size is
* 'size'. The space is initialised to all bits zero.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

static void *
alloc__main_calloc(
    _InVal_     U32 num,
    _InVal_     U32 size)
{
    return(alloc__calloc(num, size, &alloc_main_heap_desc));
}

static void
alloc__main_dispose(
    void ** ap)
{
    alloc__dispose(ap, &alloc_main_heap_desc);
}

/******************************************************************************
*
* causes the space pointed to by ptr to be deallocated (i.e., made
* available for further allocation). If ptr is a null pointer, no action
* occurs. Otherwise, if ptr does not match a pointer earlier returned by
* calloc, malloc or realloc or if the space has been deallocated by a call
* to free or realloc, the behaviour is undefined.
*
******************************************************************************/

static void
alloc__main_free(
    void * a)
{
    alloc__free(a, &alloc_main_heap_desc);
}

/******************************************************************************
*
* allocates space for an object whose size is specified by 'size' and whose
* value is indeterminate.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

static void *
alloc__main_malloc(
    _InVal_     U32 size)
{
    return(alloc__malloc(size, &alloc_main_heap_desc));
}

/******************************************************************************
*
* changes the size of the object pointed to by ptr to the size specified by
* size. The contents of the object shall be unchanged up to the lesser of
* the new and old sizes. If the new size is larger, the value of the newly
* allocated portion of the object is indeterminate. If ptr is a null
* pointer, the realloc function behaves like a call to malloc for the
* specified size. Otherwise, if ptr does not match a pointer earlier
* returned by calloc, malloc or realloc, or if the space has been
* deallocated by a call to free or realloc, the behaviour is undefined.
* If the space cannot be allocated, the object pointed to by ptr is
* unchanged. If size is zero and ptr is not a null pointer, the object it
* points to is freed.
*
* Returns: either a null pointer or a pointer to the possibly moved
*          allocated space.
*
******************************************************************************/

static void *
alloc__main_realloc(
    void * a,
    _InVal_     U32 size)
{
    return(alloc__realloc(a, size, &alloc_main_heap_desc));
}

static U32
alloc__main_size(
    void * a)
{
    return(alloc__size(a, &alloc_main_heap_desc));
}

static void
alloc__main_validate(
    void * a,
    const char * msg)
{
    alloc__validate(a, msg, &alloc_main_heap_desc);
}

/******************************************************************************
*
* allocates space for an array of nmemb objects, each of whose size is
* 'size'. The space is initialised to all bits zero.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

static void *
alloc__calloc(
    _InVal_     U32 num,
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp)
{
    U32 nbytes;
    void * a;

    /* not very good - could get overflow in calc */
    nbytes = num * size;
        
    nbytes = (nbytes + (sizeof32(S32) - 1)) & ~(sizeof32(S32) - 1);

    a = alloc__malloc(nbytes, ahp);

    if(a)
        void_memset32(a, 0, nbytes);

    return(a);
}

static void
alloc__dispose(
    void ** ap,
    P_ALLOC_HEAP_DESC ahp)
{
    void * a;

    alloc_trace_on_do(ahp);

#if defined(CHECK_ALLOCS)
    if(!ap || !*ap)
        {
        if(ahp->flags & alloc_validate_enabled)
            if(ahp->flags & alloc_validate_heap_before_free)
                alloc_validate_heap(ahp, "dispose");
        }
#endif

    if(ap)
        {
        a = *ap;

        if(a)
            {
            *ap = NULL;

            alloc__free(a, ahp);
            }
        }

    alloc_trace_off_do(ahp);
}

/******************************************************************************
*
* causes the space pointed to by ptr to be deallocated (i.e., made
* available for further allocation). If ptr is a null pointer, no action
* occurs. Otherwise, if ptr does not match a pointer earlier returned by
* calloc, malloc or realloc or if the space has been deallocated by a call
* to free or realloc, the behaviour is undefined.
*
******************************************************************************/

static void
alloc__free(
    void * usrcore,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP_USED_DATA b;

    alloc_trace_on_do(ahp);

    trace_3(TRACE_MODULE_ALLOC,
            "alloc__free(&%p) (&%p->&%p)\n",
            report_ptr_cast(usrcore), report_ptr_cast(ahp), report_ptr_cast(ahp->heap));

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_before_free)
            alloc_validate_heap(ahp, "free");
#endif

    if(!usrcore)
        {
        alloc_trace_off_do(ahp);
        return;
        }

    {
    uintptr_t heap_start = (uintptr_t) ((ahp->heap) + 1);
    uintptr_t heap_hwm   = ((uintptr_t) ahp->heap) + ahp->heap->hwm;
    uintptr_t core       = (uintptr_t) usrcore;
    if((core < heap_start) || (core >= heap_hwm))
    {
        reportf("alloc__free(0x%8.8X): outwith heap 0x%8.8X..0x%8.8X\n", core, heap_start, heap_hwm);
        alloc_trace_off_do(ahp);
        return;
    }
    } /*block*/

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_block_before_free)
            alloc_validate_block(ahp, a, "free");
#endif

    b.v = usrcore;
    b.c -= startguardsize;

#if !defined(USE_HEAP_SWIS)
    riscos_ptr_free(b.u - 1, ahp);
#else
    {
    _kernel_swi_regs  r;
    _kernel_oserror * e;
    r.r[0] = HeapReason_Free;
    r.r[1] = (int) ahp->heap;
    r.r[2] = (int) b.c;
    e = alloc_winge(_kernel_swi(OS_Heap, &r, &r), "free");
    }
#endif

#if defined(POST_CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_after_free)
            alloc_validate_heap(ahp, "AFTER_free");
#endif

    alloc_freeextrastore(ahp);

    alloc_trace_off_do(ahp);
}

/******************************************************************************
*
* allocates space for an object whose size is specified by 'size' and whose
* value is indeterminate.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

#pragma no_check_stack

static void *
alloc__malloc(
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp)
{
    P_U8 syscore;
    P_U8 usrcore;
    U32 syssize, usrsize;

    alloc_trace_on_do(ahp);

    trace_3(TRACE_MODULE_ALLOC,
            "alloc__malloc(%u, (&%p->&%p)) ",
            size, report_ptr_cast(ahp), report_ptr_cast(ahp->heap));

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_on_alloc)
            alloc_validate_heap(ahp, "malloc");
#endif

    if(!size)
        {
        trace_0(TRACE_MODULE_ALLOC, "yields NULL because zero size");
        alloc_trace_off_do(ahp);
        return(NULL);
        }

    usrsize = size;
    syssize = usrsize + startguardsize + endguardsize;

#if !defined(USE_HEAP_SWIS)
    syscore = riscos_ptr_alloc(syssize, ahp);
#else
    {
    _kernel_swi_regs  r;
    _kernel_oserror * e;
    r.r[0] = HeapReason_Get;
    r.r[1] = (int) ahp->heap;
    /* no r2 */
    r.r[3] = syssize;
    e = _kernel_swi(OS_Heap, &r, &r);

    if(e)
        {
        if(!alloc_needtoallocate(syssize, ahp))
            {
            trace_0(TRACE_MODULE_ALLOC, "yields NULL because heap extension failed");
            alloc_trace_off_do(ahp);
            return(NULL);
            }

        r.r[0] = HeapReason_Get;
        r.r[1] = (int) ahp->heap;
        /* no r2 */
        r.r[3] = syssize;
        e = alloc_winge(_kernel_swi(OS_Heap, &r, &r), "malloc");

#if TRACE_ALLOWED
        if(myassert(!e  &&  r.r[2]))
            myasserted("alloc__malloc(%u) failed unexpectedly after heap (&%p->&%p) extension\n",
                        size, ahp, ahp->heap);
#endif

        if(e)
            {
            trace_0(TRACE_MODULE_ALLOC, "yields NULL because of error");
            alloc_trace_off_do(ahp);
            return(NULL);
            }
        }

    syscore = (char *) r.r[2];
    }
#endif

    usrcore = syscore + startguardsize;

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_on) /* regardless of enable state */
        void_memset32(core, FILL_BYTE, blocksize(core));
#endif

#if defined(POST_CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_on_alloc)
            alloc_validate_heap(ahp, "AFTER_malloc");
#endif

    trace_1(TRACE_MODULE_ALLOC, " yields &%p\n", usrcore);

    alloc_trace_off_do(ahp);

    return(usrcore);
}

#if defined(REDIRECT_KERNEL_ALLOCS)

/* All this turns out to do is to redirect the procs used by
 * the kernel part of the C library away from malloc/free!
 * Useful for stack extension but nothing else.
*/

static void
alloc__kernel_free(
    void * a)
{
    alloc__free(a, &alloc_main_heap_desc);
}

static void *
alloc__kernel_malloc(
    size_t size)
{
    return(alloc__malloc(size, &alloc_main_heap_desc));
}

#endif

#pragma check_stack

/******************************************************************************
*
* changes the size of the object pointed to by ptr to the size specified by
* size. The contents of the object shall be unchanged up to the lesser of
* the new and old sizes. If the new size is larger, the value of the newly
* allocated portion of the object is indeterminate. If ptr is a null
* pointer, the realloc function behaves like a call to malloc for the
* specified size. Otherwise, if ptr does not match a pointer earlier
* returned by calloc, malloc or realloc, or if the space has been
* deallocated by a call to free or realloc, the behaviour is undefined.
* If the space cannot be allocated, the object pointed to by ptr is
* unchanged. If size is zero and ptr is not a null pointer, the object it
* points to is freed.
*
* Returns: either a null pointer or a pointer to the possibly moved
*          allocated space.
*
******************************************************************************/

static void *
alloc__realloc(
    void * usrcore,
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp)
{
    char * syscore;
    U32 syssize, usrsize, new_blksize, current_blksize;

    alloc_trace_on_do(ahp);

    /* shrinking to zero size? */

    if(!size)
        {
        if(usrcore)
            {
            trace_0(TRACE_MODULE_ALLOC, "alloc__realloc -> alloc__free because zero size: ");
            alloc__free(usrcore, ahp);
            }

        trace_0(TRACE_MODULE_ALLOC, "yields NULL because zero size");
        alloc_trace_off_do(ahp);
        return(NULL);
        }

    /* first-time allocation? */

    if(!usrcore)
        {
        trace_0(TRACE_MODULE_ALLOC, "alloc__realloc -> alloc__malloc because NULL pointer: ");
        alloc_trace_off_do(ahp);
        return(alloc__malloc(size, ahp));
        }

    /* a real realloc */

    trace_4(TRACE_MODULE_ALLOC,
            "alloc__realloc(&%p, %u, (&%p->&%p)) ",
            usrcore, size, report_ptr_cast(ahp), report_ptr_cast(ahp->heap));

    {
    uintptr_t heap_start = (uintptr_t) ((ahp->heap) + 1);
    uintptr_t heap_hwm   = ((uintptr_t) ahp->heap) + ahp->heap->hwm;
    uintptr_t core       = (uintptr_t) usrcore;
    if((core < heap_start) || (core >= heap_hwm))
    {
        reportf("alloc__realloc(0x%8.8X): outwith heap 0x%8.8X..0x%8.8X\n", core, heap_start, heap_hwm);
        alloc_trace_off_do(ahp);
        return(NULL);
    }
    } /*block*/

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        {
        if(ahp->flags & alloc_validate_heap_before_alloc)
            alloc_validate_heap(ahp, "realloc");

        if(ahp->flags & alloc_validate_block_before_alloc)
            alloc_validate_block(ahp, a, "realloc");
        }
#endif

    usrsize = size;

    syscore = (P_U8) usrcore - startguardsize;
    syssize = usrsize - startguardsize + endguardsize;

    /* must use actual full block sizes */
    new_blksize     = round_heapmgr(syssize + HEAPMGR_OVERHEAD);
    current_blksize = blocksize(syscore) + HEAPMGR_OVERHEAD;

    /* loop for structure */
    for(;;)
        {
        /* block not changing allocated size? */

        if(new_blksize == current_blksize)
            {
            trace_1(TRACE_MODULE_ALLOC,
                    "yields &%p (not moved, same allocated size)\n",
                    usrcore);
            break;
            }

        /* block shrinking? */

        if(new_blksize < current_blksize)
            {
#if !defined(USE_HEAP_SWIS)
            if((syscore = riscos_ptr_realloc_shrink(syscore, new_blksize, current_blksize, ahp)) == NULL)
            {
                alloc_trace_off_do(ahp);
                return(NULL);
            }
#else
            _kernel_swi_regs  r;
            _kernel_oserror * e;

            r.r[0] = HeapReason_ExtendBlock;
            r.r[1] = (int) ahp->heap;
            r.r[2] = (int) syscore;
            r.r[3] = (int) (new_blksize - current_blksize);
            e = alloc_winge(_kernel_swi(OS_Heap, &r, &r), "realloc(shrink)");

            if(e)
                {
                trace_0(TRACE_MODULE_ALLOC, "yields NULL because of error in shrink");
                alloc_trace_off_do(ahp);
                return(NULL);
                }

            syscore = (char *) r.r[2];
#endif

            alloc_freeextrastore(ahp);

            usrcore = syscore + startguardsize;

            trace_1(TRACE_MODULE_ALLOC, "yields &%p (shrunk)\n", usrcore);
            break;
            }

        /* block is growing */

        /* new_blksize > current_blksize */

#if !defined(USE_HEAP_SWIS)
        if((syscore = riscos_ptr_realloc_grow(syscore, new_blksize, current_blksize, ahp)) == NULL)
        {
            alloc_trace_off_do(ahp);
            return(NULL);
        }
#else
        {
        _kernel_swi_regs  r;
        _kernel_oserror * e;

        r.r[0] = HeapReason_ExtendBlock;
        r.r[1] = (int) ahp->heap;
        r.r[2] = (int) syscore;
        r.r[3] = (int) (new_blksize - current_blksize);
        e = _kernel_swi(OS_Heap, &r, &r);

        if(e)
            {
            if(!alloc_needtoallocate(new_blksize, ahp))
                {
                trace_0(TRACE_MODULE_ALLOC, "yields NULL because heap extension failed");
                alloc_trace_off_do(ahp);
                return(NULL);
                }

            r.r[0] = HeapReason_ExtendBlock;
            r.r[1] = (int) ahp->heap;
            r.r[2] = (int) syscore;
            r.r[3] = (int) (new_blksize - current_blksize);
            e = alloc_winge(_kernel_swi(OS_Heap, &r, &r), "realloc(growth)");

#if TRACE_ALLOWED
            if(myassert(!e  &&  r.r[2]))
                myasserted("alloc__realloc(&%p, %u) failed unexpectedly after heap (&%p->&%p) extension\n",
                            usrcore, usrsize, ahp, ahp->heap);
#endif

            if(e)
                {
                trace_0(TRACE_MODULE_ALLOC, "yields NULL because of error in growth");
                alloc_trace_off_do(ahp);
                return(NULL);
                }
            }

        syscore = (char *) r.r[2];
        }
#endif

        alloc_freeextrastore(ahp);

        usrcore = syscore + startguardsize;

        trace_1(TRACE_MODULE_ALLOC, "yields &%p (grown)\n", usrcore);

        break; /* always - loop only for structure */
        }

#if defined(CHECK_ALLOCS)
    /* startguard always copied safely on realloc - just consider endguard fill */

    if(ahp->flags & alloc_validate_on) /* regardless of enable state */
        void_memset32(syscore + (blocksize(syscore) - endguardsize), FILL_BYTE, endguardsize);
#endif

#if defined(POST_CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_on_alloc)
            alloc_validate_heap(ahp, "AFTER_realloc");
#endif

    alloc_trace_off_do(ahp);

    return(usrcore);
}

static U32
alloc__size(
    P_ANY usrcore,
    P_ALLOC_HEAP_DESC ahp)
{
    const char * syscore;
    U32 size;

    alloc_trace_on_do(ahp);

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_on_size)
            alloc_validate_heap(ahp, "size");
#endif

    if(!usrcore)
        {
        alloc_trace_off_do(ahp);
        return(0);
        }

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_block_on_size)
            alloc_validate_block(ahp, a, "size");
#else
    IGNOREPARM(ahp);
#endif

    syscore = usrcore;
    syscore -= startguardsize;

    size = blocksize(syscore);
    size -= startguardsize + endguardsize;

    alloc_trace_off_do(ahp);

    return(size);
}

static void
alloc__validate(
    P_ANY a,
    const char * msg,
    P_ALLOC_HEAP_DESC ahp)
{
    switch((int) a)
        {
        case 0:
            alloc_validate_heap(ahp, msg, 0);
            break;

        case 1:
            ahp->flags = (ALLOC_HEAP_FLAGS) (ahp->flags | alloc_validate_enabled);
            break;

        default:
            alloc_validate_block(ahp, a, msg, 0);
            break;
        }
}

#if defined(USE_HEAP_SWIS)

static P_ANY
riscos_ptr_alloc(
    _InVal_     U32 size,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    _kernel_swi_regs rs;
    P_ANY p_any;

    rs.r[0] = HeapReason_Get;
    rs.r[1] = (int) heap;
    /* no r2 */
    rs.r[3] =       size;
    p_any   = _kernel_swi(OS_Heap, &rs, &rs) ? NULL : (P_ANY) rs.r[2];

    if(NULL == p_any)
    {
        if(!alloc_needtoallocate(round_heapmgr(HEAPMGR_OVERHEAD + size), ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed"));
            return(NULL);
        }

        rs.r[0] = HeapReason_Get;
        rs.r[1] = (int) heap;
        /* no r2 */
        rs.r[3] =       size;
        p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "malloc 2") ? NULL : (P_ANY) rs.r[2];

        myassert3x(p_any, TEXT("alloc__malloc(%u) failed unexpectedly after heap (&%p->&%p) extension"), size, ahp, heap);
    }

    return(p_any);
}

static void
riscos_ptr_free(
    P_RISCOS_USED_BLOCK p_used,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    _kernel_swi_regs rs;

    rs.r[0] = HeapReason_Free;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) (p_used + 1);
    (void) alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "free");
}

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_used,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    U32 size_diff = new_blksize - cur_blksize;
    _kernel_swi_regs rs;
    P_ANY p_any;

    rs.r[0] = HeapReason_ExtendBlock;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) p_used;
    rs.r[3] =       size_diff;
    p_any   = _kernel_swi(OS_Heap, &rs, &rs) ? NULL : (P_ANY) rs.r[2];

    if(NULL == p_any)
    {
        if(!alloc_needtoallocate(new_blksize, ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed"));
            return(NULL);
        }

        rs.r[0] = HeapReason_ExtendBlock;
        rs.r[1] = (int) heap;
        rs.r[2] = (int) p_used;
        rs.r[3] =       size_diff;
        p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "realloc(growth) 2") ? NULL : (P_ANY) rs.r[2];

        myassert3x(p_any, TEXT("alloc__realloc(%u) failed unexpectedly after heap (&%p->&%p) extension"), new_blksize - cur_blksize, ahp, heap);
    }

    alloc_freeextrastore(ahp);

    return(p_any);
}

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    U32 size_diff = new_blksize - cur_blksize;
    _kernel_swi_regs rs;

    rs.r[0] = HeapReason_ExtendBlock;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) p_any;
    rs.r[3] =       size_diff;
    p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "realloc(shrink)") ? NULL : (P_ANY) rs.r[2];

    return(p_any);
}

#else

#if TRACE_ALLOWED && (PERSONAL_TRACE_FLAGS & TRACE_MODULE_ALLOC) && defined(ALLOC_TRACK_PROCESS_USE)

static BITMAP(used_processes, 32);

static int used_process = 0;

static void
USED_PROCESS(
    _InVal_     int n)
{
    used_process = n;

    if(!bitmap_bit_test(used_processes, used_process, 32))
    {
        bitmap_bit_set(used_processes, used_process, 32);
        trace_1(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("used process %d"), used_process);
        host_bleep();
    }
}

#else

#define USED_PROCESS(n)

#endif

static P_ANY
riscos_ptr_alloc(
    _InVal_     U32 size_requested,
    P_ALLOC_HEAP_DESC ahp)
{
    U32 size;
    P_RISCOS_HEAP heap = ahp->heap;
    P_RISCOS_HEAP_FREE_DATA p_free_block;
    P_RISCOS_HEAP_USED_DATA p_used_block;
    U32 * p_free_offset;
    U32   next_free_offset;

#if defined(ALLOC_TRACK_USAGE)
    fprintf(stderr, "alloc:%x %x", size, round_heapmgr(HEAPMGR_OVERHEAD + size));
#endif

    /* block needs to be this big */
    size = round_heapmgr(HEAPMGR_OVERHEAD + size_requested);

    /* scan free list for first fit */
    p_free_block.v = p_free_offset = &heap->free;

    while((next_free_offset = *p_free_offset) != 0)
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        if(size <= p_free_block.f->size)
        {
            /* will return this block (or part thereof) */
            p_used_block.v = p_free_block.v;

            if(size == p_free_block.f->size)
            {
                /* take free block away entirely */
                *p_free_offset = p_free_block.f->free ? *p_free_offset + p_free_block.f->free : 0;
            }
            else /* if(size < p_free_block.f->size) */
            {
                /* allocate core from front of free block */
                RISCOS_FREE_BLOCK F = *p_free_block.f;
                /* this free block is now further away than before */
                *p_free_offset += size;
                p_free_block.c += size;
                /* next free block after this one is now closer than before */
                p_free_block.f->free = F.free ? F.free - size : 0;
                p_free_block.f->size = F.size - size;
                CLEAR_FREE(p_free_block);
            }

            p_used_block.u->size = size;
            return(p_used_block.u + 1);
        }

        p_free_offset = &p_free_block.f->free;
    }

    /* trivial allocation at hwm */
    if(!alloc_needtoallocate(size, ahp))
    {
        trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed"));
        return(NULL);
    }

    p_used_block.c = P_HEAP_HWM(heap);
    heap->hwm += size;

    p_used_block.u->size = size;
    return(p_used_block.u + 1);
}

static void
riscos_ptr_free(
    P_RISCOS_USED_BLOCK p_used,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    P_RISCOS_HEAP_FREE_DATA p_free_block;
    P_RISCOS_HEAP_USED_DATA p_used_block;
    U32 * p_free_offset;
    U32   next_free_offset;

    p_used_block.u = p_used;

    /* scan free list for insertion/coalescing */
    p_free_block.v = p_free_offset = &heap->free;

    while((next_free_offset = *p_free_offset) != 0)
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        /* coalesce with lower free block? */
        if(P_END_OF_FREE(p_free_block) == p_used_block.c)
        {
            p_free_block.f->size += p_used_block.u->size;
            if(p_free_block.f->free)
            {
                /* coalesce with upper free block too? */
                P_RISCOS_HEAP_FREE_DATA p_upper_free_block;
                p_upper_free_block.c = P_NEXT_FREE(p_free_block);
                if(P_END_OF_FREE(p_free_block) == p_upper_free_block.c)
                {
                    p_free_block.f->size += p_upper_free_block.f->size;
                    p_free_block.f->free = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - p_free_block.c : 0;
                    CLEAR_FREE(p_free_block);
                }
            }
            else
            {
                CLEAR_FREE(p_free_block);

                /* coalesce with end block too? */
                if(P_HEAP_HWM(heap) == P_END_OF_FREE(p_free_block))
                {
                    *p_free_offset = 0;
                    heap->hwm -= p_free_block.f->size;
                }
            }
            return;
        }

        if(p_free_block.c > p_used_block.c)
        {
            P_RISCOS_HEAP_FREE_DATA r;
            assert(p_free_block.c >= P_END_OF_USED(p_used_block));

            r.v = p_used_block.v;
            *p_free_offset = r.c - (P_U8) p_free_offset;
            r.f->size = p_used_block.u->size;

            if(P_END_OF_FREE(r) == p_free_block.c)
            {
                /* coalesce with upper free block */
                r.f->size += p_free_block.f->size;
                r.f->free = p_free_block.f->free ? P_NEXT_FREE(p_free_block) - r.c : 0;
            }
            else
                /* found position to insert at */
                r.f->free = p_free_block.c - (P_U8) &r.f->free;

            CLEAR_FREE(r);
            return;
        }

        p_free_offset = &p_free_block.f->free;
    }

    p_free_block.v = p_used_block.v;
    p_free_block.f->size = p_used_block.u->size;
    p_free_block.f->free = 0; /* at end of free list */

    CLEAR_FREE(p_free_block);

    /* try coalescing with end block */
    if(P_HEAP_HWM(heap) == P_END_OF_FREE(p_free_block))
    {
        heap->hwm -= p_free_block.f->size;
        return;
    }

    /* add to end of free list */
    *p_free_offset = p_free_block.c - (P_U8) p_free_offset;
}

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP heap = ahp->heap;
    U32 size_diff = new_blksize - cur_blksize;
    P_RISCOS_HEAP_USED_DATA p_used_block, p_new_used_block;
    P_RISCOS_HEAP_FREE_DATA p_free_block, p_lower_free_block, p_upper_free_block, p_fit_free_block;
    U32 * p_free_offset;
    U32 * p_fit_free_offset;
    U32   next_free_offset;

#if defined(ALLOC_TRACK_USAGE)
    fprintf(stderr, "realloc:&%p, %x, %x", p_any, new_blksize, cur_blksize);
#endif

    p_used_block.v = p_any;
    p_used_block.u -= 1;

    p_lower_free_block.f = p_upper_free_block.f = NULL;

    p_fit_free_block.f = NULL;
    p_fit_free_offset = NULL; /* keep dataflower happy */

    p_free_block.v = p_free_offset = &heap->free;

    /* easiest case is if the block we are reallocing is adjacent to the hwm (also special case for heap growth parms) */
    if(P_HEAP_HWM(heap) == P_END_OF_USED(p_used_block))
    {
        if(heap->size - heap->hwm >= size_diff)
        { USED_PROCESS(1) /*EMPTY*/ }
        else
        {
            /* look to see if there is a free block immediately below too that we can use */
            for(;;)
            {
                if((next_free_offset = *p_free_offset) == 0)
                {
                    USED_PROCESS(2);
                    if(!alloc_needtoallocate(size_diff, ahp))
                    {
                        trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed (1a)"));
                        return(NULL);
                    }
                    break;
                }

                assert(next_free_offset < heap->size);
                p_free_block.c += next_free_offset;

                if(P_END_OF_FREE(p_free_block) == p_used_block.c)
                {
                    /* now try to use this lower free block: we need to ask for less core, but need to move the block */
                    USED_PROCESS(3);
                    assert(!p_free_block.f->free);
                    if(heap->size - (heap->hwm - p_free_block.f->size) < size_diff)
                        if(!alloc_needtoallocate(size_diff - p_free_block.f->size, ahp))
                        {
                            trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed (1b)"));
                            return(NULL);
                        }

                    heap->hwm -= p_free_block.f->size; /* hwm becomes invalid for a mo */
                    *p_free_offset = 0;
                    void_memmove32(p_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_free_block.v;
                    break;
                }

                /* can we sneak in a first fit allocation instead? */
                if(new_blksize <= p_free_block.f->size)
                {
                    /* return this block (or part thereof) */
                    USED_PROCESS((new_blksize == p_free_block.f->size) ? 14 : 4);
                    p_fit_free_block.f = p_free_block.f;
                    p_fit_free_offset  = p_free_offset;
                    goto use_fit_free_block;
                }

                p_free_offset = &p_free_block.f->free;
            }
        }

        /* trivial allocation at hwm now kosher */
        heap->hwm += size_diff;
        p_used_block.u->size = new_blksize;
        return(p_used_block.u + 1);
    }

    /* look to see if there is a free block immediately below and/or above that we can use or one in the middle that would do for us */
    while((next_free_offset = *p_free_offset) != 0)
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        /* usable lower free block? */
        if(P_END_OF_FREE(p_free_block) == p_used_block.c)
        {
            p_lower_free_block.f = p_free_block.f;
            if(p_lower_free_block.f->free)
            {
                /* usable upper free block too? */
                p_upper_free_block.c = P_NEXT_FREE(p_lower_free_block);
                if(P_END_OF_USED(p_used_block) != p_upper_free_block.c)
                    p_upper_free_block.f = NULL;
            }

            break;
        }

        if(P_END_OF_USED(p_used_block) <= p_free_block.c)
        {
            /* usable p_upper_free_block block? */
            if(P_END_OF_USED(p_used_block) == p_free_block.c)
                p_upper_free_block.f = p_free_block.f;

            break;
        }

        /* could we sneak in a first fit allocation instead? */
        if(new_blksize <= p_free_block.f->size)
        {
            /* either a first fit or an overriding exact fit or a subsequent better fit */
            if(!p_fit_free_block.f
            || (new_blksize == p_free_block.f->size)
            || (p_fit_free_block.f->size > p_free_block.f->size))
            {
                p_fit_free_block.f = p_free_block.f;
                p_fit_free_offset  = p_free_offset;
            }
        }

        p_free_offset = &p_free_block.f->free;
    }

    if(p_upper_free_block.f)
    {
        if(p_upper_free_block.f->size >= size_diff)
        {
            /* if we have a lower free block too then this is the free chain link we must patch */
            if(p_lower_free_block.f)
                p_free_offset = &p_lower_free_block.f->free;

            if(p_upper_free_block.f->size == size_diff)
            {
                /* new allocation fits exactly using upper free block */
                USED_PROCESS(5);
                /* remove upper free block from list */
                *p_free_offset = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - (P_U8) p_free_offset : 0;
            }
            else
            {
                /* new allocation fits completely using upper free block */
                RISCOS_FREE_BLOCK upper_F;
                USED_PROCESS(6);
                upper_F = *p_upper_free_block.f;
                /* upper free block now further away than before */
                *p_free_offset += size_diff;
                p_upper_free_block.c += size_diff;
                /* and is itself closer to the entry above it */
                p_upper_free_block.f->free = upper_F.free ? upper_F.free - size_diff : 0;
                p_upper_free_block.f->size = upper_F.size - size_diff;
                CLEAR_FREE(p_upper_free_block);
            }
            p_used_block.u->size = new_blksize;
            return(p_used_block.u + 1);
        }

        if(p_lower_free_block.f)
        {
            if(p_upper_free_block.f->size + p_lower_free_block.f->size >= size_diff)
            {
                if(p_upper_free_block.f->size + p_lower_free_block.f->size == size_diff)
                {
                    /* new allocation fits exactly using both lower and upper free blocks */
                    USED_PROCESS(7);
                    /* remove both lower and upper free blocks from list */
                    *p_free_offset = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - (P_U8) p_free_offset : 0;
                    void_memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_lower_free_block.v;
                    p_used_block.u->size = new_blksize;
                }
                else
                {
                    /* new allocation fits completely using both lower and upper free blocks */
                    P_U8 p_next_free;
                    U32 lower_F_size;
                    U32 upper_F_size;
                    USED_PROCESS(8);
                    p_next_free = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) : NULL;
                    lower_F_size = p_lower_free_block.f->size;
                    upper_F_size = p_upper_free_block.f->size;
                    void_memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_lower_free_block.v;
                    p_used_block.u->size = new_blksize;
                    /* now just one free block, above the used block, at a completely different place */
                    p_upper_free_block.c = P_END_OF_USED(p_used_block);
                    *p_free_offset = p_upper_free_block.c - (P_U8) p_free_offset;
                    p_upper_free_block.f->free = p_next_free ? p_next_free - (P_U8) &p_upper_free_block.f->free : 0;
                    p_upper_free_block.f->size = lower_F_size + upper_F_size - size_diff;
                    CLEAR_FREE(p_upper_free_block);
                }
                return(p_used_block.u + 1);
            }
        }

        p_free_block.f = p_upper_free_block.f;
        p_free_offset = &p_free_block.f->free;
    }
    else if(p_lower_free_block.f)
    {
        if(p_lower_free_block.f->size >= size_diff)
        {
            if(p_lower_free_block.f->size == size_diff)
            {
                /* new allocation fits exactly using lower free block */
                USED_PROCESS(9);
                /* remove lower free block from list */
                *p_free_offset = p_lower_free_block.f->free ? P_NEXT_FREE(p_lower_free_block) - (P_U8) p_free_offset : 0;
                void_memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                p_used_block.v = p_lower_free_block.v;
                p_used_block.u->size = new_blksize;
            }
            else
            {
                /* new allocation fits completely using lower free block */
                P_U8 p_next_free;
                U32 lower_F_size;
                USED_PROCESS(10);
                p_next_free = p_lower_free_block.f->free ? P_NEXT_FREE(p_lower_free_block) : NULL;
                lower_F_size = p_lower_free_block.f->size;
                void_memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                p_used_block.v = p_lower_free_block.v;
                p_used_block.u->size = new_blksize;
                /* free block now above the used block, at a completely different place */
                p_upper_free_block.c = P_END_OF_USED(p_used_block);
                *p_free_offset = p_upper_free_block.c - (P_U8) p_free_offset;
                /* and is itself closer to the entry above it */
                p_upper_free_block.f->free = p_next_free ? p_next_free - (P_U8) &p_upper_free_block.f->free : 0;
                p_upper_free_block.f->size = lower_F_size - size_diff;
                CLEAR_FREE(p_upper_free_block);
            }
            return(p_used_block.u + 1);
        }

        p_free_block.f = p_lower_free_block.f;
        p_free_offset = &p_free_block.f->free;
    }
    else if(*p_free_offset != 0)
        p_free_offset = &p_free_block.f->free;

    /* if we didn't get a fit block, continue looping up free list, look for first fit */
    if(p_fit_free_block.f)
    { USED_PROCESS(11) /*EMPTY*/ }
    else
    {
        while((next_free_offset = *p_free_offset) != 0)
        {
            assert(next_free_offset < heap->size);
            p_free_block.c += next_free_offset;

            /* can we sneak in a first fit allocation instead? */
            if(new_blksize <= p_free_block.f->size)
            {
                /* return this block (or part thereof) */
                p_fit_free_block.f = p_free_block.f;
                p_fit_free_offset  = p_free_offset;
                USED_PROCESS((new_blksize == p_free_block.f->size) ? 15 : 12);
                break;
            }

            p_free_offset = &p_free_block.f->free;
        }
    }

    if(p_fit_free_block.f)
    {
    use_fit_free_block:;

        /* will return either all or part of this block */
        p_new_used_block.v = p_fit_free_block.v;

        if(new_blksize == p_fit_free_block.f->size)
        {
            /* take free block away entirely */
             *p_fit_free_offset = p_fit_free_block.f->free ? *p_fit_free_offset + p_fit_free_block.f->free : 0;
         }
        else /* if(new_blksize < p_fit_free_block.f->size) */
        {
            /* allocate core from front of free block */
            RISCOS_FREE_BLOCK F = *p_fit_free_block.f;
            /* this free block is now further away than before */
            *p_fit_free_offset += new_blksize;
            p_fit_free_block.c += new_blksize;
            /* next free block after this one is now closer than before */
            p_fit_free_block.f->free = F.free ? F.free - new_blksize : 0;
            p_fit_free_block.f->size = F.size - new_blksize;
            CLEAR_FREE(p_fit_free_block);
        }
    }
    else
    {
        /* trivial allocation at hwm  - so now we need to raise the hwm by new_blksize */
        USED_PROCESS(13);

        if(!alloc_needtoallocate(new_blksize, ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("yields NULL because heap extension failed (2)"));
            return(NULL);
        }

        p_new_used_block.c = P_HEAP_HWM(heap);
        heap->hwm += new_blksize;
    }

    void_memmove32(p_new_used_block.u, p_used_block.u, p_used_block.u->size);
    p_new_used_block.u->size = new_blksize;

    riscos_ptr_free(p_used_block.u, ahp);

    return(p_new_used_block.u + 1);
}

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    P_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP_USED_DATA p_used_block;

    /* update size of block we're keeping */
    p_used_block.v = p_any;
    p_used_block.u -= 1;
    p_used_block.u->size = new_blksize;

    /* free the end bit */
    p_used_block.c += p_used_block.u->size;
    p_used_block.u->size = cur_blksize - new_blksize;

    riscos_ptr_free(p_used_block.u, ahp);

    return(p_any);
}

#endif /* USE_HEAP_SWIS */

#endif /* RISCOS */

/* end of alloc.c */
