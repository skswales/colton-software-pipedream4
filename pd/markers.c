/* markers.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Marked block handling */

/* RJM August 1987; SKS May 1989 */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscdraw.h"
#include "riscos_x.h"

#include "colh_x.h"

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

#define TRACE_DRAG (TRACE_APP_PD4)
#define TRACE_MARK (TRACE_APP_PD4)

/* what sort of drag is in progress */
int drag_type = NO_DRAG_ACTIVE;
DOCNO drag_docno = DOCNO_NONE;

static BOOL drag_autoscroll = FALSE;

/* previously marked rectangle - intersect for fast updates */
static SLR old_blkstart;
static SLR old_blkend;

/* sort a pair of column/row objects into numerical order */

static void
sort_colt(
    _InoutRef_  P_COL aa,
    _InoutRef_  P_COL bb)
{
    const COL a = *aa;
    const COL b = *bb;

    if(b < a)
    {
        *bb = a;
        *aa = b;
    }
}

static void
sort_rowt(
    _InoutRef_  P_ROW aa,
    _InoutRef_  P_ROW bb)
{
    const ROW a = *aa;
    const ROW b = *bb;

    if(b < a)
    {
        *bb = a;
        *aa = b;
    }
}

/******************************************************************************
*
*   rectangle of marked cells has changed
*
* --in--
*   assumes mergebuf has been done much
*   further up for cell correctness
*
*
* --out--
*   flag indicates that some cells need
*   redrawing rather than merely inverting
*
******************************************************************************/

static BOOL
new_marked_rectangle(void)
{
    BOOL mustdraw = (log2bpp >= 3); /* can't invert in 256 or more colour modes */
    BOOL needs_drawsome = FALSE;
    coord coff, start_coff, end_coff;
    coord roff;
    COL  tcol;
    ROW  trow;
    P_CELL tcell;
    BOOL in_new_rowr, in_old_rowr;
    BOOL in_new_rect, in_old_rect;
    P_SCRCOL cptr;
    P_SCRROW rptr;
    coord c_width, overlap;

    trace_4(TRACE_MARK, "new_marked_rectangle(%d, %d, %d, %d): ",
            blkstart.col, blkstart.row, blkend.col, blkend.row);
    trace_4(TRACE_MARK, "old rectangle was (%d, %d, %d, %d)",
            old_blkstart.col, old_blkstart.row,
            old_blkend.col,   old_blkend.row);

    /* loop over all visible cells and 'update' cells in
     * visible range of changing rows and columns
    */
    assert(vertvec_entry_valid(0));
    for(roff = 0; !((rptr = vertvec_entry(roff))->flags & LAST); roff++)
        if(!(rptr->flags & PAGE))
        {
            start_coff = end_coff = -1;

            trace_2(TRACE_MARK, "row offset %d, PICT | UNDERPICT = %d",
                        roff, rptr->flags & (PICT | UNDERPICT));

            trow = rptr->rowno;

            assert(horzvec_entry_valid(0));
            tcell = travel(col_number(0), trow);
            if(tcell  &&  (tcell->type == SL_PAGE))
                continue;

            in_new_rowr =   (blkstart.row <= trow)      &&
                            (trow <= blkend.row);

            in_old_rowr =   (old_blkstart.row <= trow)  &&
                            (trow <= old_blkend.row);

            assert(horzvec_entry_valid(0));
            for(coff = 0; !((cptr = horzvec_entry(coff))->flags & LAST); coff++)
            {
                trace_1(TRACE_MARK, "column offset %d", coff);

                tcol = cptr->colno;

                in_new_rect =   (blkstart.col <= tcol)      &&
                                (tcol <= blkend.col)        &&
                                in_new_rowr;

                in_old_rect =   (old_blkstart.col <= tcol)  &&
                                (tcol <= old_blkend.col)    &&
                                in_old_rowr;

                if(in_new_rect != in_old_rect)
                {
                    if( mustdraw  ||                                                /* forced to draw? */
                        (rptr->flags & (PICT | UNDERPICT))  ||                      /* picture on row? */
                        (   ((tcell = travel(tcol, trow)) == NULL)
                                ?   is_overlapped(coff, roff)                           /* empty cell overlapped from left? */
                                :   (   /*riscos_fonts  ||*/                            /* cell with fonts needs drawing */
                                        (   !slot_displays_contents(tcell)              /* text cells might need drawing: */
                                            ? FALSE
                                            :   (   ((c_width = colwidth(tcol)) < (overlap = chkolp(tcell, tcol, trow)))  ||
                                                    /* if cell overlaps to right */
                                                    (FALSE  &&  grid_on  &&  (c_width > overlap))
                                                    /* or if grid & shorter than colwidth */
                                                )
                                        )
                                    )
                        )
                      )
                    {
                        trace_5(TRACE_APP_PD4, "disasterville: draw this whole row and breakout because mustdraw %s, PICT %s, tcell " PTR_XTFMT " -> empty overlapped %s, text | fonts %s",
                                report_boolstring(mustdraw),
                                report_boolstring(rptr->flags & (PICT | UNDERPICT)),
                                report_ptr_cast(tcell),
                                report_boolstring(tcell ? FALSE : is_overlapped(coff, roff)),
                                report_boolstring(tcell ? ((tcell->type == SL_TEXT) || riscos_fonts) : FALSE));
                        mark_row(roff);
                        draw_screen();

                        if(xf_interrupted)
                            goto NO_MORE_AT_ALL;

                        goto NO_MORE_THIS_ROW;
                    }
                    else
                    {
                        COLOURS_OPTION_INDEX fg_colours_option_index = COI_FORE;
                        COLOURS_OPTION_INDEX bg_colours_option_index = COI_BACK;

                        if(tcell)
                        {
                            if(tcell->justify & PROTECTED)
                            {
                                trace_0(TRACE_APP_PD4, "cell is protected");
                                bg_colours_option_index = COI_PROTECT;
                            }

                            if(result_sign(tcell) < 0)
                            {
                                trace_0(TRACE_APP_PD4, "cell is negative");
                                fg_colours_option_index = COI_NEGATIVE;
                            }
                        }

                        /* can buffer up inversions */
                        if((fg_colours_option_index == COI_FORE)  &&  (bg_colours_option_index == COI_BACK))
                        {
                            if(start_coff == -1)
                            {
                                trace_1(TRACE_APP_PD4, "starting buffering up at coff %d", coff);
                                start_coff = coff;
                            }

                            trace_1(TRACE_APP_PD4, "adding coff %d to end of buffer", coff);
                            end_coff = coff;
                        }
                        else
                        {
                            if(start_coff != -1)
                            {
                                trace_2(TRACE_APP_PD4, "flush inverted section because colour change %d - %d", start_coff, end_coff);
                                please_invert_number_cells(start_coff, end_coff, roff, COI_FORE, COI_BACK);
                                start_coff = -1;
                            }

                            please_invert_number_cell(coff, roff, fg_colours_option_index, bg_colours_option_index);
                        }
                    }
                }
                else if(start_coff != -1)
                {
                    trace_2(TRACE_APP_PD4, "flush inverted section because rectangle edge %d - %d", start_coff, end_coff);
                    please_invert_number_cells(start_coff, end_coff, roff, COI_FORE, COI_BACK);
                    start_coff = -1;
                }
            }   /* loop over cols */

            if(start_coff != -1)
            {
                trace_2(TRACE_APP_PD4, "flush inverted section because row end %d - %d", start_coff, end_coff);
                please_invert_number_cells(start_coff, end_coff, roff, COI_FORE, COI_BACK);
            }

        NO_MORE_THIS_ROW:
            ;
        }   /* loop over rows */

NO_MORE_AT_ALL:
    ;

    if(!needs_drawsome)
    {
        BOOL colh_mark_state_indicator_new;

        colh_mark_state_indicator_new = ((blkstart.col != NO_COL) && (blk_docno == current_docno()));

        if(colh_mark_state_indicator != colh_mark_state_indicator_new)
        {
            colh_draw_mark_state_indicator(colh_mark_state_indicator_new);
        }
    }

    return(needs_drawsome);
}

