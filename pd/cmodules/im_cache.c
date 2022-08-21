/* im_cache.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that handles image file cache */

/* SKS 12-Sep-1991 */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#include "cmodules/typepack.h"

#include "cs-flex.h"

/*
internal structure
*/

/*
entries in the image file cache
*/

typedef struct IMAGE_CACHE
{
    BOOL autokill;
    S32 refs;

    STATUS error;

    P_DRAW_DIAG p_draw_diag;

    struct IMAGE_CACHE_RECACHE
    {
        image_cache_recache_proc proc; /* make a funclist list sometime */
        P_ANY handle;
    }
    recache;

    /* name follows here at (p_image_cache + 1) */
}
IMAGE_CACHE, * P_IMAGE_CACHE;

/*
callback function
*/

gr_riscdiag_tagstrip_proto(static, image_cache_tagstrippers_call);

/*
internal functions
*/

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_entry_new(
    _In_z_      PCTSTR name,
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _OutRef_    P_STATUS p_status);

_Check_return_
static STATUS
image_cache_load(
    P_IMAGE_CACHE p_image_cache,
    LIST_ITEMNO key);

static void
image_cache_rebind_and_shift(
    P_IMAGE_CACHE p_image_cache);

/*
extra data stored by funclist for us to understand tagstrippers
*/

typedef struct IMAGE_CACHE_TAGSTRIP_EXTRADATA
{
    U32 tag;
}
IMAGE_CACHE_TAGSTRIP_EXTRADATA;
/* never have pointers to this data - always use funclist to access */

/*
the cache descriptor list
*/

static NLISTS_BLK
image_cache =
{
    NULL,
    (sizeof(IMAGE_CACHE) + BUF_MAX_PATHSTRING),
    (sizeof(IMAGE_CACHE) + BUF_MAX_PATHSTRING) * 16
};

#define image_cache_search_key(key) \
    collect_goto_item(IMAGE_CACHE, &image_cache.lbr, (key))

/*
the interested tagstrip list
*/

static P_LIST_BLOCK
image_cache_tagstrippers = NULL;

/******************************************************************************
*
* reset a DRAW_DIAG
*
******************************************************************************/

extern void
draw_diag_dispose(
    _Inout_     P_DRAW_DIAG p_draw_diag)
{
    p_draw_diag->length = 0;
    flex_dispose(&p_draw_diag->data);
}

extern void
draw_diag_give_away(
    _Out_       P_DRAW_DIAG p_draw_diag_to,
    _Inout_     P_DRAW_DIAG p_draw_diag_from)
{
    U32 length;

    length = p_draw_diag_from->length;
    p_draw_diag_from->length = 0;
    p_draw_diag_to->length = length;

    flex_give_away(&p_draw_diag_to->data, &p_draw_diag_from->data);
}

static void
draw_diag_reset_length(
    _Inout_     P_DRAW_DIAG p_draw_diag)
{
    p_draw_diag->length = flex_size_maybe_null(&p_draw_diag->data);
}

/******************************************************************************
*
* query of RISC OS filetypes that might sensibly be loaded
*
******************************************************************************/

_Check_return_
static BOOL
image_cache_can_import_without_conversion(
    _InVal_     FILETYPE_RISC_OS filetype)
{
    switch(filetype)
    {
    case FILETYPE_PIPEDREAM: /* may be legacy chart format */
    case FILETYPE_PD_CHART:
    case FILETYPE_T5_DRAW:
    case FILETYPE_DRAW:
    case FILETYPE_SPRITE:
    case FILETYPE_POSTER:
    case FILETYPE_VECTOR:
    case FILETYPE_JPEG:
        return(TRUE);

    default:
        return(FALSE);
    }
}

_Check_return_
extern BOOL
image_cache_can_import(
    _InVal_     FILETYPE_RISC_OS filetype)
{
    if(image_cache_can_import_without_conversion(filetype))
        return(TRUE);

    if(image_convert_can_convert(filetype))
        return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* given the details of a draw file, make sure an entry for it is in the cache
*
* --out--
* -ve = error
*   1 = entry exists
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_entry_ensure(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PC_U8Z name)
{
    P_IMAGE_CACHE p_image_cache;
    P_DRAW_DIAG p_draw_diag;
    STATUS status;

    if(image_cache_entry_query(p_image_cache_handle, name))
        return(STATUS_DONE);

    if(!file_is_file(name))
        return(create_error(FILE_ERR_NOTFOUND));

    /* NB keep separate as it contains a flex anchor and listed data may move */
    if(NULL == (p_draw_diag = al_ptr_calloc_elem(DRAW_DIAG, 1, &status)))
        return(status);

    if(NULL == (p_image_cache = image_cache_entry_new(name, p_image_cache_handle, &status)))
    {
        al_ptr_free(p_draw_diag);
        return(status);
    }

    p_image_cache->p_draw_diag = p_draw_diag;

    return(STATUS_DONE);
}

/******************************************************************************
*
* create a list entry
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_entry_new(
    _In_z_      PCTSTR name,
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _OutRef_    P_STATUS p_status)
{
    static LIST_ITEMNO cahkey_gen = 0x42516000; /* NB. not tbs! */

    LIST_ITEMNO key = cahkey_gen++;
    U32 namlenp1 = tstrlen32p1(name);
    P_IMAGE_CACHE p_image_cache;

    *p_image_cache_handle = 0;

    if(NULL != (p_image_cache = collect_add_entry_bytes(IMAGE_CACHE, &image_cache, sizeof32(*p_image_cache) + (sizeof32(*name) * namlenp1), &key, p_status)))
    {
        zero_struct_ptr(p_image_cache);

        memcpy32((p_image_cache + 1), name, (sizeof32(*name) * namlenp1));

        *p_image_cache_handle = (IMAGE_CACHE_HANDLE) key;
    }

    return(p_image_cache);
}

