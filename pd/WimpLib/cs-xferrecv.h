/* cs-xferrecv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_xferrecv_h
#define __cs_xferrecv_h

#ifndef __xferrecv_h
#include "xferrecv.h"
#endif

extern int
xferrecv_import_via_file(const char * filename);
/* Receives data into the given file. (NULL -> Scrap)
   Returns -1 - the next message you receive will be an insert request
*/

#endif /* __cs_xferrecv_h */

/* end of cs-xferrecv.h */
