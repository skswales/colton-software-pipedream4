/* riscdraw_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __riscdraw_x_h
#define __riscdraw_x_h

#if RISCOS

/*
exported functions from riscdraw.c
*/

extern void
at(
    int x,
    int y);

extern void
clear_graphicsarea(
    int x0,
    int y0,
    int x1,
    int y1);

extern void
clear_thisgraphicsarea(void);

extern void
clear_textarea(
    int x0,
    int y0,
    int x1,
    int y1);

extern void
clear_thistextarea(void);

_Check_return_
extern gcoord
gcoord_x(
    coord x);

_Check_return_
extern gcoord
gcoord_y(
    coord y);

_Check_return_
extern int
gcoord_y_textout(
    int y);

_Check_return_
extern int
tcoord_x(
    int x);

_Check_return_
extern int
tcoord_y(
    int y);

_Check_return_
extern int
tcoord_x1(
    int x);

_Check_return_
extern int
tcoord_y1(
    int y);

/*
exported variables from riscdraw.c
*/

extern BOOLEAN  paint_is_update;
/*extern int      textcell_xorg;*/
/*extern int      textcell_yorg;*/
extern int      xorg;
extern int      yorg;

#endif

#endif

/* end of riscdraw_x.h */

