/* gr_diag.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Representation building */

/* SKS July 1991 */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h" /* for wimpt_dx */
#endif

/*
internal functions
*/

_Check_return_
static inline GR_DIAG_OFFSET
gr_diag_normalise_stt(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in)
{
    GR_DIAG_OFFSET sttObject = sttObject_in;

    myassert0x(p_gr_diag && array_elements32(&p_gr_diag->handle), "gr_diag_normalise_stt has no diagram");
    IGNOREPARM_InRef_(p_gr_diag);

    if(sttObject == GR_DIAG_OBJECT_FIRST)
        sttObject = sizeof32(GR_DIAG_DIAGHEADER);

    myassert1x(sttObject >= sizeof32(GR_DIAG_DIAGHEADER), "gr_diag_normalise_stt_end has sttObject " U32_XTFMT " < sizeof(GR_DIAG_DIAGHEADER)", sttObject);
    myassert2x(sttObject <= array_elements32(&p_gr_diag->handle), "gr_diag_normalise_stt_end has sttObject " U32_XTFMT " > p_gr_diag->offset " U32_XTFMT, sttObject, array_elements32(&p_gr_diag->handle));

    return(sttObject);
}

_Check_return_
static inline GR_DIAG_OFFSET
gr_diag_normalise_end(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET endObject_in)
{
    GR_DIAG_OFFSET endObject = endObject_in;

    myassert0x(p_gr_diag && array_elements32(&p_gr_diag->handle), "gr_diag_normalise_end has no diagram");

    if(endObject == GR_DIAG_OBJECT_LAST)
        endObject = array_elements32(&p_gr_diag->handle);

    myassert1x(endObject >= sizeof32(GR_DIAG_DIAGHEADER), "gr_diag_normalise_end has endObject " U32_XTFMT " < sizeof(GR_DIAG_DIAGHEADER)", endObject);
    myassert2x(endObject <= array_elements32(&p_gr_diag->handle), "gr_diag_normalise_end has endObject " U32_XTFMT " > p_gr_diag->offset " U32_XTFMT, endObject, array_elements32(&p_gr_diag->handle));

    return(endObject);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_font_tables_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _OutRef_    P_ARRAY_HANDLE p_array_handleR);

_Check_return_
static STATUS
gr_diag_create_riscdiag_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in);

/* ------------------------------------------------------------------------- */

/* diagrams are built as a pair of representations:
 * i) the mostly-system independent diagram
 * ii) a system-specific representation eg. RISC OS Draw file, WINDOWS metafile
 * there being stored in (i) offsets to the corresponding objects in (ii)
 * to enable correlations, clipping etc.
*/

#ifndef GR_DIAG_SIZE_INIT
#define GR_DIAG_SIZE_INIT 1024
#endif

#ifndef GR_DIAG_SIZE_INCR
#define GR_DIAG_SIZE_INCR 512
#endif

/******************************************************************************
*
* run over this diagram forming the system-dependent representation
*
******************************************************************************/

