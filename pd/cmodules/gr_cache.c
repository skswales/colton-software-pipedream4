/* gr_cache.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that handles Draw file cache */

/* SKS 12-Sep-1991 */

#include "common/gflags.h"

#include "gr_chart.h"

#include "gr_chari.h"

#include "cs-flex.h"

/*
internal structure
*/

/*
entries in the Draw file cache
*/

typedef struct _GR_CACHE_DRAWFILE_ENTRY
{
    P_USTR name;

    S32 error;
    S32 refs;
    S32 autokill;

    P_DRAW_DIAG p_draw_diag;

    struct _gr_cache_drawfile_entry_recache
    {
        gr_cache_recache_proc proc; /* make a funclist list sometime */
        P_ANY                  handle;
    }
    recache;
}
GR_CACHE, * P_GR_CACHE;

/*
callback function
*/

gr_riscdiag_tagstrip_proto(static, gr_cache_tagstrippers_call);

/*
internal functions
*/

static S32
gr_cache_load(
    P_GR_CACHE p_gr_cache,
    LIST_ITEMNO key);

static void
gr_cache_rebind_and_shift(
    P_GR_CACHE p_gr_cache);

/*
extra data stored by funclist for us to understand tagstrippers
*/

typedef struct _gr_cache_tagstrip_extradata
{
    U32 tag;
}
gr_cache_tagstrip_extradata;
/* never have pointers to this data - always use funclist to access */

/*
the cache descriptor list
*/

static NLISTS_BLK
gr_cache_drawfiles =
{
    NULL,
    (sizeof(GR_CACHE) + BUF_MAX_PATHSTRING),
    (sizeof(GR_CACHE) + BUF_MAX_PATHSTRING) * 16
};

/*
the interested tagstrip list
*/

static P_LIST_BLKREF
gr_cache_tagstrippers = NULL;

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

extern BOOL
gr_cache_can_import(
    FILETYPE_RISC_OS filetype)
{
    switch(filetype)
        {
        case FILETYPE_PIPEDREAM:
        case FILETYPE_DRAW:
        case FILETYPE_SPRITE:
        case FILETYPE_POSTER:
        case FILETYPE_JPEG:
            return(1);

        default:
            return(0);
        }
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
gr_cache_entry_ensure(
    P_GR_CACHE_HANDLE ecahp /*out*/,
    PC_U8 name)
{
    static LIST_ITEMNO cahkey_gen = 0x42516000; /* NB. not tbs! */

    GR_CACHE_HANDLE   cah;
    P_GR_CACHE_HANDLE cahp = ecahp ? ecahp : &cah; /* export if viable */
    LIST_ITEMNO       key;
    P_GR_CACHE        p_gr_cache;
    P_USTR            name_copy = NULL;
    P_DRAW_DIAG       p_draw_diag;
    STATUS            res;

    *cahp = 0;

    if(gr_cache_entry_query(cahp, name))
        return(1);

    if((res = file_is_file(name)) <= 0)
        return(res ? res : create_error(FILE_ERR_NOTFOUND));

    status_return(str_set(&name_copy, name));

    /* NB keep separate as it contains a flex anchor and listed data may move */
    if(NULL == (p_draw_diag = al_ptr_calloc_elem(DRAW_DIAG, 1, &res)))
        {
        str_clr(&name_copy);
        return(res);
        }

    /* create a list entry */
    key = cahkey_gen++;

    if((p_gr_cache = collect_add_entry(&gr_cache_drawfiles, sizeof(*p_gr_cache), &key)) == NULL)
        {
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_draw_diag));
        str_clr(&name_copy);
        return(status_nomem());
        }

    zero_struct_ptr(p_gr_cache);

    p_gr_cache->name = name_copy;

    p_gr_cache->p_draw_diag = p_draw_diag;

    *cahp = (GR_CACHE_HANDLE) key;

    return(1);
}

/******************************************************************************
*
* given the details of a draw file, query whether an entry for it is in the cache
*
******************************************************************************/

