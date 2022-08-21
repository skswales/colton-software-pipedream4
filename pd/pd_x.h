/* pd_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* requires "datafmt.h" */
/*          "riscos_x.h" */

/*
exported functions
*/

/*
from pdmain.c
*/

extern BOOL
act_on_c(
    S32 c);

extern void
application_process_command(
    S32 c);

extern BOOL
application_process_key(
    S32 c);

extern int
main(
    int argc,
    char *argv[]);

extern PC_USTR
product_id(void);

extern PC_USTR
product_ui_id(void);

extern PC_USTR
user_id(void);

extern PC_USTR
user_organ_id(void);

/*
from mcdiff.c
*/

extern gcoord
gcoord_x(
    coord x);

extern gcoord
gcoord_y(
    coord y);

extern void
wrch_definefunny(
    S32 ch);

extern void
wrch_undefinefunnies(void);

/*
from commlin.c
*/

extern BOOL
get_menu_item(
    MENU_HEAD *header,
    MENU *mptr,
    char *array /*filled*/);

/*
from somewhere
*/

extern void
maybe_draw_screen(void);

extern void
application_drag(
    gcoord x,
    gcoord y,
    BOOL ended);

extern void
get_slr_for_point(
    gcoord x,
    gcoord y,
    BOOL selectclicked,
    char *buffer /*out*/);

/* end of pd_x.h */
