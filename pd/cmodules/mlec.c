/* mlec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

#include "common/gflags.h"

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef __os_h
#include "os.h"
#endif

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_flex_h
#include "cs-flex.h"    /* includes flex.h */
#endif

#ifndef __cs_menu_h
#include "cs-menu.h"
#endif

#ifndef __cs_event_h
#include "cs-event.h"   /* includes event.h -> menu.h, wimp.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_akbd_h
#include "cs-akbd.h"
#endif

#ifndef __cs_xferrecv_h
#include "cs-xferrecv.h"
#endif

#ifndef __dbox_h
#include "dbox.h"       /* needed by xfersend.h */
#endif

#ifndef __cs_xfersend_h
#include "cs-xfersend.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "file.h"
#include "stringlk.h"

#ifndef __font_h
#include "font.h"
#endif

/* Font_Paint options */
#define FONT_PAINT_JUSTIFY          0x000001 /* justify text */
#define FONT_PAINT_RUBOUT           0x000002 /* rub-out box required */
                                 /* 0x000004    not used */
                                 /* 0x000008    not used */
#define FONT_PAINT_OSCOORDS         0x000010 /* OS coords supplied (otherwise 1/72000 inch) */
#define FONT_PAINT_RUBOUTBLOCK      0x000020
#define FONT_PAINT_USE_LENGTH       0x000080 /*r7*/
#define FONT_PAINT_USE_HANDLE       0x000100 /*r0*/
#define FONT_PAINT_KERNING          0x000200

/* Font_ScanString options */
#define FONT_SCANSTRING_USE_LENGTH  0x000080 /*r7*/
#define FONT_SCANSTRING_USE_HANDLE  0x000100 /*r0*/
#define FONT_SCANSTRING_KERNING     0x000200
#define FONT_SCANSTRING_FIND        0x020000

#include "mlec.h"

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

typedef struct BUFF_REGION
{
    int            start;
    int            end;
}
BUFF_REGION;

typedef struct CURSOR_POSITION
{
    int            lcol;        /* logical col                        */
    int            pcol;        /* physical col ie MIN(lcol, linelen) */
    int            row;         /* row number                         */
}
CURSOR_POSITION;

typedef struct MARK_POSITION
{
    int            col;         /* a physical col number (I think) */
    int            row;         /* row number                      */
}
MARK_POSITION;

#if SUPPORT_SELECTION || SUPPORT_LOADSAVE || SUPPORT_CUTPASTE || SUPPORT_GETTEXT
typedef struct
{
    MARK_POSITION  markstart;   /* start of marked text ie closest_to_text_home_of(cursor, selectanchor) */
    int            marklines;   /* number of line-ends in range (>=0) */
    BUFF_REGION    lower;
    BUFF_REGION    upper;
}
MARKED_TEXT;
#endif

typedef struct RGB
{
    U8 r;
    U8 g;
    U8 b;
    U8 transparent;
}
RGB, * P_RGB; typedef const RGB * PC_RGB;

typedef struct MLEC_STRUCT
{
    char            *buffptr;      /* ptr to the text buffer held in flex space                            */
    int              buffsiz;      /* the size we asked for                                                */
    BUFF_REGION      lower;        /* all characters left of, and rows above the physical cursor position  */
    BUFF_REGION      upper;        /* all characters right of, and rows below the physical cursor position */
    CURSOR_POSITION  cursor;       /* cursor row number and logical & physical column number               */
    int              maxcol;       /* length of longest line (sort of!)                                    */
    int              linecount;    /* number of line terminators ie 1 less than number of display lines    */

    int              charwidth;    /* eg 16 } graphics mode specific */
    int              termwidth;    /* on screen representation of an EOL char in a selection, typically charwidth/4 */

    wimp_w           main;
    wimp_w           pane;
#if FALSE
    int              paneposx;     /* } relative to main topleft - normally +ve */
    int              paneposy;     /* }                            normally -ve */
    int              panewidth;
    int              paneheight;
#endif
    wimp_box         paneextent;   /* work area limits */

    HOST_FONT        host_font;

    MLEC_EVENT_PROC  callbackproc;
    P_ANY            callbackhand;

#if SUPPORT_SELECTION
    BOOL             selectvalid;
    MARK_POSITION    selectanchor;
    int              selectEORcol; /* do gcol(3,selectEORcol) to show selection, repeat to remove */
#endif

#if SUPPORT_PANEMENU
    BOOL             panemenu;
#endif

    int              attributes[MLEC_ATTRIBUTE_MAX];
}
MLEC_STRUCT;

#if SUPPORT_LOADSAVE
#define lineterm_CR    "\r"
#define lineterm_LF    "\n"
#define lineterm_CRLF  "\r\n"
#define lineterm_LFCR  "\n\r"
#endif

#if FALSE
#if SUPPORT_CUTPASTE
extern MLEC_HANDLE paste;      /* The paste buffer */
#endif
#endif

#define TRACE_MODULE_MLEC 0

/* internal routines */

static void
mlec__mouse_click(
    MLEC_HANDLE mlec,
    wimp_mousestr *mousep);

static void
mlec__redraw_loop(
    MLEC_HANDLE mlec);

static void
mlec__cursor_down(
    MLEC_HANDLE mlec);

static void
mlec__cursor_left(
    MLEC_HANDLE mlec);

static void
mlec__cursor_right(
    MLEC_HANDLE mlec);

static void
mlec__cursor_up(
    MLEC_HANDLE mlec);

static void
mlec__cursor_lineend(
    MLEC_HANDLE mlec);

static void
mlec__cursor_linehome(
    MLEC_HANDLE mlec);

static void
mlec__cursor_tab_left(
    MLEC_HANDLE mlec);

static void
mlec__cursor_textend(
    MLEC_HANDLE mlec);

static void
mlec__cursor_texthome(
    MLEC_HANDLE mlec);

static void
mlec__cursor_wordleft(
    MLEC_HANDLE mlec);

static void
mlec__cursor_wordright(
    MLEC_HANDLE mlec);

static int
mlec__insert_tab(
    MLEC_HANDLE mlec);

static void
mlec__delete_left(
    MLEC_HANDLE mlec);

static void
mlec__delete_right(
    MLEC_HANDLE mlec);

static void
mlec__delete_line(
    MLEC_HANDLE mlec);

static void
mlec__delete_lineend(
    MLEC_HANDLE mlec);

static void
mlec__delete_linehome(
    MLEC_HANDLE mlec);

void
scroll_until_cursor_visible(
    MLEC_HANDLE mlec);

void
show_caret(
    MLEC_HANDLE mlec);

void
build_caretstr(
    MLEC_HANDLE mlec,
    wimp_caretstr *carrotp);

static void
move_cursor(
    MLEC_HANDLE mlec,
    int col,
    int row);

void
word_left(
    MLEC_HANDLE mlec,
    MARK_POSITION *startp);

void
word_limits(
    MLEC_HANDLE mlec,
    MARK_POSITION *startp,
    MARK_POSITION *endp);

static void
render_line(
    MLEC_HANDLE mlec,
    int lineCol,
    int x,
    int y,
    wimp_box screen,
    char **cpp,
    char *limit);

static void
report_error(
    MLEC_HANDLE mlec,
    int err);

static int checkspace_deletealltext(
    MLEC_HANDLE mlec,
    int size);

static int checkspace_delete_selection(
    MLEC_HANDLE mlec,
    int size);

void force_redraw_eoline(
    MLEC_HANDLE mlec);

void force_redraw_eotext(
    MLEC_HANDLE mlec);

#if SUPPORT_SELECTION

extern void
mlec__selection_adjust(
    MLEC_HANDLE mlec,
    int col,
    int row);

extern void
mlec__selection_clear(
    MLEC_HANDLE mlec);

extern void
mlec__selection_delete(
    MLEC_HANDLE mlec);

static void
mlec__select_word(
    MLEC_HANDLE mlec);

static void
mlec__drag_start(
    MLEC_HANDLE mlec);

static void
mlec__drag_complete(
    MLEC_HANDLE mlec,
    wimp_box *dragboxp);

null_event_proto(static, mlec__drag_null_handler); /* callback function */

void
clear_selection(
    MLEC_HANDLE mlec);

void
delete_selection(
    MLEC_HANDLE mlec);

#define remove_selection(mlec) if(mlec->selectvalid) { mlec__selection_delete(mlec); return; }

BOOL range_is_selection(
    MLEC_HANDLE mlec,
    MARKED_TEXT *range);

void find_offset(
    MLEC_HANDLE mlec,
    MARK_POSITION *find,
    int *offsetp);

static void mlec__update_loop(
    MLEC_HANDLE mlec,
    MARK_POSITION mark1,
    CURSOR_POSITION mark2);

void show_selection(
    MLEC_HANDLE mlec,
    MARK_POSITION markstart,
    MARK_POSITION markend,
    int orgx,
    int orgy,
    wimp_box screenBB);

#else
#define clear_selection(mlec)  /* Ain't got one */
#define delete_selection(mlec) /* */
#define remove_selection(mlec)
#endif

#if SUPPORT_LOADSAVE

int
mlec__atcursor_load(
    MLEC_HANDLE mlec,
    char *filename);

int
mlec__alltext_save(
    MLEC_HANDLE mlec,
    char *filename,
    FILETYPE_RISC_OS filetype,
    char *lineterm);

int
mlec__selection_save(
    MLEC_HANDLE mlec,
    char *filename,
    FILETYPE_RISC_OS filetype,
    char *lineterm);

#endif

#if SUPPORT_CUTPASTE

int
mlec__atcursor_paste(MLEC_HANDLE mlec);

int
mlec__selection_copy(MLEC_HANDLE mlec);

int
mlec__selection_cut(MLEC_HANDLE mlec);

#endif

#if SUPPORT_LOADSAVE || SUPPORT_CUTPASTE || SUPPORT_GETTEXT

typedef union XFER_HANDLE
{
    FILE_HANDLE f;      /* a file           */
    MLEC_HANDLE p;      /* the paste buffer */
    struct
    {
    char *ptr;
    int   siz;
    int   len;
    }           s;      /* a string         */
}
XFER_HANDLE, * P_XFER_HANDLE;

void
range_is_alltext(
    MLEC_HANDLE mlec,
    MARKED_TEXT *range);

int
text_in(
    MLEC_HANDLE mlec,
    char *filename,
    int (*openp)(char *filename, P_FILETYPE_RISC_OS filetypep, int *filesizep, P_XFER_HANDLE xferhandlep),
    int (*readp)(P_XFER_HANDLE xferhandlep, char *dataptr, int datasize),
    int (*closep)(P_XFER_HANDLE xferhandlep)
    );

int
text_out(
    MLEC_HANDLE mlec,
    P_XFER_HANDLE xferhandlep,
    MARKED_TEXT range,
    char *lineterm,
    int (*sizep)(P_XFER_HANDLE xferhandlep, int xfersize),
    int (*writep)(P_XFER_HANDLE xferhandlep, char *dataptr, int datasize),
    int (*closep)(P_XFER_HANDLE xferhandlep)
    );

#endif

#if SUPPORT_LOADSAVE

static int
file_read_open(
    char *filename,
    P_FILETYPE_RISC_OS filetypep/*out*/,
    int *filesizep/*out*/,
    P_XFER_HANDLE xferhandlep/*out*/);

static int
file_read_getblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize);

static int
file_read_close(
    P_XFER_HANDLE xferhandlep);

static int
file_write_open(
    P_XFER_HANDLE xferhandlep/*out*/,
    char *filename,
    FILETYPE_RISC_OS filetype);

static int
file_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize);

static int
file_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize);

static int
file_write_close(
    P_XFER_HANDLE xferhandlep);

#endif

#if SUPPORT_CUTPASTE

static int
paste_read_open(
    char *filename,
    P_FILETYPE_RISC_OS filetypep/*out*/,
    int *filesizep/*out*/,
    P_XFER_HANDLE xferhandlep/*out*/);

static int
paste_read_getblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize);

static int
paste_read_close(
    P_XFER_HANDLE xferhandlep);

static int
paste_write_open(
    P_XFER_HANDLE xferhandlep/*out*/);

static int
paste_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize);

static int
paste_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize);

static int
paste_write_close(
    P_XFER_HANDLE xferhandlep);

static MLEC_HANDLE paste = NULL;        /* The paste buffer is created automatically by paste_write_open() */
                                        /* when mlec__selection_copy() or mlec__selection_cut() are used/  */

#endif

#if SUPPORT_GETTEXT

static int
string_write_open(
    P_XFER_HANDLE xferhandlep/*out*/,
    char *buffptr,
    int buffsiz);

static int
string_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize);

static int
string_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize);

static int
string_write_close(
    P_XFER_HANDLE xferhandlep);

#endif

#if SUPPORT_PANEMENU

static void
mlec__event_menu_maker(
    const char *menu_title);

menu
mlec__event_menu_filler(
    void *handle);

BOOL
mlec__event_menu_proc(
    void *handle,
    char *hit,
    BOOL submenurequest);

BOOL
mlec__saveall(
    /*const*/ char *filename,
    void *handle);

BOOL
mlec__saveselection(
    /*const*/ char *filename,
    void *handle);

#endif

#define return_if_no_pane(mlec)      if(mlec->pane == window_NULL) return;
#define reject_if_paste_buffer(mlec) if(mlec == paste) return(create_error(MLEC_ERR_INVALID_PASTE_OP))

#define XOS_WriteN            (0x00020046)

#define DEFAULT_MLEC_BUFFSIZE 1024

#define SPACE 32
#define window_NULL 0

#define TAB_MASK 3      /* ie insert 1..4 spaces */

_Check_return_
extern int
mlec_attribute_query(
    /*_In_*/    MLEC_HANDLE mlec,
    _InVal_     MLEC_ATTRIBUTE attribute)
{
    assert((attribute >= 0) && (attribute < MLEC_ATTRIBUTE_MAX));

    return(mlec->attributes[attribute]);
}

