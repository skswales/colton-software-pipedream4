/* cs-flex.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Colton Software changes to flex module:
 *
 *  MRJC    18/11/88    If setting the slotsize fails, the original value is restored
 *  MRJC    16/6/89     flex_storefree returns amount of unused flex memory
 *  MRJC    19/6/89     store returns amount of store available from wimp
 *  MRJC    20/6/89     flex__more checks if there is any memory before writing
 *                      wimpslot to avoid lots of silly task manager messages
 *  SKS     04/08/89    exported page size for alloc.c to check
 *  SKS     22/08/89    made flex_extend more like realloc
 *  SKS     26/09/89    made flex use faster memory copier
 *  nd      08/07/96    Incorporated checking for and usage of dynamic areas
 *  SKS  17 Nov 1996    More code from Fireworkz brought back (flex__ensure,flex__wimpslot,flex__give,flex_init,dynamicarea_*)
 *  SKS  27 Jan 1997    Added flex_forbid_shrink() function to help printing
 *  SKS  04 Sep 2013    Allow use of tboxlibs' flex module
*/

#include "include.h"

#if defined(WIMPLIB_FLEX)

/* can get better code for loading structure members on ARM Norcroft */

static struct flex_
{
    char *          start;          /* start of flex memory */
    char *          freep;          /* free flex memory */
    char *          limit;          /* limit of flex memory */

    BOOL            using_dynhnd;   /* using dynamic area */
    int             dynhnd;         /* dynamic area handle */

    BOOL            shrink_forbidden;
} flex_;

int flex_pagesize;                          /* page size (exported) */

#define flex_innards(p)      ((char *) (p + 1))

#define flex_next_block(p)   ((flex__rec *) (flex_innards(p) + roundup(p->size)))

#define flex__check() /*EMPTY*/

/*
nd: 08-07-1996
*/

/* Creates a dynamic area and returns the handle of it */

static _kernel_oserror *
dynamicarea_create(
    _Out_       int * hand,
    _InVal_     int dynamic_size,
    _In_z_      const char * name)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 0;
    rs.r[1] = -1;
    rs.r[2] = 0;
    rs.r[3] = -1;
    rs.r[4] = (1 << 7);
    rs.r[5] = dynamic_size;
    rs.r[6] = 0;
    rs.r[7] = 0;
    rs.r[8] = (int) name;

    if(NULL == (e = _kernel_swi(OS_DynamicArea, &rs, &rs)))
        *hand = rs.r[1];
    else
        *hand = 0;

    return(e);
}

/* Kills off an existing dynamic area */

static _kernel_oserror *
dynamicarea_kill(
    _In_        int hand)
{
    _kernel_swi_regs rs;

    rs.r[0] = 1;
    rs.r[1] = hand;

    return(_kernel_swi(OS_DynamicArea, &rs, &rs));
}

/* Reads info about a dynamic area */

static _kernel_oserror *
dynamicarea_read(
    _In_        int hand,
    _Out_       int * p_size,
    _Out_       int * p_base)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 2;
    rs.r[1] = hand;

    if(NULL != (e = _kernel_swi(OS_DynamicArea, &rs, &rs)))
    {
        rs.r[2] = 0;
        rs.r[3] = 0;
    }

    *p_size = rs.r[2];
    *p_base = rs.r[3];

    return(e);
}

/* Changes the size of a dynamic area */

static _kernel_oserror *
dynamicarea_change(
    _In_        int hand,
    _In_        int size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = hand;

    if(NULL != (e = _kernel_swi(OS_ReadDynamicArea, &rs, &rs)))
        return(e);

    rs.r[1] = size - rs.r[1];
    rs.r[0] = hand;

    return(_kernel_swi(OS_ChangeDynamicArea, &rs, &rs));
}

/* Reads the size of the current task's slot */

static _kernel_oserror *
wimp_currentslot_read(
    _Out_       int * p_size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = -1;
    rs.r[1] = -1;

    if(NULL != (e = _kernel_swi(Wimp_SlotSize, &rs, &rs)))
        rs.r[0] = 0;

    *p_size = rs.r[0];

    return(e);
}

