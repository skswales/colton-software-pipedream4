/* viewio.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that saves and loads VIEW files */

/* RJM 01-Mar-1989 */

#include "common/gflags.h"

#include "datafmt.h"

#include "viewio.h"

/* internal functions */

static S32
view_get_ruler(
    FILE_HANDLE loadinput);

static S32
view_get_stored_command(
    FILE_HANDLE loadinput,
    uchar *justify,
    uchar *type,
    uchar *pageoffset);

static BOOL pending_space;      /* early VIEW has habit of eating real spaces! */
static BOOL extended_highlights;
static BOOL had_subscript;
static BOOL had_superscript;

struct OPT_TABLE
{
    uchar p_OFFset;     /* offset in d_poptions */
    uchar v_default;
    uchar v_sc1;
    uchar v_sc2;
    uchar v_done;
};

/* indexes for VIEW stored commands which need to be dealt with but which
 * don't have parallels in d_poptions
*/

#define O_LJ    ((uchar) 128)
#define O_CE    ((uchar) 129)
#define O_RJ    ((uchar) 130)
#define O_PE    ((uchar) 131)
#define O_PB    ((uchar) 132)
#define O_HT    ((uchar) 133)

static struct OPT_TABLE view_opts[] =
{
    {   O_PL,   66,         'P',    'L',    FALSE   },
    {   O_HE,   255,        'D',    'H',    FALSE   },
    {   O_FO,   255,        'D',    'F',    FALSE   },
    {   O_LS,   0,          'L',    'S',    FALSE   },
    {   O_TM,   4,          'T',    'M',    FALSE   },
    {   O_HM,   4,          'H',    'M',    FALSE   },
    {   O_FM,   4,          'F',    'M',    FALSE   },
    {   O_BM,   4,          'B',    'M',    FALSE   },
    {   O_LM,   0,          'L',    'M',    FALSE   },
    {   O_PS,   255,        'S',    'R',    FALSE   },  /* convert start page to SR P n */
#define NO_OF_PAGE_OPTS 10U
    {   O_PE,   J_FREE,     'P',    'E',    FALSE   },
    {   O_PB,   J_FREE,     'P',    'B',    FALSE   },
    {   O_LJ,   J_LEFT,     'L',    'J',    FALSE   },
    {   O_CE,   J_CENTRE,   'C',    'E',    FALSE   },
    {   O_RJ,   J_RIGHT,    'R',    'J',    FALSE   },
    {   O_HT,   0,          'H',    'T',    FALSE   }
};
#define NO_OF_ALL_OPTS elemof(view_opts)

static const PC_USTR highlight_table[] =
{
    "\x1C",                         /* PD H1 */
    "\x1D\x1D\x1D",                 /* PD H2 */
    "",                             /* PD H3 */
    "\x1D\x1C\x1D",                 /* PD H4 */
    "\x1D\x1C",                     /* PD H5 */
    "\x1D\x1D",                     /* PD H6 */
    "\x1D\x1C\x1C",                 /* PD H7 */
    "",                             /* PD H8 */
};
#define HTABLE_SIZE 8

#if 0
                                          /* >?@   ABCDEFGHIJK   LMNOPQRSTUVWXYZ[\] */
static const char *extended_character_set = "© \xa0ÄÖÜäöüß`éè\x9a_ùç  °¶¢¡~§® ¥µ¼½¾";

#else

