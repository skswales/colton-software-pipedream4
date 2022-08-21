/* gr_rdia2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS Draw file creation part II */

/* SKS September 1991 */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_flex_h
#include "cs-flex.h"
#endif

#ifndef __drawmod_h
#include "drawmod.h"
#endif

#ifndef GR_RISCDIAG_WACKYTAG
#define GR_RISCDIAG_WACKYTAG 0x23311881
#endif

/******************************************************************************
*
* start a path object
*
******************************************************************************/

static const DRAW_PATH_STYLE gr_riscdiag_path_style_default =
{
    (DRAW_PS_JOIN_MITRED      << DRAW_PS_JOIN_PACK_SHIFT    ) | /* NB. big mitres > width converted into bevels automagically */
    (DRAW_PS_CAP_BUTT         << DRAW_PS_ENDCAP_PACK_SHIFT  ) |
    (DRAW_PS_CAP_BUTT         << DRAW_PS_STARTCAP_PACK_SHIFT) |
    (DRAW_PS_WINDRULE_NONZERO << DRAW_PS_WINDRULE_PACK_SHIFT) |
    (DRAW_PS_DASH_ABSENT      << DRAW_PS_DASH_PACK_SHIFT    ) |
    0, /* flags */
    0, /* reserved */
    0, /* tricap_w */
    0, /* tricap_h */
};

/* keep consistent with &.CModules.GB.spr.gr_chart (sprites gr_ec_lstyp1..4, 0 is solid) */

static const struct LINESTYLE_RISCOS_DASH_2 { DRAW_DASH_HEADER hdr; S32 pattern[2]; }
gr_linestyle_riscos_dash =
{
    { gr_riscDraw_from_point(0) /* start distance */, 2 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(8) }
};

static const struct LINESTYLE_RISCOS_DASH_2
gr_linestyle_riscos_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 2 /* count */ },
    { gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const struct LINESTYLE_RISCOS_DASH_4 { DRAW_DASH_HEADER hdr; S32 pattern[4]; }
gr_linestyle_riscos_dash_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 4 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const struct LINESTYLE_RISCOS_DASH_6 { DRAW_DASH_HEADER hdr; S32 pattern[6]; }
gr_linestyle_riscos_dash_dot_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 6 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const PC_DRAW_DASH_HEADER
gr_linestyle_riscos_dashes[] =
{
    NULL, /* 0 (GR_LINE_PATTERN_STANDARD) -> solid line */
    &gr_linestyle_riscos_dash.hdr,
    &gr_linestyle_riscos_dot.hdr,
    &gr_linestyle_riscos_dash_dot.hdr,
    &gr_linestyle_riscos_dash_dot_dot.hdr
};

static PC_DRAW_DASH_HEADER
gr_linestyle_to_riscDraw(
    _InRef_opt_ PC_GR_LINESTYLE linestyle)
{
    U32 i;

    if(NULL == linestyle)
        return(NULL);

    i = (U32) linestyle->pattern;

    if(i >= elemof32(gr_linestyle_riscos_dashes))
        return(NULL); /* unknown handles -> solid line */

    return(gr_linestyle_riscos_dashes[i]);
}

_Check_return_
_Ret_writes_bytes_maybenull_(extraBytes)
extern P_ANY /* -> path guts */
gr_riscdiag_path_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPathStart,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLE fillstyle,
    _InRef_opt_ PC_DRAW_PATH_STYLE pathstyle,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status)
{
    PC_DRAW_DASH_HEADER dash_pattern = linestyle ? gr_linestyle_to_riscDraw(linestyle) : NULL;
    U32 nDashBytes = 0;
    DRAW_OBJECT_PATH path;
    P_DRAW_OBJECT pObject;

    if(NULL != dash_pattern)
        nDashBytes = sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * dash_pattern->dashcount;

    if(NULL == (pObject.p_byte = gr_riscdiag_object_new(p_gr_riscdiag, pPathStart, DRAW_OBJECT_TYPE_PATH, nDashBytes + extraBytes, p_status)))
        return(NULL);

    memcpy32(&path, pObject.p_byte, sizeof32(path));

    path.fillcolour = fillstyle ?       gr_colour_to_riscDraw(fillstyle->fg) : DRAW_COLOUR_Transparent;
    path.pathcolour = linestyle ?       gr_colour_to_riscDraw(linestyle->fg) : DRAW_COLOUR_Transparent;
    path.pathwidth  = linestyle ? (int) gr_riscDraw_from_pixit(linestyle->width) : 0;
    path.pathstyle  = pathstyle ? *pathstyle : gr_riscdiag_path_style_default;

    if(NULL != dash_pattern)
        path.pathstyle.flags |= DRAW_PS_DASH_PACK_MASK;

    memcpy32(pObject.p_byte, &path, sizeof32(path));

    if(NULL != dash_pattern)
        memcpy32(pObject.p_byte + sizeof32(path), dash_pattern, (size_t) nDashBytes);

    *p_status = STATUS_DONE;

    return(pObject.p_byte + sizeof32(path) + nDashBytes); /* -> path guts */
}

/******************************************************************************
*
* return the start of the path data in a path object structure
*
******************************************************************************/

extern P_ANY
gr_riscdiag_path_query_guts(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, pathStart);
    U32 extraBytes;

    myassert2x(((PC_DRAW_OBJECT_HEADER) pObject)->type == DRAW_OBJECT_TYPE_PATH, "gr_riscdiag_path_query_guts of a non-path object " U32_XTFMT " type %d", pathStart, ((PC_DRAW_OBJECT_HEADER) pObject));

    /* always skip path header */
    extraBytes = sizeof32(DRAW_OBJECT_PATH);

    if(((P_DRAW_OBJECT_PATH) pObject)->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
        /* and skip dash if present */
        extraBytes += sizeof32(DRAW_DASH_HEADER)
                    + sizeof32(S32) * (PtrAddBytes(PC_DRAW_DASH_HEADER, pObject, sizeof32(DRAW_OBJECT_PATH)))->dashcount;

    return(pObject + extraBytes);
}

/******************************************************************************
*
* recompute a path's bbox
*
******************************************************************************/