/******************************************************************************
*
*  alter the active edge of a marked block
*  updating the screen to reflect this
*
******************************************************************************/

extern void
alter_marked_block(
    COL nc,
    ROW nr)
{
    BOOL update = FALSE;

    trace_2(TRACE_DRAG, "alter_marked_block(%d, %d)", nc, nr);

    old_blkstart = blkstart;            /* current marked block */
    old_blkend   = blkend;

    if(blkanchor.col == blkstart.col)
    {
        trace_0(TRACE_DRAG, "marked area at (or right of) anchor: ");
        if(nc > blkend.col)
        {
            trace_0(TRACE_DRAG, "end mark moving even further right");
            blkend.col = nc;
            update = TRUE;
        }
        else if(nc < blkend.col)
        {
            if(nc > blkanchor.col)
            {
                trace_0(TRACE_DRAG, "end mark moving left a little");
                blkend.col = nc;
            }
            else
            {
                trace_0(TRACE_DRAG, "end mark moved to (or left over) anchor: flip");
                blkstart.col = nc;
                blkend.col   = blkanchor.col;
            }
            update = TRUE;
            }
        else
            trace_0(TRACE_DRAG, "no col change");
    }
    else
    {
        trace_0(TRACE_DRAG, "marked area left of anchor: ");
        if(nc < blkstart.col)
        {
            trace_0(TRACE_DRAG, "start mark moving even further left");
            blkstart.col = nc;
            update = TRUE;
        }
        else if(nc > blkstart.col)
        {
            if(nc < blkanchor.col)
            {
                trace_0(TRACE_DRAG, "start mark moving right a little");
                blkstart.col = nc;
            }
            else
            {
                trace_0(TRACE_DRAG, "start mark moved to (or right over) anchor: flip");
                blkstart.col = blkanchor.col;
                blkend.col   = nc;
            }
            update = TRUE;
        }
        else
            trace_0(TRACE_DRAG, "no col change");
    }

    if(blkanchor.row == blkstart.row)
    {
        trace_0(TRACE_DRAG, "current marked area at (or below) anchor: ");
        if(nr > blkend.row)
        {
            trace_0(TRACE_DRAG, "end mark moving even further down");
            blkend.row = nr;
            update = TRUE;
        }
        else if(nr < blkend.row)
        {
            if(nr > blkstart.row)
            {
                trace_0(TRACE_DRAG, "end mark moving up a little, still below anchor");
                blkend.row = nr;
            }
            else
            {
                trace_0(TRACE_DRAG, "end mark moved up to (or above) anchor: flip");
                blkstart.row = nr;
                blkend.row   = blkanchor.row;
            }
            update = TRUE;
        }
        else
            trace_0(TRACE_DRAG, "no row change");
    }
    else
    {
        trace_0(TRACE_DRAG, "current marked area above anchor: ");
        if(nr < blkstart.row)
        {
            trace_0(TRACE_DRAG, "start mark moving even further up");
            blkstart.row = nr;
            update = TRUE;
        }
        else if(nr > blkstart.row)
        {
            if(nr < blkend.row)
            {
                trace_0(TRACE_DRAG, "start mark moving down a little");
                blkstart.row = nr;
            }
            else
            {
                trace_0(TRACE_DRAG, "start mark moved down to (or below) anchor: flip");
                blkstart.row = blkanchor.row;
                blkend.row   = nr;
            }
            update = TRUE;
        }
        else
            trace_0(TRACE_DRAG, "no row change");
    }

    if(update)
        if(new_marked_rectangle())
        {
            /* update the screen if some cells need redrawing */
            xf_drawsome = TRUE;
            draw_screen();
        }
}

/******************************************************************************
*
* clear marked block
*  (updates screen)
*
******************************************************************************/

extern void
clear_markers(void)
{
    trace_0(TRACE_MARK, "clear_markers()");

    if(blkstart.col != NO_COL)
    {
        DOCNO old_docno;

        /* update right document's window */
        old_docno = change_document_using_docno(blk_docno);

        make_single_mark_into_block();  /* in case just one set */

        old_blkstart = blkstart;
        old_blkend   = blkend;

        /* no marks */
        blkstart.col = blkend.col = NO_COL;
        blkstart.row = blkend.row = (ROW) -1;

        if(new_marked_rectangle())
        {
            xf_drawsome = TRUE;
            draw_screen();
        }

        select_document_using_docno(old_docno);
    }
    else
        trace_0(TRACE_MARK, "no mark(s) set");
}

/******************************************************************************
*
* if just one mark is set, make it into a marked block
*
******************************************************************************/

extern void
make_single_mark_into_block(void)
{
    if((blkstart.col != NO_COL)  &&  (blkend.col == NO_COL))
    {
        trace_0(TRACE_MARK, "making single mark into marked block");
        blkend = blkstart;
    }
    else
        trace_0(TRACE_MARK, "no mark set/full marked block set");
}

/******************************************************************************
*
* set a single cell as marked
* adjusts screen to show change
*
******************************************************************************/

static void
set_single_mark(
    COL tcol,
    ROW trow)
{
    clear_markers();

    /* setting new mark, anchor here to this document */
    blk_docno  = current_docno();

    blkanchor.col = tcol;
    blkanchor.row = trow;

    /* set new block of one cell, end mark fudged */
    blkstart.col = tcol;
    blkstart.row = trow;
    blkend.col   = tcol;
    blkend.row   = trow;

    /* fudge non-existent old block position */
    old_blkstart.row = old_blkend.row = (ROW) -1;
    old_blkstart.col = old_blkend.col = (COL) -1;

    if(new_marked_rectangle())
    {
        /* update the screen if some cells need redrawing */
        xf_drawsome = TRUE;
        draw_screen();
    }

    /* set correct end mark */
    blkend.col = NO_COL;
}

/******************************************************************************
*
* adjust an existing/set a new marked block
* adjusts screen to show change
* still sorts mark pair but now ensures
* that the anchor is the PASSED scol,srow
* and not the SORTED scol, srow (got the
* extend from caret to click case)
*
******************************************************************************/

