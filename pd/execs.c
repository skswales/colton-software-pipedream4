/* execs.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that does lots of backsplash commands */

/* RJM August 1987 */

#define RJM

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
    /* if block marked, do each slot in block */
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
* set a marked block or the current slot with a format
* the justify byte is put into the justify field
* in the slot (unless NO_JUSTIFY)
* mask & bits give the information to change the format
* some operations set bits, some clear, and some toggle bits
* this is done by masking out some bits and then exclusive-oring
* eg to clear  - mask with 0 and eor with 0
*    to set    - mask with 0 and eor with 1
*    to toggle - mask with 1 and eor with 1
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
    P_SLOT tslot;

    if(fault_protection  &&  protected_slot_in_range(bs, be))
        return;

    if(blkstart.col != NO_COL && !set_up_block(TRUE))
        return;

    init_block(bs, be);

    mark_block_output(&start_bl);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        filealtered(TRUE);

        switch(justify)
            {
            case NO_JUSTIFY:
                break;

            case PROTECTED:
                /* set protected bit */
                tslot->justify |= PROTECTED;
                break;

            case J_BITS:
                /* clear protected bit */
                tslot->justify &= J_BITS;
                break;

            default:
                /* set justify bits, retaining protected status */
                tslot->justify = (tslot->justify & CLR_J_BITS) | justify;
                break;
            }

        switch(tslot->type)
            {
            case SL_NUMBER:
                tslot->format = (tslot->format & mask) ^ bits;
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
* if the slot is currently using option page defaults then those
* defaults are copied into the slot
*
******************************************************************************/

static void
alnst1(
    uchar mask,
    uchar bits)
{
    P_SLOT tslot;
    uchar format;

    if(protected_slot_in_range(&blkstart, &blkend))
        return;

    /* rjm adds this check on 15.10.91 */
    if(blkstart.col != NO_COL && !set_up_block(TRUE))
        return;

    init_marked_block();

    mark_block_output(&start_bl);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        switch(tslot->type)
            {
            case SL_NUMBER:
                filealtered(TRUE);

                format = tslot->format;

                if((format & F_DCP) == 0)
                    {
                    format |= (F_DCP | F_BRAC);
                    if(d_options_MB == 'M')
                        format &= (uchar) ~F_BRAC;
                    format &= ~F_DCPSID;
                    format |= get_dec_field_from_opt();
                    }

                tslot->format = (format & mask) ^ bits;
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
        alnst1((uchar) ~F_DCPSID, (d_decimal[0].option == 'F')
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
    P_SLOT tslot;
    uchar justify;

    /* check word perhaps */
    if(lecpos  &&  slot_in_buffer)
        check_word();

    if(d_options_IR == 'Y')
        {
        internal_process_command(N_SplitLine);

        /* remove justify bit from current slot */
        if( !xf_inexpression  &&
            ((tslot = travel_here()) != NULL))
                {
                /* shirley we don't have to worry about PROTECTED bit,
                    cos if protected what are we doing poking slot?
                */
                justify = tslot->justify & J_BITS;
                if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
                    /* set justify bits, retaining protected status */
                    tslot->justify = (tslot->justify & CLR_J_BITS) | J_FREE;
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
        /* force blank slot in */
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
    uchar *array)
{
    uchar *from = array;
    uchar *to   = array;
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
                    while(to > array && *(to-1) == SPACE)
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

    *to = '\0';
}

/******************************************************************************
*
*  define key
*
******************************************************************************/

extern void
DefineKey_fn(void)
{
    uchar array[LIN_BUFSIZ];
    S32 key;
    S32 res;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEFKEY)) /* so we can see prev defn'n */
        {
        if(!d_defkey[1].textfield)
            *array = '\0';
        else
            strcpy((char *) array, d_defkey[1].textfield);

        prccml(array);      /* convert funny characters in array */

        key = (S32) d_defkey[0].option;

        /* remove old definition */
        delete_from_list(&first_key, (S32) d_defkey[0].option);

        /* add new one */
        if(*array)
            {
            if(status_fail(res = add_to_list(&first_key, (S32) d_defkey[0].option, array)))
                {
                reperr_null(res);
                break;
                }
            }

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

extern void
DefineFunctionKey_fn(void)
{
    uchar array[LIN_BUFSIZ];
    S32 key;
    S32 res;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEF_FKEY))
        {
        if(!d_def_fkey[1].textfield)
            *array = '\0';
        else
            strcpy((char *) array, d_def_fkey[1].textfield);

        prccml(array);      /* convert funny characters in array */

        key = (S32) func_key_list[d_def_fkey[0].option];

        /* remove old definition */
        delete_from_list(&first_key, key);

        /* add new definition */
        if(*array)
            {
            if(status_fail(res = add_to_list(&first_key, key, array)))
                {
                reperr_null(res);
                break;
                }
            }

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

extern void
DefineCommand_fn(void)
{
    char array[LIN_BUFSIZ];
    S32 key;
    char *src;
    char *dst;
    S32 ch, res;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_DEF_CMD))
        {
        src = d_def_cmd[0].textfield;

        if(!src  ||  (strlen(src) > (sizeof(S32) * 8 / 5)))
            {
            bleep();
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        dst = array;
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

        if(src)
            {
            do  {
                ch = *src++;
                *dst++ = toupper(ch);   /* including terminating NULLCH */
                }
            while(ch);

            if(!str_isblank(array))
                {
                /* add new definition */
                if(status_fail(res = add_to_list(&first_command_redef, key, array)))
                    {
                    reperr_null(res);
                    break;
                    }
                }
            }

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
    U8 array[BUF_MAX_REFERENCE];

    if(!xf_inexpression && !xf_inexpression_box && !xf_inexpression_line)
        {
        bleep();
        return;
        }

    /* expand column then row into array */
    (void) write_ref(array, elemof32(array), current_docno(), curcol, currow);

    insert_string(array, FALSE);
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

extern void
SplitLine_fn(void)
{
    uchar tempchar;
    P_SLOT tslot;
    BOOL actually_splitting = FALSE;
    COL tcol;
    ROW trow;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    if(lecpos  &&  protected_slot(curcol, currow))
        return;

    tslot = travel_here();

    if(!tslot  ||  ((tslot->type == SL_TEXT))  &&  !is_protected_slot(tslot))
        {
        U32 bufflength = xustrlen32(linbuf, elemof32(linbuf));
        U32 splitpoint = MIN(bufflength, (U32) lecpos);

        assert(splitpoint < elemof32(linbuf));
        tempchar = linbuf[splitpoint];
        linbuf[splitpoint] = NULLCH;

        if( dspfld_from == -1)
            dspfld_from = splitpoint;

        actually_splitting = buffer_altered = slot_in_buffer = TRUE;

        if(!mergebuf())
            return;

        /* leave lying around for merging later on */
        linbuf[splitpoint] = tempchar;
        memmove32(linbuf, linbuf + splitpoint, (bufflength - splitpoint + 1));
        }

    /* if insert on wrap is row insert slots in this row for each column to the right,
     * and on the next row for each each column to the left, and for this one if we'll need
     * to move the rest of the line down
    */
    if(iowbit)
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
    P_SLOT thisslot, nextslot, tslot;
    S32 thislen, nextlen;
    BOOL actually_joining = TRUE;
    uchar temparray[LIN_BUFSIZ];
    BOOL allblank = TRUE;
    COL tcol;
    ROW trow;

    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    /* can't join this slot to next one if this slot exists and not null or text */
    thisslot = travel_here();

    if(thisslot  &&  (thisslot->type != SL_TEXT))
        return;

    /* can only join non-textual next slot if this slot null */
    nextslot = travel(curcol, currow + 1);
    if(nextslot  &&  (nextslot->type != SL_TEXT))
        {
        if(thisslot)
            return;

        actually_joining = FALSE;
        }

    if(protected_slot_in_block(curcol, currow, curcol, currow+1))
        return;

    if(actually_joining)
        {
        thislen = strlen(linbuf);
        memcpy32(temparray, linbuf, thislen);

        currow++;

        filbuf();

        nextlen = strlen(linbuf);

        /* SKS after 4.11 25jan92 - was >=, but MAXFLD is LIN_BUFSIZ-1 */
        if(thislen + nextlen > MAXFLD)
            {
            currow--;
            slot_in_buffer = FALSE;
            filbuf();
            reperr_null(create_error(ERR_LINETOOLONG));
            return;
            }

        memmove32(linbuf + thislen, linbuf,    nextlen+1);
        memcpy32( linbuf,           temparray, thislen);

        buffer_altered = slot_in_buffer = TRUE;
        currow--;

        if(!mergebuf())
            return;
        }

    dont_save = TRUE;

    /* wrap set to rows? */
    if(iowbit)
        {
        /* look to see if all this row bar the current column is blank.
         * If so joinlines can delete a whole row, perhaps
         * be careful of numeric slots masquerading as blanks
         * for slots to left needs to split on following line
        */
        allblank = TRUE;

        for(tcol = 0; tcol < numcol; tcol++)
            {
            trow = currow;
            if(tcol < curcol)
                ++trow;
            else if((tcol == curcol)  &&  actually_joining)
                continue;

            tslot = travel(tcol, trow);
            if(!isslotblank(tslot)  ||  (tslot  &&  (tslot->type != SL_TEXT)))
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
            internal_process_command(N_DeleteRowInColumn);     /* get rid of slot */
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

    if(protected_slot(tcol, trow))
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

    /* anything pointing to deleted slots in this row become bad */
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

    if(protected_slot_in_block(tcol, (ROW) 0, tcol, numrow-1))
        return;

    bd = blk_docno;
    bs = blkstart;
    be = blkend;

    blk_docno = current_docno();
    blkstart.col = tcol;
    blkstart.row = (ROW) 0;
    blkend.col   = tcol;
    blkend.row   = numrow - 1;

    res = save_block_and_delete(TRUE, TRUE);

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

extern void
Options_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_OPTIONS))
        {
        update_variables();

        filealtered(TRUE);

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

extern void
Colours_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_COLOURS))
        {
        P_DOCU p_docu;
        DOCNO old_docno = current_docno();

        for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
            {
            select_document(p_docu);
            riscos_invalidatemainwindow();
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
            break;
            }
        }

    dialog_box_end();
}

/******************************************************************************
*
*  exec a file
*
******************************************************************************/

extern void
DoMacroFile_fn(void)
{
    char array[BUF_MAX_PATHSTRING];
    PC_U8 filename;

    if(in_execfile)
        {
        reperr_null(create_error(ERR_BAD_PARM));
        return;
        }

    if(!dialog_box_start())
        return;

    status_assert(enumerate_dir_to_list(&ltemplate_or_driver_list, NULL, FILETYPE_PDMACRO));

    while(dialog_box(D_EXECFILE))
        {
        filename = d_execfile[0].textfield;

        if(str_isblank(filename))
            {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
            }
        else
            {
            if(file_find_on_path_or_relative(array, elemof32(array), filename, currentfilename))
                do_execfile(array);
            else
                reperr(create_error(ERR_NOTFOUND), filename);
            }

        /* bodge required as dialog_box_ended considers this too */
        exec_filled_dialog = FALSE;

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();

    delete_list(&ltemplate_or_driver_list);
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

    if(protected_slot_in_block(0, currow, numcol-1, currow))
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

        res = save_block_and_delete(TRUE, TRUE);

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
        reperr_null(create_error(ERR_NOBLOCK));
        return;
        }

    old_docno = change_document_using_docno(blk_docno);

    (void) save_block_and_delete(FALSE, TRUE);

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
* delete to end of slot
*
******************************************************************************/

extern void
DeleteToEndOfSlot_fn(void)
{
    P_SLOT tslot;

    if(protected_slot(curcol, currow))
        return;

    mark_row(currowoffset);

    if(!slot_in_buffer)
        {
        /* this is to delete non-text slots */
        tslot = travel_here();

        if(tslot  &&  (tslot->type != SL_PAGE))
            {
            char buffer[EV_MAX_IN_LEN + 1];

            prccon(buffer, tslot);
            save_words(buffer);
            }

        slot_in_buffer = buffer_altered = output_buffer = TRUE;
        lecpos = lescrl = 0;
        *linbuf = '\0';
        (void) mergebuf_nocheck();
        return;
        }

    if(strlen((const char *) linbuf) > (size_t) lecpos)
        save_words(linbuf + lecpos);

    linbuf[lecpos] = '\0';

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
*  see if a slot is protected
*
******************************************************************************/

extern BOOL
test_protected_slot(
    COL tcol,
    ROW trow)
{
    P_SLOT tslot = travel(tcol, trow);

    return(tslot  &&  is_protected_slot(tslot));
}

extern BOOL
protected_slot(
    COL tcol,
    ROW trow)
{
    if(test_protected_slot(tcol, trow))
        return(!reperr_null(create_error(ERR_PROTECTED)));

    return(FALSE);
}

/******************************************************************************
*
*  verify no protect slots in block: if there are - winge
*
*  note that this corrupts in_block.col & in_block.row
*
******************************************************************************/

extern BOOL
protected_slot_in_range(
    PC_SLR bs,
    PC_SLR be)
{
    P_SLOT tslot;
    BOOL res = FALSE;

    trace_4(TRACE_APP_PD4, "\n\n[protected_slot_in_range: bs (%d, %d) be (%d, %d)", bs->col, bs->row, be->col, be->row);

    init_block(bs, be);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        trace_3(TRACE_APP_PD4, "got slot " PTR_XTFMT ", (%d, %d)", report_ptr_cast(tslot), in_block.col, in_block.row);

        if(is_protected_slot(tslot))
            {
            res = !reperr_null(create_error(ERR_PROTECTED));
            break;
            }
        }

    return(res);
}

extern BOOL
protected_slot_in_block(
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

    return(protected_slot_in_range(&bs, &be));
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
    P_SLOT tslot)
{
    S32 fg;

    if(tslot  &&  is_protected_slot(tslot))
        {
        currently_protected = TRUE;

        fg = currently_inverted ? PROTECTC : FORE;

        setcolour(fg, fg ^ PROTECTC ^ FORE);
        }
    else
        currently_protected = FALSE;
}

/*
check that the slots between firstcol and lastcol in the row are protected
 - but not the previous slot or the subsequent one
*/

static BOOL
check_prot_range(
    ROW trow,
    COL firstcol,
    COL lastcol)
{
    if((firstcol > 0)  &&  test_protected_slot(firstcol-1, trow))
        return(FALSE);

    while(firstcol <= lastcol)
        if(!test_protected_slot(firstcol++, trow))
            return(FALSE);

    /* check slot following lastcol */
    return(!test_protected_slot(firstcol, trow));
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
    char array[NAMEBUF_LEN + LIN_BUFSIZ +2];

    cur_docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, cur_docno);

    ev_enum_resource_init(&resource, EV_RESO_NAMES, cur_docno, cur_docno, &optblock);

    for(itemno = -1;
        (ev_enum_resource_get(&resource, &itemno, namebuf, NAMEBUF_LEN -1, argbuf, sizeof(argbuf) - 1, &argcount) >= 0);
        itemno = -1
       )
        {
        xstrkpy(array, elemof32(array), namebuf);
        xstrkat(array, elemof32(array), ",");
        xstrkat(array, elemof32(array), argbuf);
        (void) mystr_set(&d_names_dbox[0].textfield, array);
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
    S32          err;

    name = from;
    for(contents = from; *contents != ','; contents++)
        if(*contents == '\0')
            return;

    /* looking at comma here - poke to null and point contents one on */
    *contents++ = '\0';

    cur_docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, cur_docno);

    err = ev_name_make(name, cur_docno, contents, &optblock, FALSE);

    if(err < 0)
        reperr_null(err);
}

/*
save all blocks of protected slots to the file

algorithm for getting block is to find top-left
    then horizontal extent is all slots marked to the right
    vertical extent is rows beneath with all cols as first row marked
        but not the slot to the left or the one to the right.

this ensures that a marked slot has already been saved if the slot
to the left is marked.
OK Ya.

sexiest little algorithm rob ever invented

don't be rude, I bet you couldn't describe an arbitrary set of slots in fewer rectangles
*/

extern void
save_protected_bits(
    FILE_HANDLE output)
{
    P_SLOT tslot;
    SLR last;
    U8 array[BUF_MAX_REFERENCE];
    uchar *ptr;

    last.col = (COL) 0;
    last.row = (ROW) 0;

    init_doc_as_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        /* if have already saved further down the block */
        if((in_block.col == last.col)  &&  (last.row > in_block.row))
            continue;

        if(is_protected_slot(tslot))
            {
            /* if slot to left is protected, this slot already saved */
            /* honest */
            if((in_block.col > 0)  &&  test_protected_slot(in_block.col-1, in_block.row))
                continue;

            last.col = in_block.col + 1;
            while(test_protected_slot(last.col, in_block.row))
                ++last.col;
            --last.col;

            /* look to see how many rows down are similar - save them too */
            last.row = in_block.row + 1;
            while(check_prot_range(last.row, in_block.col, last.col))
                ++last.row;
            --last.row;

            /* now lastcol is one past the last column */
            ptr = array;
            ptr += write_ref(ptr, elemof32(array),                 current_docno(), in_block.col, in_block.row);
            ptr += write_ref(ptr, elemof32(array) - (ptr - array), current_docno(), last.col, last.row);
            *ptr = '\0';

            (void) mystr_set(&d_protect[0].textfield, array);
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
clear the protect list, setting the slot protection
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
    /* rjm 6.11.91, fix for above */
    SLR os, oe;
    DOCNO docno = blk_docno;
    os = blkstart;
    oe = blkend;
    blk_docno = current_docno();

    for(lptr = first_in_list(&protected_blocks);
        lptr;
        lptr = next_in_list(&protected_blocks))
        {
        /* read two slot references out of lptr->value */

        buff_sofar = lptr->value;

        bs.col =        getcol();
        bs.row = (ROW) getsbd()-1;
        be.col =        getcol();
        be.row = (ROW) getsbd()-1;

        /* and mark the block */
        set_protected_block(&bs, &be);
        }

    /* rjm 6.11.91, fix for above */
    blk_docno = docno;
    blkstart = os;
    blkend   = oe;

    delete_list(&protected_blocks);
}

/*
save all blocks of protected slots to the file

*/

extern void
save_linked_columns(
    FILE_HANDLE output)
{
    COL count, first, last;

    count = 0;

    while(enumerate_linked_columns(&count, &first, &last))
        {
        char array[40];

        (void) sprintf(array, "%d,%d", first, last);

        (void) mystr_set(&d_linked_columns[0].textfield, array);
        save_opt_to_file(output, d_linked_columns, 1);
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

        /* read two slot references out of lptr->value */

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
