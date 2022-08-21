/* cursmov.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Relative cursor movement handling */

/* RJM August 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#include "riscdraw.h"
#include "riscos_x.h"

#include "colh_x.h"

/*
internal functions
*/

static S32
cal_offset_in_slot(
    COL col,
    ROW row,
    P_SLOT sl,
    S32 offset_ch,
    S32 cell_offset_os);

static COL
col_off_left(void);

static void
do_down_scroll(void);

static void
do_up_scroll(void);

static BOOL
push_screen_down_one_line(void);

static void
push_screen_up_one_line(void);

#define vertvec_entry_flags(roff) \
    vertvec_entry(roff)->flags

extern void
chknlr(
    COL tcol,
    ROW trow)
{
    movement = movement | ABSOLUTE;

    newcol = (tcol >= numcol)   ? numcol - 1
                                : tcol;

    newrow = (trow >= numrow)   ? numrow - 1
                                : trow;
}

/******************************************************************************
*
* go to given row in current column
*
******************************************************************************/

static void
TC_BC_common(ROW trow)
{
    out_forcevertcentre = TRUE;

    chknlr(curcol, trow);

    if(!xf_inexpression)        /* RCM thinks lecpos and lescrl will always be zeroed */
        lecpos = lescrl = 0;    /*     when old editor goes                           */
}

/******************************************************************************
*
*  go to the top of the column
*
******************************************************************************/

extern void
TopOfColumn_fn(void)
{
    TC_BC_common((ROW) 0);
}

/******************************************************************************
*
*  go to the bottom of the column
*
******************************************************************************/

extern void
BottomOfColumn_fn(void)
{
    TC_BC_common(numrow - 1);
}

/******************************************************************************
*
*  centre the current line
*
******************************************************************************/

extern void
CentreWindow_fn(void)
{
    cenrow(currow);
}

/******************************************************************************
*
* scroll screen up, forcing caret on screen if necessary
*
* move cursor to top of screen, scroll it and reset cursor
*
******************************************************************************/

extern void
ScrollUp_fn(void)
{
    P_SCRROW rptr;
    ROW trow = currow;

    rptr = vertvec();

    currowoffset = (rptr->flags & PAGE) ? 1 : 0;
    currow = rptr[currowoffset].rowno;

    push_screen_up_one_line();

    currowoffset = schrsc(trow);

    if(currowoffset == NOTFOUND)
        {
        xf_drawslotcoordinates = output_buffer = TRUE;

        currowoffset = rowsonscreen-1;

        if(rptr[currowoffset].flags & PAGE)
            --currowoffset;

        mark_row_praps(currowoffset, NEW_ROW);
        }

    currow = rptr[currowoffset].rowno;
}

/******************************************************************************
*
* scroll screen down, forcing caret on screen if necessary
*
******************************************************************************/

extern void
ScrollDown_fn(void)
{
    ROW trow = currow;

    /* if last row on screen don't bother */
    if(!inrowfixes(numrow-1)  &&  (schrsc(numrow-1) != NOTFOUND))
        return;

    push_screen_down_one_line();

    currowoffset = schrsc(trow);

    if(currowoffset == NOTFOUND)
        {
        xf_drawslotcoordinates = output_buffer = TRUE;
        currow = fstnrx();
        currowoffset = schrsc(currow);
        mark_row_praps(currowoffset, NEW_ROW);
        }
    else
        currow = row_number(currowoffset);
}

/******************************************************************************
*
* scroll screen left, forcing caret on screen if necessary
*
******************************************************************************/

extern void
ScrollLeft_fn(void)
{
    COL o_col = curcol;
    COL tcol;

    /* if cannot scroll left, don't bother */
    if((tcol = col_off_left()) < 0)
        {
        xf_flush = TRUE;
        return;
        }

    /* fill horzvec from column to the left */
    filhorz(tcol, tcol);

    /* if now off right of screen or in partial column, set to rightmost fully visible column (if poss) */
    curcoloffset = schcsc(o_col);

    if((curcoloffset == NOTFOUND)  ||  (curcoloffset == scrbrc))
        curcoloffset = !scrbrc ? scrbrc : scrbrc-1;

    curcol = col_number(curcoloffset);

    out_screen = TRUE;
}

/******************************************************************************
*
* scroll screen right, forcing caret on screen if necessary
*
******************************************************************************/

extern void
ScrollRight_fn(void)
{
    COL tcol,
    o_col = curcol;

    /* no more columns to bring on screen? */
    if(col_off_right() >= numcol)
        {
        xf_flush = TRUE;
        return;
        }

    tcol = fstncx();

    do  {
        /* MRJC 25.11.91 */
        if(tcol + 1 == numcol)
            break;

        ++tcol;
        }
    while(!colwidth(tcol));

    /* fill horzvec from column */
    filhorz(tcol, tcol);

    /* if now off left of screen, set to leftmost non-fixed visible column */
    curcoloffset = schcsc(o_col);
    curcol       = (curcoloffset == NOTFOUND) ? tcol : o_col;
    curcoloffset = schcsc(curcol);

    out_screen = TRUE;
}

/******************************************************************************
*
* mark the row for redrawing.  When moving up and down within the screen
* most numeric slots do not need to be redrawn.  The old and new slots
* must be redrawn for the block cursor movement.  Text and blank slots must
* be redrawn for overlap. drawnumbers specifies whether the numbers can be
* missed.
*
******************************************************************************/

extern void
mark_row_border(
    coord rowonscr)
{
    trace_2(TRACE_APP_PD4, "mark_row row: %d, offset: %d", row_number(rowonscr), rowonscr);

    /* allow multiple calls */
    if( (out_rowborout   &&  (rowonscr == rowborout ))  ||
        (out_rowborout1  &&  (rowonscr == rowborout1))  )
            return;

    /* assert(!out_rowborout1); - everything seems to work if older rowborouts get trashed - leave well alone! */

    rowborout1     = rowborout;
    out_rowborout1 = out_rowborout;

    rowborout      = rowonscr;
    out_rowborout  = TRUE;
}

extern void
mark_row(
    coord rowonscr)
{
    trace_2(TRACE_APP_PD4, "mark_row row: %d, offset: %d", row_number(rowonscr), rowonscr);

    mark_row_border(rowonscr);

    /* allow multiple calls */
    if( (out_rowout   &&  (rowonscr == rowout ))    ||
        (out_rowout1  &&  (rowonscr == rowout1))    )
            return;

    rowout1     = rowout;
    out_rowout1 = out_rowout;

    rowout      = rowonscr;
    out_rowout  = TRUE;
}

/******************************************************************************
*
* MRJC created this more optimal version 13/7/89
* I note from the speech above mark_row that the original
* sentiments from VP have been noted - but not
* implemented! So this one checks for numeric slots
* and overlap. This was done by MRKCON in VP
* This routine assumes that the column has not changed
*
******************************************************************************/

extern void
mark_row_praps(
    coord rowonscr,
    BOOL old_row)
{
    COL col, i;
    ROW row;
    P_SLOT sl, tsl;
    char *c;
    coord fwidth;

    mark_row_border(rowonscr);

    /* do-while is for structure - break out to mark row */
    do  {
        if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
            break;

        col = oldcol;
        row = row_number(rowonscr);

        trace_4(TRACE_APP_PD4, "mark_row_praps col: %d, row: %d, lescrl %d, OLD_ROW %s", col, row, lescrl, trace_boolstring(old_row == OLD_ROW));

        if(chkrpb(row))
            break;

        sl = travel(col, row);

        /* only text slots can be different */
        if(sl  &&  (sl->type != SL_TEXT))
            return;

        trace_0(TRACE_APP_PD4, "mark_row_praps found empty slot/text slot");

        /* check for justification */
        if(sl  &&  ((sl->justify & J_BITS) != J_FREE))
            {
            trace_0(TRACE_APP_PD4, "slot is justified");
            break;
            }

        /* row must be output if was scrolled/will be scrolled */
        if(old_row)
            {
            if(old_lecpos)
                {
                if(old_lescroll)
                    {
                    trace_0(TRACE_APP_PD4, "old slot had been scrolled");
                    break;
                    }

                if(grid_on  &&  (old_lecpos > colwidth(col)))
                    {
                    trace_0(TRACE_APP_PD4, "grid is on and cursor was beyond a grid bar in the field");
                    break;
                    }
                }
            }
        else
            {
            if(lecpos)
                {
                /* MRJC 8.10.91 */
                #if 0
                /* if fonts are on, mark and go home */
                if(riscos_fonts)
                    {
                    trace_0(TRACE_APP_PD4, "non-zero lecpos and fonts on");
                    break;
                    }
                #endif

                if(grid_on  &&  (lecpos > colwidth(col)))
                    {
                    trace_0(TRACE_APP_PD4, "grid is on and cursor will be beyond a grid bar in the field");
                    break;
                    }

                fwidth = limited_fwidth_of_slot(sl, col, row);

                if(--fwidth < 0)        /* rh scroll margin */
                    fwidth = 0;

                if(lecpos > fwidth)
                    {
                    trace_0(TRACE_APP_PD4, "new slot will be scrolled");
                    break;
                    }
                }
            }

        /* check for slots to the left overlapping */
        if(col  &&  !sl)
            {
            for(i = 0; i < col; i++)
                {
                tsl = travel(i, row);

                if(!tsl  ||  (tsl->type != SL_TEXT))
                    continue;

                if(!isslotblank(tsl))
                    break;
                }

            if(i != col)
                {
                trace_0(TRACE_APP_PD4, "blank slot is overlapped by something to the left");
                break;
                }
            }

        if(!sl)
            return;

        { /* search for text-at chars and highlights and anything nasty */
        const uchar text_at_char = get_text_at_char();
        for(c = sl->content.text; *c; c++)
            if((text_at_char == *c)  ||  (*c == DELETE)  ||  (*c < SPACE))
                {
                trace_0(TRACE_APP_PD4, "slot has highlights");
                goto mark_and_return;
                }
        } /*block*/

        return;
        }
    while(FALSE);

mark_and_return:

    trace_0(TRACE_APP_PD4, "mark_row_praps marking slot");
    mark_row(rowonscr);
}