extern void
set_marked_block(
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow,
    BOOL is_new_block)
{
    COL i_scol = scol;
    ROW i_srow = srow;

    trace_5(TRACE_MARK, "set_marked_block(%d, %d, %d, %d, new=%s)",
                            scol, srow, ecol, erow, report_boolstring(is_new_block));

    /* always keep markers ordered */
    sort_colt(&scol, &ecol);
    sort_rowt(&srow, &erow);

    if(is_new_block)
    {
        clear_markers();

        /* fudge non-existent old block position */
        old_blkstart.row = old_blkend.row = (ROW) -1;
        old_blkstart.col = old_blkend.col = (COL) -1;

        /* setting new marked block, anchor here to this document */
        blk_docno  = current_docno();

        blkanchor.col = i_scol; /* SKS 27.9.91 was scol, srow */
        blkanchor.row = i_srow;
    }
    else
    {
        make_single_mark_into_block();

        /* note old block position */
        old_blkstart = blkstart;
        old_blkend   = blkend;
    }

    /* set new block */
    blkstart.col = scol;
    blkstart.row = srow;
    blkend.col   = ecol;
    blkend.row   = erow;

    if(new_marked_rectangle())
    {
        /* update the screen if some cells need redrawing */
        xf_drawsome = TRUE;
        draw_screen();
    }
}

/******************************************************************************
*
* set a marker:
*
* if two marks already set or if one
* mark is set in another document
* then clear marker(s) first.
*
******************************************************************************/

extern void
set_marker(
    COL tcol,
    ROW trow)
{
    COL ecol, scol;
    ROW erow, srow;
    BOOL new;

    trace_2(TRACE_MARK, "set_marker(%d, %d)", tcol, trow);

    /* setting first mark if no marks, one set elsewhere or two anywhere */
    new = (blkstart.col == NO_COL)  ||  /* none? */
          (blkend.col   != NO_COL)  ||  /* two? */
          (blk_docno != current_docno());  /* one else? */

    if(new)
        /* setting first mark, all fresh please */
        set_single_mark(tcol, trow);
    else
    {
        /* setting second mark, leave anchor as is */
        scol = blkstart.col;
        srow = blkstart.row;
        ecol = tcol;
        erow = trow;

        set_marked_block(scol, srow, ecol, erow, FALSE);
    }
}

/******************************************************************************
*
* is the cell marked?
* marked blocks lie between blkstart.col, blkend.col,
*                           blkstart.row, blkend.row
*
******************************************************************************/

extern BOOL
inblock(
    COL tcol,
    ROW trow)
{
    if(blk_docno != current_docno())
        return(FALSE);              /* marked block not in this document */

    if(blkend.col != NO_COL)        /* block of cells */
        return( (blkstart.col <= tcol)  &&  (blkstart.row <= trow)  &&
                (tcol <= blkend.col)    &&  (trow <= blkend.row)    );

    if(blkstart.col != NO_COL)      /* one cell */
        return((tcol == blkstart.col)  &&  (trow == blkstart.row));

    return(FALSE);                  /* no block */
}

/******************************************************************************
*
* set up block i.e. make in_block top left cell
* if no block set to current cell
*
******************************************************************************/

extern void
init_marked_block(void)
{
    init_block(&blkstart, &blkend);
}

extern void
init_doc_as_block(void)
{
    SLR bs, be;

    bs.col = (COL) 0;
    bs.row = (ROW) 0;

    be.col = numcol - 1;
    be.row = numrow - 1;

    init_block(&bs, &be);
}

extern void
init_block(
    PC_SLR bs,
    PC_SLR be)
{
    trace_4(TRACE_APP_PD4, "init_block((%8x, %8x), (%8x, %8x))", bs->col, bs->row, be->col, be->row);

    start_bl = *bs;
    end_bl   = *be;

    if( start_bl.col & BADCOLBIT)
        start_bl.col = NO_COL;

    /* if no mark(s) set in this document, use current cell */

    if(start_bl.col != NO_COL)
    {
        in_block = start_bl;

        if( end_bl.col & BADCOLBIT)
            end_bl.col = NO_COL;

        if( end_bl.col == NO_COL)
            end_bl.row = start_bl.row;  /* single cell (marked) */
    }
    else
    {
        /* single cell (current) */
        start_bl.row = currow;
        in_block.col = curcol;
        in_block.row = currow;
        end_bl       = start_bl;    /* NO_COL, currow */
    }

    trace_4(TRACE_APP_PD4, "init_block set (%d, %d), (%d, %d)", start_bl.col, start_bl.row, end_bl.col, end_bl.row);

    start_block = TRUE;
}

/******************************************************************************
*
* initialise a block for traversal
*
******************************************************************************/

extern void
traverse_block_init(
    TRAVERSE_BLOCKP blk /*out*/,
    DOCNO docno,
    PC_SLR bs,
    PC_SLR be,
    TRAVERSE_BLOCK_DIRECTION direction)
{
    trace_4(TRACE_APP_PD4, "traverse_block_init((&%8x, &%8x), (&%8x, &%8x))",
            bs->col, bs->row, be->col, be->row);

    blk->docno = docno;
    blk->stt       = *bs;
    blk->end       = *be;
    blk->direction = direction;

    blk->start = TRUE;
    blk->it    = NULL;

    blk->p_docu = p_docu_from_docno(blk->docno);
    assert(NO_DOCUMENT != blk->p_docu);
    assert(!docu_is_thunk(blk->p_docu));

    if( blk->stt.col & BADCOLBIT)
        blk->stt.col = NO_COL;

    /* if no mark(s) set in this document, use current cell */

    if(blk->stt.col != NO_COL)
    {
        blk->cur = blk->stt;

        if( blk->end.col & BADCOLBIT)
            blk->end.col = NO_COL;

        if( blk->end.col == NO_COL)
            blk->end.row = blk->stt.row;  /* single cell (marked) */
    }
    else
    {
        /* single cell (current) */
        blk->stt.row = blk->p_docu->Xcurrow;
        blk->cur.col = blk->p_docu->Xcurcol;
        blk->cur.row = blk->p_docu->Xcurrow;
        blk->end     = blk->stt;    /* NO_COL, currow */
    }

    trace_4(TRACE_APP_PD4, "traverse_block_init set (%d, %d), (%d, %d)",
            blk->stt.col, blk->stt.row, blk->end.col, blk->end.row);
}

/******************************************************************************
*
* force the next call to next_in_block
* to yield (in_block.col+1,start_bl.row) or end
*
******************************************************************************/

extern void
force_next_col(void)
{
    in_block.row = end_bl.row;
}

/******************************************************************************
*
* force the next call to next_in_block
* to yield (start_bl.col,in_block.row+1) or end
*
******************************************************************************/

extern void
force_next_row(void)
{
    in_block.col = end_bl.col;
}

/******************************************************************************
*
* SKS 18.10.91 invented so we can go
*
* while(next_in_block())
*    {
*    ...
*    if(blah)
*        return(more_in_block ? incomplete : finished);
*    }
*
* especially for printing
*
******************************************************************************/

