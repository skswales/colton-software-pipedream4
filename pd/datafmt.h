/* datafmt.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the data structures for PipeDream */

/* RJM August 1987 */

/* get data type definitions */
#include "datatype.h"

/* get string definitions */
#include "strings.h"

/* get declarations of variables for each window/file */
#include "windvars.h"

/* get declarations of globals */
#include "progvars.h"

/* get declarations of marked block functions */
#include "markers.h"

/* returns from current function with FALSE */
#define false_return(expr) \
    do { \
    const BOOL bool_e = (expr); \
    if(!bool_e) \
        return(bool_e); \
    } while_constant(0)

/******************************************************************************
* character definitions
******************************************************************************/

#define SLRLD2                  CH_NULL /* this makes detecting SLRLD1 trivial and really helps debug! */
#define TAB                     ((uchar) 0x09)
#define FORMFEED                ((uchar) 0x0C)
#define FUNNYSPACE              ((uchar) 0x0E)
#define SLRLD1                  ((uchar) 0x10)
/* DO NOT CHANGE the FIRST_HIGHLIGHT..LAST_HIGHLIGHT range - these are 'well-known' and inserted by various spreadsheet users */
#define FIRST_HIGHLIGHT         ((uchar) 0x18)
#define FIRST_HIGHLIGHT_TEXT    ((uchar) '1')
#define CMDLDI                  (FIRST_HIGHLIGHT + 1)
#define CTRLZ                   ((uchar) 0x1A)
#define ESCAPE                  ((uchar) 0x1B)
#define LAST_HIGHLIGHT          ((uchar) 0x1F)
#define LAST_HIGHLIGHT_TEXT     ((LAST_HIGHLIGHT - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT)
#define SPACE                   ((uchar) 0x20)
#define DUMMYHAT                FORMFEED
#define COMMA                   ((uchar) ',')
#define QUOTES                  ((uchar) '\"')
#define CTRLDI                  ((uchar) '|')
#define QUERY                   ((uchar) '?')
#define DOT                     ((uchar) '.')

enum HIGHLIGHT_NUMBERS
{
    HIGH_UNDERLINE      =   FIRST_HIGHLIGHT + 1 - 1,
    HIGH_BOLD           =   FIRST_HIGHLIGHT + 2 - 1,
    HIGH_ITALIC         =   FIRST_HIGHLIGHT + 4 - 1,
    HIGH_SUBSCRIPT      =   FIRST_HIGHLIGHT + 5 - 1,
    HIGH_SUPERSCRIPT    =   FIRST_HIGHLIGHT + 6 - 1
};

enum HIGHLIGHT_BITS
{
    N_UNDERLINE     =   1,  /* highlight 1 */
    N_BOLD          =   2,  /* highlight 2 */
    N_ITALIC        =   8,  /* highlight 4 */
    N_SUBSCRIPT     =   16, /* highlight 5 */
    N_SUPERSCRIPT   =   32  /* highlight 6 */
};

#define ishighlight(ch) ( /*BOOL*/ \
    (ch <= LAST_HIGHLIGHT)  &&  (FIRST_HIGHLIGHT <= ch) )

#define ishighlighttext(ch) ( /*BOOL*/ \
    (ch <= LAST_HIGHLIGHT_TEXT)  &&  (FIRST_HIGHLIGHT_TEXT <= ch) )

#if 0
#define get_text_at_char() ('@')
#define is_text_at_char(c) ('@' == (c))
#else
#define get_text_at_char() ( \
    (NULL == d_options_TA)   \
        ? CH_NULL            \
        : *d_options_TA      ) /* may be CH_NULL */

#define is_text_at_char(c) ( \
    ((NULL == d_options_TA)  ||  (CH_NULL == *d_options_TA)) \
        ? FALSE              \
        : (*d_options_TA == (c)) )
#endif

/******************************************************************************
* key & function key assignments
******************************************************************************/

/* values for inkey in depressed(n) */

#define            SHIFT    -1
#define             CTRL    -2
#define              ALT    -3
#define DEPRESSED_ESCAPE    0

/*
extra bits added in to basic key codes
*/

#define KMAP_CODE_ADDED_SHIFT 0x0100
#define KMAP_CODE_ADDED_CTRL  0x0200
#define KMAP_CODE_ADDED_ALT   0x0400 /* 'alt' letters */
#define KMAP_CODE_ADDED_FN    0x0800 /* function keys */

#define RISCOS_EKEY_ESCAPE 27

#define KMAP_ESCAPE 27

/* function keys starting at F0 .. F15 */

#define KMAP_BASE_FUNC          (KMAP_CODE_ADDED_FN + 0x00)
#define KMAP_BASE_SFUNC         (KMAP_BASE_FUNC | KMAP_CODE_ADDED_SHIFT)
#define KMAP_BASE_CFUNC         (KMAP_BASE_FUNC                         | KMAP_CODE_ADDED_CTRL)
#define KMAP_BASE_CSFUNC        (KMAP_BASE_FUNC | KMAP_CODE_ADDED_SHIFT | KMAP_CODE_ADDED_CTRL)

/* function-like keys (1) */

#define KMAP_FUNC_PRINT         (KMAP_BASE_FUNC    + 0x00)
#define KMAP_FUNC_SPRINT        (KMAP_BASE_SFUNC   + 0x00)
#define KMAP_FUNC_CPRINT        (KMAP_BASE_CFUNC   + 0x00)
#define KMAP_FUNC_CSPRINT       (KMAP_BASE_CSFUNC  + 0x00)

#define KMAP_FUNC_INSERT        (KMAP_BASE_FUNC    + 0x0D)
#define KMAP_FUNC_SINSERT       (KMAP_BASE_SFUNC   + 0x0D)
#define KMAP_FUNC_CINSERT       (KMAP_BASE_CFUNC   + 0x0D)
#define KMAP_FUNC_CSINSERT      (KMAP_BASE_CSFUNC  + 0x0D)

/* function-like keys (2) */

#define KMAP_BASE_FUNC2         (KMAP_CODE_ADDED_FN + 0x10)
#define KMAP_BASE_SFUNC2        (KMAP_BASE_FUNC2 | KMAP_CODE_ADDED_SHIFT)
#define KMAP_BASE_CFUNC2        (KMAP_BASE_FUNC2                         | KMAP_CODE_ADDED_CTRL)
#define KMAP_BASE_CSFUNC2       (KMAP_BASE_FUNC2 | KMAP_CODE_ADDED_SHIFT | KMAP_CODE_ADDED_CTRL)

/* Common function key assignments */

enum CLASSIC_KEY_VALUES
{
    CLASSIC_KEY_HELP            = KMAP_BASE_FUNC   +  1, /* same as SG */
    CLASSIC_KEY_EDIT_EXPRESSION = KMAP_BASE_FUNC   +  2,
    CLASSIC_KEY_SAVE            = KMAP_BASE_FUNC   +  3, /* same as SG */
    CLASSIC_KEY_SEARCH          = KMAP_BASE_FUNC   +  4, /* same as SG */

    CLASSIC_KEY_REPEAT_COMMAND  = KMAP_BASE_FUNC   +  5,
    CLASSIC_KEY_NEXT_MATCH      = KMAP_BASE_FUNC   +  6,
    CLASSIC_KEY_INSERT_ROW      = KMAP_BASE_FUNC   +  7, /* moves to CF7 in SG mode */
    CLASSIC_KEY_DELETE_ROW      = KMAP_BASE_FUNC   +  8, /* moves to CF8 in SG mode */

    CLASSIC_KEY_PASTE           = KMAP_BASE_FUNC   +  9,
    CLASSIC_KEY_DELETE_WORD     = KMAP_BASE_FUNC   + 10,
    CLASSIC_KEY_DELETE_EOL      = KMAP_BASE_FUNC   + 11,
    /* leave F12 well alone */

    CLASSIC_KEY_GOTO            = KMAP_BASE_SFUNC  +  5, /* added in 4.56 */
    CLASSIC_KEY_SORT            = KMAP_BASE_SFUNC  +  6, /* added in 4.56 */
    CLASSIC_KEY_INSERT_COLUMN   = KMAP_BASE_SFUNC  +  7, /* added in 4.56 (same as Fireworkz) */
    CLASSIC_KEY_DELETE_COLUMN   = KMAP_BASE_SFUNC  +  8, /* added in 4.56 (same as Fireworkz) */

};

enum STYLE_GUIDE_KEY_VALUES
{
    SG_KEY_HELP             = KMAP_BASE_FUNC   +  1, /* RO SG */
    SG_KEY_LOAD_DOCUMENT    = KMAP_BASE_FUNC   +  2, /* RO SG */
    SG_KEY_SAVE_DOCUMENT    = KMAP_BASE_FUNC   +  3, /* RO SG */
    SG_KEY_FIND             = KMAP_BASE_FUNC   +  4, /* RO SG */
    SG_KEY_GOTO             = KMAP_BASE_FUNC   +  5, /* RO SG */
    SG_KEY_NEXTMATCH        = KMAP_BASE_FUNC   +  6, /* PipeDream (same as Classic) */
    SG_KEY_F7               = KMAP_BASE_FUNC   +  7,
    SG_KEY_UNDO             = KMAP_BASE_FUNC   +  8, /* RO SG */ /* not in PipeDream */
    SG_KEY_REDO             = KMAP_BASE_FUNC   +  9, /* RO SG */ /* not in PipeDream */
    SG_KEY_F10              = KMAP_BASE_FUNC   + 10,
    SG_KEY_F11              = KMAP_BASE_FUNC   + 11,

    SG_KEY_INSERT_DOCUMENT  = KMAP_BASE_SFUNC  +  2, /* RO SG */
    SG_KEY_SORT             = KMAP_BASE_SFUNC  +  6, /* PipeDream (same as Classic) */
    SG_KEY_INSERT_COLUMN    = KMAP_BASE_SFUNC  +  7, /* PipeDream (same as Classic) */
    SG_KEY_DELETE_COLUMN    = KMAP_BASE_SFUNC  +  8, /* PipeDream (same as Classic) */

    SG_KEY_CLOSE_WINDOW     = KMAP_BASE_CFUNC  +  2, /* RO SG */
    SG_KEY_REPLACE          = KMAP_BASE_CFUNC  +  4, /* RO SG */
    SG_KEY_INSERT_ROW       = KMAP_BASE_CFUNC  +  7, /* PipeDream (same as Fireworkz) */
    SG_KEY_DELETE_ROW       = KMAP_BASE_CFUNC  +  8, /* PipeDream (same as Fireworkz) */
};