extern S32
gr_cache_entry_query(
    P_GR_CACHE_HANDLE ecahp /*out*/,
    PC_U8 name)
{
    GR_CACHE_HANDLE   cah;
    P_GR_CACHE_HANDLE cahp = ecahp ? ecahp : &cah; /* export if viable */
    LIST_ITEMNO       key;
    P_GR_CACHE        p_gr_cache;
    S32               rooted;
    PC_U8            leaf, testname, entryname;

    rooted = file_is_rooted(name);
    leaf   = file_leafname(name);

    /* search for the file on the list */
    *cahp = GR_CACHE_HANDLE_NONE;

    for(p_gr_cache = collect_first(&gr_cache_drawfiles, &key); p_gr_cache; p_gr_cache = collect_next(&gr_cache_drawfiles, &key))
        {
        testname  = name;
        entryname = p_gr_cache->name;

        if(!rooted)
            {
            testname  = leaf;
            entryname = file_leafname(entryname);
            }

        if(0 == _stricmp(testname, entryname))
            {
            *cahp = (GR_CACHE_HANDLE) key;

            trace_2(TRACE_MODULE_GR_CHART, "gr_cache_entry_query found file, keys: %p %d, in list\n", report_ptr_cast(*cahp), key);

            trace_0(TRACE_MODULE_GR_CHART, "gr_cache_entry_query yields 1\n");
            return(1);
            }
        }

    trace_0(TRACE_MODULE_GR_CHART, "gr_cache_entry_query yields 0\n");
    return(0);
}

extern S32
gr_cache_entry_remove(
    P_GR_CACHE_HANDLE cahp /*inout*/)
{
    P_GR_CACHE p_gr_cache;
    LIST_ITEMNO key;

    if(!*cahp)
        return(0);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_gr_cache->p_draw_diag));
        str_clr(&p_gr_cache->name);
        collect_subtract_entry(&gr_cache_drawfiles, &key);
        return(1);
        }

    return(0);
}

extern S32
gr_cache_entry_rename(
    P_GR_CACHE_HANDLE cahp /*const*/,
    PC_U8 name)
{
    P_GR_CACHE  p_gr_cache;
    LIST_ITEMNO key;

    trace_2(TRACE_MODULE_GR_CHART, "gr_cache_entry_rename(&%p->&%p)\n", report_ptr_cast(cahp), report_ptr_cast(*cahp));

    if(!*cahp)
        return(0);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        status_return(str_set(&p_gr_cache->name, name));
        return(1);
        }

    return(0);
}

/******************************************************************************
*
* set state such that entry is removed when refs go to zero
*
******************************************************************************/

extern S32
gr_cache_entry_set_autokill(
    P_GR_CACHE_HANDLE cahp /*const*/)
{
    P_GR_CACHE  p_gr_cache;
    LIST_ITEMNO key;

    if(!*cahp)
        return(0);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        p_gr_cache->autokill = 1;

        return(1);
        }

    return(0);
}

extern S32
gr_cache_error_query(
    P_GR_CACHE_HANDLE cahp /*const*/)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;
    S32         err = 0;

    if(!*cahp)
        return(err);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        err = p_gr_cache->error;

    return(err);
}

extern S32
gr_cache_error_set(
    P_GR_CACHE_HANDLE cahp /*const*/,
    S32 err)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;

    if(!*cahp)
        return(err);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
    {
        p_gr_cache->error = err;

        if(err)
            /* chuck the diagram - it's of no use now */
            draw_diag_dispose(p_gr_cache->p_draw_diag);
    }

    return(err);
}

/******************************************************************************
*
* allow clients to distingish chart files from normal Draw or PD files
*
******************************************************************************/

extern S32
gr_cache_file_is_chart(
    PC_U8 name)
{
    FILE_HANDLE fin;
    S32         res;

    if((res = file_open(name, file_open_read, &fin)) <= 0)
        return(res);

    res = gr_cache_filehan_is_chart(fin);

    (void) file_close(&fin);

    return(res);
}

