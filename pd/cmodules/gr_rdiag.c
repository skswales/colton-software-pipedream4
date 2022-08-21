/* gr_rdiag.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* RISC OS Draw file creation */

/* SKS August 1991 */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#ifndef __cs_font_h
#include "cs-font.h"
#endif

#ifndef __cs_flex_h
#include "cs-flex.h"
#endif

#ifndef __fontlxtr_h
#include "cmodules/riscos/fontlxtr.h"
#endif

#ifndef GR_RISCDIAG_SIZE_INIT
#define GR_RISCDIAG_SIZE_INIT 0x1000
#endif

#ifndef GR_RISCDIAG_SIZE_INCR
#define GR_RISCDIAG_SIZE_INCR 0x1000
#endif

_Check_return_
static inline DRAW_DIAG_OFFSET
gr_riscdiag_normalise_stt(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in)
{
    DRAW_DIAG_OFFSET sttObject = sttObject_in;

    myassert0x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.length, "gr_riscdiag_normalise_stt has no diagram");
    IGNOREPARM_InRef_(p_gr_riscdiag);

    if(sttObject == GR_RISCDIAG_OBJECT_FIRST)
        sttObject = sizeof32(DRAW_FILE_HEADER);

    myassert1x(sttObject >= sizeof32(DRAW_FILE_HEADER), "gr_riscdiag_normalise_stt has sttObject " U32_XTFMT " < sizeof(DRAW_FILE_HEADER)", sttObject);
    myassert2x(sttObject <= p_gr_riscdiag->draw_diag.length, "gr_riscdiag_normalise_stt has sttObject " U32_XTFMT " > p_gr_riscdiag->draw_diag.length " U32_XTFMT, sttObject, p_gr_riscdiag->draw_diag.length);

    return(sttObject);
}

_Check_return_
static inline DRAW_DIAG_OFFSET
gr_riscdiag_normalise_end(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET endObject_in)
{
    DRAW_DIAG_OFFSET endObject = endObject_in;

    myassert0x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.length, "gr_riscdiag_normalise_end has no diagram");

    if(endObject == GR_RISCDIAG_OBJECT_LAST)
        endObject = p_gr_riscdiag->draw_diag.length;

    myassert1x(endObject >= sizeof32(DRAW_FILE_HEADER), "gr_riscdiag_normalise_end has endObject " U32_XTFMT " < sizeof(DRAW_FILE_HEADER)", endObject);
    myassert2x(endObject <= p_gr_riscdiag->draw_diag.length, "gr_riscdiag_normalise_end has endObject " U32_XTFMT " > p_gr_riscdiag->draw_diag.length " U32_XTFMT, endObject, p_gr_riscdiag->draw_diag.length);

    return(endObject);
}

/*
transforms
*/

#ifdef UNUSED

/*
scaling matrix for riscDraw units to pixits
*/

const GR_XFORMMATRIX
gr_riscdiag_pixit_from_riscDraw_xformer =
{
    /* &0000.0800 == 1/32 (inexact) */
    GR_SCALE_ONE / GR_RISCDRAW_PER_PIXIT,
        GR_SCALE_ZERO,

    GR_SCALE_ZERO,
        GR_SCALE_ONE / GR_RISCDRAW_PER_PIXIT,

    GR_SCALE_ZERO,
        GR_SCALE_ZERO
};

#endif /* UNUSED */

/*
scaling matrix for riscDraw units to OS units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscos_from_riscDraw_xformer =
{
    /* &0000.0100 == 1/256 (inexact) */
    (GR_SCALE_ONE * GR_RISCOS_PER_INCH) / GR_RISCDRAW_PER_INCH,
        GR_SCALE_ZERO,

    GR_SCALE_ZERO,
        (GR_SCALE_ONE * GR_RISCOS_PER_INCH) / GR_RISCDRAW_PER_INCH,

    GR_SCALE_ZERO,
        GR_SCALE_ZERO
};

/*
scaling matrix for pixits to riscDraw units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_pixit_xformer =
{
    /* &0020.0000 == 32 */
    GR_SCALE_ONE * GR_RISCDRAW_PER_PIXIT,
        GR_SCALE_ZERO,

    GR_SCALE_ZERO,
        GR_SCALE_ONE * GR_RISCDRAW_PER_PIXIT,

    GR_SCALE_ZERO,
        GR_SCALE_ZERO
};

#ifdef UNUSED

