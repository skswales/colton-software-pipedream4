/* slot.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that travels around the PipeDream data structures */

/* MRJC February 1989 updated for new matrix cell engine from CTYPE */
/* MRJC August 1991 updated for new evaluator */

#include "common/gflags.h"

#include "datafmt.h"

/*
internal functions
*/

#define RJM_NEW_SWAP_SLOT_CORE

typedef struct
{
    COL col;
    ROW  row;
    P_SLOT sl;
    S32   size;
    uchar type;
}
swap_slot_struct;

static void
default_col_entries(
    P_COLENTRY cp);

static S32
swap_slot_core(
    swap_slot_struct *p1,
    swap_slot_struct *p2,
    P_ANY tempslot);

/******************************************************************************
*
* return a flag indicating if past the end of a column
*
******************************************************************************/

extern S32
atend(
    COL col,
    ROW row)
{
    return(row >= list_numitem(indexcollb(col)));
}

/******************************************************************************
*
* return the current row number of a specified column
*
******************************************************************************/

extern ROW
atrow(
    COL tcol)
{
    return((ROW) list_atitem(indexcollb(tcol)));
}

/******************************************************************************
*
* blow away so many row from a column
*
* this does not free slot resources
* use only if you know what you are doing!
* else use killslot etc.
*
******************************************************************************/

extern void
blow_away(
    COL col,
    ROW row,
    ROW n_rows)
{
    P_LIST_BLOCK lp;

    lp = indexcollb(col);
    if(row < list_numitem(lp))
        list_deleteitems(lp, row, n_rows);
}

/******************************************************************************
*
* return column width of column
*
******************************************************************************/

extern coord
colwidth(
    COL col)
{
    P_COLENTRY cp;

    return(((cp = indexcol(col)) != NULL) ? cp->colwidth : 0);
}

/******************************************************************************
*
* return wrap width of a column
*
******************************************************************************/

extern coord
colwrapwidth(
    COL tcol)
{
    P_COLENTRY cp;
    coord ww;

    ww = ((cp = indexcol(tcol)) == NULL) ? 0 : cp->wrapwidth;

    return(!ww ? colwidth(tcol) : ww);
}

/******************************************************************************
*
* is column one of a range of linked columns
*
******************************************************************************/

extern BOOL
colislinked(
    COL tcol)
{
    P_COLENTRY cp;
    S32 colflags;

    colflags = ((cp = indexcol(tcol)) == NULL) ? 0 : cp->colflags;

    return(colflags & (LINKED_COLUMN_LINKRIGHT | LINKED_COLUMN_LINKSTOP));
}

extern BOOL
linked_column_linkright(
    COL tcol)
{
    P_COLENTRY cp;
    S32 colflags;

    colflags = ((cp = indexcol(tcol)) == NULL) ? 0 : cp->colflags;

    return(colflags & LINKED_COLUMN_LINKRIGHT);
}

/******************************************************************************
*
*  copy the contents of one slot into another
*
******************************************************************************/

extern void
copycont(
    P_SLOT nsl,
    P_SLOT osl,
    S32 size)
{
    nsl->type    = osl->type;
    nsl->flags   = osl->flags;
    nsl->justify = osl->justify;

    switch(nsl->type)
        {
        case SL_TEXT:
            memcpy32(nsl->content.text, osl->content.text, size);
            break;

        case SL_PAGE:
            nsl->content.page.condval = osl->content.page.condval;
            nsl->content.page.cpoff   = osl->content.page.cpoff;
            break;

        case SL_NUMBER:
            nsl->format = osl->format;
            ev_exp_copy(&nsl->content.number.guts, &osl->content.number.guts);
            break;

        default:
            break;
        }
}

/******************************************************************************
*
* create a column
* returns success value
* note that colsintable is one less than the maximum numcol that will fit
*
******************************************************************************/

extern BOOL
createcol(
    COL tcol)
{
    P_COLENTRY newblock, nc;
    COL y;
    COL newsize;

    /* col already exists */
    if(tcol < numcol)
        return(TRUE);

    if(tcol > LARGEST_COL_POSSIBLE)
        return(FALSE);

    trace_2(TRACE_APP_PD4, "createcol, numcol: %d, colsintable: %d",
            numcol, colsintable);

    /* allocate new column table if table not big enough */
    newblock = NULL;

    if(tcol >= colsintable)
        {
        /* allocate new array */
        STATUS status;

        newsize = tcol + 1;

        if(NULL == (newblock = al_ptr_alloc_elem(COLENTRY, (S32) newsize, &status)))
            return(reperr_null(status));

        /* de-register old table */
        deregcoltab();

        /* copy across old table into the new one */
        if(colstart)
            memcpy32(newblock, colstart, sizeof32(COLENTRY) * numcol);

        /* must re-register copied info before freeing old colstart
         * as the free may move the objects pointed to by the list blocks
        */
        regtempcoltab(newblock, numcol);

        al_ptr_dispose((void **) &colstart);

        colsintable = newsize;
        colstart    = newblock;
        }

    trace_2(TRACE_APP_PD4, "createcol *, numcol: %d, colsintable: %d",
            numcol, colsintable);

    /* clear out new entries */
    for(y = numcol, nc = (colstart + numcol); y < colsintable; y++, nc++)
        {
        default_col_entries(nc);

        /* created at the end of the sheet, so linked-bit should be clear */

        if(y < tcol + 1)
            list_register(&nc->lb);
        }

    numcol = tcol + 1;

    trace_2(TRACE_APP_PD4, "createcol **, numcol: %d, colsintable: %d",
            numcol, colsintable);

    return(TRUE);
}

