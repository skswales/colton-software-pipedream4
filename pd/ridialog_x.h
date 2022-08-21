/* ridialog_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* requires "riscos_x.h" */

#ifndef __ridialog_x_h
#define __ridialog_x_h

#if RISCOS

/*
exported functions from riscdialog.c
*/

extern void
dialog_end(void);

extern int
dialog_execute(
    dialog_proc proc,
    const char *dname,
    DIALOG *dptr);

extern int
dialog_go(void);

extern void
dialog_setentry(
    int i);

extern void
dialog_sethelp(
    const char *str);

extern int
dialog_start(
    const char *name,
    int width,
    int items,
    int boxnumber);

extern void
dproc_aboutfile(
    DIALOG *dptr);

extern void
dproc_aboutprog(
    DIALOG *dptr);

extern void
dproc_execfile(
    DIALOG *dptr);

extern void
dproc_loadfile(
    DIALOG *dptr);

extern void
dproc_namefile(
    DIALOG *dptr);

/*
exported variables
*/

extern HOST_WND dialog_window_handle;
extern BOOLEAN dialog_window_has_caret;

#endif

#endif

/* end of ridialog_x.h */
