/* aligator.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Header file for aligator.c */

/* MRJC December 1991 */

#ifndef __aligator_h
#define __aligator_h

#if defined(ALIGATOR_USE_ALLOC) && !defined(__alloc_h)
#include "cmodules/alloc.h"
#endif

/*
prototypes for full-event clients
*/

typedef U32 (* P_PROC_AL_FULL_EVENT) (
    _InVal_     U32 n_bytes);

_Check_return_
extern STATUS
al_full_event_client_register(
    _In_        P_PROC_AL_FULL_EVENT proc);

/*
malloc() replacement
*/

_Check_return_
_Ret_writes_bytes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_al_ptr_alloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_alloc_bytes(__base_type, n_bytes, p_status) ( \
    (__base_type *) _al_ptr_alloc((n_bytes), p_status) )

#define al_ptr_alloc_elem(__base_type, n_elem, p_status) ( \
    (__base_type *) _al_ptr_alloc((n_elem) * sizeof(__base_type), p_status) )

/*
calloc() replacement
*/

_Check_return_
_Ret_writes_bytes_maybenull_(n_bytes) /* may be NULL */
extern P_BYTE
_al_ptr_calloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_calloc_bytes(__base_type, n_bytes, p_status) ( \
    (__base_type *) _al_ptr_calloc((n_bytes), p_status) )

#define al_ptr_calloc_elem(__base_type, n_elem, p_status) ( \
    (__base_type *) _al_ptr_calloc((n_elem) * sizeof(__base_type), p_status) )
/* NB different args and arg order to calloc() */

/*
free() replacement
*/

extern void
al_ptr_free(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any);

static __forceinline void
al_ptr_dispose(
    _InoutRef_opt_ P_P_ANY p_p_any)
{
    if(NULL != p_p_any)
    {
        P_ANY p_any = *p_p_any;

        if(NULL != p_any)
        {
            *p_p_any = NULL;
            al_ptr_free(p_any);
        }
    }
}

/*
realloc() replacement
*/

_Check_return_
_Ret_writes_bytes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_al_ptr_realloc(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any,
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_realloc_bytes(__base_type, p_any, n_bytes, p_status) ( \
    (__base_type *) _al_ptr_realloc(p_any, (n_bytes), p_status) )

#define al_ptr_realloc_elem(__base_type, p_any, n_elem, p_status) ( \
    (__base_type *) _al_ptr_realloc(p_any, (n_elem) * sizeof(__base_type), p_status) )

/*
definition of al_array_xxx index type
*/

typedef S32 ARRAY_INDEX; typedef ARRAY_INDEX * P_ARRAY_INDEX;

typedef U32 ARRAY_HANDLE; typedef ARRAY_HANDLE * P_ARRAY_HANDLE, ** P_P_ARRAY_HANDLE; typedef const ARRAY_HANDLE * PC_ARRAY_HANDLE;

#define P_ARRAY_HANDLE_NONE _P_DATA_NONE(P_ARRAY_HANDLE)

/*
structure of array space allocator
*/

typedef struct _array_block_parms
{
    /* private to aligator - export only for macros */
    UBF auto_compact            : 1;
    UBF compact_off             : 1;
    UBF entry_free              : 1;
    UBF clear_new_block         : 1;
#if WINDOWS
    UBF use_GlobalAlloc         : 1; /* use GlobalAlloc()/GlobalLock()/GlobalFree() for HANDLE based allocation */
#else
    UBF _spare                  : 1;
#endif
#if WINDOWS
    UBF _spare_was_packed_element_size     : 14;
    UBF _spare_was_packed_size_increment   : 13;
#elif RISCOS
    UBF packed_element_size     : 14; /* needed visible for array_element_size32() */
    UBF packed_size_increment   : 13;
#endif
    /* SKS after 1.06 09nov93 reordered to give compiler a good chance to use those wonderful shift operators */
}
ARRAY_BLOCK_PARMS;

typedef struct _array_block
{
    /* private to aligator - export only for macros */
    P_ANY               p_any;
    ARRAY_INDEX         free;
    ARRAY_INDEX         size;
    ARRAY_BLOCK_PARMS   parms;

#if WINDOWS /* SKS 24feb2012 unpacked */
    U32                 element_size;
    U32                 size_increment;
#if defined(_WIN64)
    U32                 _padding; /* round to 8 words */
#endif
#endif /* WINDOWS */
}
ARRAY_BLOCK, * P_ARRAY_BLOCK; typedef const ARRAY_BLOCK * PC_ARRAY_BLOCK;

