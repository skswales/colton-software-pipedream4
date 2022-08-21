/* im_cache.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Image file cache */

/* SKS May 1991  */

#ifndef __im_cache_h
#define __im_cache_h

/*
exported typedefs from im_cache.c
*/

typedef struct DRAW_DIAG
{
    void * data;
    U32 length;
}
DRAW_DIAG, * P_DRAW_DIAG, ** P_P_DRAW_DIAG; typedef const DRAW_DIAG * PC_DRAW_DIAG;

#if RISCOS

/*
image cache tagstrip
*/

typedef struct IMAGE_CACHE_TAGSTRIP_INFO * P_IMAGE_CACHE_TAGSTRIP_INFO;

typedef S32 (* image_cache_tagstrip_proc) (
    P_ANY handle,
    P_IMAGE_CACHE_TAGSTRIP_INFO p_info);

#define image_cache_tagstrip_proto(_e_s, _p_proc_image_cache_tagstrip) \
_e_s S32 _p_proc_image_cache_tagstrip( \
    P_ANY handle, \
    P_IMAGE_CACHE_TAGSTRIP_INFO p_info)

#endif /* RISCOS */

/*
exports from im_cache.c
*/

/*
abstract 32-bit cache handle for export
*/

typedef struct IMAGE_CACHE_HANDLE * IMAGE_CACHE_HANDLE, ** P_IMAGE_CACHE_HANDLE; typedef const IMAGE_CACHE_HANDLE * PC_IMAGE_CACHE_HANDLE;

#define IMAGE_CACHE_HANDLE_NONE ((IMAGE_CACHE_HANDLE) 0)

typedef void (* image_cache_recache_proc) (
    P_ANY handle,
    IMAGE_CACHE_HANDLE cah,
    S32 cres);

#define image_cache_recache_proto(_e_s, _p_proc_image_cache_recache) \
_e_s void _p_proc_image_cache_recache( \
    P_ANY handle, \
    IMAGE_CACHE_HANDLE cah, \
    S32 cres)

/*
exported functions from im_cache.c
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
image_cache_can_import(
    _InVal_     FILETYPE_RISC_OS filetype);

_Check_return_
extern STATUS
image_cache_entry_ensure(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PC_U8Z name);

_Check_return_
extern STATUS
image_cache_entry_query(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PC_U8Z name);

extern void
image_cache_entry_remove(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

_Check_return_
extern STATUS
image_cache_entry_rename(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    _In_z_      PC_U8Z name);

/*ncr*/
extern BOOL
image_cache_entry_set_autokill(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

_Check_return_
extern STATUS
image_cache_error_query(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

/*ncr*/
extern STATUS
image_cache_error_set(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    STATUS err);

_Check_return_
extern STATUS
image_cache_file_is_chart(
    PC_U8 name);

_Check_return_
extern STATUS
image_cache_filehan_is_chart(
    FILE_HANDLE fin);

_Check_return_
extern STATUS
image_cache_fileheader_is_chart(
    _In_reads_bytes_(bytesof_buffer) PC_ANY data,
    _InVal_     U32 bytesof_buffer);

/*ncr*/
extern P_DRAW_DIAG
image_cache_loaded_ensure(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

/*ncr*/
extern BOOL
image_cache_name_query(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    char * buffer /*out*/,
    size_t buflen);

/*ncr*/
extern STATUS
image_cache_recache(
    _In_z_      PC_U8Z name);

_Check_return_
extern STATUS
image_cache_recache_key(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

/*ncr*/
extern BOOL
image_cache_recache_inform(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    image_cache_recache_proc proc,
    P_ANY handle);

/*ncr*/
extern BOOL
image_cache_ref(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/,
    S32 add);

/*ncr*/
extern S32
image_cache_reref(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InRef_     PC_IMAGE_CACHE_HANDLE new_image_cache_handle);

_Check_return_
extern S32
image_cache_refs(
    _In_z_      PC_U8Z name);

_Check_return_
_Ret_maybenull_
extern P_DRAW_DIAG
image_cache_search(
    _InRef_     PC_IMAGE_CACHE_HANDLE p_image_cache_handle /*const*/);

_Check_return_
_Ret_valid_
extern P_DRAW_DIAG
image_cache_search_empty(void);

_Check_return_
extern STATUS
image_cache_tagstripper_add(
    image_cache_tagstrip_proc proc,
    P_ANY handle,
    _InVal_     U32 tag);

extern void
image_cache_tagstripper_remove(
    image_cache_tagstrip_proc proc,
    P_ANY handle);

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

typedef struct IMAGE_CACHE_TAGSTRIP_INFO
{
    GR_RISCDIAG_TAGSTRIP_INFO r;

    IMAGE_CACHE_HANDLE image_cache_handle;
}
IMAGE_CACHE_TAGSTRIP_INFO;

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

/*
im_convert.c
*/

_Check_return_
extern BOOL
image_convert_can_convert(
    _InVal_     FILETYPE_RISC_OS filetype);

_Check_return_
extern STATUS
image_convert_do_convert(
    _OutRef_    P_PTSTR p_converted_name,
    _In_z_      PCTSTR name);

#endif /* __im_cache_h */

/* end of im_cache.h */
