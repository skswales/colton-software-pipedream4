/* riscdraw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Some code for RISC OS text cell handling */

/* Stuart K. Swales 15-Mar-1989 */

#include "common/gflags.h"

#include "datafmt.h"

#if RISCOS
/* Module only compiled if RISCOS */

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __print_h
#include "print.h"
#endif

#include "cmodules/mlec.h"

#include "cmodules/muldiv.h"

#include "colh_x.h"
#include "riscos_x.h"
#include "riscdraw.h"
#include "pd_x.h"

/*
internal types
*/

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

/*
internal functions
*/

#define pd_muldiv64(a, b, c) \
    ((S32) muldiv64_limiting(a, b, c))

/* ----------------------------------------------------------------------- */

#define TRACE_CLIP      (TRACE_APP_PD4_RENDER & 0)
#define TRACE_DRAW      (TRACE_APP_PD4_RENDER)
#define TRACE_SETCOLOUR (TRACE_APP_PD4_RENDER)

/*
* +---------------------------------------------------------------------------+
* |                                                                           |
* |     work area extent origin                                               |
* |       + --  --  --  --  --  --  --  --  --  -- --  --  --  --  --  -- --  |
* |             .                                                   ^         |
* |       | TLS          TMS                                        |         |
* |         .   +   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |  .  .   |
* |       |     TCO                                              yscroll      |
* |                 +---+---+------------------------------+---+    |         |
* |       |     .   | B | C |     T i t l e    b a r       | T |    v         |
* |                 +---+---+------------------------------+---+ <---v_a.ymax |
* |       |     .   |                                      | U |              |
* |                 |                                      +---+              |
* |       |     .   |                                      |   |              |
* |                 |                                      | = |              |
* |       |   L .   |                                      | # |              |
* |           M     |                                      | = |              |
* |       |   S .   |                                      |   |              |
* |                 |                                      |   |              |
* |       |     .   |                                      |   |              |
* |                 |                                      +---+              |
* |       |     .   |                                      | D |              |
* |                 +---+------------------------------+---+---+ <---v_a.ymin |
* |       |     .   | L |      [-#-]                   | R | S |              |
* |                 +---+------------------------------+---+---+              |
* |       |<--xsc-->^                                      ^                  |
* |                 |                                      |                  |
* |       |         |                                      |                  |
* |                 v_a.xmin                               v_a.xmax           |
* +---------------------------------------------------------------------------+
* ^
* graphics origin
*/

/* gdi_org.x = v_a.xmin - xscroll */
/* gdi_org.x = v_a.ymax - yscroll */

/*  a text cell
*   ===========
*
*        | <-------- charwidth --------> |
*        |                               |
*        |                           ->| |<- dx
*        |                             | |
*
*       +-.-|-.-|-.-|-.-|-.-|-.-|-.-|-.-+
*       |                               |
*       |               X   (9+dy)      |
*       | ----------------------------- |
*       |                               |   <- vpos for VDU 5 text plot
*       |               7   (+dy)       |
*       | ----------------------------- |
*       |                               |
*       |               6   (7+dy)      |   v
*       | ----------------------------- |   -
*       |                               |   - dy
*       |               5   (6+dy)      |   ^
*       | ----------------------------- |
*       |                               |
*       |               4   (5+dy)      |
*       | ----------------------------- |
*       |                               |
*       |               3   (4+dy)      |   v
*       | ----------------------------- |   -
*       |                               |
*       |               2   (3+dy)      |     v_dy
*       | ----------------------------- |   -
*       |                               |   ^
*       |               1   (2+dy)      |   <- system font baseline
*       | ----------------------------- |
*       |                               |
*       |               0   (1+dy)      |   <- notional cell yorg
*       + ----------------------------- +
*       |                               |
*       |               X   (0+dy)      |
*       | ----------------------------- |
*       |               X   (0)         |   <- notional cell yorg if grid
*       +-.-|-.-|-.-|-.-|-.-|-.-|-.-|-.-+
*
*        ^
*       cell xorg
*/

/* log2 bits per pixel */
S32 log2bpp;

/* OS units per real pixel - needed for drawing correctly */

       S32 dx;
static S32 dxm1;       /* dx - 1 for AND/BIC */

       S32 dy;
static S32 dym1;
static S32 chmdy;      /* charheight - dy */

/* a mode independent y spacing value */
#define v_dy 4

/* size and spacing of VDU 5 chars in this mode */
static S32 default_gcharxsize;
static S32 default_gcharysize;
static S32 default_gcharxspace;
static S32 default_gcharyspace;

/* maximum size the contents of a window could be given in this screen mode */
static GDI_COORD max_poss_os_height;
static GDI_COORD max_poss_os_width;

/* current fg/bg colours for this call of update/redraw (temp globals) */
U32 current_fg_wimp_colour_value = (U32) -1;
U32 current_bg_wimp_colour_value = (U32) -1;

static U32 invert_fg_colours_option_index;
static U32 invert_bg_colours_option_index;

static BOOL font_colours_invalid = TRUE;

static GDI_BOX saved_graphics_window; /* for restore */

/******************************************************************************
*
* calculate org, textcell_org given the (x, y) position of
* the top left of the visible region
*
******************************************************************************/

static void
setorigins(
    S32 x,
    S32 y)
{
    int xorg = x + leftslop;        /* adjust for slop now */
    int yorg = y - topslop;

    trace_2(TRACE_APP_PD4_RENDER, "setting origin (%d, %d)", xorg, yorg);

    textcell_xorg = xorg;
    textcell_yorg = yorg;
}

extern void
killfontcolourcache(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "killfontcolourcache()");

    font_colours_invalid = TRUE;
}

extern void
killcolourcache(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "killcolourcache()");

    current_fg_wimp_colour_value = (U32) -1;
    current_bg_wimp_colour_value = (U32) -1;

    font_colours_invalid = TRUE;
}

/******************************************************************************
*
* screen redraw
*
* called as many times as needed until all rectangles are repainted
*
******************************************************************************/

extern void
application_redraw_core(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block)
{
    UNREFERENCED_PARAMETER_InRef_(redraw_window_block);

    /* calculate 'text window' in document that needs painting */
    /* note the flip & xlate of y coordinates from graphics to text */
    cliparea.x0 = tcoord_x( graphics_window.x0);
    cliparea.x1 = tcoord_x1(graphics_window.x1);
    cliparea.y0 = tcoord_y( graphics_window.y0);
    cliparea.y1 = tcoord_y1(graphics_window.y1);

    trace_2(TRACE_APP_PD4_RENDER, "app_redraw: textcell org %d, %d; ", textcell_xorg, textcell_yorg);
    trace_4(TRACE_APP_PD4_RENDER, " graphics window %d, %d, %d, %d",
            graphics_window.x0, graphics_window.y0, graphics_window.x1, graphics_window.y1);
    trace_4(TRACE_APP_PD4_RENDER, " invalid text area %d, %d, %d, %d",
            thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1);

    setcolours(COI_FORE, COI_BACK);

    /* ensure all of buffer displayed on redraw */
    dspfld_from = -1;

    maybe_draw_screen();

    #ifdef HAS_FUNNY_CHARACTERS_FOR_WRCH
    wrch_undefinefunnies();     /* restore soft character definitions */
    #endif
}

/******************************************************************************
*
* New colour value handling
*
******************************************************************************/

_Check_return_
static int /*colnum*/
colourtrans_ReturnColourNumberForMode(
    _In_        unsigned int word,
    _In_        unsigned int mode,
    _In_        unsigned int palette)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    rs.r[1] = mode;
    rs.r[2] = palette;
    return(_kernel_swi(ColourTrans_ReturnColourNumberForMode, &rs, &rs) ? 0 : rs.r[0]);
}

_Check_return_
static inline _kernel_oserror *
colourtrans_SetGCOL(
    _In_        unsigned int word,
    _In_        int flags,
    _In_        int gcol_action)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    rs.r[3] = flags;
    rs.r[4] = gcol_action;
    assert((rs.r[3] & 0xfffffe7f) == 0); /* just bits 7 and 8 are valid */
    return(_kernel_swi(ColourTrans_SetGCOL, &rs, &rs)); /* ignore gcol_out */
}

static inline void
os_rgb_from_wimp_colour_value(
    _Out_       RISCOS_PALETTE_U * p_os_rgb,
    _InVal_     U32 wimp_colour_value /* BBGGRR10 */ )
{
    p_os_rgb->bytes.gcol  = 0;
    p_os_rgb->bytes.red   = (wimp_colour_value >>  8) & 0xFF;
    p_os_rgb->bytes.green = (wimp_colour_value >> 16) & 0xFF;
    p_os_rgb->bytes.blue  = (wimp_colour_value >> 24) & 0xFF;
}

extern void
riscos_set_wimp_colour_value_index_byte(
    _InoutRef_  P_U32 p_wimp_colour_value)
{
    U32 wimp_colour_value = *p_wimp_colour_value & ~0x1F;

    /* map this BBGGRRS0 to a 16-colour mode index */
    int colnum = colourtrans_ReturnColourNumberForMode(wimp_colour_value, 12, (int) &wimptx__palette.c[0]);

    /*reportf("cvib: 0x%.8X -> 0x%.2X", wimp_colour_value, colnum);*/
    if(colnum <= 0x0F)
        wimp_colour_value |= (U32) colnum;

    wimp_colour_value |= 0x10;

    reportf("cvib: 0x%.8X", wimp_colour_value);
    *p_wimp_colour_value = wimp_colour_value;
}

/******************************************************************************
*
* Set document fg/bg colours iff different to those already set
*
******************************************************************************/

extern void
riscos_set_bg_colour_from_wimp_colour_value(
    _InVal_     U32 wimp_colour_value)
{
    if(wimp_colour_value == current_bg_wimp_colour_value)
        return;

    current_bg_wimp_colour_value = wimp_colour_value;
    font_colours_invalid = TRUE;

    trace_1(TRACE_SETCOLOUR, "wimp_setcolour(bg " U32_XTFMT ")", wimp_colour_value);
    if(0 != (0x10 & wimp_colour_value))
    {
        RISCOS_PALETTE_U os_rgb;
        os_rgb_from_wimp_colour_value(&os_rgb, wimp_colour_value);
        void_WrapOsErrorReporting(
            colourtrans_SetGCOL(os_rgb.word,
                                0x100 /* allow ECF */ | 0x80 /*background*/,
                                0 /*GCol action is store*/));
    }
    else
    {
        void_WrapOsErrorReporting(
            wimp_setcolour(wimp_colour_value | 0x80 /*background*/));
    }
}

extern void
riscos_set_fg_colour_from_wimp_colour_value(
    _InVal_     U32 wimp_colour_value)
{
    if(wimp_colour_value == current_fg_wimp_colour_value)
        return;

    current_fg_wimp_colour_value = wimp_colour_value;
    font_colours_invalid = TRUE;

    trace_1(TRACE_SETCOLOUR, "wimp_setcolour(fg " U32_XTFMT ")", wimp_colour_value);
    if(0 != (0x10 & wimp_colour_value))
    {
        RISCOS_PALETTE_U os_rgb;
        os_rgb_from_wimp_colour_value(&os_rgb, wimp_colour_value);
        void_WrapOsErrorReporting(
            colourtrans_SetGCOL(os_rgb.word,
                                0x100 /* allow ECF */ /* | 0x00 foreground*/,
                                0 /*GCol action is store*/));
    }
    else
    {
        void_WrapOsErrorReporting(
            wimp_setcolour(wimp_colour_value));
    }
}

