/* blocks.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC 8.8.91 */

#include "common/gflags.h"

#include "datafmt.h"

#define DO_SAVE 1
#define DONT_SAVE 0
#define DO_DELETE 1
#define DONT_DELETE 0
#define ALLOW_WIDENING 1
#define DONT_ALLOW_WIDENING 0

/*
internal functions
*/

_Check_return_
static S32
copyslot(
    DOCNO docno_from,
    COL cs_oldcol,
    ROW cs_oldrow,
    DOCNO docno_to,
    COL tcol,
    ROW trow,
    COL coldiff,
    ROW rowdiff,
    S32 update_refs,
    S32 add_refs);

static void
do_the_replicate(
    _InRef_     PC_SLR src_start,
    _InRef_     PC_SLR src_end,
    _InRef_     PC_SLR res_start,
    _InRef_     PC_SLR res_end);

/*ncr*/
static BOOL
insert_blank_block(
    COL atcol,
    ROW atrow,
    COL colsno,
    ROW rowsno);

_Check_return_
static BOOL
recover_deleted_block(
    SAVED_BLOCK_DESCRIPTOR * sbdp);

/*ncr*/
static S32
refs_adjust_add(
    P_CELL p_cell,
    _InRef_     PC_EV_SLR slrp,
    _InRef_     PC_UREF_PARM upp,
    S32 update_refs,
    S32 add_refs);

static void
remove_deletion(
    S32 key);

/*
Paste data
*/

static S32 start_pos_on_stack   = 0;   /* one before the first one on stack */
       S32 latest_word_on_stack = 0;

/*
block_updref is like upfred except that it takes only a column number,
and operates on the block of that column and all columns to the right

Adds on coffset to the column part of cell references

Used by:    insert on load
            save_block_on_stack
            restore_block_from_stack
*/

/******************************************************************************
*
* adjust the references in a block of cells
* (all cells to the right of, and below startcol, startrow)
*
* the references must not be on the tree at the time of call!
*
* when leaving blocks on the paste list, call with to_doc==DOCNO_NONE;
* this sets the document numbers of internal cell references to zero;
* when restoring the block, set the from_doc==DOCNO_NONE &
* to_doc to the target document; this restores internal references.
*
* external reference document numbers are left alone, but zapped when
* restoring if they point to duff documents. clearly the refs may become
* wild if the document numbers have re-shuffled in the meantime
*
******************************************************************************/

extern void
block_updref(
    _InVal_     DOCNO docno_to,
    _InVal_     DOCNO docno_from,
    COL startcol,
    ROW startrow,
    COL coffset,
    ROW roffset)
{
    P_CELL tcell;
    SLR topleft;
    SLR botright;
    UREF_PARM urefb;
    EV_SLR slr;

    topleft.col = startcol;
    topleft.row = startrow;
    botright.col = numcol-1;
    botright.row = numrow-1;

    assert(botright.row >= topleft.row);

    init_block(&topleft, &botright);

    /* set source document number */
    urefb.slr2.docno = docno_from;

    set_ev_slr(&urefb.slr3, coffset, roffset);
    urefb.slr3.docno = docno_to;

    urefb.action = UREF_ADJUST;

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        set_ev_slr(&slr, in_block.col, in_block.row);
        refs_adjust_add(tcell, &slr, &urefb, TRUE, FALSE);
    }
}

/******************************************************************************
*
* check second range is null, or comes after first
*
******************************************************************************/

_Check_return_
static BOOL
check_range(
    _InRef_     PC_SLR first,
    _InRef_     PC_SLR second)
{
    if(second->col == NO_COL)
        return(TRUE);

    return((second->col >= first->col)  &&  (second->row >= first->row));
}

/******************************************************************************
*
*  copy block
*
******************************************************************************/

/*ncr*/
static BOOL
do_CopyBlock(
    S32 update_refs,
    S32 add_refs)
{
    COL coldiff, newcolumn, colsno, tcol;
    ROW rowdiff, rowsno, roff;
    S32 errorval = 1, block_was_blank;
    DOCNO docno_from, docno_to;
    UREF_PARM urefb;

    if(!set_up_block(FALSE))
        return(FALSE);

    docno_from = blk_docno;
    docno_to = current_docno();

    rowdiff = currow - blkstart.row;
    coldiff = curcol - blkstart.col;
    rowsno  = blkend.row - blkstart.row + 1;
    colsno  = blkend.col - blkstart.col + 1;

    set_ev_slr(&urefb.slr1, curcol,          currow);
    set_ev_slr(&urefb.slr2, curcol + colsno, currow + rowsno);

    /* if the target block is blank just replicate over it */
    /* if block not blank, insert over all rows */
    if(!is_blank_block(curcol, currow, curcol+colsno-1, currow+rowsno-1))
    {
        block_was_blank = 0;

        if(!insert_blank_block(0, currow, numcol, rowsno))
            return(FALSE);

        /* check if new block is above cos rowdiff needs
         * to take account of old block moving down
        */
        if((docno_from == docno_to)  &&  (currow <= blkstart.row))
            rowdiff -= rowsno;
    }
    else
    {
        urefb.action = UREF_REPLACE;
        ev_uref(&urefb);
        block_was_blank = 1;
    }

    out_screen = out_rebuildvert = out_rebuildhorz = TRUE;

    escape_enable();

    /* copy column by column */

    for(newcolumn = curcol, tcol = blkstart.col;
        !ctrlflag  &&  (tcol <= blkend.col);
        tcol++, newcolumn++)
    {
        for(roff = 0; !ctrlflag  &&  (roff < rowsno); roff++)
        {
            errorval = copyslot(docno_from, tcol, blkstart.row+roff,
                                docno_to, newcolumn, currow+roff,
                                coldiff, rowdiff,
                                update_refs, add_refs);

            if(errorval < 0)
                goto BREAKOUT;
        }
    }

BREAKOUT:

    escape_disable();

    if(errorval < 0)
        reperr_null(errorval);

    /* tell dependents of the blank area they have been affected */
    if(block_was_blank)
    {
        urefb.action = UREF_CHANGE;
        ev_uref(&urefb);
    }

    filealtered(TRUE);

    return((BOOL) (errorval >= 0));
}