/******************************************************************************
*
* given the details of a draw file, query whether an entry for it is in the cache
*
******************************************************************************/

_Check_return_
extern BOOL
image_cache_entry_query(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PC_U8Z name)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE p_image_cache;
    BOOL rooted = file_is_rooted(name);
    PC_U8 leaf = file_leafname(name);

    /* search for the file on the list */
    *p_image_cache_handle = IMAGE_CACHE_HANDLE_NONE;

    for(p_image_cache = collect_first(IMAGE_CACHE, &image_cache.lbr, &key);
        p_image_cache;
        p_image_cache = collect_next( IMAGE_CACHE, &image_cache.lbr, &key))
    {
        PC_U8 testname  = name;
        PCTSTR entryname = (PCTSTR) (p_image_cache + 1);

        if(!rooted)
        {
            testname  = leaf;
            entryname = file_leafname(entryname);
        }

        if(0 == _stricmp(testname, entryname))
        {
            *p_image_cache_handle = (IMAGE_CACHE_HANDLE) key;

            trace_2(TRACE_MODULE_GR_CHART, "image_cache_entry_query found file, keys: %p %d, in list", report_ptr_cast(*p_image_cache_handle), key);

            trace_0(TRACE_MODULE_GR_CHART, "image_cache_entry_query yields TRUE");
            return(TRUE);
        }
    }

    trace_0(TRACE_MODULE_GR_CHART, "image_cache_entry_query yields FALSE");
    return(FALSE);
}

static void
image_cache_entry_data_remove(
    P_IMAGE_CACHE p_image_cache)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_image_cache->p_draw_diag));
}

extern void
image_cache_entry_remove(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    P_IMAGE_CACHE p_image_cache;
    LIST_ITEMNO key;

    if(!*p_image_cache_handle)
        return;

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if(NULL == (p_image_cache = image_cache_search_key(key)))
    {
        assert0();
        return;
    }

    image_cache_entry_data_remove(p_image_cache);

    collect_subtract_entry(&image_cache.lbr, key);
}

#ifdef UNUSED

_Check_return_
extern STATUS
image_cache_entry_rename(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    _In_z_      PC_U8Z name)
{
    P_IMAGE_CACHE  p_image_cache;
    LIST_ITEMNO key;

    trace_2(TRACE_MODULE_GR_CHART, "image_cache_entry_rename(&%p->&%p)", report_ptr_cast(p_image_cache_handle), report_ptr_cast(*p_image_cache_handle));

    if(!*p_image_cache_handle)
        return(0);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        status_return(str_set(&p_image_cache->name, name));
        return(1);
    }

    return(0);
}

#endif /* UNUSED */

/******************************************************************************
*
* set state such that entry is removed when refs go to zero
*
******************************************************************************/

/*ncr*/
extern BOOL
image_cache_entry_set_autokill(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    P_IMAGE_CACHE  p_image_cache;
    LIST_ITEMNO key;

    if(!*p_image_cache_handle)
        return(0);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        p_image_cache->autokill = 1;

        return(1);
    }

    return(0);
}

_Check_return_
extern STATUS
image_cache_error_query(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;
    STATUS status = 0;

    if(!*p_image_cache_handle)
        return(status);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
        status = p_image_cache->error;

    return(status);
}


/*ncr*/
extern STATUS
image_cache_error_set(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    S32 err)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;

    if(!*p_image_cache_handle)
        return(err);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        p_image_cache->error = err;

        if(err)
            /* chuck the diagram - it's of no use now */
            draw_diag_dispose(p_image_cache->p_draw_diag);
    }

    return(err);
}