extern void
mlec_attribute_set(
    /*_Inout_*/ MLEC_HANDLE mlec,
    _InVal_     MLEC_ATTRIBUTE attribute,
    _InVal_     int value)
{
    assert((attribute >= 0) && (attribute < MLEC_ATTRIBUTE_MAX));

    mlec->attributes[attribute] = value;
}

_Check_return_
static HOST_FONT
mlec_get_host_font(void)
{
    HOST_FONT host_font = HOST_FONT_NONE;

    /*U32 size_x = 0;*/
    U32 size_y = 12;

    /* RISC OS font manager needs 16x fontsize */
    /*U32 x16_size_x = 16 * 0;*/
    U32 x16_size_y = 16 * size_y;

    /* c.f. host_font_find() in Fireworkz */
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[1] = (int) "\\F" "DejaVuSans.Mono" "\\E" "Latin1";
    rs.r[2] = /*x16_size_x ? x16_size_x :*/ x16_size_y;
    rs.r[3] = x16_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = (_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
    {
        host_font = (HOST_FONT) rs.r[0];
        return(host_font);
    }

    rs.r[1] = (int) "\\F" "Corpus.Medium" "\\E" "Latin1";
    rs.r[2] = /*x16_size_x ? x16_size_x :*/ x16_size_y;
    rs.r[3] = x16_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = (_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
    {
        host_font = (HOST_FONT) rs.r[0];
        return(host_font);
    }

    return(HOST_FONT_NONE);
}

/******************************************************************************
*
* Create the data structures for an mlec (multi-line edit control).
*
******************************************************************************/

int
mlec_create(
    MLEC_HANDLE *mlecp)
{
    static const RGB rgb_background = { 0xFF, 0xFF, 0xFF }; /* white */
    static const RGB rgb_foreground = { 0x00, 0x00, 0x00 }; /* black */

    STATUS status;
    MLEC_HANDLE mlec;
    int buffsiz = DEFAULT_MLEC_BUFFSIZE;

    trace_0(TRACE_MODULE_MLEC, "mlec_create");

    if(NULL != (*mlecp = mlec = al_ptr_alloc_elem(MLEC_STRUCT, 1, &status)))
    {
        if(flex_alloc((flex_ptr)&mlec->buffptr, buffsiz))
        {
            /* empty buffer */
            mlec->lower.start = mlec->lower.end = 0;
            mlec->upper.start = mlec->upper.end = mlec->buffsiz = buffsiz;
            mlec->maxcol      = 0;
            mlec->linecount   = 0;

            /* home cursor */
            mlec->cursor.lcol = mlec->cursor.pcol = mlec->cursor.row = 0;

            mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]  = 4; /*2;*/ /*32;*/
            mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]   = 4; /*16;*/

            mlec->host_font = mlec_get_host_font();

            if(HOST_FONT_NONE != mlec->host_font)
                mlec->charwidth = 18; /* fixed pitch font */
            else
                mlec->charwidth = 16; /* System font */

            mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT]  = 32;

            mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]      = mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
            mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTPOS] = mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] + 4;
            mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTNEG] = 4;

            mlec->attributes[MLEC_ATTRIBUTE_BG_RGB] = * (PC_S32) &rgb_background;
            mlec->attributes[MLEC_ATTRIBUTE_FG_RGB] = * (PC_S32) &rgb_foreground;

            mlec->termwidth   = mlec->charwidth/4;
#if FALSE
            mlec->termwidth   = mlec->charwidth * 3; /*>>>made bigger for testing*/
#endif

            mlec->main        = window_NULL;
            mlec->pane        = window_NULL;     /* ie not attached */
#if FALSE
         /* done by attach */
            mlec->paneposx    =  32;
            mlec->paneposy    = -32;
            mlec->panewidth   =  500;
            mlec->paneheight  =  250;
#endif
            mlec->callbackproc = NULL;
            mlec->callbackhand = NULL;

#if SUPPORT_SELECTION
            /* No initial selection */
            mlec->selectvalid  = FALSE;
            mlec->selectEORcol = 7;     /*>>>wrong! should be realcolour_for_wimpcol(0) EOR realcolour_for_wimpcol(7) */
#endif

#if SUPPORT_PANEMENU
            mlec->panemenu     = FALSE;
#endif

            return(STATUS_OK);
        }

        al_ptr_free(mlec);
    }

    *mlecp = NULL;
    return(status);
}

/******************************************************************************
*
* Destroy the data structures of an mlec (multiline edit control).
*
******************************************************************************/

void
mlec_destroy(
    MLEC_HANDLE *mlecp)
{
    MLEC_HANDLE mlec = *mlecp;

    if(mlec)
    {
        /*>>>I suppose we could call detach???*/

        if(HOST_FONT_NONE != mlec->host_font)
            font_lose(mlec->host_font);

        flex_dispose((flex_ptr)&mlec->buffptr);

        al_ptr_free(mlec);

        *mlecp = NULL;
    }
}

/******************************************************************************
*
* Attach an mlec to a pair of windows.
*
* The mlec outputs its text to a 'pane' window attached to some other 'main' window
* and receives characters typed when the pane window has the input focus.
* This call registers the windows with the mlec and installs an event handler and
* an optional menu handler for the pane.
*
******************************************************************************/

void
mlec_attach(
    MLEC_HANDLE mlec,
    wimp_w main_win_handle,
    wimp_w pane_win_handle,
    wimp_box paneWorkArea,
    const char *menu_title)
{
    mlec->main       = main_win_handle;
    mlec->pane       = pane_win_handle;
#if FALSE
    mlec->paneposx   = paneBB.x0 - mainBB.x0;
    mlec->paneposy   = paneBB.y1 - mainBB.y1;
    mlec->panewidth  = paneBB.x1 - paneBB.x0;
    mlec->paneheight = paneBB.y1 - paneBB.y0;
#endif
    mlec->paneextent = paneWorkArea;

    win_register_new_event_handler(pane_win_handle, mlec__event_handler, mlec);

#if SUPPORT_PANEMENU
    if((menu_title != NULL) && (mlec != paste))
    {   /* Construct the menu skeleton, and attach the filler & process routines to the window */
        mlec__event_menu_maker(menu_title);
        if(event_attachmenumaker_to_w_i(
                pane_win_handle, (wimp_i) -1,
                mlec__event_menu_filler,    /* Fade/tick the menu entries       */
                mlec__event_menu_proc,      /* Decode and action the menu entry */
                (P_ANY) mlec))
        {
            mlec->panemenu = TRUE;
        }
    }
#endif
}

/******************************************************************************
*
* Deregister the windows with the mlec and un-install the panes event handler
* and menu handler.
*
******************************************************************************/

void
mlec_detach(
    MLEC_HANDLE mlec)
{
#if SUPPORT_PANEMENU
    if(mlec->panemenu)
    {   /* Remove attached the filler & process routines from the window */
        (void) event_attachmenumaker_to_w_i(mlec->pane, (wimp_i) -1, NULL, NULL, NULL);
        mlec->panemenu = FALSE;
    }
#endif
    win_register_new_event_handler(mlec->pane, NULL, NULL);

    mlec->main = window_NULL;
    mlec->pane = window_NULL;   /* ie not attached */
}

/******************************************************************************
*
* Attach an event handler to an mlec.
*
* Events sent to an mlec pane window (open, redraw, mouse_clicks, key presses etc)
* are normally processed by mlec__event_handler. This routine allows an alternate
* handler to be installed to process SOME of these events, or some unknown events.
*
* Typically this is used to allow the owner of the 'main' window to open/resize it
* when the 'pane' window is openned/resized.
*
******************************************************************************/

void
mlec_attach_eventhandler(
    MLEC_HANDLE mlec,
    MLEC_EVENT_PROC proc,
    P_ANY handle,
    S32 add)
{
    if(add)
    {
        mlec->callbackproc = proc;
        mlec->callbackhand = handle;
    }
    else
    {
        mlec->callbackproc = NULL;
        mlec->callbackhand = NULL;
    }
}

#if SUPPORT_GETTEXT

/******************************************************************************
*
* buffptr   pointer to buffer that receives the text
* buffsize  its size
*
* NB The string returned is NULL terminated (so max strlen actually buffsize-1).
*    The lineterm used (fourth param of text_out) is lineterm_LF.
*    If the buffer is too small, an error is returned, with buffer contents undefined.
*
******************************************************************************/

int
mlec_GetText(
    MLEC_HANDLE mlec,
    char *buffptr,
    int buffsize)
{
    int         err;
    MARKED_TEXT range;
    XFER_HANDLE handle;

    range_is_alltext(mlec, &range);

    if((err = string_write_open(&handle, buffptr, buffsize)) >= 0)
        err = text_out(mlec, &handle, range, lineterm_LF, string_write_size, string_write_putblock, string_write_close);
    return(err);
}

int
mlec_GetTextLen(
    MLEC_HANDLE mlec)
{
    MARKED_TEXT range;

    range_is_alltext(mlec, &range);

    /* WARNING the calculation assumes the line terminator specified by mlec_GetText will expand to ONE character */

    return((range.lower.end - range.lower.start) + (range.upper.end - range.upper.start));      /* excluding terminator */
}

static int
string_write_open(
    P_XFER_HANDLE xferhandlep/*out*/,
    char *buffptr,
    int buffsiz)
{
    xferhandlep->s.ptr = buffptr;
    xferhandlep->s.siz = buffsiz;
    xferhandlep->s.len = 0;

    return(0);
}

static int
string_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize)
{
    /* xfersize is the total number of printable chars & lineterm chars that will be output */
    /* by text_out to string_write_putblock, it does NOT include any end-of-text char       */

    if(xferhandlep->s.siz > (xferhandlep->s.len + xfersize))    /* > not >= to allow for eot */
        return(0);

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF));
}

static int
string_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize)
{
    if(xferhandlep->s.siz > (xferhandlep->s.len + datasize))
    {
        char *ptr = &(xferhandlep->s.ptr[xferhandlep->s.len]);
        int   i;

        for(i = 0; i < datasize; i++)
            ptr[i] = *dataptr++;

        xferhandlep->s.len += datasize;
        return(0);
    }

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF)); /* shouldn't happen, string_write_size should catch the problem */
}

static int
string_write_close(
    P_XFER_HANDLE xferhandlep)
{
    /* terminate the string */

    if(xferhandlep->s.siz > xferhandlep->s.len)
    {
        xferhandlep->s.ptr[xferhandlep->s.len] = '\0';
        return(0);
    }

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF)); /* shouldn't happen, string_write_size should catch the problem */
}

#endif

int
mlec_SetText(
    MLEC_HANDLE mlec,
    char *text)
{
    int err;

    trace_0(TRACE_MODULE_MLEC, "mlec_SetText");

    if((err = checkspace_deletealltext(mlec, strlen(text))) >= 0)
    {
        /* current contents flushed only when we know the new text will fit */

        /*>>>what about clearing the selection???*/

        /* empty buffer */
        mlec->lower.end = mlec->lower.start;
        mlec->upper.start = mlec->upper.end;
        mlec->maxcol    = 0;
        mlec->linecount = 0;

        /* home cursor */
        mlec->cursor.lcol = mlec->cursor.pcol = mlec->cursor.row = 0;

#if SUPPORT_SELECTION
        /* ditch selection */
        mlec->selectvalid = FALSE;
#endif
        force_redraw_eotext(mlec);      /* cursor (lcol,pcol,row) now (0,0,0), */
                                        /* so this redraws whole window        */
        err = mlec__insert_text(mlec, text);  /* space test WILL be successful */
    }

    trace_3(TRACE_MODULE_MLEC, "exit mlec_SetText with cursor (%d,%d,%d)", mlec->cursor.row,mlec->cursor.lcol,mlec->cursor.pcol);
    return(err);
}

/******************************************************************************
*
* Event handler for a multi-line edit control.
*
* This is attached to the editor display window, which is usually
* a pane window linked to a normal window or to a dialog box.
* The event handler processes mouse-clicks, key-presses, messages
* and redraw requests etc, sent to the pane window.
*
******************************************************************************/

