/* pd123.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* External header file for PipeDream/LOTUS converter */

/* MRJC March 1988 */

#ifndef __pd123_ext
#define __pd123_ext

/* flag for other modules */
#define LOTUSREAD

/******************************************************************************
*
* code selection
*
* UTIL_LTP creates stand-alone utility for Lotus to PipeDream
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus
* INT_LPTPL creates internal code for converter using function lp_lptpl
*
******************************************************************************/

/* #define UTIL_PTL */
/* #define UTIL_LTP */
#define INT_LPTPL

/*
types of PipeDream
*/

#define PD_VP   1
#define PD_Z88  2
#define PD_PC   3
#define PD_ARCH 4
#define PD_3    5
#define PD_4    6

/*
function declarations
*/

extern S32
islotusfile(
    FILE *filein);

extern S32
readlotus(
    FILE *filein,
    FILE *fileout,
    S32 PD_type,
    S32 (* counterproc)(S32));

extern S32
writelotus(
    FILE *filein,
    FILE *fileout,
    S32 PD_type,
    S32 (* counterproc)(S32));

/*
error definition
*/

#define PD123_ERRLIST_DEF \
	errorstring(PD123_ERR_FILE,    "Error reading or writing file") \
	errorstring(PD123_ERR_MEM,     "Out of memory") \
	errorstring(PD123_ERR_EXP,     "Expression not completely converted") \
	errorstring(PD123_ERR_BADFILE, "Bad input file") \
	errorstring(PD123_ERR_BIGFILE, "File has too many rows or columns")

/*
error definition
*/

#define PD123_ERR_BASE    (-4000)

#define PD123_ERR_FILE    (-4000)
#define PD123_ERR_MEM     (-4001)
#define PD123_ERR_EXP     (-4002)
#define PD123_ERR_BADFILE (-4003)
#define PD123_ERR_BIGFILE (-4004)

#define PD123_ERR_END     (-4005)

#endif /* __pd123_ext */

/* end of pd123.ext */