/******************************************************************************
*
* returns the next row up off the top of the screen
* if no more rows returns -1
*
******************************************************************************/

static ROW
row_off_top(void)
{
    ROW trow = fstnrx() - 1;

    while((trow >= 0)  &&  inrowfixes(trow))
        trow--;

    return(trow);
}

/******************************************************************************
*
*  adjust the page offset and number for going up and down
*
******************************************************************************/

static BOOL
adjpud(
    BOOL down,
    ROW rowno)
{
    BOOL softbreak = FALSE;
    S32 tpoff;

    curpnm = pagnum;

    trace_2(TRACE_APP_PD4, "adjpud(%s, %d)", trace_boolstring(down), rowno);

    if(down)
        {
        if(chkrpb(rowno))
            {
            /* hard page break: set offset in page break and check active */

            if(chkpbs(rowno, (pagoff==0) ? encpln : pagoff))
                {
                /* don't update pagnum if hard and soft together */
                if(pagoff != enclns)
                    {
                    pagnum = ++curpnm;
                    pagoff = enclns;
                    }
                }

            return(FALSE);
            }
        }
    else
        {
        /* check for conditional page break */
        if(chkrpb(rowno-1))
            {
            /* read active state and check active */
            pagoff = travel(0, rowno-1)->content.page.cpoff;

            /* if on soft break, adjust pagoff */
            if(pagoff == encpln)
                pagoff++;

            if(chkpac(rowno-1))
                if(pagoff != enclns)
                    {
                    --curpnm;
                    pagnum = curpnm;
                    }

            return(FALSE);
            }
        }

    /* check for soft break */
    tpoff = pagoff;

    if(tpoff == 0)
        plusab(curpnm, (down) ? 1 : -1);

    tpoff += (down) ? enclns : -enclns;

      if(tpoff < 0)
        tpoff = encpln;
    else if(tpoff > encpln)
        tpoff = 0;

    softbreak = ((tpoff == 0)  &&  chkfsb());
    pagoff = tpoff;

    pagnum = curpnm;

    return(softbreak);
}

/******************************************************************************
*
*  move from oldrow to newrow updating page parameters
*
******************************************************************************/

extern void
fixpage(
    ROW o_row,
    ROW n_row)
{
    BOOL down   = (n_row >= o_row);
    S32 amount = (down) ? 1 : -1;
    ROW trow;

    trace_2(TRACE_APP_PD4, "fixpage(%d, %d)", o_row, n_row);

    if(n_row == (ROW) 0)
        {
        pagoff = filpof;
        pagnum = filpnm;
        return;
        }

    while(n_row != o_row)
        {
        trow = o_row + amount;

        trace_1(TRACE_APP_PD4, "trying trow %d", trow);

        /* check hard and soft breaks together */
        if(down  &&  (pagoff == 0)  &&  chkrpb(trow))
            {
            ++trow;
            ++o_row;
            }

        if(!adjpud(down, o_row))
            o_row = trow;
        }
}

/******************************************************************************
*
* move cursor up a line
*
******************************************************************************/

extern void
curup(void)
{
    coord origoffset;

    xf_flush = TRUE;

    if(currowoffset == 0)
        {
        /* scroll another line on if at top of screen */
        push_screen_up_one_line();
        return;
        }

    /* may be a page break at top of file */
    if(currow == 0)
        return;

    origoffset = currowoffset--;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        {
        /* move up over the page break */
        if(currowoffset == 0)
            {
            pagnum++;
            curpnm++;
            }

        curup();

        /* in the case of moving up over a page break
         * and scrolling the screen, the old row will
         * now be at offset 2
        */
        if(origoffset == 1)
            origoffset = 2;

        mark_row_praps(origoffset, OLD_ROW);
        }
    else
        {
        mark_row_praps(origoffset, OLD_ROW);
        currow = row_number(currowoffset);
        mark_row_praps(currowoffset, NEW_ROW);
        }
}

/*
push_screen_up_one_line scrolls the screen up
it assumes the cursor is on the top (non-page break) line
*/

extern void
push_screen_up_one_line(void)
{
    P_SCRROW rptr = vertvec();
    BOOL gone_over_pb;
    ROW trow;

    gone_over_pb = FALSE;

    if(rptr->flags & FIX)
        {
        /* if last row fixed, can't do anything */
        if(rptr[rows_available].flags & FIX)
            return;

        trow = row_off_top();

        if(trow < 0)
            return;

        filvert(trow, currow, DONT_CALL_FIXPAGE);
        }
    else if(currow > 0)
        {
        adjpud(FALSE, currow);

        currow--;

        filvert(currow, currow, DONT_CALL_FIXPAGE);

        if(rptr->flags & PAGE)
            {
            if(currow == 0)
                {
                curdown();
                draw_row(0);                /* display page break */
                return;
                }

            currowoffset = 0;
            currow++;                       /* couldn't decrement it cos of pb */

            curup();
            mark_row_praps(2, OLD_ROW);     /* old row is now row 2 */
            gone_over_pb = TRUE;
            }
        }
    else
        return;

    /* if we were in the middle of drawing the whole screen, start the draw
     * again. Otherwise scroll the screen and redraw top two lines
    */
    if(out_below)
        {   /* must be in block cos of mark_to_end is macro */
        mark_to_end(0);
        }
    else
        {
        do_down_scroll();

        if(!gone_over_pb)
            {
            if(n_rowfixes+1 < rows_available)
                mark_row_praps(n_rowfixes+1, NEW_ROW);

            mark_row(n_rowfixes);
            }
        else
            draw_row(1);
        }
}

/*
 * On Acorn machines we have a choice of fast or pretty (or RISC OS, hard).
 * The faster method is using hard scroll and then redrawing the first
 * few lines. This is known as epileptic scrolling.
 * The nicer method is to scroll only those lines which are changing.
 *
 * For fast scroll do
 *   down_scroll(0);
 *   draw top four lines on screen;
 *
 * ( up_scroll(0);
 *   draw top four lines on screen; )
 *
 * On Archimedes soft scroll takes longer for larger memory screen modes.
 * Since there is no point in using modes with more than four colours,
 * the worst real cases are mode 19 (4 colours,  64r*80c,   80k)
 *                      and mode 16 (16 colours, 32r*132c, 132k)
*/

static void
do_down_scroll(void)
{
    down_scroll(n_rowfixes);
}

/******************************************************************************
*
*  move cursor down a line
*
******************************************************************************/

extern void
curdown(void)
{
    uchar flags;
    coord origoffset;

    xf_flush = TRUE;

    if(currowoffset >= rows_available-1)
        {
        /* cursor at bottom of screen */
        if( (vertvec_entry_flags(currowoffset) & FIX)  ||
            !push_screen_down_one_line())
            return;
        }
    else
        {
        /* cursor not yet at bottom of screen */
        flags = vertvec_entry_flags(currowoffset+1);

        if((flags & PAGE))
            {
            origoffset = currowoffset;

            if(currowoffset == rows_available-2)
                origoffset--;

            currowoffset++;
            curdown();
            draw_row(origoffset);
            return;
            }

        if(!(flags & LAST))
            {
            mark_row_praps(currowoffset++,  OLD_ROW);       /* mark old row */
            mark_row_praps(currowoffset,    NEW_ROW);       /* mark new row */
            }
        }

    currow = row_number(currowoffset);
}

