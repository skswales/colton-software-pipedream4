/* cs-event.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

static struct _event_statics
{
    BOOL       recreatepending;

    struct _event_menu_click_cache_data
    {
        BOOL            valid;
        int             x;
        int             y;
        HOST_WND        window_handle;
        int             icon_handle;
    }
    menuclick;

    struct _event_submenu_opening_cache_data
    {
        BOOL            valid;
        wimp_menuptr    m;
        int             x;
        int             y;
    }
    submenu;
}
event_statics;

static BOOL
event__default_process(
    const int event_code,
    WimpPollBlock * const event_data);

static wimp_menuptr
event_read_submenudata(void);

#undef BOOL
#undef TRUE
#undef FALSE

#include "event.c"

static _kernel_oserror *
event__default_process_Redraw_Window_Request(
    const WimpRedrawWindowRequestEvent * const redraw_window_request)
{
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;
    _kernel_oserror * e;

    trace_0(TRACE_RISCOS_HOST, "unclaimed Redraw_Window_Request - doing redraw");

    e = tbl_wimp_redraw_window_x(redraw_window_request->window_handle, &redraw_window_block, &more); /* more := FALSE on error */

    while(more)
    {
        /* nothing to redraw */

        e = tbl_wimp_get_rectangle(&redraw_window_block, &more); /* more := FALSE on error */
    }

    return(e);
}

static _kernel_oserror *
event__default_process_Open_Window_Request(
    /*poked*/ WimpOpenWindowRequestEvent * const open_window_request)
{
    trace_0(TRACE_RISCOS_HOST, "unclaimed Open_Window_Request - doing open");

    return(winx_open_window(open_window_request));
}

static _kernel_oserror *
event__default_process_Close_Window_Request(
    const WimpCloseWindowRequestEvent * const close_window_request)
{
    trace_0(TRACE_RISCOS_HOST, "unclaimed Close_Window_Request - doing close");

    return(winx_close_window(close_window_request->window_handle));
}

static _kernel_oserror *
event__default_process_Key_Pressed(
    const WimpKeyPressedEvent * const key_pressed)
{
    return(wimp_processkey(key_pressed->key_code));
}

static BOOL
event__default_process_User_Message(
    const WimpUserMessageEvent * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case wimp_MCLOSEDOWN:
        exit(EXIT_SUCCESS);

    default:
        return(FALSE);
    }
}

static BOOL
event__default_process(
    const int event_code,
    WimpPollBlock * const event_data)
{
    _kernel_oserror * e = NULL;

    switch(event_code)
    {
    case Wimp_ENull:
        /* machine idle: say so */
        tracef0("unclaimed idle event");
        return(TRUE);

    case Wimp_ERedrawWindow:
        e = event__default_process_Redraw_Window_Request(&event_data->redraw_window_request);
        break;

    case Wimp_EOpenWindow:
        e = event__default_process_Open_Window_Request(&event_data->open_window_request);
        break;

    case Wimp_ECloseWindow:
        e = event__default_process_Close_Window_Request(&event_data->close_window_request);
        break;

    case Wimp_EKeyPressed:
        e = event__default_process_Key_Pressed(&event_data->key_pressed);
        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        trace_1(TRACE_RISCOS_HOST, "unclaimed User_Message %s", report_wimp_event(event_code, event_data));
        return(event__default_process_User_Message(&event_data->user_message));

    default:
        trace_1(TRACE_RISCOS_HOST, "unclaimed Wimp Event %s", report_wimp_event(event_code, event_data));
        break;
    }

    wimpt_complain(e);
    return(FALSE);
}

/*
SKS: read the menu recreation pending state
*/

_Check_return_
extern BOOL
event_is_menu_recreate_pending(void)
{
    return(event_statics.recreatepending);
}

/*
SKS: for dbox/win to zap pendingrecreate
*/

_Check_return_
_Ret_maybenull_
extern os_error *
event_create_menu(wimp_menustr * m, int x, int y)
{
    event_statics.recreatepending = FALSE;

    return(wimp_create_menu(m, x, y));
}

_Check_return_
_Ret_maybenull_
extern os_error *
event_create_submenu(wimp_menustr * m, int x, int y)
{
    event_statics.recreatepending = FALSE;

    return(wimp_create_submenu(m, x, y));
}

/*
SKS: read the cache of submenu opening data - only valid during menu processing!
*/

static wimp_menuptr
event_read_submenudata(void)
{
#if TRACE
    if(!event_statics.submenu.valid)
        werr(FALSE, "reading invalid submenu data");
#endif

    return(event_statics.submenu.m);
}

extern void
event_read_submenupos(
    _Out_ int * const p_x,
    _Out_ int * const p_y)
{
#if TRACE
    if(!event_statics.submenu.valid)
        werr(FALSE, "reading invalid submenu data");
#endif

    *p_x = event_statics.submenu.x;
    *p_y = event_statics.submenu.y;
}

/*
SKS: read the cache of menu click data - only valid during menu makers!
*/

extern void
event_read_menuclickdata(
    _Out_ int * const p_x,
    _Out_ int * const p_y,
    _Out_ HOST_WND * const p_window_handle,
    _Out_ int * const p_icon_handle)
{
#if TRACE
    if(!event_statics.menuclick.valid)
        werr(FALSE, "reading invalid menuclick data");
#endif

    *p_x = event_statics.menuclick.x;
    *p_y = event_statics.menuclick.y;

    *p_window_handle = event_statics.menuclick.window_handle;
    *p_icon_handle   = event_statics.menuclick.icon_handle;
}

/* end of cs-event.c */
