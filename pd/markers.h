/* markers.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __pd__markers_h
#define __pd__markers_h

/*
exported functions from markers.c
*/

extern void
alter_marked_block(
    COL tcol,
    ROW trow);

extern void
clear_markers(void);

extern void
ended_drag(void);

extern void
force_next_col(void);

extern void
force_next_row(void);

extern BOOL
inblock(                            /* is this slot in marked block? */
    COL tcol,
    ROW trow);

extern void
init_block(                         /* initializes block to be given block or current slot */
    PC_SLR bs,
    PC_SLR be);

extern void
init_marked_block(void);            /* initializes block to be marked block or current slot */

extern void
init_doc_as_block(void);            /* initializes block to be whole document */

extern void
make_single_mark_into_block(void);

#define DOWN_COLUMNS    TRUE
#define ACROSS_ROWS     FALSE

extern BOOL
more_in_block(
    BOOL direction);

extern BOOL
next_in_block(
    BOOL direction);

extern S32
percent_in_block(
    BOOL direction);

extern void
prepare_for_drag_mark(
    coord tx,
    coord ty,
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow);

extern void
prepare_for_drag_column_width(
    coord tx,
    COL col);

extern void
prepare_for_drag_column_wrapwidth(
    coord tx,
    COL col,
    BOOL do_dstwrp);

extern void
process_drag(void);

extern void
set_marked_block(
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow,
    BOOL is_new_block);

extern void
set_marker(
    COL tcol,
    ROW trow);

extern void
start_drag(
    S32 dragt);

extern void
start_marking_block(
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow,
    S32 drag,
    BOOL allow_continue);

extern void
traverse_block_init(
    /*out*/ TRAVERSE_BLOCKP blk,
    DOCNO docno,
    PC_SLR bs,
    PC_SLR be,
    TRAVERSE_BLOCK_DIRECTION direction);

extern BOOL
traverse_block_next(
    /*inout*/ TRAVERSE_BLOCKP blk);

#if RISCOS

extern void
application_button_click_in_main(
    gcoord x,
    gcoord y,
    S32 buttonstate);

extern void
application_drag(
    gcoord x,
    gcoord y,
    BOOL ended);

#endif

#define MARKER_SOMEWHERE()      (blkstart.col != NO_COL)

#define MARKER_DEFINED()        ((blk_docno == current_docno())  &&  \
                                MARKER_SOMEWHERE())

#define MARKED_BLOCK_DEFINED()  (MARKER_DEFINED()  &&  (blkend.col != NO_COL))

#define IN_ROW_BORDER(coff)     ((coff == -1)  &&  (borderwidth  != 0))

/* 'Active' corner of a marked block */
#define ACTIVE_COL  ((blkanchor.col == blkstart.col) ? blkend.col : blkstart.col)
#define ACTIVE_ROW  ((blkanchor.row == blkstart.row) ? blkend.row : blkstart.row)

#endif  /* __pd__markers_h */

/* end of markers.h */
