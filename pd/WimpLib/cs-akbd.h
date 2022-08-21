/* cs_akbd.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_akbd_h
#define __cs_akbd_h

#ifndef __akbd_h
#include "akbd.h"
#endif

#define cs_akbd_PrintK  (akbd_Fn +  0) /* 0x180,190,1A0,1B0 */ /* F0 */
#define cs_akbd_TabK    (akbd_Fn + 10) /* 0x18A,19A,1AA,1BA */
#define cs_akbd_CopyK   (akbd_Fn + 11) /* 0x18B,19B,1AB,1BB */
#define cs_akbd_LeftK   (akbd_Fn + 12) /* 0x18C,19C,1AC,1BC */
#define cs_akbd_RightK  (akbd_Fn + 13) /* 0x18D,19E,1AD,1BE */
#define cs_akbd_DownK   (akbd_Fn + 14) /* 0x18E,19E,1AE,1BE */
#define cs_akbd_UpK     (akbd_Fn + 15) /* 0x18F,19F,1AF,1BF */
#define cs_akbd_Fn10    (0x1CA)        /*     0x1DA,1EA,1FA */
#define cs_akbd_Fn11    (0x1CB)        /*     0x1DB,1EB,1FB */
#define cs_akbd_Fn12    (0x1CC)        /*     0x1DC,1EC,1FC */
#define cs_akbd_InsertK (0x1CD)        /*     0x1DD,1ED,1FD */ /* F13 */
/* SKS says reserve 0x1CE and 1CF for F14 and F15 */

#define cs_akbd_BackspaceK (0x08)
#define cs_akbd_HomeK      (30)
#define cs_akbd_DeleteK    (0x7F)

/* SKS proposes a new range at 0x1000, with: */
#if 0
#define cs_akbd_HomeKFn      0x1000 /* 0x1010,01020,1030 */
#define cs_akbd_DeleteKFn    0x1001 /* 0x1011,01021,1031 */
#define cs_akbd_BackSpaceKFn 0x1002 /* 0x1012,01022,1032 */
#define cs_akbd_ReturnKFn    0x1003 /* 0x1013,01023,1033 */
#endif

#endif /* __cs_akbd_h */

/* end of cs_akbd.h */