extern S32
gr_cache_filehan_is_chart(
    FILE_HANDLE fin)
{
    DRAW_FILE_HEADER testbuf;
    S32 res;

    if((res = file_rewind(fin)) < 0)
        return(res);

    if((res = file_read(&testbuf, sizeof(testbuf), 1, fin)) < 1)
        return(res);

    if((res = file_rewind(fin)) < 0)
        return(res);

    return(gr_cache_fileheader_is_chart(&testbuf, sizeof32(testbuf)));
}

extern S32
gr_cache_fileheader_is_chart(
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

extern P_DRAW_DIAG
gr_cache_loaded_ensure(
    P_GR_CACHE_HANDLE cahp /*const*/)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;

    if(!*cahp)
        return(NULL);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        if(NULL == p_gr_cache->p_draw_diag->data)
            {
            /* let caller query any errors from this load */
            if(gr_cache_load(p_gr_cache, key) <= 0)
                return(NULL);
            }

        return(p_gr_cache->p_draw_diag);
        }

    return(NULL);
}

extern S32
gr_cache_name_query(
    P_GR_CACHE_HANDLE cahp /*const*/, char * buffer /*out*/,
    size_t buflen)
{
    LIST_ITEMNO key;
    P_GR_CACHE   p_gr_cache;

    if(!*cahp)
        return(0);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        void_strkpy(buffer, buflen, p_gr_cache->name);
        return(1);
        }

    return(0);
}

/******************************************************************************
*
* when a graph is changed, the graphics program calls
* PipeDream to update any draw files it has of the graph
*
******************************************************************************/

extern S32
gr_cache_recache(
    PC_U8 drawfilename)
{
    P_GR_CACHE  p_gr_cache;
    LIST_ITEMNO key;
    S32         rooted;
    PC_U8      leaf, testname, entryname;
    S32         res = 0;

    trace_1(TRACE_MODULE_GR_CHART, "gr_cache_recache(%s)\n", drawfilename);

    if(!gr_cache_drawfiles.lbr)
        return(0);

    rooted = file_is_rooted(drawfilename);
    leaf   = file_leafname(drawfilename);

    for(p_gr_cache = collect_first(&gr_cache_drawfiles, &key); p_gr_cache; p_gr_cache = collect_next(&gr_cache_drawfiles, &key))
        {
        testname  = drawfilename;
        entryname = p_gr_cache->name;

        /* if ambiguity in name, just check leafnames */
        if(!rooted)
            {
            testname  = leaf;
            entryname = file_leafname(entryname);
            }

        trace_2(TRACE_MODULE_GR_CHART, "gr_cache_recache: comparing entry %s with %s\n", entryname, testname);

        if(0 == _stricmp(testname, entryname))
            {
            /* throwing away the Draw file seems easiest way to recache */
            p_gr_cache->p_draw_diag->length = 0;
            flex_free(&p_gr_cache->p_draw_diag->data);

            if((res = gr_cache_load(p_gr_cache, key)) < 0)
                break;
            }
        }

    return(res);
}

extern S32
gr_cache_recache_key(
    P_GR_CACHE_HANDLE cahp /*const*/)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;
    S32         res = 0;

    trace_2(TRACE_MODULE_GR_CHART, "gr_cache_recache_key(&%p->&%p)\n", report_ptr_cast(cahp), report_ptr_cast(cahp ? *cahp : 0));

    if(!*cahp)
        return(res);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        /* throwing away the Draw file seems easiest way to recache */
        draw_diag_dispose(p_gr_cache->p_draw_diag);

        res = gr_cache_load(p_gr_cache, key);
        }

    return(res);
}

/******************************************************************************
*
* add recache upcall handler for given Draw file
*
******************************************************************************/