/******************************************************************************
*
* create a filler slot in the structure
*
******************************************************************************/

extern S32
createhole(
    COL col,
    ROW row)
{
    P_LIST_BLOCK lp;
    S32 res;

    /* make sure there is a column */
    if((col >= numcol)  &&  !createcol(col))
        return(FALSE);

    lp = indexcollb(col);

    if(list_createitem(lp, row, 0, FALSE) == NULL)
        res = FALSE;
    else
        res = TRUE;

    if(list_numitem(lp) > numrow)
        numrow = (ROW) list_numitem(lp);

    return(res);
}

/******************************************************************************
*
* Create a slot for a particular column and row of a given size and type.
*
* the size must include any extra space required apart from the slot
* overhead itself: a size of 0 means a blank slot; a size of 1 leaves space
* for a delimiter.
*
* in the case of a number slot for the new evaluator, NUMBEROVH includes
* all of PD's overhead, so size must be the space required by the
* evaluator: ie the number returned by ev_len()
*
******************************************************************************/

extern P_SLOT
createslot(
    COL col,
    ROW row,
    S32 size,
    uchar type)
{
    P_LIST_ITEM it;
    P_SLOT sl;
    P_LIST_BLOCK lp;

    trace_4(TRACE_APP_PD4, "createslot(%d, %d; contents size %d, type %d)", col, row, size, type);

    /* make sure there is a column */
    if(col >= numcol && !createcol(col))
        return(NULL);

    switch(type)
        {
        case SL_NUMBER:
            size += SL_NUMBEROVH;
            break;
        case SL_TEXT:
            size += SL_TEXTOVH;
            break;
        case SL_PAGE:
            size += SL_PAGEOVH;
            break;
        }

    trace_1(TRACE_APP_PD4, "createslot full size %d", size);

    sl = NULL;
    lp = indexcollb(col);
    trace_3(TRACE_APP_PD4, "calling list_createitem(&%p, %d, %d, FALSE)", report_ptr_cast(lp), row, size);
    if((it = list_createitem(lp, row, size, FALSE)) != NULL)
        {
        sl = slot_contents(it);

        /* set type of final slot */
        sl->type = type;

        sl->flags = 0;
        sl->format = 0;
        sl->justify = J_FREE;

        sl->content.text[0] = '\0';

        if(type == SL_NUMBER)
            {
            P_EV_RESULT resp;

            result_extract(sl, &resp);
            resp->did_num = RPN_DAT_WORD8;
            resp->arg.integer = 0;
            }
        }

    if(list_numitem(lp) > numrow)
        numrow = (ROW) list_numitem(lp);

    return(sl);
}

/******************************************************************************
*
* set column entries to default values
*
******************************************************************************/

static void
default_col_entries(
    P_COLENTRY cp)
{
    list_init(&cp->lb, MAXITEMSIZE, MAXPOOLSIZE);
    cp->colwidth  = DEFAULTWIDTH;
    cp->wrapwidth = DEFAULTWRAPWIDTH;
    cp->colflags  = DEFAULTCOLFLAGS;    /* probably 0, ie not a linked column */
}

/******************************************************************************
*
* delete all the slots in a set of columns
*
******************************************************************************/

extern void
delcol(
    COL tcol,
    COL size)
{
    COL first_col;

    size = MIN(size, numcol - tcol);

    if(size <= 0)
        return;

    first_col = tcol;
    tcol += size;

    while(--tcol >= first_col)
        {
        P_LIST_BLOCK lp;

        if((lp = indexcollb(tcol)) != NULL)
            {
            P_LIST_ITEM it;
            LIST_ITEMNO item;

            item = 0;

            it = list_initseq(lp, &item);
            while(it)
                {
                #if TRACE_ALLOWED
                LIST_ITEMNO olditem;

                olditem = item;
                #endif

                slot_free_resources(slot_contents(it));
                it = list_nextseq(lp, &item);

                #if TRACE_ALLOWED
                if(olditem == item)
                    olditem = 0;
                #endif
                }
            list_free(lp);
            }
        }
}

/******************************************************************************
*
* delete all the slots and entries in a set of column entries, closing up the table
*
******************************************************************************/

extern void
delcolandentry(
    COL tcol,
    COL size)
{
    delcol(tcol, size);
    delcolentry(tcol, size);
}

/******************************************************************************
*
* delete a set of column entries, closing up the table
*
******************************************************************************/

extern void
delcolentry(
    COL tcol,
    COL size)
{
    size = MIN(size, numcol - tcol);

    if(size <= 0)
        return;

    /* remove references to table */
    deregcoltab();

    /* close up table */
    memmove32(colstart + tcol,
              colstart + tcol + size,
              (numcol - (tcol + size)) * sizeof32(COLENTRY));

    minusab(numcol, size);

    /* reregister table */
    regcoltab();
}