/******************************************************************************
*
* swap fg/bg colours
*
******************************************************************************/

extern void
riscos_invert(void)
{
    const U32 new_bg_wimp_colour_value = current_fg_wimp_colour_value;
    const U32 new_fg_wimp_colour_value = current_bg_wimp_colour_value;

    riscos_set_bg_colour_from_wimp_colour_value(new_bg_wimp_colour_value);
    riscos_set_fg_colour_from_wimp_colour_value(new_fg_wimp_colour_value);
}

extern void
new_font_leading(
    GR_MILLIPOINT new_font_leading_mp)
{
    global_font_leading_millipoints = new_font_leading_mp;

    if(riscos_fonts)
    {
        /* line spacing - round up to nearest OS unit */
        charallocheight = idiv_ceil(global_font_leading_millipoints, millipoints_per_os_y);

        /* and then to nearest pixel corresponding */
        charallocheight = idiv_ceil(charallocheight, dy) * dy;

        /* lines are at least two pixels high */
        charallocheight = MAX(charallocheight, 2*dy);

        /* SKS after PD 4.11 06jan92 - only set page length if in auto mode */
        if(auto_line_height)
        {
            S32 paper_size_millipoints;
            /* actual paper size */
            paper_size_millipoints = (d_print_QL == 'L')
                                        ? paper_width_millipoints
                                        : paper_length_millipoints;

            /* effective paper size including scaling (bigger scale -> less lines + v.v.) */
            paper_size_millipoints = muldiv64(paper_size_millipoints, 100, d_print_QS);

            /* page length (round down) */
            d_poptions_PL = paper_size_millipoints / global_font_leading_millipoints;
        }
    }
    else
    {
        /* ignore font leading for system fonts on screen */

        /* leave page length alone too */

        charallocheight = 32;
    }

    update_variables();
}

/*
RJM and RCM add on 6.10.91
*/

extern void
new_font_leading_based_on_y(void)
{
    new_font_leading((S32) ((global_font_y * 1.2 * 1000.0 / 16.0) + 0.5));
}

/******************************************************************************
*
* recache variables after a mode change
*
******************************************************************************/

static BOOL first_open = TRUE;

extern void
cache_mode_variables(void)
{
    GDI_SIZE os_display_size;

    /* cache some variables so Draw rendering works */
    wimpt_checkmode();

    os_display_size.cx = wimptx_display_size_cx();
    os_display_size.cy = wimptx_display_size_cy();

    if(os_display_size.cy != g_os_display_size.cy)
        /* ensure window default pos reset to sensible for mode again */
        riscos_setinitwindowpos(os_display_size.cy - wimptx_title_height());

    g_os_display_size = os_display_size;

    default_gcharxsize  = bbc_vduvar(bbc_GCharSizeX)  << wimptx_XEigFactor();
    default_gcharysize  = bbc_vduvar(bbc_GCharSizeY)  << wimptx_YEigFactor();
    default_gcharxspace = bbc_vduvar(bbc_GCharSpaceX) << wimptx_XEigFactor();
    default_gcharyspace = bbc_vduvar(bbc_GCharSpaceY) << wimptx_YEigFactor();
    trace_4(TRACE_APP_PD4_RENDER, "default system font %d %d %d %d",
            default_gcharxsize, default_gcharysize, default_gcharxspace, default_gcharyspace);

    log2bpp = bbc_vduvar(bbc_Log2BPP);

    /* OS units per pixel */
    dx      = wimpt_dx();
    dxm1    = dx - 1;

    dy      = wimpt_dy();
    dym1    = dy - 1;
    chmdy   = charheight - dy;

    trace_4(TRACE_APP_PD4_RENDER, "dx = %d, dy = %d, x_size = %d, y_size = %d", dx, dy, os_display_size.cx, os_display_size.cy);

    max_poss_os_width  = os_display_size.cx - leftline_width - wimptx_vscroll_width();
    max_poss_os_height = os_display_size.cy - wimptx_title_height() - wimptx_hscroll_height() /* has hscroll not bottomline */;
    trace_2(TRACE_APP_PD4_RENDER, "max poss height = %d (os), max poss width = %d",
                    max_poss_os_height, max_poss_os_width);

    font_readscalefactor(&millipoints_per_os_x, &millipoints_per_os_y);
    trace_2(TRACE_APP_PD4_RENDER, "font scale factor x = %d, y = %d", millipoints_per_os_x, millipoints_per_os_y);

    /* just in case this needs bpp set etc. and for colour changes between modes */
    cache_palette_variables();

    { /* loop over all documents */
    DOCNO old_docno = current_docno();
    P_DOCU p_docu;

    /* new height etc. first phase */
    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        select_document(p_docu);

        new_grid_state();

        colh_position_icons();  /* mainly because the cell coordinates box is made */
                                /* up of two icons, one dx,dy inside the other     */

        (void) new_main_window_height(main_window_height());
    }

    /* do caret stuff in a second phase */
    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        select_document(p_docu);

        /* may need to set caret up again on mode change (256 colour modes) */
        xf_caretreposition = TRUE;

        if(main_window_handle == caret_window_handle)
            draw_caret();
    }

    select_document_using_docno(old_docno);
    } /*block*/
}

/******************************************************************************
*
* recache variables after a palette change
*
******************************************************************************/

extern void
cache_palette_variables(void)
{
    wimptx_checkpalette();

#if CHECKING || 0
    {
    int i;
    for(i = 0; i < sizeof(wimptx__palette.c)/sizeof(wimptx__palette.c[0]); ++i)
        reportf("palette entry %2d = &%8.8X", i, wimptx__palette.c[i].word);
    } /*block*/
#endif
}

/* merely set the vspace/vrubout variables */
/* does NOT force redraw or set new window height */

extern void
new_grid_state(void)
{
    charvspace = charallocheight;

    /* grid is dy thick with one v_dy line above it and below it (at top of char) */
    /* SKS 31.10.91 added !riscos_fonts */
    if(grid_on && !riscos_fonts)
        charvspace += 2*v_dy + dy;

    /* system font is charheight high, but is plotted from dy below */
    vdu5textoffset = charheight - dy;

    if(grid_on)
        vdu5textoffset += dy + v_dy;

    /*fontbaselineoffset = charallocheight / 4;*/
    /* global_font_y is 1/16 of a point, we want 1/4 of it in os units! */
    fontbaselineoffset = (global_font_y * 1000) / (16 * MILLIPOINTS_PER_OS * 4);

    /* SKS/MRJC 13/11/91 - don't allow fontscreenheightlimit_mp to get negative */
    if(fontbaselineoffset > charvspace)
        fontbaselineoffset = 0;

#if 1
    /* SKS says why didn't we use this to start with? */
    fontscreenheightlimit_millipoints = global_font_leading_millipoints;
#else
    /* 15/12 is pragmatic fix to get 12pt still without use of grid */
    fontscreenheightlimit_millipoints = (charvspace - fontbaselineoffset) * (15*MILLIPOINTS_PER_OS/12);
#endif

    charvrubout_pos = charvspace - (vdu5textoffset + dy);

    charvrubout_neg = vdu5textoffset;

    /* don't rubout grid line! */
    if(grid_on)
        charvrubout_neg -= dy;

    trace_5(TRACE_APP_PD4_RENDER, "charvspace = %d, vdu5textoffset = %d, fontbaselineoffset = %d, charvrubout_pos = %d, charvrubout_neg = %d",
            charvspace, vdu5textoffset, fontbaselineoffset, charvrubout_pos, charvrubout_neg);
}

/******************************************************************************
*
*  how many whole text cells fit in the window
*
******************************************************************************/

_Check_return_
extern tcoord
main_window_height(void)
{
    WimpGetWindowStateBlock window_state;
    const S32 min_height = calrad(3); /* else rebols & friends explode */
    tcoord height;
    BOOL ok;

    ok = (main_window_handle != HOST_WND_NONE);

    if(ok)
    {
        window_state.window_handle = main_window_handle;
        ok = (NULL == tbl_wimp_get_window_state(&window_state));
    }

    if(ok)
    {
        S32 os_height = BBox_height(&window_state.visible_area) - topslop;
        height = os_height / charvspace;
        unused_bit_at_bottom = ((height * charvspace) != os_height);
        height = MAX(height, min_height);

        trace_3(TRACE_APP_PD4_RENDER, "main_window_height is %d os %d text, ubb = %s",
            os_height, height, report_boolstring(unused_bit_at_bottom));
    }
    else
    {
        height = min_height;
        unused_bit_at_bottom = FALSE;

        trace_2(TRACE_APP_PD4_RENDER, "main_window_height(!OK) is xxx os %d text, ubb = %s",
            height, report_boolstring(unused_bit_at_bottom));
    }

    return(height);
}

_Check_return_
extern tcoord
main_window_width(void)
{
    WimpGetWindowStateBlock window_state;
    tcoord width;
    BOOL ok;

    ok = (main_window_handle != HOST_WND_NONE);

    if(ok)
    {
        window_state.window_handle = main_window_handle;
        ok = (NULL == tbl_wimp_get_window_state(&window_state));
    }

    if(ok)
    {
        GDI_COORD os_width = BBox_width(&window_state.visible_area) - leftslop;
        width = os_width / charwidth;

        trace_1(TRACE_APP_PD4_RENDER, "main_window_width is %d", width);
    }
    else
    {
        width = BORDERWIDTH + 4;

        trace_1(TRACE_APP_PD4_RENDER, "main_window_width(!OK) is %d", width);
    }

    return(width);
}

/******************************************************************************
*
* position graphics cursor for text output
*
******************************************************************************/

extern void
at(
    tcoord tx,
    tcoord ty)
{
    trace_2(TRACE_DRAW, "at(%d, %d)", tx, ty);

    wimpt_safe(bbc_move(gcoord_x(tx), gcoord_y_textout(ty)));
}

extern void
at_fonts(
    coord x,
    tcoord ty)
{
    if(riscos_fonts)
    {
        trace_3(TRACE_DRAW, "at_fonts(%d (%d), %d)", x, textcell_xorg + x, ty);

        wimpt_safe(bbc_move(textcell_xorg + x, gcoord_y_textout(ty)));
    }
    else
        at(x, ty);
}

/******************************************************************************
*
*  clear out specified text area to bg
*  graphics cursor spastic at end
*
******************************************************************************/

extern void
clear_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    BOOL zap_grid)
{
    GDI_COORD x0, y0, x1, y1;

    trace_4(TRACE_APP_PD4_RENDER, "clear_textarea(%d, %d, %d, %d)", tx0, ty0, tx1, ty1);

    if((tx0 != tx1)  &&  ((ty0 != ty1) || zap_grid))
    {
        x0 = gcoord_x(tx0);
        y0 = gcoord_y(ty0);
        x1 = gcoord_x(tx1) - dx;
        y1 = gcoord_y(ty1) - (zap_grid ? 0 : dy);

        /* limit coordinates for RISC OS */
        x0 = MAX(x0, SHRT_MIN);
        y0 = MAX(y0, SHRT_MIN);
        x1 = MIN(x1, SHRT_MAX);
        y1 = MIN(y1, SHRT_MAX);

        wimpt_safe(bbc_move(x0, y0));
        wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawAbsBack, x1, y1));
    }
}

