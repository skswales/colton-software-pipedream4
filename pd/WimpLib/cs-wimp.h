/* cs-wimp.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_wimp_h
#define __cs_wimp_h

/* The main purpose of this file is to try to force the
 * inclusion of ACW's patched wimp.h rather than tboxlibs' wimp.h
 * but we also define PipeDream specific messages here
 */

#define Wimp_MPD_DDE ((wimp_msgaction) 0x0600) /* PipeDream dynamic data exchange */

typedef enum {                              /* ddetype */

/* PD tramsmits */
    Wimp_MPD_DDE_SendSlotContents = 1,      /* C, no reply expected */
    Wimp_MPD_DDE_SheetClosed,               /* B, no reply expected  */
    Wimp_MPD_DDE_ReturnHandleAndBlock,      /* A, no reply expected  */

/* PD receives */
    Wimp_MPD_DDE_IdentifyMarkedBlock,       /* E; replies with ReturnHandleAndBlock */
    Wimp_MPD_DDE_EstablishHandle,           /* A; replies with ReturnHandleAndBlock */
    Wimp_MPD_DDE_RequestUpdates,            /* B; no immediate reply, updates do SendSlotContents */
    Wimp_MPD_DDE_RequestContents,           /* B; replies with 0 or more SendSlotContents */
    Wimp_MPD_DDE_GraphClosed,               /* B; no reply */
    Wimp_MPD_DDE_DrawFileChanged,           /* D; no reply */
    Wimp_MPD_DDE_StopRequestUpdates         /* B; no immediate reply, updates don't SendSlotContents */
} wimp_msgpd_dde_id;

typedef char wimp_msgpd_ddetypeA_text[236 -4 -12];

typedef struct _wimp_msgpd_ddetypeA {
    int                         handle;
    int                         xsize;
    int                         ysize;
    wimp_msgpd_ddetypeA_text    text;       /* leafname & tag, both 0-terminated */
} wimp_msgpd_ddetypeA;

typedef struct _wimp_msgpd_ddetypeB {
    int     handle;
} wimp_msgpd_ddetypeB;

typedef enum {
    Wimp_MPD_DDE_typeC_type_Text,
    Wimp_MPD_DDE_typeC_type_Number,
    Wimp_MPD_DDE_typeC_type_End
} wimp_msgpd_ddetypeC_type;

typedef struct _wimp_msgpd_ddetypeC {
    int                         handle;
    int                         xoff;
    int                         yoff;
    wimp_msgpd_ddetypeC_type    type;
    union _wimp_msgpd_ddetypeC_content {
        char                        text[236 -4 -16]; /* textual content 0-terminated */
        double                      number;
    } content;
} wimp_msgpd_ddetypeC;

typedef struct _wimp_msgpd_ddetypeD {
    char    leafname[236 -4];                   /* leafname of DrawFile, 0-terminated */
} wimp_msgpd_ddetypeD;

typedef struct _wimp_msgpd_dde {                /* structure used in all PD DDE messages */
    wimp_msgpd_dde_id id;

    union _wimp_msgpd_dde_type {
        wimp_msgpd_ddetypeA a;
        wimp_msgpd_ddetypeB b;
        wimp_msgpd_ddetypeC c;
        wimp_msgpd_ddetypeD d;
    /*  wimp_msgpd_ddetypeE e;  (no body) */
    } type;
} wimp_msgpd_dde;

#ifndef __wimp_h
#include "./wimp.h"
#endif

#if defined(Wimp_MQuit)
#error That's the wrong wimp.h - it's from tboxlibs not ACW. Methinks you haven't done the rlib_patch/apply.
#endif

#endif /* __cs_wimp_h */

/* end of cs-wimp.h */