extern void
gr_riscdiag_path_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart)
{
    P_DRAW_OBJECT pObject;
    P_DRAW_DASH_HEADER line_dash_pattern = NULL;
    P_ANY path_seq;
    drawmod_pathelemptr bodge_path_seq;
    drawmod_options options;
    drawmod_line line;
    drawmod_filltype fill;
    DRAW_BOX path_box;

    /* compute bbox of path object by asking the Draw module */

    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, pathStart);

    /* path really starts here (unless dashed) */
    path_seq = pObject.path + 1;

    /* requesting stroke bbox */
    options.tag      = tag_box;
    options.data.box = (drawmod_box *) &path_box;     /* where to put bbox */

    if(pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
    {
        line_dash_pattern = (P_DRAW_DASH_HEADER) path_seq;

        path_seq = PtrAddBytes(P_U8, line_dash_pattern, sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * line_dash_pattern->dashcount);
    }

    bodge_path_seq.bytep = path_seq;

    /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
    line.flatness            = 2 * GR_RISCDRAW_PER_RISCOS;
    line.thickness           =  pObject.path->pathwidth;
    line.spec.join           = (pObject.path->pathstyle.flags & DRAW_PS_JOIN_PACK_MASK)     >> DRAW_PS_JOIN_PACK_SHIFT;
    line.spec.leadcap        = (pObject.path->pathstyle.flags & DRAW_PS_STARTCAP_PACK_MASK) >> DRAW_PS_STARTCAP_PACK_SHIFT;
    line.spec.trailcap       = (pObject.path->pathstyle.flags & DRAW_PS_ENDCAP_PACK_MASK)   >> DRAW_PS_ENDCAP_PACK_SHIFT;
    line.spec.reserved8      = 0;
    line.spec.mitrelimit     = 0x000A0000;     /* 10.0 "PostScript default" from DrawFiles doc'n */
    line.spec.lead_tricap_w  =
    line.spec.trail_tricap_w = (pObject.path->pathstyle.tricap_w * pObject.path->pathwidth) / 16;
    line.spec.lead_tricap_h  =
    line.spec.trail_tricap_h = (pObject.path->pathstyle.tricap_h * pObject.path->pathwidth) / 16;
    line.dash_pattern        = (P_ANY) line_dash_pattern;

    fill = (drawmod_filltype) (fill_PReflatten | fill_PThicken | fill_PFlatten | fill_FBint | fill_FNonbint | fill_FBext);

    (void) drawmod_processpath(bodge_path_seq, /* path sequence */
                               fill,           /* fill style */
                               NULL,           /* xform matrix */
                               &line,          /* line style */
                               &options,       /* process options */
                               NULL);          /* output buffer length for count */

    pObject.path->bbox = path_box;
}

/******************************************************************************
*
* line object
*
******************************************************************************/

struct GR_RISCDIAG_LINE_GUTS
{
    DRAW_PATH_MOVE  pos;    /* move to first point  */
    DRAW_PATH_LINE  second; /* line to second point */
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_line_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    struct GR_RISCDIAG_LINE_GUTS * pLine;
    STATUS status;

    if(NULL == (pLine = gr_riscdiag_path_new(p_gr_riscdiag, pObjectStart, linestyle, NULL, NULL, sizeof32(*pLine), &status)))
        return(status);

    pLine->pos.tag      = DRAW_PATH_TYPE_MOVE;
    pLine->pos.pt       = *pPos;

    pLine->second.tag   = DRAW_PATH_TYPE_LINE;
    pLine->second.pt.x  = pPos->x + pOffset->x;
    pLine->second.pt.y  = pPos->y + pOffset->y;

    pLine->term.tag     = DRAW_PATH_TYPE_TERM;

    return(1);
}

/******************************************************************************
*
* pie sector object
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_piesector_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    PC_DRAW_POINT pPos,
    DRAW_COORD radius,
    PC_F64 alpha,
    PC_F64 beta,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    DRAW_POINT bezCentre = *pPos;
    DRAW_COORD bezRadius = radius;
    DRAW_POINT bezStart, bezEnd, bezCP1, bezCP2;
    U32 n_segments, segment_id;
    STATUS status;
    P_BYTE pPathGuts;

    if(bezRadius <= 0)
        return(STATUS_OK);

    n_segments = bezier_arc_start(&bezCentre, bezRadius, alpha, beta, &bezStart, &bezEnd, &bezCP1, &bezCP2);

    /* NB. variable number of segments precludes fixed structure */
    if(NULL == (pPathGuts =
        gr_riscdiag_path_new(p_gr_riscdiag, pObjectStart,
                             linestyle, fillstyle, NULL,
                             sizeof32(DRAW_PATH_MOVE)  +
                             sizeof32(DRAW_PATH_LINE)  +
                             (sizeof32(DRAW_PATH_CURVE) * n_segments) +
                             sizeof32(DRAW_PATH_LINE)  +
                             sizeof32(DRAW_PATH_CLOSE) +
                             sizeof32(DRAW_PATH_TERM),
                             &status)))
        return(status);

    segment_id = 1;

    {
    DRAW_PATH_MOVE move;
    move.tag = DRAW_PATH_TYPE_MOVE;
    move.pt  = bezCentre;
    memcpy32(pPathGuts, &move, sizeof32(move));
    pPathGuts += sizeof32(move);
    } /*block*/

    {
    DRAW_PATH_LINE line;
    line.tag = DRAW_PATH_TYPE_LINE;
    line.pt  = bezStart;
    memcpy32(pPathGuts, &line, sizeof32(line));
    pPathGuts += sizeof32(line);
    } /*block*/

    /* accumulate curvy bits */
    do  {
        DRAW_PATH_CURVE curve;
        curve.tag = DRAW_PATH_TYPE_CURVE;
        curve.cp1 = bezCP1;
        curve.cp2 = bezCP2;
        curve.end = bezEnd;
        memcpy32(pPathGuts, &curve, sizeof32(curve));
        pPathGuts += sizeof32(curve);
    }
    while((segment_id = bezier_arc_segment(segment_id, &bezEnd, &bezCP1, &bezCP2)) != 0);

    {
    DRAW_PATH_LINE line;
    line.tag = DRAW_PATH_TYPE_LINE;
    line.pt  = bezCentre;
    memcpy32(pPathGuts, &line, sizeof32(line));
    pPathGuts += sizeof32(line);
    } /*block*/

    * (P_U32) pPathGuts = DRAW_PATH_TYPE_CLOSE_WITH_LINE; /* ok to poke single words */
    pPathGuts += sizeof32(DRAW_PATH_CLOSE);

    * (P_U32) pPathGuts = DRAW_PATH_TYPE_TERM;

    return(STATUS_OK);
}

/******************************************************************************
*
* quadrilateral object
*
******************************************************************************/

struct GR_RISCDIAG_QUADRILATERAL_GUTS
{
    DRAW_PATH_MOVE  pos;    /* move to bottom left  */
    DRAW_PATH_LINE  second; /* line to bottom right */
    DRAW_PATH_LINE  third;  /* line to top right    */
    DRAW_PATH_LINE  fourth; /* line to top left    */
    DRAW_PATH_CLOSE close;
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_quadrilateral_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset1,
    _InRef_     PC_DRAW_POINT pOffset2,
    _InRef_     PC_DRAW_POINT pOffset3,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    DRAW_PATH_STYLE pathstyle;
    struct GR_RISCDIAG_QUADRILATERAL_GUTS * pQuad;
    STATUS status;

    zero_struct(pathstyle);

    pathstyle.flags |= DRAW_PS_JOIN_ROUND;

    if(NULL == (pQuad = gr_riscdiag_path_new(p_gr_riscdiag, pObjectStart, linestyle, fillstyle, &pathstyle, sizeof(*pQuad), &status)))
        return(status);

    pQuad->pos.tag      = DRAW_PATH_TYPE_MOVE;
    pQuad->pos.pt       = *pPos;

    pQuad->second.tag   = DRAW_PATH_TYPE_LINE;
    pQuad->second.pt.x  = pPos->x + pOffset1->x;
    pQuad->second.pt.y  = pPos->y + pOffset1->y;