extern S32
gr_cache_recache_inform(
    P_GR_CACHE_HANDLE cahp /*const*/, gr_cache_recache_proc proc,
    P_ANY handle)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;
    S32         res = 0;

    trace_4(TRACE_MODULE_GR_CHART, "gr_cache_recache_inform(&%p->&%p &%p, &%p)\n", report_ptr_cast(cahp), report_ptr_cast(*cahp), report_procedure_name(report_proc_cast(proc)), report_ptr_cast(handle));

    if(!*cahp)
        return(res);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        {
        p_gr_cache->recache.proc   = proc;
        p_gr_cache->recache.handle = handle;

        res = 1;
        }

    return(res);
}

/******************************************************************************
*
* add / remove ref for draw file
*
******************************************************************************/

extern S32
gr_cache_ref(
    P_GR_CACHE_HANDLE cahp /*const*/,
    S32 add)
{
    LIST_ITEMNO key;
    P_GR_CACHE  p_gr_cache;

    trace_3(TRACE_MODULE_GR_CHART, "gr_cache_ref(&%p->&%p, %d)\n", report_ptr_cast(cahp), report_ptr_cast(*cahp), add);

    if(!*cahp)
        return(0);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) == NULL)
        return(0);

    if(add)
        {
        ++p_gr_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, "gr_cache_ref: &%x refs up to %d\n", key, p_gr_cache->refs);
        }
    else
        {
        --p_gr_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, "gr_cache_ref: &%x refs down to %d\n", key, p_gr_cache->refs);
        myassert0x(p_gr_cache->refs >= 0, "gr_cache_ref decremented cache ref count past zero");
        if(!p_gr_cache->refs)
            {
            trace_1(TRACE_MODULE_GR_CHART, "gr_cache_ref: refs down to 0, so free diagram &%p (leave the entry around)\n", p_gr_cache->p_draw_diag->data);
            draw_diag_dispose(p_gr_cache->p_draw_diag);

            if(p_gr_cache->autokill)
                gr_cache_entry_remove(cahp);
            }
        }

    return(1);
}

extern S32
gr_cache_reref(P_GR_CACHE_HANDLE  cahp /*inout*/,
               /*const*/ P_GR_CACHE_HANDLE new_cahp)
{
    GR_CACHE_HANDLE old_cah, new_cah;

    old_cah = *cahp;
    new_cah = new_cahp ? *new_cahp : GR_CACHE_HANDLE_NONE;

    /* changing reference? */
    if(old_cah != new_cah)
        {
        /* remove a prior ref if there was one */
        if(old_cah != GR_CACHE_HANDLE_NONE)
            gr_cache_ref(cahp, 0);

        /* add new ref if there is one */
        if(new_cah != GR_CACHE_HANDLE_NONE)
            gr_cache_ref(new_cahp, 1);

        /* poke the picture ref */
        *cahp = new_cah;
        }

    return(1);
}

extern S32
gr_cache_refs(
    PC_U8 name)
{
    GR_CACHE_HANDLE cah;
    LIST_ITEMNO     key;
    P_GR_CACHE      p_gr_cache;

    if(!gr_cache_entry_query(&cah, name))
        return(0);

    key = (LIST_ITEMNO) cah;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) == NULL)
        return(0);

    return(p_gr_cache->refs);
}

/******************************************************************************
*
* search draw file cache for a file
*
******************************************************************************/

/* SKS after 4.12 30mar92 - 0x1000 coords were too small to see on screen */

struct _duffo_diagram
{
    DRAW_FILE_HEADER diagHdr;

    DRAW_OBJECT_PATH pathHdr;

