/* cs-flex.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_flex_h
#define __cs_flex_h

#if defined(WIMPLIB_FLEX)

#ifndef __flex_h
#include "./flex.h" /* WimpLib */
#endif

#define flex_init flex_init_new

_Check_return_
extern int
flex_init(
    char *program_name,
    int *error_fd,
    int dynamic_size);

extern int
flex_set_budge(int newstate);

#else /* NOT WIMPLIB_FLEX */

#ifndef __flex_h
#if defined(__CC_NORCROFT)
#include "C:flex.h" /* tboxlibs */
#else
#include "\coltsoft\trunk\cs-nonfree\Acorn\Library\32\tboxlibs\flex.h" /* tboxlibs */
#endif
#endif

#define REPORT_TBOX_FLEX 1

#if defined(REPORT_TBOX_FLEX) /* reporting shims */
#define flex_alloc(anchor, size) report_flex_alloc(anchor, size)
extern BOOL report_flex_alloc(flex_ptr anchor, int size);

#define flex_extend(anchor, newsize) report_flex_extend(anchor, newsize)
extern BOOL report_flex_extend(flex_ptr anchor, int newsize);

#define flex_free(anchor) report_flex_free(anchor)
extern void report_flex_free(flex_ptr anchor);
#endif

#endif /* WIMPLIB_FLEX */

/* granularity of allocation available from OS */
extern int flex_pagesize;

/* like flex_free(), but caters for already freed / not yet allocated */

extern void
flex_dispose(
    flex_ptr anchor);

extern BOOL
flex_forbid_shrink(BOOL forbid);

/* give flex memory to another owner */

extern void
flex_give_away(
    flex_ptr new_anchor,
    flex_ptr old_anchor);

_Check_return_
extern BOOL
flex_realloc(
    flex_ptr anchor,
    int newsize);

/* like flex_size(), but caters for already freed / not yet allocated */

_Check_return_
extern int
flex_size_maybe_null(
    flex_ptr anchor);

_Check_return_
extern int
flex_storefree(void);

#endif /* __cs_flex_h */

/* end of cs-flex.h */