/******************************************************************************
*
* allow clients to distingish chart files from normal Draw or PD files
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_file_is_chart(
    PC_U8 name)
{
    FILE_HANDLE fin;
    STATUS status;

    if((status = file_open(name, file_open_read, &fin)) <= 0)
        return(status);

    status = image_cache_filehan_is_chart(fin);

    (void) file_close(&fin);

    return(status);
}

_Check_return_
extern STATUS
image_cache_filehan_is_chart(
    FILE_HANDLE fin)
{
    DRAW_FILE_HEADER testbuf;
    STATUS status;

    status_return(file_rewind(fin));

    if((status = file_read(&testbuf, sizeof(testbuf), 1, fin)) < 1)
        return(status);

    status_return(file_rewind(fin));

    return(image_cache_fileheader_is_chart(&testbuf, sizeof32(testbuf)));
}

_Check_return_
extern STATUS
image_cache_fileheader_is_chart(
    _In_reads_bytes_(bytesof_buffer) PC_ANY data,
    _InVal_     U32 bytesof_buffer)
{
    /* check that it looks sufficiently like one of our own Draw + App files */
    PC_DRAW_FILE_HEADER test = (PC_DRAW_FILE_HEADER) data;

    if( (bytesof_buffer < sizeof32(*test)) ||
        (0 != memcmp(test->title, "Draw", sizeofmemb(DRAW_FILE_HEADER, title))) ||
        (test->major_stamp != 201) ||
        (test->minor_stamp != 0)   ||
        (0 != memcmp(test->creator_id, GR_CHART_CREATORNAME, sizeofmemb(DRAW_FILE_HEADER, creator_id))) )
            return(0);

    return(1);
}

/******************************************************************************
*
* ensure that the cache has loaded data for this handle
*
******************************************************************************/

/*ncr*/
extern P_DRAW_DIAG
image_cache_loaded_ensure(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;

    if(!*p_image_cache_handle)
        return(NULL);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        if(NULL == p_image_cache->p_draw_diag->data)
        {
            /* let caller query any errors from this load */
            if(image_cache_load(p_image_cache, key) <= 0)
                return(NULL);
        }

        return(p_image_cache->p_draw_diag);
    }

    return(NULL);
}

/*ncr*/
extern BOOL
image_cache_name_query(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    char * buffer /*out*/,
    size_t elemof_buffer)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE   p_image_cache;

    if(!*p_image_cache_handle)
        return(0);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    p_image_cache = image_cache_search_key(key);

    if(NULL != p_image_cache)
    {
        xstrkpy(buffer, elemof_buffer, (PCTSTR) (p_image_cache + 1));
        return(TRUE);
    }

    buffer[0] = CH_NULL;
    return(FALSE);
}

/******************************************************************************
*
* when a graph is changed, the graphics program calls
* PipeDream to update any draw files it has of the graph
*
******************************************************************************/

/*ncr*/
extern STATUS
image_cache_recache(
    _In_z_      PC_U8Z drawfilename)
{
    P_IMAGE_CACHE  p_image_cache;
    LIST_ITEMNO key;
    BOOL        rooted;
    PC_U8       leaf;
    STATUS      res = 0;

    trace_1(TRACE_MODULE_GR_CHART, "image_cache_recache(%s)", drawfilename);

    if(!image_cache.lbr)
        return(0);

    rooted = file_is_rooted(drawfilename);
    leaf   = file_leafname(drawfilename);

    for(p_image_cache = collect_first(IMAGE_CACHE, &image_cache.lbr, &key);
        p_image_cache;
        p_image_cache = collect_next( IMAGE_CACHE, &image_cache.lbr, &key))
    {
        PCTSTR testname  = drawfilename;
        PCTSTR entryname = (PCTSTR) (p_image_cache + 1);

        /* if ambiguity in name, just check leafnames */
        if(!rooted)
        {
            testname  = leaf;
            entryname = file_leafname(entryname);
        }

        trace_2(TRACE_MODULE_GR_CHART, "image_cache_recache: comparing entry %s with %s", entryname, testname);

        if(0 == _stricmp(testname, entryname))
        {
            /* throwing away the Draw file seems easiest way to recache */
            p_image_cache->p_draw_diag->length = 0;
            flex_free(&p_image_cache->p_draw_diag->data);

            if((res = image_cache_load(p_image_cache, key)) < 0)
                break;
        }
    }

    return(res);
}

#ifdef UNUSED

_Check_return_
extern STATUS
image_cache_recache_key(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;
    STATUS      res = 0;

    trace_2(TRACE_MODULE_GR_CHART, "image_cache_recache_key(&%p->&%p)", report_ptr_cast(p_image_cache_handle), report_ptr_cast(p_image_cache_handle ? *p_image_cache_handle : 0));

    if(!*p_image_cache_handle)
        return(res);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        /* throwing away the Draw file seems easiest way to recache */
        draw_diag_dispose(p_image_cache->p_draw_diag);

        res = image_cache_load(p_image_cache, key);
    }

    return(res);
}

#endif /* UNUSED */

/******************************************************************************
*
* add recache upcall handler for given Draw file
*
******************************************************************************/

/*ncr*/
extern BOOL
image_cache_recache_inform(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    image_cache_recache_proc proc,
    P_ANY handle)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;
    S32         res = 0;

    trace_4(TRACE_MODULE_GR_CHART, "image_cache_recache_inform(&%p->&%p &%p, &%p)", report_ptr_cast(p_image_cache_handle), report_ptr_cast(*p_image_cache_handle), report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));

    if(!*p_image_cache_handle)
        return(res);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
    {
        p_image_cache->recache.proc   = proc;
        p_image_cache->recache.handle = handle;

        res = 1;
    }

    return(res);
}

/******************************************************************************
*
* add / remove ref for draw file
*
******************************************************************************/

