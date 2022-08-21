/* pdsearch.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that does search and replace */

/* RJM Mar 1988 */

#include "common/gflags.h"

#include "datafmt.h"

#include "ridialog.h"
#include "riscos_x.h"

#include "colh_x.h"

/*
internal functions
*/

static void
find_and_replace(
    BOOL next,
    BOOL replace_option,
    BOOL confirm_option);

/*
local variables
*/

static BOOL only_at_start = FALSE;  /* check only at start of slot? */
static S32 found_sofar;      /* how many strings found in this search */
static S32 hidden_sofar; /* strings in columns width 0 */
static uchar *target_string = NULL;
static uchar *replace_string = NULL;

static S32 wild_current = 0;

static S32 last_wild_strings;

/******************************************************************************
*
* compile the search or replace string
*
******************************************************************************/

static BOOL
compile_search_string(
    char **targ,
    char *from_str)
{
    uchar array[LIN_BUFSIZ];
    U32 to_idx = 0;
    uchar *from = from_str ? (uchar *) from_str : UNULLSTR;
    BOOL only_start = FALSE;
    BOOL is_replace = (from != (uchar *) d_search[SCH_TARGET].textfield);
    uchar ch;

    do  {
        ch = *from++;

        switch(ch)
            {
            case '^':
                switch(toupper(*from))
                    {
                    case '^':
                        array[to_idx++] = '^';
                        break;

                    case 'S':
                        array[to_idx++] = SPACE;
                        break;

                    case 'B':
                        /* check at start of search string */
                        if(from != (uchar *) d_search[SCH_TARGET].textfield + 1)
                            goto BAD_HAT_POINT;
                        only_start = TRUE;
                        break;

                    case '?':
                    case '#':
                        if(is_replace  &&  (!from[1]  ||  !isdigit(from[1])))
                            goto BAD_HAT_POINT;
                        array[to_idx++] = DUMMYHAT;
                        array[to_idx++] = toupper(*from);
                        break;

                    default:
                        if((*from >= FIRST_HIGHLIGHT_TEXT)  &&  (*from <= LAST_HIGHLIGHT_TEXT))
                            array[to_idx++] = (uchar) (FIRST_HIGHLIGHT + (*from - FIRST_HIGHLIGHT_TEXT));
                        else
                            goto BAD_HAT_POINT;
                        break;
                    }
                from++;
                break;

            case CTRLDI:
                if(*from == CTRLDI)
                    array[to_idx++] = CTRLDI;
                else if(isalpha(*from))
                    array[to_idx++] = toupper(*from) - ('A'-1);
                from++;
                break;

            case SPACE:
                while(*from++ == SPACE)
                    ;
                from--;
                array[to_idx++] = FUNNYSPACE;
                break;

            default:
                array[to_idx++] = ch;
                break;
            }
        }
    while(ch);

    if(!mystr_set(targ, array))
        return(FALSE);

    if(from_str == d_search[SCH_TARGET].textfield)
        only_at_start = only_start;     /* has ^b been set? */

    return(TRUE);

BAD_HAT_POINT:
    return(reperr_null(ERR_BAD_HAT));
}

/* search just the marked block in the containing document */

static BOOL
set_search_marked_block(void)
{
    if(!MARKER_DEFINED())
        return(reperr_null((blkstart.col != NO_COL)
                                    ? ERR_NOBLOCKINDOC
                                    : ERR_NOBLOCK));

#ifdef SEARCH_COLRANGE
    /* not doing marked block and column range */
    if(d_search[SCH_COLRANGE].option == 'Y')
        return(reperr_null(ERR_BAD_OPTION));
#endif

    sch_docno = blk_docno;
    sch_stt   = blkstart;
    sch_end   = (blkend.col != NO_COL) ? blkend : blkstart;

    return(TRUE);
}

/* search all rows in the current document */