_Check_return_
static STATUS
gr_diag_create_riscdiag(
    P_GR_DIAG p_gr_diag)
{
    P_GR_DIAG_DIAGHEADER pDiagHdr;
    ARRAY_HANDLE font_table_array_handleR;
    STATUS status;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_create_riscdiag has no diagram &%p->&%d", p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    gr_riscdiag_diagram_delete(&p_gr_diag->gr_riscdiag);

    pDiagHdr = gr_diag_getoffptr(GR_DIAG_DIAGHEADER, p_gr_diag, 0);

    status_return(gr_diag_create_riscdiag_font_tables_between(p_gr_diag, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST, &font_table_array_handleR));

    if(status_ok(status = gr_riscdiag_diagram_new(&p_gr_diag->gr_riscdiag, pDiagHdr->szCreatorName, font_table_array_handleR)))
        status = gr_diag_create_riscdiag_between(p_gr_diag, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST);

    if(status_ok(status))
        {
        gr_riscdiag_diagram_end(&p_gr_diag->gr_riscdiag);
        status = STATUS_DONE; /* paranoia */
        }
    else
        gr_riscdiag_diagram_delete(&p_gr_diag->gr_riscdiag);

    al_array_dispose(&font_table_array_handleR);

    return(status);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in)
{
    P_GR_RISCDIAG p_gr_riscdiag = &p_gr_diag->gr_riscdiag;
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_create_riscdiag_between has no diagram &%p->&%d",
               p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    while(thisObject < endObject)
        {
        P_GR_DIAG_OBJECT pObject;
        U32 objectSize;
        GR_DIAG_OBJTYPE objectType;
        GR_DIAG_OFFSET sys_off;
        STATUS status = STATUS_DONE;

        pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

        objectType = pObject.hdr->tag;
        objectSize = pObject.hdr->size;

        myassert2x(objectSize >= sizeof(*pObject.hdr),
                   "gr_diag_create_riscdiag_between object " U32_XTFMT " size " U32_XTFMT " > sizeof(*pObject.hdr)",
                   thisObject, objectSize);

        myassert3x(thisObject + objectSize <= array_elements32(&p_gr_diag->handle),
                   "gr_diag_create_riscdiag_between object " U32_XTFMT " size " U32_XTFMT " > p_gr_diag->offset " U32_XTFMT,
                   thisObject, objectSize, array_elements32(&p_gr_diag->handle));

        sys_off = GR_RISCDIAG_OBJECT_NONE; /* just in case */
        status = STATUS_DONE;

        switch(objectType)
            {
            case GR_DIAG_OBJTYPE_GROUP:
                {
                /* recurse unconditionally creating group contents.
                 * could proceed linearly if we had a patchup list - but it don't go that deep
                */
                if((status = gr_riscdiag_group_new(p_gr_riscdiag, &sys_off, *pObject.group->name ? pObject.group->name : NULL)) < 0)
                    break;

                status = gr_diag_create_riscdiag_between(p_gr_diag,
                                                         thisObject + sizeof(GR_DIAG_OBJGROUP),
                                                         thisObject + objectSize);

                /* fix up the group even if error */
                gr_riscdiag_group_end(p_gr_riscdiag, &sys_off);
                }
                break;

            case GR_DIAG_OBJTYPE_TEXT:
                {
                GR_BOX   box;
                GR_POINT point, size;
                DRAW_POINT draw_point;
                char szText[256], szFontName[64];
                GR_COORD  fsizex, fsizey;
                GR_COLOUR fg, bg;
                PC_U8     textp, segendp;
                size_t    textoff, seglen, wholelen;

                point  = pObject.text->pos;
                size   = pObject.text->wid_hei;
                fsizex = pObject.text->textstyle.width;
                fsizey = pObject.text->textstyle.height;
                fg     = pObject.text->textstyle.fg;
                bg     = pObject.text->textstyle.bg;

                strcpy(szFontName, pObject.text->textstyle.szFontName);

                box.x0 = point.x;
                box.y0 = point.y;
                box.x1 = point.x + size.x;
                box.y1 = point.y + size.y;

                /* loop for structure */
                for(;;)
                    {
                    textoff  = 0;
                    wholelen = strlen((PC_U8) (pObject.text + 1));

                    while(textoff < wholelen)
                        {
                        GR_DIAG_OFFSET sys_off_string;

                        textp = (PC_U8) (pObject.text + 1) + textoff;

                        /* is there a line break delimiter? */
                        segendp = strchr(textp, 10);

                        if(!segendp)
                            {
                            seglen = strlen(textp);
                            seglen = MIN(seglen, sizeof(szText)-1);
                            }
                        else
                            {
                            seglen = segendp - textp;
                            if(seglen > sizeof(szText)-1)
                                seglen = sizeof(szText)-1;
                            else
                                ++textoff; /* skip line term iff whole line sent */

                            /* defer group creation till actually needed */
                            if(GR_RISCDIAG_OBJECT_NONE == sys_off)
                                if((status = gr_riscdiag_group_new(p_gr_riscdiag, &sys_off, NULL)) < 0)
                                    break;
                            }

                        textoff += seglen;

                        safe_strnkpy(szText, elemof32(szText), textp, seglen);

                        gr_point_xform((P_GR_POINT) &draw_point, &point, &gr_riscdiag_riscDraw_from_pixit_xformer);

                        if((status = gr_riscdiag_string_new(p_gr_riscdiag, &sys_off_string,
                                                            &draw_point, szText, szFontName,
                                                            fsizex, fsizey, fg, bg)) < 0)
                            break;

                        point.y -= (fsizey * 12) / 10;
                        }

                    /* end of loop for structure */
                    break;
                    }

                /* fix up the group even if error (iff created) */
                if(GR_RISCDIAG_OBJECT_NONE != sys_off)
                    gr_riscdiag_group_end(p_gr_riscdiag, &sys_off);
                }
                break;

            case GR_DIAG_OBJTYPE_RECTANGLE:
                {
                GR_BOX box;
                DRAW_BOX draw_box;
                GR_LINESTYLE linestyle;
                GR_FILLSTYLE fillstyle;

                box.x0 = pObject.rect->pos.x;
                box.y0 = pObject.rect->pos.y;
                box.x1 = pObject.rect->pos.x + pObject.rect->wid_hei.x;
                box.y1 = pObject.rect->pos.y + pObject.rect->wid_hei.y;

                linestyle = pObject.rect->linestyle;
                fillstyle = pObject.rect->fillstyle;

                gr_box_xform((P_GR_BOX) &draw_box, &box, &gr_riscdiag_riscDraw_from_pixit_xformer);

                status = gr_riscdiag_rectangle_new(p_gr_riscdiag, &sys_off,
                                                   &draw_box, &linestyle, &fillstyle);
                }
                break;

            case GR_DIAG_OBJTYPE_LINE:
                {
                GR_POINT origin, ps1;
                DRAW_POINT draw_origin, draw_ps1;
                GR_LINESTYLE linestyle;

                origin.x  = pObject.line->pos.x;
                origin.y  = pObject.line->pos.y;
                ps1.x     = pObject.line->d.x;
                ps1.y     = pObject.line->d.y;

                linestyle = pObject.line->linestyle;

                gr_point_xform((P_GR_POINT) &draw_origin, &origin, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps1, &ps1, &gr_riscdiag_riscDraw_from_pixit_xformer);

                status = gr_riscdiag_parallelogram_new(p_gr_riscdiag, &sys_off,
                                                       &draw_origin, &draw_ps1, &draw_ps1,
                                                       &linestyle, NULL);
                }
                break;

            case GR_DIAG_OBJTYPE_PARALLELOGRAM:
                {
                GR_POINT origin, ps1, ps2;
                DRAW_POINT draw_origin, draw_ps1, draw_ps2;
                GR_LINESTYLE linestyle;
                GR_FILLSTYLE fillstyle;

                origin.x  = pObject.para->pos.x;
                origin.y  = pObject.para->pos.y;
                ps1.x     = pObject.para->wid_hei1.x;
                ps1.y     = pObject.para->wid_hei1.y;
                ps2.x     = pObject.para->wid_hei2.x;
                ps2.y     = pObject.para->wid_hei2.y;

                linestyle = pObject.para->linestyle;
                fillstyle = pObject.para->fillstyle;

                gr_point_xform((P_GR_POINT) &draw_origin, &origin, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps1, &ps1, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps2, &ps2, &gr_riscdiag_riscDraw_from_pixit_xformer);

                status = gr_riscdiag_parallelogram_new(p_gr_riscdiag, &sys_off,
                                                       &draw_origin, &draw_ps1, &draw_ps2,
                                                       &linestyle, &fillstyle);
                }
                break;

            case GR_DIAG_OBJTYPE_TRAPEZOID:
                {
                GR_POINT origin, ps1, ps2, ps3;
                DRAW_POINT draw_origin, draw_ps1, draw_ps2, draw_ps3;
                GR_LINESTYLE linestyle;
                GR_FILLSTYLE fillstyle;

                origin.x  = pObject.trap->pos.x;
                origin.y  = pObject.trap->pos.y;
                ps1.x     = pObject.trap->wid_hei1.x;
                ps1.y     = pObject.trap->wid_hei1.y;
                ps2.x     = pObject.trap->wid_hei2.x;
                ps2.y     = pObject.trap->wid_hei2.y;
                ps3.x     = pObject.trap->wid_hei3.x;
                ps3.y     = pObject.trap->wid_hei3.y;

                linestyle = pObject.trap->linestyle;
                fillstyle = pObject.trap->fillstyle;

                gr_point_xform((P_GR_POINT) &draw_origin, &origin, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps1, &ps1, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps2, &ps2, &gr_riscdiag_riscDraw_from_pixit_xformer);
                gr_point_xform((P_GR_POINT) &draw_ps3, &ps3, &gr_riscdiag_riscDraw_from_pixit_xformer);

                status = gr_riscdiag_trapezoid_new(p_gr_riscdiag, &sys_off,
                                                   &draw_origin, &draw_ps1, &draw_ps2, &draw_ps3,
                                                   &linestyle, &fillstyle);
                }
                break;

            case GR_DIAG_OBJTYPE_PIESECTOR:
                {
                GR_POINT pos;
                GR_COORD       radius;
                DRAW_POINT     draw_origin;
                DRAW_COORD     draw_radius;
                F64            alpha, beta;
                GR_LINESTYLE   linestyle;
                GR_FILLSTYLE   fillstyle;

                pos       = pObject.pie->pos;
                radius    = pObject.pie->radius;
                alpha     = pObject.pie->alpha;
                beta      = pObject.pie->beta;
                linestyle = pObject.pie->linestyle;
                fillstyle = pObject.pie->fillstyle;

                gr_point_xform((P_GR_POINT) &draw_origin, &pos, &gr_riscdiag_riscDraw_from_pixit_xformer);
                draw_radius = gr_riscDraw_from_pixit(radius);

                status = gr_riscdiag_piesector_new(p_gr_riscdiag, &sys_off,
                                                   &draw_origin, draw_radius, &alpha, &beta,
                                                   &linestyle, &fillstyle);
                }
                break;

            case GR_DIAG_OBJTYPE_PICTURE:
                {
                GR_BOX    box;
                DRAW_BOX        draw_box;
                GR_CACHE_HANDLE picture;
                GR_FILLSTYLE    fillstyle;
                P_ANY diag;

                /* a representation of 'nothing' */

                box.x0 = pObject.pict->pos.x;
                box.y0 = pObject.pict->pos.y;
                box.x1 = pObject.pict->pos.x + pObject.pict->wid_hei.x;
                box.y1 = pObject.pict->pos.y + pObject.pict->wid_hei.y;

                picture   = pObject.pict->picture;

                fillstyle = pObject.pict->fillstyle;

                diag = gr_cache_loaded_ensure(&picture);

                draw_box.x0 = gr_riscDraw_from_pixit(box.x0);
                draw_box.y0 = gr_riscDraw_from_pixit(box.y0);
                draw_box.x1 = gr_riscDraw_from_pixit(box.x1);
                draw_box.y1 = gr_riscDraw_from_pixit(box.y1);

                if(diag)
                    status = gr_riscdiag_scaled_diagram_add(p_gr_riscdiag, &sys_off,
                                                            &draw_box, diag, &fillstyle);
                }
                break;

            default:
                break;
            }

        status_return(status);

        /* poke diagram with offset of corresponding object in RISC OS diagram */
        pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

        pObject.hdr->sys_off = sys_off;

        thisObject += objectSize;
        }

    return(STATUS_DONE);
}