/*ncr*/
extern BOOL
image_cache_ref(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    S32 add)
{
    LIST_ITEMNO key;
    P_IMAGE_CACHE  p_image_cache;

    trace_3(TRACE_MODULE_GR_CHART, "image_cache_ref(&%p->&%p, %d)", report_ptr_cast(p_image_cache_handle), report_ptr_cast(*p_image_cache_handle), add);

    if(!*p_image_cache_handle)
        return(0);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) == NULL)
        return(0);

    if(add)
    {
        ++p_image_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, "image_cache_ref: &%x refs up to %d", key, p_image_cache->refs);
    }
    else
    {
        --p_image_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, "image_cache_ref: &%x refs down to %d", key, p_image_cache->refs);
        myassert0x(p_image_cache->refs >= 0, "image_cache_ref decremented cache ref count past zero");
        if(!p_image_cache->refs)
        {
            trace_1(TRACE_MODULE_GR_CHART, "image_cache_ref: refs down to 0, so free diagram &%p (leave the entry around)", p_image_cache->p_draw_diag->data);
            draw_diag_dispose(p_image_cache->p_draw_diag);

            if(p_image_cache->autokill)
                image_cache_entry_remove(p_image_cache_handle);
        }
    }

    return(1);
}

/*ncr*/
extern BOOL
image_cache_reref(
    _InoutRef_  P_IMAGE_CACHE_HANDLE  p_image_cache_handle /*inout*/,
    _InRef_     PC_IMAGE_CACHE_HANDLE new_cahp)
{
    IMAGE_CACHE_HANDLE old_cah, new_cah;

    old_cah = *p_image_cache_handle;
    new_cah = new_cahp ? *new_cahp : IMAGE_CACHE_HANDLE_NONE;

    /* changing reference? */
    if(old_cah != new_cah)
    {
        /* remove a prior ref if there was one */
        if(old_cah != IMAGE_CACHE_HANDLE_NONE)
            image_cache_ref(p_image_cache_handle, 0);

        /* add new ref if there is one */
        if(new_cah != IMAGE_CACHE_HANDLE_NONE)
            image_cache_ref(new_cahp, 1);

        /* poke the picture ref */
        *p_image_cache_handle = new_cah;
    }

    return(1);
}

_Check_return_
extern S32
image_cache_refs(
    _In_z_      PC_U8Z name)
{
    IMAGE_CACHE_HANDLE cah;
    LIST_ITEMNO     key;
    P_IMAGE_CACHE      p_image_cache;

    if(!image_cache_entry_query(&cah, name))
        return(0);

    key = (LIST_ITEMNO) cah;

    if((p_image_cache = image_cache_search_key(key)) == NULL)
        return(0);

    return(p_image_cache->refs);
}

/******************************************************************************
*
* search draw file cache for a file
*
******************************************************************************/

/* SKS after PD 4.12 30mar92 - 0x1000 coords were too small to see on screen */

struct DUFFO_DIAGRAM
{
    DRAW_FILE_HEADER diagHdr;

    DRAW_OBJECT_PATH pathHdr;

    struct DUFFO_DIAGRAM_PATHGUTS
    {
        DRAW_PATH_MOVE  move;
        DRAW_PATH_LINE  br;
        DRAW_PATH_LINE  tr;
        DRAW_PATH_LINE  tl;
        DRAW_PATH_MOVE  bl;
        DRAW_PATH_CLOSE close1;

        DRAW_PATH_MOVE  bl2;
        DRAW_PATH_LINE  tr2;
        DRAW_PATH_CLOSE close2a;

        DRAW_PATH_MOVE  tl2;
        DRAW_PATH_LINE  br2;
        DRAW_PATH_CLOSE close2b;

        DRAW_PATH_TERM  term;
    }
    pathGuts;
};

static /*non-const - we poke it once!*/ struct DUFFO_DIAGRAM
duffo_diagram =
{ /* struct DUFFO_DIAGRAM */
      { /* diagHdr */
        /* 0    1    2    3 */
        { 'D', 'r', 'a', 'w' },
        201, /* major */
        0,   /* minor */
        /* 0    1    2    3    4    5    6    7    8    9   10   11 */
        { 'P', 'D', 'r', 'e', 'a', 'm', 'C', 'h', 'a', 'r', 't', 's' },
        { 0x0, 0x0, 0x10000, 0x10000 } /* bbox */
      },

      { /* pathHdr */
        DRAW_OBJECT_TYPE_PATH, /* tag */
        sizeof(DRAW_OBJECT_PATH) + sizeof(struct DUFFO_DIAGRAM_PATHGUTS), /* size */
        { 0x0, 0x0, 0x10000, 0x10000 }, /* bbox */
        -1, /* fillcolour */
        0,  /* pathcolour */
        0,  /* pathwidth */
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
        } /* pathstyle */
      },

      { /* pathGuts */
        { DRAW_PATH_TYPE_MOVE, { 0x0,     0x0     } }, /* move (bl) */
        { DRAW_PATH_TYPE_LINE, { 0x10000, 0x0     } }, /* br */
        { DRAW_PATH_TYPE_LINE, { 0x10000, 0x10000 } }, /* tr */
        { DRAW_PATH_TYPE_LINE, { 0x0,     0x10000 } }, /* tl */
        { DRAW_PATH_TYPE_LINE, { 0x0,     0x0     } }, /* bl */
        { DRAW_PATH_TYPE_CLOSE_WITH_LINE            }, /* close1 */

        { DRAW_PATH_TYPE_MOVE, { 0x0,     0x0     } }, /* bl2 */
        { DRAW_PATH_TYPE_LINE, { 0x10000, 0x10000 } }, /* tr2 */
        { DRAW_PATH_TYPE_CLOSE_WITH_LINE            }, /* close2a */

        { DRAW_PATH_TYPE_MOVE, { 0x0,     0x10000 } }, /* tl2 */
        { DRAW_PATH_TYPE_LINE, { 0x10000, 0x0     } }, /* br2 */
        { DRAW_PATH_TYPE_CLOSE_WITH_LINE            }, /* close2b */

        { DRAW_PATH_TYPE_TERM }
      }
};

