/* cs-bbcx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_bbcx_h
#define __cs_bbcx_h

#if defined(NORCROFT_INLINE_SWIX)
#include "C:swis.h"
#endif

#ifndef __bbc_h
#include "bbc.h"
#endif

/*
cs-riscasm.s
*/

#if defined(NORCROFT_INLINE_SWIX)
#define os_writeN(s, count) _swix(OS_WriteN, _INR(0,1), (s), (count))
#else
_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
os_writeN(
    _In_reads_(count) const char * s,
    _InVal_     U32 count);
#endif /* NORCROFT_INLINE_SWIX */

#if defined(NORCROFT_INLINE_SWIX)
_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
os_plot(
    _InVal_     int code,
    _InVal_     int x,
    _InVal_     int y)
{
    return(
        _swix(OS_Plot, _INR(0, 2),
        /*in*/  code, x, y));
}
//#elif defined(NORCROFT_INLINE_SWIX_COMPILER_BARFS)
#define os_plot(code, x, y) _swix(OS_Plot, _INR(0,2), (code), (x), (y))
#else
_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
os_plot(
    _InVal_     int code,
    _InVal_     int x,
    _InVal_     int y);
#endif /* NORCROFT_INLINE_SWIX */

#if 1 || !defined(COMPILING_WIMPLIB)

#define bbc_draw(x, y) \
    os_plot(bbc_SolidBoth + bbc_DrawAbsFore, x, y)

/* Draw a line to coordinates specified relative to current graphic cursor. */
#define bbc_drawby(x, y) \
    os_plot(bbc_SolidBoth + bbc_DrawRelFore, x, y)

#define bbc_move(x, y) \
    os_plot(bbc_SolidBoth + bbc_MoveCursorAbs, x, y)

#endif /* COMPILING_WIMPLIB */

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
riscos_vdu_define_graphics_window(
    _In_        GDI_COORD x1,
    _In_        GDI_COORD y1,
    _In_        GDI_COORD x2,
    _In_        GDI_COORD y2);

#endif /* __cs_bbcx_h */

/* end of cs_bbcx.h */