/* Changes the size of the current task's slot */

static _kernel_oserror *
wimp_currentslot_change(
    _Inout_     int * p_size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = *p_size;
    rs.r[1] = -1;

    if(NULL != (e = _kernel_swi(Wimp_SlotSize, &rs, &rs)))
        return(e);

    *p_size = rs.r[0];

    return(NULL);
}

/* Write the top of available memory */

static _kernel_oserror *
flex__area_change(
    _Inout_     int * p_top)
{
    _kernel_oserror * e;
    int top = *p_top;
    int base = 0x8000;
    int size;

    if(flex_.dynhnd)
    {
        base = (int) flex_.start;
        size = top - base;
        e = dynamicarea_change(flex_.dynhnd, size);
    }
    else
    {
        size = top - base;
        e = wimp_currentslot_change(&size);
    }

    if(NULL == e)
        *p_top = base + size;

    return(e);
}

/* Read the top of available memory */

static void inline
flex__area_read(
    _Out_       int * p_top)
{
    _kernel_oserror * e;
    int base = 0x8000;
    int size = 0;
    
    if(flex_.dynhnd)
        e = dynamicarea_read(flex_.dynhnd, &size, &base);
    else
        e = wimp_currentslot_read(&size);

    if(NULL != e)
    {
        base = 0;
        size = 0;
    }

    *p_top = base + size;
}

#define flex__base  flex_.start
#define flex__start flex_.start
#define flex__freep flex_.freep
#define flex__lim   flex_.limit

#include "flex.c"

static void
flex_kill(void)
{
    if(flex_.dynhnd)
    {
        flex_.using_dynhnd = FALSE;
        dynamicarea_kill(flex_.dynhnd);
        flex_.dynhnd = 0;
    }
}

static void
flex_atexit(void)
{
    flex_kill();
}

/* replacement function with tboxlibs-like interface */

extern int
flex_init(
    char *program_name,
    int *error_fd,
    int dynamic_size)
{
    IGNOREPARM(error_fd);

    {
    _kernel_swi_regs rs;
    (void) _kernel_swi(OS_ReadMemMapInfo, &rs, &rs);
    flex_pagesize = rs.r[0];
    } /*block*/

    if(flex_pagesize < 0x4000)
        /* SKS says don't page violently on RISC PC (allow 2Mb A3000 etc to get away with 16kb pages though) */
        flex_pagesize = 0x8000;

#if defined(SHAKE_HEAP_VIOLENTLY)
    flex_pagesize = 0x0080;
#endif
    tracef1("flex_init(): flex_pagesize = %d\n", flex_pagesize);

    if((dynamic_size != 0) && (NULL == dynamicarea_create(&flex_.dynhnd, dynamic_size, program_name))) /* call should fail if OS_DynamicArea not supported */
    {
        flex_.using_dynhnd = TRUE;
        atexit(flex_atexit);
    }

    { /* Read current top of memory (either Window Manager current slot or Dynamic area just created) */
    int top;
    flex__area_read(&top);
    flex_.start = flex_.freep = flex_.limit = (char *) top;
    tracef1("flex_limit = &%p\n", flex_.limit);
    } /*block*/

    {
    void * a;
    if(flex_alloc(&a, 1))
    {
        flex_free(&a);
        return(flex_.dynhnd);
    }
    } /*block*/

    flex_kill();

    return(-1);
}

extern int flex_set_budge(int newstate)
{
    IGNOREPARM(newstate);
    return(0);
}

extern BOOL
flex_forbid_shrink(BOOL forbid)
{
    BOOL res = flex_.shrink_forbidden;

    flex_.shrink_forbidden = forbid;

    return(res);
}

/* how much store do we have unused at the end of the flex area? */

_Check_return_
extern int
flex_storefree(void)
{
    return(flex_.limit - flex_.freep);
}

#else

int flex_pagesize = 0x8000; /* page size (exported) */

typedef struct {
  flex_ptr anchor;      /* *anchor should point back to here. */
  int size;             /* in bytes. Exact size of logical area. */
                        /* then the actual store follows. */
} flex__rec;