    pQuad->third.tag    = DRAW_PATH_TYPE_LINE;
    pQuad->third.pt.x   = pPos->x + pOffset2->x;
    pQuad->third.pt.y   = pPos->y + pOffset2->y;

    pQuad->fourth.tag   = DRAW_PATH_TYPE_LINE;
    pQuad->fourth.pt.x  = pPos->x + pOffset3->x;
    pQuad->fourth.pt.y  = pPos->y + pOffset3->y;

    pQuad->close.tag = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    pQuad->term.tag  = DRAW_PATH_TYPE_TERM;

    return(1);
}

/******************************************************************************
*
* rectangle object
*
******************************************************************************/

struct GR_RISCDIAG_RECTANGLE_GUTS
{
    DRAW_PATH_MOVE  bl;  /* move to bottom left  */
    DRAW_PATH_LINE  br;  /* line to bottom right */
    DRAW_PATH_LINE  tr;  /* line to top left     */
    DRAW_PATH_LINE  tl;  /* line to top right    */
    DRAW_PATH_CLOSE close;
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_rectangle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    _InRef_     PC_DRAW_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLE fillstyle)
{
    struct GR_RISCDIAG_RECTANGLE_GUTS * pRect;
    STATUS status;

    if(NULL == (pRect = gr_riscdiag_path_new(p_gr_riscdiag, pObjectStart, linestyle, fillstyle, NULL, sizeof(*pRect), &status)))
        return(status);

    pRect->bl.tag    = DRAW_PATH_TYPE_MOVE;
    pRect->bl.pt.x   = pBox->x0;
    pRect->bl.pt.y   = pBox->y0;

    pRect->br.tag    = DRAW_PATH_TYPE_LINE;
    pRect->br.pt.x   = pBox->x1;
    pRect->br.pt.y   = pBox->y0;

    pRect->tr.tag    = DRAW_PATH_TYPE_LINE;
    pRect->tr.pt.x   = pBox->x1;
    pRect->tr.pt.y   = pBox->y1;

    pRect->tl.tag    = DRAW_PATH_TYPE_LINE;
    pRect->tl.pt.x   = pBox->x0;
    pRect->tl.pt.y   = pBox->y1;

    pRect->close.tag = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    pRect->term.tag  = DRAW_PATH_TYPE_TERM;

    return(1);
}

extern void
gr_riscdiag_sprite_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET spriteStart)
{
    P_DRAW_OBJECT_SPRITE pObject;

    pObject = gr_riscdiag_getoffptr(DRAW_OBJECT_SPRITE, p_gr_riscdiag, spriteStart);

    /* only recalculate (and therefore set default size) if deliberately made bad */
    if(pObject->bbox.x0 >= pObject->bbox.x1)
    {
        PC_BYTE p_sprite = PtrAddBytes(PC_BYTE, pObject, sizeof32(DRAW_OBJECT_HEADER));
        PC_SCB p_scb = (PC_SCB) p_sprite;
        S32 x1, y1;
        SPRITE_MODE_WORD sprite_mode_word;
        _kernel_swi_regs rs;

        sprite_mode_word.u.u32 = (U32) p_scb->mode;

        rs.r[0] = 40 + 512;
        rs.r[1] = (int) 0x89ABFEDC; /* kill the OS or any twerp who dares to access this! */
        rs.r[2] = (int) p_sprite;

        (void) _kernel_swi(/*OS_SpriteOp*/ 0x2E, &rs, &rs);

        if((sprite_mode_word.u.u32 < 256U) || (0 == (sprite_mode_word.u.riscos_3_5.mode_word_bit)))
        {   /* Mode Number or Mode Specifier */
            S32 XEigFactor, YEigFactor;

            XEigFactor = bbc_modevar(p_scb->mode, 4/*XEigFactor*/);
            YEigFactor = bbc_modevar(p_scb->mode, 5/*YEigFactor*/);

            x1 = (rs.r[3] << XEigFactor) * GR_RISCDRAW_PER_RISCOS;
            y1 = (rs.r[4] << YEigFactor) * GR_RISCDRAW_PER_RISCOS;
        }
        else if(SPRITE_TYPE_RO5_WORD == sprite_mode_word.u.riscos_3_5.type)
        {   /* RISC OS 5 style Mode Word */
            x1 = (rs.r[3] * GR_RISCDRAW_PER_INCH) / (180 >> sprite_mode_word.u.riscos_5.x_eig);
            y1 = (rs.r[4] * GR_RISCDRAW_PER_INCH) / (180 >> sprite_mode_word.u.riscos_5.y_eig);
        }
        else
        {   /* RISC OS 3.5 style Mode Word */
            x1 = (rs.r[3] * GR_RISCDRAW_PER_INCH) / sprite_mode_word.u.riscos_3_5.h_dpi;
            y1 = (rs.r[4] * GR_RISCDRAW_PER_INCH) / sprite_mode_word.u.riscos_3_5.v_dpi;
        }

        /* assumes bbox.x0, bbox.y0 correct */
        pObject->bbox.x1 = pObject->bbox.x0 + x1;
        pObject->bbox.y1 = pObject->bbox.y0 + y1;
    }
}

/******************************************************************************
*
* add a fonty string to the file
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_string_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    PC_DRAW_POINT    point,
    PC_U8Z           szText,
    PC_U8Z           szFontName,
    GR_COORD         fsizex,
    GR_COORD         fsizey,
    GR_COLOUR        fg,
    GR_COLOUR        bg)
{
    U32 size;
    P_DRAW_OBJECT pObject;
    DRAW_OBJECT_TEXT * pObjectText;
    DRAW_FONT_REF16 fontRef;
    S32 fsizey_mp;
    STATUS status;
    DRAW_DIAG_OFFSET rectStart;

    size = strlen32p1(szText); /* includes CH_NULL*/

    /* round up to output word boundary */
    size = round_up(size, 4);

    if(bg.visible)
    {
        DRAW_BOX dummy_box;
        GR_FILLSTYLE fillstyle;

        dummy_box.x0 = point->x;
        dummy_box.y0 = point->y;
        dummy_box.x1 = point->x + 1;
        dummy_box.y1 = point->y + 1;

        fillstyle.fg = bg;

        status_return(status = gr_riscdiag_rectangle_new(p_gr_riscdiag, &rectStart, &dummy_box, NULL, &fillstyle));
    }

    if(NULL == (pObject.p_byte = gr_riscdiag_object_new(p_gr_riscdiag, pObjectStart, DRAW_OBJECT_TYPE_TEXT, size, &status)))
        return(status);

    pObjectText = (DRAW_OBJECT_TEXT *) pObject.hdr;

    pObjectText->textcolour = gr_colour_to_riscDraw(fg);

    if(bg.visible)
        pObjectText->background = gr_colour_to_riscDraw(bg); /* hint colour */
    else
        pObjectText->background = (int) 0xFFFFFF00; /* hint is white if unspecified */

    fontRef = gr_riscdiag_fontlist_lookup(p_gr_riscdiag, p_gr_riscdiag->dd_fontListR, szFontName);

    /* blat reserved fields in this 32-bit structure */
    * (P_U32) &pObjectText->textstyle = (U32) fontRef;

    /* NB. NOT DRAW UNITS!!! --- 1/640 point */
    pObjectText->fsize_x = (int) draw_fontsize_from_mp(gr_mp_from_pixit(fsizex));
    fsizey_mp            = (int) gr_mp_from_pixit(fsizey);
    pObjectText->fsize_y = (int) draw_fontsize_from_mp(fsizey_mp);

    /* baseline origin coords */
    pObjectText->coord.x = point->x;
    pObjectText->coord.y = point->y;

    if(fontRef == 0)
        /* Draw rendering positions system font strangely: quickly correct baseline */
        pObjectText->coord.y -= (int) (gr_riscDraw_from_pixit(fsizey) / 8);

    /* now poke the variable part */
    strcpy((P_USTR) (pObjectText + 1), szText);

    if(bg.visible)
    {
        /* poke background box path */
        struct GR_RISCDIAG_RECTANGLE_GUTS * pRect;
        S32 bot_y, top_y;

        gr_riscdiag_string_recompute_bbox(p_gr_riscdiag, *pObjectStart);

        pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, *pObjectStart);

        pRect = gr_riscdiag_path_query_guts(p_gr_riscdiag, rectStart);

        bot_y = pObjectText->coord.y - (fsizey_mp * 1 * GR_RISCDRAW_PER_POINT) / (GR_MILLIPOINTS_PER_POINT * 4);
        bot_y = MIN(bot_y, pObjectText->bbox.y0);

        top_y = pObjectText->coord.y + (fsizey_mp * 3 * GR_RISCDRAW_PER_POINT) / (GR_MILLIPOINTS_PER_POINT * 4);
        top_y = MAX(top_y, pObjectText->bbox.y1);

        pRect->bl.pt.x = pObjectText->bbox.x0;
        pRect->bl.pt.y = bot_y;

        pRect->br.pt.x = pObjectText->bbox.x1;
        pRect->br.pt.y = bot_y;

        pRect->tr.pt.x = pObjectText->bbox.x1;
        pRect->tr.pt.y = top_y;

        pRect->tl.pt.x = pObjectText->bbox.x0;
        pRect->tl.pt.y = top_y;
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* recompute a jpeg object's bbox
*
******************************************************************************/