/*
scaling matrix for millipoints to riscDraw units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_mp_xformer =
{
    /* &0000.AD37 ~= 640/1000 (inexact) */
    (GR_SCALE_ONE * GR_RISCDRAW_PER_POINT) / GR_MILLIPOINTS_PER_POINT,
        GR_SCALE_ZERO,

    GR_SCALE_ZERO,
        (GR_SCALE_ONE * GR_RISCDRAW_PER_POINT) / GR_MILLIPOINTS_PER_POINT,

    GR_SCALE_ZERO,
        GR_SCALE_ZERO
};

#endif /* UNUSED */

extern GR_COLOUR
gr_colour_from_riscDraw(
    DRAW_COLOUR riscDraw)
{
    GR_COLOUR colour;

    * (int *) &colour = 0;

    if(riscDraw != DRAW_COLOUR_Transparent)
        {
        colour.visible  = 1;
        colour.red      = ((unsigned int) riscDraw >>  8) & 0xFF;
        colour.green    = ((unsigned int) riscDraw >> 16) & 0xFF;
        colour.blue     = ((unsigned int) riscDraw >> 24) & 0xFF;
        }

    return(colour);
}

extern DRAW_COLOUR
gr_colour_to_riscDraw(
    GR_COLOUR colour)
{
    if(!colour.visible)
        return(DRAW_COLOUR_Transparent); /* transparent (if it gets this far) */

    return((((((colour.blue) << 8) | colour.green) << 8) | colour.red) << 8);
}

/******************************************************************************
*
* delete a diagram
*
******************************************************************************/

extern void
gr_riscdiag_diagram_delete(
    P_GR_RISCDIAG p_gr_riscdiag)
{
    if(p_gr_riscdiag)
        {
        p_gr_riscdiag->dd_allocsize = 0;

        draw_diag_dispose(&p_gr_riscdiag->draw_diag);
        }
}

/******************************************************************************
*
* end adding data to a diagram
*
******************************************************************************/

extern U32
gr_riscdiag_diagram_end(
    P_GR_RISCDIAG p_gr_riscdiag)
{
    U32 n_bytes;
    GR_RISCDIAG_PROCESS_T process;

    /* close root group */
    gr_riscdiag_group_end(p_gr_riscdiag, &p_gr_riscdiag->dd_rootGroupStart);

    n_bytes = p_gr_riscdiag->draw_diag.length;

    /* reduce size of diagram to final size (never zero) */
    if(n_bytes != p_gr_riscdiag->dd_allocsize)
        {
        /* shrink */
        (void) flex_realloc(&p_gr_riscdiag->draw_diag.data, (int) n_bytes);
        p_gr_riscdiag->dd_allocsize = n_bytes;
        }

    /* always set sensible bbox */
    * (int *) &process = 0;
    process.recurse    = 1;
    process.recompute  = 1;
    gr_riscdiag_diagram_reset_bbox(p_gr_riscdiag, process);

    return(n_bytes);
}

extern void
gr_riscdiag_diagram_init(
    _OutRef_    P_DRAW_FILE_HEADER pDrawFileHdr,
    _In_z_      PC_L1STR szCreatorName)
{
    /* create the header manually */
    U32 i;

    PREFAST_ONLY_ZERO(pDrawFileHdr, sizeof32(*pDrawFileHdr));

    pDrawFileHdr->title[0] = 'D';
    pDrawFileHdr->title[1] = 'r';
    pDrawFileHdr->title[2] = 'a';
    pDrawFileHdr->title[3] = 'w';

    pDrawFileHdr->major_stamp = 201;
    pDrawFileHdr->minor_stamp = 0;

    for(i = 0; i < sizeofmemb32(DRAW_FILE_HEADER, creator_id); ++i)
    {
        U8 ch = *szCreatorName++;
        if(NULLCH == ch)
        {
            ch = ' '; /* NB program identification string must NOT be NULLCH-terminated */
            szCreatorName--; /* retract back to NULLCH */
        }
        pDrawFileHdr->creator_id[i] = ch;
    }

    draw_box_make_bad(&pDrawFileHdr->bbox);
}