#define RISCOS_EKEY_DELETE 0x7F /* transforms to ... */

#define KMAP_FUNC_DELETE        (KMAP_BASE_FUNC2   + 0x00)
#define KMAP_FUNC_SDELETE       (KMAP_BASE_SFUNC2  + 0x00)
#define KMAP_FUNC_CDELETE       (KMAP_BASE_CFUNC2  + 0x00)
#define KMAP_FUNC_CSDELETE      (KMAP_BASE_CSFUNC2 + 0x00)

#define RISCOS_EKEY_HOME   30 /* transforms to ... */

#define KMAP_FUNC_HOME          (KMAP_BASE_FUNC2   + 0x01)
#define KMAP_FUNC_SHOME         (KMAP_BASE_SFUNC2  + 0x01)
#define KMAP_FUNC_CHOME         (KMAP_BASE_CFUNC2  + 0x01)
#define KMAP_FUNC_CSHOME        (KMAP_BASE_CSFUNC2 + 0x01)

#define RISCOS_EKEY_BACKSPACE 0x08 /* transforms to ... */

#define KMAP_FUNC_BACKSPACE     (KMAP_BASE_FUNC2   + 0x02)
#define KMAP_FUNC_SBACKSPACE    (KMAP_BASE_SFUNC2  + 0x02)
#define KMAP_FUNC_CBACKSPACE    (KMAP_BASE_CFUNC2  + 0x02)
#define KMAP_FUNC_CSBACKSPACE   (KMAP_BASE_CSFUNC2 + 0x02)

#define KMAP_FUNC_TAB           (KMAP_BASE_FUNC2   + 0x0A)
#define KMAP_FUNC_STAB          (KMAP_BASE_SFUNC2  + 0x0A)
#define KMAP_FUNC_CTAB          (KMAP_BASE_CFUNC2  + 0x0A)
#define KMAP_FUNC_CSTAB         (KMAP_BASE_CSFUNC2 + 0x0A)

#define KMAP_FUNC_END           (KMAP_BASE_FUNC2   + 0x0B)
#define KMAP_FUNC_SEND          (KMAP_BASE_SFUNC2  + 0x0B)
#define KMAP_FUNC_CEND          (KMAP_BASE_CFUNC2  + 0x0B)
#define KMAP_FUNC_CSEND         (KMAP_BASE_CSFUNC2 + 0x0B)

#define KMAP_FUNC_ARROW_LEFT    (KMAP_BASE_FUNC2   + 0x0C)
#define KMAP_FUNC_SARROW_LEFT   (KMAP_BASE_SFUNC2  + 0x0C)
#define KMAP_FUNC_CARROW_LEFT   (KMAP_BASE_CFUNC2  + 0x0C)
#define KMAP_FUNC_CSARROW_LEFT  (KMAP_BASE_CSFUNC2 + 0x0C)

#define KMAP_FUNC_ARROW_RIGHT   (KMAP_BASE_FUNC2   + 0x0D)
#define KMAP_FUNC_SARROW_RIGHT  (KMAP_BASE_SFUNC2  + 0x0D)
#define KMAP_FUNC_CARROW_RIGHT  (KMAP_BASE_CFUNC2  + 0x0D)
#define KMAP_FUNC_CSARROW_RIGHT (KMAP_BASE_CSFUNC2 + 0x0D)

#define KMAP_FUNC_ARROW_DOWN    (KMAP_BASE_FUNC2   + 0x0E)
#define KMAP_FUNC_SARROW_DOWN   (KMAP_BASE_SFUNC2  + 0x0E)
#define KMAP_FUNC_CARROW_DOWN   (KMAP_BASE_CFUNC2  + 0x0E)
#define KMAP_FUNC_CSARROW_DOWN  (KMAP_BASE_CSFUNC2 + 0x0E)

#define KMAP_FUNC_ARROW_UP      (KMAP_BASE_FUNC2   + 0x0F)
#define KMAP_FUNC_SARROW_UP     (KMAP_BASE_SFUNC2  + 0x0F)
#define KMAP_FUNC_CARROW_UP     (KMAP_BASE_CFUNC2  + 0x0F)
#define KMAP_FUNC_CSARROW_UP    (KMAP_BASE_CSFUNC2 + 0x0F)

typedef S32 KMAP_CODE; typedef KMAP_CODE * P_KMAP_CODE; typedef const KMAP_CODE * PC_KMAP_CODE;

/******************************************************************************
* movement manifests
******************************************************************************/

#define SCREENFUL   64
#define END         128

#define CURSOR_UP       1
#define CURSOR_DOWN     2
#define CURSOR_PREV_COL 3
#define CURSOR_NEXT_COL 4

#define CURSOR_SUP      (SCREENFUL + CURSOR_UP)
#define CURSOR_SDOWN    (SCREENFUL + CURSOR_DOWN)
#define CURSOR_FIRSTCOL (END + CURSOR_FIRST_COL)
#define CURSOR_LASTCOL  (END + CURSOR_NEXT_COL)

#define ABSOLUTE (0x80)

/******************************************************************************
* screen update flags
******************************************************************************/

/******************************************************************************
* pdmain.c
******************************************************************************/

/*
exported functions
*/

extern void
exec_file(
    const char *filename);

extern int
main(
    int argc,
    char *argv[]);

/*ncr*/
_Ret_z_
extern PTSTR
make_var_name(
    _Out_writes_z_(elemof_buffer) PTSTR var_name /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR suffix);

/******************************************************************************
* slot.c
******************************************************************************/

extern S32
atend(
    COL col,
    ROW row);

extern ROW
atrow(
    COL tcol);

extern void
blow_away(
    COL col,
    ROW row,
    ROW n_rows);

extern coord
colwidth(
    COL col);

extern coord
colwrapwidth(
    COL tcol);

extern BOOL
colislinked(
    COL tcol);

extern BOOL
linked_column_linkright(
    COL tcol);

extern void
copycont(
    P_CELL,
    P_CELL,
    S32);

extern BOOL
createcol(
    COL tcol);

extern S32
createhole(
    COL col,
    ROW row);

extern P_CELL
createslot(
    COL tcol,
    ROW trow,
    S32 size,
    uchar type);

extern void
delcol(
    COL col,
    COL size);

extern void
delcolandentry(
    COL tcol,
    COL size);

extern void
delcolentry(
    COL tcol,
    COL size);


extern void
delcolstart(
    P_COLENTRY tcolstart,
    COL size);

extern void
deregcoltab(void);

extern void
deregtempcoltab(
    P_COLENTRY tcolstart,
    COL size);

extern void
dstwrp(
    COL tcol,
    coord width);

extern void
garbagecollect(void);

extern BOOL
inscolentry(
    COL tcol);

extern BOOL
insertslotat(
    COL tcol,
    ROW trow);

extern void
killcoltab(void);

extern void
killslot(
    COL tcol,
    ROW trow);

extern P_CELL
next_cell_in_block(
    BOOL direction);

extern void
pack_column(
    COL col);

extern void
readpcolvars(
    _InVal_     COL col,
    _OutRef_    P_P_S32 widp,
    _OutRef_    P_P_S32 wwidp);

extern void
readpcolvars_(
    _InVal_     COL col,
    _OutRef_    P_P_S32 widp,
    _OutRef_    P_P_S32 wwidp,
    _OutRef_    P_P_S32 colflagsp);

extern void
regcoltab(void);

extern void
regtempcoltab(
    P_COLENTRY tcolstart,
    COL size);

extern void
reset_numrow(void);

extern BOOL
restcoltab(void);

extern void
savecoltab(void);

extern void
slot_changed(
    COL col,
    ROW row);

extern void
slot_free_resources(
    P_CELL tcell);

extern void
slot_to_be_replaced(
    COL col,
    ROW row,
    P_CELL tcell);

extern S32
slotcontentssize(
    P_CELL tcell);

extern S32
slotsize(
    P_CELL tcell);

extern BOOL
swap_rows(
    ROW trow1,
    ROW trow2,
    COL firstcol,
    COL lastcol,
    BOOL updaterefs);

extern BOOL
swap_cells(
    COL tcol1,
    ROW trow1,
    COL tcol2,
    ROW trow2);

extern P_CELL
travel(
    COL tcol,
    ROW row);

extern P_CELL
travel_externally(
    _InVal_     DOCNO docno,
    COL col,
    ROW row);

extern P_CELL
travel_here(void);

extern P_CELL
traverse_block_next_slot(
    TRAVERSE_BLOCKP blk /*inout*/);

extern P_CELL
traverse_block_travel(
    TRAVERSE_BLOCKP blk /*const*/);

extern void
tree_insert_fail(
    _InRef_     PC_EV_SLR slrp);

extern void
updref(
    COL mrksco,
    ROW mrksro,
    COL mrkeco,
    ROW mrkero,
    COL coldiff,
    ROW rowdiff,
    S32 action,
    _InVal_     DOCNO docno_to);

extern void
uref_block(
    S32 action);

/*
macros
*/

#define indexcol(col) (                 \
    (!colstart  ||  ((col) >= numcol))  \
        ? NULL                          \
        : (colstart + (col))            )

#define x_indexcollb(p_docu, col) (                             \
    (!(p_docu)->Xcolstart  ||  ((col) >= (p_docu)->Xnumcol))    \
        ? NULL                                                  \
        : &(((p_docu)->Xcolstart + (col))->lb)                  )

#define indexcollb(col) \
    x_indexcollb(current_p_docu, col)

#define slot_contents(it) \
    list_itemcontents(struct CELL, it)

/*
allocator parameters
*/

#define MAXPOOLSIZE  5000
/* need to be able to enter an expression that fills the expression edit buffer */
#define MAXITEMSIZE  MAX_SLOTSIZE
#define MAXFREESPACE 5000

/******************************************************************************
* mcdiff.c
******************************************************************************/

/*
exported functions
*/

extern void
ack_esc(void);

extern void
bleep(void);

extern void
clearkeyboardbuffer(void);

extern void
curon(void);

extern BOOL /*ctrl_pressed*/
host_keyboard_status(
    _OutRef_    P_BOOL p_shift_pressed);

enum CS_Mouse_State { /* for Click and Drag */
    CS_Mouse_SELECT = 0,
    CS_Mouse_SELECT_with_Ctrl, /* +1 */
    CS_Mouse_SELECT_with_Shift, /* +2 */
    CS_Mouse_SELECT_with_CtrlShift,