/******************************************************************************
*
* delete all columns and free memory
* associated with a column array
*
******************************************************************************/

extern void
delcolstart(
    P_COLENTRY tcolstart,
    COL size)
{
    if((NULL != tcolstart) && (0 != size))
    {
        P_COLENTRY cp = tcolstart + size;

        while(--cp >= tcolstart)
            list_free(&cp->lb);

        deregtempcoltab(tcolstart, size);

        al_ptr_free(tcolstart);
    }
}

/******************************************************************************
*
*  deregister all the columns in the column table
*
******************************************************************************/

extern void
deregcoltab(void)
{
    deregtempcoltab(colstart, numcol);
}

/******************************************************************************
*
* deregister all columns in temporary column table
*
******************************************************************************/

extern void
deregtempcoltab(
    P_COLENTRY tcolstart,
    COL size)
{
    if((NULL != tcolstart) && (0 != size))
    {
        P_COLENTRY cp = tcolstart + size;

        while(--cp >= tcolstart)
            list_deregister(&cp->lb);
    }
}

/******************************************************************************
*
*  distribute a given wrap width among columns to right until all used up
*
******************************************************************************/

extern void
dstwrp(
    COL tcol,
    coord width)
{
    P_COLENTRY colinfo;

    while((tcol < numcol) && (width >= 0) && (!colislinked(tcol)))
        {
        colinfo = indexcol(tcol);
        colinfo->wrapwidth = width;
        width -= colinfo->colwidth;
        ++tcol;
        }

    xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
    filealtered(TRUE);
}

/******************************************************************************
*
* garbage collect in the sparse matrix
*
******************************************************************************/

extern void
garbagecollect(void)
{
    COL tcol = numcol;

    while(--tcol >= 0)
        while(list_garbagecollect(indexcollb(tcol)))
            ;
}

/******************************************************************************
*
* insert column entry in table
* returns success value
*
******************************************************************************/

extern BOOL
inscolentry(
    COL tcol)
{
    if(!createcol(numcol))
        return(FALSE);

    deregcoltab();

    /* note that createcol increments numcol */
    memmove32(colstart + tcol + 1,
              colstart + tcol,
              (numcol - (tcol + 1)) * sizeof32(COLENTRY));

    default_col_entries(colstart + tcol);

    regcoltab();

    /* if column(tcol-1) is linked, we've inserted this column within a range of linked columns */
    /* so make our new column linked, and recalulate right margins                              */
    if((tcol > 0) && (linked_column_linkright(tcol -1)))
        {
        P_S32 widp, wwidp, colflagsp;

        readpcolvars_(tcol, &widp, &wwidp, &colflagsp);

        *colflagsp |= LINKED_COLUMN_LINKRIGHT;
        adjust_all_linked_columns();    /* would adjust_this_linked_column(tcol +1 ) work???*/
        }

    return(TRUE);
}

/******************************************************************************
*
* insert a new blank slot at this position
* return TRUE if slot inserted or no need to because after end of column
* return FALSE if no room
*
******************************************************************************/

static STATUS
insertslot(
    _InVal_     DOCNO docno,
    COL col,
    ROW row)
{
    P_LIST_BLOCK lp;
    P_DOCU p_docu;

    trace_3(TRACE_APP_PD4, "insertslot: %d, %d, %d", docno, col, row);

    /*assert(docno < DOCNO_MAX);*/
    p_docu = p_docu_from_docno(docno);
    assert(NO_DOCUMENT != p_docu);
    assert(!docu_is_thunk(p_docu));

    lp = x_indexcollb(p_docu, col);

    status_return(list_insertitems(lp, row, (ROW) 1));

    if(list_numitem(lp) > numrow)
        numrow = (ROW) list_numitem(lp);

    filealtered(TRUE);

    return(STATUS_DONE);
}

extern BOOL
insertslotat(
    COL col,
    ROW row)
{
    S32 res = insertslot(current_docno(), col, row);

    if(res < 0)
        return(reperr_null(res));

    return(TRUE);
}

/******************************************************************************
*
* kill column table
*
******************************************************************************/

extern void
killcoltab(void)
{
    deregcoltab();

    al_ptr_dispose((void **) &colstart);

    numcol = colsintable = 0;
    colstart = NULL;
}

/******************************************************************************
*
* delete a slot
*
******************************************************************************/

extern void
killslot(
    COL col,
    ROW row)
{
    P_LIST_BLOCK lp;

    slot_free_resources(travel(col, row));

    lp = indexcollb(col);

    if(row < list_numitem(lp))
        list_deleteitems(lp, row, (ROW) 1);
}

/******************************************************************************
*
* speedy next_in_block for sparse matrix
* returns next slot address
*
******************************************************************************/

