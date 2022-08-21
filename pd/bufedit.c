/* bufedit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that edits the line buffer & simple movements */

/* RJM August 87 */

#include "common/gflags.h"

#include "os.h"
#include "bbc.h"

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#include "cmodules/spell.h"

#include "datafmt.h"

#include "colh_x.h"

#include "riscos_x.h"

/*
internal functions
*/

static void
del_chkwrp(void);       /* check for reformat situation on deleting characters */

/******************************************************************************
*
* formatting routines
*
******************************************************************************/

static BOOL
chkcfm(
    P_CELL tcell,
    ROW trow);

static coord
fndlbr(
    ROW row);

static uchar *
colword(void);

static BOOL
mergeinline(
    coord);

static void
init_colword(
    ROW trow);

static uchar *
word_on_new_line(void);

static S32
font_line_break(
    char *str,
    S32 wrap_point);

/* variables used in formatting */
static ROW curr_inrow;             /* input row for colword() */
static ROW was_curr_inrow;
static uchar *lastword;
static S32 word_in;
static S32 word_out;
static S32 last_word_in;
static uchar old_just;
static uchar new_just;
static S32 old_flags;
static S32 cur_leftright;

static uchar lastwordbuff[LIN_BUFSIZ];

static ROW curr_outrow;            /* output row for mergeinline() */
static S32 out_offset;
static S32 lindif;                 /* number of rows displaced in format */
static BOOL firstword;

/******************************************************************************
*
* insert character into buffer
* If nothing in buffer, decide whether
* it's a text or number cell
*
******************************************************************************/

extern void
inschr(
    uchar ch)
{
    P_CELL tcell;

    if((xf_inexpression || xf_inexpression_box || xf_inexpression_line) && (ch < SPACE))
        return;

#if FALSE
    if(!mergebuf())     /* RCM & MRJC think this is needed */
        return;
#endif

    tcell = travel_here();

    for(;;) /* loop for structure */
    {
        /* insert character as text if no cell and not in number mode */
        if(NULL == tcell)
        {
            if(d_options_TN == 'T')
                break;

            /* allow some characters to begin entering text into the buffer in blank 'number' cells */
            if(ishighlight(ch))
                break; /* highlight characters are some such */

            if(is_text_at_char(ch))
                break; /* text-at character is another */

            if(slot_in_buffer && (CH_NULL != linbuf[0]))
                break; /* this will only break if text has already sneaked into the buffer from cases above */
        }
        else
        {
            if(is_protected_cell(tcell))
            {
                reperr_null(ERR_PROTECTED);
                return;
            }

            /* insert character as text iff text cell or blank text cell and text mode */
            if((tcell->type == SL_TEXT)  ||  (tcell->type == SL_PAGE))
                if((d_options_TN == 'T')  ||  !is_blank_cell(tcell))
                    break;
        }

        /* new number cell */
        if(!(xf_inexpression || xf_inexpression_box || xf_inexpression_line))
        {
            expedit_edit_current_cell_freshcontents(FALSE);
            if(ch < SPACE)
                return;
        }

        /* don't record chars going into number cells */
        chrina(ch, FALSE);

        return;
    }

    /* SKS after PD 4.11 08jan92 - only record chars going into text cells not number cells */
    if(macro_recorder_on)
        output_char_to_macro_file(ch);

    chrina(ch, TRUE);
}

/******************************************************************************
*
* insert character in buffer
*
******************************************************************************/

extern void
chrina(
    uchar ch,
    BOOL allow_check)
{
    S32 length;
    uchar *currpos;

    if(xf_inexpression_box || xf_inexpression_line)
    {
        been_error = expedit_insert_char(ch);
        /*>>>should I set buffer_altered???*/
        return;
    }

    if(!slot_in_buffer)
        return;

    length  = strlen((char *) linbuf);
    currpos = linbuf + lecpos;

    if(lecpos > length)
    {
        /* pad with spaces */
        memset32(linbuf+length, SPACE, (lecpos-length));
        length = lecpos;
        linbuf[length] = CH_NULL;
    }

    if(xf_iovbit)                  /* insert mode, create a space */
    {
        if(length >= MAXFLD)
        {
            (void) mergebuf_nocheck();
            bleep();
            clearkeyboardbuffer();
            return;
        }

        memmove32(currpos+1, currpos, (length-lecpos+1));
    }
    else if(lecpos == length)
    {
        if(length >= MAXFLD)
        {
            bleep();
            been_error = xf_flush = TRUE;
            return;
        }

        linbuf[length+1] = CH_NULL;
    }

    *currpos = ch;
    output_buffer = buffer_altered = TRUE;

    if( dspfld_from == -1)
        dspfld_from = lecpos;

    ++lecpos;

    /* check spelling? */
    if( allow_check  &&  (lecpos >= 2)  &&  check_word_wanted()  &&
        !spell_iswordc(master_dictionary, ch)  &&  spell_iswordc(master_dictionary, linbuf[lecpos-2])  )
            check_word();
}

extern BOOL
insert_string(
    const char *str,
    BOOL allow_check)
{
    optiontype t_xf_iovbit;
    char ch;

    if(xf_inexpression_box || xf_inexpression_line)
    {
        been_error = expedit_insert_string(str);
        return(!been_error);
    }

    t_xf_iovbit = xf_iovbit;
    xf_iovbit = TRUE;               /* Force insert mode */

    while((ch = *str++) != CH_NULL)
    {
        chrina(ch, allow_check);

        if(been_error)
            break;
    }

    xf_iovbit = t_xf_iovbit;

    return(!been_error);
}

extern BOOL
insert_string_check_number(
    const char *str,
    BOOL allow_check)
{
    P_CELL tcell = travel_here();

    for(;;) /* loop for structure */
    {
        if(NULL == tcell)
        {
            /*if(d_options_TN == 'T') SKS 2016-07-27 allow empty 'text' cell to be pasted with number data */
                /*break;*/
        }
        else
        {
            if(is_protected_cell(tcell))
                return(reperr_null(ERR_PROTECTED));

            /* insert the string into text cell iff text cell or blank text cell and text mode */
            if((tcell->type == SL_TEXT)  ||  (tcell->type == SL_PAGE))
                if((d_options_TN == 'T')  ||  !is_blank_cell(tcell))
                    break;
        }

        /* new number cell */
        if(!(xf_inexpression || xf_inexpression_box || xf_inexpression_line))
        {
            expedit_edit_current_cell_freshcontents(FALSE);
        }

        break; /* out of loop for structure */
    }

    return(insert_string(str, allow_check));
}

/******************************************************************************
*
* beginning of cell
*
******************************************************************************/

extern void
StartOfSlot_fn(void)
{
    if(slot_in_buffer)
        lecpos = lescrl = 0;
}

/******************************************************************************
*
* end of cell
*
******************************************************************************/

extern void
EndOfSlot_fn(void)
{
    if(slot_in_buffer)
        lecpos = strlen((char *) linbuf);
}

/******************************************************************************
*
* cursor up
*
******************************************************************************/

extern void
CursorUp_fn(void)
{
    movement = CURSOR_UP;
}

/******************************************************************************
*
* cursor down
*
******************************************************************************/

extern void
CursorDown_fn(void)
{
    movement = CURSOR_DOWN;
}

/******************************************************************************
*
* screen up
*
******************************************************************************/

extern void
PageUp_fn(void)
{
    movement = CURSOR_SUP;
}

/******************************************************************************
*
* screen down
*
******************************************************************************/

extern void
PageDown_fn(void)
{
    movement = CURSOR_SDOWN;
}

/******************************************************************************
*
* screen left
*
******************************************************************************/

