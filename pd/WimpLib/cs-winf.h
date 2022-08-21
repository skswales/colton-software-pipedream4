/* cs-winf.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_winf_h
#define __cs_winf_h

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

/*
cs-winf.c
*/

extern BOOL
winf_adjustbumphit(
    _Inout_     int * const p_hit_icon_handle,
    int val);

extern void
winf_changedfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

extern void
winf_getfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    char * buffer /*out*/,
    size_t size);

extern void
winf_setfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    _In_z_      const char * value);

/* --------------------------- winf_fadefield ------------------------------
 * Description:   Makes an icon unselectable (i.e. faded by WIMP).
 *
 * Parameters:    HOST_WND window_handle -- the window in which icon resides
 *                int icon_handle -- the icon to be faded.
 * Returns:       void.
 * Other Info:    Fading an already faded icon has no effect.
 *
 */

extern void
winf_fadefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

/* ---------------------------- winf_unfadefield ---------------------------
 * Description:   Makes an icon selectable (i.e. "unfades" it).
 *
 * Parameters:    HOST_WND window_handle -- the window in which icon resides
 *                int icon_handle -- the icon to be unfaded.
 * Returns:       void.
 * Other Info:    Unfading an already selectable icon has no effect
 *
 */

extern void
winf_unfadefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

/* --------------------------- winf_hidefield ------------------------------
 * Description:   Makes an icon completely invisible and unselectable.
 *
 * Parameters:    HOST_WND window_handle -- the window in which icon resides
 *                int icon_handle -- the icon to be hidden.
 * Returns:       void.
 * Other Info:    This operation is hard to undo, as it clears the icons
 *                ITEXT, ISPRITE, IBORBER, IFILLED and IBUTMASK bits.
 *
 */

extern void
winf_hidefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

extern void
winf_invertfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

/*
double field handling
*/

extern BOOL
winf_bumpdouble(
    HOST_WND window_handle,
    int hit_icon_handle,
    int value_icon_handle,
    double * var /*inout*/,
    const double * delta,
    const double * dmin,
    const double * dmax,
    int            decplaces); /* INT_MAX for no rounding */

extern void
winf_checkdouble(
    HOST_WND window_handle,
    int icon_handle,
    double * var /*inout*/,
    int * modify /*out-if-mod*/,
    const double * dmin,
    const double * dmax,
    int            decplaces); /* INT_MAX for no rounding */

extern double
winf_getdouble(
    HOST_WND window_handle,
    int icon_handle,
    const double * default_val,
    int            decplaces); /* INT_MAX for no rounding */

extern void
winf_setdouble(
    HOST_WND window_handle,
    int icon_handle,
    const double * val,
    int decplaces);

/*
int field handling
*/

extern BOOL
winf_bumpint(
    HOST_WND window_handle,
    int hit_icon_handle,
    int value_icon_handle,
    int * ivar /*inout*/,
    int idelta, int imin, int imax);

extern void
winf_checkint(
    HOST_WND window_handle,
    int value_icon_handle,
    int * ivar /*inout*/,
    int * modify /*out-if-mod*/,
    int imin, int imax);

extern int
winf_getint(
    HOST_WND window_handle,
    int icon_handle,
    int default_val);

extern void
winf_setint(
    HOST_WND window_handle,
    int icon_handle,
    int val);

/*
on/off handling
*/

extern BOOL
winf_getonoff(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle);

extern void
winf_setonoff(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    BOOL on);

extern void
winf_setonoffpair(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    BOOL on);

extern int /*icon_handle*/
winf_whichonoff(
    _HwndRef_   HOST_WND window_handle,
    int first_icon_handle,
    int last_icon_handle,
    int dft_icon_handle);

extern int /*icon_handle*/
winf_whichonoffpair(
    _HwndRef_   HOST_WND window_handle,
    int first_icon_handle);

#endif /* __cs_winf_h */

/* end of cs-winf.h */