extern void
gr_riscdiag_jpeg_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET jpegStart)
{
    P_DRAW_OBJECT_JPEG pJpegObject = gr_riscdiag_getoffptr(DRAW_OBJECT_JPEG, p_gr_riscdiag, jpegStart);

    /* only recalculate (and therefore set default size) if deliberately made bad */

    if(pJpegObject->bbox.x0 >= pJpegObject->bbox.x1)
    {
        S32 x1, y1;

        x1 = pJpegObject->width;  //muldiv64(pJpegObject->width,  GR_RISCDRAW_PER_INCH, pJpegObject->dpi_x);
        y1 = pJpegObject->height; //muldiv64(pJpegObject->height, GR_RISCDRAW_PER_INCH, pJpegObject->dpi_y);

        /* assumes bbox.x0, bbox.y0 correct */
        pJpegObject->bbox.x1 = pJpegObject->bbox.x0 + x1;
        pJpegObject->bbox.y1 = pJpegObject->bbox.y0 + y1;
    }
}

extern void
gr_riscdiag_string_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET textStart)
{
    DRAW_OBJECT_TEXT * pObjectText;
    font f = 0;
    S32 fsizex16, fsizey16; /* in 16ths of a point */
    PC_U8Z szHostFontName;

    pObjectText = gr_riscdiag_getoffptr(DRAW_OBJECT_TEXT, p_gr_riscdiag, textStart);

    szHostFontName = gr_riscdiag_fontlist_name(p_gr_riscdiag, GR_RISCDIAG_OBJECT_NONE, pObjectText->textstyle.fontref16);

    /* structure font sizes stored in 1/640th point; FontManager requires 1/16 point */
    fsizex16 = (pObjectText->fsize_x * 16) / 640;
    fsizey16 = (pObjectText->fsize_y * 16) / 640;

    if(szHostFontName)
        if(font_find((char *) szHostFontName, fsizex16, fsizey16, 0, 0, &f))
            f = 0;

    if(f)
    {
        if(NULL == font_SetFont(f))
            (void) font_stringbbox((P_USTR) (pObjectText + 1),
                                   (font_info *) &pObjectText->bbox); /* NB. in mp */

        font_LoseFont(f);

        /* scale bbox to riscDraw units (**imprecise**) */
        pObjectText->bbox.x0 = (int) gr_round_pixit_to_floor((GR_COORD) pObjectText->bbox.x0 * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        pObjectText->bbox.x1 = (int) gr_round_pixit_to_ceil( (GR_COORD) pObjectText->bbox.x1 * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        pObjectText->bbox.y0 = (int) gr_round_pixit_to_floor((GR_COORD) pObjectText->bbox.y0 * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        pObjectText->bbox.y1 = (int) gr_round_pixit_to_ceil( (GR_COORD) pObjectText->bbox.y1 * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
    }
    else
    {
        /* have a stab at VDU 5 System font bboxing
         * (I'd wondered why 1/640th point; its GR_POINTS_PER_RISCDRAW!)
         * note that Draw expects standard System font to be 12.80pt x 6.40pt
        */
        pObjectText->bbox.x0 = 0;
        pObjectText->bbox.x1 = pObjectText->fsize_x * strlen((PC_USTR) (pObjectText + 1));
        pObjectText->bbox.y0 = 0;
        pObjectText->bbox.y1 = pObjectText->fsize_y;
    }

    /* move box to have its origin at string baseline origin */
    draw_box_translate(&pObjectText->bbox, NULL, &pObjectText->coord);
}

extern void
gr_riscdiag_diagram_tagged_object_strip(
    P_GR_RISCDIAG p_gr_riscdiag,
    gr_riscdiag_tagstrip_proc proc,
    P_ANY handle)
{
    DRAW_DIAG_OFFSET thisObject, endObject, wastagObject;
    P_DRAW_OBJECT pObject;
    U32 objectSize;
    U32 hdrandgoopSize;

    /* loop till all the little buggers removed */
    for(wastagObject = GR_RISCDIAG_OBJECT_NONE, hdrandgoopSize = 0;;)
    {
        thisObject = GR_RISCDIAG_OBJECT_FIRST;
        endObject  = GR_RISCDIAG_OBJECT_LAST; /* keeps getting closer */

        if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
        {
            do  {
                if(wastagObject != GR_RISCDIAG_OBJECT_NONE)
                {
                    /* remove test for removed goop when past where the header was */
                    if(wastagObject <= thisObject)
                        wastagObject = GR_RISCDIAG_OBJECT_NONE;
                    else if(pObject.hdr->type == DRAW_OBJECT_TYPE_GROUP)
                    {
                        objectSize = pObject.hdr->size;

                        /* did this group enclose the tag hdr and goop we removed? */
                        if(thisObject + objectSize > wastagObject)
                            pObject.hdr->size -= (int) hdrandgoopSize;
                    }
                }

                if(pObject.hdr->type == DRAW_OBJECT_TYPE_TAG)
                {
                    P_DRAW_OBJECT pEnclObject;
                    U32 tagHdrSize, enclObjectSize, tagGoopSize;
                    S32 PRM_conformant;
                    U32 tag;

                    wastagObject   = thisObject;

                    /* limit tagged object size to end of diagram for debungling chart reloading */
                    if(pObject.hdr->size < (U32) (p_gr_riscdiag->draw_diag.length - thisObject))
                        objectSize = pObject.hdr->size;
                    else
                        objectSize = (p_gr_riscdiag->draw_diag.length - thisObject);

                    /* test for PRM or Draw conformant file - is it a tag or a kosher object here? */
                    tag            = ((const DRAW_OBJECT_TAG *) pObject.p_byte)->intTag;

                    PRM_conformant = (tag > 0x0C); /* as of 28jan92 */

                    tagHdrSize = sizeof(DRAW_OBJECT_TAG);
                    if(!PRM_conformant)
                        tagHdrSize -= 4;

                    pEnclObject.p_byte = pObject.p_byte + tagHdrSize;
                    enclObjectSize    = pEnclObject.hdr->size;

                    if(!PRM_conformant)
                        tag = * (P_U32) (pEnclObject.p_byte + enclObjectSize);

                    tagGoopSize       = objectSize - enclObjectSize - tagHdrSize;

                    if(proc)
                    {
                        GR_RISCDIAG_TAGSTRIP_INFO info;

                        info.ppDiag         = &p_gr_riscdiag->draw_diag.data;
                        info.tag            = tag;
                        info.PRM_conformant = PRM_conformant;
                        info.thisObject     = thisObject;
                        info.goopOffset     = thisObject + tagHdrSize + enclObjectSize;
                        info.goopSize       = tagGoopSize;

                        if(!PRM_conformant)
                        {
                            /* skip first word for Draw conformant tagged objects */
                            info.goopOffset += 4;
                            info.goopSize   -= 4;
                        }

                        (* proc) (handle, &info);
                    }

                    /* remove the tag goop by copying the rest of the diagram down over it */
                    /* note stupid way midextend works */
                    if(tagGoopSize)
                        flex_midextend(&p_gr_riscdiag->draw_diag.data,
                                       (int) (thisObject + tagHdrSize + enclObjectSize + tagGoopSize),
                                       - (int) tagGoopSize);

                    /* remove the tag header by copying the enclosed object over it */
                    flex_midextend(&p_gr_riscdiag->draw_diag.data,
                                   (int) (thisObject + tagHdrSize),
                                   - (int) tagHdrSize);

                    hdrandgoopSize    = tagHdrSize + tagGoopSize;

                    assert(p_gr_riscdiag->dd_allocsize == p_gr_riscdiag->draw_diag.length);
                    p_gr_riscdiag->draw_diag.length -= hdrandgoopSize;
                    p_gr_riscdiag->dd_allocsize = p_gr_riscdiag->draw_diag.length;

                    /* extreme paranoia updrefs */
                    if(p_gr_riscdiag->dd_fontListR > thisObject)
                        p_gr_riscdiag->dd_fontListR -= hdrandgoopSize;

                    if(p_gr_riscdiag->dd_rootGroupStart > thisObject)
                        p_gr_riscdiag->dd_rootGroupStart -= hdrandgoopSize;

                    break;
                }
            }
            while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
        }

        if(thisObject != GR_RISCDIAG_OBJECT_FIRST)
            /* continue, resetting end offset first */
            continue;

        /* done the lot */
        break;
    }
}

/******************************************************************************
*
* put a scaled copy of the contents of a diagram in as a group in the Draw file.
* aren't font list objects a pain in the ass? Also note that text column objects
* and tagged objects can NOT be scaled at this point
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_scaled_diagram_add(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    PC_DRAW_BOX pBox,
    P_DRAW_DIAG diag,
    PC_GR_FILLSTYLE  fillstyle)
{
    GR_RISCDIAG source_gr_riscdiag;
    STATUS status;
    U32 diagLength;
    DRAW_DIAG_OFFSET diagStart, groupStart;
    DRAW_DIAG_OFFSET thisObject, endObject;
    P_DRAW_OBJECT pObject;
    P_DRAW_FILE_HEADER pDiagHdr;
    DRAW_POINT initposn, initsize, posn, size;
    GR_SCALE_PAIR simple_scale;
    GR_SCALE simple_scaling;
    GR_XFORMMATRIX scale_xform;
    DRAW_BOX group_box;
    GR_RISCDIAG_PROCESS_T process;
    S32 isotropic = !fillstyle ||  fillstyle->bits.isotropic;
    S32 recolour  = !fillstyle || !fillstyle->bits.norecolour;

    trace_8(TRACE_MODULE_GR_CHART, "gr_riscdiag_scaled_diagram_add(&%p, (%d,%d,%d,%d), (&%p,%d), iso=%d)",
            report_ptr_cast(p_gr_riscdiag),
            pBox->x0, pBox->y0, pBox->x1, pBox->y1,
            report_ptr_cast(diag->data), diag->length,
            isotropic);

    if(!diag)
        return(create_error(GR_CHART_ERR_INVALID_DRAWFILE));

    diagLength = diag->length;
    if(diagLength <= sizeof(DRAW_FILE_HEADER))
        return(create_error(GR_CHART_ERR_INVALID_DRAWFILE));
    diagLength -= sizeof(DRAW_FILE_HEADER);

    if((status = gr_riscdiag_group_new(p_gr_riscdiag, &groupStart, NULL)) < 0)
        return(status);

    /* note offset of where diagram body (skip file header and font table) is to be copied */
    diagStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    /* scan the diagram to be copied for a font table */

    /* steal source diagram away from its rightful owner */
    gr_riscdiag_diagram_setup_from_draw_diag(&source_gr_riscdiag, diag);

#if 0
    { /* never put the file header, font lists or RISC OS 3 crap object in our diagram */
    U32 total_skip_size;
    P_U8 pDiagCopy = NULL; /* keep dataflower happy */
    UINT pass;

    total_skip_size = sizeof32(DRAW_FILE_HEADER);

    for(pass = 1; pass <= 2; ++pass)
    {
        DRAW_DIAG_OFFSET sttObject = gr_riscdiag_normalise_stt(&source_gr_riscdiag, DRAW_DIAG_OFFSET_FIRST);
        DRAW_DIAG_OFFSET endObject = gr_riscdiag_normalise_end(&source_gr_riscdiag, DRAW_DIAG_OFFSET_LAST);
        DRAW_DIAG_OFFSET s = sttObject; /* after normalisation */

        if(gr_riscdiag_object_first(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE)) /* flat scan good enough for what I want */
        {
            do {
                switch(*DRAW_OBJHDR(U32, pObject.p_byte, type))
                {
                /* these skipped objects can't be grouped (which they never are) otherwise the group header(s) would need patching too! */
                case DRAW_OBJECT_TYPE_FONTLIST:
                    source_gr_riscdiag.dd_fontListR = sttObject;
reportf(TEXT("sda: ") PTR_XTFMT TEXT(" fontListR at %d"), &source_gr_riscdiag, sttObject);
                    goto skip_object;

                case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                    source_gr_riscdiag.dd_fontListW = sttObject;
reportf(TEXT("sda: ") PTR_XTFMT TEXT(" fontListW at %d"), &source_gr_riscdiag, sttObject);
                    goto skip_object;

                case DRAW_OBJECT_TYPE_OPTIONS:
                    source_gr_riscdiag.dd_options = sttObject;
reportf(TEXT("sda: ") PTR_XTFMT TEXT(" options at %d"), &source_gr_riscdiag, sttObject);
                skip_object:;
                    if(pass == 1)
                        total_skip_size += *DRAW_OBJHDR(U32, pObject.p_byte, size);
                    else
                    {
                        /* flush out whatever we had so far */
                        U32 n_bytes = sttObject - s;
                        if(n_bytes)
                        {
                            memcpy32(pDiagCopy, gr_riscdiag_getoffptr(BYTE, &source_gr_riscdiag, s), n_bytes);
                            pDiagCopy += n_bytes;
                        }
                        s = sttObject + *DRAW_OBJHDR(U32, pObject.p_byte, size); /* next object to be output starts here */
                    }
                    break;

                default:
reportf(TEXT("sda: ") PTR_XTFMT TEXT(" %d at %d"), &source_gr_riscdiag, *DRAW_OBJHDR(U32, pObject.p_byte, type), sttObject);
                    break;
                }
            }
            while(gr_riscdiag_object_next(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE));

            if(pass == 1)
            {
                U32 n_bytes = diagLength - total_skip_size;
                if(0 == n_bytes)
                    break;
                if(NULL == (pDiagCopy = gr_riscdiag_ensure(U8, p_gr_riscdiag, n_bytes, &status)))
                    return(status);
            }
            else
            {
                /* flush out end objects */
                U32 n_bytes = diagLength - s;
                if(n_bytes)
                    memcpy32(pDiagCopy, gr_riscdiag_getoffptr(BYTE, &source_gr_riscdiag, s), n_bytes);
            }
        }
    }
    } /*block*/
#else
    {
    P_U8 pDiagCopy = NULL; /* keep dataflower happy */

    if(gr_riscdiag_fontlist_scan(&source_gr_riscdiag, GR_RISCDIAG_OBJECT_FIRST, GR_RISCDIAG_OBJECT_LAST))
    {
        /* never put the font list in our diagram */
        P_DRAW_OBJECT_FONTLIST pFontList;
        U32 fontListSize;
        U32 nBytesBefore, nBytesAfter;

        pFontList = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, &source_gr_riscdiag, source_gr_riscdiag.dd_fontListR);

        fontListSize = pFontList->size;
        diagLength  -= fontListSize;

        if(NULL == (pDiagCopy = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, diagLength, &status)))
            return(status);

        /* must reload after that realloc */
        pObject.p_byte = gr_riscdiag_getoffptr(BYTE, &source_gr_riscdiag, sizeof(DRAW_FILE_HEADER)); /* first bit to copy from */
        pFontList = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, &source_gr_riscdiag, source_gr_riscdiag.dd_fontListR);

        nBytesBefore = source_gr_riscdiag.dd_fontListR - sizeof(DRAW_FILE_HEADER);
        nBytesAfter  = source_gr_riscdiag.draw_diag.length - (source_gr_riscdiag.dd_fontListR + fontListSize);
        /* two part copy needed - if font list grouped then we won't have found it (which
         * is a good job otherwise the group header(s) would have needed patching too!
        */

        /* first bit starts after diagram header and ends at start of font list */
        memcpy32(pDiagCopy, pObject.p_byte, nBytesBefore);

        /* next bits starts after font list and ends at end of diagram */
        memcpy32(pDiagCopy + nBytesBefore,
                      PtrAddBytes(PC_BYTE, pFontList, fontListSize),
                      nBytesAfter);
    }
    else
    {
        if(NULL == (pDiagCopy = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, diagLength, &status)))
            return(status);

        memcpy32(pDiagCopy,
                      PtrAddBytes(PC_BYTE, source_gr_riscdiag.draw_diag.data, sizeof32(DRAW_FILE_HEADER)),
                      diagLength);
    }
    } /*block*/