/******************************************************************************
*
* start a diagram: allocate descriptor and initial chunk & initialise
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_diagram_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PC_L1STR szCreatorName,
    _InVal_     ARRAY_HANDLE array_handleR)
{
    P_DRAW_FILE_HEADER pDrawFileHdr;
    STATUS status;

    p_gr_riscdiag->draw_diag.data = NULL;
    p_gr_riscdiag->draw_diag.length = 0;
    p_gr_riscdiag->dd_allocsize = 0;

    if(NULL == (pDrawFileHdr = gr_riscdiag_ensure(DRAW_FILE_HEADER, p_gr_riscdiag, sizeof32(DRAW_FILE_HEADER), &status)))
        return(status);

    gr_riscdiag_diagram_init(pDrawFileHdr, szCreatorName);

    /* create font list object in top level - seems best for Draw renderers */
    status = STATUS_DONE;

    if(array_elements(&array_handleR) && status_done(status))
        status = gr_riscdiag_fontlistR_new(p_gr_riscdiag, &array_handleR);

    /* create root group */
    if(status_done(status))
        status = gr_riscdiag_group_new(p_gr_riscdiag, &p_gr_riscdiag->dd_rootGroupStart, "$");

    /* clean up mess if needed */
    if(status_fail(status))
        gr_riscdiag_diagram_delete(p_gr_riscdiag);

    return(status);
}

/******************************************************************************
*
* reset a diagram's bbox
*
******************************************************************************/

extern void
gr_riscdiag_diagram_reset_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    GR_RISCDIAG_PROCESS_T process)
{
    P_DRAW_FILE_HEADER pDiagHdr;
    DRAW_BOX diag_box;

    gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag,
                                          &diag_box,
                                          GR_RISCDIAG_OBJECT_FIRST, /* first object */
                                          GR_RISCDIAG_OBJECT_LAST,  /* last object */
                                          process);

    pDiagHdr = gr_riscdiag_getoffptr(DRAW_FILE_HEADER, p_gr_riscdiag, 0);

    pDiagHdr->bbox = diag_box;
}

/******************************************************************************
*
* save out a diagram
*
******************************************************************************/

extern S32
gr_riscdiag_diagram_save(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_U8 filename)
{
    FILE_HANDLE f;
    S32        res, res1;

    if((res = file_open(filename, file_open_write, &f)) <= 0)
        return(res ? res : create_error(FILE_ERR_CANTOPEN));

    file_set_type(f, FILETYPE_DRAW);

    res = gr_riscdiag_diagram_save_into(p_gr_riscdiag, f);

    res1 = file_close(&f);
    res = res ? res : res1;

    return(res ? res : 1);
}

/******************************************************************************
*
* save out a Draw file into an opened file
*
******************************************************************************/

extern S32
gr_riscdiag_diagram_save_into(
    P_GR_RISCDIAG p_gr_riscdiag,
    FILE_HANDLE file_handle)
{
    PC_BYTE pDiagHdr = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, 0);
    S32 res = file_write_err(pDiagHdr, 1, p_gr_riscdiag->draw_diag.length, file_handle);

    return(res);
}

/******************************************************************************
*
* set up a diagram from some data (usually for font scanning and such like)
*
******************************************************************************/

extern void
gr_riscdiag_diagram_setup_from_draw_diag(
    _OutRef_    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG p_draw_diag) /* flex data stolen */
{
    zero_struct_ptr(p_gr_riscdiag);

    draw_diag_give_away(&p_gr_riscdiag->draw_diag, p_draw_diag);

    p_gr_riscdiag->dd_allocsize = p_gr_riscdiag->draw_diag.length;
}

/******************************************************************************
*
* ensure enough memory for bytes to be added to diagram
*
* --out--
*
*  -ve  allocation error
*  +ve  ok
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(size)
extern P_BYTE
_gr_riscdiag_ensure(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _In_        U32 size,
    _OutRef_    P_STATUS p_status)
{
    P_BYTE result;

    *p_status = STATUS_OK;

    myassert0x(p_gr_riscdiag, "gr_riscdiag_ensure has no diagram");

    assert((size & 3) == 0);

    /* need to get more space for the diag? */
    if(size > (p_gr_riscdiag->dd_allocsize - p_gr_riscdiag->draw_diag.length))
        {
        U32 requiredsize, allocsize, allocdelta;
        P_BYTE mp;

        if(0 != p_gr_riscdiag->draw_diag.length)
            requiredsize = p_gr_riscdiag->dd_allocsize + size;
        else
            requiredsize = MAX(size, GR_RISCDIAG_SIZE_INIT);

        allocsize = round_up(requiredsize, GR_RISCDIAG_SIZE_INCR);

        allocdelta = allocsize - p_gr_riscdiag->dd_allocsize;

#if RISCOS
        if(!flex_realloc(&p_gr_riscdiag->draw_diag.data, (int) allocsize))
            *p_status = create_error(GR_CHART_ERR_NOMEM);
#endif

        if(status_fail(*p_status))
            return(NULL);

        /* point to new allocation; clear to zero.
         * depends on objects being discarded emptying their space for reuse
        */
        mp = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, p_gr_riscdiag->dd_allocsize);

        void_memset32(mp, 0, allocdelta);

        /* optionally return pointer to base of ensured allocation and skip it */
        result = mp - p_gr_riscdiag->dd_allocsize + p_gr_riscdiag->draw_diag.length;

        p_gr_riscdiag->dd_allocsize = allocsize;
        }
    else
        { /* return pointer to base of ensured allocation and skip it */
        result = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, p_gr_riscdiag->draw_diag.length);
        }

    p_gr_riscdiag->draw_diag.length += size;

    return(result);
}

