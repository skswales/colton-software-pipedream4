/* quickblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Header file for quickblk.c */

/* SKS November 2006 */

#ifndef __quickblk_h
#define __quickblk_h

#if (CHECKING && 1) || 0
#define QUICK_BLOCK_CHECK 1
#else
#define QUICK_BLOCK_CHECK 0
#endif

/*
U8-based Quick Block (could contain any byte-based info)
*/

typedef struct _quick_block
{
    ARRAY_HANDLE h_array_buffer;
    P_U8 p_static_buffer;
    U32 static_buffer_size; /* NB in bytes */
    U32 static_buffer_used; /* NB in bytes */
}
QUICK_BLOCK, * P_QUICK_BLOCK; typedef const QUICK_BLOCK * PC_QUICK_BLOCK;

#define P_QUICK_BLOCK_NONE _P_DATA_NONE(P_QUICK_BLOCK)

#define QUICK_BLOCK_WITH_BUFFER(name, size) \
    QUICK_BLOCK name; U8 name ## _buffer[size]

/* NB buffer ^^^ immediately follows qb in a structure for subsequent fixup */

/*
functions as macros
*/

#if QUICK_BLOCK_CHECK

static inline void
__aqb_fill(
    _Out_writes_bytes_(sizeof_buffer) P_U8 p_buf,
    _InVal_     U32 sizeof_buffer)
{
    memset32(p_buf, 0xEE, sizeof_buffer);
}

#endif /* QUICK_BLOCK_CHECK */

static inline void
_quick_block_setup(
    _OutRef_    P_QUICK_BLOCK p_quick_block,
#if QUICK_BLOCK_CHECK
    _Out_writes_bytes_(sizeof_buffer) P_U8 p_buf,
#else
    _In_        P_U8 p_buf,
#endif
    _InVal_     U32 sizeof_buffer)
{
    p_quick_block->h_array_buffer     = 0;
    p_quick_block->p_static_buffer    = p_buf;
    p_quick_block->static_buffer_size = sizeof_buffer;
    p_quick_block->static_buffer_used = 0;
#if QUICK_BLOCK_CHECK
    __aqb_fill(p_quick_block->p_static_buffer,  p_quick_block->static_buffer_size);
#endif
}

#define quick_block_with_buffer_setup(name /*ref*/) \
    _quick_block_setup(&name, name ## _buffer, sizeof32(name ## _buffer))

#define quick_block_setup(p_quick_block, buf /*ref*/) \
    _quick_block_setup(p_quick_block, buf, sizeof32(buf))

/* set up a quick_block from an existing buffer (only for xsnprintf - no aqb_init) */
#define quick_block_setup_without_aqb_init(p_quick_block, p_buf, buf_size) ( \
    (p_quick_block)->h_array_buffer     = 0,                    \
    (p_quick_block)->p_static_buffer    = (p_buf),              \
    (p_quick_block)->static_buffer_size = (buf_size),           \
    (p_quick_block)->static_buffer_used = 0                     )

/* set up a quick buf from an existing array handle */
#define quick_block_from_array(p_quick_block, handle) (         \
    (p_quick_block)->h_array_buffer     = (handle),             \
    (p_quick_block)->p_static_buffer    = NULL,                 \
    (p_quick_block)->static_buffer_size = 0,                    \
    (p_quick_block)->static_buffer_used = 0                     )

/* set up a quick_block from an existing buffer */
#define quick_block_fill_from_buf(p_quick_block, p_buf, buf_size) ( \
    (p_quick_block)->h_array_buffer     = 0,                    \
    (p_quick_block)->p_static_buffer    = (p_buf),              \
    (p_quick_block)->static_buffer_size = (buf_size),           \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size )

/* set up a quick_block from an existing const buffer */
#define quick_block_fill_from_const_buf(p_quick_block, p_buf, buf_size) ( \
    (p_quick_block)->h_array_buffer     = 0,                    \
    (p_quick_block)->p_static_buffer    = de_const_cast(P_U8, p_buf), \
    (p_quick_block)->static_buffer_size = (buf_size),           \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size )