    struct _duffo_diagram_pathGuts
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

static /*non-const - we poke it once!*/ struct _duffo_diagram
duffo_diagram =
{ /* struct _duffo_diagram */
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
        sizeof(DRAW_OBJECT_PATH) + sizeof(struct _duffo_diagram_pathGuts), /* size */
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
        { DRAW_PATH_TYPE_MOVE, 0x0,     0x0     }, /* move (bl) */
        { DRAW_PATH_TYPE_LINE, 0x10000, 0x0     }, /* br */
        { DRAW_PATH_TYPE_LINE, 0x10000, 0x10000 }, /* tr */
        { DRAW_PATH_TYPE_LINE, 0x0,     0x10000 }, /* tl */
        { DRAW_PATH_TYPE_LINE, 0x0,     0x0     }, /* bl */
        { DRAW_PATH_TYPE_CLOSE },                /* close1 */

        { DRAW_PATH_TYPE_MOVE, 0x0,     0x0     }, /* bl2 */
        { DRAW_PATH_TYPE_LINE, 0x10000, 0x10000 }, /* tr2 */
        { DRAW_PATH_TYPE_CLOSE },                /* close2a */

        { DRAW_PATH_TYPE_MOVE, 0x0,     0x10000 }, /* tl2 */
        { DRAW_PATH_TYPE_LINE, 0x10000, 0x0     }, /* br2 */
        { DRAW_PATH_TYPE_CLOSE },                /* close2b */

        { DRAW_PATH_TYPE_TERM }
      }
};

static const DRAW_DIAG
duffo_diag =
{
    (P_ANY) &duffo_diagram,
    sizeof(  duffo_diagram)
};

extern P_DRAW_DIAG
gr_cache_search_empty(void)
{
    return((P_DRAW_DIAG) &duffo_diag);
}

/******************************************************************************
*
* search draw file cache for a file using a key
*
******************************************************************************/

extern P_DRAW_DIAG
gr_cache_search(
    P_GR_CACHE_HANDLE cahp /*const*/)
{
    P_GR_CACHE  p_gr_cache;
    LIST_ITEMNO key;
    P_DRAW_DIAG p_draw_diag = NULL;

    trace_2(TRACE_MODULE_GR_CHART, "gr_cache_search(&%p->&%p)\n", report_ptr_cast(cahp), report_ptr_cast(*cahp));

    if(!*cahp)
        return(NULL);

    key = (LIST_ITEMNO) *cahp;

    if((p_gr_cache = collect_search(&gr_cache_drawfiles, &key)) != NULL)
        if(NULL != p_gr_cache->p_draw_diag->data)
            p_draw_diag = p_gr_cache->p_draw_diag;

    trace_1(TRACE_MODULE_GR_CHART, "gr_cache_search yields &%p\n", report_ptr_cast(p_draw_diag));
    return(p_draw_diag);
}

/******************************************************************************
*
* ensure Draw file for given entry is loaded
*
******************************************************************************/

