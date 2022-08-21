/* vsload.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External definitions for ViewSheet loader */

/* MRJC May 1989 */

#ifndef __vsload_h
#define __vsload_h

/*
functions
*/

_Check_return_
extern BOOL
vsload_fileheader_isvsfile(
    _In_reads_(size) PC_U8 ptr,
    _InVal_     U32 size);

_Check_return_
extern STATUS
vsload_isvsfile(
    FILE_HANDLE fin);

_Check_return_
extern STATUS
vsload_loadvsfile(
    FILE_HANDLE fin);

extern void
vsload_fileend(void);

extern char *
vsload_travel(
    S32 col,
    S32 row,
    P_S32 type,
    P_S32 decp,
    P_S32 justright,
    P_S32 minus);

/*
types of cell
*/

#define VS_TEXT   0
#define VS_NUMBER 1

/*
error definition
*/

#define VSLOAD_ERRLIST_DEF \
    errorstring(VSLOAD_ERR_CANTREAD, "ViewSheet: Can't read file")

/*
error definition
*/

#define VSLOAD_ERR_BASE      (-5000)

#define VSLOAD_ERR_CANTREAD  (-5000)

#define VSLOAD_ERR_END       (-5002)

#endif /* __vsload_h */

/* end of vsload.h */
