/* cs-wimptx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_wimptx_h
#define __cs_wimptx_h

#ifndef __os_h
#include "os.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __wimpt_h
#include "wimpt.h"
#endif

#ifdef PARANOID
#define wimpt_safe(a) wimpt_complain(a)
#else
#define wimpt_safe(a) a
#endif

/*
wimpt.c
*/

/* ----------------------- wimpt_set_spritename ---------------------------
 * Description:   Sets a sprite name to use when the application reports
 *                info / warning / errors using the Window Manager
 *                for RISC OS 3.5 and later.
 *
 *                If omitted, the Window Manager looks for !<application>.
*/

extern void
wimpt_set_spritename(const char * spritename);

extern const char *
wimpt_get_spritename(void);

/* ----------------------- wimpt_set_taskname -----------------------------
 * Description:   Sets a name by which the application is registered with
 *                the Window and Task Managers.
 *
 *                Must be done prior to wimpt_init() to have any effect.
 *                If omitted, application is registered using the name
 *                passed to wimpt_init().
*/

extern void
wimpt_set_taskname(const char * taskname);

extern const char *
wimpt_get_taskname(void);

/* ----------------------- wimpt_set_safepoint ----------------------------
 * Descripton:    Sets a safe point to longjmp() to on faulting.
 *                NULL to disable, thereby forcing immediate exit.
*/

#ifndef __setjmp_h
#include <setjmp.h>
#endif

extern void
wimpt_set_safepoint(jmp_buf * safepoint);

/*
obtain callback just after/before calling wimp_poll
*/
typedef void (*wimpt_atentry_t)(wimp_eventstr *);
typedef void (*wimpt_atexit_t) (wimp_eventstr *);

extern wimpt_atentry_t
wimpt_atentry(wimpt_atentry_t pfnNewProc);

extern wimpt_atexit_t
wimpt_atexit(wimpt_atexit_t pfnNewProc);

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
wimpt_os_version_determine(void);

extern int
wimpt_os_version_query(void);

extern int
wimpt_platform_features_query(void);

/*
Fast access via macros
*/

extern int wimpt__mode;
#define wimpt_mode() wimpt__mode

extern int wimpt__dx;
extern int wimpt__dy;
#define wimpt_dx() wimpt__dx
#define wimpt_dy() wimpt__dy

extern int wimpt__bpp;
#define wimpt_bpp() wimpt__bpp

/* ---------------------- wimpt_xeig/wimpt_yeig ------------------------------
 * Description:   Inform caller of OS x/y units to screen pixel shift factor
 *
 * Parameters:    void
 * Returns:       OS x/y units to screen pixel shift factor
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern int
wimpt_xeig(void);

extern int
wimpt_yeig(void);

extern int wimpt__xeig;
extern int wimpt__yeig;
#define wimpt_xeig() wimpt__xeig
#define wimpt_yeig() wimpt__yeig

/* --------------------- wimpt_xsize/wimpt_ysize -----------------------------
 * Description:   Inform caller of OS x/y units dimensions of screen
 *
 * Parameters:    void
 * Returns:       OS x/y units dimensions of screen
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern int
wimpt_xsize(void);

extern int
wimpt_ysize(void);

extern int wimpt__xsize;
extern int wimpt__ysize;
#define wimpt_xsize() wimpt__xsize
#define wimpt_ysize() wimpt__ysize

/* -------------------------- wimpt_* ------------------------------------
 * Description:   More stuff.
 *
 * Parameters:    void
 * Returns:       values (in current mode).
 * Other Info:    faster than a normal OS call. Value is only valid if
 *                wimpt_checkmode is called at redraw events.
 *
 */

extern int wimpt__title_height;
extern int wimpt__hscroll_height;
extern int wimpt__vscroll_width;
#define wimpt_title_height()   wimpt__title_height
#define wimpt_hscroll_height() wimpt__hscroll_height
#define wimpt_vscroll_width()  wimpt__vscroll_width

/* ----------------------- wimpt_checkpalette ----------------------------
 * Description:   Invalidates ColourTrans cached colours and caches current
 *                palette for all wimp colours. Must be called on a
 *                palette change message (& on mode change) to work properly
*/

extern void
wimpt_checkpalette(void);

/* ------------------------- wimpt_palette -------------------------------
 * Description:   Returns the palette entry for a given wimp colour
*/

extern wimp_paletteword
wimpt_palette(int wimpcolour);

extern wimp_palettestr wimpt__palette;
#define wimpt_palette(wimpcolour) wimpt__palette.c[wimpcolour]

/* --------------------- wimpt_RGB_for_wimpcolour ------------------------
 * Description:   Returns the RGB value in the palette entry for a
 *                given wimp colour
*/

extern int
wimpt_RGB_for_wimpcolour(int wimpcolour);

#define wimpt_RGB_for_wimpcolour(wimpcolour) \
(wimpt_palette(wimpcolour).word & 0xFFFFFF00)

/* --------------------- wimpt_GCOL_for_wimpcolour ------------------------
 * Description:   Returns the GCOL value in the palette entry for a
 *                given wimp colour
*/

extern int
wimpt_GCOL_for_wimpcolour(int wimpcolour);

#define wimpt_GCOL_for_wimpcolour(wimpcolour) \
(wimpt_palette(wimpcolour).bytes.gcol)

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
