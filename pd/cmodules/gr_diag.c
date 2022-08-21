/* gr_diag.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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
    UNREFERENCED_PARAMETER_InRef_(p_gr_diag);

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

_Check_return_
static BOOL
gr_diag_line_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

_Check_return_
static BOOL
gr_diag_piesector_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

_Check_return_
static BOOL
gr_diag_quadrilateral_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

/* ------------------------------------------------------------------------- */

/* diagrams are built as a pair of representations:
 * i) the mostly-system independent diagram
 * ii) a system-specific representation e.g. RISC OS Draw file, WINDOWS metafile
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
        GR_DIAG_OFFSET sys_off = GR_RISCDIAG_OBJECT_NONE; /* just in case */
        STATUS status = STATUS_DONE;

        pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, thisObject);

        objectType = pObject.hdr->tag;
        objectSize = pObject.hdr->n_bytes;

        myassert2x(objectSize >= sizeof(*pObject.hdr),
                   "gr_diag_create_riscdiag_between object " U32_XTFMT " size " U32_XTFMT " > sizeof(*pObject.hdr)",
                   thisObject, objectSize);

        myassert3x(thisObject + objectSize <= array_elements32(&p_gr_diag->handle),
                   "gr_diag_create_riscdiag_between object " U32_XTFMT " size " U32_XTFMT " > p_gr_diag->offset " U32_XTFMT,
                   thisObject, objectSize, array_elements32(&p_gr_diag->handle));

        status = STATUS_DONE;

        switch(objectType)
        {
        case GR_DIAG_OBJTYPE_GROUP:
            {
            /* recurse unconditionally creating group contents.
             * could proceed linearly if we had a patchup list - but it don't go that deep
            */
            if((status = gr_riscdiag_group_new(p_gr_riscdiag, &sys_off, NULL)) < 0)
                break;

            status = gr_diag_create_riscdiag_between(p_gr_diag,
                                                     thisObject + sizeof(GR_DIAG_OBJGROUP),
                                                     thisObject + objectSize);

            /* fix up the group even if error */
            gr_riscdiag_group_end(p_gr_riscdiag, &sys_off);

            break;
            }

        case GR_DIAG_OBJTYPE_TEXT:
            {
            GR_DIAG_OFFSET sys_off_group = GR_RISCDIAG_OBJECT_NONE;
            GR_BOX   box;
            GR_POINT point;
            GR_SIZE  size;
            DRAW_POINT draw_point;
            char szText[256], szFontName[64];
            GR_COORD  fsizex, fsizey;
            GR_COLOUR fg, bg;
            PC_U8     textp, segendp;
            size_t    textoff, seglen, wholelen;

            point  = pObject.text->pos;
            size   = pObject.text->size;
            fsizex = pObject.text->textstyle.width;
            fsizey = pObject.text->textstyle.height;
            fg     = pObject.text->textstyle.fg;
            bg     = pObject.text->textstyle.bg;

            strcpy(szFontName, pObject.text->textstyle.szFontName);

            box.x0 = point.x;
            box.y0 = point.y;
            box.x1 = point.x + size.cx;
            box.y1 = point.y + size.cy;

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
                        if(GR_RISCDIAG_OBJECT_NONE == sys_off_group)
                        {
                            if((status = gr_riscdiag_group_new(p_gr_riscdiag, &sys_off_group, NULL)) < 0)
                                break;

                            assert(GR_RISCDIAG_OBJECT_NONE == sys_off);
                            sys_off = sys_off_group;
                        }
                    }

                    textoff += seglen;

                    xstrnkpy(szText, elemof32(szText), textp, seglen);

                    gr_point_xform((P_GR_POINT) &draw_point, &point, &gr_riscdiag_riscDraw_from_pixit_xformer);

                    if((status = gr_riscdiag_string_new(p_gr_riscdiag, &sys_off_string,
                                                        &draw_point, szText, szFontName,
                                                        fsizex, fsizey, fg, bg)) < 0)
                        break;

                    point.y -= (fsizey * 12) / 10;

                    if(GR_RISCDIAG_OBJECT_NONE == sys_off)
                        sys_off = sys_off_string;
                }

                /* end of loop for structure */
                break;
            }

            /* fix up the group even if error (iff created) */
            if(GR_RISCDIAG_OBJECT_NONE != sys_off_group)
                gr_riscdiag_group_end(p_gr_riscdiag, &sys_off_group);

            break;
            }

        case GR_DIAG_OBJTYPE_RECTANGLE:
            {
            GR_BOX box;
            DRAW_BOX draw_box;
            GR_LINESTYLE linestyle;
            GR_FILLSTYLE fillstyle;

            box.x0 = pObject.rect->pos.x;
            box.y0 = pObject.rect->pos.y;
            box.x1 = pObject.rect->pos.x + pObject.rect->size.cx;
            box.y1 = pObject.rect->pos.y + pObject.rect->size.cy;

            linestyle = pObject.rect->linestyle;
            fillstyle = pObject.rect->fillstyle;

            draw_box_from_gr_box(&draw_box, &box);

            status = gr_riscdiag_rectangle_new(p_gr_riscdiag, &sys_off, &draw_box, &linestyle, &fillstyle);

            break;
            }

        case GR_DIAG_OBJTYPE_LINE:
            {
            GR_POINT pos, offset;
            DRAW_POINT draw_point, draw_offset;
            GR_LINESTYLE linestyle;

            pos = pObject.line->pos;
            offset = pObject.line->offset;

            linestyle = pObject.line->linestyle;

            draw_point_from_gr_point(&draw_point, &pos);
            draw_point_from_gr_point(&draw_offset, &offset);

            status = gr_riscdiag_line_new(p_gr_riscdiag, &sys_off, &draw_point, &draw_offset, &linestyle);

            break;
            }

        case GR_DIAG_OBJTYPE_QUADRILATERAL:
            {
            GR_POINT pos, offset1, offset2, offset3;
            DRAW_POINT draw_point, draw_offset1, draw_offset2, draw_offset3;
            GR_LINESTYLE linestyle;
            GR_FILLSTYLE fillstyle;

            pos = pObject.quad->pos;
            offset1 = pObject.quad->offset1;
            offset2 = pObject.quad->offset2;
            offset3 = pObject.quad->offset3;

            linestyle = pObject.quad->linestyle;
            fillstyle = pObject.quad->fillstyle;

            draw_point_from_gr_point(&draw_point, &pos);
            draw_point_from_gr_point(&draw_offset1, &offset1);
            draw_point_from_gr_point(&draw_offset2, &offset2);
            draw_point_from_gr_point(&draw_offset3, &offset3);

            status = gr_riscdiag_quadrilateral_new(p_gr_riscdiag, &sys_off, &draw_point, &draw_offset1, &draw_offset2, &draw_offset3, &linestyle, &fillstyle);

            break;
            }

        case GR_DIAG_OBJTYPE_PIESECTOR:
            {
            GR_POINT pos;
            GR_COORD       radius;
            DRAW_POINT     draw_point;
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

            draw_point_from_gr_point(&draw_point, &pos);
            draw_radius = gr_riscDraw_from_pixit(radius);

            status = gr_riscdiag_piesector_new(p_gr_riscdiag, &sys_off, &draw_point, draw_radius, &alpha, &beta, &linestyle, &fillstyle);

            break;
            }

        case GR_DIAG_OBJTYPE_PICTURE:
            {
            GR_BOX box;
            DRAW_BOX draw_box;
            IMAGE_CACHE_HANDLE picture;
            GR_FILLSTYLE fillstyle;
            P_ANY diag;

            /* a representation of 'nothing' */

            box.x0 = pObject.pict->pos.x;
            box.y0 = pObject.pict->pos.y;
            box.x1 = pObject.pict->pos.x + pObject.pict->size.cx;
            box.y1 = pObject.pict->pos.y + pObject.pict->size.cy;

            picture   = pObject.pict->picture;

            fillstyle = pObject.pict->fillstyle;

            diag = image_cache_loaded_ensure(&picture);

            draw_box.x0 = gr_riscDraw_from_pixit(box.x0);
            draw_box.y0 = gr_riscDraw_from_pixit(box.y0);
            draw_box.x1 = gr_riscDraw_from_pixit(box.x1);
            draw_box.y1 = gr_riscDraw_from_pixit(box.y1);

            if(diag)
                status = gr_riscdiag_scaled_diagram_add(p_gr_riscdiag, &sys_off, &draw_box, diag, &fillstyle);

            break;
            }

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
    xstrkpy(f.szHostFontName, sizeof32(f.szHostFontName), _sbstr_from_tstr(pszFontName));

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
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + strlen32p1(pFontListElem->szHostFontName); /* for CH_NULL */
            GR_RISCDIAG_RISCOS_FONTLIST_ENTRY f;

            zero_struct(f);
            xstrkpy(f.szHostFontName, sizeof32(f.szHostFontName), pFontListElem->szHostFontName);

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
                IMAGE_CACHE_HANDLE picture;
                P_DRAW_DIAG diag;

                memcpy32(&pict, pObject.pict, sizeof32(pict));

                picture = pict.picture;

                diag = image_cache_loaded_ensure(&picture);

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

    zero_struct(process);
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
            tstr_xstrkpy(pDiagHdr->szCreatorName, elemof32(pDiagHdr->szCreatorName), szCreatorName);
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
    _InoutRef_  P_GR_DIAG p_gr_diag,
    GR_DIAG_PROCESS_T process)
{
    P_GR_DIAG_DIAGHEADER pDiagHdr;
    GR_BOX diag_box;

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
    _InVal_     GR_DIAG_OBJID_T objid)
{
    GR_DIAG_PROCESS_T process;

    zero_struct(process);
    process.recurse    = 1;

    return(gr_diag_object_search_between(p_gr_diag, objid,
                                         GR_DIAG_OBJECT_FIRST,
                                         GR_DIAG_OBJECT_LAST,
                                         process));
}

