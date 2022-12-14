/* execs.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that does lots of backsplash commands */

/* RJM August 1987 */

/*#define RJM*/

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"
#include "ridialog.h"

/*
internal functions
*/

static void
alnsto(
    uchar justify,
    uchar mask,
    uchar bits,
    BOOL fault_protection);

#define NO_JUSTIFY 0xFF

#define bad_col(tcol) (((tcol) < (COL) 0)  ||  ((tcol) & BADCOLBIT))
#define bad_row(trow) (((trow) < (ROW) 0)  ||  ((trow) & BADROWBIT))

extern BOOL
bad_reference(
    COL tcol,
    ROW trow)
{
    return((tcol == NO_COL)  ||  bad_col(tcol)  ||  bad_row(trow));
}

/******************************************************************************
*
*  left align
*
******************************************************************************/

extern void
LeftAlign_fn(void)
{
    alnsto(J_LEFT, (uchar) 0xFF, (uchar) 0, TRUE);
}

/******************************************************************************
*
* centre align
*
******************************************************************************/

extern void
CentreAlign_fn(void)
{
    /* if block marked, do each cell in block */
    alnsto(J_CENTRE, (uchar) 0xFF, (uchar) 0, TRUE);
}

/******************************************************************************
*
*  right align
*
******************************************************************************/

extern void
RightAlign_fn(void)
{
    alnsto(J_RIGHT, (uchar) 0xFF, (uchar) 0, TRUE);
}

/******************************************************************************
*
*  free align
*
******************************************************************************/

extern void
FreeAlign_fn(void)
{
    alnsto(J_FREE, (uchar) 0xFF, (uchar) 0, TRUE);
}

/******************************************************************************
*
* lcr align
*
******************************************************************************/

extern void
LCRAlign_fn(void)
{
    alnsto(J_LCR, (uchar) 0xFF, (uchar) 0, TRUE);
}

/******************************************************************************
*
* default format - set format to 0
*
******************************************************************************/

extern void
DefaultFormat_fn(void)
{
    alnsto(NO_JUSTIFY, 0, 0, TRUE);
}

/******************************************************************************
*
* leading character
* toggle lead char bit, set dcp bit
*
******************************************************************************/

extern void
LeadingCharacters_fn(void)
{
    alnsto(NO_JUSTIFY, 0xFF /* & (uchar) ~F_DCP */ ,  F_LDS, TRUE);
}

/******************************************************************************
*
* trailing character
* toggle lead char bit, set dcp bit
*
******************************************************************************/

extern void
TrailingCharacters_fn(void)
{
    alnsto(NO_JUSTIFY, 0xFF /* & (uchar) ~F_DCP */ ,  F_TRS, TRUE);
}

/******************************************************************************
*
* mark the area of a given block for output
* MRJC 13.7.89
*
******************************************************************************/

static void
mark_block_output(
    PC_SLR bs)
{
    S32 offset;

    if(bs->col == NO_COL)
    {
        out_currslot = TRUE;
        return;
    }

    if((offset = schrsc(bs->row)) != NOTFOUND)
        mark_to_end(offset);
    else
        out_screen = TRUE;
}

/******************************************************************************
*
* set a marked block or the current cell with a format
* the justify byte is put into the justify field
* in the cell (unless NO_JUSTIFY)
* mask & bits give the information to change the format
* some operations set bits, some clear, and some toggle bits
* this is done by masking out some bits and then exclusive-oring
* e.g. to clear  - mask with 0 and eor with 0
*      to set    - mask with 0 and eor with 1
*      to toggle - mask with 1 and eor with 1
*
******************************************************************************/

static void
alnsto_block(
    uchar justify,
    uchar mask,
    uchar bits,
    BOOL fault_protection,
    PC_SLR bs,
    PC_SLR be)
{
    P_CELL tcell;

    if(fault_protection  &&  protected_cell_in_range(bs, be))
        return;

    if(blkstart.col != NO_COL && !set_up_block(TRUE))
        return;

    init_block(bs, be);

    mark_block_output(&start_bl);

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        filealtered(TRUE);

        switch(justify)
        {
        case NO_JUSTIFY:
            break;

        case PROTECTED:
            /* set protected bit */
            tcell->justify |= PROTECTED;
            break;

        case J_BITS:
            /* clear protected bit */
            tcell->justify &= J_BITS;
            break;

        default:
            /* set justify bits, retaining protected status */
            tcell->justify = (tcell->justify & CLR_J_BITS) | justify;
            break;
        }

        switch(tcell->type)
        {
        case SL_NUMBER:
            tcell->format = (tcell->format & mask) ^ bits;
            break;

        default:
            break;
        }
    }
}

static void
alnsto(
    uchar justify,
    uchar mask,
    uchar bits,
    BOOL fault_protection)
{
    alnsto_block(justify, mask, bits, fault_protection, &blkstart, &blkend);
}

/******************************************************************************
*
* alnst1 is like alnsto
* for sign minus, sign bracket and set decimal places
* if the cell is currently using option page defaults then those
* defaults are copied into the cell
*
******************************************************************************/

