/* wlalloc.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* WimpLib (now RISC_OSLib) allocation and tracing thereof */

/* Stuart K. Swales 09-May-1989 */

#ifndef __wlalloc_h
#define __wlalloc_h

/*
exported functions
*/

extern void *
wlalloc_calloc(
    size_t num,
    size_t size);

extern void
wlalloc_dispose(
    void **v);

extern void
wlalloc_free(
    void *a);

extern void
wlalloc_init(void);

extern void *
wlalloc_malloc(
    size_t size);

extern void *
wlalloc_realloc(
    void *a,
    size_t size);

#if defined(COMPILING_WIMPLIB)
/* trivial redirect */
#define calloc(num, size) wlalloc_calloc(num, size)
#define free(a)           wlalloc_free(a)
#define malloc(size)      wlalloc_malloc(size)
#define realloc(a, size)  wlalloc_realloc(a, size)
#endif

#endif /* __wlalloc_h */

/* end of wlalloc.h */