static const char
extended_character_set[] =
{
    '\xA9', /* > -> © */
    '\x20', /* ? ->  */
    '\xA0', /* @ -> \xa0 */
    '\xC4', /* A -> Ä */
    '\xD6', /* B -> Ö */
    '\xDC', /* C -> Ü */
    '\xE4', /* D -> ä */
    '\xF6', /* E -> ö */
    '\xFC', /* F -> ü */
    '\xDF', /* G -> ß */
    '\x60', /* H -> ` */
    '\xE9', /* I -> é */
    '\xE8', /* J -> è */
    '\x9A', /* K -> \x9a */
    '\x5F', /* L ->  _*/
    '\xF9', /* M -> ù */
    '\xE7', /* N -> ç */
    '\x20', /* O ->   */
    '\x20', /* P ->   */
    '\xB0', /* Q -> ° */
    '\xB6', /* R -> ¶ */
    '\xA2', /* S -> ¢ */
    '\xA1', /* T -> ¡ */
    '\x7E', /* U -> ~ */
    '\xA7', /* V -> § */
    '\xAE', /* W -> ® */
    '\x20', /* X ->   */
    '\xA5', /* Y -> ¥ */
    '\xB5', /* Z -> µ */
    '\xBC', /* [ -> ¼ */
    '\xBD', /* \ -> ½ */
    '\xBE'  /* ] -> ¾ */
};

#endif

