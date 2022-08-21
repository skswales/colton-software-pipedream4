/* spellchk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spell checking */

/* SKS Sep 2016 split off from browse.c */

#include "common/gflags.h"

#include "datafmt.h"

#include "cmodules/spell.h"

/*
internal functions
*/

static BOOL
not_in_user_dicts_or_list(
    const char *word);

static BOOL
worth_trying_again(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*inout*/);

/******************************************************************************
*
*  switch auto checking on or off
*
******************************************************************************/

extern void
AutoSpellCheck_fn(void)
{
    d_auto[0].option = (optiontype) (d_auto[0].option ^ ('Y' ^ 'N'));

    check_state_changed();
}

/* be generous as to what letters one allows into the spelling buffers */
/* - fault foreign words at adding stage */

/* is character valid as the first in a word? */

static S32
browse_valid_1(
    _InVal_     DICT_NUMBER dict,
    char ch)
{
    return(spell_valid_1(dict, ch)  ||  isalpha(ch));
}

/* SKS believes even with new spelling checker one should allow anything that is
 * isalpha() in the current locale to be treated as part of a word so we can at least
 * add silly words that aren't quite valid to local misspellings list or replace them
 * wholly in a check/browse.
*/

static S32
browse_iswordc(
    _InVal_     DICT_NUMBER dict,
    char ch)
{
    return(spell_iswordc(dict, ch)  ||  isalpha(ch));
}

/******************************************************************************
*
* check word at current position in linbuf
*
******************************************************************************/

extern BOOL
check_word_wanted(void)
{
    if((d_auto[0].option != 'Y')  ||  xf_inexpression  ||  !slot_in_buffer)
        return(FALSE);

    if(err_open_master_dict())
    {
        d_auto[0].option = 'N';
        return(FALSE);
    }

    return(TRUE);
}

extern void
check_word(void)
{
    char array[LIN_BUFSIZ];
    S32 res, dict;
    BOOL tried_again;

    if(!check_word_wanted())
        return;

    dict = master_dictionary;

    if(get_word_from_line(dict, array, lecpos, NULL))
    {
        tried_again = FALSE;

    TRY_AGAIN:

        res = spell_checkword(dict, array);

        if(SPELL_ERR_BADWORD == res)
            res = 0;

        if(res < 0)
        {
            reperr_null(res);
        }
        else if(!res  &&  not_in_user_dicts_or_list(array))
        {
            if(!tried_again)
            {
                if(worth_trying_again(dict, array))
                {
                    tried_again = TRUE;
                    goto TRY_AGAIN;
                }
            }

            bleep();
        }
    }
}

/******************************************************************************
*
*  check word is not in the user dictionaries
*
******************************************************************************/

static BOOL
not_in_user_dicts_or_list(
    const char *word)
{
    PC_LIST lptr;

    for(lptr = first_in_list(&first_spell);
        lptr;
        lptr = next_in_list(&first_spell))
    {
        if(0 == C_stricmp((char *) lptr->value, word))
            return(FALSE);
    }

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
    {
        S32 res;

        if((res = spell_checkword((S32) lptr->key, (char *) word)) > 0)
            return(FALSE);

        if((res < 0)  &&  (res != create_error(SPELL_ERR_BADWORD)))
            reperr_null(res);
    }

    return(TRUE);
}

#define C_BLOCK 0
#define C_ALL 1

/*
 * set up the block to check
*/

static BOOL
set_check_block(void)
{
    /* set up the check block */

    if(d_checkon[C_BLOCK].option == 'Y')
    {
        /* marked block selected - check exists */
        if(!MARKER_DEFINED())
            return(reperr_null((blkstart.col != NO_COL)
                                        ? create_error(ERR_NOBLOCKINDOC)
                                        : create_error(ERR_NOBLOCK)));

        sch_stt = blkstart;
        sch_end = (blkend.col != NO_COL) ? blkend : blkstart;
    }
    else
    {
        /* all rows, maybe all files */

        sch_stt.row = 0;
        sch_end.row = numrow-1;

        sch_stt.col = 0;
        sch_end.col = numcol-1;
    }

    /* set up the initial position */

    sch_pos_stt.col = sch_stt.col - 1;
    sch_pos_stt.row = sch_stt.row;
    sch_stt_offset  = -1;

    return(TRUE);
}