/******************************************************************************
*
*  clear out thisarea (text) to bg
*  graphics cursor spastic at end
*
******************************************************************************/

extern void
clear_thistextarea(void)
{
    clear_textarea(thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1, FALSE);
}

/******************************************************************************
*
* clear the underlay for a subsequent string print
* graphics cursor restored to current text cell
*
******************************************************************************/

extern void
clear_underlay(
    tcoord len)
{
    GDI_COORD x, y_pos, y_neg;

    trace_1(TRACE_DRAW, "clear_underlay(%d)", len);

    if(len <= 0)
        return;

    x = cw_to_os(len) - dx;
    y_pos = charvrubout_pos;
    y_neg = charvrubout_neg;

    if(y_pos)
        wimpt_safe(os_plot(bbc_MoveCursorRel,               0, +y_pos));

    wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -y_pos -y_neg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,                  -x,        +y_neg));
}

/******************************************************************************
*
* conversions from text cell coordinates to
* absolute graphics coordinates for output
*
* returns bottom left corner of text cell
*
* NB. text output requires a further correction
*  of +charfudgey as VDU 5 plotting is spastic
*
******************************************************************************/

_Check_return_
extern gcoord
gcoord_x(
    tcoord x)
{
    return(textcell_xorg + cw_to_os(x));
}

#define gc_y(y) (textcell_yorg - (y+1) * charvspace)

_Check_return_
extern gcoord
gcoord_y(
    tcoord y)
{
    return(gc_y(y));
}

#ifdef UNUSED

_Check_return_
extern gcoord
gcoord_y_fontout(
    tcoord y)
{
    return(gc_y(y) + fontbaselineoffset);
}

#endif /* UNUSED */

_Check_return_
extern gcoord
gcoord_y_textout(
    tcoord y)
{
    return(gc_y(y) + vdu5textoffset);
}

/******************************************************************************
*
*  print n spaces
*
******************************************************************************/

extern void
ospca(
    S32 nspaces)
{
    if(nspaces > 0)
    {
        if(sqobit)
        {
            if(riscos_printing)
                riscos_movespaces(nspaces);
            else
                wrchrep(SPACE, nspaces);
            return;
        }

        riscos_printspaces(nspaces);
    }
}

/* needn't worry about printing */

extern void
ospca_fonts(
    S32 nspaces)
{
    (riscos_fonts ? riscos_printspaces_fonts : ospca) (nspaces);
}

/******************************************************************************
*
* request that an area of text cells be cleared to bg
*
******************************************************************************/

static void
redraw_clear_area(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block)
{
    UNREFERENCED_PARAMETER_InRef_(redraw_window_block);

    trace_4(TRACE_APP_PD4_RENDER, "redraw_cleararea: graphics window %d, %d, %d, %d",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1);

    set_bg_colour_from_option(COI_BACK);
    wimpt_safe(bbc_vdu(bbc_ClearGraph));
}

extern void
please_clear_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_clear_textarea(%d, %d, %d, %d)", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_clear_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    /* no text printing so don't need scaling wrapper */
    riscos_updatearea(  redraw_clear_area,
                        main_window_handle,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

/******************************************************************************
*
* request that an area of text cells be inverted
*
* NB can't invert in 256 or more colour modes
*
******************************************************************************/

static void
redraw_invert_area(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block)
{
    const U32 invert_fg_wimp_colour_index = wimp_colour_index_from_option(invert_fg_colours_option_index); /* index will be valid for bpp < 8 */
    const U32 invert_bg_wimp_colour_index = wimp_colour_index_from_option(invert_bg_colours_option_index);
    const int invertEORcolour =
        wimptx_GCOL_for_wimpcolour(invert_fg_wimp_colour_index) ^
        wimptx_GCOL_for_wimpcolour(invert_bg_wimp_colour_index);

    UNREFERENCED_PARAMETER_InRef_(redraw_window_block);

    assert(wimpt_bpp() < 8);

    trace_5(TRACE_APP_PD4_RENDER, "redraw_invert_area: graphics window (%d, %d, %d, %d), EOR %d",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1, invertEORcolour);

    wimpt_safe(bbc_gcol(3, 0x80 | invertEORcolour));
    wimpt_safe(bbc_vdu(bbc_ClearGraph));
}

static void
please_invert_textarea(
    _InVal_     tcoord tx0,
    _InVal_     tcoord ty0,
    _InVal_     tcoord tx1,
    _InVal_     tcoord ty1,
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index)
{
    trace_6(TRACE_APP_PD4_RENDER, "please_invert_textarea(%d, %d, %d, %d) fg = %d, bg = %d",
                tx0, ty0, tx1, ty1, (int) fg_colours_option_index, (int) bg_colours_option_index);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_invert_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    invert_fg_colours_option_index = fg_colours_option_index;
    invert_bg_colours_option_index = bg_colours_option_index;

    /* no text printing so don't need scaling wrapper */
    riscos_updatearea(  redraw_invert_area,
                        main_window_handle,
                        texttooffset_x(tx0),
                        /* invert grid hspace, not hbar */
                        texttooffset_y(ty0) + ((grid_on) ? dy : 0),
                        /* don't invert grid vbar */
                        texttooffset_x(tx1) - ((grid_on) ? dx : 0),
                        texttooffset_y(ty1));
}

extern void
please_invert_number_cell(
    _InVal_     S32 coff,
    _InVal_     S32 roff,
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index)
{
    tcoord tx0 = calcad(coff);
    tcoord tx1 = tx0 + colwidth(col_number(coff));
    tcoord ty0 = calrad(roff);
    tcoord ty1 = ty0 - 1;

    please_invert_textarea(tx0, ty0, tx1, ty1, fg_colours_option_index, bg_colours_option_index);
}

/* invert a set of cells, taking care with the grid */

extern void
please_invert_number_cells(
    _InVal_     S32 start_coff,
    _InVal_     S32 end_coff,
    _InVal_     S32 roff,
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index)
{
    S32 coff;

    if(grid_on)
    {
        for(coff = start_coff; coff <= end_coff; ++coff)
            please_invert_number_cell(coff, roff, fg_colours_option_index, bg_colours_option_index);
    }
    else
    {   /* accumulate all columns into one textarea */
        const tcoord tx0 = calcad(start_coff);
              tcoord tx1 = tx0;
        const tcoord ty0 = calrad(roff);
        const tcoord ty1 = ty0 - 1;

        for(coff = start_coff; coff <= end_coff; ++coff)
            tx1 += colwidth(col_number(coff));

        please_invert_textarea(tx0, ty0, tx1, ty1, fg_colours_option_index, bg_colours_option_index);
    }
}

/******************************************************************************
*
* request that an area of text cells be redrawn by a given procedure
*
******************************************************************************/

static RISCOS_REDRAWPROC updatearea_proc;

static void
updatearea_wrapper(
    _In_        const RISCOS_RedrawWindowBlock * const redraw_window_block)
{
    updatearea_proc(redraw_window_block);
}

extern void
please_update_textarea(
    RISCOS_REDRAWPROC proc,
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_update_textarea(%d, %d, %d, %d)", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_update_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    updatearea_proc = proc;

    riscos_updatearea(  updatearea_wrapper,
                        main_window_handle,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

extern void
please_update_window(
    RISCOS_REDRAWPROC proc,
    _HwndRef_   HOST_WND window_handle,
    S32 gx0,
    S32 gy0,
    S32 gx1,
    S32 gy1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_update_window(%d, %d, %d, %d)", gx0, gy0, gx1, gy1);

    updatearea_proc = proc;

    riscos_updatearea(updatearea_wrapper, window_handle, gx0, gy0, gx1, gy1);
}

/******************************************************************************
*
* request that an area of text cells be redrawn
*
******************************************************************************/

extern void
please_redraw_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_redraw_textarea(%d, %d, %d, %d)", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_redraw_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    updatearea_proc = application_redraw_core;

    riscos_updatearea(  updatearea_wrapper,
                        main_window_handle,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

extern void
please_redraw_textline(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1)
{
    please_redraw_textarea(tx0, ty0, tx1, ty0 - 1);
}

extern void
please_redraw_entire_window(void)
{
    /* must draw left & top slops too */
    please_redraw_textarea(-1, paghyt + 2, RHS_X, -1);
}

/******************************************************************************
*
* set a graphics window to cover the intersection
* of the passed graphics window and the desired window
*
* returns TRUE if some part of window visible
*
******************************************************************************/

extern BOOL
set_graphics_window_from_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    BOOL set_gw)
{
    GDI_BOX clipper;

    clipper.x0 = gcoord_x(tx0);
    clipper.y0 = gcoord_y(ty0);
    clipper.x1 = gcoord_x(tx1);
    clipper.y1 = gcoord_y(ty1);

    trace_4(TRACE_APP_PD4_RENDER, "set_gw_from_textarea(%d, %d, %d, %d)",
            tx0, ty0, tx1, ty1);
    trace_4(TRACE_APP_PD4_RENDER, "textarea  (%d, %d, %d, %d) (OS)",
            clipper.x0, clipper.y0, clipper.x1, clipper.y1);
    trace_4(TRACE_APP_PD4_RENDER, "window gw (%d, %d, %d, %d) (OS)",
            graphics_window.x0, graphics_window.y0, graphics_window.x1, graphics_window.y1);

    clipper.x0 = MAX(clipper.x0, graphics_window.x0);
    clipper.y0 = MAX(clipper.y0, graphics_window.y0);
    clipper.x1 = MIN(clipper.x1, graphics_window.x1);
    clipper.y1 = MIN(clipper.y1, graphics_window.y1);

    trace_4(TRACE_APP_PD4_RENDER, "intersection (%d, %d, %d, %d) (OS)", clipper.x0, clipper.y0, clipper.x1, clipper.y1);

    if((clipper.x0 >= clipper.x1)  ||  (clipper.y0 >= clipper.y1))
    {
        trace_0(TRACE_APP_PD4_RENDER, "zero size window requested - OS incapable");
        return(FALSE);
    }

    if(set_gw)
    {
        /* limit coordinates for RISC OS */
        clipper.x0 = MAX(clipper.x0, SHRT_MIN);
        clipper.y0 = MAX(clipper.y0, SHRT_MIN);
        clipper.x1 = MIN(clipper.x1, SHRT_MAX);
        clipper.y1 = MIN(clipper.y1, SHRT_MAX);

        /* take a copy to restore later */
        saved_graphics_window = graphics_window;

        /* set up so that clip routines can make further use of it */
        graphics_window = clipper;

        trace_4(TRACE_APP_PD4_RENDER, "setting gw (%d, %d, %d, %d) (OS)", clipper.x0, clipper.y0, clipper.x1, clipper.y1);

        /* when setting graphics window, all points are inclusive */
        wimpt_safe(riscos_vdu_define_graphics_window(clipper.x0,      clipper.y0,
                                                     clipper.x1 - dx, clipper.y1 - dy));
    }

    return(TRUE);
}

extern void
restore_graphics_window(void)
{
    /* restore from saved copy  */
    graphics_window = saved_graphics_window;

    wimpt_safe(riscos_vdu_define_graphics_window(graphics_window.x0,      graphics_window.y0,
                                                 graphics_window.x1 - dx, graphics_window.y1 - dy));
}

/******************************************************************************
*
* scroll the given area down by n lines as if it were a real text window
*
******************************************************************************/

extern void
scroll_textarea(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1,
    S32 nlines)
{
    gcoord sxmin;
    gcoord symin;
    gcoord sxmax;
    gcoord symax;
    S32 dy, uy0, uy1, ht;

    trace_5(0, "scroll_textarea((%d, %d, %d, %d), %d)", tx0, ty0, tx1, ty1, nlines);

    myassert5x((tx0 <= tx1)  &&  (ty0 >= ty1), "scroll_textarea((%d, %d, %d, %d), %d) is stupid", tx0, ty0, tx1, ty1, nlines);

    if(nlines != 0)
    {
        sxmin = texttooffset_x(tx0);
        sxmax = texttooffset_x(tx1+1);
        ht     = nlines * charvspace;

        if(ht > 0)
        {
            /* scrolling area down, clear top line(s) */
            symin = texttooffset_y(ty0) + ht;
            symax = texttooffset_y(ty1-1);

            dy  = symin - ht;
            uy0 = symax - ht;
            uy1 = symax;
        }
        else
        {
            /* scrolling area up, clear bottom line(s) */
            symin = texttooffset_y(ty0);
            symax = texttooffset_y(ty1-1) + ht;

            dy  = symin - ht;
            uy0 = symin;
            uy1 = dy;
        }

        riscos_caret_hide();

        trace_6(TRACE_APP_PD4_RENDER, "wimp_blockcopy((%d, %d, %d, %d), %d, %d)",
                            sxmin, symin, sxmax, symax, sxmin, dy);

        void_WrapOsErrorReporting(
            tbl_wimp_block_copy(main_window_handle, sxmin, symin, sxmax, symax, sxmin /*dx*/, dy));

        /* get our clear lines routine called - again no text drawn */
        riscos_updatearea(  redraw_clear_area,
                            main_window_handle,
                            sxmin, uy0,
                            sxmax, uy1);

        riscos_caret_restore();
    }
}

/******************************************************************************
*
* conversions from graphics coordinates in main window
* to text cell coordinates for input
* (and their friends for calculating cliparea)
*
* returns bottom left corner of text cell
*
******************************************************************************/

_Check_return_
extern tcoord
tsize_x(
    gcoord os_x)
{
    return(idiv_ceil(os_x, charwidth));
}

_Check_return_
extern tcoord
tsize_y(
    gcoord os_y)
{
    return(idiv_ceil(os_y, charvspace));
}

_Check_return_
extern tcoord
tcoord_x(
    gcoord os_x)
{
    return(idiv_floor_fn(os_x - textcell_xorg, charwidth));
}

_Check_return_
extern tcoord
tcoord_y(
    gcoord os_y)
{
    return(idiv_ceil_fn(textcell_yorg - os_y, charvspace) - 1);
}

_Check_return_
extern tcoord
tcoord_x_remainder(
    gcoord os_x)
{
    tcoord tx = tcoord_x(os_x);

    return((os_x - textcell_xorg) - cw_to_os(tx));
}

_Check_return_
extern tcoord
tcoord_x1(
    gcoord os_x)
{
    return(idiv_ceil_fn(os_x - textcell_xorg, charwidth));
}

_Check_return_
extern tcoord
tcoord_y1(
    gcoord os_y)
{
    return(idiv_floor_fn(textcell_yorg - os_y, charvspace) - 1);
}

/******************************************************************************
*
* work out whether an text object intersects the cliparea
* if it does, stick the given coordinates in thisarea
*
******************************************************************************/

extern BOOL
textobjectintersects(
    tcoord tx0,
    tcoord ty0,
    tcoord tx1,
    tcoord ty1)
{
    BOOL intersects = !((cliparea.x0 >=         tx1)  ||
                        (        ty1 >= cliparea.y0)  ||
                        (        tx0 >= cliparea.x1)  ||
                        (cliparea.y1 >=         ty0)  );

    trace_4(TRACE_CLIP, "textobjectintersects: %d %d %d %d (if any true, fails)",
             tx0, ty0, tx1, ty1);
    trace_1(TRACE_CLIP, "tx0 >= cliparea.x1 %s, ", report_boolstring(tx0 >= cliparea.x1));
    trace_1(TRACE_CLIP, "tx1 <= cliparea.x0 %s, ", report_boolstring(tx1 <= cliparea.x0));
    trace_1(TRACE_CLIP, "ty0 <= cliparea.y1 %s, ", report_boolstring(ty0 <= cliparea.y1));
    trace_1(TRACE_CLIP, "ty1 >= cliparea.y0 %s",   report_boolstring(ty1 >= cliparea.y0));

    if(intersects)
    {
        thisarea.x0 = tx0;
        thisarea.y0 = ty0;
        thisarea.x1 = tx1;
        thisarea.y1 = ty1;
    }

    return(intersects);
}

/******************************************************************************
*
* work out whether an text x pair intersects the cliparea
*
******************************************************************************/

extern BOOL
textxintersects(
    tcoord tx0,
    tcoord tx1)
{
    BOOL intersects = !((cliparea.x0 >=         tx1)    ||
                        (        tx0 >= cliparea.x1)    );

    trace_2(TRACE_CLIP, "textxintersects: %d %d (if any true, fails)",
             tx0, tx1);
    trace_1(TRACE_CLIP, "tx0 >= cliparea.x1 %s, ", report_boolstring(tx0 >= cliparea.x1));
    trace_1(TRACE_CLIP, "tx1 <= cliparea.x0 %s, ", report_boolstring(tx1 <= cliparea.x0));

    if(intersects)
    {
        thisarea.x0 = tx0;
        thisarea.x1 = tx1;
    }

    return(intersects);
}

/******************************************************************************
*
* return bottom left corner of text cell
* relative to the real work area extent origin
* useful for setting caret position etc.
*
******************************************************************************/

_Check_return_
extern gcoord
texttooffset_x(
    tcoord tx)
{
    return((/*curr_scroll_x*/ + leftslop) + cw_to_os(tx));
}

_Check_return_
extern gcoord
texttooffset_y(
    tcoord ty)
{
    return((/*curr_scroll_y*/ - topslop) - (ty+1) * charvspace);
}

/******************************************************************************
*
* move graphics cursor along by n spaces
* assumes we are currently text cell printing aligned
*
******************************************************************************/

extern void
riscos_movespaces(
    S32 nspaces)
{
    trace_1(TRACE_DRAW, "riscos_movespaces(%d)", nspaces);

    if(nspaces == 0) /* -ve allowed */
        return;

    wimpt_safe(os_plot(bbc_MoveCursorRel, cw_to_os(nspaces), 0));
}

extern void
riscos_movespaces_fonts(
    S32 nspaces)
{
    trace_1(TRACE_DRAW, "riscos_movespaces_fonts(%d)", nspaces);

    if(nspaces == 0) /* -ve allowed */
        return;

    wimpt_safe(os_plot(bbc_MoveCursorRel,
                       riscos_fonts ? nspaces : cw_to_os(nspaces), 0));
}

/******************************************************************************
*
* clear out n spaces to background
* assumes we are currently text cell printing aligned
*
* --out--
*   graphics cursor advanced to next text cell
*
******************************************************************************/

extern void
riscos_printspaces(
    S32 nspaces)
{
    S32 ldx, x, y_pos, y_neg;

    trace_1(TRACE_DRAW, "riscos_printspaces(%d)", nspaces);

    if(nspaces == 0) /* -ve allowed */
        return;

    ldx = dx;
    x = cw_to_os(nspaces) - ldx;
    y_pos = charvrubout_pos;
    y_neg = charvrubout_neg;

    if(y_pos)
        wimpt_safe(os_plot(bbc_MoveCursorRel,                0, +y_pos));

    wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -y_pos  -y_neg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,                  ldx,         +y_neg));
}

extern void
riscos_printspaces_fonts(
    S32 nspaces)
{
    GDI_COORD ldx, x, y_pos, y_neg;

    trace_1(TRACE_DRAW, "riscos_printspaces_fonts(%d)", nspaces);

    if(nspaces == 0) /* -ve allowed */
        return;

    ldx = dx;
    x = (riscos_fonts ? nspaces : cw_to_os(nspaces)) - ldx;
    y_pos = charvrubout_pos;
    y_neg = charvrubout_neg;

    if(y_pos)
        wimpt_safe(os_plot(bbc_MoveCursorRel,                0, +y_pos));

    wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -y_pos  -y_neg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,                  ldx,         +y_neg));
}

