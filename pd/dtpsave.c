/* dtpsave.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that saves dtp/fwp and loads fwp files */

/* RJM Jul 1989 */

/* Assumes format of FWP files as described in 1st Word Plus
 * Software Developers Guide, Release 1.11 of June 1987
 */

#include "common/gflags.h"

#include "datafmt.h"

#include "dtpsave.h"

static uchar fwp_highlights[] =
{
    8,      /* PD underline */
    1,      /* PD bold */
    0,
    4,      /* PD italics */
    32,     /* PD subscript */
    16,     /* PD superscript */
    0,
    0
};

static BOOL
chkcfm_for_fwp(
    COL tcol,
    ROW trow,
    BOOL first)
{
    P_SLOT tslot;

    if(chkrpb(trow))
        return(FALSE);

    tslot = travel(tcol, trow);

    if(tslot  &&  (tslot->type == SL_TEXT))
        {
        /* note: if protected bit set will return false */
        switch(tslot->justify)
            {
            case J_FREE:
            case J_LEFTRIGHT:
            case J_RIGHTLEFT:
                if(first  ||  (*tslot->content.text != SPACE))
                    return(TRUE);

            default:
                break;
            }
        }

    return(FALSE);
}

static BOOL fwp_save_first_slot_on_line;
static ROW fwp_save_stt_row;
static ROW fwp_save_end_row;

extern BOOL
dtp_save_slot(
    P_SLOT tslot,
    COL tcol,
    ROW trow,
    FILE_HANDLE output)
{
    uchar *lptr = linbuf;
    uchar ch;
    S32 trailing_spaces = 0;
    BOOL possible_para_dtp;

    trace_3(TRACE_APP_PD4, "dtp_save_slot(" PTR_XTFMT ", %d, %d)\n", report_ptr_cast(tslot), tcol, trow);

    if(tslot)
        plain_slot(tslot, tcol, trow, PARAGRAPH_CHAR, linbuf);

    trace_1(TRACE_APP_PD4, "slot converted to '%s'\n", linbuf);

    /* if this and the slot above can be formatted together call it a paragraph */
    possible_para_dtp =
                        chkcfm_for_fwp(tcol, trow, FALSE)  &&
                        ((trow > fwp_save_stt_row)  &&  chkcfm_for_fwp(tcol, trow-1,  FALSE))  &&
                        is_block_blank(tcol+1,trow-1,numcol,trow);

    if(fwp_save_first_slot_on_line)
        {
        fwp_save_first_slot_on_line = FALSE;

        /* rjm 28.1.92 ensures that lines being concatenated have space between them */
        /* try to make formattable paragraphs in DTP mode too, but unformattable tables */
        if(possible_para_dtp  &&  trow > 0)
            {
            /* do we need to send out space first? */
            P_SLOT tslot1;

            tslot1 = travel(tcol, trow-1);
            if(tslot1)
                {
                uchar buffer[LIN_BUFSIZ];
                S32 length;

                plain_slot(tslot1, tcol, trow-1, PARAGRAPH_CHAR, buffer);
                length = strlen(buffer);
                if(length > 0 && buffer[length-1] != SPACE)
                    if(!away_byte(SPACE, output))
                        return(FALSE);
                }
            }

        if(!possible_para_dtp  &&  trow > 0 && !away_eol(output))
            return(FALSE);
        }

    if(!tslot)
        return(TRUE);

    /* output contents, chucking highlight chars */
    while((ch = *lptr++) != '\0')
        {
        if(ch == SPACE)
            ++trailing_spaces;
        else
            {
            if(trailing_spaces)
                do
                    /* beware that FWP_SPACE is probably highlight char */
                    if(!away_byte(SPACE, output))
                        return(FALSE);
                while(--trailing_spaces); /* leaving trailing_spaces at zero */

            if(ishighlight(ch))
                {
                }
            else if(!away_byte(ch, output))
                return(FALSE);
            }
        }

    while(trailing_spaces--)
        if(!away_byte(SPACE, output))
            break;

    return(TRUE);
}

