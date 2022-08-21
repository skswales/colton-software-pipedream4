/* cs-event.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_event_h
#define __cs_event_h

#ifndef __event_h
#include "event.h"
#endif

/* Process the given event with default action
*/
extern BOOL
event_do_process(wimp_eventstr * e);

typedef BOOL (* event_menu_ws_proc)(void *handle, char* hit, BOOL submenurequest);

_Check_return_
extern BOOL
event_attachmenumaker_to_w_i(wimp_w, wimp_i, event_menu_maker, event_menu_ws_proc, void *handle);

_Check_return_
_Ret_maybenull_
extern os_error *
event_create_menu(wimp_menustr * m, int x, int y);

_Check_return_
_Ret_maybenull_
extern os_error *
event_create_submenu(wimp_menustr * m, int x, int y);
/* for dbox/win to zap recreatepending */

_Check_return_
extern BOOL
event_is_menu_recreate_pending(void);
/* Callable inside your menu processors, it indicates whether the menu will be recreated
*/

extern void
event_read_submenupos(
    _Out_ int * const xp,
    _Out_ int * const yp);
/* Read the x, y position to open a submenu at */

extern void
event_read_menuclickdata(
    _Out_ int * const xp,
    _Out_ int * const yp,
    _Out_ wimp_w * const wp,
    _Out_ wimp_i * const ip);
/* Read the data associated with the mouse click that created the menu you're making or handling */

#endif /* __cs_event_h */

/* end of cs-event.h */