static BOOL
push_screen_down_one_line(void)
{
    ROW tnewrow;
    S32 oldpagoff;
    BOOL gone_over_pb;

    if(currow+1 >= numrow)
        return(FALSE);

    tnewrow = fstnrx();
    oldpagoff = pagoff;
    gone_over_pb = FALSE;

    adjpud(TRUE, tnewrow);

    if((oldpagoff != 0)  ||  chkrpb(tnewrow)  ||  !chkfsb())
        {
        /* if top line isn't soft break, or there is hard break too */
        if(++tnewrow >= numrow)
            return(FALSE);
        }

    filvert(tnewrow, currow+1, DONT_CALL_FIXPAGE);

    if(vertvec_entry_flags(rows_available-1) & PAGE)
        {
        currowoffset = rows_available-1;
        curdown();
        mark_row_praps(currowoffset-2, OLD_ROW);                /* mark old row */
        gone_over_pb = TRUE;
        }

    /* if the scrolling comes in the middle of a screen draw, draw the
     * whole lot, otherwise scroll up and redraw bottom rows
    */
    if(out_below)
        {               /* must be in block cos of macro */
        mark_to_end(0);
        }
    else
        {
        do_up_scroll();

        currowoffset = rows_available-1;

        if(!gone_over_pb)
            {
            mark_row_praps(currowoffset-1, OLD_ROW);    /* mark old row */
            mark_row(currowoffset);                     /* mark new row */
            }
        else
            draw_row(rows_available-2);     /* display page break */
        }

    return(TRUE);
}

static void
do_up_scroll(void)
{
    up_scroll(n_rowfixes);
}

/******************************************************************************
*
* return number of rows to be jumped over in screen shifts
* if fixed, then number of non-fixed rows
* otherwise number of rows minus soft pages
*
******************************************************************************/

static coord
calpli(void)
{
    P_SCRROW rptr = vertvec();
    P_SCRROW last_rptr;
    coord res;

    if(rptr->flags & FIX)
        res = rows_available - n_rowfixes;
    else
        {
        res = 0;

        last_rptr = rptr + rows_available;

        while(rptr < last_rptr)
            if(!(rptr++->flags & PAGE))
                res++;
        }

    return((res <= 1) ? 1 : res-1);
}

/******************************************************************************
*
* move cursor down a screenful
*
******************************************************************************/

extern void
cursdown(void)
{
    coord rowstogo;
    ROW newfirstrow,
    oldfirst = fstnrx();
    coord oldcuroff = currowoffset;

    for(rowstogo = calpli(), newfirstrow = oldfirst;
        (newfirstrow < numrow-1)  &&  (rowstogo > 0);
        newfirstrow++)
            if(!inrowfixes(newfirstrow))
                rowstogo--;

    xf_flush = TRUE;

    if(newfirstrow == oldfirst)
        return;

    if(chkrpb(newfirstrow) && chkpac(newfirstrow))
        newfirstrow++;

    filvert(newfirstrow, newfirstrow, CALL_FIXPAGE);

    currowoffset = (oldcuroff >= rowsonscreen)
                            ? rowsonscreen-1
                            : oldcuroff;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        currowoffset = (currowoffset == 0)
                                ? 1
                                : currowoffset-1;

    currow = row_number(currowoffset);
    mark_to_end(n_rowfixes);
}

/******************************************************************************
*
*  move cursor up a screenful
*
******************************************************************************/

extern void
cursup(void)
{
    coord rowstogo = calpli();
    ROW firstrow;
    coord oldcuroff = currowoffset;

    for(firstrow = fstnrx(); (firstrow >= 0)  &&  (rowstogo > 0); firstrow--)
        if(!inrowfixes(firstrow))
            rowstogo--;

    xf_flush = TRUE;

    if(firstrow == fstnrx())
        return;

    if(vertvec()->flags & PAGE)
        {
        pagnum++;
        curpnm++;
        }

    filvert(firstrow, firstrow, CALL_FIXPAGE);

    currowoffset = (firstrow < 0)
                        ? (coord) 0
                        : oldcuroff;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        currowoffset = (currowoffset == 0)
                                ? 1
                                : currowoffset-1;

    currow = row_number(currowoffset);
    mark_to_end(n_rowfixes);
}

/******************************************************************************
*
*                           sideways movement
*
******************************************************************************/

/******************************************************************************
*
*  add column at right and move to it
*
******************************************************************************/

extern void
AddColumn_fn(void)
{
    if(createcol(numcol))
        {
        out_rebuildhorz = TRUE;

        internal_process_command(N_LastColumn);
        }
}

/******************************************************************************
*
* move to given column
*
******************************************************************************/

static void
FCO_LCO_common(COL tcol)
{
    chknlr(tcol, currow);

    lecpos = lescrl = 0;
}

/******************************************************************************
*
* move to first column
*
******************************************************************************/

extern void
FirstColumn_fn(void)
{
    FCO_LCO_common((COL) 0);
}

/******************************************************************************
*
* move to last column
*
******************************************************************************/

extern void
LastColumn_fn(void)
{
    FCO_LCO_common(numcol-1);
}

/******************************************************************************
*
*  find the next column to come on the left of the screen; note fixed cols
*
******************************************************************************/

static COL
col_off_left(void)
{
    COL colno = fstncx();

    do  {
        --colno;
        }
    while(  (colno >= 0)                                &&
            (incolfixes(colno)  ||  !colwidth(colno))   );

    return(colno);
}

/******************************************************************************
*
*  move to previous column
*
******************************************************************************/

extern void
prevcol(void)
{
    COL tcol;

    if(!xf_inexpression)        /* RCM thinks lecpos and lescrl will always be zeroed */
        lecpos = lescrl = 0;    /*     when old editor goes                           */

    /* can we move to a column to the left on screen? */
    if(curcoloffset > 0)
        {
        curcol = col_number(--curcoloffset);
        xf_drawcolumnheadings = TRUE;
        mark_row(currowoffset);
        return;
        }

    if((tcol = col_off_left()) < 0)
        {
        xf_flush = TRUE;
        return;
        }

    filhorz(tcol, tcol);
    curcoloffset = 0;
    curcol = col_number(curcoloffset);

    out_screen = TRUE;
}

/******************************************************************************
*
* find the next column on the right
*
******************************************************************************/

extern COL
col_off_right(void)
{
    COL colno = col_number((scrbrc && (curcoloffset != scrbrc)) ? scrbrc-1 : scrbrc);

    do  {
        ++colno;
        }
    while(  (colno < numcol)                            &&
            (incolfixes(colno)  ||  !colwidth(colno))   );

    trace_1(TRACE_APP_PD4, "col_off_right returns %d", colno);
    return(colno);
}

/******************************************************************************
*
*  move to next column
*
******************************************************************************/

extern void
nextcol(void)
{
    COL tcol;
    COL firstcol;
    P_SCRCOL cptr;

    if(!xf_inexpression)        /* RCM thinks lecpos and lescrl will always be zeroed */
        lecpos = lescrl = 0;    /*     when old editor goes                           */

    /* can we move to a column to the right on screen? */
    if(curcoloffset + 1 < scrbrc)
        {
        curcol = col_number(++curcoloffset);
        xf_drawcolumnheadings = TRUE;
        mark_row(currowoffset);
        return;
        }

    tcol = col_off_right();

    if((tcol >= numcol)  ||  (tcol == col_number(curcoloffset)))
        {
        xf_flush = TRUE;
        return;
        }

/* something needed here for situation where new column won't fit
 * on screen completely ?
*/

    for(firstcol = fstncx(); firstcol <= tcol; ++firstcol)
        {
        filhorz(firstcol, tcol);

        if(schcsc(tcol) != NOTFOUND)
            {
            cptr = horzvec_entry(scrbrc);

            if((cptr->flags & LAST)  ||  (cptr->colno != tcol))
                break;
            }
        }

    xf_drawcolumnheadings = out_screen = TRUE;
}

/******************************************************************************
*
* save, restore and swap position of cursor
*
******************************************************************************/

/******************************************************************************
*
* push current slot onto (FA) stack
*
******************************************************************************/

extern void
SavePosition_fn(void)
{
    if(saved_index == SAVE_DEPTH)
        {
        /* lose first stacked position */
        memmove32(&saved_pos[0], &saved_pos[1], sizeof32(SAVPOS) * (SAVE_DEPTH - 2));

        saved_index--;
        }

    saved_pos[saved_index].ref_col = curcol;
    saved_pos[saved_index].ref_row = currow;
    saved_pos[saved_index].ref_docno = current_docno();
    saved_index++;
}

/******************************************************************************
*
*  pop current slot from (FA) stack
*
******************************************************************************/