#endif

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

    case GR_DIAG_OBJTYPE_PIESECTOR:
        return(sizeof(GR_DIAG_OBJPIESECTOR));

    case GR_DIAG_OBJTYPE_PICTURE:
        return(sizeof(GR_DIAG_OBJPICTURE));

    case GR_DIAG_OBJTYPE_QUADRILATERAL:
        return(sizeof(GR_DIAG_OBJQUADRILATERAL));

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

extern BOOL
gr_diag_object_correlate_between(
    P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size,
    P_GR_DIAG_OFFSET pHitObject /*[]out*/,
    S32 recursionLimit,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in)
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
        BOOL hit;

        { /* block to reduce recursion stack overhead */
        /* find next object to process: scan up from start till we meet current object */
        GR_DIAG_OFFSET findObject = sttObject;
        P_GR_DIAG_OBJECT pObject;

        pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, findObject);

        for(;;)
        {
            objectType = pObject.hdr->tag;
            objectSize = pObject.hdr->n_bytes;

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

        /* refine hit where this is easy enough to do */
        switch(pObject.hdr->tag)
        {
        case GR_DIAG_OBJTYPE_LINE:
            hit = gr_diag_line_hit(p_gr_diag, findObject, point, size);
            break;

        case GR_DIAG_OBJTYPE_PIESECTOR:
            hit = gr_diag_piesector_hit(p_gr_diag, findObject, point, size);
            break;

        case GR_DIAG_OBJTYPE_QUADRILATERAL:
            hit = gr_diag_quadrilateral_hit(p_gr_diag, findObject, point, size);
            break;

        default:
            /* simple test to see whether we hit this object */
            hit = gr_box_hit(&pObject.hdr->bbox, point, size);
            break;
        }

        if(!hit)
            continue; /* loop for next object */
        } /*block*/

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
             */

            /* SKS after PD 4.12 12feb92 - must limit search to the part of this group spanned by stt...end */
            hit = gr_diag_object_correlate_between(p_gr_diag, point, size,
                                                   pHitObject + 1, /* will be poked at a finer level */
                                                   recursionLimit - 1,
                                                   thisObject + sizeof(GR_DIAG_OBJGROUP),
                                                   MIN(thisObject + objectSize, endObject));

            if(!hit)
            {   /* kill group hit, keep terminated */
                pHitObject[0] = GR_DIAG_OBJECT_NONE;
                continue; /* loop for next object */
            }
        }

        return(hit);
    }

    return(FALSE);
}

