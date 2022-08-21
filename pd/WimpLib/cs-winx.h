/* cs-winx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_winx_h
#define __cs_winx_h

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __win_h
#include "win.h"
#endif

#ifndef __cs_winf_h
#include "cs-winf.h"
#endif

/*
win.c
*/

#define win_IDLE_OFF (-1)

/*
cs-winx.c (wraps win.c)
*/

_Check_return_
extern BOOL
winx_adjustclicked(void);

extern void
winx_changedtitle(
    _HwndRef_   HOST_WND window_handle);

/* send a close request to this window */

extern void
winx_send_close_window_request(
    _HwndRef_   HOST_WND window_handle);

/* send an open request to this window */

extern void
winx_send_open_window_request(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate,
    const WimpOpenWindowBlock * open_window);

/* send a front request to this window */

extern void
winx_send_front_window_request(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate);

extern void
winx_send_front_window_request_at(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate,
    const BBox * const bbox);

/*
enhancements to win.c
*/

extern os_error *
winx_drag_box(wimp_dragstr * dr);

_Check_return_
extern void *
winx_menu_get_handle(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle); /* 0 if not set */
/* This is for use by higher level code to manage the automatic
 * association of menus to windows. No interpretation is placed on this
 * word within this module.
*/

extern void
winx_menu_set_handle(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    void * handle);

/* NB. win copes with linking against old binaries where the function
 * prototype was a void-return -> use of win_register_event_handler is
 * deprecated
*/

typedef BOOL (*winx_new_event_handler) (wimp_eventstr * e, void * handle);

extern BOOL
winx_register_new_event_handler(
    _HwndRef_   HOST_WND window_handle,
    winx_new_event_handler proc,
    void * handle);

extern os_error *
winx_close_window(
    _HwndRef_   HOST_WND window_handle);

extern os_error *
winx_create_window(
    WimpWindowWithBitset * const p_window_template,
    _Out_       HOST_WND * const p_window_handle,
    winx_new_event_handler proc,
    void * handle);

extern os_error *
winx_delete_window(
    _Inout_     HOST_WND * const p_window_handle);

extern os_error *
winx_open_window(WimpOpenWindowBlock * const p_open_window_block);

extern BOOL
winx_register_child(
    _HwndRef_   HOST_WND parent_window_handle,
    _HwndRef_   HOST_WND window_handle);

extern BOOL
winx_deregister_child(
    _HwndRef_   HOST_WND window_handle);

/* open a window as the submenu window.
 * any prior submenu windows are sent a close event.
*/

extern os_error *
winx_create_submenu(
    _HwndRef_   HOST_WND window_handle,
    int x,
    int y);

extern os_error *
winx_create_menu(
    _HwndRef_   HOST_WND window_handle,
     int x,
     int y);

extern BOOL
winx_submenu_query_is_submenu(
    _HwndRef_   HOST_WND window_handle);

extern BOOL
winx_submenu_query_closed(void);

#endif /* __cs_winx_h */

/* end of cs-winx.h */