BOOL mlec__event_handler(wimp_eventstr *e, void *handle)
{
    MLEC_HANDLE mlec = (MLEC_HANDLE)handle;

    /* Process the event */
    switch (e->e)
    {
    case wimp_EOPEN:                                                /* 2 */
        trace_0(TRACE_MODULE_MLEC, "** Open_Window_Request on mlec pane window **");
        if(mlec->callbackproc)
        {
            if(mlec_event_openned == (*mlec->callbackproc)(Mlec_IsOpen, mlec->callbackhand, &e->data.o))
                break;
          /*else             */
          /*    drop into... */
        }
        wimp_open_wind(&e->data.o);
        break;

    case wimp_EREDRAW:                                              /* 1 */
        mlec__redraw_loop(mlec);    /* redraw text & selection */
        break;

    case wimp_ECLOSE:                                               /* 3 */
#if TRUE
        trace_0(TRACE_MODULE_MLEC, "** Close_Window_Request on mlec pane window **");
        if(mlec->callbackproc)
        {
            if(mlec_event_closed == (*mlec->callbackproc)(Mlec_IsClose, mlec->callbackhand, &e->data.close))
                break;
          /*else             */
          /*    drop into... */
        }
        win_close_wind(e->data.close.w);
      /*wimp_close_wind(e->data.close.w);*/
#else
        return(FALSE);              /* default action */
#endif
        break;

    case wimp_EBUT:                                                 /* 6 */
        if((int)e->data.but.m.i == -1)      /* work area background */
            mlec__mouse_click(mlec, &e->data.but.m);
        else
        {
            if(mlec->callbackproc)
                (*mlec->callbackproc)(Mlec_IsClick, mlec->callbackhand, &e->data.but.m);
        }
        break;

#if SUPPORT_SELECTION
    /* We don't receive dragging-null-events this way, mlec__drag_start() passes mlec__drag_null_handler() */
    /* to Null_EventHandler(), null events are then passed directly to us.                                 */
    /*                                                                                                     */
  /*case wimp_ENULL:*/                                              /* 0 */
  /*    break;      */
#endif

#if SUPPORT_SELECTION
    case wimp_EUSERDRAG:                                            /* 7 */
        /* Returned when a 'User_Drag' operation (started by win_drag_box) completes */
        mlec__drag_complete(mlec, &e->data.dragbox);
        break;
#endif
    case wimp_EKEY:                                                 /* 8 */
        {
        int err = 0;

        trace_1(TRACE_MODULE_MLEC, "** Key_Pressed on EditBox pane window, key code=%d **",e->data.key.chcode);
        switch(e->data.key.chcode)                                        /*>>>this is a load of crap, but it will do for now*/
        {
        case akbd_LeftK:                      mlec__cursor_left     (mlec); break;
        case akbd_RightK:                     mlec__cursor_right    (mlec); break;
        case akbd_DownK:                      mlec__cursor_down     (mlec); break;
        case akbd_UpK:                        mlec__cursor_up       (mlec); break;

        case akbd_LeftK  + akbd_Ctl:          mlec__cursor_linehome (mlec); break;
        case akbd_RightK + akbd_Ctl:          mlec__cursor_lineend  (mlec); break;
        case akbd_DownK  + akbd_Ctl:          mlec__cursor_textend  (mlec); break;
        case akbd_UpK    + akbd_Ctl:          mlec__cursor_texthome (mlec); break;

        case akbd_LeftK  + akbd_Sh:           mlec__cursor_wordleft (mlec); break;
        case akbd_RightK + akbd_Sh:           mlec__cursor_wordright(mlec); break;

        case akbd_TabK:                 err = mlec__insert_tab      (mlec); break;
        case akbd_TabK + akbd_Sh:             mlec__cursor_tab_left (mlec); break;

        case akbd_CopyK:                      mlec__delete_right    (mlec); break;
        case akbd_CopyK           + akbd_Ctl: mlec__delete_line     (mlec); break;
        case akbd_CopyK + akbd_Sh:            mlec__delete_lineend  (mlec); break;
        case akbd_CopyK + akbd_Sh + akbd_Ctl: mlec__delete_linehome (mlec); break;

        case cs_akbd_BackspaceK:              mlec__delete_left     (mlec); break;

#ifdef SUPPORT_CUTPASTE
        /* SKS - no menus on gr_editt window so lets have some keyboard shortcuts */
        case cs_akbd_DeleteK:
            if(akbd_pollsh())
                err = mlec__selection_cut(mlec);
            else
                      mlec__delete_left  (mlec);
            break;
#else
        case akbd_DeleteK:                    mlec__delete_left     (mlec); break;
#endif

#ifdef SUPPORT_CUTPASTE
        /* SKS - no menus on gr_editt window so lets have some keyboard shortcuts */
        case akbd_InsertK + akbd_Sh:    err = mlec__atcursor_paste(mlec); break;
        case akbd_InsertK + akbd_Ctl:   err = mlec__selection_copy(mlec); break;
#endif

        case 13 :
            if(!akbd_pollctl())
            {
                if(mlec->callbackproc)
                {
                    if(mlec_event_return == (*mlec->callbackproc)(Mlec_IsReturn, mlec->callbackhand, NULL))
                        break;
                  /*else             */
                  /*    drop into... */
                }
            }

            err = mlec__insert_newline(mlec);
            break;

        case 27 :
            if(mlec->callbackproc)
            {
                if(mlec_event_escape == (*mlec->callbackproc)(Mlec_IsEscape, mlec->callbackhand, NULL))
                    break;
              /*else             */
              /*    drop into... */
            }

            /* processed - we ignored it! */
            break;

        default:
            if(((e->data.key.chcode >= 32)  && (e->data.key.chcode <127)) ||
               ((e->data.key.chcode >= 128) && (e->data.key.chcode <256))
              )
                err = mlec__insert_char(mlec,e->data.key.chcode);
            else
                return(FALSE);
            break;
        }

        /* Any error is the result of direct user interaction with the mlec, */
        /* so report the error here, cos there is no caller to return it to. */
        if(err < 0)
            report_error(mlec, err);
        }
        break;

    case wimp_ESEND:
    case wimp_ESENDWANTACK:                                         /* 17 */
        trace_1(TRACE_MODULE_MLEC, "action is %d",e->data.msg.hdr.action);           /* 18 */
        switch(e->data.msg.hdr.action)
        {
#if SUPPORT_LOADSAVE
        case wimp_MDATALOAD:   /* File dragged from file viewer, dropped on our window */
        case wimp_MDATAOPEN:   /* File double clicked in file viewer */
            {
            char *filename;

            if(FILETYPE_TEXT == xferrecv_checkinsert(&filename))
            {
                int err = mlec__atcursor_load(mlec, filename);
                xferrecv_insertfileok();  /* a badly named fn, means finished/given up, */
                                          /* so delete any scrap file                   */
                if(err < 0)
                    report_error(mlec, err);    /* Report the error here, cos there is no caller to return it to. */
            }
            break;
            }

        case wimp_MDATASAVE:    /* File dragged from another application, dropped on our window */
            {
            int size;

            if(FILETYPE_TEXT == xferrecv_checkimport(&size))
            {
                /* we don't support RAM xfers, this will cause the data to be saved  */
                /* in a scrap file, we are then sent a wimp_MDATALOAD command for it */
                xferrecv_import_via_file(NULL);
            }
            break;
            }
#endif
        default:
            return(FALSE);
            break;
        }
        break;

    default:
        return(FALSE);
        break;
    }

    /* done something, so... */
    return(TRUE);
}

/*>>>split out key code and have mlec__key_press*/

/******************************************************************************
*
* Process mouse click, double-click and drag events.
*
******************************************************************************/

static void
mlec__mouse_click(
    MLEC_HANDLE mlec,
    wimp_mousestr *mousep)
{
    wimp_wstate r;
    int         x,y, orgx,orgy;

    trace_3(TRACE_MODULE_MLEC, "** Mouse_Click on EditBox pane window at (%d,%d), state &%x **",mousep->x,mousep->y,mousep->bbits);

    wimp_get_wind_state(mousep->w, &r);
    orgx = r.o.box.x0 - r.o.scx; orgy = r.o.box.y1 - r.o.scy;
    x = mousep->x - orgx;       /* mouse position relative to */
    y = mousep->y - orgy;       /* window (ie EditBox) origin */

    trace_2(TRACE_MODULE_MLEC, "(%d,%d)",x,y);

    x = ( x - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] +8 ) / 16;                 /* 8=half char width, 16=char width*/
    y = (-y - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] -1) / mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];

    /* Decode the mouse buttons */
    if(mousep->bbits & wimp_BCLICKLEFT)         /* 0x400 Single 'select' */
    {
        mlec__cursor_setpos(mlec, x,y);
        mlec_claim_focus(mlec);
    }
#if SUPPORT_SELECTION
    else
    if(mousep->bbits & wimp_BDRAGLEFT)          /* 0x040 Long   'select' */
    {
        mlec__cursor_setpos(mlec, x,y);
        mlec_claim_focus(mlec);
        mlec__drag_start(mlec);
    }
    else
    if(mousep->bbits & wimp_BLEFT)              /* 0x004 Double 'select' */
    {
        mlec__cursor_setpos(mlec, x,y);
        mlec__select_word(mlec);
        mlec_claim_focus(mlec);
    }
    else
    if(mousep->bbits & wimp_BCLICKRIGHT)        /* 0x100 Single 'adjust' */
    {
        /* Alter selection, (will create one if needed, starting at cursor position) */
        mlec__selection_adjust(mlec, x,y);
        /* NB don't claim focus */
    }
    else
    if(mousep->bbits & wimp_BDRAGRIGHT)         /* 0x010 Long   'adjust' */
    {
        mlec__selection_adjust(mlec, x,y);
        mlec__drag_start(mlec);
        /* NB don't claim focus */
    }
    else
    if(mousep->bbits & wimp_BRIGHT)             /* 0x001 Double 'adjust' */
    {
        mlec__selection_adjust(mlec, x,y);
        mlec__select_word(mlec);
        /* NB don't claim focus */
    }
#endif
}

typedef union RISCOS_PALETTE_U
{
    unsigned int word;

    struct RISCOS_PALETTE_U_BYTES
    {
        char gcol;
        char red;
        char green;
        char blue;
    } bytes;
}
RISCOS_PALETTE_U;

static void
host_setfontcolours_for_mlec(
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background)
{
    RISCOS_PALETTE_U rgb_fg;
    RISCOS_PALETTE_U rgb_bg;
    _kernel_swi_regs rs;

    rgb_fg.bytes.gcol  = 0;
    rgb_fg.bytes.red   = p_rgb_foreground->r;
    rgb_fg.bytes.green = p_rgb_foreground->g;
    rgb_fg.bytes.blue  = p_rgb_foreground->b;

    rgb_bg.bytes.gcol  = 0;
    rgb_bg.bytes.red   = p_rgb_background->r;
    rgb_bg.bytes.green = p_rgb_background->g;
    rgb_bg.bytes.blue  = p_rgb_background->b;

    rs.r[0] = 0;
    rs.r[1] = rgb_bg.word;
    rs.r[2] = rgb_fg.word;
    rs.r[3] = 14; /* max offset - some magic number, !Draw uses 14 */

    (void) _kernel_swi(ColourTrans_SetFontColours, &rs, &rs);
}

/******************************************************************************
*
* Redraw any invalid rectangles in the pane window
*
* This renders text and highlights any selection
*
******************************************************************************/

static void mlec__redraw_loop(MLEC_HANDLE mlec)
{
    BOOL            more;
    wimp_redrawstr  r;
    char           *lower_start;
    char           *lower_row;
    char           *lower_end;
    char           *upper_start;
    char           *upper_end;

    char *linestart;
    char *lineend;
    char *ptr;

#if SUPPORT_SELECTION
    MARK_POSITION markstart, markend;   /* only valid if mlec->selectvalid == TRUE */

    markstart.col = 0; markstart.row = 0; /* keep dataflower happy */
    markend.col   = 0; markend.row   = 0;

    if(mlec->selectvalid)
    {
        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        {
            markstart = mlec->selectanchor; markend.col = mlec->cursor.pcol; markend.row = mlec->cursor.row;
        }
        else
        {
            markstart.col = mlec->cursor.pcol; markstart.row = mlec->cursor.row; markend = mlec->selectanchor;
        }
    }
#endif

    trace_0(TRACE_MODULE_MLEC, "** Redraw_Window_Request on EditBox pane window - mlec__redraw_loop called **");

    lower_start = &mlec->buffptr[mlec->lower.start];                    /* first character                         */
    lower_row   = &mlec->buffptr[mlec->lower.end - mlec->cursor.pcol];  /* character at (0,cursor.row)             */
    lower_end   = &mlec->buffptr[mlec->lower.end];                      /* 1 byte past character to left of cursor */
    upper_start = &mlec->buffptr[mlec->upper.start];                    /* character to right of cursor            */
    upper_end   = &mlec->buffptr[mlec->upper.end];                      /* 1 byte past last character              */

  /* Start the redraw */
  r.w = mlec->pane;
  wimpt_noerr(wimp_redraw_wind(&r, &more));

 trace_4(TRACE_MODULE_MLEC, "wimp_redraw_wind returns: (%d,%d,%d,%d) ",r.box.x0,r.box.y0,r.box.x1,r.box.y1);
 trace_2(TRACE_MODULE_MLEC, "(%d,%d) ",r.scx,r.scy);
 trace_4(TRACE_MODULE_MLEC, "(%d,%d,%d,%d)",r.g.x0,r.g.y0,r.g.x1,r.g.y1);

    /* Do the redraw loop */
    while (more)
    {
        wimp_box screenBB;              /* bounding box of a region of the screen that needs updating */
        wimp_box lineBB;                /* partial bounding box of a line considered for rendering    */
        wimp_box cursor;                /* all characters on cursor row, to the left of the cursor    */

        int lineCol, lineRow;

        int orgx=r.box.x0-r.scx;       /* graphics units */
        int orgy=r.box.y1-r.scy;       /*>>>> could be put outside loop?*/

        RGB rgb_foreground, rgb_background;

        /* bounding box of characters 0..cursor.pcol-1 on row cursor.row */

        cursor.x0 = orgx      + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT];
        cursor.x1 = cursor.x0 + mlec->cursor.pcol * mlec->charwidth;
        cursor.y1 = orgy      - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * mlec->cursor.row;
        cursor.y0 = cursor.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

        screenBB = r.g;

        * (P_S32) &rgb_background = mlec->attributes[MLEC_ATTRIBUTE_BG_RGB];
        * (P_S32) &rgb_foreground = mlec->attributes[MLEC_ATTRIBUTE_FG_RGB];

        if(HOST_FONT_NONE != mlec->host_font)
            host_setfontcolours_for_mlec(&rgb_foreground, &rgb_background);

        {
        /* Render characters to the right of the cursor and the lines below it */

        lineCol = mlec->cursor.pcol; lineRow = mlec->cursor.row;

        lineBB.x0 = lineBB.x1 = cursor.x1;
        lineBB.y1 = cursor.y1;
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

        ptr    = upper_start;

        /* Skip lines that are above the current graphics window */
        while((lineBB.y0 > screenBB.y1) && (ptr < upper_end))
        {
            /* line is above the region to be redrawn, so skip it */
            while(ptr < upper_end)
            {
                if(CR == *ptr++)
                {
                    lineCol = 0; lineRow++;

                    lineBB.x0  = cursor.x0;
                    lineBB.y1 -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
                    lineBB.y0  = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
                    break;
                }
            }
        }

        /* Render lines that fall within the current graphics window */
        while((lineBB.y1 > screenBB.y0) && (ptr < upper_end))
        {
            /* line within redraw region, so render it */
            render_line(mlec, lineCol, lineBB.x0, lineBB.y1, screenBB, &ptr, upper_end);

            lineCol = 0; lineRow++;

            lineBB.x0  = cursor.x0;
            lineBB.y1 -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            lineBB.y0  = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
        }

        /* remaining lines are below the graphics window */
        } /*block*/

        {
        /* Render characters to the left of the cursor and the lines above it */

        lineCol = 0; lineRow = mlec->cursor.row;

        lineBB.x0 = lineBB.x1 = cursor.x0;
        lineBB.y1 = cursor.y1;
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

        linestart = lower_row;
        lineend   = lower_end;

        /* Skip lines that are below the current graphics window */
        while((lineBB.y1 < screenBB.y0) && (linestart >= lower_start))
        {
            /* line is below region to be redrawn, so skip it */
            lineend = --linestart;          /* point at terminator for previous line */
            while((linestart > lower_start) && (CR != *(linestart - 1)))
                --linestart;

            lineRow--;

            lineBB.y1 += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            lineBB.y0  = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
        }

        /* Render lines that fall within the current graphics window */
        while((lineBB.y0 < screenBB.y1) && (linestart >= lower_start))
        {
            /* line within redraw region, so render it */
            ptr = linestart;
            render_line(mlec, lineCol, lineBB.x0, lineBB.y1, screenBB, &ptr, lineend);

            lineend = --linestart;          /* point at terminator for previous line */
            while((linestart > lower_start) && (CR != *(linestart - 1)))
                --linestart;

            lineRow--;

            lineBB.y1 += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
        }

        /* remaining lines are above the graphics window */

#if SUPPORT_SELECTION
        if(mlec->selectvalid)
            show_selection(mlec, markstart, markend, orgx, orgy, screenBB);
#endif
        } /*block*/

#if FALSE
        {
        int x = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * mlec->maxcol;
        int y = orgy - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * (mlec->linecount + 1);      /* lowest scan line on last row */

        bbc_move(orgx,y - mlec->pixheight); bbc_drawby(200 * mlec->charwidth, 0);       /* draw line on first unused scanline */
        bbc_move(x,y); bbc_drawby(0, 200);
        }
#endif

        wimp_get_rectangle(&r, &more);
    }
}