static STATUS
gr_diag_ensure_riscdiag_font_tableR_entry(
    _InRef_     PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p_f,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    { /* check for existing entry */
    ARRAY_INDEX i;
    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY e = array_ptr(p_array_handle, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, i);
        if(0 == memcmp(e, p_f, sizeof32(*p_f)))
            return(STATUS_OK); /* already in table */
    }
    } /*block*/

    { /* add new entry at end */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_f), FALSE);
    status_return(al_array_add(p_array_handle, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, 1, &array_init_block, p_f));
    } /*block*/

    return(STATUS_OK);
}

static STATUS
gr_diag_ensure_riscdiag_font_tableR_entry_for_TEXT(
    _In_z_      PCTSTR pszFontName,
    /*inout*/ P_ARRAY_HANDLE p_array_handle)
{
    GR_RISCDIAG_RISCOS_FONTLIST_ENTRY f;

    zero_struct(f);
    safe_strkpy(f.szHostFontName, sizeof32(f.szHostFontName), _l1str_from_tstr(pszFontName));

    return(gr_diag_ensure_riscdiag_font_tableR_entry(&f, p_array_handle));
}

static STATUS
gr_diag_ensure_riscdiag_font_tableR_entries_for_PICTURE(
    _In_        P_DRAW_DIAG p_draw_diag,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;

    /* scan the diagram for font tables */

    /* give loaded diagram away to temp diagram for scan process */
    gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);

    gr_riscdiag_fontlist_scan(p_gr_riscdiag, DRAW_DIAG_OFFSET_FIRST, DRAW_DIAG_OFFSET_LAST);

    if(p_gr_riscdiag->dd_fontListR)
    {
        const DRAW_DIAG_OFFSET fontListR = p_gr_riscdiag->dd_fontListR;
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        const DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        PC_DRAW_FONTLIST_ELEM pFontListElem = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

        /* actual end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + strlen32p1(pFontListElem->szHostFontName); /* for NULLCH */
            GR_RISCDIAG_RISCOS_FONTLIST_ENTRY f;

            zero_struct(f);
            safe_strkpy(f.szHostFontName, sizeof32(f.szHostFontName), pFontListElem->szHostFontName);

            status_break(status = gr_diag_ensure_riscdiag_font_tableR_entry(&f, p_array_handle));

            thisOffset += thislen;

            pFontListElem = PtrAddBytes(PC_DRAW_FONTLIST_ELEM, pFontListElem, thislen);
        }
    }

    draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

    return(status);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_font_tables_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _OutRef_    P_ARRAY_HANDLE p_array_handleR)
{
    GR_DIAG_OFFSET thisObject = sttObject_in;
    GR_DIAG_OFFSET endObject = endObject_in;
    STATUS status = STATUS_OK;
    P_GR_DIAG_OBJECT pObject;

    *p_array_handleR = 0;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_create_riscdiag_between has no diagram &%p->&") S32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    if(gr_diag_object_first(p_gr_diag, &thisObject, &endObject, &pObject))
    {
        do  {
            switch(pObject.hdr->tag)
            {
            case GR_DIAG_OBJTYPE_TEXT:
                /* NB font name is copied away safely before any realloc... */
                status = gr_diag_ensure_riscdiag_font_tableR_entry_for_TEXT(pObject.text->textstyle.szFontName, p_array_handleR);
                break;

            case GR_DIAG_OBJTYPE_PICTURE:
                {
                GR_DIAG_OBJPICTURE pict;
                GR_CACHE_HANDLE picture;
                P_DRAW_DIAG diag;

                memcpy32(&pict, pObject.pict, sizeof32(pict));

                picture = pict.picture;

                diag = gr_cache_loaded_ensure(&picture);

                if(diag)
                    status = gr_diag_ensure_riscdiag_font_tableR_entries_for_PICTURE(diag, p_array_handleR);

                break;
                }

            default:
                break;
            }

            status_break(status);
        }
        while(gr_diag_object_next(p_gr_diag, &thisObject, endObject, &pObject));
    }

    return(status);
}

