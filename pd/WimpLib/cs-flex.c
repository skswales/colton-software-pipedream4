/* cs-flex.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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
 *  SKS  16 Feb 2014    Compile tboxlibs' flex module into WimpLib
 *  SKS  17 Feb 2014    Added flex_forbid_shrink() function to tboxlibs' flex module to help printing
 *                      Added flex_granularity to tboxlibs' flex module
*/

#include "include.h"

/* the well-known implementation of flex has allocated block preceded by: */

typedef struct FLEX_BLOCK
{
    flex_ptr    anchor;
    int         size;           /* exact size of allocation in bytes */
}
FLEX_BLOCK, * P_FLEX_BLOCK;

/* like flex_free(), but caters for already freed / not yet allocated */

extern void
flex_dispose(
    flex_ptr anchor)
{
#if defined(REPORT_FLEX)
    reportf("flex_dispose(%p)", report_ptr_cast(anchor));
    reportf("flex_dispose(%p->%p)", report_ptr_cast(anchor), *anchor);
#endif

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
    P_FLEX_BLOCK p;

    /*trace_4(TRACE_MODULE_ALLOC, TEXT("flex_give_away(") PTR_XTFMT TEXT(":=") PTR_XTFMT TEXT("->") PTR_XTFMT TEXT(" (size %d))"), new_anchor, old_anchor, *old_anchor, flex_size(old_anchor));*/
#if defined(REPORT_FLEX)
    reportf(TEXT("flex_give_away(&%p := &%p->&%p)") TEXT(" (size %d))"), report_ptr_cast(new_anchor), report_ptr_cast(old_anchor), *old_anchor, flex_size_maybe_null(old_anchor));
#endif

    p = (P_FLEX_BLOCK) *old_anchor;

    *old_anchor = NULL;
    *new_anchor = p;

    if(NULL == p--)
        return;

    p->anchor = new_anchor;
}

_Check_return_
extern BOOL
flex_realloc(
    flex_ptr anchor,
    int newsize)
{
    /*trace_3(TRACE_MODULE_ALLOC, TEXT("flex_realloc(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(", %d)"), anchor, *anchor, newsize);*/
#if defined(REPORT_FLEX)
    reportf("flex_realloc(%p)", report_ptr_cast(anchor));
    reportf("flex_realloc(%p->%p, size %d)", report_ptr_cast(anchor), *anchor, newsize);
#endif

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
    P_FLEX_BLOCK p;

#if defined(REPORT_FLEX)
    reportf("flex_size_maybe_null(%p)", report_ptr_cast(anchor));
#endif

    p = (P_FLEX_BLOCK) *anchor;

    if(NULL == p--)
    {
        reportf("flex_size_maybe_null(%p->NULL): size 0 returned", report_ptr_cast(anchor));
        return(0);
    }

    reportf("flex_size_maybe_null(%p->%p): size %d", report_ptr_cast(anchor), *anchor, p->size);
    return(p->size);
}

#if defined(REPORT_FLEX)

#undef flex_alloc

_Check_return_
extern BOOL
report_flex_alloc(
    flex_ptr anchor,
    int size)
{
    reportf("report_flex_alloc(%p)", report_ptr_cast(anchor));

    if(0 == size)
        reportf("report_flex_alloc(%p->%p, size %d): size 0 will stuff up", report_ptr_cast(anchor), *anchor, size);

    if(*anchor)
        reportf("report_flex_alloc(%p->%p, size %d): anchor not NULL - will discard data without freeing", report_ptr_cast(anchor), *anchor, size);
    else
        reportf("report_flex_alloc(%p->NULL, size %d)", report_ptr_cast(anchor), size);

    return(flex_alloc(anchor, size));
}

#undef flex_extend