extern void
CopyBlock_fn(void)
{
    (void) do_CopyBlock(TRUE, TRUE);
}

/*
copy a block of cells to a new column of to the right
if it fails it must tidy up the world as if nothing happened
*/

_Check_return_
static S32
copy_cells_to_eoworld(
    COL fromcol,
    ROW fromrow,
    COL csize,
    ROW rsize)
{
    const DOCNO docno = current_docno();
    COL o_numcol = numcol;
    COL coldiff  = numcol - fromcol;
    COL tcol;
    ROW trow;
    STATUS errorval = 1;

    trace_0(TRACE_APP_PD4, "copy_cells_to_eoworld");

    for(tcol = 0; !ctrlflag  &&  (tcol < csize); tcol++)
    {
        trace_2(TRACE_APP_PD4, "copy_cells_to_eoworld, tcol: %d, numcol: %d", tcol, numcol);

        /* copy from fromcol + tcol to o_numcol + tcol */
        /* create a gap in the column to copy the cells to  */
        for(trow = 0; !ctrlflag  &&  (trow < rsize); trow++)
        {
            /* RJM 21.11.91 - if at end of both columns, don't bother */
            if(trow > 0 && atend(fromcol+tcol, fromrow+trow) && atend(o_numcol+tcol, trow+BLOCK_UPDREF_ROW))
                break;

            errorval = copyslot(docno, fromcol  + tcol, fromrow + trow,
                                docno, o_numcol + tcol, trow+BLOCK_UPDREF_ROW,
                                coldiff, -fromrow,
                                FALSE, FALSE);

            if(errorval < 0)
                break;
        }

        if(errorval < 0)
            break;

        /* make sure it's compact */
        pack_column(o_numcol + tcol);
    }

    if((errorval > 0)  &&  ctrlflag)
        errorval = create_error(ERR_ESCAPE);

    if(errorval < 0)
        /* tidy up mess */
        delcolandentry(o_numcol, numcol - o_numcol);

    trace_0(TRACE_APP_PD4, "exit copy_cells_to_eoworld");
    return(errorval);
}

/******************************************************************************
*
* createslot at tcol, trow and copy srcslot to it
* update cell references, adding on coldiff and rowdiff
* works between windows, starts and finishes in towindow
*
******************************************************************************/

_Check_return_
static S32
copyslot(
    DOCNO docno_from,
    COL cs_oldcol,
    ROW cs_oldrow,
    DOCNO docno_to,
    COL tcol,
    ROW trow,
    COL coldiff,
    ROW rowdiff,
    S32 update_refs,
    S32 add_refs)
{
    P_CELL newslot;
    EV_SLR slr;
    UREF_PARM urefb;

    select_document_using_docno(docno_to);
    set_ev_slr(&slr, tcol, trow);

    /* if we are not copying cell to itself */
    if((docno_from != docno_to) ||
       (cs_oldcol  != tcol) ||
       (cs_oldrow  != trow) )
    {
        P_CELL oldslot;
        S32 slotlen;
        char type;

        oldslot = travel_externally(docno_from, cs_oldcol, cs_oldrow);

        slot_free_resources(travel(tcol, trow));

        /* create blank new cell */
        if(!oldslot)
        {
            S32 res;

            res = createhole(tcol, trow);
            return(res ? 1 : status_nomem());
            }

        type    = oldslot->type;
        slotlen = slotcontentssize(oldslot);

        if((newslot = createslot(tcol, trow, slotlen, type)) == NULL)
            return(status_nomem());

        /* reload old pointer */
        oldslot = travel_externally(docno_from, cs_oldcol, cs_oldrow);

        copycont(newslot, oldslot, slotlen);
    }
    else
        newslot = travel_externally(docno_to, tcol, trow);

    /* MRJC 8.10.91 */
    mark_slot(newslot);

    urefb.slr2.docno = docno_from;
    set_ev_slr(&urefb.slr3, coldiff, rowdiff);
    urefb.action = UREF_COPY;

    /* adjust refs in copied cell and add to tree */
    return(refs_adjust_add(newslot, &slr, &urefb, update_refs, add_refs));
}

extern void
ClearBlock_fn(void)
{
    /* fault marked blocks not in this document */
    if((blk_docno != current_docno())  &&  (blkstart.col != NO_COL))
    {
        reperr_null(ERR_NOBLOCKINDOC);
        return;
    }

    if(!mergebuf())
        return;

    /* clear the block out */
    linbuf[0] = CH_NULL;
    buffer_altered = TRUE;
    if(!merstr(blkstart.col, blkstart.row, TRUE, TRUE))
        return;

    /* copy the empty cell across the rest of the block */
    do_the_replicate(&blkstart, &blkstart,
                     &blkstart, &blkend);
}

/******************************************************************************
*
* delete block
*
******************************************************************************/

extern void
DeleteBlock_fn(void)
{
    /* remember top-left of block for repositioning */
    BOOL caret_in_block = inblock(curcol, currow);
    COL col = blkstart.col;
    ROW row = blkstart.row;

    /* fault marked blocks not in this document */
    if((blk_docno != current_docno())  &&  (col != NO_COL))
    {
        reperr_null(ERR_NOBLOCKINDOC);
        return;
    }

    do_delete_block(DO_SAVE, ALLOW_WIDENING, FALSE);      /* does mergebuf() eventually */

    if(caret_in_block)
    {
        /* move to top left of block */
        chknlr(col, row);
        lecpos = 0;
    }

    out_rebuildvert = out_screen = TRUE;
    filealtered(TRUE);
}

/******************************************************************************
*
* delete the marked block
*
* called by DeleteBlock and a failed load
*
******************************************************************************/

