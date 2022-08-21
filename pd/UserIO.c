/* userIO.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* User I/O routines - alert and input */

/* RCM October 1991 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_template_h
#include "cs-template.h" /* includes template.h */
#endif

#include "userIO_x.h"

/* internal routines */

static S32
io_create(
    userIO_handle *userp,
    const char *ident,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2);

static void
io_destroy(
    userIO_handle *userp);

static S32
io_open(
    userIO_handle *userp);

static S32
io_poll(
    userIO_handle *userp,
    char *text_out,
    int max_len);

static void
io_close(
    userIO_handle *userp);

static BOOL
io_event_handler(
    wimp_eventstr *e,
    void *handle);

typedef struct USERIO_STRUCT
{
    HOST_WND window_handle;
    WimpWindowWithBitset * window_template;
    S32 button;
}
USERIO_STRUCT;

#define userIO_ButtonL 0
#define userIO_ButtonM 3
#define userIO_ButtonR 2
#define userIO_Message 1
#define userIO_UserText 4

/******************************************************************************
*
* Alert box
*
* open, poll and close
*
******************************************************************************/

extern S32
userIO_alert_user_open(
    userIO_handle *userp,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2)
{
    S32   err;

    if((err = io_create(userp, "alert", title, message, button1, button2)) >= 0)
    {
        if((err = io_open(userp)) >= 0)
            return(0);

        io_destroy(userp);
    }

    return(err);
}

extern S32
userIO_alert_user_poll(
    userIO_handle *userp)
{
    return(io_poll(userp, NULL, 0));
}

extern void
userIO_alert_user_close(
    userIO_handle *userp)
{
    io_close(userp);
    io_destroy(userp);
}

/******************************************************************************
*
* Input box
*
* open, poll and close
*
******************************************************************************/

extern S32
userIO_input_from_user_open(
    userIO_handle *userp,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2)
{
    S32   err;

    if((err = io_create(userp, "input", title, message, button1, button2)) >= 0)
    {
        if((err = io_open(userp)) >= 0)
        {
            /*>>>????clear text from input field???*/
            return(0);
        }

        io_destroy(userp);
    }

    return(err);
}

extern S32
userIO_input_from_user_poll (userIO_handle *userp, char *text_out, int max_len)
{
    return(io_poll(userp, text_out, max_len));
}

extern void
userIO_input_from_user_close(
    userIO_handle *userp)
{
    io_close(userp);
    io_destroy(userp);
}

static S32
io_create(
    userIO_handle *userp,
    const char *ident,
    const char *title,
    const char *message,
    const char *button1,
    const char *button2)
{
    userIO_handle  user;
    template      *template_handle;
    HOST_WND       window_handle;
    S32            err = STATUS_NOMEM;     /* a useful default value */

    *userp = user = al_ptr_alloc_elem(USERIO_STRUCT, 1, &err);

    if(user)
    {
        user->window_handle = HOST_WND_NONE;
        user->window_template = NULL;
        user->button = 0; /* closed */

        template_handle = template_find_new(ident);

        if(NULL != template_handle)
        {
            user->window_template = template_copy_new(template_handle);

            if(NULL != user->window_template)
            {
                if(NULL == WrapOsErrorReporting(winx_create_window(user->window_template, &window_handle, io_event_handler, (void *) user)))
                {
                    user->window_handle = window_handle;

                    win_settitle(window_handle, de_const_cast(char *, title));

                    winf_setfield(window_handle, userIO_Message, message);
                    winf_setfield(window_handle, userIO_UserText, "");

                    if(button2)
                    {
                        /* put text in left and right buttons, make middle button invisible */

                        winf_setfield(window_handle, userIO_ButtonL, button1);
                        winf_setfield(window_handle, userIO_ButtonR, button2);
                        winf_hidefield(window_handle, userIO_ButtonM);
                    }
                    else
                    {
                        /* put text in middle button, make left and right buttons invisible */

                        winf_setfield(window_handle, userIO_ButtonM, button1);
                        winf_hidefield(window_handle, userIO_ButtonL);
                        winf_hidefield(window_handle, userIO_ButtonR);
                    }

                    return(0);
                }
              /*else                                               */
              /*    no room to create window/install event handler */
            }
          /*else                         */
          /*    no room to copy template */
        }
      /*else                        */
      /*    failed to find template */
    }
  /*else                          */
  /*    no room for userIO_handle */

    io_destroy(userp);  /* destroys as much as we created */

    *userp = NULL;
    return(create_error(err));        /* probably still set to USERIO_ERR_NOROOM */
}