/*
save slot in fwp format

spaces should be soft spaces ($1E), could send out stretch spaces too but no
    point for fwp?
trailing spaces indicate to FWP that the two lines can be joined by formatting (yuk)
so spaces need to be stored and either suppressed or forced at eos

highlights get converted to FWP highlights and get switched off at eos
whole highlight status gets output on all highlight changes
*/

extern BOOL
fwp_save_slot(
    P_SLOT tslot,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    BOOL saving_fwp)
{
    uchar *lptr = linbuf;
    uchar ch;
    uchar h_byte = FWP_NOHIGHLIGHTS;
    S32 trailing_spaces = 0;
    BOOL possible_para_fwp, possible_para_dtp;

    trace_3(TRACE_APP_PD4, "fwp_save_slot(" PTR_XTFMT ", %d, %d)\n", report_ptr_cast(tslot), tcol, trow);

    plain_slot(tslot, tcol, trow, saving_fwp ? FWP_CHAR : PARAGRAPH_CHAR, linbuf);

    trace_1(TRACE_APP_PD4, "slot converted to '%s'\n", linbuf);

    /* if this and the slot below can be formatted together call it a paragraph in FWP */
    possible_para_fwp =  saving_fwp  &&
                        chkcfm_for_fwp(tcol, trow,  TRUE)  &&
                        ((trow != fwp_save_end_row)  &&  chkcfm_for_fwp(tcol, trow+1, FALSE));

    /* if this and the slot above can be formatted together call it a paragraph in DTP */
    possible_para_dtp = !saving_fwp  &&
                        chkcfm_for_fwp(tcol, trow, FALSE)  &&
                        ((trow == fwp_save_stt_row)  ||  chkcfm_for_fwp(tcol, trow-1,  TRUE))  &&
                        is_block_blank(tcol+1,trow,numcol,trow);

    if(fwp_save_first_slot_on_line)
        {
        fwp_save_first_slot_on_line = FALSE;

        /* try to make formattable paragraphs in DTP mode too, but unformattable tables */
        if(!saving_fwp  &&  !possible_para_dtp  &&  !away_string("\x0A$", output))
            return(FALSE);
        }

    /* output contents, dealing with highlight chars */
    while((ch = *lptr++) != '\0')
        {
        if(ch == SPACE)
            ++trailing_spaces;
        else
            {
            if(trailing_spaces)
                do
                    /* beware that FWP_SPACE is probably highlight char */
                    if(!away_byte(saving_fwp ? FWP_SPACE : SPACE, output))
                        return(FALSE);
                while(--trailing_spaces); /* leaving trailing_spaces at zero */

            if(ishighlight(ch))
                {
                if(saving_fwp)
                    {
                    /* poke highlight byte with new highlight */
                    h_byte ^= fwp_highlights[ch-FIRST_HIGHLIGHT];

                    /* output new highlight state */
                    if(!away_byte(FWP_HIGH_PREFIX, output)  ||  !away_byte(h_byte, output))
                        return(FALSE);
                    }
                }
            else if(!away_byte(ch, output))
                return(FALSE);
            }
        }

    /* switch all highlights off at end of slot */
    if(h_byte != FWP_NOHIGHLIGHTS)
        if(!away_byte(FWP_HIGH_PREFIX, output)  ||  !away_byte(FWP_NOHIGHLIGHTS, output))
            return(FALSE);

    if(!saving_fwp)
        {
        while(trailing_spaces--)
            if(!away_byte(SPACE, output))
                break;
    /* if this and next lines can be formatted together, output space (always if DTP) */
        if(!possible_para_dtp)
            if(!away_byte(LF, output))

        if(!away_byte('#', output))
            return(FALSE);
        }

    /* if this and next lines can be formatted together, output space (always if DTP) */
    if(saving_fwp  &&  possible_para_fwp)
        if(!away_byte(FWP_SPACE, output))
            return(FALSE);

    return(TRUE);
}