/******************************************************************************
*
* print character after explicitly filling background
* assumes we are currently text cell printing aligned
*
* --out--
*   graphics cursor advanced to next text cell
*
******************************************************************************/

extern void
riscos_printchar(
    S32 ch)
{
    const GDI_COORD x = charwidth - dx; /* avoids reload over procs */
    const GDI_COORD y_pos = charvrubout_pos;
    const GDI_COORD y_neg = charvrubout_neg;

    trace_1(TRACE_DRAW, "riscos_printchar(%d)", ch);

    if(y_pos)
        wimpt_safe(os_plot(bbc_MoveCursorRel,               0, +y_pos));

    wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -y_pos -y_neg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,                  -x,        +y_neg));

    wimpt_safe(bbc_vdu(ch));
}

/* ----------------------------------------------------------------------- */

/******************************************************************************
*
* scroll requests come through when the
* user clicks on the scroll icons or in the
* the scroll bar (not in the sausage).
*
******************************************************************************/

static BOOL caretposallowed = TRUE;

static void
application_repeat_command(
    S32 c,
    int count)
{
    while(count != 0)
    {
        --count;
        application_process_command(c);
    }
}

static void
process_Scroll_Request_xscroll(
    int xscroll)
{
    for(;;)
    {
        reportf(/*trace_1(TRACE_APP_PD4,*/ "app_Scroll_Request: xscroll %d", xscroll);

        switch(xscroll)
        {
        case +2:
            application_process_command(N_PageRight);
            return;

        case +3: /* AutoScroll */
        case +1:
            application_process_command(N_ScrollRight);
            return;

        case 0:
            return;

        case -1:
        case -3: /* AutoScroll */
            application_process_command(N_ScrollLeft);
            return;

        case -2:
            application_process_command(N_PageLeft);
            return;

        default:
            break;
        }

        /* expect multiples of ±4 for scroll wheel */
        if(xscroll >= 4)
        {
            xscroll -= 4;
            application_process_command(N_ScrollRight);
            return; /* left/right is very fast */
        }
        else if(xscroll <= -4)
        {
            xscroll += 4;
            application_process_command(N_ScrollLeft);
            return; /* left/right is very fast */
        }
    }
}

static void
process_Scroll_Request_yscroll(
    int yscroll)
{
    for(;;)
    {
        reportf(/*trace_1(TRACE_APP_PD4,*/ "app_Scroll_Request: yscroll %d", yscroll);

        switch(yscroll)
        {
        case +3: /* AutoScroll */
            application_repeat_command(N_ScrollUp, 3);
            return;

        case +2:
            application_process_command(N_PageUp);
            return;

        case +1:
            application_process_command(N_ScrollUp);
            return;

        case 0:
            return;

        case -1:
            application_process_command(N_ScrollDown);
            return;

        case -2:
            application_process_command(N_PageDown);
            return;

        case -3: /* AutoScroll */
            application_repeat_command(N_ScrollDown, 3);
            return;

        default:
            break;
        }

        /* expect multiples of ±4 for scroll wheel */
        if(yscroll >= 4)
        {
            yscroll -= 4;
            application_repeat_command(N_ScrollUp, 5);

        }
        else if(yscroll <= -4)
        {
            yscroll += 4;
            application_repeat_command(N_ScrollDown, 5);
        }
    }
}