/*ncr*/
extern S32
do_delete_block(
    BOOL do_save,
    BOOL allow_widening,
    BOOL ignore_protection)
{
    S32 res = TRUE;

    trace_0(TRACE_APP_PD4, "do_delete_block()");

    if(!ignore_protection && protected_cell_in_range(&blkstart, &blkend))
        return(-1);

    /* just save the block to the paste list, don't delete it yet */
    if(do_save)
        res = save_block_and_delete(DONT_DELETE, DO_SAVE);

    if(res)
    {
        /* if we can't widen just delete the block */
        if(!allow_widening)
            res = save_block_and_delete(DO_DELETE, DONT_SAVE);
        else
        {
            /* can widen so see if we are blank at sides */

            /* if blank to the side of the block */
            if( is_blank_block(0, blkstart.row, blkstart.col-1, blkend.row) &&
                is_blank_block(blkend.col+1,blkstart.row,numcol-1,blkend.row) )
            {
                /* blank at sides so delete the whole rows */
                blkstart.col = 0;
                blkend.col = numcol-1;
                res = save_block_and_delete(DO_DELETE, DONT_SAVE);
            }
            else
            {
                /* not blank so delete the block then create a hole; effectively a clear operation */
                res = save_block_and_delete(DO_DELETE, DONT_SAVE);
                if(res)
                {
                    insert_blank_block(blkstart.col, blkstart.row,
                            blkend.col-blkstart.col+1,
                            blkend.row-blkstart.row+1);
                }
            }
        }
    }

    /* delete markers */
    if(res)
        blkstart.col = NO_COL;

    return(res);
}

static void
do_fill(
    int command)
{
    SLR src_start, src_end;
    SLR res_start, res_end;

    if(!set_up_block(TRUE))
        return;
/*
    if(!MARKER_DEFINED())
        reperr_null((blkstart.col != NO_COL)
                            ? create_error(ERR_NOBLOCKINDOC)
                            : create_error(ERR_NOBLOCK));
*/
    if(blkend.col == NO_COL)
    {
        reperr_null(ERR_BAD_MARKER);
        return;
    }

    switch(command)
    {
	default: default_unhandled();
    case N_ReplicateDown:
        if(blkend.row == blkstart.row)
        {
            reperr_null(ERR_BAD_MARKER);
            return;
        }

        src_start.col = blkstart.col;
        src_start.row = blkstart.row;

        src_end.col = blkend.col;
        src_end.row = src_start.row;

        res_start.col = blkstart.col;
        res_start.row = blkstart.row + 1;

        res_end.col = res_start.col;
        res_end.row = blkend.row;
        break;

    case N_ReplicateUp:
        if(blkend.row == blkstart.row)
        {
            reperr_null(ERR_BAD_MARKER);
            return;
        }

        src_start.col = blkstart.col;
        src_start.row = blkend.row;

        src_end.col = blkend.col;
        src_end.row = src_start.row;

        res_start.col = blkstart.col;
        res_start.row = blkstart.row;

        res_end.col = res_start.col;
        res_end.row = blkend.row - 1;
        break;

    case N_ReplicateRight:
        if(blkend.col == blkstart.col)
        {
            reperr_null(ERR_BAD_MARKER);
            return;
        }

        src_start.col = blkstart.col;
        src_start.row = blkstart.row;

        src_end.col = src_start.col;
        src_end.row = blkend.row;

        res_start.col = blkstart.col + 1;
        res_start.row = blkstart.row;

        res_end.col = blkend.col;
        res_end.row = res_start.row;
        break;

    case N_ReplicateLeft:
        if(blkend.col == blkstart.col)
        {
            reperr_null(ERR_BAD_MARKER);
            return;
        }

        src_start.col = blkend.col;
        src_start.row = blkstart.row;

        src_end.col = src_start.col;
        src_end.row = blkend.row;

        res_start.col = blkstart.col;
        res_start.row = blkstart.row;

        res_end.col = blkend.col - 1;
        res_end.row = res_start.row;
        break;
    }

    do_the_replicate(&src_start, &src_end,
                     &res_start, &res_end);
}

/*
do the hard work in a replicate. Called by Cfunc, BRRfunc, BRDfunc, BREfunc
*/

static void
do_the_replicate(
    _InRef_     PC_SLR src_start,
    _InRef_     PC_SLR src_end,
    _InRef_     PC_SLR res_start,
    _InRef_     PC_SLR res_end)
{
    const DOCNO docno = current_docno();
    COL src_col, tcol;
    ROW src_row, trow;
    S32 errorval = 1;
    S32 slot_count, slot_pos;
    UREF_PARM urefb;
    SLR actual_res_end;

    /* calculate the actual block affected - don't be deceived
     * by the names of the parameters passed in
     */
    actual_res_end = *res_end;
    if(res_end->col == res_start->col)
        actual_res_end.col += src_end->col - src_start->col;
    if(res_end->row == res_start->row)
        actual_res_end.row += src_end->row - src_start->row;

    if(protected_cell_in_range(res_start, &actual_res_end))
        return;

    /* do tree operation */
    set_ev_slr(&urefb.slr1, res_start->col,         res_start->row);
    set_ev_slr(&urefb.slr2, actual_res_end.col + 1, actual_res_end.row + 1);
    urefb.action = UREF_REPLACE;
    ev_uref(&urefb);

    slot_count = ((S32) src_end->col - src_start->col + 1) *
                 ((S32) src_end->row - src_start->row + 1) *
                 ((S32) res_end->col - res_start->col + 1) *
                 ((S32) res_end->row - res_start->row + 1);
    slot_pos = 0;

    escape_enable();

    /* each column in source */
    for(src_col = src_start->col;
        !ctrlflag  &&  (src_col <= src_end->col);
        src_col++)
    {
        /* each row in source */
        for(src_row = src_start->row;
            !ctrlflag  &&  (src_row <= src_end->row);
            src_row++)
        {
            /* target block is already set up as current block, just need to initialize */
            init_block(res_start, res_end);

            while(!ctrlflag  &&  next_in_block(DOWN_COLUMNS))
            {
                tcol = in_block.col + src_col - src_start->col;
                trow = in_block.row + src_row - src_start->row;

                actind((S32) ((100l * slot_pos++) / slot_count));

                if(tcol >= numcol)
                    if(!createcol(tcol))
                    {
                        errorval = status_nomem();
                        break;
                    }

                /* copy the cell */
                if(errorval < 0)
                {
                    slot_free_resources(travel(tcol, trow));
                    createhole(tcol, trow);
                }
                else
                    errorval = copyslot(docno, src_col, src_row,
                                        docno, tcol, trow,
                                        tcol - src_col, trow - src_row,
                                        TRUE, TRUE);
            }
        }
    }

    escape_disable();

    actind_end();

    if(errorval < 0)
        reperr_null(errorval);

    /* send change block */
    urefb.action = UREF_CHANGE;
    ev_uref(&urefb);

    slot_in_buffer = FALSE;
    out_screen = TRUE;

    filealtered(TRUE);
}

