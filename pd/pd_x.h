/* pd_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* requires "datafmt.h" */
/*          "riscos_x.h" */

/*
exported functions
*/

/*
from pdmain.c
*/

extern int
main(
    int argc,
    char *argv[]);

_Check_return_
_Ret_z_
extern PC_USTR
product_id(void);

_Check_return_
_Ret_z_
extern PC_USTR
product_ui_id(void);

_Check_return_
_Ret_z_
extern PCTSTR
registration_number(void);

_Check_return_
_Ret_z_
extern PC_USTR
user_id(void);
_Check_return_
_Ret_z_
extern PC_USTR
user_organ_id(void);

/*
from commlin.c
*/

/*ncr*/
extern BOOL
act_on_c(
    S32 c);

extern void
application_process_command(
    S32 c);

_Check_return_
extern BOOL
application_process_key(
    KMAP_CODE kmap_code);

_Check_return_
extern BOOL
get_menu_item(
    const MENU_HEAD * const header,
    MENU *mptr,
    BOOL classic_m,
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
    BOOL select_clicked,
    char *buffer /*out*/);

/* end of pd_x.h */
