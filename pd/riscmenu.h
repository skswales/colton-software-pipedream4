/* riscmenu.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __pd__riscmenu_h
#define __pd__riscmenu_h

#if RISCOS

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef __cs_menu_h
#include "cs-menu.h"
#endif

extern MENU_HEAD iconbar_headline[];

/*
exported functions from riscmenu.c
*/

extern void
riscmenu_attachmenutree(
    _HwndRef_   HOST_WND window_handle);

extern void
riscmenu_buildmenutree(
    BOOL classic_m,
    BOOL short_m);

extern void
riscmenu_clearmenutree(void);

extern void
riscmenu_detachmenutree(
    _HwndRef_   HOST_WND window_handle);

extern void
riscmenu_initialise_once(void);

extern void
riscmenu_tidy_up(void);

extern void
pdfontselect_finalise(
    BOOL kill);

extern void
function__event_menu_maker(void);

extern menu
function__event_menu_filler(
    void *handle);

extern BOOL
function__event_menu_proc(
    void *handle,
    char *hit,
    BOOL submenurequest);

_Check_return_
extern BOOL
pdfontselect_is_active(void);

extern void
PrinterFont_fn(void);

extern void
InsertFont_fn(void);

extern void
LineSpacing_fn(void);

#define FONT_LEADING_UP   (31)
#define FONT_LEADING_DOWN (32)
#define FONT_LEADING      (33)
#define FONT_LEADING_AUTO (34)

#endif  /* RISCOS */

#endif  /* __pd__riscmenu_h */

/* end of riscmenu.h */
