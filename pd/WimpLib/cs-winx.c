/* cs-winx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

#include "cmodules/aligator.h"

_Check_return_
extern BOOL
winx_adjustclicked(void)
{
    wimp_mousestr m;

    if(wimpt_complain(wimp_get_point_info(&m)))
        return(FALSE);

    return(0 != (m.bbits & wimp_BRIGHT));
}

extern void
winx_changedtitle(
    _HwndRef_   HOST_WND window_handle)
{
    wimp_wstate wstate;

    if(wimpt_complain(wimp_get_wind_state(window_handle, &wstate)))
        return;

    if(wstate.flags & wimp_WOPEN)
    {
        wimp_redrawstr redraw;
        int dy = wimpt_dy();
        redraw.w      = (wimp_w) -1; /* entire screen */
        redraw.box.x0 = wstate.o.box.x0;
        redraw.box.y0 = wstate.o.box.y1 + dy; /* title bar contents starts one raster up */
        redraw.box.x1 = wstate.o.box.x1;
        redraw.box.y1 = redraw.box.y0 + wimp_win_title_height(dy) - 2*dy;
        trace_0(TRACE_RISCOS_HOST, TEXT("queuing global redraw of inside of title bar area"));
        (void) wimpt_complain(wimp_force_redraw(&redraw));
    }
}

/*****************************************************
*
* send (via WM queue) a close request to this window
*                             ~~~~~~~
*****************************************************/

extern void
winx_send_close_window_request(
    _HwndRef_   HOST_WND window_handle)
{
    wimp_eventdata msg;

    msg.o.w = window_handle;
    (void) wimpt_complain(wimp_send_message_to_window(Wimp_ECloseWindow, (wimp_msgstr *) &msg, window_handle, -1));
}

/************************************************
*
* send (either via WM queue, or immediate fake)
* an open request to this window
*         ~~~~~~~
************************************************/

extern void
winx_send_open_window_request(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate,
    const WimpOpenWindowBlock * open_window)
{
    WimpEvent ev;

    tracef2("[winx_send_open_window_request(&%p, immediate = %s)]",
            (void *) window_handle, report_boolstring(immediate));

    ev.event_data.open_window_request   = *open_window;
    ev.event_data.open_window_request.window_handle = window_handle;
    if(immediate)
    {
        ev.event_code = Wimp_EOpenWindow;
        wimpt_fake_event((wimp_eventstr *) &ev);
        event_process();
    }
    else
    {
        (void) wimpt_complain(wimp_send_message_to_window(Wimp_EOpenWindow, (wimp_msgstr *) &ev.event_data, window_handle, -1));
    }
}

/************************************************
*
* send (either via WM queue, or immediate fake)
* an open at front request to this window
*                  ~~~~~~~
************************************************/

extern void
winx_send_front_window_request(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate)
{
    wimp_wstate wstate;

    tracef2("[winx_send_front_window_request(&%p, immediate = %s)]", (void *) window_handle, report_boolstring(immediate));

    if(wimpt_complain(wimp_get_wind_state(window_handle, &wstate)))
        return;

    wstate.o.behind = (wimp_w) -1;          /* force to the front */
    winx_send_open_window_request(window_handle, immediate, (WimpOpenWindowBlock *) &wstate.o);
}

extern void
winx_send_front_window_request_at(
    _HwndRef_   HOST_WND window_handle,
    BOOL immediate,
    const BBox * const bbox)
{
    wimp_wstate wstate;

    tracef2("[winx_send_front_window_request_at(&%p, immediate = %s)]", (void *) window_handle, report_boolstring(immediate));

    /* get current scroll offsets */
    if(wimpt_complain(wimp_get_wind_state(window_handle, &wstate)))
        return;

    wstate.o.behind = (wimp_w) -1;          /* force to the front */
    wstate.o.box    = * (const wimp_box *) bbox;
    winx_send_open_window_request(window_handle, immediate, (WimpOpenWindowBlock *) &wstate.o);
}

/*
enhancements to win.c
*/

#define WIN_WINDOW_LIST 1

/*
data structure for win-controlled windows - exists as long as window, even if hidden
*/