extern BOOL
more_in_block(
    BOOL direction)
{
    COL col;
    ROW row;

    /* always can return first cell - perhaps current cell */
    if(start_block)
        return(TRUE);

    /* for zero or one markers, only cell returned above */
    /* for zero or one markers, only cell returned above */
    if(end_bl.col == NO_COL)
        return(FALSE);

    col = in_block.col;
    row = in_block.row;

    /* next column */
    if(direction == DOWN_COLUMNS)
    {
        if(++row > end_bl.row)
        {
            /* off bottom so would reset and step to next column */
            row = start_bl.row;
            IGNOREVAR(row);

            if(++col > end_bl.col)
                return(FALSE);                  /* off side too */
        }
    }
    else
    {
        if(++col > end_bl.col)
        {
            /* off rhs so would reset and step to next row */
            col = start_bl.col;
            IGNOREVAR(col);

            if(++row > end_bl.row)
                return(FALSE);                  /* off bottom too */
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* this must be called after init_block and before reading first cell
*
******************************************************************************/

extern BOOL
next_in_block(
    BOOL direction)
{
    /* always return first cell - perhaps current cell */
    if(start_block)
    {
        start_block = FALSE;
        return(TRUE);
    }

    /* for zero or one markers, only cell returned above */
    if(end_bl.col == NO_COL)
        return(FALSE);

    /* next column */
    if(direction == DOWN_COLUMNS)
    {
        if(++in_block.row > end_bl.row)
        {
            /* off bottom so reset and step to next column */
            in_block.row = start_bl.row;

            if(++in_block.col > end_bl.col)
                return(FALSE);                  /* off side too */
        }
    }
    else
    {
        if(++in_block.col > end_bl.col)
        {
            /* off rhs so reset and step to next row */
            in_block.col = start_bl.col;

            if(++in_block.row > end_bl.row)
                return(FALSE);                  /* off bottom too */
        }
    }

    return(TRUE);
}

extern BOOL
traverse_block_next(
    TRAVERSE_BLOCKP blk /*inout*/)
{
    assert(blk);

    /* always return first cell - perhaps current cell */
    if(blk->start)
    {
        blk->start = FALSE;
        return(TRUE);
    }

    /* for zero or one markers, only cell returned above */
    if(blk->end.col == NO_COL)
        return(FALSE);

    /* next column */
    if(blk->direction == TRAVERSE_DOWN_COLUMNS)
    {
        if(++blk->cur.row > blk->end.row)
        {
            /* off bottom so reset and step to next column */
            blk->cur.row = blk->stt.row;

            if(++blk->cur.col > blk->end.col)
                return(FALSE);                  /* off side too */
        }
    }
    else
    {
        if(++blk->cur.col > blk->end.col)
        {
            /* off rhs so reset and step to next row */
            blk->cur.col = blk->stt.col;

            if(++blk->cur.row > blk->end.row)
                return(FALSE);                  /* off bottom too */
        }
    }

    return(TRUE);
}

/******************************************************************************
*
*  calculate percentage through block
*
******************************************************************************/

extern S32
percent_in_block(
    BOOL direction)
{
    COL ncol = end_bl.col - start_bl.col + 1;
    ROW nrow = end_bl.row - start_bl.row + 1;
    COL tcol = in_block.col - start_bl.col;
    ROW trow = in_block.row - start_bl.row;

    trace_5(TRACE_APP_PD4, "percent_in_block: DOWN_COLUMNS %s ncol %d nrow %d tcol %d trow %d",
            report_boolstring(direction == DOWN_COLUMNS), ncol, nrow, tcol, trow);

    if(direction == DOWN_COLUMNS)
    {   /* a bit less than S32_MAX / 100 */
        if(trow <= 0x01400000)
            return( (100 * (S32) tcol +     ((100 * trow) / nrow)) / ncol );
        else
            return( (100 * (S32) tcol + muldiv64(100, trow, nrow)) / ncol );
    }
    else
    {
        if(tcol <= 0x01400000)
            return( (100 * (S32) trow +     ((100 * tcol) / ncol)) / nrow );
        else
            return( (100 * (S32) trow + muldiv64(100, tcol, ncol)) / nrow );
    }
}

/******************************************************************************
*
* mark block
* if 0 or 2 markers already set, set first marker to current cell
* if 1 marker set, set second. Afterwards first marker is top left
* and second marker is bottom right even if markers not specified at
* these cells or in this order.
*
******************************************************************************/

/******************************************************************************
*
* clear markers
*
******************************************************************************/

extern void
ClearMarkedBlock_fn(void)
{
    clear_markers();
}

extern void
MarkSheet_fn(void)
{
#if 1
    /* SKS believes this way round will be invaluable for database users! at 27.9.91
     * - set the anchor to the BOTTOM RIGHT, then you can 'extend' down safely past
     * your column headings!
    */
    set_marked_block(numcol-1, numrow-1, 0, 0, TRUE);
#else
    set_marked_block(0, 0, numcol-1, numrow-1, TRUE);
#endif
}

extern void
MarkSlot_fn(void)
{
    set_marker(curcol, currow);
}

/* where the mouse got clicked */
static gcoord mx;
static gcoord my;

/******************************************************************************
*
*  someone has clicked & dragged in our main window
*
******************************************************************************/

static BOOL drag_not_started_to_mark;
static coord dragstart_tx;
static coord dragstart_ty;
static SLR dragstart_first;
static SLR dragstart_second;

extern void
prepare_for_drag_mark(
    coord tx,
    coord ty,
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow)
{
    dragstart_tx            = tx;
    dragstart_ty            = ty;
    dragstart_first.col     = scol;
    dragstart_first.row     = srow;
    dragstart_second.col    = ecol;
    dragstart_second.row    = erow;
    drag_not_started_to_mark = TRUE;
}

static coord dragcol_tx;
static COL  dragcol_col;
static S32   dragcol_minwidth;
static S32   dragcol_maxwidth;
static BOOL  dragcol_do_dstwrp;

static BOOL  dragcol_linked;
/*>>>do these two routines need different limits for linked columns??? */
/*>>>so that the margins of linked cols don't get too big???*/

extern void
prepare_for_drag_column_width(
    coord tx,
    COL col)
{
    dragcol_tx     = tx;
    dragcol_col    = col;
    dragcol_linked = colislinked(col);

    if(all_widths_zero(col, col) ||     /* if all other columns are zero width */
       incolfixes(col)                  /*    or this column is fixed,         */
      )
        dragcol_minwidth = 1;
    else
        dragcol_minwidth = 0;

    dragcol_maxwidth  = 255;
    dragcol_do_dstwrp = FALSE;  /* not currently used - might be one day? */
}

extern void
prepare_for_drag_column_wrapwidth(
    coord tx,
    COL col,
    BOOL do_dstwrp)
{
    dragcol_tx        = tx;
    dragcol_col       = col;
    dragcol_linked    = colislinked(col);
    dragcol_minwidth  = 2;
    dragcol_maxwidth  = 255;
    dragcol_do_dstwrp = do_dstwrp;
}

null_event_proto(static, drag_null_query)
{
    UNREFERENCED_PARAMETER_CONST(p_null_event_block);

    trace_0(TRACE_APP_PD4, "drag call to ask if we want nulls");

    return( (drag_type == NO_DRAG_ACTIVE)
                        ? NULL_EVENTS_OFF
                        : NULL_EVENTS_REQUIRED );
}

null_event_proto(static, drag_null_event)
{
    P_DOCNO dochanp = (P_DOCNO) p_null_event_block->client_handle;

    trace_0(TRACE_APP_PD4, "drag null call");

    if(!select_document_using_docno(*dochanp))
        return(NULL_EVENT_COMPLETED);

    process_drag();

    return(NULL_EVENT_COMPLETED);
}

null_event_proto(static, drag_null_handler)
{
    trace_1(TRACE_APP_PD4, "drag_null_handler, rc=%d", p_null_event_block->rc);

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(drag_null_query(p_null_event_block));

    case NULL_EVENT:
        return(drag_null_event(p_null_event_block));

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

/******************************************************************************
*
*  start a drag off
*
******************************************************************************/

extern void
start_drag(
    S32 dragt)
{
    wimp_dragstr dragstr;

    drag_type = dragt;
    drag_docno = current_docno();

    dragstr.window      = (wimp_w) main_window_handle;
    dragstr.type        = wimp_USER_HIDDEN;
#if 0
    /* Window Manager ignores inner box on hidden drags */
    dragstr.box.x0      = mx;
    dragstr.box.y0      = my;
    dragstr.box.x1      = mx+30;
    dragstr.box.y1      = my+30;
#endif
    dragstr.parent.x0   = 0;
    dragstr.parent.y0   = 0;
    dragstr.parent.x1   = g_os_display_size.cx;
    dragstr.parent.y1   = g_os_display_size.cy;

    if(NULL != WrapOsErrorReporting(wimp_drag_box(&dragstr)))
        return;

#if defined(TRY_AUTOSCROLL) /* try using Wimp_AutoScroll if available and appropriate, falling back to the existing mechanism if not */
    switch(drag_type)
    {
    case DRAG_COLUMN_WIDTH:
    case DRAG_COLUMN_WRAPWIDTH:
        /* not for colh window - too hard! */
        break;

    default:
        { /* Window Manager copies this block on setup */
        WimpAutoScrollBlock wasb;
        wasb.window_handle = rear_window_handle;
        wasb.left_pause_zone_width = texttooffset_x(2);
        wasb.bottom_pause_zone_width = charvspace;
        wasb.right_pause_zone_width = wasb.left_pause_zone_width;
        wasb.top_pause_zone_width = wasb.bottom_pause_zone_width;
        wasb.pause_duration_cs = -1; /* use configured pause length */
        wasb.state_change_handler = 1; /* use Wimp‘s pointer shape routine */
        wasb.state_change_handler_handle = 0;

        if(displaying_borders) /* main window contains row borders - scroll when we hit them too */
        {
            wasb.left_pause_zone_width = texttooffset_x(borderwidth);
        }

        if(displaying_borders) /* main window displaced from rear window for colh window */
        {
            int borderline;
            WimpGetIconStateBlock icon_state;
            icon_state.window_handle = colh_window_handle;
            icon_state.icon_handle = COLH_COLUMN_HEADINGS;
            void_WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state));
            borderline = icon_state.icon.bbox.ymin; /* approx -132 */
            wasb.top_pause_zone_width += (-borderline);
        }

        {
        _kernel_swi_regs rs;
        rs.r[0] = 0x03 /* autoscroll both directions */ | (1 << 2) /* send Scroll_Request */ ;
        rs.r[1] = (int) &wasb;
        drag_autoscroll = (NULL == _kernel_swi(Wimp_AutoScroll, &rs, &rs)); /* don't winge */
        } /*block*/

        break;
        }
    }
#endif

    status_assert(Null_EventHandlerAdd(drag_null_handler, &drag_docno, 0));

    switch(drag_type)                                       /* be helpful, tell the user ... */
    {
    case DRAG_COLUMN_WIDTH:
        colh_pointershape_drag_notification(TRUE);
        colh_draw_drag_state(colwidth(dragcol_col));        /* ... what the width is */
        break;

    case DRAG_COLUMN_WRAPWIDTH:
        colh_pointershape_drag_notification(TRUE);
        colh_draw_drag_state(colwrapwidth(dragcol_col));    /* ... where the margin is */
        break;
    }
}

/* Normally called when default_event_handler (riscos.c) receives a Wimp_EUserDrag event */

extern void
ended_drag(void)
{
    if(drag_type == NO_DRAG_ACTIVE)
        return;

    switch(drag_type)
    {
    case DRAG_COLUMN_WIDTH:
    case DRAG_COLUMN_WRAPWIDTH:
        colh_pointershape_drag_notification(FALSE);
        colh_draw_drag_state(-1);                       /* put the silly bar back */
    /*  adjust_all_linked_columns();*/
        break;
    }

    drag_type = NO_DRAG_ACTIVE;

    if(drag_autoscroll)
    {
        _kernel_swi_regs rs;
        rs.r[0] = 0 /* turn autoscroll off */ ;
        rs.r[1] = NULL;
        void_WrapOsErrorReporting(_kernel_swi(Wimp_AutoScroll, &rs, &rs));

        drag_autoscroll = FALSE;
    }

    Null_EventHandlerRemove(drag_null_handler, &drag_docno);

    drag_docno = DOCNO_NONE;
}

/* column to constrain selection made by dragging to */
static COL dragcol = -1;

static void
application_startdrag(
    coord tx,
    coord ty,
    BOOL select_clicked)
{
    DOCNO docno = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL)  &&  (docno == blk_docno);
    BOOL shift_pressed = host_shift_pressed();
    BOOL ctrl_pressed  = host_ctrl_pressed();
    coord coff = calcoff(tx); /* not _click */
    coord roff = calroff_click(ty);
    COL tcol;
    ROW trow;

    BOOL huntleft = select_clicked && !shift_pressed && !ctrl_pressed;
    BOOL extend = shift_pressed || !select_clicked;

    trace_3(TRACE_APP_PD4, "it's a drag start: at roff %d, coff %d, select = %s: ",
                roff, coff, report_boolstring(select_clicked));

    dragcol = -1;                       /* no constraint on drag yet */
    drag_not_started_to_mark = FALSE;   /* drags mark immediately */

    if(vertvec_entry_valid(roff))
    {
        P_SCRROW rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
        {
            trace_0(TRACE_APP_PD4, "in soft page break - drag ignored");
        }
        else
        {
            trow = rptr->rowno;

            if(horzvec_entry_valid(coff)  ||  (coff == OFF_RIGHT))
            {
                P_DOCU p_docu = find_document_with_input_focus();

                if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
                {
                    trace_0(TRACE_APP_PD4, "dragging to insert reference to a range - first coordinate already entered");
                    start_drag(INSERTING_REFERENCE);
                }
                else
                {
                    /* mark normal block */
                    coff = get_column(tx, trow, 0, huntleft);
                    assert(horzvec_entry_valid(coff));
                    tcol = col_number(coff);

                    if(blkindoc && extend)
                    {
                        trace_0(TRACE_APP_PD4, "continue mark");
                        make_single_mark_into_block();
                    }
                    else
                    {
                        trace_2(TRACE_APP_PD4, "col #%d, row #%d - start mark", tcol, trow);
                        prepare_for_drag_mark(tx, ty, tcol, trow, tcol, trow);
                    }

                    start_drag(MARK_BLOCK);
                }
            }
            else if(IN_ROW_BORDER(coff))
            {
                if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)  /* everything suppressed whilst editing */
                    return;

                /* mark all columns over given rows */
                if(blkindoc && extend)
                {
                    trace_0(TRACE_APP_PD4, "in row border - continuing all columns mark");
                    make_single_mark_into_block();
                }
                else
                {
                    trace_0(TRACE_APP_PD4, "in row border - starting all columns mark");
#if 1 /* SKS */
                    prepare_for_drag_mark(tx, ty, numcol-1, trow, 0, trow);
#else
                    prepare_for_drag_mark(tx, ty, 0, trow, numcol-1, trow);
#endif
                }

                start_drag(MARK_ALL_COLUMNS);
            }
            else
                trace_0(TRACE_APP_PD4, "off left - ignored");
        }
    }
}

