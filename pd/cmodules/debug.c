/* debug.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS/MRJC February and June 1991 */

#include "common/gflags.h"

#ifdef  TRACE_ALLOWED
#undef  TRACE_ALLOWED
#endif
#define TRACE_ALLOWED 1

#include "cmodules/debug.h"

#if RISCOS
#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif
#endif

/*
internal functions
*/

static BOOL trace__enabled     = 1;
static BOOL trace__initialised = 0;

S32 trace__count = 0;

/******************************************************************************
*
* tracef routine
*
******************************************************************************/

extern void __cdecl
tracef(
    _InVal_     U32 mask,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;

    if(!tracing(mask))
        return;

    va_start(args, format);
    vreportf(format, args);
    va_end(args);
}

static TCHARZ trace_buffer[4096];

extern void
vtracef(
    _InVal_     U32 mask,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    int len;

    if(!tracing(mask))
        return;

#if WINDOWS
    len = _vsntprintf_s(trace_buffer, elemof32(trace_buffer), _TRUNCATE, format, args);
    if(-1 == len)
        len = strlen32(trace_buffer); /* limit to what actually was achieved */
#else /* C99 CRT */
    len = vsnprintf(trace_buffer, elemof32(trace_buffer), format, args);
    if(len < 0)
        len = 0;
    /*else if((U32) len >= elemof32(trace_buffer))
        len = strlen32(trace_buffer);*/ /* limit to what actually was achieved */
#endif

    if(0 != len)
        report_output(trace_buffer);
}

extern void
trace_disable(void)
{
    while(trace__count)
        trace_off();

    trace__enabled = FALSE;
}

extern BOOL
trace_is_enabled(void)
{
    return(trace__enabled);
}

extern S32
trace_is_on(void)
{
    return(trace__count);
}

extern void
trace_off(void)
{
    if(trace__enabled)
        if(trace__count)
            --trace__count;
}

extern void
trace_on(void)
{
    if(trace__enabled)
    {
        if(trace__count++ == 0)
            if(!trace__initialised)
            {
#if RISCOS
                setbuf(stderr, NULL);
#endif
                trace__initialised = TRUE;
            }
    }
}

#if defined(TRACE_LIST)

#define writetodebugf vreportf

extern void
trace_list(
    _InVal_     U32 mask,
    P_ANY /*P_LIST_BLOCK*/ e_p_list_block)
{
    P_LIST_BLOCK p_list_block = (P_LIST_BLOCK) e_p_list_block;
    ARRAY_INDEX i;
    LIST_ITEMNO high_itemno = -1;
    LIST_ITEMNO last_itemno = -1;

    if(!tracing(mask))
        return;

    writetodebugf(TEXT("list &%p, numitem &%4lX, offsetc &%4hX, itemc &%4hX, ix_pooldesc &%2lX, h_pooldesc &%4lX, n_pooldesc &%2lX"),
                  p_list_block, (S32) p_list_block->numitem,
                  (U16) p_list_block->offsetc, (U16) p_list_block->itemc,
                  (S32) p_list_block->ix_pooldesc, (S32) p_list_block->h_pooldesc,
                  array_elements32(&p_list_block->h_pooldesc));

    for(i = 0; i < array_elements(&p_list_block->h_pooldesc); ++i)
    {
        P_POOLDESC p_pooldesc = array_ptr(&p_list_block->h_pooldesc, POOLDESC, i);

        writetodebugf(TEXT("  pool &%2lX %s, &%p, h_pool &%4lX, pool item &%6lX, pool bytes &%6lX"),
                      (S32) i, ((i == p_list_block->ix_pooldesc) ? "(*current*) " : ""),
                      p_pooldesc, (S32) p_pooldesc->h_pool,
                      (S32) p_pooldesc->poolitem, array_elements32(&p_pooldesc->h_pool));

        if(p_pooldesc->h_pool)
        {
            P_LIST_ITEM base_it = array_base(&p_pooldesc->h_pool, LIST_ITEM);
            P_LIST_ITEM it      = base_it;
            P_LIST_ITEM end_it  = al_array_end(&p_pooldesc->h_pool, LIST_ITEM);
            LIST_ITEMNO itemno  = p_pooldesc->poolitem;

            if(itemno < last_itemno)
                writetodebugf(TEXT("*** list corrupt ***: itemno &%6lX < last pool itemno &%6lX"), (S32) itemno, (S32) last_itemno);

            while(it < end_it)
            {
                OFF_TYPE itemsize;

                if(it == base_it)
                    if(it->offsetp)
                        writetodebugf(TEXT("*** pool corrupt ***: base it &%p has non-zero offsetp &%4hX"), it, (U16) it->offsetp);

                if(it->offsetn)
                    itemsize = (OFF_TYPE) it->offsetn;
                else
                    itemsize = (OFF_TYPE) ((PC_U8) end_it - (PC_U8) it);

                if(it->fill)
                {
                    writetodebugf(TEXT("    item &%6lX &%p, filler &%6lX"), (S32) itemno, it, (S32) it->i.itemfill);

                    if(itemsize != sizeof32(*it))
                        writetodebugf(TEXT("*** filler corrupt ***: size &%4hX > sizeof-LIST_ITEM &%4hX"), (U16) itemsize, (U16) sizeof32(*it));
                }
                else
                {
                    writetodebugf(TEXT("    item &%6lX &%p, contents &%p, size &%4hX"), (S32) itemno, it, list_itemcontents(void, it), itemsize - offsetof32(LIST_ITEM, i));
                    high_itemno = MAX(high_itemno, itemno);
                }

                itemno += list_leapnext(it);

                if(itemno > list_numitem(p_list_block))
                    writetodebugf(TEXT("*** list corrupt ***: item &%6lX > numitem &%6lX"), (S32) itemno, (S32) list_numitem(p_list_block));

                it = (P_ANY) ((P_U8) it + itemsize);
            }

            if(it > end_it)
                writetodebugf(TEXT("  *** pool corrupt ***: it &%p > end_it &%p"), it, end_it);

            writetodebugf(TEXT("  pool end, itemno &%6lX"), (S32) itemno);
            last_itemno = itemno;
        }
    }

    writetodebugf(TEXT("highest non-filler itemno &%6lX, highest &%6lX, numitem &%6lX"), (S32) high_itemno, (S32) last_itemno, (S32) list_numitem(p_list_block));
}

#endif /* TRACE_LIST */

/* end of debug.c */