static void
alnst1(
    uchar mask,
    uchar bits)
{
    P_CELL tcell;
    uchar format;

    if(protected_cell_in_range(&blkstart, &blkend))
        return;

    /* RJM adds this check on 15.10.91 */
    if(blkstart.col != NO_COL && !set_up_block(TRUE))
        return;

    init_marked_block();

    mark_block_output(&start_bl);

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        switch(tcell->type)
        {
        case SL_NUMBER:
            filealtered(TRUE);

            format = tcell->format;

            if((format & F_DCP) == 0)
            {
                format |= F_DCP;
                if(d_options_MB == 'B')
                    format |= F_BRAC;
                format &= ~F_DCPSID;
                format |= get_dec_field_from_opt();
            }

            tcell->format = (format & mask) ^ bits;
            break;

        default:
            break;
        }
    }
}

extern void
SignBrackets_fn(void)
{
    alnst1(0xFF & (uchar) ~F_BRAC, F_BRAC);
}

extern void
SignMinus_fn(void)
{
    alnst1(0xFF & (uchar) ~F_BRAC,  0);
}

extern void
DecimalPlaces_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DECIMAL))
    {
        alnst1((uchar) ~F_DCPSID,
               (d_decimal[0].option == 'F')
                   ? (uchar) 0xF
                   : d_decimal[0].option - '0');

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* do RETfunc, worrying about Insert on return status
*
******************************************************************************/

extern void
Return_fn(void)
{
    P_CELL tcell;
    uchar justify;

    /* check word perhaps */
    if(lecpos  &&  slot_in_buffer)
        check_word();

    if(d_options_IR == 'Y')
    {
        internal_process_command(N_SplitLine);

        /* remove justify bit from current cell */
        if( !xf_inexpression  &&
            ((tcell = travel_here()) != NULL))
        {
            /* shirley we don't have to worry about PROTECTED bit,
                cos if protected what are we doing poking cell?
            */
            justify = tcell->justify & J_BITS;
            if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
                /* set justify bits, retaining protected status */
                tcell->justify = (tcell->justify & CLR_J_BITS) | J_FREE;
        }
    }

    /* finish expression editing or at row if at end of text */

    Return_fn_core();
}

extern BOOL
Return_fn_core(void)
{
    if(!mergebuf())
        return(FALSE);

    lecpos = lescrl = 0;

    trace_2(TRACE_APP_PD4, "Return_fn currow: %d, numrow: %d", currow, numrow);

    if(currow + 1 >= numrow)
    {
        /* force blank cell in */
        if(!createhole(curcol, currow + 1))
            return(reperr_null(status_nomem()));

        out_rebuildvert = TRUE;
        filealtered(TRUE);
        mark_row(currowoffset + 1);
    }

    mark_row_praps(currowoffset, OLD_ROW);

    internal_process_command(N_CursorDown);

    return(TRUE);
}

/******************************************************************************
*
* convert string:
* converts \ to CMDLDI
* \\ to \
* |x to ctrl-x
* || to |
*
******************************************************************************/

extern void
prccml(
    uchar *buffer)
{
    uchar *from = buffer;
    uchar *to   = buffer;
    BOOL instring = FALSE;

    while(*from)
    {
        switch(*from)
        {
        case '|':
            if(instring)
            {
                if(from[1] == '\"')
                    from++;
                *to++ = *from++;
                break;
            }

            from++;

            if(*from == '|'  ||  *from == '\"')
                *to++ = *from;
            else if(isalpha(*from))
                *to++ = toupper(*from) - 'A' + 1;

            from++;
            break;

        case '\"':
            /* remove leading spaces before strings */
            if(!instring)
                while(to > buffer && *(to-1) == SPACE)
                    to--;

            instring = !instring;
            from++;
            /* and remove trailing spaces after strings */
            if(!instring)
                while(*from == SPACE)
                    from++;

            break;

        case '\\':
            if(!instring)
            {
                from++;
                if(*from == '\\')
                    *to++ = *from++;
                else
                    *to++ = CMDLDI;
                break;
            }

            /* deliberate fall through */

        default:
            *to++ = *from++;
            break;
        }
    }

    *to = CH_NULL;
}

/******************************************************************************
*
*  define key
*
******************************************************************************/

static BOOL
definekey_fn_core(void)
{
    uchar buffer[LIN_BUFSIZ];
    S32 key;

    buffer[0] = CH_NULL;

    if(NULL != d_defkey[1].textfield)
        strcpy((char *) buffer, d_defkey[1].textfield);

    prccml(buffer); /* convert funny characters in buffer */

    key = (S32) d_defkey[0].option;

    /* remove old definition */
    delete_from_list(&first_key, key);

    /* add new one */
    if(CH_NULL != buffer[0])
    {
        S32 res;
        if(status_fail(res = add_to_list(&first_key, key, buffer)))
            return(reperr_null(res));
    }

    return(TRUE);
}

extern void
DefineKey_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEFKEY)) /* so we can see prev defn'n */
    {
        if(!definekey_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  define function key
*
******************************************************************************/

static BOOL
definefunctionkey_fn_core(void)
{
    const KMAP_CODE kmap_code = func_key_list[d_def_fkey[0].option];
    uchar buffer[LIN_BUFSIZ];

    buffer[0] = CH_NULL;

    if(NULL != d_def_fkey[1].textfield)
        strcpy((char *) buffer, d_def_fkey[1].textfield);

    prccml(buffer); /* convert funny characters in buffer */

    /* remove old definition */
    delete_from_list(&first_key, (S32) kmap_code);

    /* add new definition */
    if(CH_NULL != buffer[0])
    {
        S32 res;
        if(status_fail(res = add_to_list(&first_key, (S32) kmap_code, buffer)))
            return(reperr_null(res));
    }

    return(TRUE);
}

extern void
DefineFunctionKey_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEF_FKEY))
    {
        if(!definefunctionkey_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  define command
*
******************************************************************************/

static int
definecommand_fn_core(void)
{
    char buffer[LIN_BUFSIZ];
    S32 key;
    char *src;
    char *dst;
    S32 ch;

    src = d_def_cmd[0].textfield;

    if((NULL == src)  ||  (strlen(src) > (sizeof(S32) * 8 / 5)))
    {
        bleep();
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    dst = buffer;
    key = 0;

    do  {
        ch = *src++;
        ch = toupper(ch);
        if((ch >= 'A')  &&  (ch <= 'Z'))
            key = (key << 5) + ((S32) ch - 'A');
        else
            ch = '_';

        *dst++ = ch;
    }
    while(ch != '_');

    /* remove old definition */
    delete_from_list(&first_command_redef, key);

    src = d_def_cmd[1].textfield;

    if(NULL != src)
    {
        do  {
            ch = *src++;
            *dst++ = toupper(ch);   /* including terminating CH_NULL */
        }
        while(CH_NULL != ch);

        if(!str_isblank(buffer))
        {   /* add new definition */
            S32 res;
            if(status_fail(res = add_to_list(&first_command_redef, key, buffer)))
                return(reperr_null(res));
        }
    }

    return(TRUE);
}

extern void
DefineCommand_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEF_CMD))
    {
        int core_res = definecommand_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* ESCAPE
*
******************************************************************************/

extern void
Escape_fn(void)
{
    ack_esc();
}

/******************************************************************************
*
* pause until key press
* length of pause is given by dialog box
* if 'p' pressed, pause until next key press
*
* should this routine understand and execute commands?
*
******************************************************************************/

extern void
Pause_fn(void)
{
    if(!dialog_box_start())
        return;

    if(!dialog_box(D_PAUSE))
        return;

    dialog_box_end();

    riscdialog_dopause(d_pause[0].option);
}

/******************************************************************************
*
* insert reference in editing line
*
******************************************************************************/

extern void
InsertReference_fn(void)
{
    U8 buffer[BUF_MAX_REFERENCE];

    if(!xf_inexpression && !xf_inexpression_box && !xf_inexpression_line)
    {
        bleep();
        return;
    }

    /* expand column then row into buffer */
    (void) write_ref(buffer, elemof32(buffer), current_docno(), curcol, currow);

    insert_string(buffer, FALSE);
}

extern void
mark_to_end(
    coord rowonscr)
{
    out_below = TRUE;

    if( rowonscr < 0)
        rowonscr = 0;

    if( rowtoend > rowonscr)
        rowtoend = rowonscr;
}

/******************************************************************************
*
* split line
*
******************************************************************************/

_Check_return_
static U32
xustrlen32(
    _In_reads_(elemof_buffer) PC_USTR buffer,
    _InVal_     U32 elemof_buffer)
{
    PC_USTR p = memchr(buffer, CH_NULL, elemof_buffer);
    assert(elemof_buffer);
    if(NULL == p)
        return(elemof_buffer - 1);
    return(PtrDiffBytesU32(p, buffer));
}

extern void
SplitLine_fn(void)
{
    uchar tempchar;
    P_CELL tcell;
    BOOL actually_splitting = FALSE;
    COL tcol;
    ROW trow;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    if(lecpos  &&  protected_cell(curcol, currow))
        return;

    tcell = travel_here();

    if(!tcell  ||  ((tcell->type == SL_TEXT)  &&  !is_protected_cell(tcell)))
    {
        U32 bufflength = xustrlen32(linbuf, elemof32(linbuf));
        U32 splitpoint = MIN(bufflength, (U32) lecpos);

        assert(splitpoint < elemof32(linbuf));
        tempchar = linbuf[splitpoint];
        linbuf[splitpoint] = CH_NULL;

        if( dspfld_from == -1)
            dspfld_from = splitpoint;

        actually_splitting = buffer_altered = slot_in_buffer = TRUE;

        if(!mergebuf())
            return;

        /* leave lying around for merging later on */
        linbuf[splitpoint] = tempchar;
        memmove32(linbuf, linbuf + splitpoint, (bufflength - splitpoint + 1));
    }

    /* if insert on wrap is row insert cells in this row for each column to the right,
     * and on the next row for each each column to the left, and for this one if we'll need
     * to move the rest of the line down
    */
    if(d_options_IW == 'R')
    {
        for(tcol = 0; tcol < numcol; ++tcol)
        {
            trow = currow;

            if(tcol < curcol)
                ++trow;
            else if((tcol == curcol)  &&  actually_splitting)
                ++trow;

            if(!insertslotat(tcol, trow))
            {
                /* remove those added */
                while(--tcol >= 0)
                {
                    trow = currow;
                    if(tcol < curcol)
                        ++trow;
                    else if((tcol == curcol)  &&  actually_splitting)
                        ++trow;

                    killslot(tcol, trow);
                }

                return;
            }
        }

        mark_to_end(currowoffset);
        out_rebuildvert = TRUE;

        reset_numrow();

        /* do one updref for the main part of the block,
         * and one updref for the block past the current column
         */
        if(!curcol)
            trow = currow;
        else
            trow = currow + 1;

        updref(0, trow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 0, 1, UREF_UREF, DOCNO_NONE);

        if(curcol)
            updref(curcol, currow, numcol-1, currow, 0, 1, UREF_UREF, DOCNO_NONE);

        if(been_error)
            return;
    }
    /* insert on wrap is column, insert in just this column */
    else
    {
        if(actually_splitting)
            currow++;

        internal_process_command(N_InsertRowInColumn);

        if(actually_splitting)
            currow--;
    }

    if(actually_splitting)
    {
        currow++;

        buffer_altered = slot_in_buffer = TRUE;

        if(!mergebuf_nocheck())
            return;

        currow--;
    }
}

/******************************************************************************
*
* join lines
*
******************************************************************************/

extern void
JoinLines_fn(void)
{
    P_CELL thisslot, nextslot, tcell;
    S32 thislen, nextlen;
    BOOL actually_joining = TRUE;
    uchar tempbuffer[LIN_BUFSIZ];
    BOOL allblank = TRUE;
    COL tcol;
    ROW trow;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    /* can't join this cell to next one if this cell exists and not null or text */
    thisslot = travel_here();

    if(thisslot  &&  (thisslot->type != SL_TEXT))
        return;

    /* can only join non-textual next cell if this cell null */
    nextslot = travel(curcol, currow + 1);
    if(nextslot  &&  (nextslot->type != SL_TEXT))
    {
        if(thisslot)
            return;

        actually_joining = FALSE;
    }

    if(protected_cell_in_block(curcol, currow, curcol, currow+1))
        return;

    if(actually_joining)
    {
        thislen = strlen(linbuf);
        memcpy32(tempbuffer, linbuf, thislen);

        currow++;

        filbuf();

        nextlen = strlen(linbuf);

        /* SKS after PD 4.11 25jan92 - was >=, but MAXFLD is LIN_BUFSIZ-1 */
        if(thislen + nextlen > MAXFLD)
        {
            currow--;
            slot_in_buffer = FALSE;
            filbuf();
            reperr_null(ERR_LINETOOLONG);
            return;
        }

        memmove32(linbuf + thislen, linbuf,     nextlen+1);
        memcpy32( linbuf,           tempbuffer, thislen);

        buffer_altered = slot_in_buffer = TRUE;
        currow--;

        if(!mergebuf())
            return;
    }

    dont_save = TRUE;

    /* wrap set to rows? */
    if(d_options_IW == 'R')
    {
        /* look to see if all this row bar the current column is blank.
         * If so joinlines can delete a whole row, perhaps
         * be careful of number cells masquerading as blanks
         * for cells to left needs to split on following line
        */
        allblank = TRUE;

        for(tcol = 0; tcol < numcol; tcol++)
        {
            trow = currow;
            if(tcol < curcol)
                ++trow;
            else if((tcol == curcol)  &&  actually_joining)
                continue;

            tcell = travel(tcol, trow);
            if(!is_blank_cell(tcell)  ||  (tcell  &&  (tcell->type != SL_TEXT)))
            {
                allblank = FALSE;
                break;
            }
        }

        if(allblank)
        {
            for(tcol = 0; tcol < numcol; tcol++)
            {
                trow = currow;
                if(tcol < curcol)
                    ++trow;
                else if((tcol == curcol) && actually_joining)
                    ++trow;

                killslot(tcol, trow);
            }

            reset_numrow();

            /* mark deleted area */
            updref(curcol, currow, LARGEST_COL_POSSIBLE, currow, BADCOLBIT, 0, UREF_REPLACE, DOCNO_NONE);

            /* do one updref for the main part of the block,
             * and one updref for the block past the current column
             */
            if(!curcol)
                trow = currow + 1;
            else
            {
                trow = currow + 2;
                updref(curcol, currow + 1, numcol - 1, currow + 1, 0, -1, UREF_UREF, DOCNO_NONE);
            }

            updref(0, trow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 0, -1, UREF_UREF, DOCNO_NONE);

            out_rebuildvert = xf_flush = TRUE;
            filealtered(TRUE);
            mark_to_end(currowoffset-1);
        }
        else if(actually_joining)
        {
            currow++;
            internal_process_command(N_DeleteRowInColumn);     /* get rid of cell */
            internal_process_command(N_InsertRowInColumn);     /* and leave a hole */
            currow--;
        }
    }
    else
    {
        if(actually_joining)
            currow++;

        internal_process_command(N_DeleteRowInColumn);

        if(actually_joining)
            currow--;
    }

    dont_save = FALSE;
}

/******************************************************************************
*
* delete row in column
*
******************************************************************************/

extern void
DeleteRowInColumn_fn(void)
{
    COL tcol = curcol;
    ROW trow = currow;

    xf_flush = TRUE;

    if(protected_cell(tcol, trow))
        return;

    /* save to list first */
    if(!dont_save)
        save_words(linbuf);

    if(!mergebuf_nocheck())
    {
        buffer_altered = slot_in_buffer = FALSE;
        return;
    }

    filealtered(TRUE); /* SKS 10jun97 */

    killslot(tcol, trow);

    reset_numrow();

    /* anything pointing to deleted cells in this row become bad */
    updref(curcol, currow,     curcol, currow,               BADCOLBIT, (ROW) 0, UREF_DELETE, DOCNO_NONE);

    /* rows in all those columns move up */
    updref(curcol, currow + 1, curcol, LARGEST_ROW_POSSIBLE, (COL) 0, (ROW) -1, UREF_UREF, DOCNO_NONE);

    mark_to_end(currowoffset);
}

/******************************************************************************
*
* insert column
*
******************************************************************************/

extern void
InsertColumn_fn(void)
{
    COL tcol = curcol;

    if(!inscolentry(tcol))
        return;

    reset_numrow();

    updref(tcol, (ROW) 0, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 1, (ROW) 0, UREF_UREF, DOCNO_NONE);

    lecpos = lescrl = 0;

    xf_flush = out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
    filealtered(TRUE);
}

/******************************************************************************
*
* delete column
*
******************************************************************************/

extern void
DeleteColumn_fn(void)
{
    COL tcol = curcol;
    DOCNO bd;
    SLR bs, be;
    BOOL res;

    /* don't delete if only column */
    if((numcol <= 1)  ||  (tcol >= numcol))
        return;

    /* don't delete if only visible column */
    if(all_widths_zero(tcol, tcol))
        return;

    if(protected_cell_in_block(tcol, (ROW) 0, tcol, numrow-1))
        return;

    bd = blk_docno;
    bs = blkstart;
    be = blkend;

    blk_docno = current_docno();
    blkstart.col = tcol;
    blkstart.row = (ROW) 0;
    blkend.col   = tcol;
    blkend.row   = numrow - 1;

    res = save_block_and_delete(TRUE/*is_deletion*/, TRUE/*do_save*/);

    blk_docno = bd;
    blkstart  = bs;
    blkend    = be;

    if(res)
    {
        /* if this column is one of two linked columns, clear linking */
        if(colislinked(tcol) && (count_this_linked_column(tcol) <= 2))
            remove_overlapped_linked_columns(tcol, tcol);

        /* data deleted, now close up the emptied column */
        delcolentry(tcol, 1);

        /* update for columns moving left */
        updref(tcol+1, (ROW) 0, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, (COL) -1, (ROW) 0, UREF_UREF, DOCNO_NONE);

        chknlr(tcol, currow);
        lecpos = 0;

        xf_flush = out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
        filealtered(TRUE);
    }
}

/******************************************************************************
*
*  options
*
******************************************************************************/

static BOOL
options_fn_core(void)
{
    update_variables();

    filealtered(TRUE);

    return(TRUE);
}

extern void
Options_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_OPTIONS))
    {
        if(!options_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  recalculate
*
******************************************************************************/

extern void
Recalculate_fn(void)
{
    ev_recalc_all();
}

/******************************************************************************
*
*  select display colours
*
******************************************************************************/

static BOOL
colours_fn_core(void)
{
    P_DOCU p_docu;
    DOCNO old_docno = current_docno();

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        select_document(p_docu);
        riscos_invalidate_document_window();
    }

    select_document_using_docno(old_docno);

    if(!dialog_box_can_persist())
    {
        /* must force ourselves to set caret position
         * in order to set new caret colour after killing
         * the menu tree (in case of writeable icons)
        */
        if(NO_DOCUMENT != (p_docu = find_document_with_input_focus()))
            p_docu->Xxf_acquirecaret = p_docu->Xxf_interrupted = TRUE;
        return(FALSE);
    }

    return(TRUE);
}

extern void
Colours_fn(void)
{
#if defined(EXTENDED_COLOUR)
    const int i_dialogue_box = D_EXTENDED_COLOURS;
#else
    const int i_dialogue_box = D_COLOURS;
#endif

    if(!dialog_box_start())
        return;

    while(dialog_box(i_dialogue_box))
    {
        if(!colours_fn_core()) /* handles dialog_box_can_persist() */
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  exec a file
*
******************************************************************************/

static int
domacrofile_fn_core(void)
{
    PCTSTR filename = d_execfile[0].textfield;
    char buffer[BUF_MAX_PATHSTRING];
    STATUS res;

    if(str_isblank(filename))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    if((res = add_path_or_relative_using_dir(buffer, elemof32(buffer), filename, TRUE, MACROS_SUBDIR_STR)) <= 0)
    {
        consume_bool(reperr(res ? res : ERR_NOTFOUND, filename));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    // reportf("Executing command file %s", buffer);

    do_execfile(buffer);

    /* bodge required as dialog_box_ended considers this too */
    exec_filled_dialog = FALSE;

    return(TRUE);
}

static BOOL
domacrofile_fn_prepare(void)
{
    LIST_ITEMNO macros_list_key;

    false_return(dialog_box_can_start());

    if(in_execfile)
        return(reperr_null(ERR_BAD_PARM));

    delete_list(&macros_list);

    macros_list_key = 0;
    status_assert(enumerate_dir_to_list(&macros_list, &macros_list_key, NULL, FILETYPE_PD_MACRO)); /* ones adjacent to current document as there won't be any on path here */
    status_assert(enumerate_dir_to_list(&macros_list, &macros_list_key, MACROS_SUBDIR_STR, FILETYPE_PD_MACRO)); /* ones in the given sub-directoryboth here and on path */

    return(dialog_box_start());
}

extern void
DoMacroFile_fn(void)
{
    if(!domacrofile_fn_prepare())
        return;

    /* don't allow this dialogue box to persist as command fillin will bork */
    if(!dialog_box(D_EXECFILE))
        return;

    dialog_box_end();

    (void) domacrofile_fn_core();

    delete_list(&macros_list);
}

/******************************************************************************
*
* About this file
*
******************************************************************************/

extern void
About_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_ABOUT))
    {
        /* nothing at all */

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  Leave PipeDream
*
******************************************************************************/

extern void
Quit_fn(void)
{
    S32 nmodified;

    mergebuf_all();

    nmodified = documents_modified();

    if(nmodified)
        if(!riscos_quit_okayed(nmodified))
            return;

    /* close macro file & stamp it now it's all valid */
    if(macro_recorder_on)
        internal_process_command(N_RecordMacroFile);

    exit(EXIT_SUCCESS); /* trapped by various atexit()ed functions */
}

/******************************************************************************
*
* delete row
*
******************************************************************************/

extern void
DeleteRow_fn(void)
{
    SLR bs, be, ba;
    DOCNO bd;
    BOOL res = TRUE;

    xf_flush = TRUE;

    if(protected_cell_in_block(0, currow, numcol-1, currow))
        return;

    if(!mergebuf())
    {
        buffer_altered = slot_in_buffer = xf_inexpression = FALSE; /*>>>RCM says what the heck do I do with this???*/
        return;
    }

    if(!dont_save)
    {
        bd = blk_docno;
        bs = blkstart;
        be = blkend;
        ba = blkanchor;

        blk_docno    = current_docno();
        blkstart.col = 0;
        blkstart.row = currow;
        blkend.col   = numcol - 1;
        blkend.row   = currow;

        res = save_block_and_delete(TRUE/*is_deletion*/, TRUE/*do_save*/);

        if(res)
        {
            if(blk_docno == bd)
            {
                /* if deleting row above start of block then move block up */
                if(bs.row > currow)
                {
                    if( ba.row == bs.row--)
                        ba.row = bs.row;
                }

                /* if deleting row in middle of or last row of block then
                 * move end of block up, and anchor too possibly
                */
                if(be.row >= currow)
                {
                    if( ba.row == be.row--)
                        ba.row = be.row;
                }

                /* if last row of block has been deleted, remove block */
                if(be.row < bs.row)
                    bs.col = NO_COL;
            }
        }

        blk_docno = bd;
        blkstart  = bs;
        blkend    = be;
        blkanchor = ba;
    }

    if(res)
    {
        out_rebuildvert = TRUE;
        filealtered(TRUE);
        mark_to_end(currowoffset);
    }
}

/******************************************************************************
*
* copy block to buffer
*
* if block not in current buffer switch windows temporarily
*
******************************************************************************/

extern void
CopyBlockToPasteList_fn(void)
{
    DOCNO old_docno;

    if(blkstart.col == NO_COL)
    {
        reperr_null(ERR_NOBLOCK);
        return;
    }

    old_docno = change_document_using_docno(blk_docno);

    consume_bool(save_block_and_delete(FALSE/*is_deletion*/, TRUE/*do_save*/));

    select_document_using_docno(old_docno);
}

/******************************************************************************
*
* save string to stack
*
******************************************************************************/

extern BOOL
save_words(
    uchar *ptr)
{
    S32 res;

    if(been_error)
        return(FALSE);

    if(words_allowed == (S32) 0)
        return(TRUE);

    /* save deleted bit */

    if(status_fail(res = add_to_list(&deleted_words, ++latest_word_on_stack, ptr)))
        reperr_null(res);

    if(been_error)
    {
        --latest_word_on_stack;
        return(FALSE);
    }

    ensure_paste_list_clipped();

    return(TRUE);
}

/******************************************************************************
*
* delete to end of cell
*
******************************************************************************/

extern void
DeleteToEndOfSlot_fn(void)
{
    P_CELL tcell;

    if(protected_cell(curcol, currow))
        return;

    mark_row(currowoffset);

    if(!slot_in_buffer)
    {
        /* this is to delete non-text cells */
        tcell = travel_here();

        if(tcell  &&  (tcell->type != SL_PAGE))
        {
            char buffer[EV_MAX_IN_LEN + 1];

            prccon(buffer, tcell);
            save_words(buffer);
        }

        slot_in_buffer = buffer_altered = output_buffer = TRUE;
        lecpos = lescrl = 0;
        linbuf[0] = CH_NULL;
        (void) mergebuf_nocheck();
        return;
    }

    if(strlen((const char *) linbuf) > (size_t) lecpos)
        save_words(linbuf + lecpos);

    linbuf[lecpos] = CH_NULL;

    if(dspfld_from == -1)
        dspfld_from = lecpos;

    buffer_altered = output_buffer = TRUE;

    mergebuf();
}

/******************************************************************************
*
*                               protection
*
******************************************************************************/

/******************************************************************************
*
*  see if a cell is protected
*
******************************************************************************/

extern BOOL
test_protected_cell(
    COL tcol,
    ROW trow)
{
    P_CELL tcell = travel(tcol, trow);

    return(tcell  &&  is_protected_cell(tcell));
}

extern BOOL
protected_cell(
    COL tcol,
    ROW trow)
{
    if(test_protected_cell(tcol, trow))
        return(!reperr_null(ERR_PROTECTED));

    return(FALSE);
}

/******************************************************************************
*
*  verify no protect cells in block: if there are - whinge
*
*  note that this corrupts in_block.col & in_block.row
*
******************************************************************************/

extern BOOL
protected_cell_in_range(
    PC_SLR bs,
    PC_SLR be)
{
    P_CELL tcell;
    BOOL res = FALSE;

    trace_4(TRACE_APP_PD4, "\n\n[protected_cell_in_range: bs (%d, %d) be (%d, %d)", bs->col, bs->row, be->col, be->row);

    init_block(bs, be);

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        trace_3(TRACE_APP_PD4, "got cell " PTR_XTFMT ", (%d, %d)", report_ptr_cast(tcell), in_block.col, in_block.row);

        if(is_protected_cell(tcell))
        {
            res = !reperr_null(ERR_PROTECTED);
            break;
        }
    }

    return(res);
}

extern BOOL
protected_cell_in_block(
    COL firstcol,
    ROW firstrow,
    COL lastcol,
    ROW lastrow)
{
    SLR bs, be;

    bs.col = firstcol;
    bs.row = firstrow;

    be.col = lastcol;
    be.row = lastrow;

    return(protected_cell_in_range(&bs, &be));
}

extern void
ClearProtectedBlock_fn(void)
{
    alnsto(J_BITS, (uchar) 0xFF, (uchar) 0, FALSE);
}

static void
set_protected_block(
    PC_SLR bs,
    PC_SLR be)
{
    alnsto_block(PROTECTED, (uchar) 0xFF, (uchar) 0, FALSE, bs, be);
}

extern void
SetProtectedBlock_fn(void)
{
    set_protected_block(&blkstart, &blkend);
}

extern void
setprotectedstatus(
    P_CELL tcell)
{
    COLOURS_OPTION_INDEX fg_colours_option_index;

    if(tcell  &&  is_protected_cell(tcell))
    {
        currently_protected = TRUE;

        fg_colours_option_index = currently_inverted ? COI_PROTECT : COI_FORE;

        setcolours(fg_colours_option_index, (COLOURS_OPTION_INDEX) (fg_colours_option_index ^ COI_PROTECT ^ COI_FORE));
    }
    else
        currently_protected = FALSE;
}

/*
check that the cells between firstcol and lastcol in the row are protected
 - but not the previous cell or the subsequent one
*/

static BOOL
check_prot_range(
    ROW trow,
    COL firstcol,
    COL lastcol)
{
    if((firstcol > 0)  &&  test_protected_cell(firstcol-1, trow))
        return(FALSE);

    while(firstcol <= lastcol)
        if(!test_protected_cell(firstcol++, trow))
            return(FALSE);

    /* check cell following lastcol */
    return(!test_protected_cell(firstcol, trow));
}

/*
save spreadsheet names to file
*/

#define NAMEBUF_LEN (BUF_EV_INTNAMLEN + MAX_PATHSTRING)

extern void
save_names_to_file(
    FILE_HANDLE output)
{
    EV_RESOURCE resource;
    EV_OPTBLOCK optblock;
    S32         itemno;
    char        namebuf[NAMEBUF_LEN];
    char        argbuf[256];
    S32         argcount;
    EV_DOCNO    cur_docno;
    char buffer[NAMEBUF_LEN + LIN_BUFSIZ +2];

    cur_docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, cur_docno);

    ev_enum_resource_init(&resource, EV_RESO_NAMES, cur_docno, cur_docno, &optblock);

    for(itemno = -1;
        (ev_enum_resource_get(&resource, &itemno, namebuf, NAMEBUF_LEN -1, argbuf, sizeof(argbuf) - 1, &argcount) >= 0);
        itemno = -1
       )
    {
        xstrkpy(buffer, elemof32(buffer), namebuf);
        xstrkat(buffer, elemof32(buffer), ",");
        xstrkat(buffer, elemof32(buffer), argbuf);
        consume_bool(mystr_set(&d_names_dbox[0].textfield, buffer));
        save_opt_to_file(output, d_names_dbox, 1);
    }
}

/* incoming option in d_names_dbox[0] from file being loaded
 * add to list of spreadsheet names
*/

extern void
add_to_names_list(
    uchar *from)
{
    char        *name;
    char        *contents;
    EV_OPTBLOCK  optblock;
    EV_DOCNO     cur_docno;
    STATUS       err;

    name = from;
    for(contents = from; *contents != ','; contents++)
        if(*contents == CH_NULL)
            return;

    /* looking at comma here - poke to null and point contents one on */
    *contents++ = CH_NULL;

    cur_docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, cur_docno);

    err = ev_name_make(name, cur_docno, contents, &optblock, FALSE);

    if(err < 0)
        reperr_null(err);
}

