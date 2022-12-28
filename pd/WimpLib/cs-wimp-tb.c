/* cs-wimp-tb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

#include "os.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h"
#endif

/* tboxlibs compatible shims a la WimpLib */

_kernel_oserror *
tbl_wimp_create_window(WimpWindow *defn, int *handle)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    int result;

    __asm {
        MOV     R1, defn;
        SVC     #_XOS(Wimp_CreateWindow), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     result, R0;
        MOVVS   result, #0;
        STR     result, [handle];
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) defn;

    e = cs_kernel_swi(Wimp_CreateWindow, &rs);

    *handle = e ? 0 : rs.r[0];
#endif

    return(e);
}

_kernel_oserror *
tbl_wimp_create_icon(
    int priority,
    WimpCreateIconBlock *defn,
    int *handle)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    int result;

    __asm {
        MOV     R0, priority;
        MOV     R1, defn;
        SVC     #_XOS(Wimp_CreateIcon), /*in*/ {R0,R1}, /*out*/ {R0,PSR};
        MOV     result, R0;
        MOVVS   result, #0;
        STR     result, [handle];
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[0] = (int) priority;
    rs.r[1] = (int) defn;

    e = cs_kernel_swi(Wimp_CreateIcon, &rs);

    *handle = e ? 0 : rs.r[0];
#endif

    return(e);
}

#if !defined(NORCROFT_INLINE_SWIX)

_kernel_oserror *
tbl_wimp_delete_window(WimpDeleteWindowBlock *block)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_DeleteWindow), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = cs_kernel_swi(Wimp_DeleteWindow, &rs);
#endif

    return(e);
}

#endif /* NORCROFT_INLINE_SWIX */

#if !defined(NORCROFT_INLINE_SWIX)

_kernel_oserror *
tbl_wimp_delete_icon(WimpDeleteIconBlock *block)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_DeleteIcon), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = cs_kernel_swi(Wimp_DeleteIcon, &rs);
#endif

    return(e);
}

#endif /* NORCROFT_INLINE_SWIX */

#if !defined(NORCROFT_INLINE_SWIX)

_kernel_oserror *
tbl_wimp_open_window(WimpOpenWindowBlock *show)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) show;

    return(cs_kernel_swi(Wimp_OpenWindow, &rs));
}

#endif /* NORCROFT_INLINE_SWIX */

#if !defined(NORCROFT_INLINE_SWIX_NOT_YET)

_kernel_oserror *
tbl_wimp_redraw_window(WimpRedrawWindowBlock *block, int *more)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    int result;

    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_RedrawWindow), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     result, R0;
        MOVVS   result, #0; // FALSE
        STR     result, [more];
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = cs_kernel_swi(Wimp_RedrawWindow, &rs);

    *more = e ? 0 : rs.r[0];
#endif

    return(e);
}

_kernel_oserror *
tbl_wimp_update_window(WimpRedrawWindowBlock *block, int *more)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    int result;

    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_UpdateWindow), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     result, R0;
        MOVVS   result, #0; // FALSE
        STR     result, [more];
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = cs_kernel_swi(Wimp_UpdateWindow, &rs);

    *more = e ? 0 : rs.r[0];
#endif

    return(e);
}

_kernel_oserror *
tbl_wimp_get_rectangle(WimpRedrawWindowBlock *block, int *more)
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    int result;

    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_GetRectangle), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     result, R0;
        MOVVS   result, #0; // FALSE
        STR     result, [more];
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = cs_kernel_swi(Wimp_GetRectangle, &rs);

    *more = e ? 0 : rs.r[0];
#endif

    return(e);
}

#endif /* NORCROFT_INLINE_SWIX_NOT_YET */

#if !defined(NORCROFT_INLINE_SWIX)

_kernel_oserror *
tbl_wimp_get_window_state(WimpGetWindowStateBlock *state)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) state;

    return(cs_kernel_swi(Wimp_GetWindowState, &rs));
}

_kernel_oserror *
tbl_wimp_set_icon_state(WimpSetIconStateBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(cs_kernel_swi(Wimp_SetIconState, &rs));
}

_kernel_oserror *
tbl_wimp_get_icon_state(WimpGetIconStateBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(cs_kernel_swi(Wimp_GetIconState, &rs));
}

#endif /* NORCROFT_INLINE_SWIX */

_kernel_oserror *
tbl_wimp_force_redraw(
    int window_handle,
    int xmin,
    int ymin,
    int xmax,
    int ymax)
{
    union _force_redraw_u
    {
        _kernel_swi_regs rs;
        WimpForceRedrawBlock force_redraw_block;
    } u;

    u.force_redraw_block.window_handle = window_handle;
    u.force_redraw_block.redraw_area.xmin = xmin;
    u.force_redraw_block.redraw_area.ymin = ymin;
    u.force_redraw_block.redraw_area.xmax = xmax;
    u.force_redraw_block.redraw_area.ymax = ymax;

    return(cs_kernel_swi(Wimp_ForceRedraw, &u.rs));
}

_kernel_oserror *
tbl_wimp_set_caret_position(
    int window_handle,
    int icon_handle,
    int xoffset,
    int yoffset,
    int height,
    int index)
{
    _kernel_swi_regs rs;

    rs.r[0] = window_handle;
    rs.r[1] = icon_handle;
    rs.r[2] = xoffset;
    rs.r[3] = yoffset;
    rs.r[4] = height;
    rs.r[5] = index;

    return(cs_kernel_swi(Wimp_SetCaretPosition, &rs));
}

#if !defined(NORCROFT_INLINE_SWIX)

_kernel_oserror *
tbl_wimp_get_caret_position(WimpGetCaretPositionBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(cs_kernel_swi(Wimp_GetCaretPosition, &rs));
}

_kernel_oserror *
tbl_wimp_set_colour(int colour)
{
    _kernel_swi_regs rs;

    rs.r[0] = colour;

    return(cs_kernel_swi(Wimp_SetColour, &rs));
}

_kernel_oserror *
tbl_wimp_set_extent(int window_handle, BBox *area)
{
    _kernel_swi_regs rs;

    rs.r[0] = window_handle;
    rs.r[1] = (int) area;

    return(cs_kernel_swi(Wimp_SetExtent, &rs));
}

#endif /* NORCROFT_INLINE_SWIX */

#if !defined(NORCROFT_INLINE_SWIX_NOT_YET)

_kernel_oserror *
tbl_wimp_plot_icon(WimpPlotIconBlock *block) /* not yet */
{
    _kernel_oserror * e;

#if defined(NORCROFT_INLINE_ASM)
    __asm {
        MOV     R1, block;
        SVC     #_XOS(Wimp_PlotIcon), /*in*/ {R1}, /*out*/ {R0,PSR};
        MOV     e, R0;
        MOVVC   e, #0;
    };
#else
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(cs_kernel_swi(Wimp_PlotIcon, &rs));
#endif

    return(e);
}

#endif /* NORCROFT_INLINE_SWIX */

_kernel_oserror *
tbl_wimp_block_copy(
    int handle,
    int sxmin,
    int symin,
    int sxmax,
    int symax,
    int dxmin,
    int dymin)
{
    _kernel_swi_regs rs;

    rs.r[0] = handle;
    rs.r[1] = sxmin;
    rs.r[2] = symin;
    rs.r[3] = sxmax;
    rs.r[4] = symax;
    rs.r[5] = dxmin;
    rs.r[6] = dymin;

    return(cs_kernel_swi(Wimp_BlockCopy, &rs));
}

/* end of cs-wimp-tb.c */
