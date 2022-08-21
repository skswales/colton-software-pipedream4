/* cs-wimp.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_wimp_h
#define __cs_wimp_h

/* The main purpose of this file is to try to force the
 * inclusion of ACW's patched wimp.h rather than tboxlibs' wimp.h
 * but we also define PipeDream specific messages here
 */

typedef enum WIMP_MSGPD_DDE_ID {            /* ddetype */

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
} WIMP_MSGPD_DDE_ID;

typedef char WIMP_MSGPD_DDETYPEA_TEXT[236 -4 -12];

typedef struct WIMP_MSGPD_DDETYPEA {
    int                         handle;
    int                         x_size;
    int                         y_size;
    WIMP_MSGPD_DDETYPEA_TEXT    text;       /* leafname & tag, both 0-terminated */
} WIMP_MSGPD_DDETYPEA;

typedef struct WIMP_MSGPD_DDETYPEB {
    int     handle;
} WIMP_MSGPD_DDETYPEB;

typedef enum WIMP_MSGPD_DDETYPEC_TYPE {
    Wimp_MPD_DDE_typeC_type_Text,
    Wimp_MPD_DDE_typeC_type_Number,
    Wimp_MPD_DDE_typeC_type_End
} WIMP_MSGPD_DDETYPEC_TYPE;

typedef struct WIMP_MSGPD_DDETYPEC {
    int                         handle;
    int                         x_off;
    int                         y_off;
    WIMP_MSGPD_DDETYPEC_TYPE    type;
    union WIMP_MSGPD_DDETYPEC_CONTENT {
        char                        text[236 -4 -16]; /* textual content 0-terminated */
        double                      number;
    } content;
} WIMP_MSGPD_DDETYPEC;

typedef struct WIMP_MSGPD_DDETYPED {
    char    leafname[236 -4];                   /* leafname of DrawFile, 0-terminated */
} WIMP_MSGPD_DDETYPED;

typedef struct WIMP_MSGPD_DDE {                 /* structure used in all PD DDE messages */
    WIMP_MSGPD_DDE_ID id;

    union WIMP_MSGPD_DDE_TYPE {
        WIMP_MSGPD_DDETYPEA a;
        WIMP_MSGPD_DDETYPEB b;
        WIMP_MSGPD_DDETYPEC c;
        WIMP_MSGPD_DDETYPED d;
    /*  WIMP_MSGPD_DDETYPEE e;  (no body) */
    } type;
} wimp_msgpd_dde;

#ifndef __wimp_h
#include "./wimp.h"
#endif

#if defined(Wimp_MQuit)
#error That is the wrong wimp.h - it is from tboxlibs not ACW. Methinks you have not done the rlib_patch/apply.
#endif

#endif /* __cs_wimp_h */

/* end of cs-wimp.h */