static void
application_singleclick_in_main(
    coord tx,
    coord ty,
    S32 xcelloffset,
    BOOL select_clicked)
{
    DOCNO docno = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL)  &&  (docno == blk_docno);
    BOOL shift_pressed = host_shift_pressed();
    BOOL ctrl_pressed  = host_ctrl_pressed();
    coord coff = calcoff(tx); /* not _click */
    coord roff = calroff_click(ty);
    COL tcol;
    ROW trow;

    BOOL huntleft = select_clicked && !shift_pressed && !ctrl_pressed;
    BOOL extend = shift_pressed || !select_clicked;       /* i.e. shift-anything or unshifted-adjust */

    BOOL acquire    = FALSE; /* don't want caret on block marking operations */
    BOOL motion     = FALSE;

    trace_0(TRACE_APP_PD4, "it's a click: ");

    if(vertvec_entry_valid(roff))
    {
        P_SCRROW rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
        {
            trace_0(TRACE_APP_PD4, "in soft page break - click ignored");
            acquire = TRUE;
        }
        else
        {
            trow = rptr->rowno;

            if(horzvec_entry_valid(coff)  ||  (coff == OFF_RIGHT))
            {
                /* i.e. a click on a sheet */
                P_DOCU p_docu = find_document_with_input_focus(); /* examines all sheets, editlines & edit boxes */

                if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
                {
                    /* a sheet has been found with the input focus and it is being edited */
                    /* so give that sheet a reference to the clicked-on-document          */

                    coff = calcoff_click(tx);
                    assert(horzvec_entry_valid(coff));
                    tcol = col_number(coff);

                    trace_0(TRACE_APP_PD4, "editing expression, use ADJUST paradigm: ");

                    /* a 'normal' cell reference would be 'A1' where both col (A) and row (1) are relocated if replicated    */
                    /* sometimes it is useful to prevent either col or row or both from changing when replicated - so called */
                    /* absolute references. We use ctrl_pressed to give an absolute col, shift_pressed for absolute row.       */

                    insert_reference_to(p_docu->docno, docno, tcol, trow, ctrl_pressed, shift_pressed, TRUE);

                    /* ensure no drawing done in clicked-on-document
                     * or it might get the caret. must be done after insert_reference_to
                     * or maybe_cur_docno returns wrong value.
                    */
                    select_document(p_docu);
                }
                else if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                {
                    /* can't find someone to give a reference to, but the clicked-on-document */
                    /* is being edited, so give its editor the focus & caret                  */

                    acquire = TRUE;
                    trace_0(TRACE_APP_PD4, "clicked in document being edited, couldn't find anyone to give a cell ref too");
                }
                else
                {
                    if(chkrpb(trow)  &&  chkfsb()  &&  chkpac(trow))
                    {
                        trace_0(TRACE_APP_PD4, "in hard page break - go to the start");
                        tcol = 0;
                        g_newoffset = 0;
                    }
                    else
                    {
                        coff = get_column(tx, trow, xcelloffset, huntleft);
                        assert(horzvec_entry_valid(coff));
                        tcol = col_number(coff);
                    }

                    trace_2(TRACE_APP_PD4, "not editing expression: in sheet at row #%d, col #%d", trow, tcol);

                    if(extend)
                    {
                        /* either alter current block or set new block:
                         * mergebuf has been done by caller to ensure cell marking correct
                        */
                        if(blkindoc)
                        {
                            trace_0(TRACE_APP_PD4, "extending marked block");
                            make_single_mark_into_block();
                            alter_marked_block(tcol, trow);
                        }
                        else
                        {
                            trace_0(TRACE_APP_PD4, "creating a block - anchor at caret");
                            set_marked_block(curcol, currow, tcol, trow, TRUE);
                        }
                    }
                    else
                    {
                        if((tcol != curcol)  ||  (trow != currow))
                        {
                            /* position caret in new cell; mergebuf has been done by caller */
                            slot_in_buffer = FALSE;
#if FALSE
                            /* RCM says: I saw something like this in PDSearch ! */
                            chknlr(curcol = tcol, currow = trow);
#else
                            chknlr(tcol, trow);
#endif
                            lescrl = 0;
                        }

                        lecpos = g_newoffset;

                        acquire = motion = TRUE;
#if FALSE
                        /* If the click is in a macro sheet number cell, fire up an editor. */
                        if(ev_doc_is_custom_sheet(doc))
                        {
                            P_CELL tcell = travel(tcol, trow);

                            if( (NULL != tcell)            &&
                                (tcell->type == SL_NUMBER) &&
                                (ev_is_formula(&tcell->content.number.guts)) )
                            {
                                expedit_edit_current_cell(lecpos -3, FALSE); /* -3 cos Mark prints '...' infront of line! */
                            }
                        }
#endif
                    }
                }
            }
            else if(IN_ROW_BORDER(coff))
            {
                /* if editing, suppress (curcol,currow) movement and selection dragging etc. */
                if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                {
                  /*acquire = !extend; redundant assignment as we're exiting */
                    return;
                }

                if(extend)
                {
                    /* either alter current block or set new block:
                     * mergebuf has been done by caller to ensure cell marking correct
                    */
                    if(blkindoc)
                    {
                        trace_0(TRACE_APP_PD4, "alter number of marked rows");
                        make_single_mark_into_block();
                        alter_marked_block(ACTIVE_COL, trow);
                    }
                    else
                    {
                        trace_0(TRACE_APP_PD4, "mark all columns from caret to given row - anchor at caret row");
                        set_marked_block(0, currow, numcol-1, trow, TRUE);
                    }
                }
                else
                {
                    trace_0(TRACE_APP_PD4, "not extending");

                    if(trow != currow)
                    {
                        /* position caret in new row (same col); mergebuf has been done by caller */
                        slot_in_buffer = FALSE;
                        chknlr(curcol, trow);
                        lescrl = lecpos = 0;
                        motion = TRUE;
                    }

                    acquire = TRUE;
                }
            }
            else
            {
                trace_0(TRACE_APP_PD4, "off left/right - ignored");
                acquire = TRUE;
            }
        }
    }

    if(acquire)
        xf_caretreposition = TRUE;

    if(xf_caretreposition  ||  motion)
    {
        draw_screen();
        draw_caret();
    }
}