    CS_Mouse_ADJUST,
    CS_Mouse_ADJUST_with_Ctrl,
    CS_Mouse_ADJUST_with_Shift,
    CS_Mouse_ADJUST_with_CtrlShift,
};

extern enum CS_Mouse_State
cs_mutate_mouse_click(BOOL f_select_clicked);

extern void
down_scroll(
    coord first_line);

extern void
eraeol(void);

extern BOOL
escape_disable(void);

extern S32
escape_disable_nowinge(void);

extern void
init_mc(void);

extern void
invert(void);

extern BOOL
keyinbuffer(void);

extern void
mycls(
    coord leftx,
    coord boty,
    coord rightx,
    coord topy);

extern void
reset_mc(void);

extern coord
scrnheight(void);

extern coord
scrnwidth(void);

extern void
setcolours(
    _In_        COLOURS_OPTION_INDEX fg_colours_option_index,
    _In_        COLOURS_OPTION_INDEX bg_colours_option_index);

extern coord
stringout(
    const char *str);

extern S32
translate_received_key(
    S32 key);

extern void
up_scroll(
    coord first_line);

extern void
wrchrep(
    uchar ch,
    coord i);

extern void
clearmousebuffer(void);

extern void
escape_enable(void);

extern S32
rdch_riscos(void);

extern void
set_bg_colour_from_option(
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index);

extern void
set_fg_colour_from_option(
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index);

extern void
wrch_h(
    char ch);

/*
exported variables
*/

/*
macro definitions
*/

#define invoff()                if(currently_inverted) invert()
#define rdch(x, y)              rdch_riscos()
#define wrch(x)                 bbc_vdu((int) x)

/******************************************************************************
* savload.c
******************************************************************************/

typedef struct LOAD_FILE_OPTIONS
{
    /* these can be derived from d_load[] options */
    const char * document_name;
    const char * row_range; /* NULL -> none */

    char filetype_option; /* PD4_CHAR etc. */
    BOOLEAN temp_file;
    BOOLEAN inserting;
    BOOLEAN _spare;

    const char * insert_at_slot; /* NULL -> none */
}
LOAD_FILE_OPTIONS, * P_LOAD_FILE_OPTIONS;

extern void
load_file_options_init(
    _OutRef_    P_LOAD_FILE_OPTIONS p_load_file_options,
    _In_z_      const char * document_name,
    _InVal_     S32 filetype_option /* PD4_CHAR etc. */);

typedef struct SAVE_FILE_OPTIONS
{
    /* these can be derived from d_save[] options */
    const char * row_condition; /* NULL -> none */

    char filetype_option; /* PD4_CHAR etc. */
    char line_sep_option;
    BOOLEAN saving_block;
    BOOLEAN saving_choices_file;

    BOOLEAN temp_file;
    BOOLEAN _spare[3];
}
SAVE_FILE_OPTIONS, * P_SAVE_FILE_OPTIONS;

extern void
save_file_options_init(
    _OutRef_    P_SAVE_FILE_OPTIONS p_save_file_options,
    _InVal_     S32 filetype_option /* PD4_CHAR etc. */,
    _InVal_     S32 line_sep_option /* LINE_SEP_LF etc. */);

/*
exported functions
*/

extern void
common_save_version_string(
    _Out_writes_(elemof_array) P_U8 array,
    _InVal_     U32 elemof_array);

/*ncr*/
extern STATUS
enumerate_dir_to_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _In_opt_z_  PC_U8Z subdir /*maybe NULL*/,
    _InVal_     FILETYPE_RISC_OS filetype);

/*ncr*/
extern STATUS
enumerate_dir_to_list_using_path(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _In_opt_z_  PCTSTR doc_subdir /*maybe NULL*/,
    _In_z_      PCTSTR path,
    _In_opt_z_  PCTSTR path_subdir /*maybe NULL*/,
    _InVal_     FILETYPE_RISC_OS filetype);

extern S32 /* filetype_option, 0 or error (reported) */
find_filetype_option(
    _In_z_      PCTSTR name,
                FILETYPE_RISC_OS filetype);

extern BOOL
loadfile(
    _In_z_      PCTSTR filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options);

extern BOOL
loadfile_recurse(
    _In_z_      PCTSTR filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options);

extern U32
loadfile_recurse_load_supporting_documents(
    _In_z_      PCTSTR filename);

extern BOOL
plain_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    char filetype,
    uchar *buffer);

extern BOOL
savefile(
    _In_z_      PCTSTR filename,
    _InRef_     P_SAVE_FILE_OPTIONS p_save_file_options);

_Check_return_
extern BOOL
save_or_discard_existing(void);

extern void
save_opt_to_file(
    FILE_HANDLE output,
    DIALOG *start,
    S32 n);

extern void
set_width_and_wrap(
    COL tcol,
    coord width,
    coord wrapwidth);

extern FILETYPE_RISC_OS
currentfiletype(
    _InVal_     char filetype_option);

extern FILETYPE_RISC_OS
rft_from_filetype_option(
    _InVal_     char filetype_option);

extern void
restore_current_window_pos(void);

extern void
save_current_window_pos(void);

enum D_SAVE_OFFSETS
{
    SAV_NAME,
    SAV_ROWCOND,
    SAV_BLOCK,
    SAV_LINESEP,        /* position of CRLF option in dialog box */
    SAV_FORMAT          /* position of save format option in dialog box */
};

enum LINE_SEP_LIST_OFFSETS /* fits in U8 */
{
    LINE_SEP_LF,
    LINE_SEP_CR,
    LINE_SEP_CRLF,
    LINE_SEP_LFCR
};

/* File types */

#define AUTO_CHAR           'A'
#define AUTO_CHAR_STR       "A"
#define CSV_CHAR            'C'
#define CSV_CHAR_STR        "C"
#define FWP_CHAR            'F'
#define FWP_CHAR_STR        "F"
#define LOTUS123_CHAR       'L'
#define PD4_CHAR            'P'
#define PD_CHAR_STR         "P"
#define PD4_CHART_CHAR      'R'
#define VIEWSHEET_CHAR      'S'
#define VIEWSHEET_CHAR_STR  "S"
#define TAB_CHAR            'T'
#define TAB_CHAR_STR        "T"
#define PARAGRAPH_CHAR      'U'
#define PARAGRAPH_CHAR_STR  "U"
#define VIEW_CHAR           'V'
#define VIEW_CHAR_STR       "V"

/******************************************************************************
* pdriver.c
******************************************************************************/

/*
exported functions
*/

/* changed by RJM on 22.9.91
    force_load parameter added so that if driver is same name as existing it doesn't bother loading it
    so now it is windvar, PRINT_fn can happily call it
*/
extern void
load_driver( /* load a printer driver: called on startup and by PDfunc */
    char **namep,
    BOOL force_load);

extern void
kill_driver(void);

/******************************************************************************
* slector.c
******************************************************************************/

/*
exported functions
*/

extern STATUS
add_path_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR filename /*inout*/,
    _InVal_     U32 elemof_buffer,
    const char *src,
    _In_z_      PC_USTR dir);

extern STATUS
add_path_or_relative_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR filename /*inout*/,
    _InVal_     U32 elemof_buffer,
    const char *src,
    _InVal_     BOOL allow_cwd,
    _In_z_      PC_USTR dir);

extern char *
add_prefix_to_name_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR buffer /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR name,
    _InVal_     BOOL allow_cwd,
    _In_z_      PCTSTR dir);

extern char *
add_choices_write_prefix_to_name_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR buffer /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR name,
    _In_opt_z_  PCTSTR dir);

extern BOOL
checkoverwrite(
    const char *filename);

extern FILE_HANDLE
pd_file_open(
    const char * name,
    FILE_OPEN_MODE mode);

extern int
pd_file_close(
    FILE_HANDLE * file);

extern int
pd_file_read(
    void *ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE stream);

extern int
pd_file_getc(
    FILE_HANDLE input);

extern int
pd_file_putc(
    _InVal_     U8 ch,
    FILE_HANDLE output);

extern BOOL
away_byte(
    _InVal_     U8 ch,
    FILE_HANDLE output);

extern BOOL
away_eol(
    FILE_HANDLE output);

extern BOOL
away_string(
    _In_z_      PC_U8Z str,
    FILE_HANDLE output);

extern BOOL
away_string_simple(
    _In_z_      PC_U8Z str,
    FILE_HANDLE output);

/******************************************************************************
* browse.c
******************************************************************************/

extern S32
browse(
    _InVal_     DICT_NUMBER dict,
    char *wild_str);

_Check_return_
extern STATUS
close_dict(
    _InVal_     DICT_NUMBER dict);

extern void
close_user_dictionaries(
    BOOL force);

typedef struct A_LETTER
{
    uchar letter;
    BOOLEAN used;
}
A_LETTER;

/* Number of items in a browse box */
#define BROWSE_DEPTH 12

extern void
anagram_null(void);

extern void
browse_null(void);

extern void
dumpdict_null(void);

extern void
mergedict_null(void);

/******************************************************************************
* spellchk.c
******************************************************************************/

extern void
check_word(void);   /* check current word on line */

extern BOOL
check_word_wanted(void);

extern void
del_spellings(void);

extern P_U8
get_word_from_line(
    _InVal_     DICT_NUMBER dict,
    /*out*/ P_U8 array,
    S32 stt_offset,
    P_S32 found_offsetp);

/******************************************************************************
* userdict.c
******************************************************************************/

_Check_return_
extern STATUS
dict_number(
    const char *name,
    BOOL create);

/*ncr*/
extern BOOL
err_open_master_dict(void);

extern BOOL
insert_most_recent_userdict(
    char **field);

_Check_return_
extern STATUS
open_appropriate_dict(
    const DIALOG * const dptr);

_Check_return_
extern STATUS
open_master_dict(void);

/******************************************************************************
* bufedit.c
******************************************************************************/

extern BOOL
all_widths_zero(
    COL tcol1,
    COL tcol2);

extern void
block_highlight_core(
    BOOL delete,
    uchar h_ch);

extern void
chkwrp(void);           /* check for and do wrap */

extern void
chrina(
    uchar ch,
    BOOL allow_check);

extern void
delete_bit_of_line(
    S32 stt_offset,
    S32 length,
    BOOL save);

extern BOOL
fnxtwr(void);

extern void
inschr(
    uchar ch);

