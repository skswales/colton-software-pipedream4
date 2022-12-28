/* cs-bbcx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

#include "os.h"

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

/* wee shim saves a tiny bit in callers */

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
cs_kernel_swi(
    _InVal_     int swi_code,
    _Inout_     _kernel_swi_regs * const in_and_out)
{
    return(_kernel_swi(swi_code, in_and_out, in_and_out));
}

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
    buffer[0] = 24;
    buffer[1] = u8_from_int(x1);
    buffer[2] = u8_from_int(x1 >> 8);
    buffer[3] = u8_from_int(y1);
    buffer[4] = u8_from_int(y1 >> 8);
    buffer[5] = u8_from_int(x2);
    buffer[6] = u8_from_int(x2 >> 8);
    buffer[7] = u8_from_int(y2);
    buffer[8] = u8_from_int(y2 >> 8);
    return(os_writeN(buffer, sizeof32(buffer)));
}

#undef BOOL
#undef TRUE
#undef FALSE

#include "bbc.c"

/* end of cs-bbcx.c */