/*
save all blocks of protected cells to the file

algorithm for getting block is to find top-left
    then horizontal extent is all cells marked to the right
    vertical extent is rows beneath with all cols as first row marked
        but not the cell to the left or the one to the right.

this ensures that a marked cell has already been saved if the cell
to the left is marked.
OK Ya.

sexiest little algorithm rob ever invented

don't be rude, I bet you couldn't describe an arbitrary set of cells in fewer rectangles
*/

extern void
save_protected_bits(
    FILE_HANDLE output)
{
    P_CELL tcell;
    SLR last;
    U8 buffer[BUF_MAX_REFERENCE];
    uchar *ptr;

    last.col = (COL) 0;
    last.row = (ROW) 0;

    init_doc_as_block();

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        /* if have already saved further down the block */
        if((in_block.col == last.col)  &&  (last.row > in_block.row))
            continue;

        if(is_protected_cell(tcell))
        {
            /* if cell to left is protected, this cell already saved */
            /* honest */
            if((in_block.col > 0)  &&  test_protected_cell(in_block.col-1, in_block.row))
                continue;

            last.col = in_block.col + 1;
            while(test_protected_cell(last.col, in_block.row))
                ++last.col;
            --last.col;

            /* look to see how many rows down are similar - save them too */
            last.row = in_block.row + 1;
            while(check_prot_range(last.row, in_block.col, last.col))
                ++last.row;
            --last.row;

            /* now lastcol is one past the last column */
            ptr = buffer;
            ptr += write_ref(ptr, elemof32(buffer),                  current_docno(), in_block.col, in_block.row);
            ptr += write_ref(ptr, elemof32(buffer) - (ptr - buffer), current_docno(), last.col, last.row);
            *ptr = CH_NULL;

            consume_bool(mystr_set(&d_protect[0].textfield, buffer));
            save_opt_to_file(output, d_protect, 1);

            /* last doubles as remembering the last top-left */
            last.col = in_block.col;
            last.row++;
        }
    }
}