extern BOOL
insert_string(
    const char *str,
    BOOL allow_check);

extern BOOL
insert_string_check_number(
    const char *str,
    BOOL allow_check);

extern BOOL
adjust_all_linked_columns(void);

extern BOOL
adjust_this_linked_column(
    COL tcol);

extern S32
count_this_linked_column(
    COL tcol);

extern BOOL
enumerate_linked_columns(
    P_COL scanstart,
    P_COL found_first,
    P_COL found_last);

extern void
mark_as_linked_columns(
    COL first,
    COL last);

extern void
remove_overlapped_linked_columns(
    COL tcol1,
    COL tcol2);

/******************************************************************************
* option page construct offsets
******************************************************************************/

enum D_OPTIONS_OFFSETS
{
    O_DE,       /* title string */
    O_TN,       /* text/numbers */
    O_IW,       /* insert on wrap */
    O_BO,       /* borders */
    O_JU,       /* justify */
    O_WR,       /* wrap */
    O_DP,       /* decimal places */
    O_MB,       /* minus/brackets */
    O_TH,       /* thousands separator */
    O_IR,       /* insert on return */
    O_DF,       /* date format */
    O_LP,       /* leading characters */
    O_TP,       /* trailing characters */
    O_GR,       /* grid on/off */
    O_KE        /* kerning on/off */
};

enum D_OPTIONS_THOUSANDS_OFFSETS
{
    TH_BLANK,
    TH_COMMA,
    TH_DOT,
    TH_SPACE
};

enum D_POPTIONS_OFFSETS
{
    O_PL,   /* page length */
    O_LS,   /* line spacing */
    O_PS,   /* start page string */
    O_TM,   /* top margin */
    O_HM,   /* header margin */
    O_FM,   /* footer margin */
    O_BM,   /* bottom margin */
    O_LM,   /* left margin */
    O_HE,   /* header string */
    O_FO    /* footer string */
};

enum D_PROGVARS_OFFSETS
{
    OR_AM,      /* auto/manual recalc (SHOULD REALLY BE OR_AN) */
    OR_IO,      /* insert/overtype */
    OR_AC,      /* auto/manual chart recalc */
    OR_CF       /* new/old chart format */
};

extern void
getoption( /* read option from input stream */
    uchar *);

_Check_return_
extern BOOL
str_isblank( /* is string only spaces? */
    _In_opt_z_  PC_USTR str);

/******************************************************************************
* dialog.c : dialog boxes
******************************************************************************/

#define F_TEXT      ((uchar) 0)
#define F_NUMBER    ((uchar) 1)
#define F_SPECIAL   ((uchar) 2)
#define F_ERRORTYPE ((uchar) 3)
#define F_COMPOSITE ((uchar) 4)
#define F_COLOUR    ((uchar) 5)
#define F_CHAR      ((uchar) 6)
#define F_ARRAY     ((uchar) 7)
#define F_LIST      ((uchar) 8)

/* These constants MUST be kept in line with the dialog header
 * table as they are indexes into it !!!
 * RJM thinks this conditional compilation is going too far
 * can get rid of it if we force dialog_header to have all entries
*/

enum DIALOG_HEADER_OFFSETS
{
    D_ERROR,
    D_LOAD,
    D_OVERWRITE,
    D_SAVE_FILE_AS,
    D_SAVE_TEMPLATE,
    D_PRINT,
    D_MSPACE,
    D_DRIVER,
    D_POPTIONS,
    D_SORT,
    D_REPLICATE,
    D_SEARCH,
    D_OPTIONS,
    D_PARAMETER,
    D_WIDTH,
    D_MARGIN,
    D_NAME,
    D_EXECFILE,
    D_GOTO,
    D_DECIMAL,
    D_INSHIGH,
    D_REMHIGH,
    D_DEFKEY,
    D_DEF_FKEY,
    D_INSERT_PAGE_BREAK,
    D_SAVE_FILE_AS_POPUP,
    D_COLOURS,
    D_EXTENDED_COLOURS,
    D_ABOUT,
    D_was_STATUS,
    D_COUNT,
    D_PAUSE,
    D_SAVE_DELETED,
    D_AUTO,
    D_CHECKON,
    D_CHECK,
    D_CHECK_MESS,
    D_USER_CREATE,
    D_USER_OPEN,
    D_USER_CLOSE,
    D_USER_INSERT,
    D_USER_DELETE,
    D_USER_BROWSE,
    D_USER_DUMP,
    D_USER_MERGE,
    D_USER_ANAG,
    D_USER_SUBGRAM,
    D_USER_LOCK,
    D_USER_UNLOCK,
    D_USER_PACK,
    D_FIXES,
    D_DELETED,
    D_PROGVARS,
    D_PROTECT,
    D_VERSION,
    D_NAMES_DBOX,
    D_MACRO_FILE,
    D_FONTS,
    D_MENU,
    D_DEF_CMD,
    D_USER_OPTIONS,
    D_USER_FLUSH,
    D_DEFINE_NAME,
    D_EDIT_NAME,
    D_OPEN_BOX,
    D_LINKED_COLUMNS,
    D_LOAD_TEMPLATE,
    D_EDIT_DRIVER,
    D_FORMULA_ERROR,
    D_CHART_OPTIONS,
    D_INSERT_PAGE_NUMBER,
    D_INSERT_DATE,
    D_INSERT_TIME,
    D_SAVE_FILE_SIMPLE,
    D_SAVE_FILE_SIMPLE_POPUP,
    D_THE_LAST_ONE
};

enum D_FONTS_OFFSETS
{
    D_FONTS_G,
    D_FONTS_X,
    D_FONTS_Y,
    D_FONTS_S
};

/* offsets in d_driver */

#define D_DRIVER_TYPE 0
#define D_DRIVER_NAME 1
#define D_DRIVER_ECON 2
#define D_DRIVER_BAUD 3
#define D_DRIVER_DATA 4
#define D_DRIVER_PRTY 5
#define D_DRIVER_STOP 6

extern DHEADER dialog_head[];

extern DIALOG d_load[];
extern DIALOG d_overwrite[];
extern DIALOG d_save[];
extern DIALOG d_save_template[];
extern DIALOG d_print[];
extern DIALOG d_mspace[];
extern DIALOG d_driver[];
extern DIALOG d_poptions[];
extern DIALOG d_sort[];
extern DIALOG d_replicate[];
extern DIALOG d_search[];
extern DIALOG d_options[];
extern DIALOG d_parameter[];
extern DIALOG d_width[];
extern DIALOG d_name[];
extern DIALOG d_execfile[];
extern DIALOG d_goto[];
extern DIALOG d_decimal[];
extern DIALOG d_insremhigh[];
extern DIALOG d_defkey[];
extern DIALOG d_def_fkey[];
extern DIALOG d_insert_page_break[];
extern DIALOG d_create[];
extern DIALOG d_colours[];
extern DIALOG d_inschar[];
extern DIALOG d_about[];
extern DIALOG d_count[];
extern DIALOG d_auto[];
extern DIALOG d_checkon[];
extern DIALOG d_check[];
extern DIALOG d_check_mess[];
extern DIALOG d_user_create[];
extern DIALOG d_user_open[];
extern DIALOG d_user_close[];
extern DIALOG d_user_flush[];
extern DIALOG d_user_delete[];
extern DIALOG d_user_insert[];
extern DIALOG d_user_browse[];
extern DIALOG d_user_dump[];
extern DIALOG d_user_merge[];
extern DIALOG d_user_anag[];
extern DIALOG d_user_lock[];
extern DIALOG d_user_unlock[];
extern DIALOG d_user_options[];
extern DIALOG d_user_pack[];
extern DIALOG d_pause[];
extern DIALOG d_save_deleted[];
extern DIALOG d_fixes[];

#define D_PASTE_SIZE    10
extern DIALOG d_deleted[];

extern DIALOG d_protect[];
extern DIALOG d_version[];
extern DIALOG d_names_dbox[];
extern DIALOG d_macro_file[];
extern DIALOG d_fonts[];
extern DIALOG d_menu[];
extern DIALOG d_def_cmd[];

extern DIALOG d_progvars[];

extern DIALOG d_define_name[];
extern DIALOG d_edit_name[];
extern DIALOG d_open_box[];
extern DIALOG d_linked_columns[];
extern DIALOG d_load_template[];
extern DIALOG d_formula_error[];
extern DIALOG d_chart_options[];
extern DIALOG d_edit_driver[];
extern DIALOG d_insert_page_number[];
extern DIALOG d_insert_date[];
extern DIALOG d_insert_time[];

/*
exported table of internal key values corresponding to the define function key dialog box list
*/

extern const KMAP_CODE func_key_list[];

extern BOOL
allowed_in_dialog(
    S32 c);

extern BOOL
dialog_box(
    _InVal_     U32 boxnumber);

extern BOOL
dialog_box_can_persist(void);

extern BOOL
dialog_box_can_retry(void);

extern BOOL
dialog_box_can_start(void);

extern void
dialog_box_end(void);

extern BOOL
dialog_box_start(void);

extern void
dialog_finalise(void);

extern void
dialog_hardwired_defaults(void);

extern BOOL
dialog_initialise(void);

extern void
dialog_initialise_once(void);

extern BOOL
init_dialog_box(
    _InVal_     U32 boxnumber);

extern void
save_options_to_list(void);

extern void
update_all_dialog_from_windvars(void);

extern void
update_all_windvars_from_dialog(void);

extern void
update_dialog_from_fontinfo(void);

extern void
update_dialog_from_windvars(
    _InVal_     U32 boxnumber);

extern void
update_fontinfo_from_dialog(void);

extern void
update_windvars_from_dialog(
    _InVal_     U32 boxnumber);

/******************************************************************************
* colh.c
******************************************************************************/

extern void
colh_draw_column_headings(void);

extern void
colh_draw_contents_of_number_cell(void);

extern void
colh_draw_cell_count(
    _In_opt_z_      char *text);

extern void
colh_draw_cell_count_in_document(
    _In_opt_z_      char *text);

extern void
colh_position_icons(void);

extern void
colh_colour_icons(void);

/******************************************************************************
* constr.c
******************************************************************************/

#if 0 /* SKS 24.10.91 - make corresponding to Text template */
#define DEFAULTWIDTH        8
#else
#define DEFAULTWIDTH        12
#endif
#define DEFAULTWRAPWIDTH    0
#define DEFAULTCOLFLAGS     0   /* i.e. column not linked */

