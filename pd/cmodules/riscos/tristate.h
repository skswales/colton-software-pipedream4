/* tristate.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __tristate_h
#define __tristate_h

#define RISCOS_TRISTATE_DONT_CARE 0
#define RISCOS_TRISTATE_ON 1
#define RISCOS_TRISTATE_OFF 2
#define RISCOS_TRISTATE_STATE S32

/*
exported functions
*/

extern RISCOS_TRISTATE_STATE
riscos_tristate_hit(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle);

extern RISCOS_TRISTATE_STATE
riscos_tristate_query(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle);

extern void
riscos_tristate_set(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    RISCOS_TRISTATE_STATE state);

#endif /* __tristate_h */

/* end of tristate.h */