extern P_SLOT
next_slot_in_block(
    BOOL direction)
{
    static P_LIST_ITEM it;
    LIST_ITEMNO in_block_item;
    P_SLOT sl;
    P_LIST_BLOCK lp;
    BOOL was_start;

    trace_3(TRACE_APP_PD4, "next_slot_in_block(DOWN_COLUMNS = %s): in_block (%d, %d)", trace_boolstring(direction == DOWN_COLUMNS), in_block.col, in_block.row);

    was_start = start_block;
    if(was_start)
        {
        trace_0(TRACE_APP_PD4, "next_slot_in_block: starting block so set it := NULL");
        start_block = FALSE;
        it = NULL;
        }

    if(end_bl.col == NO_COL)
        {
        /* if zero or one markers, only return one slot */
        sl = was_start ? travel(in_block.col, in_block.row) : NULL;
        trace_1(TRACE_APP_PD4, "next_slot_in_block(ONE_SLOT_ONLY) returns &%p", report_ptr_cast(sl));
        return(sl);
        }

    /* this is constant while going loopy either way */
    in_block_item = (LIST_ITEMNO) in_block.row;

    if(direction == DOWN_COLUMNS)
        {
        /* loop to find a slot, going onto next column whenever necessary */
        do  {
            lp = indexcollb(in_block.col);

            if(!it)
                {
                in_block_item = (LIST_ITEMNO) start_bl.row;

                trace_2(TRACE_APP_PD4, "calling list_initseq from position (%d, %d)", in_block.col, in_block_item);
                it = list_initseq(lp, &in_block_item);
                trace_4(TRACE_APP_PD4, "new column %d started, list_initseq returned &%p and row %d: end_bl.row = %d", in_block.col, report_ptr_cast(it), in_block_item, end_bl.row);

                while(it  &&  (in_block_item < start_bl.row))
                    {
                    trace_0(TRACE_APP_PD4, "slot found before start of block");
                    it = list_nextseq(lp, &in_block_item);
                    }
                }
            else
                it = list_nextseq(lp, &in_block_item);

            if(it)
                {
                if(in_block_item <= end_bl.row)
                    {
                    sl = slot_contents(it);
                    break;
                    }

                it = NULL;

                trace_0(TRACE_APP_PD4, "beyond end row --- try a new column");
                }
            else
                trace_0(TRACE_APP_PD4, "no slot found --- try a new column");

            if(++in_block.col > end_bl.col)
                {
                trace_0(TRACE_APP_PD4, "next_slot_in_block ran out of columns: returning NULL");
                sl = NULL;
                break;
                }
            }
        while(TRUE);
        }
    else
        {
        do  {
            /* if an item has already been read on this row step to next column */
            if(!it)
                in_block.col = start_bl.col;
            else
                ++in_block.col;

            sl = travel(in_block.col, in_block.row);

            /* abuse of it */
            it = (P_LIST_ITEM) 1;

            if(sl)
                {
                trace_1(TRACE_APP_PD4, "next_slot_in_block(ACROSS_ROWS) returns &%p", report_ptr_cast(sl));
                break;
                }

            if(in_block.col >= end_bl.col)
                {
                /* at rhs so reset */
                trace_0(TRACE_APP_PD4, "next_slot_in_block at end column --- try a new row");

                /* a new row */
                it = NULL;

                if(++in_block_item > end_bl.row)
                    {
                    trace_0(TRACE_APP_PD4, "next_slot_in_block(ACROSS_ROWS) ran out of rows: returning NULL");
                    sl = NULL;
                    break;
                    }
                }
            }
        while(TRUE);
        }

    trace_1(TRACE_APP_PD4, "next_slot_in_block returns &%p", report_ptr_cast(sl));

    in_block.row = (ROW) in_block_item;
    return(sl);
}

/******************************************************************************
*
* SKS after 4.11 13jan92 derives from next_slot_in_block
*
* ONLY USE ONE OF traverse_block_next and traverse_block_next_slot
*
******************************************************************************/