#define LINKED_COLUMN_LINKRIGHT 1
#define LINKED_COLUMN_LINKSTOP  2

extern uchar *
find_next_csr(
    uchar * csr);

extern BOOL
check_not_blank_sheet(void);

extern void
constr_finalise(void);

extern BOOL
constr_initialise(void);

extern void
constr_initialise_once(void);

extern void
reset_filpnm(void);

extern BOOL
dftcol(void);       /* setup default columns */

extern void
delfil(void);       /* delete contents of file (cols and rows) */

/* we require mystr_set to error if failed */

_Check_return_
extern BOOL
mystr_set(
    _InoutRef_  P_P_USTR aa,
    _In_opt_z_  PC_USTR b);

_Check_return_
extern BOOL
mystr_set_n(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b,
    _InVal_     U32 n);

/******************************************************************************
*
*       positions of things on screen. note that Y DECREASES up screen
*
******************************************************************************/

#if defined(CLIP_TO_WINDOW_X)
#define RHS_X           pagwid_plus1
#else
#define RHS_X           (pagwid_plus1 + 1)  /* allow semi-printed chars off right */
#endif

/* coordinates of current cell */

/* Careful, _t_ are text coordinates    */
/*          _g_ are graphic coordinates */
#define CELLCOORDS_t_X0 (-1)
#define CELLCOORDS_t_X1 (COLUMNHEAD_t_X0)
#define CELLCOORDS_g_Y0 (-128)
#define CELLCOORDS_g_Y1 (-64)

/* column headings (if present) */

/* Careful, _t_ are text coordinates    */
/*          _g_ are graphic coordinates */
#define COLUMNHEAD_t_X0 (0)
#define COLUMNHEAD_t_X1 (RHS_X)
#define COLUMNHEAD_g_Y0 (-128)
#define COLUMNHEAD_g_Y1 (-64)

/* row numbers displayed as five digits + space wide */
#define BORDERWIDTH     ((coord) 6)

/******************************************************************************
* commlin.c
******************************************************************************/

/*
exported functions
*/

extern void
chart_format_state_may_have_changed(void);

extern void
chart_recalc_state_may_have_changed(void);

extern void
check_state_changed(void);

extern void
cmd_seq_cancel(void);

extern BOOL
cmd_seq_is_empty(void);

extern void
display_heading(
    S32 idx);

extern BOOL
do_command(
    MENU * mptr,
    S32 funcnum,
    BOOL spool);

extern void
do_execfile(
    _In_z_      PCTSTR filename);

extern void
draw_bar(
    coord x_pos,
    coord y_pos,
    coord length);

extern S32
fiddle_with_menus(
    S32 c,
    BOOL alt_detected);

extern MENU *
find_loadcommand(void);

extern void
get_menu_change(
    const char *from);

extern S32
getsbd(void);

extern void
headline_initialise_once(void);

extern S32
inpchr(
    BOOL curs);

extern void
insert_overtype_state_may_have_changed(void);

extern void
internal_process_command(
    S32 funcnum);

extern void
menu_state_changed(void);

extern void
output_char_to_macro_file(
    uchar ch);

extern void
out_comm_start_to_macro_file(
    S32 funcnum);

extern void
out_comm_parms_to_macro_file(
    DIALOG * dptr,
    S32 size,
    BOOL ok);

extern void
out_comm_end_to_macro_file(
    S32 funcnum);

extern void
recalc_state_may_have_changed(void);

extern BOOL
schkvl(
    S32 c);

#define ALLOW_MENUS     1
#define NO_MENUS        0

#define NO_VAL ((coord) -1)     /* assumed negative in calls to getsbd */

#define NO_OPTS         ((uchar) 0)
#define BAD_OPTS        ((uchar) 0x80)

/* menus */

extern void
output_formula_to_macro_file(
    char * str);

extern void
save_menu_changes(
    FILE_HANDLE output);

/******************************************************************************
* execs.c
******************************************************************************/

enum FUNCTION_NUMBERS
{
    N_Quit = 1,
    N_Escape,
    N_NewWindow,
    N_NextWindow,
    N_CloseWindow,
    N_LoadTemplate,

    /* movements */
    N_CursorUp,
    N_CursorDown,
    N_CursorLeft,
    N_CursorRight,
    N_PrevWord,
    N_NextWord,
    N_StartOfSlot,
    N_EndOfSlot,
    N_ScrollUp,
    N_ScrollDown,
    N_ScrollLeft,
    N_ScrollRight,
    N_CentreWindow,
    N_PageUp,
    N_PageDown,
    N_PrevColumn,
    N_NextColumn,
    N_TopOfColumn,
    N_BottomOfColumn,
    N_FirstColumn,
    N_LastColumn,
    N_SavePosition,
    N_RestorePosition,
    N_SwapPosition,
    N_GotoSlot,

    /* editing operations */
    N_Return,
    N_InsertSpace,
    N_SpareXXXXXX,
    N_SwapCase,
    N_DeleteCharacterLeft,
    N_DeleteCharacterRight,
    N_DeleteWord,
    N_DeleteToEndOfSlot,
    N_Paste,
    N_JoinLines,
    N_SplitLine,
    N_InsertRow,
    N_DeleteRow,
    N_InsertRowInColumn,
    N_DeleteRowInColumn,
    N_AddColumn,
    N_InsertColumn,
    N_DeleteColumn,
    N_FormatParagraph,
    N_InsertPageBreak,
    N_EditFormula,
    N_EditFormulaInWindow,
    N_InsertReference,

    /* layout manipulation */
    N_FixColumns,
    N_FixRows,
    N_ColumnWidth,
    N_RightMargin,
    N_MoveMarginLeft,
    N_MoveMarginRight,
    N_AutoWidth,
    N_LeftAlign,
    N_CentreAlign,
    N_RightAlign,
    N_LCRAlign,
    N_FreeAlign,
    N_LeadingCharacters,
    N_TrailingCharacters,
    N_DecimalPlaces,
    N_SignBrackets,
    N_SignMinus,
    N_DefaultFormat,

    N_Search,
    N_NextMatch,
    N_PrevMatch,

    /* block operations */
    N_ClearMarkedBlock,
    N_ClearProtectedBlock,
    N_CopyBlock,
    N_CopyBlockToPasteList,
    N_DeleteBlock,
    N_ExchangeNumbersText,
    N_HighlightBlock,
    N_MarkSlot,
    N_MoveBlock,
    N_RemoveHighlights,
    N_Replicate,
    N_ReplicateDown,
    N_ReplicateRight,
    N_SetProtectedBlock,
    N_Snapshot,
    N_SortBlock,

    N_LoadFile,
    N_NameFile,
    N_SaveFileAs,
    N_SaveTemplateFile,

    N_Print,
    N_PageLayout,
    N_PrinterConfig,
    N_MicrospacePitch,
    N_EditPrinterDriver,
    N_SetMacroParm,
    N_AlternateFont,
    N_Bold,
    N_ExtendedSequence,
    N_Italic,
    N_Subscript,
    N_Superscript,
    N_Underline,
    N_UserDefinedHigh,

    N_Options,
    N_InsertOvertype,
    N_Colours,
    N_Recalculate,
    N_AutoRecalculation,
    N_About,
    N_WordCount,
    N_RecordMacroFile,
    N_DoMacroFile,
    N_PasteListDepth,
    N_MenuSize,
    N_DefineKey,
    N_DefineFunctionKey,
    N_DefineCommand,
    N_Help,
    N_Pause,
    N_was_OSCommand,

    N_AutoSpellCheck,
    N_BrowseDictionary,
    N_DumpDictionary,
    N_MergeFileWithDict,
    N_Anagrams,
    N_Subgrams,
    N_CheckDocument,
    N_InsertWordInDict,
    N_DeleteWordFromDict,
    N_LockDictionary,
    N_UnlockDictionary,
    N_CreateUserDict,
    N_OpenUserDict,
    N_CloseUserDict,
    N_PackUserDict,
    N_DisplayOpenDicts

,   N_PRINTERFONT
,   N_INSERTFONT
,   N_PRINTERLINESPACE
,   N_SaveFileAs_Imm
,   N_MarkSheet
,   N_Debug
,   N_DictionaryOptions
,   N_SaveChoices
,   N_FlushUserDict
,   N_ChartNew
,   N_ChartAdd
,   N_ChartSelect
,   N_ChartEdit
,   N_ChartDelete
,   N_ChangeMenus
,   N_FormatBlock
,   N_Newslotcontents
,   N_FullScreen
,   N_TransposeBlock
,   N_RepeatCommand
,   N_ChartRemove
,   N_PageLeft
,   N_PageRight

,   N_DefineName
,   N_EditName
,   N_PasteName

,   N_LinkColumns

,   N_OldSearch
,   N_OldExchangeNumbersAndText
,   N_ChartOptions
,   N_AutoChartRecalculation

,   N_ToNumber /* added in PD 4.50 */
,   N_ToText
,   N_ClearBlock

,   N_ToConstant /* added in PD 4.56 */
,   N_ReplicateUp
,   N_ReplicateLeft
,   N_MenuStyle
,   N_InsertFile
,   N_InsertColourChange
,   N_InsertDate
,   N_InsertTime
,   N_InsertPageNumber
,   N_ShowLicence
,   N_InteractiveHelp

,   N_ChartFormat /* added in PD 4.57 */

,   N_SaveFileSimple /* added in PD 4.58 */
,   N_SaveFileSimple_Imm
};

extern void
Quit_fn(void);

extern void
Escape_fn(void);

extern void
NewWindow_fn(void);

extern void
NextWindow_fn(void);

extern void
CloseWindow_fn(void);

extern void
LoadTemplate_fn(void);

extern void
SaveChoices_fn(void);

extern void
ChangeMenus_fn(void);

/*
movements
*/

extern void
CursorUp_fn(void);

extern void
CursorDown_fn(void);

extern void
CursorLeft_fn(void);

extern void
CursorRight_fn(void);

extern void
PrevWord_fn(void);

extern void
NextWord_fn(void);

extern void
StartOfSlot_fn(void);

extern void
EndOfSlot_fn(void);

extern void
ScrollUp_fn(void);

extern void
ScrollDown_fn(void);

extern void
ScrollLeft_fn(void);

extern void
ScrollRight_fn(void);

extern void
CentreWindow_fn(void);

extern void
PageUp_fn(void);

extern void
PageDown_fn(void);

extern void
PageLeft_fn(void);

extern void
PageRight_fn(void);