extern S32
view_convert(
    S32 c,
    FILE_HANDLE loadinput,
    uchar *lastch,
    uchar *type,
    uchar *justify,
    BOOL *been_ruler,
    uchar *pageoffset)
{
    trace_2(TRACE_APP_PD4, "view_convert(%c %d)", c, c);

    if(pending_space)
    {
        pending_space = FALSE;

        switch(c)
        {
        case CR:
        case LF:
        case SPACE:
            break;

        default:
            linbuf[lecpos++] = SPACE;
            break;
        }
    }

    for(;;) /* There are two ranges of View control characters, < 0x20 and >= 0x80 */
    {
        if(c < 0x20)
        {
            switch(c)
            {
            default:
                c = 0; /* ignore all other control chars */
                break;

            case TAB:
            case VIEW_LINESEP: /*CR*/
                break;

            case VIEW_HIGH_UNDERLINE:
                c = HIGH_UNDERLINE;
                break;

            case VIEW_HIGH_BOLD:
                if(extended_highlights)
                {
                    /* decode as extended highlights */
                    if((c = pd_file_getc(loadinput)) < 0)
                        return(c);

                    switch(c)
                    {
                    case VIEW_HIGH_UNDERLINE:
                        /* H4, H5, H7 or RESET */
                        if((c = pd_file_getc(loadinput)) < 0)
                            return(c);

                        switch(c)
                        {
                        case VIEW_HIGH_BOLD:
                            /* H4 */
                            c = HIGH_ITALIC;
                            break;

                        case VIEW_HIGH_UNDERLINE:
                            /* H7 or RESET */
                            if((c = pd_file_getc(loadinput)) < 0)
                                return(c);

                            if(c == VIEW_HIGH_UNDERLINE)
                            {
                                trace_0(TRACE_APP_PD4, "view_convert got extended printer reset --- ignored");
                                c = 0;
                            }
                            else
                            {
                                /* H7 */
                                *lastch = linbuf[lecpos++] = FIRST_HIGHLIGHT + 7 - 1;
                                continue; /* as we've an unprocessed char in c */
                            }

                            break;

                        default:
                            /* H5 */
                            if(had_superscript)
                            {
                                had_superscript = FALSE;
                                linbuf[lecpos++] = HIGH_SUPERSCRIPT;
                            }

                            had_subscript = TRUE;
                            *lastch = linbuf[lecpos++] = HIGH_SUBSCRIPT;
                            continue; /* as we've an unprocessed char in c */
                        }

                        break;

                    case VIEW_HIGH_BOLD:
                        /* H2, H6 or end H5/H6 */
                        if((c = pd_file_getc(loadinput)) < 0)
                            return(c);

                        if(c == VIEW_HIGH_BOLD)
                            /* H2 */
                            c = HIGH_BOLD;
                        else if(c == VIEW_HIGH_UNDERLINE)
                        {
                            /* end H5/H6 */
                            if(had_subscript)
                            {
                                had_subscript = FALSE;
                                c = HIGH_SUBSCRIPT;
                            }
                            else if(had_superscript)
                            {
                                had_superscript = FALSE;
                                c = HIGH_SUPERSCRIPT;
                            }
                            else
                            {
                                trace_0(TRACE_APP_PD4, "view_convert: superfluous sub/super normalisation]");
                                c = 0;
                            }
                        }
                        else
                        {
                            /* H6 */
                            if(had_subscript)
                            {
                                had_subscript = FALSE;
                                linbuf[lecpos++] = HIGH_SUBSCRIPT;
                            }

                            had_superscript = TRUE;
                            *lastch = linbuf[lecpos++] = HIGH_SUPERSCRIPT;
                            continue; /* as we've an unprocessed char in c */
                        }

                        break;

                    default:
                        /* extended character set? */
                        trace_2(TRACE_APP_PD4, "view_convert got h2 with %c %2x - extended char?", c, c);
                        if((c >= '>')  &&  (c <= '>' + 32))
                            c = extended_character_set[c - '>'];
                        trace_2(TRACE_APP_PD4, "view_convert got extended char %c %2x", c, c);

                        break;
                    }
                }
                else
                    c = HIGH_BOLD;

                break;

            case VIEW_LEFT_MARGIN:
                c = TAB;
                break;

            case '|':
                if((c = pd_file_getc(loadinput)) < 0)
                    return(c);

                switch(toupper(c))
                {
                case 'D':
                case 'P':
                    if(CH_NULL != get_text_at_char())
                    {
                        linbuf[lecpos++] = get_text_at_char();
                        *lastch = linbuf[lecpos++] = (char) c;
                        c = get_text_at_char();
                        break;
                    }

                    /*DROPTHRU*/

                default:
                    *lastch = linbuf[lecpos++] = '|';
                    continue; /* unprocessed char in c */
                }

                break;

            case VIEW_SOFT_SPACE:
                if(*justify == J_FREE)
                {
                    *justify = xf_leftright ? J_LEFTRIGHT : J_RIGHTLEFT;
                    xf_leftright = !xf_leftright;
                    d_options_JU = 'Y';
                }

                pending_space = TRUE;

                c = 0; /* soft space processed - get more input */
                break;
            }
        }
        else if(c >= 0x80)
        {
            switch(c)
            {
            default:
                break;

            /* look for ruler or stored command at beginning of line */
            case VIEW_STORED_COMMAND:
                if(lecpos == 0)
                    c = view_get_stored_command(loadinput, justify, type, pageoffset);

                break;

            case VIEW_RULER:
                if(!*been_ruler  &&  !lecpos)
                {
                    c = view_get_ruler(loadinput);
                    *been_ruler = TRUE;
                }
                else
                {   /* read and ignore ruler */
                    for(;;)
                    {
                        if((c = pd_file_getc(loadinput)) < 0)
                            return(c);

                        if((c == CR)  ||  (c == LF))
                            break;
                    }

                    c = 0; /* ruler ignored - get more input */
                }

                break;
            }
        }

        break;
        }

    trace_2(TRACE_APP_PD4, "view_convert returns %c %d", c, c);
    return(c);
}

/******************************************************************************
*
* read ruler and set up columns
*
******************************************************************************/

static S32
view_get_ruler(
    FILE_HANDLE loadinput)
{
    S32 c, width;
    COL tcol = 0;
    coord wrappoint = 0;
    coord wrapcount = 0;

    if(((c = pd_file_getc(loadinput)) != '.')  ||  ((c = pd_file_getc(loadinput)) != '.'))
        return(c);

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

            case '>':
            case '*':
                trace_2(TRACE_APP_PD4, "view_get_ruler creating column %d because %c found in ruler", tcol, c == CR ? 'e' : c);
                if(!createcol(tcol))
                    return(-1);

                set_width_and_wrap(tcol, width, 0);
                goto NEXTCOL;

            case '<':
                wrappoint = wrapcount+1;
                break;

            default:
                break;
            }

            width++;
        }
        while(c != CR);

    NEXTCOL:

        ++tcol;
    }
    while(c != CR);

    /* set wrap point for all columns */
    for(tcol = 0; (wrappoint > 0)  &&  (tcol < numcol); tcol++)
    {
        width = colwidth(tcol);
        set_width_and_wrap(tcol, width, wrappoint);
        wrappoint -= width;
    }

    return(0);      /* ruler processed - get more input */
}