static void
application_doubleclick_in_main(
    coord tx,
    coord ty,
    BOOL select_clicked)
{
    coord coff = calcoff(tx);   /* not _click */
    coord roff = calroff_click(ty);
 /* COL tcol;*/
    ROW trow;
    P_SCRROW rptr;

    BOOL shift_pressed = host_shift_pressed();
 /* BOOL ctrl_pressed  = host_ctrl_pressed();*/
 /* BOOL huntleft = select_clicked && !shift_pressed && !ctrl_pressed; */
    BOOL extend = shift_pressed || !select_clicked;

    trace_3(TRACE_APP_PD4, "it's a double-click: at roff %d, coff %d, select = %s: ",
                roff, coff, report_boolstring(select_clicked));

    if(extend)
        return;

    if(vertvec_entry_valid(roff))
    {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
        {
            trace_0(TRACE_APP_PD4, "in soft page break - double click ignored");
        }
        else
        {
            trow = rptr->rowno;

            if(IN_ROW_BORDER(coff))
            {
                if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)      /* everything suppressed whilst editing */
                    return;
#if 1 /* SKS */
                trace_0(TRACE_APP_PD4, "in row border - mark row - anchor at last col");
                set_marked_block(numcol-1, trow, 0, trow, TRUE);
#else
                trace_0(TRACE_APP_PD4, "in row border - mark row - anchor at first col");
                set_marked_block(0, trow, numcol-1, trow, TRUE);
#endif
            }
            else
                trace_0(TRACE_APP_PD4, "not in row border - ignored");
        }
    }
}

