/* userIO.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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
    char *ident,
    char *title,
    char *message,
    char *button1,
    char *button2);

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

typedef struct __userIO_struct
{
    wimp_w  window;
    void   *template;
    S32     button;
}   _userIO_struct;

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
    char *title,
    char *message,
    char *button1,
    char *button2)
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
    char *title,
    char *message,
    char *button1,
    char *button2)
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
    char *ident,
    char *title,
    char *message,
    char *button1,
    char *button2)
{
    userIO_handle  user;
    template      *templatehan;
    wimp_wind     *template;
    wimp_w         window;
    S32            err = STATUS_NOMEM;     /* a useful default value */

    *userp = user = al_ptr_alloc_elem(_userIO_struct, 1, &err);

    if(user)
        {
        user->window   = 0;
        user->template = NULL;
        user->button   = 0;     /* closed */

        templatehan = template_find_new(ident);

        if(templatehan)
            {
            user->template = template = template_copy_new(templatehan);

            if(template)
                {
                if(!win_create_wind(template, &window, io_event_handler, (void*)user))
                    {
                    user->window = window;

                    win_settitle(window, title);

                    win_setfield(window, (wimp_i)userIO_Message, message);
                    win_setfield(window, (wimp_i)userIO_UserText, "");

                    if(button2)
                        {
                        /* put text in left and right buttons, make middle button invisible */

                        win_setfield(window, (wimp_i)userIO_ButtonL, button1);
                        win_setfield(window, (wimp_i)userIO_ButtonR, button2);
                        win_hidefield(window, (wimp_i)userIO_ButtonM);
                        }
                    else
                        {
                        /* put text in middle button, make left and right buttons invisible */

                        win_setfield(window, (wimp_i)userIO_ButtonM, button1);
                        win_hidefield(window, (wimp_i)userIO_ButtonL);
                        win_hidefield(window, (wimp_i)userIO_ButtonR);
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
    if(*userp)
        {
        if((*userp)->window)
            win_delete_wind(&(*userp)->window);

        if((*userp)->template)
            al_ptr_free((*userp)->template);

        al_ptr_free(*userp);
        }

    *userp = NULL;
}

static S32
io_open(
    userIO_handle *userp)
{
    wimp_wstate    state;
    userIO_handle  user = *userp;

    if(user)
        {
        if(wimpt_complain(wimp_get_wind_state(user->window, &state)) == 0)
            {
            state.o.w      = user->window;      /* Probably set, but make sure! */
            state.o.behind = (wimp_w)-1;        /* Make sure window is opened in front */

            if(!win_open_wind(&state.o))
                {
                user->button = -1;      /* nothing has happened yet */

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
    if(*userp)
        {
        if(text_out)
            win_getfield((*userp)->window, (wimp_i)userIO_UserText, text_out, max_len);

        return((*userp)->button);
        }

    return(0);  /* closed */
}

static void
io_close(
    userIO_handle *userp)
{
    if(*userp)
        {
        win_close_wind((*userp)->window);  /* no need to validate window handle, win_close_wind does it */
        }
}

static BOOL
io_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    userIO_handle user = (userIO_handle)handle;

    /* Deal with event */
    switch (e->e)
        {
        case wimp_EOPEN:                                                /* 2 */
            return(FALSE);              /* default action */
            break;

        case wimp_EREDRAW:                                              /* 1 */
            return(FALSE);              /* default action */
            break;

        case wimp_ECLOSE:                                               /* 3 */
            /* merely register as user input */
            if(user->button < 0)
                user->button = 0;   /* closed */
            break;

        case wimp_EBUT:                                                 /* 6 */
            if(e->data.but.m.bbits & 0x5)   /* 'select' or 'adjust' */
                {
                switch ((int)e->data.but.m.i)
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
                    }
                }
            break;

        case wimp_EKEY:
            switch(e->data.key.chcode)
                {
                case 13:
                    if(user->button < 0)
                        user->button = 1;
                    break;

                case 27:
                    if(user->button < 0)
                        user->button = 0;
                    break;
                }
            break;

        default:   /* Ignore any other events */
            return(FALSE);
            break;
        }

    /* done something, so... */
    return(TRUE);
}

/* end of userIO.c */