static BOOL
fwp_head_foot(
    uchar *lcr_field,
    S32 type,
    FILE_HANDLE output)
{
    uchar array[32], expanded[PAINT_STRSIZ];
    uchar *second, *third;

    if(!str_isblank(lcr_field))
        {
        expand_lcr(lcr_field,
                   -1, expanded, LIN_BUFSIZ /* field width not buffer size */,
                   FALSE, FALSE, FALSE,
                   FALSE);
        second = expanded + strlen(expanded) + 1;
        third  = second   + strlen(second)   + 1;

        (void) sprintf(array,
                       "\x1F" "%d",
                       type);

        if( !away_string(array,         output) ||
            !away_string(expanded,      output) ||
            !away_byte  ('\x1F',        output) ||
            !away_string(second,        output) ||
            !away_byte  ('\x1F',        output) ||
            !away_string(third,         output) ||
            !away_byte  (FWP_LINESEP,   output) )
                return(FALSE);
        }

    return(TRUE);
}

/*
 * Output FWP ruler.  Just output tab stops at each column position
 * and right margin at column A right margin position
*/

static BOOL
fwp_ruler(
    FILE_HANDLE output)
{
    coord rmargin, count;
    COL tcol = 0;

    trace_0(TRACE_APP_PD4, "fwp_ruler()\n");

    if(!away_string("\x1F" "9[", output))
        return(FALSE);

    /* find first column on_screen and get its right margin */
    while(!colwidth(tcol))
        tcol++;

    rmargin = colwrapwidth(tcol);
    if(!rmargin)
        rmargin = colwidth(tcol);

    if( rmargin > FWP_LINELENGTH)
        rmargin = FWP_LINELENGTH;

    /* output dots */
    for(count=1, tcol=0; tcol<numcol && count < FWP_LINELENGTH; tcol++)
        {
        coord thiswidth = colwidth(tcol);
        coord scount = 0;

        /* already sent [ at start */
        if(tcol == 0)
            thiswidth--;

        for( ; scount < thiswidth-1 && count < FWP_LINELENGTH; scount++)
            if(!away_byte((uchar) ((count++ == rmargin-2) ? FWP_RMARGIN : '.'), output))
                return(FALSE);

        /* output tabstop unless it is last position */
        if( (tcol < numcol-1) &&
            !away_byte((uchar) ((count++ == rmargin-2) ? FWP_RMARGIN : FWP_TABSTOP), output))
                return(FALSE);
        }

    /* maybe still output right margin */
    if(count <= rmargin-2)
        {
        for( ; count < rmargin-2; count++)
            if(!away_byte('.', output))
                return(FALSE);

        if(!away_byte(FWP_RMARGIN, output))
            return(FALSE);
        }

    /* output pitch, ragged/justify, line spacing */
    if( !away_byte(FWP_PICA, output)                            ||
        !away_byte((d_options_JU == 'Y') ? '1' : '0', output)   ||
        !away_byte(d_poptions_LS + '0', output)                 )
            return(FALSE);

    return(away_eol(output));
}

extern BOOL
fwp_save_file_preamble(
    FILE_HANDLE output,
    BOOL saving_fwp,
    ROW stt_row,
    ROW end_row)
{
    uchar array[LIN_BUFSIZ];

    trace_0(TRACE_APP_PD4, "fwp_save_file_preamble()\n");

    fwp_save_stt_row = stt_row;
    fwp_save_end_row = end_row;

    if(saving_fwp)
        {
        /* send out start of fwp file */

        /* mandatory page layout */
        (void) sprintf(array,
                        "\x1F" "0%02d%02d%02d%02d%02d000" "\x0A",
                        d_poptions_PL % 100,
                        d_poptions_TM % 100,
                        d_poptions_HM % 100,
                        d_poptions_FM % 100,
                        d_poptions_BM % 100);

        if(!away_string(array, output))
            return(FALSE);

        /* optional header and footer */
        if( !fwp_head_foot(d_poptions_HE, 1, output)    ||
            !fwp_head_foot(d_poptions_FO, 2, output)    )
                return(FALSE);

        /* no footnote format */

        /* mandatory ruler line */
        if(!fwp_ruler(output))
            return(FALSE);
        }

    return(TRUE);
}

extern BOOL
fwp_save_line_preamble(
    FILE_HANDLE output,
    BOOL saving_fwp)
{
    IGNOREPARM(output);
    IGNOREPARM(saving_fwp);

    trace_0(TRACE_APP_PD4, "fwp_save_line_preamble()\n");

    fwp_save_first_slot_on_line = TRUE;
    return(TRUE);
}

