/* gr_chart.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Charting module interface */

/* SKS May 1991 */

#ifndef __gr_chart_h
#define __gr_chart_h

#ifndef ERRDEF_EXPORT /* not errdef export */

/*
required includes
*/

#ifdef  __file_h
#include "file.h"
#endif

/*
exported types
*/

typedef S32 GR_MILLIPOINT;

#if RISCOS
typedef S32 GR_OSUNIT; /* has to be castable to and from GR_COORD, even though physically limited by RISC OS */
#endif

/* no GR_INCH_PER_xxx - an inch is too big */

#define GR_MILLIPOINTS_PER_INCH     (GR_MILLIPOINTS_PER_POINT * GR_POINTS_PER_INCH)  /* 72000      */
#define GR_PIXITS_PER_INCH          (GR_PIXITS_PER_POINT      * GR_POINTS_PER_INCH)  /* 1440       */
#define GR_POINTS_PER_INCH          72     /* carved in lithographic stone har har */
#define GR_RISCDRAW_PER_INCH        (GR_RISCDRAW_PER_RISCOS   * GR_RISCOS_PER_INCH)  /* 46080      */
#define GR_RISCOS_PER_INCH          180    /* Acorn derived from 90dpi (logical inch) decent monitor size */

/* no GR_xxx_PER_MILLIPOINT - too small */

#define GR_MILLIPOINTS_PER_PIXIT    (GR_MILLIPOINTS_PER_INCH  / GR_PIXITS_PER_INCH)  /* exact: 50  */
/* no GR_POINTS_PER_PIXIT - 1/20 */
#define GR_RISCDRAW_PER_PIXIT       (GR_RISCDRAW_PER_INCH     / GR_PIXITS_PER_INCH)  /* exact: 32  */
/* no GR_RISCOS_PER_PIXIT - 1/9 */

#define GR_MILLIPOINTS_PER_POINT    1000
#define GR_PIXITS_PER_POINT         20     /* logical twips */
#define GR_RISCDRAW_PER_POINT       (GR_RISCDRAW_PER_INCH     / GR_POINTS_PER_INCH)  /* exact: 640 */
/* no GR_RISCOS_PER_POINT - 5/2 */

#define GR_MILLIPOINTS_PER_RISCOS   (GR_MILLIPOINTS_PER_INCH  / GR_RISCOS_PER_INCH)  /* exact: 400 */
#define GR_PIXITS_PER_RISCOS        (GR_PIXITS_PER_INCH       / GR_RISCOS_PER_INCH)  /* exact: 8   */
/* no GR_POINTS_PER_RISCOS - 2/5 */
#define GR_RISCDRAW_PER_RISCOS      256    /* Acorn subpixel units ('football field' space) */

/* no GR_xxx_PER_RISCDRAW - too small */

/*
convert from RISC OS OS units to pixits
*/
#define gr_pixit_from_riscos(os) ((os) * (GR_COORD) GR_PIXITS_PER_RISCOS)

/*
convert from pixits to OS units (harder - must consider rounding in general)
*/
#define gr_riscos_from_pixit(px) ((GR_OSUNIT) ((px) / GR_PIXITS_PER_RISCOS))

/*
convert from pixits to millipoints
*/
#define gr_millipoint_from_pixit(p) ((p) * GR_MILLIPOINTS_PER_PIXIT)

/*
convert from pixits to RISC OS Draw units
*/
#define gr_riscDraw_from_pixit(p) ((p) * GR_RISCDRAW_PER_PIXIT)

/*
convert from points to RISC OS Draw units
*/
#define gr_riscDraw_from_point(p) ((p) * GR_RISCDRAW_PER_POINT)

/*
colour definition: RGB bytes range from 0x00 (full black) to 0xFF (full colour)
*/

typedef struct GR_COLOUR
{
    UBF manual   : 1;  /* colour has been set manually, now non-automatic */
    UBF visible  : 1;

    UBF reserved : sizeof(char)*8 - 2*1; /* pack up to char boundary */

    UBF red      : 8;  /* RGB seems most widely applicable, maybe allow HSV or CYMK selectors? with a bit and a union */
    UBF green    : 8;
    UBF blue     : 8;
}
GR_COLOUR; /* sys indep size U32 */