/******************************************************************************
*
* read stored command and set up any PipeDream flags
*
******************************************************************************/

static S32
view_get_stored_command(
    FILE_HANDLE loadinput,
    uchar *justify,
    uchar *type,
    uchar *pageoffset)
{
    S32 c, ch1, ch2;
    U32 a_index = 0;
    char array[LIN_BUFSIZ];
    char cvtarray[LIN_BUFSIZ];
    PC_USTR from;
    P_U8 to;
    S32 ch, add_in;
    S32 number, option;
    struct OPT_TABLE *optptr;

    ch1 = pd_file_getc(loadinput);
    if(!isupper(ch1))
        return(ch1);

    ch2 = pd_file_getc(loadinput);
    if(!isupper(ch2))
        return(ch2);

    trace_2(TRACE_APP_PD4, "view_get_stored_command: Got stored command %c%c", ch1, ch2);

    to = array;
    do  {
        c = pd_file_getc(loadinput);

        trace_2(TRACE_APP_PD4, "got char %c %d for stored command", c, c);

        switch(c)
        {
        case EOF:
            return(-1);

        case CR:
        case LF:
            c = CH_NULL;
            break;

        default:
            break;
        }

        if(PtrDiffBytesU32(to, array) >= sizeof(array))
            return(-1);

        *to++ = (char) c;
    }
    while(CH_NULL != c);

    trace_1(TRACE_APP_PD4, "view_get_stored_command: got parameters '%s'", array);

    a_index = 0;
    optptr = &view_opts[a_index];
    do  {
        trace_2(TRACE_APP_PD4, "Comparing with stored command %c%c", optptr->v_sc1, optptr->v_sc2);

        if((ch1 == optptr->v_sc1)  &&  (ch2 == optptr->v_sc2))
        {
            trace_2(TRACE_APP_PD4, "Matched stored command %c%c", ch1, ch2);

            update_dialog_from_windvars(D_POPTIONS);

            /* only store first occurrence of option */
            if(!optptr->v_done)
            {
                optptr->v_done = TRUE;

                option = optptr->p_OFFset;

                add_in = 0;

                switch(option)
                {
                case O_HE:
                case O_FO:
                    /* convert page and dates in headers & footers */
                    from = array;
                    to   = cvtarray;
                    do  {
                        ch = *from++;

                        if(ch == '|')
                            switch(toupper(*from))
                            {
                            case 'D':
                            case 'P':
                                if(CH_NULL != get_text_at_char())
                                {
                                    *to++ = get_text_at_char();
                                    *to++ = *from++;
                                    ch = get_text_at_char();
                                    break;
                                }
                                /*DROPTHRU*/

                            default:
                                break;
                            }

                        *to++ = (char) ch;
                    }
                    while(ch);

                    consume_bool(mystr_set(&d_poptions[option].textfield, cvtarray));
                    break;

                case O_LS:
                    add_in = 1;

                case O_PL:
                case O_TM:
                case O_HM:
                case O_FM:
                case O_BM:
                case O_LM:
                    number = atoi(array);

                    if( number < 0)
                        number = 0;

                    d_poptions[option].option = (uchar) (number + add_in);
                    break;

                case O_PB:
                    /* get status, page breaks always on unless
                     * 0* or off* after leading spaces
                    */
                    from = array;

                    while((ch = *from++) == SPACE)
                        ;

                    if(!ch  ||  (0 == _stricmp(from - 1, "OFF")))
                        d_poptions[O_PL].option = 0;

                    break;

                case O_HT:
                    /* check HT 2 definition */
                    from = array;

                    while((ch = *from++) == SPACE)
                        ;

                    if(ch == '2')
                    {
                        extended_highlights = (atoi(from) == 130);
                        trace_2(TRACE_APP_PD4, "view_get_stored_command: HT 2 %s -> extended highlights %s", from, report_boolstring(extended_highlights));
                    }

                    optptr->v_done = FALSE;     /* can do many times */
                    break;

                case O_PS:
                    /* ignore set register commands */
                    break;

                case O_PE:
                    /* set page break */
                    *type = SL_PAGE;
                    *pageoffset = (uchar) atoi(array);

                    /* falls through */

            /*  case O_CE:  */
            /*  case O_LJ:  */
            /*  case O_RJ:  */
                default:
                    /* set justify state and get line */
                    *justify = optptr->v_default;
                    lecpos = strlen(strcpy(linbuf, array));
                    optptr->v_done = FALSE;     /* can do many times */
                    c = CR;
                    break;
                }
            }

            update_windvars_from_dialog(D_POPTIONS);

            return(c);
        }

        ++optptr;
    }
    while(++a_index < NO_OF_ALL_OPTS);

    /* unrecognised stored command - just put into linbuf */
    linbuf[0] = (uchar) ch1;
    linbuf[1] = (uchar) ch2;
    strcpy(linbuf + 2, array);
    lecpos = strlen(linbuf);

    return(CR);
}