extern void
ensure_paste_list_clipped(void)
{
    /* remove all duffo entries from paste list */

    while(latest_word_on_stack - start_pos_on_stack > words_allowed)
        remove_deletion(++start_pos_on_stack);
}

/*
insert a blank block, with updref
*/

/*ncr*/
static BOOL
insert_blank_block(
    COL atcol,
    ROW atrow,
    COL colsno,
    ROW rowsno)
{
    COL tcol;
    ROW roff;

    /* maybe we ought to clear up a bit here, on failure ??? */

    for(tcol = atcol; tcol < atcol+colsno; tcol++)
        for(roff = 0; roff < rowsno; roff++)
            if(!insertslotat(tcol, atrow))
                return(FALSE);

    /* update references for the gap just created */
    updref(atcol, atrow, atcol+colsno-1, LARGEST_ROW_POSSIBLE, 0, rowsno, UREF_UREF, DOCNO_NONE);

    return(TRUE);
}

/*
is this block all blank
*/

_Check_return_
extern BOOL
is_blank_block(
    COL cs,
    ROW rs,
    COL ce,
    ROW re)
{
    COL tcol;
    ROW trow;

    for(tcol = cs; tcol <= ce; tcol++)
    {
        for(trow = rs; trow <= re; trow++)
        {
            /* see if past the end of the column */
            if(atend(tcol, trow))
                break;

            if(!is_blank_cell(travel(tcol, trow)))
                return(FALSE);
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* move block
*
******************************************************************************/

extern void
MoveBlock_fn(void)
{
    MoveBlock_fn_do(FALSE);
}

extern void
MoveBlock_fn_do(S32 add_refs)
{
    COL colsize, col_to;
    ROW rowsize, row_to, block_will_move = 0;
    SLR bs, be;
    DOCNO bd, docno_from, docno_to;

    if(!mergebuf())
        return;

    /* get size of markers */
    if(!set_up_block(FALSE))
        return;

    docno_from = blk_docno;
    docno_to = current_docno();

    colsize = blkend.col - blkstart.col;
    rowsize = blkend.row - blkstart.row;

    /* only worry about overlap when the block would split in the copy
        this is when cursor on a row coinciding with the block but not the first
        could be more optimal, not a problem if moving to a gap
    */
    if(docno_from == docno_to)
    {
        if((currow > blkstart.row)  &&  (currow <= blkend.row))
        {
            /* RJM 27.9.91 is provoked into:
                if target block is blank no overlap problem cos rows
                will not be inserted.  Looks like I thought
                about this before and decided to save precious RAM
            */
            if(!is_blank_block(curcol, currow, curcol+colsize, currow+rowsize))
            {
                reperr_null(ERR_OVERLAP);
                return;
            }
        }
        /* do nothing if current position is top of block */
        if(curcol == blkstart.col && currow == blkstart.row)
            return;
    }

    if(!do_CopyBlock(FALSE, add_refs))
        return;

    /* remember block position so can delete the right block after the updref */
    bd = blk_docno;
    bs = blkstart;
    be = blkend;

    col_to = curcol;
    row_to = currow;

    __assume(IS_ARRAY_INDEX_VALID(docno_from, elemof32(docu_array)));
    select_document_using_docno(docno_from);

    /* updref - redirect cell references from old block to new block */
    updref(blkstart.col, blkstart.row, blkend.col, blkend.row, col_to-blkstart.col, row_to-blkstart.row, UREF_UREF, docno_to);

    /* reset markers to old block */
    blk_docno = bd;
    blkstart  = bs;
    blkend    = be;

    /* maybe deleting old block affects position of new block ; better check */
    if((docno_from == docno_to) &&
        (blkend.row < currow) &&
        is_blank_block(0,blkstart.row, blkstart.col-1, blkend.row) &&
        is_blank_block(blkend.col+1, blkstart.row, numcol-1, blkend.row))
        {
        block_will_move = blkend.row - blkstart.row + 1;
        }

    /* now go delete old block */
    do_delete_block(DONT_SAVE, ALLOW_WIDENING, TRUE);

    if(docno_to != docno_from)
    {
        /* the mergebuf was in the to window */
        reset_numrow();
        slot_in_buffer = FALSE;
        out_screen = out_rebuildvert = TRUE;
        filealtered(TRUE);
        draw_screen();
        select_document_using_docno(docno_to);
        reset_numrow();
    }

    /* put markers at cursor pos */
    blkend.col = colsize + (blkstart.col = curcol);
    blkend.row = rowsize + (blkstart.row = currow - block_will_move);
    blk_docno = docno_to;
}

/******************************************************************************
*
* paste word back into text
*
******************************************************************************/

extern void
Paste_fn(void)
{
    PC_LIST lptr;
    SLR oldpos;
    S32 tlecpos;

    xf_flush = TRUE;

    if(latest_word_on_stack <= start_pos_on_stack)
    {
        latest_word_on_stack = start_pos_on_stack = 0;
        bleep();
        return;
    }

    /* find the top entry on the stack */
    lptr = search_list(&deleted_words, latest_word_on_stack);

    if(!lptr)
    {
        /* no word on list for this number - is there a block? */
        lptr = search_list(&deleted_words, latest_word_on_stack + BLOCK_OFFSET);

        if(!lptr)
        {
            /* no block either so go home */
            --latest_word_on_stack;
            return;
        }
        else if(!xf_inexpression && !xf_inexpression_box && !xf_inexpression_line)
        {
            if(!mergebuf())
                return;

            if(recover_deleted_block((SAVED_BLOCK_DESCRIPTOR *) lptr->value))
                /* delete entry from stack */
                delete_from_list(&deleted_words, BLOCK_OFFSET + latest_word_on_stack--);

            return;
        }
        else
        {
            /* won't recover block when editing expression */
            reperr_null(ERR_EDITINGEXP);
            return;
        }
    }

    /* insert word: save old position */
    oldpos.col = curcol;
    oldpos.row = currow;
    tlecpos = lecpos;

    if(!insert_string(lptr->value, TRUE))
        return;

    chkwrp();

    /* now move cursor back */
    if(!mergebuf_nocheck())
        return;

    chknlr(oldpos.col, oldpos.row);
    lecpos = tlecpos;

    /* delete entry from stack */
    delete_from_list(&deleted_words, latest_word_on_stack--);
}

/******************************************************************************
*
* set the paste list depth
*
******************************************************************************/

extern void
PasteListDepth_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DELETED))
    {
        ensure_paste_list_clipped();

        if(!dialog_box_can_persist())
            break;
    }

/* printf("latest=%ld,oldest=%ld,allowed=%d",latest_word_on_stack,start_pos_on_stack,(int) words_allowed);rdch(1,1); */

    dialog_box_end();
}