extern P_SLOT
traverse_block_next_slot(
    TRAVERSE_BLOCKP blk /*inout*/)
{
    LIST_ITEMNO item;
    P_SLOT       sl;
    P_LIST_BLOCK      lp;
    BOOL        was_start;

    trace_4(TRACE_APP_PD4, "traverse_block_next_slot(&%p): blk->cur (%d, %d) %s",
            report_ptr_cast(blk), blk->cur.col, blk->cur.row,
            ((blk->direction == TRAVERSE_DOWN_COLUMNS)
                             ? "TRAVERSE_DOWN_COLUMNS"
                             : "TRAVERSE_ACROSS_ROWS"));

    was_start = blk->start;
    if(was_start)
        {
        trace_0(TRACE_APP_PD4, "traverse_block_next_slot: starting block so set blk->it := NULL");
        blk->start = FALSE;
        blk->it    = NULL;
        }

    if(blk->end.col == NO_COL)
        {
        /* if zero or one markers, only return one slot */
        sl = was_start ? traverse_block_travel(blk) : NULL;
        trace_1(TRACE_APP_PD4, "traverse_block_next_slot: ONE_SLOT_ONLY returns &%p", report_ptr_cast(sl));
        return(sl);
        }

    /* this is constant while going loopy either way */
    item = (LIST_ITEMNO) blk->cur.row;

    if(blk->direction == TRAVERSE_DOWN_COLUMNS)
        {
        /* loop to find a slot, going onto next column whenever necessary */
        do  {
            P_DOCU p_docu = blk->p_docu;

            lp = x_indexcollb(p_docu, blk->cur.col);

            if(!blk->it)
                {
                item = (LIST_ITEMNO) blk->stt.row;

                trace_2(TRACE_APP_PD4, "calling list_initseq from position (%d, %d)", blk->cur.col, item);
                blk->it = list_initseq(lp, &item);
                trace_4(TRACE_APP_PD4, "new column %d started, list_initseq returned &%p and row %d: blk->end.row = %d",
                        blk->cur.col, report_ptr_cast(blk->it), item, blk->end.row);

                while(blk->it  &&  (item < blk->stt.row))
                    {
                    trace_0(TRACE_APP_PD4, "slot found before start of block");
                    blk->it = list_nextseq(lp, &item);
                    }
                }
            else
                blk->it = list_nextseq(lp, &item);

            if(blk->it)
                {
                if(item <= blk->end.row)
                    {
                    sl = slot_contents(blk->it);
                    break;
                    }

                blk->it = NULL;

                trace_0(TRACE_APP_PD4, "beyond end row --- try a new column");
                }
            else
                trace_0(TRACE_APP_PD4, "no slot found --- try a new column");

            if(++blk->cur.col > blk->end.col)
                {
                trace_0(TRACE_APP_PD4, "traverse_block_next_slot ran out of columns: returning NULL");
                sl = NULL;
                break;
                }
            }
        while(TRUE);
        }
    else
        {
        do  {
            /* if an item has already been read on this row step to next column */
            if(!blk->it)
                blk->cur.col = blk->stt.col;
            else
                ++blk->cur.col;

            sl = traverse_block_travel(blk);

            /* abuse of blk->it */
            blk->it = (P_LIST_ITEM) 1;

            if(sl)
                {
                trace_1(TRACE_APP_PD4, "traverse_block_next_slot(ACROSS_ROWS) returns &%p", report_ptr_cast(sl));
                break;
                }

            if(blk->cur.col >= blk->end.col)
                {
                /* at rhs so reset */
                trace_0(TRACE_APP_PD4, "traverse_block_next_slot at end column --- try a new row");

                /* a new row */
                blk->it = NULL;

                if(++item > blk->end.row)
                    {
                    trace_0(TRACE_APP_PD4, "traverse_block_next_slot(ACROSS_ROWS) ran out of rows: returning NULL");
                    sl = NULL;
                    break;
                    }
                }
            }
        while(TRUE);
        }

    trace_1(TRACE_APP_PD4, "traverse_block_next_slot returns &%p", report_ptr_cast(sl));

    blk->cur.row = (ROW) item;
    return(sl);
}

/******************************************************************************
*
* pack a column, releasing any free space
*
******************************************************************************/

extern void
pack_column(
    COL col)
{
    list_packlist(indexcollb(col), -1);
}

/******************************************************************************
*
* read pointers to column variables
*
******************************************************************************/

extern void
readpcolvars(
    _InVal_     COL col,
    _OutRef_    P_P_S32 widp,
    _OutRef_    P_P_S32 wwidp)
{
    P_COLENTRY cp;

    cp = indexcol(col);
    assert(cp);
    *widp = &cp->colwidth;
    *wwidp = &cp->wrapwidth;
}

/******************************************************************************
*
* read pointers to column variables
*
******************************************************************************/

extern void
readpcolvars_(
    _InVal_     COL col,
    _OutRef_    P_P_S32 widp,
    _OutRef_    P_P_S32 wwidp,
    _OutRef_    P_P_S32 colflagsp)
{
    P_COLENTRY cp;

    cp         = indexcol(col);
    assert(cp);
    *widp      = &cp->colwidth;
    *wwidp     = &cp->wrapwidth;
    *colflagsp = &cp->colflags;
}

/******************************************************************************
*
* register all the columns in the column table
*
******************************************************************************/

extern void
regcoltab(void)
{
    regtempcoltab(colstart, numcol);
}

/******************************************************************************
*
* register all the columns in temporary column table
*
******************************************************************************/

extern void
regtempcoltab(
    P_COLENTRY tcolstart,
    COL size)
{
    COL i;
    P_COLENTRY cp;

    for(i = 0, cp = tcolstart; i < size; ++i, ++cp)
        list_register(&cp->lb);
}

/******************************************************************************
*
* rebuild numrow because of insertions or deletions
*
******************************************************************************/

extern void
reset_numrow(void)
{
    COL col;
    ROW maxrow;

    for(col = 0, maxrow = 0; col < numcol; col++)
        {
        ROW n = (ROW) list_numitem(indexcollb(col));
        maxrow = MAX(maxrow, n);
        }

    numrow = maxrow ? maxrow : 1;
}

/******************************************************************************
*
* restore column table
*
******************************************************************************/

