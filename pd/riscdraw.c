/* riscdraw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

#ifndef __colourtran_h
#include "colourtran.h"
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
internal functions
*/

#define round_with_mask(value, mask) \
    { (value) = ((value) + (mask)) & ~(mask); }

#define pd_muldiv64(a, b, c) \
    ((S32) muldiv64_limiting(a, b, c))

/* ----------------------------------------------------------------------- */

#define TRACE_CLIP      (TRACE_APP_PD4_RENDER & 0)
#define TRACE_DRAW      (TRACE_APP_PD4_RENDER)
#define TRACE_SETCOLOUR (TRACE_APP_PD4_RENDER)

/*
* +-----------------------------------------------------------------------+
* |                                                                       |
* |     work area extent origin                                           |
* |       + --  --  --  --  --  --  --  --  --  -- --  --  --  --  --  -- |
* |             .                                                   ^     |
* |       | TLS          TMS                                        |     |
* |         .   +   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |  .  |
* |       |     TCO                                                scy    |
* |                 +---+---+------------------------------+---+    |     |
* |       |     .   | B | C |     T i t l e    b a r       | T |    v     |
* |                 +---+---+------------------------------+---+ <---wy1  |
* |       |     .   |                                      | U |          |
* |                 |                                      +---+          |
* |       |     .   |                                      |   |          |
* |                 |                                      | = |          |
* |       |   L .   |                                      | # |          |
* |           M     |                                      | = |          |
* |       |   S .   |                                      |   |          |
* |                 |                                      |   |          |
* |       |     .   |                                      |   |          |
* |                 |                                      +---+          |
* |       |     .   |                                      | D |          |
* |                 +---+------------------------------+---+---+ <---wy0  |
* |       |     .   | L |      [-#-]                   | R | S |          |
* |                 +---+------------------------------+---+---+          |
* |       |<--scx-->^                                      ^              |
* |                 |                                      |              |
* |       |         |                                      |              |
* |                 wx0                                    wx1            |
* +-----------------------------------------------------------------------+
* ^
* graphics origin
*/

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
       S32 dxm1;       /* dx - 1 for AND/BIC */
static S32 two_dx;

       S32 dy;
static S32 dym1;
static S32 two_dy;
static S32 chmdy;      /* charheight - dy */

/* a mode independent y spacing value */
#define v_dy 4

/* size and spacing of VDU 5 chars in this mode */
static S32 default_gcharxsize;
static S32 default_gcharysize;
static S32 default_gcharxspace;
static S32 default_gcharyspace;

/* maximum size the contents of a window could be given in this screen mode */
static S32 max_poss_height;
static S32 max_poss_width;

/* current fg/bg colours for this call of update/redraw (temp globals) */
S32 current_fg = -1;
S32 current_bg = -1;

static S32 invert_fg;
static S32 invert_bg;

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

    trace_2(TRACE_APP_PD4_RENDER, "setting origin (%d, %d)\n", xorg, yorg);

    textcell_xorg = xorg;
    textcell_yorg = yorg;
}

extern void
killfontcolourcache(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "killfontcolourcache()\n");

    font_colours_invalid = TRUE;
}