/******************************************************************************
*
* end an object: go back and patch its size field
*
******************************************************************************/

extern U32
gr_diag_object_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
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
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("object_end: destroying type ") U32_TFMT TEXT(" n_bytes ") U32_XTFMT TEXT(" at ") U32_XTFMT, pObject.hdr->tag, n_bytes, *pObjectStart);
        n_bytes = 0;
    }
    else
        /* update object size */
        pObject.hdr->n_bytes = n_bytes;

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
    _InVal_     GR_DIAG_OBJID_T objid,
    _OutRef_    P_P_GR_DIAG_OBJECT ppObject,
    _InVal_     GR_DIAG_OBJTYPE objectType,
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

    pObject.hdr->tag = objectType;
    pObject.hdr->n_bytes = (int) allocBytes;
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
    U32 n_bytes;

    pObject.p_byte = gr_diag_getoffptr(BYTE, p_gr_diag, *pSttObject);

    n_bytes = pObject.hdr->n_bytes;

    /* force scans to be linear, not recursive */
    if(pObject.hdr->tag == GR_DIAG_OBJTYPE_GROUP)
        n_bytes = sizeof32(GR_DIAG_OBJGROUP);

    myassert2x(n_bytes >= sizeof32(GR_DIAG_OBJHDR),
               TEXT("gr_diag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof(*pObject.hdr)"), *pSttObject, n_bytes);
    myassert3x(*pSttObject + n_bytes <= array_elements32(&p_gr_diag->handle),
               TEXT("gr_diag_diagram_render object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" > p_gr_diag->offset ") U32_XTFMT, *pSttObject, n_bytes, array_elements32(&p_gr_diag->handle));

    *pSttObject += n_bytes;

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
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_    P_GR_BOX pBox,
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
        objectSize = pObject.hdr->n_bytes;
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

            assert(GR_RISCDIAG_OBJECT_NONE != pObject.hdr->sys_off);
            if(GR_RISCDIAG_OBJECT_NONE != pObject.hdr->sys_off)
            {   /* merely copy over bbox info from corresponding object in Draw file */
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
    _InVal_     GR_DIAG_OBJID_T objid,
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
        objectSize = pObject.hdr->n_bytes;
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
* end a group: go back and patch its size field (leave bbox till diag end)
*
******************************************************************************/

extern U32
gr_diag_group_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    PC_GR_DIAG_OFFSET pGroupStart)
{
    return(gr_diag_object_end(p_gr_diag, pGroupStart));
}

/******************************************************************************
*
* start a group: leave size & bbox empty to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_group_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    _InVal_     GR_DIAG_OBJID_T objid)
{
    P_GR_DIAG_OBJECT pObject;

    return(gr_diag_object_new(p_gr_diag, pGroupStart, objid, &pObject, GR_DIAG_OBJTYPE_GROUP, 0));
}

/******************************************************************************
*
* add a line as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_line_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_LINE, 0));

    pObject.line->pos = *pPos;

    pObject.line->offset = *pOffset;

    pObject.line->linestyle = *linestyle;

    return(STATUS_OK);
}

/* NB this refining test presumes the bounding box test has passed */

_Check_return_
static BOOL
line_hit(
    _InRef_     PC_GR_POINT line_p1,
    _InRef_     PC_GR_POINT line_p2,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const GR_COORD Tx = point->x;
    const GR_COORD Ty = point->y;
    const GR_COORD Ex = size->cx;
    const GR_COORD Ey = size->cy;
    /* x3 = Tx + Ex; x4 = Tx - Ex; */
    /* y3 = Ty + Ey; y4 = Ty - Ey; */
    const GR_COORD x1 = line_p1->x;
    const GR_COORD y1 = line_p1->y;
    const GR_COORD x2 = line_p2->x;
    const GR_COORD y2 = line_p2->y;
    const GR_COORD dx = x1 - x2;
    const GR_COORD dy = y1 - y2;
    GR_COORD Px, Py;

    /* where does the line (x1,y1) (x2,y2) pass through the horizontal line defined by Ty? */
    if(0 == dy)
    {   /* this line is horizontal */
        Py = y1;

        if(Py < (Ty - Ey))
            return(FALSE);
        if(Py > (Ty + Ey))
            return(FALSE);
    }
    else
    {
        Px = ((Ty * dx) - (x1 * y2 - y1 * x2)) / dy;

        if(Px < (Tx - Ex))
            return(FALSE);
        if(Px > (Tx + Ex))
            return(FALSE);
    }

    /* where does the line (x1,y1) (x2,y2) pass through the vertical line defined by Tx? */
    if(0 == dx)
    {   /* this line is vertical */
        /* if dx == dy == 0 it is a point, but these combined tests effectively do point-in-box test */
        Px = x1;

        if(Px < (Tx - Ex))
            return(FALSE);
        if(Px > (Tx + Ex))
            return(FALSE);
    }
    else
    {
        Py = ((Tx * dy) + (x1 * y2 - y1 * x2)) / dx;

        if(Py < (Ty - Ey))
            return(FALSE);
        if(Py > (Ty + Ey))
            return(FALSE);
    }

    return(TRUE);
}

_Check_return_
static BOOL
gr_diag_line_hit_refine(
    const GR_DIAG_OBJLINE * const line,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    GR_POINT line_p1;
    GR_POINT line_p2 = line->pos;

    line_p1.x = line->pos.x + line->offset.x;
    line_p1.y = line->pos.y + line->offset.y;

    return(line_hit(&line_p1, &line_p2, point, size));
}

_Check_return_
static BOOL
gr_diag_line_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJLINE * const line = (const GR_DIAG_OBJLINE *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&line->bbox, point, size))
        return(FALSE);

    return(gr_diag_line_hit_refine(line, point, size));
}