typedef struct WIN__STR {
  struct WIN__STR *link;

  HOST_WND window_handle;
  winx_new_event_handler proc;
  void *handle;
  int new_proc;
  void *menuh;

  HOST_WND parent_window_handle;  /* parent window handle if a child */
  struct WIN__CHILD_STR *children; /* windows can have children hanging off them */

  int menuhi_icon_handle; /* icon to which menu is registered */
  void * menuhi;   /* registered icon menu (if any) */

  /* extra data follows here */
} WIN__STR;

#define WIN_OFFSET_MENUH    ((int) offsetof(WIN__STR, menuh)                 - (int) sizeof(WIN__STR))
#define WIN_OFFSET_PARENTW  ((int) offsetof(WIN__STR, parent_window_handle)  - (int) sizeof(WIN__STR))
#define WIN_OFFSET_CHILDREN ((int) offsetof(WIN__STR, children)              - (int) sizeof(WIN__STR))

static WIN__STR * win__window_list = NULL;

/*
data structure for child windows hanging off a parent window
*/

typedef struct WIN__CHILD_STR
{
    struct WIN__CHILD_STR *link;
    HOST_WND window_handle; /* window handle of child */
    int offset_x, offset_y; /* offsets of child x0,y1 from parent x0,y1 */
}
WIN__CHILD_STR;

static struct WINX_STATICS
{
    HOST_WND drag_window_handle;

    HOST_WND submenu_window_handle;
}
winx_statics;

static BOOL winx__register_new(
    _HwndRef_   HOST_WND window_handle,
    winx_new_event_handler eventproc,
    void *handle,
    int new_proc)
{
    /* search list to see if it's already there */
    WIN__STR *p = (WIN__STR *) &win__window_list;
    BOOL new_thing = TRUE;

    while((p = p->link) != NULL)
    {
        if(p->window_handle == window_handle)
        {
            new_thing = FALSE; /* found previous definition */
            break;
        }
    }

    if(new_thing)
        p = wlalloc_calloc(1, sizeof(WIN__STR)); /* zeroing */

    if(p)
    {
        if(new_thing)
        {
            p->link = win__window_list; /* add to head of list */
            win__window_list = p;

            p->window_handle = window_handle;
        }

        p->proc     = eventproc;
        p->handle   = handle;
        p->new_proc = new_proc;
    }

    return(p != NULL);
}

static void win__discard(
    wimp_w window_handle);

extern BOOL
winx_register_new_event_handler(
    _HwndRef_   HOST_WND window_handle,
    winx_new_event_handler newproc,
    void *handle)
{
    if(NULL == newproc)
    {
        win__discard(window_handle);
        return(TRUE);
    }

    return(winx__register_new(window_handle, newproc, handle, 1 /* for new binaries */));
}

#undef BOOL
#undef TRUE
#undef FALSE

#define win__str WIN__STR

#include "win.c"

extern os_error *
winx_drag_box(wimp_dragstr * dr)
{
    WIN__STR *p = win__find(dr->window);

    /* MUST be valid, unlike what PRM doc says, due to changed event handling here */
    myassert1x(p != NULL, "winx_drag_box for unregistered window &%p", (void *) dr->window);
    if(p)
        winx_statics.drag_window_handle = dr->window;

    return(wimp_drag_box(dr));
}

_Check_return_
extern void *
winx_menu_get_handle(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle)
{
    WIN__STR *p_winx_window;

    if(icon_handle == -1 /*WORKAREA*/)
        return(win_getmenuh(window_handle));

    p_winx_window = win__find(window_handle);

    if(NULL != p_winx_window)
        if(p_winx_window->menuhi_icon_handle == icon_handle)
            return(p_winx_window->menuhi);

    return(NULL);
}

extern void
winx_menu_set_handle(
    _HwndRef_   HOST_WND window_handle,
    _InVal_     int icon_handle,
    void * handle)
{
    WIN__STR *p_winx_window;

    if(icon_handle == -1 /*WORKAREA*/)
    {
        win_setmenuh(window_handle, handle);
        return;
    }

    p_winx_window = win__find(window_handle);

    if(NULL != p_winx_window)
    {
        p_winx_window->menuhi_icon_handle = icon_handle;
        p_winx_window->menuhi = handle;
    }
}