extern void
RestorePosition_fn(void)
{
    DOCNO old_docno = current_docno();
    DOCNO docno;
    S32 index;

    index = --saved_index;

    if(index < 0)
        {
        saved_index = 0;
        bleep();
        return;
        }

    /* errors in POP are tough */

    docno = saved_pos[index].ref_docno;
    trace_1(TRACE_APP_PD4, "POP to docno %d", docno);

    /* has document been deleted since position saved? */
    if(docno == DOCNO_NONE)
        {
        bleep();
        return;
        }

    select_document_using_docno(docno);

    if(!mergebuf())
        return;

    trace_2(TRACE_APP_PD4, "restoring position to col %d row %d",
            saved_pos[index].ref_col, saved_pos[index].ref_row);

    chknlr(saved_pos[index].ref_col, saved_pos[index].ref_row);
    lecpos = lescrl = 0;

    if(docno != old_docno)
        xf_frontmainwindow = TRUE;
}

extern void
SwapPosition_fn(void)
{
    S32 oldindex = saved_index;
    SAVPOS oldpos;

    oldpos.ref_col = curcol;
    oldpos.ref_row = currow;
    oldpos.ref_docno = current_docno();

    internal_process_command(N_RestorePosition);

    if(oldindex > 0)
        saved_pos[saved_index] = oldpos;

    saved_index++;
}

/******************************************************************************
*
* goto slot
*
******************************************************************************/