#endif

    trace_2(TRACE_MODULE_GR_CHART, "gr_riscdiag_scaled_diagram_add: diag copy start %d end %d",
            diagStart, p_gr_riscdiag->draw_diag.length);

    trace_0(TRACE_MODULE_GR_CHART, "gr_riscdiag_scaled_diagram_add: convert font references in copied diagram to this diagram's font references");

    /* note that we can't cope here with the main diagram's font table growing as
     * that'd bugger up all the sys_offs we've placed in the diagram! so if it uses
     * a font that ain't on our system then they've had it. so there.
    */
    thisObject = diagStart; /* may as well miss out the group header! */

    endObject = diagStart + diagLength;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
    {
        do  {
            switch(pObject.hdr->type)
            {
            case DRAW_OBJECT_TYPE_TEXT:
                {
                const DRAW_OBJECT_TEXT * pObjectText = (const DRAW_OBJECT_TEXT *) pObject.p_byte;
                DRAW_FONT_REF16 old_fontref, new_fontref;
                PC_U8 old_fontname;
                old_fontref  = pObjectText->textstyle.fontref16;
                old_fontname = gr_riscdiag_fontlist_name(&source_gr_riscdiag, source_gr_riscdiag.dd_fontListR, old_fontref);
                new_fontref  = gr_riscdiag_fontlist_lookup(p_gr_riscdiag, p_gr_riscdiag->dd_fontListR, old_fontname);
                * (P_U32) &pObjectText->textstyle = new_fontref;
                trace_3(TRACE_MODULE_GR_CHART, "converted copy ref %d to font %s to main ref %d",
                        old_fontref, old_fontname, new_fontref);
                }
                break;

            default:
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
    }

    /* now can give source diagram back to its rightful owner */
    draw_diag_give_away(diag, &source_gr_riscdiag.draw_diag);

    /* can now close the group */
    gr_riscdiag_group_end(p_gr_riscdiag, &groupStart);

    /* run over all appropriate objects shifting and scaling */
    pDiagHdr = diag->data;

    initposn.x = pDiagHdr->bbox.x0;
    initposn.y = pDiagHdr->bbox.y0;

    initsize.x = pDiagHdr->bbox.x1 - initposn.x;
    initsize.y = pDiagHdr->bbox.y1 - initposn.y;

    posn.x = pBox->x0;
    posn.y = pBox->y0;

    size.x = pBox->x1 - posn.x;
    size.y = pBox->y1 - posn.y;

    if(isotropic)
    {
        /* make box square and reposition to centre */
        if(size.x > size.y)
        {
            posn.x += (size.x - size.y) / 2;
            size.x = size.y;
        }
        else if(size.x < size.y)
        {
            posn.x += (size.y - size.x) / 2;
            size.y = size.x;
        }
    }

    simple_scale.x = gr_scale_from_s32_pair(size.x, initsize.x);
    simple_scale.y = gr_scale_from_s32_pair(size.y, initsize.y);

    /* simple_scaling is (for want of a better value) the geometric mean of x and y scales */
    if(simple_scale.x == simple_scale.y)
        simple_scaling = simple_scale.x;
    else
        simple_scaling = gr_scale_from_f64(sqrt(gr_f64_from_scale(simple_scale.x) *
                                                gr_f64_from_scale(simple_scale.y)));

    /* now I have sufficient confidence, I do this as a comination xform: */
    /* i)   translation to get old origin at (0,0) */
    /* ii)  scaling about (0,0) */
    /* iii) translation to new origin */
    {
    GR_XFORMMATRIX temp_xform;

    /* i)   translation to get old origin at (0,0) */
    initposn.x = -initposn.x;
    initposn.y = -initposn.y;
    gr_xform_make_translation(&temp_xform, (P_GR_POINT) &initposn);
    initposn.x = -initposn.x;
    initposn.y = -initposn.y;

    /* ii)  scaling about (0,0) */
    gr_xform_make_scale(&scale_xform, NULL, simple_scale.x, simple_scale.y);

    gr_xform_make_combination(&scale_xform, &temp_xform, &scale_xform);

    /* iii) translation to new origin */
    gr_xform_make_translation(&temp_xform, (P_GR_POINT) &posn);

    gr_xform_make_combination(&scale_xform, &scale_xform, &temp_xform);
    }

    thisObject = diagStart; /* may as well miss out the group header again! */
    endObject  = diagStart + diagLength;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
    {
        do  {
            /* note that there is no awkward font table object now !*/
            switch(pObject.hdr->type)
            {
            case DRAW_OBJECT_TYPE_TEXT:
                {
                DRAW_OBJECT_TEXT * pObjectText = (DRAW_OBJECT_TEXT *) pObject.hdr;

                /* shift and scale and shift baseline origin */
                draw_point_xform(&pObjectText->coord, &pObjectText->coord, &scale_xform);

                /* can scale both width and height */
                pObjectText->fsize_x = gr_coord_scale(pObjectText->fsize_x, simple_scale.x);
                pObjectText->fsize_y = gr_coord_scale(pObjectText->fsize_y, simple_scale.y);

                break;
                }

            case DRAW_OBJECT_TYPE_PATH:
                {
                P_BYTE p_path;

                /* simply scale the line width if not Thin */
                if(pObject.path->pathwidth != 0)
                    pObject.path->pathwidth = gr_coord_scale(pObject.path->pathwidth, simple_scaling);

                if(recolour)
                {
                    DRAW_COLOUR colour;

                    recolour = 0;

                    assert(fillstyle);
                    colour = gr_colour_to_riscDraw(fillstyle->fg);

                    /* set both stroke and fill */
                    pObject.path->fillcolour = colour;
                    pObject.path->pathcolour = colour;
                }

                /* path ordinarily starts here */
                p_path = (char *) (pObject.path + 1);

                /* simply scale the dash lengths and start offset if present */
                if(pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
                {
                    const S32 dashcount = ((PC_DRAW_DASH_HEADER) p_path)->dashcount;
                    S32 i;
                    P_S32 p_s32;

                    p_s32 = (P_S32) &((P_DRAW_DASH_HEADER) p_path)->dashstart;
                    *p_s32 = gr_coord_scale(*p_s32, simple_scaling);

                    for(i = 0; i < dashcount; ++i)
                    {
                        p_s32 = PtrAddBytes(P_S32, p_path, sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * i);
                        *p_s32 = gr_coord_scale(*p_s32, simple_scaling);
                    }

                    /* skip dash header and pattern */
                    p_path += sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * dashcount;
                }

                /* shift and scale the path coordinates */

                while(* (P_U32) p_path != DRAW_PATH_TYPE_TERM)
                    switch(* (P_U32) p_path)
                    {
                    case DRAW_PATH_TYPE_MOVE:
                    case DRAW_PATH_TYPE_LINE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_LINE line;

                        memcpy32(&line, p_path, sizeof32(line));

                        draw_point_xform(&line.pt, &line.pt, &scale_xform);

                        memcpy32(p_path, &line, sizeof32(line));

                        p_path += sizeof32(line);
                        break;
                        }

                    case DRAW_PATH_TYPE_CURVE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_CURVE curve;

                        memcpy32(&curve, p_path, sizeof32(curve));

                        draw_point_xform(&curve.cp1, &curve.cp1, &scale_xform);
                        draw_point_xform(&curve.cp2, &curve.cp2, &scale_xform);
                        draw_point_xform(&curve.end, &curve.end, &scale_xform);

                        memcpy32(p_path, &curve, sizeof32(curve));

                        p_path += sizeof32(curve);
                        break;
                        }

                    default:
#if CHECKING
                        myassert1x(0, "unknown path object tag %d", * (P_U32) p_path);

                        /*FALLTHRU*/

                    case DRAW_PATH_TYPE_CLOSE_WITH_LINE:
#endif
                        p_path += sizeof32(DRAW_PATH_CLOSE);
                        break;
                    }

                break;
                }

            case DRAW_OBJECT_TYPE_SPRITE:
            case DRAW_OBJECT_TYPE_TRFMSPRITE:
            case DRAW_OBJECT_TYPE_TRFMTEXT:
            case DRAW_OBJECT_TYPE_JPEG:
            case DRAW_OBJECT_TYPE_DS_DIB:
            case DRAW_OBJECT_TYPE_DS_DIBROT:
                {
                /* the bbox of these objects determines the contents format, not vice versa! */
                /* shift and scale and shift bbox */
                draw_box_xform(&pObject.hdr->bbox, NULL, &scale_xform);
                break;
                }

            default:
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
    }

    /* ensure this group recomputed and rebound */
    zero_struct(process);
    process.recurse    = 1;
    process.recompute  = 1;
    gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag,
                                          &group_box, /* just a dummy here, this reset_bbox is going to fix the ensapsulator too */
                                          groupStart,
                                          groupStart + sizeof(DRAW_OBJECT_GROUP) + diagLength,
                                          process);

    /* now client's responsibility to ensure whole diagram simply
     * rebound after (or it happens on diagram end anyway)
    */

    /* not forgetting to tell client where we put it! */
    *pObjectStart = groupStart;

    return(STATUS_DONE);
}

extern void
gr_riscdiag_shift_diagram(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    PC_DRAW_POINT pShiftBy)
{
    DRAW_DIAG_OFFSET thisObject = GR_RISCDIAG_OBJECT_FIRST;
    DRAW_DIAG_OFFSET endObject = GR_RISCDIAG_OBJECT_LAST;
    P_DRAW_OBJECT pObject;

    draw_box_translate(&((P_DRAW_FILE_HEADER) (p_gr_riscdiag->draw_diag.data))->bbox, NULL, pShiftBy);

    if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
    {
        do  {
            switch(pObject.hdr->type)
            {
            case DRAW_OBJECT_TYPE_FONTLIST:
            case DRAW_OBJECT_TYPE_OPTIONS:
            case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                /* objects with either no, or no meaningful, bbox */
                continue;

            default:
                break;
            }

            draw_box_translate(&pObject.hdr->bbox, NULL, pShiftBy);

            switch(pObject.hdr->type)
            {
            case DRAW_OBJECT_TYPE_TEXT:
                {
                DRAW_OBJECT_TEXT * pObjectText = (DRAW_OBJECT_TEXT *) pObject.hdr;

                /* shift baseline origin */
                pObjectText->coord.x += pShiftBy->x;
                pObjectText->coord.y += pShiftBy->y;

                break;
                }

            case DRAW_OBJECT_TYPE_PATH:
                {
                P_BYTE p_path;

                /* path ordinarily starts here */
                p_path = (char *) (pObject.path + 1);

                /* simply scale the dash lengths and start offset if present */
                if(pObject.path->pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
                {
                    const S32 dashcount = ((PC_DRAW_DASH_HEADER) p_path)->dashcount;

                    /* skip dash header and pattern */
                    p_path += sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * dashcount;
                }

                /* shift the path coordinates */

                while(* (P_U32) p_path != DRAW_PATH_TYPE_TERM)
                    switch(* (P_U32) p_path)
                    {
                    case DRAW_PATH_TYPE_MOVE:
                    case DRAW_PATH_TYPE_LINE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_LINE line;

                        memcpy32(&line, p_path, sizeof32(line));

                        line.pt.x += pShiftBy->x;
                        line.pt.y += pShiftBy->y;

                        memcpy32(p_path, &line, sizeof32(line));

                        p_path += sizeof32(line);
                        break;
                        }

                    case DRAW_PATH_TYPE_CURVE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_CURVE curve;

                        memcpy32(&curve, p_path, sizeof32(curve));

                        curve.cp1.x += pShiftBy->x;
                        curve.cp1.y += pShiftBy->y;

                        curve.cp2.x += pShiftBy->x;
                        curve.cp2.y += pShiftBy->y;

                        curve.end.x += pShiftBy->x;
                        curve.end.y += pShiftBy->y;

                        memcpy32(p_path, &curve, sizeof32(curve));

                        p_path += sizeof32(curve);
                        break;
                        }

                    default:
#if CHECKING
                        myassert1x(0, "unknown path object tag %d", * (P_U32) p_path);

                        /*FALLTHRU*/

                    case DRAW_PATH_TYPE_CLOSE_WITH_LINE:
#endif
                        p_path += sizeof32(DRAW_PATH_CLOSE);
                        break;
                    }

                break;
                }

            default:
#if CHECKING
            case DRAW_OBJECT_TYPE_SPRITE:
            case DRAW_OBJECT_TYPE_TRFMSPRITE:
            case DRAW_OBJECT_TYPE_TRFMTEXT:
            case DRAW_OBJECT_TYPE_JPEG:
            case DRAW_OBJECT_TYPE_DS_DIB:
            case DRAW_OBJECT_TYPE_DS_DIBROT:
#endif
                /* the bbox of these objects determines the contents format, not vice versa! */
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
    }
}

extern font
gr_riscdiag_font_from_textstyle(
    PC_GR_TEXTSTYLE textstyle)
{
    S32 fsizex16, fsizey16; /* in 16ths of a point */
    font f;

    fsizex16 = (S32) ((textstyle->width  * 16) / GR_PIXITS_PER_POINT);
    fsizey16 = (S32) ((textstyle->height * 16) / GR_PIXITS_PER_POINT);

    if(font_find((P_U8) textstyle->szFontName, fsizex16, fsizey16, 0, 0, &f))
        f = 0;

    /* SKS after 4.11 21jan92 - try looking up our alternate font */
    if(!f)
        if(font_find((P_U8) string_lookup(GR_CHART_MSG_ALTERNATE_FONTNAME), fsizex16, fsizey16, 0, 0, &f))
            f = 0;

    return(f);
}

extern void
gr_riscdiag_font_dispose(
    font * fp /*inout*/)
{
    if(*fp)
    {
        font f = *fp;
        *fp = 0;
        font_LoseFont(f);
    }
}

/*
A type 0B Draw file descriptor ripped off out of an otherwise empty Draw (0.77) file

Address  : 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F

00000028 : 0B 00 00 00 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00000040 : 00 05 00 00 00 01 00 00 00 00 F0 3F 00 00 00 00 02 00 00 00 00 00 00 00
00000058 : 00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
00000070 : 00 00 00 00 01 00 00 00 02 00 00 00 88 13 00 00
*/

static const char
gr_riscdiag_wackytag_encapsulated_object[] =
{
    '\x0B', '\x00', '\x00', '\x00', /* type */ /* 28-2F */
    '\x58', '\x00', '\x00', '\x00', /* size */

    '\x00', '\x00', '\x00', '\x00', /* bbox */ /* 30-3F */
    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00',

    '\x00', '\x05', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', /* 40-4F */
    '\x00', '\x00', '\xF0', '\x3F', '\x00', '\x00', '\x00', '\x00',

    '\x02', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', /* 50-5F */
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',

    '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', /* 60-6F */
    '\x01', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00',

    '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', /* 70-7F */
    '\x02', '\x00', '\x00', '\x00', '\x88', '\x13', '\x00', '\x00'
};

#ifdef SAVE_KOSHER_WACKYTAG_OBJECT
#define GR_RISCDIAG_WACKYTAG_EXTRABYTES 0 /* uneditable */
#else
#define GR_RISCDIAG_WACKYTAG_EXTRABYTES 0x01000000
#endif

static const DRAW_OBJECT_TAG
gr_riscdiag_wackytag_tag_object =
{
    /* type  */ DRAW_OBJECT_TYPE_TAG,
    /* size  */ sizeof(DRAW_OBJECT_TAG) +
                sizeof(gr_riscdiag_wackytag_encapsulated_object) +
                       GR_RISCDIAG_WACKYTAG_EXTRABYTES, /* at least */
    /* bbox  */ { 0, 0, 0, 0 },
    /* tagID */ GR_RISCDIAG_WACKYTAG
};

/******************************************************************************
*
* start saving out a wacky tagged object into an opened Draw file
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_wackytag_save_start(
    FILE_HANDLE file_handle,
    _OutRef_    P_DRAW_DIAG_OFFSET p_offset)
{
    filepos_t pos;

    *p_offset = 0;

    status_return(file_getpos(file_handle, &pos));

    status_return(file_write_bytes(&gr_riscdiag_wackytag_tag_object, sizeof32(gr_riscdiag_wackytag_tag_object), file_handle));

    status_return(file_write_bytes(gr_riscdiag_wackytag_encapsulated_object, sizeof32(gr_riscdiag_wackytag_encapsulated_object), file_handle));

    *p_offset = pos.lo;

    return(1);
}

_Check_return_
extern STATUS
gr_riscdiag_wackytag_save_end(
    FILE_HANDLE file_handle,
    _InRef_     PC_DRAW_DIAG_OFFSET p_offset)
{
    filepos_t curpos;
    filepos_t pos;
    U32 size;

    /* Draw objects and tag goop must be word-aligned/sized */
    status_return(file_pad(file_handle, 2 /* == log2(4) */, CH_NULL));

    status_return(file_getpos(file_handle, &curpos));

    pos.lo = *p_offset + offsetof(DRAW_OBJECT_TAG, size);
  /*pos.hi = 0;*/

    /* total number of bytes written as tag object, encapsulated object and goop */
    size = curpos.lo - *p_offset;

    status_return(file_setpos(file_handle, &pos));

    status_return(file_write_bytes(&size, sizeof32(U32), file_handle));

    status_return(file_setpos(file_handle, &curpos));

    return(1);
}

/* end of gr_rdia2.c */
