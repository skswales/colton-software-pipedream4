/* markers.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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
int       dragtype   = NO_DRAG_ACTIVE;
DOCNO drag_docno = DOCNO_NONE;

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
*   rectangle of marked slots has changed
*
* --in--
*   assumes mergebuf has been done much
*   further up for slot correctness
*
*
* --out--
*   flag indicates that some slots need
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
    P_SLOT tslot;
    BOOL in_new_rowr, in_old_rowr;
    BOOL in_new_rect, in_old_rect;
    SCRCOL *cptr;
    SCRROW *rptr;
    coord c_width, overlap;

    trace_4(TRACE_MARK, "new_marked_rectangle(%d, %d, %d, %d): ",
            blkstart.col, blkstart.row, blkend.col, blkend.row);
    trace_4(TRACE_MARK, "old rectangle was (%d, %d, %d, %d)\n",
            old_blkstart.col, old_blkstart.row,
            old_blkend.col,   old_blkend.row);

    /* loop over all visible slots and 'update' slots in
     * visible range of changing rows and columns
    */
    for(roff = 0; !((rptr = vertvec_entry(roff))->flags & LAST); roff++)
        if(!(rptr->flags & PAGE))
            {
            start_coff = end_coff = -1;

            trace_2(TRACE_MARK, "row offset %d, PICT | UNDERPICT = %d\n",
                        roff, rptr->flags & (PICT | UNDERPICT));

            trow = rptr->rowno;

            tslot = travel(col_number(0), trow);
            if(tslot  &&  (tslot->type == SL_PAGE))
                continue;

            in_new_rowr =   (blkstart.row <= trow)      &&
                            (trow <= blkend.row);

            in_old_rowr =   (old_blkstart.row <= trow)  &&
                            (trow <= old_blkend.row);

            for(coff = 0; !((cptr = horzvec_entry(coff))->flags & LAST); coff++)
                {
                trace_1(TRACE_MARK, "column offset %d\n", coff);

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
                        (   ((tslot = travel(tcol, trow)) == NULL)
                                ?   is_overlapped(coff, roff)                           /* empty slot overlapped from left? */
                                :   (   /*riscos_fonts  ||*/                            /* slot with fonts needs drawing */
                                        (   !slot_displays_contents(tslot)              /* text slots might need drawing: */
                                            ? FALSE
                                            :   (   ((c_width = colwidth(tcol)) < (overlap = chkolp(tslot, tcol, trow)))  ||
                                                    /* if slot overlaps to right */
                                                    (FALSE  &&  grid_on  &&  (c_width > overlap))
                                                    /* or if grid & shorter than colwidth */
                                                )
                                        )
                                    )
                        )
                      )
                        {
                        trace_5(TRACE_APP_PD4, "disasterville: draw this whole row and breakout because mustdraw %s, PICT %s, tslot " PTR_XTFMT " -> empty overlapped %s, text | fonts %s\n",
                                trace_boolstring(mustdraw),
                                trace_boolstring(rptr->flags & (PICT | UNDERPICT)),
                                report_ptr_cast(tslot),
                                trace_boolstring(tslot ? FALSE : is_overlapped(coff, roff)),
                                trace_boolstring(tslot ? ((tslot->type == SL_TEXT) || riscos_fonts) : FALSE));
                        mark_row(roff);
                        draw_screen();

                        if(xf_interrupted)
                            goto NO_MORE_AT_ALL;

                        goto NO_MORE_THIS_ROW;
                        }
                    else
                        {
                        S32 fg = FORE;
                        S32 bg = BACK;

                        if(tslot)
                            {
                            if(tslot->justify & PROTECTED)
                                {
                                trace_0(TRACE_APP_PD4, "slot is protected\n");
                                bg = PROTECTC;
                                }

                            if(result_sign(tslot) < 0)
                                {
                                trace_0(TRACE_APP_PD4, "slot is negative\n");
                                fg = NEGATIVEC;
                                }
                            }

                        /* can buffer up inversions */
                        if((fg == FORE)  &&  (bg == BACK))
                            {
                            if(start_coff == -1)
                                {
                                trace_1(TRACE_APP_PD4, "starting buffering up at coff %d\n", coff);
                                start_coff = coff;
                                }

                            trace_1(TRACE_APP_PD4, "adding coff %d to end of buffer\n", coff);
                            end_coff = coff;
                            }
                        else
                            {
                            if(start_coff != -1)
                                {
                                trace_2(TRACE_APP_PD4, "flush inverted section because colour change %d - %d\n", start_coff, end_coff);
                                please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
                                start_coff = -1;
                                }

                            please_invert_numeric_slot(coff, roff, fg, bg);
                            }
                        }
                    }
                else if(start_coff != -1)
                    {
                    trace_2(TRACE_APP_PD4, "flush inverted section because rectangle edge %d - %d\n", start_coff, end_coff);
                    please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
                    start_coff = -1;
                    }
                }   /* loop over cols */

            if(start_coff != -1)
                {
                trace_2(TRACE_APP_PD4, "flush inverted section because row end %d - %d\n", start_coff, end_coff);
                please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
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

    trace_2(TRACE_DRAG, "alter_marked_block(%d, %d)\n", nc, nr);

    old_blkstart = blkstart;            /* current marked block */
    old_blkend   = blkend;

    if(blkanchor.col == blkstart.col)
        {
        trace_0(TRACE_DRAG, "marked area at (or right of) anchor: ");
        if(nc > blkend.col)
            {
            trace_0(TRACE_DRAG, "end mark moving even further right\n");
            blkend.col = nc;
            update = TRUE;
            }
        else if(nc < blkend.col)
            {
            if(nc > blkanchor.col)
                {
                trace_0(TRACE_DRAG, "end mark moving left a little\n");
                blkend.col = nc;
                }
            else
                {
                trace_0(TRACE_DRAG, "end mark moved to (or left over) anchor: flip\n");
                blkstart.col = nc;
                blkend.col   = blkanchor.col;
                }
            update = TRUE;
            }
        else
            trace_0(TRACE_DRAG, "no col change\n");
        }
    else
        {
        trace_0(TRACE_DRAG, "marked area left of anchor: ");
        if(nc < blkstart.col)
            {
            trace_0(TRACE_DRAG, "start mark moving even further left\n");
            blkstart.col = nc;
            update = TRUE;
            }
        else if(nc > blkstart.col)
            {
            if(nc < blkanchor.col)
                {
                trace_0(TRACE_DRAG, "start mark moving right a little\n");
                blkstart.col = nc;
                }
            else
                {
                trace_0(TRACE_DRAG, "start mark moved to (or right over) anchor: flip\n");
                blkstart.col = blkanchor.col;
                blkend.col   = nc;
                }
            update = TRUE;
            }
        else
            trace_0(TRACE_DRAG, "no col change\n");
        }

    if(blkanchor.row == blkstart.row)
        {
        trace_0(TRACE_DRAG, "current marked area at (or below) anchor: ");
        if(nr > blkend.row)
            {
            trace_0(TRACE_DRAG, "end mark moving even further down\n");
            blkend.row = nr;
            update = TRUE;
            }
        else if(nr < blkend.row)
            {
            if(nr > blkstart.row)
                {
                trace_0(TRACE_DRAG, "end mark moving up a little, still below anchor\n");
                blkend.row = nr;
                }
            else
                {
                trace_0(TRACE_DRAG, "end mark moved up to (or above) anchor: flip\n");
                blkstart.row = nr;
                blkend.row   = blkanchor.row;
                }
            update = TRUE;
            }
        else
            trace_0(TRACE_DRAG, "no row change\n");
        }
    else
        {
        trace_0(TRACE_DRAG, "current marked area above anchor: ");
        if(nr < blkstart.row)
            {
            trace_0(TRACE_DRAG, "start mark moving even further up\n");
            blkstart.row = nr;
            update = TRUE;
            }
        else if(nr > blkstart.row)
            {
            if(nr < blkend.row)
                {
                trace_0(TRACE_DRAG, "start mark moving down a little\n");
                blkstart.row = nr;
                }
            else
                {
                trace_0(TRACE_DRAG, "start mark moved down to (or below) anchor: flip\n");
                blkstart.row = blkanchor.row;
                blkend.row   = nr;
                }
            update = TRUE;
            }
        else
            trace_0(TRACE_DRAG, "no row change\n");
        }

    if(update)
        if(new_marked_rectangle())
            {
            /* update the screen if some slots need redrawing */
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
    trace_0(TRACE_MARK, "clear_markers()\n");

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
        trace_0(TRACE_MARK, "no mark(s) set\n");
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
        trace_0(TRACE_MARK, "making single mark into marked block\n");
        blkend = blkstart;
        }
    else
        trace_0(TRACE_MARK, "no mark set/full marked block set\n");
}

