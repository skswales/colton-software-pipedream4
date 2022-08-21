/* viewio.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
exported functions from viewio.c
*/

extern S32
view_convert(
    S32 c,
    FILE_HANDLE loadinput,
    uchar *lastch,
    uchar *type,
    uchar *justify,
    BOOL *been_ruler,
    uchar *pageoffset);

extern void
view_load_line_ended(void);

extern void
view_load_preinit(
    BOOL *been_ruler,
    BOOL inserting);

extern void
view_load_postinit(void);

extern BOOL
view_save_ruler_options(
    coord *rightmargin,
    FILE_HANDLE output);

extern BOOL
view_save_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    coord *v_chars_sofar,
    P_S32 splitlines,
    coord rightmargin);

extern BOOL
view_save_stored_command(
    const char *command,
    FILE_HANDLE output);

/* macros */

#define VIEW_LEFT_MARGIN        ((uchar) 0x0B)
#define VIEW_LINESEP            CR
#define VIEW_SOFT_SPACE         ((uchar) 0x1A)
/* need temporary character for soft space to distinguish it from PD H3 */
#define TEMP_SOFT_SPACE         TAB
#define VIEW_HIGH_UNDERLINE     ((uchar) 0x1C)      /* VIEW H1 */
#define VIEW_HIGH_BOLD          ((uchar) 0x1D)      /* VIEW H2 */

#define VIEW_STORED_COMMAND     0x80
#define VIEW_RULER              0x81

/* end of viewio.h */
