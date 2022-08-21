/* cs-wimptx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_wimptx_h
#define __cs_wimptx_h

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __wimpt_h
#include "wimpt.h"
#endif

#ifdef PARANOID
#define wimpt_safe(a) wimpt_complain((a))
#else
#define wimpt_safe(a) (void) (a)
#endif

/*
cs-wimptx.c (wraps wimpt.c)
*/

extern int /*button_clicked*/
wimp_ReportError_wrapped(
    const _kernel_oserror * e,
    int errflags,
    const char * name,
    const char * spritename,
    const char * spritearea,
    const char * buttons);

/* ----------------------- wimptx_set_spritename --------------------------
 * Description:   Sets a sprite name to use when the application reports
 *                info / warning / errors using the Window Manager
 *                for RISC OS 3.5 and later.
 *
 *                If omitted, the Window Manager looks for !<application>.
*/

extern void
wimptx_set_spritename(const char * spritename);

extern const char *
wimptx_get_spritename(void);

/* ----------------------- wimptx_set_taskname ----------------------------
 * Description:   Sets a name by which the application is registered with
 *                the Window Manager and Task Manager.
 *
 *                Must be done prior to wimpt_init() to have any effect.
 *                If omitted, application is registered using the name
 *                passed to wimpt_init().
*/

extern void
wimptx_set_taskname(const char * taskname);

extern const char *
wimptx_get_taskname(void);

/* ----------------------- wimptx_set_safepoint ---------------------------
 * Descripton:    Sets a safe point to longjmp() to on faulting.
 *                NULL to disable, thereby forcing immediate exit.
*/

#ifndef __setjmp_h
#include <setjmp.h>
#endif

extern void
wimptx_set_safepoint(jmp_buf * safepoint);

/*
obtain callback just after/before calling wimp_poll
*/

typedef void (*wimptx_atentry_t)(wimp_eventstr *e);
typedef void (*wimptx_atexit_t) (wimp_eventstr *e);

extern wimptx_atentry_t
wimptx_atentry(wimptx_atentry_t pfnNewProc);

extern wimptx_atexit_t
wimptx_atexit(wimptx_atexit_t pfnNewProc);

/*
cs-wimptx.c
*/

/* ----------------------------- os_set_error -------------------------------
 * Set up a dynamic error block.
 *
 */

extern os_error *
os_set_error(int errnum, const char * errmess);

#ifndef RISC_OS_3_5
#define RISC_OS_3_5 0xa5
#endif

extern void
wimptx_os_version_determine(void);

extern int
wimptx_os_version_query(void);

extern int
wimptx_platform_features_query(void);

/*
Fast access to RISC_OSLib mode variables in wimpt.c via macros
*/

extern int wimpt__mode;
#define wimpt_mode() wimpt__mode

extern int wimpt__dx;
extern int wimpt__dy;
#define wimpt_dx() wimpt__dx
#define wimpt_dy() wimpt__dy

extern int wimpt__bpp;
#define wimpt_bpp() wimpt__bpp

/* ---------------- wimptx_XEigFactor/wimptx_YEigFactor ----------------------
 * Description:   Inform caller of OS x/y units to screen pixel shift factor
 *
 * Parameters:    void
 * Returns:       OS x/y units to screen pixel shift factor
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern unsigned int
wimptx_XEigFactor(void);

extern unsigned int
wimptx_YEigFactor(void);

extern unsigned int wimptx__XEigFactor;
extern unsigned int wimptx__YEigFactor;
#define wimptx_XEigFactor() wimptx__XEigFactor
#define wimptx_YEigFactor() wimptx__YEigFactor

/* ----------- wimptx_display_size_cx/wimptx_display_size_cy -----------------
 * Description:   Inform caller of OS x/y units dimensions of screen
 *
 * Parameters:    void
 * Returns:       OS x/y units dimensions of screen
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern unsigned int
wimptx_display_size_cx(void);

extern unsigned int
wimptx_display_size_cy(void);

extern unsigned int wimptx__display_size_cx;
extern unsigned int wimptx__display_size_cy;
#define wimptx_display_size_cx() wimptx__display_size_cx
#define wimptx_display_size_cy() wimptx__display_size_cy

/* -------------------------- wimpt_* ------------------------------------
 * Description:   More stuff.
 *
 * Parameters:    void
 * Returns:       values (in current mode).
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern int wimptx__title_height;
extern int wimptx__hscroll_height;
extern int wimptx__vscroll_width;
#define wimptx_title_height()   wimptx__title_height
#define wimptx_hscroll_height() wimptx__hscroll_height
#define wimptx_vscroll_width()  wimptx__vscroll_width

/* ----------------------- wimptx_checkpalette ---------------------------
 * Description:   Invalidates ColourTrans cached colours and caches current
 *                palette for all wimp colours. Must be called on a
 *                palette change message (& on mode change) to work properly
*/

extern void
wimptx_checkpalette(void);

/* ------------------------- wimptx_palette ------------------------------
 * Description:   Returns the palette entry
 *                for a given wimp colour index
*/

extern wimp_paletteword
wimptx_palette(int wimpcolour);

extern wimp_palettestr wimptx__palette;
#define wimptx_palette(wimpcolour) ( \
    wimptx__palette.c[(wimpcolour)] )

/* --------------------- wimptx_RGB_for_wimpcolour -----------------------
 * Description:   Returns the RGB value in the palette entry
 *                for a given wimp colour index
*/

extern int
wimptx_RGB_for_wimpcolour(int wimpcolour);

#define wimptx_RGB_for_wimpcolour(wimpcolour) (\
    wimptx_palette(wimpcolour).word & 0xFFFFFF00 )

/* --------------------- wimptx_GCOL_for_wimpcolour -----------------------
 * Description:   Returns the GCOL value in the palette entry
 *                for a given wimp colour index
*/

extern int
wimptx_GCOL_for_wimpcolour(int wimpcolour);

#define wimptx_GCOL_for_wimpcolour(wimpcolour) ( \
    wimptx_palette(wimpcolour).bytes.gcol )

_Check_return_
extern BOOL
host_must_die_query(void);

extern void
host_must_die_set(
    _InVal_     BOOL must_die);

extern void
wimptx_stack_overflow_handler(int sig);

/*
cs-riscasm.s
*/

extern void
riscos_hourglass_off(void);

extern void
riscos_hourglass_on(void);

extern void
riscos_hourglass_percent(int percent);

extern void
riscos_hourglass_smash(void);

#endif /* __cs_wimptx_h */

/* end of cs-wimptx.h */