/******************************************************************************
*
* set a single slot as marked
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

    /* set new block of one slot, end mark fudged */
    blkstart.col = tcol;
    blkstart.row = trow;
    blkend.col   = tcol;
    blkend.row   = trow;

    /* fudge non-existent old block position */
    old_blkstart.row = old_blkend.row = (ROW) -1;
    old_blkstart.col = old_blkend.col = (COL) -1;

    if(new_marked_rectangle())
        {
        /* update the screen if some slots need redrawing */
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

    trace_5(TRACE_MARK, "set_marked_block(%d, %d, %d, %d, new=%s)\n",
                            scol, srow, ecol, erow, trace_boolstring(is_new_block));

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
        /* update the screen if some slots need redrawing */
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

    trace_2(TRACE_MARK, "set_marker(%d, %d)\n", tcol, trow);

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
* is the slot marked?
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

    if(blkend.col != NO_COL)        /* block of slots */
        return( (blkstart.col <= tcol)  &&  (blkstart.row <= trow)  &&
                (tcol <= blkend.col)    &&  (trow <= blkend.row)    );

    if(blkstart.col != NO_COL)      /* one slot */
        return((tcol == blkstart.col)  &&  (trow == blkstart.row));

    return(FALSE);                  /* no block */
}

/******************************************************************************
*
* set up block ie. make in_block top left slot
* if no block set to current slot
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
    trace_4(TRACE_APP_PD4, "init_block((%8x, %8x), (%8x, %8x))\n", bs->col, bs->row, be->col, be->row);

    start_bl = *bs;
    end_bl   = *be;

    if( start_bl.col & BADCOLBIT)
        start_bl.col = NO_COL;

    /* if no mark(s) set in this document, use current slot */

    if(start_bl.col != NO_COL)
        {
        in_block = start_bl;

        if( end_bl.col & BADCOLBIT)
            end_bl.col = NO_COL;

        if( end_bl.col == NO_COL)
            end_bl.row = start_bl.row;  /* single slot (marked) */
        }
    else
        {
        /* single slot (current) */
        start_bl.row = currow;
        in_block.col = curcol;
        in_block.row = currow;
        end_bl       = start_bl;    /* NO_COL, currow */
        }

    trace_4(TRACE_APP_PD4, "init_block set (%d, %d), (%d, %d)\n", start_bl.col, start_bl.row, end_bl.col, end_bl.row);

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
    trace_4(TRACE_APP_PD4, "traverse_block_init((&%8x, &%8x), (&%8x, &%8x))\n",
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

    /* if no mark(s) set in this document, use current slot */

    if(blk->stt.col != NO_COL)
        {
        blk->cur = blk->stt;

        if( blk->end.col & BADCOLBIT)
            blk->end.col = NO_COL;

        if( blk->end.col == NO_COL)
            blk->end.row = blk->stt.row;  /* single slot (marked) */
        }
    else
        {
        /* single slot (current) */
        blk->stt.row = blk->p_docu->Xcurrow;
        blk->cur.col = blk->p_docu->Xcurcol;
        blk->cur.row = blk->p_docu->Xcurrow;
        blk->end     = blk->stt;    /* NO_COL, currow */
        }

    trace_4(TRACE_APP_PD4, "traverse_block_init set (%d, %d), (%d, %d)\n",
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

    /* always can return first slot - perhaps current slot */
    if(start_block)
        return(TRUE);

    /* for zero or one markers, only slot returned above */
    /* for zero or one markers, only slot returned above */
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

            if(++row > end_bl.row)
                return(FALSE);                  /* off bottom too */
            }
        }

    return(TRUE);
}

