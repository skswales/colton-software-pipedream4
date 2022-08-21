/* wlalloc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* WimpLib (i.e. patched & wrapped RISC_OSLib) allocation and tracing thereof */

/* Stuart K. Swales 09-May-1989 */

/* can #define BARF_ON_FAIL to get useful low-memory error tracing */

#include "common/gflags.h"

#include "cmodules/riscos/wlalloc.h"

extern void *
wlalloc_calloc(
    size_t num,
    size_t size)
{
    STATUS status;
    void * a = al_ptr_calloc_bytes(void *, num * size, &status);

    trace_3(TRACE_RISCOS_HOST, "wlalloc_calloc(%d, %d) yields &%p", num, size, a);

#ifdef BARF_ON_FAIL
    if(!a)
        messagef("wlalloc_calloc(%d, %d) yields NULL", num, size);
#endif

    return(a);
}

extern void
wlalloc_dispose(
    void ** v)
{
    if(v)
    {
        void * a = *v;

        if(a)
        {
            *v = NULL;
            wlalloc_free(a);
        }
    }
}

extern void
wlalloc_free(
    void *a)
{
    trace_1(TRACE_RISCOS_HOST, "wlalloc_free(&%p)", a);

    al_ptr_free(a);
}

extern void *
wlalloc_malloc(
    size_t size)
{
    STATUS status;
    void * a = al_ptr_alloc_bytes(void *, size, &status);

    trace_2(TRACE_RISCOS_HOST, "wlalloc_malloc(%d) yields &%p", size, a);

#ifdef BARF_ON_FAIL
    if(!a)
        messagef("wlalloc_malloc(%d) yields NULL", size);
#endif

    return(a);
}

extern void *
wlalloc_realloc(
    void *old_a,
    size_t size)
{
    STATUS status;
    void * a = al_ptr_realloc_bytes(void *, old_a, size, &status);

    trace_3(TRACE_RISCOS_HOST, "wlalloc_realloc(&%p, %d) yields &%p", old_a, size, a);

#ifdef BARF_ON_FAIL
    if(!a  &&  size)
        messagef("wlalloc_realloc(&%p, %d) yields NULL", a, size);
#endif

    return(a);
}

/* end of wlalloc.c */
