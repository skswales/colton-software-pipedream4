/* riscdraw.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if RISCOS

#ifndef __riscdraw_h
#define __riscdraw_h

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

/*
exported functions from riscdraw.c
*/

extern void
acquirecaret(void);

extern void
at(
    tcoord tx,
    tcoord ty);

extern void
at_fonts(
    coord x,
    tcoord ty);

extern void
application_Open_Window_Request(
    /*poked*/ WimpOpenWindowRequestEvent * const open_window_request);

extern void
application_redraw_core(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block);

extern void
application_Scroll_Request(
    _In_        const WimpScrollRequestEvent * const scroll_request);

extern void
cache_mode_variables(void);

extern void
cache_palette_variables(void);

extern void
clear_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    BOOL zap_grid);

extern void
clear_thistextarea(void);

extern void
clear_underlay(
    S32 len);

extern BOOL
draw_altered_state(void);

extern void
draw_grid_hbar(
    S32 len);

extern void
draw_grid_vbar(
    BOOL include_hbar);

extern void
ensurefontcolours(void);

_Check_return_
extern gcoord
gcoord_x(
    coord x);

_Check_return_
extern gcoord
gcoord_y(
    coord y);

_Check_return_
extern S32
gcoord_y_textout(
    tcoord ty);

extern void
killfontcolourcache(void);

extern void
killcolourcache(void);

extern void
ospca(
    S32 nspaces);

extern void
ospca_fonts(
    S32 nspaces);

extern void
new_grid_state(void);

extern void
please_clear_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1);

extern void
please_invert_number_cell(
    _InVal_     S32 coff,
    _InVal_     S32 roff,
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index);

extern void
please_invert_number_cells(
    _InVal_     S32 start_coff,
    _InVal_     S32 end_coff,
    _InVal_     S32 roff,
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index);

extern void
please_redraw_entire_window(void);

extern void
please_redraw_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1);

extern void
please_redraw_textline(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1);

extern void
please_update_textarea(
    RISCOS_REDRAWPROC proc,
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1);

extern void
please_update_window(
    RISCOS_REDRAWPROC proc,
    _HwndRef_   HOST_WND window_handle,
    gcoord os_x0,
    gcoord os_y0,
    gcoord os_x1,
    gcoord os_y1);

extern void
print_setcolours(
    _InVal_     COLOURS_OPTION_INDEX fore_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX back_colours_option_index);

extern void
print_setfontcolours(void);

extern void
restore_graphics_window(void);

extern S32
rgb_for_wimpcolour(
    S32 wimpcolour);

extern void
riscos_movespaces(
    S32 nspaces);

extern void
riscos_movespaces_fonts(
    S32 nspaces);

extern void
riscos_printspaces(
    S32 nspaces);

extern void
riscos_printspaces_fonts(
    S32 nspaces);

extern void
scroll_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    S32 nlines);

extern BOOL
set_graphics_window_from_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    BOOL set_gw);

_Check_return_
extern tcoord
tcoord_x(
    gcoord os_x);

_Check_return_
extern tcoord
tcoord_y(
    gcoord os_y);

_Check_return_
extern tcoord
tcoord_x_remainder(
    gcoord os_x);

_Check_return_
extern tcoord
tcoord_x1(
    gcoord os_x);

_Check_return_
extern tcoord
tcoord_y1(
    gcoord os_y);

_Check_return_
extern tcoord
tsize_x(
    gcoord os_x);

_Check_return_
extern tcoord
tsize_y(
    gcoord os_y);

extern BOOL
textobjectintersects(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1);

extern BOOL
textxintersects(
    tcoord tx0,
    tcoord tx1);

_Check_return_
extern gcoord
texttooffset_x(
    tcoord tx);

_Check_return_
extern gcoord
texttooffset_y(
    tcoord ty);

_Check_return_
extern tcoord
main_window_height(void);

_Check_return_
extern tcoord
main_window_width(void);

extern BOOL
riscprint_resume(void);

extern void
riscprint_set_printer_data(void);

extern BOOL
riscprint_start(void);

extern BOOL
riscprint_suspend(void);

extern BOOL
riscprint_end(
    BOOL ok);

extern BOOL
riscprint_page(
    S32 copies,
    BOOL landscape,
    S32 scale,
    S32 sequence,
    RISCOS_PRINTPROC pageproc);

/*
exported variables
*/

extern S32 dx;
extern S32 dy;
extern U32 current_bg_wimp_colour_value;
extern U32 current_fg_wimp_colour_value;
extern S32 log2bpp;

/* PipeDream specific character printing stuff */

#define leftslop    8   /* needed for visible lhs caret */
#define topslop     0   /* (charheight/4) */

#define CARET_REALCOLOUR    (1 << 27)
#define CARET_COLOURED      (1 << 26)
#define CARET_INVISIBLE     (1 << 25)
#define CARET_SYSTEMFONT    (1 << 24)
#define CARET_COLOURSHIFT   16              /* bits 16..23 */
#define CARET_BODGE_Y       (charheight/4)  /* extra bits on top & bottom of caret */

#endif  /* __riscdraw_h */

#endif  /* RISCOS */

/* end of riscdraw.h */
