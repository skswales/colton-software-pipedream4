/* slotconv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Slot to text conversion module */

/* RJM 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __colourtran_h
#include "colourtran.h"
#endif

#include "riscos_x.h"
#include "riscdraw.h"

/*
need to know about ev_dec_range()
*/

#include "cmodules/ev_evali.h"

/*
entry in the font list
*/

typedef struct font_cache_entry
{
    font handle;
    S32 xsize;
    S32 ysize;
    /* U8Z name[] follows */
}
* fep;

/*
local routines
*/

static font
font_cache(
    _In_reads_bytes_(namlen) const char *name,
    S32 namlen,
    S32 xs,
    S32 ys);

static void
font_cancel_hilites(
    char **pto);

static font
font_derive(
    font handle,
    _In_z_      const char * name,
    _In_z_      const char * supplement,
    _InVal_     BOOL add);

static fep
font_find_block(
    _InVal_     font fonthan);

static font
font_get_global(void);

static void
font_insert_change(
    font handle,
    char **pto);

static void
font_insert_colour_change(
    S32 r,
    S32 g,
    S32 b,
    char **pto);

static S32
font_stringwid(
    font str_font,
    char *str);

static STATUS
font_strip(
    _Out_writes_z_(elemof_newname) char * newname,
    _InVal_     U32 elemof_newname,
    _In_z_      const char * fontname,
    _In_z_      const char * strip);

static S32
font_squash(
    S32 ysize);

static void
font_super_sub(
    S32 hilite_n,
    char **pto);

static void
result_to_string(
    P_EV_RESULT resp,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    BOOL expand_ats,
    BOOL expand_ctrl);

static S32
sprintnumber(
    char *array_start,
    char *array,
    char format,
    F64 value,
    BOOL eformat,
    S32 fwidth);

#define NUM_HILITES 8
#define FONT_CHANGE 26
#define UL_CHANGE   25
#define COL_INVERT  18
/*#define COL_CHANGE  17*/
#define COL_RGB_CHANGE 19
#define Y_SHIFT     11
#define X_SHIFT     9

static P_LIST_BLOCK font_cache_list = NULL;
static font current_font = 0;
static font slot_font;
static char hilite_state[NUM_HILITES];
static font start_font = 0;
static S32 current_shift = 0;
static S32 old_height = 0;

static PC_U8 *
asc_months[] =
{
    &month_January_STR,
    &month_February_STR,
    &month_March_STR,
    &month_April_STR,
    &month_May_STR,
    &month_June_STR,
    &month_July_STR,
    &month_August_STR,
    &month_September_STR,
    &month_October_STR,
    &month_November_STR,
    &month_December_STR
};

/******************************************************************************
*
* get a colour value from a string
*
******************************************************************************/

static int
get_colour_value(
    PC_USTR pos,
    /*out*/ int * p_r,
    /*out*/ int * p_g,
    /*out*/ int * p_b)
{
    PC_USTR spos = pos;
    P_USTR epos;
    U32 r, g, b;

    *p_r = *p_g = *p_b = 0;

    r = fast_strtoul(pos, &epos);
    if(pos == epos) return(0);
    pos = epos;

    if(*pos++ != ',') return(0);

    g = fast_strtoul(pos, &epos);
    if(pos == epos)
        return(0);
    pos = epos;

    if(*pos++ != ',') return(0);

    b = fast_strtoul(pos, &epos);
    if(pos == epos)
        return(0);

    *p_r = r;
    *p_g = g;
    *p_b = b;

    return(epos - spos);
}

/******************************************************************************
*
*  copy from "from" to "to" expanding text-at fields
*
*  MRJC added draw file handling 28/6/89
*  MRJC added font handling 30/6/89
*
******************************************************************************/

