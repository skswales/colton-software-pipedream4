/* riscos/filetype.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __filetype_h
#define __filetype_h

/*
RISC OS file types
*/

typedef enum FILETYPE_RISC_OS
{
    FILETYPE_WINDOWS_BMP = 0x69C, /* As Allocated by Acorn PMF */
    FILETYPE_FWP         = 0xAF8,
    FILETYPE_DRAW        = 0xAFF,
    FILETYPE_MS_XLS      = 0xBA6,
    FILETYPE_T5_WORKZ    = 0xBDF,
    FILETYPE_T5_RECORDZ  = 0xBE0,
    FILETYPE_T5_RESULTZ  = 0xBE1,
    FILETYPE_T5_WORDZ    = 0xC1C,
    FILETYPE_T5_TEMPLATE = 0xC1D,
    FILETYPE_T5_COMMAND  = 0xC1E,
    FILETYPE_DATAPOWER   = 0xC27,
    FILETYPE_DATAPOWERGPH= 0xC28,
    FILETYPE_POSTER      = 0xCC3,
    FILETYPE_JPEG        = 0xC85,
    FILETYPE_PDMACRO     = 0xD21,
    FILETYPE_LOTUS123    = 0xDB0, /* .WK1 */
    FILETYPE_PIPEDREAM   = 0xDDE,
    FILETYPE_CSV         = 0xDFE,
    FILETYPE_VIEW        = 0xFE9,
    FILETYPE_SPRITE      = 0xFF9,
    FILETYPE_PRINTOUT    = 0xFF4,
    FILETYPE_PARAGRAPH   = 0xFFF,
    FILETYPE_TEXT        = 0xFFF,

    FILETYPE_DIRECTORY   = 0x1000,
    FILETYPE_APPLICATION = 0x2000,
    FILETYPE_UNTYPED     = 0x3000,

    FILETYPE_UNDETERMINED = -1
}
FILETYPE_RISC_OS, * P_FILETYPE_RISC_OS;

#endif /* __filetype_h */

/* end of riscos/filetype.h */