/******************************************************************************
*
*  a button event has occurred in main window
*
******************************************************************************/

extern void
application_main_Mouse_Click(
    gcoord x,
    gcoord y,
    int buttons)
{
    static BOOL initially_select_clicked = FALSE;

    /* text cell coordinates */
    coord tx = tcoord_x(x);
    coord ty = tcoord_y(y);
    S32 xcelloffset = tcoord_x_remainder(x);   /* x offset in cell (OS units) */
    BOOL select_clicked;

    trace_6(TRACE_APP_PD4, "application_main_Mouse_Click: g(%d, %d) t(%d, %d) xco %d bstate %X",
                x, y, tx, ty, xcelloffset, buttons);

    /* ensure we can find cell for positioning, overlap tests etc. must allow spellcheck as we may move */
    if(!mergebuf())
        return;
    filbuf();
    /* but guaranteed that we can simply slot_in_buffer = FALSE for movement */

    mx = x;
    my = y;

    /* Have to cope with Pink 'Plonker' Duck Man's ideas on double clicks (fixed in new RISC OS+):
     * He says that left then right (or vice versa) is a double click!
    */
    if(buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust))
    {
        select_clicked = ((buttons & Wimp_MouseButtonSelect) != 0);

        /* must be same button that started us off */
        if(initially_select_clicked != select_clicked)
            buttons = select_clicked ? Wimp_MouseButtonSingleSelect : Wimp_MouseButtonSingleAdjust; /* force a single click */
    }
    else if(buttons & (Wimp_MouseButtonDragSelect | Wimp_MouseButtonDragAdjust))
    {
        select_clicked = ((buttons & Wimp_MouseButtonDragSelect) != 0);

        /* must be same button that started us off dragging */
        if(initially_select_clicked != select_clicked)
            buttons = select_clicked ? Wimp_MouseButtonSingleSelect : Wimp_MouseButtonSingleAdjust; /* force a single click */
    }

    if(buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust))
    {
        select_clicked = ((buttons & Wimp_MouseButtonSelect) != 0);

        application_doubleclick_in_main(tx, ty, select_clicked);

        initially_select_clicked = FALSE;
    }
    else if(buttons & (Wimp_MouseButtonDragSelect | Wimp_MouseButtonDragAdjust))
    {
        select_clicked = ((buttons & Wimp_MouseButtonDragSelect) != 0);

        application_startdrag(tx, ty, select_clicked);

        initially_select_clicked = FALSE;
    }
    else if(buttons & (Wimp_MouseButtonSingleSelect | Wimp_MouseButtonSingleAdjust))
    {
        select_clicked = ((buttons & Wimp_MouseButtonSingleSelect) != 0);

        initially_select_clicked = select_clicked;

        application_singleclick_in_main(tx, ty, xcelloffset, select_clicked);
    }
}

/******************************************************************************
*
*  on null events, alter the marked block
*
******************************************************************************/

static BOOL
now_marking(
    coord tx,
    coord ty)
{
    if(!drag_not_started_to_mark)
        return(TRUE);

    (void) mergebuf_nocheck();
    filbuf();

    if((dragstart_tx != tx)  ||  (dragstart_ty != ty))
    {
        drag_not_started_to_mark = FALSE;
        set_marked_block(dragstart_first.col,   dragstart_first.row,
                         dragstart_second.col,  dragstart_second.row, TRUE);
        return(TRUE);
    }

    return(FALSE);
}