/******************************************************************************
*
* search diagram's font list object for given font, returning its fontref
*
******************************************************************************/

typedef struct _draw_fontlistelem
{
    U8 fontref8;
    char name[32]; /* String, NULLCH terminated (size only for compiler and debugger - do not use sizeof()) */
}
draw_fontlistelem;

typedef union _draw_fontlistelemp
{
    char *              p_byte;
    draw_fontlistelem * elem;
}
draw_fontlistelemp;

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_     DRAW_DIAG_OFFSET fontListR,
    PC_U8 szFontName)
{
    P_DRAW_OBJECT_FONTLIST pFontList;
    DRAW_DIAG_OFFSET     nextObject, thisOffset;
    draw_fontlistelemp pFontListElem;
    size_t             thislen;
    DRAW_FONT_REF16    fontRefNum = 0; /* will have to use system font unless request matched */

    if(fontListR == GR_RISCDIAG_OBJECT_NONE)
        fontListR = p_gr_riscdiag->dd_fontListR;

    if(fontListR == GR_RISCDIAG_OBJECT_NONE)
        return(fontRefNum);

    if(!szFontName  ||  !*szFontName)
        return(fontRefNum);

    pFontList = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);

    nextObject = fontListR + pFontList->size;

    thisOffset = fontListR + sizeof32(*pFontList);

    pFontListElem.p_byte = (P_U8) (pFontList + 1);

    /* actual end of font list object data may not be word aligned */
    while(nextObject - thisOffset >= 4)
        {
        if(0 == _stricmp(pFontListElem.elem->name, szFontName))
            {
            fontRefNum = pFontListElem.elem->fontref8;
            break;
            }

        thislen = offsetof(draw_fontlistelem, name) + strlen(pFontListElem.elem->name) + 1; /* for NULLCH */

        thisOffset += thislen;

        pFontListElem.p_byte += thislen;
        }

    /* SKS after 4.11 21jan92 - try looking up our alternate font if not looking for "System" */
    if(!fontRefNum)
        {
        static S32 recursed_internally = 0;

        if(!recursed_internally && (0 != _stricmp(szFontName, "System")))
            {
            recursed_internally = 1;
            fontRefNum = gr_riscdiag_fontlist_lookup(p_gr_riscdiag, fontListR, string_lookup(GR_CHART_MSG_ALTERNATE_FONTNAME));
            recursed_internally = 0;
            }
        }

    return(fontRefNum);
}

/******************************************************************************
*
* given a font ref and a font list, return the font name
*
******************************************************************************/

extern PC_L1STR
gr_riscdiag_fontlist_name(
    P_GR_RISCDIAG p_gr_riscdiag,
    _In_        DRAW_DIAG_OFFSET fontListR,
    DRAW_FONT_REF16 fontRef)
{
    P_DRAW_OBJECT_FONTLIST pFontList;
    DRAW_DIAG_OFFSET     nextObject, thisOffset;
    draw_fontlistelemp pFontListElem;
    size_t             thislen;
    PC_U8             szFontName = NULL;

    if(fontListR == GR_RISCDIAG_OBJECT_NONE)
        fontListR = p_gr_riscdiag->dd_fontListR;

    if(!fontListR)
        return(NULL);

    pFontList = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);

    nextObject = fontListR + pFontList->size;

    thisOffset = fontListR + sizeof32(*pFontList);

    pFontListElem.p_byte = (P_U8) (pFontList + 1);

    /* actual unpadded end of font list object data may not be word aligned */
    while(nextObject - thisOffset >= 4)
        {
        if(pFontListElem.elem->fontref8 == fontRef)
            {
            szFontName = pFontListElem.elem->name;
            break;
            }

        thislen = offsetof(draw_fontlistelem, name) + strlen(pFontListElem.elem->name) + 1; /* for NULLCH */

        thisOffset += thislen;

        pFontListElem.p_byte += thislen;
        }

    return(szFontName);
}