extern void
GotoSlot_fn(void)
{
    char tstr[BUF_EV_INTNAMLEN];
    char *extstr;
    S32 count, baddoc;
    COL tcol;
    ROW trow;
    DOCNO docno = DOCNO_NONE;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_GOTO))
        {
        extstr = (uchar *) d_goto[0].textfield;

        while(*extstr++ == SPACE)
            ;
        buff_sofar = --extstr;

        baddoc = 0;

        if(*extstr++ == '[')
            {
            /* read in name to temporary buffer */
            for(count = 0;
                *extstr  &&
                (*extstr != ']')  &&
                (count < EV_INTNAMLEN);
                ++count, ++extstr)
                    tstr[count] = *extstr;

            if(count  &&  (*extstr == ']'))
                {
                tstr[count++] = '\0';
                if(file_is_rooted(tstr))
                    {
                    docno = find_document_using_wholename(tstr);
                    }
                else
                    {
                    docno = find_document_using_leafname(tstr);
                    if(DOCNO_SEVERAL == docno)
                        baddoc = 1;
                    }
                if(DOCNO_NONE == docno)
                    baddoc = 1;
                buff_sofar = extstr;
                }
            }

        if(baddoc)
            {
            reperr_null(create_error(ERR_BAD_SLOT));  /* and let him try again... */
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        tcol = getcol();            /* assumes buff_sofar set */
        trow = (ROW) getsbd();

        if(bad_reference(tcol, trow))
            {
            reperr_null(create_error(ERR_BAD_SLOT));  /* and let him try again... */
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        if(!mergebuf())
            break;

        if(docno != DOCNO_NONE)
            {
            select_document_using_docno(docno);
            xf_frontmainwindow = TRUE;
            }

        chknlr(tcol, trow-1);
        lecpos = lescrl = 0;

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();
}

/******************************************************************************
*
* offset in vertvec that gives vertical position on screen,
* mapping off top/bottom to top/bottom row of data visible on sheet
*
******************************************************************************/

extern coord
calroff_click(
    coord ty)
{
    coord roff = calroff(ty);

    if(roff < 0)
        roff = 0;
    else if(roff > rowsonscreen-1)
        roff = rowsonscreen-1;

    trace_2(TRACE_APP_PD4, "calroff_click(%d) returns %d", ty, roff);
    return(roff);
}

/******************************************************************************
*
* horizontal position on screen of offset in horzvec
*
******************************************************************************/

extern coord
calcad(
    coord coff)
{
    coord sofar = borderwidth;
    P_SCRCOL cptr  = horzvec();
    P_SCRCOL there = cptr + coff;

    while(cptr < there)
        sofar += colwidth(cptr++->colno);

    return(sofar);
}

/******************************************************************************
*
*  offset in horzvec that gives horizontal position on screen
*
******************************************************************************/

extern coord
calcoff(
    coord xpos)
{
    P_SCRCOL cptr;
    coord coff;
    coord sofar;

    if(xpos < 0)
        return(OFF_LEFT);

    if(xpos < borderwidth)
        return(-1);

    /* loop till we've passed the xpos or falled off horzvec */

    cptr  = horzvec();
    coff  = -1;
    sofar = borderwidth;

    while(sofar <= xpos)
        {
        if(cptr->flags & LAST)
            {
            coff = OFF_RIGHT;
            break;
            }

        coff++;

        sofar += colwidth(cptr++->colno);
        }

    trace_2(TRACE_APP_PD4, "calcoff(%d) returns %d", xpos, coff);
    return(coff);
}

/******************************************************************************
*
* offset in horzvec that gives horizontal position on screen,
* mapping off left/right to left/right col of data visible on sheet
*
******************************************************************************/

extern coord
calcoff_click(
    coord xpos)
{
    P_SCRCOL cptr;
    coord coff;
    coord sofar;

    if(xpos < borderwidth)
        return(0);                      /* OFF_LEFT, -1 -> first column */

    /* loop till we've passed the xpos or falled off horzvec */

    cptr  = horzvec();
    coff  = -1;
    sofar = borderwidth;

    while(sofar <= xpos)
        {
        if(cptr->flags & LAST)
            break;                      /* OFF_RIGHT -> last kosher column */

        coff++;

        sofar += colwidth(cptr++->colno);
        }

    trace_2(TRACE_APP_PD4, "calcoff_click(%d) returns %d", xpos, coff);
    return(coff);
}

/*
 * takes real column number, not offset in table
 * is colno a fixed column on screen?
*/

extern BOOL
incolfixes(
    COL colno)
{
    coord coff;

    if(!(horzvec()->flags & FIX))
        return(FALSE);

    coff = schcsc(colno);

    return((coff != NOTFOUND)  &&  (horzvec_entry(coff)->flags & FIX));
}

/*
 * search for row in vertvec
 * return offset of row in table, or NOTFOUND
*/

extern coord
schrsc(
    ROW row)
{
    P_SCRROW i_rptr = vertvec();
    P_SCRROW rptr = i_rptr;

    trace_1(TRACE_APP_PD4, "schrsc(%d)", row);

    while(!(rptr->flags & LAST))
        {
        trace_1(TRACE_APP_PD4, "comparing with row %d\n ", rptr->rowno);

        if((rptr->rowno == row)  &&  !(rptr->flags & PAGE))
            return((coord) (rptr - i_rptr));

        rptr++;
        }

    trace_0(TRACE_APP_PD4, "row not found");
    return(NOTFOUND);
}

/*
 * search for column in horzvec
 * return offset of column in table, or NOTFOUND
*/

extern coord
schcsc(
    COL col)
{
    P_SCRCOL i_cptr = horzvec();
    P_SCRCOL cptr = i_cptr;

    while(!(cptr->flags & LAST))
        {
        if(cptr->colno == col)
            return((coord) (cptr - i_cptr));

        cptr++;
        }

    return(NOTFOUND);
}

/******************************************************************************
*
*  toggle column fix state
*
******************************************************************************/

extern void
FixColumns_fn(void)
{
    uchar flags;
    P_SCRCOL cptr = horzvec();
    P_SCRCOL last_cptr;

    if(cptr->flags & FIX)
        {
        last_cptr = cptr + n_colfixes;
        n_colfixes = 0;
        }
    else
        {
        n_colfixes = curcoloffset + 1;
        last_cptr = cptr + n_colfixes;
        }

    while((cptr < last_cptr)  &&  !((flags = cptr->flags) & LAST))
        cptr++->flags = flags ^ FIX;

    out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
    filealtered(TRUE);
}

/*
 * fill column table horzvec; forces column headings to be redrawn
 *
 * scrbrc: if all columns fit, scrbrc is offset of end marker ie 1+last col
 * if last column doesn't fit, scrbrc is offset of truncated column
*/

extern void
filhorz(
    COL nextcol,
    COL currentcolno)
{
    S32 scrwidth = (S32) cols_available;
    COL trycol;
    COL o_curcol = curcol;
    P_SCRCOL scol = horzvec();
    COL first_col_number = scol->colno;
    COL fixed_colno = first_col_number;
    coord o_colsonscreen  = colsonscreen;
    COL o_ncx = fstncx();
    coord maxwrap = 0;
    coord this_colstart = 0;
    coord this_wrap;
    uchar fixed;
    S32 tlength;
    coord vecoffset = 0;

    trace_2(TRACE_APP_PD4, "filhorz: nextcol %d curcolno %d", nextcol, currentcolno);

    out_rebuildhorz = FALSE;

    colsonscreen = 0;

    while(vecoffset < numcol)
        {
        fixed = (vecoffset < n_colfixes) ? FIX : 0;

        scol = horzvec_entry(vecoffset);

        trace_2(TRACE_APP_PD4, "filhorz: vecoffset %d, fixed %s", vecoffset, trace_boolstring(fixed));

        if(!fixed)
            do  {
                trycol = nextcol++;

                if(trycol >= numcol)
                    {
                    scrbrc = vecoffset;
                    scol->flags = LAST;
                    goto H_FILLED;
                    }
                }
            while(incolfixes(trycol)  ||  !colwidth(trycol));
        else
            trycol = fixed_colno++;

        scol->colno = trycol;
        scol->flags = fixed;

        if(trycol == currentcolno)
            curcoloffset = vecoffset;

        tlength = colwidth(trycol);

        if(tlength)
            {
            /* right margin position of this column */
            this_wrap = this_colstart + colwrapwidth(trycol);
            maxwrap = MAX(maxwrap, this_wrap);

            /* righthand edge of this column */
            this_colstart += tlength;

            scrwidth -= tlength;

            ++colsonscreen;

            if(scrwidth <= 0)
                break;

            ++vecoffset;
            }
        }

    /* three situations:
     * 1) ran out of columns: srcbrc = vecoffset (LAST)
     * 2) filled exactly:     srcbrc = vecoffset + 1 (LAST)
     * 3) ran out of space:   srcbrc = vecoffset (last col)
    */
    scrbrc = vecoffset + (scrwidth == 0);
    (scol+1)->flags = LAST;

H_FILLED:

    curcoloffset = MIN(curcoloffset, colsonscreen-1);
    curcol = col_number(curcoloffset);

    if(curcol != o_curcol)
        lecpos = lescrl = 0;

    /* how long to draw a horizontal bar - rightmost right margin or column edge */
    hbar_length = MAX(this_colstart, maxwrap);

    trace_2(TRACE_APP_PD4, "filhorz: scrbrc := %d, numcol %d", scrbrc, numcol);
    trace_3(TRACE_APP_PD4, "filhorz: colsonscreen %d != o_colsonscreen %d    %s",
            colsonscreen, o_colsonscreen, trace_boolstring(colsonscreen != o_colsonscreen));
    trace_3(TRACE_APP_PD4, "filhorz: col_number(0) %d != first_col_number %d %s",
            col_number(0), first_col_number, trace_boolstring(col_number(0) != first_col_number));
    trace_3(TRACE_APP_PD4, "filhorz: o_ncx %d != fstncx() %d                 %s",
            o_ncx, fstncx(), trace_boolstring(o_ncx != fstncx()));

    /* if different number of columns, need to redraw */
    if( (colsonscreen  != o_colsonscreen)       ||
        (col_number(0) != first_col_number)     ||
        (n_colfixes  &&  (fstncx() != o_ncx))   )
            xf_drawcolumnheadings = out_screen = TRUE;
}

extern void
FixRows_fn(void)
{
    P_SCRROW rptr = vertvec();
    uchar flags;

    if(rptr->flags & FIX)
        {
        n_rowfixes = 0;

        while(!((flags = rptr->flags) & LAST))
            rptr++->flags = flags & ~FIX;       /* clear fix bit */

        /* reevaluate page numbers */
        update_variables();
        }
    else
        {
        ROW thisrow = rptr->rowno;

        while((thisrow <= currow)  &&  !((flags = rptr->flags) & LAST))
            {
            rptr->rowno   = thisrow++;
            rptr++->flags = flags | FIX;
            }

        n_rowfixes = (coord) (currow - row_number(0) + 1);
        }

    out_rebuildvert = out_screen = TRUE;
    filealtered(TRUE);
}

/******************************************************************************
*
*  centre column on screen
*
******************************************************************************/

extern void
cencol(
    COL colno)
{
    S32 stepback;
    S32 firstcol;
    coord fixwidth = 0;
    P_SCRCOL cptr = horzvec();

    trace_1(TRACE_APP_PD4, "cencol(%d)", colno);

    colno = MIN(colno, numcol-1);

    while(!(cptr->flags & LAST)  &&  (cptr->flags & FIX))
        fixwidth += colwidth(cptr++->colno);

    stepback = (cols_available - fixwidth) / 2;
    stepback = MAX(stepback, 0);

    for(firstcol = (S32) colno; (stepback > 0) && (firstcol >= 0); --firstcol)
        {
        while((firstcol > 0)  &&  (incolfixes((COL) firstcol) || !colwidth(firstcol)))
            --firstcol;

        stepback -= colwidth((COL) firstcol);
        }

    firstcol += 1 + (stepback < 0);
    firstcol = MIN(firstcol, colno);

    filhorz((COL) firstcol, colno);
}

/******************************************************************************
*
* centre row on screen
*
******************************************************************************/

extern void
cenrow(
    ROW rowno)
{
    S32 stepback;
    ROW firstrow;

    rowno = MIN(rowno, numrow - 1);

    stepback = (rows_available - n_rowfixes - 1) / 2;

    for(firstrow = rowno; (stepback > 0)  &&  (firstrow >= 0); --firstrow, --stepback)
        while(inrowfixes(firstrow)  &&  (firstrow > 0))
            --firstrow;

    firstrow = MAX(firstrow, 0);

    /* ensure a page-break at tos doesn't confuse things */
/*  vertvec->flags &= ~PAGE; */
    out_screen = TRUE;

    filvert(firstrow, rowno, CALL_FIXPAGE);
}

/******************************************************************************
*
*  fill vertical screen vector
*
******************************************************************************/

extern void
filvert(
    ROW nextrow,
    ROW currentrowno,
    BOOL call_fixpage)
{
    P_SCRROW rptr;
    coord vecoffset;
    coord lines_on_screen = rows_available;
    S32 temp_pagoff;
    uchar saveflags;
    ROW first_row_number = row_number(0);
    P_SLOT tslot;
    BOOL on_break;
    S32 pictrows;

    pict_currow = currow;
    pict_on_currow = 0;

    out_forcevertcentre = out_rebuildvert = FALSE;

    trace_3(TRACE_APP_PD4, "************** filvert(%d, %d, %s)",
            nextrow, currentrowno, trace_boolstring(call_fixpage));

    if(nextrow < 0)
        nextrow = 0;

    /* update the page numbering if allowed to and no fixes */
    if(call_fixpage  &&  !n_rowfixes  &&  chkfsb())
        fixpage(fstnrx(), nextrow);

    curpnm = pagnum;
    temp_pagoff = pagoff;

    for(vecoffset = 0, rowsonscreen = 0, pictrows = 0, pict_on_screen = 0;
        lines_on_screen > 0;
        nextrow++, rowsonscreen++, lines_on_screen--, vecoffset++)
        {
        on_break = FALSE;

        rptr = vertvec_entry(vecoffset);

        saveflags = rptr->flags & (~FIX & ~LAST & ~PAGE & ~PICT & ~UNDERPICT);

        trace_2(TRACE_APP_PD4, "filvert - rptr " PTR_XTFMT ", %d", report_ptr_cast(rptr), vecoffset);

        if(vecoffset < n_rowfixes)
            {
            rptr->rowno = first_row_number + vecoffset;
            saveflags |= FIX;
            }

        if(nextrow >= numrow)
            {
            rptr->flags = LAST;
            rptr->rowno = 0;
            trace_1(TRACE_APP_PD4, "filvert pict_on_screen is: %d", pict_on_screen);
            return;
            }

        /* if not on a fixed row */
        if(!(saveflags & FIX))
            {
            while(inrowfixes(nextrow))
                nextrow++;

            if(nextrow >= numrow)
                {
                rptr->flags = LAST;
                rptr->rowno = 0;
                trace_1(TRACE_APP_PD4, "filvert pict_on_screen is: %d", pict_on_screen);
                return;
                }

            /* do pages if rows not fixed */
            if(chkfsb())
                {
                on_break = TRUE;
                /* on break */

                /* if hard break - do chkrpb(nextrow) explicitly to save time */
                tslot = travel(0, nextrow);
                if(tslot  &&  (tslot->type == SL_PAGE))
                    {
                    if(chkpbs(nextrow, temp_pagoff))
                        temp_pagoff = 0;
                    else if(temp_pagoff == 0)
                        saveflags |= PAGE;
                    else
                        {
                        temp_pagoff -= enclns;
                        on_break = FALSE;
                        }
                    }
                else if(temp_pagoff == 0)
                    saveflags |= PAGE;
                else
                    on_break = FALSE;
                }
            }

        if(!on_break && !(saveflags & PAGE))
            {
            P_DRAW_DIAG p_draw_diag;
            drawfrp dfrp;

            /* check for a picture */
            if(draw_file_refs.lbr &&
               draw_find_file(current_docno(),
                              -1, nextrow, &p_draw_diag, &dfrp))
                {
                if(nextrow == currentrowno)
                    pict_on_currow = TRUE;
                else
                    {
                    trace_2(TRACE_APP_PD4, "filvert found draw file row: %d, %d rows",
                            nextrow, tsize_y(dfrp->ysize_os));
                    saveflags |= PICT;
                    pictrows = tsize_y(dfrp->ysize_os);
                    }
                }
            }

        if(!(saveflags & FIX))
            {
            rptr->rowno = nextrow;
            rptr->page  = curpnm;

            /* chkpba() */
            if(encpln > 0)
                {
                /* add line spacing */
                temp_pagoff += enclns;
                if(temp_pagoff > encpln)
                    temp_pagoff = 0;
                }

            if(on_break)
                {
                curpnm++;
                pictrows = 0;
                }

            if(saveflags & PAGE)
                {
                nextrow--;
                pictrows = 0;
                }
            }

        if(pictrows)
            {
            pictrows--;
            saveflags |= UNDERPICT;
            }

        if(saveflags & PICT)
            plusab(pict_on_screen, ((S32) nextrow + 1) * (rowsonscreen + 1));

        rptr->flags = saveflags;

        if(saveflags & FIX)
            nextrow--;

        /* mark current row position in vector */
        if((rptr->rowno == currentrowno)  &&  !(saveflags & PAGE))
            currowoffset = rowsonscreen;
        }

    vertvec_entry(rowsonscreen)->flags = LAST;

    trace_1(TRACE_APP_PD4, "filvert pict_on_screen is: %d", pict_on_screen);
}

/******************************************************************************
*
* is this row fixed on the screen ?
* real row number, not offset
*
******************************************************************************/

extern BOOL
inrowfixes(
    ROW row)
{
    return(n_rowfixes  &&  inrowfixes1(row));
}

extern BOOL
inrowfixes1(
    ROW row)
{
    coord roff = schrsc(row);

    return((roff != NOTFOUND)  &&  (vertvec_entry_flags(roff) & FIX));
}

/******************************************************************************
*
* check row for conditional page break
* real row number, not offset
*
******************************************************************************/

extern BOOL
chkrpb(
    ROW rowno)
{
    P_SLOT tslot = travel(0, rowno);

    return(tslot  &&  (tslot->type == SL_PAGE));
}

/*
 * set hard page offset and return whether active
*/

extern BOOL
chkpbs(
    ROW rowno,
    S32 offset)
{
    travel(0, rowno)->content.page.cpoff = offset;

    return(chkpac(rowno));
}

/*
 * check if hard page break is expanded
 *
 * hard page break stores offset of line in current page
*/

extern BOOL
chkpac(
    ROW rowno)
{
    P_SLOT tslot = travel(0, rowno);
    S32 condval = tslot->content.page.condval;
    S32 rowonpage = tslot->content.page.cpoff;

    return((condval == 0)  ||  (condval >= (encpln-rowonpage) / enclns));
}

/******************************************************************************
*
* get first non fixed column on screen
* if all fixed, return first column
* returns actual column number
*
******************************************************************************/

extern COL
fstncx(void)
{
    P_SCRCOL i_cptr = horzvec();
    P_SCRCOL cptr = i_cptr - 1;

    while(!((++cptr)->flags & LAST))
        if(!(cptr->flags & FIX))
            return(cptr->colno);

    return(i_cptr->colno);
}

/******************************************************************************
*
* get first non fixed row on screen
* if all fixed, return first row
* returns actual row number
*
******************************************************************************/

extern ROW
fstnrx(void)
{
    P_SCRROW i_rptr = vertvec();
    P_SCRROW rptr = i_rptr - 1;

    while(!((++rptr)->flags & LAST))
        if(!(rptr->flags & (FIX | PAGE)))
            return(rptr->rowno);

    return(i_rptr->rowno);
}

/******************************************************************************
*
* update all the variables that depend upon the page layout options box
* and the global options box
*
******************************************************************************/

extern void
update_variables(void)
{
    S32   idx;
    S32   linespace;
    ROW  oldtop;
    BOOL  old_borbit = borbit;

    iowbit = (uchar) (d_options_IW == 'R'); /* insert on wrap */
    txnbit = (uchar) (d_options_TN == 'T'); /* text/numbers */
    borbit = (uchar) (d_options_BO == 'Y'); /* borders on/off */

    /*
     * note that all the following variables depend on borbit
     * this should only happen in the global options menu
    */
    borderwidth  = (borbit) ? BORDERWIDTH  : 0;

    if((rows_available = paghyt + 1) < 0)
        rows_available = 0;
    if((cols_available = pagwid_plus1 - borderwidth) < 0)
        cols_available = 0;

    jusbit = (uchar) (d_options_JU == 'Y'); /* justified text */
    wrpbit = (uchar) (d_options_WR == 'Y'); /* wrapped text */
    minbit = (uchar) (d_options_MB == 'M'); /* minus/brackets */

    linespace = (S32) d_poptions_LS;
    if(linespace < 1)
        linespace = 1;

    idx =     (S32) d_poptions_PL
            - (S32) d_poptions_TM
            - (S32) d_poptions_HM
            - (S32) d_poptions_FM
            - (S32) d_poptions_BM;

    if(!str_isblank(d_poptions_HE))
        idx--;

    if(!str_isblank(d_poptions_H1))
        idx--;
    if(!str_isblank(d_poptions_H2))
        idx--;

    if(!str_isblank(d_poptions_FO))
        idx--;

    if(!str_isblank(d_poptions_F1))
        idx--;
    if(!str_isblank(d_poptions_F2))
        idx--;

    /* don't let encpln go to dangerous values */

    encpln = (idx <= linespace) ? 0 : idx;
    enclns = linespace;

    real_pagelength = encpln;

    /* set encpln to next multiple of enclns */

    while((encpln % enclns) != 0)
        encpln++;

    if(!n_rowfixes  &&  vertvec_mh)
        {
        oldtop = fstnrx();
        filvert((ROW) 0, (ROW) 0, CALL_FIXPAGE);
        reset_filpnm();
        filvert(oldtop, currow, CALL_FIXPAGE);
        }

    out_screen = out_rebuildhorz = out_rebuildvert = out_currslot = TRUE;

    grid_on = (d_options_GR == 'Y');
    new_grid_state();
    /* may need to move caret if grid state different */
    position_cursor();
    xf_caretreposition = TRUE;

    (void) new_window_height(windowheight());

    if((old_borbit != borbit)       &&
       (rear_window != window_NULL) &&
       (main_window != window_NULL) &&  /* cos we may be called before windows exist!!! */
       (colh_window != window_NULL)
      )
        {
        /* the border state has changed, so reopen the rear_window at its current position */
        /* allowing openpane to show/hide the colh_window as appropriate                   */

        wimp_wstate rear_wstate;

        /* if we are losing the border (and hence the formula line) and the */
        /* formula line is in use, transfer its text to a formula window    */

        if(old_borbit && xf_inexpression_line)
            {
            expedit_transfer_line_to_box(FALSE);        /* don't force a newline */

            if(xf_inexpression_line)
                formline_canceledit();          /* didn't work! so must abort the edit, cos the formula line will disappear */
            }

        if(borbit)                      /* if a mode change occured whilst borders were off, various icons will */
            colh_position_icons();      /* have moved left by the width of the row border!!, so put them back   */

        wimp_get_wind_state(rear_window, &rear_wstate);
        win_send_open(rear_window, TRUE, &rear_wstate.o);
        }
    else
        {
#if TRUE
        riscos_invalidatemainwindow(); /* (and colh_window) */
#else
        my_force_redraw(main_window);  /*RCM says, routines were similar, so I */
                                       /*    removed my_force_redraw           */
#endif
        }
}

/******************************************************************************
*
* get size of string accounting for highlights etc
* ie. length of string when drawn on screen by draw_slot()
*
******************************************************************************/

extern S32
calsiz(
    uchar *str)
{
    S32 size;

    for(size = 0; *str /*!= '\0'*/; str++)
        if(!ishighlight(*str))
            size++;

    return(size);
}

/*
*/

static void
rebols(void)
{
    newcol = oldcol;
    newrow = oldrow;
    chkmov();
    curosc();
}

/*
 * all calls to this should be followed by a curosc()
*/

extern void
chkmov(void)
{
    COL  tcol;
    coord coff;
    coord roff, troff;

    trace_4(TRACE_APP_PD4, "chkmov(): newcol %d newrow %d oldcol %d oldrow %d", newcol, newrow, oldcol, oldrow);

    if(!colwidth(newcol))
        {
        trace_1(TRACE_APP_PD4, "chkmov: colwidth(%d) == 0 --- silly!", newcol);

        /* find somewhere to go then! */
        tcol = newcol;

        while((tcol >= 0)  &&  !colwidth(tcol))
            --tcol;

        if(tcol == -1)
            {
            trace_0(TRACE_APP_PD4, "chkmov: fell off left; try marching right");
            tcol = newcol;
            while((tcol < numcol)  &&  !colwidth(tcol))
                ++tcol;
            }

        trace_2(TRACE_APP_PD4, "chkmov: colwidth(%d) == 0; setting newcol %d", newcol, (tcol != numcol) ? tcol : curcol);
        newcol = (tcol != numcol) ? tcol : curcol;
        }

    curcol = newcol;

    if(oldcol != newcol)
        xf_drawcolumnheadings = TRUE;

    coff = schcsc(newcol);

    /* when coff == scrbrc, only part of the column is on the screen */
    if((coff == NOTFOUND)  ||  (coff == scrbrc))
        {
        out_screen = TRUE;
        cencol(newcol);
        }
    else
        curcoloffset = coff;

    trace_3(TRACE_APP_PD4, "chkmov(): row: old %d cur %d new %d", oldrow, currow, newrow);

    currow = newrow;

    if( (inrowfixes(newrow)  &&  out_forcevertcentre)   ||
        ((roff = schrsc(newrow)) == NOTFOUND)           )
        {
        cenrow(newrow);
        out_screen = TRUE;
        }
    else
        {
        troff = schrsc(oldrow);

        if(troff != NOTFOUND)
            {
            if(newcol == oldcol)
                mark_row_praps(troff, NEW_ROW);
            else
                mark_row(troff);
            }

        currowoffset = roff;

        if((newcol == oldcol)  &&  !word_to_invert)
            mark_row_praps(currowoffset, NEW_ROW);
        else
            mark_row(currowoffset);
        }
}

extern void
curosc(void)
{
    BOOL had_caret = (main_window == caret_window);
    BOOL maybe_caretreposition = FALSE;
    P_SCRCOL cptr;
    P_SCRROW rptr;

    if(curcoloffset >= scrbrc)
        {
        /* if nothing on screen */

        cptr = horzvec();

        if(!scrbrc  &&  (cptr->flags & LAST))
            {
            rebols();
            return;
            }

        curcoloffset = (cptr[scrbrc].flags & LAST)
                            ? (scrbrc-1)
                            : scrbrc;

        curcol = cptr[curcoloffset].colno;

        maybe_caretreposition = TRUE;
        }

    rptr = vertvec();

    if(rptr->flags & LAST)
        {
        rebols();
        return;
        }

    /* row not off end, should also check page break */

    if(currowoffset > rowsonscreen-1)
        {
        currowoffset = rowsonscreen-1;
        /* SKS after 4.11 09jan92 - attempt to bring cursor back on screen in kosher fashion */
        mark_row_border(currowoffset);
        maybe_caretreposition = TRUE;
        }

    if(rptr[currowoffset].flags & PAGE)
        {
        rebols();
        return;
        }

    currow = rptr[currowoffset].rowno; /* must be outside previous block */

    if(maybe_caretreposition)
        if(had_caret)
            xf_caretreposition = TRUE;
}

/******************************************************************************
*
*  check slot is blank
*
******************************************************************************/

extern BOOL
isslotblank(
    P_SLOT tslot)
{
    uchar *str;
    uchar ch;

    if(!tslot)
        return(TRUE);

    switch(tslot->type)
        {
        case SL_TEXT:
            str = tslot->content.text;
            break;

        default:
            return(FALSE);
        }

    /* check if only characters in slot are spaces
     * Is done explicitly for speed
    */
    do { ch = *str++; } while(ch == SPACE);

    return(ch == '\0');
}

/******************************************************************************
*
*  decide what column a click at the given text cell would go into
*
******************************************************************************/

/* offset of given text cell in column that we decided upon */
S32 g_newoffset; /* needed in markers.c */

extern coord
get_column(
    coord tx,
    ROW trow,
    S32 xcelloffset,
    BOOL selectclicked)
{
    coord coff = calcoff_click(tx); /* actual grid address with l/r map */
    coord trycoff;
    COL  tcol;
    P_SLOT tslot;
    S32 tryoffset;

    /* ensure we can find caret in current slot! */
    (void) mergebuf_nocheck();
    filbuf();

    g_newoffset = 0;

    if(selectclicked)   /* selectclicked means place caret where pd wants to, else force into col user clicked in*/
        {
        trycoff = coff;

        trace_0(TRACE_APP_PD4, "SELECT: find the leftmost column");

        /* find any preceding non-blank slot,
         * or stop at first column on screen.
        */
        do  {
            tryoffset = tx - calcad(trycoff);
            tcol  = col_number(trycoff);
            tslot = travel(tcol, trow);
            }
        while(!tslot  &&  (--trycoff >= 0));

        /* Manic jump-left always clicking */
        /* if there is a non-blank text slot which overlaps, have it, otherwise
            have the column we appear to be in
        */
        if(tslot && (tslot->type == SL_TEXT))
            {
            g_newoffset = cal_offset_in_slot(tcol, trow, tslot, tryoffset, xcelloffset);
            trace_2(TRACE_APP_PD4, "preceding text slot at coff %d, g_newoffset %d", trycoff, g_newoffset);
            return(trycoff);
            }
        else
            {
            trace_0(TRACE_APP_PD4, "preceding non-text slot: first non-blank one");
            return(coff);
            }
        }

    trace_0(TRACE_APP_PD4, "ADJUST: found the underlying column");

    tcol  = col_number(coff);
    tslot = travel(tcol, trow);

    if(tslot  &&  (tslot->type == SL_TEXT))
        g_newoffset = cal_offset_in_slot(tcol, trow, tslot, tx - calcad(coff), xcelloffset);

    return(coff);
}

/******************************************************************************
*
* given a slot and a character + cell offset, work
* out the new cursor offset position in the slot
*
* MRJC 19/7/89
*
******************************************************************************/

static S32
cal_offset_in_slot(
    COL col,
    ROW row,
    P_SLOT sl,
    S32 offset_ch,
    S32 cell_offset_os)
{
    BOOL in_linbuf;
    S32 justify;
    S32 fwidth_ch, swidth_ch, count, gaps, whole = 0, odd = 0, non_odd_gaps = 0, lead;
    char *c;
    S32 fwidth_adjust_ch;
    char tbuf[PAINT_STRSIZ];
    S32 success;
    font_string fs;
    S32 fwidth_mp, swidth_mp;
    S32 this_font;
    char wid_buf[PAINT_STRSIZ];
    S32 lead_spaces, lead_space_mp;
    S32 spaces;
    S32 text_length;

    in_linbuf = is_protected_slot(sl)
                        ? FALSE
                        : ((col == curcol)  &&  (row == currow));

    justify = in_linbuf ? 0 : (sl->justify & J_BITS);

    fwidth_ch = chkolp(sl, col, row);

    trace_4(TRACE_APP_PD4, "cal_offset_in_slot: (%d, %d): offset_ch = %d, cell_offset = %d (OS)",
            col, row, offset_ch, cell_offset_os);

    if( offset_ch < 0)
        offset_ch = cell_offset_os = 0;

    if(riscos_fonts)
        {
        success = FALSE;

        fs.s = tbuf;
        fs.x = os_to_mp(ch_to_os(offset_ch) + cell_offset_os);
        fs.y = 0;

        trace_4(TRACE_APP_PD4, "offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d",
                offset_ch, cell_offset_os, fs.x, fs.y);

        if(in_linbuf)
            /* get current fonty string */
            expand_current_slot_in_fonts(tbuf, FALSE, &this_font);
        else
            /* get a fonty string */
            font_expand_for_break(tbuf, sl->content.text);

        swidth_mp = font_width(tbuf);

        /* never look for characters that won't be plotted */
        switch(justify)
            {
        /*  case J_CENTRE:      */
        /*  case J_RIGHT:       */
        /*  case J_LCR:         */
        /*  case J_LEFTRIGHT:   */
        /*  case J_RIGHTLEFT:   */
            default:
                fwidth_adjust_ch = 1;
                break;

            case J_FREE:
            case J_LEFT:
                fwidth_adjust_ch = 0;
                break;
            }

        fwidth_ch -= fwidth_adjust_ch;
        fwidth_mp = ch_to_mp(fwidth_ch);

        trace_1(TRACE_APP_PD4, "cal_offset_in_slot: fwidth_ch = %d", fwidth_ch);
        trace_2(TRACE_APP_PD4, "cal_offset_in_slot: fwidth_mp = %d, swidth_mp = %d", fwidth_mp, swidth_mp);

        if( swidth_mp > fwidth_mp)
            {
            swidth_mp = font_truncate(tbuf, fwidth_mp + ch_to_mp(fwidth_adjust_ch));

            justify = 0;
            }

        /* what a muddle are the font calls; they NEVER do
         * what you want, resulting in messes like this...
        */
        if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
            {
            /* strip the goddamn leading spaces */
            lead_space_mp = font_strip_spaces(wid_buf, tbuf, &lead_spaces);

            trace_3(TRACE_APP_PD4, "lead_spaces: %d, lead_space_mp: %d, fs.x: %d",
                    lead_spaces, lead_space_mp, fs.x);

            /* see if we clicked past the leading spaces */
            if(fs.x > lead_space_mp)
                {
                fs.s = wid_buf;
                fs.x -= lead_space_mp;
                fwidth_mp -= lead_space_mp;
                swidth_mp = font_width(wid_buf);

                /* do you believe that you have to do this ? */
                c = wid_buf;
                spaces = 0;
                while(*c)
                    if(*c == SPACE)
                        {
                        ++c;
                        ++spaces;
                        continue;
                        }
                    else
                        c += font_skip(c);
                /* I didn't */

                trace_4(TRACE_APP_PD4, "fwidth_mp: %d, swidth_mp: %d, lead_space_mp: %d, spaces: %d",
                        fwidth_mp, swidth_mp, lead_space_mp, spaces);

                if(spaces  &&  (swidth_mp > 0)  &&  (fwidth_mp >= swidth_mp))
                    {
                    font_complain(
                        font_findcaretj(&fs,
                                        (fwidth_mp - swidth_mp) / spaces,
                                        0)
                                 );
                    fs.term += lead_spaces;
                    success = TRUE;
                    /* well, everything's relative */

                    trace_5(TRACE_APP_PD4, "offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d, fs.term: %d",
                            offset_ch, cell_offset_os, fs.x, fs.y, fs.term);
                    }
                }
            }

        /* at least it's fairly simple in the unjustified case */
        if(!success)
            {
            _kernel_swi_regs rs;
            int plottype = (1 << 17);

            if(d_options_KE == 'Y')
                plottype |= (1 << 9);

            rs.r[0] = 0;
            rs.r[1] = (int) fs.s;
            rs.r[2] = plottype;
            rs.r[3] = fs.x;
            rs.r[4] = fs.y;
            rs.r[5] = 0;
            rs.r[6] = 0;

            font_complain((os_error *) _kernel_swi(0x400A1 /*Font_ScanString*/, &rs, &rs));

            fs.term = rs.r[1] - (int) fs.s;

            trace_5(TRACE_APP_PD4, "offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d, fs.term: %d",
                    offset_ch, cell_offset_os, fs.x, fs.y, fs.term);
            }

        /* skip fonty stuff to get real position */
        c = tbuf;
        offset_ch = in_linbuf ? lescrl : 0;
        while(*c  &&  (fs.term > 0))
            if(is_font_change(c))
                {
                /* miss initial font change */
                if(c != tbuf)
                    ++offset_ch;
                fs.term -= font_skip(c);
                c += font_skip(c);
                }
            else
                {
                if(in_linbuf)
                    {
                    char ch = linbuf[offset_ch];

                    trace_4(TRACE_APP_PD4, "cal_offset_in_slot: linbuf[%d] = %c %d, fs.term = %d", offset_ch, ch, ch, fs.term);

                    /* if clicked in the middle of a highlight/expanded ctrlchar, maybe put caret to left */
                    if(ishighlight(ch))
                        {
                        if(fs.term < 2)
                            break;

                        fs.term -= 3 - 1;
                        }
                    else if((ch < SPACE)    ||
                         ((ch >= 127)  &&  (ch < 160)  &&  !font_charwid(this_font, ch)))
                        {
                        if(fs.term < 3)
                            break;

                        fs.term -= 4 - 1;
                        }
                    }
                --fs.term;
                ++offset_ch;
                ++c;
                }
        }
    else
        {
        if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
            {
            swidth_ch = 0;

            c = sl->content.text;

            while(*c++ == SPACE)
                ++swidth_ch;
            --c;

            for(gaps = 0; *c; ++c)
                {
                /* highlights have zero width */
                if(ishighlight(*c))
                    continue;

                /* count gaps */
                if(*c == SPACE)
                    {
                    do { ++swidth_ch; } while(*++c == SPACE);

                    if(*c--)
                        ++gaps;

                    continue;
                    }

                /* stop on <t-a-c>A1<t-a-c> field */
                if(SLRLD1 == *c)
                    {
                    justify = 0;
                    break;
                    }

                ++swidth_ch;
                }

            if((swidth_ch >= fwidth_ch)  ||  !gaps)
                justify = 0;

            if(justify)
                {
                whole = (fwidth_ch - swidth_ch) / gaps;
                odd = fwidth_ch - (whole * gaps) - swidth_ch;
                if(justify == J_LEFTRIGHT)
                    non_odd_gaps = 0;
                else
                    non_odd_gaps = gaps - odd;
                }

            trace_6(TRACE_APP_PD4, "cal_off gaps: %d, fwidth: %d, swidth: %d, whole: %d, odd: %d, non_oddg: %d",
                    gaps, fwidth_ch, swidth_ch, whole, odd, non_odd_gaps);
            }
        else
            justify = 0;

        /* account for highlights */
        if(!in_linbuf)
            for(c = sl->content.text, count = offset_ch, gaps = 0, lead = 1;
                *c && count > 0;
                ++c, --count)
                switch(*c)
                    {
                    case SLRLD1: /* give up if we get an SLRLD1 - too hard */
                        count = 0;
                        break;

                    case HIGH_UNDERLINE:
                    case HIGH_BOLD:
                    case HIGH_ITALIC:
                    case HIGH_SUBSCRIPT:
                    case HIGH_SUPERSCRIPT:
                        ++offset_ch;
                        ++count;
                        lead = 0;
                        break;

                    case SPACE:
                        {
                        S32 to_remove, odd_used;

                        if(lead || !justify)
                            break;
                        ++gaps;
                        odd_used = (odd && (gaps > non_odd_gaps)) ? 1 : 0;
                        to_remove = whole + odd_used;
                        offset_ch -= to_remove;
                        count -= to_remove;
                        odd -= odd_used;
                        trace_3(TRACE_APP_PD4, "gaps: %d, odd_used: %d, to_remove: %d",
                                gaps, odd_used, to_remove);
                        }

                    default:
                        lead = 0;
                        break;
                    }

        /* move on one if click in RHS of character */
        if(cell_offset_os > (charwidth*3/4))
            ++offset_ch;
        }

    trace_0(TRACE_APP_PD4, "");

    /* check that offset is inside string */
    if(in_linbuf)
        text_length = (S32) strlen(linbuf);
    else
        text_length = compiled_text_len(sl->content.text) - 1; /*included NULLCH*/

    return(MIN(offset_ch, text_length));
}

/******************************************************************************
*
*  insert a reference to a (possibly) external slot in the given document
*
******************************************************************************/

extern void
insert_reference_to(
    _InVal_     DOCNO docno_ins,
    _InVal_     DOCNO docno_to,
    COL tcol,
    ROW trow,
    BOOL abscol,
    BOOL absrow,
    BOOL allow_draw)
{
    char array[LIN_BUFSIZ];
    DOCNO old_docno;
    EV_SLR slr;

    trace_4(TRACE_APP_PD4, "inserting reference to docno %u, col #%d, row #%d in docno %u", docno_to, tcol, trow, docno_ins);

    old_docno = change_document_using_docno(docno_ins);

    /* remember where we put it for case of drag */
    insert_reference_stt_offset = lecpos;

    insert_reference_docno   = docno_to;
    insert_reference_slr.col = tcol;
    insert_reference_slr.row = trow;
    insert_reference_abs_col = abscol;
    insert_reference_abs_row = absrow;

    set_ev_slr(&slr, tcol, trow);

    /* make external reference if in another document */
    if(docno_ins != docno_to)
        slr.docno = docno_to;

    /* set absolute bits */
    if(abscol)
        slr.flags |= SLR_ABS_COL;
    if(absrow)
        slr.flags |= SLR_ABS_ROW;

    /* expand column then row into array */
    (void) write_slr_ref(array, elemof32(array), &slr);

    insert_string(array, FALSE);

    /* store position of inserted string, so it can be replaced with sorted reference range */
    /* if this turns into a drag operation!                                                 */
    if(xf_inexpression_box || xf_inexpression_line)
        {
        expedit_get_cursorpos(&insert_reference_end_offset, &insert_reference_row_number);
        insert_reference_stt_offset = insert_reference_end_offset - strlen(array);
        }
    else
        {
        insert_reference_row_number = 0;
        insert_reference_end_offset = lecpos;
      /*insert_reference_stt_offset set earlier on */
        }

    if(allow_draw)
        {
        draw_screen();              /* in expr. editing doc */
        draw_caret();
        }

    select_document_using_docno(old_docno);
}

/* end of cursmov.c */
