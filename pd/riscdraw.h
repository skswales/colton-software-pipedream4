/* riscdraw.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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
    S32 tx,
    S32 ty);

extern void
at_fonts(
    S32 x,
    S32 ty);

extern void
application_open_request(
    wimp_eventstr *e);

extern void
application_redraw(
    RISCOS_REDRAWSTR *r);

extern void
application_scroll_request(
    wimp_eventstr *e);

extern void
cachemodevariables(void);

extern void
cachepalettevariables(void);

extern void
clear_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
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
    S32 ty);

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
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1);

extern void
please_invert_numeric_slot(
    S32 coff,
    S32 roff,
    S32 fg,
    S32 bg);

extern void
please_invert_numeric_slots(
    S32 start_coff,
    S32 end_coff,
    S32 roff,
    S32 fg,
    S32 bg);

extern void
please_redraw_entire_window(void);

extern void
please_redraw_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1);

extern void
please_redraw_textline(
    S32 tx0,
    S32 ty0,
    S32 tx1);

extern void
please_update_textarea(
    RISCOS_REDRAWPROC proc,
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1);

extern void
please_update_window(
    RISCOS_REDRAWPROC proc,
    wimp_w window,
    S32 gx0,
    S32 gy0,
    S32 gx1,
    S32 gy1);

extern void
print_setcolours(
    S32 fore,
    S32 back);

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
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    S32 nlines);

extern void
setcaretpos(
    S32 x,
    S32 y);

extern BOOL
set_graphics_window_from_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    BOOL set_gw);

_Check_return_
extern S32
tcoord_x(
    S32 x);

_Check_return_
extern S32
tcoord_y(
    S32 y);

_Check_return_
extern S32
tcoord_x_remainder(
    S32 x);

_Check_return_
extern S32
tcoord_x1(
    S32 x);

_Check_return_
extern S32
tcoord_y1(
    S32 y);

_Check_return_
extern S32
tsize_x(
    S32 x);

_Check_return_
extern S32
tsize_y(
    S32 y);

extern BOOL
textobjectintersects(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1);

extern BOOL
textxintersects(
    S32 tx0,
    S32 tx1);

_Check_return_
extern S32
texttooffset_x(
    S32 tx);

_Check_return_
extern S32
texttooffset_y(
    S32 ty);

_Check_return_
extern S32
windowheight(void);

_Check_return_
extern S32
windowwidth(void);

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
extern S32 current_bg;
extern S32 current_fg;
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