#define P_ARRAY_BLOCK_NONE _P_DATA_NONE(P_ARRAY_BLOCK)

#if WINDOWS
/* return size of an element in array (with valid block) */
#define array_block_element_size(p_array_block) \
    (p_array_block->element_size)

#define array_block_size_increment(p_array_block) \
    (p_array_block->size_increment)
#elif RISCOS
/* return size of an element in array (with valid block) */
#define array_block_element_size(p_array_block) \
    ((U32) p_array_block->parms.packed_element_size)

#define array_block_size_increment(p_array_block) \
    ((U32) p_array_block->parms.packed_size_increment)
#endif /* OS */

/*
different typedef for root allocation allows us to see better in debugger
eg watch and expand array_root.p_array_block[25]
and it also makes aligator.c implementation simpler
*/

typedef struct _array_root_block
{
    /* private to aligator - export only for macros */
    PC_ARRAY_BLOCK      p_array_block; /* NB const makes for safer access outside of aligator */
    U32                 free;
    U32                 size;
    ARRAY_BLOCK_PARMS   parms;

#if WINDOWS /* SKS 24feb2012 unpacked */
    U32                 element_size;
    U32                 size_increment;
#if defined(_WIN64)
    U32                 _padding; /* round to 8 words */
#endif
#endif /* WINDOWS */
}
ARRAY_ROOT_BLOCK;

/* NB. SKS 1.03 19mar93 made handle zero info kosher - ie. NULL pointer, zero size, zero element size etc. */

/*
functions as macros
*/

/* NB These ones are for aligator internal use only (when handle has been validated) */

/* return pointer to first array element */
#define array_base_no_checks(pc_array_handle, __base_type) ( \
    ((__base_type *) (array_blockc_no_checks(pc_array_handle)->p_any)) )

/* return const pointer to first array element */
#define array_basec_no_checks(pc_array_handle, __base_type) ( \
    ((const __base_type *) (array_blockc_no_checks(pc_array_handle)->p_any)) )

/* return const pointer to array block for given handle - NB. for internal use only */
#define array_blockc_no_checks(pc_array_handle) ( \
    array_root.p_array_block + *(pc_array_handle) )

/* return number of used elements in array */
#define array_elements_no_checks(pc_array_handle) ( \
    array_blockc_no_checks(pc_array_handle)->free )

/* return number of used elements in array (unsigned 32-bit) */
#define array_elements32_no_checks(pc_array_handle) \
    (U32) array_elements_no_checks(pc_array_handle)

/* return size of an element in array */
#define array_element_size32_no_checks(pc_array_handle) (  \
    array_block_element_size(array_blockc_no_checks(pc_array_handle)) )

/* return pointer to array element - NB. pc_array_handle must point to a valid handle */
#define array_ptr_no_checks(pc_array_handle, __base_type, ele) ( \
    ((__base_type *) array_blockc_no_checks(pc_array_handle)->p_any) + (ele) )

/* return const pointer to array element */
#define array_ptrc_no_checks(pc_array_handle, __base_type, ele) ( \
    ((const __base_type *) array_blockc_no_checks(pc_array_handle)->p_any) + (ele) )

#if CHECKING

/* return pointer to first array element */

_Check_return_
_Ret_/*opt_*/
extern P_ANY
_array_base_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_base(pc_array_handle, __base_type) \
    ((__base_type *) _array_base_check(pc_array_handle))

#define array_basec(pc_array_handle, __base_type) \
    ((const __base_type *) _array_base_check(pc_array_handle))

/* return pointer to array block for given handle - NB. for internal use only */

_Check_return_
_Ret_/*opt_*/
extern PC_ARRAY_BLOCK
_array_block(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_blockc(pc_array_handle) \
    _array_block(pc_array_handle)

/* return number of used elements in array */

_Check_return_
extern ARRAY_INDEX
_array_elements_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_elements(pc_array_handle) \
    _array_elements_check(pc_array_handle)

#define array_elements32(pc_array_handle) \
    (U32) _array_elements_check(pc_array_handle)

/* return pointer to array element */

_Check_return_
_Ret_/*opt_*/
extern PC_ARRAY_BLOCK
_array_ptr_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     ARRAY_INDEX ele);

