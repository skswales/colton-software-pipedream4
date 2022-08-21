/* scdraw_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Bits exported out of PipeDream to RISC OS module */

/* requires "datafmt.h" */

/*
exported functions
*/

extern void
application_open_request(
    wimp_eventstr *event);

extern void
application_scroll_request(
    wimp_eventstr *event);

/* end of scdraw_x.h */