extern void
PageLeft_fn(void)
{
    COL tcol;
    coord win_width, fixwidth = 0;
    P_SCRCOL cptr;

    /* find width of unfixed window */

    assert(0 != array_elements(&horzvec_mh));
    cptr = horzvec();
    PTR_ASSERT(cptr);

    while(!(cptr->flags & LAST)  &&  (cptr->flags & FIX))
    {
        fixwidth += colwidth(cptr++->colno);
    }

    win_width = cols_available - fixwidth;

    /* find first column on screen */
    tcol = fstncx();

    /* step back to find column to be first in horzvec */
    for(; (win_width > 0) && (tcol > 0); --tcol)
    {
        while((tcol > 0)  &&  (incolfixes(tcol) || !colwidth(tcol)))
            --tcol;

        win_width -= colwidth(tcol);
    }

    if(tcol >= 0)
    {
        /* go to it and stick it in horzvec() */
        chknlr(tcol, currow);
        filhorz(tcol, tcol);
    }
}

/******************************************************************************
*
* screen right
*
******************************************************************************/

extern void
PageRight_fn(void)
{
    COL new_col;

    new_col = col_off_right();
    chknlr(new_col, currow);
    filhorz(new_col, new_col);
}

/******************************************************************************
*
* next column
*
******************************************************************************/

extern void
NextColumn_fn(void)
{
    movement = CURSOR_NEXT_COL;
}

/******************************************************************************
*
* previous column
*
******************************************************************************/

extern void
PrevColumn_fn(void)
{
    if(!curcoloffset  &&  !fstncx())
    {
        xf_flush = TRUE;
        return;
    }

    movement = CURSOR_PREV_COL;
}

/******************************************************************************
*
* cursor left
*
******************************************************************************/

extern void
CursorLeft_fn(void)
{
    uchar * ptr;
    BOOL    in_protected = !slot_in_buffer  &&  is_protected_cell(travel_here());

    if(!xf_inexpression  &&  (!lecpos  ||  (!slot_in_buffer  &&  !in_protected)))
    {
        internal_process_command(N_PrevColumn);
        return;
    }

    if(lecpos  &&  check_word_wanted()  &&  !in_protected)
        /* check word perhaps */
    {
        ptr = linbuf + lecpos;

        if(spell_iswordc(master_dictionary, *ptr)  &&  !spell_iswordc(master_dictionary, *--ptr))
            check_word();
    }

    if(lecpos)
        --lecpos;
}

/******************************************************************************
*
* cursor right
*
******************************************************************************/

extern void
CursorRight_fn(void)
{
    uchar * ptr;

    if(!slot_in_buffer)
    {
#if 0
    /* SKS after PD 4.11 09jan92 - why were we letting punters move around in protected cells? */
        /* might not have cell in buffer due to protection */
        if(!is_protected_cell(travel_here()))
#endif
        {
            internal_process_command(N_NextColumn);
            return;
        }
        /* if protected, don't try checking word 'cos it's not in linbuf */
    }
    else if(lecpos  &&  check_word_wanted())
        /* check word perhaps */
    {
        ptr = linbuf + lecpos;

        if(!spell_iswordc(master_dictionary, *ptr)  &&  spell_iswordc(master_dictionary, *--ptr))
            check_word();
    }

    if(lecpos <= MAXFLD)
    {
        ++lecpos;

        /* this makes cursor jump to next column at end of current col */
        /* only for mouse cursor module */
        if(!xf_inexpression  &&  (lecpos > (S32) strlen(linbuf)  &&  !lescrl))
            #if 0  /* this is the best way ! */
            NextColumn_fn();
            #else
        {
            if(riscos_fonts)
            {
                #if 0
                /* this is tricky and is not yet implemented */
                char tbuf[PAINT_STRSIZ];
                font_string fs;

                /* get string with font changes and text-at chars */
                font_expand_for_break(tbuf, linbuf);

                fs.s = tbuf;
                fs.x = INT_MAX;
                fs.y = INT_MAX;
                fs.split = -1;
                fs.term = INT_MAX;

                if(!font_complain(font_strwidth(&fs)))
                {

                }
                #endif
            }
            /* are we at end of cell display? */
            else if(lecpos >= chkolp(travel_here(), curcol, currow))
                internal_process_command(N_NextColumn);
        }
            #endif
    }
}

/******************************************************************************
*
* count words in current file or marked block
* only counts words in text cells
* cell references don't count, not even to text cells
*
******************************************************************************/

static BOOL
wordcount_fn_prepare(void)
{
    S32 count = 0;
    char array[LIN_BUFSIZ + 1];
    P_CELL tcell;
    char ch;
    uchar *ptr;
    BOOL inword;

    false_return(dialog_box_can_start());

    /* if no block in this document look through whole file */
    if(MARKER_DEFINED())
        init_marked_block();
    else
        init_doc_as_block();

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        actind_in_block(DOWN_COLUMNS);

        if(tcell->type != SL_TEXT)
            continue;

        /* count words in this cell */

        ptr = tcell->content.text;
        inword = FALSE;

        while((ch = *ptr++) != CH_NULL)
        {
            if(SLRLD1 == ch)
            {
                ptr += COMPILED_TEXT_SLR_SIZE-1;
                continue;
            }

            if(ishighlight(ch))
                continue;

            if(SPACE == ch)
                inword = FALSE;
            else
            {
                if(!inword)
                {
                    inword = TRUE;
                    count++;
                }
            }
        }
    }

    actind_end();

    consume_int(
        xsnprintf(array, elemof32(array),
            (count == 1)
                    ? Zld_one_word_found_STR
                    : Zld_many_words_found_STR,
            count));

    false_return(mystr_set(&d_count[0].textfield, array));

    return(dialog_box_start());
}

extern void
WordCount_fn(void)
{
    if(!wordcount_fn_prepare())
        return;

    if(!dialog_box(D_COUNT))
        return;

    dialog_box_end();
}

/******************************************************************************
*
* insert a space at cursor
*
******************************************************************************/

extern void
InsertSpace_fn(void)
{
    if(insert_string(" ", TRUE))
        internal_process_command(N_CursorLeft);
}

/******************************************************************************
*
* delete char forwards
*
******************************************************************************/

/*
 * delete char forwards worrying about insert on return status
*/

extern void
DeleteCharacterRight_fn(void)
{
    #if defined(FLUSH_VIOLENTLY)
    xf_flush = TRUE;
    #endif

    small_DeleteCharacter_fn();

    if( (lecpos >= (S32) strlen((const char *) linbuf)) &&
        (d_options_IR == 'Y')                            &&
        (currow < numrow - 1)                            )
            internal_process_command(N_JoinLines);

    dspfld_from = lecpos;

    del_chkwrp();
}

extern void
small_DeleteCharacter_fn(void)
{
    if(!slot_in_buffer)
        return;

    output_buffer = TRUE;

    delete_bit_of_line(lecpos, 1, FALSE);
}

/******************************************************************************
*
*  erase character to the left of the insertion point
*
******************************************************************************/

extern void
DeleteCharacterLeft_fn(void)
{
    S32 length;
    uchar *currpos;
    S32 tlecpos;
    ROW trow;

    #if defined(FLUSH_VIOLENTLY)
    xf_flush = TRUE;
    #endif

    /* if moving back over start of line */
    if(lecpos <= 0)
    {
        if((d_options_IR == 'Y')  &&  (currow > 0))
        {
            internal_process_command(N_PrevWord);
            currow--;
            filbuf();
            internal_process_command(N_JoinLines);
            tlecpos = lecpos;
            trow = currow;

            del_chkwrp();

            chknlr(curcol, currow = trow);
            lecpos = tlecpos;
        }
        return;
    }

    if(!slot_in_buffer)
        return;

    length = (S32) strlen((char *) linbuf);
    if(--lecpos >= length)
        return;

    currpos = linbuf + lecpos;

    /* in insert mode move text backwards,
    overtype mode replace with a space */
    if(xf_iovbit)
        memmove32(currpos, currpos + 1, (length-lecpos));
    else
        *currpos = SPACE;

    buffer_altered = output_buffer = TRUE;
    del_chkwrp();
    dspfld_from = lecpos;
}