/*
 * readjust sch_stt_offset at start of next word on line
 * assumes line in buffer
*/

static BOOL
next_word_on_line(void)
{
    S32 len;
    PC_U8 ptr;
    DICT_NUMBER dict;

    len = strlen(linbuf);

    if(sch_stt_offset >= len)
        return(FALSE);

    /* if pointing to a word, skip it */
    ptr  = linbuf + sch_stt_offset;
    dict = master_dictionary;

    if(browse_valid_1(dict, *ptr++))
        do
            ;
        while(browse_iswordc(dict, *ptr++));

    sch_stt_offset = (--ptr) - linbuf;

    while(!browse_valid_1(dict, *ptr)  &&  (sch_stt_offset < len))
    {
        ++ptr;
        ++sch_stt_offset;
    }

    return(sch_stt_offset < len);
}

/*
 * find the next mis-spelt word starting with the current word
*/

static BOOL
get_next_misspell(
    char *array /*out*/)
{
    S32 res, dict;
    P_CELL tcell;
    BOOL tried_again;

    trace_1(TRACE_APP_PD4, "get_next_misspell(): sch_stt_offset = %d\n]", sch_stt_offset);

    dict = master_dictionary;

    for(;;)
    {
        if((sch_stt_offset == -1)  ||  !slot_in_buffer)
        {
            if(sch_stt_offset == -1)
            {
                /* (NB. set sch_pos_stt.col = sch_stt.col - 1 to start) */

                trace_0(TRACE_APP_PD4, "get_next_misspell: offset == -1 -> find another cell to scan for misspells");

                do  {
                    if(sch_pos_stt.col < sch_end.col)
                    {
                        ++sch_pos_stt.col;
                        trace_1(TRACE_APP_PD4, "get_next_misspell stepped to column %d", sch_pos_stt.col);
                    }
                    else if(sch_pos_stt.row < sch_end.row)
                    {
                        ++sch_pos_stt.row;
                        sch_pos_stt.col = sch_stt.col;
                        trace_2(TRACE_APP_PD4, "get_next_misspell stepped to row %d, reset to column %d", sch_pos_stt.row, sch_pos_stt.col);
                        actind((S32) ((100 * sch_pos_stt.row - sch_stt.row) / (sch_end.row - sch_stt.row + 1)));
                        if(ctrlflag)
                            return(FALSE);
                    }
                    else
                    {
                        trace_0(TRACE_APP_PD4, "get_next_misspell ran out of cells");
                        return(FALSE);
                    }

                    tcell = travel(sch_pos_stt.col, sch_pos_stt.row);
                }
                while(!tcell  ||  (tcell->type != SL_TEXT)  ||  str_isblank(tcell->content.text));

                sch_stt_offset = 0;
            }
            else
            {
                trace_0(TRACE_APP_PD4, "get_next_misspell found no cell in buffer so reload");
                tcell = travel(sch_pos_stt.col, sch_pos_stt.row);
            }

            prccon(linbuf, tcell);
        }

        /* if current word not ok set variables */

        if(browse_valid_1(dict, linbuf[sch_stt_offset]))
        {
            tried_again = FALSE;

            slot_in_buffer = TRUE;
            (void) get_word_from_line(dict, array, sch_stt_offset, NULL);
            slot_in_buffer = FALSE;

        TRY_AGAIN:

            res = spell_checkword(dict, array);

            if(res == create_error(SPELL_ERR_BADWORD))
                res = 0;

            if(res < 0)
                return(reperr_null(res));

            if(!res  &&  (not_in_user_dicts_or_list(array)))
            {
                /* can't find it anywhere: try stripping off trailing ' or 's */

                if(!tried_again)
                    if(worth_trying_again(dict, array))
                    {
                        tried_again = TRUE;
                        goto TRY_AGAIN;
                    }

                /* not in any dictionary, with or without quote: move there! */
                /* chknlr() sets the state to be picked up by a draw_screen() */
                trace_2(TRACE_APP_PD4, "get_next_misspell moving to %d, %d", sch_pos_stt.col, sch_pos_stt.row);
                chknlr(sch_pos_stt.col, sch_pos_stt.row);
                return(TRUE);
                }
        }

        /* go to next word if available, else reload from another cell */
        if(!next_word_on_line())
            sch_stt_offset = -1;
    }
}