/* incoming option in d_protect[0] from file being loaded
 * add to list of protected blocks
*/

extern void
add_to_protect_list(
    uchar *ptr)
{
    S32 res;

    if(str_isblank(ptr))
        return;

    if(status_fail(res = add_to_list(&protected_blocks, 0, ptr)))
    {
        reperr_null(res);
        been_error = FALSE; /* shouldn't affect validity of loaded file if wally error */
    }
}

/*
clear the protect list, setting the cell protection
*/

extern void
clear_protect_list(void)
{
    SLR bs, be;
    P_LIST lptr;

    /* bug in 4.01:
        if load a file and marked block exists in some window PD4 whinges about
        marked block not in this document
    */
    /* RJM 6.11.91, fix for above */
    SLR os, oe;
    DOCNO docno = blk_docno;
    os = blkstart;
    oe = blkend;
    blk_docno = current_docno();

    for(lptr = first_in_list(&protected_blocks);
        lptr;
        lptr = next_in_list(&protected_blocks))
    {
        /* read two cell references out of lptr->value */

        buff_sofar = lptr->value;

        bs.col =        getcol();
        bs.row = (ROW) getsbd()-1;
        be.col =        getcol();
        be.row = (ROW) getsbd()-1;

        /* and mark the block */
        set_protected_block(&bs, &be);
    }

    /* RJM 6.11.91, fix for above */
    blk_docno = docno;
    blkstart = os;
    blkend   = oe;

    delete_list(&protected_blocks);
}

