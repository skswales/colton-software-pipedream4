/* datatype.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* PipeDream datatypes */

/* RJM 8-Apr-1989 */

#ifndef __datatype_h
#define __datatype_h

#if RISCOS
#define uchar char
#endif

#ifndef BOOL
#define BOOL int
#endif

typedef uchar BOOLEAN;          /* packed flag */

/* handle of a graphics DDE link */
typedef S32 ghandle;

/* ----------------------------------------------------------------------- */

typedef int coord;      /* coordinate type for screen, MUST BE SIGNED */
typedef int gcoord;     /* graphics coordinate type */
typedef int tcoord;     /* text cell coordinate type */

/*
Column & Row types
*/

/*
 * 29 bit quantities, bit 29 indicates absolute, bit 30 indicates bad
 * -ve (bit 31) used for falling out of loops
*/

typedef S32 COL; typedef COL * P_COL;

#define NO_COL                  ((COL) ((1 << 30)-1))
#define LARGEST_COL_POSSIBLE    ((COL) ((1 << 29)-1))
#define COLNOBITS               LARGEST_COL_POSSIBLE
#define ABSCOLBIT               ((COL) (1 << 29))
#define BADCOLBIT               ((COL) (1 << 30))

#define COL_TFMT S32_TFMT

typedef S32 ROW; typedef ROW * P_ROW;

#define NO_ROW                  ((ROW) ((1 << 30) -1)
#define LARGEST_ROW_POSSIBLE    ((ROW) ((1 << 29) -1))
#define ROWNOBITS               LARGEST_ROW_POSSIBLE
#define ABSROWBIT               ((ROW) (1 << 29))
#define BADROWBIT               ((ROW) (1 << 30))

#define ROW_TFMT S32_TFMT

/******************************************************************************
* other constants
******************************************************************************/

#define CMD_BUFSIZ  ((S32) 255)        /* size of buffers */
#define LIN_BUFSIZ  ((S32) 1000)
#define MAXFLD      (LIN_BUFSIZ - 1)

/* On RISC OS the worst case is MAXFLD ctrlchars being expanded to
 * four characters each plus an overhead for an initial font change and CH_NULL
*/
#if 1
/* SKS after PD 4.12 23mar92 - reduce to avoid excessive stack consumption (^b and ^c now stripped on load which were the main culprits...) */
#define PAINT_STRSIZ    (LIN_BUFSIZ * 2)
#else
#define PAINT_STRSIZ    (LIN_BUFSIZ * 4)
#endif

/* Output buffer for text cell compilation: worst case is LINBUF full of
 * small SLR, op such as '@A1@'
*/
#define COMPILED_TEXT_BUFSIZ ((LIN_BUFSIZ / 4) * (COMPILED_TEXT_SLR_SIZE + 2))

/* maximum length of a textual cell reference [fullname]%$ABCDEFGH$1234567890 */
#define BUF_MAX_REFERENCE (BUF_MAX_PATHSTRING + 2 + 8 + 1 + 10)

/* maximum size of a low level cell */
#define MAX_SLOTSIZE (EV_MAX_OUT_LEN + sizeof(struct CELL))

/* ----------------------------------------------------------------------- */

/* structures for expression analyser */

#define DOCNO EV_DOCNO /* NB packed */

typedef struct SLR /* SLR with just col, row */
{
    COL col;
    ROW row;
}
SLR, * P_SLR; typedef const SLR * PC_SLR;

#define SLR_INIT { 0, 0 }

typedef struct FULL_SLR /* SLR with col, row and an EV_DOCNO for completeness */
{
    COL col;
    ROW row;
    EV_DOCNO docno;
}
FULL_SLR, * P_FULL_SLR; typedef const FULL_SLR * PC_FULL_SLR;

#define COMPILED_TEXT_SLR_SIZE (1/*SLRLD1*/ + 1/*SLRLD2*/ + sizeof(EV_DOCNO) + sizeof(COL) + sizeof(ROW))

/*
a type to go traversing blocks with
*/

typedef enum TRAVERSE_BLOCK_DIRECTION
{
    TRAVERSE_ACROSS_ROWS,
    TRAVERSE_DOWN_COLUMNS
}
TRAVERSE_BLOCK_DIRECTION;

typedef struct TRAVERSE_BLOCK
{
    /* contents are private to traversal routines */
    DOCNO docno;
    SLR cur;
    SLR stt;
    SLR end;
    TRAVERSE_BLOCK_DIRECTION direction;
    BOOL start;
    P_LIST_ITEM it; /* for fast traversal */
    struct DOCU * p_docu;
}
TRAVERSE_BLOCK, * TRAVERSE_BLOCKP;