/******************************************************************************
*
* add a parallelogram as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_parallelogram_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_QUADRILATERAL, 0));

    pObject.quad->pos       = *pPos;

    pObject.quad->offset1   = *pOffset1;
    pObject.quad->offset2   = *pOffset2;

    pObject.quad->offset3.x = pOffset2->x - pOffset1->x;
    pObject.quad->offset3.y = pOffset2->y - pOffset1->y;

    pObject.quad->linestyle = *linestyle;
    pObject.quad->fillstyle = *fillstyle;

    return(STATUS_OK);
}

/******************************************************************************
*
* add a piesector as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_piesector_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InVal_     GR_COORD radius,
    _InRef_     PC_F64 alpha,
    _InRef_     PC_F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_PIESECTOR, 0));

    pObject.pie->pos       = *pPos;

    pObject.pie->radius    = radius;
    pObject.pie->alpha     = *alpha;
    pObject.pie->beta      = *beta;

    pObject.pie->linestyle = *linestyle;
    pObject.pie->fillstyle = *fillstyle;

    return(STATUS_OK);
}

_Check_return_
static BOOL
piesector_hit(
    _InRef_     PC_GR_POINT pPos,
    _InVal_     GR_COORD radius,
    _InRef_     PC_F64 alpha,
    _InRef_     PC_F64 beta,
    _InRef_     PC_GR_POINT point)
{
    const GR_COORD dx = point->x - pPos->x;
    const GR_COORD dy = point->y - pPos->y;

    { /* trivial no-trig check - is point within the bounding circle? */
    const S32 dx_2_plus_dy_2 = (dx * dx) + (dy * dy);
    const S32 radius_squared = radius * radius;
    if(dx_2_plus_dy_2 > radius_squared)
        return(FALSE);
    } /*block*/

    {
    F64 theta = atan2(dy, dx);
    assert(*alpha <= *beta);

    /*reportf(TEXT("piesector_hit 1: test %g < %g < %g"), *alpha, theta, *beta);*/
    if((theta >= *alpha) && (theta <= *beta))
        return(TRUE);

    if(theta < 0.0)
    {
        theta += _two_pi;
    }
    else
    {
        theta -= _two_pi;
    }

    /*reportf(TEXT("piesector_hit 2: test %g < %g < %g"), *alpha, theta, *beta);*/
    if((theta >= *alpha) && (theta <= *beta))
        return(TRUE);
    } /*block*/

    return(FALSE);
}