/******************************************************************************
*
* this must be called after init_block and before reading first slot
*
******************************************************************************/

extern BOOL
next_in_block(
    BOOL direction)
{
    /* always return first slot - perhaps current slot */
    if(start_block)
        {
        start_block = FALSE;
        return(TRUE);
        }

    /* for zero or one markers, only slot returned above */
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

    /* always return first slot - perhaps current slot */
    if(blk->start)
        {
        blk->start = FALSE;
        return(TRUE);
        }

    /* for zero or one markers, only slot returned above */
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

    trace_5(TRACE_APP_PD4, "percent_in_block: DOWN_COLUMNS %s ncol %d nrow %d tcol %d trow %d\n",
            trace_boolstring(direction == DOWN_COLUMNS), ncol, nrow, tcol, trow);

    return( (S32) (
            (direction == DOWN_COLUMNS)
                    ? (100 * (S32) tcol + muldiv64(100, trow, nrow)) / ncol
                    : (100 * (S32) trow + muldiv64(100, tcol, ncol)) / nrow
            ));
}

/******************************************************************************
*
* mark block
* if 0 or 2 markers already set, set first marker to current slot
* if 1 marker set, set second. Afterwards first marker is top left
* and second marker is bottom right even if markers not specified at
* these slots or in this order.
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

null_event_proto(static, drag_null_handler)
{
    switch(p_null_event_block->rc)
        {
        case NULL_QUERY:
            return((dragtype == NO_DRAG_ACTIVE)
                              ? NULL_EVENTS_OFF
                              : NULL_EVENTS_REQUIRED);

        case NULL_EVENT:
            {
            P_DOCNO dochanp = (P_DOCNO) p_null_event_block->client_handle;
            assert(NULL != dochanp);
            if(select_document_using_docno(*dochanp))
                process_drag();
            }
            return(NULL_EVENT_COMPLETED);

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

    dragtype = dragt;
    drag_docno = current_docno();

    dragstr.window      = main__window;
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
    dragstr.parent.x1   = screen_x_os;
    dragstr.parent.y1   = screen_y_os;

    (void) wimp_drag_box(&dragstr);

    status_assert(Null_EventHandler(drag_null_handler, &drag_docno, TRUE, 0));

    switch(dragtype)                                    /* be helpful, tell the user... */
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

