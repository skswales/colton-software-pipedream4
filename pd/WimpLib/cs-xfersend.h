/* cs-xfersend.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_xfersend_h
#define __cs_xfersend_h

#ifndef __cs_wimp_h
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __xfersend_h
#include "xfersend.h"
#endif

typedef void (*xfersend_clickproc)(dbox d, dbox_field f, int *filetypep, void *handle);
/* An unknown field has been pressed in the passed dbox:
 * you may change the filetype too.
*/

BOOL
xfersend_x(
    int filetype, char *name, int estsize,
    xfersend_saveproc, xfersend_sendproc, xfersend_printproc,
    void *savehandle,
    dbox d,
    xfersend_clickproc,
    void *clickhandle);

/* A dbox may be passed, and should have the following icons:
*/

#define xfersend_FOK    ((wimp_i) 0)        /* (action) 'ok' button */
#define xfersend_FName  ((wimp_i) 1)        /* (writeable) filename */
#define xfersend_FIcon  ((wimp_i) 2)        /* (click/drag) icon to drag */

void xfersend_set_cancel_button(dbox_field f);

#endif /* __cs_xfersend_h */

/* end of cs-xfersend.h */
