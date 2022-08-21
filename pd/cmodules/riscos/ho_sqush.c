/* ho_sqush.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Squash module for use by WimpLib */

/* David De Vorchik 1993 */

#include "common/gflags.h"

#include "cmodules/riscos/ho_sqush.h"

#ifndef __cs_flex_h
#include "cs-flex.h"
#endif

/* we *may* need to call these */

#undef malloc
extern void *malloc(size_t /*size*/);
#undef free
extern void free(void * /*ptr*/);

/* Guard word used to flag squashed resources */

#define _SQUASH_GUARD_WORD 0x48535153

/*
what the file contains
*/

typedef struct _squash_header
{
    U32 guard_word;
    U32 expanded_size;
    U32 load_address;
    U32 exec_address;
    U32 reserved;
}
SQUASH_HEADER, * P_SQUASH_HEADER;

typedef void (* P_FREE_PROC) (P_ANY core);

typedef struct tagSQUASH_ALLOC_BLOCK
{
    P_FREE_PROC p_proc_free;
}
SQUASH_ALLOC_BLOCK, * P_SQUASH_ALLOC_BLOCK;

/* -------------------------------------------------------------------------
 * Given a file handle and the amount to be expanded and the expected size
 * and a block of memory of that size then attempt to read into the
 * flex area the data and expand it.
 *
 * The file pointer will have been advanced.
 * ------------------------------------------------------------------------- */

extern STATUS
host_squash_expand_from_file(
    P_ANY p_any /*filled*/,
    FILE_HANDLE file_handle,
    U32 compress_size,
    U32 expand_size)
{
    STATUS status;
    void * compressed_data  = NULL;
    void * squash_workspace = NULL;
    _kernel_swi_regs r;

    for(;;)
        {
        r.r[0] = 0x8;

        if(_kernel_swi(0x042701 /*Squash_Decompress*/, &r, &r) != NULL)
            {
            status = STATUS_FAIL;
            break;
            }

        if(!flex_alloc(&squash_workspace, r.r[0]))
            {
            status = status_nomem();
            break;
            }

        if(!flex_alloc(&compressed_data, (int) compress_size))
            {
            status = status_nomem();
            break;
            }

        if(compress_size != file_read(compressed_data, 1, compress_size, file_handle))
            {
            status = STATUS_FAIL;
            break;
            }

        r.r[0] = 0;
        r.r[1] = (int) squash_workspace;
        r.r[2] = (int) compressed_data;
        r.r[3] = (int) compress_size;
        r.r[4] = (int) p_any;
        r.r[5] = (int) expand_size;

        if((_kernel_swi(0x042701 /*Squash_Decompress*/, &r, &r) != NULL) || (r.r[3] != 0))
            {
            status = STATUS_FAIL;
            break;
            }

        status = STATUS_OK;

        break;
        /*NOTREACHED*/
        }

    flex_free(&compressed_data);
    flex_free(&squash_workspace);

    return(status);
}

/* -------------------------------------------------------------------------
 * Load an entire file into memory expanding it if required.
 * This code checks to see if the file is bigger than a squash header, if it
 * is then it loads the header and checks to see if the file is squashed (guard word valid).
 *
 * If the file is squashed then the compressed copy is loaded into a flex
 * block and the squash module is granted some workspace (in another flex block).
 *
 * We then allocate enough store for the original data and expand it
 * out into there, tidying and returning a suitable status code.
 *
 * If the file is not compressed then it is just loaded directly into memory.
 * If the call returns a failure then the buffer will have been disposed of.
 * ------------------------------------------------------------------------- */

extern STATUS
host_squash_load_data_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle)
{
    STATUS status;
    SQUASH_HEADER squash_header;
    S32 length = file_length(file_handle);

    if(length > sizeof(SQUASH_HEADER))
        {
        status_return(file_read(&squash_header, sizeof(SQUASH_HEADER), 1, file_handle));

        if(squash_header.guard_word == _SQUASH_GUARD_WORD)
            {
            const U32 expand_size = squash_header.expanded_size;

            if(status_ok(status = host_squash_alloc(p_p_any, expand_size, FALSE)))
                {
                length -= sizeof(SQUASH_HEADER);

                if(status_fail(status = host_squash_expand_from_file(*p_p_any, file_handle, length, expand_size)))
                    host_squash_dispose(p_p_any);
                }

            return(status);
            }

        file_seek(file_handle, 0, SEEK_SET);
        }

    status_return(host_squash_alloc(p_p_any, length, FALSE));

    if((status = file_read(*p_p_any, 1, length, file_handle)) != length)
        {
        host_squash_dispose(p_p_any);
        status = STATUS_FAIL;
        }
    else
        status = STATUS_OK;

    return(status);
}