/**********************************************************
*
* register a window as a child of another (parent) window
*
**********************************************************/

extern BOOL
winx_register_child(
    _HwndRef_   HOST_WND parent_window_handle,
    _HwndRef_   HOST_WND window_handle)
{
    WIN__STR *parp = win__find(parent_window_handle);
    WIN__STR *chip = win__find(window_handle);
    WIN__CHILD_STR *c;

    tracef2("[winx_register_child(child &%p to parent &%p): ", (void *) parent_window_handle, (void *) window_handle);

    /* one or other window not created via this module? */
    if(!parp || !chip)
        return(FALSE);

    /* already a child of someone else? */
    if((chip->parent_window_handle != HOST_WND_NONE)  &&  (chip->parent_window_handle != parent_window_handle))
        return(FALSE);

    /* search parent's child list to see if it's already there */
    c = (WIN__CHILD_STR *) &parp->children;

    while((c = c->link) != NULL)
        if(c->window_handle == window_handle)
            /* already registered with this window as parent */
            break;

    if(!c)
    {
        c = wlalloc_calloc(1, sizeof(WIN__CHILD_STR)); /* zeroing */

        if(c)
        {
            tracef1("adding win__child_str &%p ", c);

            c->link = parp->children; /* add to head of list */
            parp->children = c;

            c->window_handle = window_handle;

            chip->parent_window_handle = parent_window_handle;
        }
    }

    if(c)
    {   /* note current parent<->child offset */
        wimp_wstate wstate;

        if(!wimpt_complain(wimp_get_wind_state(parent_window_handle, &wstate)))
        {
            int parx = wstate.o.box.x0;
            int pary = wstate.o.box.y1;

            if(!wimpt_complain(wimp_get_wind_state(window_handle, &wstate)))
            {
                c->offset_x = wstate.o.box.x0 - parx; /* add on to parent pos to recover child pos */
                c->offset_y = wstate.o.box.y1 - pary;
                tracef2("at offset %d,%d from parent]", c->offset_x, c->offset_y);
            }
        }
    }
    else
        tracef0("failed to register child]");

    return(c != NULL);
}

/***************************************************
*
* deregister a child window from its parent window
*
***************************************************/

extern BOOL
winx_deregister_child(
    _HwndRef_   HOST_WND window_handle)
{
    WIN__STR *parp;
    WIN__STR *chip = win__find(window_handle);
    WIN__CHILD_STR *pp;
    WIN__CHILD_STR *cp;

    tracef1("winx_deregister_child(child &%p): ", (void *) window_handle);

    if(!chip)
        return(FALSE);

    /* not a child? */
    if(chip->parent_window_handle == HOST_WND_NONE)
        return(FALSE);

    parp = win__find(chip->parent_window_handle);
    if(!parp)
        return(FALSE);

    /* remove parent window ref from child window structure */
    chip->parent_window_handle = HOST_WND_NONE;

    /* remove child window from parent's list */
    pp = (WIN__CHILD_STR *) &parp->children;

    while((cp = pp->link) != NULL)
    {
        if(cp->window_handle == window_handle)
        {
            tracef1("removing win__child_str &%p", cp);
            pp->link = cp->link;
            wlalloc_free(cp);
            return(TRUE);
        }
        else
            pp = cp;
    }

    tracef0("failed to deregister child from parent");

    return(FALSE);
}

/*
use winx_create_wind rather than wimp_create_wind to get enhanced window handling
*/

extern os_error *
winx_create_window(
    WimpWindowWithBitset * const p_window_template,
    _Out_       HOST_WND * const p_window_handle,
    winx_new_event_handler proc,
    void * handle)
{
    os_error * e;

    e = wimp_create_wind((wimp_wind *) p_window_template, (wimp_w *) p_window_handle);

    if(NULL == e)
        if(!winx__register_new(*p_window_handle, proc, handle, 1))
            e = os_set_error(0, msgs_lookup("winT1" /* "Not enough memory to create window" */));

    return(e);
}

/************************************************************
*
* close (but don't delete) this window and all its children
*
************************************************************/

