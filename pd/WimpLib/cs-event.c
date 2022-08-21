/* cs-event.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#include "include.h"

static BOOL
event__default_process(wimp_eventstr * e)
{
    switch(e->e)
    {
    case wimp_ENULL:
        /* machine idle: say so */
        tracef0("unclaimed idle event");
        return(TRUE);


    case wimp_EOPEN:
        trace_0(TRACE_RISCOS_HOST, "unclaimed open request - doing open");
        wimpt_complain(wimp_open_wind(&e->data.o));
        break;


    case wimp_ECLOSE:
        trace_0(TRACE_RISCOS_HOST, "unclaimed close request - doing close");
        wimpt_complain(wimp_close_wind(e->data.close.w));
        break;


    case wimp_EREDRAW:
        {
        wimp_redrawstr r;
        BOOL more;
        trace_0(TRACE_RISCOS_HOST, "unclaimed redraw request - doing redraw");
        r.w = e->data.redraw.w;
        if(!wimpt_complain(wimp_redraw_wind(&r, &more)))
            while(more)
                if(wimpt_complain(wimp_get_rectangle(&r, &more)))
                    more = FALSE;
        }
        break;


    case wimp_EKEY:
        wimpt_complain(wimp_processkey(e->data.key.chcode));
        break;


    case wimp_ESEND:
    case wimp_ESENDWANTACK:
        trace_1(TRACE_RISCOS_HOST, "unclaimed message %s", report_wimp_event(e->e, &e->data));
        switch(e->data.msg.hdr.action)
        {
        case wimp_MCLOSEDOWN:
            exit(EXIT_SUCCESS);

        default:
            break;
        }
        break;

    default:
        trace_1(TRACE_RISCOS_HOST, "unclaimed event %s", report_wimp_event(e->e, &e->data));
        break;
    }

    return(FALSE);
}

static wimp_menuptr
event_read_submenudata(void);

#include "event.c"

/*
SKS: read the menu recreation pending state
*/

extern BOOL
event_is_menu_recreate_pending(void)
{
    return(event_.recreatepending);
}

/*
SKS: for dbox/win to zap pendingrecreate
*/

extern os_error *
event_create_menu(wimp_menustr * m, int x, int y)
{
    event_.recreatepending = FALSE;

    return(wimp_create_menu(m, x, y));
}

extern os_error *
event_create_submenu(wimp_menustr * m, int x, int y)
{
    event_.recreatepending = FALSE;

    return(wimp_create_submenu(m, x, y));
}

/*
SKS: read the cache of submenu opening data - only valid during menu processing!
*/

static wimp_menuptr
event_read_submenudata(void)
{
#if TRACE
    if(!event_.submenu.valid)
        werr(FALSE, "reading invalid submenu data");
#endif

    return(event_.submenu.m);
}

extern void
event_read_submenupos(
    _Out_ int * const xp,
    _Out_ int * const yp)
{
#if TRACE
    if(!event_.submenu.valid)
        werr(FALSE, "reading invalid submenu data");
#endif

    *xp = event_.submenu.x;
    *yp = event_.submenu.y;
}

/*
SKS: read the cache of menu click data - only valid during menu makers!
*/

extern void
event_read_menuclickdata(
    _Out_ int * const xp,
    _Out_ int * const yp,
    _Out_ wimp_w * const wp,
    _Out_ wimp_i * const ip)
{
#if TRACE
    if(!event_.menuclick.valid)
        werr(FALSE, "reading invalid menuclick data");
#endif

    *xp = event_.menuclick.x;
    *yp = event_.menuclick.y;

    *wp = event_.menuclick.w;
    *ip = event_.menuclick.i;
}

/* end of cs-event.c */
