/* replace.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that does replace for search and spell */

/* RJM 11-Apr-1989 */

#include "common/gflags.h"

#include "datafmt.h"

/*
insert the char, perhaps updating sch_end_offset if last slot
*/

static void
insert_this_ch(
    uchar ch,
    BOOL *firstone,
    S32 tcase)
{
    if(tcase != NOCASEINFO)
        {
        if(*firstone)
            {
            *firstone = FALSE;
            ch = ((tcase & FIRSTUPPER)  ? toupper : tolower) (ch);
            }
        else
            ch = ((tcase & SECONDUPPER) ? toupper : tolower) (ch);
        }

    if((curcol == sch_pos_end.col)  &&  (currow == sch_pos_end.row))
        ++sch_end_offset;

    /* inserted by RJM 15.3.89, don't know how the problem was caused */
    slot_in_buffer = TRUE;

    trace_1(TRACE_APP_PD4, "insert_this_ch(%c)", ch);

    chrina(ch, TRUE);
}

/*
replace one word at current position with one word from next_replace_word
*/

static BOOL
replace_one_word(
    S32 tcase)
{
    BOOL firstone = TRUE;
    uchar tiovbit = xf_iovbit;
    S32 offset;

    xf_iovbit = TRUE;

    for( ; *next_replace_word; next_replace_word++)
        {
        switch(*next_replace_word)
            {
            case DUMMYHAT:
                next_replace_word++;
                switch(toupper(*next_replace_word))
                    {
                    case '#':
                        if(*++next_replace_word  &&  isdigit(*next_replace_word))
                            {
                            if((offset = (S32) (*next_replace_word - '1')) < wild_strings)
                                {
                                /* insert the (offset+1)th string from wild_string */
                                uchar *tptr;
                                P_LIST lptr = search_list(&wild_string_list, (S32) offset);

                                tptr = lptr ? lptr->value : UNULLSTR;

                                /* insert the string */
                                while(*tptr)
                                    {
                                    insert_this_ch(*tptr++, &firstone, tcase);
                                    if(been_error)
                                        goto HAS_BEEN_ERROR;
                                    }
                                }
                            }
                        break;

                    case '?':
                        if(*++next_replace_word  &&  isdigit(*next_replace_word))
                            {
                            if((offset = (S32) (*next_replace_word - '1')) < wild_queries)
                                insert_this_ch(wild_query[offset], &firstone, tcase);
                            break;
                            }

                    default:
                        break;
                    }
                break;

            case FUNNYSPACE:
                return(TRUE);

            default:
                insert_this_ch(*next_replace_word, &firstone, tcase);
                break;
            }

        HAS_BEEN_ERROR:

        if(been_error)
            {
            xf_iovbit = tiovbit;
            return(FALSE);
            }
        }

    sch_pos_stt.row = currow;
    sch_pos_stt.col = curcol;
    sch_stt_offset  = lecpos /*-1*/;

    xf_iovbit = tiovbit;
    return(TRUE);
}

/*
in the middle of replacing, hit a slot break
next slot must exist, must be text and must not be blank
*/

static BOOL
next_slot_in_replace(void)
{
    P_SLOT tslot;

    if(!mergebuf())
        return(FALSE);

    mark_row_border(currowoffset);
    mark_slot(travel_here());

    tslot = travel(curcol, ++currow);

    if(!tslot)
        return(FALSE);

    /* SKS after 4.12 30mar92 - was ++currow which destroyed alternate slots until end-of-world */
    chknlr(curcol, currow);

    prccon(linbuf, tslot);

    lecpos = 0;

    slot_in_buffer = TRUE;
    return(TRUE);
}

/*
delete the char, perhaps updating sch_end_offset if we're in the last slot
*/

static void
delete_this_ch(void)
{
    if((curcol == sch_pos_end.col)  &&  (currow == sch_pos_end.row))
        --sch_end_offset;

    trace_1(TRACE_APP_PD4, "delete_this_ch: deleting %c", linbuf[lecpos]);

    small_DeleteCharacter_fn();
}

/*
replace the found target with the result
do it word by word, perhaps matching case of each word
current position is start of search string
sch_pos_stt, sch_stt_offset is start of search string
sch_pos_end, sch_end_offset is end of search string
replace_str points at word to replacing word
*/

extern BOOL
do_replace(
    uchar *replace_str,
    BOOL folding)
{
    BOOL first = TRUE;
    S32 tcase;

    next_replace_word = replace_str ? replace_str : UNULLSTR;

    trace_8(TRACE_APP_PD4, "do_replace(%s, folding = %s): stt_off %d, end_off %d, (%d, %d) - (%d, %d)",
            next_replace_word, trace_boolstring(folding), sch_stt_offset, sch_end_offset,
            sch_pos_stt.col, sch_pos_stt.row, sch_pos_end.col, sch_pos_end.row);
    trace_3(TRACE_APP_PD4, "linbuf %s, curcol %d, currow %d", linbuf, curcol, currow);

    /* delete each word and replace with a word from the replace string */

    lecpos = sch_stt_offset;

    while((currow != sch_pos_end.row)  ||  (curcol != sch_pos_stt.col)  ||  (lecpos < sch_end_offset))
        {
        tcase = NOCASEINFO;

        /* move over spaces and line slot breaks */
        assert(lecpos < elemof32(linbuf));
        if(linbuf[lecpos] == SPACE)
            {
            if(first  ||  !*next_replace_word)
                delete_this_ch();
            else
                lecpos++;

            continue;
            }

        first = FALSE;

        if(!linbuf[lecpos])
            {
            if(!next_slot_in_replace())
                return(FALSE);

            continue;
            }

        /* look at the case of the word before it is deleted */
        if(folding  &&  (isalpha(linbuf[lecpos]) || isalpha(linbuf[lecpos+1])))
            {
            tcase = 0;

            if(isupper(linbuf[lecpos]))
                tcase |= FIRSTUPPER;

            if(isupper(linbuf[lecpos+1]))
                tcase |= SECONDUPPER;
            }

        /* delete the word, only works in one slot */
        while(linbuf[lecpos]  &&  linbuf[lecpos] != SPACE  &&
              ((currow != sch_pos_end.row)  ||  (curcol != sch_pos_stt.col)  ||  (lecpos < sch_end_offset)))
            delete_this_ch();

        if(*next_replace_word == FUNNYSPACE)
            next_replace_word++;

        if(!replace_one_word(tcase))
            return(FALSE);
        }

    /* add all the other words */
    while(*next_replace_word  &&  !been_error)
        {
        if( *next_replace_word == FUNNYSPACE)
            *next_replace_word = SPACE;

        if(!replace_one_word(NOCASEINFO))
            return(FALSE);
        }

    trace_0(TRACE_APP_PD4, "leaving do_replace");

    return(!been_error);
}

/* end of replace.c */