extern void
application_drag(
    gcoord x,
    gcoord y,
    BOOL ended)
{
    coord tx    = tcoord_x(x);
    coord ty    = tcoord_y(y);
    coord coff  = calcoff_click(tx);    /* map OFF_LEFT/RIGHT to lh/rh col */
    coord roff  = calroff_click(ty);    /* map off top/bot to top/bot row */
    COL tcol;
    ROW trow;
    P_SCRROW rptr;
    SLR here;
    P_DOCU p_docu;

    trace_1(TRACE_DRAG, "application_drag: type = %d", drag_type);

    assert(vertvec_entry_valid(roff));
    rptr = vertvec_entry(roff);

    switch(drag_type)
    {
    case INSERTING_REFERENCE:
        if(ended  &&  !(rptr->flags & PAGE))
        {
            p_docu = find_document_with_input_focus();

            if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
            {
                assert(horzvec_entry_valid(coff));
                here.col = col_number(coff);
                here.row = rptr->rowno;

                /* ensure range ordered */
                sort_colt(&insert_reference_slr.col, &here.col);
                sort_rowt(&insert_reference_slr.row, &here.row);

                /* select document that's being edited */
                select_document(p_docu);

                if(xf_inexpression_box || xf_inexpression_line)
                {
                    expedit_delete_bit_of_line(insert_reference_stt_offset,         /* start col  */
                                               insert_reference_end_offset,         /* end col    */
                                               insert_reference_row_number          /* row number */
                                              );
                }
                else
                {
                    lecpos = insert_reference_stt_offset;
                    delete_bit_of_line(lecpos, insert_reference_end_offset - lecpos, FALSE);
                }

                insert_reference_to(p_docu->docno,
                                    insert_reference_docno,
                                    insert_reference_slr.col, insert_reference_slr.row,
                                    insert_reference_abs_col, insert_reference_abs_row,
                                    FALSE);

                /* SKS did this while we still remembered */
                if(insert_reference_slr.col != here.col || insert_reference_slr.row != here.row)
                    insert_reference_to(p_docu->docno,
                                        p_docu->docno,
                                        here.col,                 here.row,
                                        insert_reference_abs_col, insert_reference_abs_row,
                                        TRUE);
            }
        }
        break;

    case MARK_ALL_COLUMNS:
        /* marking fixed set of columns in dynamic selection of rows */
        if(!(rptr->flags & PAGE)  &&  now_marking(tx, ty))
        {
            trow = rptr->rowno;
            alter_marked_block(ACTIVE_COL, trow);
        }
        break;

    case MARK_ALL_ROWS:
        /* marking fixed set of rows in dynamic selection of columns */
        if(now_marking(tx, ty))
        {
            if(dragcol >= 0)
            {
                tcol = dragcol;
            }
            else
            {
                assert(horzvec_entry_valid(coff));
                tcol = col_number(coff);
            }
            alter_marked_block(tcol, ACTIVE_ROW);
        }
        break;

    case MARK_BLOCK:
        /* marking arbitrary (or constrained) block */
        if(!(rptr->flags & PAGE)  &&  now_marking(tx, ty))
        {
            if(dragcol >= 0)
            {
                tcol = dragcol;
            }
            else
            {
                assert(horzvec_entry_valid(coff));
                tcol = col_number(coff);
            }
            trow = rptr->rowno;
            alter_marked_block(tcol, trow);
        }
        break;

    case DRAG_COLUMN_WIDTH:
        {
        S32 change;
        P_S32 widp, wwidp, colflagsp;

        /* changing a columns width */

        if(0 != (change = tx - dragcol_tx))
        {
            readpcolvars_(dragcol_col, &widp, &wwidp, &colflagsp);

            /* change is >0 for expand, <0 for contract */
            /* *widp is >=0                             */

            if((*widp + change) < dragcol_minwidth)
                change = dragcol_minwidth - *widp;          /* limit reduction to min */

            if((*widp + change) > dragcol_maxwidth)
                change = dragcol_maxwidth - *widp;          /* limit increase to max */

            if(change != 0)
            {
                *widp      += change;
                dragcol_tx += change;

                if(dragcol_linked)
                    adjust_this_linked_column(dragcol_col);

                colh_draw_drag_state(*widp);        /* be helpful, show user what the width is */

                riscos_setpointershape(*widp == 0 ? POINTER_DRAGCOLUMN_RIGHT : POINTER_DRAGCOLUMN);

#if TRUE
                if(dragcol_col != curcol)
                {
                    /* position caret in dragged column (same row); mergebuf has been done by caller */
                    slot_in_buffer = FALSE;
                    chknlr(dragcol_col, currow);
                    lescrl = lecpos = 0;
                    xf_caretreposition = TRUE;
                }
#else
                chknlr(dragcol_col, currow);
#endif

                xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
                filealtered(TRUE);
                draw_screen();
                draw_caret();
            }
        }
        }
        break;

    case DRAG_COLUMN_WRAPWIDTH:
        {
        S32    change;
        S32   *widp, *wwidp, *colflagsp;
        coord  wid;

        /* changing a columns wrap width */

        if(0 != (change = tx - dragcol_tx))
        {
            /* change is >0 for expand, <0 for contract */

            readpcolvars_(dragcol_col, &widp, &wwidp, &colflagsp);

            wid = colwrapwidth(dragcol_col);

            /* colwrapwidth() is equiv. to (*wwidp != 0 ? *wwidp : *widp) */
            /* so, if right margin is tracking the colwidth, dragging     */
            /* it breaks the link                                         */

            if((wid + change) < dragcol_minwidth)
                change = dragcol_minwidth - wid;

            if((wid + change) > dragcol_maxwidth)
                change = dragcol_maxwidth - wid;

            if(change != 0)
            {
                *wwidp      = wid + change;
                dragcol_tx += change;
                colh_draw_drag_state(*wwidp);       /* be helpful, show user where the margin is */

                if(dragcol_linked)
                    adjust_this_linked_column(dragcol_col);

                if(dragcol_do_dstwrp)
                    dstwrp(dragcol_col, colwrapwidth(dragcol_col));

                xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
                filealtered(TRUE);
                draw_screen();
            }
        }
        }
        break;
    }
}

/******************************************************************************
*
* drag processor, called on null events
*
******************************************************************************/

static void
process_autoscroll_before_drag(
    _InVal_      int mouse_x,
    _InVal_      int mouse_y)
{
    if(drag_autoscroll)
    {   /* use Wimp_AutoScroll */
#if 1
        WimpAutoScrollBlock wasb;
        _kernel_swi_regs rs;
        rs.r[0] = (1 << 7) /* read current state */ ;
        rs.r[1] = (int) &wasb; /* filled on return from SWI */
        void_WrapOsErrorReporting(_kernel_swi(Wimp_AutoScroll, &rs, &rs));
        reportf("WASSW now %X", rs.r[0]);
        reportf(
            "Autoscroll %s commenced; "
            "Pointer is %s%s%s%s%s%s%s;"
            "Some work area %s%s%s%s",
            (rs.r[0] & (1<<8)) ? "has" : "not",
            (rs.r[0] & (1<<9)) ? "outside the window‘s visible area," : "",
            (rs.r[0] & (1<<10)) ? "within one or two pause zones," : "",
            (rs.r[0] & (1<<11)) ? "within the centre zone," : "",
            (rs.r[0] & (1<<12)) ? "left of the centre zone," : "",
            (rs.r[0] & (1<<13)) ? "below the centre zone," : "",
            (rs.r[0] & (1<<14)) ? "right of the centre zone," : "",
            (rs.r[0] & (1<<15)) ? "above the centre zone" : "",
            (rs.r[0] & (1<<16)) ? "left of the visible area," : "",
            (rs.r[0] & (1<<17)) ? "below the visible area," : "",
            (rs.r[0] & (1<<18)) ? "right of the visible area," : "",
            (rs.r[0] & (1<<19)) ? "above the visible area" : ""
        );
#endif
        return;
    }

    { /* scroll text ourselves if outside this guy's main window */
    const BOOL shift_pressed = host_shift_pressed();
    WimpGetWindowStateBlock window_state;

    window_state.window_handle = main_window_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
        return;

    if(drag_type != MARK_ALL_COLUMNS)
    {   /* not dragging down row border */
        const GDI_COORD test_xmin =
            window_state.visible_area.xmin
            + (displaying_borders ? texttooffset_x(borderwidth) : 0)
            - (charwidth / 2); /* SKS */

        if(mouse_x < test_xmin)
            application_process_command(shift_pressed ? N_PageLeft  : N_ScrollLeft);
        else if(mouse_x >= window_state.visible_area.xmax)
            application_process_command(shift_pressed ? N_PageRight : N_ScrollRight);
    }

    if(drag_type != MARK_ALL_ROWS)
    {   /* not dragging across col border */
        if(mouse_y < window_state.visible_area.ymin)
            application_process_command(shift_pressed ? N_PageDown : N_ScrollDown);
        else if(mouse_y >= window_state.visible_area.ymax)
            application_process_command(shift_pressed ? N_PageUp   : N_ScrollUp);
    }
    } /*block*/
}

extern void
process_drag(void)
{
    int mouse_x, mouse_y;

    {
    wimp_mousestr m;
    (void) wimp_get_point_info(&m);
    mouse_x = m.x; /* abs GDI coordinates */
    mouse_y = m.y;
    trace_4(TRACE_APP_PD4, "mouse pointer at w %d i %d x %d y %d", m.w, m.i, m.x, m.y);
    } /*block*/

    trace_0(TRACE_APP_PD4, "continuing drag: button still held");

    switch(drag_type)
    {
    case DRAG_COLUMN_WIDTH:
    case DRAG_COLUMN_WRAPWIDTH:
        break;

    default:
        process_autoscroll_before_drag(mouse_x, mouse_y);
        break;
    }

    application_drag(mouse_x, mouse_y, FALSE);
}

/* end of markers.c */
