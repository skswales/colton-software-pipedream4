/* cs-menu.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_menu_h
#define __cs_menu_h

#ifndef __menu_h
#include "menu.h"
#endif

extern menu
menu_new_c(const char *name, const char *descr);

extern menu
menu_new_unparsed(const char *name, const char *description);
/* Creates a menu with one entry (index 1). The description string is not parsed
   in any way, this allows the entry to contain opt or sep characters.
*/

extern BOOL
menu_extend_unparsed(menu *mm, const char *description);
/* Add one entry to the end of the menu. The description string is not parsed
   in any way, this allows the entry to contain opt or sep characters.
*/

extern void
menu_settitle(menu m, const char * title);

extern void
menu_entry_changetext(menu m, int entry, const char *text);
/* Change the text of a menu entry - only works if the entry is currently non-indirected text */

#endif /* __cs_menu_h */

/* end of cs-menu.h */