/******************************************************************************
*
* create a font list object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_fontlistR_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    DRAW_DIAG_OFFSET fontListStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(0 == array_elements(p_array_handle))
        return(STATUS_DONE);

    {
    ARRAY_INDEX i;
    U32 extraBytes = 0;

    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p = array_ptr(p_array_handle, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, i);
        const DRAW_DIAG_OFFSET lenp1 = strlen32p1(p->szHostFontName); /* for NULLCH */
        const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;

        extraBytes += thislen;
    }

    /* round up size to 32bit boundary */
    extraBytes = (extraBytes + (4-1)) & ~(4-1);

    {
    STATUS status;
    P_BYTE pObject;
    DRAW_OBJECT_HEADER_NO_BBOX objhdr;

    objhdr.type = DRAW_OBJECT_TYPE_FONTLIST;
    objhdr.size = sizeof32(objhdr) + extraBytes;
    /* this doesn't have a bbox */

    if(NULL == (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, objhdr.size, &status)))
        return(status);

    void_memcpy32(pObject, &objhdr, sizeof32(objhdr));
    } /*block*/
    } /*block*/

    {
    ARRAY_INDEX i;
    DRAW_DIAG_OFFSET fontListPos = fontListStart + sizeof32(DRAW_OBJECT_HEADER_NO_BBOX);

    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p = array_ptr(p_array_handle, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, i);
        const DRAW_DIAG_OFFSET lenp1 = strlen32p1(p->szHostFontName); /* for NULLCH */
        const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;
        P_DRAW_FONTLIST_ELEM pFontListElem = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, fontListPos);

        pFontListElem->fontref8 = (U8) (i + 1);

        void_memcpy32(pFontListElem->szHostFontName, p->szHostFontName, lenp1);

        fontListPos += thislen;
    }

    { /* pad up to 3 bytes with 0 */
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, fontListPos);
    while((uintptr_t) pObject & (4-1))
        *pObject++ = NULLCH;
    } /*block*/
    } /*block*/

    p_gr_riscdiag->dd_fontListR = fontListStart;

    return(STATUS_DONE);
}

/******************************************************************************
*
* search top level only of part of diagram for font list object
* note that it sets it too
*
******************************************************************************/

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_fontlist_scan(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in)
{
    DRAW_DIAG_OFFSET sttObject = sttObject_in;
    DRAW_DIAG_OFFSET endObject = endObject_in;
    P_DRAW_OBJECT pObject;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE))
        do  {
            if(pObject.hdr->type == DRAW_OBJECT_TYPE_FONTLIST)
                {
                p_gr_riscdiag->dd_fontListR = sttObject;

                return(sttObject);
                }
            }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE));

    return(GR_RISCDIAG_OBJECT_NONE);
}

/******************************************************************************
*
* end a group: go back and patch its size field (leave bbox till diag end)
*
******************************************************************************/

extern U32
gr_riscdiag_group_end(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_DRAW_DIAG_OFFSET pGroupStart)
{
    U32 nBytes;

    nBytes = gr_riscdiag_object_end(p_gr_riscdiag, pGroupStart);

    return(nBytes);
}