static S32
gr_cache_load(
    P_GR_CACHE p_gr_cache,
    LIST_ITEMNO key)
{
    P_DRAW_DIAG p_draw_diag = p_gr_cache->p_draw_diag;
    FILE_HANDLE fin;
    FILETYPE_RISC_OS filetype;
    S32 length, lengthfile, spritelength;
    P_U8 readp;
    S32 res = 1;

    /* reset error each time we try to load */
    p_gr_cache->error = 0;

    /* loop for structure */
    for(;;)
        {
        if(NULL != p_draw_diag->data)
            {
            /* already loaded */
            res = 1;
            break;
            }

        res = file_open(p_gr_cache->name, file_open_read, &fin);

        if(!fin)
            {
            trace_1(TRACE_MODULE_GR_CHART, "gr_cache_load: cannot open: %s\n", p_gr_cache->name);
            p_gr_cache->error = res ? res : create_error(FILE_ERR_CANTOPEN);
            break;
            }

        /* another loop for structure */
        for(;;)
            {
            lengthfile = file_length(fin);
            length = lengthfile;

            filetype = file_get_type(fin);

            if(!gr_cache_can_import(filetype))
                {
                trace_1(TRACE_MODULE_GR_CHART, "gr_cache_load: bad Draw file: %s\n", p_gr_cache->name);
                p_gr_cache->error = create_error(GR_CHART_ERR_INVALID_DRAWFILE);
                break; /* out of another loop */
                }

            switch(filetype) /* now kosher */
                {
                case FILETYPE_PIPEDREAM:
                    /* round up size */
                    length = round_up(length, 4);

                    spritelength = 0; /* keep dataflower happy */

                    if((res = gr_cache_filehan_is_chart(fin)) <= 0)
                        length = 0;
                    break;

                case FILETYPE_SPRITE:
                    /* round up size */
                    length = round_up(length, 4);

                    /* sprite files have a sprite area bound on
                     * the front without the length word
                    */
                    spritelength = length - (sizeof(sprite_area) - 4);

                    length = sizeof(DRAW_FILE_HEADER) +
                             sizeof(DRAW_OBJECT_HEADER) +
                             spritelength;
                    break;

                case FILETYPE_JPEG:
                    /* round up size */
                    length = round_up(length, 4);

                    spritelength = length;

                    length = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_JPEG) + spritelength;
                    break;

                case FILETYPE_POSTER:
                case FILETYPE_DRAW:
                default:
                    /* round up size */
                    length = round_up(length, 4);

                    spritelength = 0; /* keep dataflower happy */
                    break;
                }

            if(!length || (res < 0))
                {
                trace_1(TRACE_MODULE_GR_CHART, "gr_cache_load: bad Draw file: %s\n", p_gr_cache->name);
                p_gr_cache->error = (res < 0) ? res : create_error(GR_CHART_ERR_INVALID_DRAWFILE);
                break; /* out of another loop */
                }

            trace_2(TRACE_MODULE_GR_CHART, "gr_cache_load: found Draw file length: %d, %d\n", lengthfile, length);

            if(0 == (res = flex_alloc(&p_draw_diag->data, (int) length)))
                {
                p_gr_cache->error = create_error(GR_CHART_ERR_NOMEM);
                break; /* out of another loop */
                }

            p_draw_diag->length = (int) length;

            readp = p_draw_diag->data;

            /* Adjust load positions */
            switch(filetype)
            {
            default:
            case FILETYPE_DRAW:
                break;

            case FILETYPE_SPRITE:
                {
                U32 objoff = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_HEADER);
                /* need to strip header too */
                assert(  objoff >= (sizeof32(SAH) - 4));
                readp += objoff  - (sizeof32(SAH) - 4);
                break;
                }

            case FILETYPE_WINDOWS_BMP:
                {
                U32 objoff = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_HEADER);
                /* need to strip header too */
                assert(  objoff >= (sizeof_BITMAPFILEHEADER));
                readp += objoff  - (sizeof_BITMAPFILEHEADER);
                break;
                }

            case FILETYPE_JPEG:
                {
                U32 objoff = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_JPEG);
                readp += objoff;
                break;
                }
            }

            /* load file */
            if((res = file_read(readp, 1, (size_t) lengthfile, fin)) < lengthfile)
                {
                draw_diag_dispose(p_draw_diag);

                p_gr_cache->error = create_error(FILE_ERR_CANTREAD);
                break; /* out of another loop */
                }

            if(filetype == FILETYPE_SPRITE)
                {
                P_DRAW_OBJECT_HEADER pObject;
                GR_DIAG_OFFSET spriteObject;

                gr_riscdiag_diagram_init((P_DRAW_FILE_HEADER) p_gr_cache->p_draw_diag->data, "gr_cache");

                trace_2(TRACE_MODULE_GR_CHART, "gr_cache_load: about to create sprite header &%p -> &%p\n", report_ptr_cast(p_gr_cache), report_ptr_cast(p_draw_diag->data));

                /* first object in this diagram */
                spriteObject = sizeof(DRAW_FILE_HEADER);

                pObject = PtrAddBytes(P_DRAW_OBJECT_HEADER, p_draw_diag->data, spriteObject);

                pObject->type = DRAW_OBJECT_TYPE_SPRITE;
                pObject->size = spritelength + sizeof(DRAW_OBJECT_HEADER);

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pObject->bbox.x0 = 0; /* but positioned at 0,0 */
                pObject->bbox.x1 = -1;
                pObject->bbox.y0 = 0;
                pObject->bbox.y1 = -1;
                }
            else if(FILETYPE_JPEG == filetype)
                {
                P_DRAW_OBJECT_JPEG pJpegObject;
                S32 size;

                gr_riscdiag_diagram_init((P_DRAW_FILE_HEADER) p_draw_diag->data, "gr_cache");

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

                pJpegObject->trfm.a = GR_SCALE_ONE;
                pJpegObject->trfm.b = GR_SCALE_ZERO;
                pJpegObject->trfm.c = GR_SCALE_ZERO;
                pJpegObject->trfm.d = GR_SCALE_ONE;
                pJpegObject->trfm.e = GR_SCALE_ZERO;
                pJpegObject->trfm.f = GR_SCALE_ZERO;

                pJpegObject->len = (int) lengthfile;

#if WINDOWS
                p_gr_cache->cpicture = CPicture_New(&res);

                if(NULL != p_gr_cache->cpicture)
                {
                    SIZE image_size;
                    S32 image_dpi = 2540;

                    CPicture_Load_HGlobal(p_gr_cache->cpicture, hGlobal);
                    /* Once it is loaded, we can free the hGlobal (done at the bottom) */

                    CPicture_GetImageSize(p_gr_cache->cpicture, &image_size); // HIMETRIC (0.01mm)

                    /* Scale down by factor of two to fit on an A4 portrait page */
                    while( (image_size.cx > (100.0/*0.01mm*/ * 210.0/*mm*/))
                        || (image_size.cy > (100.0 * 297.0/*mm*/)) )
                    {
                        image_size.cx /= 2;
                        image_size.cy /= 2;
                        image_dpi *= 2;
                    }

                    /* fill in rest of JPEG header */
                    pJpegObject->width  = image_size.cx;
                    pJpegObject->height = image_size.cy;
                    pJpegObject->dpi_x  = image_dpi;
                    pJpegObject->dpi_y  = image_dpi;
                }
#endif

#if RISCOS
                { /* fill in rest of JPEG header */
                _kernel_swi_regs rs;
                rs.r[0] = 0; /* undocumented SpriteExtend options! */
                rs.r[1] = (int) (pJpegObject+ 1); /* point to JPEG image */
                rs.r[2] = (int) lengthfile;
                if(NULL == _kernel_swi(/*JPEG_Info*/ 0x49980, &rs, &rs))
                {
                    pJpegObject->width  = rs.r[2];
                    pJpegObject->height = rs.r[3];
                    pJpegObject->dpi_x  = rs.r[4];
                    pJpegObject->dpi_y  = rs.r[5];
                }
                } /*block*/
#endif
                }
            else
                {
                /* strip tagged objects out of real Draw files */
                GR_RISCDIAG gr_riscdiag;

                /* give loaded diagram away to temp diagram for reset/rebind process */
                gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);

                /* remove any tagged objects from the diagram, calling us back for each one found */
                gr_riscdiag_diagram_tagged_object_strip(&gr_riscdiag, gr_cache_tagstrippers_call, (GR_CACHE_HANDLE) key);