#define array_ptr(pc_array_handle, __base_type, ele) ( \
    ((__base_type *) _array_ptr_check(pc_array_handle, (ele))->p_any) + (ele) )

#define array_ptrc(pc_array_handle, __base_type, ele) ( \
    ((const __base_type *) _array_ptr_check(pc_array_handle, (ele))->p_any) + (ele) )

#else /* NOT CHECKING */

/* return pointer to first array element */
#define array_base(pc_array_handle, __base_type) ( \
    ((__base_type *) (array_blockc(pc_array_handle)->p_any)) )

#define array_basec(pc_array_handle, __base_type) ( \
    ((const __base_type *) (array_blockc(pc_array_handle)->p_any)) )

/* return const pointer to array block for given handle - NB. for internal use only */
#define array_blockc(pc_array_handle) ( \
    array_root.p_array_block + *(pc_array_handle) )

/* return number of used elements in array */
#define array_elements(pc_array_handle) ( \
    array_blockc(pc_array_handle)->free )

/* return number of used elements in array (unsigned 32-bit) */
#define array_elements32(pc_array_handle) ( \
    (U32) array_blockc(pc_array_handle)->free )

/* return pointer to array element - NB. pc_array_handle must point to a valid handle */
#define array_ptr(pc_array_handle, __base_type, ele) ( \
    ((__base_type *) array_blockc(pc_array_handle)->p_any) + (ele) )

#define array_ptrc(pc_array_handle, __base_type, ele) ( \
    ((const __base_type *) array_blockc(pc_array_handle)->p_any) + (ele) )

#endif /* CHECKING */

/* return the size of an element in array */
#define array_element_size32(pc_array_handle) (  \
    array_block_element_size(array_blockc(pc_array_handle)) )

/* return whether given array handle is valid (NB doesn't check for handle zero) */
#define array_handle_valid(pc_array_handle) ( \
    (U32) *(pc_array_handle) < array_root.free )

/* return whether given index is valid in array */
#define array_index_valid(pc_array_handle, ele) ( \
    (U32) (ele) < array_elements32(pc_array_handle) )

/* return the element index of a pointer to an element in an array */
#define array_indexof_element(p_handle, __base_type, ptr) ( \
    (ARRAY_INDEX) ((ptr) - array_base(p_handle, __base_type)) )

/* return the size of an array (NB includes unallocated elements; you probably want array_elements) */
#define array_size32(pc_array_handle) ( \
    (U32) array_blockc(pc_array_handle)->size )

/*
block passed to al_array_alloc/realloc
*/

typedef struct _array_init_block
{
    ARRAY_INDEX size_increment;     /* number of array elements to allocate at a time */
    U32         element_size;       /* sizeof32() the type stored */
    U8          clear_new_block;    /* boolean; zeroes allocated chunks */
#if WINDOWS
    U8          use_GlobalAlloc;    /* boolean; see ARRAY_BLOCK_PARMS */
#else
    U8          not_GlobalAlloc;
#endif
    U8          _spare[2];
}
ARRAY_INIT_BLOCK; typedef const ARRAY_INIT_BLOCK * PC_ARRAY_INIT_BLOCK;

#define PC_ARRAY_INIT_BLOCK_NONE _P_DATA_NONE(PC_ARRAY_INIT_BLOCK)

#define SC_ARRAY_INIT_BLOCK static const ARRAY_INIT_BLOCK /* an ARRAY_INIT_BLOCK out in const area */

#define aib_init(size_increment, element_size, clear_new_block) \
    { (size_increment), (element_size), (clear_new_block), 0, { 0, 0 } }

#if WINDOWS
#define array_init_block_setup(block, a, b, c) ( \
    (block)->size_increment   = (a), \
    (block)->element_size     = (b), \
    (block)->clear_new_block  = (c), \
    (block)->use_GlobalAlloc  = 0    )
#else
#define array_init_block_setup(block, a, b, c) ( \
    (block)->size_increment  = (a), \
    (block)->element_size    = (b), \
    (block)->clear_new_block = (c)  )
#endif

/*
remove deleted info
*/

typedef S32 (* P_PROC_ELEMENT_DELETED) (
    P_ANY p_any);

#define PROC_ELEMENT_DELETED_PROTO(_e_s, _proc_name) \
_e_s S32 \
_proc_name( \
    P_ANY p_any)