/******************************************************************************
*
* start a group: leave size & bbox empty to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_group_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pGroupStart,
    _In_opt_z_  PC_L1STR pGroupName)
{
    P_DRAW_OBJECT pObject;
    STATUS status;
    PC_USTR src;
    P_U8 dst;
    U8 ch;
    U32 i;

    if(NULL == (pObject.p_byte = gr_riscdiag_object_new(p_gr_riscdiag, pGroupStart, DRAW_OBJECT_TYPE_GROUP, 0, &status)))
        return(status);

    trace_3(TRACE_MODULE_GR_CHART, "gr_riscdiag_group_new(&%p) offset %d, name %s\n",
            report_ptr_cast(p_gr_riscdiag), pGroupStart ? *pGroupStart : 0, trace_string(pGroupName));

    /* fill name, padding at end with spaces (mustn't be NULLCH terminated) */
    src = pGroupName;
    dst = PtrAddBytes(P_U8, pObject.p_byte, offsetof32(DRAW_OBJECT_GROUP, name));

    for(i = 0; i < sizeofmemb32(DRAW_OBJECT_GROUP, name); i++)
    {
        if(NULL == src)
            ch = CH_SPACE;
        else
        {
            ch = *src;

            if(ch == NULLCH)
                ch = CH_SPACE;
            else
                ++src;
        }

        *dst++ = ch;
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* return amount of space to be allocated for the header of this object
*
******************************************************************************/

extern U32
gr_riscdiag_object_base_size(
    U32 type)
{
    switch(type)
        {
        case DRAW_OBJECT_TYPE_FONTLIST:
            return(sizeof32(DRAW_OBJECT_FONTLIST));

        case DRAW_OBJECT_TYPE_TEXT:
            return(sizeof32(DRAW_OBJECT_TEXT));

        case DRAW_OBJECT_TYPE_PATH:
            return(sizeof32(DRAW_OBJECT_PATH));

        case DRAW_OBJECT_TYPE_GROUP:
            return(sizeof32(DRAW_OBJECT_GROUP));

        default:
            return(sizeof32(DRAW_OBJECT_HEADER));
        }
}

/******************************************************************************
*
* end an object: go back and patch its size field
*
******************************************************************************/

extern U32
gr_riscdiag_object_end(
    P_GR_RISCDIAG p_gr_riscdiag,
    PC_DRAW_DIAG_OFFSET pObjectStart)
{
    P_DRAW_OBJECT pObject;
    U32 n_bytes;

    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, *pObjectStart);

    n_bytes = p_gr_riscdiag->draw_diag.length - *pObjectStart;

    trace_4(TRACE_MODULE_GR_CHART, "gr_riscdiag_object_end(&%p): started %d ended %d nBytes %d\n",
            report_ptr_cast(p_gr_riscdiag), *pObjectStart, p_gr_riscdiag->draw_diag.length, n_bytes);

    if(n_bytes == gr_riscdiag_object_base_size(pObject.hdr->type))
        {
        trace_0(TRACE_MODULE_GR_CHART, "gr_riscdiag_object_end: destroy object and zero contents (for reuse) as nothing in it\n");
        void_memset32(pObject.p_byte, 0, n_bytes);
        p_gr_riscdiag->draw_diag.length = *pObjectStart;
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

extern S32
gr_riscdiag_object_first(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InoutRef_  P_DRAW_DIAG_OFFSET pEndObject,
    P_P_DRAW_OBJECT ppObject /*out*/,
    S32 recurse)
{
    P_DRAW_OBJECT pObject;
    DRAW_DIAG_OFFSET thisObject;
    U32 objectType;
    U32 objectSize;

    *pSttObject = gr_riscdiag_normalise_stt(p_gr_riscdiag, *pSttObject);
    *pEndObject = gr_riscdiag_normalise_end(p_gr_riscdiag, *pEndObject);

    /* ppObject contains no info here */

    thisObject = *pSttObject;

    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

    /* force scans to be linear, not recursive */
    /* see comments in gr_riscdiag_object_next too ... */
    objectType = pObject.hdr->type;
    objectSize = ((objectType == DRAW_OBJECT_TYPE_GROUP) && recurse)
                             ? sizeof(DRAW_OBJECT_GROUP)
                             : pObject.hdr->size;

    myassert2x((objectSize >= sizeof(*pObject.hdr)) ||
               ((objectType == DRAW_OBJECT_TYPE_FONTLIST) && (objectSize >= sizeof(DRAW_OBJECT_FONTLIST))),
               "gr_riscdiag_object_first object " U32_XTFMT " size " U32_XTFMT " < sizeof(*pObject.hdr)",
               thisObject, objectSize);

    myassert3x(thisObject + objectSize <= p_gr_riscdiag->draw_diag.length,
               "gr_riscdiag_object_first object " U32_XTFMT " size " U32_XTFMT " larger than diagram " U32_XTFMT,
               thisObject, objectSize, p_gr_riscdiag->draw_diag.length);

    /* stay at this first object */

    if(objectType == DRAW_OBJECT_TYPE_FONTLIST)
        p_gr_riscdiag->dd_fontListR = thisObject;

    if(*pSttObject >= *pEndObject)
        {
        *pSttObject = GR_RISCDIAG_OBJECT_FIRST;
        pObject.hdr = NULL;
        }

    if(ppObject)
        /* maintain current lock as initial, now caller's responsibility */
        *ppObject = pObject;

    return(*pSttObject != GR_RISCDIAG_OBJECT_FIRST); /* NOT DRAW_FILE_HEADER!!! note above repoke^^^ on end */
}

/******************************************************************************
*
* start an object: init size, leave bbox bad, to be patched later
*
******************************************************************************/

extern P_BYTE
gr_riscdiag_object_new(
    P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pObjectStart,
    U32 type,
    U32 extraBytes,
    _OutRef_    P_STATUS p_status)
{
    P_DRAW_OBJECT pObject;

    *pObjectStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    /* add on size of base object required */
    extraBytes += gr_riscdiag_object_base_size(type);

    if(NULL == (pObject.p_byte = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, extraBytes, p_status)))
        return(NULL);

    pObject.hdr->type = type;
    pObject.hdr->size = (int) extraBytes;

    /* everything bar font list has bbox */
    if(type != DRAW_OBJECT_TYPE_FONTLIST)
        draw_box_make_bad(&pObject.hdr->bbox);

    return(pObject.p_byte);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

extern S32
gr_riscdiag_object_next(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InRef_     PC_DRAW_DIAG_OFFSET pEndObject,
    P_P_DRAW_OBJECT ppObject /*inout*/,
    S32 recurse)
{
    P_DRAW_OBJECT pObject;
    DRAW_DIAG_OFFSET thisObject;
    U32 objectType;
    U32 objectSize;

    /*gr_riscdiag_normalise done by _first, so unnecessary*/

    if(ppObject)
        pObject = *ppObject;
    else
        pObject.hdr = NULL;

    thisObject = *pSttObject;

    if(!pObject.hdr)
        pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

    /* force scans to be linear, not recursive */
    /* note however that the recurse flag merely allows entry to group objects - there is no recursion */
    objectType = pObject.hdr->type;
    objectSize = ((objectType == DRAW_OBJECT_TYPE_GROUP) && recurse)
                             ? sizeof(DRAW_OBJECT_GROUP)
                             : pObject.hdr->size;

    myassert2x((objectSize >= sizeof(DRAW_OBJECT_HEADER)) ||
               ((objectType == DRAW_OBJECT_TYPE_FONTLIST) && (objectSize >= sizeof(DRAW_OBJECT_FONTLIST))),
               "gr_riscdiag_object_next object " U32_XTFMT " size " U32_XTFMT " < sizeof(*pObject.hdr)",
               thisObject, objectSize);

    myassert3x(thisObject + objectSize <= p_gr_riscdiag->draw_diag.length,
               "gr_riscdiag_object_next object " U32_XTFMT " size " U32_XTFMT " larger than diagram " U32_XTFMT,
               thisObject, objectSize, p_gr_riscdiag->draw_diag.length);

    if(objectType == DRAW_OBJECT_TYPE_FONTLIST)
        p_gr_riscdiag->dd_fontListR = thisObject;

    /* skip current object, move to next */

    *pSttObject = thisObject + objectSize;
    pObject.p_byte += objectSize;

    if(*pSttObject >= *pEndObject)
        {
        *pSttObject = GR_RISCDIAG_OBJECT_FIRST;
        pObject.hdr = NULL;
        }

    if(ppObject)
        /* maintain current lock, still caller's responsibility */
        *ppObject = pObject;

    return(*pSttObject != GR_RISCDIAG_OBJECT_FIRST); /* NOT DRAW_FILE_HEADER!!! note above repoke^^^ on end */
}

/******************************************************************************
*
* recompute the bbox of an object - fairly limited
*
******************************************************************************/

extern void
gr_riscdiag_object_recompute_bbox(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart)
{
    P_DRAW_OBJECT pObject;
    U32 objectType;

    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, objectStart);

    objectType = pObject.hdr->type;

    switch(objectType)
        {
        case DRAW_OBJECT_TYPE_TEXT:
            gr_riscdiag_string_recompute_bbox(p_gr_riscdiag, objectStart);
            break;

        case DRAW_OBJECT_TYPE_PATH:
            gr_riscdiag_path_recompute_bbox(p_gr_riscdiag, objectStart);
            break;

        case DRAW_OBJECT_TYPE_SPRITE:
            /* sprite bbox defines its size not vice versa */
            /* however it may be forced bad, in which case we shall make default size */
            gr_riscdiag_sprite_recompute_bbox(p_gr_riscdiag, objectStart);
            break;

        case DRAW_OBJECT_TYPE_JPEG:
            /* sprite comments apply too */
            gr_riscdiag_jpeg_recompute_bbox(p_gr_riscdiag, objectStart);
            break;

        default:
            break;
        }
}

/******************************************************************************
*
* reset a bbox over a range of objects
*
******************************************************************************/

extern void
gr_riscdiag_object_reset_bbox_between(
    P_GR_RISCDIAG p_gr_riscdiag,
    P_DRAW_BOX pBox /*out*/,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in,
    GR_RISCDIAG_PROCESS_T process)
{
    P_DRAW_OBJECT pObject;
    DRAW_DIAG_OFFSET thisObject = gr_riscdiag_normalise_stt(p_gr_riscdiag, sttObject_in);
    const DRAW_DIAG_OFFSET endObject = gr_riscdiag_normalise_end(p_gr_riscdiag, endObject_in);
    U32 objectType;
    U32 objectSize;

    myassert2x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.data, "gr_riscdiag_object_reset_bbox_between has no diagram &%p->&%p",
               p_gr_riscdiag, p_gr_riscdiag ? p_gr_riscdiag->draw_diag.data : NULL);

    /* loop over constituent objects and extract bbox */

    draw_box_make_bad(pBox);

    /* keep pObject valid for as long as we can, dropping and reloading where needed */
    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

    while(thisObject < endObject)
        {
        objectType = pObject.hdr->type;
        objectSize = pObject.hdr->size;

        myassert2x((objectSize >= sizeof(*pObject.hdr)) ||
                   ((objectType == DRAW_OBJECT_TYPE_FONTLIST) && (objectSize >= sizeof(DRAW_OBJECT_FONTLIST))),
                   "gr_riscdiag_object_reset_bbox_between object " U32_XTFMT " size " U32_XTFMT " < sizeof(*pObject.hdr)",
                   thisObject, objectSize);

        myassert3x(thisObject + objectSize <= p_gr_riscdiag->draw_diag.length,
                   "gr_riscdiag_object_reset_bbox_between object " U32_XTFMT " size " U32_XTFMT " larger than diagram " U32_XTFMT,
                   thisObject, objectSize, p_gr_riscdiag->draw_diag.length);

        /* font tables do not have bboxes - beware! */
        switch(objectType)
            {
            case DRAW_OBJECT_TYPE_FONTLIST:
                p_gr_riscdiag->dd_fontListR = thisObject;
                break;

            case DRAW_OBJECT_TYPE_GROUP:
                /* NB. groups skipped wholesale (assumed correct unless recursing) */
                if(process.recurse)
                    {
                    DRAW_BOX group_box;

                    gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag,
                                                          &group_box,
                                                          thisObject + sizeof(DRAW_OBJECT_GROUP),
                                                          thisObject + objectSize,
                                                          process);

                    if(process.recompute)
                        pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

                    pObject.hdr->bbox = group_box;
                    }

                draw_box_union(pBox, NULL, &pObject.hdr->bbox);
                break;

            case DRAW_OBJECT_TYPE_TAG:
                {
                /* can't do anything with tagged object goop bar skip it - for bboxing,
                 * take bbox of tagged object (not the tag), skipping header
                */
                U32 tagHdrSize = sizeof(DRAW_OBJECT_TAG);

                /* test for PRM or Draw conformance */
                if(((const DRAW_OBJECT_TAG *) pObject.p_byte)->intTag <= 9 /*TEXTAREA*/)
                    tagHdrSize -= 4;

                draw_box_union(pBox, NULL, &PtrAddBytes(P_DRAW_OBJECT_HEADER, pObject.p_byte, tagHdrSize)->bbox);
                }
                break;

            case DRAW_OBJECT_TYPE_OPTIONS:
                /* this seems to get in the way somewhat */
                break;

            default:
                if(process.recompute)
                    {
                    gr_riscdiag_object_recompute_bbox(p_gr_riscdiag, thisObject);

                    pObject.p_byte = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);
                    }

                draw_box_union(pBox, NULL, &pObject.hdr->bbox);
                break;
            }

        thisObject    += objectSize;
        pObject.p_byte += objectSize;
        }
}