extern os_error *
winx_close_window(
    _HwndRef_   HOST_WND window_handle)
{
    os_error * e = NULL;
    os_error * e1;
    WIN__STR *p = win__find(window_handle);
    WIN__CHILD_STR *c;

    if(!p)
        return(NULL);

    if(winx_submenu_query_is_submenu(window_handle))
    {
        tracef0("closing submenu window ");

        if(!winx_submenu_query_closed())
        {
            tracef0("- explicit closure of open window");
            /* window being closed without the menu tree knowing about it */
            /* cause the menu system to close the window */
            event_clear_current_menu();
        }
        else
            tracef0("- already been closed by Wimp");

        winx_statics.submenu_window_handle = 0;
    }

    /* is this window a child? */
    if(p->parent_window_handle)
        /* remove from parent's list */
        winx_deregister_child(window_handle);

    /* loop closing child windows */
    c = (WIN__CHILD_STR *) &p->children;
    while((c = c->link) != NULL)
    {
        e1 = winx_close_window(c->window_handle);
        e  = e ? e : e1;
    }

    /* close window to Window Manager */
    e1 = wimp_close_wind(window_handle);

    return(e ? e : e1);
}

/************************************************************
*
* delete this window and all its children
*
************************************************************/

extern os_error *
winx_delete_window(
    _Inout_     HOST_WND * const p_window_handle)
{
    os_error * e;
    os_error * e1;
    WIN__STR *p;
    HOST_WND window_handle = *p_window_handle;

    if(0 == window_handle)
        return(NULL);

    *p_window_handle = 0;

    /* close internally (closes all children too) */
    e = winx_close_window(window_handle);

    /* delete children */
    p = win__find(window_handle);

    if(p)
    {
        /* is this window a child? */
        if(p->parent_window_handle)
            /* remove from parent's list */
            winx_deregister_child(window_handle);

        /* loop deleting child windows, restarting each time */
        while(p->children)
        {
            e1 = winx_delete_window(&p->children->window_handle);
            e  = e ? e : e1;
            p = win__find(window_handle);
        }
    }

    win__discard(window_handle);

    /* delete window to Window Manager */
    e1 = wimp_delete_wind(window_handle);

    return(e ? e : e1);
}

/**********************************************************
*
* open a window and its children
*
**********************************************************/

/*
*
* ---------------------------------- all behind this w
*
*       -----
*         |
*       1.1.1
*         |
*       ----------
*            |
*            |
*            |                -----
*            |                  |
*            |                1.2.1
*            |                  |
*           1.1               ----------
*            |                    |
*            |                   1.2
*            |                    |
*       -------------------------------- c1
*                       |
*                       |
*                       |                           -----------
*                       |                                |
*                       |                               2.1
*                       1                                |
*                       |                           ------------------------ c2
*                       |                                      |
*                       |                                      2
*                       |                                      |
*       -------------------------------------------------------------------- w
*
* note that the entire child 2 stack is between the parent and child 1
* and that this applies recursively e.g. to child1.1 and child1.2 stacks
*
* recursive generation of winx_open_window() via Wimp_EOpenWindow of each
* child window is the key to this happening, either by the child window
* doing the winx_open_window() itself or by allowing the default event handler
* to do it for it. note that the child windows get events to allow them to
* set extents etc. at a suitable place rather than this routine simply
* calling itself directly.
*
*/