extern void
application_Scroll_Request(
    _In_        const WimpScrollRequestEvent * const scroll_request)
{
    int xscroll = scroll_request->xscroll; /* -1 for left, +1 for right */
    int yscroll = scroll_request->yscroll; /* -1 for down, +1 for up    */
                                           /*     ±2 for page scroll    */
                                           /*     ±3 for AutoScroll     */
                                           /*     ±4 for scroll wheel   */

    trace_2(TRACE_APP_PD4, "app_Scroll_Request: xscroll %d yscroll %d", xscroll, yscroll);

    /* ensure draw_caret gets ignored if not current window */
    caretposallowed = (main_window_handle == caret_window_handle);

    process_Scroll_Request_yscroll(yscroll);

    process_Scroll_Request_xscroll(xscroll);

    /* and finally restore normality */
    caretposallowed = TRUE;
}

/******************************************************************************
*
* compute x scroll offset given current extent
*
******************************************************************************/

static S32
cols_for_extent(void)
{
    S32 res = (S32) numcol;
    COL tcol;

    for(tcol = 0; tcol < numcol; ++tcol)
        if(!colwidth(tcol))
            --res;

    return(res ? res : 1);
}

static GDI_COORD
compute_scroll_x(void)
{
    COL ffc    = fstncx();
    COL nfixes = n_colfixes;
    GDI_COORD scroll_x;

    if(nfixes)
    {
        assert(horzvec_entry_valid(0));
        if(ffc >= col_number(0))
            ffc -= nfixes;
    }

    scroll_x = pd_muldiv64(curr_extent_x, (S32) ffc, cols_for_extent()); /* +ve := +ve, +ve, +ve */

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    scroll_x = floor_using_mask(scroll_x, dxm1);

    trace_1(TRACE_APP_PD4, "computed scroll_x %d (OS)", scroll_x);
    return(scroll_x);
}

/******************************************************************************
*
* compute y scroll offset given current extent
*
******************************************************************************/

static S32
rows_for_extent(void)
{
    return((S32) numrow + 1);
}

static GDI_COORD
compute_scroll_y(void)
{
    ROW ffr    = fstnrx();
    ROW nfixes = n_rowfixes;
    GDI_COORD scroll_y;

    if(nfixes)
    {
        assert(vertvec_entry_valid(0));
        if(ffr >= row_number(0))
            ffr -= nfixes;
    }

    scroll_y = pd_muldiv64(-curr_extent_y, (S32) ffr, rows_for_extent()); /* +ve := -(-ve), +ve, +ve */

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    /* NB we choose to round scroll offsets to the lower multiple */
    scroll_y = floor_using_mask(scroll_y, dym1);

    scroll_y = -scroll_y; /* and now flip to -ve for Neil */

    trace_1(TRACE_APP_PD4, "computed scroll_y %d (OS)", scroll_y);
    return(scroll_y);
}

/******************************************************************************
*
* Open the various pane windows (main_dbox & colh_dbox) that cover rear_dbox.
*
******************************************************************************/

static void
openpane(
    const BBox * const boxp,
    _HwndRef_   HOST_WND behind)
{
    GDI_COORD borderline = 0; /* a negative number, relative to top edge of rear window */
    WimpOpenWindowBlock open_window_block;

    {
    WimpGetIconStateBlock icon_state;
    icon_state.window_handle = colh_window_handle;
    icon_state.icon_handle = COLH_COLUMN_HEADINGS;
    if(NULL == WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        borderline = icon_state.icon.bbox.ymin; /* approx -132 */
    } /*block*/

    /* open main window */

    open_window_block.window_handle = main_window_handle;
    open_window_block.visible_area = *boxp; /* if borders are off, main window completely covers rear window and colh window */
    if(displaying_borders)
        open_window_block.visible_area.ymax = open_window_block.visible_area.ymax + borderline; /* else main window abuts colh window and both cover rear window */
    open_window_block.xscroll = 0;
    open_window_block.yscroll = 0;
    open_window_block.behind = behind;

    void_WrapOsErrorReporting(tbl_wimp_open_window(&open_window_block));

    open_window_block.window_handle = colh_window_handle;
    open_window_block.visible_area = *boxp; /* pane completely covers rear window */
    open_window_block.visible_area.ymin = open_window_block.visible_area.ymax + borderline;
    open_window_block.xscroll = 0;
    open_window_block.yscroll = 0;
    open_window_block.behind = main_window_handle;

    void_WrapOsErrorReporting(tbl_wimp_open_window(&open_window_block));

    if(displaying_borders)
    {
        WimpGetIconStateBlock icon_state;
        WimpCreateIconBlock create;
        GDI_COORD os_x1;

        icon_state.window_handle = colh_window_handle;
        icon_state.icon_handle = COLH_CONTENTS_LINE;
        void_WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state));

        create.icon = icon_state.icon;

        trace_4(TRACE_APP_EXPEDIT, "Contents line icon currently (%d,%d, %d,%d)",
                                   create.icon.bbox.xmin, create.icon.bbox.ymin, create.icon.bbox.xmax, create.icon.bbox.ymax);

        os_x1 = BBox_width(&open_window_block.visible_area) - MAX(dx, dy); /*16;*/

        if(os_x1 < (create.icon.bbox.xmin + 16))
            os_x1 = create.icon.bbox.xmin + 16; /* perhaps unnecessary but I don't trust Neil */

        if(create.icon.bbox.xmax != os_x1)
        {
            /* icon size is wrong, so... */

            create.icon.bbox.xmax = os_x1;

            redefine_icon(colh_window_handle, COLH_CONTENTS_LINE, &create);
        }
    }
}

/******************************************************************************
*
* we are being asked to open our main window: if certain things are
* changed we must do all sorts of reshuffling
*
******************************************************************************/