extern void
killcolourcache(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "killcolourcache()\n");

    current_fg = current_bg = -1;

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
application_redraw(
    riscos_redrawstr *r)
{
    #if TRACE_ALLOWED
    wimp_redrawstr * redrawstr = (wimp_redrawstr *) r;
    S32 x0 = redrawstr->box.x0;
    S32 y1 = redrawstr->box.y1;
    #endif

    IGNOREPARM(r);

    /* calculate 'text window' in document that needs painting */
    /* note the flip & xlate of y coordinates from graphics to text */
    cliparea.x0 = tcoord_x( graphics_window.x0);
    cliparea.x1 = tcoord_x1(graphics_window.x1);
    cliparea.y0 = tcoord_y( graphics_window.y0);
    cliparea.y1 = tcoord_y1(graphics_window.y1);

    trace_2(TRACE_APP_PD4_RENDER, "app_redraw: x0 %d y1 %d\n", x0, y1);
    trace_2(TRACE_APP_PD4_RENDER, " textcell org %d, %d; ", textcell_xorg, textcell_yorg);
    trace_4(TRACE_APP_PD4_RENDER, " graphics window %d, %d, %d, %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1);
    trace_4(TRACE_APP_PD4_RENDER, " invalid text area %d, %d, %d, %d\n",
            thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1);

    setcolour(FORE, BACK);

    /* ensure all of buffer displayed on redraw */
    dspfld_from = -1;

    maybe_draw_screen();

    #ifdef HAS_FUNNY_CHARACTERS_FOR_WRCH
    wrch_undefinefunnies();     /* restore soft character definitions */
    #endif
}

/******************************************************************************
*
* swap fg/bg colours
*
******************************************************************************/

extern void
riscos_invert(void)
{
    int newbg = current_fg;
    int newfg = current_bg;

    current_fg = newfg;
    current_bg = newbg;

    trace_1(TRACE_SETCOLOUR, "invert: wimp_setcolour(fg %d); ", newfg);
    trace_1(TRACE_SETCOLOUR, "wimp_setcolour(bg %d)\n", newbg);

    wimpt_safe(wimp_setcolour(newfg       ));
    wimpt_safe(wimp_setcolour(newbg | 0x80));

    font_colours_invalid = TRUE;
}

/******************************************************************************
*
* Set document fg/bg colours iff different to those already set
*
******************************************************************************/

extern void
riscos_setcolour(
    int colour,
    BOOL isbackcolour)
{
    if(isbackcolour)
        {
        if(colour != current_bg)
            {
            current_bg = colour;
            trace_1(TRACE_SETCOLOUR, "wimp_setcolour(bg %d)\n", colour);
            wimpt_safe(wimp_setcolour(colour | 0x80));
            font_colours_invalid = TRUE;
            }
        }
    else
        {
        if(colour != current_fg)
            {
            current_fg = colour;
            trace_1(TRACE_SETCOLOUR, "wimp_setcolour(fg %d)\n", colour);
            wimpt_safe(wimp_setcolour(colour));
            font_colours_invalid = TRUE;
            }
        }
}

extern void
riscos_setcolours(
    int bg,
    int fg)
{
    riscos_setcolour(bg, TRUE);
    riscos_setcolour(fg, FALSE);
}

extern void
new_font_leading(
    S32 new_font_leading_mp)
{
    global_font_leading_mp = new_font_leading_mp;

    if(riscos_fonts)
        {
        /* line spacing - round up to nearest OS unit */
        charallocheight = roundtoceil(global_font_leading_mp, MILLIPOINTS_PER_OS);

        /* and then to nearest pixel corresponding */
        charallocheight = roundtoceil(charallocheight, dy) * dy;

        /* lines are at least two pixels high */
        charallocheight = MAX(charallocheight, 2*dy);

        /* SKS after 4.11 06jan92 - only set page length if in auto mode */
        if(auto_line_height)
            {
            S32 paper_size_mp;
            /* actual paper size */
            paper_size_mp = (d_print_QL == 'L')
                                    ? paper_width_mp
                                    : paper_length_mp;

            /* effective paper size including scaling (bigger scale -> less lines + v.v.) */
            paper_size_mp = (S32) muldiv64(paper_size_mp, 100, d_print_QS);

            /* page length (round down) */
            d_poptions_PL = paper_size_mp / global_font_leading_mp;
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
rjm and rcm add on 6.10.91
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
cachemodevariables(void)
{
    DOCNO old_docno;
    P_DOCU p_docu;
    S32 x, y;

    /* cache some variables so Draw rendering works */
    wimpt_checkmode();

    x = wimpt_xsize();
    y = wimpt_ysize();

    if(y != screen_y_os)
        /* ensure window default pos reset to sensible for mode again */
        riscos_setinitwindowpos(y - wimpt_title_height());

    screen_x_os = x;
    screen_y_os = y;

    default_gcharxsize  = bbc_vduvar(bbc_GCharSizeX)  << wimpt_xeig();
    default_gcharysize  = bbc_vduvar(bbc_GCharSizeY)  << wimpt_yeig();
    default_gcharxspace = bbc_vduvar(bbc_GCharSpaceX) << wimpt_xeig();
    default_gcharyspace = bbc_vduvar(bbc_GCharSpaceY) << wimpt_yeig();
    trace_4(TRACE_APP_PD4_RENDER, "default system font %d %d %d %d\n",
            default_gcharxsize, default_gcharysize, default_gcharxspace, default_gcharyspace);

    log2bpp = bbc_vduvar(bbc_Log2BPP);

    /* OS units per pixel */
    dx      = wimpt_dx();
    dxm1    = dx - 1;
    two_dx  = 2 * dx;

    dy      = wimpt_dy();
    dym1    = dy - 1;
    two_dy  = 2 * dy;
    chmdy   = charheight - dy;

    trace_4(TRACE_APP_PD4_RENDER, "dx = %d, dy = %d, xsize = %d, ysize = %d\n", dx, dy, x, y);

    max_poss_width  = x - leftline_width - wimpt_vscroll_width();
    max_poss_height = y - wimpt_title_height() - wimpt_hscroll_height() /* has hscroll not bottomline */;
    trace_2(TRACE_APP_PD4_RENDER, "max poss height = %d, max poss width = %d\n",
                    max_poss_height, max_poss_width);

    font_readscalefactor(&x_scale, &y_scale);
    trace_2(TRACE_APP_PD4_RENDER, "font x_scale = %d, font y_scale = %d\n", x_scale, y_scale);

    /* loop over documents setting new height */

    old_docno = current_docno();

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
        {
        select_document(p_docu);

        new_grid_state();

        colh_position_icons();  /* mainly because the slot coordinates box is made */
                                /* up of two icons, one dx,dy inside the other     */

        (void) new_window_height(windowheight());

        /* may need to set caret up again on mode change (256 colour modes) */
        xf_caretreposition = TRUE;

        if(main_window == caret_window)
            draw_caret();
        }

    select_document_using_docno(old_docno);
}

/******************************************************************************
*
* recache variables after a palette change
*
******************************************************************************/

extern void
cachepalettevariables(void)
{
    wimpt_checkpalette();
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
    fontscreenheightlimit_mp = global_font_leading_mp;
#else
    /* 15/12 is pragmatic fix to get 12pt still without use of grid */
    fontscreenheightlimit_mp = (charvspace - fontbaselineoffset) * (15*MILLIPOINTS_PER_OS/12);
#endif

    charvrubout_pos = charvspace - (vdu5textoffset + dy);

    charvrubout_neg = vdu5textoffset;

    /* don't rubout grid line! */
    if(grid_on)
        charvrubout_neg -= dy;

    trace_5(TRACE_APP_PD4_RENDER, "charvspace = %d, vdu5textoffset = %d, fontbaselineoffset = %d, charvrubout_pos = %d, charvrubout_neg = %d\n",
            charvspace, vdu5textoffset, fontbaselineoffset, charvrubout_pos, charvrubout_neg);
}

/******************************************************************************
*
*  how many whole text cells fit in the window
*
******************************************************************************/

extern coord
windowheight(void)
{
    wimp_wstate s;
    S32 os_height;
    S32 height;
    S32 min_height = calrad(3);        /* else rebols & friends explode */
    BOOL err;

    err = (main_window == window_NULL);

    if(!err)
        err = (BOOL) wimpt_safe(wimp_get_wind_state(main__window, &s));

    if(!err)
        os_height = (s.o.box.y1 - topslop) - s.o.box.y0;
    else
        os_height = min_height * charvspace;

    height = os_height / charvspace;

    unused_bit_at_bottom = (height * charvspace != os_height);

    height = MAX(height, min_height);

    trace_3(TRACE_APP_PD4_RENDER, "windowheight is %d os %d text, ubb = %s\n",
            os_height, height, trace_boolstring(unused_bit_at_bottom));
    return(height);
}

extern coord
windowwidth(void)
{
    wimp_wstate s;
    S32 width;
    BOOL err;

    err = (main_window == window_NULL);

    if(!err)
        err = (BOOL) wimpt_safe(wimp_get_wind_state(main__window, &s));

    if(!err)
        width = (s.o.box.x1 - (s.o.box.x0 + leftslop)) / charwidth;
    else
        width = BORDERWIDTH + 4;

    trace_1(TRACE_APP_PD4_RENDER, "windowwidth is %d\n", width);
    return(width);
}

/******************************************************************************
*
* position graphics cursor for text output
*
******************************************************************************/

extern void
at(
    S32 tx,
    S32 ty)
{
    trace_2(TRACE_DRAW, "at(%d, %d)\n", tx, ty);

    wimpt_safe(bbc_move(gcoord_x(tx), gcoord_y_textout(ty)));
}

extern void
at_fonts(
    S32 x,
    S32 ty)
{
    if(riscos_fonts)
        {
        trace_3(TRACE_DRAW, "at_fonts(%d (%d), %d)\n", x, textcell_xorg + x, ty);

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
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    BOOL zap_grid)
{
    S32 x0, y0, x1, y1;

    trace_4(TRACE_APP_PD4_RENDER, "clear_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

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
    S32 len)
{
    S32 x, ypos, yneg;

    trace_1(TRACE_DRAW, "clear_underlay(%d)\n", len);

    if(len > 0)
        {
        x = len * charwidth - dx;
        ypos = charvrubout_pos;
        yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(os_plot(bbc_MoveCursorRel,               0, +ypos));

        wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -ypos -yneg));
        wimpt_safe(os_plot(bbc_MoveCursorRel,                  -x,       +yneg));
        }
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

extern S32
gcoord_x(
    S32 x)
{
    return(textcell_xorg + x * charwidth);
}

#define gc_y(y) (textcell_yorg - (y+1) * charvspace)

extern S32
gcoord_y(
    S32 y)
{
    return(gc_y(y));
}

extern S32
gcoord_y_fontout(
    S32 y)
{
    return(gc_y(y) + fontbaselineoffset);
}

extern S32
gcoord_y_textout(
    S32 y)
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
    riscos_redrawstr *r)
{
    trace_4(TRACE_APP_PD4_RENDER, "redraw_cleararea: graphics window %d, %d, %d, %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1);

    IGNOREPARM(r);

    setbgcolour(BACK);
    wimpt_safe(bbc_vdu(bbc_ClearGraph));
}

extern void
please_clear_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_clear_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_clear_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    /* no text printing so don't need scaling wrapper */
    riscos_updatearea(  redraw_clear_area,
                        main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

/******************************************************************************
*
*  request that an area of text cells be inverted
*
******************************************************************************/

static void
redraw_invert_area(
    riscos_redrawstr *r)
{
    S32 invertEORcolour;

    invertEORcolour = wimpt_GCOL_for_wimpcolour(getcolour(invert_fg)) ^
                      wimpt_GCOL_for_wimpcolour(getcolour(invert_bg));

    trace_5(TRACE_APP_PD4_RENDER, "redraw_invert_area: graphics window (%d, %d, %d, %d), EOR %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1, invertEORcolour);

    IGNOREPARM(r);

    wimpt_safe(bbc_gcol(3, 0x80 | invertEORcolour));
    wimpt_safe(bbc_vdu(bbc_ClearGraph));
}

static void
please_invert_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    S32 fg,
    S32 bg)
{
    trace_6(TRACE_APP_PD4_RENDER, "please_invert_textarea(%d, %d, %d, %d) fg = %d, bg = %d\n",
                tx0, ty0, tx1, ty1, fg, bg);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_invert_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    invert_fg = fg;
    invert_bg = bg;

    /* no text printing so don't need scaling wrapper */
    riscos_updatearea(  redraw_invert_area,
                        main_window,
                        texttooffset_x(tx0),
                        /* invert grid hspace, not hbar */
                        texttooffset_y(ty0) + ((grid_on) ? dy : 0),
                        /* don't invert grid vbar */
                        texttooffset_x(tx1) - ((grid_on) ? dx : 0),
                        texttooffset_y(ty1));
}

extern void
please_invert_numeric_slot(
    S32 coff,
    S32 roff,
    S32 fg,
    S32 bg)
{
    S32 tx0 = calcad(coff);
    S32 tx1 = tx0 + colwidth(col_number(coff));
    S32 ty0 = calrad(roff);
    S32 ty1 = ty0 - 1;

    please_invert_textarea(tx0, ty0, tx1, ty1, fg, bg);
}

/* invert a set of slots, taking care with the grid */

extern void
please_invert_numeric_slots(
    S32 start_coff,
    S32 end_coff,
    S32 roff,
    S32 fg,
    S32 bg)
{
    S32 tx0, tx1, ty0, ty1;

    if(grid_on)
        while(start_coff <= end_coff)
            please_invert_numeric_slot(start_coff++, roff, fg, bg);
    else
        {
        tx0 = calcad(start_coff);
        tx1 = tx0;
        ty0 = calrad(roff);
        ty1 = ty0 - 1;

        while(start_coff <= end_coff)
            tx1 += colwidth(col_number(start_coff++));

        please_invert_textarea(tx0, ty0, tx1, ty1, fg, bg);
        }
}

/******************************************************************************
*
* request that an area of text cells be redrawn by a given procedure
*
******************************************************************************/

static riscos_redrawproc updatearea_proc;

static void
updatearea_wrapper(
    riscos_redrawstr *r)
{
    updatearea_proc(r);
}

extern void
please_update_textarea(
    riscos_redrawproc proc,
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_update_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_update_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    updatearea_proc = proc;

    riscos_updatearea(  updatearea_wrapper,
                        main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

extern void
please_update_thistextarea(
    riscos_redrawproc proc)
{
    please_update_textarea(proc, thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1);
}

extern void
please_update_window(
    riscos_redrawproc proc,
    wimp_w window,
    S32 gx0,
    S32 gy0,
    S32 gx1,
    S32 gy1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_update_window(%d, %d, %d, %d)\n", gx0, gy0, gx1, gy1);

    updatearea_proc = proc;

    riscos_updatearea(updatearea_wrapper, window, gx0, gy0, gx1, gy1);
}

/******************************************************************************
*
* request that an area of text cells be redrawn
*
******************************************************************************/

extern void
please_redraw_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1)
{
    trace_4(TRACE_APP_PD4_RENDER, "please_redraw_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    myassert4x((tx0 <= tx1)  &&  (ty0 >= ty1), "please_redraw_textarea(%d, %d, %d, %d) is stupid", tx0, ty0, tx1, ty1);

    updatearea_proc = application_redraw;

    riscos_updatearea(  updatearea_wrapper,
                        main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}

extern void
please_redraw_textline(
    S32 tx0,
    S32 ty0,
    S32 tx1)
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
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    BOOL set_gw)
{
    GDI_BOX clipper;

    clipper.x0 = gcoord_x(tx0);
    clipper.y0 = gcoord_y(ty0);
    clipper.x1 = gcoord_x(tx1);
    clipper.y1 = gcoord_y(ty1);

    trace_4(TRACE_APP_PD4_RENDER, "set_gw_from_textarea(%d, %d, %d, %d)\n",
            tx0, ty0, tx1, ty1);
    trace_4(TRACE_APP_PD4_RENDER, "textarea  (%d, %d, %d, %d) (OS)\n",
            clipper.x0, clipper.y0, clipper.x1, clipper.y1);
    trace_4(TRACE_APP_PD4_RENDER, "window gw (%d, %d, %d, %d) (OS)\n",
            graphics_window.x0, graphics_window.y0, graphics_window.x1, graphics_window.y1);

    clipper.x0 = MAX(clipper.x0, graphics_window.x0);
    clipper.y0 = MAX(clipper.y0, graphics_window.y0);
    clipper.x1 = MIN(clipper.x1, graphics_window.x1);
    clipper.y1 = MIN(clipper.y1, graphics_window.y1);

    trace_4(TRACE_APP_PD4_RENDER, "intersection (%d, %d, %d, %d) (OS)\n", clipper.x0, clipper.y0, clipper.x1, clipper.y1);

    if((clipper.x0 >= clipper.x1)  ||  (clipper.y0 >= clipper.y1))
        {
        trace_0(TRACE_APP_PD4_RENDER, "zero size window requested - OS incapable\n");
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

        trace_4(TRACE_APP_PD4_RENDER, "setting gw (%d, %d, %d, %d) (OS)\n", clipper.x0, clipper.y0, clipper.x1, clipper.y1);

        /* when setting graphics window, all points are inclusive */
        wimpt_safe(bbc_gwindow(clipper.x0,      clipper.y0,
                               clipper.x1 - dx, clipper.y1 - dy));
        }

    return(TRUE);
}

extern void
restore_graphics_window(void)
{
    /* restore from saved copy  */
    graphics_window = saved_graphics_window;

    wimpt_safe(bbc_gwindow(graphics_window.x0,      graphics_window.y0,
                           graphics_window.x1 - dx, graphics_window.y1 - dy));
}

/******************************************************************************
*
*  round to ± infinity at multiples of a given number
*
******************************************************************************/

extern S32
roundtoceil(
    S32 a,
    S32 b)
{
    return( ((a <= 0) ? a : (a + (b - 1))) / b );
}

extern S32
roundtofloor(
    S32 a,
    S32 b)
{
    return( ((a >= 0) ? a : (a - (b - 1))) / b );
}

/******************************************************************************
*
* scroll the given area down by n lines
* as if it were a real text window
*
******************************************************************************/

extern void
scroll_textarea(
    S32 tx0,
    S32 ty0,
    S32 tx1,
    S32 ty1,
    S32 nlines)
{
    wimp_box box;
    S32 y, uy0, uy1, ht;

    trace_5(0, "scroll_textarea((%d, %d, %d, %d), %d)\n", tx0, ty0, tx1, ty1, nlines);

    myassert5x((tx0 <= tx1)  &&  (ty0 >= ty1), "scroll_textarea((%d, %d, %d, %d), %d) is stupid", tx0, ty0, tx1, ty1, nlines);

    if(nlines != 0)
        {
        box.x0 = texttooffset_x(tx0);
        box.x1 = texttooffset_x(tx1+1);
        ht     = nlines * charvspace;

        if(ht > 0)
            {
            /* scrolling area down, clear top line(s) */
            box.y0 = texttooffset_y(ty0) + ht;
            box.y1 = texttooffset_y(ty1-1);
            y      = box.y0 - ht;
            uy0    = box.y1 - ht;
            uy1    = box.y1;
            }
        else
            {
            /* scrolling area up, clear bottom line(s) */
            box.y0 = texttooffset_y(ty0);
            box.y1 = texttooffset_y(ty1-1) + ht;
            y      = box.y0 - ht;
            uy0    = box.y0;
            uy1    = y;
            }

        riscos_removecaret();

        trace_6(TRACE_APP_PD4_RENDER, "wimp_blockcopy((%d, %d, %d, %d), %d, %d)\n",
                            box.x0, box.y0, box.x1, box.y1, box.x0, y);

        wimpt_safe(wimp_blockcopy(main__window, &box, box.x0, y));

        /* get our clear lines routine called - again no text drawn */
        riscos_updatearea(  redraw_clear_area,
                            main_window,
                            box.x0, uy0,
                            box.x1, uy1);

        riscos_restorecaret();
        }
}

/******************************************************************************
*
* conversions from absolute graphics coordinates
* to text cell coordinates for input
* (and their friends for calculating cliparea)
*
* returns bottom left corner of text cell
*
******************************************************************************/

extern S32
tsize_x(
    S32 x)
{
    return(roundtoceil(x, charwidth));
}

extern S32
tsize_y(
    S32 y)
{
    return(roundtoceil(y, charvspace));
}

extern S32
tcoord_x(
    S32 x)
{
    return(roundtofloor(x - textcell_xorg, charwidth));
}

extern S32
tcoord_y(
    S32 y)
{
    return(roundtoceil(textcell_yorg - y, charvspace) - 1);
}

extern S32
tcoord_x_remainder(
    S32 x)
{
    S32 tx = tcoord_x(x);

    return((x - textcell_xorg) - tx * charwidth);
}

extern S32
tcoord_x1(
    S32 x)
{
    return(roundtoceil(x - textcell_xorg, charwidth));
}

extern S32
tcoord_y1(
    S32 y)
{
    return(roundtofloor(textcell_yorg - y, charvspace) - 1);
}

/******************************************************************************
*
* work out whether an text object intersects the cliparea
* if it does, stick the given coordinates in thisarea
*
******************************************************************************/

extern BOOL
textobjectintersects(
    S32 x0,
    S32 y0,
    S32 x1,
    S32 y1)
{
    BOOL intersects = !((cliparea.x0 >=          x1)    ||
                        (         y1 >= cliparea.y0)    ||
                        (         x0 >= cliparea.x1)    ||
                        (cliparea.y1 >=          y0)    );

    trace_4(TRACE_CLIP, "textobjectintersects: %d %d %d %d (if any true, fails)\n",
             x0, y0, x1, y1);
    trace_1(TRACE_CLIP, "x0 >= cliparea.x1 %s, ", trace_boolstring(x0 >= cliparea.x1));
    trace_1(TRACE_CLIP, "x1 <= cliparea.x0 %s, ", trace_boolstring(x1 <= cliparea.x0));
    trace_1(TRACE_CLIP, "y0 <= cliparea.y1 %s, ", trace_boolstring(y0 <= cliparea.y1));
    trace_1(TRACE_CLIP, "y1 >= cliparea.y0 %s\n", trace_boolstring(y1 >= cliparea.y0));

    if(intersects)
        {
        thisarea.x0 = x0;
        thisarea.y0 = y0;
        thisarea.x1 = x1;
        thisarea.y1 = y1;
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
    S32 x0,
    S32 x1)
{
    BOOL intersects = !((cliparea.x0 >=          x1)    ||
                        (         x0 >= cliparea.x1)    );

    trace_2(TRACE_CLIP, "textxintersects: %d %d (if any true, fails)\n",
             x0, x1);
    trace_1(TRACE_CLIP, "x0 >= cliparea.x1 %s, ", trace_boolstring(x0 >= cliparea.x1));
    trace_1(TRACE_CLIP, "x1 <= cliparea.x0 %s, ", trace_boolstring(x1 <= cliparea.x0));

    if(intersects)
        {
        thisarea.x0 = x0;
        thisarea.x1 = x1;
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

extern S32
texttooffset_x(
    S32 x)
{
    return((/*curr_scx*/ + leftslop) + x * charwidth);
}

extern S32
texttooffset_y(
    S32 y)
{
    return((/*curr_scy*/ - topslop) - (y+1) * charvspace);
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
    trace_1(TRACE_DRAW, "riscos_movespaces(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        wimpt_safe(os_plot(bbc_MoveCursorRel, nspaces * charwidth, 0));
}

extern void
riscos_movespaces_fonts(
    S32 nspaces)
{
    trace_1(TRACE_DRAW, "riscos_movespaces_fonts(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        wimpt_safe(os_plot(bbc_MoveCursorRel,
                            riscos_fonts ? nspaces : nspaces * charwidth, 0));
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
    S32 ldx, x, ypos, yneg;

    trace_1(TRACE_DRAW, "riscos_printspaces(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        {
        ldx = dx;
        x = nspaces * charwidth - ldx;
        ypos = charvrubout_pos;
        yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(os_plot(bbc_MoveCursorRel,                0, +ypos));

        wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -ypos  -yneg));
        wimpt_safe(os_plot(bbc_MoveCursorRel,                  ldx,        +yneg));
        }
}

extern void
riscos_printspaces_fonts(
    S32 nspaces)
{
    S32 ldx, x, ypos, yneg;

    trace_1(TRACE_DRAW, "riscos_printspaces_fonts(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        {
        ldx = dx;
        x = (riscos_fonts ? nspaces : nspaces * charwidth) - ldx;
        ypos = charvrubout_pos;
        yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(os_plot(bbc_MoveCursorRel,                0, +ypos));

        wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -ypos  -yneg));
        wimpt_safe(os_plot(bbc_MoveCursorRel,                  ldx,        +yneg));
        }
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
    S32 x    = charwidth - dx;                      /* avoids reload over procs */
    S32 ypos = charvrubout_pos;
    S32 yneg = charvrubout_neg;

    trace_1(TRACE_DRAW, "riscos_printchar(%d)\n", ch);

    if(ypos)
        wimpt_safe(os_plot(bbc_MoveCursorRel,               0, +ypos));

    wimpt_safe(os_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -ypos -yneg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,                  -x,       +yneg));

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

extern void
application_scroll_request(
    wimp_eventstr *e)
{
    S32 xdir = e->data.scroll.x;   /* -1 for left, +1 for right */
    S32 ydir = e->data.scroll.y;   /* -1 for down, +1 for up    */
                                    /*     ±2 for page scroll    */

    trace_2(TRACE_APP_PD4, "app_scroll_request: xdir %d ydir %d\n", xdir, ydir);

    /* ensure draw_caret gets ignored if not current window */
    caretposallowed = (main_window == caret_window);

    switch(ydir)
        {
        case +2:
            application_process_command(N_PageUp);
            break;

        case +1:
            application_process_command(N_ScrollUp);
            break;

        default:
            break;

        case -1:
            application_process_command(N_ScrollDown);
            break;

        case -2:
            application_process_command(N_PageDown);
            break;
            }

    switch(xdir)
        {
        case +2:
            application_process_command(N_PageRight);
            break;

        case +1:
            application_process_command(N_ScrollRight);
            break;

        default:
            break;

        case -1:
            application_process_command(N_ScrollLeft);
            break;

        case -2:
            application_process_command(N_PageLeft);
            break;
            }

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

static S32
compute_scx(void)
{
    COL ffc    = fstncx();
    COL nfixes = n_colfixes;
    S32 scx;

    if(nfixes)
        if(ffc >= col_number(0))
            ffc -= nfixes;

    scx = pd_muldiv64(curr_xext, (S32) ffc, cols_for_extent());

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    round_with_mask(scx, dxm1);

    trace_1(TRACE_APP_PD4, "computed scx %d (OS)\n", scx);
    return(scx);
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

static S32
compute_scy(void)
{
    ROW ffr    = fstnrx();
    ROW nfixes = n_rowfixes;
    S32 scy;

    if(nfixes)
        if(ffr >= row_number(0))
            ffr -= nfixes;

    scy = pd_muldiv64(curr_yext, (S32) ffr, rows_for_extent()); /* -ve, +ve, +ve */

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    round_with_mask(scy, dym1);

    trace_1(TRACE_APP_PD4, "computed scy %d (OS)\n", scy);
    return(scy);
}

/******************************************************************************
*
* Open the various pane windows (main_dbox & colh_dbox) that cover rear_dbox.
*
******************************************************************************/

static void
openpane(
    wimp_box *boxp,
    wimp_w behind)
{
    wimp_icon     heading;
    S32           borderline = 0;       /* a negative number, relative to top edge of rear_window */
    wimp_openstr  o;

    wimp_get_icon_info(colh_window, (wimp_i)COLH_COLUMN_HEADINGS, &heading);

    borderline = heading.box.y0;        /* approx -132 */

    /* open main_window */

    o.w      = main__window;
    o.box    = *boxp;                   /* if borders are off, main_window completely covers rear_window and colh_window */
    if(borbit)
        o.box.y1 = o.box.y1 + borderline; /* - 132;*/   /* else main_window abutts colh_window and both cover rear_window */
    o.scx    = 0;
    o.scy    = 0;
    o.behind = behind;

    wimpt_complain(wimp_open_wind(&o));

    o.w      = colh_window;
    o.box    = *boxp;          /* pane completely covers rear_window */
    o.box.y0 = o.box.y1 + borderline; /* - 132;*/
    o.scx    = 0;
    o.scy    = 0;
    o.behind = main_window;

    wimpt_complain(wimp_open_wind(&o));

    if(borbit)
        {
        wimp_icreate create;
        S32          x1;

        wimp_get_icon_info(colh_window, (wimp_i)COLH_CONTENTS_LINE, &create.i);

        trace_4(TRACE_APP_EXPEDIT, "Contents line icon currently (%d,%d, %d,%d)\n",
                                   create.i.box.x0, create.i.box.y0, create.i.box.x1, create.i.box.y1);

        x1 = (o.box.x1 - o.box.x0) - MAX(dx, dy);       /*16;*/

        if(x1 < (create.i.box.x0 + 16))
            x1 = create.i.box.x0 + 16;          /* perhaps unnecessary but I don't trust Neil */

        if(create.i.box.x1 != x1)
            {
            /* icon size is wrong, so... */

            create.i.box.x1 = x1;

            redefine_icon(colh_window, (wimp_i)COLH_CONTENTS_LINE, &create);
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
application_open_request(
    wimp_eventstr *e)
{
    wimp_openstr * op           = &e->data.o;
    S32     scx                 = op->scx;
    S32     scy                 = op->scy;
    wimp_w  behind              = op->behind;
    BOOL    old_unused_bit      = unused_bit_at_bottom;
    /* note knowledge of height <-> pagvars conversion */
    S32     old_window_height   = paghyt + 1;
    S32     old_window_width    = pagwid_plus1;
    S32     window_height;
    S32     window_width;
    wimp_redrawstr r;
    BOOL    smash = FALSE;
    S32     smash_y0;
    S32     size_change;

    trace_2(TRACE_APP_PD4, "\n\n*** app_open_request: w %d, behind %d\n", op->w, behind);
    trace_6(TRACE_APP_PD4, "                : x0 %d, x1 %d;  y0 %d, y1 %d; scx %d, scy %d\n",
            op->box.x0, op->box.x1, op->box.y0, op->box.y1, scx, scy);

    /* have to tweak requested coordinates as we have a wierd extent */
    if( op->box.x1 > op->box.x0 + max_poss_width)
        op->box.x1 = op->box.x0 + max_poss_width;

    if( op->box.y0 < op->box.y1 - max_poss_height)
        op->box.y0 = op->box.y1 - max_poss_height;

    if(behind != (wimp_w) -2)
        {
        /* on mode change, Neil tells us to open windows behind themselves! */
        if((behind == rear__window) || (behind == colh_window))
            behind = main__window;

        /* always open pane first */
        openpane(&op->box, behind);

        /* always open rear_window behind pane colh_window */
        op->behind = colh_window;
        }

    wimpt_complain(wimp_open_wind(op));

    if(first_open)
        {
        /* remember first successful open position, bump from there (nicer on < 1024 y OS unit screens) */
        first_open = FALSE;
        riscos_setnextwindowpos(op->box.y1); /* gets bumped */
        }

    /* reopen pane with corrected coords */
    wimpt_safe(wimp_get_wind_state(rear__window, (wimp_wstate *) op));

    /* keep track of where we opened it */
    open_box = * (PC_GDI_BOX) &op->box;

    /* if rear_window was sent to the back, open panes behind the window that
     * the rear_window ended up behind
    */
    if(behind == (wimp_w) -2)
        behind = op->behind;

    openpane(&op->box, behind);

    trace_6(TRACE_APP_PD4, "opened rear_window at: x0 %d, x1 %d;  y0 %d, y1 %d; width %d (OS) height %d (OS)\n",
            op->box.x0, op->box.x1, op->box.y0, op->box.y1,
               (op->box.x1 - op->box.x0), (op->box.y1 - op->box.y0));

    {
    wimp_wstate s;

    wimpt_safe(wimp_get_wind_state(main__window, &s));

    /* note absolute coordinates of window (NOT work area extent) origin */

    setorigins(s.o.box.x0, s.o.box.y1);
    }

    window_height = windowheight();
    window_width  = windowwidth();

    /* suss resize after opening as then we know how big we really are */
    size_change = 0;

    if(old_window_height != window_height)
        {
        (void) new_window_height(window_height);
        size_change = 1;
        }

    if(old_window_width != window_width)
        {
        (void) new_window_width(window_width);
        size_change = 1;
        }

    /* is someone trying to scroll through the document
     * by dragging the scroll bars?
     * SKS - forbid scrolling to have any effect here if open window changed size
    */
    if((scx != curr_scx) && !size_change)
        {
        /* work out which column to put at left */
        COL leftcol, o_leftcol, newcurcol;
        COL nfixes, delta;

        o_leftcol = fstncx();
        nfixes    = n_colfixes;

        trace_3(TRACE_APP_PD4, "horizontal scroll bar dragged: scx %d, curr_scx %d, curr_xext %d\n",
                scx, curr_scx, curr_xext);

        leftcol = (COL) pd_muldiv64(scx, cols_for_extent(), curr_xext);

        if(nfixes)
            if(leftcol >= col_number(0))
                leftcol += nfixes;

        if( leftcol > numcol - 1)
            leftcol = numcol - 1;

        trace_1(TRACE_APP_PD4, "put col %d at left of scrolling area\n", leftcol);

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
            if(main_window == caret_window)
                xf_acquirecaret = TRUE;
            }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scx    = scx;
        delta_scx   = scx - compute_scx();
        trace_1(TRACE_APP_PD4, "delta_scx := %d\n", delta_scx);
        }

    if((scy != curr_scy) && !size_change)
        {
        /* work out which row to put at top */
        ROW toprow, o_toprow, newcurrow;
        ROW nfixes, delta;

        o_toprow = fstnrx();
        nfixes   = n_rowfixes;

        trace_3(TRACE_APP_PD4, "vertical scroll bar dragged: scy %d, curr_scy %d, curr_yext %d\n",
                scy, curr_scy, curr_yext);

        toprow = (ROW) pd_muldiv64(rows_for_extent(), scy, curr_yext);  /* +ve, -ve, -ve */

        if(nfixes)
            if(toprow >= row_number(0))
                toprow += nfixes;

        trace_3(TRACE_APP_PD4, "row %d is my first guess, numrow %d, rows_available %d\n", toprow, numrow, rows_available);

        /* eg. toprow = 42, numrow = 72, rows_available = 30 triggers fudge of 6 */

        if(!nfixes  &&  (toprow >= (numrow - 1) - ((ROW) rows_available - 1))  &&  encpln)
            {
            trace_0(TRACE_APP_PD4, "put last row at bottom of scrolling area\n");

            toprow += (ROW) rows_available / ((ROW) encpln + 1);
            }

        trace_1(TRACE_APP_PD4, "put row %d at top of scrolling area\n", toprow);

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
            if(main_window == caret_window)
                xf_acquirecaret = TRUE;
            }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scy    = scy;
        delta_scy   = scy - compute_scy();
        trace_1(TRACE_APP_PD4, "delta_scy := %d\n", delta_scy);
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
            r.w      = main__window;
            r.box.x0 = texttooffset_x(-1);              /* lhs slop too */
            r.box.x1 = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            r.box.y0 = texttooffset_y(smash_y0);
            r.box.y1 = r.box.y0 + charvspace;
            trace_5(TRACE_APP_PD4, "calling wimp_force_redraw(%d; %d, %d, %d, %d)\n",
                        r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
            wimpt_safe(wimp_force_redraw(&r));
            }
        }

    if(window_width != old_window_width)
        {
        /* window motion may reposition caret */
        (void) mergebuf();

        if(window_width > old_window_width)
            {
            r.w      = main__window;
            r.box.x0 = texttooffset_x(old_window_width);
            r.box.x1 = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            r.box.y0 = texttooffset_y(-1);              /* top slop too */
            r.box.y1 = texttooffset_y(window_height);
            trace_5(TRACE_APP_PD4, "calling wimp_force_redraw(%d; %d, %d, %d, %d)\n",
                        r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
            wimpt_safe(wimp_force_redraw(&r));
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
    wimp_redrawstr r;
    wimp_wstate wstate;
    S32 oswidth, osheight;
    S32 xext, yext, scx, scy;

    /* read current rear_window size etc. */
    wimpt_safe(wimp_get_wind_state(rear__window, &wstate));
    trace_5(TRACE_APP_PD4, "\n\n\n*** draw_altered_state(): get_wind_state(%d) returns %d, %d, %d, %d;",
                wstate.o.w, wstate.o.box.x0, wstate.o.box.y0,
                wstate.o.box.x1, wstate.o.box.y1);
    trace_2(TRACE_APP_PD4, " scx %d, scy %d\n", wstate.o.scx, wstate.o.scy);

    oswidth  = wstate.o.box.x1 - wstate.o.box.x0;
    osheight = wstate.o.box.y1 - wstate.o.box.y0;
    trace_2(TRACE_APP_PD4, " width %d (OS), height %d (OS)\n", oswidth, osheight);

    /* recompute extents */
    xext = pd_muldiv64(oswidth,  (S32) numcol, colsonscreen ? colsonscreen : 1);

    if( xext < max_poss_width)
        xext = max_poss_width;

    yext = pd_muldiv64(osheight, (S32) numrow, rowsonscreen);

    if( yext < max_poss_height)
        yext = max_poss_height;

    yext = -yext;

    /* extent must be rounded to pixels so as not to confuse Neil */
    round_with_mask(xext, dxm1);
    round_with_mask(yext, dym1);
    trace_2(TRACE_APP_PD4, "computed xext %d (OS), yext %d (OS)\n", xext, yext);

    if((xext != curr_xext)  ||  (yext != curr_yext))
        {
        trace_2(TRACE_APP_PD4, "different extents: old xext %d, yext %d\n", curr_xext, curr_yext);

        /* note new extent */
        curr_xext = xext;
        curr_yext = yext;

        /* set new extent of rear_window */
        r.w      = rear__window;
        r.box.x0 = 0;
        r.box.x1 = xext;
        r.box.y0 = yext;
        r.box.y1 = 0;

        trace_5(TRACE_APP_PD4, "calling wimp_set_extent(%d; %d, %d, %d, %d)\n",
                    r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
        wimpt_safe(wimp_set_extent(&r));
        }

    /* now think what to do with the scroll offsets */
    scx = compute_scx();
    scy = compute_scy();

    if(((scx + delta_scx) != curr_scx)  ||  ((scy + delta_scy) != curr_scy))
        {
        /* note new scroll offsets and zero the deltas */
        curr_scx    = scx;
        curr_scy    = scy;
        delta_scx   = 0;
        delta_scy   = 0;

        /* reopen rear_window at new scroll offsets */
        wstate.o.scx = scx;
        wstate.o.scy = scy;
        trace_5(TRACE_APP_PD4, "calling wimp_open_wind(%d; %d, %d, %d, %d;",
                wstate.o.w, wstate.o.box.x0, wstate.o.box.y0,
                wstate.o.box.x1, wstate.o.box.y1);
        trace_2(TRACE_APP_PD4, " %d, %d)\n", wstate.o.scx, wstate.o.scy);
        wimpt_safe(wimp_open_wind(&wstate.o));
        }

    return(FALSE);
}

/* actually put the caret in the window at the place
 * we computed it should go if reposition needed
*/

extern void
draw_caret(void)
{
    if(xf_inexpression_box)
        {
        mlec_claim_focus(editexpression_formwind->mlec);
        xf_caretreposition = FALSE;
        }
    else if(xf_inexpression_line)
        {
     /* we must some how give the caret back to the writeable icon, at the place it was lost
        from, probally, reading the caret position on a lose caret notification then using
        that as a param to set_caret_position would do the trick */

        formline_claim_focus();

        xf_caretreposition = FALSE;
        }
    else
        {
        trace_1(TRACE_APP_PD4_RENDER, "draw_caret(): reposition = %s\n", trace_boolstring(xf_caretreposition));

        if(xf_caretreposition  &&  caretposallowed)
            {
            xf_caretreposition = FALSE;
            setcaretpos(lastcursorpos_x, lastcursorpos_y);
            }
        }
}

/* RCM: seems only to be called by draw_caret */
extern void
setcaretpos(
    S32 x,
    S32 y)
{
    trace_2(TRACE_APP_PD4_RENDER, "setcaretpos(%d, %d)\n", x, y);

    riscos_setcaretpos( main_window,
                        (riscos_fonts  &&  !xf_inexpression)
                                ? x + leftslop
                                : texttooffset_x(x),
                        texttooffset_y(y));
}

/******************************************************************************
*
* set caret position in a window
* NB. this takes offsets from xorg/yorg
*
* RCM: seems only to be called by setcaretpos
*      which is only called by draw_caret
*
******************************************************************************/

extern void
riscos_setcaretpos(
    wimp_w w,
    int x,
    int y)
{
    S32 caretbits;
    wimp_caretstr caret;

    y -= CARET_BODGE_Y;

    caretbits = charvspace + 2 * CARET_BODGE_Y;

    if(!riscos_fonts  ||  xf_inexpression)
        caretbits |= CARET_SYSTEMFONT;

    if(wimpt_bpp() < 8) /* can only do non-coloured carets in very simple modes */
        {
        caretbits |= CARET_COLOURED | CARET_REALCOLOUR;
        caretbits |= (wimpt_GCOL_for_wimpcolour(getcolour(CARETC)) ^ wimpt_GCOL_for_wimpcolour(getcolour(BACK))) << CARET_COLOURSHIFT;
        }

    caret.w      = w;
    caret.i      = (wimp_i) -1;         /* never in any icon */
    caret.x      = x;
    caret.y      = y;
    caret.height = caretbits;
    caret.index  = 0;

    trace_4(TRACE_APP_PD4_RENDER, "riscos_setcaretpos(%d, %d, %d): %8.8X\n", w, x, y, caret.height);

    wimpt_safe(wimp_set_caret_pos(&caret));
}

/******************************************************************************
*
* remove caret from display in a window
* but keeping the input focus in that window
*
******************************************************************************/

/* stored caret position: relative to work area origin */
static S32 caret_rel_x;
static S32 caret_rel_y;

extern void
riscos_removecaret(void)
{
    wimp_caretstr caret;

    trace_0(TRACE_APP_PD4_RENDER, "riscos_removecaret()\n");

    if(main_window == caret_window)
        {
        wimpt_safe(wimp_get_caret_pos(&caret));

        caret_rel_x = caret.x /*- curr_scx*/;   /* +ve */
        caret_rel_y = caret.y /*- curr_scy*/;   /* -ve */

        trace_4(TRACE_APP_PD4_RENDER, "hiding caret from offset %d %d at relative pos %d, %d\n",
                caret.x, caret.y, caret_rel_x, caret_rel_y);

        /* way off top */
        caret.y = 100;

        wimpt_safe(wimp_set_caret_pos(&caret));
        }
}

/******************************************************************************
*
* restore caret after removal
*
******************************************************************************/

extern void
riscos_restorecaret(void)
{
    wimp_caretstr caret;

    trace_0(TRACE_APP_PD4_RENDER, "riscos_restorecaret()\n");

    if(main_window == caret_window)
        {
        wimpt_safe(wimp_get_caret_pos(&caret));

        caret.x = /*curr_scx*/ + caret_rel_x;
        caret.y = /*curr_scy*/ + caret_rel_y;

        trace_4(TRACE_APP_PD4_RENDER, "restoring caret from relative pos %d %d to offset %d, %d\n",
                caret_rel_x, caret_rel_y, caret.x, caret.y);

        wimpt_safe(wimp_set_caret_pos(&caret));
        }
}

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
    S32 fg = current_fg;

    setfgcolour(GRIDC);

    wimpt_safe(os_plot(bbc_MoveCursorRel,              -dx,             -vdu5textoffset));
    wimpt_safe(os_plot(bbc_SolidBoth + bbc_DrawRelFore, len * charwidth, 0));

    riscos_setcolour(fg, FALSE);
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
    S32 fg = current_fg;
    S32 ldx = dx;
    S32 ypos = charvrubout_pos;
    S32 yneg = charvrubout_neg + (include_hbar ? dy : 0);

    setfgcolour(GRIDC);

    wimpt_safe(os_plot(bbc_MoveCursorRel,              -ldx, +ypos));
    wimpt_safe(os_plot(bbc_SolidBoth + bbc_DrawRelFore,   0, -ypos -yneg));
    wimpt_safe(os_plot(bbc_MoveCursorRel,              +ldx,       +yneg));

    riscos_setcolour(fg, FALSE);
}

extern void
filealtered(
    BOOL newstate)
{
    if(xf_filealtered != newstate)
        {
        reportf("filealtered := %d for %s", newstate, report_tstr(currentfilename));

        xf_filealtered = newstate;

        riscos_settitlebar(currentfilename);
        }
}

/******************************************************************************
*
*  ensure font colours are set to reflect those set via setcolour
*
******************************************************************************/

extern void
ensurefontcolours(void)
{
    if(riscos_fonts  &&  font_colours_invalid)
        {
        #ifdef SKS_FONTCOLOURS
        /* should use ColourTrans even on screen: consider
         * user that turns wimp colour 3 red -> fonts get
         * measles if wimp_setfontcolours used!
        */
        font fh;
        wimp_paletteword bg, fg;
        int offset;

        fh = 0;
        bg.word = wimpt_RGB_for_wimpcolour(current_bg);
        fg.word = wimpt_RGB_for_wimpcolour(current_fg);
        offset = 14;
        trace_2(TRACE_APP_PD4_RENDER, "colourtran_setfontcolours(%8.8X, %8.8X)\n", fg.word, bg.word);
        (void) font_complain(colourtran_setfontcolours(&fh, &bg, &fg, &offset));
        #else
        trace_2(TRACE_APP_PD4_RENDER, "wimp_setfontcolours(%d, %d)\n", current_fg, current_bg);
        (void) font_complain(wimp_setfontcolours(current_fg, current_bg));
        #endif
        font_colours_invalid = FALSE;
        }
}

/******************************************************************************
*
*                           RISC OS printing
*
******************************************************************************/

static struct _riscos_printing_statics
{
    int  job;
    int  oldjob;
    print_pagesizestr psize;
    print_positionstr where;
    BOOL has_colour;
}
riscos_printer;

#define stat_bg     0xFFFFFF00  /* full white */
#define stat_fg     0x00000000  /* full black */
#define stat_neg    0xFF000000  /* full red */

extern void
print_setcolours(
    S32 fore,
    S32 back)
{
    if(riscos_printer.has_colour)
        setcolour(fore, back);
    else
        riscos_setcolours(0 /*bg*/, 7 /*fg*/);
}

extern void
print_setfontcolours(void)
{
    font fh;
    wimp_paletteword bg, fg;
    int offset;

    if(riscos_fonts  &&  font_colours_invalid)
        {
        fh = 0;
        bg.word = (int) stat_bg;
        fg.word = (riscos_printer.has_colour  &&  (current_fg == NEGATIVEC)) ? stat_neg : stat_fg;
        offset = 14;
        trace_2(TRACE_APP_PD4_RENDER, "colourtran_setfontcolours(%8.8X, %8.8X)\n", fg.word, bg.word);
        (void) print_complain(colourtran_setfontcolours(&fh, &bg, &fg, &offset));
        font_colours_invalid = FALSE;
        }
}

#define XOS_Find (13 + (1<<17))

extern void
riscprint_set_printer_data(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "riscprint_set_printer_data()\n");

    /* rjm thinks print_pagesize returns 0 if it worked, contrary to sks's earlier test */
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

    paper_width_mp  = riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0;
    paper_length_mp = riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0;

    riscos_printer.where.dx = riscos_printer.psize.bbox.x0;
    riscos_printer.where.dy = riscos_printer.psize.bbox.y0;

    trace_2(TRACE_APP_PD4_RENDER, "riscprint_set_printer_data: print position %d %d\n",
            riscos_printer.where.dx, riscos_printer.where.dy);
    trace_2(TRACE_APP_PD4_RENDER, "page size: x = %g in., y = %g in.\n",
            riscos_printer.psize.xsize / 72000.0,
            riscos_printer.psize.ysize / 72000.0);
    trace_2(TRACE_APP_PD4_RENDER, "page size: x = %g mm., y = %g mm.\n",
            riscos_printer.psize.xsize / 72000.0 * 25.4,
            riscos_printer.psize.ysize / 72000.0 * 25.4);
    trace_4(TRACE_APP_PD4_RENDER, "page outline %d %d %d %d (mp)\n",
            riscos_printer.psize.bbox.x0, riscos_printer.psize.bbox.y0,
            riscos_printer.psize.bbox.x1, riscos_printer.psize.bbox.y1);
    trace_2(TRACE_APP_PD4_RENDER, "usable x = %d (mp), %d (os 100%%)\n",
            riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0,
            roundtofloor(riscos_printer.psize.bbox.x1 - riscos_printer.psize.bbox.x0, MILLIPOINTS_PER_OS));
    trace_2(TRACE_APP_PD4_RENDER, "usable y = %d (mp), %d (os 100%%)\n",
            riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0,
            roundtofloor(riscos_printer.psize.bbox.y1 - riscos_printer.psize.bbox.y0, MILLIPOINTS_PER_OS));
}

extern BOOL
riscprint_start(void)
{
    print_infostr pinfo;
    _kernel_swi_regs rs;

    trace_0(TRACE_APP_PD4_RENDER, "\n\n*** riscprint_start()\n");

    riscos_printer.job    = 0;
    riscos_printer.oldjob = 0;

    if(print_info(&pinfo))
        return(reperr_null(create_error(ERR_NORISCOSPRINTER)));

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

    trace_1(TRACE_APP_PD4_RENDER, "got print handle %d\n", riscos_printer.job);

    /* don't select - leave that to caller */

    return(1);
}

extern BOOL
riscprint_page(
    S32 copies,
    BOOL landscape,
    S32 scale_pct,
    S32 sequence,
    riscos_printproc pageproc)
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

    trace_2(TRACE_APP_PD4_RENDER, "riscprint_page(%d copies, landscape = %s)\n",
            copies, trace_boolstring(landscape));

    if(!scale_pct)
        return(reperr_null(create_error(ERR_BADPRINTSCALE)));

    xform = (S32) muldiv64(0x00010000, scale_pct, 100);   /* fixed binary point number 16.16 */

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
        where_mp.dx += paper_width_mp;
        where_mp.dy += paper_length_mp;
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
        where_mp.dy += paper_length_mp;
        }

    left_margin_shift_os = charwidth * left_margin_width();

    /* how big the usable area is in scaling OS units (ie corresponding to printing OS) */
    usable_x_os = (S32) muldiv64(((landscape) ? paper_length_mp : paper_width_mp ), 100, scale_pct * MILLIPOINTS_PER_OS);
    usable_y_os = (S32) muldiv64(((landscape) ? paper_width_mp  : paper_length_mp), 100, scale_pct * MILLIPOINTS_PER_OS);

    trace_4(TRACE_APP_PD4_RENDER, "usable area %d,%d (os %d%%): header_footer_width %d (os)\n",
            usable_x_os, usable_y_os, scale_pct, charwidth * header_footer_width);

    /* our object size in OS units (always make top left actually printed 0,0) */
    size_os.x0  = -4 + 0;
    size_os.x1  = +4 + MIN(usable_x_os, charwidth * header_footer_width);
    /* hmm. SKS 18.10.91 notes however that optimise for top margin would prevent large fonty headers etc */
    size_os.y0  = -4 + 0 - usable_y_os;
    size_os.y1  = +4 + 0;

    if(landscape)
        {
        /* the rectangle has grown left across the physical page - adjust by y0 (adding -ve) */
        where_mp.dx += (S32) muldiv64(size_os.y0, MILLIPOINTS_PER_OS * scale_pct, 100);

        /* the now reduced size rectangle starts further down the physical page */
        where_mp.dy -= (S32) muldiv64(left_margin_shift_os, MILLIPOINTS_PER_OS * scale_pct, 100);
        }
    else
        {
        /* the rectangle has grown down the physical page - adjust by y0 (adding -ve) */
        where_mp.dy += (S32) muldiv64(size_os.y0, MILLIPOINTS_PER_OS * scale_pct, 100);

        /* the now reduced size rectangle starts further over to the right on the physical page */
        where_mp.dx += (S32) muldiv64(left_margin_shift_os, MILLIPOINTS_PER_OS * scale_pct, 100);
        }

    trace_6(TRACE_APP_PD4_RENDER, "print_giverectangle((%d, %d, %d, %d) (os), print_where %d %d (real mp))\n",
            size_os.x0, size_os.y0, size_os.x1, size_os.y1, where_mp.dx, where_mp.dy);
    bum = print_complain(print_giverectangle(1, &size_os,
                                             &transform,
                                             &where_mp,
                                             -1 /* background RGB (transparent) */));
    trace_1(TRACE_APP_PD4_RENDER, "print_giverectangle returned %d\n", bum);

    if(!bum)
        {
        trace_0(TRACE_APP_PD4_RENDER, "print_drawpage()\n");

        killcolourcache();

        if(curpnm)
            {
            (void) sprintf(buffer, "p%d", curpnm);
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
            trace_4(TRACE_APP_PD4_RENDER, "print loop ... gw %d %d %d %d\n",
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

                /* draw an virtual (ie possibly scaled) inches grid */
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
            riscos_font_yad = 0 - global_font_leading_mp;
            trace_1(TRACE_APP_PD4_RENDER, "\n\n*** initial riscos_font_yad := %d (mp)\n", riscos_font_yad);

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
    os_error * bum;
    os_error * bum2;

    trace_1(TRACE_APP_PD4_RENDER, "riscprint_end(): %s print\n", !ok ? "aborting" : "ending");

    if(!riscos_printer.job)
        return(FALSE);

    /* don't deselect - leave that to caller */

    if(ok)
        {
        /* close neatly */
        bum = print_endjob(riscos_printer.job);
        ok = (bum == NULL);
        }
    else
        bum = NULL;

    if(!ok)
        {
        /* winge terribly */
        bum2 = print_abortjob(riscos_printer.job);
        bum = bum ? bum : bum2;
        rep_fserr(bum->errmess);
        been_error = FALSE;
        }

    trace_0(TRACE_APP_PD4_RENDER, "riscprint_end(): closing printer stream\n");
    rs.r[0] = 0;
    rs.r[1] = riscos_printer.job;
    bum2 = wimpt_complain(_kernel_swi(OS_Find, &rs, &rs));

    bum = bum ? bum : bum2;

    return(ok ? (bum == NULL) : ok);
}

extern BOOL
riscprint_resume(void)
{
    trace_0(TRACE_APP_PD4_RENDER, "riscprint_resume()\n");

    assert(riscos_printer.job);

    assert(riscos_printer.oldjob == 0); /* test for sync from suspend */

    return(!print_complain(print_selectjob(riscos_printer.job, NULL, &riscos_printer.oldjob)));
}

extern BOOL
riscprint_suspend(void)
{
    int dummy_job;
    os_error * bum;

    trace_0(TRACE_APP_PD4_RENDER, "riscprint_suspend()\n");

    if(!riscos_printer.job)
        return(FALSE);

    bum = print_selectjob(riscos_printer.oldjob, NULL, &dummy_job);

    riscos_printer.oldjob = 0;

    if(bum)
        {
        /* job suspension failure must be severe otherwise no errors may come out */
        (void) print_reset();
        return(rep_fserr(bum->errmess));
        }
    else
        myassert2x((dummy_job == 0) || (dummy_job == riscos_printer.job), "riscprint_suspend suspended the wrong job %d not %d!", dummy_job, riscos_printer.job);

    return(TRUE);
}

#endif /* RISCOS */

/* end of riscdraw.c */