/******************************************************************************
*
* Cursor movement routines
*
* Each one should clear the selection
*
******************************************************************************/

static void mlec__cursor_down(MLEC_HANDLE mlec)
{
    char *buff = mlec->buffptr;
    BOOL  found;
    int   i;

    clear_selection(mlec);

    /* Can only move down if current line has a terminator */
    for(found = FALSE, i = mlec->upper.start; i < mlec->upper.end; i++)
    {
        if (TRUE == (found = (buff[i] == CR)))
            break;
    }

    if(found)
    {
        /* move rest of line including terminator */
        while(CR != (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
            /* null statement */;

        /* wrap cursor to beginning of next line */
        mlec->cursor.row++;
        mlec->cursor.pcol = 0;

        /* try moving to logical cursor position, must stop at eol or eot */
        while(
              (mlec->cursor.pcol < mlec->cursor.lcol) &&
              (mlec->upper.start < mlec->upper.end  ) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
    }
  /*else             */
  /*    on last line */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_left(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->lower.end > mlec->lower.start)
    {
        char *buff = mlec->buffptr;
        char  ch;

        ch = buff[--mlec->upper.start] = buff[--mlec->lower.end];

        if(ch == CR)
        {
            int i;
            /* wrap cursor to end of previous line */

            /* which means finding the line length */
            mlec->cursor.pcol = 0;
            for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
            {
                if(buff[--i] == CR)
                    break;
            }

            --mlec->cursor.row;
        }
        else
        {
            /* ordinary character */
            --mlec->cursor.pcol;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_right(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->upper.start < mlec->upper.end)
    {
        char *buff = mlec->buffptr;
        char  ch;

        ch = buff[mlec->lower.end++] = buff[mlec->upper.start++];

        if(ch == CR)
        {
            /* wrap cursor to beginning of next line */
            mlec->cursor.pcol = 0;
            mlec->cursor.row++;
        }
        else
        {
            /* ordinary character */
            mlec->cursor.pcol++;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_up(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.row > 0)
    {
        char *buff = mlec->buffptr;
        int   i;

        /* move rest of line including terminator */
        while(CR != (buff[--mlec->upper.start] = buff[--mlec->lower.end]))
            /* null statement */;

        /* wrap cursor to end of previous line */

        /* which means finding the line length */
        mlec->cursor.pcol = 0;
        for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
        {
            if(buff[--i] == CR)
                break;
        }

        --mlec->cursor.row;

        /* try moving to logical cursor postion */
        while(mlec->cursor.pcol > mlec->cursor.lcol)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
    }
  /*else            */
  /*    on top line */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_lineend(MLEC_HANDLE mlec)
{
    char *buff = mlec->buffptr;

    clear_selection(mlec);

    if(
       (mlec->upper.start < mlec->upper.end) &&
       (CR != buff[mlec->upper.start])
       )
    {
        while(
              (mlec->upper.start < mlec->upper.end) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                              */
  /*    at end of text or end of line */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_linehome(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.pcol > 0)
    {
        char *buff = mlec->buffptr;

        /* move cursor to start of line */
        while(mlec->cursor.pcol > 0)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

/******************************************************************************
*
* Move the cursor left to the previous tab position
*
* ie move 1..(TAB_MASK+1) places
*
******************************************************************************/

static void mlec__cursor_tab_left(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.pcol > 0)
    {
        char *buff = mlec->buffptr;

        /* move left (at least one) to tab-stop */
        do  {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
        while(mlec->cursor.pcol & TAB_MASK);

        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_textend(MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->upper.start < mlec->upper.end)
    {
        char *buff = mlec->buffptr;

        while(mlec->upper.start < mlec->upper.end)
        {
            if(CR == (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
            {
                mlec->cursor.pcol = 0;
                mlec->cursor.row++;
            }
            else
            {
                /* ordinary character */
                mlec->cursor.pcol++;
            }
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_texthome(
    MLEC_HANDLE mlec)
{
    clear_selection(mlec);

    if(mlec->lower.end > mlec->lower.start)
    {
        char *buff = mlec->buffptr;

        while(mlec->lower.end > mlec->lower.start)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
        }

        mlec->cursor.row  = 0;
        mlec->cursor.lcol = mlec->cursor.pcol = 0;
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

void
mlec__cursor_getpos(
    MLEC_HANDLE mlec,
    int * p_col,
    int * p_row)
{
    *p_col = mlec->cursor.pcol; *p_row = mlec->cursor.row;      /* NB returns physical cursor posn. */
}

void
mlec__cursor_setpos(
    MLEC_HANDLE mlec,
    int col,
    int row)
{
    trace_2(TRACE_MODULE_MLEC, "(%d,%d)",col,row);

    clear_selection(mlec);

    move_cursor(mlec, col, row);                                /* Cursor moved as close as possible */
                                                                /* to (col,row). Sets lcol = pcol    */
    scroll_until_cursor_visible(mlec);
}

static void mlec__cursor_wordleft(MLEC_HANDLE mlec)
{
    MARK_POSITION wordstart;

    word_left(mlec, &wordstart);

    if(mlec->cursor.pcol == wordstart.col)
    {
        /* cursor at start of line */
        mlec__cursor_left(mlec);        /* will wrap cursor to end of previous line */
    }
    else
        mlec__cursor_setpos(mlec, wordstart.col, wordstart.row);
}

static void mlec__cursor_wordright(MLEC_HANDLE mlec)
{
    MARK_POSITION wordstart, wordend;

    word_limits(mlec, &wordstart, &wordend);

    if(mlec->cursor.pcol == wordend.col)
    {
        /* cursor at eoline */
        mlec__cursor_right(mlec);       /* will wrap cursor to beginning of next line */
    }
    else
        mlec__cursor_setpos(mlec, wordend.col, wordend.row);
}

/******************************************************************************
*
* Character and block text insertion routines
*
******************************************************************************/

/******************************************************************************
*
* Insert the supplied character at the cursor position, deleting any previously selected text.
*
* The action of non printable characters is somewhat undefined.
*
* Aside: To prevent screen redraw problems, this implementation replaces non printable
*        characters with a '.' (dot), but this is subject to change.
*
******************************************************************************/

int
mlec__insert_char(
    MLEC_HANDLE mlec,
    char ch)
{
    int err;

    if((err = checkspace_delete_selection(mlec, 1)) >= 0)
    {
        char *buff = mlec->buffptr;

        force_redraw_eoline(mlec);     /* will execute later on */

        buff[mlec->lower.end++] = ch;
        mlec->cursor.pcol++;

        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                          */
  /*    sod off, the buffers full */

    scroll_until_cursor_visible(mlec);
    return(err);
}

int mlec__insert_newline(MLEC_HANDLE mlec)
{
    int err;

    if((err = checkspace_delete_selection(mlec, 1)) >= 0)
    {
        char *buff = mlec->buffptr;

        force_redraw_eotext(mlec);      /* will execute later on */

        buff[mlec->lower.end++] = CR;
        mlec->cursor.lcol = mlec->cursor.pcol = 0;  /* snap logical col and physical col to start of next line */
        mlec->cursor.row++;
        mlec->linecount++;
    }
  /*else                          */
  /*    sod off, the buffers full */

    scroll_until_cursor_visible(mlec);
    return(err);
}

/******************************************************************************
*
* Insert spaces upto the next tab position
*
* ie insert 1..(TAB_MASK+1) spaces
*
******************************************************************************/

static int mlec__insert_tab(MLEC_HANDLE mlec)
{
    int err;

    if((err = checkspace_delete_selection(mlec, 1+TAB_MASK)) >= 0)
    {
        char *buff = mlec->buffptr;

        force_redraw_eoline(mlec);      /* will execute later on */

        do  {
            buff[mlec->lower.end++] = SPACE;
            mlec->cursor.pcol++;
        }
        while(mlec->cursor.pcol & TAB_MASK);

        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                          */
  /*    sod off, the buffers full */

    scroll_until_cursor_visible(mlec);
    return(err);
}

/******************************************************************************
*
* Insert the supplied text at the cursor position, deleting any previously selected text.
* The supplied text should be '\0' terminated, and may contain CR, LF, CRLF or LFCR as
* line-break sequences.
*
* The action of non printable characters is somewhat undefined.
*
* Aside: To prevent screen redraw problems, this implementation replaces non printable
*        characters with a '.' (dot), but this is subject to change.
*
******************************************************************************/

int
mlec__insert_text(
    MLEC_HANDLE mlec,
    char *text)
{
    int err;

    if((err = checkspace_delete_selection(mlec, strlen(text))) >= 0)
    {
        char *buff = mlec->buffptr;
        int   col  = mlec->cursor.pcol;
        int   row  = mlec->cursor.row;
        int   line = mlec->linecount;
        int   lend = mlec->lower.end;

        char  ch;
        char  skip = '\0';

        force_redraw_eotext(mlec);      /* will execute later on */

        for(; 0 != (ch = *text); text++)
        {
            switch(ch)
            {
            case CR:
            case LF:
                if(ch == skip)
                {
                    skip = '\0';
                    continue;
                }
                skip = ch ^ (CR ^ LF);          /* ^ means EOR */
                ch = CR;        /*>>>would zero be better???*/
                buff[lend++] = ch;
                col = 0; row++; line++;
                continue;

            default:
                buff[lend++] = ch; col++;
                break;
            }
            skip = '\0';
        }

        mlec->cursor.lcol = mlec->cursor.pcol  = col;
        mlec->cursor.row  = row;
        mlec->linecount   = line;
        mlec->lower.end   = lend;
    }
  /*else                          */
  /*    sod off, the buffers full */

    scroll_until_cursor_visible(mlec);
    return(err);
}

/******************************************************************************
*
* Character and line deletion routines
*
******************************************************************************/

static void mlec__delete_left(MLEC_HANDLE mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete char left of cursor                                     */
    if(mlec->lower.end > mlec->lower.start)
    {
        char *buff = mlec->buffptr;
        int   i;

        if(CR == buff[--mlec->lower.end])
        {
            /* cursor moves to what was the end of the previous line */

            /* which means finding its length */
            mlec->cursor.pcol = 0;
            for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
            {
                if(buff[--i] == CR)
                    break;
            }

            --mlec->cursor.row;
            force_redraw_eotext(mlec);
            --mlec->linecount;
        }
        else
        {
            /* ordinary character */
            --mlec->cursor.pcol;
            force_redraw_eoline(mlec);
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

static void mlec__delete_right(MLEC_HANDLE mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete char right of cursor                                    */
    if(mlec->upper.start < mlec->upper.end)
    {
        char *buff = mlec->buffptr;

        if(CR == buff[mlec->upper.start++])
        {
            force_redraw_eotext(mlec);
            --mlec->linecount;
        }
        else
        {
            /* ordinary character */
            force_redraw_eoline(mlec);
        }
        mlec->cursor.lcol = mlec->cursor.pcol;  /* cos lcol may have been past old eol */
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

static void mlec__delete_line(MLEC_HANDLE mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete cursor line                                             */
    {
    char *buff = mlec->buffptr;
    BOOL  done = FALSE;

    if(mlec->cursor.pcol > 0)
    {
        done = TRUE;

        /* skip cursor to start of line, chucking chars on floor as we go */
        mlec->lower.end   -= mlec->cursor.pcol;
        mlec->cursor.lcol  = mlec->cursor.pcol = 0;
    }

    while(mlec->upper.start < mlec->upper.end)
    {
        done = TRUE;

        if(CR == buff[mlec->upper.start++])     /* chuck char on floor   */
        {
            --mlec->linecount;
            break;                              /* finish if it was a CR */
        }
    }

    if(done)
    {
        force_redraw_eotext(mlec);

        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                              */
  /*    no text or on empty last line */
    }

    scroll_until_cursor_visible(mlec);
}

static void mlec__delete_lineend(MLEC_HANDLE mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete from cursor to end-of-line                              */
    {
    char *buff = mlec->buffptr;
    BOOL  done = FALSE;

    while(
          (mlec->upper.start < mlec->upper.end) &&
          (CR != buff[mlec->upper.start])
         )
    {
        done = TRUE;
        mlec->upper.start++;    /* chuck char on floor */
    }

    if(done)
    {
        force_redraw_eoline(mlec);

        mlec->cursor.lcol = mlec->cursor.pcol; /* be paranoid, should already be snapped */
    }
  /*else                              */
  /*    at end of text or end of line */
    }

    scroll_until_cursor_visible(mlec);
}

static void mlec__delete_linehome(MLEC_HANDLE mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete from cursor to start-of-line                            */
    if(mlec->cursor.pcol > 0)
    {
        /* skip cursor to start of line, chucking chars on floor as we go */
        mlec->lower.end   -= mlec->cursor.pcol;
        mlec->cursor.lcol  = mlec->cursor.pcol = 0;

        force_redraw_eoline(mlec);
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

#if FALSE
/******************************************************************************
*
* Superceded by the mlec_attach_eventhandler() mechanism
*
******************************************************************************/

/******************************************************************************
*
* Send the main window a button pressed message.
*
* This allows actions on the pane window to fake actions on the main window
* Probable use
*   Return into OK
*   Escape into Cancel
*
******************************************************************************/

static void
mlec__main_button(
    MLEC_HANDLE mlec,
    wimp_i icon)
{
    if(mlec->main != window_NULL)
    {
        wimp_mousestr mouse;

        mouse.x = mouse.y = -1;

        mouse.bbits = wimp_BLEFT;
        mouse.w     = mlec->main;
        mouse.i     = icon;

        wimpt_safe(wimp_sendwmessage(wimp_EBUT, (wimp_msgstr*)&mouse, mlec->main, 0));
    }

}
#endif

/******************************************************************************
*
* Claim the input focus for this edit control and place the caret at its cursor position.
*
******************************************************************************/

void mlec_claim_focus(MLEC_HANDLE mlec)
{
    wimp_caretstr current;
    wimp_caretstr carrot;    /* nyeeeer, whats up doc? */

    trace_0(TRACE_MODULE_MLEC, "mlec_claim_focus - ");

    wimp_get_caret_pos(&current);

    build_caretstr(mlec, &carrot);

    if((current.w != mlec->pane) ||
       (current.x != carrot.x) || (current.y != carrot.y) ||
       (current.height != carrot.height)        /* cos this field holds caret (in)visible bit */
      )
    {
        wimp_set_caret_pos(&carrot);
        trace_3(TRACE_MODULE_MLEC, "place caret (%d,%d,%d)",carrot.x,carrot.y,carrot.height);
    }
    else
    {
        trace_0(TRACE_MODULE_MLEC, " no action (we own input focus, caret already positioned)");
    }
}

void mlec_release_focus(MLEC_HANDLE mlec)
{
    wimp_caretstr current;

    trace_0(TRACE_MODULE_MLEC, "mlec_release_focus - ");

    wimp_get_caret_pos(&current);

    if(current.w == mlec->pane)
    {
        current.w = (wimp_w)-1;
        current.i = (wimp_i)-1;
        current.x = current.y = current.index = 0;
        current.height = 0x02000000;

        wimp_set_caret_pos(&current);
        trace_0(TRACE_MODULE_MLEC, " focus given away");
    }
    else
    {
        trace_0(TRACE_MODULE_MLEC, " no action (focus belongs elsewhere)");
    }
}

void scroll_until_cursor_visible(MLEC_HANDLE mlec)
{
    trace_0(TRACE_MODULE_MLEC, "scroll_until_cursor_visible - ");

    return_if_no_pane(mlec);

    /* May need to enlarge the window extent to fit the text. */
    {
    BOOL            change  =   FALSE;
    int             extentx =   mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * mlec->maxcol;
    int             extenty = - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * (mlec->linecount + 1); /* -ve */
    wimp_redrawstr  blk;

    trace_4(TRACE_MODULE_MLEC, "window extent (%d,%d,%d,%d)",mlec->paneextent.x0,mlec->paneextent.y0,
                               mlec->paneextent.x1,mlec->paneextent.y1);
    trace_1(TRACE_MODULE_MLEC, "extenty=%d",extenty);

    if(mlec->paneextent.x1 < extentx)
    {
        mlec->paneextent.x1 = extentx;
        change = TRUE;
    }

    if(mlec->paneextent.y0 > extenty)       /* NB -ve numbers */
    {
        mlec->paneextent.y0 = extenty;
        change = TRUE;
    }

    if(change)
    {
        blk.w   = mlec->pane;
        blk.box = mlec->paneextent;
        wimp_set_extent(&blk);

        if(mlec->callbackproc)
            (*mlec->callbackproc)(Mlec_IsWorkAreaChanged, mlec->callbackhand, &blk);
    }
    } /*block*/

    /* May need to scroll the window, to keep the cursor (pcol,row) visible. */
    {
    wimp_wstate state;

    if(wimp_get_wind_state(mlec->pane, &state) == NULL)
    {
        wimp_box  curshape;
        int       visible_width  = state.o.box.x1 - state.o.box.x0;
        int       visible_height = state.o.box.y1 - state.o.box.y0;
        BOOL      done           = FALSE;

        curshape.x0 = curshape.x1 =  mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->cursor.pcol * mlec->charwidth;
        curshape.y1               = -mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * mlec->cursor.row;
        curshape.y0               =  curshape.y1      - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        trace_4(TRACE_MODULE_MLEC, "wimp_get_window_state returns: visible area=(%d,%d, %d,%d)",state.o.box.x0,state.o.box.y0,
                                                                                                state.o.box.x1,state.o.box.y1);
        trace_2(TRACE_MODULE_MLEC, "scroll offset=(%d,%d)", state.o.scx,state.o.scy);

        trace_2(TRACE_MODULE_MLEC, "cursor is (%d,%d)", curshape.x0, curshape.y1);

        while(state.o.scx > curshape.x0)
        {
            state.o.scx -= 4 * mlec->charwidth;
            done = TRUE;
        }

        while((state.o.scx + visible_width) < curshape.x1)
        {
            state.o.scx += 4 * mlec->charwidth;
            done = TRUE;
        }

        while(state.o.scy < curshape.y1)
        {
            state.o.scy += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            done = TRUE;
        }

        while((state.o.scy - visible_height) > curshape.y0)
        {
            state.o.scy -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            done = TRUE;
        }

        if(done)
        {
#if FALSE
            mlec__redraw_loop(mlec);    /* redraw all invalid rectangles BEFORE scrolling window */
                                        /* cos wimp fails to scroll the invalid rectangle list   */
            /*>>>causes a lot of flicker when cursor up/down causes a scroll */
#endif
            wimp_open_wind(&state.o);
        }

        show_caret(mlec);   /* probably better here...                             */
    }
                            /* ...than here, but then again doesn't really matter? */
    } /*block*/
}

void show_caret(MLEC_HANDLE mlec)
{
    wimp_caretstr current;

    trace_0(TRACE_MODULE_MLEC, "show_caret - ");

    return_if_no_pane(mlec);

    wimp_get_caret_pos(&current);

    if(current.w == mlec->pane)
    {
        wimp_caretstr carrot;    /* nyeeeer, whats up doc? */

        trace_0(TRACE_MODULE_MLEC, "we own input focus ");

        build_caretstr(mlec, &carrot);

        if((current.x != carrot.x) || (current.y != carrot.y) ||
           (current.height != carrot.height)    /* cos this field holds caret (in)visible bit */
          )
        {
            wimp_set_caret_pos(&carrot);
            trace_3(TRACE_MODULE_MLEC, "place caret (%d,%d,%d)",carrot.x,carrot.y,carrot.height);
        }
        else
        {
            trace_0(TRACE_MODULE_MLEC, " no action (caret already positioned)");
        }
    }
    else
    {
        trace_0(TRACE_MODULE_MLEC, " no action (focus belongs elsewhere)");
    }
}

/******************************************************************************
*
* Do the donkey work of building the structure needed to claim the caret for
* our pane window, placed at the cursor physical position (cursor(pcol,row)).
* The caret is set as invisible if there is a selection.
*
******************************************************************************/

void
build_caretstr(
    MLEC_HANDLE mlec,
    wimp_caretstr *carrotp)
{
    int caretXoffset = mlec->cursor.pcol * mlec->charwidth;
    int caretheight  = mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];        /*>>> or should this be charHeight ie 32 */
    int caretYoffset = mlec->cursor.row * mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] + 32; /*>>>what magic number is this?*/

    carrotp->w = mlec->pane;
    carrotp->i = (wimp_i)-1;
    carrotp->x =  caretXoffset + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT];
    carrotp->y = -caretYoffset - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - 4;   /*>>> -4 and                                           */
    carrotp->height= caretheight + 8; /*>>> +8 to make cursor start one pixel row below text */
    carrotp->index = 0;               /*>>> and finish one pixel row above it                */

    if(HOST_FONT_NONE == mlec->host_font)
        carrotp->height |= 0x01000000;  /* VDU 5 style caret if no font */

#if SUPPORT_SELECTION
    if(mlec->selectvalid)
        carrotp->height |= 0x02000000;  /* caret made invisible, if we have a selection */
#endif
}

static void
move_cursor(
    MLEC_HANDLE mlec,
    int col,
    int row)
{
    char *buff = mlec->buffptr;

    if(mlec->cursor.row > row)
    {
        /* move cursor to (0,row), or (0,0) if row<0 */

        while((mlec->cursor.row > row) &&
              (mlec->cursor.row > 0)
             )
        {
            /* move rest of line including terminator of previous line */
            while(CR != (buff[--mlec->upper.start] = buff[--mlec->lower.end]))
                /* null statement */;

            --mlec->cursor.row; /* now at end of previous line, column position unknown */
        }

        /* somewhere on required line (or line 0 if row < 0), find column 0 */
        while((mlec->lower.end > mlec->lower.start) &&
              (CR != buff[mlec->lower.end-1])
             )
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
        }

        mlec->cursor.pcol = 0;
    }

    while(
          (mlec->cursor.row  < row)             &&
          (mlec->upper.start < mlec->upper.end)
         )
    {
        if(CR == (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
        {
            /* wrap cursor to beginning of next line */
            mlec->cursor.pcol = 0;
            mlec->cursor.row++;
        }
        else
        {
            /* ordinary character */
            mlec->cursor.pcol++;
        }
    }

    if(mlec->cursor.row == row)
    {
        if(col < 0)
            col = 0;

        while(mlec->cursor.pcol > col)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }

        while(
              (mlec->cursor.pcol < col)             &&
              (mlec->upper.start < mlec->upper.end) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
    }

    mlec->cursor.lcol = mlec->cursor.pcol;
}

/******************************************************************************
*
* Scan left from the cursor position, looking for the start of a word.
* If the cursor is already at the start, find the start of the previous word.
*
******************************************************************************/

void
word_left(
    MLEC_HANDLE mlec,
    MARK_POSITION *startp)
{
    char *sptr = &mlec->buffptr[mlec->lower.end];       /* 1 byte past character to left of cursor */
    int   scol = mlec->cursor.pcol;

    while((scol > 0) && (' ' == *(sptr-1)))
    {
        --scol; --sptr;
    }

    while((scol > 0) && (' ' != *(sptr-1)))
    {
        --scol; --sptr;
    }

    startp->col = scol;
    startp->row = mlec->cursor.row;

    trace_1(TRACE_MODULE_MLEC, "word start %d", scol);
}

/******************************************************************************
*
* Scan either side of cursor position, return limits of a word.
*
******************************************************************************/

void
word_limits(
    MLEC_HANDLE mlec,
    MARK_POSITION *startp,
    MARK_POSITION *endp)
{
  /*char *lower_start = &mlec->buffptr[mlec->lower.start];*/                  /* first character                         */
    char *lower_end   = &mlec->buffptr[mlec->lower.end];                      /* 1 byte past character to left of cursor */
    char *upper_start = &mlec->buffptr[mlec->upper.start];                    /* character to right of cursor            */
    char *upper_end   = &mlec->buffptr[mlec->upper.end];                      /* 1 byte past last character              */

    char *sptr = lower_end;
    char *eptr = upper_start;
    int   scol, ecol;

    scol = ecol = mlec->cursor.pcol;

    if((eptr < upper_end) && (CR != *eptr) && (' ' != *eptr))
    {
        while((eptr < upper_end) && (CR != *eptr) && (' ' != *eptr))
        {
            ecol++; eptr++;
        }
    }
    else
    {
        while((scol > 0) && (' ' == *(sptr-1)))
        {
            --scol; --sptr;
        }
    }

    while((scol > 0) && (' ' != *(sptr-1)))
    {
        --scol; --sptr;
    }

    while((eptr < upper_end) && (CR != *eptr) && (' ' == *eptr))
    {
        ecol++; eptr++;
    }

    startp->col = scol;
    startp->row = endp->row = mlec->cursor.row;
    endp->col   = ecol;

    trace_2(TRACE_MODULE_MLEC, "word limits %d..%d", scol, ecol);
}

static void
render_line(
    MLEC_HANDLE mlec,
    int lineCol,
    int x,
    int y,
    wimp_box screen,
    char **cpp,
    char *limit)
{
    int   charedge;
    BOOL  scan = TRUE;
    char *showptr;      /* ptr to first visible or partially visible character */
    int   showcnt = 0;  /* number of visible or partially visible characters */

    trace_2(TRACE_MODULE_MLEC, "render_line x=%d,y=%d ",x,y);
    trace_4(TRACE_MODULE_MLEC, "screen=(%d,%d, %d,%d) ",screen.x0,screen.y0,screen.x1,screen.y1);

    charedge = x + mlec->charwidth;     /* char covers x..charedge (inc..exc) */

    /* find first character not totally hidden */
    while((charedge <= screen.x0) && (*cpp < limit))              /*>>>  <= ??? */
    {
        if(CR == *((*cpp)++))
        {
            scan = FALSE;       /* (*cpp), now points to first character on next line */
            break;
        }

        x = charedge;
        charedge += mlec->charwidth;
        lineCol++;
    }

    charedge = x;                       /* char covers charedge.. (inc..), N.B. screen.x1 is inclusive!! */
    showptr  = (*cpp);
    while((charedge </*=*/ screen.x1) && (*cpp < limit) && scan)
    {
        if(CR == *((*cpp)++))
        {
            scan = FALSE;       /* (*cpp), now points to first character on next line */
            break;
        }

        charedge += mlec->charwidth;
        lineCol++;
        showcnt++;
    }

    if (showcnt > 0)
    {
        if(HOST_FONT_NONE != mlec->host_font)
        {
            const int base_line_shift = -24;
            _kernel_swi_regs rs;

            rs.r[0] = mlec->host_font;
            rs.r[1] = (int) showptr;
            rs.r[2] = FONT_PAINT_USE_LENGTH /*r7*/ | FONT_PAINT_USE_HANDLE /*r0*/ | FONT_PAINT_OSCOORDS;
            rs.r[3] = x;
            rs.r[4] = y + base_line_shift;
            rs.r[7] = showcnt;

            (void) _kernel_swi(/*Font_Paint*/ 0x40086, &rs, &rs);
        }
        else
        {
            bbc_move(x, y-wimpt_dy());

            /* SKS after PD 4.12 27mar92 - convert CtrlChar on fly */
            while(showcnt--)
            {
                char ch = *showptr++;
                if((ch < 0x20) || (ch == 0x7F))
                    ch = '.';
                bbc_vdu(ch);
            }
        }
    }

    /* if not already there, advance (*cpp), to first character on next line */
    while((*cpp < limit) && scan)
    {
        if(CR == *((*cpp)++))
            break;

        lineCol++;
    }

    if(mlec->maxcol < lineCol)
    {
        mlec->maxcol = lineCol;
    }
}

/******************************************************************************
*
* Report an error.
*
* Normally errors in mlec routines are bubbled back to the caller, but with events, such as
* key strokes, menu paste/save or file-dropped-on-window there is no suitable caller,
* so the error must be reported from the event routine.
*
******************************************************************************/

static void
report_error(
    MLEC_HANDLE mlec,
    int err)
{
    mlec = mlec;
    message_output(string_lookup(err));
}

static int
checkspace_deletealltext(
    MLEC_HANDLE mlec,
    int size)
{
    int shortfall = size - (mlec->upper.end - mlec->lower.start);
    /* Check buffer space, note use of lower.start..upper.end instead of lower.end..upper.start */
    /* we want to know if text will fit after current text is flushed */
    if(shortfall > 0)
    {
        trace_2(TRACE_MODULE_MLEC, "do flex_midextend by %d bytes, at position %d", shortfall, mlec->upper.start);

        if(!flex_midextend((flex_ptr)&mlec->buffptr, mlec->upper.start, shortfall))
            return(create_error(status_nomem()));

        mlec->buffsiz     += shortfall;
        mlec->upper.start += shortfall;
        mlec->upper.end   += shortfall;
    }

    return(0);
}

static int
checkspace_delete_selection(
    MLEC_HANDLE mlec,
    int size)
{
    int shortfall;

    shortfall = size - (mlec->upper.start - mlec->lower.end);

    if(shortfall > 0)
    {
        shortfall = ((shortfall + 3) & -4);     /* round up to a word */

        trace_2(TRACE_MODULE_MLEC, "do flex_midextend by %d bytes, at position %d", shortfall, mlec->upper.start);

        if(!flex_midextend((flex_ptr)&mlec->buffptr, mlec->upper.start, shortfall))
            return(status_nomem());

        mlec->buffsiz     += shortfall;
        mlec->upper.start += shortfall;
        mlec->upper.end   += shortfall;
    }

    delete_selection(mlec);
    return(0); /*>>>not quite right, we don't consider the space the delete_selection will free*/
}

#if FALSE
void force_redraw(MLEC_HANDLE mlec)
{
    wimp_redrawstr redraw;

    return_if_no_pane(mlec);

    redraw.w      =  mlec->pane;
    redraw.box.x0 = -0x1FFFFFFF; redraw.box.y0 = -0x1FFFFFFF;
    redraw.box.x1 =  0x1FFFFFFF; redraw.box.y1 =  0x1FFFFFFF;

    (void) wimp_force_redraw(&redraw);
}
#endif

/******************************************************************************
*
* Mark an area of the pane window as invalid - from cursor position to end of line
*
* NB The window is redrawn later on, when our caller next does a Wimp_Poll.
*    Cursor position is (mlec->cursorpcol,mlec->cursor.row), mlec->cursor.lcol
*    is not used.
*
******************************************************************************/

void force_redraw_eoline(MLEC_HANDLE mlec)
{
    wimp_redrawstr redraw;

    return_if_no_pane(mlec);

    /* invalidate right of cursor to eol */
    redraw.w      =  mlec->pane;
    redraw.box.x0 =  mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->cursor.pcol * mlec->charwidth;
    redraw.box.x1 =  0x1FFFFFFF;
    redraw.box.y1 = -mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * mlec->cursor.row;
    redraw.box.y0 =  redraw.box.y1   - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]; /*>>>actually charheight??*/

    (void)wimp_force_redraw(&redraw);
}

/******************************************************************************
*
* Mark an area of the pane window as invalid - from cursor position to end of text
*
* NB the window is redrawn later on, when our caller next does a Wimp_Poll
*    Cursor position is (mlec->cursorpcol,mlec->cursor.row), mlec->cursor.lcol
*    is not used.
*
******************************************************************************/

void force_redraw_eotext(MLEC_HANDLE mlec)
{
    wimp_redrawstr redraw;
    int            y;

    return_if_no_pane(mlec);

    /* invalidate right of cursor to eol */
    redraw.w      =  mlec->pane;
    redraw.box.x0 =  mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->cursor.pcol * mlec->charwidth;    /* left  (inc)  */
    redraw.box.x1 =  0x1FFFFFFF;                                                /* right (exc)  */
    redraw.box.y1 = -mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * mlec->cursor.row;      /* top   (exe)  */
    redraw.box.y0 =  y = redraw.box.y1 - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];                       /* bottom (inc) */

    (void)wimp_force_redraw(&redraw);

    /* invalidate rows below cursor (must reinit all fields, cos wimp_force_redraw shits on structure) */
    redraw.w      =  mlec->pane;
    redraw.box.x0 =  mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT];                                          /* left  (inc)  */
    redraw.box.x1 =  0x1FFFFFFF;                                                /* right (exc)  */
    redraw.box.y1 =  y;                                                         /* top   (exe)  */
    redraw.box.y0 = -0x1FFFFFFF;                                                /* bottom (inc) */

    (void)wimp_force_redraw(&redraw);
}

#if SUPPORT_SELECTION

static void
mlec__drag_start(
    MLEC_HANDLE mlec)
{
    wimp_wstate  state;
    wimp_dragstr dragstr;

    wimpt_complain(wimp_get_wind_state(mlec->pane, &state));

    dragstr.window    = mlec->pane;        /* Needed by win_drag_box, so it can send EUSERDRAG to us */
    dragstr.type      = wimp_USER_HIDDEN;
#if FALSE
    /* Window Manager ignores inner box on hidden drags */
    dragstr.box.x0    = mx;
    dragstr.box.y0    = my;
    dragstr.box.x1    = mx+30;
    dragstr.box.y1    = my+30;
#endif
    dragstr.parent.x0 = state.o.box.x0 - mlec->charwidth;
    dragstr.parent.y0 = state.o.box.y0 - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
    dragstr.parent.x1 = state.o.box.x1 + mlec->charwidth;
    dragstr.parent.y1 = state.o.box.y1 + mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];

    wimpt_complain(win_drag_box(&dragstr));     /* NB win_drag_box NOT wimp_drag_box */

    status_assert(Null_EventHandlerAdd(mlec__drag_null_handler, mlec, 0));
}

static void
mlec__drag_complete(
    MLEC_HANDLE mlec,
    wimp_box *dragboxp)
{
    IGNOREPARM(dragboxp);

    Null_EventHandlerRemove(mlec__drag_null_handler, mlec);
}

/******************************************************************************
*
* Process call-back from null engine.
*
******************************************************************************/

null_event_proto(static, mlec__drag_null_handler)
{
    MLEC_HANDLE mlec = (MLEC_HANDLE) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(NULL_EVENTS_REQUIRED);

    case NULL_EVENT:
        {
        wimp_mousestr mouse;
        wimp_wstate   r;
        int           x,y, orgx,orgy;

        (void)wimp_get_point_info(&mouse);
        (void)wimp_get_wind_state(mlec->pane, &r);

        orgx = r.o.box.x0 - r.o.scx; orgy = r.o.box.y1 - r.o.scy;
        x = mouse.x - orgx;       /* mouse position relative to */
        y = mouse.y - orgy;       /* window origin              */

        x = ( x - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] +8 ) / 16;                 /* 8=half char width, 16=char width*/
        y = (-y - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] -1) / mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];

        mlec__selection_adjust(mlec, x,y);
        }
        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }

}

/******************************************************************************
*
* Move the cursor to (col,row), without clearing the selection.
*
* If a selection does not exist, one is created, with its
* anchor point at the old cursor position.
*
******************************************************************************/

extern void
mlec__selection_adjust(
    MLEC_HANDLE mlec,
    int col,
    int row)
{
    MARK_POSITION old;

    trace_2(TRACE_MODULE_MLEC, "adjust selection to (%d,%d)",col,row);

    if(!mlec->selectvalid)
    {
        /* currently no selection, so start one at the cursor position */

        mlec->selectvalid      = TRUE;
        mlec->selectanchor.col = mlec->cursor.pcol;
        mlec->selectanchor.row = mlec->cursor.row;
    }

    old.col = mlec->cursor.pcol; old.row = mlec->cursor.row;

    /* move cursor (as close as possible) to (col,row) */
    move_cursor(mlec, col, row);
    col = mlec->cursor.pcol; row = mlec->cursor.row;        /* actual position */

    mlec__update_loop(mlec, old, mlec->cursor);

    if((mlec->selectanchor.col == mlec->cursor.pcol) && (mlec->selectanchor.row == mlec->cursor.row))
        mlec->selectvalid = FALSE;

    scroll_until_cursor_visible(mlec);
}

extern void
mlec__selection_clear(
    MLEC_HANDLE mlec)
{
    clear_selection(mlec);
    show_caret(mlec);   /* do show_caret, not scroll_until_cursor_visible, so the window won't scroll */
}

extern void
mlec__selection_delete(
    MLEC_HANDLE mlec)
{
    delete_selection(mlec);
    scroll_until_cursor_visible(mlec);
}

static void
mlec__select_word(
    MLEC_HANDLE mlec)
{
    MARK_POSITION wordstart, wordend;

    word_limits(mlec, &wordstart, &wordend);

    if(!mlec->selectvalid)
    {
#if TRUE
        /* currently no selection, so set anchor to wordstart */

        mlec->selectvalid      = TRUE;
        mlec->selectanchor.col = wordstart.col;     /* Safe, as word_limits returns valid character positions */
        mlec->selectanchor.row = wordstart.row;

        mlec__update_loop(mlec, mlec->selectanchor, mlec->cursor);
#else
        move_cursor(mlec, wordstart.col, wordstart.row);        /* Works, but causes text flow */
#endif
        mlec__selection_adjust(mlec, wordend.col, wordend.row);
    }
    else
    {
        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
            mlec__selection_adjust(mlec, wordend.col, wordend.row);
        else
            mlec__selection_adjust(mlec, wordstart.col, wordstart.row);
    }
}

void
clear_selection(
    MLEC_HANDLE mlec)
{
    if(mlec->selectvalid)
    {
        mlec__update_loop(mlec, mlec->selectanchor, mlec->cursor);
        mlec->selectvalid = FALSE;
    }
}

void
delete_selection(
    MLEC_HANDLE mlec)
{
    MARKED_TEXT range;

    if(range_is_selection(mlec, &range))
    {
        /* Since it is defined that any attempt to move the cursor will either clear the selection, or retain */
        /* it and extend it to the cursor position, the cursor MUST lie within selectstart..selectend         */

        assert(range.lower.start  >= mlec->lower.start);
        assert(range.lower.start  <= mlec->lower.end);
        assert(range.lower.end    == mlec->lower.end);
        assert(range.upper.end    >= mlec->upper.start);
        assert(range.upper.end    <= mlec->upper.end);
        assert(range.upper.start  == mlec->upper.start);

        mlec->cursor.lcol  = mlec->cursor.pcol = range.markstart.col;
        mlec->cursor.row   = range.markstart.row;
        mlec->linecount   -= range.marklines;
        mlec->lower.end    = range.lower.start;
        mlec->upper.start  = range.upper.end;

        if(range.marklines == 0)
            force_redraw_eoline(mlec);
        else
            force_redraw_eotext(mlec);

        mlec->selectvalid = FALSE;
    }
}

BOOL
range_is_selection(
    MLEC_HANDLE mlec,
    MARKED_TEXT *range)
{
    if(mlec->selectvalid)
    {
        MARK_POSITION markstart, markend;

        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        {
            markstart = mlec->selectanchor; markend.col = mlec->cursor.pcol; markend.row = mlec->cursor.row;
        }
        else
        {
            markstart.col = mlec->cursor.pcol; markstart.row = mlec->cursor.row; markend = mlec->selectanchor;
        }

        range->markstart = markstart;
        range->marklines = markend.row - markstart.row;

        /* NB if mark==cursor, find_offset returns mlec->upper.start */

        range->lower.end = mlec->lower.end;
        find_offset(mlec, &markstart, &range->lower.start);
        if(range->lower.start == mlec->upper.start)
            range->lower.start = mlec->lower.end;                      /* so cater for it here */

        range->upper.start = mlec->upper.start;
        find_offset(mlec, &markend, &range->upper.end);
        if(range->upper.end == mlec->lower.end)                        /* should never happen */
            range->lower.end = mlec->upper.start;

    /*>>>wrong - should whinge if no selection */
    }

    return(mlec->selectvalid);
}

/*
*
* NB if find(col,row) == cursor(col,row) this returns mlec->upper.start
*/

void
find_offset(
    MLEC_HANDLE mlec,
    MARK_POSITION *find,
    int *offsetp)
{
    char *buff = mlec->buffptr;

    if(find->col < 0)           /* The find->(col,row) values passed in should be valid as    */
        find->col = 0;          /* they are either cursor(pcol,row) or selectanchor(col,row), */

    if(find->row < 0)           /* but a little bomb proofing never hurt anyone!?.            */
    {
        find->row = 0;
        find->col = 0;
    }

    if((find->row == mlec->cursor.row) && (find->col < mlec->cursor.pcol))
    {
        /* easy case: cursor on required row, past required col */

        *offsetp = mlec->lower.end - mlec->cursor.pcol + find->col;
    }
    else
    {
        /* hard case: must identify and search appropriate buff_region */
        int col, row, off, lim;

        if(find->row < mlec->cursor.row)
        {
            /* search lower buff_region */

            col = 0;
            row = 0;
            off = mlec->lower.start;
            lim = mlec->lower.end;
        }
        else
        {
            /* search upper buff_region */

            col = mlec->cursor.pcol;
            row = mlec->cursor.row;
            off = mlec->upper.start;
            lim = mlec->upper.end;
        }

        while(
              (row < find->row) &&
              (off < lim)
             )
        {
            if(CR == buff[off++])
            {
                col = 0;
                row++;
            }
            else
            {
                col++;
            }
                }

        /* either found required row, or reached end-of-text */
        while(
              (col < find->col) &&
              (off < lim)       &&
              (CR != buff[off])
             )
        {
            col++;
            off++;
        }

        /* pass back closest col,row found, and its offset in the buffer */
        find->col = col; find->row = row; *offsetp = off;
    }
}

/******************************************************************************
*
* Update the specified region of the pane window
*
* This inverts the specified region
*
******************************************************************************/

static void
mlec__update_loop(
    MLEC_HANDLE mlec,
    MARK_POSITION mark1,
    CURSOR_POSITION mark2)
{
    wimp_redrawstr r;
    BOOL           more;
    MARK_POSITION  markstart, markend;

    /* quit now if null region, as doing the wimp_update_wind loop causes the caret to flicker */
    if((mark1.col == mark2.pcol) && (mark1.row == mark2.row))
        return;

    if((mark1.row < mark2.row) ||
       ((mark1.row == mark2.row) && (mark1.col < mark2.pcol))
      )
    {
        markstart = mark1; markend.col = mark2.pcol; markend.row = mark2.row;
    }
    else
    {
        markstart.col = mark2.pcol; markstart.row = mark2.row; markend = mark1;
    }

 trace_4(TRACE_MODULE_MLEC, "mlec__update_loop (%d,%d, %d,%d)",markstart.col,markstart.row,markend.col,markend.row);
    r.w      =  mlec->pane;
    r.box.x0 = -0x1FFFFFFF;
    r.box.x1 =  0x1FFFFFFF;

    r.box.y1 = -mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * markstart.row;
    r.box.y0 = -mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * (markend.row + 1);

    wimp_update_wind(&r, &more);
 trace_4(TRACE_MODULE_MLEC, "wimp_update_wind returns: (%d,%d,%d,%d) ",r.box.x0,r.box.y0,r.box.x1,r.box.y1);
 trace_2(TRACE_MODULE_MLEC, "(%d,%d) ",r.scx,r.scy);
 trace_4(TRACE_MODULE_MLEC, "(%d,%d,%d,%d)",r.g.x0,r.g.y0,r.g.x1,r.g.y1);

    while(more)
    {
        int orgx=r.box.x0-r.scx;       /* graphics units */
        int orgy=r.box.y1-r.scy;       /*>>>> could be put outside loop?*/

        show_selection(mlec, markstart, markend, orgx, orgy, r.g/*screenBB*/);

        wimp_get_rectangle(&r, &more);
    }
}

static int /*colnum*/
mycolourtran_returnColourNumber(
    wimp_paletteword entry)
{
    _kernel_swi_regs rs;
    rs.r[0] = entry.word;
    return(_kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) ? 0 : rs.r[0]);
}

static void
host_set_EOR_for_mlec(void)
{
    wimp_paletteword os_rgb_foreground;
    wimp_paletteword os_rgb_background;

    os_rgb_foreground.bytes.gcol  = 0;
    os_rgb_foreground.bytes.red   = 0x00;
    os_rgb_foreground.bytes.green = 0x00;
    os_rgb_foreground.bytes.blue  = 0x00;

    os_rgb_background.bytes.gcol  = 0;
    os_rgb_background.bytes.red   = 0xFF;
    os_rgb_background.bytes.green = 0xFF;
    os_rgb_background.bytes.blue  = 0xFF;

    { /* New machines can (and some demand) this mechanism */
    int colnum_foreground = mycolourtran_returnColourNumber(os_rgb_foreground);
    int colnum_background = mycolourtran_returnColourNumber(os_rgb_background);
    _kernel_swi_regs rs;

    rs.r[0] = 3;
    rs.r[1] = colnum_foreground ^ colnum_background;
    (void) _kernel_swi(OS_SetColour, &rs, &rs);
    } /*block*/
}

void
show_selection(
    MLEC_HANDLE mlec,
    MARK_POSITION markstart,
    MARK_POSITION markend,
    int orgx,
    int orgy,
    wimp_box screenBB)
{
    char     *lower_start;
    char     *lower_end;
    char     *upper_start;
    char     *upper_end;
    char     *ptr;
    wimp_box  lineBB;
    wimp_box  cursor;

    int lineCol, lineRow;

    lower_start = &mlec->buffptr[mlec->lower.start];            /* first character                         */
    lower_end   = &mlec->buffptr[mlec->lower.end];              /* 1 byte past character to left of cursor */
    upper_start = &mlec->buffptr[mlec->upper.start];            /* character to right of cursor            */
    upper_end   = &mlec->buffptr[mlec->upper.end];              /* 1 byte past last character              */

    /* bounding box of characters 0..cursor.pcol-1 on row cursor.row */

    cursor.x0 = orgx      + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT];
    cursor.x1 = cursor.x0 + mlec->cursor.pcol * mlec->charwidth;
    cursor.y1 = orgy      - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * mlec->cursor.row;
    cursor.y0 = cursor.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    host_set_EOR_for_mlec();

    {
    /* Consider characters to the right of the cursor and the lines below it */

    lineCol = mlec->cursor.pcol; lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x1;
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    ptr    = upper_start;

    /*>>>could put in code to skip lines that are above the current graphics window */

    while((lineBB.y1 > screenBB.y0) && (ptr < upper_end))
    {
        lineBB.x1  = lineBB.x0;

        while(ptr < upper_end)
        {
            if(CR == *ptr++)
            {
                lineBB.x1 += mlec->termwidth;
                break;
            }
            lineBB.x1 += mlec->charwidth;
        }

        if((lineRow >= markstart.row) && (lineRow <= markend.row))
        {
            if(lineRow == markstart.row)
            {
                int start = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * markstart.col;

                if(lineBB.x0 < start)
                    lineBB.x0 = start;
            }

            if(lineRow == markend.row)
            {
                int end = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * markend.col;

                if(lineBB.x1 > end)
                    lineBB.x1 = end;
            }

            if(lineBB.x0 < lineBB.x1)
            {
                /* NB lineBB is (L,B,R,T) edges which are (inc,inc,exc,exc), bbc_RectangleFill takes (inc,inc,inc,inc) */

                bbc_move(lineBB.x0, lineBB.y0);
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, lineBB.x1 - 1, lineBB.y1 - 1);
            }
        }

        lineCol = 0; lineRow++;

        lineBB.x0  = cursor.x0;
        lineBB.y1 -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0  = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are below the graphics window */
    }

    {
    /* Consider characters to the left of the cursor and the lines above it */

    lineCol = 0; lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x0;      /* NB x0 & x1 the same ie no CR width */
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    ptr = lower_end;

    /*>>>could put in code to skip lines that are below the current graphics window */

    while((lineBB.y0 < screenBB.y1) && (ptr > lower_start))
    {
      /*lineBB.x1  = lineBB.x0;*/

        while(ptr > lower_start)
        {
            if(CR == *--ptr)
                break;

            lineBB.x1 += mlec->charwidth;
        }

        if((lineRow >= markstart.row) && (lineRow <= markend.row))
        {
            if(lineRow == markstart.row)
            {
                int start = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * markstart.col;

                if(lineBB.x0 < start)
                    lineBB.x0 = start;
            }

            if(lineRow == markend.row)
            {
                int end = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * markend.col;

                if(lineBB.x1 > end)
                    lineBB.x1 = end;
            }

            if(lineBB.x0 < lineBB.x1)
            {
                /* NB lineBB is (L,B,R,T) edges which are (inc,inc,exc,exc), bbc_RectangleFill takes (inc,inc,inc,inc) */

                bbc_move(lineBB.x0, lineBB.y0);
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, lineBB.x1 - 1, lineBB.y1 - 1);
            }
        }

        lineCol = 0; --lineRow;

        lineBB.x0  = cursor.x0;
        lineBB.x1  = lineBB.x0 + mlec->termwidth; /* cos CR has width */
        lineBB.y1 += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0  = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are above the graphics window */
    }
}

#endif /* SUPPORT_SELECTION */

/******************************************************************************
*
* File load routines: LoadAtCursor
*
* File save routines: SaveAll & SaveSelection
*
* Selected area routines: Cut, Copy & Delete
*
******************************************************************************/

#if SUPPORT_LOADSAVE

int
mlec__atcursor_load(
    MLEC_HANDLE mlec,
    char *filename)
{
    trace_1(TRACE_MODULE_MLEC, "mlec__atcursor_load('%s')", filename);

    return(text_in(mlec, filename, file_read_open, file_read_getblock, file_read_close));
}

int
mlec__alltext_save(
    MLEC_HANDLE mlec,
    char *filename,
    FILETYPE_RISC_OS filetype,
    char *lineterm)
{
    int         err;
    MARKED_TEXT range;
    XFER_HANDLE handle;

    range_is_alltext(mlec, &range);

    if((err = file_write_open(&handle, filename, filetype)) >= 0)
        err = text_out(mlec, &handle, range, lineterm, file_write_size, file_write_putblock, file_write_close);
    return(err);
}

int
mlec__selection_save(
    MLEC_HANDLE mlec,
    char *filename,
    FILETYPE_RISC_OS filetype,
    char *lineterm)
{
    int         err;
    MARKED_TEXT range;
    XFER_HANDLE handle;

    if(range_is_selection(mlec, &range))
    {
        if((err = file_write_open(&handle, filename, filetype)) >= 0)
            err = text_out(mlec, &handle, range, lineterm, file_write_size, file_write_putblock, file_write_close);
        return(err);
        /*>>>should this clear the selection???*//*No, for consistancy with mlec__selection_copy */
    }

    return(create_error(MLEC_ERR_NOSELECTION));
}

static int
file_read_open(
    char *filename,
    P_FILETYPE_RISC_OS filetypep/*out*/,
    int *filesizep/*out*/,
    P_XFER_HANDLE xferhandlep/*out*/)
{
    int err;

    if((err = file_open(filename, file_open_read, &(xferhandlep->f))) >= 0)
    {
        *filesizep = err = (S32) file_length(xferhandlep->f);

        if(err < 0)
            file_close(&(xferhandlep->f));

        *filetypep = file_get_type(xferhandlep->f);
    }

    return(err);        /*>>>what about filetype checks???*/
}

static int
file_read_getblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize)
{
    return(file_read(dataptr, datasize, 1, xferhandlep->f));
}

static int
file_read_close(
    P_XFER_HANDLE xferhandlep)
{
    return(file_close(&(xferhandlep->f)));
}

static int
file_write_open(
    P_XFER_HANDLE xferhandlep/*out*/,
    char *filename,
    FILETYPE_RISC_OS filetype)
{
    int err;

    if((err = file_open(filename, file_open_write, &(xferhandlep->f))) >= 0)
        err = file_set_type(xferhandlep->f, filetype);

    return(err);
}

static int
file_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize)
{
    IGNOREPARM(xferhandlep);
    IGNOREPARM(xfersize);

    return(0);    /*>>>might be better to set the file extent to xfersize - ask Tutu */
}

static int
file_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize)
{
    return(file_write_err(dataptr, datasize, 1, xferhandlep->f));
}

static int
file_write_close(
    P_XFER_HANDLE xferhandlep)
{
    return(file_close(&(xferhandlep->f)));
}

#endif /* SUPPORT_LOADSAVE */

#if SUPPORT_CUTPASTE

int
mlec__atcursor_paste(
    MLEC_HANDLE mlec)
{
    trace_0(TRACE_MODULE_MLEC, "mlec__atcursor_paste");

    return(text_in(mlec, "", paste_read_open, paste_read_getblock, paste_read_close));
}

/******************************************************************************
*
* Copy selection to the paste buffer
*
* Doesn't clear the selection
*
******************************************************************************/

int
mlec__selection_copy(
    MLEC_HANDLE mlec)
{
    int         err;
    MARKED_TEXT range;
    XFER_HANDLE handle;

    reject_if_paste_buffer(mlec);

    if(range_is_selection(mlec, &range))
    {
        if((err = paste_write_open(&handle)) >= 0)
            err = text_out(mlec, &handle, range, NULL, paste_write_size, paste_write_putblock, paste_write_close);
        return(err);
        /*>>>should this clear the selection???*//*No, windows doesn't*/
    }

    return(create_error(MLEC_ERR_NOSELECTION));
}

/******************************************************************************
*
* Cut the selection into the paste buffer
*
* Same as Copy-then-Delete
*
******************************************************************************/

int
mlec__selection_cut(
    MLEC_HANDLE mlec)
{
    int err;

    if((err = mlec__selection_copy(mlec)) >= 0)
        mlec__selection_delete(mlec);           /* delete selection only if copy succeeds */

    return(err);
}

static int
paste_read_open(
    char *filename,
    P_FILETYPE_RISC_OS filetypep/*out*/,
    int *filesizep/*out*/,
    P_XFER_HANDLE xferhandlep/*out*/)
{
    MARKED_TEXT range;

    IGNOREPARM(filename);

    xferhandlep->p = paste;

    if(!paste)
        return(create_error(MLEC_ERR_NOPASTEBUFFER));

    range_is_alltext(paste, &range);

    *filetypep = FILETYPE_TEXT;
    *filesizep = ((range.lower.end - range.lower.start) +
                  (range.upper.end - range.upper.start)
                 );

    return(0);
}

/******************************************************************************
*
* NB The data size field is ignored, all the data is written to dataptr
*
******************************************************************************/

static int
paste_read_getblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize)
{
    MLEC_HANDLE mlec = xferhandlep->p;

    datasize = datasize;

    if(mlec)
    {
        MARKED_TEXT  range;
        char        *buff = mlec->buffptr;
        int          i;

        range_is_alltext(mlec, &range);

        for(i = range.lower.start; i < range.lower.end; i++)
            *dataptr++ = buff[i];

        for(i = range.upper.start; i < range.upper.end; i++)
            *dataptr++ = buff[i];

        return(0);
    }

    return(create_error(MLEC_ERR_BUFFERWENT_AWOL));
}

static int
paste_read_close(
    P_XFER_HANDLE xferhandlep)
{
    xferhandlep->p = NULL;
    return(0);
}

static int
paste_write_open(
    P_XFER_HANDLE xferhandlep/*out*/)
{
    if(!paste)
        status_return(mlec_create(&paste));

    xferhandlep->p = paste;

    if(!paste)
        return(create_error(MLEC_ERR_NOPASTEBUFFER));

    return(mlec_SetText(paste, ""));
}

static int
paste_write_size(
    P_XFER_HANDLE xferhandlep,
    int xfersize)
{
    /* Since paste_write_open does mlec_SetText(paste, "") to clear all text (and selection!), */
    /* we can use either checkspace_deletealltext or checkspace_delete_selection               */

    return(checkspace_delete_selection(xferhandlep->p, xfersize + 1));  /* plus 1 to allow room for terminator */
}

static int
paste_write_putblock(
    P_XFER_HANDLE xferhandlep,
    char *dataptr,
    int datasize)
{
    MLEC_HANDLE mlec = xferhandlep->p;

    if(mlec)
    {
        char *ptr = &mlec->buffptr[mlec->lower.end];
        int   i;

        for(i = 0; i < datasize; i++)
            ptr[i] = *dataptr++;
        ptr[i] = '\0';

        return(mlec__insert_text(mlec, ptr));   /* space test WILL be successful */
                                                /*>>> could flex space move???   */
    }

    return(create_error(MLEC_ERR_BUFFERWENT_AWOL));
}

static int
paste_write_close(
    P_XFER_HANDLE xferhandlep)
{
    xferhandlep->p = NULL;
    return(0);
}

#endif /* SUPPORT_CUTPASTE */

#if SUPPORT_LOADSAVE || SUPPORT_CUTPASTE || SUPPORT_GETTEXT

void
range_is_alltext(
    MLEC_HANDLE mlec,
    MARKED_TEXT *range)
{
    range->markstart.col = range->markstart.row = 0;
    range->marklines     = mlec->linecount;
    range->lower         = mlec->lower;
    range->upper         = mlec->upper;
}

int
text_in(
    MLEC_HANDLE mlec,
    char *filename,
    int (*openp)(char *filename, P_FILETYPE_RISC_OS filetypep, int *filesizep, P_XFER_HANDLE xferhandlep),
    int (*readp)(P_XFER_HANDLE xferhandlep, char *dataptr, int datasize),
    int (*closep)(P_XFER_HANDLE xferhandlep)
    )
{
    int err;
    XFER_HANDLE handle;
    FILETYPE_RISC_OS filetype;
    int filesize;

    if((err = (*openp)(filename, &filetype, &filesize, &handle)) >= 0)
    {
        if((err = checkspace_delete_selection(mlec, filesize + 1)) >= 0)           /* plus 1 to allow room for terminator */
        {
            if((err = (*readp)(&handle, &mlec->buffptr[mlec->lower.end], filesize)) >= 0)
            {
                char *ptr = &mlec->buffptr[mlec->lower.end];
                ptr[filesize] = '\0';
                mlec__insert_text(mlec, ptr); /* space test WILL be successful */
            }
        }

        if(err >= 0)
            err = (*closep)(&handle);   /* no error so far, may get one from close operation */
        else
            (*closep)(&handle);         /* must do close, but we want the earlier error */
    }

    scroll_until_cursor_visible(mlec);  /* In case an error stops mlec__insert_text doing one for us */
    return(err);
}

int
text_out(
    MLEC_HANDLE mlec,
    P_XFER_HANDLE xferhandlep,
    MARKED_TEXT range,
    char *lineterm,
    int (*sizep)(P_XFER_HANDLE xferhandlep, int xfersize),
    int (*writep)(P_XFER_HANDLE xferhandlep, char *dataptr, int datasize),
    int (*closep)(P_XFER_HANDLE xferhandlep)
    )
{
    int err;
    int xfersize = ((range.lower.end - range.lower.start) + (range.upper.end - range.upper.start));
                     /* carefull, size valid for (lineterm == NULL) or (sizeof(lineterm) == 1) */
                     /* since only paste_write_open expects this to be valid we should be OK   */
                     /*>>>think about this, can we do calculation properly*/
    if((err = (*sizep)(xferhandlep, xfersize)) >= 0)
    {
        char *buff = mlec->buffptr;

        if(lineterm)
        {
            /* output the text line-by-line, replacing our terminators by the caller-supplied sequence */

            BOOL term;
            int  size;
            int  linestart, lineend;

            for(linestart = range.lower.start; linestart < range.lower.end; linestart = lineend)
            {
                for(term = FALSE, lineend = linestart; lineend < range.lower.end; lineend++)
                {
                    if (TRUE == (term = (buff[lineend] == CR)))
                        break;
                }

                if(((size = lineend - linestart) > 0) && (err >= 0))
                    err = (*writep)(xferhandlep, &buff[linestart], size);

                /* if we stopped at eol, output the supplied terminator char(s) */
                if(term && (err >= 0))
                {
                    err = (*writep)(xferhandlep, lineterm, strlen(lineterm));
                    lineend++;              /* not forgetting to skip our term */
                }
            }

            for(linestart = range.upper.start; linestart < range.upper.end; linestart = lineend)
            {
                for(term = FALSE, lineend = linestart; lineend < range.upper.end; lineend++)
                {
                    if (TRUE == (term = (buff[lineend] == CR)))
                        break;
                }

                if(((size = lineend - linestart) > 0) && (err >= 0))
                    err = (*writep)(xferhandlep, &buff[linestart], size);

                /* if we stopped at eol, output the supplied terminator char(s) */
                if(term && (err >= 0))
                {
                    err = (*writep)(xferhandlep, lineterm, strlen(lineterm));
                    lineend++;              /* not forgetting to skip our term */
                }
            }
        }
        else
        {
            /* not bothered about line terminator characters, so blast the data out in 2 (max) lumps */

            int size;

            if(((size = range.lower.end - range.lower.start) > 0) && (err >= 0))
                err = (*writep)(xferhandlep, &buff[range.lower.start], size);

            if(((size = range.upper.end - range.upper.start) > 0) && (err >= 0))
                err = (*writep)(xferhandlep, &buff[range.upper.start], size);
        }

        if(err >= 0)
            err = (*closep)(xferhandlep);   /* no error so far, may get one from close operation */
        else
            (*closep)(xferhandlep);         /* must do close, but we want the earlier error */
    }

    return(err);
}
#endif

#if SUPPORT_PANEMENU
/* Pane window menu structure */

#define MENU_ROOT_SAVE      1
#define                         MENU_SAVE_FILE        1
#define                         MENU_SAVE_SELECTION   2
#define MENU_ROOT_SELECTION 2
#define                         MENU_SELECTION_CLEAR  1
#define                         MENU_SELECTION_COPY   2
#define                         MENU_SELECTION_CUT    3
#define                         MENU_SELECTION_DELETE 4
#define MENU_ROOT_PASTE     3

static menu mlec_menu_root      = NULL;
static menu mlec_menu_save      = NULL;
static menu mlec_menu_selection = NULL;
#endif

#if SUPPORT_PANEMENU
/******************************************************************************
*
* mlec__event_menu_maker  - constructs a static menu skeleton
* mlec__event_menu_filler - ticks/shades skeleton when 'menu' is pressed
* mlec__event_menu_proc   - processes menu actions
*
******************************************************************************/

static void mlec__event_menu_maker(const char *menu_title)
{
    if(!mlec_menu_root)
    {
        mlec_menu_root      = menu_new_c(menu_title,
                                         string_lookup(MLEC_MSG_MENUBDY));

        mlec_menu_save      = menu_new_c(string_lookup(MLEC_MSG_MENUHDR_SAVE),
                                         string_lookup(MLEC_MSG_MENUBDY_SAVE));
        mlec_menu_selection = menu_new_c(string_lookup(MLEC_MSG_MENUHDR_SELECTION),
                                         string_lookup(MLEC_MSG_MENUBDY_SELECTION));

        menu_submenu(mlec_menu_root, MENU_ROOT_SAVE     , mlec_menu_save);
        menu_submenu(mlec_menu_root, MENU_ROOT_SELECTION, mlec_menu_selection);
    }
}

menu mlec__event_menu_filler(void *handle)
{
    MLEC_HANDLE mlec = (MLEC_HANDLE)handle;

    if(mlec_menu_root)
    {
        BOOL fade = !mlec->selectvalid;

        menu_setflags(mlec_menu_root, MENU_ROOT_PASTE, FALSE, paste == NULL);    /*>>>Paste NYA, so fade it*/

        if(mlec_menu_save)
        {
            menu_setflags(mlec_menu_save, MENU_SAVE_SELECTION, FALSE, fade);
        }

        if(mlec_menu_selection)
        {
            /* Copy,Cut & Delete only allowed for a valid selection */

            menu_setflags(mlec_menu_selection, MENU_SELECTION_CLEAR , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_COPY  , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_CUT   , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_DELETE, FALSE, fade);
        }
    }

    return(mlec_menu_root);
}

static dbox
mlec__save_dbox = NULL;

static void
mlec__event_save(
    MLEC_HANDLE mlec,
    BOOL selection)
{
    if(0 != (mlec__save_dbox = dbox_new("xfer_send")))
    {
        dbox_show(mlec__save_dbox);

        (void) xfersend_x(FILETYPE_TEXT,
                    selection
                        ? "Selection"
                        : "TextFile",
                    1024,
                    selection
                        ? mlec__saveselection
                        : mlec__saveall,          /* file save */
                    0,                            /* ram xfer */
                    0,                            /* print */
                    (void*) mlec,
                    mlec__save_dbox, NULL, NULL);

        dbox_hide(mlec__save_dbox);
        dbox_dispose(&mlec__save_dbox);
    }
}

BOOL mlec__event_menu_proc(void *handle, char *hit, BOOL submenurequest)
{
    MLEC_HANDLE mlec = (MLEC_HANDLE)handle;
    int         err  = 0;

    IGNOREPARM(submenurequest);

    switch(*hit++)
    {
    case MENU_ROOT_SAVE:
        switch(*hit++)
        {
        case MENU_SAVE_FILE:
            mlec__event_save(mlec, FALSE);
            break;

        case MENU_SAVE_SELECTION:
            mlec__event_save(mlec, TRUE);
            break;
        }
        break;

    case MENU_ROOT_SELECTION:
        switch(*hit++)
        {
        case MENU_SELECTION_CLEAR:
            mlec__selection_clear(mlec);
            break;

        case MENU_SELECTION_COPY:
            err = mlec__selection_copy(mlec);
            break;

        case MENU_SELECTION_CUT:
            err = mlec__selection_cut(mlec);
            break;

        case MENU_SELECTION_DELETE:
            mlec__selection_delete(mlec);
            break;
        }
        break;

    case MENU_ROOT_PASTE:
        err = mlec__atcursor_paste(mlec);
        break;
    }

    /* Any error is the result of direct user interaction with the mlec, */
    /* so report the error here, cos there is no caller to return it to. */
    if(err < 0)
        report_error(mlec, err);

    return(TRUE);
}

/******************************************************************************
*
* xfersend_saveproc's
*
* mlec__saveall       - SaveAll       } to the filing system
* mlec__saveselection - SaveSelection }
*
* Returns
*   TRUE if save was successful
*   FALSE if not, having reported the error
*
* NB only to be called by xfersend
*
******************************************************************************/

BOOL mlec__saveall(/*const*/ char *filename, void *handle)
{
    /* Reorder params and bend types as needed */

    MLEC_HANDLE mlec = (MLEC_HANDLE)handle;
    int         err;

    if((err = mlec__alltext_save(mlec, filename, FILETYPE_TEXT, lineterm_LF)) < 0)
    {
        report_error(mlec, err);        /* Report the error here, cos there is no caller to return it to. */
        return(FALSE);
    }

    return(TRUE);
}

BOOL mlec__saveselection(/*const*/ char *filename, void *handle)
{
    /* Reorder params and bend types as needed */

    MLEC_HANDLE mlec = (MLEC_HANDLE)handle;
    int         err;

    if((err = mlec__selection_save(mlec, filename, FILETYPE_TEXT, lineterm_LF)) < 0)
    {
        report_error(mlec, err);        /* Report the error here, cos there is no caller to return it to. */
        return(FALSE);
    }

    return(TRUE);
}
#endif /* SUPPORT_PANEMENU */

/* end of mlec.c */