/* Normally called when default_event_handler (c.riscos) receives a wimp_EUSERDRAG event */

extern void
ended_drag(void)
{
    if(dragtype != NO_DRAG_ACTIVE)
        {
        switch(dragtype)
            {
            case DRAG_COLUMN_WIDTH:
            case DRAG_COLUMN_WRAPWIDTH:
                colh_pointershape_drag_notification(FALSE);
                colh_draw_drag_state(-1);                       /* put the silly bar back */
            /*  adjust_all_linked_columns();*/
                break;
            }

        dragtype = NO_DRAG_ACTIVE;

        status_assert(Null_EventHandler(drag_null_handler, &drag_docno, FALSE, 0));
        }
}

/* column to constrain selection made by dragging to */
static COL dragcol = -1;

static void
application_startdrag(
    coord tx,
    coord ty,
    BOOL selectclicked)
{
    DOCNO docno = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL)  &&  (docno == blk_docno);
    BOOL shiftpressed = host_shift_pressed();
    BOOL ctrlpressed  = host_ctrl_pressed();
    coord coff = calcoff(tx); /* not _click */
    coord roff = calroff(ty); /* not _click */
    COL  tcol;
    ROW  trow;
    SCRROW *rptr;

    BOOL huntleft = selectclicked && !shiftpressed && !ctrlpressed;
    BOOL extend = shiftpressed || !selectclicked;

    /* stop us wandering off bottom of sheet */
    roff = MIN(roff, rowsonscreen - 1);

    trace_3(TRACE_APP_PD4, "it's a drag start: at roff %d, coff %d, select = %s: ",
                roff, coff, trace_boolstring(selectclicked));

    dragcol = -1;                       /* no constraint on drag yet */
    drag_not_started_to_mark = FALSE;   /* drags mark immediately */

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            trace_0(TRACE_APP_PD4, "in soft page break - drag ignored\n");
        else
            {
            trow = rptr->rowno;

            if((coff >= 0)  ||  (coff == OFF_RIGHT))
                {
                P_DOCU p_docu = find_document_with_input_focus();

                if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
                    {
                    trace_0(TRACE_APP_PD4, "dragging to insert reference to a range - first coordinate already entered\n");
                    start_drag(INSERTING_REFERENCE);
                    }
                else
                    {
                    /* mark normal block */
                    coff = get_column(tx, trow, 0, huntleft);
                    tcol = col_number(coff);

                    if(blkindoc && extend)
                        {
                        trace_0(TRACE_APP_PD4, "continue mark\n");
                        make_single_mark_into_block();
                        }
                    else
                        {
                        trace_2(TRACE_APP_PD4, "col #%d, row #%d - start mark\n", tcol, trow);
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
                    trace_0(TRACE_APP_PD4, "in row border - continuing all columns mark\n");
                    make_single_mark_into_block();
                    }
                else
                    {
                    trace_0(TRACE_APP_PD4, "in row border - starting all columns mark\n");
#if 1 /* SKS */
                    prepare_for_drag_mark(tx, ty, numcol-1, trow, 0, trow);
#else
                    prepare_for_drag_mark(tx, ty, 0, trow, numcol-1, trow);
#endif
                    }

                start_drag(MARK_ALL_COLUMNS);
                }
            else
                trace_0(TRACE_APP_PD4, "off left - ignored\n");
            }
        }
    else
        trace_0(TRACE_APP_PD4, "above column headings - ignored\n");
}