static const DRAW_DIAG
duffo_diag =
{
    (P_ANY) &duffo_diagram,
    sizeof(  duffo_diagram)
};

_Check_return_
_Ret_valid_
extern P_DRAW_DIAG
image_cache_search_empty(void)
{
    return((P_DRAW_DIAG) &duffo_diag);
}

/******************************************************************************
*
* search draw file cache for a file using a key
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_DRAW_DIAG
image_cache_search(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/)
{
    P_IMAGE_CACHE  p_image_cache;
    LIST_ITEMNO key;
    P_DRAW_DIAG p_draw_diag = NULL;

    trace_2(TRACE_MODULE_GR_CHART, "image_cache_search(&%p->&%p)", report_ptr_cast(p_image_cache_handle), report_ptr_cast(*p_image_cache_handle));

    if(!*p_image_cache_handle)
        return(NULL);

    key = (LIST_ITEMNO) *p_image_cache_handle;

    if((p_image_cache = image_cache_search_key(key)) != NULL)
        if(NULL != p_image_cache->p_draw_diag->data)
            p_draw_diag = p_image_cache->p_draw_diag;

    trace_1(TRACE_MODULE_GR_CHART, "image_cache_search yields &%p", report_ptr_cast(p_draw_diag));
    return(p_draw_diag);
}

/******************************************************************************
*
* ensure Draw file for given entry is loaded
*
******************************************************************************/

static void
image_cache_load_setup_foreign_diagram(
    _InoutRef_  P_DRAW_DIAG p_draw_diag,
    _In_z_      PC_SBSTR szCreatorName)
{
    P_DRAW_FILE_HEADER p_draw_file_header = (P_DRAW_FILE_HEADER) p_draw_diag->data;

    gr_riscdiag_diagram_init(p_draw_file_header, szCreatorName);

#if 0
    {
    DRAW_OBJECT_OPTIONS options;

    zero_struct(options); /* NB bounding box of options object must be ignored */

    options.type = DRAW_OBJECT_TYPE_OPTIONS;
    options.size = sizeof32(options);

    options.paper_size = (4 + 1) << 8; /* A4 */
    options.paper_limits.defaults = 1;

    writeval_F64_as_ARM(&options.grid_spacing[0], 1.0);
    options.grid_division = 2;
    options.grid_units = 1; /* cm */

    options.zoom_multiplier = 1;
    options.zoom_divider = 1;

    options.toolbox_presence = 1;

    options.initial_entry_mode.select = 1;

    options.undo_buffer_bytes = 5000;

    memcpy32(PtrAddBytes(P_BYTE, p_draw_file_header, sizeof32(DRAW_OBJECT_OPTIONS)), &options, sizeof32(DRAW_OBJECT_OPTIONS));
    } /*block*/
#endif
}