_Check_return_
extern BOOL
report_flex_extend(
    flex_ptr anchor,
    int newsize)
{
    reportf("report_flex_extend(%p)", report_ptr_cast(anchor));

    if(0 == newsize)
        reportf("report_flex_extend(%p->%p, newsize %d): size 0 will stuff up", report_ptr_cast(anchor), *anchor, newsize);

    if(*anchor)
        reportf("report_flex_extend(%p->%p, newsize %d) (cursize %d)", report_ptr_cast(anchor), *anchor, newsize, (flex_size)(anchor));
    else
        reportf("report_flex_extend(%p->NULL, newsize %d): anchor NULL - will fail", report_ptr_cast(anchor), newsize);

    return(flex_extend(anchor, newsize));
}

#undef flex_free

extern void
report_flex_free(
    flex_ptr anchor)
{
    reportf("report_flex_free(%p)", report_ptr_cast(anchor));

    if(*anchor)
        reportf("report_flex_free(%p->%p) (size %d)", report_ptr_cast(anchor), *anchor, (flex_size)(anchor));
    else
        reportf("report_flex_free(%p->NULL): anchor NULL - will fail", report_ptr_cast(anchor));

    (flex_free)(anchor);
}

#undef flex_size

_Check_return_
extern int
report_flex_size(
    flex_ptr anchor)
{
    int size = 0;

    reportf("report_flex_size(%p)", report_ptr_cast(anchor));

    if(*anchor)
    {
        size = (flex_size)(anchor);
        reportf("report_flex_size(%p->%p): size %d", report_ptr_cast(anchor), *anchor, size);
    }
    else
    {
        reportf("report_flex_size(%p->NULL): anchor NULL - will fail", report_ptr_cast(anchor));
        size = (flex_size)(anchor);
    }

    return(size);
}

#endif /* REPORT_FLEX */

int flex_granularity = 0x8000;     /* must be a power-of-two size or zero (exported) */

static inline int
flex_granularity_ceil(int n)
{
  if(flex_granularity)
  {
    int mask = flex_granularity - 1; /* flex_granularity must be a power-of-two */
    n = (n + mask) & ~mask;
  }
  return n;
}

static inline int
flex_granularity_floor(int n)
{
  if(flex_granularity)
  {
    int mask = flex_granularity - 1; /* flex_granularity must ve a power-of-two */
    n = n & ~mask;
  }
  return n;
}

/* wrapper for TBOXLIBS_FLEX */

/* can get better code for loading structure members on ARM Norcroft */

static struct FLEX_STATICS
{
    char *          start;          /* start of flex memory */
    char *          freep;          /* free flex memory */
    char *          limit;          /* limit of flex memory */

    int             area_num;       /* dynamic area handle */

    BOOL            shrink_forbidden;
} flex_;

/*ncr*/
extern BOOL
flex_forbid_shrink(
    BOOL forbid)
{
    BOOL res = flex_.shrink_forbidden;

#if defined(REPORT_FLEX)
    reportf("flex_forbid_shrink(%s)", report_boolstring(forbid));
#endif

    flex_.shrink_forbidden = forbid;

    return(res);
}

/******************************************************************************
*
* how much store do we have unused at the end of the flex area?
*
******************************************************************************/

_Check_return_
extern int
flex_storefree(void)
{
#if defined(REPORT_FLEX) && 0
    reportf("flex_storefree(): flex_.limit = %p, flex_.freep = %p, => free = %d",
            report_ptr_cast(flex_.limit), report_ptr_cast(flex_.freep), flex_.limit - flex_.freep);
#endif
    return(flex_.limit - flex_.freep);
}

#define flex__base     flex_.start
#define flex__start    flex_.start
#define flex__freep    flex_.freep
#define flex__lim      flex_.limit

#define flex__area_num flex_.area_num

#define flex__check() /*EMPTY*/

#undef TRACE /* don't mess with devices:parallel ! */

#define DefaultSize 0 /* for the Dynamic Area */

static void
flex__fail(int i);

#include "flex.c"

static void
flex__fail(int i)
{
    IGNOREPARM(i);
    werr(FALSE, msgs_lookup(MSGS_flex1)); /* don't abort */
}

/* end of cs-flex.c */