#define gr_colour_set_NONE(col) \
{ \
    (col).visible  = 0; \
    (col).reserved = 0; \
    (col).red      = 0; \
    (col).green    = 0; \
    (col).blue     = 0; \
}

#define gr_colour_set_RGB(col, r, g, b) \
{ \
    (col).visible  =  1;  \
    (col).reserved =  0;  \
    (col).red      = (r); \
    (col).green    = (g); \
    (col).blue     = (b); \
}

#define gr_colour_set_WHITE(col) \
    gr_colour_set_RGB(col, 0xFF, 0xFF, 0xFF)

#define gr_colour_set_VLTGRAY(col) \
    gr_colour_set_RGB(col, 0xDD, 0xDD, 0xDD)

#define gr_colour_set_LTGRAY(col) \
    gr_colour_set_RGB(col, 0xBB, 0xBB, 0xBB)

#define gr_colour_set_MIDGRAY(col) \
    gr_colour_set_RGB(col, 0x7F, 0x7F, 0x7F) /* or 0x80 ??? */

#define gr_colour_set_DKGRAY(col) \
    gr_colour_set_RGB(col, 0x44, 0x44, 0x44)

#define gr_colour_set_BLACK(col) \
    gr_colour_set_RGB(col, 0x00, 0x00, 0x00)

#define gr_colour_set_BLUE(col) \
    gr_colour_set_RGB(col, 0x00, 0x00, 0xBB)

/*
style for lines in objects
*/

typedef struct GR_LINE_PATTERN_HANDLE * GR_LINE_PATTERN_HANDLE;

#define GR_LINE_PATTERN_STANDARD ((GR_LINE_PATTERN_HANDLE) 0)

typedef struct GR_LINESTYLE
{
    GR_COLOUR              fg; /* principal line colour */

    GR_LINE_PATTERN_HANDLE pattern;

    GR_COORD               width;

#ifdef GR_CHART_FILL_LINE_INTERSTICES
    GR_COLOUR              bg; /* fill interstices along line in this colour */
#endif
}
GR_LINESTYLE, * P_GR_LINESTYLE; typedef const GR_LINESTYLE * PC_GR_LINESTYLE;

#define gr_linestyle_init(linestylep) memset32((linestylep), CH_NULL, sizeof32(*(linestylep)))

/* NB. on WINDOWS thick or coloured lines must be done by rectangle fill */

/*
style for fills in objects
*/

/* NB. on RISC OS no way to pattern fill a Draw object */
/* On WINDOWS, it's very hard, you've got window relative brushOrigin poo to deal with unless make a new context per object! */

typedef struct GR_FILL_PATTERN_HANDLE * GR_FILL_PATTERN_HANDLE;

#define GR_FILL_PATTERN_NONE ((GR_FILL_PATTERN_HANDLE) 0)

typedef struct GR_FILLSTYLE
{
    GR_COLOUR              fg; /* principal colour for fill */

    GR_FILL_PATTERN_HANDLE pattern; /* zero -> no pattern */

    struct GR_FILLSTYLE_BITS
    {
        UBF isotropic  : 1;
        UBF norecolour : 1;
        UBF notsolid   : 1;
        UBF pattern    : 1;

        UBF reserved   : sizeof(U16)*8 - 4*1;
    }
    bits; /* sys dep size U16/U32 - no expansion */

#ifdef GR_CHART_FILL_AREA_INTERSTICES
    GR_COLOUR              bg; /* alternate colour for fill */
#endif
}
GR_FILLSTYLE, * P_GR_FILLSTYLE; typedef const GR_FILLSTYLE * PC_GR_FILLSTYLE;

#define gr_fillstyle_init(fillstylep) memset32((fillstylep), CH_NULL, sizeof32(*(fillstylep)))

/*
style for strings
*/

typedef struct GR_TEXTSTYLE
{
    GR_COLOUR fg;
    GR_COLOUR bg;

    GR_COORD  width;
    GR_COORD  height;

    char      szFontName[64];
}
GR_TEXTSTYLE, * P_GR_TEXTSTYLE; typedef const GR_TEXTSTYLE * PC_GR_TEXTSTYLE;