/*
save linked column info to the file
*/

extern void
save_linked_columns(
    FILE_HANDLE output)
{
    COL count = 0;
    COL first, last;

    while(enumerate_linked_columns(&count, &first, &last))
    {
        char buffer[40];

        consume_int(sprintf(buffer, "%d,%d", first, last));

        if(mystr_set(&d_linked_columns[0].textfield, buffer))
        {
            save_opt_to_file(output, d_linked_columns, 1);
        }
    }
}

/* incoming option in d_linked_columns[0] from file being loaded
 * add to list of linked_columns
*/

extern void
add_to_linked_columns_list(
    uchar *ptr)
{
    S32 res;

    if(str_isblank(ptr))
        return;

    if(status_fail(res = add_to_list(&linked_columns, 0, ptr)))
    {
        reperr_null(res);
        been_error = FALSE; /* shouldn't affect validity of loaded file if wally error */
    }
}

/*
clear the linked column, setting linked columns
*/

extern void
clear_linked_columns(void)
{
    P_LIST lptr;

    for(lptr = first_in_list(&linked_columns);
        lptr;
        lptr = next_in_list(&linked_columns))
    {
        COL first,last;

        /* read two cell references out of lptr->value */

        buff_sofar = lptr->value;

        first = (COL) getsbd();
        buff_sofar++;           /* skip past comma cos getsbd() won't */
        last  = (COL) getsbd();

        /* and mark the block */
        mark_as_linked_columns(first, last);
    }

    adjust_all_linked_columns();
    out_screen = TRUE;
    delete_list(&linked_columns);
}

/* end of execs.c */