_Check_return_
static BOOL
gr_diag_piesector_hit_refine(
    const GR_DIAG_OBJPIESECTOR * const pie,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    UNREFERENCED_PARAMETER_InRef_(size);

    return(piesector_hit(&pie->pos, pie->radius, &pie->alpha, &pie->beta, point));
}

_Check_return_
static BOOL
gr_diag_piesector_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJPIESECTOR * const pie = (const GR_DIAG_OBJPIESECTOR *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&pie->bbox, point, size))
        return(FALSE);

    return(gr_diag_piesector_hit_refine(pie, point, size));
}

/******************************************************************************
*
* add a quadrilateral as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_quadrilateral_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_POINT pOffset3,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_QUADRILATERAL, 0));

    pObject.quad->pos       = *pPos;

    pObject.quad->offset1   = *pOffset1;
    pObject.quad->offset2   = *pOffset2;
    pObject.quad->offset3   = *pOffset3;

    pObject.quad->linestyle = *linestyle;
    pObject.quad->fillstyle = *fillstyle;

    return(STATUS_OK);
}

_Check_return_
static BOOL
gr_diag_quadrilateral_hit_refine(
    const GR_DIAG_OBJQUADRILATERAL * const quad,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    GR_POINT points[4 + 1];

    UNREFERENCED_PARAMETER_InRef_(size);

    points[0] = quad->pos;

    points[1].x = quad->pos.x + quad->offset1.x;
    points[1].y = quad->pos.y + quad->offset1.y;

    points[2].x = quad->pos.x + quad->offset2.x;
    points[2].y = quad->pos.y + quad->offset2.y;

    points[3].x = quad->pos.x + quad->offset3.x;
    points[3].y = quad->pos.y + quad->offset3.y;

    points[4] = points[0];

    if(0 != wn_PnPoly(point, points, elemof32(points))) /* NB NOT n */
        return(TRUE);

    { /* might be very thin, line-like, so test for proximity to edges */
    U32 i;
    for(i = 0; i < 4; ++i)
    {
        if(line_hit(&points[i], &points[i+1], point, size))
            return(TRUE);
    }
    } /*block*/

    return(FALSE);
}

