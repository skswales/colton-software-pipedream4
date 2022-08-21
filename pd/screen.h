/* screen.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Header for PipeDream screen drawing modules */

/* RJM August 1987 */

/*
scinfo.c
*/

extern void
cencol(
    COL);

extern void
cenrow(
    ROW);

#define CALL_FIXPAGE 1
#define DONT_CALL_FIXPAGE 0

extern BOOLEAN
incolfixes(
    COL);

extern coord
schcsc(              /* search for col on screen */
    COL);

extern coord
schrsc(              /* search for row on screen */
    ROW);

extern ROW
fstnrx(void);

extern COL
fstncx(void);

extern coord
calcad(
    coord);

extern BOOLEAN
chkpbs(
    ROW,
    S32);

extern BOOLEAN
chkpac(
    ROW);

extern void
curosc(void);               /* check cursor on screen */

extern void
chkmov(void);

/*
cursmov.c
*/

extern void
curup(void);

extern void
curdown(void);

extern void
prevcol(void);

extern void
nextcol(void);

extern void
mark_row(
    coord);

extern void
cursdown(void);

extern void
cursup(void);

extern coord
calcoff(
    coord x_pos);

extern coord
calcoff_overlap(
    coord x_pos,
    ROW trow);

/* end of screen.h */