#if TRACE_ALLOWED && 0
                gr_riscdiag_diagram_save(&dcps, "$.Temp.Drawfile");
#endif
                draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

                draw_diag_reset_length(p_draw_diag); /* may well have changed size! */
                }

            /* always rebind the Draw file */
            gr_cache_rebind_and_shift(p_gr_cache);

            /* end of another loop for structure */
            break;
            }

        file_close(&fin);

        /* end of loop for structure */
        break;
        }

    res = p_gr_cache->error ? p_gr_cache->error : 1;

    /* call client to tell him this file was loaded (or not) */
    if(p_gr_cache->recache.proc)
        (* p_gr_cache->recache.proc) (p_gr_cache->recache.handle, (GR_CACHE_HANDLE) key, res);

    return(res);
}

static void
gr_cache_rebind_and_shift(
    P_GR_CACHE p_gr_cache)
{
    P_DRAW_DIAG p_draw_diag = p_gr_cache->p_draw_diag;
    GR_RISCDIAG gr_riscdiag;
    GR_RISCDIAG_PROCESS_T process;
    DRAW_BOX draw_box;
    DRAW_POINT shiftBy;

#if TRACE_ALLOWED
    draw_box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
    trace_4(TRACE_MODULE_GR_CHART, "gr_cache_rebind: diagram bbox prior to rebind %d, %d, %d, %d (Draw)\n",
            draw_box.x0, draw_box.y0, draw_box.x1, draw_box.y1);
#endif

    * (int *) &process = 0;
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
    trace_4(TRACE_MODULE_GR_CHART, "gr_cache_rebind: diagram bbox after rebind %d, %d, %d, %d (Draw)\n",
            draw_box.x0, draw_box.y0, draw_box.x1, draw_box.y1);

    /* move diagram over into the bottom left corner */
    shiftBy.x = -draw_box.x0;
    shiftBy.y = -draw_box.y0;
    if((0 != shiftBy.x) || (0 != shiftBy.y))
    {
        trace_2(TRACE_MODULE_GR_CHART, "gr_cache_rebind: shift_diag by (%d, %d)\n", shiftBy.x, shiftBy.y);

        /* give loaded diagram away to temp diagram for shift process */
        gr_riscdiag_diagram_setup_from_draw_diag(&gr_riscdiag, p_draw_diag);

        gr_riscdiag_shift_diagram(&gr_riscdiag, &shiftBy);

        draw_diag_give_away(p_draw_diag, &gr_riscdiag.draw_diag);

#if TRACE_ALLOWED
        draw_box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;
        trace_4(TRACE_MODULE_GR_CHART, "gr_cache_rebind: diagram bbox after shift %d, %d, %d, %d (Draw)\n",
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
gr_cache_tagstrip(
    gr_cache_tagstrip_proc proc,
    P_ANY handle,
    U32 tag,
    S32 add)
{
    if(add)
        {
        LIST_ITEMNO item;

        status_return(
            funclist_add(&gr_cache_tagstrippers,
                         (funclist_proc) proc, handle,
                         &item,
                         1 /*non-zero tag*/,
                         0,
                         sizeof(gr_cache_tagstrip_extradata)));

        /* write interested tag data */
        funclist_writedata_ip(&gr_cache_tagstrippers,
                              &item, &tag,
                              offsetof(gr_cache_tagstrip_extradata, tag),
                              sizeof(tag));
        }
    else
        {
        funclist_remove(&gr_cache_tagstrippers,
                        (funclist_proc) proc, handle);
        }

    return(STATUS_OK);
}

/******************************************************************************
*
* callback from gr_riscdiag_tagstrip of a diagram we just loaded - call our clients
*
******************************************************************************/

gr_riscdiag_tagstrip_proto(static, gr_cache_tagstrippers_call)
{
    LIST_ITEMNO   item;
    funclist_proc proc;
    P_ANY          prochandle;
    S32           proctag;
    U32           wantTag;
    GR_CACHE_TAGSTRIP_INFO info;

    for(proctag = funclist_first(&gr_cache_tagstrippers,
                                 &proc, &prochandle, &item, FALSE);
        proctag;
        proctag = funclist_next( &gr_cache_tagstrippers,
                                 &proc, &prochandle, &item, FALSE))
        {
        gr_cache_tagstrip_proc tsp = (gr_cache_tagstrip_proc) proc;

        /* test request */
        funclist_readdata_ip(&gr_cache_tagstrippers,
                             &item, &wantTag,
                             offsetof(gr_cache_tagstrip_extradata, tag),
                             sizeof(wantTag));

        if(wantTag && (wantTag != p_info->tag))
            continue;

        info.r   = *p_info; /* each client gets a fresh copy */
        info.cah = handle;

        /* call this client's handler */
        (* tsp) (prochandle, &info);
        }

    return(1);
}

/* end of gr_cache.c */
