/* tristate.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Provide support for tristate buttons */

/* SKS 17-Jan-92 */

#include "common/gflags.h"

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
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
    wimp_w w,
    wimp_i i)
{
    RISCOS_TRISTATE_STATE state;

    state = riscos_tristate_query(w, i);

    if(++state >= elemof32(riscos_tristate_sprite_names))
        state = 0;

    riscos_tristate_set(w, i, state);

    return(state);
}

extern RISCOS_TRISTATE_STATE
riscos_tristate_query(
    wimp_w w,
    wimp_i i)
{
    RISCOS_TRISTATE_STATE state;
    wimp_icon    icon;
    char *       tristate_name;
    const char * test_name;

    wimp_get_icon_info(w, i, &icon);

    tristate_name = icon.data.indirecttext.validstring;
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
    wimp_w w,
    wimp_i i,
    RISCOS_TRISTATE_STATE state)
{
    wimp_icon icon;
    char *    tristate_name;

    if(state == riscos_tristate_query(w, i))
        return;

    wimp_get_icon_info(w, i, &icon);

    tristate_name = icon.data.indirecttext.validstring;
    assert(tristate_name);

    assert(state < elemof32(riscos_tristate_sprite_names));
    strcpy(tristate_name, riscos_tristate_sprite_names[state]);

    wimp_set_icon_state(w, i, (wimp_iconflags) 0, (wimp_iconflags) 0); /* poke it for redraw */
}

/* end of tristate.c */
