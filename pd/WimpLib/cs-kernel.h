/* cs-kernel.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2022 Stuart Swales */

#ifndef __cs_kernel_h
#define __cs_kernel_h

#include "kernel.h" /*C:*/

#if defined(EXPOSE_RISCOS_SWIS) || defined(NORCROFT_INLINE_SWIX)
#include "swis.h" /*C:*/
#endif

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
cs_kernel_swi(
    _InVal_     int swi_code,
    _Inout_     _kernel_swi_regs * const in_and_out);

#endif /* __cs_kernel_h */

/* end of cs_kernel.h */