extern os_error *
winx_open_window(WimpOpenWindowBlock * const p_open_window_block)
{
    wimp_openstr * const o = (wimp_openstr *) p_open_window_block;
    const HOST_WND   window_handle = p_open_window_block->window_handle;
    os_error * e;
    os_error * e1 = NULL;
    WIN__STR *       p = win__find(window_handle);
    WIN__CHILD_STR * c;
    wimp_openstr     childo;
    wimp_wstate      wstate;
    HOST_WND         behind = p_open_window_block->behind;

    tracef6("[winx_open_window(&%p at %d,%d;%d,%d behind &%p]",
        (void *) window_handle, o->box.x0, o->box.y0, o->box.x1, o->box.y1, o->behind);

    if(p)
    {
        if(behind != (HOST_WND) -2)
        {
            /* not sending to back: bring stack forwards first to minimize redraws */

            /* on mode change, Neil tells us to open windows behind themselves! */
            if((behind == window_handle)  &&  p->children)
            {
                /* see which window the pane stack is really behind */
                e1 = wimp_get_wind_state(p->children->window_handle, &wstate);

                if(NULL == e1)
                {
                    tracef2("[window &%p pane stack is behind window &%p]", (void *) window_handle, (void *) wstate.o.behind);
                    behind = wstate.o.behind;
                }
            }

            /* loop trying to open child windows at new positions */
            c = (WIN__CHILD_STR *) &p->children;
            while((c = c->link) != NULL)
            {
                os_error * e2;

                childo.w      = c->window_handle;
                childo.box.x0 = o->box.x0 + c->offset_x;
                childo.box.y0 = o->box.y0 + c->offset_y;
                childo.box.x1 = o->box.x1 + c->offset_x;
                childo.box.y1 = o->box.y1 + c->offset_y;
                childo.behind = behind;
                e2 = winx_open_window((WimpOpenWindowBlock *) &childo);
                behind = c->window_handle; /* open next pane behind this one */

                e1 = e1 ? e1 : e2;
            }

            /* always open parent window behind last pane window */
            o->behind = behind;
        }
    }

    /* open the window */
    e = wimp_open_wind(o);

    e = e ? e : e1;

    if(p)
    {
        /* loop reopening panes with now-corrected coords */
        c = (WIN__CHILD_STR *) &p->children;
        while((c = c->link) != NULL)
        {
            os_error * e2 = wimp_get_wind_state(c->window_handle, &wstate);

            if(NULL == e2)
            {
                /* just translate to correct place, don't shuffle stack */
                wstate.o.box.x0 = o->box.x0 + c->offset_x;
                wstate.o.box.y0 = o->box.y0 + c->offset_y;
                wstate.o.box.x1 = o->box.x1 + c->offset_x;
                wstate.o.box.y1 = o->box.y1 + c->offset_y;

                e2 = winx_open_window((WimpOpenWindowBlock *) &wstate.o);
            }

            e = e ? e : e2;
        }
    }

    return(e);
}

/***********************************************************
*
* SKS centralise submenu window handling code
*
***********************************************************/

extern os_error *
winx_create_submenu(
    _HwndRef_   HOST_WND window_handle,
    int x,
    int y)
{
    os_error * err;

    /* do we have a different menu tree window up already? if so, close it */
    if((winx_statics.submenu_window_handle != 0)  &&  (winx_statics.submenu_window_handle != window_handle))
        winx_send_close_window_request(winx_statics.submenu_window_handle);

    err = event_create_submenu((wimp_menustr *) window_handle, x, y);

    if(!err)
        winx_statics.submenu_window_handle = window_handle; /* there can be only one */
    else
        winx_statics.submenu_window_handle = 0;

    return(err);
}

/* only needed 'cos Window Manager is too stupid to realise that submenu creation
 * without a parent menu should be valid
*/

extern os_error *
winx_create_menu(
    _HwndRef_   HOST_WND window_handle,
    int x,
    int y)
{
    os_error * err;

    /* do we have a different menu tree window up already? if so, close it */
    if((winx_statics.submenu_window_handle != 0)  &&  (winx_statics.submenu_window_handle != window_handle))
        winx_send_close_window_request(winx_statics.submenu_window_handle);

    err = event_create_menu((wimp_menustr *) window_handle, x, y);

    if(!err)
        winx_statics.submenu_window_handle = window_handle; /* there can be only one */
    else
        winx_statics.submenu_window_handle = 0;

    return(err);
}

extern BOOL
winx_submenu_query_is_submenu(
    _HwndRef_   HOST_WND window_handle)
{
    if(winx_statics.submenu_window_handle != 0)
        return(window_handle == winx_statics.submenu_window_handle);

    return(FALSE);
}

extern BOOL
winx_submenu_query_closed(void)
{
    wimp_wstate ws;

    if(winx_statics.submenu_window_handle != 0)
        if(!wimpt_complain(wimp_get_wind_state(winx_statics.submenu_window_handle, &ws)))
            return(0 == (ws.flags & wimp_WOPEN));

    /* there is none, you fool */
    return(TRUE);
}

/* end of cs-winx.c */