extern void
view_load_line_ended(void)
{
    had_superscript = had_subscript = FALSE;
}

/******************************************************************************
*
* fudge top and bottom margins if no header or footer
* don't leave line spacing set to 0
*
******************************************************************************/

extern void
view_load_postinit(void)
{
    trace_0(TRACE_APP_PD4, "view_load_postinit()");

    if(str_isblank(d_poptions_HE))
        d_poptions_TM++;

    if(str_isblank(d_poptions_FO))
        d_poptions_FM++;

    if( d_poptions_LS == 0)
        d_poptions_LS = 1;
}

/******************************************************************************
*
*   initialise stored command table and d_poptions
*
******************************************************************************/

extern void
view_load_preinit(
    BOOL *been_ruler,
    BOOL inserting)
{
    U32 opt_idx;
    S32 offset, def;

    trace_0(TRACE_APP_PD4, "view_load_preinit()");

    /* if inserting ignore any rulers that come in */
    *been_ruler = inserting;

    update_dialog_from_windvars(D_POPTIONS);

    opt_idx = 0;
    do  {
        offset = view_opts[opt_idx].p_OFFset;
        def    = view_opts[opt_idx].v_default;

        view_opts[opt_idx].v_done = FALSE;

        if(def == 255)
            consume_bool(mystr_set(&d_poptions[offset].textfield, NULL));
        else
            d_poptions[offset].option = (uchar) def;
    }
    while(++opt_idx < NO_OF_PAGE_OPTS);

    /*count = NO_OF_PAGE_OPTS;*/
    do  {
        view_opts[opt_idx].v_done = FALSE;
    }
    while(++opt_idx < NO_OF_ALL_OPTS);

    update_windvars_from_dialog(D_POPTIONS);

    pending_space = extended_highlights = had_subscript = had_superscript = FALSE;
}

/******************************************************************************
*
* save VIEW ruler and general file options
*
******************************************************************************/