extern void
Underline_fn(void)
{
    inschr(FIRST_HIGHLIGHT);
}

extern void
Bold_fn(void)
{
    inschr(FIRST_HIGHLIGHT+1);
}

extern void
ExtendedSequence_fn(void)
{
    inschr(FIRST_HIGHLIGHT+2);
}

extern void
Italic_fn(void)
{
    inschr(FIRST_HIGHLIGHT+3);
}

extern void
Subscript_fn(void)
{
    inschr(FIRST_HIGHLIGHT+4);
}

extern void
Superscript_fn(void)
{
    inschr(FIRST_HIGHLIGHT+5);
}

extern void
AlternateFont_fn(void)
{
    inschr(FIRST_HIGHLIGHT+6);
}

extern void
UserDefinedHigh_fn(void)
{
    inschr(FIRST_HIGHLIGHT+7);
}

/******************************************************************************
*
* insert or delete highlights in block
*
******************************************************************************/

enum BLOCK_HIGHLIGHT_PARM
{
    H_INSERT = FALSE,
    H_DELETE = TRUE
};

static void
highlight_words_on_line(
    BOOL delete,
    uchar h_ch)
{
    BOOL last_was_space = TRUE;
    uchar ch;

    lecpos = 0;

    while((ch = linbuf[lecpos]) != CH_NULL)
    {
        if(SLRLD1 == ch)
        {
            lecpos += COMPILED_TEXT_SLR_SIZE;
            continue;
        }

        if(delete)
        {
            if(ch == h_ch)
                delete_bit_of_line(lecpos, 1, FALSE);
            else
                ++lecpos;
        }
        else
        {
            if( ( last_was_space  &&  ((ch != SPACE) &&  ch))   ||
                (!last_was_space  &&  ((ch == SPACE) || !ch))   )
            {
                chrina(h_ch, FALSE);
                last_was_space = !last_was_space;
            }

            ++lecpos;
        }
    }
}

extern void
block_highlight_core(
    BOOL delete,
    uchar h_ch)
{
    BOOL txf_iovbit = xf_iovbit;
    S32 tlecpos = lecpos;
    P_CELL tcell;

    xf_iovbit = TRUE;       /* force insert mode */

    init_marked_block();

    while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
    {
        actind_in_block(DOWN_COLUMNS);

        if((tcell->type != SL_TEXT)  ||  is_blank_cell(tcell))
            continue;

        prccon(linbuf, tcell);      /* decompile to linbuf */
        slot_in_buffer = TRUE;

        highlight_words_on_line(H_DELETE, h_ch);

        if(!delete)
            highlight_words_on_line(H_INSERT, h_ch);

        if(buffer_altered)
        {
            if(!merstr(in_block.col, in_block.row, TRUE, TRUE))
                break;

            out_screen = TRUE;
            filealtered(TRUE);
        }

        slot_in_buffer = FALSE;
    }

    actind_end();

    filbuf();
    lecpos = tlecpos;
    xf_iovbit = txf_iovbit;
}

/******************************************************************************
*
* insert highlights into block
*
******************************************************************************/