extern STATUS
draw_do_render(
    void * data,
    int length,
    int x,
    int y,
    F64 xfactor,
    F64 yfactor,
    P_GDI_BOX graphics_window)
{
    /* use Acorn Drawfile module (but don't link with o.Drawfile) */
    _kernel_swi_regs rs;
    int flags = 0;
    int trfm[6];
    int flatness;
    STATUS status = STATUS_OK;

    trfm[0] = (int) (xfactor * GR_SCALE_ONE);
    trfm[1] = 0;
    trfm[2] = 0;
    trfm[3] = (int) (yfactor * GR_SCALE_ONE);
    trfm[4] = x << 8;
    trfm[5] = y << 8;

    /* no DrawFiles recommendation. Draw module recommends 2 OS units */
    flatness = (int) muldiv64(2 * GR_RISCDRAW_PER_RISCOS, (int) GR_SCALE_ONE, (int) (yfactor * GR_SCALE_ONE));

    rs.r[0] = flags;
    rs.r[1] = (int) data;
    rs.r[2] = length;
    rs.r[3] = (int) &trfm;
    rs.r[4] = (int) graphics_window;
    rs.r[5] = flatness;

    (void) _kernel_swi(/*DrawFile_Render*/ 0x45540, &rs, &rs);

    return(status);
}

/* end of gr_rdiag.c */
