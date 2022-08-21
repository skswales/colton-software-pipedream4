/* riscos/osfile.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Header for RISC OS filing system calls */

/* Stuart K. Swales 24-May-1991 */

#ifndef __osfile_h
#define __osfile_h

/*
OS_File reason codes
*/

typedef enum
    {
    OSFile_Load            = 0xFF,    /* Uses File$Path */
    OSFile_Save            = 0,
    OSFile_WriteInfo       = 1,
    OSFile_WriteLoad       = 2,
    OSFile_WriteExec       = 3,
    OSFile_WriteAttr       = 4,
    OSFile_ReadInfo        = 5,       /* Uses File$Path */
    OSFile_Delete          = 6,
    OSFile_Create          = 7,
    OSFile_CreateDir       = 8,
    OSFile_SetStamp        = 9,
    OSFile_SaveStamp       = 10,
    OSFile_CreateStamp     = 11,
    OSFile_LoadPath        = 12,      /* Uses given path */
    OSFile_ReadPath        = 13,      /* Uses given path */
    OSFile_LoadPathVar     = 14,      /* Uses given path variable */
    OSFile_ReadPathVar     = 15,      /* Uses given path variable */
    OSFile_LoadNoPath      = 16,      /* No nonsense load */
    OSFile_ReadNoPath      = 17,      /* No nonsense read */
    OSFile_SetType         = 18,
    OSFile_MakeError       = 19
    }
OSFile_ReasonCode;

/*
object types
*/
typedef enum
    {
    OSFile_ObjectType_None = 0,
    OSFile_ObjectType_File = 1,
    OSFile_ObjectType_Dir  = 2
    }
OSFile_ObjectType;

/*
object attributes
*/

typedef enum
    {
    OSFile_ObjectAttribute_read         = (1 << 0),
    OSFile_ObjectAttribute_write        = (1 << 1),
    OSFile_ObjectAttribute_locked       = (1 << 3),
    OSFile_ObjectAttribute_public_read  = (1 << 4),
    OSFile_ObjectAttribute_public_write = (1 << 5)
    }
OSFile_ObjectAttribute;

/*
OS_GBPB reason codes
*/

typedef enum
    {
    OSGBPB_WriteAtGiven          = 1,
    OSGBPB_WriteAtPTR            = 2,
    OSGBPB_ReadFromGiven         = 3,
    OSGBPB_ReadFromPTR           = 4,

    OSGBPB_ReadDiscName          = 5,
    OSGBPB_ReadCSDName           = 6,
    OSGBPB_ReadLIBName           = 7,
    OSGBPB_ReadCSDEntries        = 8,

    OSGBPB_ReadDirEntries        = 9,
    OSGBPB_ReadDirEntriesInfo    = 10,
    OSGBPB_ReadDirEntriesCatInfo = 11
    }
OSGBPB_ReasonCode;

/*
OS_Args reason codes
*/

typedef enum
    {
    OSArgs_ReadPTR    = 0,
    OSArgs_SetPTR     = 1,
    OSArgs_ReadEXT    = 2,
    OSArgs_SetEXT     = 3,
    OSArgs_ReadSize   = 4,
    OSArgs_EOFCheck   = 5,
    OSArgs_EnsureSize = 6,

    OSArgs_ReadInfo   = 0xFE,
    OSArgs_Flush      = 0xFF
    }
OSArgs_ReasonCode;

/*
OS_Find reason codes
*/

typedef enum
    {
    OSFind_CloseFile    = 0x00,
    OSFind_OpenRead     = 0x40,
    OSFind_CreateUpdate = 0x80,
    OSFind_OpenUpdate   = 0xC0
    }
OSFind_ReasonCode;

typedef enum
    {
    OSFind_UseFilePath  = 0x00, /* these four are mutually exclusive */
    OSFind_UsePath      = 0x01,
    OSFind_UsePathVar   = 0x02,
    OSFind_UseNoPath    = 0x03,

    OSFind_EnsureNoDir  = 0x04,

    OSFind_EnsureOpen   = 0x08  /* gives error instead of handle = 0 return */
    }
OSFind_ExtraRCBits;

#endif /* __osfile_h */

/* end of riscos/osfile.h */