extern void
PrevColumn_fn(void);

extern void
NextColumn_fn(void);

extern void
TopOfColumn_fn(void);

extern void
BottomOfColumn_fn(void);

extern void
FirstColumn_fn(void);

extern void
LastColumn_fn(void);

extern void
SavePosition_fn(void);

extern void
RestorePosition_fn(void);

extern void
SwapPosition_fn(void);

extern void
GotoSlot_fn(void);

extern void
FullScreen_fn(void);

/*
editing operations
*/

extern void
RepeatCommand_fn(void);

extern void
Return_fn(void);

extern void
InsertSpace_fn(void);

extern void
SwapCase_fn(void);

extern void
DeleteCharacterLeft_fn(void);

extern void
DeleteCharacterRight_fn(void);

extern void
DeleteWord_fn(void);

extern void
DeleteToEndOfSlot_fn(void);

extern void
Paste_fn(void);

extern void
JoinLines_fn(void);

extern void
SplitLine_fn(void);

extern void
InsertRow_fn(void);

extern void
DeleteRow_fn(void);

extern void
InsertRowInColumn_fn(void);

extern void
DeleteRowInColumn_fn(void);

extern void
AddColumn_fn(void);

extern void
InsertColumn_fn(void);

extern void
DeleteColumn_fn(void);

extern void
FormatParagraph_fn(void);

extern void
FormatBlock_fn(void);

extern void
InsertPageBreak_fn(void);

extern void
InsertPageNumber_fn(void);

extern void
InsertDate_fn(void);

extern void
InsertTime_fn(void);

extern void
InsertColourChange_fn(void);

extern void
EditFormula_fn(void);

extern void
Newslotcontents_fn(void);

extern void
EditFormulaInWindow_fn(void);

extern void
TransposeBlock_fn(void);

extern void
InsertReference_fn(void);

/*
layout manipulation
*/

extern void
FixColumns_fn(void);

extern void
FixRows_fn(void);

extern void
ColumnWidth_fn(void);

extern void
RightMargin_fn(void);

extern void
MoveMarginLeft_fn(void);

extern void
MoveMarginRight_fn(void);

extern void
AutoWidth_fn(void);

extern void
LinkColumns_fn(void);

extern void
LeftAlign_fn(void);

extern void
CentreAlign_fn(void);

extern void
RightAlign_fn(void);

extern void
LCRAlign_fn(void);

extern void
FreeAlign_fn(void);

extern void
LeadingCharacters_fn(void);

extern void
TrailingCharacters_fn(void);

extern void
DecimalPlaces_fn(void);

extern void
SignBrackets_fn(void);

extern void
SignMinus_fn(void);

extern void
DefaultFormat_fn(void);

extern void
Replace_fn(void);

extern void
Search_fn(void);

extern void
NextMatch_fn(void);

extern void
PrevMatch_fn(void);

/*
block operations
*/

extern void
ClearBlock_fn(void);

extern void
ClearMarkedBlock_fn(void);

extern void
ClearProtectedBlock_fn(void);

extern void
CopyBlock_fn(void);

extern void
CopyBlockToPasteList_fn(void);

extern void
DeleteBlock_fn(void);

extern void
ExchangeNumbersText_fn(void);

extern void
HighlightBlock_fn(void);

extern void
MarkSheet_fn(void);

extern void
MarkSlot_fn(void);

extern void
MoveBlock_fn(void);

extern void
RemoveHighlights_fn(void);

extern void
Replicate_fn(void);

extern void
SetProtectedBlock_fn(void);

extern void
Snapshot_fn(void);

extern void
SortBlock_fn(void);

extern void
ToConstant_fn(void);

extern void
ToNumber_fn(void);

extern void
ToText_fn(void);

extern void
LoadFile_fn(void);

extern void
InsertFile_fn(void);

extern void
NameFile_fn(void);

extern void
SaveFileAs_fn(void);

extern void
SaveFileAs_Imm_fn(void);

extern void
SaveFileSimple_fn(void);

extern void
SaveFileSimple_Imm_fn(void);

extern void
SaveTemplateFile_fn(void);

extern void
Print_fn(void);

extern void
PageLayout_fn(void);

extern void
PrinterConfig_fn(void);

extern void
MicrospacePitch_fn(void);

extern void
EditPrinterDriver_fn(void);

extern void
SetMacroParm_fn(void);

extern void
AlternateFont_fn(void);

extern void
Bold_fn(void);

extern void
ExtendedSequence_fn(void);

extern void
Italic_fn(void);

extern void
Subscript_fn(void);

extern void
Superscript_fn(void);

extern void
Underline_fn(void);

extern void
UserDefinedHigh_fn(void);

extern void
Options_fn(void);

extern void
InsertOvertype_fn(void);

extern void
Colours_fn(void);

extern void
Recalculate_fn(void);

extern void
About_fn(void);

extern void
WordCount_fn(void);

extern void
RecordMacroFile_fn(void);

extern void
DoMacroFile_fn(void);

extern void
PasteListDepth_fn(void);

extern void
MenuSize_fn(void);

extern void
MenuStyle_fn(void);

extern void
DefineKey_fn(void);

extern void
DefineFunctionKey_fn(void);

extern void
DefineCommand_fn(void);

extern void
Pause_fn(void);

/* Spell */

extern void
AutoSpellCheck_fn(void);

extern void
BrowseDictionary_fn(void);

extern void
DumpDictionary_fn(void);

extern void
MergeFileWithDict_fn(void);

extern void
Anagrams_fn(void);

extern void
Subgrams_fn(void);

extern void
CheckDocument_fn(void);

extern void
InsertWordInDict_fn(void);

extern void
DeleteWordFromDict_fn(void);

extern void
LockDictionary_fn(void);

extern void
UnlockDictionary_fn(void);

extern void
DictionaryOptions_fn(void);

extern void
CreateUserDict_fn(void);

extern void
OpenUserDict_fn(void);

extern void
CloseUserDict_fn(void);

extern void
FlushUserDict_fn(void);

extern void
DisplayOpenDicts_fn(void);

extern void
PackUserDict_fn(void);

/* Chart */

extern void
ChartNew_fn(void);

extern void
ChartAdd_fn(void);

extern void
ChartEdit_fn(void);

extern void
ChartDelete_fn(void);

extern void
ChartOptions_fn(void);

extern void
ChartSelect_fn(void);

/* Help */

extern void
Help_fn(void);

extern void
ShowLicence_fn(void);

extern void
InteractiveHelp_fn(void);

extern void
AutoRecalculate_fn(void);

extern void
AutoChartRecalculate_fn(void);

extern void
small_DeleteCharacter_fn(void);     /* without reformatting */

extern void
add_to_protect_list(
    uchar *ptr);

extern void
add_to_linked_columns_list(
    uchar *ptr);

extern void
add_to_names_list(
    uchar *from);

extern BOOL
bad_reference(
    COL tcol,
    ROW trow);

extern void
clear_protect_list(void);

extern void
clear_linked_columns(void);

extern void
ensure_paste_list_clipped(void);

extern void
mark_to_end(
    coord rowonscr);

extern void
prccml(
    uchar *array);

extern BOOL
protected_cell(
    COL tcol,
    ROW trow);

extern BOOL
protected_cell_in_block(
    COL firstcol,
    ROW firstrow,
    COL lastcol,
    ROW lastrow);

extern BOOL
protected_cell_in_range(
    PC_SLR bs,
    PC_SLR be);

extern void
setprotectedstatus(
    P_CELL tcell);

extern void
save_linked_columns(
    FILE_HANDLE output);

extern void
save_protected_bits(
    FILE_HANDLE output);

extern void
save_names_to_file(
    FILE_HANDLE output);

extern BOOL
save_words(
    uchar *ptr);

extern BOOL
test_protected_cell(
    COL tcol,
    ROW trow);

extern S32 latest_word_on_stack;

#define is_protected_cell(tcell) ((tcell)->justify & PROTECTED)

#define words_allowed ((S32) (d_deleted[0].option))

/* things on deleted_words list > BLOCK_OFFSET are blocks, else words */
#define BLOCK_OFFSET ((S32) 65536)

/******************************************************************************
* blocks.c
******************************************************************************/

#define BLOCK_UPDREF_ROW (2<<20)
#define BLOCK_UPDREF_COL (2<<20)

extern void
block_updref(
    _InVal_     DOCNO docno_to,
    _InVal_     DOCNO docno_from,
    COL startcol,
    ROW startrow,
    COL coffset,
    ROW roffset);

extern void
CopyBlock_fn(void);

extern void
DeleteBlock_fn(void);

/*ncr*/
extern S32
do_delete_block(
    BOOL do_save,
    BOOL allow_widening,
    BOOL ignore_protection);

extern void
ensure_paste_list_clipped(void);

_Check_return_
extern BOOL
is_blank_block(
    COL cs,
    ROW rs,
    COL ce,
    ROW re);

extern void
MoveBlock_fn(void);

extern void
MoveBlock_fn_do(S32 add_refs);

extern void
Paste_fn(void);

extern void
Replicate_fn(void);

extern void
ReplicateDown_fn(void);

extern void
ReplicateLeft_fn(void);

extern void
ReplicateRight_fn(void);

extern void
ReplicateUp_fn(void);

_Check_return_
extern BOOL
save_block_and_delete(
    BOOL is_deletion,
    BOOL do_save);

_Check_return_
extern BOOL
set_up_block(
    BOOL check_block_in_doc);

/*ncr*/
extern char *
text_csr_uref(
    uchar * csr,
    _InRef_     PC_UREF_PARM upp);

extern void
TransposeBlock_fn(void);

/******************************************************************************
* expedit.c
******************************************************************************/

extern void
DefineName_fn(void);

extern void
EditName_fn(
    _InVal_     enum EV_RESOURCE_TYPES category,
    S32 itemno);

extern void
PasteName_fn(
    _InVal_     enum EV_RESOURCE_TYPES category,
    S32 itemno);

extern void
expedit_close_file(
    _InVal_     DOCNO docno);

extern BOOL
expedit_owns_window(
    P_DOCU p_docu,
    _HwndRef_   HOST_WND window_handle);

/******************************************************************************
* scdraw.c
******************************************************************************/

/*
exported functions
*/

extern void
bottomline(
    coord x_pos,
    coord y_pos,
    coord x_size);

extern void
display_heading(
    S32 idx);

extern void
draw_one_altered_slot(
    COL col,
    ROW row);

extern void
draw_screen(void);