extern BOOL
restcoltab(void)
{
    P_COLENTRY d_colstart = def_colstart;
    COL d_numcol = def_numcol;

    if(!d_colstart)
        return(TRUE);

    if(!createcol(d_numcol - 1))
        return(FALSE);

    deregcoltab();

    memcpy32(colstart, d_colstart, sizeof32(COLENTRY) * d_numcol);

    regcoltab();

    return(TRUE);
}

/******************************************************************************
*
* save column table
*
******************************************************************************/

extern void
savecoltab(void)
{
    deregcoltab();

    def_numcol = numcol;
    def_colstart = colstart;

    colstart = NULL;
}

/******************************************************************************
*
* tell people that ought to know that slot has changed
*
******************************************************************************/

extern void
slot_changed(
    COL col,
    ROW row)
{
    if(!in_load)
        {
        UREF_PARM urefb;

        /* first, free all tree entries owned by this slot */
        set_ev_slr(&urefb.slr1,     col,     row);
        set_ev_slr(&urefb.slr2, col + 1, row + 1);
        urefb.action = UREF_CHANGE;

        ev_uref(&urefb);

        ev_todo_add_dependents(&urefb.slr1);
        }
}

/******************************************************************************
*
* free resources owned by slot
* (currently only evaluator slots
* can own resources
*
******************************************************************************/

extern void
slot_free_resources(
    P_SLOT tslot)
{
    if(tslot && tslot->type == SL_NUMBER)
        ev_exp_free_resources(&tslot->content.number.guts);
}

/******************************************************************************
*
* a slot is about to be replaced
* (probably with a createslot or createhole)
* tslot can be NULL
*
******************************************************************************/

extern void
slot_to_be_replaced(
    COL col,
    ROW row,
    P_SLOT tslot)
{
    UREF_PARM urefb;

    if(tslot && !in_load)
        {
        /* first, free all tree entries owned by this slot */
        set_ev_slr(&urefb.slr1,     col,     row);
        set_ev_slr(&urefb.slr2, col + 1, row + 1);
        urefb.action = UREF_REPLACE;

        ev_uref(&urefb);

        /* second, free any resources owned by slot */
        slot_free_resources(tslot);
        }
}

/******************************************************************************
*
* return size of contents of slot in bytes, including necessary termination
*
******************************************************************************/

extern S32
slotcontentssize(
    P_SLOT tslot)
{
    switch(tslot->type)
        {
        case SL_TEXT:
            return(compiled_text_len(tslot->content.text)); /* includes NULLCH */

        case SL_NUMBER:
            return(ev_len(&tslot->content.number.guts));

        case SL_PAGE:
            return(0);

        default:
            return(0);
        }
}

/******************************************************************************
*
* return size of slot in bytes, including any overhead
*
******************************************************************************/

extern S32
slotsize(
    P_SLOT tslot)
{
    switch(tslot->type)
        {
        case SL_TEXT:
            return(SL_TEXTOVH +
                   compiled_text_len(tslot->content.text)); /* includes NULLCH */

        case SL_NUMBER:
            return(SL_NUMBEROVH +
                   ev_len(&tslot->content.number.guts));

        case SL_PAGE:
            return(SL_PAGEOVH);

        default:
            return(SL_SLOTOVH);
        }
}

/******************************************************************************
*
* update references when rows are exchanged in sort
* updated for external references 30.5.89 MRJC
*
******************************************************************************/

static void
sortrefs(
    ROW trow1,
    ROW trow2,
    COL firstcol,
    COL lastcol)
{
    UREF_PARM urefb;

    /* set up uref block */
    set_ev_slr(&urefb.slr1,    firstcol, trow1);
    set_ev_slr(&urefb.slr2, lastcol + 1, trow2);

    urefb.action = UREF_SWAP;

    ev_uref(&urefb);
}

/******************************************************************************
*
* swap the rows, can assume slots exist in all columns
* trow1 is guaranteed to be higher up than trow2
*
******************************************************************************/

extern BOOL
swap_rows(
    ROW trow1,
    ROW trow2,
    COL firstcol,
    COL lastcol,
    BOOL updaterefs)
{
    COL col;
    char tempslot[MAX_SLOTSIZE];
    swap_slot_struct s1, s2;

    IGNOREPARM(updaterefs);

    trace_2(TRACE_APP_PD4, "swap rows: %d, %d", (S32) trow1, (S32) trow2);

    s1.row = trow1;
    s2.row = trow2;

    for(col = firstcol; col <= lastcol; ++col)
        {
        trace_1(TRACE_APP_PD4, "swap_rows: col %d", col);

        s1.col = col;
        s2.col = col;

        s1.sl = travel(col, s1.row);
        if(s1.sl)
            {
            s1.size = slotsize(s1.sl);
            s1.type = s1.sl->type;
            trace_3(TRACE_APP_PD4, "swap_rows moved: %d, %d, type %d", col, s1.row, s1.type);
            }
        else
            s1.size = 0;

        s2.sl = travel(col, s2.row);
        if(s2.sl)
            {
            s2.size = slotsize(s2.sl);
            s2.type = s2.sl->type;
            trace_3(TRACE_APP_PD4, "swap_rows moved: %d, %d, type %d", col, s2.row, s2.type);
            }
        else
            s2.size = 0;

        /* swapo the slots */
        if(s1.size > s2.size)
            {
            trace_2(TRACE_APP_PD4, "swap_rows: size1 %d > size2 %d", s1.size, s2.size);
            if(!swap_slot_core(&s1, &s2, tempslot))
                return(FALSE);
            }
        else if(s1.size < s2.size)
            {
            trace_2(TRACE_APP_PD4, "swap_rows: size1 %d < size2 %d", s1.size, s2.size);
            if(!swap_slot_core(&s2, &s1, tempslot))
                return(FALSE);
            }
        /* same size, non-zero? */
        else if(s1.size)
            {
            trace_1(TRACE_APP_PD4, "swap_rows: same size %d", s1.size);
            memcpy32(tempslot, s2.sl,    s2.size);
            memcpy32(s2.sl,    s1.sl,    s1.size);
            memcpy32(s1.sl,    tempslot, s2.size);
            }
        }

    trace_0(TRACE_APP_PD4, "swap_rows: update references");
    sortrefs(s1.row, s2.row, firstcol, lastcol);

    trace_0(TRACE_APP_PD4, "swap_rows out");

    return(TRUE);
}

