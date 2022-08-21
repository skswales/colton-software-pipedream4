/* userIO_x.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* User I/O routines - alert and input */

/* RCM October 1991 */

#ifndef __userIO_x_h
#define __userIO_x_h

/* exported types */

typedef struct USERIO_STRUCT * userIO_handle;

/* exported routines */

extern S32
userIO_alert_user_open(
    userIO_handle *userp,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2);

extern S32
userIO_alert_user_poll(
    userIO_handle *userp);

extern void
userIO_alert_user_close(
    userIO_handle *userp);

extern S32
userIO_input_from_user_open(
    userIO_handle *userp,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2);

extern S32
userIO_input_from_user_poll(
    userIO_handle *userp,
    char *text_out,
    int max_len);

extern void
userIO_input_from_user_close(
    userIO_handle *userp);

#endif /* __userIO_x_h */

/* end of userIO_x.h */