#ifdef GR_DIAG_FULL_SET

/******************************************************************************
*
* run backwards over diagram looking for top object that clips
*
* Hierarchy does help here too!
*
******************************************************************************/

extern S32
gr_diag_diagram_correlate(
    P_GR_DIAG         p_gr_diag,
    PC_GR_POINT point,
    PC_GR_POINT semimajor,
    P_GR_DIAG_OFFSET  pHitObject /*[]out*/,
    S32              recursionLimit)
{
    return(gr_diag_object_correlate_between(
                p_gr_diag, point,
                semimajor,
                pHitObject,
                GR_DIAG_OBJECT_FIRST,
                GR_DIAG_OBJECT_LAST,
                recursionLimit));
}

#endif

/******************************************************************************
*
* dispose of a diagram
*
******************************************************************************/

extern void
gr_diag_diagram_dispose(
    _InoutRef_  P_P_GR_DIAG p_p_gr_diag)
{
    P_GR_DIAG p_gr_diag;

    if(p_p_gr_diag && ((p_gr_diag = *p_p_gr_diag) != NULL))
        {
        /* remove system-dependent representation too */
        gr_riscdiag_diagram_delete(&p_gr_diag->gr_riscdiag);

        al_array_dispose(&p_gr_diag->handle);

        al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_gr_diag));
        }
}

/******************************************************************************
*
* end adding data to a diagram
*
******************************************************************************/

extern GR_DIAG_OFFSET
gr_diag_diagram_end(
    P_GR_DIAG p_gr_diag)
{
    GR_DIAG_PROCESS_T process;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_diagram_end has no diagram &%p->&%d", p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    /* kill old one */
    gr_riscdiag_diagram_delete(&p_gr_diag->gr_riscdiag);

    /* build new system-dependent representation */
    status_assert(gr_diag_create_riscdiag(p_gr_diag));

    * (int *) &process       = 0;
    process.recurse          = 1;
    process.recompute        = 1;
    process.severe_recompute = 0;
    gr_diag_diagram_reset_bbox(p_gr_diag, process);

    return(array_elements32(&p_gr_diag->handle));
}

/******************************************************************************
*
* start a diagram: allocate descriptor and initial chunk & initialise
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_GR_DIAG
gr_diag_diagram_new(
    _In_z_      PCTSTR szCreatorName,
    _OutRef_    P_STATUS p_status)
{
    P_GR_DIAG p_gr_diag = al_ptr_calloc_elem(GR_DIAG, 1, p_status);

    if(NULL != p_gr_diag)
        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(512, sizeof32(U8), 1 /*zero allocation*/);
        P_GR_DIAG_DIAGHEADER pDiagHdr;
        const U32 n_bytes = sizeof32(*pDiagHdr);

        if(NULL != (pDiagHdr = (P_GR_DIAG_DIAGHEADER) al_array_alloc(&p_gr_diag->handle, BYTE, n_bytes, &array_init_block, p_status)))
            {
            safe_tstrkpy(pDiagHdr->szCreatorName, elemof32(pDiagHdr->szCreatorName), szCreatorName);
            gr_box_make_bad(&pDiagHdr->bbox);
            }
        else
            {
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_gr_diag));
            }
        }

    return(p_gr_diag);
}