extern void
application_Open_Window_Request(
    WimpOpenWindowRequestEvent * const open_window_request)
{
    S32 scroll_x = open_window_request->xscroll;
    S32 scroll_y = open_window_request->yscroll;
    HOST_WND behind = open_window_request->behind;
    BOOL old_unused_bit = unused_bit_at_bottom;
    /* note knowledge of height <-> pagvars conversion */
    tcoord  old_window_height   = paghyt + 1;
    tcoord  old_window_width    = pagwid_plus1;
    tcoord  window_height;
    tcoord  window_width;
    BOOL    smash = FALSE;
    S32     smash_y0;
    S32     size_change;

    trace_2(TRACE_APP_PD4, "\n\n*** app_open_request: w %d, behind %d", open_window_request->window_handle, behind);
    trace_6(TRACE_APP_PD4, "                : x0 %d, x1 %d;  y0 %d, y1 %d; scroll_x %d, scroll_y %d",
            open_window_request->visible_area.xmin, open_window_request->visible_area.xmax,
            open_window_request->visible_area.ymin, open_window_request->visible_area.ymax, scroll_x, scroll_y);

    /* have to tweak requested coordinates as we have a wierd extent */
    if( open_window_request->visible_area.xmax > open_window_request->visible_area.xmin + max_poss_os_width)
        open_window_request->visible_area.xmax = open_window_request->visible_area.xmin + max_poss_os_width;

    if( open_window_request->visible_area.ymin < open_window_request->visible_area.ymax - max_poss_os_height)
        open_window_request->visible_area.ymin = open_window_request->visible_area.ymax - max_poss_os_height;

    if(behind != (HOST_WND) -2)
    {
        /* on mode change, Neil tells us to open windows behind themselves! */
        if((behind == rear_window_handle) || (behind == colh_window_handle))
            behind = main_window_handle;

        /* always open pane first */
        openpane(&open_window_request->visible_area, behind);

        /* always open rear window behind pane colh window */
        open_window_request->behind = colh_window_handle;
    }

    void_WrapOsErrorReporting(tbl_wimp_open_window(open_window_request));

    if(first_open)
    {
        /* remember first successful open position, bump from there (nicer on < 1024 y OS unit screens) */
        first_open = FALSE;
        riscos_setnextwindowpos(open_window_request->visible_area.ymax); /* gets bumped */
    }

    /* reopen pane with corrected coords (NB widdles off end - flags word) */
    void_WrapOsErrorReporting(tbl_wimp_get_window_state((WimpGetWindowStateBlock *) open_window_request));

    /* keep track of where we opened it */
    open_box = * (PC_GDI_BOX) &open_window_request->visible_area;

    /* if the rear window was sent to the back,
     * open panes behind the window that
     * the rear window ended up behind
    */
    if(behind == (wimp_w) -2)
        behind = open_window_request->behind;

    openpane(&open_window_request->visible_area, behind);

    trace_6(TRACE_APP_PD4, "opened rear window at: x0 %d, x1 %d;  y0 %d, y1 %d; width %d (OS) height %d (OS)",
            open_window_request->visible_area.xmin, open_window_request->visible_area.xmax,
            open_window_request->visible_area.ymin, open_window_request->visible_area.ymax,
            BBox_width(&open_window_request->visible_area), BBox_height(&open_window_request->visible_area));

    {
    WimpGetWindowStateBlock window_state;
    window_state.window_handle = main_window_handle;
    if(NULL == WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
    {   /* note absolute coordinates of window (NOT work area extent) origin */
        setorigins(window_state.visible_area.xmin, window_state.visible_area.ymax);
    }
    } /*block*/

    window_height = main_window_height();
    window_width  = main_window_width();

    /* suss resize after opening as then we know how big we really are */
    size_change = 0;

    if(old_window_height != window_height)
    {
        (void) new_main_window_height(window_height);
        size_change = 1;
    }

    if(old_window_width != window_width)
    {
        (void) new_main_window_width(window_width);
        size_change = 1;
    }

    /* is someone trying to scroll through the document
     * by dragging the scroll bars?
     * SKS - forbid scrolling to have any effect here if open window changed size
    */
    if((scroll_x != curr_scroll_x) && !size_change)
    {
        /* work out which column to put at left */
        COL leftcol, o_leftcol, newcurcol;
        COL nfixes, delta;

        o_leftcol = fstncx();
        nfixes    = n_colfixes;

        trace_3(TRACE_APP_PD4, "horizontal scroll bar dragged: scroll_x %d, curr_scroll_x %d, curr_extent_x %d",
                scroll_x, curr_scroll_x, curr_extent_x);

        leftcol = (COL) pd_muldiv64(scroll_x, cols_for_extent(), curr_extent_x);

        if(nfixes)
        {
            assert(horzvec_entry_valid(0));
            if(leftcol >= col_number(0))
                leftcol += nfixes;
        }

        if( leftcol > numcol - 1)
            leftcol = numcol - 1;

        trace_1(TRACE_APP_PD4, "put col %d at left of scrolling area", leftcol);

        if(leftcol != o_leftcol)
        {
            /* window motion may reposition caret */
            (void) mergebuf();

            if(curcoloffset < nfixes)
                /* caret is in fixed section - do not move */
                delta = curcol - leftcol;
            else
            {
                /* keep caret at same/similar offset to right of fixed section */
                delta = curcol - o_leftcol;

                if( delta > numcol - 1 - leftcol)
                    delta = numcol - 1 - leftcol;
            }

            newcurcol = leftcol + delta;

            filhorz(leftcol, newcurcol);

            if(curcol != newcurcol)
                chknlr(newcurcol, currow);

            out_screen = TRUE;

            /* may need caret motion */
            if(main_window_handle == caret_window_handle)
                xf_acquirecaret = TRUE;
        }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scroll_x    = scroll_x;
        delta_scroll_x   = scroll_x - compute_scroll_x();
        trace_1(TRACE_APP_PD4, "delta_scroll_x := %d", delta_scroll_x);
    }

    if((scroll_y != curr_scroll_y) && !size_change)
    {
        /* work out which row to put at top */
        ROW toprow, o_toprow, newcurrow;
        ROW nfixes, delta;

        o_toprow = fstnrx();
        nfixes   = n_rowfixes;

        trace_3(TRACE_APP_PD4, "vertical scroll bar dragged: scroll_y %d, curr_scroll_y %d, curr_extent_y %d",
                scroll_y, curr_scroll_y, curr_extent_y);

        toprow = (ROW) pd_muldiv64(rows_for_extent(), scroll_y, curr_extent_y); /* +ve, -ve, -ve */

        if(nfixes)
        {
            assert(vertvec_entry_valid(0));
            if(toprow >= row_number(0))
                toprow += nfixes;
        }

        trace_3(TRACE_APP_PD4, "row %d is my first guess, numrow %d, rows_available %d", toprow, numrow, rows_available);

        /* e.g. toprow = 42, numrow = 72, rows_available = 30 triggers fudge of 6 */

        if(!nfixes  &&  (toprow >= (numrow - 1) - ((ROW) rows_available - 1))  &&  encpln)
        {
            trace_0(TRACE_APP_PD4, "put last row at bottom of scrolling area");

            toprow += (ROW) rows_available / ((ROW) encpln + 1);
        }

        trace_1(TRACE_APP_PD4, "put row %d at top of scrolling area", toprow);

        if( toprow > numrow - 1)
            toprow = numrow - 1;

        if(toprow != o_toprow)
        {
            /* window motion may reposition caret */
            (void) mergebuf();

            if(currowoffset < nfixes)
                /* caret is in fixed section - do not move */
                delta = currow - toprow;
            else
            {
                /* keep caret at same/similar offset below fixed section */
                delta = currow - o_toprow;

                if( delta > numrow - 1 - toprow)
                    delta = numrow - 1 - toprow;
            }

            newcurrow = toprow + delta;

            filvert(toprow, newcurrow, TRUE);

            if(newcurrow != currow)
                chknlr(curcol, newcurrow);

            out_below = TRUE;
            rowtoend = schrsc(fstnrx());

            /* may need caret motion */
            if(main_window_handle == caret_window_handle)
                xf_acquirecaret = TRUE;
        }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scroll_y    = scroll_y;
        delta_scroll_y   = scroll_y - compute_scroll_y();
        trace_1(TRACE_APP_PD4, "delta_scroll_y := %d", delta_scroll_y);
    }

    if(window_height != old_window_height)
    {
        /* window motion may reposition caret */
        (void) mergebuf();

        if(window_height > old_window_height)
        {
            /* Window Manager assumes when making window bigger that
             * we had been clipping to the window itself and not
             * just some subwindow like we do, so add the old unused
             * rectangle to its list that it'll give us next.
            */
            if(old_unused_bit)
            {
                smash    = TRUE;
                smash_y0 = old_window_height;
            }
        }
        else if(unused_bit_at_bottom)
        {
            /* When making window smaller it assumes that
             * we will clip to the window itself and not
             * just some subwindow like we do, so add the new unused
             * rectangle to its list that it'll give us next.
            */
            smash    = TRUE;
            smash_y0 = window_height;
        }

        if(smash)
        {
            BBox redraw_area;
            redraw_area.xmin = texttooffset_x(-1);              /* lhs slop too */
            redraw_area.xmax = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            redraw_area.ymin = texttooffset_y(smash_y0);
            redraw_area.ymax = redraw_area.ymin + charvspace;
            trace_5(TRACE_APP_PD4, "calling wimp_force_redraw(%d; %d, %d, %d, %d)",
                        main_window_handle, redraw_area.xmin, redraw_area.ymin, redraw_area.xmax, redraw_area.ymax);
            wimpt_safe(tbl_wimp_force_redraw(main_window_handle, redraw_area.xmin, redraw_area.ymin, redraw_area.xmax, redraw_area.ymax));
        }
    }

    if(window_width != old_window_width)
    {
        /* window motion may reposition caret */
        (void) mergebuf();

        if(window_width > old_window_width)
        {
            BBox redraw_area;
            redraw_area.xmin = texttooffset_x(old_window_width);
            redraw_area.xmax = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            redraw_area.ymin = texttooffset_y(-1);              /* top slop too */
            redraw_area.ymax = texttooffset_y(window_height);
            trace_5(TRACE_APP_PD4, "calling wimp_force_redraw(%d; %d, %d, %d, %d)",
                        main_window_handle, redraw_area.xmin, redraw_area.ymin, redraw_area.xmax, redraw_area.ymax);
            wimpt_safe(tbl_wimp_force_redraw(main_window_handle, redraw_area.xmin, redraw_area.ymin, redraw_area.xmax, redraw_area.ymax));
        }
    }

    draw_screen();          /* which does draw_altered_state() */

    /* if newly opened, might need to claim caret on delayed open event */
    if(xf_acquirecaret)
    {
        xf_acquirecaret = FALSE;
        xf_caretreposition = TRUE;
        draw_caret();
    }
}

/* suss what RISC OS specific stuff has changed since last draw_screen
 * and take appropriate action
*/

extern BOOL
draw_altered_state(void)
{
    WimpGetWindowStateBlock window_state;
    S32 os_width, os_height;
    S32 extent_x, extent_y;
    S32 scroll_x, scroll_y;

    /* read current rear window size etc. */
    window_state.window_handle = rear_window_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
        return(FALSE);
    trace_5(TRACE_APP_PD4, "\n\n\n*** draw_altered_state(): get_wind_state(" UINTPTR_XTFMT ") returns %d, %d, %d, %d;",
                (uintptr_t) window_state.window_handle,
                window_state.visible_area.xmin, window_state.visible_area.ymin,
                window_state.visible_area.xmax, window_state.visible_area.ymax);
    trace_2(TRACE_APP_PD4, " scroll_x %d, scroll_y %d", window_state.xscroll, window_state.yscroll);

    os_width  = BBox_width( &window_state.visible_area);
    os_height = BBox_height(&window_state.visible_area);
    trace_2(TRACE_APP_PD4, " width %d (OS), height %d (OS)", os_width, os_height);

    /* recompute extents (both +ve for now) */
    extent_x = pd_muldiv64(os_width,  (S32) numcol, colsonscreen ? colsonscreen : 1);
    extent_y = pd_muldiv64(os_height, (S32) numrow, rowsonscreen);

    if( extent_x < max_poss_os_width)
        extent_x = max_poss_os_width;

    if( extent_y < max_poss_os_height)
        extent_y = max_poss_os_height;

    /* extents must be rounded to pixels so as not to confuse Neil */
    /* NB we choose to round extents to the higher multiple */
    extent_x = ceil_using_mask(extent_x, dxm1);
    extent_y = ceil_using_mask(extent_y, dym1);
    trace_2(TRACE_APP_PD4, "computed extent_x %d (OS), extent_y %d (OS)", extent_x, extent_y);

    extent_y = -extent_y; /* and now flip to -ve for Neil */

    if((extent_x != curr_extent_x)  ||  (extent_y != curr_extent_y))
    {
        BBox bbox;
        trace_2(TRACE_APP_PD4, "different extents: old extent_x %d, extent_y %d", curr_extent_x, curr_extent_y);

        /* note new extent */
        curr_extent_x = extent_x;
        curr_extent_y = extent_y;

        /* set new extent of rear window */
        bbox.xmin = 0;
        bbox.ymin = extent_y;
        bbox.xmax = extent_x;
        bbox.ymax = 0;

        trace_5(TRACE_APP_PD4, "calling wimp_set_extent(" UINTPTR_XTFMT "; %d, %d, %d, %d)",
                    (uintptr_t) rear_window_handle, bbox.xmin, bbox.ymin, bbox.xmax, bbox.ymax);
        void_WrapOsErrorReporting(tbl_wimp_set_extent(rear_window_handle, &bbox));
    }

    /* now think what to do with the scroll offsets */
    scroll_x = compute_scroll_x();
    scroll_y = compute_scroll_y();

    if(((scroll_x + delta_scroll_x) != curr_scroll_x)  ||  ((scroll_y + delta_scroll_y) != curr_scroll_y))
    {
        WimpOpenWindowBlock open_window_block;

        /* note new scroll offsets and zero the deltas */
        curr_scroll_x  = scroll_x;
        curr_scroll_y  = scroll_y;
        delta_scroll_x = 0;
        delta_scroll_y = 0;

        /* reopen rear window at new scroll offsets */
        memcpy32(&open_window_block, &window_state, sizeof32(open_window_block));
        open_window_block.xscroll = scroll_x;
        open_window_block.yscroll = scroll_y;
        trace_5(TRACE_APP_PD4, "calling wimp_open_window(" UINTPTR_XTFMT "; %d, %d, %d, %d;",
                (uintptr_t) window_state.window_handle,
                window_state.visible_area.xmin, window_state.visible_area.ymin,
                window_state.visible_area.xmax, window_state.visible_area.ymax);
        trace_2(TRACE_APP_PD4, " %d, %d)", open_window_block.xscroll, open_window_block.yscroll);
        void_WrapOsErrorReporting(tbl_wimp_open_window(&open_window_block));
    }

    return(FALSE);
}

/******************************************************************************
*
* actually put the caret in the window at the place
* we computed it should go if reposition needed
*
******************************************************************************/

extern void
draw_caret(void)
{
    if(xf_inexpression_box)
    {
        mlec_claim_focus(editexpression_formwind->mlec);

        xf_caretreposition = FALSE;
        return;
    }

    if(xf_inexpression_line)
    {
     /* we must some how give the caret back to the writeable icon, at the place it was lost
        from, probally, reading the caret position on a lose caret notification then using
        that as a param to set_caret_position would do the trick */

        formline_claim_focus();

        xf_caretreposition = FALSE;
        return;
    }

    trace_1(TRACE_APP_PD4_RENDER, "draw_caret(): reposition = %s", report_boolstring(xf_caretreposition));

    if(xf_caretreposition  &&  caretposallowed)
    {
        xf_caretreposition = FALSE;

        riscos_caret_set_position(
            main_window_handle,
            (riscos_fonts  &&  !xf_inexpression)
                ? lastcursorpos_x + leftslop
                : texttooffset_x(lastcursorpos_x),
            texttooffset_y(lastcursorpos_y));
    }
}

/******************************************************************************
*
* set caret position in a window
* NB. this takes offsets from xorg/yorg
*
* RCM: seems only to be called by draw_caret
*
******************************************************************************/

extern void
riscos_caret_set_position(
    _HwndRef_   HOST_WND window_handle,
    int x,
    int y)
{
    int caret_bits;

    y -= CARET_BODGE_Y;

    caret_bits = charvspace + 2 * CARET_BODGE_Y;

    if(!riscos_fonts  ||  xf_inexpression)
        caret_bits |= CARET_SYSTEMFONT;

    if(wimpt_bpp() < 8) /* can only do non-coloured carets in very simple modes */
    {
        const U32 caret_wimp_colour_value = wimp_colour_value_from_option(COI_CARET);
        const U32 bg_wimp_colour_value = wimp_colour_value_from_option(COI_BACK);

        if((0 == (0x10 & caret_wimp_colour_value)) && (0 == (0x10 & bg_wimp_colour_value)))
        {
            const int colour_bits =
                wimptx_GCOL_for_wimpcolour(caret_wimp_colour_value & 0x0F) ^
                wimptx_GCOL_for_wimpcolour(bg_wimp_colour_value & 0x0F);
            caret_bits |= colour_bits << CARET_COLOURSHIFT;
            caret_bits |= CARET_COLOURED | CARET_REALCOLOUR;
         }
    }

    trace_4(TRACE_APP_PD4_RENDER, "riscos_caret_set_position(%d, %d, %d): %8.8X", window_handle, x, y, caret_bits);

    /* presumably this gets called before main window is shown and gives the 'Input focus window not found' error */
    (void) /*void_WrapOsErrorReporting*/ (
        tbl_wimp_set_caret_position(window_handle, -1, x, y, caret_bits, 0));
}

/******************************************************************************
*
* remove caret from display in a window
* but keeping the input focus in that window
*
******************************************************************************/

/* stored caret position: relative to work area origin */
static GDI_POINT caret_rel;

extern void
riscos_caret_hide(void)
{
    WimpCaret caret;

    trace_0(TRACE_APP_PD4_RENDER, "riscos_caret_hide()");

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        return;

    if(main_window_handle != caret_window_handle)
        return;

    assert(caret.window_handle == caret_window_handle);

    caret_rel.x = caret.xoffset /*- curr_scroll_x*/;   /* +ve */
    caret_rel.y = caret.yoffset /*- curr_scroll_y*/;   /* -ve */

    trace_4(TRACE_APP_PD4_RENDER, "hiding caret from offset %d %d at relative pos %d, %d",
            caret.xoffset, caret.yoffset, caret_rel.x, caret_rel.y);

    /* way off top */
    caret.yoffset = 100;

    void_WrapOsErrorReporting(
        tbl_wimp_set_caret_position(caret.window_handle, caret.icon_handle,
                                    caret.xoffset, caret.yoffset,
                                    caret.height, caret.index));
}

/******************************************************************************
*
* restore caret after hiding
*
******************************************************************************/

extern void
riscos_caret_restore(void)
{
    WimpCaret caret;

    trace_0(TRACE_APP_PD4_RENDER, "riscos_caret_restore()");

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        return;

    if(main_window_handle != caret_window_handle)
        return;

    assert(caret.window_handle == caret_window_handle);

    caret.xoffset = /*curr_scroll_x*/ + caret_rel.x;
    caret.yoffset = /*curr_scroll_y*/ + caret_rel.y;

    trace_4(TRACE_APP_PD4_RENDER, "restoring caret from relative pos %d %d to offset %d, %d",
            caret_rel.x, caret_rel.y, caret.xoffset, caret.yoffset);

    void_WrapOsErrorReporting(
        tbl_wimp_set_caret_position(caret.window_handle, caret.icon_handle,
                                    caret.xoffset, caret.yoffset,
                                    caret.height, caret.index));}

/******************************************************************************
*
*  --in--   'at' position
*
*  --out--  position corrupted
*
******************************************************************************/

extern void
draw_grid_hbar(
    S32 len)
{
    const U32 restore_fg_wimp_colour_value = current_fg_wimp_colour_value;

    set_fg_colour_from_option(COI_GRID);

    wimpt_safe(os_plot(bbc_MoveCursorRel,              -dx,             -vdu5textoffset));
    wimpt_safe(os_plot(bbc_SolidBoth + bbc_DrawRelFore, len * charwidth, 0));

    riscos_set_fg_colour_from_wimp_colour_value(restore_fg_wimp_colour_value);
}

/******************************************************************************
*
*  --in--   'at' position
*
*  --out--  position preserved
*
******************************************************************************/

extern void
draw_grid_vbar(
    BOOL include_hbar)
{
    const U32 restore_fg_wimp_colour_value = current_fg_wimp_colour_value;
    GDI_COORD ldx = dx;
    GDI_COORD y_pos = charvrubout_pos;
    GDI_COORD y_neg = charvrubout_neg + (include_hbar ? dy : 0);

    set_fg_colour_from_option(COI_GRID);

    wimpt_safe(os_plot(bbc_MoveCursorRel,              -ldx, +y_pos));
    wimpt_safe(os_plot(bbc_SolidBoth + bbc_DrawRelFore,   0, -y_pos -y_neg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,              +ldx,        +y_neg));

    riscos_set_fg_colour_from_wimp_colour_value(restore_fg_wimp_colour_value);
}

extern void
filealtered(
    BOOL newstate)
{
    if(xf_filealtered != newstate)
    {
        reportf("filealtered := %d for %s", newstate, report_tstr(currentfilename()));

        xf_filealtered = newstate;

        riscos_settitlebar(currentfilename());
    }
}

/******************************************************************************
*
*  ensure font colours are set to reflect those set via set_fg_colour (and bg)
*
******************************************************************************/

_Check_return_
static inline _kernel_oserror *
colourtrans_SetFontColours(
    _In_        unsigned int font_handle,
    _In_        unsigned int bg_colour,
    _In_        unsigned int fg_colour,
    _In_        unsigned int max_offset)
{
    _kernel_swi_regs rs;
    rs.r[0] = font_handle;
    rs.r[1] = bg_colour;
    rs.r[2] = fg_colour;
    rs.r[3] = max_offset;
    return(_kernel_swi(ColourTrans_SetFontColours, &rs, &rs)); /* don't care about updated values here */
}

extern void
ensurefontcolours(void)
{
    if(riscos_fonts  &&  font_colours_invalid)
    {
        font fh = 0; /* current font */
        U32 bg_colour, fg_colour;
        const int max_offset = 14;

        if(0 != (0x10 & current_bg_wimp_colour_value))
            bg_colour = current_bg_wimp_colour_value & ~0x1F;
        else
            bg_colour = wimptx_RGB_for_wimpcolour(current_bg_wimp_colour_value & 0x0F);

        if(0 != (0x10 & current_fg_wimp_colour_value))
            fg_colour = current_fg_wimp_colour_value & ~0x1F;
        else
            fg_colour = wimptx_RGB_for_wimpcolour(current_fg_wimp_colour_value & 0x0F);

        trace_2(TRACE_APP_PD4_RENDER, "ColourTrans_SetFontColours(" U32_XTFMT ", " U32_XTFMT ")", fg_colour, bg_colour);
        (void) font_complain(colourtrans_SetFontColours(fh, bg_colour, fg_colour, max_offset));

        font_colours_invalid = FALSE;
    }
}

/******************************************************************************
*
*                           RISC OS printing
*
******************************************************************************/

static struct RISCOS_PRINTING_STATICS
{
    int  job;
    int  oldjob;
    print_pagesizestr psize;
    print_positionstr where;
    BOOL has_colour;
}
riscos_printer;

#define stat_bg     0xFFFFFF00U /* full white */
#define stat_fg     0x00000000U /* full black */
#define stat_neg    0x0000FF00U /* full red */

extern void
print_setcolours(
    _InVal_     COLOURS_OPTION_INDEX fore_colours_option_index,
    _InVal_     COLOURS_OPTION_INDEX back_colours_option_index)
{
    if(riscos_printer.has_colour)
    {
        setcolours(fore_colours_option_index, back_colours_option_index);
    }
    else
    {
        riscos_set_bg_colour_from_wimp_colour_value(0U);
        riscos_set_fg_colour_from_wimp_colour_value(7U);
    }
}

extern void
print_setfontcolours(void)
{
    if(riscos_fonts  &&  font_colours_invalid)
    {
        const font fh = 0; /* current font */
        const int max_offset = 14;
        const U32 bg_colour = stat_bg;
        U32 fg_colour = stat_fg;
        if(riscos_printer.has_colour)
            if(current_fg_wimp_colour_value == wimp_colour_value_from_option(COI_NEGATIVE))
                fg_colour = stat_neg;
        trace_2(TRACE_APP_PD4_RENDER, "ColourTrans_SetFontColours(" U32_XTFMT ", " U32_XTFMT ")", fg_colour, bg_colour);
        (void) print_complain(colourtrans_SetFontColours(fh, bg_colour, fg_colour, max_offset));
        font_colours_invalid = FALSE;
    }
}

#define XOS_Find (13 + (1<<17))

extern void
riscprint_set_printer_data(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "riscprint_set_printer_data()");

    /* RJM thinks print_pagesize returns 0 if it worked, contrary to SKS's earlier test */
    if(print_pagesize(&riscos_printer.psize) != 0)
    {
        /* error in reading paper size - assume a sort of normal A4 (LaserWriter defaults) */
        riscos_printer.psize.bbox.x0 =  18000;
        riscos_printer.psize.bbox.x1 = 577000;
        riscos_printer.psize.bbox.y0 =  23000;
        riscos_printer.psize.bbox.y1 = 819000;

        riscos_printer.psize.xsize = riscos_printer.psize.bbox.x0 + riscos_printer.psize.bbox.x1 + riscos_printer.psize.bbox.x0;
        riscos_printer.psize.ysize = riscos_printer.psize.bbox.y0 + riscos_printer.psize.bbox.y1 + riscos_printer.psize.bbox.y0;
    }

    paper_width_millipoints  = riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0;
    paper_length_millipoints = riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0;

    riscos_printer.where.dx = riscos_printer.psize.bbox.x0;
    riscos_printer.where.dy = riscos_printer.psize.bbox.y0;

    trace_2(TRACE_APP_PD4_RENDER, "riscprint_set_printer_data: print position %d %d",
            riscos_printer.where.dx, riscos_printer.where.dy);
    trace_2(TRACE_APP_PD4_RENDER, "page size: x = %g in., y = %g in.",
            riscos_printer.psize.xsize / 72000.0,
            riscos_printer.psize.ysize / 72000.0);
    trace_2(TRACE_APP_PD4_RENDER, "page size: x = %g mm., y = %g mm.",
            riscos_printer.psize.xsize / 72000.0 * 25.4,
            riscos_printer.psize.ysize / 72000.0 * 25.4);
    trace_4(TRACE_APP_PD4_RENDER, "page outline %d %d %d %d (mp)",
            riscos_printer.psize.bbox.x0, riscos_printer.psize.bbox.y0,
            riscos_printer.psize.bbox.x1, riscos_printer.psize.bbox.y1);
    trace_2(TRACE_APP_PD4_RENDER, "usable x = %d (mp), %d (os 100%%)",
            riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0,
            idiv_floor_fn(riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0, MILLIPOINTS_PER_OS));
    trace_2(TRACE_APP_PD4_RENDER, "usable y = %d (mp), %d (os 100%%)",
            riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0,
            idiv_floor_fn(riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0, MILLIPOINTS_PER_OS));
}