#define quick_block_array_handle_ref(p_quick_block) \
    (p_quick_block)->h_array_buffer /* really only internal use but needed for QB sort, no brackets */

_Check_return_
static inline U32
quick_block_bytes(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
        return(array_elements32(&quick_block_array_handle_ref(p_quick_block)));

    return(p_quick_block->static_buffer_used);
}

_Check_return_
_Ret_notnull_
static inline P_U8
quick_block_ptr(
    _InRef_     P_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_base/*_no_checks*/(&quick_block_array_handle_ref(p_quick_block), U8));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline PC_U8
quick_block_ptrc(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_basec/*_no_checks*/(&quick_block_array_handle_ref(p_quick_block), U8));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline PC_U8
quick_block_str(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    return((PC_U8Z) quick_block_ptrc(p_quick_block));
}

/*
exported routines
*/

_Check_return_
extern STATUS
quick_block_bytes_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_reads_bytes_(n_bytes) PC_ANY p_any_in,
    _InVal_     U32 n_bytes);

static inline void
quick_block_dispose(
    _InoutRef_  P_QUICK_BLOCK p_quick_block)
{
#if QUICK_BLOCK_CHECK /* SKS 23jan95 trash buffer on exit */
    if(p_quick_block->static_buffer_size)
        __aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
#endif
    p_quick_block->static_buffer_used = 0;

    if(p_quick_block->h_array_buffer)
        al_array_dispose(&p_quick_block->h_array_buffer);
}

extern void
quick_block_dispose_leaving_text_valid(
    _InoutRef_  P_QUICK_BLOCK p_quick_block);

extern void
quick_block_empty(
    _InoutRef_  P_QUICK_BLOCK p_quick_block);

extern void
quick_block_nullch_strip(
    _InoutRef_  P_QUICK_BLOCK p_quick_block);

_Check_return_
_Ret_maybenull_
extern P_U8
quick_block_extend_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _InVal_     U32 size_wanted,
    _OutRef_    P_STATUS p_status);

extern void
quick_block_shrink_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_        S32 size_wanted); /* NB -ve */

_Check_return_
extern STATUS
quick_block_string_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_      PC_U8Z p_u8);

_Check_return_
extern STATUS
quick_block_string_with_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_      PC_U8Z p_u8);

_Check_return_
extern STATUS
quick_block_u8_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
   _InVal_      U8 u8);

_Check_return_
extern STATUS
quick_block_wchars_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_reads_(len) PCWCH pwch_in,
    _InVal_     U32 len);

_Check_return_
extern STATUS __cdecl
quick_block_printf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...);

_Check_return_
extern STATUS
quick_block_vprintf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        va_list args);

_Check_return_
static inline STATUS
quick_block_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block)
{
    return(quick_block_u8_add(p_quick_block, NULLCH));
}

/*
UCHARS-based quick block
*/

#if USTR_IS_L1STR

#define    QUICK_UBLOCK    QUICK_BLOCK
#define  P_QUICK_UBLOCK  P_QUICK_BLOCK
#define PC_QUICK_UBLOCK PC_QUICK_BLOCK

#define P_QUICK_UBLOCK_NONE P_QUICK_BLOCK_NONE

#define QUICK_UBLOCK_WITH_BUFFER                QUICK_BLOCK_WITH_BUFFER

#define _quick_ublock_setup                     _quick_block_setup

#define quick_ublock_with_buffer_setup          quick_block_with_buffer_setup
#define quick_ublock_setup                      quick_block_setup
#define quick_ublock_setup_without_uqb_init     quick_block_setup_without_aqb_init
#define quick_ublock_from_array                 quick_block_from_array
#define quick_ublock_fill_from_ubuf             quick_block_fill_from_buf
#define quick_ublock_fill_from_const_ubuf       quick_block_fill_from_const_buf

