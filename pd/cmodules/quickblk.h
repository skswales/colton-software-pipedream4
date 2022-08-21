/* quickblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Header file for quickblk.c (BYTE-based quick block - can contain any byte-based info) */

/* SKS November 2006 split out of aligator.h */

#ifndef __quickblk_h
#define __quickblk_h

#if CHECKING && 1
#define CHECKING_QUICK_BLOCK 1
#else
#define CHECKING_QUICK_BLOCK 0
#endif

typedef struct QUICK_BLOCK
{
    ARRAY_HANDLE h_array_buffer;
    P_BYTE p_static_buffer;
    U32 static_buffer_size; /* NB in bytes */
    U32 static_buffer_used; /* NB in bytes */
}
QUICK_BLOCK, * P_QUICK_BLOCK; typedef const QUICK_BLOCK * PC_QUICK_BLOCK;

#define P_QUICK_BLOCK_NONE _P_DATA_NONE(P_QUICK_BLOCK)

#define QUICK_BLOCK_WITH_BUFFER(name, size) \
    QUICK_BLOCK name; BYTE name ## _buffer[size]

/* NB buffer ^^^ immediately follows QB in a structure for subsequent fixup */

#define QUICK_BLOCK_INIT_NULL() \
    { 0, NULL, 0, 0 }

#define quick_block_array_handle_ref(p_quick_block) \
    (p_quick_block)->h_array_buffer /* really only internal use but needed for QB sort, no brackets */

/*
functions as macros
*/

#if CHECKING_QUICK_BLOCK

static inline void
_do_aqb_fill(
    _Out_writes_bytes_(bytesof_buffer) P_BYTE buf,
    _InVal_     U32 bytesof_buffer)
{
    (void) memset(buf, 0xEE, bytesof_buffer);
}

#else
#define _do_aqb_fill(buf, bytesof_buffer) /*nothing*/
#endif /* CHECKING_QUICK_BLOCK */

static inline void
_quick_block_setup(
    _OutRef_    P_QUICK_BLOCK p_quick_block /*set up*/,
#if CHECKING_QUICK_BLOCK
    _Out_writes_bytes_(bytesof_buffer) P_BYTE buf,
#else
    _In_        P_BYTE buf,
#endif
    _InVal_     U32 bytesof_buffer)
{
    p_quick_block->h_array_buffer     = 0;
    p_quick_block->p_static_buffer    = buf;
    p_quick_block->static_buffer_size = bytesof_buffer;
    p_quick_block->static_buffer_used = 0;

    _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
}

#define quick_block_with_buffer_setup(name /*ref*/) \
    _quick_block_setup(&name, name ## _buffer, sizeof32(name ## _buffer))

#define quick_block_setup(p_quick_block, buf /*ref*/) \
    _quick_block_setup(p_quick_block, buf, sizeof32(buf))

/* set up a quick_block from an existing buffer (no _do_aqb_fill - see xvsnprintf), not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_without_clearing_buf(p_quick_block, buf, buf_size) (      \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->p_static_buffer    = (buf),                                    \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = 0                                         )

/* set up a quick buf from an existing array handle, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_using_array(p_quick_block, handle) (                      \
    (p_quick_block)->h_array_buffer     = (handle),                                 \
    (p_quick_block)->p_static_buffer    = NULL,                                     \
    (p_quick_block)->static_buffer_size = 0,                                        \
    (p_quick_block)->static_buffer_used = 0                                         )

/* set up a quick_block from an existing buffer, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_fill_from_buf(p_quick_block, buf, buf_size) (             \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->p_static_buffer    = (buf),                                    \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size       )

/* set up a quick_block from an existing const buffer, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_fill_from_const_buf(p_quick_block, buf, buf_size) (       \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->p_static_buffer    = de_const_cast(P_BYTE, (buf)),             \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size       )

/*
exported routines
*/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern P_BYTE
quick_block_extend_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status);

extern void
quick_block_shrink_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*shrunk*/,
    _InVal_     S32 shrink_by); /* NB -ve */

extern void
quick_block_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*disposed*/);

extern void
quick_block_empty(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*emptied*/);

_Check_return_
extern STATUS
quick_block_byte_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _InVal_     BYTE u8);

_Check_return_
extern STATUS
quick_block_bytes_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_reads_bytes_(n_bytes) PC_ANY p_any,
    _InVal_     U32 n_bytes);

_Check_return_
extern STATUS
quick_block_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/);

extern void
quick_block_nullch_strip(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*stripped*/);

_Check_return_
extern STATUS __cdecl
quick_block_printf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...);

_Check_return_
extern STATUS
quick_block_vprintf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        va_list args);

static inline void
quick_block_dispose(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*disposed*/)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
#if CHECKING_QUICK_BLOCK
        /* trash handle on dispose when CHECKING_QUICK_BLOCK */
        const U32 n_bytes = array_elements32(&quick_block_array_handle_ref(p_quick_block));
        _do_aqb_fill(array_range(&quick_block_array_handle_ref(p_quick_block), BYTE, 0, n_bytes), n_bytes);
#endif
        al_array_dispose(&quick_block_array_handle_ref(p_quick_block));
    }

    /* trash buffer on dispose when CHECKING_QUICK_BLOCK */
    _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
    p_quick_block->static_buffer_used = 0;
}