/******************************************************************************
*
* swap just a pair of slots
* trow1 is guaranteed to be higher up than trow2
*
******************************************************************************/

extern BOOL
swap_slots(
    COL tcol1,
    ROW trow1,
    COL tcol2,
    ROW trow2)
{
    char tempslot[MAX_SLOTSIZE];
    swap_slot_struct s1, s2;
    UREF_PARM urefb;

    trace_4(TRACE_APP_PD4, "swap slots: %d, %d, %d, %d", (S32) tcol1, (S32) trow1, (S32) tcol2, (S32) trow2);

    s1.row = trow1;
    s1.col = tcol1;

    s2.row = trow2;
    s2.col = tcol2;

        s1.sl = travel(s1.col, s1.row);
        if(s1.sl)
            {
            s1.size = slotsize(s1.sl);
            s1.type = s1.sl->type;
            }
        else
            s1.size = 0;

        s2.sl = travel(s2.col, s2.row);
        if(s2.sl)
            {
            s2.size = slotsize(s2.sl);
            s2.type = s2.sl->type;
            }
        else
            s2.size = 0;

        /* swapo the slots */
        if(s1.size > s2.size)
            {
            if(!swap_slot_core(&s1, &s2, tempslot))
                return(FALSE);
            }
        else if(s1.size < s2.size)
            {
            if(!swap_slot_core(&s2, &s1, tempslot))
                return(FALSE);
            }
        /* same size, non-zero? */
        else if(s1.size)
            {
            memcpy32(tempslot, s2.sl,    s2.size);
            memcpy32(s2.sl,    s1.sl,    s1.size);
            memcpy32(s1.sl,    tempslot, s2.size);
            }

    /* set up uref block */
    set_ev_slr(&urefb.slr1, s1.col, s1.row);
    set_ev_slr(&urefb.slr2, s2.col, s2.row);
    urefb.action = UREF_SWAPSLOT;

    ev_uref(&urefb);

    trace_0(TRACE_APP_PD4, "swap_slots out");

    return(TRUE);
}

/******************************************************************************
*
* helper routine for swap rows - to swap two slots
*
******************************************************************************/

static S32
swap_slot_core(
    swap_slot_struct *p1,
    swap_slot_struct *p2,
    P_ANY tempslot)
{
    /* copy p2 to temp slot */
    if(p2->size)
        memcpy32(tempslot, p2->sl, p2->size);

    /* create slot at p2 for p1 */
    if((p2->sl = createslot(p2->col, p2->row, p1->size + 4, p1->type)) == NULL)
        return(FALSE);

    p1->sl = travel(p1->col, p1->row);

    /* copy from p1 to p2 */
    memcpy32(p2->sl, p1->sl, p1->size);

    /* if old p2 blank, make hole at p1 */
    if(!p2->size)
        return(createhole(p1->col, p1->row));

    /* otherwise copy p2 to p1 */
    if((p1->sl = createslot(p1->col, p1->row, p2->size + 4, p2->type)) == NULL)
        return(FALSE);

    memcpy32(p1->sl, tempslot, p2->size);

    return(TRUE);
}

/******************************************************************************
*
* travel to a particular row, column
*
* returns pointer to slot if it exists.
* Returns NULL if column doesn't exist
* Returns NULL if column does exist, but row doesn't.  In this case, new
* position stored in column vector.
*
* the function atrow() must then be called to return the actual
* row that was achieved
*
******************************************************************************/

extern P_SLOT
travel(
    COL col,
    ROW row)
{
    P_LIST_ITEM it;
    P_SLOT sl = NULL;

    if(NULL != (it = list_gotoitem(indexcollb(col), row)))
        sl = slot_contents(it);

    trace_3(TRACE_APP_PD4, "travel(%d, %d) to slot &%p", col, row, report_ptr_cast(sl));

    return(sl);
}

extern P_SLOT
travel_here(void)
{
    return(travel(curcol, currow));
}