extern BOOL
riscprint_start(void)
{
    print_infostr pinfo;
    _kernel_swi_regs rs;

    trace_0(TRACE_APP_PD4_RENDER, "\n\n*** riscprint_start()");

    riscos_printer.job    = 0;
    riscos_printer.oldjob = 0;

    if(print_info(&pinfo))
        return(reperr_null(ERR_NORISCOSPRINTER));

    riscos_printer.has_colour = (pinfo.features & print_colour);

    /* update cache, just in case */
    riscprint_set_printer_data();

    /* open an output stream onto "printer:" */
    rs.r[0] = 0x8F; /* OpenOut, Ignore File$Path, give err if a dir.! */
    rs.r[1] = (int) "printer:";
    if(_kernel_swi(OS_Find, &rs, &rs))
    {
        reperr(ERR_PRINT_WONT_OPEN, _kernel_last_oserror()->errmess);
        return(FALSE);
    }

    /* check for RISC OS 2.00 FileSwitch bug (returning a zero handle) */
    if((riscos_printer.job = rs.r[0]) == 0)
    {
        reperr_null(ERR_PRINT_WONT_OPEN);
        return(FALSE);
    }

    trace_1(TRACE_APP_PD4_RENDER, "got print handle %d", riscos_printer.job);

    /* don't select - leave that to caller */

    return(1);
}

