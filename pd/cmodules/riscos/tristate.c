/* tristate.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Provide support for tristate buttons */

/* SKS 17-Jan-92 */

#include "common/gflags.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"
#endif

/* external header */
#include "tristate.h"

/*
NB. all sprite names must be unique up to the character offset
of the end of the shortest name 'cos of Neil and 13s
*/

static const char *
riscos_tristate_sprite_names[] =
{
    "S" "optunk",
    "S" "opton",
    "S" "optoff"
};

extern RISCOS_TRISTATE_STATE
riscos_tristate_hit(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle)
{
    RISCOS_TRISTATE_STATE state = riscos_tristate_query(window_handle, icon_handle);

    if(++state >= elemof32(riscos_tristate_sprite_names))
        state = 0;

    riscos_tristate_set(window_handle, icon_handle, state);

    return(state);
}

extern RISCOS_TRISTATE_STATE
riscos_tristate_query(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle)
{
    RISCOS_TRISTATE_STATE state;
    WimpGetIconStateBlock icon_state;
    char * tristate_name;
    const char * test_name;

    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return(RISCOS_TRISTATE_DONT_CARE);

    tristate_name = icon_state.icon.data.it.validation;
    assert(tristate_name);

    state = elemof32(riscos_tristate_sprite_names) - 1;

    do  {
        test_name = riscos_tristate_sprite_names[state];

        if(0 == strncmp(test_name, tristate_name, strlen(test_name))) /* first time round Neil has left a 13 terminated string! */
            return(state);
    }
    while(--state >= 0);

    return(RISCOS_TRISTATE_DONT_CARE);
}

extern void
riscos_tristate_set(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    RISCOS_TRISTATE_STATE state)
{
    WimpGetIconStateBlock icon_state;
    char * tristate_name;

    if(state == riscos_tristate_query(window_handle, icon_handle))
        return;

    icon_state.window_handle = window_handle;
    icon_state.icon_handle = icon_handle;
    if(NULL != WrapOsErrorReporting(tbl_wimp_get_icon_state(&icon_state)))
        return;

    tristate_name = icon_state.icon.data.it.validation;
    assert(tristate_name);

    assert(state < elemof32(riscos_tristate_sprite_names));
    strcpy(tristate_name, riscos_tristate_sprite_names[state]);

    winf_changedfield(window_handle, icon_handle); /* just poke it for redraw */
}

/* end of tristate.c */