/******************************************************************************
*
* recover deleted block
*
******************************************************************************/

_Check_return_
static BOOL
recover_deleted_block(
    SAVED_BLOCK_DESCRIPTOR * sbdp)
{
    P_COLENTRY delete_colstart = sbdp->del_colstart;
    COL delete_size_col = sbdp->del_col_size;
    ROW delete_size_row = sbdp->del_row_size;
    COL new_numcol;
    ROW new_numrow;
    DOCNO bd;
    SLR bs, be;
    BOOL res;

    bd = blk_docno;
    bs = blkstart;
    be = blkend;

    blk_docno = current_docno();

    new_numcol = MAX(numcol, curcol + delete_size_col);
    new_numrow = BLOCK_UPDREF_ROW;

    /* put block at column starting at numcol */
    if(!createcol(new_numcol + delete_size_col - 1))
    {
        trace_0(TRACE_APP_PD4, "failed to create enough columns for block: leave on list");
        return(reperr_null(ERR_CANTLOADPASTEBLOCK));
    }

    /* copy column lists into end of colstart */
    deregcoltab();
    memcpy32(colstart + new_numcol, delete_colstart, sizeof32(COLENTRY) * delete_size_col);
    regcoltab();

    reset_numrow();

    /* poke its cell references so that columns are sensible */
    block_updref(current_docno(), DOCNO_NONE,
                 new_numcol, new_numrow,
                 curcol-BLOCK_UPDREF_COL, currow-BLOCK_UPDREF_ROW);

    /* this is rather crude, but should work given beaucoup de memory
        if not enough memory, will leave a copy of the block at either the cursor or, more likely
        the end of world, and a mess at the other place.  User doesn't lose data, but gains a
        pig's ear.  Is this good enough?

        the excuse, mlud, is that I wrote it when we were in "do what's quickest for the
        programmer mode"
    */

    blkstart.col = new_numcol;
    blkend.col =   blkstart.col + delete_size_col -1;
    blkstart.row = BLOCK_UPDREF_ROW;
    blkend.row =   blkstart.row + delete_size_row -1;
    MoveBlock_fn_do(TRUE);

    res = !been_error;
    if(!been_error)
    {
        /* delete any leftover contents and structure */
        delcolandentry(new_numcol, numcol - new_numcol);
        al_ptr_free(delete_colstart);
    }

    reset_numrow();

    blk_docno = bd;
    blkstart  = bs;
    blkend    = be;

    out_screen = out_rebuildvert = out_rebuildhorz = TRUE;

    return(res);
}

/******************************************************************************
*
* maybe adjust the references in the cell and maybe then add them to the tree
*
******************************************************************************/

/*ncr*/
static S32
refs_adjust_add(
    P_CELL p_cell,
    _InRef_     PC_EV_SLR slrp,
    _InRef_     PC_UREF_PARM upp,
    S32 update_refs,
    S32 add_refs)
{
    S32 err;

    err = 0;
    if(p_cell && (update_refs || add_refs))
    {
        if(p_cell->type == SL_NUMBER)
        {
            P_EV_CELL p_ev_cell;

            /* update references in the copied cell for any move */
            if(ev_travel(&p_ev_cell, slrp) > 0)
            {
                if(update_refs)
                    ev_rpn_adjust_refs(p_ev_cell, upp);

                if(add_refs)
                {
                    if(ev_add_exp_slot_to_tree(p_ev_cell, slrp) < 0)
                    {
                        tree_insert_fail(slrp);
                        err = status_nomem();
                    }
                    else
                        ev_todo_add_slr(slrp, 0);
                }
            }
        }
        else if(p_cell->type != SL_PAGE)
        {
            if(update_refs)
            {
                uchar * csr = p_cell->content.text;
                while(NULL != (csr = find_next_csr(csr)))
                {
                    /*eportf("refs_adjust_add: text_csr_uref");*/
                    csr = text_csr_uref(csr, upp);
                }
            }

            if(add_refs)
            {
                if(draw_tree_str_insertslot((COL) slrp->col, (ROW) slrp->row, 0) < 0)
                {
                    tree_insert_fail(slrp);
                    err = status_nomem();
                }
            }
        }
    }

    return(err);
}

/*
delete a numbered entry from the deletions list

if keyed entry exists it is string, otherwise key+BLOCK_OFFSET is ptr to block
*/

