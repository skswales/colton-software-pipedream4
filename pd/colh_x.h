/* colh_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RCM Aug 1991 */

#ifndef __colh_x_h
#define __colh_x_h

#ifndef __cs_wimp_h
#include "cs-wimp.h"
#endif

/* exported routines */

extern void
colh_draw_column_headings(void);

extern void
colh_draw_contents_of_number_cell(void);

extern void
colh_draw_drag_state(
    int size);

extern void
colh_draw_edit_state_indicator(void);

extern void
colh_draw_mark_state_indicator(
    BOOL sheet_marked);

extern void
colh_draw_cell_count(
    _In_opt_z_      char *text);

extern void
colh_draw_cell_count_in_document(
    _In_opt_z_      char *text);

extern BOOL
colh_event_handler(
    wimp_eventstr *e,
    void *handle);

extern void
colh_pointershape_drag_notification(
    BOOL start);

extern void
colh_position_icons(void);

extern void
colh_colour_icons(void);    /*>>>rename as colh_recolour_icons*/

extern void
redefine_icon(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    WimpCreateIconBlock * const create);

/* Icons in the colh window */

#define COLH_BUTTON_OK          (0)
#define COLH_BUTTON_CANCEL      (1)
#define COLH_CONTENTS_LINE      (2)
#define COLH_FUNCTION_SELECTOR  (3)
/*#define COLH_PIPEDREAM_LOGO     (4) no longer needed */

#define COLH_STATUS_BORDER      (5)
/*#define COLH_STATUS_BAR         (6) no longer needed */
#define COLH_STATUS_TEXT        (7)
#define COLH_COLUMN_HEADINGS    (8)

#define COLH_BUTTON_REPLICD     (9)
#define COLH_BUTTON_REPLICR     (10)
#define COLH_BUTTON_TOTEXT      (11)
#define COLH_BUTTON_TONUMBER    (12)
#define COLH_BUTTON_GRAPH       (13)

#define COLH_BUTTON_SAVE        (14)
#define COLH_BUTTON_PRINT       (15)

#define COLH_BUTTON_LJUSTIFY    (16)
#define COLH_BUTTON_CJUSTIFY    (17)
#define COLH_BUTTON_RJUSTIFY    (18)
#define COLH_BUTTON_FJUSTIFY    (19)

#define COLH_BUTTON_FONT        (20)
#define COLH_BUTTON_BOLD        (21)
#define COLH_BUTTON_ITALIC      (22)
#define COLH_BUTTON_UNDERLINED  (23)
#define COLH_BUTTON_SUBSCRIPT   (24)
#define COLH_BUTTON_SUPERSCRIPT (25)

#define COLH_BUTTON_LEADTRAIL   (26)
#define COLH_BUTTON_DECPLACES   (27)

#define COLH_BUTTON_MARK        (28)
#define COLH_BUTTON_COPY        (29)
#define COLH_BUTTON_DELETE      (30)
#define COLH_BUTTON_PASTE       (31)
#define COLH_BUTTON_FORMATBLOCK (32)

#define COLH_BUTTON_SEARCH      (33)
#define COLH_BUTTON_SORT        (34)
#define COLH_BUTTON_SPELLCHECK  (35)

#define COLH_BUTTON_CMD_RECORD  (36)
#define COLH_BUTTON_CMD_EXEC    (37)

/* Pointer shape change stuff */

typedef struct
{
    char *name;         /* Sprite name   */
    int   x;            /* Offset to     */
    int   y;            /*  active point */

}
pointer_shape;

extern void
colh_where_in_column_headings(
    const WimpMouseClickEvent * const mouse_click,
    pointer_shape **shapep,
    int *subpositionp,
    P_COL p_col,
    coord *txp);

extern void
riscos_setpointershape(
    pointer_shape * shape);

#define OVER_COLUMN_DEAD_SPACE          23
#define OVER_COLUMN_CENTRE              24
#define OVER_COLUMN_MARGIN_ADJUSTOR     25
#define OVER_COLUMN_WIDTH_ADJUSTOR      26

#define POINTER_DEFAULT                 NULL
#define POINTER_DROPMENU                &pointer_dropmenu
#define POINTER_DRAGCOLUMN              &pointer_dragcolumn
#define POINTER_DRAGCOLUMN_RIGHT        &pointer_dragcolumn_right
#define POINTER_DRAGMARGIN              &pointer_dragmargin

extern pointer_shape pointer_dropmenu;
extern pointer_shape pointer_dragcolumn;
extern pointer_shape pointer_dragcolumn_right;
extern pointer_shape pointer_dragmargin;

/*>>>this lot should really be in h.expedit or something */

extern void
formline_claim_focus(void);

/* exported types */

typedef struct FORMULA_WINDOW
{
    struct MLEC_STRUCT * mlec;

    HOST_WND     fw_main_window_handle;
    HOST_WND     fw_pane_window_handle;

    void        *fw_main_template;
    void        *fw_pane_template;

    /* pane window opened relative to main window by these offsets */
    GDI_COORD    offset_left;
    GDI_COORD    offset_bottom;
    GDI_COORD    offset_right;
    GDI_COORD    offset_top;

    DOCNO        docno;
    BOOL         tracking;
}
FORMULA_WINDOW, * FORMULA_WINDOW_HANDLE, ** P_FORMULA_WINDOW_HANDLE;

extern void
EditFormula_fn(void);

extern void
EditFormulaInWindow_fn(void);

extern void
expedit_edit_current_cell(
    S32 caretpos,
    BOOL force_inbox);

extern void
expedit_edit_current_cell_freshcontents(
    BOOL force_inbox);

extern void
expedit_recompile_current_cell_reporterrors(void);

extern void
expedit_transfer_line_to_box(
    BOOL newline);

extern BOOL
expedit_insert_char(
    _InVal_     char ch);

extern BOOL
expedit_insert_string(
    _In_z_      const char *insertstr);

extern BOOL
expedit_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp);

extern void
expedit_delete_bit_of_line(
    S32 col_start,
    S32 col_end,
    S32 row);

extern void
expedit_get_cursorpos(
    P_S32 p_col,
    P_S32 p_row);

extern void
expedit_set_cursorpos(
    S32 col,
    S32 row);

extern void
expedit_close_file(
    _InVal_     DOCNO docno);

extern void
formline_cancel_edit(void);

extern BOOL
formline_mergebacktext(
    BOOL reporterror,
    P_S32 caretposp);

extern void
formline_claim_focus(void);

#endif /* __colh_x_h */

/* end of colh_x.h */
