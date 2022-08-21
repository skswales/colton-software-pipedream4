/* riscos_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __riscos_x_h
#define __riscos_x_h

#ifndef __cs_wimptx_h
#include "cs-wimptx.h" /* includes cs-wimp.h -> os.h, sprite.h; wimpt.h */
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
exported functions from riscos.c
*/

extern BOOL
riscos_adjust_clicked(void);

extern void
riscos_caret_hide(void);

extern void
riscos_caret_restore(void);

extern void
riscos_caret_set_position(
    _HwndRef_   HOST_WND window_handle,
    int x,
    int y);

extern char *
riscos_cleanupstring(
    char *str);

extern BOOL
riscos_create_document_window(void);

extern void
riscos_destroy_document_window(void);

extern void
riscos_finalise(void);

extern void
riscos_finalise_once(void);

extern void
riscos_front_document_window(
    BOOL immediate);

extern void
riscos_front_document_window_atbox(
    BOOL immediate);

extern void
riscos_initialise_once(void);

extern BOOL
riscos_initialise(void);

extern void
riscos_invalidate_document_window(void);

extern void
riscos_invert(void);

extern PCTSTR
riscos_obtainfilename(
    _In_opt_z_  PCTSTR filename);

extern BOOL
riscos_quit_okayed(
    int nmodified);

extern void
riscos_printchar(
    int ch);

extern void
riscos_rambufferinfo(
    char **bufferp,
    P_S32 sizep);

extern BOOL
riscos_readfileinfo(
    RISCOS_FILEINFO * const rip /*out*/,
    const char *name);

extern void
riscos_readtime(
    RISCOS_FILEINFO * const rip /*inout*/);

extern void
riscos_resetwindowpos(void);

extern void
riscos_send_Message_HelpReply(
    /*acked*/ WimpMessage * const user_message,
    const char * msg);

extern void
riscos_sendcellcontents(
    _InoutRef_opt_ P_GRAPHICS_LINK_ENTRY glp,
    int x_off,
    int y_off);

extern void
riscos_sendallcells(
    _InoutRef_  P_GRAPHICS_LINK_ENTRY glp);

extern void
riscos_sendsheetclosed(
    _InRef_opt_ PC_GRAPHICS_LINK_ENTRY glp);

extern void
riscos_set_wimp_colour_value_index_byte(
    _InoutRef_  P_U32 p_wimp_colour_value);

extern void
riscos_set_bg_colour_from_wimp_colour_value(
    _InVal_     U32 wimp_colour_value);

extern void
riscos_set_fg_colour_from_wimp_colour_value(
    _InVal_     U32 wimp_colour_value);

extern void
riscos_setdefwindowpos(
    S32 new_y1);

extern void
riscos_setinitwindowpos(
    S32 new_init_y1);

extern void
riscos_setnextwindowpos(
    S32 this_y1);

extern void
riscos_settitlebar(
    const char *filename);

extern void
riscos_settype(
    RISCOS_FILEINFO * const rip /*inout*/,
    int filetype);

extern void
riscos_updatearea(
    RISCOS_REDRAWPROC redrawproc,
    _HwndRef_   HOST_WND window_handle,
    int x0,
    int y0,
    int x1,
    int y1);

extern void
riscos_window_dispose(
    _Inout_     HOST_WND * const p_window_handle);

extern void
riscos_writefileinfo(
    const RISCOS_FILEINFO * const rip,
    const char *name);

extern void
print_file(
    _In_z_      PCTSTR filename);

extern void
filer_opendir(
    _In_z_      PCTSTR filename);

extern void
filer_launch(
    _In_z_      PCTSTR filename);

extern void
riscos_event_handler_report(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const event_data,
    void * handle,
    _In_z_      const char * name);

/*
exported variables
*/

extern int drag_type;
extern DOCNO drag_docno;

/*
macro definitions
*/

/* drag types */
#define NO_DRAG_ACTIVE          0
#define MARK_BLOCK              1
#define MARK_ALL_COLUMNS        2
#define MARK_ALL_ROWS           3
#define INSERTING_REFERENCE     4
#define DRAG_COLUMN_WIDTH       5
#define DRAG_COLUMN_WRAPWIDTH   6

/* Sizes of window things in OS units */

#define normal_charwidth  16
#define normal_charheight 32

#define iconbar_height  96

#define leftline_width  (dx)

#define OS_PER_INCH          (180)
#define MILLIPOINTS_PER_INCH (72000)
#define MILLIPOINTS_PER_OS   (MILLIPOINTS_PER_INCH/OS_PER_INCH) /* 400 */

/*
riscmenu.c
*/

extern void
pdfontselect_finalise(
    BOOL kill);

#endif  /* __riscos_x_h */

/* end of riscos_x.h */