extern P_SLOT
travel_externally(
    _InVal_     DOCNO docno,
    COL col,
    ROW row)
{
    P_DOCU p_docu;
    P_LIST_ITEM it;
    P_SLOT sl = NULL;

    if((NO_DOCUMENT == (p_docu = p_docu_from_docno(docno)))  ||  docu_is_thunk(p_docu))
    {
        assert(DOCNO_NONE != docno);
        assert(docno < DOCNO_MAX);
        assert(NO_DOCUMENT != p_docu);
        assert((NO_DOCUMENT != p_docu) && docu_is_thunk(p_docu));
        return(NULL);
    }

    if(NULL != (it = list_gotoitem(x_indexcollb(p_docu, col), row)))
        sl = slot_contents(it);

    return(sl);
}

extern P_SLOT
traverse_block_travel(
    TRAVERSE_BLOCKP blk /*const*/)
{
    P_DOCU p_docu = blk->p_docu;
    P_LIST_ITEM it;
    P_SLOT sl = NULL;

    assert(NO_DOCUMENT != p_docu);
    assert((NO_DOCUMENT != p_docu) && docu_is_thunk(p_docu));

    if(NULL != (it = list_gotoitem(x_indexcollb(p_docu, blk->cur.col), blk->cur.row)))
        sl = slot_contents(it);

    return(sl);
}

/******************************************************************************
*
* called when an insert of a slot into
* the tree fails. this removes all entries
* for this slot in the tree, and blats the slot too
*
******************************************************************************/

extern void
tree_insert_fail(
    _InRef_     PC_EV_SLR slrp)
{
    UREF_PARM urefb;

    urefb.slr1 = urefb.slr2 = *slrp;
    urefb.slr2.col += 1;
    urefb.slr2.row += 1;

    urefb.action = UREF_DELETE;
    ev_uref(&urefb);

    createhole((COL) slrp->col, (ROW) slrp->row);
}

/******************************************************************************
*
* see if a slot reference (SLR) needs updating
*
******************************************************************************/

static void
update_marks(
    _InVal_     DOCNO docno,
    P_SLR slot,
    COL mrksco,
    COL mrkeco,
    ROW mrksro,
    ROW mrkero,
    COL coldiff,
    ROW rowdiff)
{
    if(docno != current_docno())
        return;

    if( slot->col <= mrkeco  &&  slot->col >= mrksco  &&
        slot->row <= mrkero  &&  slot->row >= mrksro)
        {
        plusab(slot->row, rowdiff);
        plusab(slot->col, coldiff);
        }

    if((slot->row > numrow-1)  &&  (slot->row < LARGEST_ROW_POSSIBLE))
        slot->row = numrow-1;

    if((slot->col > numcol-1)  &&  (slot->col < LARGEST_COL_POSSIBLE))
        slot->col = numcol-1;
}

/******************************************************************************
*
* update references in spreadsheet for a move
* updated for external refs MRJC 30.5.89
* updated for draw files MRJC 28.6.89
* updated for new evaluator MRJC 5.8.91
* RJM put in reference to end_bl 26.8.91
*
* NOTE:
* latest updref expects physical operation done
* BEFORE calling updref
*
******************************************************************************/

extern void
updref(
    COL mrksco,
    ROW mrksro,
    COL mrkeco,
    ROW mrkero,
    COL coldiff,
    ROW rowdiff,
    S32 action,
    _InVal_     DOCNO docno_to)
{
    UREF_PARM urefb;

    set_ev_slr(&urefb.slr1, (mrksco  & COLNOBITS),
                            (mrksro  & ROWNOBITS));
    set_ev_slr(&urefb.slr2, (mrkeco  & COLNOBITS) + 1,
                            (mrkero  & ROWNOBITS) + 1);
    set_ev_slr(&urefb.slr3, coldiff < 0 ? coldiff : (coldiff & COLNOBITS),
                            rowdiff < 0 ? rowdiff : (rowdiff & ROWNOBITS));

    /* check for move into a new document */
    if(docno_to != DOCNO_NONE)
        urefb.slr3.docno = docno_to;

    urefb.action = action;

    ev_uref(&urefb);

    update_marks(blk_docno,  &blkanchor,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);
    update_marks(blk_docno,  &blkstart,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);
    update_marks(blk_docno,  &blkend,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);
    update_marks(blk_docno,  &end_bl,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);

    update_marks(sch_docno,  &sch_stt,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);
    update_marks(sch_docno,  &sch_end,
                 mrksco, mrkeco, mrksro, mrkero, coldiff, rowdiff);
}

/******************************************************************************
*
* call uref with a PipeDream block
*
******************************************************************************/

extern void
uref_block(
    S32 action)
{
    UREF_PARM urefb;

    if(start_bl.col != NO_COL)
        {
        set_ev_slr(&urefb.slr1,   blkstart.col,     blkstart.row);
        set_ev_slr(&urefb.slr2, blkend.col + 1,   blkend.row + 1);
        }
    else
        {
        set_ev_slr(&urefb.slr1, curcol    , currow);
        set_ev_slr(&urefb.slr2, curcol + 1, currow + 1);
        }

    urefb.action = action;

    ev_uref(&urefb);
}

/* end of slot.c */