static void
application_singleclick_in_main(
    coord tx,
    coord ty,
    S32 xcelloffset,
    BOOL selectclicked)
{
    DOCNO docno = current_docno();
    BOOL blkindoc = (blkstart.col != NO_COL)  &&  (docno == blk_docno);
    BOOL shiftpressed = host_shift_pressed();
    BOOL ctrlpressed  = host_ctrl_pressed();
    coord coff = calcoff(tx); /* not _click */
    coord roff = calroff(ty); /* not _click */
    COL  tcol;
    ROW  trow;
    SCRROW *rptr;

    BOOL huntleft = selectclicked && !shiftpressed && !ctrlpressed;
    BOOL extend = shiftpressed || !selectclicked;       /* ie shift-anything or unshifted-adjust */

    BOOL acquire    = FALSE; /* don't want caret on block marking operations */
    BOOL motion     = FALSE;

    /* stop us wandering off bottom of sheet */
    roff = MIN(roff, rowsonscreen - 1);

    trace_0(TRACE_APP_PD4, "it's a click: ");

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            {
            trace_0(TRACE_APP_PD4, "in soft page break - click ignored\n");
            acquire = TRUE;
            }
        else
            {
            trow = rptr->rowno;

            if((coff >= 0)  ||  (coff == OFF_RIGHT))
                {
                /* ie a click on a sheet */
                P_DOCU p_docu = find_document_with_input_focus(); /* examines all sheets, editlines & edit boxes */

                if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
                    {
                    /* a sheet has been found with the input focus and it is being edited */
                    /* so give that sheet a reference to the clicked-on-document          */

                    coff = calcoff_click(tx);
                    tcol = col_number(coff);

                    trace_0(TRACE_APP_PD4, "editing expression, use ADJUST paradigm: ");

                    /* a 'normal' slot reference would be 'A1' where both col (A) and row (1) are relocated if replicated    */
                    /* sometimes it is useful to prevent either col or row or both from changing when replicated - so called */
                    /* absolute references. We use ctrlpressed to give an absolute col, shiftpressed for absolute row.       */

                    insert_reference_to(p_docu->docno, docno, tcol, trow, ctrlpressed, shiftpressed, TRUE);

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
                    trace_0(TRACE_APP_PD4, "clicked in document being edited, couldn't find anyone to give a slot ref too\n");
                    }
                else
                    {
                    if(chkrpb(trow)  &&  chkfsb()  &&  chkpac(trow))
                        {
                        trace_0(TRACE_APP_PD4, "in hard page break - go to the start\n");
                        tcol = 0;
                        g_newoffset = 0;
                        }
                    else
                        {
                        coff = get_column(tx, trow, xcelloffset, huntleft);
                        tcol = col_number(coff);
                        }

                    trace_2(TRACE_APP_PD4, "not editing expression: in sheet at row #%d, col #%d\n", trow, tcol);

                    if(extend)
                        {
                        /* either alter current block or set new block:
                         * mergebuf has been done by caller to ensure slot marking correct
                        */
                        if(blkindoc)
                            {
                            trace_0(TRACE_APP_PD4, "extending marked block\n");
                            make_single_mark_into_block();
                            alter_marked_block(tcol, trow);
                            }
                        else
                            {
                            trace_0(TRACE_APP_PD4, "creating a block - anchor at caret\n");
                            set_marked_block(curcol, currow, tcol, trow, TRUE);
                            }
                        }
                    else
                        {
                        if((tcol != curcol)  ||  (trow != currow))
                            {
                            /* position caret in new slot; mergebuf has been done by caller */
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
                        /* If the click is in a macro sheet numeric slot, fire up an editor. */
                        if(ev_doc_is_custom_sheet(doc))
                            {
                            P_SLOT tslot = travel(tcol, trow);

                            if((tslot)                    &&
                               (tslot->type == SL_NUMBER) &&
                               (ev_is_formula(&tslot->content.number.guts))
                              )
                                {
                                expedit_editcurrentslot(lecpos -3, FALSE); /* -3 cos mark prints '...' infront of line! */
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
                    acquire = !extend;
                    return;
                    }

                if(extend)
                    {
                    /* either alter current block or set new block:
                     * mergebuf has been done by caller to ensure slot marking correct
                    */
                    if(blkindoc)
                        {
                        trace_0(TRACE_APP_PD4, "alter number of marked rows\n");
                        make_single_mark_into_block();
                        alter_marked_block(ACTIVE_COL, trow);
                        }
                    else
                        {
                        trace_0(TRACE_APP_PD4, "mark all columns from caret to given row - anchor at caret row\n");
                        set_marked_block(0, currow, numcol-1, trow, TRUE);
                        }
                    }
                else
                    {
                    trace_0(TRACE_APP_PD4, "not extending\n");

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
                trace_0(TRACE_APP_PD4, "off left/right - ignored\n");
                acquire = TRUE;
                }
            }
        }
    else
        {
        trace_0(TRACE_APP_PD4, "above sheet data - mostly ignored\n");
        acquire = TRUE;
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
    BOOL selectclicked)
{
    coord coff = calcoff(tx);   /* not _click */
    coord roff = calroff(ty);   /* not _click */
 /* COL  tcol;*/
    ROW  trow;
    SCRROW *rptr;

    BOOL shiftpressed = host_shift_pressed();
 /* BOOL ctrlpressed  = host_ctrl_pressed();*/
 /* BOOL huntleft = selectclicked && !shiftpressed && !ctrlpressed; */
    BOOL extend = shiftpressed || !selectclicked;

    /* stop us wandering off bottom of sheet */
    roff = MIN(roff, rowsonscreen - 1);

    trace_3(TRACE_APP_PD4, "it's a double-click: at roff %d, coff %d, select = %s: ",
                roff, coff, trace_boolstring(selectclicked));

    if(extend)
        return;

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            trace_0(TRACE_APP_PD4, "in soft page break - double click ignored\n");
        else
            {
            trow = rptr->rowno;

            if(IN_ROW_BORDER(coff))
                {
                if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)      /* everything suppressed whilst editing */
                    return;
#if 1 /* SKS */
                trace_0(TRACE_APP_PD4, "in row border - mark row - anchor at last col\n");
                set_marked_block(numcol-1, trow, 0, trow, TRUE);
#else
                trace_0(TRACE_APP_PD4, "in row border - mark row - anchor at first col\n");
                set_marked_block(0, trow, numcol-1, trow, TRUE);
#endif
                }
            else
                trace_0(TRACE_APP_PD4, "not in row border - ignored\n");
            }
        }
    else
        trace_0(TRACE_APP_PD4, "above column headings - ignored\n");
}

/******************************************************************************
*
*  a button event has occurred in main_window
*
******************************************************************************/

extern void
application_button_click_in_main(
    gcoord x,
    gcoord y,
    S32 buttonstate)
{
    static BOOL initially_selectclicked = FALSE;

    /* text cell coordinates */
    coord tx = tcoord_x(x);
    coord ty = tcoord_y(y);
    S32 xcelloffset = tcoord_x_remainder(x);   /* x offset in cell (OS units) */
    BOOL selectclicked;

    trace_6(TRACE_APP_PD4, "application_button_click_in_main: g(%d, %d) t(%d, %d) xco %d bstate %X\n",
                x, y, tx, ty, xcelloffset, buttonstate);

    /* ensure we can find slot for positioning, overlap tests etc. must allow spellcheck as we may move */
    if(!mergebuf())
        return;
    filbuf();
    /* but guaranteed that we can simply slot_in_buffer = FALSE for movement */

    mx = x;
    my = y;

    /* have to cope with Pink 'Plonker' Duck Man's ideas on double clicks (fixed in new RISC OS+):
     * He says that left then right (or vice versa) is a double click!
    */
    if(buttonstate & (wimp_BLEFT | wimp_BRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BLEFT) != 0);

        /* must be same button that started us off */
        if(initially_selectclicked != selectclicked)
            buttonstate = selectclicked ? wimp_BCLICKLEFT : wimp_BCLICKRIGHT; /* force a single click */
        }
    else if(buttonstate & (wimp_BDRAGLEFT  | wimp_BDRAGRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BDRAGLEFT) != 0);

        /* must be same button that started us off dragging */
        if(initially_selectclicked != selectclicked)
            buttonstate = selectclicked ? wimp_BCLICKLEFT : wimp_BCLICKRIGHT; /* force a single click */
        }

    if(buttonstate & (wimp_BLEFT      | wimp_BRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BLEFT) != 0);

        application_doubleclick_in_main(tx, ty, selectclicked);

        initially_selectclicked = 0;
        }
    else if(buttonstate & (wimp_BDRAGLEFT  | wimp_BDRAGRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BDRAGLEFT) != 0);

        application_startdrag(tx, ty, selectclicked);

        initially_selectclicked = 0;
        }
    else if(buttonstate & (wimp_BCLICKLEFT | wimp_BCLICKRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BCLICKLEFT) != 0);

        initially_selectclicked = selectclicked;
        application_singleclick_in_main(tx, ty, xcelloffset, selectclicked);
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
    SCRROW * rptr;
    SLR here;
    P_DOCU p_docu;

    /* stop us wandering off bottom (or top) of sheet */
    if(roff > rowsonscreen - 1)
        roff = rowsonscreen - 1;
    else if(roff < 0)
        roff = 0;

    trace_1(TRACE_DRAG, "application_drag: type = %d\n", dragtype);

    rptr = vertvec_entry(roff);

    switch(dragtype)
        {
        case INSERTING_REFERENCE:
            if(ended  &&  !(rptr->flags & PAGE))
                {
                p_docu = find_document_with_input_focus();

                if((NO_DOCUMENT != p_docu)  &&  (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line))
                    {
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
                tcol = (dragcol < 0) ? col_number(coff) : dragcol;
                alter_marked_block(tcol, ACTIVE_ROW);
                }
            break;

        case MARK_BLOCK:
            /* marking arbitrary (or constrained) block */
            if(!(rptr->flags & PAGE)  &&  now_marking(tx, ty))
                {
                tcol = (dragcol < 0) ? col_number(coff) : dragcol;
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

                    setpointershape(*widp == 0 ? POINTER_DRAGCOLUMN_RIGHT : POINTER_DRAGCOLUMN);

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

extern void
process_drag(void)
{
    wimp_wstate wstate;
    wimp_mousestr m;
    int x, y;
    BOOL shiftpressed = host_shift_pressed();

    (void) wimp_get_point_info(&m);
    x = m.x;
    y = m.y;

    trace_4(TRACE_APP_PD4, "mouse pointer at w %d i %d x %d y %d\n", m.w, m.i, m.x, m.y);

    trace_0(TRACE_APP_PD4, "continuing drag: button still held\n");

    switch(dragtype)
        {
        case DRAG_COLUMN_WIDTH:
        case DRAG_COLUMN_WRAPWIDTH:
            break;

        default:
            /* scroll text if outside this guy's window */
            (void) wimp_get_wind_state(main__window, &wstate);

            if(dragtype != MARK_ALL_COLUMNS)
                {
                /* not dragging down row border */           /* vvvv change to N_PageLeft/Right when implemented */
                if(x < (wstate.o.box.x0
                       + (borbit ? texttooffset_x(borderwidth) : 0)
                       - (charwidth / 2))) /* SKS */
                    application_process_command(shiftpressed ? N_ScrollLeft  : N_ScrollLeft);
                else if(x >= wstate.o.box.x1)
                    application_process_command(shiftpressed ? N_ScrollRight : N_ScrollRight);
                }

            if(dragtype != MARK_ALL_ROWS)
                {
                /* not dragging across col border */
                if(y < wstate.o.box.y0)
                    application_process_command(shiftpressed ? N_PageDown : N_ScrollDown);
                else if(y >= wstate.o.box.y1)
                    application_process_command(shiftpressed ? N_PageUp   : N_ScrollUp);
                }
            break;
        }

    application_drag(x, y, FALSE);
}

/* end of markers.c */