_Check_return_
static STATUS
image_cache_load(
    P_IMAGE_CACHE p_image_cache,
    LIST_ITEMNO key)
{
    P_DRAW_DIAG p_draw_diag = p_image_cache->p_draw_diag;
    PTSTR converted_file = NULL;
    FILE_HANDLE fin;
    P_U8 readp;
    STATUS res = 1;

    /* reset error each time we try to load */
    p_image_cache->error = 0;

    /* loop for structure */
    for(;;)
    {
        if(NULL != p_draw_diag->data)
        {
            /* already loaded */
            res = 1;
            break;
        }

        res = file_open((PCTSTR) (p_image_cache + 1), file_open_read, &fin);

        if(!fin)
        {
            trace_1(TRACE_MODULE_GR_CHART, "image_cache_load: cannot open: %s", (PCTSTR) (p_image_cache + 1));
            p_image_cache->error = res ? res : create_error(FILE_ERR_CANTOPEN);
            break;
        }

        /* another loop for structure */
        for(;;)
        {
            FILETYPE_RISC_OS filetype = file_get_type(fin);
            U32 filelength;
            U32 drawlength;
            U32 spritelength = 0; /* keep dataflower happy */
            U32 object_offset = 0;
            U32 object_data_offset = 0;

            if(status_fail(res = file_length(fin)))
            {
                p_image_cache->error = res;
                break;
            }

            filelength = res;

            drawlength = filelength;

            if(!image_cache_can_import_without_conversion(filetype))
            {
                if(!image_convert_can_convert(filetype))
                {
                    trace_1(TRACE_MODULE_GR_CHART, "image_cache_load: unable to handle file: %s", (PCTSTR) (p_image_cache + 1));
                    p_image_cache->error = create_error(GR_CHART_ERR_INVALID_DRAWFILE);
                    break; /* out of another loop */
                }

                file_close(&fin);

                if(status_fail(res = image_convert_do_convert(&converted_file, (PCTSTR) (p_image_cache + 1))))
                {
                    p_image_cache->error = res;
                    break;
                }

                res = file_open(converted_file, file_open_read, &fin);

                if(!fin)
                {
                    trace_1(TRACE_MODULE_GR_CHART, "image_cache_load: cannot open: %s", converted_file);
                    p_image_cache->error = res ? res : create_error(FILE_ERR_CANTOPEN);
                    break;
                }

                filetype = file_get_type(fin);

                if(status_fail(res = file_length(fin)))
                {
                    p_image_cache->error = res;
                    break;
                }

                filelength = res;

                drawlength = filelength;
            }

            switch(filetype) /* now kosher */
            {
            default: default_unhandled();
#if CHECKING
            case FILETYPE_PD_CHART:
            case FILETYPE_T5_DRAW:
            case FILETYPE_DRAW:
            case FILETYPE_POSTER:
            case FILETYPE_VECTOR:
#endif
                /* round up size to be paranoid */
                drawlength = round_up(filelength, 4);
                assert(drawlength == filelength); /* should already be a multiple of 4 */
                break;

            case FILETYPE_SPRITE:
                /* sprite files have a sprite area bound on the front without the length word */
                spritelength = filelength - (sizeof32(sprite_area) - 4);

                /* round up size for packaging in a Draw object */
                spritelength = round_up(spritelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) /*+ sizeof32(DRAW_OBJECT_OPTIONS)*/;
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_HEADER);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_JPEG:
                /* round up size for packaging in a Draw object */
                spritelength = round_up(filelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) /*+ sizeof32(DRAW_OBJECT_OPTIONS)*/;
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_JPEG);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_PIPEDREAM:
                /* round up size */
                drawlength = round_up(filelength, 4);

                if((res = image_cache_filehan_is_chart(fin)) <= 0)
                    drawlength = 0;
                break;
            }

            if((0 == drawlength) || (res < 0))
            {
                trace_1(TRACE_MODULE_GR_CHART, "image_cache_load: bad Draw file: %s", (PCTSTR) (p_image_cache + 1));
                p_image_cache->error = (res < 0) ? res : create_error(GR_CHART_ERR_INVALID_DRAWFILE);
                break; /* out of another loop */
            }

            reportf(TEXT("image_cache_load: file length: ") U32_TFMT TEXT(", Draw file length: ") U32_TFMT, filelength, drawlength);

            if(0 == (res = flex_alloc(&p_draw_diag->data, (int) drawlength)))
            {
                p_image_cache->error = status_nomem();
                break; /* out of another loop */
            }

            p_draw_diag->length = (int) drawlength;

            readp = p_draw_diag->data;

            /* Adjust the position where data is to be loaded */
            readp += object_data_offset;

            switch(filetype)
            {
            default: default_unhandled();
#if CHECKING
            case FILETYPE_T5_DRAW:
            case FILETYPE_DRAW:
            case FILETYPE_POSTER:
            case FILETYPE_VECTOR:
            case FILETYPE_JPEG:
#endif
                break;

            case FILETYPE_SPRITE:
                /* need to strip header too */
                assert(  (sizeof32(SAH) - 4) <= object_data_offset);
                readp -= (sizeof32(SAH) - 4);
                break;
            }

            /* Load all of the file at this position */
            if((res = file_read(readp, 1, (size_t) filelength, fin)) < (STATUS) filelength)
            {
                draw_diag_dispose(p_draw_diag);
                p_image_cache->error = (res < 0) ? res : create_error(FILE_ERR_CANTREAD);
                break; /* out of another loop */
            }

            /* Got the file loaded in p_image_cache->'h_data' */

            switch(filetype)
            {
            default: default_unhandled();
#if CHECKING
            case FILETYPE_T5_DRAW:
            case FILETYPE_DRAW:
            case FILETYPE_POSTER:
            case FILETYPE_VECTOR:
#endif
                break;

            case FILETYPE_PD_CHART:
            case FILETYPE_PIPEDREAM:
                {
                /* strip tagged objects out of PipeDream charts */
                GR_RISCDIAG gr_riscdiag;

                /* give loaded diagram away to temp diagram for reset/rebind process */
                gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);

                /* remove any tagged objects from the diagram, calling us back for each one found */
                gr_riscdiag_diagram_tagged_object_strip(&gr_riscdiag, image_cache_tagstrippers_call, (IMAGE_CACHE_HANDLE) key);

#if TRACE_ALLOWED && 0
                gr_riscdiag_diagram_save(&dcps, "$.Temp.Drawfile");
#endif
                draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

                draw_diag_reset_length(p_draw_diag); /* may well have changed size! */
                break;
                }

            case FILETYPE_SPRITE:
                {
                P_DRAW_OBJECT_SPRITE pSpriteObject;

                image_cache_load_setup_foreign_diagram(p_draw_diag, "PDC_SPRITE");

                /* sprite is the first and only real object in this diagram */
                pSpriteObject = PtrAddBytes(P_DRAW_OBJECT_SPRITE, p_draw_diag->data, object_offset);

                pSpriteObject->type = DRAW_OBJECT_TYPE_SPRITE;
                pSpriteObject->size = sizeof32(DRAW_OBJECT_HEADER)+ pSpriteObject->sprite.next; /* actual size */
                pSpriteObject->sprite.next = 0;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pSpriteObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pSpriteObject->bbox.x1 = -1;
                pSpriteObject->bbox.y0 =  0;
                pSpriteObject->bbox.y1 = -1;

                break;
                }

            case FILETYPE_JPEG:
                {
                U8 density_type = readp[13];
                U32 density_x = (U32) readval_U16_BE(&readp[14]);
                U32 density_y = (U32) readval_U16_BE(&readp[16]);
                P_DRAW_OBJECT_JPEG pJpegObject;
                S32 size;

                image_cache_load_setup_foreign_diagram(p_draw_diag, "PDC_JFIF");

                /* JPEG image is the first and only object in this diagram */
                pJpegObject = PtrAddBytes(P_DRAW_OBJECT_JPEG, p_draw_diag->data, sizeof32(DRAW_FILE_HEADER));

                pJpegObject->type = DRAW_OBJECT_TYPE_JPEG;

                size  = sizeof32(*pJpegObject);

                size += spritelength; /* actual size */

                pJpegObject->size = size;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pJpegObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pJpegObject->bbox.x1 = -1;
                pJpegObject->bbox.y0 =  0;
                pJpegObject->bbox.y1 = -1;

                pJpegObject->width  = GR_RISCDRAW_PER_INCH;
                pJpegObject->height = GR_RISCDRAW_PER_INCH;

                switch(density_type)
                {
                default: default_unhandled();
                case 0: /* aspect ratio - convert to something 'reasonable' as pixels per inch */
                    if((density_x == 1) && (density_y == 1))
                        density_x = density_y = 90;

                    /*FALLTRHU*/

                case 1: /* pixels per inch */
                    pJpegObject->dpi_x = density_x;
                    pJpegObject->dpi_y = density_y;
                    break;

                case 2: /* pixels per cm */
                    pJpegObject->dpi_x = (S32) (density_x / 2.54);
                    pJpegObject->dpi_y = (S32) (density_y / 2.54);
                    break;
                }

                pJpegObject->trfm.a = GR_SCALE_ONE;
                pJpegObject->trfm.b = GR_SCALE_ZERO;
                pJpegObject->trfm.c = GR_SCALE_ZERO;
                pJpegObject->trfm.d = GR_SCALE_ONE;
                pJpegObject->trfm.e = GR_SCALE_ZERO;
                pJpegObject->trfm.f = GR_SCALE_ZERO;

                pJpegObject->len = (int) filelength;

#if RISCOS
                { /* fill in rest of JPEG header */
                _kernel_swi_regs rs;
                rs.r[0] = 1; /* return dimensions */
                rs.r[1] = (int) (pJpegObject+ 1); /* point to JPEG image */
                rs.r[2] = (int) filelength;
                if(NULL == _kernel_swi(/*JPEG_Info*/ 0x49980, &rs, &rs))
                {
                  /*if(rs.r[0] & (1<<2))
                        reportf(TEXT("JPEG_Info: simple ratio"));*/
                    pJpegObject->width  = rs.r[2]; /* pixels */
                    pJpegObject->height = rs.r[3];
                  /*pJpegObject->dpi_x  = rs.r[4];*/
                  /*pJpegObject->dpi_y  = rs.r[5];*/
                    reportf(TEXT("JPEG_Info: w=%d, h=%d (px), dpi x=%d,y=%d"), pJpegObject->width, pJpegObject->height, pJpegObject->dpi_x, pJpegObject->dpi_y);

                    pJpegObject->width  = (pJpegObject->width  * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_x; /* convert from pixels to Draw units */
                    pJpegObject->height = (pJpegObject->height * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_y;
                }
                } /*block*/

#endif
                reportf(TEXT("JPEG: w=%d, h=%d Draw units"), pJpegObject->width, pJpegObject->height);

                pJpegObject->bbox.x1 = pJpegObject->width;
                pJpegObject->bbox.y1 = pJpegObject->height;

                break;
                }
            }

            /* always rebind the Draw file */
            image_cache_rebind_and_shift(p_image_cache);

            /* end of another loop for structure */
            break;
            /*NOTREACHED*/
        }

        file_close(&fin);

        /* end of loop for structure */
        break;
        /*NOTREACHED*/
    }

    res = p_image_cache->error ? p_image_cache->error : 1;

    /* call client to tell him this file was loaded (or not) */
    if(p_image_cache->recache.proc)
        (* p_image_cache->recache.proc) (p_image_cache->recache.handle, (IMAGE_CACHE_HANDLE) key, res);

    if(NULL != converted_file)
    {
        remove(converted_file);
        str_clr(&converted_file);
    }

    return(res);
}