/******************************************************************************
*
* reset a diagram's bbox
*
******************************************************************************/

extern void
gr_diag_diagram_reset_bbox(
    P_GR_DIAG          p_gr_diag,
    GR_DIAG_PROCESS_T process)
{
    P_GR_DIAG_DIAGHEADER pDiagHdr;
    GR_BOX        diag_box;

    gr_diag_object_reset_bbox_between(p_gr_diag,
                                      &diag_box,
                                      GR_DIAG_OBJECT_FIRST,
                                      GR_DIAG_OBJECT_LAST,
                                      process);

    pDiagHdr = gr_diag_getoffptr(GR_DIAG_DIAGHEADER, p_gr_diag, 0);

    pDiagHdr->bbox = diag_box;
}

#ifdef GR_DIAG_FULL_SET

/******************************************************************************
*
* run over diagram looking for first object that matches
*
******************************************************************************/

extern GR_DIAG_OFFSET
gr_diag_diagram_search(
    P_GR_DIAG p_gr_diag,
    GR_DIAG_OBJID_T objid)
{
    GR_DIAG_PROCESS_T process;

    * (int *) &process = 0;
    process.recurse    = 1;

    return(gr_diag_object_search_between(p_gr_diag, objid,
                                         GR_DIAG_OBJECT_FIRST,
                                         GR_DIAG_OBJECT_LAST,
                                         process));
}

#endif

/******************************************************************************
*
* end a group: go back and patch its size field (leave bbox till diag end)
*
******************************************************************************/

extern U32
gr_diag_group_end(
    P_GR_DIAG         p_gr_diag,
    PC_GR_DIAG_OFFSET pGroupStart)
{
    return(gr_diag_object_end(p_gr_diag, pGroupStart));
}

/******************************************************************************
*
* start a group: leave size & bbox empty to be patched later
*
******************************************************************************/

extern S32
gr_diag_group_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    GR_DIAG_OBJID_T objid,
    PC_U8 szGroupName)
{
    static const char spaces_for_group_name[] =
        /* 012345678901 */
          "            ";

    P_GR_DIAG_OBJECT pObject;
    S32 res;

    if((res = gr_diag_object_new(p_gr_diag, pGroupStart, objid, &pObject, GR_DIAG_OBJTYPE_GROUP, 0)) < 0)
        return(res);

    /* leave name empty? */
    if(szGroupName)
        {
        size_t nameLength;

        trace_3(TRACE_MODULE_GR_CHART,
                "gr_diag_group_new(" PTR_XTFMT ") offset %d, name %s",
                report_ptr_cast(p_gr_diag), pGroupStart ? *pGroupStart : 0,
                trace_string(szGroupName));

        /* fill name with twelve spaces */
        memcpy32(&pObject.group->name, spaces_for_group_name, sizeof32(pObject.group->name));

        /* mustn't be NULLCH terminated */
        nameLength = strlen(szGroupName);
        nameLength = MIN(nameLength, sizeof32(pObject.group->name));
        memcpy32(pObject.group->name, szGroupName, nameLength);
        }

    return(1);
}

/******************************************************************************
*
* add a line as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_line_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_GR_LINESTYLE linestyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_LINE, 0)) < 0)
        return(res);

    pObject.line->pos.x     = pBox->x0;
    pObject.line->pos.y     = pBox->y0;

    pObject.line->d.x       = pBox->x1 - pBox->x0;
    pObject.line->d.y       = pBox->y1 - pBox->y0;

    pObject.line->linestyle = *linestyle;

    return(1);
}

/******************************************************************************
*
* return amount of space to be allocated for the header of this object
*
******************************************************************************/

extern U32
gr_diag_object_base_size(
    GR_DIAG_OBJTYPE objectType)
{
    switch(objectType)
        {
        case GR_DIAG_OBJTYPE_GROUP:
            return(sizeof(GR_DIAG_OBJGROUP));

        case GR_DIAG_OBJTYPE_TEXT:
            return(sizeof(GR_DIAG_OBJTEXT));

        case GR_DIAG_OBJTYPE_LINE:
            return(sizeof(GR_DIAG_OBJLINE));

        case GR_DIAG_OBJTYPE_RECTANGLE:
            return(sizeof(GR_DIAG_OBJRECTANGLE));

        case GR_DIAG_OBJTYPE_PARALLELOGRAM:
            return(sizeof(GR_DIAG_OBJPARALLELOGRAM));

        case GR_DIAG_OBJTYPE_TRAPEZOID:
            return(sizeof(GR_DIAG_OBJTRAPEZOID));

        case GR_DIAG_OBJTYPE_PIESECTOR:
            return(sizeof(GR_DIAG_OBJPIESECTOR));

        case GR_DIAG_OBJTYPE_PICTURE:
            return(sizeof(GR_DIAG_OBJPICTURE));

        default:
            myassert1x(0, "gr_diag_object_base_size of objectType %d", objectType);
            return(sizeof(GR_DIAG_OBJHDR));
        }
}

/******************************************************************************
*
* run backwards over range of objects looking for top object that clips
*
* Hierarchy does help here too!
*
******************************************************************************/

