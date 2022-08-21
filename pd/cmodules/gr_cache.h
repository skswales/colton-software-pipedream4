/* gr_cache.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Draw file cache */

/* SKS May 1991  */

#ifndef __gr_cache_h
#define __gr_cache_h

/*
exported typedefs from gr_cache.c
*/

typedef struct DRAW_DIAG
{
    void * data;
    U32 length;
}
DRAW_DIAG, * P_DRAW_DIAG, ** P_P_DRAW_DIAG; typedef const DRAW_DIAG * PC_DRAW_DIAG;

#if RISCOS

/*
gr_cache tagstrip
*/

typedef struct GR_CACHE_TAGSTRIP_INFO * P_GR_CACHE_TAGSTRIP_INFO;

typedef S32 (* gr_cache_tagstrip_proc) (
    P_ANY handle,
    P_GR_CACHE_TAGSTRIP_INFO p_info);

#define gr_cache_tagstrip_proto(_e_s, _p_proc_gr_cache_tagstrip) \
_e_s S32 _p_proc_gr_cache_tagstrip( \
    P_ANY handle, \
    P_GR_CACHE_TAGSTRIP_INFO p_info)

#endif /* RISCOS */

/*
exports from gr_cache.c
*/

/*
abstract 32-bit cache handle for export
*/

typedef struct GR_CACHE_HANDLE * GR_CACHE_HANDLE, ** P_GR_CACHE_HANDLE; typedef const GR_CACHE_HANDLE * PC_GR_CACHE_HANDLE;

#define GR_CACHE_HANDLE_NONE ((GR_CACHE_HANDLE) 0)

typedef void (* gr_cache_recache_proc) (
    P_ANY handle,
    GR_CACHE_HANDLE cah,
    S32 cres);

#define gr_cache_recache_proto(_e_s, _p_proc_gr_cache_recache) \
_e_s void _p_proc_gr_cache_recache( \
    P_ANY handle, \
    GR_CACHE_HANDLE cah, \
    S32 cres)

/*
exported functions from gr_cache.c
*/

extern void
draw_diag_dispose(
    _Inout_     P_DRAW_DIAG p_draw_diag);

extern void
draw_diag_give_away(
    _Out_       P_DRAW_DIAG p_draw_diag_to,
    _Inout_     P_DRAW_DIAG p_draw_diag_from);

_Check_return_
extern BOOL
gr_cache_can_import(
    _InVal_     FILETYPE_RISC_OS filetype);

_Check_return_
extern STATUS
gr_cache_entry_ensure(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _In_z_      PC_U8Z name);

_Check_return_
extern STATUS
gr_cache_entry_query(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _In_z_      PC_U8Z name);

/*ncr*/
extern S32
gr_cache_entry_remove(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

_Check_return_
extern STATUS
gr_cache_entry_rename(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/,
    _In_z_      PC_U8Z name);

/*ncr*/
extern BOOL
gr_cache_entry_set_autokill(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

_Check_return_
extern STATUS
gr_cache_error_query(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

/*ncr*/
extern STATUS
gr_cache_error_set(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/,
    STATUS err);

_Check_return_
extern STATUS
gr_cache_file_is_chart(
    PC_U8 name);

_Check_return_
extern STATUS
gr_cache_filehan_is_chart(
    FILE_HANDLE fin);

_Check_return_
extern STATUS
gr_cache_fileheader_is_chart(
    _In_reads_bytes_(bytesof_buffer) PC_ANY data,
    _InVal_     U32 bytesof_buffer);

/*ncr*/
extern P_DRAW_DIAG
gr_cache_loaded_ensure(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

/*ncr*/
extern BOOL
gr_cache_name_query(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/,
    char * buffer /*out*/,
    size_t buflen);

/*ncr*/
extern STATUS
gr_cache_recache(
    _In_z_      PC_U8Z name);

_Check_return_
extern STATUS
gr_cache_recache_key(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

/*ncr*/
extern BOOL
gr_cache_recache_inform(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/,
    gr_cache_recache_proc proc,
    P_ANY handle);

/*ncr*/
extern BOOL
gr_cache_ref(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/,
    S32 add);

/*ncr*/
extern S32
gr_cache_reref(
    _InoutRef_  P_GR_CACHE_HANDLE cahp,
    _InRef_     PC_GR_CACHE_HANDLE new_cahp);

_Check_return_
extern S32
gr_cache_refs(
    _In_z_      PC_U8Z name);

_Check_return_
_Ret_maybenull_
extern P_DRAW_DIAG
gr_cache_search(
    _InRef_     PC_GR_CACHE_HANDLE cahp /*const*/);

_Check_return_
_Ret_valid_
extern P_DRAW_DIAG
gr_cache_search_empty(void);

_Check_return_
extern STATUS
gr_cache_tagstrip(
    gr_cache_tagstrip_proc proc,
    P_ANY handle,
    U32 tag,
    S32 add);

/* offset in our internal object/group/diagram (can be large) */
typedef U32 GR_DIAG_OFFSET;  typedef GR_DIAG_OFFSET * P_GR_DIAG_OFFSET; typedef const GR_DIAG_OFFSET * PC_GR_DIAG_OFFSET;

typedef struct GR_RISCDIAG_TAGSTRIP_INFO
{
    void **        ppDiag; /* flex_alloc'ed */
    U32            tag;
    S32            PRM_conformant;
    GR_DIAG_OFFSET thisObject;
    GR_DIAG_OFFSET goopOffset;
    U32 goopSize;
}
GR_RISCDIAG_TAGSTRIP_INFO;

typedef struct GR_CACHE_TAGSTRIP_INFO
{
    GR_RISCDIAG_TAGSTRIP_INFO r;

    GR_CACHE_HANDLE           cah;
}
GR_CACHE_TAGSTRIP_INFO;

typedef struct GR_RISCDIAG
{
    DRAW_DIAG draw_diag; /* data is actually a flex anchor on PipeDream */

    U32 dd_allocsize;

    DRAW_DIAG_OFFSET dd_fontListR;
    DRAW_DIAG_OFFSET dd_fontListW;
    DRAW_DIAG_OFFSET dd_options;
    DRAW_DIAG_OFFSET dd_rootGroupStart;
}
GR_RISCDIAG, * P_GR_RISCDIAG, ** P_P_GR_RISCDIAG; typedef const GR_RISCDIAG * PC_GR_RISCDIAG;

extern STATUS
draw_do_render(
    void * data,
    int length,
    int x,
    int y,
    F64 xfactor,
    F64 yfactor,
    P_GDI_BOX graphics_window);

#endif /* __gr_cache_h */

/* end of gr_cache.h */
