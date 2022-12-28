/* scdraw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Screen drawing module for PipeDream */

/* RJM Aug 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#include "viewio.h"

#include "riscos_x.h"
#include "riscdraw.h"
#include "pd_x.h"
#include "colh_x.h"

/*
internal functions
*/

#if TRUE
static void
adjust_lescrl(
    S32 x,
    S32 fwidth);

static S32
adjust_rowout(
    S32 rowoff);

static S32
adjust_rowborout(
    S32 rowoff);

static S32
cal_dead_text(void);

static void
check_output_current_slot(
    S32 move);

static void
draw_empty_bottom_of_screen(void);

static void
draw_empty_right_of_screen(void);

static void
draw_altered_cells(void);

static void
draw_row_border(
    S32 roff);

static void
draw_screen_below(
    S32 roff);

static void
draw_cell_in_buffer(void);

static void
draw_cell_in_buffer_in_place(void);

static S32
dspfld(
    S32 x,
    S32 y,
    S32 fwidth_ch);

static S32
end_of_block(
    S32 x_pos,
    ROW trow);

static S32
limited_fwidth_of_slot_in_buffer(void);

static S32
onejst_plain(
    uchar *str,
    S32 fwidth_ch,
    uchar type);

static GR_MILLIPOINT
onejst_riscos_fonts(
    uchar *str,
    S32 fwidth_ch,
    uchar type);

static void
really_draw_row_border(
    S32 roff,
    S32 rpos);

static S32
really_draw_row_contents(
    S32 roff,
    S32 rpos);

static S32
really_draw_cell(
    S32 coff,
    S32 roff,
    BOOL in_draw_row);

static BOOL         /* set inverse, depends on cell,mark */
setivs(
    COL,
    ROW);

static S32
strout(
    uchar *str,
    S32 fwidth);

static void
draw_spaces_with_grid(
    S32 spaces,
    S32 start);

static S32
font_paint_justify(
    char *str,
    S32 fwidth);

static void
maybe_draw_empty_bottom_of_screen(void);

static void
maybe_draw_empty_right_of_screen(void);

static void
maybe_draw_pictures(void);

static BOOL
maybe_draw_row(S32 roff);

static void
maybe_draw_row_border(
    S32 roff,
    S32 rpos);

static void
maybe_draw_row_contents(
    S32 roff,
    S32 rpos);

static void
maybe_draw_rows(void);

static void
maybe_draw_unused_bit_at_bottom(void);

static void
switch_off_highlights(void);

#define cal_lescrl(fw) (MAX( ((lecpos <= lescrl)                \
                                    ? lecpos - ((fw) / 2)       \
                                    : (lecpos - (fw) > lescrl)  \
                                            ? lecpos - (fw)     \
                                            : lescrl),          \
                             0))

static S32 screen_xad_os, screen_yad_os;

/* ----------------------------------------------------------------------- */

/* conditional tracing flags (lose the & 0 to enable with TRACE_APP_PD4) */

#define TRACE_DRAW      (TRACE_APP_PD4 & 0)
#define TRACE_MAYBE     (TRACE_APP_PD4 & 0)
#define TRACE_CLIP      (TRACE_APP_PD4 & 0)
#define TRACE_REALLY    (TRACE_APP_PD4 & 0)
#define TRACE_OVERLAP   (TRACE_APP_PD4 & 0)

/* ----------------- RISC OS specific screen drawing --------------------- */

/* now consider objects to repaint */

extern void
maybe_draw_screen(void)
{
    maybe_draw_rows();
    maybe_draw_empty_bottom_of_screen();
    maybe_draw_empty_right_of_screen();
    maybe_draw_pictures();
    /* erase bit at bottom after all drawn */
    maybe_draw_unused_bit_at_bottom();
}

/* -------------- end of RISC OS specific drawing routines --------------- */

/******************************************************************************
*
* print a string (LJ) in a field, clearing out bg if necessary
* graphics cursor advanced to next text cell
* NB. overlong strings get poked, so there ...
*
******************************************************************************/

static S32
stringout_field(
    char *str,
    S32 fwidth)
{
    S32 len = strlen(str);
    S32 drawn;
    char ch;

    trace_2(TRACE_REALLY, "stringout_field(%d, \"%s\")", fwidth, str);

    if(len > fwidth)
    {
        ch = str[fwidth];
        str[fwidth] = CH_NULL;
        drawn = stringout(str);
        str[fwidth] = ch;
    }
    else
        drawn = stringout(str);

    /* move to end of field */
    if(fwidth - drawn > 0)
        riscos_movespaces(fwidth - drawn);

    return(drawn);
}

/******************************************************************************
*
* set up row information done
* whenever screen height set:
* new_main_window_height, DSfunc, window size change
*
******************************************************************************/

extern void
reinit_rows(void)
{
    /* max number of rows possible in this mode */
    maxnrow = (paghyt + 1);
    trace_1(TRACE_APP_PD4, "maxnrow        := %d", maxnrow);

    /* number of rows we can actually use at the moment */
    rows_available = maxnrow;    /* depends on displaying_borders */
    trace_1(TRACE_APP_PD4, "rows_available := %d", rows_available);
}

/******************************************************************************
*
* try to set a new screen height
* if it works, variables updated
*
* --out--
*   FALSE -> no vertvec, sorry...
*
******************************************************************************/

extern BOOL
new_main_window_height(
    tcoord height)
{
    SC_ARRAY_INIT_BLOCK vertvec_init_block = aib_init(8, sizeof32(SCRROW), TRUE);
    S32 nrows;
    STATUS status;

    trace_1(TRACE_APP_PD4, "new_main_window_height(%d)", height);

    /* calculate no. of rows needed: h=25 -> 22 rows if borders off,
     * (and headline present) +1 for terminating row -> nrows=23
    */
    /* e.g. h=7, b=2 -> nrows=6 (7+1-2) */
    nrows = (height + 1 /*one for LAST*/);

    if(nrows > maximum_rows) /* never consider shrinking! */
        if(NULL == al_array_extend_by(&vertvec_mh, SCRROW, nrows - maximum_rows, &vertvec_init_block, &status))
            return(0);

    if((nrows != maximum_rows)  &&  (0 != vertvec_mh))
    {
        /* only (re)set variables if realloc ok */
        S32 old_maximum_rows = maximum_rows;

        if(nrows > old_maximum_rows)
        {
            const S32 limit = nrows - maximum_rows;
            P_SCRROW rptr = vertvec_entry(old_maximum_rows);
            S32 i;

            for(i = 0; i < limit; ++i)
                rptr[i].flags = LAST;
        }

        maximum_rows = nrows;
        trace_1(TRACE_APP_PD4, "maximum_rows   := %d", maximum_rows);

        paghyt = height - 1;
        trace_1(TRACE_APP_PD4, "paghyt         := %d", paghyt);

        reinit_rows();          /* derives things from paghyt */

        out_rebuildvert = TRUE;     /* shrinking or growing */
    }
    else
        trace_0(TRACE_APP_PD4, "no row change: why were we called?");

    return(0 != vertvec_mh); /* 0 -> buggered */
}

/******************************************************************************
*
* try to set a new screen width
* if it works, variables updated
*
* --out--
*   FALSE -> no horzvec, sorry...
*
******************************************************************************/

extern BOOL
new_main_window_width(
    tcoord width)
{
    SC_ARRAY_INIT_BLOCK horzvec_init_block = aib_init(4, sizeof32(SCRCOL), TRUE);
    /* number of columns needed: +1 for terminating column */
    S32 ncols = (width + 1 /*one for LAST*/);
    STATUS status;

    trace_1(TRACE_APP_PD4, "new_main_window_width(%d)", width);

    if(ncols > maximum_cols) /* never consider shrinking! */
        if(NULL == al_array_extend_by(&horzvec_mh, SCRROW, ncols - maximum_cols, &horzvec_init_block, &status))
            return(0);

    if((ncols != maximum_cols)  &&  (0 != horzvec_mh))
    {
        /* only (re)set variables if realloc ok */
        S32 old_maximum_cols = maximum_cols;

        if(ncols > old_maximum_cols)
        {
            const S32 limit = ncols - maximum_cols;
            P_SCRCOL cptr = horzvec_entry(old_maximum_cols);
            S32 i;

            for(i = 0; i < limit; ++i)
                cptr[i].flags = LAST;
        }

        maximum_cols = ncols;
        trace_1(TRACE_APP_PD4, "maximum_cols   := %d", maximum_cols);

        pagwid_plus1 = width;
        trace_1(TRACE_APP_PD4, "pagwid_plus1   := %d", pagwid_plus1);

        pagwid = pagwid_plus1 - 1;

        maxncol = pagwid_plus1;
        trace_1(TRACE_APP_PD4, "maxncol        := %d", maxncol);

        cols_available = maxncol - borderwidth;
        trace_1(TRACE_APP_PD4, "cols_available := %d", cols_available);

        /* columns need much more redrawing than rows on resize */
        out_rebuildhorz = TRUE;
    }
    else
        trace_0(TRACE_APP_PD4, "no col change: why were we called?");

    return(0 != horzvec_mh); /* 0 -> buggered */
}

/******************************************************************************
*
*  initialise screen data
*
******************************************************************************/

extern BOOL
screen_initialise(void)
{
    BOOL ok;

    trace_0(TRACE_APP_PD4, "screen_initialise()");

    new_grid_state();       /* set charvspace/vrubout - doesn't redraw */

    /* even on RISC OS, may have changed from burned-in grid state */
    ok =    new_main_window_height(main_window_height())  &&
            new_main_window_width(main_window_width());

    if(!ok)
    {
        trace_0(TRACE_APP_PD4, "***** screen_initialise() failed *****");
        screen_finalise();
    }

    return(ok);
}

/******************************************************************************
*
* finalise screen data
*
******************************************************************************/

extern void
screen_finalise(void)
{
    al_array_dispose(&horzvec_mh);

    al_array_dispose(&vertvec_mh);
}

/******************************************************************************
*
* draw screen appropriately after something has happened
*
******************************************************************************/

static BOOL draw_screen_donePict;
static MONOTIME draw_screen_initialTime;

#ifndef DRAW_SCREEN_TIMEOUT
#define DRAW_SCREEN_TIMEOUT MONOTIME_VALUE(250) /* ms */
#endif

#if 1 /* SKS. change to 0 if perturbed by this */
#define draw_screen_timeout_criterion() /* BOOL(): whether time out enabled */ ( \
    TRUE )
#else
#define draw_screen_timeout_criterion() /* BOOL(): whether time out enabled */ ( \
    (drag_type == DRAG_COLUMN_WIDTH)     || \
    (drag_type == DRAG_COLUMN_WRAPWIDTH) )
#endif

#define draw_screen_timeout_init() /* void() */ \
{ \
    if(draw_screen_timeout_criterion()) \
    { \
        draw_screen_donePict = 0; \
        draw_screen_initialTime = monotime(); \
    } \
}

#define draw_screen_timeout() /* BOOL(): TRUE if should time out */ ( \
    draw_screen_timeout_criterion()  && \
    (monotime_diff(draw_screen_initialTime) >= (draw_screen_donePict ? 3 * DRAW_SCREEN_TIMEOUT : DRAW_SCREEN_TIMEOUT)) )

#define BIG_TX ((3 << 13) / normal_charwidth)
#define BIG_TY ((3 << 13) / normal_charheight)