extern void
HighlightBlock_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_INSHIGH))
    {
        block_highlight_core(H_INSERT, (uchar) (d_insremhigh[0].option - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* remove highlights from block
*
******************************************************************************/

extern void
RemoveHighlights_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_REMHIGH))
    {
        block_highlight_core(H_DELETE, (uchar) (d_insremhigh[0].option - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* toggle insert/overtype flag
*
******************************************************************************/

extern void
InsertOvertype_fn(void)
{
    xf_iovbit = !xf_iovbit;

    insert_overtype_state_may_have_changed();
}

/******************************************************************************
*
* move to next word in line buffer
* if no next word return false
*
******************************************************************************/

extern BOOL
fnxtwr(void)
{
    S32 length = strlen((char *) linbuf);

    if(lecpos >= length)
        return(FALSE);

    while((lecpos < length)  &&  (linbuf[lecpos] != SPACE))
        lecpos++;

    while((lecpos < length)  &&  (linbuf[lecpos] == SPACE))
        lecpos++;

    return(TRUE);
}

/******************************************************************************
*
* delete word
*
******************************************************************************/

extern void
DeleteWord_fn(void)
{
    S32 templecpos = lecpos;

    if(!fnxtwr()  &&  !xf_inexpression)
    {
        /* delete line join */
        internal_process_command(N_DeleteCharacterRight);
        return;
    }

    delete_bit_of_line(templecpos, lecpos - templecpos, TRUE);

    lecpos = templecpos;

    output_buffer = TRUE;

    del_chkwrp();
}

extern void
delete_bit_of_line(
    S32 stt_offset,
    S32 length,
    BOOL save)
{
    char *dst, *src;
    char ch;
    S32 len = strlen(linbuf);

    if(!slot_in_buffer  ||  (stt_offset >= len))
        return;

    length = MIN(length, len);

    dst = linbuf + stt_offset;
    src = dst + length;

    if(src <= dst)
        return;

    if(save)
    {
        /* save word to paste list */
        ch = dst[length];
        dst[length] = CH_NULL;
        save_words(dst);
        dst[length] = ch;
    }

    buffer_altered = TRUE;

    /* compact up (+1 to copy CH_NULL) */
    memmove32(dst, src, strlen32p1(src));
}

/******************************************************************************
*
* next word
*
******************************************************************************/

extern void
NextWord_fn(void)
{
    if(slot_in_buffer  &&  fnxtwr())
        return;

    if(xf_inexpression)
        return;

    if(currow+1 < numrow)
    {
        lecpos = lescrl = 0;
        internal_process_command(N_CursorDown);
        return;
    }
}

/******************************************************************************
*
* previous word
*
******************************************************************************/

extern void
PrevWord_fn(void)
{
    P_CELL tcell;

    if(slot_in_buffer  &&  (lecpos > 0))
    {
        /* move lecpos back to start of previous word in line buffer */

        while((lecpos > 0)  &&  (linbuf[lecpos-1] == SPACE))
            --lecpos;

        while((lecpos > 0)  &&  (linbuf[lecpos-1] != SPACE))
            --lecpos;

        return;
    }

    if(xf_inexpression  ||  !currow  ||  !mergebuf())
        return;

    /* move to end of previous row */

    tcell = travel(curcol, currow - 1);

    if(tcell  &&  (tcell->type == SL_TEXT))
        lecpos = strlen((char *) tcell->content.text);
    else
        lecpos = lescrl = 0;

    internal_process_command(N_CursorUp);
}

/******************************************************************************
*
* swap case
*
******************************************************************************/

extern void
SwapCase_fn(void)
{
    int ch;

    if(!slot_in_buffer)
        return;

    /* if cursor off end of text move to next line */
    if(lecpos > (S32) strlen(linbuf))
    {
        internal_process_command(N_NextWord);
        return;
    }

    ch = (int) linbuf[lecpos];

    linbuf[lecpos] = (uchar) (isupper(ch)
                                ? tolower(ch)
                                : toupper(ch));

    buffer_altered = output_buffer = TRUE;

    dspfld_from = lecpos;

    if(lecpos <= MAXFLD)
        ++lecpos;
}

extern void
AutoWidth_fn(void)
{
    /* set widths for each column
        look down column:
            if blank, don't touch it
            set width to maximum "kosher cell" display size
            kosher cell is number cell or text cell that has something to its right
    */
    COL tcol;
    BOOL changed = FALSE;
    DOCNO bd;
    SLR bs, be;

    /* save markers */
    bs = blkstart;
    be = blkend;
    bd = blk_docno;

    /* SKS 19feb97 do something sensible when block not in doc for PD 4.50/09 */
    if((blkstart.col == NO_COL) || (blk_docno != current_docno()))
    {
        /* no block so use current column */
        blkstart.col = blkend.col = curcol;
        blk_docno = current_docno();
    }
    else if(!set_up_block(FALSE)) /* SKS 17jan99 amend marked block setup */
        return;

    /* SKS after PD 4.11 01feb92 - widths depend on calced results! */
    ev_recalc_all();

    /* each column in block */
    for(tcol=blkstart.col; tcol <= blkend.col; tcol++)
    {
        coord biggest_width = 0;
        coord biggest_margin = 0;
        P_CELL tcell;
        SLR cs, ce;
        /* coord right_margin; no longer used */
        P_S32 widp, wwidp;

        readpcolvars(tcol, &widp, &wwidp);
        /* right_margin = *wwidp; no longer used */

        cs.col = tcol;
        cs.row = 0;
        ce.col = tcol;
        ce.row = numrow;

        init_block(&cs, &ce);

        while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
        {
            S32 iwidth;
            coord this_width = 0;
            char tjust;
            BOOL nothing_on_right = FALSE;
            char array[LIN_BUFSIZ];
            char *ptr;

            tjust = tcell->justify;
            tcell->justify = J_LEFT;

            switch(tcell->type)
            {
            case SL_TEXT:
                /* if we are not last column and nothing to right, don't bother */
                if(tcol < numcol-1 && is_blank_block(tcol+1, in_block.row, numcol-1, in_block.row))
                    nothing_on_right = TRUE;

                /* deliberate fall thru */

            case SL_NUMBER:
                (void) expand_cell(
                            current_docno(), tcell, in_block.row, array, elemof32(array),
                            DEFAULT_EXPAND_REFS /*expand_refs*/,
                            EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                            EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ |
                            EXPAND_FLAGS_FONTY_RESULT(riscos_fonts) /*allow_fonty_result*/ /*expand_flags*/,
                            TRUE /*cff*/);

                if(riscos_fonts)
                {
                    /* remove trailing spaces CAREFULLY as we may have a multibyte font sequence */
                    BOOL last_was_font_change = FALSE;

                    ptr = array;
                    while(CH_NULL != *ptr)
                    {
                        S32 nchar = font_skip(ptr);

                        last_was_font_change = (nchar != 1);

                        ptr += nchar;
                    }

                    if(!last_was_font_change)
                        while((--ptr >= array) && (*ptr == SPACE))
                            *ptr = CH_NULL;

                    /* SKS after PD 4.11 29jan92 - twiddle in this side for finer
                     * tuning in fonts considering 1/2 chars etc.
                    */
                    iwidth  = font_width(array);
                    iwidth += (charwidth * MILLIPOINTS_PER_OS * 3) / 2;
                    this_width = idiv_ceil_fn(iwidth, charwidth * MILLIPOINTS_PER_OS);
                }
                else
                {
                    /* bug in 4.00 - system font highlights and trailing spaces counted */
                    /* this_width = strlen(array)+2; */
                    /* changed by RJM 18.10.91 */
                    char *aptr;
                    S32 hcount = 0;

                    /* remove trailing spaces */
                    ptr = array + strlen(array);
                    while((--ptr >= array) && (*ptr == SPACE))
                        *ptr = CH_NULL;

                    for(aptr = array ; *aptr != CH_NULL; aptr++)
                    {
                        if(ishighlight(*aptr))
                            hcount++;
                    }

                    /* count back trailing spaces and highlights */
                    for( ; aptr>array; aptr--)
                        if(*(aptr-1) == SPACE) /* wot? */
                            ;
                        else if(ishighlight(*(aptr-1)))
                            hcount--;
                        else
                            break;

                    this_width = aptr-array-hcount +2;
                }
                break;
            default:
                break;
            }
            /* only don't bother with right-most text if it fits in right margin */
            if(nothing_on_right /* && this_width < right_margin */)
            {
                if(this_width > biggest_margin)
                    biggest_margin = this_width;
            }
            else
            {
                if(this_width > biggest_width)
                    biggest_width = this_width;
            }
            tcell->justify = tjust;
        }

        /* got a width for this column */
        if(biggest_width > 0)
        {
            changed = TRUE;
            *widp = biggest_width;
        }

        /* do we need a bigger margin than existing one? */
        if(*wwidp < biggest_margin)
        {
            *wwidp = biggest_margin;
            changed = TRUE;
        }
        /* if margin less than column width, set it to 0 */
        if(*wwidp < *widp)
        {
            *wwidp = 0;
            changed = TRUE;
        }
    }

    /* Examine all columns in sheet, tweek right margins of all linked columns */
    changed |= adjust_all_linked_columns();

    /* better do a redraw, perhaps */
    if(changed)
    {
        xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
        filealtered(TRUE);
    }

    /* restore markers */
    blk_docno = bd;
    blkstart  = bs;
    blkend    = be;
}

/******************************************************************************
*
* check if all widths are zero for columns
* other than between tcol1 and tcol2
*
******************************************************************************/

extern BOOL
all_widths_zero(
    COL tcol1,
    COL tcol2)
{
    COL trycol;

    trace_3(TRACE_APP_PD4, "all_widths_zero(%d, %d): numcol = %d", tcol1, tcol2, numcol);

    for(trycol = 0; trycol < numcol; trycol++)
        if(((trycol < tcol1)  ||  (trycol > tcol2))  &&  colwidth(trycol))
            return(FALSE);

    return(TRUE);
}

static BOOL
do_widths_admin_prepare(
    _InVal_     S32 width)
{
    uchar array[20];

    d_width[0].option = (optiontype) width;

    (void) write_col(array, elemof32(array), curcol);

    false_return(mystr_set(&d_width[1].textfield, array));

    return(dialog_box_start());
}

static BOOL
do_widths_admin_core(
    _OutRef_    P_COL tcol1,
    _OutRef_    P_COL tcol2)
{
    buff_sofar = (uchar *) d_width[1].textfield;

    /* assumes buff_sofar set */
    if((*tcol1 = getcol()) == NO_COL)
        *tcol1 = curcol;

    /* get second column in range */
    if((*tcol2 = getcol()) == NO_COL)
        *tcol2 = *tcol1;

    /* SKS 04.11.91 - stop the damned winges */
    if(*tcol2 > numcol - 1)
       *tcol2 = numcol - 1;

    return(/*(*tcol2 < numcol)  &&*/  (*tcol1 <= *tcol2));
}

static int
columnwidth_fn_core(void)
{
    COL tcol, tcol1, tcol2;
    P_S32 widp, wwidp;
    BOOL badparm = !do_widths_admin_core(&tcol1, &tcol2);

    /* width of zero special */
    if(!badparm  &&  (d_width[0].option == 0))
    {
        /* if setting current column to zero width force cell out of buffer */
        if((curcol >= tcol1)  &&  (curcol <= tcol2))
            if(!mergebuf())
                return(FALSE);

        /* must have some other columns possibly visible */
        badparm = all_widths_zero(tcol1, tcol2);
        trace_1(TRACE_APP_PD4, "all_widths_zero %d", badparm);

        /* can't set fixed cols to zero width */
        if(!badparm)
        {
            tcol = tcol1;
            do  {
                if(incolfixes(tcol))
                    badparm = TRUE;
            }
            while(++tcol <= tcol2);
        }
    }

    if(badparm)
    {
        reperr_null(ERR_BAD_PARM);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    tcol = tcol1;
    do  {
        readpcolvars(tcol, &widp, &wwidp);
        *widp = d_width[0].option;
    }
    while(++tcol <= tcol2);

    /* Examine all columns in sheet, tweek right margins of all linked columns */
    adjust_all_linked_columns();

    /* if the current width now 0 move ahead to first non-zero column */

    for( ; !colwidth(curcol)  &&  (curcol < numcol-1); ++curcol)
        movement = ABSOLUTE;

    /* if there aren't any ahead, move back to one */

    for( ; !colwidth(curcol); --curcol)
        movement = ABSOLUTE;

    newcol = curcol;
    newrow = currow;

    xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
    filealtered(TRUE);

    return(TRUE);
}

static BOOL
columnwidth_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    return(do_widths_admin_prepare(colwidth(curcol)));
}

extern void
ColumnWidth_fn(void)
{
    if(!columnwidth_fn_prepare())
        return;

    while(dialog_box(D_WIDTH))
    {
        int core_res = columnwidth_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

static int
rightmargin_fn_core(void)
{
    COL tcol, tcol1, tcol2;
    P_S32 widp, wwidp;

    if(!do_widths_admin_core(&tcol1, &tcol2))
    {
        reperr_null(ERR_BAD_PARM);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    remove_overlapped_linked_columns(tcol1, tcol2);

    tcol = tcol1;
    do  {
        readpcolvars(tcol, &widp, &wwidp);
        *wwidp = d_width[0].option;
    }
    while(++tcol <= tcol2);

    xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
    filealtered(TRUE);

    return(TRUE);
}

static BOOL
rightmargin_fn_prepare(void)
{
    /* Changed for PD 4.12 by RCM at RJM's request */
    P_S32 widp, wwidp;

    false_return(dialog_box_can_start());

    /* show wrap width, shows zero if right margin is tracking the colwidth */
    readpcolvars(curcol, &widp, &wwidp);

    return(do_widths_admin_prepare(*wwidp));
}

extern void
RightMargin_fn(void)
{
    if(!rightmargin_fn_prepare())
        return;

    while(dialog_box(D_MARGIN))
    {
        int core_res = rightmargin_fn_core();

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
*  WL and WR decrement (left) and increment (right) the current wrap width
*
******************************************************************************/

extern void
MoveMarginLeft_fn(void)
{
    coord wid = colwrapwidth(curcol);

    /* If curcol is linked, don't disturb it. dstwrp takes care not to touch linked columns to our right. */
    if(colislinked(curcol))
        return;

    if(wid > 2)     /* this to follow 6502 which can't get to 1 */
        dstwrp(curcol, wid-1);

    xf_flush = TRUE;
}

extern void
MoveMarginRight_fn(void)
{
    /* If curcol is linked, don't disturb it. dstwrp takes care not to touch linked columns to our right. */
    if(colislinked(curcol))
        return;

    dstwrp(curcol,colwrapwidth(curcol) + 1);

    xf_flush = TRUE;
}

/*
based on AutoWidth_fn() in c.bufedit
*/

extern void
LinkColumns_fn(void)
{
    S32   *widp, *wwidp, *colflagsp;
    COL   tcol;
    coord  right_margin;

    if(!set_up_block(TRUE))
        return;         /* has already reported create_error(ERR_NOBLOCK) */

    remove_overlapped_linked_columns(blkstart.col, blkend.col);

    /* selecting a single column just does remove_overlapped_linked_columns(), and nowt else */

    if(blkstart.col != blkend.col)
    {
        /* more than one column, so link them */

        /* Rightmost column needs a 'LINKSTOP' */

        readpcolvars_(blkend.col, &widp, &wwidp, &colflagsp);

#if TRUE
        *wwidp = 0; /* Mark (and SKS too) wants the right margin to lock to the column width */
#endif

        *colflagsp = LINKED_COLUMN_LINKSTOP | (*colflagsp & ~LINKED_COLUMN_LINKRIGHT);

        right_margin = colwrapwidth(blkend.col);    /* don't use *wwidp, cos may be 0, we want a real margin position */

        /* for each preceeding column, the right margin is just blkend.col's margin plus the */
        /* running total of the column widths (excluding blkend.col's width)                 */

        for(tcol = blkend.col; --tcol >= blkstart.col; )
        {
            readpcolvars_(tcol, &widp, &wwidp, &colflagsp);

            right_margin += *widp;

            *colflagsp = LINKED_COLUMN_LINKRIGHT | (*colflagsp & ~LINKED_COLUMN_LINKSTOP);
            *wwidp     = right_margin;
        }
    }

    /* better do a redraw */
    xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
    filealtered(TRUE);
}

/******************************************************************************
*
* scan all the columns in the sheet, within a group of linked columns, adjust the right-margins to coincide
*
******************************************************************************/

extern BOOL
adjust_all_linked_columns(void)
{
    S32   *widp, *wwidp, *colflagsp;
    COL   tcol;
    coord  right_margin;
    BOOL   changed = FALSE;

    for(tcol = numcol; --tcol >= 0; )
    {
        readpcolvars_(tcol, &widp, &wwidp, &colflagsp);

        if(*colflagsp & LINKED_COLUMN_LINKSTOP)
        {
            right_margin = colwrapwidth(tcol);    /* don't use *wwidp, cos may be 0, we want a real margin position */

            for( ; --tcol >= 0; )
            {
                readpcolvars_(tcol, &widp, &wwidp, &colflagsp);

                if(*colflagsp & LINKED_COLUMN_LINKRIGHT)
                {
                    right_margin += *widp;

                    if(*wwidp != right_margin)
                    {
                        *wwidp  = right_margin;
                        changed = TRUE;
                    }
                }
                else
                {
                    ++tcol;     /* overshot, so restore tcol */
                    break;
                }
            }
        }
    }

    return(changed);
}

extern BOOL
adjust_this_linked_column(
    COL tcol)
{
    UNREFERENCED_PARAMETER(tcol);

    return(adjust_all_linked_columns());
}

/******************************************************************************
*
* tcol is a linked column, return the number of columns in the group
*
******************************************************************************/

extern S32
count_this_linked_column(
    COL tcol)
{
    COL  tcol1, tcol2;
    COL  scanend = numcol - 1;

    tcol1 = tcol2 = tcol;

    while(linked_column_linkright(tcol2) && (tcol2 < scanend))
        ++tcol2;

    /* if pred(tcol1) links to tcol1, find first column in group */

    while((tcol1 > 0) && (linked_column_linkright(tcol1 -1)))
        --tcol1;

    return(tcol2 - tcol1 + 1);
}

/******************************************************************************
*
* break the linkage of any columns that lie within or overlap tcol1..tcol2
*
******************************************************************************/

extern void
remove_overlapped_linked_columns(
    COL tcol1,
    COL tcol2)
{
    P_S32 widp, wwidp, colflagsp;
    COL  scanend = numcol - 1;

    /* Must widen range tcol1..tcol2 to include any linked columns that overlap it */

    /* if tcol2 links to succ(tcol2), find last column in group */

    while(linked_column_linkright(tcol2) && (tcol2 < scanend))
        ++tcol2;

    /* if pred(tcol1) links to tcol1, find first column in group */

    while((tcol1 > 0) && (linked_column_linkright(tcol1 -1)))
        --tcol1;

    for( ; tcol1 <= tcol2; tcol1++)
    {
        readpcolvars_(tcol1, &widp, &wwidp, &colflagsp);
        *colflagsp &= ~(LINKED_COLUMN_LINKRIGHT | LINKED_COLUMN_LINKSTOP);
    }
}

/******************************************************************************
*
* enumerate ranges of linked columns
*
* used by SavLoad when saving linked column constructs
*
******************************************************************************/

extern BOOL
enumerate_linked_columns(
    P_COL scanstart,
    P_COL found_first,
    P_COL found_last)
{
    BOOL  found   = FALSE;
    COL  scanend = numcol - 1;
    COL  tcol;

    for(tcol = *scanstart; tcol <= scanend; tcol++)
    {
        if(colislinked(tcol))
        {
            found        = TRUE;
            *found_first = tcol;

            while(linked_column_linkright(tcol) && (tcol < scanend))
                ++tcol;

            *found_last = tcol++;
            break;
        }
    }

    *scanstart = tcol;
    return(found);
}

/******************************************************************************
*
* mark a range of columns as being linked
*
* used by SavLoad when loading linked column constructs
*
* NB when all ranges have been marked, call
*    adjust_all_linked_columns() to position the right-margins
*
******************************************************************************/

extern void
mark_as_linked_columns(
    COL first,
    COL last)
{
    S32   *widp, *wwidp, *colflagsp;
    COL   tcol;

    if((first < 0) || (last >= numcol) || (first >= last))
        return; /* naff range */

    remove_overlapped_linked_columns(first, last);

    /* rightmost column needs a 'LINKSTOP' */
    readpcolvars_(last, &widp, &wwidp, &colflagsp);
    *colflagsp = (*colflagsp & ~LINKED_COLUMN_LINKRIGHT) | LINKED_COLUMN_LINKSTOP;

    /* each preceeding column in the range needs a 'LINKRIGHT' */
    for(tcol = last; --tcol >= first; )
    {
        readpcolvars_(tcol, &widp, &wwidp, &colflagsp);
        *colflagsp = (*colflagsp & ~LINKED_COLUMN_LINKSTOP) | LINKED_COLUMN_LINKRIGHT;
    }
}

/******************************************************************************
*
* fill linbuf with words from colword
* returns TRUE if it has done some formatting
* startrow is the row to at which insertions should be
* made in other columns for iow
*
******************************************************************************/

static BOOL
fill_linbuf(
    ROW startrow)
{
    char buffer[LIN_BUFSIZ];
    const uchar * from;
    uchar * to;
    U8 ch;
    S32 wordlen;
    P_CELL tcell;
    coord splitpoint;
    EV_DOCNO docno;

    firstword = TRUE;

    /* colword() sets up lastword to point to next word */
    while(colword())
    {
        firstword = FALSE;

        /* copy word and spaces to temp buffer: any cell references need decompiling */
        from = lastword;
        to = buffer;
        while(((ch = *from++) != CH_NULL)  &&  (ch != SPACE))
        {
            if(SLRLD1 == ch)
            { /* decompile compiled cell reference */
                const uchar * csr = from + 1; /* CSR is past the SLRLD1/2 */
                COL tcol;
                ROW trow;

                from = talps_csr(csr, &docno, &tcol, &trow);
                /*eportf("fill_linbuf: decompiled CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);*/

                to += write_ref(to, elemof32(buffer) - (to - buffer), docno, tcol, trow);

                continue;
            }

            *to++ = ch;
        }

        /* and spaces after word */
        while(ch == SPACE)
        {
            *to++ = ch;
            ch = *from++;
        }

        --from; /* retract to point to CH_NULL or first character after a SPACE */
        *to = CH_NULL;

        wordlen = strlen(buffer);
        trace_2(TRACE_APP_PD4, "fill_linbuf received word '%s' of length %d", buffer, wordlen);

        /* don't join words together from joined lines */
        to = linbuf + lecpos;
        if(lecpos > 0  &&  *(to - 1) != SPACE)
        {
            lecpos++;
            *to++ = SPACE;
        }

        if(wordlen > LIN_BUFSIZ - lecpos)
        {
            trace_0(TRACE_APP_PD4, "fill_linbuf got a word that is too long!");
            *to = CH_NULL;
            splitpoint = lecpos;
        }
        else
        {
            /* update global if we've taken word */
            lastword = (uchar *) from;

            strcpy(to, buffer);
            lecpos += wordlen;

            ++word_out;

            splitpoint = fndlbr(curr_outrow);
            trace_1(TRACE_APP_PD4, "fill_linbuf got splitpoint %d", splitpoint);
        }

        if(splitpoint > 0)
        {
            new_just =
                (d_options_JU == 'Y')
                    ? (cur_leftright ? J_LEFTRIGHT : J_RIGHTLEFT)
                    : J_FREE;

            /* this line too long so merge it in */
            if(!mergeinline(splitpoint))
                return(TRUE);

            tcell = travel(curcol, curr_outrow - 1);

            /* set to justify perhaps */
            if((d_options_JU == 'Y')  &&  ((tcell->justify & J_BITS) == J_FREE))
            {
                /* set new justify status, keeping protected bit */
                tcell->justify = (tcell->justify & CLR_J_BITS) | new_just;
                cur_leftright = !cur_leftright;
            }
        }
    }

    assert(lecpos + 1 < elemof32(linbuf));
    linbuf[lecpos + 1] = CH_NULL;
    new_just = J_FREE;

    trace_2(TRACE_APP_PD4, "fill_linbuf settled on linbuf '%s', lecpos %d", linbuf, lecpos);

    if(firstword)
        /* no words to format */
        return(FALSE);
    else
        /* don't set to justify cos last line in paragraph */
        mergeinline(lecpos);

    /* must output below here if a registration difference */
    if(lindif)
    {
        mark_to_end(currowoffset);
        out_rebuildvert = TRUE;
    }
    else
        reset_numrow();

    trace_0(TRACE_APP_PD4, "fill_linbuf about to do adjustment after reformat");

    /* if insert on wrap, need to insert lindif rows in other columns */
    if(lindif > 0)
    {
        if(d_options_IW == 'R')
        {
            COL tcol;

            for(tcol = 0; tcol < numcol; tcol++)
            {
                S32 i;

                if(tcol != curcol)
                    for(i = 0; i < lindif; i++)
                        insertslotat(tcol, startrow);
            }

            /* update numrow */
            reset_numrow();

            /* update for insert in all columns */
            updref(0, startrow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 0, (ROW) lindif, UREF_UREF, DOCNO_NONE);
        }
        else
        {
            /* update numrow */
            reset_numrow();

            /* update for insert in this column only */
            updref(curcol, startrow, curcol, LARGEST_ROW_POSSIBLE, 0, (ROW) lindif, UREF_UREF, DOCNO_NONE);
        }
    }
    /* paragraph is now shorter so fill in some lines */
    else
    {
        /* only pad with blank rows if insert on wrap is rows */
        if(d_options_IW == 'R')
        {
            for( ; lindif < 0; lindif++)
            {
                BOOL rowblank = TRUE;
                COL tcol;

                /* try not to pad if rest of line is blank */
                for(tcol = 0; tcol < numcol; tcol++)
                {
                    if((tcol != curcol) && !is_blank_cell(travel(tcol, curr_outrow)))
                    {
                        rowblank = FALSE;
                        break;
                    }
                }

                if(rowblank)
                {
                    for(tcol = 0; tcol < numcol; tcol++)
                    {
                        if(tcol != curcol)
                            killslot(tcol, curr_outrow);
                    }

                    /* anything pointing to deleted cells in this row become bad */
                    updref(0, currow,     LARGEST_COL_POSSIBLE, currow,               BADCOLBIT, (ROW) 0, UREF_DELETE, DOCNO_NONE);

                    /* rows in all those columns move up */
                    updref(0, currow + 1, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, (COL) 0, (ROW) -1, UREF_UREF, DOCNO_NONE);

                    mark_row(currowoffset - 1);
                }
                else
                    insertslotat(curcol, curr_outrow);
            }

            reset_numrow();
        }
    }

    trace_1(TRACE_APP_PD4, "fill_linbuf numrow: %d", numrow);
    return(TRUE);
}

/******************************************************************************
*
* merge in line from linbuf taking care of curr_inrow
* line contains words too many so only write out up to the split point
* and then copy extra words to start of linbuf
*
******************************************************************************/

static BOOL
mergeinline(
    coord splitpoint)
{
    uchar *from, *to;
    U8 splitch;
    ROW temp_row;

    splitch = linbuf[splitpoint];
    linbuf[splitpoint] = CH_NULL;
    insertslotat(curcol, curr_outrow);
    curr_inrow++;
    lindif++;

    buffer_altered = slot_in_buffer = TRUE;
    temp_row = currow;
    currow = curr_outrow;
    if(!mergebuf_nocheck())
        return(FALSE);
    currow = temp_row;

    /* move to new row */
#if 0
    currow++;
#endif
    curr_outrow++;
    linbuf[splitpoint] = splitch;

    trace_5(TRACE_APP_PD4, "currow: %d, inrow: %d, outrow: %d, old_just: %d, new_just: %d",
            currow, curr_inrow, curr_outrow,
            old_just, new_just);

    /* work out if line was actually altered */
    if( (old_flags & SL_ALTERED)    ||
        !(!lindif                               &&
         (curr_inrow == curr_outrow)            &&
         (word_in <= 1)                         &&
         (word_out - word_in == last_word_in)   &&
         (old_just == new_just)                 )
                                                    )
    {
        mark_slot(travel(curcol, curr_outrow - 1));
    }

    /* move word left at end of line back to start */
    from = linbuf + splitpoint;
    to = linbuf;

    /* set word out counter */
    if(*from)
        word_out = 1;
    else
        word_out = 0;

    while(CH_NULL != *from)
    {
        if(SLRLD1 == (*to++ = *from++))
        { /* copy the rest of the CSR over */
            memmove32(to, from, COMPILED_TEXT_SLR_SIZE-1); /* NB may overlap! */
            to += COMPILED_TEXT_SLR_SIZE-1;
            from += COMPILED_TEXT_SLR_SIZE-1;
        }
    }

    *to = CH_NULL;

    lecpos = to - linbuf;
    return(TRUE);
}

/******************************************************************************
*
* collect word from cell or next cell
* return address of word or NULL if break in formatting
* enters with lastword set to NULL the first time,
* points to space after last word read subsequently
*
******************************************************************************/

static uchar *
colword(void)
{
    P_CELL sl;
    uchar *next_word;

    if(!lastword)
        next_word = word_on_new_line();
    else
    {
        /* look for another word on same line */
        while(*lastword == SPACE)
            lastword++;

        if(*lastword)
            next_word = lastword;
        else
        {
            /* no more words on this line so delete the cell */
            if((sl = travel(curcol, curr_inrow)) != NULL)
            {
                old_just = (sl->justify & J_BITS);
                old_flags = sl->flags;
            }

            killslot(curcol, curr_inrow);

            /* was_curr_inrow maintains the row where the next cell first came from */
            was_curr_inrow++;

            lindif--;
            next_word = word_on_new_line();
        }
    }

    if(next_word)
        ++word_in;

    return(next_word);
}

static uchar *
word_on_new_line(void)
{
    P_CELL tcell = travel(curcol, curr_inrow);

    last_word_in = word_in;
    word_in = 0;

    if(!tcell  ||  !chkcfm(tcell, was_curr_inrow))
        return(NULL);

    memcpy32(lastwordbuff, tcell->content.text, slotcontentssize(tcell)); /* includes terminator */
    lastword = lastwordbuff;

    return(((*lastword == SPACE  &&  !firstword)  ||  *lastword == CH_NULL)
                ? NULL
                : lastword);
}

static BOOL
do_FormatParagraph(void)
{
    if(d_options_WR != 'Y')
    {
        reperr_null(ERR_CANTWRAP);
        return(FALSE);
    }

    /* if can't format, just move cursor down */
    if(!chkcfm(travel_here(), currow))
    {
        internal_process_command(N_CursorDown);
        return(TRUE);
    }

    init_colword(currow);
    fill_linbuf(currow + 1);
    chknlr(curcol, (curr_outrow >= numrow)
                            ? numrow - 1
                            : curr_outrow);
    return(TRUE);
}

extern void
FormatBlock_fn(void)
{
    COL tcol = curcol;
    ROW trow = currow;
#ifdef RJM_DEBUG
    COL startcol;
#endif

    /* SKS for PD 4.50 do something sensible when no marked block in this doc */
    if((NO_COL == blkstart.col) || (blk_docno != current_docno()))
    {
        FormatParagraph_fn();
        return;
    }

    if(!set_up_block(TRUE))
        return;

    init_marked_block();

#ifdef RJM_DEBUG
    startcol = end_bl.col;
#endif

    for(;next_cell_in_block(DOWN_COLUMNS);)
    {
        curcol = in_block.col;
        currow = in_block.row;

        filbuf();

        if(!do_FormatParagraph())
            break;

        if(movement == CURSOR_DOWN)
        {
            currow++;
        }
        else if(movement)
        {
            if(newrow <= currow)
                newrow = currow+1;

            currow = newrow;
        }
        else
            break;

        /* update block movements to new row */
        in_block.row = currow-1;

        /* display activity indicator */
        actind_in_block(ACROSS_ROWS /* another bluff, but looks better */);

#ifdef RJM_DEBUG
        if(end_bl.col != startcol)
        {
            COL debug_col = end_bl.col;

            reperr_null(ERR_BAD_PARM);
            break;
        }
#endif
    }

    actind_end();

    chknlr(tcol, trow);
    out_screen = TRUE;
}

/******************************************************************************
*
* format paragraph
*
******************************************************************************/

extern void
FormatParagraph_fn(void)
{
    do_FormatParagraph();
}

/******************************************************************************
*
* reformat from this line to end of paragraph, leaving cursor where it is
*
******************************************************************************/

static void
reformat_with_steady_cursor(
    coord splitpoint,
    S32 nojmp)
{
    S32 tlecpos;
    ROW trow;

    if(!mergebuf_nocheck())
        return;

    /* save current row and position */
    trow = currow;
    tlecpos = lecpos;

    init_colword(currow);
    fill_linbuf(currow + 1);

    /* if cursor was past line break, move down to next line,
    subtracting size of split part to get new position */
    if(tlecpos >= splitpoint && !nojmp)
    {
        /* this does not cater for weird case
        where new word may jump two lines */
        lecpos = tlecpos - splitpoint;
        ++trow;
    }
    else
        /* otherwise, stay where we were */
        lecpos = tlecpos;

    chknlr(curcol, trow);
}

/*
just done a deletion so try a reformat
*/

static void
del_chkwrp(void)
{
    ROW nextrow;
    P_CELL sl;
    S32 splitpoint;
    S32 wrapwidth;
    char *c;
    char tbuf[PAINT_STRSIZ];
    S32 word_wid;

    /* do a reformat if insert on return */
    if( (d_options_WR != 'Y')  ||
        (d_options_IR != 'Y')  ||
        xf_inexpression  ||
        !chkcfm(travel_here(), currow) )
            return;

    /* get length of line */
    if((splitpoint = fndlbr(currow)) == 0)
        return;

    /* if there is no split on the line, check to
     * see if we can add the first word on the next line
    */
    if(splitpoint < 0)
    {
        nextrow = currow + 1;

        sl = travel(curcol, nextrow);

        if(sl  &&  chkcfm(sl, nextrow))
        {
            wrapwidth = chkolp(travel_here(), curcol, currow);

            if(riscos_fonts)
            {
                /* get string with font changes and text-at chars */
                font_expand_for_break(tbuf, sl->content.text);
                c = tbuf;

                while(*c && *c != SPACE)
                    c += font_skip(c);

                *c = CH_NULL;
                word_wid = font_width(tbuf);

                wrapwidth *= charwidth * millipoints_per_os_x;

                if((wrapwidth - -splitpoint) < word_wid)
                    return;
            }
            else
            {
                c = sl->content.text;

                while((CH_NULL != *c) && (SPACE != *c))
                {
                    if(SLRLD1 == *c)
                        c += COMPILED_TEXT_SLR_SIZE;
                    else
                        c++;
                }

                word_wid = c - sl->content.text;

                if((wrapwidth - -splitpoint) < word_wid)
                    return;
            }
        }
        else
            return;

        /* cancel any hope of a split */
        splitpoint = 0;
    }

    reformat_with_steady_cursor(splitpoint, TRUE);
}

/******************************************************************************
*
* check for and do wrap
*
******************************************************************************/

extern void
chkwrp(void)
{
    ROW row;
    coord splitpoint;

    row = currow;

    if((d_options_WR != 'Y')  ||  !chkcfm(travel_here(), row))
        return;

    if((splitpoint = fndlbr(row)) > 0)
        reformat_with_steady_cursor(splitpoint, FALSE);
}

/******************************************************************************
*
* check can format cell
*
******************************************************************************/

static BOOL
chkcfm(
    P_CELL tcell,
    ROW trow)
{
    if(xf_inexpression  ||  chkrpb(trow))
        return(FALSE);

    if(!tcell)
        return(!str_isblank(linbuf));

    if(tcell->type != SL_TEXT)
        return(FALSE);

    /* note: if protected bit set will return false */
    switch(tcell->justify)
    {
    case J_FREE:
    case J_LEFTRIGHT:
    case J_RIGHTLEFT:
        break;

    default:
        return(FALSE);
    }

    /* don't allow database reformat, hopefully
        no reformat on lines below the first if something to left or right
    */
    if( (d_options_IW == 'R')  &&  (trow != currow)  &&  (
        !is_blank_block(0,        trow, curcol-1, trow) ||
        !is_blank_block(curcol+1, trow, numcol-1, trow) ) )
        return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* determine whether string consists of anything but spaces
*
******************************************************************************/

_Check_return_
extern BOOL
str_isblank(
    _In_opt_z_  PC_USTR str)
{
    U8 ch;

    if(!str)
        return(TRUE);

    do
        ch = *str++;
    while(ch == SPACE);

    return(ch == CH_NULL);
}

/******************************************************************************
*
* find line break point in linbuf
* needs to take account of highlights and text-at fields
*
* MRJC modified 7.7.89 for fonts
*
******************************************************************************/

static coord
fndlbr(
    ROW row)
{
    uchar *tptr;
    uchar ch;
    const uchar text_at_char = get_text_at_char();
    S32 width;
    coord startofword, wrapwidth;
    char tbuf[PAINT_STRSIZ], *break_point, *c;
    S32 split_count, had_space;

    wrapwidth = chkolp(travel(curcol, row), curcol, row) - 1;

    if(riscos_fonts)
    {
        /* get string with font changes and text-at chars */
        font_expand_for_break(tbuf, linbuf);

        split_count = font_line_break(tbuf, wrapwidth);

        if(split_count < 0)
            return(split_count);
        else if(split_count == 0)
            /* look for first gap to break line */
            split_count = 1;

        /* find that gap at which we broke */
        had_space = FALSE;
        break_point = linbuf;

        while(split_count--  &&  *break_point)
        {
            /* skip over word */
            while(((ch = *break_point++) != CH_NULL)  &&  (ch != SPACE))
                ;

            if(ch != SPACE)
                --break_point;
            else
                had_space = TRUE;
        }

        /* find end of gap */
        while(*break_point == SPACE)
        {
            had_space = TRUE;
            ++break_point;
        }

        /* if we found a gap, and we didn't get stuck at the line end, break */
        return(had_space && *break_point ? break_point - linbuf : 0);
    }

    /* skip leading spaces */
    width = 0;
    tptr = linbuf;
    while(*tptr++ == SPACE)
        ++width;
    --tptr;

    /* for each word */
    while(*tptr)
    {
        startofword = tptr - linbuf;

        /* walk past word - updating for text-at fields */
        while(((ch = *tptr) != CH_NULL)  &&  (ch != SPACE))
        {
            if((text_at_char == ch) /*no need to test (CH_NULL != text_at_char)*/)
            {
                /* length of text-at field is number of trailing text-at chars */
                tptr++;
                switch(toupper(*tptr))
                {
                case SLRLD1:
                    tptr += COMPILED_TEXT_SLR_SIZE;
                    break;

                case 'N': /* now */
                case 'L': /* leafname */
                case 'D':
                case 'T':
                case 'P':
                    if(*++tptr != text_at_char)
                        tptr -= 2;
                    break;

                /* a rather simplistic consume here */
                case 'F': /* RJM added this 7.9.91 cos it seemed like a good idea */
                case 'G':
                    /* skip over internal part */
                    c = tptr;
                    while(((ch = *c++) != CH_NULL)  &&  (ch != text_at_char))
                        ;
                    if(!ch)
                        break;
                    --c;
                    /* skip over text-at chars - GF have zero width */
                    while(((ch = *c++) != CH_NULL)  &&  (ch == text_at_char))
                        ;
                    tptr = --c;
                    break;

                default:
                    if(*tptr == text_at_char) /* case text_at_char: */
                        /* n text-at chars to start yield (n-1) text-at chars out */
                        break;

                    /* is it macro parameter? */
                    if(isdigit(*tptr))
                        while(isdigit(*tptr))
                            ++tptr;
                    else
                        /* don't recognize following
                         * so count text-at char anyway
                        */
                        --tptr;
                    break;
                }

                /* read past last text-at chars, updating width */
                while(*tptr++ == text_at_char)
                    ++width;
                --tptr;
            }
            else
            {
                if(!ishighlight(ch))
                    ++width;

                if(SLRLD1 == ch) /* have to cater for orphaned SLRLD1 (text_at_char changed) */
                    tptr += COMPILED_TEXT_SLR_SIZE;
                else
                    ++tptr;
            }
        }

        /* end of word too far over so return offset of start of word */
        trace_3(TRACE_APP_PD4, "fndlbr width: %d, wrapwidth: %d, tptr: %s",
                width, wrapwidth, tptr);

        if((width > wrapwidth)  &&  startofword)
            return(startofword);

        /* walk past spaces */
        while(*tptr++ == SPACE)
            ++width;
        --tptr;
    }

    return(-width);
}

/******************************************************************************
*
* calculate the line break point of a string
*
******************************************************************************/

static S32
font_line_break(
    char *str,
    S32 wrap_point)
{
    font_string fs;
    GR_MILLIPOINT wrap_mp;
    S32 res;

    wrap_mp = (wrap_point * charwidth) * millipoints_per_os_x;

    fs.s = str;
    fs.x = wrap_mp;
    fs.y = INT_MAX;
    fs.split = SPACE;
    fs.term = INT_MAX;

    trace_2(TRACE_APP_PD4, "font_line_break str: '%s', wrap: %d: ", str, wrap_mp);

    if(font_complain(font_strwidth(&fs)))
        return(-1);

    /* check if we stopped before the end of the line */
    if(!fs.term  ||  !str[fs.term])
        res = -fs.x;
    else
        res = fs.split + 1;

    trace_5(TRACE_APP_PD4, "width: %d, split: %d, break: %d, '%s', res = %d", fs.x, fs.split, fs.term, str + fs.term, res);
    return(res);
}

/*
initialise word collector
*/

static void
init_colword(
    ROW trow)
{
    P_CELL sl;

    was_curr_inrow =
    curr_inrow = curr_outrow = trow;
    out_offset = 0;
    lastword = NULL;
    lindif = 0;
    last_word_in = word_in = word_out = 0;
    old_just = 0;
    old_flags = 0;
    lecpos = 0;
    dspfld_from = -1;

    /* set up initial left/right state */
    sl = travel(curcol, trow);
    cur_leftright = sl ? ((sl->justify & J_BITS) == J_LEFTRIGHT) : TRUE;
}

/* end of bufedit.c */