/******************************************************************************
*
*  delete the words on the mis-spellings list
*
******************************************************************************/

extern void
del_spellings(void)
{
    delete_list(&first_spell);
}

/******************************************************************************
*
*  get the word on the line currently, or previous one
*  this allows valid chars and some that might not be
*
******************************************************************************/

extern P_U8
get_word_from_line(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*out*/,
    S32 stt_offset,
    P_S32 found_offsetp)
{
    P_U8 to = array;
    S32 len;
    PC_U8 src, ptr;
    S32 found_offset = stt_offset;

    trace_2(TRACE_APP_PD4, "get_word_from_line(): linbuf '%s', stt_offset = %d", linbuf, stt_offset);

    if(is_current_document()  &&  !xf_inexpression  &&  slot_in_buffer)
    {
        src = linbuf;
        len = strlen(src);
        ptr = src + MIN(len, stt_offset);

        /* goto start of this or last word */

        /* skip back until a word is hit */
        while((ptr > src)  &&  !browse_iswordc(dict, *ptr))
            --ptr;

        /* skip back until a valid word start is hit */
        while((ptr > src)  &&  browse_iswordc(dict, *(ptr-1)))
            --ptr;

        /* words must start with a letter from the current character set
         * as we don't know which dictionary will be used
        */
        while(browse_iswordc(dict, *ptr)  &&  !browse_valid_1(dict, *ptr))
            ++ptr;

        if(browse_valid_1(dict, *ptr))
        {
            found_offset = ptr - src;

            while(browse_iswordc(dict, *ptr)  &&  ((to - array) < MAX_WORD))
                *to++ = (S32) *ptr++;
        }
    }

    *to = CH_NULL;

    if(found_offsetp)
        *found_offsetp = found_offset;

    trace_2(TRACE_APP_PD4, "get_word_from_line returns '%s', found_offset = %d",
            trace_string((*array != CH_NULL) ? array : NULL), found_offsetp ? *found_offsetp : -1);

    return((*array != CH_NULL) ? array : NULL);
}

/******************************************************************************
*
* determine whether word is worth having another bash at for spelling
*
******************************************************************************/

static BOOL
worth_trying_again(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*inout*/)
{
    P_U8 ptr;
    BOOL res = FALSE;

    /* english: try stripping off trailing ' or 's */
    ptr = array + strlen(array);
    if( (*--ptr == '\'')  ||
        ((spell_tolower(dict, *ptr) == 's')  &&  (*--ptr == '\'')))
    {
        *ptr = CH_NULL;
        res = TRUE;
    }

    return(res);
}

#define C_CHANGE 0
#define C_BROWSE 1
#define C_ADD    2

/******************************************************************************
*
*  check current document
*
******************************************************************************/

static void
checkdocument_do_report(
    _InVal_     S32 mis_spells,
    _InVal_     S32 words_added)
{
    char buffer1[MAX_WORD+1];
    char buffer2[MAX_WORD+1];

    /* put up message box saying what happened */
    consume_int(
        xsnprintf(buffer1, elemof32(buffer1),
            (mis_spells == 1)
                    ? Zd_unrecognised_word_STR
                    : Zd_unrecognised_words_STR,
            mis_spells));

    consume_int(
        xsnprintf(buffer2, elemof32(buffer2),
            (words_added == 1)
                    ? Zd_word_added_to_user_dict_STR
                    : Zd_words_added_to_user_dict_STR,
            words_added));

    if(dialog_box_start())
    {
        d_check_mess[0].textfield = buffer1;
        d_check_mess[1].textfield = buffer2;

        consume_bool(dialog_box(D_CHECK_MESS));

        dialog_box_end();
    }
}