extern void
draw_screen(void)
{
    BOOL old_movement;
    BOOL colh_mark_state_indicator_new;

    trace_0(TRACE_DRAW, "\n*** draw_screen()");

    oldcol = curcol;
    oldrow = currow;

    if(xf_draweverything)
    {
        xf_draweverything = FALSE; /* unset this redraw flag */

        xf_drawcellcoordinates = xf_drawcolumnheadings =
        out_rebuildhorz = out_rebuildvert = out_screen = out_currslot = TRUE;

        /* clear out entire window */
        please_clear_textarea(-BIG_TX, BIG_TY, BIG_TX, BIG_TY);
    }

    xf_interrupted = FALSE; /* always reset, gets set again if needed */

    colh_mark_state_indicator_new = ((blkstart.col != NO_COL) &&
                                    (blk_docno == current_docno()));
    if(colh_mark_state_indicator_new != colh_mark_state_indicator)
        colh_draw_mark_state_indicator(colh_mark_state_indicator_new);

    assert(vertvec_entry_valid(0));
    if(row_number(0) >= numrow)
        cenrow(numrow-1);

    if(out_rebuildvert)
        filvert(fstnrx(), currow, CALL_FIXPAGE);    /* first non-fixed row */

    if(out_rebuildhorz)
        filhorz(fstncx(), curcol);

    old_movement = movement;

    if((int) movement)
    {
        xf_drawcellcoordinates = TRUE;

        if(!dont_update_lescrl)
            lescrl = 0;
        else
            dont_update_lescrl = FALSE;

        if(movement & ABSOLUTE)
            chkmov();
        else
        {
            switch((int) movement)
            {
            case CURSOR_UP:
                curup();
                break;

            case CURSOR_DOWN:
                curdown();
                break;

            case CURSOR_PREV_COL:
                prevcol();
                break;

            case CURSOR_NEXT_COL:
                nextcol();
                break;

            case CURSOR_SUP:
                cursup();
                break;

            case CURSOR_SDOWN:
                cursdown();
                break;

            default:
                break;
            }
        }

        movement = 0;
    }

    curosc();

    /* re-load screen vector if we change rows with draw files about */
    if(draw_file_refs.lbr  &&  (pict_currow != currow))
    {
        trace_0(TRACE_APP_PD4, "draw_screen calling filvert: pict_currow != currow");
        filvert(fstnrx(), currow, DONT_CALL_FIXPAGE);
    }

    if(pict_on_screen_prev != pict_on_screen)
    {
        trace_0(TRACE_APP_PD4, "*** forcing out_screen because pict_on_screen changed");
        pict_on_screen_prev = pict_on_screen;
        out_screen = TRUE;
    }

    check_output_current_slot(old_movement);

    if(xf_flush)
    {
        xf_flush = FALSE;
        clearkeyboardbuffer();
    }

    (void) draw_altered_state();        /* sets extent, scroll offsets */

    /* send window to the front after adjusting scroll offsets! */
    if(xf_front_document_window)
    {
        /* this sort of fronting demands caret claim at the grand opening */
        xf_acquirecaret = TRUE;
        riscos_front_document_window(in_execfile);
    }

    if(xf_drawcolumnheadings)
        colh_draw_column_headings();
#if 1
    /* SKS after PD 4.11 08jan92 - consider interrupt in draw_screen_below() for full screen case ... */
    if(out_screen)
    {
        out_screen = FALSE;

        out_below  = TRUE;
        rowtoend   = 0;

        xf_draw_empty_right = 1;
    }

    if(xf_draw_empty_right)
        draw_empty_right_of_screen(); /* ditto 13jan92 - this is now needed */

    /* SKS after PD 4.11 08jan92 - move here after fixed draw_screen overheads */
    draw_screen_timeout_init();

    if(out_below)
        draw_screen_below(rowtoend);
#else
    /* SKS after PD 4.11 08jan92 - move here after fixed draw_screen overheads */
    draw_screen_timeout_init();

    if(out_screen)
        draw_screen_below(0);
    else if(out_below)
        draw_screen_below(rowtoend);
#endif

    if(out_rowout)
        draw_row(adjust_rowout(rowout));

    if(out_rowout1)
        draw_row(adjust_rowout(rowout1));

    if(out_rowborout)
        draw_row_border(adjust_rowborout(rowborout));

    if(out_rowborout1)
        draw_row_border(adjust_rowborout(rowborout1));

    if(xf_drawsome)
        draw_altered_cells();

    draw_cell_in_buffer();      /* if there is one */

    position_cursor();

    if(xf_caretreposition)
        if(main_window_handle == caret_window_handle)
            draw_caret();

    out_currslot = FALSE;
}

/******************************************************************************
*
*                           display draw files
*
******************************************************************************/

static void
really_draw_picture(
    P_DRAW_DIAG p_draw_diag,
    P_DRAW_FILE_REF p_draw_file_ref,
    S32 x0,
    S32 y0,
    S32 x1,
    S32 y1)
{
    int x;
    int y;

    trace_0(TRACE_REALLY, "really_draw_picture()");

    if(!set_graphics_window_from_textarea(x0, y0, x1, y1, TRUE))
        return;

    /* drawing may shuffle core by use of flex - which now upcalls */
    /* list_unlockpools(); */

    set_bg_colour_from_option(COI_BACK);

    #if defined(CLEAR_DRAWFILES_BACKGROUND)
    clear_thistextarea();
    #endif

    /* origin of diagram positioned at r.box.x0,r.box.y1 (abs) */
    x = gcoord_x(x0);
    y = gcoord_y(y1) - p_draw_file_ref->y_size_os - dy; /* <<< do we REALLY want this dy? */

    if(status_fail(draw_do_render(p_draw_diag->data, p_draw_diag->length, x, y, p_draw_file_ref->x_factor, p_draw_file_ref->y_factor, &graphics_window)))
        image_cache_error_set(&p_draw_file_ref->draw_file_key, create_error(ERR_BADDRAWFILE));

    restore_graphics_window();

    /* Draw rendering destroys current graphics & font colour settings */
    killcolourcache();
    setcolours(COI_FORE, COI_BACK);
}

#if 0 /* nobody using this anymore (SKS 25oct96) */

static S32
find_coff(
    COL tcol)
{
    P_SCRCOL cptr = horzvec();
    S32 coff;

    PTR_ASSERT(cptr);

    if(tcol < cptr->colno)
        return(-1);

    coff = 0;

    while(!(cptr->flags & LAST))
    {
        if(tcol == cptr->colno)
            return(coff);

        coff++;
        cptr++;
    }

    return(-2);
}

#endif

static void
maybe_draw_pictures(void)
{
    S32 coff, roff;
    COL tcol;
    ROW trow;
    P_DRAW_DIAG p_draw_diag;
    P_DRAW_FILE_REF p_draw_file_ref;
    S32 x0, x1, y0, y1;
    P_SCRCOL cptr;
    P_SCRROW rptr;

    trace_0(TRACE_MAYBE, "maybe_draw_pictures(): ");

    if(pict_on_screen)
    {
        assert(0 != array_elements(&vertvec_mh));
        rptr = vertvec();
        PTR_ASSERT(rptr);

        for(roff = 0; roff < rowsonscreen; roff++, rptr++)
        {
            assert(vertvec_entry_valid(roff));

            if(rptr->flags & PICT)
            {
                trow = rptr->rowno;

                assert(0 != array_elements(&horzvec_mh));
                cptr = horzvec();
                PTR_ASSERT(cptr);

                for(coff = 0; !(cptr->flags & LAST); coff++, cptr++)
                {
                    assert(horzvec_entry_valid(coff));

                    tcol = cptr->colno;

                    if(draw_find_file(current_docno(), tcol, trow, &p_draw_diag, &p_draw_file_ref))
                    {
                        trace_2(TRACE_APP_PD4, "found picture at col %d, row %d", tcol, trow);

                        x0 = calcad(coff /*find_coff(p_draw_file_ref->col)*/);
                        x1 = x0 + tsize_x(p_draw_file_ref->x_size_os + 4); /* SKS after PD 4.11 09feb92 - make pictures oversize for redraw consideration */
                        y1 = calrad(roff) - 1;      /* NB. picture hangs from top */
                        y0 = y1 + tsize_y(p_draw_file_ref->y_size_os + 4);
                        y0 = MIN(y0, paghyt);

                        trace_6(TRACE_APP_PD4, "x_size = %d, y_size = %d; (%d %d %d %d)",
                                tsize_x(p_draw_file_ref->x_size_os), tsize_y(p_draw_file_ref->y_size_os), x0, y0, x1, y1);

                        if(textobjectintersects(x0, y0, x1, y1))
                        {
                            draw_screen_donePict = 1;

                            really_draw_picture(p_draw_diag, p_draw_file_ref, x0, y0, x1, y1);

                            /* reload as drawing may shuffle core */
                            /* cptr = horzvec_entry(coff); */
                            /* rptr = vertvec_entry(roff); */
                        }
                    }
                }
            }
        }
    }
    else
        trace_0(TRACE_APP_PD4, "pict_on_screen = 0");
}

/******************************************************************************
*
* display slot_in_buffer if there is one. Either in place or in expression
*
******************************************************************************/