_Check_return_
static BOOL
gr_diag_quadrilateral_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJQUADRILATERAL * const quad = (const GR_DIAG_OBJQUADRILATERAL *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&quad->bbox, point, size))
        return(FALSE);

    return(gr_diag_quadrilateral_hit_refine(quad, point, size));
}

/******************************************************************************
*
* add a rectangle as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_rectangle_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_RECTANGLE, 0));

    pObject.rect->pos.x = pBox->x0;
    pObject.rect->pos.y = pBox->y0;

    pObject.rect->size.cx = pBox->x1 - pBox->x0;
    pObject.rect->size.cy = pBox->y1 - pBox->y0;

    pObject.rect->linestyle = *linestyle;
    pObject.rect->fillstyle = *fillstyle;

    return(STATUS_OK);
}

/******************************************************************************
*
* add a picture (scaled into a rectangle) as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_scaled_picture_add(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    IMAGE_CACHE_HANDLE picture,
    _InRef_     PC_GR_FILLSTYLE fillstyle)
{
    P_GR_DIAG_OBJECT pObject;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_PICTURE, 0));

    pObject.pict->pos.x = pBox->x0;
    pObject.pict->pos.y = pBox->y0;

    pObject.pict->size.cx = pBox->x1 - pBox->x0;
    pObject.pict->size.cy = pBox->y1 - pBox->y0;

    pObject.pict->picture = picture;

    pObject.pict->fillstyle = *fillstyle;

    return(STATUS_OK);
}

/******************************************************************************
*
* add a string as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_text_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR szText,
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    U32 size;
    P_GR_DIAG_OBJECT pObject;

    size = strlen32p1(szText); /* includes CH_NULL */

    /* round up to output word boundary */
    size = round_up(size, 4);

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, objid, &pObject, GR_DIAG_OBJTYPE_TEXT, size));

    pObject.text->pos.x = pBox->x0;
    pObject.text->pos.y = pBox->y0;

    pObject.text->size.cx = pBox->x1 - pBox->x0;
    pObject.text->size.cy = pBox->y1 - pBox->y0;

    pObject.text->textstyle = *textstyle;

    strcpy((char *) (pObject.text + 1), szText);

    return(STATUS_OK);
}

/* end of gr_diag.c */
