/* riscos/ho_dll.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Dynamic Linking for Fireworkz - load a module and relocate it */

/* David De Vorchik 21-Jun-93 */

#include "common/gflags.h"

#if RISCOS

#include "cmodules/riscos/ho_dll.h"

#include "cmodules/riscos/ho_sqush.h"

#include "datafmt.h"

#ifndef __cs_flex_h
#include "cs-flex.h"
#endif

/*
internal structure
*/

/* Guard word that prefixs the module to ensure that we are reading
 * something semi-sensible.
 */

#define _MODULE_GUARD_WORD 0x50465350

/* The relocation section of the file contains lots of these
 * entries, these are used for fixing up the runtime
 * address and sorting out all external references.
 */

typedef struct _ob_module_reloc
{
    S32 offset;                                 /* to perform relocation at within block */
    S32 value;                                  /* either PC relative or absolute */

    UBF relocation_type : 4;                    /* type of relocation required */
    UBF external_reference : 1;                 /* reference to an externally declared function */
    UBF PC_relative : 1;                        /* PC relative value (ie. instruction to be relocated) */
    UBF internal_reloc : 1;                     /* add area base to value for non PC relative relocations */

    UBF absolute_value : 1;                     /* value is a constant and should not be relocated on loading */
}
OB_MODULE_RELOC, * P_OB_MODULE_RELOC;

enum _reloc_types
{
    RELOC_TYPE_BYTE,                            /* byte value to be modified */
    RELOC_TYPE_WORD,                            /* word value (16 bits) to be modified */
    RELOC_TYPE_DOUBLE                           /* double word (32 bits) to be modified */
};

/* Define the chunk information, its type, compressed size
 * and expanded size.  The squashed bit allows for compressed
 * chunks within the file
 */

typedef struct _ob_chunk_header
{
    S32 type;                                   /* is it code, data, relocation */
    S32 size;                                   /* size of area within file */
    S32 expanded_size;                          /* size when expanded (if compressed) */

    UBF squashed_area : 1;                      /* area is squashed using the 'Squash' module */
}
OB_CHUNK_HEADER, * P_OB_CHUNK_HEADER;

enum _area_types
{
    _AREA_CODE,
    _AREA_RELOC
};

/* Module header, this prefixes the module and defines
 * the version information required, along with a
 * suitable guard word
 */

typedef struct _ob_module_header
{
    U32 guard_word;
    S32 skeleton_version;
    S32 module_version;
    S32 module_id;
    U32 entry_offset;
}
OB_MODULE_HEADER, * P_OB_MODULE_HEADER;

/* The stubs file contains n entries as defined below.
 * Each entry has a value field, but is only valid if the undefined bit is zero.
 */

typedef struct _ob_stub
{
    S32 value;
    UBF absolute  : 1;                          /* =0 -> address, else constant value */
    UBF undefined : 1;
}
OB_STUB, * P_OB_STUB;

/*
Load in the stubs table filling in the structure passed to contain a
suitable array handle.  This needs to be done before any stubs are loaded.
*/

extern STATUS
host_load_stubs(
    _In_z_      PCTSTR filename,
    _OutRef_    P_STUB_DESC p_stub_desc)
{
    FILE_HANDLE stub_file_handle;
    STATUS status;

    zero_struct_ptr(p_stub_desc);

    if(status_done(status = file_open(filename, file_open_read, &stub_file_handle)))
    {
        status = host_squash_load_data_file(&p_stub_desc->p_stub_data, stub_file_handle);

        file_close(&stub_file_handle);
    }

    return(status);
}

/* -------------------------------------------------------------------------
 * Attempt to load a code module into memory and relocate it.  The code
 * handles decoding the headers, decompessing the areas of code and
 * general fix up of the environment around it.
 *
 * If the caller passes NULL for a ob_stub table then the table is assumed
 * to be the global one and that will be references as required.
 * ------------------------------------------------------------------------- */

extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     S32 host_version,
    P_ANY p_stub_table_in)
{
    P_OB_STUB p_stub_table = p_stub_table_in;
    FILE_HANDLE module_file;
    OB_MODULE_HEADER module_header;
    OB_CHUNK_HEADER chunk_header;
    STATUS status = STATUS_OK;

    if(NULL == p_stub_table)
        return(create_error(-1/*ERR_NO_STUBS*/));

    zero_struct_ptr(p_runtime_info);

    status_return(file_open(filename, file_open_read, &module_file));

    if(file_read(&module_header, 1, sizeof32(OB_MODULE_HEADER), module_file) != sizeof32(OB_MODULE_HEADER))
        status = -1/*STATUS_READ_FAILURE*/; /* reading the header failed */
    else if(module_header.guard_word != _MODULE_GUARD_WORD)
        status = -1/*STATUS_NOT_A_MODULE*/; /* guard word does not contain a sensible value */
    else if(host_version < module_header.skeleton_version)
        status = -1/*STATUS_BAD_VERSION*/; /* skeleton is too old for this module */
    else
        while(status == STATUS_OK)
        {
            if(file_read(&chunk_header, 1, sizeof32(OB_CHUNK_HEADER), module_file) == sizeof32(OB_CHUNK_HEADER))
            {
                switch(chunk_header.type)
                {
                case _AREA_CODE:
                    {
                    p_runtime_info->code_size = chunk_header.expanded_size;

                    if(chunk_header.squashed_area)
                    {
                        status_break(status = host_squash_alloc(&p_runtime_info->p_code, chunk_header.expanded_size, TRUE));

                        status_break(status = host_squash_expand_from_file(p_runtime_info->p_code, module_file, chunk_header.size, chunk_header.expanded_size));
                    }
                    else
                    {
                        status_break(status = host_squash_alloc(&p_runtime_info->p_code, chunk_header.size, TRUE));

                        /* SKS after 1.20/50 - try to ensure loaded code will be able to initialise properly */
                        /*if(status_fail(status = ensure_memory_froth()))
                            host_squash_dispose(&p_runtime_info->p_code);
                        else*/ if((status = file_read(p_runtime_info->p_code, 1, chunk_header.size, module_file)) != chunk_header.size)
                            status = -1/*STATUS_READ_FAILURE*/;
                        else
                            status = STATUS_OK;
                    }

                    break;
                    }

                case _AREA_RELOC:
                    {
                    p_runtime_info->reloc_size = chunk_header.expanded_size;

                    if(flex_alloc(&p_runtime_info->reloc_flex, (int) chunk_header.size))
                      if((status = file_read(p_runtime_info->reloc_flex, 1, chunk_header.size, module_file)) == chunk_header.size)
                      {
                          status = STATUS_OK;

                          if(chunk_header.squashed_area)
                          {
                              void * squash_workspace = NULL;
                              void * expansion_space  = NULL;
                              _kernel_swi_regs rs;

                              for(;;)
                              {
                                  rs.r[0] = 0x8;

                                  if(_kernel_swi(/*Squash_Decompress*/ 0x042701, &rs, &rs) != NULL)
                                  {
                                      status = STATUS_FAIL;
                                      break;
                                  }

                                  if(!flex_alloc(&squash_workspace, rs.r[0]))
                                  {
                                      status = status_nomem();
                                      break;
                                  }

                                  if(!flex_alloc(&expansion_space, (int) chunk_header.expanded_size))
                                  {
                                      status = status_nomem();
                                      break;
                                  }

                                  rs.r[0] = 0;
                                  rs.r[1] = (int) squash_workspace;
                                  rs.r[2] = (int) p_runtime_info->reloc_flex;
                                  rs.r[3] = (int) chunk_header.size;
                                  rs.r[4] = (int) expansion_space;
                                  rs.r[5] = (int) chunk_header.expanded_size;

                                  if((_kernel_swi(0x042701 /*Squash_Decompress*/, &rs, &rs) == NULL) && (rs.r[3] == 0x0))
                                  {
                                      flex_free(&p_runtime_info->reloc_flex);
                                      flex_give_away(&p_runtime_info->reloc_flex, &expansion_space);
                                      break;
                                  }

                                  status_break(status = STATUS_FAIL);
                              }

                              flex_free(&squash_workspace);
                              flex_free(&expansion_space);
                          }
                      }
                      else
                          status = -1/*STATUS_READ_FAILURE*/;
                    else
                        status = status_nomem();

                    break;
                    }

                default:
                    status = -1/*STATUS_BAD_CHUNK*/;
                    break;
                }
            }
            else
            {
                if(file_eof(module_file))
                    break; /* no error 'cos end of file when trying to read in new header */

                status = -1/*STATUS_READ_FAILURE*/; /* failed to read in chunk header (no eof, just didn't read it all!) */
            }
        }

    if((status == STATUS_OK) && (p_runtime_info->p_code != NULL) && (p_runtime_info->reloc_size != 0))
    {
        P_U8 p_code = p_runtime_info->p_code;
        P_OB_MODULE_RELOC p_reloc = (P_OB_MODULE_RELOC) p_runtime_info->reloc_flex;
        S32 relocations = p_runtime_info->reloc_size / sizeof32(OB_MODULE_RELOC);

        while((relocations-- > 0) && (status == STATUS_OK))
        {
            P_U8 address = p_code + p_reloc->offset;
            S32 value = p_reloc->value;

            if(p_reloc->external_reference)
            {
                P_OB_STUB p_stub_reference = p_stub_table +value;
                value = p_stub_reference->value;
            }
            else if(!p_reloc->absolute_value)
                value += (S32) p_code;

            if(p_reloc->PC_relative)
            {
                P_U32 p_instruction = (P_U32) address;
                U32 instruction = *p_instruction;

                switch(instruction & 0x0F000000)
                {
                case 0x0A000000: /* case for B and BL instruction */
                case 0x0B000000:
                  value -= (U32) (p_instruction +2);
                  value  = (instruction & 0xFF000000) | ((value >>2) & 0x00FFFFFF);
                  *p_instruction = value;
                  break;

                default:
                  status = -1/*STATUS_BAD_INSTRUCT*/;
                  break;
                }
            }
            else if(p_reloc->external_reference)
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + value);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + value);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + value);
                    break;
                    }
                }
            }
            else if(p_reloc->internal_reloc == 0)
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + (U32) p_code);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + (U32) p_code);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + (U32) p_code);
                    break;
                    }
                }
            }
            else
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + value);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + value);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + value);
                    break;
                    }
                }
            }

              p_reloc++;
          }

        /*if(wimpt_platform_features_query() & PLATFEAT_SYNCH_CODE_AREAS)*/
        {
            /* flush processor cache on this area */
            _kernel_swi_regs rs;
            rs.r[0] = 1; /* Address range to be synchronised */
            rs.r[1] = (int) p_runtime_info->p_code;
            rs.r[2] = (int) p_runtime_info->p_code + (int) p_runtime_info->code_size;
            rs.r[2] = ((rs.r[2] + 3) & ~3) - 4; /* inclusive address required, so round up then lose a word */
            (void) _kernel_swi(/*OS_SynchroniseCodeAreas*/ 0x6E, &rs, &rs);
        }

        if(module_header.entry_offset != 0)
        {
            union _keep_stuart_happy
            {
                PC_BYTE pc_byte;
                void (__cdecl * p_proc) (void);
            } stuart_happy;

            stuart_happy.pc_byte = PtrAddBytes(PC_BYTE, p_runtime_info->p_code, module_header.entry_offset);
            p_runtime_info->p_module_entry = stuart_happy.p_proc;
        }
    }
    else if(status == STATUS_OK)
        status = -1/*STATUS_NOT_ALL_LOADED*/;

    file_close(&module_file);

    flex_free(&p_runtime_info->reloc_flex);

    if(status != STATUS_OK)
        host_discard_module(p_runtime_info);

    return(status);
}

/* -------------------------------------------------------------------------
 * Discard a loaded module.  This requires the runtime description
 * block to be passed.
 * ------------------------------------------------------------------------- */

extern void
host_discard_module(
    P_RUNTIME_INFO p_runtime_info)
{
    host_squash_dispose(&p_runtime_info->p_code);
}

#endif /* RISCOS */

/* end of riscos/ho_dll.c */