static void
draw_cell_in_buffer(void)
{
    S32 fwidth_ch;
    GR_MILLIPOINT fwidth_mp, swidth_mp;
    char paint_str[PAINT_STRSIZ];

    if(slot_in_buffer)
    {
        if(!output_buffer)
        {
            trace_2(TRACE_APP_PD4, "draw_cell_in_buffer: lescrl %d, old_lescrl %d", lescrl, old_lescroll);

            /* if scroll position is different, we must output */
            if(lescrl != old_lescroll)
                output_buffer = TRUE;
            else
            {
                /* if scroll position will be different, we must output */
                fwidth_ch = (xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                                    ? pagwid_plus1
                                    : limited_fwidth_of_slot_in_buffer();

                if(--fwidth_ch < 0)     /* rh scroll margin */
                    fwidth_ch = 0;

                trace_2(TRACE_APP_PD4, "draw_cell_in_buffer: fwidth_ch %d, lecpos %d", fwidth_ch, lecpos);

                if(riscos_fonts  &&
                   !(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  &&
                   fwidth_ch
                  )
                {
                    /* fonty cal_lescrl a little more complex */

                    trace_0(TRACE_APP_PD4, "draw_cell_in_buffer: fonty_cal_lescrl(fwidth)");

                    /* is the caret off the left of the field? (scroll margin one char) */
                    if(lecpos <= lescrl)
                    {
                        if(lescrl)
                            /* off left - will need to centre caret in field */
                            output_buffer = TRUE;
                    }
                    else
                    {
                        /* is the caret off the right of the field? (scroll margin one char) */
                        fwidth_mp = cw_to_millipoints(fwidth_ch);

                        expand_current_slot_in_fonts(paint_str, TRUE, NULL);
                        swidth_mp = font_width(paint_str);

                        trace_2(TRACE_APP_PD4, "draw_cell_in_buffer: fwidth_mp %d, swidth_mp %d", fwidth_mp, swidth_mp);

                        if(swidth_mp > fwidth_mp)
                            /* off right - will need to right justify */
                            output_buffer = TRUE;
                    }
                }
                else
                {
                    trace_1(TRACE_APP_PD4, "draw_cell_in_buffer: cal_lescrl(fwidth) %d", cal_lescrl(fwidth_ch));

                    if(cal_lescrl(fwidth_ch) != lescrl)
                        output_buffer = TRUE;
                }
            }
        }

        trace_1(TRACE_APP_PD4, "draw_cell_in_buffer: output_buffer = %s", report_boolstring(output_buffer));

        if(output_buffer)
        {
            if(!(xf_inexpression || xf_inexpression_box || xf_inexpression_line))
                draw_cell_in_buffer_in_place();

            old_lescroll = lescrl;
            old_lecpos = lecpos;    /* for movement checks */
            output_buffer = FALSE;
        }
    }
}

/******************************************************************************
*
* cell is being edited in place
*
******************************************************************************/

static void
draw_cell_in_buffer_in_place(void)
{
    S32 x0 = calcad(curcoloffset);
    S32 x1;
    S32 y0 = calrad(currowoffset);
    S32 y1 = y0 - 1;
    S32 fwidth = colwidth(curcol);
    S32 overlap = chkolp(travel_here(), curcol, currow);
    S32 dead_text;

    trace_0(TRACE_DRAW, "\n*** draw_cell_in_buffer_in_place()");

    adjust_lescrl(x0, overlap);

    fwidth = MAX(fwidth, overlap);

    x1 = x0 + fwidth;

    dead_text = cal_dead_text();

    x0 += dead_text;

    riscos_caret_hide();
    please_redraw_textarea(x0, y0, x1, y1);
    riscos_caret_restore();
}

/******************************************************************************
*
*           display rows (numbers and contents) across screen
*
******************************************************************************/

#define os_if_fonts(nchars) (riscos_fonts ? cw_to_os(nchars) : (nchars))

/******************************************************************************
*
* get screen redrawn from row roff
*
******************************************************************************/

static void
draw_screen_below(
    S32 roff)
{
    S32 i_roff = roff;

    out_below = FALSE; /* reset my redraw flag */

    trace_1(TRACE_DRAW, "\n*** draw_screen_below(%d)", roff);

    while(roff < rowsonscreen)
    {
        draw_row(roff);

        ++roff;

        if( (roff < rowsonscreen) &&
            (draw_screen_timeout() || keyinbuffer()))
        {
            trace_1(TRACE_OUT | TRACE_ANY, "*** draw_screen_below interrupted: leaving out_below set, rowtoend = %d", roff);
            rowtoend = roff;
            xf_interrupted = out_below = TRUE;
            break;
        }
    }

    if(xf_interrupted)
        return;

    /* if we completed the whole redraw from the top, we can't have any SL_ALTERED cells to draw */
    if(i_roff == 0)
        xf_drawsome = FALSE;

    draw_empty_bottom_of_screen();

    rowtoend = rows_available;
}

/* loop down all 'visible' rows, stopping after the bottom partial row
 * (highest roff) in the clipped area has been drawn.
*/

static void
maybe_draw_rows(void)
{
    S32 roff = 0;
    BOOL morebelow;

    while(roff < rowsonscreen)
    {
        morebelow = maybe_draw_row(roff);

        if(!morebelow)
            break;

        ++roff;
    }
}

/* returns FALSE if this line is below the bottom of the cliparea and
 * so no more lines should be considered for redraw.
*/

static BOOL
maybe_draw_row(
    S32 roff)
{
    S32 rpos = calrad(roff);

    trace_2(TRACE_MAYBE, "maybe_draw_row(%d), rpos = %d --- ", roff, rpos);

    /* NB. cliparea.y0 >= rpos > cliparea.y1 is condition we want to draw */

    if(cliparea.y0 < rpos)
    {
        trace_0(TRACE_MAYBE, "row below clipped area");
        return(FALSE);
    }

    if(cliparea.y1 < rpos)
    {
        trace_0(TRACE_MAYBE, "row in clipped area");

        /* limit matched textarea to this line */
        thisarea.y0 = rpos;
        thisarea.y1 = rpos-1;

        maybe_draw_row_border(roff, rpos);
        maybe_draw_row_contents(roff, rpos);
    }
    else
        trace_0(TRACE_MAYBE, "row above clipped area");

    return(TRUE);
}

/* only called if some part of row intersects (y) */

static void
maybe_draw_row_border(
    S32 roff,
    S32 rpos)
{
    /* required to redraw left slop too */

    if(!displaying_borders)
    {
        if(textxintersects(-1, 0))
        {
            clear_thistextarea();

            /* draw first vbar - needs to include hbar */
            at(-1, rpos);
            draw_grid_vbar(TRUE);
        }

        return;
    }

    if(textxintersects(-1, borderwidth))
        really_draw_row_border(roff, rpos);
}

/* the new length that this row should be given (only for update) */
static S32 new_row_length;

static void
maybe_draw_row_contents(
    S32 roff,
    S32 rpos)
{
    if(textxintersects(borderwidth, borderwidth + hbar_length))
    {
        new_row_length = really_draw_row_contents(roff, rpos);
        /* only matters if update, not redraw */

        if(grid_on)
        {
            /* draw last vbar */
            at(borderwidth + hbar_length, rpos);
            draw_grid_vbar(FALSE);

            at(borderwidth, rpos);
            draw_grid_hbar(hbar_length);
        }
    }
}

extern void
draw_row(
    S32 roff)
{
    S32 rpos;

    trace_1(TRACE_DRAW, "\n*** draw_row(%d) --- ", roff);

    assert(vertvec_entry_valid(roff));
    if(vertvec_entry(roff)->flags & LAST)
    {
        trace_0(TRACE_REALLY, "last row");
        return;
    }

    if(roff > rowsonscreen)
    {
        trace_2(TRACE_DRAW, "roff (%d) > rowsonscreen (%d)", roff, rowsonscreen);
        return;
    }

    if( out_rowout   &&  (rowout  == roff))
        out_rowout  = FALSE;

    if( out_rowout1  &&  (rowout1 == roff))
        out_rowout1 = FALSE;

#if 1
    /* SKS 09jan92 - can't understand why but leaving this in is the only way to make it work */
    if(out_rowborout   &&  (rowborout  == roff))
        out_rowborout  = FALSE;

    if(out_rowborout1  &&  (rowborout1 == roff))
        out_rowborout1 = FALSE;
#endif

    rpos = calrad(roff);

    trace_1(TRACE_DRAW, "rpos = %d", rpos);

    /* must get WHOLE line updated as new line may be longer than old */
#if 1
    please_redraw_textarea(         -1, rpos, borderwidth + hbar_length, rpos-1);
#else
    please_redraw_textarea(borderwidth, rpos, borderwidth + hbar_length, rpos-1);
#endif
}

static void
draw_row_border(
    S32 roff)
{
    S32 rpos;

    trace_1(TRACE_DRAW, "\n*** draw_row_border(%d) --- ", roff);

    assert(vertvec_entry_valid(roff));
    if(vertvec_entry(roff)->flags & LAST)
    {
        trace_0(TRACE_REALLY, "last row");
        return;
    }

    if(roff > rowsonscreen)
    {
        trace_2(TRACE_DRAW, "roff (%d) > rowsonscreen (%d)", roff, rowsonscreen);
        return;
    }

    if( out_rowborout   &&  (rowborout  == roff))
        out_rowborout  = FALSE;

    if( out_rowborout1  &&  (rowborout1 == roff))
        out_rowborout1 = FALSE;

    if(!displaying_borders)
        return;

    rpos = calrad(roff);

    trace_1(TRACE_DRAW, "rpos = %d", rpos);

    please_redraw_textarea(-1, rpos, borderwidth, rpos-1);
}

/* only called if displaying_borders on RISC OS */

static void
really_draw_row_border(
    S32 roff,
    S32 rpos)
{
    P_SCRROW rptr;

    trace_1(TRACE_REALLY, "really_draw_row_border(%d): ", roff);

  /*  clear_thistextarea();*/

  /*  at(0, rpos);*/

    assert(vertvec_entry_valid(roff));
    rptr = vertvec_entry(roff);

    /* is the row a soft page break? if RISCOS, draw just that bit of it
     * that is in the row border for faster clipping,
     * otherwise drawn whole by really_draw_row_contents()
    */
    if(rptr->flags & PAGE)
    {
        /* soft page breaks are shown as a bar, right across the page - here we draw the bit */
        /* crossing the border row, the rest is drawn by really_draw_row_contents            */

        set_bg_colour_from_option(COI_SOFTPAGEBREAK);
        clear_thistextarea();
    }
    else
    {
        ROW trow = rptr->rowno;
        /* We plot the row number in two parts, so that the icon borders overlap */
        /* instead of touching, and so the border can be a unique colour.        */
        WimpIconBlock number;
        WimpIconBlock border;

        COLOURS_OPTION_INDEX fg_colours_option_index, bg_colours_option_index;

        if(trow == currow)
        {
            fg_colours_option_index = COI_CURRENT_BORDERFORE;
            bg_colours_option_index = COI_CURRENT_BORDERBACK;
        }
        else
        {
            fg_colours_option_index = COI_BORDERFORE;
            bg_colours_option_index = COI_BORDERBACK;

            if(rptr->flags & FIX)
                bg_colours_option_index = COI_FIXED_BORDERBACK;
        }

        border.bbox.xmin = texttooffset_x(-1);             /* -1 character, so icon is clipped at the window edge, */
        number.bbox.xmin = border.bbox.xmin;

        border.bbox.ymin = texttooffset_y(rpos);
        number.bbox.ymin = border.bbox.ymin + dy;

        border.bbox.xmax = texttooffset_x(borderwidth);    /*  with no left hand border */
        number.bbox.xmax = border.bbox.xmax - dx;

        number.bbox.ymax = border.bbox.ymin + charvspace;
        border.bbox.ymax = number.bbox.ymax + dy;

        border.flags = (int) (
            wimp_IBORDER | (WimpIcon_FGColour * wimp_colour_index_from_option(COI_GRID)) ); /*COI_BORDERFORE*/

        number.flags = (int) (
            wimp_ITEXT | wimp_IRJUST |
            wimp_IVCENTRE |
            /* */          (WimpIcon_FGColour * wimp_colour_index_from_option(fg_colours_option_index)) |
            wimp_IFILLED | (WimpIcon_BGColour * wimp_colour_index_from_option(bg_colours_option_index)) );

        consume_int(sprintf(number.data.t, "%d", (int) trow + 1));

        if( !WrapOsErrorReporting_IsError(tbl_wimp_plot_icon(&number)) )
        {
            if( !WrapOsErrorReporting_IsError(tbl_wimp_plot_icon(&border)) )
            {   /* draw first vbar - needs to include hbar */
                if(grid_on)
                    draw_grid_vbar(TRUE);
            }
        }
    }

    killcolourcache(); /* cos Wimp_PlotIcon changes foreground and background colours */
    setcolours(COI_FORE, COI_BACK);
}

/* only called if displaying_borders on RISC OS */

static void
really_draw_extended_row_border(void)
{
    S32 start = calrad(rowsonscreen);
    WimpIconBlock border;

    trace_3(TRACE_REALLY, "really_draw_extended_row_border - rowsonscreen=%d, start=%d, paghyt=%d", rowsonscreen, start, paghyt);

    border.bbox.xmin = texttooffset_x(-1);             /* -1 character, so icon is clipped at the window edge, */
    border.bbox.ymin = texttooffset_y(paghyt+1) - dy;
    border.bbox.xmax = texttooffset_x(borderwidth);    /*  with no left hand border */
    border.bbox.ymax = texttooffset_y(start-1)  + dy;

    border.flags = (int) (
        wimp_IBORDER | (WimpIcon_FGColour * wimp_colour_index_from_option(COI_GRID)) |
        wimp_IFILLED | (WimpIcon_BGColour * wimp_colour_index_from_option(COI_BORDERBACK)) );

    /* border.data unused */

    void_WrapOsErrorReporting(tbl_wimp_plot_icon(&border));

    killcolourcache(); /* cos Wimp_PlotIcon changes foreground and background colours */
    setcolours(COI_FORE, COI_BACK);
}

static S32
really_draw_row_contents(
    S32 roff,
    S32 rpos)
{
    P_SCRROW rptr;
    S32 newlen, len;
    S32 coff;
    char tbuf[32];

    trace_1(TRACE_REALLY, "really_draw_row_contents(%d): ", roff);

    assert(vertvec_entry_valid(roff));
    rptr = vertvec_entry(roff);

    /* is the row a soft (automatic) page break? */

    if(rptr->flags & PAGE)
    {
        /* soft page breaks are shown as a bar, right across the page - here we draw the bit */
        /* that crosses the columns, the left-hand bit is drawn by really_draw_row_border    */

        len = MIN(hbar_length, RHS_X - borderwidth);
        trace_0(TRACE_REALLY, "row has soft page break");

        set_bg_colour_from_option(COI_SOFTPAGEBREAK);
        clear_thistextarea();
        set_bg_colour_from_option(COI_BACK);

        newlen = os_if_fonts(len);
    }
    else if(chkrpb(rptr->rowno))
    {
        if(chkfsb()  &&  chkpac(rptr->rowno))
        {
            /* conditional page break occurred */
            /* shown as a bar crossing the columns, the row border has the usual row number */

            len = MIN(hbar_length, RHS_X - borderwidth);
            trace_0(TRACE_REALLY, "row has good hard page break");

            set_bg_colour_from_option(COI_HARDPAGEBREAK);
            clear_thistextarea();
          /*set_bg_colour_from_option(COI_BACK); done later on */

            newlen = os_if_fonts(len);
        }
        else
        {
            /* conditional page break not breaking              */
            /* shown as '~ nnn' in text foreground break colour */

            len = sprintf(tbuf, "~ %d", travel(0, rptr->rowno)->content.page.condval);
            trace_1(TRACE_REALLY, "row has failed hard page break %s", tbuf);
            at(borderwidth, rpos);
            set_fg_colour_from_option(COI_FORE); /* When first asked, Rob wanted COI_PAGEBREAK, then he */
            clear_underlay(len);                 /* changed his mind, if he changes it again, thump him */
            stringout(tbuf);
            newlen = os_if_fonts(len);
            trace_1(TRACE_REALLY, "length of line so far: %d", newlen);
        }

        setcolours(COI_FORE, COI_BACK); /* cos we've altered fg or bg colour */
    }
    else
    {
        trace_0(TRACE_REALLY, "normal row: drawing cells");

        at(borderwidth, rpos);

        newlen = 0;     /* maybe nothing drawn! */

        assert(horzvec_entry_valid(0));
        for(coff = 0; !(horzvec_entry(coff)->flags & LAST); coff++)
        {
            newlen = really_draw_cell(coff, roff, TRUE);
            invoff();
        }

        newlen -= os_if_fonts(borderwidth);
    }

    trace_0(TRACE_REALLY, "clear crud off eol");
    ospca_fonts(os_if_fonts(hbar_length) - newlen);

    return(newlen);
}

static void
draw_cell(
    S32 coff,
    S32 roff,
    BOOL in_draw_row)
{
    S32 cpos = calcad(coff);
    S32 rpos = calrad(roff);
    COL tcol;
    ROW trow;
    P_CELL tcell;
    S32 fwidth;
    S32 overlap;

    assert(horzvec_entry_valid(coff));
    assert(vertvec_entry_valid(roff));
    tcol = col_number(coff);
    trow = row_number(roff);

    fwidth = colwidth(tcol);

    tcell = travel(tcol, trow);

    UNREFERENCED_PARAMETER(in_draw_row);    /* NEVER in_draw_row in RISC OS */

    trace_2(TRACE_DRAW, "\n*** draw_cell(%d, %d)", coff, roff);

    if(tcell  &&  slot_displays_contents(tcell))
    {
        overlap = chkolp(tcell, tcol, trow);
        fwidth = MAX(fwidth, overlap);
    }

    please_redraw_textarea(cpos, rpos, cpos + fwidth, rpos-1);
}

/******************************************************************************
*
* returns new screen column
*
******************************************************************************/

static S32
really_draw_cell(
    S32 coff,
    S32 roff,
    BOOL in_draw_row)
{
    S32 cpos = calcad(coff);
    S32 rpos = calrad(roff);
    COL tcol;
    P_SCRROW rptr;
    ROW trow;
    P_CELL thisslot;
    BOOL slotblank;
    S32 os_cpos;
    S32 c_width;
    S32 fwidth, overlap;
    S32 widthofslot, spaces_at_end, spaces_to_draw;
    S32 end_of_inverse, to_do_inverse;
    BOOL slot_in_block, slot_visible;

    static S32 last_offset;        /* last draw_cell finished here */

    assert(horzvec_entry_valid(coff));
    tcol = col_number(coff);

    assert(vertvec_entry_valid(roff));
    rptr = vertvec_entry(roff);
    trow = rptr->rowno;

    thisslot = travel(tcol, trow);
    slotblank = is_blank_cell(thisslot);

    os_cpos = os_if_fonts(cpos);
    c_width = colwidth(tcol);

    trace_4(TRACE_REALLY, "really_draw_cell(%d, %d, %s): slotblank = %s",
                coff, roff, report_boolstring(in_draw_row),
                report_boolstring(slotblank));

    /* if at beginning of line reset last offset to bos */

    if((coff == 0)  ||  !in_draw_row)
    {
        last_offset = os_cpos;
        trace_1(TRACE_REALLY, "last_offset := %d", last_offset);
    }

    /* special case for blank, partially overlapped cells.
     * just need to space to the end of the column
    */
    if(slotblank  &&  (last_offset > os_cpos))
    {
        /* c_width is column width truncated to eos */
        if( c_width > RHS_X - cpos)
            c_width = RHS_X - cpos;

        spaces_to_draw = os_cpos + os_if_fonts(c_width) - last_offset;

        trace_1(TRACE_REALLY, "blank, partially overlapped cell: spaces_to_draw := %d", spaces_to_draw);

        if(spaces_to_draw > 0)          /* last cell partially overlaps */
        {
            at_fonts(last_offset, rpos);
            (void) setivs(tcol, trow);
        /*  setprotectedstatus(NULL); */
            ospca_fonts(spaces_to_draw);
            last_offset += spaces_to_draw;
            if(grid_on)
                draw_grid_vbar(FALSE);
        }

        return(last_offset);
    }

    /* possible overlap state */
    overlap = chkolp(thisslot, tcol, trow);

    /* if column width > overlap draw some spaces at end */
    spaces_at_end = c_width - overlap;

    if(slotblank  ||  slot_displays_contents(thisslot)  ||  (spaces_at_end > 0))
    {
        /* draw blanks & text to overlap position, spaces (>=0) beyond */
        fwidth = overlap;
        spaces_at_end = MAX(spaces_at_end, 0);
    }
    else
    {
        fwidth = c_width;
        spaces_at_end = 0;
    }

    /* we are drawing to last_offset, which is the value returned */

    last_offset = os_cpos + os_if_fonts(fwidth + spaces_at_end);

    /* stored page number invalid if silly cases */
    curpnm = (!encpln || n_rowfixes) ? 0 : rptr->page;

    /* switch inverse on if in block (or on number cell?) */

    slot_in_block = setivs(tcol, trow);
    setprotectedstatus(thisslot);

    /* set window covering cell to prevent bits of fonts sticking out:
     * if no window set then nothing to plot - hoorah!
     * this may even help system font plotting
    */
    slot_visible = set_graphics_window_from_textarea(cpos, rpos,
                                                     cpos + fwidth + spaces_at_end, rpos - 1,
                                                     TRUE /*riscos_fonts <<< SKS*/);

    if(!slot_visible)
    {
        at(cpos, rpos);
        widthofslot = 0;
    }
    else
    {
        /* this is current cell to be drawn by dspfld
         * don't bother drawing current cell if not number
        */
        if( (tcol == curcol)  &&  (trow == currow)  &&  !xf_blockcursor  &&
            (!thisslot  ||
             ((thisslot->type == SL_TEXT)  &&  !(thisslot->justify & PROTECTED)) ||
             (thisslot->type == SL_PAGE)))
        {
            widthofslot = dspfld(cpos, rpos, fwidth);

            if(riscos_fonts)
                /* convert from millipoints to OS units, rounding out to pixels */
                widthofslot = idiv_ceil_fn(widthofslot, millipoints_per_os_x * dx) * dx;
        }
        else if(slotblank)
        {
            at(cpos, rpos);
            widthofslot = 0;
        }
        else
        {
            if( thisslot->flags & SL_ALTERED)
                thisslot->flags = SL_ALTERED ^ thisslot->flags;

            if(result_sign(thisslot) < 0)
            {
                (currently_inverted ? set_bg_colour_from_option : set_fg_colour_from_option) (COI_NEGATIVE);
            }

            if(riscos_fonts)
            {
                /* position font output */
                screen_xad_os = gcoord_x(cpos);
                screen_yad_os = gcoord_y(rpos);

                riscos_font_ad_millipoints_x = millipoints_per_os_x * (screen_xad_os);
                riscos_font_ad_millipoints_y = millipoints_per_os_y * (screen_yad_os + fontbaselineoffset);

                ensurefontcolours();

                widthofslot = outslt(thisslot, trow, fwidth);

                /* convert from millipoints to OS units, rounding out to pixels */
                widthofslot = idiv_ceil_fn(widthofslot, millipoints_per_os_x * dx) * dx;

                trace_1(TRACE_APP_PD4, "widthofslot = %d (OS)", widthofslot);
            }
            else
            {
                at(cpos, rpos);

                widthofslot = outslt(thisslot, trow, fwidth);

                trace_1(TRACE_APP_PD4, "widthofslot = %d (CH)", widthofslot);
            }
        }
    }

    /* now get fwidth & cpos into os units */

    os_cpos += widthofslot;
    fwidth = os_if_fonts(fwidth + spaces_at_end) - widthofslot;

    if(slot_visible)
        /* reset graphics window to paint end of line */
        restore_graphics_window();

    /* still have to output fwidth spaces, some of them inverse.
     * note that maybe both in block and in expression, therefore being
     * in block doesn't imply we are drawing in inverse

     * need to draw spaces to end of block,
     * toggle inverse and draw spaces to fwidth
    */
    end_of_inverse = end_of_block(os_cpos, trow);
    to_do_inverse  = end_of_inverse - os_cpos;

    trace_3(TRACE_APP_PD4, "end_of_inverse %d, os_cpos %d -> to_do_inverse %d",
            end_of_inverse, os_cpos, to_do_inverse);

    if(to_do_inverse > 0)
    {
        to_do_inverse = MIN(to_do_inverse, fwidth);
        if(slot_visible)
        {
            at_fonts(os_cpos, rpos);
            draw_spaces_with_grid(to_do_inverse, os_cpos);
        }
        os_cpos += to_do_inverse;
        fwidth -= to_do_inverse;
    }

    if(slot_in_block)
        invert();

    at_fonts(os_cpos, rpos);
    draw_spaces_with_grid(fwidth, os_cpos);

    switch_off_highlights();
    setcolours(COI_FORE, COI_BACK);

    return(last_offset);            /* new screen x address */
}

/******************************************************************************
*
* draw spaces on screen from position start, poking in grid if necessary
*
******************************************************************************/

static void
draw_spaces_with_grid(
    S32 spaces_left,
    S32 start)
{
    COL  tcol;
    S32 coff;
    S32 c_width, end;
    BOOL last;

    trace_2(TRACE_REALLY, "draw_spaces_with_grid(%d, %d)",
                spaces_left, start);

    if(grid_on)
    {
        /* where we will draw the last grid vbar */
        end = start + spaces_left;

        /* which column are we starting to draw spaces in? */
        coff = calcoff(riscos_fonts ? start / charwidth : start);

        if(horzvec_entry_valid(coff))
        {
            tcol = col_number(coff);
            c_width = os_if_fonts(colwidth(tcol)) - (start - os_if_fonts(calcad(coff)));
            trace_2(TRACE_APP_PD4, "c_width(%d) = %d", tcol, c_width);
            last = FALSE;
        }
        else
        {
            c_width = 0; /* SKS 10.10.91 */
            last = TRUE;
        }

        while(spaces_left >= 0)
        {
            trace_1(TRACE_APP_PD4, "spaces_left = %d", spaces_left);

            if(last)
            {
                trace_0(TRACE_APP_PD4, "last column hit - space to end");
                ospca_fonts(spaces_left);
                /* don't draw grid vbar as we are given spastic (but correct)
                 * widths for right margin positions beyond last column end
                */
                break;
            }

            if( c_width > spaces_left)
                c_width = spaces_left;

            if(c_width)
            {
                ospca_fonts(c_width);
                spaces_left -= c_width;

                draw_grid_vbar(FALSE);
            }

            coff++;

            assert(horzvec_entry_valid(coff));
            last = (horzvec_entry(coff)->flags & LAST);

            if(!last  &&  (spaces_left > 0))
            {
                assert(horzvec_entry_valid(coff));
                tcol = col_number(coff);
                c_width = os_if_fonts(colwidth(tcol));

                trace_2(TRACE_APP_PD4, "c_width(%d) = %d", tcol, c_width);
            }
        }
    }
    else
        ospca_fonts(spaces_left);
}

#endif

/******************************************************************************
*
* if mark_row has marked a soft page break
* inadvertenly, draw page row and next row down
*
******************************************************************************/

static S32
adjust_rowout(
    S32 rowoff)
{
    uchar flags;
    
    assert(vertvec_entry_valid(rowoff));
    flags = vertvec_entry(rowoff)->flags;

    if((flags & PAGE)  &&  !(flags & LAST))
        draw_row(rowoff + 1);

    return(rowoff);
}

static S32
adjust_rowborout(
    S32 rowoff)
{
    uchar flags;

    assert(vertvec_entry_valid(rowoff));
    flags = vertvec_entry(rowoff)->flags;

    if((flags & PAGE)  &&  !(flags & LAST))
        draw_row_border(rowoff + 1);

    return(rowoff);
}

static void
check_output_current_slot(
    S32 move)
{
    if(out_currslot)
    {
        if(atend(curcol, currow))
            mark_to_end(currowoffset);
        else
            mark_slot(travel_here());
    }

    if(!slot_in_buffer  ||  move)
        colh_draw_contents_of_number_cell();

    if( out_screen  ||
        out_below   ||
        (out_rowout  &&  (rowout  == currowoffset)) ||
        (out_rowout1 &&  (rowout1 == currowoffset)) )
    {
        output_buffer = TRUE;
        dspfld_from = -1;
    }

    filbuf();
}

/******************************************************************************
*
* set inverse state
* Varies if current number cell and in marked block
*
******************************************************************************/

static BOOL
setivs(
    COL tcol,
    ROW trow)
{
    BOOL invert_because_block_cursor = (xf_blockcursor   &&
                                        (tcol == curcol) &&
                                        (trow == currow)
                                       );
    BOOL invert_because_inblock = inblock(tcol, trow);

    if(invert_because_inblock ^ invert_because_block_cursor)
        invert();

    return(invert_because_inblock);
}

/******************************************************************************
*
* if xf_drawsome, some cells need redrawing.
* Scan through horzvec and vertvec
* drawing the ones that have changed
*
******************************************************************************/

static void
draw_altered_cells(void)
{
    P_SCRCOL i_cptr;
    P_SCRROW i_rptr;
    P_SCRCOL cptr;
    P_SCRROW rptr;
    S32 coff;
    S32 roff;
    P_CELL tcell;

    trace_0(TRACE_DRAW, "\n*** draw_altered_cells()");

    assert(0 != array_elements(&horzvec_mh));
    i_cptr = horzvec();
    PTR_ASSERT(i_cptr);

    assert(0 != array_elements(&vertvec_mh));
    i_rptr = vertvec();
    PTR_ASSERT(i_rptr);

    rptr = i_rptr;

    while(!(rptr->flags & LAST))
    {
        if(!(rptr->flags & PAGE))
        {
            cptr = i_cptr;

            while(!(cptr->flags & LAST))
            {
                tcell = travel(cptr->colno, rptr->rowno);

                if(tcell)
                {
                    if(tcell->type == SL_PAGE)
                        break;

                    if(tcell->flags & SL_ALTERED)
                    {
                        coff = (S32) (cptr - i_cptr);
                        roff = (S32) (rptr - i_rptr);

                        draw_cell(coff, roff, FALSE);

                        assert(0 != array_elements(&horzvec_mh));
                        i_cptr  = horzvec();
                        PTR_ASSERT(i_cptr);

                        assert(0 != array_elements(&vertvec_mh));
                        i_rptr  = vertvec();
                        PTR_ASSERT(i_rptr);

                        assert(horzvec_entry_valid(coff));
                        cptr = i_cptr + coff;

                        assert(vertvec_entry_valid(roff));
                        rptr = i_rptr + roff;

                        invoff();
                    }

                    if(draw_screen_timeout() || keyinbuffer())
                    {
                        trace_0(TRACE_OUT | TRACE_ANY, "*** draw_altered_cells interrupted - leaving xf_drawsome set");
                        xf_interrupted = TRUE;
                        return;
                    }
                }

                cptr++;
            }
        }

        rptr++;
    }

    xf_drawsome = FALSE;        /* only reset when all are done */
}

/******************************************************************************
*
* draw one cell, given the cell reference
* called just from evaluator for
* interruptability
*
******************************************************************************/

extern void
draw_one_altered_slot(
    COL col,
    ROW row)
{
    P_SCRCOL i_cptr;
    P_SCRROW i_rptr;
    P_SCRCOL cptr;
    P_SCRROW rptr;
    P_CELL tcell;

    trace_0(TRACE_DRAW, "\n*** draw_one_altered_slot()");

    assert(0 != array_elements(&horzvec_mh));
    i_cptr = horzvec();
    PTR_ASSERT(i_cptr);

    assert(0 != array_elements(&vertvec_mh));
    i_rptr = vertvec();
    PTR_ASSERT(i_rptr);

    cptr = i_cptr;
    rptr = i_rptr;

    /* cheap tests to save loops if cell not in window - fixing complicates */
    if( !(rptr->flags & FIX)  &&
        ((rptr->rowno > row)  ||  (rptr[rowsonscreen-1].rowno < row))
        )
        return;

    if( !(cptr->flags & FIX)  &&
        (cptr->colno > col)
        )
        return;

    /* no need to reload pointers as only one cell is being drawn */
    while(!(rptr->flags & LAST))
    {
        if((rptr->rowno == row)  &&  !(rptr->flags & PAGE))
            while(!(cptr->flags & LAST))
            {
                if(cptr->colno == col)
                {
                    tcell = travel(cptr->colno, rptr->rowno);

                    if(tcell)
                    {
                        draw_cell((S32)(cptr - i_cptr),
                                  (S32)(rptr - i_rptr), FALSE);

                        invoff();
                    }

                    return;
                }

                cptr++;
            }

        rptr++;
    }
}

/******************************************************************************
*
*  clear to bottom of screen from end of sheet
*
******************************************************************************/

static void
draw_empty_bottom_of_screen(void)
{
    S32 start = calrad(rowsonscreen);

    trace_0(TRACE_DRAW, "\n*** draw_empty_bottom_of_screen()");

    if(start > paghyt)
    {
        trace_2(TRACE_DRAW, "--- start (%d) > paghyt (%d) ", start, paghyt);
        return;
    }

    /* may be overlapped by a Draw file - redraw not clear */
    please_redraw_textarea(-1, paghyt, RHS_X, start-1);
}

static void
maybe_draw_empty_bottom_of_screen(void)
{
    S32 start = calrad(rowsonscreen);

    trace_0(TRACE_MAYBE, "maybe_draw_empty_bottom_of_screen() --- ");

    if(start > paghyt)
    {
        trace_2(TRACE_MAYBE, "start (%d) > paghyt (%d) ", start, paghyt);
    }
    else
    {
        trace_0(TRACE_MAYBE, "some bottom on screen");

        /* required to redraw left slop too */

        if(textobjectintersects(-1, paghyt, RHS_X, start-1))
        {
            trace_0(TRACE_REALLY, "really_draw_empty_bottom_of_screen()");

            clear_thistextarea();

            if(displaying_borders)
                really_draw_extended_row_border();
        }
    }
}

static void
maybe_draw_unused_bit_at_bottom(void)
{
    if(unused_bit_at_bottom)
    {
        trace_0(TRACE_MAYBE, "maybe_draw_unused_bit_at_bottom()");

        /* required to redraw left slop too */

        if(textobjectintersects(-1, paghyt+1, RHS_X, paghyt))
        {
            trace_0(TRACE_REALLY, "really_draw_unused_bit_at_bottom()");

            /* may have been overlapped by a Draw file */
            clear_thistextarea();

            if(displaying_borders)
                really_draw_extended_row_border();
        }
    }
}

/******************************************************************************
*
* clear to right of screen from rightmost right margin
*
******************************************************************************/

static void
draw_empty_right_of_screen(void)
{
    S32 x0 = borderwidth + hbar_length;
    S32 x1 = RHS_X;

    xf_draw_empty_right = 0;

    /* required to redraw one more raster if grid below editing line */

    trace_0(TRACE_DRAW, "*** draw_empty_right_of_screen()");

    /* may be overlapped by a Draw file - redraw not clear */
    /* NB. right of screen may not be visible */
    if(x1 > x0)
    {
        BOOL grid_below = !displaying_borders  &&  grid_on;
        S32 grid_adjust = grid_below ? 1 : 0;
        S32 y0 = calrad(rowsonscreen) - 1;
        S32 y1 = calrad(0) - 1 - grid_adjust;

        please_redraw_textarea(x0, y0, x1, y1);
    }
}

static void
maybe_draw_empty_right_of_screen(void)
{
    BOOL grid_below = !displaying_borders  &&  grid_on;
    S32 grid_adjust = grid_below ? 1 : 0;
    S32 x0 = borderwidth + hbar_length;
    S32 y0 = calrad(rowsonscreen) - 1;
    S32 x1 = RHS_X;
    S32 y1 = calrad(0) - 1 - grid_adjust;

    /* required to redraw one more raster if grid below editing line */

    if(textobjectintersects(x0, y0, x1, y1))
    {
        trace_0(TRACE_REALLY, "really_draw_empty_right_of_screen()");

        clear_textarea(thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1 + grid_adjust,
                       grid_below);
    }
}

/******************************************************************************
*
* expand the cell and call the appropriate justification routine to print it
*
* fwidth maximum width of screen available
* returns number of characters drawn or number of millipoints
*
******************************************************************************/

extern S32 /* GR_MILLIPOINT iff riscos_fonts */
outslt(
    P_CELL tcell,
    ROW trow,
    S32 fwidth)
{
    /* need lots more space cos of fonty bits in RISCOS */
    uchar array[PAINT_STRSIZ];
    uchar justify;
    S32 res;

    trace_3(TRACE_APP_PD4, "outslt: cell &%p, row %d, fwidth %d", report_ptr_cast(tcell), trow, fwidth);

    justify = expand_cell(
                    current_docno(), tcell, trow, array, fwidth,
                    DEFAULT_EXPAND_REFS /*expand_refs*/,
                    EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                    EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ |
                    EXPAND_FLAGS_FONTY_RESULT(riscos_fonts) /*allow_fonty_result*/ /*expand_flags*/,
                    TRUE /*cff*/);

    switch(justify)
    {
    case J_LCR:
        if(riscos_fonts)
            return(lcrjust_riscos_fonts(array, fwidth, FALSE));

        return(lcrjust_plain(array, fwidth, FALSE));

    case J_LEFTRIGHT:
    case J_RIGHTLEFT:
        if(riscos_fonts)
            return(font_paint_justify(array, fwidth));

        res = justifyline(array, fwidth, justify, NULL);
        microspacing = FALSE;
        return(res);

    default:
        if(riscos_fonts)
            return(onejst_riscos_fonts(array, fwidth, justify));

        return(onejst_plain(array, fwidth, justify));
    }
}

/******************************************************************************
*
* set up a font rubout box given swidth and current pos
*
* NB. Only for screen use!
*
******************************************************************************/

static void
font_setruboutbox(
    GR_MILLIPOINT swidth_mp)
{
    S32 swidth_os = idiv_ceil_fn(swidth_mp, millipoints_per_os_x * dx) * dx; /* round out to pixels */

    trace_1(TRACE_APP_PD4, "font_ruboutbox: swidth_os = %d", swidth_os);

    wimpt_safe(bbc_move(screen_xad_os, screen_yad_os + (grid_on ? dy : 0)));
    wimpt_safe(os_plot(bbc_MoveCursorRel, swidth_os, +charvrubout_pos +charvrubout_neg));
}

/******************************************************************************
*
*  output one justified string
*
******************************************************************************/

static S32
onejst_plain(
    uchar *str,
    S32 fwidth_ch,
    uchar type)
{
    /* leave one char space to the right in some cases */
    const S32 fwidth_adjust_ch = ((type == J_CENTRE)  ||  (type == J_RIGHT)) ? 1 : 0;
    S32 spaces, swidth_ch;
    uchar *ptr, ch;

    assert(!riscos_fonts);

    fwidth_ch -= fwidth_adjust_ch;

    /* non-fonty code */

    /* ignore leading and trailing spaces */

    if(type != J_FREE)
    {
        --str;
        do { ch = *++str; } while(ch == SPACE);
    }

    /* find end of string */
    ptr = str + strlen(str); /* NB this is a plain string */

    /* strip trailing spaces */
    do { ch = *--ptr; } while(ch == SPACE);

    /* reinstate funny space put on by sprintnumber */
    if(ch == FUNNYSPACE)
        *ptr = SPACE;

    *++ptr = CH_NULL;

    swidth_ch = calsiz(str);

    spaces = 0;

    if(swidth_ch > fwidth_ch)
        /* fill as much field as possible if too big */
        fwidth_ch += fwidth_adjust_ch;
    else
        switch(type)
        {
        default:
            trace_0(TRACE_APP_PD4, "*** error in OneJst ***");

        case J_FREE:
        case J_LEFT:
            break;

        case J_RIGHT:
            spaces = fwidth_ch - swidth_ch;
            break;

        case J_CENTRE:
            spaces = (fwidth_ch - swidth_ch) >> 1;
            break;
        }

    if(spaces > 0)
        ospca(spaces);

    return(spaces + strout(str, fwidth_ch));
}

#ifndef font_KERN /* RISC_OSLib does define this - but only at tag RISC_OSLib-5_77 (Mar 2013)! */
#define font_KERN 0x200  /* perform kerning on the plot */
#endif

static GR_MILLIPOINT
onejst_riscos_fonts(
    uchar *str,
    S32 fwidth_ch,
    uchar type)
{
    /* leave one char space to the right in some cases */
    const S32 fwidth_adjust_ch = ((type == J_CENTRE)  ||  (type == J_RIGHT)) ? 1 : 0;
    GR_MILLIPOINT fwidth_mp, swidth_mp, x_mp, xx_mp;
    char paint_buf[PAINT_STRSIZ], *paint_str;
    S32 paint_op = font_ABS;

    assert(riscos_fonts);

    fwidth_ch -= fwidth_adjust_ch;

    if(type != J_FREE)
    {
        font_strip_spaces(paint_buf, str, NULL);
        paint_str = paint_buf;
    }
    else
        paint_str = str;

    swidth_mp = font_width(paint_str);

    fwidth_mp = cw_to_millipoints(fwidth_ch);

    if(swidth_mp > fwidth_mp)
    {
        /* fill as much field as possible if too big */
        x_mp  = 0;
        xx_mp = font_truncate(paint_str, fwidth_mp + cw_to_millipoints(fwidth_adjust_ch));
    }
    else
    {
        switch(type)
        {
        default:
            trace_0(TRACE_APP_PD4, "*** error in OneJst ***");

        case J_FREE:
        case J_LEFT:
            x_mp  = 0;
            xx_mp = swidth_mp;
            break;

        case J_RIGHT:
            x_mp  = fwidth_mp - swidth_mp;
            xx_mp = fwidth_mp;
            break;

        case J_CENTRE:
            x_mp  = (fwidth_mp - swidth_mp) / 2;
            xx_mp = fwidth_mp;  /* make centred fields fill field (for grid) */
            break;
        }
    }

    if(draw_to_screen)
    {
        font_setruboutbox(xx_mp);

        paint_op |= font_RUBOUT;
    }
    if('Y' == d_options_KE)
    {
        paint_op |= font_KERN;
    }

    trace_2(TRACE_APP_PD4, "onejst_riscos_fonts font_paint x: %d, y: %d",
            riscos_font_ad_millipoints_x + x_mp, riscos_font_ad_millipoints_y);

    font_complain(font_paint(paint_str, paint_op,
                             riscos_font_ad_millipoints_x + x_mp, riscos_font_ad_millipoints_y));

    return(xx_mp);
}

/******************************************************************************
*
* get rid of leading and trailing spaces in a plain non-fonty string
* puts CH_NULL at end and returns first non-space char
*
******************************************************************************/

static uchar *
trim_spaces_nf(
    uchar *str)
{
    uchar *ptr, ch;

    /* lose leading spaces */
    do { ch = *str++; } while(ch == SPACE);
    --str;

    /* set ptr to end of (plain) string */
    ptr = str + strlen(str);

    /* move back over spaces */
    ++ptr;
    while((--ptr > str)  &&  (*(ptr-1) == SPACE))
        ;

    /* lose trailing spaces */
    *ptr = CH_NULL;

    return(str);
}

/******************************************************************************
*
* display three strings at str to width of fwidth
*
******************************************************************************/

extern coord
lcrjust_plain(
    uchar *str,
    S32 fwidth_ch,
    BOOL reversed)
{
    uchar *str2, *str3;
    S32 str1len, str2len, str3len;
    S32 spaces1, spaces2;
    S32 sofar;

    trace_3(TRACE_REALLY, "lcrjust_plain(\"%s\", fwidth_ch = %d, reversed = \"%s\")",
                str, fwidth_ch, report_boolstring(reversed));

    assert(!riscos_fonts);

    /* wee three strings from outslt are: baring nulls wee travel() sofar */
    str2 = str;
    str2 += strlen(str2); /* str is a plain non-fonty string */
    ++str2;

    str3 = str2;
    str3 += strlen(str3); /* str2 is a plain non-fonty string */
    ++str3;

    if(reversed)
    {
        uchar * tstr = str3;
        str3 = str;
        str  = tstr;
    }

    /* leave one char space to the right */
    --fwidth_ch;

    /* non-fonty code */

    /* get rid of leading and trailing spaces */
    str  = trim_spaces_nf(str);
    str2 = trim_spaces_nf(str2);
    str3 = trim_spaces_nf(str3);

    str1len = calsiz(str);
    str2len = calsiz(str2);
    str3len = calsiz(str3);

    spaces1 = ((fwidth_ch - str2len) >> 1) - str1len;

    spaces2 = fwidth_ch - str1len - spaces1 - str2len - str3len;

    if(spaces2 < 0)
        spaces1 += spaces2;

    sofar = strout(str, fwidth_ch);

    switch_off_highlights();

    if(spaces1 > 0)
    {
        sofar += spaces1;
        ospca(spaces1);
    }

    sofar += strout(str2, fwidth_ch - sofar);

    switch_off_highlights();

    if(spaces2 > 0)
    {
        sofar += spaces2;
        ospca(spaces2);
    }

    sofar += strout(str3, fwidth_ch - sofar);

    switch_off_highlights();

    return(sofar);
}

extern GR_MILLIPOINT
lcrjust_riscos_fonts(
    uchar *str,
    S32 fwidth_ch,
    BOOL reversed)
{
    uchar *str2, *str3;
    char paint1_buf[PAINT_STRSIZ], paint2_buf[PAINT_STRSIZ], paint3_buf[PAINT_STRSIZ];
    GR_MILLIPOINT swidth1_mp, swidth2_mp, swidth3_mp;
    GR_MILLIPOINT fwidth_mp, x_mp, xx_mp;
    S32 paint_op = font_ABS;

    trace_3(TRACE_REALLY, "lcrjust_riscos_fonts(\"%s\", fwidth_ch = %d, reversed = \"%s\")",
                str, fwidth_ch, report_boolstring(reversed));

    assert(riscos_fonts);

    /* wee three strings from outslt are: baring nulls wee travel() sofar */
    str2 = str;
    while(CH_NULL != *str2)
        str2 += font_skip(str2);
    ++str2;

    str3 = str2;
    while(CH_NULL != *str3)
        str3 += font_skip(str3);
    ++str3;

    if(reversed)
    {
        uchar * tstr = str3;
        str3 = str;
        str  = tstr;
    }

    /* leave one char space to the right */
    --fwidth_ch;

    /* fonty code */

    x_mp  = 0;
    xx_mp = 0;

    if('Y' == d_options_KE)
    {
        paint_op |= font_KERN;
    }

    font_strip_spaces(paint1_buf, str,  NULL);
    font_strip_spaces(paint2_buf, str2, NULL);
    font_strip_spaces(paint3_buf, str3, NULL);

    swidth1_mp = font_width(paint1_buf);
    swidth2_mp = font_width(paint2_buf);
    swidth3_mp = font_width(paint3_buf);

    fwidth_mp = cw_to_millipoints(fwidth_ch);

    if(draw_to_screen)
        clear_underlay(fwidth_ch);

    if( swidth1_mp > fwidth_mp)
        swidth1_mp = font_truncate(paint1_buf, fwidth_mp);

    if(swidth1_mp)
    {
        font_complain(font_paint(paint1_buf, paint_op,
                                 riscos_font_ad_millipoints_x, riscos_font_ad_millipoints_y));
        xx_mp = swidth1_mp;
    }

    x_mp = (fwidth_mp - swidth2_mp) / 2;

    if(swidth2_mp  &&  (x_mp >= xx_mp))
    {
        font_complain(font_paint(paint2_buf, paint_op,
                                 riscos_font_ad_millipoints_x + x_mp, riscos_font_ad_millipoints_y));
        xx_mp = x_mp + swidth2_mp;
    }

    x_mp = fwidth_mp - swidth3_mp;

    if(swidth3_mp  &&  (x_mp >= xx_mp))
    {
        font_complain(font_paint(paint3_buf, paint_op,
                                 riscos_font_ad_millipoints_x + x_mp, riscos_font_ad_millipoints_y));
        xx_mp = x_mp + swidth3_mp;
    }

    return(xx_mp);
}

/******************************************************************************
*
* paint justified fonty string
*
******************************************************************************/

static GR_MILLIPOINT
font_paint_justify(
    char *str_in,
    S32 fwidth_ch)
{
    /* leave one char space to the right */
    const S32 fwidth_adjust_ch = 1;
    GR_MILLIPOINT fwidth_mp, swidth_mp, lead_space_mp;
    char paint_buf[PAINT_STRSIZ];
    S32 paint_op = font_ABS;

    fwidth_ch -= fwidth_adjust_ch;

    /* account for spaces */
    lead_space_mp = font_strip_spaces(paint_buf, str_in, NULL);

    fwidth_mp = cw_to_millipoints(fwidth_ch);

    swidth_mp = font_width(paint_buf);

    trace_4(TRACE_APP_PD4, "font_justify width: %d, fwidth_mp: %d, fwidth_ch: %d, lead_space_mp: %d",
            swidth_mp, fwidth_mp, fwidth_ch, lead_space_mp);

    if(swidth_mp + lead_space_mp > fwidth_mp)
        swidth_mp = font_truncate(paint_buf, fwidth_mp + cw_to_millipoints(fwidth_adjust_ch) - lead_space_mp);
    else
    {
        swidth_mp = fwidth_mp;

        paint_op |= font_JUSTIFY;
    }

    if(draw_to_screen)
    {
        /* rubout box must be set up before justification point */
        font_setruboutbox(swidth_mp);

        paint_op |= font_RUBOUT;
    }

    if('Y' == d_options_KE)
    {
        paint_op |= font_KERN;
    }

    if(paint_op & font_JUSTIFY)
        /* provide right-hand justification point */
        wimpt_safe(bbc_move(idiv_floor_fn(riscos_font_ad_millipoints_x + fwidth_mp, millipoints_per_os_x * dx) * dx,
                            riscos_font_ad_millipoints_y / millipoints_per_os_y));

    font_complain(font_paint(paint_buf, paint_op,
                             riscos_font_ad_millipoints_x + lead_space_mp,
                             riscos_font_ad_millipoints_y));

    return(swidth_mp);
}

/******************************************************************************
*
* display the line justified to fwidth
* expand the the line with soft spaces into out_array (VIEW saving)
*
******************************************************************************/

extern S32
justifyline(
    uchar * str,
    S32 fwidth,
    uchar type,
    uchar * out_array)
{
    uchar * from;
    S32 leadingspaces = 0, trailingspaces = 0, words, gaps, spacestoadd;
    S32 linelength, firstgap, secondgap, gapbreak, fudge_for_highlights;
    S32 c_width, count;
    S32 added_sofar = 0, mcount = 0;

    /* send out the leading spaces */
    while(*str++ == SPACE)
        leadingspaces++;

    from = --str;   /* point to first non-space */

    fwidth -= leadingspaces;

    /* if outputting to VIEW file, send the spaces */
    if(out_array)
        while(leadingspaces > 0)
        {
            *out_array++ = SPACE;
            --leadingspaces;
        }

    ospca(leadingspaces);

    if(!*from)
        return((S32) leadingspaces);   /* line was just full of spaces */

    /* count words, gaps, trailing spaces */
    fudge_for_highlights = 0;
    words = 0;
    while(*from)
    {
        words++;

        while(*from  &&  (*from != SPACE))
        {
            if(ishighlight(*from))
                fudge_for_highlights++;
                /* if it's a highlight but displayed as inverse char
                 * it uses an extra character space on screen
                */
            from++;
        }

        trailingspaces = 0;
        while(*from++ == SPACE)
            trailingspaces++;
        --from;
    }

    /* get rid of trailing spaces */
    *(from - trailingspaces) = CH_NULL;

    gaps = words-1;
    if(gaps < 1)
    {
        if(out_array)
        {
            strncpy(out_array, str, fwidth);
            return(leadingspaces + strlen(out_array));
        }

        return(leadingspaces + strout(str, fwidth));
    }

    linelength = (from-str) - fudge_for_highlights - trailingspaces;

    setssp();                       /* set standard spacing */

    /* c_width is width of character, one on screen, if microspacing pitch + offset */
    microspacing = (prnbit  &&  (micbit  ||  riscos_printing));
    c_width =   !microspacing
                    ? 1
                    :
                riscos_printing
                    ? charwidth
                    : (S32) smispc;

    /* spacestoadd is in micro space units */
    if((spacestoadd = (fwidth-linelength-1) * c_width) <= 0)
    {
        if(out_array)
        {
            strncpy(out_array, str, fwidth);
            return(leadingspaces + strlen(out_array));
        }

        return(leadingspaces + strout(str, fwidth));
    }

    /* firstgap in microspace units */
    firstgap = spacestoadd / gaps;

    /* gapbreak is number of words in for change in gap */
    gapbreak = spacestoadd - (gaps * firstgap);

    if(type == J_LEFTRIGHT)
    {
        secondgap = firstgap;
        firstgap++;
    }
    else
    {
        secondgap = firstgap+1;
        gapbreak = gaps - gapbreak;
    }

    /* send out each word, followed by gap */
    from = str;
    count = 0;
    while(((count + added_sofar) <
         (fwidth + (out_array ? fudge_for_highlights : 0)))
            &&  *from)
    {
        BOOL nullfound = FALSE;
        S32 oldcount;
        S32 spacesout;
        uchar *lastword;
        S32 addon;
        S32 widthleft;

        /* find end of word */
        for(lastword = from; *from != SPACE; from++)
            if(*from == CH_NULL)
            {
                nullfound = TRUE;
                break;
            }

        /* *from must be space here */
        *from = CH_NULL;

        /* send out word */
        if(out_array)
        {
            S32 tcount = strlen(strncpy(out_array, lastword,
                                 fwidth-count-added_sofar+fudge_for_highlights));
            out_array += tcount;
            count += tcount;
        }
        else
            count += strout(lastword, fwidth-count - added_sofar);

        if(nullfound)
            break;

        oldcount = count;
        /* reset *from to space */

        for(spacesout = 0, *from = SPACE;
            (added_sofar + count < fwidth)  &&  (*from == SPACE);
            count++, spacesout += c_width, from++)
            ;

        if(*from == CH_NULL)
        {
            count = oldcount;
            break;
        }

        /* add gap */
        addon = (gapbreak-- > 0) ? firstgap : secondgap;

        widthleft = c_width * (fwidth-count-added_sofar);
        if(addon > widthleft)
            addon = widthleft;

        /* need to output real spaces in gap plus micro increment */
        spacesout += addon;

        /* retain count of micro spaces sent */
        mcount += addon;
        added_sofar = mcount / c_width;

        /* send space out */

        if(microspacing)
            mspace((uchar) spacesout);
        else if(out_array)
        {
            /* spacesout-addon hard, addon soft */
            while(spacesout-- > addon)
                *out_array++ = SPACE;

            while(addon-- > 0)
                *out_array++ = TEMP_SOFT_SPACE;
        }
        else
        {
            if(spacesout > 0)
            {
                if(sqobit  &&  !riscos_printing)
                    do { sndchr(SPACE); } while(--spacesout > 0);
                else
                {
                    if(highlights_on)
                    {
                        if(draw_to_screen)
                            clear_underlay(spacesout);

                        do { sndchr(SPACE); } while(--spacesout > 0);
                    }
                    else
                        ospca(spacesout);
                }
            }
        }
    }

    return(leadingspaces + count + added_sofar);
}

/******************************************************************************
*
* return the width of a fonty string in millipoints
*
******************************************************************************/

typedef struct FONT_SCOORDBUFF
{
    int x1;
    int y1;
    int x2;
    int y2;
    int split;
    int bbx1;
    int bby1;
    int bbx2;
    int bby2;
}
FONT_SCOORDBUFF;

extern S32
font_width(
    char *str)
{
    FONT_SCOORDBUFF c;
    _kernel_swi_regs rs;
    int plottype = (1 << 8);

    if(d_options_KE == 'Y')
        plottype |= (1 << 9);

    rs.r[0] = 0;
    rs.r[1] = (int) str;
    rs.r[2] = plottype;
    rs.r[3] = INT_MAX;
    rs.r[4] = INT_MAX;
    rs.r[5] = (int) &c;
    rs.r[6] = (int) NULL;

    font_complain(cs_kernel_swi(0x400A1 /*Font_ScanString*/, &rs));

    return(rs.r[3] /*length*/);
}

/******************************************************************************
*
* truncate a fonty string to a given width
*
******************************************************************************/

extern S32
font_truncate(
    char *str,
    S32 width)
{
    font_string fs;

    fs.s = str;
    fs.x = width;
    fs.y = INT_MAX;
    fs.split = -1;
    fs.term = INT_MAX;

    font_complain(font_strwidth(&fs));
    trace_3(TRACE_APP_PD4, "font_truncate before width: %d, term: %d, str: \"%s\"",
            width, fs.term, str);
    str[fs.term] = CH_NULL;
    trace_2(TRACE_APP_PD4, "font_truncate after width: %d, str: \"%s\"", fs.x, str);
    return(fs.x);
}

/******************************************************************************
*
* account for and strip leading and trailing spaces
* from a fonty string, taking note of leading font/
* highlight changes too
*
******************************************************************************/

extern GR_MILLIPOINT
font_strip_spaces(
    char *out_buf,
    char *str_in,
    P_S32 spaces)
{
    S32 lead_spaces, font_change;
    GR_MILLIPOINT lead_space_mp;
    char *i, *o, *str, ch;

    /* find end of string and find leading space */
    lead_spaces = font_change = 0;
    str = NULL;
    i = str_in;
    o = out_buf;
    while(CH_NULL != *i)
    {
        S32 nchar;

        font_change = is_font_change(i);

        if(!str)
        {
            if(*i == SPACE)
            {
                ++lead_spaces;
                ++i;
                continue;
            }
            else if(!font_change)
                str = i;
        }

        nchar = font_skip(i);
        memcpy32(o, i, nchar);

        i += nchar;
        o += nchar;
    }

    /* stuff trailing spaces */
    if(o != out_buf)
    {
        while(*--o == SPACE && !font_change);
        *(o + 1) = CH_NULL;
    }
    else
        *o = CH_NULL;

    if(str)
    {
        ch = *str;
        *str = CH_NULL;
        lead_space_mp = font_width(str_in);
        *str = ch;
    }
    else
        lead_space_mp = 0;

    if(spaces)
        *spaces = lead_spaces;

    return(lead_space_mp);
}

/******************************************************************************
*
* initialise flags and justification(microspacing)
* maybe skip leading spaces
*
* for(each char in string && !end of field)
*   deal with highlights, gaps, microspacing
*   sndchr(ch)
* clean up after justification and highlights
* return number of chars output
*
* with_atts false displays in white on black
*
******************************************************************************/

static S32
strout(
    uchar *str,
    S32 fwidth)
{
    S32 count = 0;
    S32 len;

    if(draw_to_screen  &&  (fwidth > 0))
    {
        len = strlen(str);
        clear_underlay(MIN(len, fwidth));
    }

    while(*str  &&  (count < fwidth))
    {
        if( *str == FUNNYSPACE)
            *str = SPACE;

        /* sndchr returns width of thing (highlight?) output */
        if(sndchr(*str++))
            count++;
    }

    return(count);
}

/******************************************************************************
*
* send char to screen or printer dealing with highlights etc.
* doesn't clear it's own background - caller is responsible
* return TRUE if print-head moved on
*
******************************************************************************/

extern BOOL
sndchr(
    U8 ch)
{
    S32 eorval;

    /* if RISC OS printing, need to paint char as usual */
    if(sqobit  &&  !riscos_printing)
        return(prnout(ch));

    if((ch >= CH_SPACE)  &&  (ch != CH_DELETE))
    {
        if(highlights_on)
            wrch_h(ch);
        else
            wrch(ch);

        return(TRUE);
    }

    switch(ch)
    {
    case HIGH_UNDERLINE:
        eorval = N_UNDERLINE;
        break;

    case HIGH_ITALIC:
        eorval = N_ITALIC;
        break;

    case HIGH_SUBSCRIPT:
        eorval = N_SUBSCRIPT;
        break;

    case HIGH_SUPERSCRIPT:
        eorval = N_SUPERSCRIPT;
        break;

    case HIGH_BOLD:
        eorval = N_BOLD;
        break;

    default:
        twzchr(ch);
        return(TRUE);
    }

    highlights_on ^= eorval;
    return(FALSE);
}

/*
find the last column position on the screen in an inverse block given
an x position in inverse and the column
*/

static S32
end_of_block(
    S32 x_pos_in,
    ROW trow)
{
    S32 x_pos = x_pos_in;
    S32 coff = calcoff(riscos_fonts ? (x_pos / charwidth) : x_pos);
    P_SCRCOL cptr;
    COL tcol;

    if(!horzvec_entry_valid(coff)) /* expect OFF_RIGHT */
        return(x_pos_in);

    /* get screen address of beginning of column underneath x */
    x_pos = os_if_fonts(calcad(coff));

    assert(horzvec_entry_valid(coff));
    cptr = horzvec_entry(coff);

    /* while the column is marked add its width on to x */
    for(;;)
    {
        if(cptr->flags & LAST)
            return(x_pos);

        tcol = cptr->colno;

        if(!inblock(tcol, trow))
            return(x_pos);

        x_pos += os_if_fonts(colwidth(tcol));

        cptr++;
    }
}

/******************************************************************************
*
* correct lescrl to that value suitable
* for outputting the current buffer
* in the given field at a position
*
******************************************************************************/

static void
adjust_lescrl(
    S32 x,
    S32 fwidth_ch)
{
    char paint_str[PAINT_STRSIZ];
    GR_MILLIPOINT fwidth_mp, swidth_mp;

    fwidth_ch = MIN(fwidth_ch, pagwid_plus1 - x);

    if(--fwidth_ch < 0)     /* rh scroll margin */
        fwidth_ch = 0;

    if(riscos_fonts  &&
       !(xf_inexpression || xf_inexpression_box || xf_inexpression_line) &&
       fwidth_ch
      )
    {   /* adjust for fancy font printing */

        fwidth_mp = cw_to_millipoints(fwidth_ch);

        for(;;)
        {
            /* fonty cal_lescrl a little more complex */

            /* is the caret off the left of the field? (scroll margin one char) */
            if(lecpos <= lescrl)
            {
                /* off left - try to centre caret in field */
                if(lescrl)
                    lescrl = MAX(lecpos - 2, 0);        /* 3rd attempt! */

                break;
            }
            else
            {
                /* is the caret off the right of the field? (scroll margin one char) */
                expand_current_slot_in_fonts(paint_str, TRUE, NULL);
                swidth_mp = font_width(paint_str);

                trace_2(TRACE_APP_PD4, "adjust_lescrl: fwidth_mp %d, swidth_mp %d", fwidth_mp, swidth_mp);

                if(swidth_mp > fwidth_mp)
                    /* off right - try to right justify */
                    lescrl++;
                else
                    break;
            }
        }
    }
    else
    {   /* adjust for standard text printing */

        lescrl = cal_lescrl(fwidth_ch);
    }
}

/******************************************************************************
*
* optimise re-painting only from dspfld_from
* to stop the flickering and speed it up
*
******************************************************************************/

static S32
cal_dead_text(void)
{
    S32    linelen = strlen(linbuf);
    S32    offset = MIN(lescrl, linelen);
    char * ptr = (char *) linbuf + offset;
    S32    dead_text;

    dspfld_from = MIN(dspfld_from, linelen);

    dead_text = dspfld_from - offset;

    /* always reset */
    dspfld_from = -1;

    if((lescrl == old_lescroll)  &&  (dead_text > 0))
    {
        ptr += dead_text;
        while((*--ptr == SPACE)  &&  dead_text)
            --dead_text;
        ++ptr;

        trace_1(TRACE_APP_PD4, "cal_dead_text: %d dead text", dead_text);
    }
    else
        dead_text = 0;

    if(riscos_fonts  &&  !xf_inexpression)
        /* too hard man */
        dead_text = 0;

    return(dead_text);
}

/******************************************************************************
*
* display the field in linbuf to x, y
* fwidth is maximum field width allowed
*
* it must clear its own background
*
******************************************************************************/

static S32
dspfld(
    S32 x,
    S32 y,
    S32 fwidth_ch)
{
    S32 dspfld_written;
    S32 trailing_spaces, chars_printed, width_left;
    S32 linelen, offset;
    char *ptr, *start;
    char paint_str[PAINT_STRSIZ];
    GR_MILLIPOINT fwidth_mp, swidth_mp;
    S32 this_font;
    S32 paint_op = font_ABS+font_RUBOUT;

    trace_3(TRACE_APP_PD4, "dspfld: x %d, y %d, fwidth_ch %d", x, y, fwidth_ch);

    if(riscos_fonts  &&  !(xf_inexpression || xf_inexpression_box || xf_inexpression_line))
    {   /* fancy font printing */

        screen_xad_os = gcoord_x(x);
        screen_yad_os = gcoord_y(y);

        riscos_font_ad_millipoints_x = millipoints_per_os_x * screen_xad_os;
        riscos_font_ad_millipoints_y = millipoints_per_os_y * (screen_yad_os + fontbaselineoffset);

        ensurefontcolours();

        fwidth_mp = cw_to_millipoints(fwidth_ch);

        expand_current_slot_in_fonts(paint_str, FALSE, &this_font);
        swidth_mp = font_width(paint_str);
        trace_2(TRACE_APP_PD4, "dspfld: fwidth_mp = %d, swidth_mp = %d", fwidth_mp, swidth_mp);

        if('Y' == d_options_KE)
        {
            paint_op |= font_KERN;
        }

        if( swidth_mp > fwidth_mp)
            swidth_mp = font_truncate(paint_str, fwidth_mp);

        font_setruboutbox(swidth_mp);

        font_complain(font_paint(paint_str, paint_op,
                                 riscos_font_ad_millipoints_x, riscos_font_ad_millipoints_y));

        if(word_to_invert)
            killfontcolourcache();

        dspfld_written = swidth_mp;
    }
    else
    {   /* standard text printing */

        linelen = strlen(linbuf);
        offset = MIN(lescrl, linelen);
        ptr = (char *) linbuf + offset;
        trailing_spaces = 0;
        chars_printed = 0;
        dspfld_written = 0; /* SKS 10.10.91 */

        fwidth_ch = MIN(fwidth_ch, RHS_X - x);

        width_left = fwidth_ch;

        at(x, y);

        trace_4(TRACE_APP_PD4, "dspfld: x = %d, fwidth_ch = %d, width_left = %d, lescrl = %d",
                x, fwidth_ch, width_left, lescrl);

#if 1
        /* SKS after PD 4.12 24mar92 - having added code in middle of loop to handle
         * end of field and break this shouldn't loop conditionally here
        */
        for(;;)
#else
        while(width_left > 0)
#endif
        {
            start = ptr;

            /* gather up ordinary characters for block print */
            while((*ptr++ > SPACE)  &&  (width_left > 0))
                --width_left;

            /* and print them */
            if(--ptr != start)
            {
                S32 done;

                /* any trailing spaces left over from last time? */
                if(trailing_spaces)
                {
                    chars_printed += trailing_spaces;
                    ospca(trailing_spaces);
                    trailing_spaces = 0;
                }

                /* output string */
                done = ptr - start;

                chars_printed += done;
                clear_underlay(done);
                stringout_field(start, done);
            }

            /* gather up trailing spaces for eventual block print */
            while((*ptr++ == SPACE)  &&  (width_left > 0))
            {
                ++trailing_spaces;
                --width_left;
            }

            /* at end of field */
            if(!*--ptr  ||  (width_left <= 0))
            {
                /* may be some spaces to output if
                 * lecpos is off the end of the buffer
                */
                trailing_spaces = (lecpos - lescrl) - chars_printed;
                trailing_spaces = MIN(trailing_spaces, width_left);
                if(trailing_spaces > 0)
                {
                    chars_printed += trailing_spaces;
                    ospca(trailing_spaces);
                }
                dspfld_written = chars_printed;
                break;
            }
            else if(*ptr < SPACE)
            {
                if(trailing_spaces > 0)
                {
                    chars_printed += trailing_spaces;
                    ospca(trailing_spaces);
                    trailing_spaces = 0;
                }

                /* output space or control char */
                --width_left;
                ++chars_printed;
                twzchr(*ptr++);
            }
        }

        /* do we need to invert a particular word? */
        if(word_to_invert)
        {
            S32 len = strlen(word_to_invert);

            at(x + lecpos - lescrl, y);

            invert();
            clear_underlay(len);
            stringout(word_to_invert);
            invert();

            at(x + chars_printed, y);
        }
    }

    return(dspfld_written);
}

/******************************************************************************
*
*  position the cursor/caret on the screen
*
******************************************************************************/

extern void
position_cursor(void)
{
    S32 x, y;
    char paint_str[PAINT_STRSIZ];
    GR_MILLIPOINT swidth_mp;

    trace_0(TRACE_APP_PD4, "position_cursor()");

    if(cbuff_offset)
        return;

    x = calcad(curcoloffset);
    y = calrad(currowoffset);

    if(riscos_fonts)
    {   /* fancy font positioning */

        if(lecpos > lescrl)
        {
            expand_current_slot_in_fonts(paint_str, TRUE, NULL);
            swidth_mp = font_width(paint_str);
        }
        else
            swidth_mp = 0;

        x = cw_to_os(x) + idiv_floor_fn(swidth_mp, millipoints_per_os_x * dx) * dx; /* millipoints into OS via pixels */
    }
    else
    {   /* standard text positioning */

        x += (lecpos - lescrl < 0)
                    ? (S32) 0
                    : (S32) (lecpos - lescrl);

        x = MIN(x, pagwid_plus1);
    }

    /* merely note new caret position, updated later */

    if((x != lastcursorpos_x)  ||  (y != lastcursorpos_y))
    {
        /*reportf("caret moving to %d, %d", x, y);*/
        lastcursorpos_x = x;
        lastcursorpos_y = y;
        xf_caretreposition = TRUE;
    }
    /*else
        reportf("not moving caret");*/
}

/******************************************************************************
*
* switch off highlights
*
******************************************************************************/

static void
switch_off_highlights(void)
{
    if(prnbit)
    {
        prnout(EOS);
        return;
    }

    highlights_on = 0;
}

/******************************************************************************
*
* display highlights (inverse 1-8) and control characters (inverse @-?)
*
******************************************************************************/

extern void
twzchr(
    char ch)
{
    ch = (ch == CH_DELETE)
                ?   '?'
                :
         ishighlight(ch)
                ?   ch + (FIRST_HIGHLIGHT_TEXT - FIRST_HIGHLIGHT)
                :   ch + ('A' - 1);

    riscos_invert();
    riscos_printchar(ch);
    riscos_invert();
}

/******************************************************************************
*
*  how big a field the given cell must plot in
*
******************************************************************************/

extern S32
limited_fwidth_of_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow)
{
    S32 fwidth = chkolp(tcell, tcol, trow);    /* text cell contents never drawn beyond overlap */
    S32 limit  = pagwid_plus1 - calcad(curcoloffset);

    assert(tcol == curcol);

    return(MIN(fwidth, limit));
}

static S32
limited_fwidth_of_slot_in_buffer(void)
{
    return(limited_fwidth_of_slot(travel_here(), curcol, currow));
}

/******************************************************************************
*
* get overlap width for cell taking account of column width
*
* RJM, realises on 18.5.89 that when printing,
* not all the columns are in horzvec. Hence the sqobits.
*
******************************************************************************/

extern S32
chkolp(
    P_CELL tcell,
    COL tcol,
    ROW trow)
{
    BOOL printing   = sqobit;
    S32 totalwidth = colwidth(tcol);
    S32 wrapwidth  = colwrapwidth(tcol);
    COL trycol = -1; /* keep dataflower happy */
    P_SCRCOL cptr = BAD_POINTER_X(P_SCRCOL, 0); /* keep dataflower happy */

    trace_5(TRACE_OVERLAP, "chkolp(&%p, %d, %d): totalwidth = %d, wrapwidth = %d",
            report_ptr_cast(tcell), tcol, trow, totalwidth, wrapwidth);

    if(totalwidth >= wrapwidth)
        return(wrapwidth);

    if((curcol == tcol)  &&  (currow == trow)  &&  xf_blockcursor)
        return(totalwidth);

    /* check to see if its a weirdo masquerading as a blank */
    if(tcell  &&  (tcell->type != SL_TEXT)  &&  is_blank_cell(tcell))
        return(totalwidth);

    if(printing)
        trycol = tcol;
    else
    {
        /* cptr := horzvec entry (+1) for trycol */
        assert(0 != array_elements(&horzvec_mh));
        cptr = horzvec();
        PTR_ASSERT(cptr);

        do  {
            if(cptr->flags & LAST)
                /* no more cols so return wrapwidth */
                return(wrapwidth);
        }
        while(cptr++->colno != tcol);
    }

    while(totalwidth < wrapwidth)
    {
        if(printing)
            /* when printing, next column is always trycol+1 */
            trycol++;
        else
        {
            /* get next column in horzvec */
            if(cptr->flags & LAST)
                return(wrapwidth);

            trycol = cptr++->colno;
        }

        if(trycol >= numcol)
            return(wrapwidth);

        /* can we overlap it? */
        trace_2(TRACE_OVERLAP, "travelling to %d, %d", trycol, trow);
        tcell = travel(trycol, trow);
        if(tcell  &&  !is_blank_cell(tcell))
            break;

        if(!printing)
        {
            /* don't overlap to current cell */
            if( ((trycol == curcol)  &&  (trow == currow))          ||
                /* don't allow overlap from non-marked to marked */
                (inblock(trycol, trow)  &&  !inblock(tcol, trow))
                )
                break;
        }

        totalwidth += colwidth(trycol);
    }

    return(MIN(totalwidth, wrapwidth));
}

/******************************************************************************
*
* is a cell possibly overlapped on the screen
*
* tests whether there is a text cell on the screen to the left
* with a big enough wrap width to overlap this cell
*
******************************************************************************/

extern BOOL
is_overlapped(
    S32 coff,
    S32 roff)
{
    COL tcol;
    ROW trow;
    P_CELL tcell;
    S32 gap = 0;

    assert(vertvec_entry_valid(roff));
    trow = row_number(roff);

    while(--coff >= 0)
    {
        assert(horzvec_entry_valid(coff));
        tcol  = col_number(coff);
        tcell = travel(tcol, trow);

        gap += colwidth(tcol);

        if(tcell)
        {
            if(!slot_displays_contents(tcell))
                return(FALSE);

#if 1
            /* SKS after PD 4.11 23jan92 */
            return(colwrapwidth(tcol) > gap);
#else
            return(colwrapwidth(tcol) >= gap);
#endif
        }
    }

    return(FALSE);
}

/* end of scdraw.c */