static void
checkdocument_fn_action(void)
{
    SLR oldpos;
    S32 tlecpos;
    S32 mis_spells = 0, words_added = 0, res;
    char array[MAX_WORD];
    char original[LIN_BUFSIZ];
    BOOL do_disable = FALSE;

    if( !mergebuf_nocheck()    ||
        !set_check_block()     ||
        err_open_master_dict() )
            return;

    /* must be at least one line we can get to */
    if(n_rowfixes >= rowsonscreen)
        internal_process_command(N_FixRows);

    /* save current position */
    oldpos.col = curcol;
    oldpos.row = currow;
    tlecpos    = lecpos;

    escape_enable(); do_disable = TRUE;

    while(!been_error)
    {
        /* find next mis-spelled word */
        for(; get_next_misspell(array) && !been_error && !ctrlflag; ++mis_spells)
        {
            actind_end();

            escape_disable(); do_disable = FALSE;

            /* NB. lowercase version! */
            if(!mystr_set(&d_check[C_CHANGE].textfield, array))
                break;

            d_check[C_CHANGE].option = d_check[C_BROWSE].option = 'N';

            /* take a copy of just the misplet worm */
            xstrnkpy(original, elemof32(original), linbuf + sch_stt_offset, strlen(array));
            trace_1(TRACE_APP_PD4, "CheckDocument_fn: misplet word is '%s'", original);

            word_to_invert = original;
            lecpos = sch_stt_offset;

            /* desperate attempts to get correct redraw */
            output_buffer = TRUE;
            slot_in_buffer = TRUE;
            draw_screen();
            draw_caret(); /* otherwise caret will keep flashing on/off at wrong place */
            slot_in_buffer = FALSE;

        DOG_BOX:
            (void) insert_most_recent_userdict(&d_check[C_ADD].textfield);

            clearkeyboardbuffer();

            if(!dialog_box_start()  ||  !dialog_box(D_CHECK))
            {
                been_error = TRUE;
                break;
            }

            dialog_box_end();

            /* add to user dictionary? */
            if(d_check[C_ADD].option == 'Y')
            {
                res = dict_number(d_check[C_ADD].textfield, TRUE);

                if(status_ok(res))
                {
                    if((res = spell_addword((DICT_NUMBER) res,
                                            d_check[C_CHANGE].textfield))
                                            > 0)
                        ++words_added;
                }

                if(res < 0)
                {
                    reperr_null(res);
                    break;
                }
            }

            /* browse? */
            if(d_check[C_BROWSE].option == 'Y')
            {
                res = browse(master_dictionary, d_check[C_CHANGE].textfield);

                trace_1(TRACE_APP_PD4, "word_to_insert = _%s_", word_to_insert);

                if(res < 0)
                {
                    reperr_null(res);
                    break;
                }
                else if(*word_to_insert)
                {
                    consume_bool(mystr_set(&d_check[C_CHANGE].textfield, word_to_insert));
                    d_check[C_CHANGE].option = 'Y';
                }

                d_check[C_BROWSE].option = 'N';
                goto DOG_BOX;
            }

            /* redraw the row sometime */
            word_to_invert = NULL;
            mark_row(currowoffset);

            /* replace word? */
            if(d_check[C_CHANGE].option == 'Y')
            {
                sch_pos_end.col = sch_pos_stt.col;
                sch_pos_end.row = sch_pos_stt.row;
                sch_end_offset  = sch_stt_offset + strlen((char *) array);

                if(protected_cell(sch_pos_stt.col, sch_pos_stt.row))
                    break;

                filbuf();

                res = do_replace((uchar *) d_check[C_CHANGE].textfield, TRUE);

                if(!res)
                {
                    bleep();
                    break;
                }

                if(!mergebuf_nocheck())
                    break;
            }
            /* if all set to no, add to temporary spell list */
            else if((d_check[C_BROWSE].option == 'N')  &&  (d_check[C_ADD].option == 'N'))
            {
                if(status_fail(res = add_to_list(&first_spell, 0, array)))
                {
                    reperr_null(res);
                    break;
                }
            }

            escape_enable(); do_disable = TRUE;
        } /*for()*/

        break;
    }

    if(do_disable)
        escape_disable();

    actind_end();

    word_to_invert = NULL;

    if(is_current_document())
    {
        /* restore old position - don't do mergebuf */
        slot_in_buffer = buffer_altered = FALSE;

            {
            chknlr(oldpos.col, oldpos.row);
            lecpos = tlecpos;
            }

        out_screen = xf_drawcolumnheadings = xf_interrupted = TRUE;

        draw_screen();
        draw_caret();
    }

    if(!in_execfile)
        checkdocument_do_report(mis_spells, words_added);
}

extern void
CheckDocument_fn(void)
{
    if(!init_dialog_box(D_CHECKON))
        return;

    if(!dialog_box_start())
        return;

    if(!dialog_box(D_CHECKON))
        return;

    dialog_box_end();

    checkdocument_fn_action();
}

/* end of spellchk.c */