#define TRAVERSE_BLOCK_INIT { \
    DOCNO_NONE, \
    SLR_INIT, \
    SLR_INIT, \
    SLR_INIT, \
    TRAVERSE_ACROSS_ROWS, \
    FALSE, \
    NULL, \
    NULL, \
}

#define traverse_block_cur_col(blk) ((blk)->cur.col)
#define traverse_block_cur_row(blk) ((blk)->cur.row)

/* ----------------------------- lists.c --------------------------------- */

#include "lists_x.h"

#include "cmodules/collect.h"

/******************************************************************************
* slot.c
******************************************************************************/

typedef struct CELL * P_CELL; /* cell pointer type */

/*
column entry for use in sparse matrix
*/

typedef struct COLENTRY
{
    LIST_BLOCK lb;
    S32 wrapwidth;
    S32 colwidth;
    S32 colflags;
}
COLENTRY, * P_COLENTRY;

/* object on deleted words list describing a block */

typedef struct SAVED_BLOCK_DESCRIPTOR
{
    P_COLENTRY del_colstart;
    COL del_col_size;
    ROW del_row_size;
}
SAVED_BLOCK_DESCRIPTOR;

/* ------------------------------ commlin.c ------------------------------ */

#define short_menus()   (d_menu[0].option)
#define classic_menus() (d_menu[1].option)

#define MF_NOMENU 0

#define MF_LONG_SHORT_CHANGED 1
#define MF_LONG_ONLY    2
#define MF_TICKABLE     4
#define MF_TICK_STATUS  8
#define MF_GREYABLE     16
#define MF_GREY_STATUS  32
#define MF_HAS_DIALOG   64
#define MF_DOCUMENT_REQUIRED 128  /* document required to do this command */
#define MF_GREY_EXPEDIT 256
#define MF_NO_REPEAT    512  /* REPEAT COMMAND ignores this */
#define MF_NO_MACROFILE 1024 /* don't output this command in the macro recorder */
#define MF_ALWAYS_SHORT 2048 /* some commands can't be removed from short menus */
#define MF_DO_MERGEBUF  4096 /* mergebuf() prior to command */

#define on_short_menus(flag) ( \
    !((((flag) & MF_LONG_ONLY) == 0) ^ (((flag) & MF_LONG_SHORT_CHANGED) == 0)) )

typedef struct MENU
{
    const char *command;
    S32 flags;
    PC_U8 *title;
    void (*cmdfunc)(void);
    S32 funcnum;
    S32 classic_key;
    S32 sg_key;
}
MENU;

typedef struct MENU_HEAD
{
    PC_U8 *name;
    MENU *tail;
    U32 items;
    S32 titlelen;
    S32 commandlen;
    void * m; /* abuse: RISC OS submenu^ kept here */
}
MENU_HEAD;

/* ----------------------------- scdraw.c -------------------------------- */

/* horizontal and vertical screen tables */

typedef struct SCRROW
{
    ROW rowno;
    S32 page;
    uchar flags;
}
SCRROW, * P_SCRROW;

typedef struct SCRCOL
{
    COL colno;
    uchar flags;
}
SCRCOL, * P_SCRCOL;

/* ------------------------------ dialog.c ------------------------------- */

typedef S32 optiontype;

static inline void
set_option_from_u32(optiontype * const p_optiontype, U32 u32)
{
    * (P_U32) p_optiontype = u32;
}

static inline U32
get_option_as_u32(const optiontype * const p_optiontype)
{
    return(* (PC_U32) p_optiontype);
}

typedef struct DIALOG
{
    uchar type;         /* type of field, text, number, special */
    uchar ch1;          /* first character of save option string */
    uchar ch2;          /* second character of save option string */
    /* uchar _spare; */
    optiontype option;  /* single character option e.g. Y, sometimes int index */
    PC_U8Z *optionlist; /* range of possible values for option, first is default */
    char *textfield;    /* user specified name of something */
    U32 wv_offset;      /* of corresponding variable in windvars */
}
DIALOG;

/* dialog box entry checking: n is maximum used in d_D[n].whatever expr. */
#define assert_dialog(n, D) assert((n+1) == dialog_head[D].items)

typedef void (* dialog_proc) (DIALOG *dptr);

typedef struct DHEADER
{
    DIALOG * dialog_box;
    S32 items;
    S32 flags;
    dialog_proc dproc;
    const char * dname;
}
DHEADER;

/* --------------------------- cursmov.c --------------------------------- */

#define SAVE_DEPTH 5