static BOOL
set_search_all_rows(void)
{
    sch_docno   = current_docno();
    sch_stt.row = 0;
    sch_end.row = numrow-1;

#ifdef SEARCH_COLRANGE
    if(d_search[SCH_COLRANGE].option == 'Y')
        {
        /* column range */

        buff_sofar = (uchar *) d_search[SCH_COLRANGE].textfield;

        sch_stt.col = getcol();
        sch_end.col = getcol();

        if(sch_end.col == NO_COL)
            sch_end.col = sch_stt.col;

        if((sch_end.col == NO_COL)  ||  (sch_end.col < sch_stt.col))
            return(reperr_null(ERR_BAD_COL));
        }
    else
#endif
        {
        /* whole sheet selected */

        sch_stt.col = 0;
        sch_end.col = numcol-1;
        }

    return(TRUE);
}

/* set up the search block, this will be updated on insertions, deletions */

static BOOL
set_search_block(
    BOOL next)
{
    BOOL res;

    if(d_search[SCH_BLOCK].option == 'Y')
        /* marked block option selected */
        res = set_search_marked_block();
    else
        /* all rows, current document */
        res = set_search_all_rows();

    /* set initial position according to next/previous match wanted */
    if(res)
        sch_pos_stt = (next) ? sch_stt : sch_end;

    return(res);
}

/******************************************************************************
*
*  search
*
******************************************************************************/