#define quick_ublock_array_handle_ref           quick_block_array_handle_ref
#define quick_ublock_bytes                      quick_block_bytes
#define quick_ublock_uptr                       quick_block_ptr
#define quick_ublock_uptrc                      quick_block_ptrc
#define quick_ublock_ustr                       quick_block_str
#define quick_ublock_ustr_inline                quick_block_str

#define quick_ublock_dispose                    quick_block_dispose
#define quick_ublock_dispose_leaving_text_valid quick_block_dispose_leaving_text_valid
#define quick_ublock_empty                      quick_block_empty
#define quick_ublock_nullch_add                 quick_block_nullch_add
#define quick_ublock_nullch_strip               quick_block_nullch_strip
#define quick_ublock_extend_by                  quick_block_extend_by
#define quick_ublock_l1chars_add                quick_block_bytes_add
#define quick_ublock_l1str_add                  quick_block_string_add
#define quick_ublock_l1str_with_nullch_add      quick_block_string_with_nullch_add
#define quick_ublock_tchar_add                  quick_block_u8_add
#define quick_ublock_tchars_add                 quick_block_bytes_add
#define quick_ublock_tstr_add(q, s)             quick_block_string_add(q, _l1str_from_tstr(s))
#define quick_ublock_tstr_with_nullch_add(q, s) quick_block_string_with_nullch_add(q, _l1str_from_tstr(s))
#define quick_ublock_u8_add                     quick_block_u8_add
#define quick_ublock_uchars_add                 quick_block_bytes_add
#define quick_ublock_ustr_add                   quick_block_string_add
#define quick_ublock_ustr_with_nullch_add       quick_block_string_with_nullch_add
#define quick_ublock_wchars_add                 quick_block_wchars_add
#define quick_ublock_printf                     quick_block_printf
#define quick_ublock_vprintf                    quick_block_vprintf

#endif /* USTR_IS_L1STR */

/*
TCHAR-based quick block
*/

#if TSTR_IS_L1STR

#define    QUICK_TBLOCK    QUICK_BLOCK
#define  P_QUICK_TBLOCK  P_QUICK_BLOCK
#define PC_QUICK_TBLOCK PC_QUICK_BLOCK

#define P_QUICK_TBLOCK_NONE P_QUICK_BLOCK_NONE

#define QUICK_TBLOCK_WITH_BUFFER                QUICK_BLOCK_WITH_BUFFER

#define _quick_tblock_setup                     _quick_block_setup

#define quick_tblock_with_buffer_setup          quick_block_with_buffer_setup
#define quick_tblock_setup                      quick_block_setup
#define quick_tblock_setup_without_tqb_init     quick_block_setup_without_aqb_init
#define quick_tblock_from_array                 quick_block_from_array
#define quick_tblock_fill_from_tbuf             quick_block_fill_from_buf

#define quick_tblock_array_handle_ref           quick_block_array_handle_ref
#define quick_tblock_elements                   quick_block_bytes
#define quick_tblock_tptr                       quick_block_ptr
#define quick_tblock_tptrc                      quick_block_ptrc
#define quick_tblock_tstr                       quick_block_str

#define quick_tblock_dispose                    quick_block_dispose
#define quick_tblock_dispose_leaving_text_valid quick_block_dispose_leaving_text_valid
#define quick_tblock_empty                      quick_block_empty
#define quick_tblock_nullch_add                 quick_block_nullch_add
#define quick_tblock_nullch_strip               quick_block_nullch_strip
#define quick_tblock_extend_by                  quick_block_extend_by
#define quick_tblock_tchar_add                  quick_block_u8_add
#define quick_tblock_tchars_add                 quick_block_bytes_add
#define quick_tblock_tstr_add                   quick_block_string_add
#define quick_tblock_tstr_with_nullch_add       quick_block_string_with_nullch_add
#define quick_tblock_printf                     quick_block_printf
#define quick_tblock_vprintf                    quick_block_vprintf

#endif /* TSTR_IS_L1STR */

#endif /* __quickblk_h */

/* end of quickblk.h */