_Check_return_
static inline BOOL
quick_block_byte_add_fast(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _InVal_     BYTE u8)
{
    const U32 static_buffer_used = p_quick_block->static_buffer_used;
    const BOOL fast_possible = (p_quick_block->static_buffer_size != static_buffer_used);

    if(fast_possible)
    {
        p_quick_block->static_buffer_used = static_buffer_used + 1;
        PtrPutByteOff(p_quick_block->p_static_buffer, static_buffer_used, u8);
    }

    return(fast_possible);
}

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
static inline PC_BYTE
quick_block_ptr(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_basec_no_checks(&quick_block_array_handle_ref(p_quick_block), BYTE));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline P_BYTE
quick_block_ptr_wr(
    _InRef_     P_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_base_no_checks(&quick_block_array_handle_ref(p_quick_block), BYTE));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_z_
static inline PC_U8Z
quick_block_str(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    return((PC_U8Z) quick_block_ptr(p_quick_block));
}

/*
UCHARS-based quick block (for Fireworkz-derived numform and evaluator functions)
*/

#if USTR_IS_L1STR

#define    QUICK_UBLOCK    QUICK_BLOCK
#define  P_QUICK_UBLOCK  P_QUICK_BLOCK
#define PC_QUICK_UBLOCK PC_QUICK_BLOCK

#define P_QUICK_UBLOCK_NONE P_QUICK_BLOCK_NONE

#define QUICK_UBLOCK_WITH_BUFFER                    QUICK_BLOCK_WITH_BUFFER

#define _quick_ublock_setup                         _quick_block_setup

#define quick_ublock_with_buffer_setup              quick_block_with_buffer_setup
#define quick_ublock_setup                          quick_block_setup
#define quick_ublock_setup_without_clearing_buf     quick_block_setup_without_clearing_buf
#define quick_ublock_setup_using_array              quick_block_setup_using_array
#define quick_ublock_setup_fill_from_ubuf           quick_block_setup_fill_from_buf
#define quick_ublock_setup_fill_from_const_ubuf     quick_block_setup_fill_from_const_buf

#define quick_ublock_array_handle_ref               quick_block_array_handle_ref
#define quick_ublock_bytes                          quick_block_bytes
#define quick_ublock_uptr                           quick_block_ptr
#define quick_ublock_uptr_wr                        quick_block_ptr_wr
#define quick_ublock_ustr                           quick_block_str

#define quick_ublock_extend_by                      quick_block_extend_by
#define quick_ublock_dispose                        quick_block_dispose
#define quick_ublock_dispose_leaving_buffer_valid   quick_block_dispose_leaving_buffer_valid
#define quick_ublock_empty                          quick_block_empty
#define quick_ublock_a7char_add                     quick_block_byte_add
#define quick_ublock_ucs4_add                       quick_block_byte_add
#define quick_ublock_uchars_add                     quick_block_bytes_add
#define quick_ublock_nullch_add                     quick_block_nullch_add
#define quick_ublock_nullch_strip                   quick_block_nullch_strip
#define quick_ublock_printf                         quick_block_printf
#define quick_ublock_vprintf                        quick_block_vprintf

_Check_return_
extern STATUS
quick_ublock_ustr_add(
    _InoutRef_      P_QUICK_UBLOCK p_quick_ublock,
    _In_z_          PC_USTR ustr);

#endif /* USTR_IS_L1STR */

/*
TCHAR-based quick block (for Fireworkz-derived numform)
*/

#if TSTR_IS_L1STR

#define    QUICK_TBLOCK    QUICK_BLOCK
#define  P_QUICK_TBLOCK  P_QUICK_BLOCK
#define PC_QUICK_TBLOCK PC_QUICK_BLOCK

#define P_QUICK_TBLOCK_NONE P_QUICK_BLOCK_NONE

#define QUICK_TBLOCK_WITH_BUFFER                    QUICK_BLOCK_WITH_BUFFER

#define _quick_tblock_setup                         _quick_block_setup

#define quick_tblock_with_buffer_setup              quick_block_with_buffer_setup
#define quick_tblock_setup                          quick_block_setup
#define quick_tblock_setup_without_clearing_buf     quick_block_setup_without_clearing_buf
#define quick_tblock_setup_using_array              quick_block_setup_using_array
#define quick_tblock_setup_fill_from_tbuf           quick_block_setup_fill_from_buf

#define quick_tblock_array_handle_ref               quick_block_array_handle_ref
#define quick_tblock_elements                       quick_block_bytes
#define quick_tblock_tptr                           quick_block_ptr
#define quick_tblock_tptr_wr                        quick_block_ptr_wr
#define quick_tblock_tstr                           quick_block_str

#define quick_tblock_extend_by                      quick_block_extend_by
#define quick_tblock_dispose                        quick_block_dispose
#define quick_tblock_dispose_leaving_buffer_valid   quick_block_dispose_leaving_buffer_valid
#define quick_tblock_empty                          quick_block_empty
#define quick_tblock_tchar_add                      quick_block_byte_add
#define quick_tblock_tchars_add                     quick_block_bytes_add
#define quick_tblock_nullch_add                     quick_block_nullch_add
#define quick_tblock_nullch_strip                   quick_block_nullch_strip

#endif /* TSTR_IS_L1STR */

#endif /* __quickblk_h */

/* end of quickblk.h */