static void
image_cache_rebind_and_shift(
    P_IMAGE_CACHE p_image_cache)
{
    P_DRAW_DIAG p_draw_diag = p_image_cache->p_draw_diag;
    GR_RISCDIAG gr_riscdiag;
    GR_RISCDIAG_PROCESS_T process;
    DRAW_BOX draw_box;
    DRAW_POINT shiftBy;

#if TRACE_ALLOWED
    draw_box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
    trace_4(TRACE_MODULE_GR_CHART, "image_cache_rebind: diagram bbox prior to rebind %d, %d, %d, %d (Draw)",
            draw_box.x0, draw_box.y0, draw_box.x1, draw_box.y1);
#endif

    zero_struct(process);
    process.recurse    = 1;
    process.recompute  = 1; /* especially for loaded sprite - see above */

    /* give loaded diagram away to temp diagram for reset/rebind process */
    gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);
    gr_riscdiag.dd_fontListR = GR_RISCDIAG_OBJECT_NONE;

    gr_riscdiag_diagram_reset_bbox(&gr_riscdiag, process);

    draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

    draw_diag_reset_length(p_draw_diag); /* shouldn't have changed size - but let's be paranoid */

    /* get box in Draw units */
    draw_box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
    trace_4(TRACE_MODULE_GR_CHART, "image_cache_rebind: diagram bbox after rebind %d, %d, %d, %d (Draw)",
            draw_box.x0, draw_box.y0, draw_box.x1, draw_box.y1);

    /* move diagram over into the bottom left corner */
    shiftBy.x = -draw_box.x0;
    shiftBy.y = -draw_box.y0;
    if((0 != shiftBy.x) || (0 != shiftBy.y))
    {
        trace_2(TRACE_MODULE_GR_CHART, "image_cache_rebind: shift_diag by (%d, %d)", shiftBy.x, shiftBy.y);

        /* give loaded diagram away to temp diagram for shift process */
        gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);

        gr_riscdiag_shift_diagram(&gr_riscdiag, &shiftBy);

        draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