static void
remove_deletion(
    S32 key)
{
    PC_LIST lptr;
    SAVED_BLOCK_DESCRIPTOR * sbdp;
    P_COLENTRY cptr;
    COL csize;

    trace_4(TRACE_APP_PD4, "remove_deletion key: %d, start_pos: %d, latest: %d, allowed: %d",
            key, start_pos_on_stack, latest_word_on_stack, words_allowed);

    /* try removing as a string */
    if(delete_from_list(&deleted_words, key))
    {
        trace_0(TRACE_APP_PD4, "remove_deletion removed string");
        return;
    }

    /* try removing as a block */
    lptr = search_list(&deleted_words, key + BLOCK_OFFSET);
    if(lptr)
    {
        trace_2(TRACE_APP_PD4, "remove_deletion lptr: %x, lptr->value: %x",
                (S32) lptr, (S32) lptr->value);

        sbdp    = (SAVED_BLOCK_DESCRIPTOR *) lptr->value;

        cptr    = sbdp->del_colstart;
        csize   = sbdp->del_col_size;

        trace_2(TRACE_APP_PD4, "remove_deletion cptr: %x, csize: %x",
                (S32) cptr, (S32) csize);

        /* must register before deletion */
        regtempcoltab(cptr, csize);

        trace_0(TRACE_APP_PD4, "remove_deletion about to delcolstart");
        delcolstart(cptr, csize);

        trace_0(TRACE_APP_PD4, "remove_deletion about to delete_from_list");
        delete_from_list(&deleted_words, key + BLOCK_OFFSET);

        trace_0(TRACE_APP_PD4, "remove_deletion done delete_from_list");
    }

    trace_0(TRACE_APP_PD4, "exit remove_deletion");
}

/******************************************************************************
*
* replicate
*
******************************************************************************/