extern S32
gr_diag_object_correlate_between(
    P_GR_DIAG p_gr_diag,
    PC_GR_POINT point,
    PC_GR_POINT semimajor,
    P_GR_DIAG_OFFSET pHitObject /*[]out*/,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    S32 recursionLimit)
{
    const GR_DIAG_OFFSET sttObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    GR_DIAG_OFFSET thisObject;
    U32 objectSize;
    GR_DIAG_OBJTYPE objectType;

    /* always say nothing hit at this level so caller can scan hitObject list and find a terminator */
    pHitObject[0] = GR_DIAG_OBJECT_NONE;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_object_correlate_between has no diagram &%p->&%d",
               p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    /* start with current object off end of range */
    thisObject = endObject;

    /* whenever current object becomes same as start object we've missed */
    while(thisObject > sttObject)
        {
        S32 hit;

        /* block to reduce recursion stack overhead */
        {
        /* find next object to process: scan up from start till we meet current object */
        GR_DIAG_OFFSET findObject = sttObject;
        P_GR_DIAG_OBJECT pObject;
        GR_POINT hit_point;

        pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, findObject);

        for(;;)
            {
            objectType = pObject.hdr->tag;
            objectSize = pObject.hdr->size;

            myassert2x(objectSize >= sizeof32(*pObject.hdr),
                       "gr_diag_object_correlate_between object " U32_XTFMT " size " U32_XTFMT " > sizeof(*pObject.hdr)",
                       findObject, objectSize);

            myassert3x(findObject + objectSize <= array_elements32(&p_gr_diag->handle),
                       "gr_diag_object_correlate_between object " U32_XTFMT " size " U32_XTFMT " > p_gr_diag->offset " U32_XTFMT,
                       findObject, objectSize, array_elements32(&p_gr_diag->handle));

            if(findObject + objectSize >= thisObject)
                {
                /* leave lock on pObject.hdr for bbox scan - loop ALWAYS terminates through here */
                /* also moves thisObject down */
                thisObject = findObject;
                break;
                }

            findObject    += objectSize;
            pObject.p_byte += objectSize;
            }

        /* test to see whether we hit this object */
        hit = gr_box_hit(&hit_point, &pObject.hdr->bbox, point, semimajor);
        } /*block*/

        if(hit > 0)
            {
            pHitObject[0] = thisObject;
            pHitObject[1] = GR_DIAG_OBJECT_NONE; /* keep terminated */

            /* did we hit hierarchy that needs searching at a finer level
             * and that we are allowed to search into?
            */
            if((objectType == GR_DIAG_OBJTYPE_GROUP)  &&  (recursionLimit != 0))
                {
                /* find a new version of hit as we may have hit a group
                 * at this level but miss all its components at the next
                 * so groups are not found as leaves when recursing

                 * SKS after 4.12 12feb92 - must limit search to the part of
                 *                          this group spanned by stt...end
                */
                hit = gr_diag_object_correlate_between(p_gr_diag, point,
                                                       semimajor,
                                                       pHitObject + 1, /* will be poked at a finer level */
                                                       thisObject + sizeof(GR_DIAG_OBJGROUP),
#if 1
                                                       MIN(thisObject + objectSize, endObject),
#else
                                                       thisObject + objectSize,
#endif
                                                       recursionLimit - 1);

                if(hit > 0)
                    return(hit);

                /* kill group hit, keep terminated */
                pHitObject[0] = GR_DIAG_OBJECT_NONE;
                }
            else
                return(hit);
            }

        /* loop for next object */
        }

    return(0);
}

/******************************************************************************
*
* end an object: go back and patch its size field
*
******************************************************************************/

extern U32
gr_diag_object_end(
    P_GR_DIAG        p_gr_diag,
    PC_GR_DIAG_OFFSET pObjectStart)
{
    P_GR_DIAG_OBJECT pObject;
    U32 n_bytes;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_object_end has no diagram &%p->&%d",
               p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, *pObjectStart);

    n_bytes = array_elements32(&p_gr_diag->handle) - *pObjectStart;

    if(n_bytes == gr_diag_object_base_size(pObject.hdr->tag))
        {
        /* destroy object and contents if nothing in it */
        al_array_shrink_by(&p_gr_diag->handle, - (S32) n_bytes);
        trace_3(0 | TRACE_OUT, TEXT("object_end: destroying type ") U32_TFMT TEXT(" n_bytes ") U32_XTFMT TEXT(" at ") U32_XTFMT, pObject.hdr->tag, n_bytes, *pObjectStart);
        n_bytes = 0;
        }
    else
        /* update object size */
        pObject.hdr->size = n_bytes;

    return(n_bytes);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

extern BOOL
gr_diag_object_first(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InoutRef_  P_GR_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObject)
{
    P_GR_DIAG_OBJECT pObject;

    *pSttObject = gr_diag_normalise_stt(p_gr_diag, *pSttObject);
    *pEndObject = gr_diag_normalise_end(p_gr_diag, *pEndObject);

    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, *pSttObject);

    if(*pSttObject >= *pEndObject)
        {
        *pSttObject = GR_DIAG_OBJECT_FIRST;
        pObject.hdr = NULL;
        }

    /* maintain current lock as initial, now caller's responsibility */
    *ppObject = pObject;

    return(*pSttObject != GR_DIAG_OBJECT_FIRST); /* note above repoke^^^ on end */
}