/*
diagram
*/

typedef struct GR_DIAG
{
    ARRAY_HANDLE handle;

    GR_RISCDIAG gr_riscdiag;
}
GR_DIAG, * P_GR_DIAG, ** P_P_GR_DIAG; /* no const version as most calls will alter */

/* NB. bg and pattern unused on RISC OS as Draw files can't be explicitly pattern filled */

/*
exports from gr_chart.c
*/

/*
abstract chart handle for export
*/

typedef struct GR_CHART_HANDLE * GR_CHART_HANDLE, ** P_GR_CHART_HANDLE; typedef const GR_CHART_HANDLE * PC_GR_CHART_HANDLE;

/*
abstract chart comms handle for export
*/

typedef struct GR_INT_HANDLE * GR_INT_HANDLE, ** P_GR_INT_HANDLE;

typedef S32                       GR_CHART_ITEMNO;
typedef /*unsigned*/ S32          GR_ESERIES_NO;
typedef /*unsigned*/ S32          GR_EAXES_NO;

typedef F64 GR_CHART_NUMBER;
#define GR_CHART_NUMBER_MAX DBL_MAX

typedef struct GR_CHART_NUMPAIR
{
    GR_CHART_NUMBER x, y;
}
GR_CHART_NUMPAIR;

/*
values that are passed from data source to chart via callbacks
*/

typedef enum GR_CHART_VALUE_TYPE
{
    GR_CHART_VALUE_NONE    = 0,   /* value unavailable or not convertible to usable format */
    GR_CHART_VALUE_TEXT    = 1,
    GR_CHART_VALUE_NUMBER  = 2,
    GR_CHART_VALUE_N_ITEMS = 3
}
GR_CHART_VALUE_TYPE;

/*
special item numbers for callback info
*/

typedef enum GR_CHART_SPECIAL_ITEMNO
{
    GR_CHART_ITEMNO_N_ITEMS = -1,
    GR_CHART_ITEMNO_LABEL   = -2
}
GR_CHART_SPECIAL_ITEMNO;

typedef struct GR_CHART_VALUE
{
    GR_CHART_VALUE_TYPE type;

    union GR_CHART_VALUE_DATA
    {
        GR_CHART_NUMBER number;
        char            text[256]; /* fixed so callbacks can check to not overwrite */
        GR_CHART_ITEMNO n_items;
    }
    data;
}
GR_CHART_VALUE, * P_GR_CHART_VALUE;

/*
callback to data source from chart to obtain data
*/

typedef S32 (* gr_chart_travelproc) (
    P_ANY handle,
    GR_CHART_HANDLE ch,
    GR_CHART_ITEMNO item,
    _OutRef_    P_GR_CHART_VALUE val /*out*/);

#define gr_chart_travel_proto(_e_s, _p_proc_gr_chart_travel) \
_e_s S32 _p_proc_gr_chart_travel( \
    P_ANY handle, \
    GR_CHART_HANDLE ch, \
    GR_CHART_ITEMNO item, \
    _OutRef_    P_GR_CHART_VALUE val /*out*/)

_Check_return_
extern BOOL
gr_chart_preferred_get_name(
    P_U8 buffer,
    U32 bufsiz);

_Check_return_
extern STATUS
gr_chart_save_external(
    GR_CHART_HANDLE ch,
    FILE_HANDLE f,
    P_U8 save_filename /*const*/,
    P_ANY ext_handle);

_Check_return_
extern STATUS
gr_ext_construct_load_this(
    GR_CHART_HANDLE ch,
    P_U8 args,
    U16 contab_ix);

/*
exported functions
*/