extern void
draw_caret(void);

extern S32
font_strip_spaces(
    char *out_buf,
    char *str_in,
    P_S32 spaces);

extern S32
font_width(
    char *str);

extern S32
font_truncate(
    char *str,
    S32 width);

extern BOOL
is_overlapped(
    coord coff,
    coord roff);

extern coord
justifyline(
    uchar *,
    coord,
    uchar,
    uchar *);

extern S32
limited_fwidth_of_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow);

extern void
my_rectangle(
    coord x_pos,
    coord y_pos,
    coord x_size,
    coord y_size);

extern coord
outslt(
    P_CELL tcell,
    ROW row,
    coord fwidth);

extern void
position_cursor(void);

extern void
topline(
    coord x_pos,
    coord y_pos,
    coord x_size);

extern void
twzchr(
    char ch);

extern coord
lcrjust_plain(
    uchar *str,
    coord fwidth,
    BOOL reversed);

extern coord
lcrjust_riscos_fonts(
    uchar *str,
    coord fwidth,
    BOOL reversed);

#define FIX       ((uchar) (0x01))
#define LAST      ((uchar) (0x02))
#define PAGE      ((uchar) (0x04))
#define PICT      ((uchar) (0x08))
#define UNDERPICT ((uchar) (0x10))

/******************************************************************************
* doprint.c
******************************************************************************/

#define EOS     CH_NULL      /* prnout called with this to turn off highlights */
#define P_ON    ((S32) 257)  /* printer on */
#define P_OFF   ((S32) 258)  /* printer off */
#define E_P     ((S32) 259)  /* end page */
#define HMI_P   ((S32) 260)  /* HMI prefix */
#define HMI_S   ((S32) 261)  /* HMI suffix */
#define HMI_O   ((S32) 262)  /* HMI offset */

extern coord header_footer_width;

extern void
fixpage(
    ROW, ROW);

extern S32
getfield(
    FILE_HANDLE input,
    uchar *array,
    BOOL ignore_lead_spaces);

extern coord
header_length(
    COL first,
    COL last);

extern void
mspace(
    S32 n);

extern BOOL
print_complain(
    _kernel_oserror * err);

extern void
print_document(void);

extern BOOL
prnout(
    U8 ch);

extern S32
left_margin_width(void);

extern void
set_pitch(
    S32 n);

extern void
setssp(void);

extern BOOL
sndchr(
    U8 ch);

extern void
thicken_grid_lines(
    S32 thickness);

/******************************************************************************
* colour.c
******************************************************************************/

#if defined(EXTENDED_COLOUR_WINDVARS)
#define wimp_colour_value_from_option(colours_option_index) ( \
    (&current_p_docu->Xd_colours_CF)[(colours_option_index)] )
#else
#define wimp_colour_value_from_option(colours_option_index) ( \
    d_colours[colours_option_index].option )
#endif

#define wimp_colour_index_from_option(colours_option_index) ( \
    0x0F & wimp_colour_value_from_option(colours_option_index) )

#if defined(EXTENDED_COLOUR)

extern U32
decode_wimp_colour_value(
    const char * from);

extern int /* sprintf-like */
encode_wimp_colour_value(
    /*out*/ char * buffer,
    U32 buffer_bytes,
    U32 wimp_colour_value);

#endif

extern U32
get_colour_value(
    PC_USTR pos,
    _OutRef_    P_U32 p_r,
    _OutRef_    P_U32 p_g,
    _OutRef_    P_U32 p_b,
    _InVal_     uchar text_at_char);

/******************************************************************************
* cursmov.c
******************************************************************************/

extern coord
calcad(                 /* horzvec offset -> x pos */
    coord coff);

extern coord
calcoff(                /* x pos -> horzvec offset or OFF_LEFT/OFF_RIGHT */
    coord tx);

extern coord
calcoff_click(          /* ditto, but round into l/r cols */
    coord tx);

extern coord
calcoff_overlap(
    coord x_pos,
    ROW trow);

#define calrad(roff) /* vertvec offset -> y pos */ \
    (roff)

extern coord
calroff(                /* y pos -> vertvec offset */
    coord ty);

extern coord
calroff_click(          /* ditto, but round into t/b rows */
    coord ty);

extern coord
chkolp(                 /* find overlap width */
    P_CELL tcell,
    COL tcol,
    ROW trow);

extern BOOL
chkrpb(                 /* check row for page break */
    ROW trow);

extern BOOL
chkpac(                 /* page break active? */
    ROW trow);

extern BOOL
chkpbs(
    ROW trow,
    S32 offset);

/* check page breaks enabled */
#define chkfsb() (encpln  &&  (!n_rowfixes  ||  prnbit))

extern void
filhorz(
    COL,
    COL);

extern void
filvert(
    ROW,
    ROW,
    BOOL);

extern void
chknlr(             /* for absolute movement */
    COL tcol,
    ROW trow);

#define rstpag()    /* reset page counts */

/*
horzvec SCRCOL[maximum_cols]
*/

#define horzvec() \
    array_base(&horzvec_mh, SCRCOL)

#define horzvec_entry_valid(coff) \
    array_index_is_valid(&horzvec_mh, coff)

#define horzvec_entry(coff) \
    array_ptr(&horzvec_mh, SCRCOL, coff)

#define col_number(coff) \
    horzvec_entry(coff)->colno

/*
vertvec SCRROW[maximum_rows]
*/

#define vertvec() \
    array_base(&vertvec_mh, SCRROW)

#define vertvec_entry_valid(roff) \
    array_index_is_valid(&vertvec_mh, roff)

#define vertvec_entry(roff) \
    array_ptr(&vertvec_mh, SCRROW, roff)

#define row_number(roff) \
    vertvec_entry(roff)->rowno

extern S32 g_newoffset;

extern coord
get_column(
    coord tx,
    ROW trow,
    S32 xcelloffset,
    BOOL select_clicked);

extern void
get_slr_for_point(
    gcoord x,
    gcoord y,
    BOOL select_clicked,
    char *buffer /*out*/);

extern void
insert_reference_to(
    _InVal_     DOCNO docno_ins,
    _InVal_     DOCNO docno_to,
    COL tcol,
    ROW trow,
    BOOL abscol,
    BOOL absrow,
    BOOL allow_draw);

/* column offsets returned from calcoff */
#define OFF_LEFT (-1)
#define OFF_RIGHT (-2)

#define NOTFOUND        ((coord) -1)
#define BORDERCH        DOT
#define BORDERFIXCH     ((uchar) '_')

#define CALL_FIXPAGE 1
#define DONT_CALL_FIXPAGE 0

extern S32
calsiz(
    uchar *);

extern void
cencol(
    COL tcol);

extern void
cenrow(
    ROW trow);

extern void
chkmov(void);

extern COL
col_off_right(void);

extern void
curosc(void);               /* check cursor on screen */

extern void
cursdown(void);

extern void
cursup(void);

extern void
curup(void);

extern void
curdown(void);

extern COL
fstncx(void);               /* first non-fixed column on screen */

extern ROW
fstnrx(void);               /* first non-fixed row on screen */

extern BOOL
incolfixes(
    COL tcol);

extern BOOL
inrowfixes(
    ROW trow);

extern BOOL
inrowfixes1(
    ROW trow);

extern BOOL
is_blank_cell(
    P_CELL tcell);

extern void
mark_row(
    coord);

extern void
mark_row_border(
    coord);

extern void
mark_row_praps(
    coord rowonscr,
    BOOL old_row);

#define OLD_ROW TRUE
#define NEW_ROW FALSE

extern void
mark_slot(P_CELL);

extern void
nextcol(void);

extern void
prevcol(void);

extern coord
schcsc(                 /* search for col on screen */
    COL);

extern coord
schrsc(                 /* search for row on screen */
    ROW);

/******************************************************************************
* numbers.c
******************************************************************************/

/*
overheads required by different types of evaluator cell
*/

#define NUM_DAT_OVH (offsetof(EV_CELL, parms) + sizeof(EV_PARMS))
#define NUM_CON_OVH  offsetof(EV_CELL, rpn.con.rpn_str)
#define NUM_VAR_OVH  offsetof(EV_CELL, rpn.var.rpn_str)

extern S32
compile_constant(
    P_U8 text_in,
    P_EV_RESULT p_ev_result,
    P_EV_PARMS parmsp);

extern S32
compile_expression(
    char *compiled_out,
    char *text_in,
    S32 len_out,
    P_S32 at_pos,
    P_EV_RESULT p_ev_result,
    P_EV_PARMS parmsp);

extern void
actind(
    S32 number);

extern void
actind_in_block(
    S32 down_columns);

extern void
actind_end(void);

extern S32
compile_text_slot(
    char *array /*out*/,
    const char *from,
    uchar *slot_refsp /*out*/);

extern S32
compiled_text_len(
    PC_U8 ptr);

extern BOOL
dependent_links_warning(void);

extern S32
draw_find_file(
    _InVal_     DOCNO docno,
    COL col,
    ROW row,
    P_P_DRAW_DIAG p_p_draw_diag,
    P_P_DRAW_FILE_REF drawref);

extern void
draw_redraw_all_pictures(void);

extern S32
draw_str_insertslot(
    COL col,
    ROW row);

extern S32
draw_tree_str_insertslot(
    COL col,
    ROW row,
    S32 sort);

extern void
endeex(void);

extern void
endeex_nodraw(void);

PROC_UREF_PROTO(extern, eval_wants_slot_drawn);

extern void
filbuf(void);

extern COL
getcol(void);

extern BOOL
graph_active_present(
    _InVal_     DOCNO docno);

extern ghandle
graph_add_entry(
    ghandle ghan,
    _InVal_     DOCNO docno,
    COL col,
    ROW row,
    S32 x_size,
    S32 y_size,
    const char *leaf,
    const char *tag,
    S32 task);

extern void
graph_remove_entry(
    ghandle ghan);

extern P_GRAPHICS_LINK_ENTRY
graph_search_list(
    ghandle ghan);

extern S32
is_draw_file_slot(
    _InVal_     DOCNO docno,
    COL col,
    ROW row);

extern void
mark_slot(
    P_CELL tcell);

extern void
merexp(void);

extern void
merexp_reterr(
    P_S32 errp,
    P_S32 at_posp,
    S32 send_replace);