static void
io_destroy(
    userIO_handle *userp)
{
    userIO_handle user = *userp;

    if(NULL != user)
    {
        if(HOST_WND_NONE != user->window_handle)
            winx_delete_window(&user->window_handle);

        template_copy_dispose(&user->window_template);

        al_ptr_free(*userp);
    }

    *userp = NULL;
}

static S32
io_open(
    userIO_handle *userp)
{
    userIO_handle user = *userp;

    if(NULL != user)
    {
        WimpGetWindowStateBlock window_state;

        window_state.window_handle = user->window_handle;
        if(NULL == WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state)))
        {
            WimpOpenWindowBlock open_window_block = * (WimpOpenWindowBlock *) &window_state;

            open_window_block.window_handle = user->window_handle; /* Probably set, but make sure! */

            open_window_block.behind = -1; /* Make sure this window is opened in front */

            if(NULL == WrapOsErrorReporting(winx_open_window(&open_window_block)))
            {
                user->button = -1; /* nothing has happened yet */
                return(0);
            }
        }
    }

    return(status_nomem());  /* null handle or open failed */
}

static S32
io_poll(
    userIO_handle *userp,
    char *text_out,
    int max_len)
{
    userIO_handle user = *userp;

    if(user)
    {
        if(text_out)
            winf_getfield(user->window_handle, userIO_UserText, text_out, max_len);

        return(user->button);
    }

    return(0);  /* closed */
}

static void
io_close(
    userIO_handle *userp)
{
    userIO_handle user = *userp;

    if(user)
    {
        winx_close_window(user->window_handle); /* no need to validate window handle, winx_close_wind does it */
    }
}

static BOOL
io_event_Close_Window(
    userIO_handle user)
{
    /* merely register as user input */
    if(user->button < 0)
        user->button = 0;   /* closed */

    /* done something, so... */
    return(TRUE);
}

static BOOL
io_event_Mouse_Click(
    const WimpMouseClickEvent * mouse_click,
    userIO_handle user)
{
    if(mouse_click->buttons & (Wimp_MouseButtonSelect | Wimp_MouseButtonAdjust)) /* 'Select' or 'Adjust' */
    {
        switch(mouse_click->icon_handle)
        {
        case userIO_ButtonL:
        case userIO_ButtonM:
            if(user->button < 0)
                user->button = 1;
            break;

        case userIO_ButtonR:
            if(user->button < 0)
                user->button = 2;
            break;

        default:
            break;
        }
    }

    /* done something, so... */
    return(TRUE);
}

static BOOL
io_event_Key_Pressed(
    const WimpKeyPressedEvent * const key_pressed,
    userIO_handle user)
{
    switch(key_pressed->key_code)
    {
    case 13:
        if(user->button < 0)
            user->button = 1;
        break;

    case 27:
        if(user->button < 0)
            user->button = 0;
        break;

    default:
        break;
    }

    /* done something, so... */
    return(TRUE);
}

#define io_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "u_io")

static BOOL
io_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;
    userIO_handle user = (userIO_handle) handle;

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(FALSE); /* default action */

    case Wimp_EOpenWindow:
        return(FALSE); /* default action */

    case Wimp_ECloseWindow:
        return(io_event_Close_Window(user));

    case Wimp_EMouseClick:
        return(io_event_Mouse_Click(&event_data->mouse_click, user));

    case Wimp_EKeyPressed:
        return(io_event_Key_Pressed(&event_data->key_pressed, user));

    default: /* Ignore other events */
        return(FALSE);
    }
}

/* end of userIO.c */