typedef struct SAVPOS
{
    COL ref_col;
    ROW ref_row;
    DOCNO ref_docno;
}
SAVPOS;

/* --------------------------- pdriver.c -------------------------------- */

typedef struct DRIVER
{
    BOOL (* out) (_InVal_ U8 ch);
    BOOL (* on)  (void);
    BOOL (* off) (BOOL ok);
}
DRIVER;

/* --------------------------- numbers.c -------------------------------- */

/*
structure of reference to a draw file
*/

typedef struct DRAW_FILE_REF
{
    IMAGE_CACHE_HANDLE draw_file_key;
    EV_DOCNO docno;
    COL col;
    ROW row;
    F64 x_factor;
    F64 y_factor;
    S32 x_size_os;
    S32 y_size_os;
}
DRAW_FILE_REF, * P_DRAW_FILE_REF, ** P_P_DRAW_FILE_REF;

/*
 * structure of an entry in the graphics link list
 * ghandle of entry used as key
*/

typedef struct GRAPHICS_LINK_ENTRY
{
    DOCNO docno;            /* where the block is */
    COL col;
    ROW row;

    S32 task;               /* task id of client */
    BOOL update;            /* does client want updates? */
    BOOL datasent;          /* data sent without end marker */

    S32 ext_dep_han;        /* handle to external dependency */
    ghandle ghan;
    S32 x_size;
    S32 y_size;
    //char text[1];           /* leafname & tag, 0-terminated */
}
GRAPHICS_LINK_ENTRY, * P_GRAPHICS_LINK_ENTRY; typedef const GRAPHICS_LINK_ENTRY * PC_GRAPHICS_LINK_ENTRY;

/* --------------------------- riscos.c ---------------------------------- */

/* Abstract objects for export */
typedef struct RISCOS__RedrawWindowBlock * RISCOS_RedrawWindowBlock;

typedef struct RISCOS_FILEINFO
{
    unsigned int exec; /* order important! */
    unsigned int load;
    unsigned int length;
}
RISCOS_FILEINFO;

typedef void (* RISCOS_REDRAWPROC) (_In_ const RISCOS_RedrawWindowBlock * const redraw_window_block);

typedef void (* RISCOS_PRINTPROC) (void);

/* ----------------------------------------------------------------------- */

enum DRIVER_TYPES
{
    driver_riscos,
    driver_parallel,
    driver_serial,
    driver_network,
    driver_user
};

/* set of colours used by PipeDream */

typedef enum D_COLOUR_OFFSETS
{
    COI_FORE,
    COI_BACK,
    COI_PROTECT,
    COI_NEGATIVE,
    COI_GRID,
    COI_CARET,
    COI_SOFTPAGEBREAK,
    COI_HARDPAGEBREAK,

    COI_BORDERFORE,
    COI_BORDERBACK,
    COI_CURRENT_BORDERFORE,
    COI_CURRENT_BORDERBACK,
    COI_FIXED_BORDERBACK,

    N_COLOURS
}
COLOURS_OPTION_INDEX;

/* the following are entries in the print dialog box */

enum D_PRINT_OFFSETS
{
    P_PSFON,
    P_PSFNAME,
    P_DATABON,
    P_DATABNAME,
    P_OMITBLANK,
    P_BLOCK,
    P_TWO_ON,
    P_TWO_MARGIN,
    P_COPIES,
    P_WAIT,
    P_ORIENT,
    P_SCALE,
    P_THE_LAST_ONE
};

/* entries in the sort block dialog box */

#define SORT_FIELD_DEPTH        5           /* number of col,ascend pairs */
#define SORT_FIELD_COLUMN       0
#define SORT_FIELD_ASCENDING    1

/* RJM removes update refs 14.9.91 */
#if 1
#define SORT_MULTI_ROW        (SORT_FIELD_COLUMN + (SORT_FIELD_DEPTH * 2))
#else
#define SORT_UPDATE_REFS        (SORT_FIELD_COLUMN + (SORT_FIELD_DEPTH * 2))
#define SORT_MULTI_ROW          (SORT_UPDATE_REFS + 1)
#endif

typedef struct PDCHART_TAGSTRIP_INFO
{
    /*
    out
    */

    P_ANY pdchartdatakey;
}
* P_PDCHART_TAGSTRIP_INFO;

typedef struct PD_CONFIG
{
    P_NUMFORM_CONTEXT p_numform_context;
}
PD_CONFIG, * P_PD_CONFIG; typedef const PD_CONFIG * PC_PD_CONFIG;

#endif /* __datatype_h */

/* end of datatype.h */