extern S32
merge_compiled_exp(
    _InRef_     PC_EV_SLR slrp,
    char justify,
    char format,
    char *compiled_out,
    S32 rpn_len,
    P_EV_RESULT p_ev_result,
    P_EV_PARMS parmsp,
    S32 load_flag,
    S32 add_refs);

extern BOOL
mergebuf(void);

extern BOOL
mergebuf_nocheck(void);

extern BOOL
merstr(
    COL tcol,
    ROW trow,
    S32 send_replace,
    S32 add_refs);

extern BOOL
pdchart_load_isolated_chart(
    PC_U8 filename);

extern S32
pdchart_load_preferred(
    P_U8 filename);

extern BOOL
pdeval_initialise(void);

extern void
prccon(
    uchar *target,
    P_CELL ptr);

extern S32
readfxy(
    S32 id,
    PC_U8 *pfrom,
    P_U8 *pto,
    PC_U8Z *name,
    _OutRef_ P_F64 xp,
    _OutRef_ P_F64 yp);

extern const uchar *
report_compiled_text_string(
    _In_z_      const uchar * cts_in);

extern S32
row_select(
    char *rpn_in,
    ROW row);

extern void
set_ev_slr(
    _OutRef_    P_EV_SLR slrp,
    _InVal_     COL col,
    _InVal_     ROW row);

extern void
seteex(void);

extern void
seteex_nodraw(void);

extern S32
slot_displays_contents(
    P_CELL tcell);

/*extern void
splat(
    uchar *to,
    S32 num,
    S32 size);*/

extern uchar *
splat_csr(
    uchar * csr,
    _InVal_     EV_DOCNO docno,
    _InVal_     COL col,
    _InVal_     ROW row);

/*extern S32
talps(
    _In_reads_bytes_(size) PC_U8 from,
    _InVal_     U32 size);*/

extern const uchar *
talps_csr(
    const uchar * csr,
    _OutRef_    P_EV_DOCNO p_docno,
    _OutRef_    P_COL p_col,
    _OutRef_    P_ROW p_row);

extern S32
write_col(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
                COL col);

extern S32
write_ref(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO docno,
                COL tcol,
                ROW trow);

extern S32
write_slr_ref(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_EV_SLR slrp);

/* actind arguments */
#define TIMER_DELAY     32

/******************************************************************************
* reperr.c
******************************************************************************/

#include "errdef.h"

/******************************************************************************
* pdsearch.c
******************************************************************************/

enum D_SEARCH_OFFSETS
{
    SCH_TARGET,
    SCH_REPLACE,
    SCH_CONFIRM,
    SCH_CASE,
    SCH_EXPRESSIONS,
    SCH_BLOCK
};

/* following variables used in check as well as search */

extern BOOL
do_replace(
    uchar *replace_str,
    BOOL folding);

/******************************************************************************
definition of cell structure
******************************************************************************/

struct CELL
{
    uchar type;                 /* type of cell */
    uchar flags;                /* error, altered & contains cell references */
    uchar justify;              /* justify bits and protected bit */
    uchar format;               /* format if it's a number cell */

    union CELL_CONTENT
    {
        uchar text[1];          /* text cell contents */

        struct CELL_CONTENT_NUMBER
        {
            EV_CELL guts;
        } number;

        struct CELL_CONTENT_PAGE
        {
            S32 cpoff;         /* current page offset */
            S32 condval;       /* value for conditional page eject, 0=unconditional */
        } page;

    } content;
};

#define SL_NUMBEROVH    ((S32) (offsetof(struct CELL, content.number.guts)))
#define SL_PAGEOVH      ((S32) (offsetof(struct CELL, content.page) + \
                                 sizeof(struct CELL_CONTENT_PAGE)))
#define SL_CELLOVH      ((S32)  offsetof(struct CELL, content))
#define SL_TEXTOVH      SL_CELLOVH

/******************************************************************************
cell types
******************************************************************************/

#define SL_TEXT     ((uchar) 0)
#define SL_NUMBER   ((uchar) 1)
#define SL_PAGE     ((uchar) 2)

/******************************************************************************
cell flags
******************************************************************************/

#define SL_ALTERED  ((uchar) 1)         /* re-display flag */
#define SL_TREFS    ((uchar) 2)         /* cell has references in text */

/******************************************************************************
format flags
******************************************************************************/

#define F_BRAC      ((uchar) 0x80)      /* 0 = -, 1=() */
#define F_DCP       ((uchar) 0x40)      /* 1 = use DCPSID as format, otherwise option dcp */
#define F_LDS       ((uchar) 0x20)      /* 1 = lead characters */
#define F_TRS       ((uchar) 0x10)      /* 1 = trail characters */
#define F_DCPSID    ((uchar) 0x0F)      /* 0x0F=free, 0-9 */

/******************************************************************************
justification flags
******************************************************************************/

#define J_FREE      ((uchar) 0) /* these must be same as C_FREE -> C_LCR */
#define J_LEFT      ((uchar) 1)
#define J_CENTRE    ((uchar) 2)
#define J_RIGHT     ((uchar) 3)
#define J_LEFTRIGHT ((uchar) 4)
#define J_RIGHTLEFT ((uchar) 5)
#define J_LCR       ((uchar) 6)

#define J_BITS      ((uchar) 127)
#define CLR_J_BITS  ((uchar) 128)
#define PROTECTED   ((uchar) 128)

/* things that need to find a home */

extern BOOL
new_main_window_height(
    tcoord height);

extern BOOL
new_main_window_width(
    tcoord width);

extern BOOL
screen_initialise(void);

extern void
screen_finalise(void);

extern void
reinit_rows(void);      /* update row information after mode change */

extern void
draw_row(
    coord);

extern void
defkey(void);           /* define a soft key */

extern void
adddef(void);           /* add definition to list */

extern void
dsp_border_col(
    coord x,
    coord y,
    uchar *string);

extern void
update_variables(void); /* write option page info internally */

extern void
change_border_variables(void);

extern void
filealtered(
    BOOL newstate);

extern S32
inpchr_riscos(
    S32 c);

extern BOOL
dependent_files_warning(void);

extern void
close_window(void);

/******************************************************************************
* slotconv.c
******************************************************************************/

extern BOOL
ensure_global_font_valid(void);

extern void
expand_current_slot_in_fonts(
    char *to /*out*/,
    BOOL partial,
    P_S32 this_fontp /*out*/);

#define DEFAULT_EXPAND_REFS 9 /*was 1*/

#define EXPAND_FLAGS_DONT_EXPAND_CTRL           (0)
#define EXPAND_FLAGS_EXPAND_CTRL                (1 << 0)

#define EXPAND_FLAGS_DONT_EXPAND_ATS            (0)
#define EXPAND_FLAGS_EXPAND_ATS_1               (1 << 1) /* SLRLDI,@D,@N */ /* for 'Make constant' */
#define EXPAND_FLAGS_EXPAND_ATS_2               (1 << 2) /* @F,@G,@P,@T */
#define EXPAND_FLAGS_EXPAND_ATS_ALL             (EXPAND_FLAGS_EXPAND_ATS_1 | EXPAND_FLAGS_EXPAND_ATS_2)

#define EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT    (0)
#define EXPAND_FLAGS_ALLOW_FONTY_RESULT         (1 << 8)
#define EXPAND_FLAGS_FONTY_RESULT(bVal)         ((bVal) ? EXPAND_FLAGS_ALLOW_FONTY_RESULT : EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT)

extern char
expand_cell(
    _InVal_     DOCNO docno,
    P_CELL p_cell,
    _InVal_     ROW row,
    char *array /*out*/,
    coord fwidth,
    S32 expand_refs,
    _InVal_     S32 expand_flags,
    _InVal_     BOOL cust_func_formula);

extern void
expand_lcr(
    char *from,
    _InVal_     ROW row,
    char *array /*out*/,
    coord fwidth,
    S32 expand_refs,
    _InVal_     S32 expand_flags,
    _InVal_     BOOL compile_lcr);

extern void
font_close_all(
    _InVal_     BOOL shutdown);

extern void
font_expand_for_break(
    char *to,
    char *from);

extern S32
font_skip(
    const char * at);

extern char
get_dec_field_from_opt(void);

extern S32
is_font_change(
    char *ch);

extern S32
font_charwid(
    HOST_FONT ch_font,
    S32 ch);

extern _kernel_oserror *
font_complain(
    _kernel_oserror * err);

extern S32
font_stack(
    S32 to_stack);

extern S32
font_unstack(
    S32 stack_state);

extern S32
result_extract(
    P_CELL sl,
    P_EV_RESULT * p_p_ev_result);

extern S32
result_sign(
    P_CELL sl);

#include "ridialog.h"

extern void
new_font_leading(
    S32 new_font_leading_mp);

extern void
new_font_leading_based_on_y(void);

/*
pdchart.c
*/

/*
exported functions
*/

extern void
ChartAdd_fn(void);

extern void
ChartDelete_fn(void);

extern void
ChartEdit_fn(void);

extern void
ChartNew_fn(void);

extern void
ChartOptions_fn(void);

extern void
ChartSelect_fn(void);

extern BOOL
dependent_charts_warning(void);

extern void
expand_cell_for_chart_export(
    _InVal_     DOCNO docno,
    P_CELL p_cell,
    char * buffer /*out*/,
    U32 elemof_buffer,
    ROW row);

extern S32
pdchart_dependent_documents(
    _InVal_     EV_DOCNO cur_docno);

_Check_return_
_Ret_notnull_
extern char *
pdchart_docname_query(void);

extern S32
pdchart_load_dependents(
    P_ANY epdchartdatakey,
    PC_U8 chartfilename);

extern void
pdchart_load_ended(
    P_ANY epdchartdatakey,
    S32 loaded_preferred);

_Check_return_
extern STATUS
pdchart_preferred_save(
    P_USTR filename);

extern void
pdchart_select_using_handle(
    P_ANY epdchartdatakey);

_Check_return_
extern STATUS
pdchart_show_editor_using_handle(
    P_ANY epdchartdatakey);

extern BOOL
pdchart_submenu_maker(void);

extern void
pdchart_submenu_select_from(
    _InVal_     S32 hit);

extern void
pdchart_submenu_show(
    BOOL submenurequest);

image_cache_tagstrip_proto(extern, pdchart_tagstrip);
image_cache_tagstrip_proto(extern, pdchart_tagstrip_legacy);

extern BOOL
Return_fn_core(void);

_Check_return_
extern STATUS
ho_help_url(
    _In_z_      PCTSTR url);

#include "pd_x.h"

/* end of datafmt.h */
