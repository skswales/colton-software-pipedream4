/* cs-winx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_winx_h
#define __cs_winx_h

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __win_h
#include "win.h"
#endif

/*
win.c
*/

#define win_settitle_c(w, newtitle) \
    win_settitle(w, de_const_cast(char *, newtitle))

#define win_IDLE_OFF (-1)

extern void
win_setmenuhi(wimp_w w, wimp_i i, void *handle);

extern void *
win_getmenuhi(wimp_w w, wimp_i i); /* 0 if not set */
/* This is for use by higher level code to manage the automatic
 * association of menus to windows. No interpretation is placed on this
 * word within this module.
*/

extern void *
win_getwindowword(wimp_w w, unsigned int offset);

extern void *
win_setwindowword(wimp_w w, unsigned int offset, void * word);

/*
cs-winx.c
*/

/* send a close request to this window */

extern void
win_send_close(wimp_w w);

/* send an open request to this window */

extern void
win_send_open(wimp_w w, BOOL immediate, const wimp_openstr * openstr);

/* send a front request to this window */

extern void
win_send_front(wimp_w w, BOOL immediate);

extern void
win_send_front_at(wimp_w w, BOOL immediate, const wimp_box * box);

extern BOOL
win_adjustbumphit(wimp_i * hitp /*inout*/, wimp_i val);

extern BOOL
win_adjustclicked(void);

extern void
win_changedtitle(wimp_w w);

extern void
win_getfield(wimp_w w, wimp_i i, char * buffer /*out*/, size_t size);

extern void
win_setfield(wimp_w w, wimp_i i, const char * value);

/* --------------------------- win_fadefield -------------------------------
 * Description:   Makes an icon unselectable (ie. faded by WIMP).
 *
 * Parameters:    wimp_w w -- the window in which icon resides
 *                wimp_i i -- the icon to be faded.
 * Returns:       void.
 * Other Info:    Fading an already faded icon has no effect.
 *
 */

extern void
win_fadefield(wimp_w w, wimp_i i);

/* ---------------------------- win_unfadefield ----------------------------
 * Description:   Makes an icon selectable (ie "unfades" it).
 *
 * Parameters:    wimp_w w -- the window in which icon resides
 *                wimp_i i -- the icon to be unfaded.
 * Returns:       void.
 * Other Info:    Unfading an already selectable icon has no effect
 *
 */

extern void
win_unfadefield(wimp_w w, wimp_i i);

/* --------------------------- win_hidefield -------------------------------
 * Description:   Makes an icon completely invisible and unselectable.
 *
 * Parameters:    wimp_w w -- the window in which icon resides
 *                wimp_i i -- the icon to be hidden.
 * Returns:       void.
 * Other Info:    This operation is hard to undo, as it clears the icons
 *                ITEXT, ISPRITE, IBORBER, IFILLED and IBUTMASK bits.
 *
 */

extern void
win_hidefield(wimp_w w, wimp_i i);

extern void
win_invertfield(wimp_w w, wimp_i i);

/*
double field handling
*/

extern BOOL
win_bumpdouble(
    wimp_w w, wimp_i hit, wimp_i val,
    double * var /*inout*/,
    const double * delta,
    const double * dmin,
    const double * dmax,
    int            decplaces); /* INT_MAX for no rounding */

extern void
win_checkdouble(
    wimp_w w, wimp_i i,
    double * var /*inout*/,
    int * modify /*out-if-mod*/,
    const double * dmin,
    const double * dmax,
    int            decplaces); /* INT_MAX for no rounding */

extern double
win_getdouble(
    wimp_w w, wimp_i i,
    const double * default_val,
    int            decplaces); /* INT_MAX for no rounding */

extern void
win_setdouble(
    wimp_w w, wimp_i i,
    const double * val,
    int decplaces);

/*
int field handling
*/

extern BOOL
win_bumpint(
    wimp_w w, wimp_i hit, wimp_i val,
    int * ivar /*inout*/,
    int idelta, int imin, int imax);

extern void
win_checkint(
    wimp_w w, wimp_i val,
    int * ivar /*inout*/,
    int * modify /*out-if-mod*/,
    int imin, int imax);

extern int
win_getint(
    wimp_w w, wimp_i i,
    int default_val);

extern void
win_setint(
    wimp_w w, wimp_i i,
    int val);

/*
on/off handling
*/

extern BOOL
win_getonoff(wimp_w w, wimp_i i);

extern void
win_setonoff(wimp_w w, wimp_i i, BOOL on);

extern void
win_setonoffpair(wimp_w w, wimp_i i, BOOL on);

extern wimp_i
win_whichonoff(wimp_w w, wimp_i first, wimp_i last, wimp_i dft);

extern wimp_i
win_whichonoffpair(wimp_w w, wimp_i first);

/*
enhancements to win.c
*/

/* NB. win copes with linking against old binaries where the function
 * prototype was a void-return -> use of win_register_event_handler is
 * deprecated
*/

typedef BOOL (*win_new_event_handler) (wimp_eventstr * e, void * handle);

extern BOOL
win_register_new_event_handler(
    wimp_w w,
    win_new_event_handler proc,
    void * handle);

extern os_error *
win_drag_box(wimp_dragstr * dr);

extern BOOL
win_register_child(wimp_w parent, wimp_w w);

extern BOOL
win_deregister_child(wimp_w w);

extern os_error *
win_close_wind(wimp_w w);

extern os_error *
win_create_wind(wimp_wind * wwp, wimp_w * wp,
                win_new_event_handler proc, void * handle);

extern os_error *
win_delete_wind(wimp_w * wp);

extern os_error *
win_open_wind(wimp_openstr * o);

/* open a window as the submenu window. any prior submenu windows are
 * sent a close event.
*/

extern os_error *
win_create_submenu(wimp_w w, int x, int y);

extern os_error *
win_create_menu(wimp_w w, int x, int y);

extern BOOL
win_submenu_query_is_submenu(wimp_w w);

extern BOOL
win_submenu_query_closed(void);

#endif /* __cs_winx_h */

/* end of cs-winx.h */