/*ncr*/
extern BOOL
flex_forbid_shrink(BOOL forbid)
{
    flex_set_deferred_compaction(TRUE);

    return(!forbid);
}

_Check_return_
extern int
flex_storefree(void)
{
    return(0);
}

#ifdef flex_alloc
extern BOOL report_flex_alloc(flex_ptr anchor, int size)
{
reportf("report_flex_alloc(%p)", report_ptr_cast(anchor));
reportf("flex_alloc(%p->%p, %d)", report_ptr_cast(anchor), *anchor, size);
    return((flex_alloc)(anchor, size));
}
#endif

#ifdef flex_extend
extern BOOL report_flex_extend(flex_ptr anchor, int newsize)
{
reportf("report_flex_extend(%p)", report_ptr_cast(anchor));
reportf("flex_extend(%p->%p, %d)", report_ptr_cast(anchor), *anchor, newsize);
    return((flex_extend)(anchor, newsize));
}
#endif

#ifdef flex_free
extern void report_flex_free(flex_ptr anchor)
{
reportf("report_flex_free(%p)", report_ptr_cast(anchor));
reportf("flex_free(%p->%p)", report_ptr_cast(anchor), *anchor);
    (flex_free)(anchor);
}
#endif


/* like flex_free(), but caters for already freed / not yet allocated */

extern void
flex_dispose(
    flex_ptr anchor)
{
    reportf("flex_dispose(%p)", report_ptr_cast(anchor));
    reportf("flex_dispose(%p->%p)", report_ptr_cast(anchor), *anchor);

    if(NULL == *anchor)
        return;

    flex_free(anchor);
}

/* SKS adds mechanism to give core from one anchor to another */

extern void
flex_give_away(
    flex_ptr new_anchor,
    flex_ptr old_anchor)
{
    flex__rec * p;

    reportf(TEXT("flex_give_away(&%p := &%p->&%p)") TEXT(" (size %d))"), report_ptr_cast(new_anchor), report_ptr_cast(old_anchor), *old_anchor, flex_size_maybe_null(old_anchor));

    p = (flex__rec *) *old_anchor;

    *old_anchor = NULL;
    *new_anchor = p;

    if(NULL == p--)
        return;

#if defined(WIMPLIB_FLEX)
    if(((char *) p < flex_.start) || ((char *) (p + 1) > flex_.freep))
        werr(FALSE, "flex_give_away: block (%p->%p) is not in flex store (%p:%p)", old_anchor, *new_anchor, flex_.start, flex_.freep);

    if(p->anchor != old_anchor)
        werr(FALSE, "flex_give_away: old_anchor (%p->%p) does not match that stored in block (%p->%p)", old_anchor, *new_anchor, p, p->anchor);
#endif /* WIMPLIB_FLEX */

    p->anchor = new_anchor;
}

_Check_return_
extern BOOL
flex_realloc(
    flex_ptr anchor,
    int newsize)
{
    reportf("flex_realloc(%p)", report_ptr_cast(anchor));
    reportf("flex_realloc(%p->%p, size %d)", report_ptr_cast(anchor), *anchor, newsize);

    if(0 == newsize)
    {
        flex_dispose(anchor);
        return(FALSE);
    }

    if(NULL != *anchor)
        return(flex_extend(anchor, newsize));

    return(flex_alloc(anchor, newsize));
}

/* like flex_size(), but caters for already freed / not yet allocated */

_Check_return_
extern int
flex_size_maybe_null(
    flex_ptr anchor)
{
    flex__rec * p;

    reportf("flex_size_maybe_null(%p)", report_ptr_cast(anchor));

    p = (flex__rec *) *anchor;

    if(NULL == p--)
    {
        reportf("flex_size_maybe_null(%p->NULL): size 0 returned", report_ptr_cast(anchor));
        return(0);
    }

    reportf("flex_size_maybe_null(%p->%p): size %d", report_ptr_cast(anchor), *anchor, p->size);
    return(p->size);
}

#endif /* WIMPLIB_FLEX */

/* end of cs-flex.c */