extern BOOL
view_save_ruler_options(
    coord *rightmargin,
    FILE_HANDLE output)
{
    coord rmargin, count, thiswidth, scount;
    U32 opt_idx;
    COL tcol;
    S32 offset;
    uchar array[20];
    BOOL out_stored;
    BOOL blank;
    BOOL noheader = TRUE;
    BOOL nofooter = TRUE;
    S32 margin;
    PC_USTR from;
    P_U8 to;
    S32 ch;
    BOOL res = TRUE;

    /* Output VIEW ruler.  Just output tab stops at each column position
     * and right margin at column A right margin position
    */
    if(!away_byte(VIEW_RULER, output)  ||  !away_string("..", output))
        return(FALSE);

    /* find first column on_screen and get its right margin */
    for(tcol = 0; colwidth(tcol) == 0; ++tcol)
        ;

    if((rmargin = colwrapwidth(tcol)) == 0)
        rmargin = colwidth(tcol);

    /* nothing silly, thank you */
    if( rmargin > 130)
        rmargin = 130;

    *rightmargin = rmargin;

    /* output dots */
    for(count = tcol = 0; (tcol < numcol)  &&  (count < 130); tcol++)
    {
        thiswidth = colwidth(tcol);
        scount = 0;

        for( ; scount < thiswidth-1 && count < 130; scount++)
            if(!away_byte((uchar) ((count++ == rmargin-2) ? '<' : '.'), output))
                return(FALSE);

        /* output tabstop unless it is last position */
        if(tcol < numcol-1 &&
                !away_byte((uchar) ((count++ == rmargin-2) ? '<' : '*'), output))
            return(FALSE);
    }

    /* maybe still output right margin */
    if(count <= rmargin-2)
    {
        for( ; count < rmargin-2; count++)
            if(!away_byte('.', output))
                return(FALSE);

        if(!away_byte('<', output))
            return(FALSE);
    }

    if(!away_eol(output))
        return(FALSE);

    /* output page information */

    update_dialog_from_windvars(D_POPTIONS);

    for(opt_idx = 0; res  &&  (opt_idx < NO_OF_PAGE_OPTS); opt_idx++)
    {
        out_stored = FALSE;
        margin = -1;

        /* set up 2 letter stored command */
        *array   = view_opts[opt_idx].v_sc1;
        array[1] = view_opts[opt_idx].v_sc2;

        offset = view_opts[opt_idx].p_OFFset;

        switch(offset)
        {
        case O_HE:
        case O_FO:
            /* if there isn't a header(footer) we need to subtract one from
                either the top(bottom) margin or header(footer) margin
            */
            blank = str_isblank(d_poptions[offset].textfield);

            if(offset == O_HE)
                noheader = blank;
            else
                nofooter = blank;

            if(!blank)
            {
                from = d_poptions[offset].textfield;
                to   = array + 2;
                do  {
                    ch = *from++;

                    if(is_text_at_char(ch))
                        switch(toupper(*from))
                        {
                        case 'D':
                        case 'P':
                            if(is_text_at_char(*(from+1)))
                            {
                                *to++ = '|';
                                *to++ = *from++;
                                /* and skip spurious text-at chars too */
                                do
                                    ch = *from++;
                                while(is_text_at_char(ch));
                            }
                            break;

                        default:
                            break;
                        }

                    *to++ = (char) ch;
                }
                while(ch);

                out_stored = TRUE;
            }

            break;

        case O_TM:
        case O_HM:
            margin = d_poptions[offset].option;

            if(noheader  &&  (margin > 0))
            {
                /* no header so subtract one from margin */
                noheader = FALSE;
                margin--;
            }

            /* don't bother if same as VIEW default */
            if(view_opts[opt_idx].v_default == (uchar) margin)
                margin = -1;

            break;

        case O_FM:
        case O_BM:
            margin = d_poptions[offset].option;

            if(nofooter && margin > 0)
            {
                nofooter = FALSE;
                margin--;
            }

            /* don't bother if same as VIEW default */
            if(view_opts[opt_idx].v_default == (uchar) margin)
                margin = -1;

            break;

        case O_PL:
        case O_LM:
            /* PD and VIEW the same */
            if(view_opts[opt_idx].v_default != d_poptions[offset].option)
                margin = d_poptions[offset].option;

            break;

        case O_LS:
            /* PD line spacing one greater than VIEW line spacing
             * Don't bother if LS=0 in PD
            */
            if(d_poptions[O_LS].option > 1)
                margin = d_poptions[O_LS].option - 1;

            break;

        case O_PS:
            /* start page - output SR P n */
            if(!str_isblank(d_poptions[O_PS].textfield))
            {
                (void) xsnprintf(array + 2, elemof32(array) - 2, "P %s", d_poptions[O_PS].textfield);
                out_stored = TRUE;
            }

            break;

        default:
            break;
        }

        /* if parameter has been set, build stored command */
        if(margin >= 0)
        {
            consume_int(sprintf(array + 2, "%d", margin));
            out_stored = TRUE;
        }

        if(out_stored)
            if(!view_save_stored_command(array, output)  ||  !away_eol(output))
                res = FALSE;
    }

    update_windvars_from_dialog(D_POPTIONS);

    /* enable extended highlights */
    if(res  &&  (!view_save_stored_command("HT2 130", output)  ||  !away_eol(output)))
        res = FALSE;

    /* reset print effects */
    if(res  &&  !away_string("\x1d\x1c\x1c\x1c", output))
        res = FALSE;

    return(res);
}