typedef struct _al_garbage_flags
{
    UBF remove_deleted : 1;
    UBF shrink : 1;
    UBF may_dispose : 1;
    UBF spare_1 : 8 - (3*1);
    UBF spare_2 : 8;
    UBF spare_3 : 8;
    UBF spare_4 : 8;
}
AL_GARBAGE_FLAGS, * P_AL_GARBAGE_FLAGS;

#define AL_GARBAGE_FLAGS_INIT \
    { 0, 0, 0,  0, 0, 0, 0 }

#if defined(UNUSED)

#define AL_GARBAGE_FLAGS_CLEAR(p_al_garbage_flags) \
    * (P_U32) p_al_garbage_flags = 0

#endif /* UNUSED */

/*
exported data
*/

extern ARRAY_ROOT_BLOCK array_root;

/*
exported routines
*/

_Check_return_
extern STATUS
aligator_init(void);

extern void
aligator_exit(void);

_Check_return_
extern STATUS
_al_array_add(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 num_elements,
    _InRef_opt_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _In_reads_bytes_(bytesof_elem_x_num_elem) PC_ANY p_data_in /*copied*/
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_add(p_array_handle, __base_type, num_elements, p_array_init_block, p_data_in) \
    _al_array_add(p_array_handle, num_elements, p_array_init_block, p_data_in \
    PREFAST_ONLY_ARG((num_elements) * sizeof32(__base_type)))

_Check_return_
_Ret_writes_bytes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_alloc(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 num_elements,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_alloc(p_array_handle, __base_type, num_elements, p_array_init_block, p_status) ( \
    (__base_type *) _al_array_alloc(p_array_handle, num_elements, p_array_init_block, p_status \
    PREFAST_ONLY_ARG((num_elements) * sizeof32(__base_type))) )

_Check_return_
extern STATUS
al_array_alloc_zero(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block);

_Check_return_
extern STATUS
al_array_preallocate_zero(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block);

extern void
al_array_auto_compact_set(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

_Check_return_
extern ARRAY_INDEX
_al_array_bfind(
    _In_reads_bytes_(bytesof_elem) PC_ANY key,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_bfind(key, p_array_handle, __base_type, p_proc_bsearch, p_hit) ( \
    _al_array_bfind(key, p_array_handle, p_proc_bsearch, p_hit \
    PREFAST_ONLY_ARG(sizeof32(__base_type))) )

_Check_return_
_Ret_writes_bytes_maybenull_(bytesof_elem)
extern P_ANY
_al_array_bsearch(
    _In_        PC_ANY key,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_bsearch(key, p_array_handle, __base_type, p_proc_bsearch) ( \
    (__base_type *) _al_array_bsearch(key, p_array_handle, p_proc_bsearch \
    PREFAST_ONLY_ARG(sizeof32(__base_type))) )

_Check_return_
_Ret_writes_bytes_maybenull_(bytesof_elem)
extern P_ANY
_al_array_lsearch(
    _In_        PC_ANY key,
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_lsearch(key, p_array_handle, __base_type, p_proc_bsearch) ( \
    (__base_type *) _al_array_lsearch(key, p_array_handle, p_proc_bsearch \
    PREFAST_ONLY_ARG(sizeof32(__base_type))) )

extern void
__al_array_free(
    _InVal_     ARRAY_HANDLE array_handle);

static __forceinline void
al_array_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    ARRAY_HANDLE array_handle = *p_array_handle;

    if(0 != array_handle)
    {
        *p_array_handle = 0;
        __al_array_free(array_handle);
    }
}

_Check_return_
extern STATUS
al_array_duplicate(
    _OutRef_    P_ARRAY_HANDLE p_dup_array_handle,
    _InRef_     PC_ARRAY_HANDLE pc_src_array_handle);

extern void
al_array_empty(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

/*ncr*/
extern S32 /* number of elements remaining */
al_array_garbage_collect(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     ARRAY_INDEX element_start,
    _In_opt_    P_PROC_ELEMENT_DELETED p_proc_element_deleted,
    _InVal_     AL_GARBAGE_FLAGS al_garbage_flags);

_Check_return_
extern S32 /* number of allocated handles */
al_array_handle_check(
    _InVal_     S32 winge /* winge about allocated handles */);

extern void
al_array_delete_at(
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements, /* NB num_elements still -ve */
    _InVal_     ARRAY_INDEX delete_at);

_Check_return_
_Ret_writes_bytes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_insert_before(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements,
    _InRef_opt_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status,
    _InVal_     ARRAY_INDEX insert_before
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_insert_before(p_array_handle, __base_type, num_elements, p_array_init_block, p_status, insert_before) ( \
    (__base_type *) _al_array_insert_before(p_array_handle, num_elements, p_array_init_block, p_status, insert_before \
    PREFAST_ONLY_ARG((num_elements) * sizeof32(__base_type))) )

extern void
al_array_qsort(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_QSORT p_proc_compare_qsort);

extern void /* declared as al_array_qsort replacement */
al_array_check_sorted(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_QSORT p_proc_compare_qsort);

_Check_return_
_Ret_writes_bytes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_extend_by(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 add_elements,
    _InRef_opt_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status
    PREFAST_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_extend_by(p_array_handle, __base_type, add_elements, p_array_init_block, p_status) ( \
    (__base_type *) _al_array_extend_by(p_array_handle, add_elements, p_array_init_block, p_status \
    PREFAST_ONLY_ARG((add_elements) * sizeof32(__base_type))) )

extern void
al_array_shrink_by(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements); /* NB num_elements still -ve */

#if WINDOWS

extern void
al_array_resized(
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _In_        HGLOBAL hglobal,
    _InVal_     U32 size);

_Check_return_
_Ret_maybenull_
extern HGLOBAL
al_array_steal_hglobal(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle);

#endif

extern void
al_array_trim(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

#if WINDOWS

_Check_return_
_Ret_writes_bytes_to_maybenull_(dwBytes, 0) /* may be NULL */
extern P_BYTE
GlobalAllocAndLock(
    _InVal_     UINT uFlags,
    _InVal_     U32 dwBytes,
    _Out_       HGLOBAL * const p_hglobal);

#endif /* OS */

#if TRACE_ALLOWED

extern void
al_list_big_handles(
    _InVal_     U32 over_n_bytes);

#endif

/*
simply-typed variants of ARRAY_HANDLE
*/

#define    ARRAY_HANDLE_L1STR    ARRAY_HANDLE
#define  P_ARRAY_HANDLE_L1STR  P_ARRAY_HANDLE

#define    ARRAY_HANDLE_USTR     ARRAY_HANDLE
#define  P_ARRAY_HANDLE_USTR   P_ARRAY_HANDLE
#define PC_ARRAY_HANDLE_USTR  PC_ARRAY_HANDLE

#define array_ustr(pc_array_handle_ustr) \
    array_basec(pc_array_handle_ustr, UCHARZ)

#define    ARRAY_HANDLE_TSTR     ARRAY_HANDLE
#define  P_ARRAY_HANDLE_TSTR   P_ARRAY_HANDLE
#define PC_ARRAY_HANDLE_TSTR  PC_ARRAY_HANDLE

#define array_tstr(pc_array_handle_tstr) \
    array_basec(pc_array_handle_tstr, TCHARZ)

/*
---------------------------------------------------------------------------------------------------------------------------
AllocBlock stuff now part of aligator too
---------------------------------------------------------------------------------------------------------------------------
*/

typedef struct _ALLOCBLOCK /* really private to implementation, but stop old ARM compiler barfing over debug info */
{
    struct _ALLOCBLOCK * next;
    U32 hwm;
    U32 size;
#if !defined(_WIN64)
    U32 _padding;
#endif
}
ALLOCBLOCK, * P_ALLOCBLOCK, ** P_P_ALLOCBLOCK;

#define ALLOCBLOCK_OVH 16 /* sizeof32(ALLOCBLOCK) */

/* each DOCU has a general_string_alloc_block but there are a few *limited* occasions where this is useful, eg. fontmap, fileutil */
extern P_ALLOCBLOCK global_string_alloc_block;

_Check_return_
extern STATUS
alloc_block_create(
    _OutRef_    P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes);

extern void
alloc_block_dispose(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

_Check_return_
_Ret_writes_bytes_to_maybenull_(n_bytes_requested, 0) /* may be NULL */
extern P_BYTE
alloc_block_malloc(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes_requested,
    _OutRef_    P_STATUS p_status);

_Check_return_
extern STATUS
alloc_block_ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_z_      PC_USTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

_Check_return_
extern STATUS
alloc_block_tstr_set(
    _OutRef_    P_PTSTR aa,
    _In_z_      PCTSTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

#include "cmodules/quickblk.h"

#endif /* __aligator_h */

/* end of aligator.h */