static void
do_search(
    BOOL replace)
{
    BOOL confirm_option, replace_option;

    if(!dialog_box_start())
        return;

    /* switch off replace, confirmation and expression slot searching */

    d_search[SCH_REPLACE].option = replace ? 'Y' : 'N';

    d_search[SCH_CONFIRM].option = 'Y';

    while(dialog_box(D_SEARCH))
        {
        if(str_isblank(d_search[SCH_TARGET].textfield))
            {
            reperr_null(ERR_BAD_STRING);
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        break; /* never persists in this case */
        }

    dialog_box_end();

    if(!compile_search_string((char **) (&target_string),
                              d_search[SCH_TARGET].textfield))
        return;

    if(!compile_search_string((char **) (&replace_string),
                              d_search[SCH_REPLACE].textfield))
        return;

    if(!set_search_block(TRUE))
        return;

    sch_stt_offset = -1;

    hidden_sofar = found_sofar = (S32) 0;

    confirm_option = (d_search[SCH_CONFIRM].option == 'Y');
    replace_option = (d_search[SCH_REPLACE].option == 'Y');

    /* get next match */
    find_and_replace(TRUE, replace_option, confirm_option);
}

extern void
Replace_fn(void)
{
    do_search(TRUE);
}

extern void
Search_fn(void)
{
    do_search(FALSE);
}

/******************************************************************************
*
*  next or previous match
*
*  called only by NextMatch_fn and PrevMatch_fn
*
******************************************************************************/

static void
next_or_previous_match(
    BOOL next)
{
  /*S32 tlecpos;*/

    xf_flush = TRUE;

    trace_2(TRACE_APP_PD4, "next_or_prev_match: schhan %d curhan %d", sch_docno, current_docno());

    /* cannot check with str_isblank cos might be looking for spaces */
    if(!target_string  ||  (*target_string == '\0'))
        {
        reperr_null(ERR_BAD_STRING);
        return;
        }

    /* if moved to another document, allow next/prev match (allow col option) */
    if(sch_docno != current_docno())
        {
        if(!set_search_all_rows())
            return;
        }

    /* do now, so we don't get flashing dbox */
    riscos_frontmainwindow(TRUE);

    if(xf_inexpression_box || xf_inexpression_line)
        {
        /* NextMatch_fn or PrevMatch_fn called whilst in an editor, so */
        /* mergeback current edit buffer contents before doing search  */

        S32 caretpos;  /* buffer offset to cursor column and row (might be multiline) within slot */

        if(!expedit_mergebacktext(FALSE, &caretpos))    /* don't report compile errors, naff formula saved as text */
            return;             /* mergeback failed, probably text too big for linbuf */

        lecpos = caretpos;      /* so we don't continually find same sub-string */
        }
    else if(!mergebuf())
        return;

    /* redraw current slot */
    mark_row_border(currowoffset);
    mark_slot(travel_here());

    /* go from current position */
    sch_pos_stt.col = curcol;
    sch_pos_stt.row = currow;
    sch_stt_offset  = lecpos;

    find_and_replace(next, FALSE, FALSE);
}

/******************************************************************************
*
*  next match
*
******************************************************************************/

extern void
NextMatch_fn(void)
{
    next_or_previous_match(TRUE);
}

/******************************************************************************
*
*  previous match
*
******************************************************************************/

extern void
PrevMatch_fn(void)
{
    next_or_previous_match(FALSE);
}

/*
save the old wild string in the wild string buffer
*/

static void
save_wild_string(
    uchar *y,
    uchar *oldypos)
{
    uchar array[LIN_BUFSIZ];
    uchar *ptr = array;
    S32 res;

    if(wild_strings == 0)
        delete_list(&wild_string_list);

    while(oldypos < y)
        *ptr++ = *oldypos++;

    *ptr = NULLCH;

    if(status_fail(res = add_to_list(&wild_string_list, (S32) wild_strings, array)))
        reperr_null(res);
    else
        last_wild_strings = wild_strings++;
}

/*
find the next slot in the search and decompile it to linbuf
if no more slots in file return NULL
*/

static uchar *
get_next_slot_in_search(void)
{
    P_SLOT tslot = travel(sch_pos_end.col, sch_pos_end.row);

    /* only look across boundaries of text slots */
    if(!tslot  ||  (tslot->type != SL_TEXT))
        return(NULL);

    /* look for second slot */

    tslot = travel(sch_pos_end.col, sch_pos_end.row+1);
    if(!tslot)
        return(NULL);

    /* can only match non-blank text slots */
    if((tslot->type != SL_TEXT)  ||  str_isblank(tslot->content.text))
        return(NULL);

    /* tslot is new slot, decompile it */
    prccon(linbuf, tslot);
    sch_pos_end.row++;
    return(linbuf);
}

/*
recover first slot to search from
the thing in linbuf has not changed, so can
decompile the slot in sch_pos_stt into linbuf
*/

static void
recover_first_slot(void)
{
    sch_pos_end = sch_pos_stt;
    prccon(linbuf, travel(sch_pos_stt.col, sch_pos_stt.row));
}

/*
search for string in decompiled slot
assumes strings are decompiled
x is search string, y is string in slot
copes with ^? and ^# for single and multiple wild characters in target_string
returns TRUE if match, FALSE no match
*/

static BOOL
sch_stringcmp(
    uchar *ptr2,
    BOOL next,
    BOOL folding)
{
    BOOL moved_slots = FALSE;   /* are we starting in correct slot */
    uchar *current_slot;
    S32 increment = (next) ? 1 : -1;

    /* this needs to be changed for slot boundaries. <-- whatsis mean?? */
    sch_pos_end = sch_pos_stt;

    /* look all through this string for target_string */
    for( ; *ptr2  &&  sch_stt_offset >= 0; ptr2+=increment, sch_stt_offset+=increment)
        {
        uchar *x, *y;
        uchar *ox, *oy, *oldypos;
        BOOL wild_x;
        last_wild_strings = wild_strings = 0;

        if(only_at_start && sch_stt_offset > 0)
            return(FALSE);

        wild_queries = 0;
        wild_strings = -1;
        wild_current = 0;
        oldypos = NULL;

        if(moved_slots)
            {
            recover_first_slot();
            moved_slots = FALSE;
            }
        y = current_slot = ptr2;
        x = target_string-1;

STAR:
        last_wild_strings = wild_strings;

        x++;

        wild_x = (*x == DUMMYHAT);
        if(wild_x)
            x++;

        /* save the wild_string from oy to y, but not first time thru */
        if(wild_strings == -1)
            last_wild_strings = wild_strings = 0;
        else
            {
            /* remember position of start of match */
            oldypos = y;
            }

        oy = y;

        /* loop1: NEXT_STAR */
        for(;;)
            {
            wild_strings = last_wild_strings;
            oy++;
            ox = x;

            /*  loop3: */
            for(;;)
                {
                uchar xch, ych;
                S32 pos_res;

                if(wild_x  &&  (*x == '#'))
                    {
                    if(*y == SPACE)
                        goto NOT_THIS_POS;

                    goto STAR;
                    }

                if(!*x)
                    {
                    sch_end_offset = y-current_slot;
                    return(TRUE);       /* found x-string in y */
                    }

                /* see if we have space matching */
                if(*x == FUNNYSPACE)
                    {
                    /* read to last space or end of slot */
                    while((*y == SPACE)  &&  ((y[1] == SPACE)  ||  !y[1]))
                        y++;

                    if(!*y)
                        {
                        /* get next non-blank slot */
                        moved_slots = TRUE;

                        y = current_slot = get_next_slot_in_search();

                        if(!y)
                            goto NOT_THIS_POS;

                        if(*y != SPACE)
                            {
                            y--;    /* y is going to be incremented */
                            goto HAD_MATCH;
                            }

                        while(y[1] == SPACE)
                            y++;

                        goto HAD_MATCH;
                        }

                    if(*y != SPACE)
                        goto NOT_THIS_POS;

                    goto HAD_MATCH;
                    }

                /* see if characters at x and y match */

                xch = folding ? toupper(*x) : *x;
                ych = folding ? toupper(*y) : *y;

                if((pos_res = xch - ych) != 0)
                    {
                    /* are we trying to match space with wildcard? */
                    if(*y == SPACE)
                        goto NOT_THIS_POS;

                    /* are we at end of y string? */
                    if(!*y)
                        goto NOT_THIS_POS;          /* first bigger */

                    /* single character wildcard at x? */
                    if(!wild_x  ||  (*x != '?')  ||  (*y == SPACE))
                        {
                        if(moved_slots)
                            {
                            recover_first_slot();
                            moved_slots = FALSE;
                            }

                        x = ox;
                        y = oy;

                        /* in a wild match ? */
                        if(!oldypos)
                            goto NOT_THIS_POS;

                        break;
                        }
                    else if(wild_queries < WILD_FIELDS)
                        wild_query[wild_queries++] = *y;

                    }

                HAD_MATCH:

                if(oldypos)
                    {
                    save_wild_string(y, oldypos);
                    oldypos = NULL;
                    }

                /* characters at x and y match, so increment x and y */

                wild_x = (*++x == DUMMYHAT);
                if(wild_x)
                    x++;

                y++;
                }
            }

        NOT_THIS_POS:
            ;
        /* the string was not found at this position, look at the next one */
        }

    return(FALSE);
}

/*
try this slot for the search string, start at sch_stt_offset
if find a match, leave the position in sch_stt_offset and return TRUE
*/

static BOOL
look_in_this_slot(
    P_SLOT tslot,
    BOOL next)
{
    S32 len;

    prccon(linbuf, tslot);

    /* check that sch_stt_offset not past end of string */

    len = strlen((char *) linbuf);

    if(next)
        {
        if(sch_stt_offset > len)
            return(FALSE);
        }
    else
        {
        if(sch_stt_offset < 0)
            return(FALSE);

        if( sch_stt_offset > len)
            sch_stt_offset = len-1;
        }

    return(sch_stringcmp(linbuf + sch_stt_offset, next, d_search[SCH_CASE].option == 'Y'));
}

/*
find the string

look through slots and files
*/

static BOOL
find_sch_string(
    BOOL next,
    BOOL replace_option,
    BOOL confirm_option)
{
    P_SLOT tslot;
    ROW number_of_rows = sch_end.row - sch_stt.row + 1;
  /*S32 res;*/

    if(xf_inexpression)
        {
        merexp();
        endeex();
        }
    else if(xf_inexpression_box || xf_inexpression_line)
        {
        if(!expedit_mergebacktext(FALSE, NULL))
            return(FALSE);      /* mergeback failed, probably text too big for linbuf */
        }
    else if(!mergebuf())
        return(FALSE);

    /* mark the slot cos we might have replaced something in it */
    mark_row_border(currowoffset);
    mark_slot(travel_here());

    for(;;)
        {
        if(ctrlflag)
            {
            ack_esc();
            return(reperr_null(ERR_ESCAPE));
            }

        tslot = travel(sch_pos_stt.col, sch_pos_stt.row);

        if(tslot)
            switch(tslot->type)
                {
                case SL_NUMBER:
                    if(d_search[SCH_EXPRESSIONS].option != 'Y')
                        break;
                case SL_TEXT:
                    if(look_in_this_slot(tslot, next))
                        goto FOUNDONE;

                default:
                    break;
                }

        if(next)
            {
            if(++sch_pos_stt.col > sch_end.col)
                {
                sch_pos_stt.col = sch_stt.col;

                if(++sch_pos_stt.row > sch_end.row)
                    {
                    return(FALSE);
                    }

                if(replace_option  &&  !confirm_option)
                    /* Tutu says 'sod the columns' */
                    actind((S32) (((sch_pos_stt.row - sch_stt.row) * 100) / number_of_rows));
                }

            /* having moved on, reset sch_stt_offset */
            sch_stt_offset = 0;
            }
        else
            {
            if(--sch_pos_stt.col < sch_stt.col)
                {
                sch_pos_stt.col = sch_end.col;

                if(--sch_pos_stt.row < sch_stt.row)
                    {
                    return(FALSE);
                    }
                }

            /* having moved on, reset sch_stt_offset */
            sch_stt_offset = LIN_BUFSIZ;
            }
        }

FOUNDONE:

    if(n_rowfixes >= rowsonscreen)
        internal_process_command(N_FixRows);

    chknlr(curcol = sch_pos_stt.col, currow = sch_pos_stt.row);

    /* if all of string in one slot, increment end offset */
    if(sch_pos_end.row == sch_pos_stt.row)
        sch_end_offset += sch_stt_offset;

    tslot = travel_here();

    if(tslot  &&  (tslot->type != SL_TEXT))
        seteex();
    else
        filbuf();

    lecpos = sch_stt_offset;
    lescrl = 0;

/*  #if RISCOS                    */ /* alright SKS did it */
/*  xf_acquirecaret = TRUE;       */
/*  #endif                        */
    xf_drawcolumnheadings = TRUE;

    return(TRUE);
}

/*
prompt user for whether to replace string
*/

static S32
prompt_user(void)
{
    S32 overlap, len, offset;
    S32 res;

    /* find where co starts on screen, cos not all might be on screen */
    if(schcsc(curcol) == NOTFOUND)
        cencol(curcol);

    /* need to ensure that the relevant bit of the field is on the screen */
    filbuf();

    offset = calcad(schcsc(curcol));

    overlap = chkolp(travel_here(), curcol, currow);

    /* if not all on screen, truncate */
    overlap = MIN(overlap, pagwid - offset);

    len = (sch_pos_end.row == sch_pos_stt.row)
                ? (sch_end_offset - lecpos)
                : strlen((char *) linbuf) - lecpos;

    /* if doesn't fit */
    if(lecpos + len > overlap)
        {
        lescrl = lecpos - overlap / 2;
        dont_update_lescrl = TRUE;
        }

    /* get screen redraw after dbox shown */
    xf_interrupted = xf_acquirecaret = TRUE;

    if(xf_inexpression)
        {
        S32 caretpos = lecpos; /* cos endeex zeros it */
        merexp();
        endeex();
        expedit_editcurrentslot(caretpos, FALSE);
        }

    res = riscdialog_replace_dbox(NULL, NULL);

    if(xf_inexpression_box || xf_inexpression_line)
        {
        S32   caretpos; /* buffer offset to cursor column, row (might be multiline) within slot */
        P_SLOT tslot;

        if(!expedit_mergebacktext(FALSE, &caretpos))       /* don't report compile errors, naff formula saved as text */
            return(ESCAPE);             /* mergeback failed, probably text too big for linbuf */

        *linbuf = '\0';
        tslot = travel_here();

        if(tslot)
            prccon(linbuf, tslot);

        seteex();
        lecpos = caretpos;
        lescrl = 0;
        }

    return(res);
}

/******************************************************************************
*
*  find the search string
*
******************************************************************************/

static void
find_and_replace(
    BOOL next,
    BOOL replace_option,
    BOOL confirm_option)
{
    BOOL update_toldpos = TRUE;
    SLR toldpos;
    S32 tlecpos = lecpos;
    char array1[256];
    char array2[256];
    BOOL do_it;

    actind(0);

    escape_enable();

    for(;;)
        {
        if(update_toldpos)
            {
            toldpos.col = curcol;
            toldpos.row = currow;
            tlecpos = lecpos;
            }
        else
            update_toldpos = TRUE;

        if(next)
            sch_stt_offset++;
        else
            sch_stt_offset--;

        /* find the string */

        if(!find_sch_string(next, replace_option, confirm_option))
            {
            escape_disable();

            curcol = toldpos.col;
            currow = toldpos.row;

            chknlr(curcol, currow);
            lecpos = tlecpos;

            (void) xsnprintf(array1, elemof32(array1),
                    (found_sofar == 1)
                            ? Zld_one_string_found_STR
                            : Zld_many_strings_found_STR,
                    found_sofar);

            if(hidden_sofar /* != (S32) 0 */)
                (void) xsnprintf(array2, elemof32(array2),
                        (hidden_sofar == 1)
                                ? Zld_one_string_hidden_STR
                                : Zld_many_strings_hidden_STR,
                        hidden_sofar);
            else
                array2[0] = '\0';

            bleep();

            /* get screen redrawn after dbox shown */
            xf_interrupted = xf_acquirecaret = TRUE;

            (void) riscdialog_replace_dbox(array1, array2);

            goto ENDPOINT;
            }

        if(next)
            found_sofar++;
        else
            found_sofar--;

        /* string found */

        /* don't stop on colwidths zero */
        if(colwidth(curcol) == 0)
            {
            if(next)
                hidden_sofar++;
            else
                hidden_sofar--;
            update_toldpos = FALSE;

            /* if a search or replace with prompt, go round again */
            if(!replace_option  ||  confirm_option)
                continue;
            }

        if(protected_slot(curcol, currow))
            continue;

        escape_disable();

            {
            /* do a replace or return to user ? */
            if(replace_option)
                {
                do_it = TRUE;

                if(confirm_option)
                    /* ask the user if the string should be changed */
                    switch(prompt_user())
                        {
                        case ESCAPE:
                            goto ENDPOINT;

                        case 'A':
                            confirm_option = FALSE;
                            break;

                        case 'Y':
                            break;

                        default:
                            do_it = FALSE;
                            break;
                        }

                /* replace the target string with the replace string */
                if(do_it)
                    {
                    do_it = do_replace(replace_string, d_search[SCH_CASE].option == 'Y');
                    delete_list(&wild_string_list);
                    if(!do_it)
                        goto ENDPOINT;
                    --sch_stt_offset;     /* as we will increment it at the loop start */
                    /* don't worry about the backward replace case - it can't happen! */
                    }

                /* mark the slot anyway */
                mark_row_border(currowoffset);
                mark_slot(travel_here());

                if(confirm_option)
                    {
                    draw_screen();
                    draw_caret();
                    }
                }
            else
                /* found string in search, next match */
                goto ENDPOINT;
            }

        escape_enable();
        }

ENDPOINT:

#if FALSE
    escape_disable();

    /* rcm and rjm try the following on 13.1.92 to clear the persistent escape condition */
    {
    fx_x2(126, 0);
    }
#endif

    actind_end();

    riscdialog_replace_dbox_end();

    if(xf_inexpression)
        {
        S32 caretpos = lecpos; /* cos endeex zeros it */
        merexp();
        endeex();
        expedit_editcurrentslot(caretpos, FALSE);
        }
}

/* end of pdsearch.c */