_Check_return_
extern BOOL
gr_chart_add(
    P_GR_CHART_HANDLE chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

_Check_return_
extern BOOL
gr_chart_add_labels(
    P_GR_CHART_HANDLE chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

_Check_return_
extern BOOL
gr_chart_add_text(
    P_GR_CHART_HANDLE chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

extern void
gr_chart_damage(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE p_int_handle /*const*/);

extern void
gr_chart_diagram(
    PC_GR_CHART_HANDLE chp,
    _OutRef_    P_P_GR_DIAG dcpp);

extern void
gr_chart_diagram_ensure(
    PC_GR_CHART_HANDLE chp);

extern void
gr_chart_dispose(
    P_GR_CHART_HANDLE chp /*inout*/);

_Check_return_
extern BOOL
gr_chart_insert(
    P_GR_CHART_HANDLE chp /*inout*/,
    gr_chart_travelproc proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out,
    P_GR_INT_HANDLE p_int_handle_after /*const*/);

extern void
gr_chart_modify_and_rebuild(
    PC_GR_CHART_HANDLE chp);

extern void
gr_chart_name_query(
    PC_GR_CHART_HANDLE chp,
    _Out_writes_(bufsiz) P_U8Z szName,
    _InVal_     U32 bufsiz);

_Check_return_
extern STATUS
gr_chart_name_set(
    PC_GR_CHART_HANDLE chp,
    _In_opt_z_  PC_U8Z szName);

_Check_return_
extern STATUS
gr_chart_new(
    P_GR_CHART_HANDLE chp /*out*/,
    P_ANY ext_handle,
    S32 new_untitled);

_Check_return_
extern U32
gr_chart_order_query(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE p_int_handle /*const*/);

_Check_return_
extern STATUS
gr_chart_preferred_new(
    /*out*/ P_GR_CHART_HANDLE chp,
    P_ANY ext_handle);

_Check_return_
extern BOOL
gr_chart_preferred_query(void);

_Check_return_
extern STATUS
gr_chart_preferred_save(
    P_U8 filename /*const*/);

_Check_return_
extern STATUS
gr_chart_preferred_set(
    PC_GR_CHART_HANDLE chp);

_Check_return_
extern STATUS
gr_chart_preferred_use(
    PC_GR_CHART_HANDLE chp);

_Check_return_
extern BOOL
gr_chart_query_exists(
    /*out*/ P_GR_CHART_HANDLE chp,
    /*out*/ P_P_ANY p_ext_handle,
    _In_z_      PC_U8Z szName);

_Check_return_
extern BOOL
gr_chart_query_labelling(
    PC_GR_CHART_HANDLE chp);

_Check_return_
extern BOOL
gr_chart_query_labels(
    PC_GR_CHART_HANDLE chp);

_Check_return_
extern BOOL
gr_chart_query_modified(
    PC_GR_CHART_HANDLE chp);

/*ncr*/
extern S32
gr_chart_subtract(
    PC_GR_CHART_HANDLE chp,
    P_GR_INT_HANDLE p_int_handle /*inout*/);

/*ncr*/
extern BOOL
gr_chart_update_handle(
    PC_GR_CHART_HANDLE chp,
    P_ANY new_ext_handle,
    P_GR_INT_HANDLE p_int_handle);

#define GR_CHART_CREATORNAME "PDreamCharts"
                            /*012345678901*/

/*
end of exports from gr_chart.c
*/

/*
exports from gr_chtIO.c
*/

/*
construct table for clients
*/

typedef struct GR_CONSTRUCT_TABLE_ENTRY
{
    P_U8  txt_id;
    U16   arg_type_save;
    U16   arg_type_load;
    U16   obj_type;
    U16   offset;
#if CHECKING
    U16   contab_ix_dbg;
#endif
}
GR_CONSTRUCT_TABLE_ENTRY; typedef GR_CONSTRUCT_TABLE_ENTRY * P_GR_CONSTRUCT_TABLE_ENTRY;

#if CHECKING
#define gr_contab_entry(str, contab_ix_dbg) { (str), 0, 0, 0, 0, (contab_ix_dbg) }
#define gr_contab_entry_last                {  NULL, 0, 0, 0, 0, 0               }
#else
#define gr_contab_entry(str, contab_ix_dbg) { (str), 0, 0, 0, 0 }
#define gr_contab_entry_last                {  NULL, 0, 0, 0, 0 }
#endif

_Check_return_
extern STATUS
gr_chart_construct_save_frag_end(
    FILE_HANDLE f);

_Check_return_
extern STATUS
gr_chart_construct_save_frag_stt(
    FILE_HANDLE f,
    U16 ext_contab_ix);

_Check_return_
extern STATUS
gr_chart_construct_save_frag_txt(
    FILE_HANDLE f,
    P_U8 str /*const*/);

_Check_return_
extern STATUS
gr_chart_construct_save_txt(
    FILE_HANDLE f,
    U16 ext_contab_ix,
    P_U8 str /*const*/);

extern void
gr_chart_construct_table_register(
    P_GR_CONSTRUCT_TABLE_ENTRY table,
    U16 last_ext_contab_ix);

_Check_return_
extern STATUS
gr_chart_construct_tagstrip_process(
    P_GR_CHART_HANDLE chp,
    P_IMAGE_CACHE_TAGSTRIP_INFO p_info);

_Check_return_
extern STATUS
gr_chart_save_chart_with_dialog(
    PC_GR_CHART_HANDLE chp);

_Check_return_
extern STATUS
gr_chart_save_chart_without_dialog(
    PC_GR_CHART_HANDLE chp,
    P_U8 filename /*const*/);

_Check_return_
extern STATUS
gr_chart_save_draw_file_with_dialog(
    PC_GR_CHART_HANDLE cehp);

_Check_return_
extern STATUS
gr_chart_save_draw_file_without_dialog(
    PC_GR_CHART_HANDLE chp,
    P_U8 filename /*const*/);

_Check_return_
extern BOOL
gr_chart_saving_chart(
    /*out*/ P_U8 name_during_send);

/*
end of exports from gr_chtIO.c
*/

/*
exports from gr_editc.c
*/

/*
abstract chart editor handle for export
*/

typedef struct GR_CHARTEDIT_HANDLE * GR_CHARTEDIT_HANDLE, ** P_GR_CHARTEDIT_HANDLE; typedef const GR_CHARTEDIT_HANDLE * PC_GR_CHARTEDIT_HANDLE;

/*
values that are passed from chart editor to owner via callbacks
*/

typedef enum GR_CHARTEDIT_NOTIFY_TYPE
{
    GR_CHARTEDIT_NOTIFY_RESIZEREQ,
    GR_CHARTEDIT_NOTIFY_CLOSEREQ,
    GR_CHARTEDIT_NOTIFY_SELECTREQ,
    GR_CHARTEDIT_NOTIFY_TITLEREQ
}
GR_CHARTEDIT_NOTIFY_TYPE;

typedef struct GR_CHARTEDIT_NOTIFY_RESIZE_STR
{
    GR_POINT new_size, old_size;
}
GR_CHARTEDIT_NOTIFY_RESIZE_STR;

typedef struct GR_CHARTEDIT_NOTIFY_TITLE_STR
{
    char title[256];
}
GR_CHARTEDIT_NOTIFY_TITLE_STR;

/*
callback from chart editor to owner for various reasons
*/

typedef STATUS (* gr_chartedit_notify_proc) (
    P_ANY handle,
    GR_CHARTEDIT_HANDLE ceh,
    GR_CHARTEDIT_NOTIFY_TYPE ntype,
    P_ANY nextra);

#define gr_chartedit_notify_proto(_e_s, _p_proc_gr_chartedit_notify, handle, ceh, ntype, nextra) \
_e_s STATUS _p_proc_gr_chartedit_notify( \
    P_ANY handle, \
    GR_CHARTEDIT_HANDLE ceh, \
    GR_CHARTEDIT_NOTIFY_TYPE ntype, \
    P_ANY nextra)

/*
exported functions from gr_editc.c
*/

extern void
gr_chartedit_dispose(
    /*inout*/ P_GR_CHARTEDIT_HANDLE cehp);

extern void
gr_chartedit_front(
    PC_GR_CHARTEDIT_HANDLE cehp);

_Check_return_
extern STATUS
gr_chartedit_new(
    _OutRef_    P_GR_CHARTEDIT_HANDLE cehp,
    GR_CHART_HANDLE ch,
    gr_chartedit_notify_proc nproc,
    P_ANY nhandle);

/*
callback from self AND (NB!) default cases of externals
*/

gr_chartedit_notify_proto(extern, gr_chartedit_notify_default, handle, ceh, ntype, nextra);

/*
end of exports from gr_editc.c
*/

/*
exports from gr_editt.c
*/

/*
exported functions
*/

extern void
gr_chart_text_order_set(
    P_GR_CHART_HANDLE chp /*const*/,
    S32 order);

/*
end of exports from gr_editt.c
*/

#endif /* ERRDEF_EXPORT */

/*
error definition
*/

#define GR_CHART_ERRLIST_DEF \
    errorstring(GR_CHART_ERR_INVALID_DRAWFILE,          "Not a valid Draw file") \
    errorstring(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED,   "Draw files must be saved before being imported") \
    errorstring(GR_CHART_ERR_NO_SELF_INSERT,            "Cannot save chart files into chart editing windows") \
    errorstring(GR_CHART_ERR_NOT_ENOUGH_INPUT,          "Not enough data for chart") \
    errorstring(GR_CHART_ERR_NEGATIVE_OR_ZERO_IGNORED,  "Negative or zero data ignored") \
    errorstring(GR_CHART_ERR_ALREADY_EDITING,           "Already editing chart") \
    errorstring(GR_CHART_ERR_EXCEPTION,                 "Arithmetic overflow in chart creation") \

/*
error definition
*/

#define GR_CHART_ERR_BASE                       (-8000)

#define GR_CHART_ERR_spare_8000                 (-8000)
#define GR_CHART_ERR_INVALID_DRAWFILE           (-8001)
#define GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED    (-8002)
#define GR_CHART_ERR_NO_SELF_INSERT             (-8003)
#define GR_CHART_ERR_NOT_ENOUGH_INPUT           (-8004)
#define GR_CHART_ERR_NEGATIVE_OR_ZERO_IGNORED   (-8005)
#define GR_CHART_ERR_ALREADY_EDITING            (-8006)
#define GR_CHART_ERR_EXCEPTION                  (-8007)

#define GR_CHART_ERR_END                        (-8008)

/*
message definition (keep consistent with CModules.*.msg.gr_chart)
*/

#define GR_CHART_MSG_BASE               8000

#define GR_CHART_MSG_MENUHDR_CHART      8000
#define GR_CHART_MSG_MENU_CHART         8001

enum GR_CHART_MO_CHART_ENUM
{
    GR_CHART_MO_CHART_OPTIONS = 1,
    GR_CHART_MO_CHART_SAVE,
    GR_CHART_MO_CHART_GALLERY,
    GR_CHART_MO_CHART_SELECTION,
    GR_CHART_MO_CHART_LEGEND,
    GR_CHART_MO_CHART_NEW_TEXT,

    GR_CHART_MO_CHART_TRACE, /* always defined, often unused */
    GR_CHART_MO_CHART_DEBUG  /* ditto */
};

#define GR_CHART_MSG_MENUHDR_SAVE       8010
#define GR_CHART_MSG_MENU_SAVE          8011

enum GR_CHART_MO_SAVE_ENUM
{
    GR_CHART_MO_SAVE_CHART = 1,
    GR_CHART_MO_SAVE_DRAWFILE
};

#define GR_CHART_MSG_MENUHDR_GALLERY    8020
#define GR_CHART_MSG_MENU_GALLERY       8021

enum GR_CHART_MO_GALLERY_ENUM
{
    GR_CHART_MO_GALLERY_PIE = 1,
    GR_CHART_MO_GALLERY_BAR,
    GR_CHART_MO_GALLERY_LINE,
    GR_CHART_MO_GALLERY_SCATTER,
    GR_CHART_MO_GALLERY_OVERLAYS,
    GR_CHART_MO_GALLERY_PREFERRED,
    GR_CHART_MO_GALLERY_SET_PREFERRED,
    GR_CHART_MO_GALLERY_SAVE_PREFERRED
};

#define GR_CHART_MSG_MENUHDR_SELECTION  8030
#define GR_CHART_MSG_MENU_SELECTION     8031

enum GR_CHART_MO_SELECTION_ENUM
{
    GR_CHART_MO_SELECTION_FILLCOLOUR = 1,
    GR_CHART_MO_SELECTION_FILLSTYLE,
    GR_CHART_MO_SELECTION_LINECOLOUR,
    GR_CHART_MO_SELECTION_LINEWIDTH,
    GR_CHART_MO_SELECTION_LINEPATTERN,
    GR_CHART_MO_SELECTION_TEXT,
    GR_CHART_MO_SELECTION_TEXTCOLOUR,
    GR_CHART_MO_SELECTION_TEXTSTYLE,
    GR_CHART_MO_SELECTION_HINTCOLOUR,
    GR_CHART_MO_SELECTION_AXIS,
    GR_CHART_MO_SELECTION_SERIES
};

#define GR_CHART_MSG_MENUHDR_SELECTION_TEXT 8035
#define GR_CHART_MSG_MENU_SELECTION_TEXT    8036

enum GR_CHART_MO_SELECTION_TEXT_ENUM
{
    GR_CHART_MO_SELECTION_TEXT_EDIT = 1,
    GR_CHART_MO_SELECTION_TEXT_DELETE
};

#define GR_CHART_MSG_MENUHDR_LEGEND     8040
#define GR_CHART_MSG_MENU_LEGEND        8041

enum GR_CHART_MO_LEGEND_ENUM
{
    GR_CHART_MO_LEGEND_ARRANGE = 1
};

enum GR_CHART_MSG_ID
{
    GR_CHART_MSG_DEFAULT_CHARTLEAFNAME   = 8100,
    GR_CHART_MSG_CHARTSAVE_ERROR         = 8101,
    GR_CHART_MSG_DEFAULT_DRAWLEAFNAME    = 8102,
    GR_CHART_MSG_DRAWFILESAVE_ERROR      = 8103,
    GR_CHART_MSG_EDIT_APPEND_COLOUR      = 8104,
    GR_CHART_MSG_EDIT_APPEND_STYLE       = 8105,
    GR_CHART_MSG_EDIT_APPEND_TEXTSTYLE   = 8106,
    GR_CHART_MSG_EDIT_APPEND_TEXTCOLOUR  = 8107,
    GR_CHART_MSG_EDIT_APPEND_HINTCOLOUR  = 8108,
    GR_CHART_MSG_EDIT_APPEND_BORDER      = 8109,
    GR_CHART_MSG_EDIT_APPEND_AREA        = 8110,
    GR_CHART_MSG_EDIT_APPEND_LINE        = 8111,
    GR_CHART_MSG_EDIT_APPEND_FILL        = 8112,
    GR_CHART_MSG_TEXT_ZD                 = 8113,
    GR_CHART_MSG_DEFAULT_FONTNAME        = 8114,
    GR_CHART_MSG_EDIT_APPEND_BASE        = 8115,
    GR_CHART_MSG_EDITOR_LINEWIDTH_THIN   = 8116,
    GR_CHART_MSG_DEFAULT_CHARTZD         = 8117,
    GR_CHART_MSG_WINGE_SERIES_PICTURE    = 8118,
    GR_CHART_MSG_WINGE_SERIES_MARKER     = 8119,
    GR_CHART_MSG_FILES_SERIES_PICTURE    = 8120, /* and 21,22,23 */
    GR_CHART_MSG_X_AXIS_LEFT_POKE_BOTTOM = 8124,
    GR_CHART_MSG_X_AXIS_RIGHT_POKE_TOP   = 8125,
    GR_CHART_MSG_FILES_SERIES_MARKER     = 8126, /* and 27,28,29 */
    GR_CHART_MSG_ALTERNATE_FONTNAME      = 8130,
    GR_CHART_MSG_DEFAULT_FONTWIDTH       = 8131,
    GR_CHART_MSG_DEFAULT_FONTHEIGHT      = 8132,
    GR_CHART_MSG_DEFAULT_CHARTINNAMEZD   = 8133
};

extern STATUS
draw_do_render(
    void * data,
    int length,
    int x,
    int y,
    F64 xfactor,
    F64 yfactor,
    P_GDI_BOX graphics_window);

/*
required functions
*/

extern BOOL
gr_chart_ensure_sprites(void);

extern FILETYPE_RISC_OS
gr_chart_save_as_filetype(void);

#endif /* __gr_chart_h */

/* end of gr_chart.h */
