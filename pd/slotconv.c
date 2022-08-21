/* slotconv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Cell to text conversion module */

/* RJM 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#include "riscos_x.h"
#include "riscdraw.h"
#include "swis.h"

/*
need to know about ev_dec_slr()
*/

#include "cmodules/ev_evali.h"

#include <time.h>

/*
entry in the font list
*/

typedef struct FONT_CACHE_ENTRY
{
    font handle;
    S32 x_size;
    S32 y_size;
    /* U8Z name[] follows */
}
* P_FONT_CACHE_ENTRY;

/*
local routines
*/

static font
font_cache(
    _In_reads_bytes_(namlen) const char * name,
    S32 namlen,
    S32 x_size,
    S32 y_size);

static void
font_cancel_hilites(
    char **pto);

static font
font_derive(
    font handle,
    _In_z_      const char * name,
    _In_z_      const char * supplement,
    _InVal_     BOOL add);

static P_FONT_CACHE_ENTRY
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
    U32 r,
    U32 g,
    U32 b,
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
    S32 y_size);

static void
font_super_sub(
    S32 hilite_n,
    char **pto);

static void
result_to_string(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    _InVal_     S32 expand_flags);

static S32
sprintnumber(
    char *array_start,
    char *array,
    char format,
    F64 value,
    BOOL eformat,
    S32 fwidth);

#define NUM_HILITES 8

#define RFM_FONT_CHANGE     26
#define RFM_UL_CHANGE       25
#define RFM_COL_INVERT      18
#define RFM_COL_RGB_CHANGE  19
#define RFM_Y_SHIFT         11
#define RFM_X_SHIFT         9

static P_LIST_BLOCK font_cache_list = NULL;
static font current_font = 0;
static font slot_font;
static char hilite_state[NUM_HILITES];
static font start_font = 0;
static S32 current_shift = 0;
static S32 old_height = 0;

/******************************************************************************
*
*  bitstr: copy from "from" to "to" expanding text-at fields
*
*  MRJC added draw file handling 28/6/89
*  MRJC added font handling 30/6/89
*
******************************************************************************/

static void
bitstr_harder(
    const char * const from_in,
    _InVal_     ROW row,
    char * const buffer /*out*/,
    _InVal_     coord fwidth_in,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags);

/* SKS 20160831 try optimising for the case where we have no complications whatsoever */

static void
bitstr(
    const char * const from_in,
    _InVal_     ROW row,
    char * const buffer /*out*/,
    _InVal_     coord fwidth_in,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    const char * from = from_in;
    const coord fwidth = (riscos_fonts) ? 6144 /* output is truncated at a later point */ : fwidth_in;
    const uchar text_at_char = get_text_at_char();
    coord length = 0;

    while(length <= fwidth)
    {
        const uchar u8 = *from++;

        if(u8 < CH_SPACE)
        {
            if(CH_NULL == u8)
                break;

            goto harder; /* CtrlChar handling is harder */
        }

        if(u8 >= CH_DELETE)
        {
            goto harder; /* CtrlChar and t.b.s. handling is harder */
        }

        if((u8 == text_at_char) /*no need to test (CH_NULL == text_at_char)*/)
        {
            goto harder; /* text-at char handling is harder */
        }

        /* keep it simple for 99% of cases */
        buffer[length++] = u8;
    }

    /* add final delimiter */
    buffer[length] = CH_NULL;
    return;

harder:; /* you may not like this but it generates far better code */

    /* keep going, doing it the hard way as before */
    bitstr_harder(from - 1, row, buffer + length, fwidth_in - length, expand_refs, expand_flags);
    return;
}

static void
bitstr_highlight_underline(
    P_U8 * p_to);

static void
bitstr_highlight_bold(
    P_U8 * p_to);

static void
bitstr_highlight_italic(
    P_U8 * p_to);

#define TEXT_AT_FIELD_EXPANDED     0
#define TEXT_AT_FIELD_UNEXPANDED   1
#define TEXT_AT_FIELD_UNRECOGNIZED 2

_Check_return_
static S32
bitstr_text_at_SLRLD1(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     ROW row,
    _InVal_     coord fwidth,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags,
    /*inout*/   coord * const p_length,
    _InoutRef_  P_BOOL p_output_trailing_text_at_chars);