extern BOOL
riscprint_page(
    S32 copies,
    BOOL landscape,
    S32 scale_pct,
    S32 sequence,
    RISCOS_PRINTPROC pageproc)
{
    print_box size_os;
    print_box pbox;
    print_transmatstr transform;
    print_positionstr where_mp;
    char   buffer[16];
    char * pageptr;
    int    ID;
    BOOL   more, bum;
    S32    xform;
    S32    left_margin_shift_os;
    S32    usable_x_os, usable_y_os;

    trace_2(TRACE_APP_PD4_RENDER, "riscprint_page(%d copies, landscape = %s)",
            copies, report_boolstring(landscape));

    if(!scale_pct)
        return(reperr_null(ERR_BADPRINTSCALE));

    xform = muldiv64(0x00010000, scale_pct, 100);   /* fixed binary point number 16.16 */

    /* start with rectangle x0,y0 origin at bottom left of printable area */
    where_mp = riscos_printer.where;

    if(landscape)
    {
        /* landscape output: +90 deg (clockwise) rotation */
        transform.xx =  0;
        transform.xy = -xform;
        transform.yx = +xform;
        transform.yy =  0;

        /* move x0,y0 origin and therefore text origin near top right of
         * physical portrait page, descending towards left, across going down
        */
        where_mp.dx += paper_width_millipoints;
        where_mp.dy += paper_length_millipoints;
    }
    else
    {
        /* portrait output: no rotation */
        transform.xx =  xform;
        transform.xy =  0;
        transform.yx =  0;
        transform.yy =  xform;

        /* move x0,y0 origin and therefore text origin near top left of
         * physical portrait page, descending towards bottom, across going right
        */
        where_mp.dx += 0;
        where_mp.dy += paper_length_millipoints;
    }

    left_margin_shift_os = charwidth * left_margin_width();

    /* how big the usable area is in scaling OS units (i.e. corresponding to printing OS) */
    usable_x_os = muldiv64(((landscape) ? paper_length_millipoints : paper_width_millipoints ), 100, scale_pct * MILLIPOINTS_PER_OS);
    usable_y_os = muldiv64(((landscape) ? paper_width_millipoints  : paper_length_millipoints), 100, scale_pct * MILLIPOINTS_PER_OS);

    trace_4(TRACE_APP_PD4_RENDER, "usable area %d,%d (os %d%%): header_footer_width %d (os)",
            usable_x_os, usable_y_os, scale_pct, charwidth * header_footer_width);

    /* our object size in OS units (always make top left actually printed 0,0) */
    size_os.x0  = -4 + 0;
    size_os.x1  = +4 + MIN(usable_x_os, charwidth * header_footer_width);
    /* hmm. SKS 18.10.91 notes however that optimise for top margin would prevent large fonty headers etc. */
    size_os.y0  = -4 + 0 - usable_y_os;
    size_os.y1  = +4 + 0;

    if(landscape)
    {
        /* the rectangle has grown left across the physical page - adjust by y0 (adding -ve) */
        where_mp.dx += muldiv64(size_os.y0, MILLIPOINTS_PER_OS * scale_pct, 100);

        /* the now reduced size rectangle starts further down the physical page */
        where_mp.dy -= muldiv64(left_margin_shift_os, MILLIPOINTS_PER_OS * scale_pct, 100);
    }
    else
    {
        /* the rectangle has grown down the physical page - adjust by y0 (adding -ve) */
        where_mp.dy += muldiv64(size_os.y0, MILLIPOINTS_PER_OS * scale_pct, 100);

        /* the now reduced size rectangle starts further over to the right on the physical page */
        where_mp.dx += muldiv64(left_margin_shift_os, MILLIPOINTS_PER_OS * scale_pct, 100);
    }

    trace_6(TRACE_APP_PD4_RENDER, "print_giverectangle((%d, %d, %d, %d) (os), print_where %d %d (real mp))",
            size_os.x0, size_os.y0, size_os.x1, size_os.y1, where_mp.dx, where_mp.dy);
    bum = print_complain(print_giverectangle(1, &size_os,
                                             &transform,
                                             &where_mp,
                                             -1 /* background RGB (transparent) */));
    trace_1(TRACE_APP_PD4_RENDER, "print_giverectangle returned %d", bum);

    if(!bum)
    {
        trace_0(TRACE_APP_PD4_RENDER, "print_drawpage()");

        killcolourcache();

        if(curpnm)
        {
            consume_int(sprintf(buffer, "p%d", curpnm));
            pageptr = buffer;
        }
        else
            pageptr = NULL;

        reportf("print_drawpage(%s)", report_tstr(pageptr));
        bum = print_complain(print_drawpage(copies,
                                            sequence,   /* sequence number */
                                            pageptr,    /* textual page number */
                                            &pbox,
                                            &more,
                                            &ID));

        while(!bum  &&  more)
        {
            trace_4(TRACE_APP_PD4_RENDER, "print loop ... gw %d %d %d %d",
                    graphics_window.x0, graphics_window.y0,
                    graphics_window.x1, graphics_window.y1);

            #ifdef DEBUG_PRINT_INCHES_GRID
            if(1 /* && trace_is_enabled()*/)
            {
                int x, y;

                setcolour(FORE, BACK);

                bum = print_complain(bbc_circle(0, 0, 90)); /* expect this one */

                bum = print_complain(bbc_circle(+180, +180, 90));
                bum = print_complain(bbc_move(  +180, +180));
                bum = print_complain(bbc_stringprint("++"));

                bum = print_complain(bbc_circle(-180, -180, 90));
                bum = print_complain(bbc_move(  -180, -180));
                bum = print_complain(bbc_stringprint("--"));

                bum = print_complain(bbc_circle(+180, -180, 90)); /* expect this one */
                bum = print_complain(bbc_move(  +180, -180));
                bum = print_complain(bbc_stringprint("+x-y"));

                bum = print_complain(bbc_circle(-180, +180, 90));
                bum = print_complain(bbc_move(  -180, +180));
                bum = print_complain(bbc_stringprint("-x+y"));

                bum = print_complain(bbc_circle(+4*180, +4*180, 90));
                bum = print_complain(bbc_move(  +4*180, +4*180));
                bum = print_complain(bbc_stringprint("++"));

                bum = print_complain(bbc_circle(-4*180, -4*180, 90));
                bum = print_complain(bbc_move(  -4*180, -4*180));
                bum = print_complain(bbc_stringprint("--"));

                bum = print_complain(bbc_circle(+4*180, -4*180, 90)); /* expect this one */
                bum = print_complain(bbc_move(  +4*180, -4*180));
                bum = print_complain(bbc_stringprint("+x-y"));

                bum = print_complain(bbc_circle(-4*180, +4*180, 90));
                bum = print_complain(bbc_move(  -4*180, +4*180));
                bum = print_complain(bbc_stringprint("-x+y"));

                /* draw an virtual (i.e. possibly scaled) inches grid */
                for(x = - (4*180); x < + (usable_x_os + 4*180); x += 180)
                {
                    if(!bum)
                        bum = print_complain(bbc_move(x, + (4*180+90))); /* move across top */
                    if(!bum)
                        bum = print_complain(os_plot(bbc_SolidBoth + bbc_DrawRelFore, /* line down */
                                                      0,
                                                      - (usable_y_os +  (4*180+90) + 90)));
                    if(bum)
                        break;
                }

                for(y = + (4*180); y > - (usable_y_os + 4*180); y -= 180)
                {
                    if(!bum)
                        bum = print_complain(bbc_move(- (4*180+90), y)); /* move up left side */
                    if(!bum)
                        bum = print_complain(os_plot(bbc_SolidBoth + bbc_DrawRelFore, /* line across to right */
                                                      + (usable_x_os +  (4*180+90) + 90),
                                                      0));
                    if(bum)
                        break;
                }
            }
            #endif

            /* print page */

            /* set up initial baseline */
            riscos_font_ad_millipoints_y = 0 - global_font_leading_millipoints;
            trace_1(TRACE_APP_PD4_RENDER, "\n\n*** initial riscos_font_ad_millipoints_y := %d", riscos_font_ad_millipoints_y);

            killcolourcache();

            if(!bum)
                (* pageproc) ();

            if(been_error)
                bum = TRUE;

            if(!bum)
                bum = print_complain(print_getrectangle(&pbox, &more, &ID));
            /* will output showpage etc. when no more rectangles for page */
        }
    }

    killcolourcache();

    return(!bum);
}

extern BOOL
riscprint_end(
    BOOL ok)
{
    _kernel_swi_regs rs;
    os_error * e;
    os_error * e2;

    trace_1(TRACE_APP_PD4_RENDER, "riscprint_end(): %s print", !ok ? "aborting" : "ending");

    if(!riscos_printer.job)
        return(FALSE);

    /* don't deselect - leave that to caller */

    if(ok)
    {
        /* close neatly */
        e = print_endjob(riscos_printer.job);
        ok = (e == NULL);
    }
    else
        e = NULL;

    if(!ok)
    {
        /* whinge terribly */
        e2 = print_abortjob(riscos_printer.job);
        e = e ? e : e2;
        assert(NULL != e);
        if(NULL != e)
            reperr_kernel_oserror(e);
        been_error = FALSE;
    }

    trace_0(TRACE_APP_PD4_RENDER, "riscprint_end(): closing printer stream");
    rs.r[0] = 0;
    rs.r[1] = riscos_printer.job;
    e2 = WrapOsErrorReporting(_kernel_swi(OS_Find, &rs, &rs));

    e = e ? e : e2;

    return(ok ? (e == NULL) : ok);
}

extern BOOL
riscprint_resume(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "riscprint_resume()");

    assert(riscos_printer.job);

    assert(riscos_printer.oldjob == 0); /* test for sync from suspend */

    return(!print_complain(print_selectjob(riscos_printer.job, NULL, &riscos_printer.oldjob)));
}

extern BOOL
riscprint_suspend(void)
{
    int dummy_job;
    os_error * e;

    trace_0(TRACE_APP_PD4_RENDER, "riscprint_suspend()");

    if(!riscos_printer.job)
        return(FALSE);

    e = print_selectjob(riscos_printer.oldjob, NULL, &dummy_job);

    riscos_printer.oldjob = 0;

    if(NULL != e)
    {
        /* job suspension failure must be severe otherwise no errors may come out */
        (void) print_reset();
        return(reperr_kernel_oserror(e));
    }
    else
        myassert2x((dummy_job == 0) || (dummy_job == riscos_printer.job), "riscprint_suspend suspended the wrong job %d not %d!", dummy_job, riscos_printer.job);

    return(TRUE);
}

#endif /* RISCOS */

/* end of riscdraw.c */