static void
bitstr(
    const char *from,
    ROW row,
    char * buffer /*out*/,
    coord fwidth,
    S32 expand_refs,
    BOOL expand_ats,
    BOOL expand_ctrl)
{
    char * to = buffer;
    const uchar text_at_char = get_text_at_char();
    coord length = 0;
    S32 consume;
    P_SLOT tslot;
    COL tcol;
    ROW trow;
    EV_DOCNO docno;
    coord reflength;
    char *tempto;
    S32 i = 0;
    BOOL revert_colour_change = FALSE;

    /* start slot with global font */
    if(riscos_fonts)
        {
        start_font = 0;
        current_shift = old_height = 0;

        /* truncated at a later point */
        fwidth = 5000;
        }

    do { hilite_state[i] = 0; } while(++i < NUM_HILITES);

    while(*from  &&  (length <= fwidth))
        {
        /* consume = not expanding text-at chars, yet zero width field,
         * so eat up all trailing text-at chars so they have no width
        */
        if((*from != text_at_char)  /*||  (NULLCH == text_at_char)*/)
            {
            if(SLRLD1 == *from) /* have to cater for orphaned SLRLD1 (text_at_char changed) */
                { /* we need to do this otherwise we get the SLRLD1 character and following bytes displayed... */
                const uchar * csr = from + 2; /* CSR is past the SLRLD1/2 */

                from = talps_csr(csr, &docno, &tcol, &trow);
                /*eportf("bitstr: decompiled (orphaned) CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);*/

                reflength = write_ref(to, BUF_MAX_REFERENCE, docno, tcol, trow);
                to += reflength;
                length += reflength+1;

                continue;
                }

            if(!ishighlight(*from))
                {
                /* check for C0 control characters & Acorn C1 redefs */
                if(expand_ctrl                                                                                         &&
                    ((*from < CH_SPACE) || (CH_DELETE == *from) ||
                     (riscos_fonts  &&  (*from >= 128)  &&  (*from < 160)  &&  !font_charwid(current_font, *from)))
                                                                                                                       )
                    {
                    to += sprintf(to, "[%.2x]", (S32) *from);
                    length += 4;
                    from++;
                    continue;
                    }

                *to++ = *from++;
                length++;
                continue;
                }

            /* process highlights */
            if(!riscos_fonts)
                {
                hilite_state[*from - FIRST_HIGHLIGHT] ^= 1;
                *to++ = *from++;
                continue;
                }

            /* turn highlights into font changes on RISC OS */
            switch(*from)
                {
                case HIGH_UNDERLINE:
                    trace_0(TRACE_APP_PD4, "bitstr highlight 1 underline");

                    *to++ = UL_CHANGE;

                    /* underline pos */
                    *to++ = 242;

                    /* underline height */
                    if(!hilite_state[0])
                        {
                        if(sqobit)
                            *to++ = 10;
                        else
                            *to++ = 14;
                        }
                    else
                        *to++ = 0;

                    hilite_state[0] ^= 1;
                    break;

                case HIGH_BOLD:
                    {
                    fep fp = font_find_block(current_font);

                    trace_2(TRACE_APP_PD4, "bitstr highlight 2 bold B->%d I=%d", !hilite_state[1], hilite_state[3]);

                    if(NULL != fp)
                        {
                        PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                        font newfont = 0;
                        char basefont[BUF_MAX_PATHSTRING];

                        /* first strip off any Italic suffixes - they are always at the end if present */
                        if(!status_done(font_strip(basefont, elemof32(basefont), fp_name, ".Italic")))
                            (void) font_strip(basefont, elemof32(basefont), fp_name, ".Oblique");

                        /* basefont contains name stripped of any Italic suffixes */

                        if(hilite_state[1])
                            {
                            /* try removing any Bold suffix */
                            (void) font_strip(basefont, elemof32(basefont), basefont, ".Bold");

                            /* basefont contains name stripped of any Bold and Italic suffixes */

                            /* try to derive the base font from the attribute-stripped current font */
                            newfont = font_derive(current_font, basefont, ".Medium", TRUE);

                            if(newfont < 1)
                                newfont = font_derive(current_font, basefont, ".Roman", TRUE);

                            if(newfont < 1)
                                newfont = font_derive(current_font, basefont, "", TRUE);

                            if(hilite_state[3] && (newfont > 1))
                                { /* try to derive an Italic version iff base font found (saves having to try all possible permutations of Normal & Italic) */
                                font newfont_i;

                                fp = font_find_block(newfont);
                                __assume(fp);
                                fp_name = (PC_U8Z) (fp + 1);

                                newfont_i = font_derive(newfont, fp_name, ".Italic", TRUE);

                                if(newfont_i < 1)
                                    newfont_i = font_derive(newfont, fp_name, ".Oblique", TRUE);

                                if(newfont_i > 0)
                                    newfont = newfont_i;
                                }
                            }
                        else
                            {
                            /* try removing any Normal suffixes */
                            if(!status_done(font_strip(basefont, elemof32(basefont), basefont, ".Medium")))
                                (void) font_strip(basefont, elemof32(basefont), basefont, ".Roman");

                            /* basefont contains name stripped of any Normal and Italic suffixes */

                            if(hilite_state[3])
                                { /* try to derive a Bold Italic version */
                                newfont = font_derive(current_font, basefont, ".Bold.Italic", TRUE);

                                if(newfont < 1)
                                    newfont = font_derive(current_font, basefont, ".Bold.Oblique", TRUE);
                                }

                            /* at least try to derive a Bold version (either that's what we wanted or Bold Italic just failed) */
                            if(newfont < 1)
                                newfont = font_derive(current_font, basefont, ".Bold", TRUE);
                            }

                        if(newfont > 0)
                            font_insert_change(newfont, &to);
                        }

                    hilite_state[1] ^= 1;
                    break;
                    }

                case HIGH_ITALIC:
                    {
                    fep fp = font_find_block(current_font);

                    trace_2(TRACE_APP_PD4, "bitstr highlight 4 italic B=%d I->%d", hilite_state[1], !hilite_state[3]);

                    if(NULL != fp)
                        {
                        PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                        const BOOL add = !hilite_state[3];
                        font newfont;

                        /* just need to try adding or removing Italic suffixes as they are always at the end */
                        newfont = font_derive(current_font, fp_name, ".Italic", add);

                        if(newfont < 1)
                            newfont = font_derive(current_font, fp_name, ".Oblique", add);

                        if(newfont > 0)
                            font_insert_change(newfont, &to);
                    }

                    hilite_state[3] ^= 1;
                    break;
                    }

                case HIGH_SUBSCRIPT:
                    font_super_sub(4, &to);
                    break;

                case HIGH_SUPERSCRIPT:
                    font_super_sub(5, &to);
                    break;

                /* other highlights disappear into a big pit */
                default:
                    break;
                }

            ++from;
            continue;

            *to++ = *from++;
            continue;
            }

        /* text-at chars drop thru to here */

        /* skip leading text-at char */
        ++from;

        *to = NULLCH;
        consume = 0;
        switch(toupper(*from))
            {
            case SLRLD1:
                { /* expand a compiled slot reference */
                const uchar * csr = from + 2; /* CSR is past the SLRLD1/2 */

                from = talps_csr(csr, &docno, &tcol, &trow);
                /*eportf("bitstr: CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);*/

                if(!expand_ats)
                    break;

                /* and expand it into current slot */
                if((expand_refs > 0) && !bad_reference(tcol, trow))
                    {
                    tslot = travel_externally(docno, tcol & COLNOBITS, trow & ROWNOBITS);

                    if(tslot)
                        (void) expand_slot(docno, tslot, row, to, fwidth,
                                           expand_refs-1, expand_ats, expand_ctrl,
                                           FALSE /*allow_fonty_result*/, TRUE /*cff*/);

                    /* skip string and delete funny spaces from plain non-fonty string */
                    tempto = to;
                    while(*to)
                        {
                        /* evict highlights and funny spaces */
                        if(*to < SPACE)
                            {
                            ++to;
                            continue;
                            }

                        *tempto++ = *to++;
                        ++length;
                        }

                    to = tempto;
                    }
                else
                    {
                    /* if we are not allowed to expand it because we are
                     * already expanding, or error, just print slot reference
                    */
                    *to++ = text_at_char;
                    reflength = write_ref(to, BUF_MAX_REFERENCE, docno, tcol, trow);
                    to += reflength;
                    length += reflength+1;
                    *to++ = text_at_char;
                    }

                *to = '\0';
                }
                break;

            /* display today's date */
            case 'D':
                if(from[1] == text_at_char)
                    {
                    struct tm *temp_time;
                    DATE now;
                    int month, mday, year;

                    ++from;

                    if(!expand_ats)
                        break;

                    time(&now);
                    temp_time = localtime(&now);

                    month = temp_time->tm_mon+1;
                    mday  = temp_time->tm_mday;
                    year  = temp_time->tm_year;

                    year += 1900; /* SKS 01 Dec 1996 says sod them. They're having the proper date from now on */

                    if(d_options_DF == 'T')
                        /* generate 21 March 1988 */
                        sprintf(to, "%d %s %d",
                                mday,
                                *(asc_months[month-1]),
                                year);
                    else
                        /* generate 21.3.1988 or 3.21.1988 */
                        sprintf(to, "%d.%d.%d",
                                (d_options_DF == 'A') ? month : mday,
                                (d_options_DF == 'A') ? mday : month,
                                year);

                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            /* display title in option page */
            case 'T':
                if(from[1] == text_at_char)
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    if(!str_isblank(d_options_DE))
                        {
                        char *title = d_options_DE;

                        while(*title  &&  (length <= fwidth))
                            {
                            if(riscos_fonts  &&  (*title < SPACE))
                                {
                                ++title;
                                continue;
                                }

                            *to++ = *title++;
                            length++;
                            }
                        }

                    *to = '\0';
                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            /* display current page number */
            case 'P':
                if(from[1] == text_at_char)
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    sprintf(to, "%d", curpnm);      /* must be made valid by caller */

                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            /* time string */
            case 'N':
                if(from[1] == text_at_char)
                    {
                    time_t today_time;
                    struct tm *split_timep;

                    ++from;

                    if(!expand_ats)
                        break;

                    today_time = time(NULL);
                    split_timep = localtime(&today_time);

                    sprintf(to,
                            "%.2d:%.2d:%.2d",
                            split_timep->tm_hour,
                            split_timep->tm_min,
                            split_timep->tm_sec);

                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            /* leafname */
            case 'L':
                if(from[1] == text_at_char)
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    strcat(to, current_document()->ss_instance_data.ss_doc.docu_name.leaf_name);

                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            /* font colour change */
            case 'C':
                {
                int r, g, b, n;

                /* check for colour revert */
                if((from[1] == text_at_char) || ((from[1] == ':') && (from[2] == text_at_char)))
                    {
                    ++from;
                    if(*from == ':')
                       ++from;

                    if(riscos_fonts) /* <t-a-c>C<t-a-c> -> back to black */
                        {
                        font_insert_colour_change(0, 0, 0, &to);
                        revert_colour_change = FALSE;
                        }

                    *to = '\0';
                    break;
                    }

                if((from[1] == ':') && (0 != (n = get_colour_value(from+2, &r, &g, &b))) && (from[2+n] == text_at_char))
                    {
                    from += 2 + n;

                    if(riscos_fonts)
                        {
                        font_insert_colour_change(r, g, b, &to);
                        revert_colour_change = TRUE;
                        }

                    *to = '\0';
                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                }
                break;

            /* font change */
            case 'F':
                {
                PC_U8Z name;
                char *startto;
                S32 len;
                F64 x, y;
                font fonthan;

                /* try to make sense of the font change */
                startto = to;
                if((len = readfxy('F', &from, &to, &name, &x, &y)) != 0)
                    {
                    to = startto;

                    /* read current font if none given */
                    if(isdigit(*name))
                        {
                        fep fp = font_find_block(current_font);

                        if(NULL != fp)
                            {
                            PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                            y = x; /* carry over 2nd parameter */
                            (void) sscanf(name, "%lg", &x);
                            if(-1 == y)
                                y = x;
                            name = fp_name;
                            len = strlen(name);
                            }
                        }

                    /* read current size if none given */
                    if(x == -1)
                        {
                        fep fp = font_find_block(current_font);

                        if(NULL != fp)
                            {
                            x = fp->xsize;
                            y = fp->ysize;
                            }
                        else
                            x = y = 12 * 16;
                        }
                    else
                        {
                        x *= 16;
                        y *= 16;
                        }

                    if(riscos_fonts &&
                       (fonthan = font_cache(name, len,
                                             (S32) x,
                                             font_squash((S32) y))) > 0)
                        {
                        /* add font change */
                        font_cancel_hilites(&to);
                        font_insert_change(fonthan, &to);
                        }
                    else
                        font_cancel_hilites(&to);

                    consume = 1;
                    *to = '\0';
                    break;
                    }

                /* check for font revert */
                trace_1(TRACE_APP_PD4, "bitstr text-at field F str: %s", report_l1str(from));
                if((from[1] == text_at_char) || ((from[1] == ':') && (from[2] == text_at_char)))
                    {
                    ++from;
                    if(*from == ':')
                        ++from;

                    font_cancel_hilites(&to);

                    if(riscos_fonts)
                        font_insert_change(slot_font, &to);

                    consume = 1;
                    *to = '\0';
                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;
                }

            /* draw file */
            case 'G':
                {
                PC_U8Z name;
                char *startto;
                S32 len, err;
                F64 x, y;

                /* look up draw file; replace reference with
                error if there is one, otherwise delete the reference */
                startto = to;
                if(row != currow &&
                   (len = readfxy('G', &from, &to, &name, &x, &y)) != 0)
                    {
                    char namebuf[BUF_MAX_PATHSTRING];
                    GR_CACHE_HANDLE cah;

                    safe_strnkpy(namebuf, elemof32(namebuf), name, len);

                    err = 0;

                    if(gr_cache_entry_query(&cah, namebuf))
                        err = gr_cache_error_query(&cah);
                    else
                        err = create_error(ERR_NOTFOUND);

                    to = startto;
                    if(err)
                        {
                        S32 len;
                        PC_U8 errstr;

                        errstr = reperr_getstr(err);
                        len = strlen(errstr);
                        strcpy(to, errstr);
                        to += len;
                        length += len;
                        }

                    *to = '\0';
                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;
                }

            default:
                if(!expand_ats)
                    break;

                if(*from == text_at_char) /*case text_at_char:*/
                    {
                    /* display text-at chars */
                    while(*from == text_at_char)
                        {
                        from++;
                        length++;
                        *to++ = text_at_char;
                        }

                    *to = '\0';
                    break;
                    }

                /* check for a mailing list parameter */
                if(isdigit(*from))
                    {
                    S32 key = (S32) atoi(from);
                    LIST *lptr = search_list(&first_macro, key);

                    if(lptr)
                        {
                        char *value = lptr->value;

                        while(*value  &&  (length <= fwidth))
                            {
                            *to++ = *value++;
                            length++;
                            }
                        }
                    else if(!sqobit)
                        {
                        /* on screen copy across blank parameters */
                        *to++ = text_at_char;
                        *to   = '\0';
                        length++;
                        break;
                        }

                    /* munge digits */
                    while(isdigit(*from))
                        ++from;

                    *to = '\0';
                    break;
                    }

                /* replace text-at char and continue if unrecognised */
                *to++ = text_at_char;
                *to   = '\0';
                length++;
                break;

            } /* end of switch */

        /* move "to" to end of string */
        while(*to && length <= fwidth)
            {
            ++to;
            ++length;
            }

        /* skip over text-at chars on input */
        while(*from == text_at_char)
            {
            ++from;
            if(!expand_ats && !consume)
                *to++ = text_at_char;
            }

        } /* end of while */

    if(revert_colour_change)
        {
        font_insert_colour_change(0, 0, 0, &to);
        revert_colour_change = FALSE;
        }

    /* add final delimiter */
    *to = NULLCH;
}

/******************************************************************************
*
* expands lcr field here
* finds three little strings to send off to bitstr
*
******************************************************************************/

extern void
expand_lcr(
    char *from,
    ROW row,
    /*out*/ char *array,
    coord fwidth,
    S32 expand_refs,
    BOOL expand_ats,
    BOOL expand_ctrl,
    BOOL allow_fonty_result,
    BOOL compile_lcr)
{
    char delimiter = *from;
    char *to = array;
    S32 i;
    char compiled[COMPILED_TEXT_BUFSIZ];
    char tbuf[PAINT_STRSIZ];
    char *tptr;
    S32 old_riscos_fonts;

    old_riscos_fonts = font_stack(riscos_fonts);

    if(!allow_fonty_result)
        riscos_fonts = FALSE;

    if(riscos_fonts)
        {
        current_font = 0;

        if((slot_font = font_get_global()) > 0)
            font_insert_change(slot_font, &to);
        }

    for(i = 0; i <= 2; i++)
        {
        if(is_text_at_char(delimiter)  ||  !ispunct(delimiter))
            {
            if(compile_lcr)
                {
                compile_text_slot(compiled, from, NULL);
                tptr = compiled;
                }
            else
                tptr = from;

            bitstr(tptr, row, to, fwidth,
                   expand_refs, expand_ats, expand_ctrl);

            /* move to after end of possibly fonty string */
            if(riscos_fonts)
                {
                while(NULLCH != *to)
                    to += font_skip(to);
                }
            else
                {
                to += strlen(to); /* plain non-fonty string */
                }
            ++to;
            break;
            }

        if(*from != delimiter)
            break;

        ++from;

        tptr = tbuf;

        while(*from  &&  (*from != delimiter))
            {
            if(SLRLD1 == (*tptr++ = *from++))
                { /* copy the rest of the CSR over */
                tiny_memcpy32(tptr, from, COMPILED_TEXT_SLR_SIZE-1);
                tptr += COMPILED_TEXT_SLR_SIZE-1;
                from += COMPILED_TEXT_SLR_SIZE-1;
                }
            }

        *tptr = NULLCH;

        /* revert to slot font before each section */
        if(riscos_fonts  &&  i)
            font_insert_change(slot_font, &to);

        if(compile_lcr)
            {
            compile_text_slot(compiled, tbuf, NULL);
            tptr = compiled;
            }
        else
            tptr = tbuf;

        bitstr(tptr, row, to, fwidth,
               expand_refs, expand_ats, expand_ctrl);

        trace_1(TRACE_APP_PD4, "expand_lcr: %s", report_l1str(to));

        if(riscos_fonts)
            {
            while(NULLCH != *to)
                to += font_skip(to);
            }
        else
            {
            to += strlen(to); /* plain non-fonty string */
            }
        ++to;
        }

    /* ensure three strings */
    memset32(to, 0, 3);

    /* restore old font state */
    riscos_fonts = font_unstack(old_riscos_fonts);
}

/******************************************************************************
*
* expand the contents of the slot into array
*
* returns the justification state
*
* expand_refs level says whether we are allowed to expand slot references inside
* the slot so text slots referring to themselves are not recursive
*
* note that strings returned by expand_slot() can now contain RISC OS font
* changes and thus embedded NULLCHs if riscos_fonts && allow_fonty_result.
* Use the function font_skip() to work out the size of each character in the resulting string
*
******************************************************************************/

extern char
expand_slot(
    _InVal_     DOCNO docno,
    P_SLOT tslot,
    ROW row,
    /*out*/ char *array,
    coord fwidth,
    S32 expand_refs,
    BOOL expand_ats,
    BOOL expand_ctrl,
    BOOL allow_fonty_result,
    BOOL cust_func_formula)
{
    char justify = tslot->justify & J_BITS;
    char *array_start;
    S32 old_riscos_fonts;
    P_EV_RESULT resp;
    DOCNO old_docno;

    /* switch to target document */
    old_docno = change_document_using_docno(docno);

    array_start = array;

    old_riscos_fonts = font_stack(riscos_fonts);

    if(!allow_fonty_result)
        riscos_fonts = 0;

    if(riscos_fonts)
        {
        current_font = 0;

        if((slot_font = font_get_global()) > 0)
            font_insert_change(slot_font, &array);
        }

    switch(result_extract(tslot, &resp))
        {
        case SL_TEXT:
            if(justify == J_LCR)
                {
                expand_lcr(tslot->content.text, row, array, fwidth,
                           expand_refs, expand_ats, expand_ctrl,
                           allow_fonty_result, FALSE /*compile_lcr*/);
                }
            else
                {
                bitstr(tslot->content.text, row, array, fwidth,
                       expand_refs, expand_ats, expand_ctrl);
                /* bitstr() will return a fonty result if riscos_fonts and allow_fonty_result on entry */
                }
            break;

        case SL_PAGE:
            sprintf(array, "~ %d", tslot->content.page.condval);
            justify = J_LEFT;
            break;

        case SL_NUMBER:
            if(cust_func_formula && ev_doc_is_custom_sheet(docno) &&
               ev_is_formula(&tslot->content.number.guts))
                {
                EV_OPTBLOCK optblock;

                ev_set_options(&optblock, docno);

                strcpy(array, "...");

                ev_decode_slot(docno, array + 3, &tslot->content.number.guts, &optblock);

                /* SKS after 4.11 29jan92 - LCR custom function display gave drug-crazed redraw */
                if(justify == J_LCR)
                    justify = J_LEFT;
                }
            else
                {
                result_to_string(resp, docno, array_start, array,
                                 tslot->format,
                                 fwidth, &justify,
                                 expand_refs, expand_ats, expand_ctrl);
                }
            break;

        default:
            trace_0(TRACE_APP_PD4, "expand_slot found bad type");
            break;
        }

    /* restore state of font flag */
    riscos_fonts = font_unstack(old_riscos_fonts);

    /* get back old document */
    select_document_using_docno(old_docno);

    return(justify);
}

/******************************************************************************
*
* search font list for font/size combination;
* if not there, add font/size combination to list
*
******************************************************************************/

static font
font_cache(
    _In_reads_bytes_(namlen) const char *name,
    S32 namlen,
    S32 xs,
    S32 ys)
{
    char namebuf[BUF_MAX_PATHSTRING];
    U32 namelen_p1;
    LIST *lptr;
    fep fp;
    P_U8Z fp_name;
    font fonthan;
    S32 res;

    safe_strnkpy(namebuf, elemof32(namebuf), name, namlen);

    trace_3(TRACE_APP_PD4, "font_cache %s, x:%d, y:%d ************", report_l1str(namebuf), xs, ys);

    /* search list for entry */
    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
        {
        fp = (fep) lptr->value;

        fp_name = (P_U8Z) (fp + 1);

        if( (fp->xsize == xs)  &&
            (fp->ysize == ys)  &&
            (0 == _stricmp(namebuf, fp_name))
          ) {
            if(fp->handle > 0)
                {
                trace_4(TRACE_APP_PD4, "font_cache found cached font: (handle:%d), size: %d,%d %s",
                        fp->handle, xs, ys, report_l1str(namebuf));
                }
            else
                {
                /* matched entry in cache but need to reestablish font handle */
                if(font_find(namebuf, xs, ys, 0, 0, &fonthan))
                    fonthan = 0;
                fp->handle = fonthan;
                if(fp->handle > 0)
                    trace_4(TRACE_APP_PD4, "font_cache reestablish cached font OK: (handle:%d) size: %d,%d %s",
                            fonthan, xs, ys, report_l1str(namebuf));
                else
                    trace_4(TRACE_APP_PD4, "font_cache reestablish cached font FAIL: (handle:%d) size: %d,%d %s",
                            fp->handle, xs, ys, report_l1str(namebuf));
                }
            return(fp->handle);
            }
        }

    /* create entry for font */

    /* ask font manager for handle */
    if(font_find(namebuf, xs, ys, 0, 0, &fonthan))
        fonthan = 0;

    namelen_p1 = strlen32p1(namebuf);

    if(NULL == (lptr = add_list_entry(&font_cache_list, sizeof(struct font_cache_entry) + namelen_p1, &res)))
        {
        trace_4(TRACE_APP_PD4, "font_cache failed to add entry for uncached font: (handle:%d) size: %d,%d  %s",
                fonthan, xs, ys, report_l1str(namebuf));
        reperr_null(res);
        if(fonthan > 0)
            font_LoseFont(fonthan);
        return(0);
        }

    /* change key so we don't find it */
    lptr->key = 0;

    fp = (fep) lptr->value;
    fp->handle = fonthan;
    fp->xsize = xs;
    fp->ysize = ys;

    fp_name = (P_U8Z) (fp + 1);
    memcpy(fp_name, namebuf, namelen_p1);

    trace_4(TRACE_APP_PD4, "font_cache found uncached font: (handle:%d) size: %d,%d %s",
            fonthan, xs, ys, report_l1str(namebuf));
    return(fp->handle);
}

/******************************************************************************
*
* calculate shift for super/subs based on
* old font height and whether we are printing
*
******************************************************************************/

static S32
font_calshift(
    S32 hilite_n,
    S32 height)
{
    /* convert to 72000's */
    height = (height * 1000) / 16;

    switch(hilite_n)
        {
        /* subs */
        case 4:
            return(sqobit ? -(height / 2) : 0);

        /* super */
        case 5:
            return(sqobit ? height / 2 : height / 4);

        /* this should never happen */
        default:
            return(0);
            break;
        }
}

/******************************************************************************
*
* cancel all hilites activated at the moment
*
******************************************************************************/

static void
font_cancel_hilites(
    char **pto)
{
    S32 i;
    char *to;

    trace_0(TRACE_APP_PD4, "font_cancel_hilites");

    to = *pto;

    if(!riscos_fonts)
        {
        for(i = 0; i < NUM_HILITES; ++i)
            if(hilite_state[i])
                {
                *to++ = FIRST_HIGHLIGHT + i;
                hilite_state[i] = 0;
                }
        }
    else
        {
        if(hilite_state[0])
            {
            *to++ = UL_CHANGE;
            *to++ = 0;
            *to++ = 0;
            }

        if(hilite_state[4])
            {
            font_super_sub(4, &to);
            hilite_state[5] = 0;
            }

        if(hilite_state[5])
            font_super_sub(5, &to);

        for(i = 0; i < NUM_HILITES; ++i)
            hilite_state[i] = 0;
        }

    *pto = to;
}

/******************************************************************************
*
* return the width of a character in a given font
*
******************************************************************************/

extern S32
font_charwid(
    font ch_font,
    S32 ch)
{
    char tbuf[2];
    tbuf[0] = ch;
    tbuf[1] = '\0';
    return(font_stringwid(ch_font, tbuf));
}

/******************************************************************************
*
* release all font handles
*
******************************************************************************/

extern void
font_close_all(
    _InVal_     BOOL shutdown)
{
    LIST * lptr;

    if(shutdown)
        lptr = NULL; /* just for breakpoint */

    lptr = first_in_list(&font_cache_list);

    while(NULL != lptr)
    {
        fep fp = (fep) lptr->value;

        if(fp->handle > 0)
        {
            font_LoseFont(fp->handle);
            fp->handle = 0;
        }

        if(shutdown)
        {
            lptr->key = 1; /* key to be deleted with */
            delete_from_list(&font_cache_list, 1);

            lptr = first_in_list(&font_cache_list); /* restart */
        }
        else
        {   /* leave cache entry valid to be reestablished when next needed */
            lptr = next_in_list(&font_cache_list);
        }
    }
}

/******************************************************************************
*
* complain when a fonty error is encountered
*
******************************************************************************/

extern _kernel_oserror *
font_complain(
    _kernel_oserror * err)
{
    if(err)
        {
        if(riscos_printing)
            print_complain(err);
        else
            {
            reperr(create_error(ERR_FONTY), err->errmess);
            font_close_all(TRUE); /* shut down font system releasing handles */
            riscos_fonts = FALSE;
            riscos_font_error = TRUE;
            xf_draweverything = TRUE;   /* screen is in a mess */
            xf_interrupted = TRUE;      /* we have work to do! */
            }
        }

    return(err);
}

/******************************************************************************
*
* given a font handle and a string, derive a new font name
* and try to cache that variant at the current size
*
******************************************************************************/

static font
font_derive(
    font handle,
    _In_z_      const char * name,
    _In_z_      const char * supplement,
    _InVal_     BOOL add)
{
    fep fp;
    char namebuf[BUF_MAX_PATHSTRING];

    trace_4(TRACE_APP_PD4, "font_derive: handle: %d, name: %s %s supplement: %s",
            handle, name, add ? "add" : "remove", report_l1str(supplement));

    if(NULL == (fp = font_find_block(handle)))
        return(0);

    /* should we add or remove the supplement ? */
    if(add)
    {
        safe_strkpy(namebuf, elemof32(namebuf), name);
        safe_strkat(namebuf, elemof32(namebuf), supplement);
    }
    else
    {
        PC_U8Z fp_name = (PC_U8Z) (fp + 1);

        if(!status_done(font_strip(namebuf, elemof32(namebuf), fp_name, supplement)))
            { /* unable to strip */
            trace_0(TRACE_APP_PD4, "font_derive_font unable to strip");
            return(0);
            }
    }

    trace_1(TRACE_APP_PD4, "font_derive_font about to cache: %s", report_l1str(namebuf));
    return(font_cache(namebuf, strlen(namebuf), fp->xsize, fp->ysize));
}

/******************************************************************************
*
* expand a string including font information ready
* for line break calculation - text-at fields are converted
* to the relevant number of text-at chars, font information is
* inserted; expects riscos_fonts to be TRUE
* and knows ctrl chars are displayed expanded
*
******************************************************************************/

extern void
font_expand_for_break(
    char *to,
    char *from)
{
    current_font = 0;

    if((slot_font = font_get_global()) > 0)
        font_insert_change(slot_font, &to);

    bitstr(from, -1, to, 0,
           0 /*expand_refs*/, FALSE /*expand_ats*/, TRUE /*expand_ctrl*/);
}

/******************************************************************************
*
* return pointer to a font block given the font handle
*
******************************************************************************/

static fep
font_find_block(
    _InVal_     font fonthan)
{
    LIST *lptr;

    trace_1(TRACE_APP_PD4, "font_find_block: %d", fonthan);

    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
        {
        fep fp;

        fp = (fep) lptr->value;

        if(fp->handle == fonthan)
            {
            trace_1(TRACE_APP_PD4, "font_find_block found: %d", fonthan);
            return(fp);
            }
        }

    trace_1(TRACE_APP_PD4, "font_find_block %d failed", fonthan);
    return(NULL);
}

/******************************************************************************
*
* load the global font
*
******************************************************************************/

static font
font_get_global(void)
{
    font glob_font;

    trace_1(TRACE_APP_PD4, "font_get_global: %s", report_l1str(global_font));

    if((glob_font = font_cache(global_font,
                               strlen(global_font),
                               global_font_x,
                               font_squash(global_font_y))) > 0)
        return(glob_font);

    riscos_fonts = FALSE;
    riscos_font_error = TRUE;
    return(glob_font);
}

/******************************************************************************
*
* insert a font change given a font handle;
* keep track of the current font
*
******************************************************************************/

static void
font_insert_change(
    font handle,
    char **pto)
{
    trace_1(TRACE_APP_PD4, "font_insert_change: handle = %d", handle);

    if(handle)
        {
        *(*pto)++ = FONT_CHANGE;
        *(*pto)++ = (char) handle;
        current_font = handle;
        }
}

/******************************************************************************
*
* insert a font colour change
*
******************************************************************************/

static void
font_insert_colour_change(
    S32 r,
    S32 g,
    S32 b,
    char **pto)
{
    S32 curbg = wimpt_RGB_for_wimpcolour(current_bg);

    if(wimpt_os_version_query() >= RISC_OS_3_5)
        {
        *(*pto)++ = COL_RGB_CHANGE;
        *(*pto)++ = (char) (curbg >>  8);
        *(*pto)++ = (char) (curbg >> 16);
        *(*pto)++ = (char) (curbg >> 24);
        *(*pto)++ = (char) r;
        *(*pto)++ = (char) g;
        *(*pto)++ = (char) b;
        *(*pto)++ = (char) 14;

        return;
        }
}

/******************************************************************************
*
* insert a font colour inversion given a font handle
*
******************************************************************************/

static void
font_insert_colour_inversion(
    font_state *fp,
    char **pto)
{
    S32 curbg, curfg, curoffset;
    S32 newbg, newfg, newoffset;

    trace_0(TRACE_APP_PD4, "font_insert_colour_inversion");

    curbg = fp->back_colour;
    curfg = fp->fore_colour;
    curoffset = fp->offset;

    trace_3(TRACE_APP_PD4, "inversion: bg %d, fg %d, offset %d", curbg, curfg, curoffset);

    if(wimpt_os_version_query() >= RISC_OS_3_5)
        {
        newbg = curfg;
        newfg = curbg;

        fp->back_colour = newbg;
        fp->fore_colour = newfg;

        *(*pto)++ = COL_RGB_CHANGE;
        *(*pto)++ = (char) (newbg >>  8);
        *(*pto)++ = (char) (newbg >> 16);
        *(*pto)++ = (char) (newbg >> 24);
        *(*pto)++ = (char) (newfg >>  8);
        *(*pto)++ = (char) (newfg >> 16);
        *(*pto)++ = (char) (newfg >> 24);
        *(*pto)++ = (char) curoffset;

        return;
        }

    if(log2bpp >= 3)
        {
        /* bg ignored by font manager in 256 colour modes, but preserve to reinvert later on */
        newbg = curfg;
        newfg = curbg;
        newoffset = curoffset;
        }
    else
        {
        newbg = curfg + ((signed char) curoffset);
        newfg = curbg + ((signed char) curoffset);
        newoffset = -((signed char) curoffset);
        }

    trace_3(TRACE_APP_PD4, "inversion: bg' %d, fg' %d, offset' %d", newbg, newfg, newoffset);

    fp->back_colour = newbg;
    fp->fore_colour = newfg;
    fp->offset = newoffset;

    *(*pto)++ = COL_INVERT;
    *(*pto)++ = (char) newbg;
    *(*pto)++ = (char) newfg;
    *(*pto)++ = (char) newoffset;
}

/******************************************************************************
*
* insert a y shift into the font string
*
******************************************************************************/

static void
font_insert_shift(
    S32 type,
    S32 shift,
    char **pto)
{
    *(*pto)++ = (char) type;
    *(*pto)++ = (char) (shift & 0xFF);
    *(*pto)++ = (char) ((shift >> 8) & 0xFF);
    *(*pto)++ = (char) ((shift >> 16) & 0xFF);
    plusab(current_shift, shift);
}

/******************************************************************************
*
* skip over font changes and other mess in a string
*
******************************************************************************/

extern S32
font_skip(
    char *at)
{
    if(riscos_fonts)
        {
        switch(*at)
            {
            case FONT_CHANGE:
            /*case COL_CHANGE:*/
                return(2);

            case UL_CHANGE:
                return(3);

            case COL_INVERT:
            case X_SHIFT:
            case Y_SHIFT:
                return(4);

            case COL_RGB_CHANGE:
                return(8);
            }
        }

    return(1);
}

/******************************************************************************
*
* given a font y size, squash it down
* to maximum line height on the screen
*
******************************************************************************/

static S32
font_squash(
    S32 ysize)
{
    S32 limit;

    if(sqobit)
        return(ysize);

    limit = (fontscreenheightlimit_mp * 16) / 1000;
    trace_3(TRACE_APP_PD4, "font_squash(%d): limit %d, limit_mp %d", ysize, limit, fontscreenheightlimit_mp);

    return(MIN(ysize, limit));
}

/******************************************************************************
*
* return a value for riscos_fonts to save on the stack
*
******************************************************************************/

extern S32
font_stack(
    S32 to_stack)
{
    return(to_stack);
}

/******************************************************************************
*
* return the width of a string in a given font
*
******************************************************************************/

static S32
font_stringwid(
    font str_font,
    char *str)
{
    font_string fs;
    S32 swidth_mp = 0;

    fs.s     = (char *) str;
    fs.x     = INT_MAX;
    fs.y     = INT_MAX;
    fs.split = -1;
    fs.term  = INT_MAX;

    if((NULL == font_SetFont(str_font)) &&
       (NULL == font_strwidth(&fs)) )
        swidth_mp = fs.x;

    return(swidth_mp);
}

/******************************************************************************
*
* strip a section from a font name, possibly in-place
*
******************************************************************************/

static STATUS
font_strip(
    _Out_writes_z_(elemof_newname) char * newname,
    _InVal_     U32 elemof_newname,
    _In_z_      const char * fontname,
    _In_z_      const char * strip)
{
    /* look for supplement in name */
    const char * place = stristr(fontname, strip);

    if(NULL == place)
        {
        if(newname != fontname)
            safe_strkpy(newname, elemof_newname, fontname);
        return(STATUS_OK); /* supplement not stripped */
        }

    if(newname != fontname)
        safe_strnkpy(newname, elemof_newname, fontname, place - fontname);
    else
        newname[place - fontname] = NULLCH;

    safe_strkat(newname, elemof_newname, place + strlen(strip));
    return(STATUS_DONE); /* supplement stripped */
}

/******************************************************************************
*
* copy a string into a buffer ignoring
* unwanted non-fonty characters
*
******************************************************************************/

static S32
font_strip_strcpy(
    char * to,
    const char * from)
{
    char * oldto = to;

    while(*from)
        {
        if(*from < SPACE)
            {
            ++from;
            continue;
            }

        *to++ = *from++;
        }

    *to++ = NULLCH;

    return(to - oldto);
}

/******************************************************************************
*
* handle super/subscripts
* switching off a super/subscript reverts to the font active
* when the first super/subs was switched on - this includes
* bold and italic - which are cancelled by a super/subs off
*
******************************************************************************/

static void
font_super_sub(
    S32 hilite_n,
    char **pto)
{
    /* off highlight of a pair - revert to old state */
    if(hilite_state[hilite_n])
        {
        font_insert_shift(Y_SHIFT, -current_shift, pto);
        font_insert_change(start_font, pto);
        start_font = 0;
        old_height = 0;

        /* reset both highlight states */
        hilite_state[4] = hilite_state[5] = 0;
        }
    else
        {
        /* already in a super/subs ? */
        if(start_font)
            {
            font_insert_shift(Y_SHIFT,
                              font_calshift(hilite_n, old_height)
                                                - current_shift,
                              pto);

            hilite_state[hilite_n] ^= 1;
            }
        else
            { /* not in either super or subs */
            fep fp = font_find_block(current_font);

            if(NULL != fp)
                {
                PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                font subfont;

                if((subfont = font_cache(fp_name,
                                         strlen(fp_name),
                                         (fp->xsize * 3) / 4,
                                         (fp->ysize * 3) / 4)) > 0)
                    {
                    old_height = fp->ysize;
                    start_font = fp->handle;

                    font_insert_change(subfont, pto);
                    font_insert_shift(Y_SHIFT, font_calshift(hilite_n,
                                                             old_height), pto);
                    }
                }

            hilite_state[hilite_n] ^= 1;
            }
        }
}

/******************************************************************************
*
* return the unstacked riscos_fonts value, accounting
* for a font error in the meantime
*
******************************************************************************/

extern S32
font_unstack(
    S32 stack_state)
{
    if(riscos_font_error)
        return(FALSE);

    return(stack_state);
}

extern char
get_dec_field_from_opt(void)
{
    char decimals = d_options_DP;

    return((decimals == 'F') ? (char) 0xF : (char) (decimals-'0'));
}

/******************************************************************************
*
* check to see if character is a font change or a highlight
*
******************************************************************************/

extern S32
is_font_change(
    char *ch)
{
    if(riscos_fonts)
        {
        switch(*ch)
            {
            case FONT_CHANGE:
            /*case COL_CHANGE:*/
            case COL_INVERT:
            case COL_RGB_CHANGE:
            case UL_CHANGE:
            case X_SHIFT:
            case Y_SHIFT:
                return(1);
            }

        return(0);
        }

    return(ishighlight(*ch));
}

/******************************************************************************
*
* extract pointer to result structure from slot
* provides a level of indirection from slot
* structure details
*
******************************************************************************/

extern S32
result_extract(
    P_SLOT sl,
    P_EV_RESULT *respp)
{
    if(sl->type == SL_NUMBER)
        *respp = &sl->content.number.guts.result;

    return(sl->type);
}

/******************************************************************************
*
* return sign of result
* -1 <0
*  0 =0 or not number
*  1 >0
*
******************************************************************************/

extern S32
result_sign(
    P_SLOT sl)
{
    P_EV_RESULT resp;
    S32 res;

    res = 0;

    if(!ev_doc_is_custom_sheet(current_docno()) &&
       result_extract(sl, &resp) == SL_NUMBER)
        {
        switch(resp->did_num)
            {
            case RPN_DAT_REAL:
                if(resp->arg.fp < 0)
                    res = -1;
                else if(resp->arg.fp > 0)
                    res = 1;
                break;
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
                if(resp->arg.integer < 0)
                    res = -1;
                else if(resp->arg.integer > 0)
                    res = 1;
                break;
            }
        }

    return(res);
}

/******************************************************************************
*
* convert data in result structure to string ready for output
*
******************************************************************************/

static void
result_to_string(
    P_EV_RESULT resp,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    BOOL expand_ats,
    BOOL expand_ctrl)
{
    switch(resp->did_num)
        {
        F64 number;

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            number = (F64) resp->arg.integer;
            goto num_print;

        case RPN_DAT_REAL:
            number = resp->arg.fp;

        num_print:
            /* try to print number fully expanded - if not possible
             * try to print number in exponential notation - if not possible
             * print %
            */
            if(riscos_fonts)
                fwidth = ch_to_mp(fwidth);

            if(sprintnumber(array_start, array,
                            format, number, FALSE, fwidth) >= fwidth)
                if(sprintnumber(array_start, array,
                                format, number, TRUE, fwidth) >= fwidth)
                    strcpy(array, "% ");

            if(!expand_refs)
                {
                char *last = array + strlen(array);

                if(*last == SPACE) /* should be OK even with riscos_fonts */
                    *last = NULLCH;
                }

            /* free alignment should display numbers right */
            if(*justify == J_FREE)
                *justify = J_RIGHT;
            else if(*justify == J_LCR)
                *justify = J_LEFT;
            break;

        case RPN_RES_STRING:
            bitstr(resp->arg.string.data, 0, array, fwidth,
                   expand_refs, expand_ats, expand_ctrl);
            /* bitstr() will return a fonty result if riscos_fonts */
            break;

        case RPN_DAT_BLANK:
            array[0] = '\0';
            break;

        case RPN_RES_ARRAY:
            {
            EV_RESULT temp_res;
            EV_DATA temp_data;

            /* go via EV_DATA */
            ev_result_to_data_convert(&temp_data, resp);

            ev_data_to_result_convert(&temp_res, ss_array_element_index_borrow(&temp_data, 0, 0));

            result_to_string(&temp_res, docno, array_start, array, format, fwidth, justify,
                             expand_refs, expand_ats, expand_ctrl);

            ev_result_free_resources(&temp_res);
            break;
            }

        case RPN_DAT_DATE:
            {
            S32 len;
            EV_DATE temp;

            len = 0;
            temp = resp->arg.date;

            if(temp.date)
                {
                S32 year, month, day;

                if(temp.date < 0)
                    {
                    array[len++]= '-';
                    temp.date = -temp.date;
                    }

                ss_dateval_to_ymd(&temp.date, &year, &month, &day);

                if(d_options_DF == 'T')
                    /* generate 21 Mar 1988 */
                    len += sprintf(array + len, "%d %.3s %.4d",
                                   day + 1,
                                   *(asc_months[month]),
                                   year + 1);
                else
                    /* generate 21.3.1988 or 3.21.1988 */
                    len += sprintf(array + len, "%d.%d.%.4d",
                                   ((d_options_DF == 'A')) ? (month + 1) : (day + 1),
                                   ((d_options_DF == 'A')) ? (day + 1) : (month + 1),
                                   year + 1);
                }

            if(!temp.date || temp.time)
                {
                S32 hour, minute, second;

                /* separate time from date */
                if(temp.date)
                    array[len++] = ' ';

                ss_timeval_to_hms(&temp.time, &hour, &minute, &second);

                len += sprintf(array + len, "%.2d:%.2d:%.2d", hour, minute, second);
                }

            if(*justify == J_LCR)
                *justify = J_LEFT;
            break;
            }

        default:
            assert(0);
        case RPN_DAT_ERROR:
            {
            S32 len = 0;

            if(resp->arg.error.type == ERROR_PROPAGATED)
            {
                U8 buffer_slr[BUF_EV_MAX_SLR_LEN];
                EV_SLR ev_slr; /* unpack */
                ev_slr.col = resp->arg.error.col;
                ev_slr.row = resp->arg.error.row;
                ev_slr.docno = resp->arg.error.docno;
                ev_slr.flags = 0;
                (void) ev_dec_slr(buffer_slr, docno, &ev_slr, TRUE);
                len += sprintf(array + len,
                               PropagatedErrStr,
                               buffer_slr);
            }
            else
            if(resp->arg.error.type == ERROR_CUSTOM)
            {
                len += sprintf(array + len,
                               CustFuncErrStr,
                               resp->arg.error.row + 1);
            }

            strcpy(array + len, reperr_getstr(resp->arg.error.num));
            *justify = J_LEFT;
            break;
            }
        }
}

/******************************************************************************
*
* sprintnumber does a print of the number in tslot into array
* it prints lead chars, '-' | '(', number, ')', trail chars
* if eformat is FALSE it prints number in long form
* if eformat is TRUE  it prints number in e format
* it returns the number of non-space characters printed
* the format is picked up from the slot or from the option page
*
******************************************************************************/

static char float_e_format[]        = "%g";
static char decimalsformat[]        = "%.0f";
static char decimals_e_format[] = "%.0e";
#define PLACES_OFFSET 2

static S32
sprintnumber(
    char *array_start,
    char *array,
    char format,
    F64 value,
    BOOL eformat,
    S32 fwidth)
{
    char floatformat[8];    /* written to to create "%0.nng" format below */
    char *formatstr, *nextprint;
    char decimals, ch;
    BOOL negative, brackets;
    char *tptr;
    S32 len,
    width = 0, inc;
    char t_lead[10], t_trail[10];
    S32 padding, logval;
    S32 digitwid_mp = 0, dotwid_mp = 0, thswid_mp = 0, bracwid_mp = 0;

    negative = brackets = FALSE;
    nextprint = array;

    if(riscos_fonts)
        {
        digitwid_mp = font_charwid(current_font, '0');
        dotwid_mp   = font_charwid(current_font, '.');

        if(d_options_TH != TH_BLANK)
            switch(d_options_TH)
                {
                case TH_DOT:
                    thswid_mp = dotwid_mp;
                    dotwid_mp = font_charwid(current_font, ',');
                    break;
                case TH_COMMA:
                    thswid_mp = font_charwid(current_font, ',');
                    break;
                default:
                    thswid_mp = font_charwid(current_font, ' ');
                    break;
                }
        }

    if(value < 0.)
        {
        value = 0. - value;
        negative = TRUE;
        }

    brackets = (format & F_DCP)
                    ? (format & F_BRAC)
                    : (d_options_MB == 'B');

    if(brackets  &&  riscos_fonts)
        bracwid_mp = font_charwid(current_font, ')');

    decimals = (format & F_DCP)
                    ? (format & F_DCPSID)
                    : get_dec_field_from_opt();

    /* process leading and trailing characters */
    if(format & F_LDS)
        {
        *t_lead = NULLCH;
        if(d_options_LP)
            {
            if(riscos_fonts)
                font_strip_strcpy(t_lead, d_options_LP);
            else
                strcpy(t_lead, d_options_LP);
            }
        }

    if(format & F_TRS)
        {
        *t_trail = NULLCH;
        if(d_options_TP)
            {
            if(riscos_fonts)
                font_strip_strcpy(t_trail, d_options_TP);
            else
                strcpy(t_trail, d_options_TP);
            }
        }

    if(decimals == 0xF)
        {
        if(eformat)
            formatstr = float_e_format;
        else
            {
            if(riscos_fonts)
                {
                padding  = brackets ? bracwid_mp : 0;
                padding += negative ?
                                (brackets ? bracwid_mp
                                          : font_charwid(current_font, '-'))
                                    : 0;
                }
            else
                {
                padding =  brackets ? 1 : 0;
                padding += negative ? 1 : 0;
                }

            if(format & F_LDS)
                {
                if(riscos_fonts)
                    padding += font_stringwid(current_font, t_lead);
                else
                    padding += strlen(t_lead);
                }

            if(format & F_TRS)
                {
                if(riscos_fonts)
                    padding += font_stringwid(current_font, t_trail);
                else
                    padding += strlen(t_trail);
                }

            formatstr = floatformat;

            logval = (S32) ((value == 0.) ? 0. : log10(value));

            if(d_options_TH != TH_BLANK)
                {
                if(riscos_fonts)
                    padding += (logval / 3) * thswid_mp;
                else
                    padding += logval / 3;
                }

            width = fwidth - padding;
            if(!riscos_fonts)
                width -= 1;

            if(value != floor(value))
                {
                /* if displaying fractional part, allow  for . */
                if(riscos_fonts)
                    {
                    if((width / digitwid_mp) > logval)
                        width -= dotwid_mp;

                    /* if too small */
                    if(logval < 0 && -logval > (width / digitwid_mp))
                        return(INT_MAX);
                    }
                else
                    {
                    if(width > logval)
                        width--;

                    /* if too small */
                    if(logval < 0 && -logval > width)
                        return(INT_MAX);
                    }
                }

            /* account for leading zero */
            if(value < 1.)
                {
                if(riscos_fonts)
                    width -= digitwid_mp;
                else
                    width -= 1;
                }

            /* calculate space for digits */
            if(riscos_fonts)
                {
                trace_2(TRACE_APP_PD4, "sprintnumber width_mp: %d, width_digit: %d",
                        width, (S32) width / digitwid_mp);
                width /= digitwid_mp;
                }

            (void) sprintf(formatstr, "%%.%dg", width);
            }
        }
    else
        {
        formatstr = eformat ? decimals_e_format
                            : decimalsformat;

        formatstr[PLACES_OFFSET] = (char) (decimals + '0');
        }

    /* print leadin characters */
    if(format & F_LDS)
        {
        strcpy(nextprint, t_lead);
        nextprint += strlen(t_lead);
        }

    *nextprint = '\0';

    /* print number */
    tptr = nextprint;
    if(negative)
        *nextprint++ = brackets ? '(' : '-';

    len = sprintf(nextprint, formatstr, value);

    trace_2(TRACE_APP_PD4, "sprintnumber formatstr: %s, result: %s",
            report_l1str(formatstr), report_l1str(nextprint));

    /* work out thousands separator */
    if(d_options_TH != TH_BLANK && !eformat)
        {
        char *dotp, separator;
        S32 beforedot;

        for(beforedot = 0, dotp = nextprint; *dotp != DOT && *dotp; ++dotp)
            ++beforedot;

        switch(d_options_TH)
            {
            case TH_COMMA:
                separator = COMMA;
                break;

            case TH_DOT:
                separator = DOT;
                if(*dotp)
                    *dotp = COMMA;
                break;

            default:
                separator = SPACE;
                break;
            }

        while(beforedot > 3)
            {
            inc = ((beforedot - 1) % 3) + 1;
            nextprint += inc;
            len -= inc;

            memmove32(nextprint + 1, nextprint, len + 1);
            *nextprint++ = separator;

            beforedot -= inc;
            }
        }

    while(*nextprint)
        ++nextprint;

    if(negative && brackets)
        strcpy(nextprint++, ")");

    /* get rid of superfluous E padding */
    /*if(eformat)*/
        {
        while((ch = *tptr++) != '\0')
            if(ch == 'e')
                {
                if(*tptr == '+')
                    {
                    memmove32(tptr, tptr + 1, strlen32(tptr));
                    nextprint--;
                    }
                else if(*tptr == '-')
                    tptr++;

                while((*tptr == '0')  &&  (*(tptr+1) != '\0'))
                    {
                    memmove32(tptr, tptr + 1, strlen32(tptr));
                    nextprint--;
                    }

                break;
                }
        }

    /* print trailing characters */
    if(format & F_TRS)
        {
        strcpy(nextprint, t_trail);
        nextprint += strlen(t_trail);
        }

    *nextprint = '\0';

    /* do font width before we add funny space */
    if(riscos_fonts)
        {
        S32 expect_wid = 0;
        S32 xshift = 0;
        char *dot, *comma, *point;
        S32 widpoint;

        trace_1(TRACE_APP_PD4, "sprintnumber string width is: %d",
                font_width(array_start));

        /* work out x shift to add to align numbers at the point */
        if(decimals != 0xF)
            {
            dot   = strrchr(array, '.');
            comma = strrchr(array, ',');

            if(dot || comma)
                {
                point = MAX(dot, comma);
                widpoint = font_stringwid(current_font, point + 1);
                expect_wid = decimals * digitwid_mp;
                if(format & F_TRS)
                    expect_wid += font_stringwid(current_font, t_trail);
                if(brackets)
                    expect_wid += bracwid_mp;
                if(widpoint <= expect_wid)
                    xshift = expect_wid - widpoint;
                }
            }

        /* increase shift for positive (bracketed) numbers */
        if(!xshift && brackets && !negative)
            xshift += bracwid_mp;

        if(xshift)
            {
            trace_2(TRACE_APP_PD4, "sprintnumber shift: %d, expected: %d",
                    xshift, expect_wid);

            font_insert_shift(X_SHIFT, xshift, &nextprint);
            }

        *nextprint = '\0';
        width = font_width(array_start);
        }

    /* add spaces on end, but don't count them in length */
    if(!riscos_fonts)
        if(brackets && !negative)
            {
            *nextprint++ = FUNNYSPACE;
            *nextprint = '\0';
            }

    if(riscos_fonts)
        {
        trace_2(TRACE_APP_PD4, "sprintnumber returns width: %d, fwidth is: %d",
                width, fwidth);
        return(width);
        }
    else
        {
        trace_2(TRACE_APP_PD4, "sprintnumber returns width: %d, fwidth is: %d",
                nextprint - array, fwidth);
        return(nextprint - array);
        }
}

extern void
expand_current_slot_in_fonts(
    /*out*/ char *to,
    BOOL partial,
    /*out*/ P_S32 this_fontp)
{
    S32 offset, split;
    PC_U8 from;
    char ch;
    font_state f;
    PC_U8 p1, p2;
    font fh;
    wimp_paletteword bg, fg;
    int f_offset;

    current_font = 0;

    if((slot_font = font_get_global()) > 0)
        font_insert_change(slot_font, &to);

    if(this_fontp)
        *this_fontp = slot_font;

    offset = strlen(linbuf);
    from   = linbuf + MIN(lescrl, offset);
    split  = lecpos - lescrl;

    p1 = (word_to_invert ? (PC_U8) linbuf + lecpos : NULL);
    p2 = (word_to_invert ? p1 + strlen(word_to_invert) : NULL);

    if(p1)
        {
        trace_3(TRACE_APP_PD4, "word to invert is &%p &%p %s", p1, p2, word_to_invert);

        /* find out what colours we must use */

        if(wimpt_os_version_query() >= RISC_OS_3_5)
            {
            f.fore_colour = wimpt_RGB_for_wimpcolour(current_fg);
            f.back_colour = wimpt_RGB_for_wimpcolour(current_bg);
            f.offset = 14;
            }
        else if(log2bpp >= 3)
            {
            fh = slot_font;
            bg.word = wimpt_RGB_for_wimpcolour(current_bg);
            fg.word = wimpt_RGB_for_wimpcolour(current_fg);
            f_offset = 14;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) has RGB bg %8.8X, fg %8.8X", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &f_offset));
            f.fore_colour = fg.word;
            f.offset = f_offset;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) gets fg %d, offset %d", fg.word, f_offset);

            fh = slot_font;
            bg.word = wimpt_RGB_for_wimpcolour(current_fg);
            fg.word = wimpt_RGB_for_wimpcolour(current_bg);
            f_offset = 14;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) has RGB bg %8.8X, fg %8.8X", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &f_offset));
            f.back_colour = fg.word;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) gets bg %d, offset %d", fg.word, f_offset);
            }
        else
            {
            font_complain(font_current(&f));
            trace_3(TRACE_APP_PD4, "expand_current_slot gets bg %d, fg %d, offset %d", f.back_colour, f.fore_colour, f.offset);
            }
        }

    while((split > 0)  ||  !partial)
        {
        if(p1  &&  ((p1 == from)  ||  (p2 == from)))
            font_insert_colour_inversion(&f, &to);

        ch = *from;

        if(!ch)
            {
            if(split > 0)
                {
                /* must pad with spaces - kill inversion check too */
                ch = SPACE;
                p1 = NULL;
                }
            else
                break;
            }
        else
            from++;

        /* check for highlights, 'ISO' control characters & Acorn redefs */
        if(ishighlight(ch))
            {
            *to++ = '[';
            *to++ = (ch - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;
            *to++ = ']';
            }

        else if((ch < SPACE)  ||
             ((ch >= 127)  &&  (ch < 160)  &&  !font_charwid(current_font, ch)))
            {
            to += sprintf(to, "[%.2x]", ch);
            }

        else if(ch)
            *to++ = ch;

        --split;
        }

    *to = '\0';
}

extern BOOL
ensure_global_font_valid(void)
{
    return(font_get_global() > 0);
}

/* end of slotconv.c */