#if TRACE_ALLOWED
        draw_box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
        trace_4(TRACE_MODULE_GR_CHART, "image_cache_rebind: diagram bbox after shift %d, %d, %d, %d (Draw)",
                draw_box.x0, draw_box.y0, draw_box.x1, draw_box.y1);
#endif
    }
}

/******************************************************************************
*
* add/remove procedure to be called on Draw file tag detection
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_tagstripper_add(
    image_cache_tagstrip_proc proc,
    P_ANY handle,
    _InVal_     U32 tag)
{
    LIST_ITEMNO item;

    status_return(
        funclist_add(&image_cache_tagstrippers,
                     (funclist_proc) proc, handle,
                     &item,
                     1 /*non-zero tag*/,
                     0,
                     sizeof(IMAGE_CACHE_TAGSTRIP_EXTRADATA)));

    /* write interested tag data */
    funclist_writedata_ip(&image_cache_tagstrippers,
                          item, &tag,
                          offsetof(IMAGE_CACHE_TAGSTRIP_EXTRADATA, tag),
                          sizeof(tag));

    return(STATUS_OK);
}

extern void
image_cache_tagstripper_remove(
    image_cache_tagstrip_proc proc,
    P_ANY handle)
{
    funclist_remove(&image_cache_tagstrippers,
                    (funclist_proc) proc, handle);
}

/******************************************************************************
*
* callback from gr_riscdiag_tagstrip of a diagram we just loaded - call our clients
*
******************************************************************************/

gr_riscdiag_tagstrip_proto(static, image_cache_tagstrippers_call)
{
    LIST_ITEMNO   item;
    funclist_proc proc;
    P_ANY         prochandle;
    S32           proctag;
    U32           wantTag;
    IMAGE_CACHE_TAGSTRIP_INFO image_cache_tagstrip_info;

    for(proctag = funclist_first(&image_cache_tagstrippers,
                                 &proc, &prochandle, &item, FALSE);
        proctag;
        proctag = funclist_next( &image_cache_tagstrippers,
                                 &proc, &prochandle, &item, FALSE))
    {
        image_cache_tagstrip_proc tsp = (image_cache_tagstrip_proc) proc;

        /* test request */
        funclist_readdata_ip(&image_cache_tagstrippers,
                             item, &wantTag,
                             offsetof(IMAGE_CACHE_TAGSTRIP_EXTRADATA, tag),
                             sizeof(wantTag));

        if(wantTag && (wantTag != p_image_cache_tagstrip_info->tag))
            continue;

        image_cache_tagstrip_info.r = *p_image_cache_tagstrip_info; /* each client gets a fresh copy */
        image_cache_tagstrip_info.image_cache_handle = handle;

        /* call this client's handler */
        (* tsp) (&image_cache_tagstrip_info, prochandle);
    }

    return(1);
}

/* end of im_cache.c */