/******************************************************************************
*
*                           FWP file loading routines
*
******************************************************************************/

/******************************************************************************
*
* examine first few bytes of file to determine whether
* we have read a First Word Plus file
*
******************************************************************************/

extern BOOL
fwp_isfwpfile(
    const char *array)
{
    S32 i;
    PC_U8 ptr = array;
    BOOL maybe_fwp = FALSE;

    if((*ptr++ == FWP_FORMAT_LINE)  &&  (*ptr++ == '0'))
        {
        /* could be FWP page layout line.
         * If so next characters are ASCII digits giving page layout.
        */
        maybe_fwp = TRUE;

        for(i = 2; i < 12; ++i, ++ptr)
            if(!isdigit(*ptr))
                {
                maybe_fwp = FALSE;
                break;
                }
        }

    return(maybe_fwp);
}

/******************************************************************************
*
* get the ruler from a FWP file
*
******************************************************************************/

static S32
fwp_get_ruler(
    FILE_HANDLE loadinput)
{
    S32 width, i, c;
    COL tcol = 0;
    coord wrappoint = 0;
    coord wrapcount = 0;

    if(!createcol(0))
        return(-1);

    do  {
        width = 1;

        do  {
            wrapcount++;

            c = pd_file_getc(loadinput);

            switch(c)
                {
                case EOF:
                    return(-1);

                case CR:
                case LF:
                    c = CR;

                case ']':
                    /* fudge width one more so column heading doesn't coincide with
                        with right margin
                    */
                    width++;

                case '[':
                    /* set a new column at the left margin if not 0 position */
                    if(wrapcount == 1)
                        break;
                    /* deliberate fall thru */

                case '#':
                case FWP_TABSTOP:
                    /* got a tabstop */
                    if(!createcol(tcol))
                        return(-1);

                    set_width_and_wrap(tcol, width, 0);

                    if(c != ']')
                        goto NEXTCOL;

                    wrappoint = wrapcount+1;

                    for(i = 0; ; i++)
                        {
                        if((c = pd_file_getc(loadinput)) < 0)
                            return(c);

                        if((c == CR)  ||  (c == LF))
                            {
                            c = CR;
                            break;
                            }

                        /* look for justification and line spacing */
                        if(i == 1)
                            {
                            /* '0' is ragged, '1' is justify */
                            if(c == '0')
                                d_options_JU = 'N';
                            else if(c == '1')
                                d_options_JU = 'Y';
                            }

                        /* line spacing ? */
                        if(i == 2)
                            d_poptions_LS = c - '0';
                        }
                    break;

                default:
                    break;
                }

            width++;
            }
        while(c != CR);

    NEXTCOL:
        tcol++;
        }
    while(c != CR);

    /* set wrap point for all columns */
    for(tcol = 0; (wrappoint > 0)  &&  (tcol < numcol); tcol++)
        {
        width = colwidth(tcol);
        set_width_and_wrap(tcol, width, wrappoint);
        wrappoint -= width;
        }

    return(c);
}

/******************************************************************************
*
* get mandatory page layout and first
* ruler plus possible header and footer
* ignores any other odds & sods
*
******************************************************************************/