_Check_return_
static S32
bitstr_text_at_TEXT_AT(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_NUMBER(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_C(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags,
    _InoutRef_  P_BOOL p_needs_colour_change_revert);

_Check_return_
static S32
bitstr_text_at_D(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_F(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_G(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     ROW row,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_L(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_N(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_P(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

_Check_return_
static S32
bitstr_text_at_T(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags);

static void
bitstr_harder(
    const char * const from_in,
    _InVal_     ROW row,
     /*out*/ char * const buffer,
    _InVal_     coord fwidth_in,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    const char * from = from_in;
    char * to = buffer;
    const coord fwidth = (riscos_fonts) ? 6144 /* output is truncated at a later point */ : fwidth_in;
    const uchar text_at_char = get_text_at_char();
    coord length = 0;
    BOOL output_trailing_text_at_chars;
    coord reflength;
    const BOOL expand_ctrl = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_CTRL));
    S32 res;
    S32 i = 0;
    BOOL needs_colour_change_revert = FALSE;

    /* start cell with global font */
    if(riscos_fonts)
    {
        start_font = 0;
        current_shift = old_height = 0;
    }

    do { hilite_state[i] = 0; } while(++i < NUM_HILITES);

    while(length <= fwidth)
    {
        /*eportf("bitstr loop: 0x%.2x %s", *from, from);*/

        if(CH_NULL == *from)
            break;

        if(SLRLD1 == *from) /* have to cater for orphaned SLRLD1 (text_at_char changed) */
        {   /* we need to do this otherwise we get the SLRLD1 character and following bytes displayed... */
            COL tcol;
            ROW trow;
            EV_DOCNO docno;
            const uchar * csr = from + 2; /* CSR is past the SLRLD1/2 */

            /* skip this compiled cell reference */
            from = talps_csr(csr, &docno, &tcol, &trow);
            reportf("bitstr: orphaned CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);

            reflength = write_ref(to, BUF_MAX_REFERENCE, docno, tcol, trow);
            to += reflength;
            length += reflength+1;

            /* and there will be no trailing text-at chars as that has changed! */
            continue;
        }

        if((*from != text_at_char)  /*||  (CH_NULL == text_at_char)*/)
        {
            if(!ishighlight(*from))
            {
                /* check for C0 control characters (and Acorn C1 redefs too if riscos_fonts) */
                if(expand_ctrl                                                                                         &&
                    ((*from < CH_SPACE) || (CH_DELETE == *from) ||
                     (riscos_fonts  &&  (*from >= 128)  &&  (*from < 160)  &&  !font_charwid(current_font, *from)))
                                                                                                                       )
                {
                    const S32 add_n = sprintf(to, "[%.2x]", (S32) *from);
                    to += add_n;
                    length += add_n;
                    from++;
                    continue;
                }

                /* in case you're wondering, this is the 99% case! */
                *to++ = *from++;
                length++;
                continue;
            }

            /*eportf("bitstr highlight %d", (*from - FIRST_HIGHLIGHT) + 1);*/

            /* process highlights */
            if(!riscos_fonts)
            {   /* not riscos_fonts: simply toggle highlight state */
                hilite_state[*from - FIRST_HIGHLIGHT] ^= 1;
                *to++ = *from++;
                continue;
            }

            /* riscos_fonts: turn highlights into font change sequences */
            switch(*from)
            {
            case HIGH_UNDERLINE:
                bitstr_highlight_underline(&to);
                break;

            case HIGH_BOLD:
                bitstr_highlight_bold(&to);
                break;

            case HIGH_ITALIC:
                bitstr_highlight_italic(&to);
                break;

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
        }

        /* text-at chars drop thru to here */
        /*eportf("bitstr text-at sequence: %s", from);*/

        /* skip leading text-at char */
        ++from;

        /* expand to this point in the output buffer */
        /* generally truncates to fwidth as necessary */
        *to = CH_NULL;

        /* set this to output all the trailing ones from "from" onwards */
        output_trailing_text_at_chars = FALSE;

        res = TEXT_AT_FIELD_UNRECOGNIZED;

        switch(toupper(*from))
        {
        case SLRLD1: /* display decompiled cell reference */
            res = bitstr_text_at_SLRLD1(&from, &to, text_at_char,
                                        row, fwidth,
                                        expand_refs,
                                        expand_flags,
                                        &length,
                                        &output_trailing_text_at_chars);
            break;

        case 'C': /* font Colour change */
            res = bitstr_text_at_C(&from, &to, text_at_char, expand_flags,
                                   &needs_colour_change_revert);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'D': /* display today's Date */
            res = bitstr_text_at_D(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'F': /* Font change */
            res = bitstr_text_at_F(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'G': /* imaGe file */
            res = bitstr_text_at_G(&from, &to, text_at_char,
                                   row,
                                   expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'L': /* display Leafname */
            res = bitstr_text_at_L(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'N': /* display current time (Now) */
            res = bitstr_text_at_N(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'P': /* display current Page number */
            res = bitstr_text_at_P(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        case 'T': /* display Title from option page */
            res = bitstr_text_at_T(&from, &to, text_at_char, expand_flags);
            if(TEXT_AT_FIELD_EXPANDED != res)
                output_trailing_text_at_chars = TRUE;
            break;

        default:
            if(*from == text_at_char) /*case text_at_char:*/
            {
                res = bitstr_text_at_TEXT_AT(&from, &to, text_at_char, expand_flags);
                if(TEXT_AT_FIELD_EXPANDED != res)
                    output_trailing_text_at_chars = TRUE;
                break;
            }

            if(isdigit(*from)) /* mailing list parameter */
            {
                res = bitstr_text_at_NUMBER(&from, &to, text_at_char, expand_flags);
                if(TEXT_AT_FIELD_EXPANDED != res)
                    output_trailing_text_at_chars = TRUE;
                break;
            }

            res = TEXT_AT_FIELD_UNRECOGNIZED;
            break;

        } /* end of switch('letter') */

        switch(res)
        {
        default:
        case TEXT_AT_FIELD_EXPANDED:
            /* everything is already in the output */
            /* See SLRLD1 special case - don't be tempted to set output_trailing_text_at_chars = FALSE; */
            break;

        case TEXT_AT_FIELD_UNEXPANDED:
            { /* copy the input from "from" (pointing at the letter) to the output */
            U32 outidx = 0;

            to[outidx++] = text_at_char; /* emit the text-at char we skipped to get this far */

            /* skip to trailing text-at chars on input */
            while(*from != text_at_char)
            {
                if(CH_NULL == *from)
                    break;

                to[outidx++] = *from++;
            }

            to[outidx] = CH_NULL;

            output_trailing_text_at_chars = TRUE; /* and all the trailing ones please */
            break;
            }

        case TEXT_AT_FIELD_UNRECOGNIZED:
            reportf("Unrecognized @%s:", from);
            /* emit text-at char - we will encounter this plain letter next time, then maybe a text-at chars sequence. or not. */
            to[0] = text_at_char;
            to[1] = CH_NULL;
            /* don't output any trailing text-at chars as none of them belong to this unrecognized letter! */
            break;
        }

        /* move "to" to end of string (may already have been done if font changes inserted) */
        while((CH_NULL != *to) && (length <= fwidth))
        {
            ++to;
            ++length;
        }

        /* skip over all trailing text-at chars on input */
        while(*from == text_at_char)
        {
            ++from;

            /* if required, copy them over to the output */
            if(output_trailing_text_at_chars && (length <= fwidth))
            {
                *to++ = text_at_char;
                ++length;
            }
        }

    } /* end of while */

    if(needs_colour_change_revert)
    {
        font_insert_colour_change(0, 0, 0, &to);
      /*needs_colour_change_revert = FALSE; redundant assignment */
    }

    /* add final delimiter */
    *to = CH_NULL;
}

static void
bitstr_highlight_underline(
    P_U8 * p_to)
{
    P_U8 to = *p_to;

    trace_0(TRACE_APP_PD4, "bitstr highlight 1: underline");

    *to++ = RFM_UL_CHANGE;

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
    {
        *to++ = 0;
    }

    hilite_state[0] ^= 1;

    *p_to = to;
}

static void
bitstr_highlight_bold(
    P_U8 * p_to)
{
    P_U8 to = *p_to;
    P_FONT_CACHE_ENTRY fp = font_find_block(current_font);

    trace_2(TRACE_APP_PD4, "bitstr highlight 2: bold B:=%d, I=%d", !hilite_state[1], hilite_state[3]);

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

    *p_to = to;
}

static void
bitstr_highlight_italic(
    P_U8 * p_to)
{
    P_U8 to = *p_to;

    P_FONT_CACHE_ENTRY fp = font_find_block(current_font);

    trace_2(TRACE_APP_PD4, "bitstr highlight 4: italic B=%d I:=%d", hilite_state[1], !hilite_state[3]);

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

    *p_to = to;
}

_Check_return_
static S32
bitstr_text_at_SLRLD1(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     ROW row,
    _InVal_     coord fwidth,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags,
    /*inout*/   coord * const p_length,
    _InoutRef_  P_BOOL p_output_trailing_text_at_chars)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_1 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_1));

    COL tcol;
    ROW trow;
    EV_DOCNO docno;
    const uchar * csr = from + 2; /* CSR is past the SLRLD1/2 */

    /* skip this compiled cell reference */
    from = talps_csr(csr, &docno, &tcol, &trow);
    /*eportf("bitstr: CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);*/

    if(expand_ats_1 && (expand_refs > 0) && !bad_reference(tcol, trow))
    {
        P_CELL tcell = travel_externally(docno, tcol & COLNOBITS, trow & ROWNOBITS);
        const S32 expand_flags_recurse =
            expand_flags & ~(riscos_fonts ? 0 : EXPAND_FLAGS_ALLOW_FONTY_RESULT);

        if(NULL != tcell)
            (void) expand_cell(
                        docno, tcell, row, to, fwidth - *p_length,
                        expand_refs-1,
                        expand_flags_recurse,
                        TRUE /*cff*/);

        if(riscos_fonts)
        {
            /* skip over possibly fonty string */
            while(CH_NULL != *to)
                to += font_skip(to);
        }
        else
        {
            /* skip string and delete funny spaces from plain non-fonty string */
            P_U8Z tempto = to;

            while(CH_NULL != *to)
            {
#if 1
                if(FUNNYSPACE == *to)
                {
                    ++to;
                    continue;
                }
                /* else discuss. SKS 20160830 why strip highlights when we don't elsewhere - containing string may have them */
#else
                /* evict highlights and funny spaces */
                if(*to < CH_SPACE)
                {
                        ++to;
                        continue;
                }
#endif

                *tempto++ = *to++;
                *p_length += 1;
            }

            to = tempto;
            *to = CH_NULL;
        }

        *p_output_trailing_text_at_chars = FALSE;
    }
    else
    {
        /* if we are not allowed to expand it because we are
         * already expanding, or error, just emit @cell_reference@
        */
        coord reflength;

        *to++ = text_at_char;
        *p_length += 1;

        reflength = write_ref(to, BUF_MAX_REFERENCE, docno, tcol, trow);
        to += reflength;
        *p_length += reflength;

        *to = CH_NULL;

        /* but ... */
        *p_output_trailing_text_at_chars = TRUE;
    }

    *p_from = from; /* pointing at the trailing text-at chars */
    *p_to = to; /* update to */

    return(TEXT_AT_FIELD_EXPANDED);
}

_Check_return_
static S32
bitstr_text_at_TEXT_AT(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    if(!expand_ats_2)
        return(TEXT_AT_FIELD_UNEXPANDED);

    /* emit n-1 text-at chars (i.e. all those we are pointing to) */
    while(*from == text_at_char)
        *to++ = *from++;

    *to = CH_NULL;

    *p_from = from; /* pointing past the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

_Check_return_
static S32
bitstr_text_at_NUMBER(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));
    S32 key;
    P_LIST lptr;

    key = (S32) atoi(from);

    /* munge digits */
    while(isdigit(*from))
        ++from;

    UNREFERENCED_PARAMETER_InVal_(text_at_char);
    /* NO!!! if(*from != text_at_char)
        return(TEXT_AT_FIELD_UNRECOGNIZED); */

    if(!expand_ats_2)
        return(TEXT_AT_FIELD_UNEXPANDED);

    lptr = search_list(&first_macro, key);

    if(NULL != lptr)
    {
        const char * value = lptr->value;

        strcpy(to, value);
    }
    else if(sqobit)
    {   /* leave blank when printing if missing */
    }
    else
    {
        return(TEXT_AT_FIELD_UNEXPANDED);
    }

    *p_from = from; /* pointing past the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

_Check_return_
static S32
bitstr_text_at_C(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags,
    _InoutRef_  P_BOOL p_needs_colour_change_revert)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    U32 r, g, b;
    U32 n_chars;

    /* check for colour revert */
    if((from[1] == text_at_char) || ((from[1] == ':') && (from[2] == text_at_char)))
    {
        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from;
        if(*from == ':')
           ++from;

        if(riscos_fonts) /* <t-a-c>C<t-a-c> -> back to black */
        {
            font_insert_colour_change(0, 0, 0, &to);
            *p_needs_colour_change_revert = FALSE;
        }

        *to = CH_NULL;
        *p_from = from; /* pointing at the trailing text-at chars */
        *p_to = to; /* update to */
        return(TEXT_AT_FIELD_EXPANDED);
    }

    if((from[1] == ':') && (0 != (n_chars = get_colour_value(from+2, &r, &g, &b, text_at_char))) && (from[2+n_chars] == text_at_char))
    {
        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        from += 2 + n_chars;

        if(riscos_fonts)
        {
            font_insert_colour_change(r, g, b, &to);
            *p_needs_colour_change_revert = TRUE;
        }

        *to = CH_NULL;
        *p_from = from; /* pointing at the trailing text-at chars */
        *p_to = to; /* update to */
        return(TEXT_AT_FIELD_EXPANDED);
    }

    return(TEXT_AT_FIELD_UNRECOGNIZED);
}

_Check_return_
static STATUS
ss_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_SS_DATA p_ss_data)
{
    NUMFORM_PARMS numform_parms /*= NUMFORM_PARMS_INIT*/;

    numform_parms.ustr_numform_numeric =
    numform_parms.ustr_numform_datetime =
    numform_parms.ustr_numform_texterror = ustr;

    numform_parms.p_numform_context = get_p_numform_context();

    return(numform(p_quick_ublock, P_QUICK_UBLOCK_NONE, p_ss_data, &numform_parms));
}

/* skip the format to the trailing text-at chars (date/time format ought not to include any!) */

_Check_return_
static U32
bitstr_text_at_D_N_P_format_count(
    _In_z_      PC_U8Z from,
    _InVal_     uchar text_at_char)
{
    U32 n_chars;

    for(n_chars = 0; text_at_char != from[n_chars]; ++n_chars)
    {
        if(CH_NULL == from[n_chars])
            return(0);
    }

    return(n_chars);
}

/* output ss_data using the format we just detected */

static void
bitstr_text_at_D_N_P_format_common(
    /*out*/     P_U8Z to,
    _In_reads_(n_chars) PC_UCHARS format,
    _InVal_     U32 n_chars,
    _InRef_     PC_SS_DATA p_ss_data)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    /* like c_text */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, format, n_chars)))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ss_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data);

    if(status_fail(status))
        strcpy(to, reperr_getstr(status));
    else
        strcpy(to, quick_ublock_ustr(&quick_ublock_result));
}

static void
bitstr_text_at_D_classic(
    /*out*/ P_U8 to)
{
    time_t now;
    struct tm * temp_time;
    int month, mday, year;

    time(&now);
    temp_time = localtime(&now);

    month = temp_time->tm_mon+1;
    mday  = temp_time->tm_mday;
    year  = temp_time->tm_year;

    year += 1900; /* SKS 01 Dec 1996 says sod them. They're having the proper date from now on */

    if(d_options_DF == 'T')
    {   /* generate 21 March 1988 */
        const S32 month_idx = month - 1;
        PC_USTR ustr_monthname = get_p_numform_context()->month_names[month_idx];
        consume_int(sprintf(to, "%d %s %.4d",
                            mday,
                            ustr_monthname,
                            year));
    }
    else
    {   /* generate 21.3.1988 or 3.21.1988 */
        consume_int(sprintf(to, "%d.%d.%d",
                            (d_options_DF == 'A') ? month : mday,
                            (d_options_DF == 'A') ? mday : month,
                            year));
    }
}

_Check_return_
static S32
bitstr_text_at_D(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_1 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_1));

    U32 n_chars;

    if((':' == from[1]) && (0 != (n_chars = bitstr_text_at_D_N_P_format_count(from + 2, text_at_char))))
    {
        SS_DATA ss_data;

        if(!expand_ats_1)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */
        ++from; /* skip the colon to the format */

        /* like c_now */
        ss_data.data_id = DATA_ID_DATE;
        ss_date_set_from_local_time(&ss_data.arg.ss_date);

        bitstr_text_at_D_N_P_format_common(to, from, n_chars, &ss_data);

        from += n_chars; /* pointing at the trailing text-at chars */
    }
    else
    {
        if(from[1] != text_at_char)
            return(TEXT_AT_FIELD_UNRECOGNIZED);

        if(!expand_ats_1)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */

        bitstr_text_at_D_classic(to);
    }

    *p_from = from; /* pointing at the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

_Check_return_
static S32
bitstr_text_at_F(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    char *startto = to;
    PC_U8Z name;
    F64 x, y;
    S32 len;

    trace_1(TRACE_APP_PD4, "bitstr text-at field F str: %s", report_sbstr(from));

    /* check for font revert */
    if((from[1] == text_at_char) || ((from[1] == ':') && (from[2] == text_at_char)))
    {
        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from;
        if(*from == ':')
            ++from;

        font_cancel_hilites(&to);

        if(riscos_fonts)
            font_insert_change(slot_font, &to);

        *to = CH_NULL;
        *p_from = from; /* pointing at the trailing text-at chars */
        *p_to = to; /* update to */
        return(TEXT_AT_FIELD_EXPANDED);
    }

    /* try to make sense of the font change */
    if(0 != (len = readfxy('F', &from, &to, &name, &x, &y)))
    {
        font fonthan;

        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        to = startto;

        /* read current font if none given */
        if(isdigit(*name))
        {
            P_FONT_CACHE_ENTRY fp = font_find_block(current_font);

            if(NULL != fp)
            {
                PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                y = x; /* carry over 2nd parameter */
                consume_int(sscanf(name, "%lg", &x));
                if(-1 == y)
                    y = x;
                name = fp_name;
                len = strlen(name);
            }
        }

        /* read current size if none given */
        if(x == -1)
        {
            P_FONT_CACHE_ENTRY fp = font_find_block(current_font);

            if(NULL != fp)
            {
                x = fp->x_size;
                y = fp->y_size;
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

        *to = CH_NULL;
        *p_from = from; /* pointing at the trailing text-at chars */
        *p_to = to; /* update to */
        return(TEXT_AT_FIELD_EXPANDED);
    }

    return(TEXT_AT_FIELD_UNRECOGNIZED);
}

_Check_return_
static S32
bitstr_text_at_G(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     ROW row,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2)) && (row != currow); /* SKS thinks latter condition is not needed */

    char *startto = to;
    PC_U8Z name;
    S32 len;
    F64 x, y;

    UNREFERENCED_PARAMETER_InVal_(text_at_char);

    trace_1(TRACE_APP_PD4, "bitstr text-at field G str: %s", report_sbstr(from));

    if(0 != (len = readfxy('G', &from, &to, &name, &x, &y)))
    {
        /* look up image file; replace reference with error if there is one, otherwise don't show any of the reference */
        char namebuf[BUF_MAX_PATHSTRING];
        IMAGE_CACHE_HANDLE cah;
        S32 err;

        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        xstrnkpy(namebuf, elemof32(namebuf), name, len);

        if(image_cache_entry_query(&cah, namebuf))
            err = image_cache_error_query(&cah); /* not actually displayed at this point! */
        else
            err = ERR_NOTFOUND;

        to = startto;
        if(err)
        {
            const PC_U8Z errstr = reperr_getstr(err);
            strcpy(to, errstr);
        }

        *to = CH_NULL;
        *p_from = from; /* pointing at the trailing text-at chars */
      /**p_to = to; don't update to - let caller work out fwidth restrictions */
        return(TEXT_AT_FIELD_EXPANDED);
    }

    return(TEXT_AT_FIELD_UNRECOGNIZED);
}

_Check_return_
static S32
bitstr_text_at_L(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    if(from[1] != text_at_char)
        return(TEXT_AT_FIELD_UNRECOGNIZED);

    if(!expand_ats_2)
        return(TEXT_AT_FIELD_UNEXPANDED);

    ++from; /* skip the letter */

    strcpy(to, current_document()->ss_instance_data.ss_doc.docu_name.leaf_name);

    *p_from = from; /* pointing at the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

static void
bitstr_text_at_N_classic(
    /*out*/ P_U8 to)
{
    time_t today_time = time(NULL);
    struct tm * split_timep = localtime(&today_time);

    consume_int(sprintf(to,
                        "%.2d:%.2d:%.2d",
                        split_timep->tm_hour,
                        split_timep->tm_min,
                        split_timep->tm_sec));
}

_Check_return_
static S32
bitstr_text_at_N(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_1 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_1));

    U32 n_chars;

    if((':' == from[1]) && (0 != (n_chars = bitstr_text_at_D_N_P_format_count(from + 2, text_at_char))))
    {
        SS_DATA ss_data;

        if(!expand_ats_1)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */
        ++from; /* skip the colon to the format */

        /* like c_now */
        ss_data.data_id = DATA_ID_DATE;
        ss_date_set_from_local_time(&ss_data.arg.ss_date);

        bitstr_text_at_D_N_P_format_common(to, from, n_chars, &ss_data);

        from += n_chars; /* pointing at the trailing text-at chars */
    }
    else
    {
        if(from[1] != text_at_char)
            return(TEXT_AT_FIELD_UNRECOGNIZED);

        if(!expand_ats_1)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */

        bitstr_text_at_N_classic(to);
    }

    *p_from = from; /* pointing at the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

static void
bitstr_text_at_P_classic(
    /*out*/ P_U8 to)
{
    consume_int(sprintf(to, "%d", curpnm)); /* must be made valid by caller */
}

_Check_return_
static S32
bitstr_text_at_P(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    U32 n_chars;

    if((':' == from[1]) && (0 != (n_chars = bitstr_text_at_D_N_P_format_count(from + 2, text_at_char))))
    {
        SS_DATA ss_data;

        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */
        ++from; /* skip the colon to the format */

        ss_data_set_integer(&ss_data, curpnm);

        bitstr_text_at_D_N_P_format_common(to, from, n_chars, &ss_data);

        from += n_chars; /* pointing at the trailing text-at chars */
    }
    else
    {
        if(from[1] != text_at_char)
            return(TEXT_AT_FIELD_UNRECOGNIZED);

        if(!expand_ats_2)
            return(TEXT_AT_FIELD_UNEXPANDED);

        ++from; /* skip the letter */

        bitstr_text_at_P_classic(to);
    }

    *p_from = from; /* pointing at the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
}

_Check_return_
static S32
bitstr_text_at_T(
    PC_U8Z * p_from,
    P_U8 * p_to,
    _InVal_     uchar text_at_char,
    _InVal_     S32 expand_flags)
{
    PC_U8Z from = *p_from; /* pointing at the letter */
    P_U8 to = *p_to;
    const BOOL expand_ats_2 = (0 != (expand_flags & EXPAND_FLAGS_EXPAND_ATS_2));

    if(from[1] != text_at_char)
        return(TEXT_AT_FIELD_UNRECOGNIZED);

    if(!expand_ats_2)
        return(TEXT_AT_FIELD_UNEXPANDED);

    ++from; /* skip the letter */

    if(!str_isblank(d_options_DE))
    {
        const char * title = d_options_DE;

        while(CH_NULL != *title)
        {
            if(riscos_fonts  &&  (*title < SPACE))
            {
                ++title;
                continue;
            }

            *to++ = *title++;
        }
    }

    *to = CH_NULL;

    *p_from = from; /* pointing at the trailing text-at chars */
  /**p_to = to; don't update to - let caller work out fwidth restrictions */

    return(TEXT_AT_FIELD_EXPANDED);
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
    _InVal_     ROW row,
    /*out*/ char *array,
    coord fwidth,
    S32 expand_refs,
    _InVal_     S32 expand_flags,
    _InVal_     BOOL compile_lcr)
{
    char delimiter = *from;
    char *to = array;
    S32 i;
    char compiled[COMPILED_TEXT_BUFSIZ];
    char tbuf[PAINT_STRSIZ];
    char *tptr;
    S32 old_riscos_fonts;

    old_riscos_fonts = font_stack(riscos_fonts);

    if(0 == (expand_flags & EXPAND_FLAGS_ALLOW_FONTY_RESULT))
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

            bitstr(
                tptr, row, to, fwidth,
                expand_refs,
                expand_flags);

            /* move to after end of possibly fonty string */
            if(riscos_fonts)
            {
                while(CH_NULL != *to)
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
                short_memcpy32nz(tptr, from, COMPILED_TEXT_SLR_SIZE-1);
                tptr += COMPILED_TEXT_SLR_SIZE-1;
                from += COMPILED_TEXT_SLR_SIZE-1;
            }
        }

        *tptr = CH_NULL;

        /* revert to cell font before each section */
        if(riscos_fonts  &&  i)
            font_insert_change(slot_font, &to);

        if(compile_lcr)
        {
            compile_text_slot(compiled, tbuf, NULL);
            tptr = compiled;
        }
        else
            tptr = tbuf;

        bitstr(
            tptr, row, to, fwidth,
            expand_refs,
            expand_flags);

        trace_1(TRACE_APP_PD4, "expand_lcr: %s", report_sbstr(to));

        if(riscos_fonts)
        {
            while(CH_NULL != *to)
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
* expand the contents of the cell into array
*
* returns the justification state
*
* expand_refs level says whether we are allowed to expand cell references inside
* the cell so text cells referring to themselves are not recursive
*
* Note that strings returned by expand_cell() can now contain RISC OS
* font change sequences and thus embedded CH_NULLs if
* riscos_fonts && EXPAND_FLAGS_ALLOW_FONTY_RESULT.
*
* Use font_skip() to work out the size of each character in the resulting string.
*
******************************************************************************/

extern char
expand_cell(
    _InVal_     DOCNO docno,
    P_CELL tcell,
    _InVal_     ROW row,
    /*out*/ char *array,
    coord fwidth,
    S32 expand_refs,
    _InVal_     S32 expand_flags,
    _InVal_     BOOL cust_func_formula)
{
    char justify = tcell->justify & J_BITS;
    char *array_start = array;
    S32 old_riscos_fonts;
    P_EV_RESULT p_ev_result;
    DOCNO old_docno;

    /* switch to target document */
    old_docno = change_document_using_docno(docno);

    old_riscos_fonts = font_stack(riscos_fonts);

    if((0 == (expand_flags & EXPAND_FLAGS_ALLOW_FONTY_RESULT)))
        riscos_fonts = FALSE;

    if(riscos_fonts)
    {
        current_font = 0;

        if((slot_font = font_get_global()) > 0)
            font_insert_change(slot_font, &array);
    }

    switch(result_extract(tcell, &p_ev_result))
    {
    case SL_TEXT:
        if(justify == J_LCR)
        {
            expand_lcr(
                tcell->content.text, row, array, fwidth,
                expand_refs,
                expand_flags,
                FALSE /*!compile_lcr*/);
        }
        else
        {
            bitstr(
                tcell->content.text, row, array, fwidth,
                expand_refs,
                expand_flags);
            /* NB bitstr() will return a fonty result if riscos_fonts */
        }
        break;

    case SL_PAGE:
        sprintf(array, "~ %d", tcell->content.page.condval);
        justify = J_LEFT;
        break;

    case SL_NUMBER:
        if(cust_func_formula && ev_doc_is_custom_sheet(docno) &&
           ev_is_formula(&tcell->content.number.guts))
        {
            EV_OPTBLOCK optblock;

            ev_set_options(&optblock, docno);

            strcpy(array, "...");

            ev_decode_slot(docno, array + 3, &tcell->content.number.guts, &optblock);

            /* SKS after PD 4.11 29jan92 - LCR custom function display gave drug-crazed redraw */
            if(justify == J_LCR)
                justify = J_LEFT;
        }
        else
        {
            result_to_string(
                p_ev_result, docno, array_start, array,
                tcell->format,
                fwidth, &justify,
                expand_refs,
                expand_flags);
        }
        break;

    default:
        trace_0(TRACE_APP_PD4, "expand_cell found bad type");
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
    S32 x_size,
    S32 y_size)
{
    char namebuf[BUF_MAX_PATHSTRING];
    U32 namelen_p1;
    P_LIST lptr;
    P_FONT_CACHE_ENTRY fp;
    P_U8Z fp_name;
    font fonthan;
    S32 res;

    xstrnkpy(namebuf, elemof32(namebuf), name, namlen);

    trace_3(TRACE_APP_PD4, "font_cache %s, x:%d, y:%d ************", report_sbstr(namebuf), x_size, y_size);

    /* search list for entry */
    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
    {
        fp = (P_FONT_CACHE_ENTRY) lptr->value;

        fp_name = (P_U8Z) (fp + 1);

        if( (fp->x_size == x_size)  &&
            (fp->y_size == y_size)  &&
            (0 == C_stricmp(namebuf, fp_name)) )
        {
            if(fp->handle > 0)
            {
                trace_4(TRACE_APP_PD4, "font_cache found cached font: (handle:%d), size: %d,%d %s",
                        fp->handle, x_size, y_size, report_sbstr(namebuf));
            }
            else
            {
                /* matched entry in cache but need to reestablish font handle */
                if(font_find(namebuf, x_size, y_size, 0, 0, &fonthan))
                    fonthan = 0;
                fp->handle = fonthan;
                if(fp->handle > 0)
                    trace_4(TRACE_APP_PD4, "font_cache reestablish cached font OK: (handle:%d) size: %d,%d %s",
                            fonthan, x_size, y_size, report_sbstr(namebuf));
                else
                    trace_4(TRACE_APP_PD4, "font_cache reestablish cached font FAIL: (handle:%d) size: %d,%d %s",
                            fp->handle, x_size, y_size, report_sbstr(namebuf));
            }
            return(fp->handle);
        }
    }

    /* create entry for font */

    /* ask font manager for handle */
    if(font_find(namebuf, x_size, y_size, 0, 0, &fonthan))
        fonthan = 0;

    namelen_p1 = strlen32p1(namebuf);

    if(NULL == (lptr = add_list_entry(&font_cache_list, sizeof32(struct FONT_CACHE_ENTRY) + namelen_p1, &res)))
    {
        trace_4(TRACE_APP_PD4, "font_cache failed to add entry for uncached font: (handle:%d) size: %d,%d  %s",
                fonthan, x_size, y_size, report_sbstr(namebuf));
        reperr_null(res);
        if(fonthan > 0)
            font_LoseFont(fonthan);
        return(0);
    }

    /* change key so we don't find it */
    lptr->key = 0;

    fp = (P_FONT_CACHE_ENTRY) lptr->value;
    fp->handle = fonthan;
    fp->x_size = x_size;
    fp->y_size = y_size;

    fp_name = (P_U8Z) (fp + 1);
    memcpy(fp_name, namebuf, namelen_p1);

    trace_4(TRACE_APP_PD4, "font_cache found uncached font: (handle:%d) size: %d,%d %s",
            fonthan, x_size, y_size, report_sbstr(namebuf));
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
            *to++ = RFM_UL_CHANGE;
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
    tbuf[1] = CH_NULL;
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
    P_LIST lptr;

    if(shutdown)
        lptr = NULL; /* just for breakpoint */

    lptr = first_in_list(&font_cache_list);

    while(NULL != lptr)
    {
        P_FONT_CACHE_ENTRY fp = (P_FONT_CACHE_ENTRY) lptr->value;

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
            reperr(ERR_FONTY, err->errmess);
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
    P_FONT_CACHE_ENTRY fp;
    char namebuf[BUF_MAX_PATHSTRING];

    trace_4(TRACE_APP_PD4, "font_derive: handle: %d, name: %s %s supplement: %s",
            handle, name, add ? "add" : "remove", report_sbstr(supplement));

    if(NULL == (fp = font_find_block(handle)))
        return(0);

    /* should we add or remove the supplement ? */
    if(add)
    {
        xstrkpy(namebuf, elemof32(namebuf), name);
        xstrkat(namebuf, elemof32(namebuf), supplement);
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

    trace_1(TRACE_APP_PD4, "font_derive_font about to cache: %s", report_sbstr(namebuf));
    return(font_cache(namebuf, strlen(namebuf), fp->x_size, fp->y_size));
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
           0 /*expand_refs*/,
           EXPAND_FLAGS_DONT_EXPAND_ATS /*!expand_ats*/ |
           EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ /*expand_flags*/);
}

/******************************************************************************
*
* return pointer to a font block given the font handle
*
******************************************************************************/

static P_FONT_CACHE_ENTRY
font_find_block(
    _InVal_     font fonthan)
{
    P_LIST lptr;

    trace_1(TRACE_APP_PD4, "font_find_block: %d", fonthan);

    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
    {
        P_FONT_CACHE_ENTRY fp = (P_FONT_CACHE_ENTRY) lptr->value;

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

    trace_1(TRACE_APP_PD4, "font_get_global: %s", report_sbstr(global_font));

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
        *(*pto)++ = RFM_FONT_CHANGE;
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
    U32 r,
    U32 g,
    U32 b,
    char **pto)
{
    U32 curbg;

    if(wimptx_os_version_query() < RISC_OS_3_5)
        return;

    if(0 != (0x10 & current_bg_wimp_colour_value))
        curbg = current_bg_wimp_colour_value & ~0x1F;
    else
        curbg = wimptx_RGB_for_wimpcolour(current_bg_wimp_colour_value & 0x0F);

    *(*pto)++ = RFM_COL_RGB_CHANGE;
    *(*pto)++ = (char) (curbg >>  8);
    *(*pto)++ = (char) (curbg >> 16);
    *(*pto)++ = (char) (curbg >> 24);
    *(*pto)++ = (char) r;
    *(*pto)++ = (char) g;
    *(*pto)++ = (char) b;
    *(*pto)++ = (char) 14;
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

    if(wimptx_os_version_query() >= RISC_OS_3_5)
    {
        newbg = curfg;
        newfg = curbg;

        fp->back_colour = newbg;
        fp->fore_colour = newfg;

        *(*pto)++ = RFM_COL_RGB_CHANGE;
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
        /* bg ignored by font manager in 256 colour modes on old systems, but preserve to reinvert later on */
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

    *(*pto)++ = RFM_COL_INVERT;
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
    current_shift += shift;
}

/******************************************************************************
*
* skip over font changes and other mess in a string
*
******************************************************************************/

extern S32
font_skip(
    const char * at)
{
    if(!riscos_fonts)
        return(1);

    switch(*at)
    {
    case RFM_FONT_CHANGE:
        return(2);

    case RFM_UL_CHANGE:
        return(3);

    case RFM_COL_INVERT:
    case RFM_X_SHIFT:
    case RFM_Y_SHIFT:
        return(4);

    case RFM_COL_RGB_CHANGE:
        return(8);

    default:
        return(1);
    }
}

/******************************************************************************
*
* given a font y size, squash it down
* to maximum line height on the screen
*
******************************************************************************/

static S32
font_squash(
    S32 y_size)
{
    S32 limit;

    if(sqobit)
        return(y_size);

    limit = (fontscreenheightlimit_millipoints * 16) / 1000;
    trace_3(TRACE_APP_PD4, "font_squash(%d): limit %d, limit_mp %d", y_size, limit, fontscreenheightlimit_millipoints);

    return(MIN(y_size, limit));
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

static GR_MILLIPOINT
font_stringwid(
    font str_font,
    char *str)
{
    font_string fs;
    GR_MILLIPOINT swidth_mp = 0;

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
            xstrkpy(newname, elemof_newname, fontname);
        return(STATUS_OK); /* supplement not stripped */
    }

    if(newname != fontname)
        xstrnkpy(newname, elemof_newname, fontname, place - fontname);
    else
        newname[place - fontname] = CH_NULL;

    xstrkat(newname, elemof_newname, place + strlen(strip));
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

    *to++ = CH_NULL;

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
        font_insert_shift(RFM_Y_SHIFT, -current_shift, pto);
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
            font_insert_shift(RFM_Y_SHIFT,
                              font_calshift(hilite_n, old_height)
                                                - current_shift,
                              pto);

            hilite_state[hilite_n] ^= 1;
        }
        else
        {   /* not in either super or subs */
            P_FONT_CACHE_ENTRY fp = font_find_block(current_font);

            if(NULL != fp)
            {
                const PC_U8Z fp_name = (PC_U8Z) (fp + 1);
                font subfont;

                if((subfont = font_cache(fp_name,
                                         strlen(fp_name),
                                         (fp->x_size * 3) / 4,
                                         (fp->y_size * 3) / 4)) > 0)
                {
                    old_height = fp->y_size;
                    start_font = fp->handle;

                    font_insert_change(subfont, pto);
                    font_insert_shift(RFM_Y_SHIFT, font_calshift(hilite_n, old_height), pto);
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
    if(!riscos_fonts)
        return(ishighlight(*ch));

    switch(*ch)
    {
    case RFM_FONT_CHANGE:
    case RFM_COL_INVERT:
    case RFM_COL_RGB_CHANGE:
    case RFM_UL_CHANGE:
    case RFM_X_SHIFT:
    case RFM_Y_SHIFT:
        return(1);

    default:
        return(0);
    }
}

/******************************************************************************
*
* extract pointer to result structure from cell
* provides a level of indirection from cell
* structure details
*
******************************************************************************/

extern S32
result_extract(
    P_CELL sl,
    P_EV_RESULT * p_p_ev_result)
{
    if(sl->type == SL_NUMBER)
        *p_p_ev_result = &sl->content.number.guts.ev_result;

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
    P_CELL sl)
{
    S32 res = 0;
    P_EV_RESULT p_ev_result;

    if( !ev_doc_is_custom_sheet(current_docno()) &&
        (result_extract(sl, &p_ev_result) == SL_NUMBER) )
    {
        switch(p_ev_result->data_id)
        {
        case DATA_ID_REAL:
            if(p_ev_result->arg.fp < 0.0)
                res = -1;
            else if(p_ev_result->arg.fp > 0.0)
                res = 1;
            break;

        case DATA_ID_LOGICAL:
        case DATA_ID_WORD16:
        case DATA_ID_WORD32:
            if(p_ev_result->arg.integer < 0)
                res = -1;
            else if(p_ev_result->arg.integer > 0)
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
result_to_string_NUMBER(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    F64 number;

    UNREFERENCED_PARAMETER_InVal_(docno);
    UNREFERENCED_PARAMETER_InVal_(expand_flags);

    if(DATA_ID_REAL == p_ev_result->data_id)
        number = p_ev_result->arg.fp;
    else
        number = (F64) p_ev_result->arg.integer;

    /* try to print number fully expanded - if not possible
     * try to print number in exponential notation - if not possible
     * print %
    */
    if(riscos_fonts)
        fwidth = cw_to_millipoints(fwidth);

    if(sprintnumber(array_start, array, format, number, FALSE, fwidth) >= fwidth)
        if(sprintnumber(array_start, array, format, number, TRUE, fwidth) >= fwidth)
            strcpy(array, "% ");

    if(!expand_refs)
    {
        char *last = array + strlen(array);

        if(*last == SPACE) /* should be OK even with riscos_fonts */
            *last = CH_NULL;
    }

    /* free alignment should display numbers right */
    if(*justify == J_FREE)
        *justify = J_RIGHT;
    else if(*justify == J_LCR)
        *justify = J_LEFT;
}

static void
result_to_string_ARRAY(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    EV_RESULT temp_ev_result;
    SS_DATA temp_data;

    /* go via SS_DATA */
    ev_result_to_data_convert(&temp_data, p_ev_result);

    ss_data_to_result_convert(&temp_ev_result, ss_array_element_index_borrow(&temp_data, 0, 0));

    result_to_string(&temp_ev_result, docno, array_start, array, format, fwidth, justify, expand_refs, expand_flags);

    ev_result_free_resources(&temp_ev_result);
}

static void
result_to_string_DATE(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    S32 len = 0;
    SS_DATE temp = p_ev_result->arg.ss_date;

    UNREFERENCED_PARAMETER_InVal_(docno);
    UNREFERENCED_PARAMETER(array_start);
    UNREFERENCED_PARAMETER(format);
    UNREFERENCED_PARAMETER(fwidth);
    UNREFERENCED_PARAMETER_InVal_(expand_refs);
    UNREFERENCED_PARAMETER_InVal_(expand_flags);

    if(temp.date)
    {
        S32 year, month, day;
        BOOL valid;

        valid = (ss_dateval_to_ymd(temp.date, &year, &month, &day) >= 0);

        if(d_options_DF == 'T')
        {
            if(valid)
            {   /* generate 21 Mar 1988 */
                const S32 month_idx = month - 1;
                const PC_USTR ustr_monthname = get_p_numform_context()->month_names[month_idx];
                len = sprintf(array, "%d %.3s %.4d", day, ustr_monthname, year);
            }
            else
            {
                len = strlen(strcpy(array, "**" " " "***" " " "****"));
            }
        }
        else
        {
            if(valid)
            {   /* generate 21.3.1988 or 3.21.1988 */
                len = sprintf(array, "%d.%d.%.4d",
                              ((d_options_DF == 'A')) ? month : day,
                              ((d_options_DF == 'A')) ? day : month,
                              year);
            }
            else
            {
                len = strlen(strcpy(array, "**" " " "**" " " "****"));
            }
        }
    }

    if(!temp.date || temp.time)
    {
        S32 hours, minutes, seconds;

        /* separate time from date */
        if(temp.date)
            array[len++] = ' ';

        status_consume(ss_timeval_to_hms(temp.time, &hours, &minutes, &seconds));

        consume_int(sprintf(array + len, "%.2d:%.2d:%.2d", hours, minutes, seconds));
    }

    if(*justify == J_LCR)
        *justify = J_LEFT;
}

static void
result_to_string_ERROR(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    _InVal_     S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    S32 len = 0;

    UNREFERENCED_PARAMETER(array_start);
    UNREFERENCED_PARAMETER(format);
    UNREFERENCED_PARAMETER(fwidth);
    UNREFERENCED_PARAMETER_InVal_(expand_refs);
    UNREFERENCED_PARAMETER_InVal_(expand_flags);

    if(p_ev_result->arg.ss_error.type == ERROR_PROPAGATED)
    {
        U8 buffer_slr[BUF_EV_MAX_SLR_LEN];
        EV_SLR ev_slr; /* unpack */
        ev_slr.col = p_ev_result->arg.ss_error.col;
        ev_slr.row = p_ev_result->arg.ss_error.row;
        ev_slr.docno = p_ev_result->arg.ss_error.docno;
        ev_slr.flags = 0;
        (void) ev_dec_slr(buffer_slr, docno, &ev_slr, true);
        len = sprintf(array, PropagatedErrStr, buffer_slr);
    }
    else if(p_ev_result->arg.ss_error.type == ERROR_CUSTOM)
    {
        len = sprintf(array, CustFuncErrStr, p_ev_result->arg.ss_error.row + 1);
    }

    strcpy(array + len, reperr_getstr(p_ev_result->arg.ss_error.status));

    *justify = J_LEFT;
}

static void
result_to_string(
    _InRef_     PC_EV_RESULT p_ev_result,
    _InVal_     DOCNO docno,
    char *array_start,
    char *array,
    char format,
    coord fwidth,
    char *justify,
    S32 expand_refs,
    _InVal_     S32 expand_flags)
{
    switch(p_ev_result->data_id)
    {
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_REAL:
        result_to_string_NUMBER(p_ev_result, docno, array_start, array, format, fwidth, justify, expand_refs, expand_flags);
        return;

    case RPN_RES_STRING:
        bitstr(p_ev_result->arg.string.uchars, 0, array, fwidth, expand_refs, expand_flags);
        /* NB bitstr() will return a fonty result if riscos_fonts */
        break;

    default:
        assert(0);
    case DATA_ID_BLANK:
        array[0] = CH_NULL;
        break;

    case RPN_RES_ARRAY:
        result_to_string_ARRAY(p_ev_result, docno, array_start, array, format, fwidth, justify, expand_refs, expand_flags);
        return;

    case DATA_ID_DATE:
        result_to_string_DATE(p_ev_result, docno, array_start, array, format, fwidth, justify, expand_refs, expand_flags);
        return;

    case DATA_ID_ERROR:
        result_to_string_ERROR(p_ev_result, docno, array_start, array, format, fwidth, justify, expand_refs, expand_flags);
        return;
    }
}

/******************************************************************************
*
* sprintnumber does a print of the number in tcell into array
* it prints lead chars, '-' | '(', number, ')', trail chars
* if eformat is FALSE it prints number in long form
* if eformat is TRUE  it prints number in e format
* it returns the number of non-space characters printed
* the format is picked up from the cell or from the option page
*
******************************************************************************/

#define currency_symbol_char '\xA4'

static const char *
get_subtitute_currency_string(void)
{
    _kernel_oserror * e;
    _kernel_swi_regs rs;
    rs.r[0] = -1; /* current territory */
    rs.r[1] =  4; /* read currency symbol in local alphabet */
    if(NULL != (e = _kernel_swi(Territory_ReadSymbols, &rs, &rs)))
        return(NULL);
    return((const char *) (intptr_t) rs.r[0]);
}

static void
lead_trail_substitute_currency(
    char * leading_buf,
    char * trailing_buf,
    _InVal_     U32 buffer_bytes)
{
    const char * substitute_str = NULL;
    char * b;
    char * p;

    /* only substitute in either leading or trailing buffer but not both */
    if(NULL == (p = strchr(b = leading_buf, currency_symbol_char)))
        if(NULL == (p = strchr(b = trailing_buf, currency_symbol_char)))
            return;

    /* p marks the spot in buffer b where a replacement should be plonked */
    if(NULL == (substitute_str = get_subtitute_currency_string()))
        return;

    U32 substitute_len = strlen32(substitute_str); /* without CH_NULL */
    U32 tail_len = strlen32p1(p + 1); /* probably one (CH_NULL), but there might be a space etc.*/
    U32 remaining_bytes = buffer_bytes - (p - b);
    /* if we can't fit the substitute string in after moving the tail up, give up */
    if(substitute_len + tail_len > remaining_bytes)
        return;
    /* move the tail up, including the CH_NULL terminator,
     * to make room for the substitute string
     * NB the currency_symbol_char is overwritten
     */
    memmove32(p + substitute_len, p + 1, tail_len);
    memmove32(p, substitute_str, substitute_len);
}

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
    BOOL negative = FALSE;
    BOOL brackets = FALSE;
    char *tptr;
    S32 len, width = 0, inc;
    char t_lead[16], t_trail[16];
    S32 padding, logval;
    GR_MILLIPOINT digitwid_mp = 1; /* keep clang happy */
    GR_MILLIPOINT dotwid_mp = 0, thswid_mp = 0, bracwid_mp = 0;

    nextprint = array;

    if(isless(value, 0.0))
    {
        value = -value;
        negative = TRUE;
    }

    decimals = (format & F_DCP)
                    ? (format & F_DCPSID)
                    : get_dec_field_from_opt();

    brackets = (format & F_DCP)
                    ? (format & F_BRAC)
                    : (d_options_MB == 'B');

    if(riscos_fonts)
    {
        digitwid_mp = font_charwid(current_font, '0');
        dotwid_mp   = font_charwid(current_font, '.');
        assert(0 != digitwid_mp);

        if(d_options_TH != TH_BLANK)
        {
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

        if(brackets)
            bracwid_mp = font_charwid(current_font, ')');
    }

    /* process leading and trailing characters */
    t_lead[0] = CH_NULL;
    if(format & F_LDS)
    {
        if(d_options_LP)
        {
            if(riscos_fonts)
                font_strip_strcpy(t_lead, d_options_LP);
            else
                strcpy(t_lead, d_options_LP);
        }
    }

    t_trail[0] = CH_NULL;
    if(format & F_TRS)
    {
        if(d_options_TP)
        {
            if(riscos_fonts)
                font_strip_strcpy(t_trail, d_options_TP);
            else
                strcpy(t_trail, d_options_TP);
        }
    }

    if( t_lead[0] | t_trail[0] ) /* that's a deliberate binary OR, there's no need to test independently */
        lead_trail_substitute_currency(t_lead, t_trail, elemof32(t_trail));

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

            logval = ((value == 0.0) ? 0 : (S32) log10(value));

            if(d_options_TH != TH_BLANK)
            {
                if(riscos_fonts)
                    padding += (logval / 3) * thswid_mp;
                else
                    padding += (logval / 3);
            }

            width = fwidth - padding;
            if(!riscos_fonts)
                width -= 1;

            if(value != floor(value))
            {
                /* if displaying fractional part, allow for . */
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
            if(value < 1.0)
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

            consume_int(sprintf(formatstr, "%%.%dg", width));
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

    *nextprint = CH_NULL;

    /* print number */
    tptr = nextprint;
    if(negative)
        *nextprint++ = brackets ? '(' : '-';

    len = sprintf(nextprint, formatstr, value);

    trace_2(TRACE_APP_PD4, "sprintnumber formatstr: %s, result: %s",
            report_sbstr(formatstr), report_sbstr(nextprint));

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
        while((ch = *tptr++) != CH_NULL)
        {
            if(ch == 'e')
            {
                if(*tptr == '+')
                {
                    memmove32(tptr, tptr + 1, strlen32(tptr));
                    nextprint--;
                }
                else if(*tptr == '-')
                    tptr++;

                while((*tptr == '0')  &&  (*(tptr+1) != CH_NULL))
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

    *nextprint = CH_NULL;

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

            font_insert_shift(RFM_X_SHIFT, xshift, &nextprint);
        }

        *nextprint = CH_NULL;
        width = font_width(array_start);
    }

    /* add spaces on end, but don't count them in length */
    if(!riscos_fonts)
        if(brackets && !negative)
        {
            *nextprint++ = FUNNYSPACE;
            *nextprint = CH_NULL;
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

        if(0 != (0x10 & current_fg_wimp_colour_value))
        {
            f.fore_colour = current_fg_wimp_colour_value & ~0x1F;
        }
        else
        {
            f.fore_colour = wimptx_RGB_for_wimpcolour(current_fg_wimp_colour_value & 0x0F);
        }

        if(0 != (0x10 & current_bg_wimp_colour_value))
        {
            f.back_colour = current_bg_wimp_colour_value & ~0x1F;
        }
        else
        {
            f.back_colour = wimptx_RGB_for_wimpcolour(current_bg_wimp_colour_value & 0x0F);
        }

        f.offset = 14;

#if 0
        if(log2bpp >= 3) /* 256 colour modes on old systems */
        {
            font fh;
            wimp_paletteword bg, fg;
            int f_offset;

            fh = slot_font;
            bg.word = wimptx_RGB_for_wimpcolour(current_bg_wimp_colour_value & 0x0F);
            fg.word = wimptx_RGB_for_wimpcolour(current_fg_wimp_colour_value & 0x0F);
            f_offset = 14;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) has RGB bg %8.8X, fg %8.8X", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &f_offset));
            f.fore_colour = fg.word;
            f.offset = f_offset;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) gets fg %d, offset %d", fg.word, f_offset);

            fh = slot_font;
            bg.word = wimptx_RGB_for_wimpcolour(current_fg_wimp_colour_value & 0x0F);
            fg.word = wimptx_RGB_for_wimpcolour(current_bg_wimp_colour_value & 0x0F);
            f_offset = 14;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) has RGB bg %8.8X, fg %8.8X", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &f_offset));
            f.back_colour = fg.word;
            trace_2(TRACE_APP_PD4, "expand_current_slot (256) gets bg %d, offset %d", fg.word, f_offset);
        }
        else /* old systems */
        {
            font_complain(font_current(&f));
            trace_3(TRACE_APP_PD4, "expand_current_slot gets bg %d, fg %d, offset %d", f.back_colour, f.fore_colour, f.offset);
        }
#endif
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

    *to = CH_NULL;
}

extern BOOL
ensure_global_font_valid(void)
{
    return(font_get_global() > 0);
}

/* end of slotconv.c */