static int
replicate_fn_core(void)
{
    SLR src_start, src_end;
    SLR res_start, res_end;

    /* get two blocks from dialog box:
     *      first block in src_start, src_end
     *      second block in blkstart, blkend
    */

    /* get source range */
    buff_sofar = (uchar *) d_replicate[0].textfield;
    if(buff_sofar == NULL)
        buff_sofar = UNULLSTR;
    src_start.col =       getcol();
    src_start.row = (ROW) getsbd()-1;
    src_end.col   =       getcol();
    src_end.row   = (ROW) getsbd()-1;

    /* get target range */
    buff_sofar = (uchar *) d_replicate[1].textfield;
    if(buff_sofar == NULL)
        buff_sofar = UNULLSTR;
    res_start.col =       getcol();
    res_start.row = (ROW) getsbd()-1;
    res_end.col   =       getcol();
    res_end.row   = (ROW) getsbd()-1;

    /* check first markers */
    if( bad_reference(res_start.col, res_start.row) ||
        bad_reference(src_start.col, src_start.row) )
    {
        reperr_null(ERR_BAD_CELL);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    /* make sure second marker is sensible */
    if( res_end.col == NO_COL)
        res_end = res_start;

    if( src_end.col == NO_COL)
        src_end = src_start;

    /* check not column of columns or row of rows */
    if( ((res_end.col - res_start.col > 0)  &&  (src_end.col - src_start.col > 0))  ||
        ((res_end.row - res_start.row > 0)  &&  (src_end.row - src_start.row > 0))  )
    {
        reperr_null(ERR_BAD_RANGE);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    /* check ranges point in right direction */
    if( !check_range(&res_start, &res_end)  ||
        !check_range(&src_start, &src_end)  )
    {
        reperr_null(ERR_BAD_RANGE);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    do_the_replicate(&src_start, &src_end,
                     &res_start, &res_end);

    out_rebuildvert = out_rebuildhorz = TRUE;

    return(TRUE);
}

static BOOL
replicate_fn_prepare(void)
{
    char array[LIN_BUFSIZ];

    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_REPLICATE));

    /* write current block into the source range */

    if(MARKER_DEFINED())
    {
        U32 out_idx = 0;

        out_idx += write_ref(array, elemof32(array), current_docno(), blkstart.col, blkstart.row);

        if(blkend.col == NO_COL)
            blkend = blkstart;

        if((blkend.col != blkstart.col)  ||  (blkend.row != blkstart.row))
        {
            array[out_idx++] = SPACE;
            /*out_idx +=*/ (void) write_ref(&array[out_idx], elemof32(array) - out_idx, current_docno(), blkend.col, blkend.row);
        }

        false_return(mystr_set(&d_replicate[0].textfield, array));
    }

    /* write current position into the target range */

    (void) write_ref(array, elemof32(array), current_docno(), curcol, currow);

    false_return(mystr_set(&d_replicate[1].textfield, array));

    return(dialog_box_start());
}

extern void
Replicate_fn(void)
{
    if(!replicate_fn_prepare())
        return;

    while(dialog_box(D_REPLICATE))
    {
        int core_res = replicate_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

extern void
ReplicateDown_fn(void)
{
    do_fill(N_ReplicateDown);
}

extern void
ReplicateRight_fn(void)
{
    do_fill(N_ReplicateRight);
}

extern void
ReplicateUp_fn(void)
{
    do_fill(N_ReplicateUp);
}

extern void
ReplicateLeft_fn(void)
{
    do_fill(N_ReplicateLeft);
}

/* save block of cells on to the deleted block stack
 * returns TRUE if successfully saved or user wants to proceed anyway
 * if cannot save block it leaves world as it found it and puts out dialog
 * box asking if user wants to proceed.  If so it returns TRUE, otherwise FALSE
*/

_Check_return_
extern BOOL
save_block_and_delete(
    BOOL is_deletion,
    BOOL do_save)
{
    SLR bs, be, curpos;
    DOCNO bd;
    BOOL res = TRUE;
    S32 array_size_bytes = 0; /* keep compiler dataflow analyser happy */
    S32 copyres, save_active;
    P_LIST lptr = NULL;
    P_COLENTRY delete_colstart;
    COL delete_size_col;
    ROW delete_size_row;
    SAVED_BLOCK_DESCRIPTOR * sbdp;
    S32 mres, eres;

    trace_0(TRACE_APP_PD4, "save_block_and_delete");

    if(!set_up_block(FALSE))
        return(FALSE);

    bd = blk_docno;
    bs = blkstart;
    be = blkend;        /* after forcing sensible block! */

    delete_size_col = be.col - bs.col + 1;
    delete_size_row = be.row - bs.row + 1;

    curpos.col = numcol;
    curpos.row = BLOCK_UPDREF_ROW;
    save_active = do_save && (words_allowed > 0);

    if(save_active)
    {
        /* allow escape during copying phase */
        escape_enable();

        /* get new colstart first of all */
        array_size_bytes = sizeof32(COLENTRY) * delete_size_col;

        /* put block on deleted_words list */
        if( (NULL != (delete_colstart = al_ptr_alloc_elem(COLENTRY, delete_size_col, &mres))) &&
            (NULL != (lptr = add_list_entry(&deleted_words, sizeof32(SAVED_BLOCK_DESCRIPTOR), &mres))) )
        {
            lptr->key = ++latest_word_on_stack + BLOCK_OFFSET;

            sbdp = (SAVED_BLOCK_DESCRIPTOR *) lptr->value;

            trace_4(TRACE_APP_PD4, "save_block_and_delete saving block: key %d colstart " PTR_XTFMT ", cols %d, rows %d",
                    lptr->key, report_ptr_cast(delete_colstart), delete_size_col, delete_size_row);

            sbdp->del_colstart  = delete_colstart;
            sbdp->del_col_size  = delete_size_col;
            sbdp->del_row_size  = delete_size_row;
        }

        /* if those worked, copy cells to a parallel structure in this sheet */
        if(delete_colstart  &&  lptr)
            copyres = copy_cells_to_eoworld(bs.col, bs.row,
                                            delete_size_col, delete_size_row);
        else
            copyres = (mres < 0) ? mres : status_nomem();

        if(((eres = escape_disable_nowinge()) < 0)  &&  (copyres >= 0))
            copyres = eres;

        /*if(is_deletion)
            copyres = status_nomem();*/ /* for testing */

        if(copyres < 0)
        {
            /* copy cells failed - might be escape or memory problem */

            if(NULL != lptr)
            {
                --latest_word_on_stack;
                delete_from_list(&deleted_words, lptr->key);
            }

            al_ptr_dispose(P_P_ANY_PEDANTIC(&delete_colstart));

            if((copyres != ERR_ESCAPE)  &&  is_deletion)
            {
                if(init_dialog_box(D_SAVE_DELETED)  &&  dialog_box_start())
                {
                    res = dialog_box(D_SAVE_DELETED);

                    dialog_box_end();
                }

                if(res)
                {
                    res = (d_save_deleted[0].option == 'Y');

                    /* continue with deletion */
                    save_active = FALSE;
                }
            }
            else
            {
                res = reperr_null((copyres == STATUS_NOMEM) ? ERR_CANTSAVEPASTEBLOCK : copyres);
            }

            if(0 == res)
                goto FINISH_OFF;
        }

        /* may have said 'Yes' to continue deletion */
        if(save_active)
        {
            ensure_paste_list_clipped();

            trace_3(TRACE_APP_PD4, "save_block_and_delete, numcol: %d, curpos.col: %d, *cptr: " PTR_XTFMT,
                    numcol, curpos.col, report_ptr_cast(delete_colstart));

            trace_0(TRACE_APP_PD4, "save_block_and_delete: block copied - now curpos.col..numcol-1");
        }
    }

    /* if it's a deletion, delete it and update refs for a move */
    if(is_deletion)
    {
        COL tcol;
        ROW i;

        updref(bs.col, bs.row, be.col, be.row, 0, 0, UREF_DELETE, DOCNO_NONE);

        trace_0(TRACE_APP_PD4, "emptying the deleted block");
        for(tcol = bs.col; tcol <= be.col; tcol++)
            for(i = 0; !ctrlflag  &&  i < delete_size_row; i++)
            {
                trace_2(TRACE_APP_PD4, "killing cell col %d row %d", tcol, bs.row);
                killslot(tcol, bs.row);
            }

        trace_0(TRACE_APP_PD4, "recalcing number of rows");
        reset_numrow();

        trace_0(TRACE_APP_PD4, "updref cells below which have moved up");
        updref(bs.col, be.row + 1, be.col, LARGEST_ROW_POSSIBLE, 0, -delete_size_row, UREF_UREF, DOCNO_NONE);
    }

    if(save_active)
        block_updref(DOCNO_NONE, current_docno(),
                     curpos.col, curpos.row,
                     BLOCK_UPDREF_COL-bs.col, BLOCK_UPDREF_ROW-bs.row);

    if(save_active)
    {
        trace_0(TRACE_APP_PD4, "removing copied columns from sheet");

        /* lose the columns from curpos.col to numcol */
        updref(curpos.col, curpos.row, numcol - 1, curpos.row + delete_size_row - 1, 0, 0, UREF_DELETE, DOCNO_NONE);

        list_unlockpools();

        deregcoltab();

        /* reset number of columns after unlocking and deregistering all new ones */
        numcol = curpos.col;

        /* take a copy of the deregistered end segment to delete_colstart */
        assert(delete_colstart); /* otherwise we would not have got here */
        CODE_ANALYSIS_ONLY(if(NULL != delete_colstart)) /* but clang isn't convinced... */
        memcpy32(delete_colstart, colstart + numcol, array_size_bytes);

        regcoltab();
    }

    /* sort out numrow cos columns disappearing */
    reset_numrow();

    if(is_deletion)
        /* anything left in the file pointing to the disappeared block is bad */
        updref(curpos.col, curpos.row, curpos.col + delete_size_col - 1, curpos.row + delete_size_row - 1, 0, BADROWBIT, UREF_DELETE, DOCNO_NONE);

FINISH_OFF:

    blkstart  = bs;
    blkend    = be;
    blk_docno = bd;

    trace_0(TRACE_APP_PD4, "exit save_block_and_delete");

    /* Nothing to do (I think!), if our caller will call delcolentry() he should */
    /* do something about linked columns first, see DeleteColumn_fn() in execs.c */

    return(res);
}

/*
ensure valid block
*/

_Check_return_
extern BOOL
set_up_block(
    BOOL check_block_in_doc)
{
    if(!mergebuf())
        return(FALSE);

    if(blkstart.col == NO_COL)
        return(reperr_null(ERR_NOBLOCK));

    if(blkend.col == NO_COL)
    {
        trace_0(TRACE_APP_PD4, "set_up_block forcing single mark to block");
        blkend = blkstart;
    }

    if(check_block_in_doc && (blk_docno != current_docno()))
        return(reperr_null(ERR_NOBLOCKINDOC));

    trace_4(TRACE_APP_PD4, "set_up_block ok: start %d %d; end %d %d", blkstart.col, blkstart.row, blkend.col, blkend.row);

    return(TRUE);
}

/******************************************************************************
*
* uref a compiled cell reference embedded in a PipeDream format
* text cell - given pointer to compiled cell reference
*
* --out--
* pointer to byte after compiled cell reference
*
******************************************************************************/

/*ncr*/
extern char *
text_csr_uref(
    uchar * csr,
    _InRef_     PC_UREF_PARM upp)
{
    EV_SLR slr;
    EV_DOCNO docno;
    COL col;
    ROW row;

    /* read slr from text compiled cell reference */
    (void) talps_csr(csr, &docno, &col, &row);
    /*eportf("text_csr_uref: talps docno %d col 0x%x row 0x%x", docno, col, row);*/

    slr.docno = docno;
    slr.col = (EV_COL) (col & COLNOBITS);
    slr.row = (EV_ROW) (row & ROWNOBITS);
    slr.flags = 0;

    if(col & ABSCOLBIT)
        slr.flags |= SLR_ABS_COL;

    if(row & ABSROWBIT)
        slr.flags |= SLR_ABS_ROW;

    if((col & BADCOLBIT) || (row & BADROWBIT))
        slr.flags |= SLR_BAD_REF;

    ev_match_slr(&slr, upp);

    docno = slr.docno;
    col = (COL) slr.col;
    row = (ROW) slr.row;

    if(slr.flags & SLR_ABS_COL)
        col |= ABSCOLBIT;

    if(slr.flags & SLR_ABS_ROW)
        row |= ABSROWBIT;

    if(slr.flags & SLR_BAD_REF)
    {
        col |= BADCOLBIT;
        row |= BADROWBIT;
    }

    /*eportf("text_csr_uref: splat docno %d col 0x%x row 0x%x", docno, col, row);*/
    return(splat_csr(csr, docno, col, row));
}

/*
transpose block

if the block given is a rectangle, check that nothing gets overwritten
if something would get overwritten, insert rows and columns to compensate
*/

extern void
TransposeBlock_fn(void)
{
    S32 exchanges_to_do, exchanges_done, latest_down, last_across, no_of_cols, no_of_rows, last_down;
    COL tcol, new_last_col;
    ROW trow, new_last_row, more_rows/*, extent*/;

    /* get size of markers */
    if(!set_up_block(TRUE))
        return;

    no_of_cols      = blkend.col - blkstart.col + 1;
    no_of_rows      = blkend.row - blkstart.row + 1;
    more_rows       = no_of_rows - (ROW) no_of_cols;
  /*extent          = no_of_rows > (ROW) no_of_cols ? no_of_rows : (ROW) no_of_cols; never used */
    new_last_col    = blkstart.col + (COL) no_of_rows -1;
    new_last_row    = blkstart.row + (ROW) no_of_cols -1;
    exchanges_to_do = no_of_cols * no_of_rows / 2;
    exchanges_done  = 0;
    last_down       = no_of_rows < no_of_cols ? no_of_rows : no_of_cols;
    last_across     = no_of_rows < no_of_cols ? no_of_cols : no_of_rows;

    /* check that nothing gets overwritten */
    if(more_rows > 0)
    {
        /* too many rows */
        if(!is_blank_block(blkend.col+1, blkstart.row, new_last_col, new_last_row))
        {
            /* need to insert columns here, can't use insert_blank_block */

            reperr_null(ERR_OVERLAP);
            return;
        }

        /* try creating the last column first cos its more efficient and the thing
            most likely to fail
        */
        if(!createcol(new_last_col))
        {
            reperr_null(status_nomem());
            return;
        }
    }
    else if(more_rows < 0)
    {
        /* too many cols */
        if(!is_blank_block(blkstart.col, blkend.row+1, new_last_col, new_last_row))
            /* RJM on 6.2.91, after PD 4.11
                changes numcol-1 to numcol in the following
            */
            insert_blank_block(0,blkend.row+1,numcol,-more_rows);
    }

    out_screen = out_rebuildvert = out_rebuildhorz = TRUE;

    escape_enable();

    for(trow = blkstart.row, latest_down=0;
        !ctrlflag && latest_down < last_down;
        trow++, latest_down++)
    {
        S32 latest_right = latest_down + 1;

        for(tcol = blkstart.col+latest_right;
            !ctrlflag && latest_right < last_across;
            tcol++, latest_right++)
        {
            if(!swap_cells(tcol, trow, blkstart.col+latest_down, blkstart.row+latest_right))
            {
                /* we're in deep doo-doo here - half a block transposed and can't go further */
                reperr_null(status_nomem());
                escape_disable();
                actind_end();
                return;
            }

            actind((S32) ((100l * exchanges_done++) / exchanges_to_do));
        }
    }

    escape_disable();

    actind_end();

    /* delete blank rows or cols ? */

    /* update markers for new position */
    blkend.col = new_last_col;
    blkend.row = new_last_row;
}

/* end of blocks.c */
