/* cs-bbcx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

#include "os.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
riscos_vdu_define_graphics_window(
    _In_        GDI_COORD x1,
    _In_        GDI_COORD y1,
    _In_        GDI_COORD x2,
    _In_        GDI_COORD y2)
{
    /* SKS 19apr95 (Fireworkz), 09sep16 (PipeDream) get round VDU funnel/multiple SWI overhead */
    U8 buffer[9 /*length of VDU 24 sequence*/];
    P_U8 p_u8 = buffer;
    *p_u8++ = 24;
    *p_u8++ = u8_from_int(x1);
    *p_u8++ = u8_from_int(x1 >> 8);
    *p_u8++ = u8_from_int(y1);
    *p_u8++ = u8_from_int(y1 >> 8);
    *p_u8++ = u8_from_int(x2);
    *p_u8++ = u8_from_int(x2 >> 8);
    *p_u8++ = u8_from_int(y2);
    *p_u8++ = u8_from_int(y2 >> 8);
    return(os_writeN(buffer, sizeof32(buffer)));
}

#include "bbc.c"

/* end of cs-bbcx.c */