/******************************************************************************
*
* start an object: init size, leave bbox bad, to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_object_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_opt_ P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObject,
    GR_DIAG_OBJTYPE objectType,
    _InVal_     U32 extraBytes)
{
    U32 baseBytes, allocBytes;
    P_GR_DIAG_OBJECT pObject;
    STATUS status;

    myassert0x(p_gr_diag, "gr_diag_object_new has no diagram");

    if(NULL != pObjectStart)
        *pObjectStart = array_elements32(&p_gr_diag->handle);

    ppObject->hdr = NULL;

    /* add on size of base object required */
    baseBytes = gr_diag_object_base_size(objectType);
    allocBytes = baseBytes + extraBytes;

    if(NULL == (pObject.p_byte = al_array_extend_by(&p_gr_diag->handle, BYTE, allocBytes, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    pObject.hdr->tag   = objectType;
    pObject.hdr->size  = (int) allocBytes;
    pObject.hdr->objid = objid;

    /* everything has bbox */
    gr_box_make_bad(&pObject.hdr->bbox);

    *ppObject = pObject;

    return(STATUS_DONE);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

extern BOOL
gr_diag_object_next(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObject)
{
    P_GR_DIAG_OBJECT pObject;
    U32 size;

    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, *pSttObject);

    size = pObject.hdr->size;

    /* force scans to be linear, not recursive */
    if(pObject.hdr->tag == GR_DIAG_OBJTYPE_GROUP)
        size = sizeof32(GR_DIAG_OBJGROUP);

    myassert2x(size >= sizeof32(GR_DIAG_OBJHDR),
               TEXT("gr_diag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof(*pObject.hdr)"), *pSttObject, size);
    myassert3x(*pSttObject + size <= array_elements32(&p_gr_diag->handle),
               TEXT("gr_diag_diagram_render object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" > p_gr_diag->offset ") U32_XTFMT, *pSttObject, size, array_elements32(&p_gr_diag->handle));

    *pSttObject += size;

    if(*pSttObject >= endObject)
        {
        *pSttObject = GR_DIAG_OBJECT_FIRST;
        pObject.hdr = NULL;
        }

    *ppObject = pObject;

    return(*pSttObject != GR_DIAG_OBJECT_FIRST); /* note above repoke^^^ on end */
}

/******************************************************************************
*
* reset a bbox over a range of objects in the diagram
*
******************************************************************************/

extern void
gr_diag_object_reset_bbox_between(
    P_GR_DIAG p_gr_diag,
    _OutRef_        P_GR_BOX pBox,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process)
{
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    U32 objectSize;
    GR_DIAG_OBJTYPE objectType;
    P_GR_DIAG_OBJECT pObject;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_object_reset_bbox_between has no diagram &%p->&%d", p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    /* loop over constituent objects and extract bbox */

    gr_box_make_bad(pBox);

    /* keep pObject valid through loop, dropping and reloading where necessary */
    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

    while(thisObject < endObject)
        {
        objectSize = pObject.hdr->size;
        objectType = pObject.hdr->tag;

        myassert2x(objectSize >= sizeof32(*pObject.hdr),
                   "gr_diag_object_reset_bbox_between object " U32_XTFMT " size " U32_XTFMT " < sizeof(*pObject.hdr)",
                   thisObject, objectSize);

        myassert3x(thisObject + objectSize <= array_elements32(&p_gr_diag->handle),
                   "gr_diag_object_reset_bbox_between object " U32_XTFMT " size " U32_XTFMT " larger than diagram " U32_XTFMT,
                   thisObject, objectSize, array_elements32(&p_gr_diag->handle));

        /* fix up hierarchy recursively */
        if(objectType == GR_DIAG_OBJTYPE_GROUP)
            {
            if(process.recurse)
                {
                GR_BOX group_box;

                gr_diag_object_reset_bbox_between(p_gr_diag,
                                                  &group_box,
                                                  thisObject + sizeof32(GR_DIAG_OBJGROUP),
                                                  thisObject + objectSize,
                                                  process);

                if(process.recompute)
                    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

                pObject.hdr->bbox = group_box;
                }
            }
        else if(process.recompute)
            {
            if(process.severe_recompute)
                {
                /* get corresponding object recomputed */

                /* probably no need for this yet, but ... */
                /* pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject); */
                }

            {
            /* merely copy over bbox info from corresponding object in Draw file */
            P_DRAW_OBJECT pObjectR;
            S32 div_x, div_y, mul_x, mul_y;

            pObjectR.p_byte = gr_riscdiag_getoffptr(BYTE, &p_gr_diag->gr_riscdiag, pObject.hdr->sys_off);

            /* pixits from riscDraw ain't exact so take care - even go via pixels */
            div_x = GR_RISCDRAW_PER_RISCOS * wimpt_dx();
            div_y = GR_RISCDRAW_PER_RISCOS * wimpt_dy();
            mul_x = GR_PIXITS_PER_RISCOS   * wimpt_dx();
            mul_y = GR_PIXITS_PER_RISCOS   * wimpt_dy();

            pObject.hdr->bbox.x0 = gr_round_pixit_to_floor(pObjectR.hdr->bbox.x0, div_x) * mul_x;
            pObject.hdr->bbox.y0 = gr_round_pixit_to_floor(pObjectR.hdr->bbox.y0, div_y) * mul_y;
            pObject.hdr->bbox.x1 = gr_round_pixit_to_ceil( pObjectR.hdr->bbox.x1, div_x) * mul_x;
            pObject.hdr->bbox.y1 = gr_round_pixit_to_ceil( pObjectR.hdr->bbox.y1, div_y) * mul_y;
            }
            }

        gr_box_union(pBox, NULL, &pObject.hdr->bbox);

        thisObject    += objectSize;
        pObject.p_byte += objectSize;
        }
}

/******************************************************************************
*
* scan a range of objects in the diagram for the given id
*
******************************************************************************/

extern GR_DIAG_OFFSET
gr_diag_object_search_between(
    P_GR_DIAG          p_gr_diag,
    GR_DIAG_OBJID_T   objid,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    GR_DIAG_PROCESS_T process)
{
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    U32 objectSize;
    GR_DIAG_OBJTYPE objectType;
    P_GR_DIAG_OBJECT pObject;
    GR_DIAG_OFFSET  hitObject = GR_DIAG_OBJECT_NONE;

    myassert2x(p_gr_diag && p_gr_diag->handle, "gr_diag_object_search_between has no diagram &%p->&%d",
               p_gr_diag, p_gr_diag ? p_gr_diag->handle : NULL);

    /* loop over constituent objects */

    /* keep pObject valid through loop, dropping and reloading where necessary */
    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

    while(thisObject < endObject)
        {
        objectSize = pObject.hdr->size;
        objectType = pObject.hdr->tag;

        myassert2x(objectSize >= sizeof32(*pObject.hdr),
                   "gr_diag_object_search_between object " U32_XTFMT " size " U32_XTFMT " < sizeof(*pObject.hdr)",
                   thisObject, objectSize);

        myassert3x(thisObject + objectSize <= array_elements32(&p_gr_diag->handle),
                   "gr_diag_object_search_between object " U32_XTFMT " size " U32_XTFMT " larger than diagram " U32_XTFMT,
                   thisObject, objectSize, array_elements32(&p_gr_diag->handle));

        if((* (P_U32) &objid) == (* (P_U32) &pObject.hdr->objid))
            {
            hitObject = thisObject;
            break;
            }

        if(process.find_children)
            {
            GR_DIAG_OBJID_T id = pObject.hdr->objid;

            gr_chart_objid_find_parent(&id);

            if((* (P_U32) &objid) == (* (P_U32) &id))
                {
                hitObject = thisObject;
                break;
                }
            }

        if(objectType == GR_DIAG_OBJTYPE_GROUP)
            if(process.recurse)
                {
                /* search hierarchy recursively */
                hitObject = gr_diag_object_search_between(p_gr_diag,
                                                          objid,
                                                          thisObject + sizeof(GR_DIAG_OBJGROUP),
                                                          thisObject + objectSize,
                                                          process);

                if(hitObject != GR_DIAG_OBJECT_NONE)
                    break;
                }

        thisObject    += objectSize;
        pObject.p_byte += objectSize;
        }

    return(hitObject);
}

/******************************************************************************
*
* add a parallelogram as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_parallelogram_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET  pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pOrigin,
    PC_GR_POINT ps1,
    PC_GR_POINT ps2,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_PARALLELOGRAM, 0)) < 0)
        return(res);

    pObject.para->pos       = *pOrigin;

    pObject.para->wid_hei1  = *ps1;
    pObject.para->wid_hei2  = *ps2;

    pObject.para->linestyle = *linestyle;
    pObject.para->fillstyle = *fillstyle;

    return(1);
}

/******************************************************************************
*
* add a piesector as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_piesector_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pPos,
    GR_COORD radius,
    PC_F64 alpha,
    PC_F64 beta,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_PIESECTOR, 0)) < 0)
        return(res);

    pObject.pie->pos       = *pPos;

    pObject.pie->radius    = radius;
    pObject.pie->alpha     = *alpha;
    pObject.pie->beta      = *beta;

    pObject.pie->linestyle = *linestyle;
    pObject.pie->fillstyle = *fillstyle;

    return(1);
}

/******************************************************************************
*
* add a rectangle as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_rectangle_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_RECTANGLE, 0)) < 0)
        return(res);

    pObject.rect->pos.x     = pBox->x0;
    pObject.rect->pos.y     = pBox->y0;

    pObject.rect->wid_hei.x = pBox->x1 - pBox->x0;
    pObject.rect->wid_hei.y = pBox->y1 - pBox->y0;

    pObject.rect->linestyle = *linestyle;
    pObject.rect->fillstyle = *fillstyle;

    return(1);
}

/******************************************************************************
*
* add a picture (scaled into a rectangle) as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_scaled_picture_add(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    GR_CACHE_HANDLE picture,
    PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_PICTURE, 0)) < 0)
        return(res);

    pObject.pict->pos.x     = pBox->x0;
    pObject.pict->pos.y     = pBox->y0;

    pObject.pict->wid_hei.x = pBox->x1 - pBox->x0;
    pObject.pict->wid_hei.y = pBox->y1 - pBox->y0;

    pObject.pict->picture   = picture;

    pObject.pict->fillstyle = *fillstyle;

    return(1);
}

extern S32
gr_diag_text_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_BOX pBox,
    PC_USTR szText,
    PC_GR_TEXTSTYLE textstyle)
{
    U32 size;
    P_GR_DIAG_OBJECT pObject;
    S32 res;

    size = strlen32p1(szText); /* includes NULLCH */

    /* round up to output word boundary */
    size = round_up(size, 4);

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_TEXT, size)) < 0)
        return(res);

    pObject.text->pos.x = pBox->x0;
    pObject.text->pos.y = pBox->y0;

    pObject.text->wid_hei.x = pBox->x1 - pBox->x0;
    pObject.text->wid_hei.y = pBox->y1 - pBox->y0;

    pObject.text->textstyle = *textstyle;

    strcpy((P_U8) (pObject.text + 1), szText);

    return(1);
}

/******************************************************************************
*
* add a trapezoid as a named object in the diagram
*
******************************************************************************/

extern S32
gr_diag_trapezoid_new(
    P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET  pObjectStart,
    GR_DIAG_OBJID_T objid,
    PC_GR_POINT pOrigin,
    PC_GR_POINT ps1,
    PC_GR_POINT ps2,
    PC_GR_POINT ps3,
    PC_GR_LINESTYLE linestyle,
    PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;
    S32             res;

    if((res = gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_TRAPEZOID, 0)) < 0)
        return(res);

    pObject.trap->pos       = *pOrigin;

    pObject.trap->wid_hei1  = *ps1;
    pObject.trap->wid_hei2  = *ps2;
    pObject.trap->wid_hei3  = *ps3;

    pObject.trap->linestyle = *linestyle;
    pObject.trap->fillstyle = *fillstyle;

    return(1);
}

/* end of gr_diag.c */