extern void
fwp_load_preinit(
    FILE_HANDLE loadinput,
    BOOL inserting)
{
    uchar array[LIN_BUFSIZ];
    BOOL no_layout = TRUE;
    BOOL no_ruler = TRUE;
    BOOL footer;
    S32 c, i;

    do  {
        footer = TRUE;

        if((c = pd_file_getc(loadinput)) < 0)
            return;

        if(c != FWP_FORMAT_LINE)
            continue;

        /* got a format line - process it */
        if((c = pd_file_getc(loadinput)) < 0)
            return;

        switch(c)
            {
            case FWP_PAGE_LAYOUT:
                if(no_layout)
                    {
                    no_layout = FALSE;

                    /* next ten bytes in ASCII decimal give margins */
                    if(file_read(array, 1, 10, loadinput) != 10)
                        return;

                    /* convert to sensible bytes */
                    for(i = 0; i < 5; ++i)
                        array[i] = (array[i*2] - '0') * 10 + array[i*2+1] - '0';

                    d_poptions_PL = array[0];
                    d_poptions_TM = array[1];
                    d_poptions_HM = array[2];
                    d_poptions_FM = array[3];
                    d_poptions_BM = array[4];
                    }
                break;

            case FWP_HEADER:
                footer = FALSE;

                /* deliberate fall-thru */

            case FWP_FOOTER:
                /* get three parameters. Force in '/', quick & nasty */
                    {
                    uchar *ptr = array+1;

                    *array = FWP_DELIMITER;

                    for(;;)
                        {
                        if((c = pd_file_getc(loadinput)) < 0)
                            return;

                        switch(c)
                            {
                            case FWP_FORMAT_LINE:
                                *ptr++ = FWP_DELIMITER;
                                continue;

                            case CR:
                            case LF:
                                *ptr = '\0';
                                break;

                            default:
                                *ptr++ = (uchar) c;
                                continue;
                            }

                        break;
                        }

                    mystr_set(footer ? &d_poptions_FO : &d_poptions_HE, array);
                    }
                break;

            case FWP_RULER:
                if(inserting)
                    no_ruler = FALSE;

                if(no_ruler)
                    {
                    no_ruler = FALSE;
                    c = fwp_get_ruler(loadinput);
                    break;
                    }
                /* deliberate fall-through if we don't want rulers */

            default:
                /* dunno wot this format line is so read past it */
                for(;;)
                    {
                    if((c = pd_file_getc(loadinput)) < 0)
                        return;

                    if((c == CR)  ||  (c == LF))
                        break;
                    }
                break;
            }
        }
    while(no_layout || no_ruler);
}

extern void
fwp_change_highlights(
    uchar new_byte,
    uchar old_byte)
{
    uchar diffs = new_byte ^ old_byte;
    S32 i;

    if(diffs)
        for(i = 0; i < 8; i++)
            if(fwp_highlights[i] & diffs)
                /* insert highlight i into linbuf */
                linbuf[lecpos++] = FIRST_HIGHLIGHT + i;
}

/*
* munge input characters from FWP file
* returns 0 if dealt with else char
*/

extern S32
fwp_convert(
    S32 c,
    FILE_HANDLE loadinput,
    uchar *field_separator,
    uchar *justify,
    uchar *h_byte,
    uchar *pageoffset,
    uchar *type)
{
    for(;;)
        {
        switch(c)
            {
            case FWP_INDENT_SPACE:
                /* got an indent so put in new column */
                c = *field_separator;
                break;

            case FWP_STRETCH_SPACE:
                /* this is what VIEW calls a soft space */
                if(*justify == J_FREE)
                    {
                    *justify = xf_leftright ? J_LEFTRIGHT : J_RIGHTLEFT;
                    xf_leftright = !xf_leftright;
                    d_options_JU = 'Y';
                    }

                c = 0;
                break;

            case FWP_SOFT_SPACE:
                /* this is what VIEW calls a hard space */
                c = SPACE;
                break;

            case FWP_ESCAPE_CHAR:
                if((c = pd_file_getc(loadinput)) < 0)
                    break;

                /* if top-bit set itsa a highlight change
                 * else ignore the rest of the line
                */
                if(c & 0x80)
                    {
                    /* highlight changes */
                    fwp_change_highlights((uchar) c, *h_byte);
                    *h_byte = c;
                    c = 0;
                    }
                else
                    {
                    do
                        if((c = pd_file_getc(loadinput)) < 0)
                            break;
                    while((c != CR)  &&  (c != LF));

                    continue; /* with newly-read char */
                    }

                break;

            case FWP_SOFT_PAGE:
                if((c = pd_file_getc(loadinput)) < 0)
                    break;

                /* c is condition strangely encoded */
                if(c > 0x80)
                    *pageoffset = 256 - c;
                else
                    *pageoffset = c - 16;

                /* deliberate fall-thru */

            case FWP_HARD_PAGE:
                *type = SL_PAGE;
                c = CR;     /* force onto new line */
                break;

            case TAB:
                break;

            default:
                if(c < 0x20)
                    {
                    c = 0; /* ignore all other control chars */
                    }
                break;
            }

        break;
        }

    return(c);
}

/* end of dtpsave.c */