/******************************************************************************
*
* save cell in VIEW format
* may only output ~132 chars on line, so at 130 print CR and set warning
*
******************************************************************************/

extern BOOL
view_save_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    coord *v_chars_sofar,
    P_S32 splitlines,
    coord rightmargin)
{
    const uchar text_at_char = get_text_at_char();
    uchar *lptr, ch;
    uchar justify = tcell->justify & J_BITS;

    if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
    {
        /* set null terminator */
        memset32(linbuf, 0, LIN_BUFSIZ);

        /* expand justified line into linbuf, not forgetting terminator */
        justifyline(tcell->content.text, rightmargin - *v_chars_sofar, justify, linbuf);
    }
    else
        /* write just text or formula part of cell out */
        plain_slot(tcell, tcol, trow, VIEW_CHAR, linbuf);

    /* output contents, dealing with highlight chars */
    for(lptr = linbuf; *lptr; lptr++)
    {
        ch = *lptr;

        if(*v_chars_sofar >= 130)
        {
            *splitlines += 1;
            *v_chars_sofar = 0;
            if(!away_eol(output))
                return(FALSE);
        }

        switch(ch)
        {
        /* soft spaces inserted by justify line are 0x1A which is H3 */
        case TEMP_SOFT_SPACE:
            if(!away_byte(VIEW_SOFT_SPACE, output))
                return(FALSE);

            break;

        default:
            if((ch == text_at_char) && (CH_NULL != text_at_char)) /* case text_at_char: */
            {
                ch = *++lptr;

                switch(toupper(ch))
                {
                case 'P':
                case 'D':
                    if(!away_byte('|', output)  ||  !away_byte(ch, output))
                        return(FALSE);

                    while(*++lptr == text_at_char)
                        ;

                    --lptr;
                    break;

                default:
                    if(ch == text_at_char)
                        ++lptr;

                    --lptr;
                    if(!away_byte(text_at_char, output))
                        return(FALSE);
                    break;
                }

                break;
            }

            if(ishighlight(ch))
            {
                if(!away_string(highlight_table[ch - FIRST_HIGHLIGHT], output))
                    return(FALSE);
            }
            else if(!away_byte(ch, output))
                return(FALSE);

            break;
        }
    }

    return(TRUE);
}

/******************************************************************************
*
*  output stored command to VIEW file
*
******************************************************************************/

extern BOOL
view_save_stored_command(
    const char *command,
    FILE_HANDLE output)
{
    return(away_byte((uchar) VIEW_STORED_COMMAND, output)  &&  away_string(command, output));
}

/* end of viewio.c */
