/* cs-dbox.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_dbox_h
#define __cs_dbox_h

#ifndef __dbox_h
#include "dbox.h"
#endif

#define DBOX_NEW_NEW_PRESENT 1
extern dbox
dbox_new_new(const char *name, char **errorp /*out*/);
/* Build a dialog box with the given name. Use the name to find a template
 * for the dialog box, prepared using the template editor utility and available
 * as a resource for this program. If there is not enough space to do this then
 * return *errorp=0; error in window creation returns *errorp=errormessage and
 * return 0.
*/

extern void
dbox_motion_updates(dbox d);

extern BOOL
dbox_adjusthit(dbox_field *fp, dbox_field a, dbox_field b, BOOL adjustclicked);

#define dbox_field_to_icon(d, f) ((wimp_i) f)

/* 'OK' buttons are conventionally icon number 0 (RETURN 'hits' this field) */
#define dbox_OK     ((dbox_field) 0)

/* yield the current raw_eventhandling values */
extern void
dbox_raw_eventhandlers(dbox d, dbox_raw_handler_proc *handlerp, void **handlep);

extern void
dbox_note_position_on_completion(BOOL f);

extern void
dbox_noted_position_restore(void);

extern void
dbox_noted_position_save(void);

extern void
dbox_noted_position_trash(void);

#endif /* __cs_dbox_h */

/* end of cs-dbox.h */