extern STATUS
host_squash_load_messages_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle)
{
    STATUS status;
    SQUASH_HEADER squash_header;
    S32 length = file_length(file_handle);
    const U32 messages_bodge = 2; /* one for trailing linesep, one for final NUL */
    U32 expand_size = length;

    *p_p_any = NULL;

    for (;;) /* loop for structure */
        {
        if(length > sizeof(SQUASH_HEADER))
            {
            status_break(status = file_read(&squash_header, sizeof(SQUASH_HEADER), 1, file_handle));

            if(squash_header.guard_word == _SQUASH_GUARD_WORD)
                {
                expand_size = squash_header.expanded_size;

                status_break(status = host_squash_alloc(p_p_any, expand_size + messages_bodge, FALSE));

                length -= sizeof(SQUASH_HEADER);

                status = host_squash_expand_from_file(*p_p_any, file_handle, length, expand_size);
                break;
                }

            file_seek(file_handle, 0, SEEK_SET);
            }

        status_break(status = host_squash_alloc(p_p_any, length + messages_bodge, FALSE));

        if((status = file_read(*p_p_any, 1, length, file_handle)) != length)
            {
            status = STATUS_FAIL;
            break;
            }

        status = STATUS_OK;
        break;
        }

    if(status_fail(status))
        {
        host_squash_dispose(p_p_any);

        return(status);
        }

    return(expand_size);
}

extern STATUS
host_squash_load_sprite_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle)
{
    STATUS status;
    SQUASH_HEADER squash_header;
    S32 length = file_length(file_handle);
    const U32 sprite_bodge = 4;

    *p_p_any = NULL;

    for (;;) /* loop for structure */
        {
        P_U32 ptr;

        if(length > sizeof(SQUASH_HEADER))
            {
            status_break(status = file_read(&squash_header, sizeof(SQUASH_HEADER), 1, file_handle));

            if(squash_header.guard_word == _SQUASH_GUARD_WORD)
                {
                U32 expand_size = squash_header.expanded_size;

                status_break(status = host_squash_alloc(p_p_any, expand_size + sprite_bodge, FALSE));

                /* fill in sprite area header */
                ptr = *p_p_any;
                *ptr++ = expand_size + sprite_bodge;

                length -= sizeof(SQUASH_HEADER);

                status = host_squash_expand_from_file(ptr, file_handle, length, expand_size);
                break;
                }

            file_seek(file_handle, 0, SEEK_SET);
            }

        status_break(status = host_squash_alloc(p_p_any, length + sprite_bodge, FALSE));

        /* fill in sprite area header */
        ptr = *p_p_any;
        *ptr++ = length + sprite_bodge;

        /* read body of file - file is sprite area without the length word */
        if((status = file_read(ptr, 1, length, file_handle)) != length)
            {
            status = STATUS_FAIL;
            break;
            }

        status = STATUS_OK;
        break;
        }

    if(status_fail(status))
        host_squash_dispose(p_p_any);

    return(status);
}

extern STATUS
host_squash_alloc(
    P_P_ANY p_p_any /*out*/,
    U32 size,
    BOOL may_be_code)
{
    P_SQUASH_ALLOC_BLOCK p_squash_alloc_block;
    P_FREE_PROC p_proc_free = NULL;

    size += sizeof(*p_squash_alloc_block);

    if(may_be_code && (0 != flex_dynamic_area_query()))
        {
        /* allocate from low heap as we can't execute code up there in the dynamic areas */
        if(NULL != (p_squash_alloc_block = malloc((size_t) size)))
            p_proc_free = free;
        }
    else
        {
        if(NULL != (p_squash_alloc_block = list_allocptr(size)))
            p_proc_free = list_deallocptr;
        }

    if(NULL != p_squash_alloc_block)
        {
        p_squash_alloc_block->p_proc_free = p_proc_free;
        p_squash_alloc_block += 1;
        *p_p_any = p_squash_alloc_block;
        return(STATUS_OK);
        }

    return(status_nomem());
}

extern void
host_squash_dispose(
    P_P_ANY p_p_any /*inout*/)
{
    P_SQUASH_ALLOC_BLOCK p_squash_alloc_block = *p_p_any;

    *p_p_any = NULL;

    if(NULL != p_squash_alloc_block)
        {
        p_squash_alloc_block -= 1;
        (* p_squash_alloc_block->p_proc_free) (p_squash_alloc_block);
        }
}

/* end of ho_sqush.c */
