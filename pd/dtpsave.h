/* dtpsave.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* RJM Jul 1989 */

#ifndef __fwpio_h
#define __fwpio_h

#define FWP_LINESEP         LF
#define FWP_LINELENGTH      ((uchar) 159)
#define FWP_RMARGIN         ((uchar) ']')
#define FWP_TABSTOP         ((uchar) '\x7F')
#define FWP_PICA            ((uchar) '0')
#define FWP_SPACE           ((uchar) '\x1E')
#define FWP_NOHIGHLIGHTS    ((uchar) '\x80')
#define FWP_FORMFEED        ((uchar) '\x0C')

#define FWP_PAGE_LAYOUT     '0'
#define FWP_HEADER          '1'
#define FWP_FOOTER          '2'
#define FWP_RULER           '9'

#define FWP_DELIMITER       '/'
#define FWP_ESCAPE_CHAR     ((uchar) '\x1B')
#define FWP_HIGH_PREFIX     ((uchar) '\x1B')
#define FWP_STRETCH_SPACE   ((uchar) '\x1C')
#define FWP_INDENT_SPACE    ((uchar) '\x1D')
#define FWP_SOFT_SPACE      ((uchar) '\x1E')
#define FWP_FORMAT_LINE     ((uchar) '\x1F')

#define FWP_HARD_PAGE       ((uchar) '\x0C')
#define FWP_SOFT_PAGE       ((uchar) '\x0B')

/*
exported functions
*/

extern void
fwp_change_highlights(
    uchar new_byte,
    uchar old_byte);

extern S32
fwp_convert(
    S32 c,
    FILE_HANDLE loadinput,
    uchar *type,
    uchar *justify,
    uchar *field_separator,
    uchar *h_byte,
    uchar *pageoffset);

extern BOOL
fwp_isfwpfile(
    const char *bytes);

extern void
fwp_load_preinit(
    FILE_HANDLE loadinput,
    BOOL inserting);

extern void
fwp_load_postinit(void);

extern BOOL
fwp_save_file_preamble(
    FILE_HANDLE output,
    BOOL saving_fwp,
    ROW stt_row,
    ROW end_row);

extern BOOL
fwp_save_line_preamble(
    FILE_HANDLE output,
    BOOL saving_fwp);

extern BOOL
fwp_save_slot(
    P_SLOT tslot,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    BOOL saving_fwp);

extern BOOL
dtp_save_slot(
    P_SLOT tslot,
    COL tcol,
    ROW trow,
    FILE_HANDLE output);

#endif  /* __fwpio_h */

/* end of dtpsave.h */
