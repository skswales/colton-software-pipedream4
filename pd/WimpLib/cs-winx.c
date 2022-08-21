/* cs-winx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#include "include.h"

#include "cmodules/aligator.h"

/*****************************************************
*                                                    *
* send (via WM queue) a close request to this window *
*                             ~~~~~~~                *
*****************************************************/

extern void
win_send_close(wimp_w w)
{
    wimp_eventdata msg;

    msg.o.w = w;
    wimpt_safe(wimp_sendwmessage(wimp_ECLOSE, (wimp_msgstr *) &msg, w, (wimp_i) -1));
}

/************************************************
*                                               *
* send (either via WM queue, or immediate fake) *
* an open request to this window                *
*         ~~~~~~~                               *
************************************************/

extern void
win_send_open(wimp_w w, BOOL immediate, const wimp_openstr * openstr)
{
    wimp_eventstr e;

    tracef2("[win_send_open(&%p, immediate = %s)]",
            w, trace_boolstring(immediate));

    e.data.o   = *openstr;
    e.data.o.w = w;
    if(immediate)
        {
        e.e = wimp_EOPEN;
        wimpt_fake_event(&e);
        event_process();
        }
    else
        wimpt_safe(wimp_sendwmessage(wimp_EOPEN, (wimp_msgstr *) &e.data.o, w, (wimp_i) -1));
}

/************************************************
*                                               *
* send (either via WM queue, or immediate fake) *
* an open at front request to this window       *
*                  ~~~~~~~                      *
************************************************/

extern void
win_send_front(wimp_w w, BOOL immediate)
{
    wimp_wstate wstate;

    tracef2("[win_send_front(&%p, immediate = %s)]", w, trace_boolstring(immediate));

    wimpt_safe(wimp_get_wind_state(w, &wstate));
    wstate.o.behind = (wimp_w) -1;          /* force to the front */
    win_send_open(w, immediate, &wstate.o);
}

extern void
win_send_front_at(wimp_w w, BOOL immediate, const wimp_box * box)
{
    wimp_wstate wstate;

    tracef2("[win_send_front_at(&%p, immediate = %s)]", w, trace_boolstring(immediate));

    /* get current scroll offsets */
    wimpt_safe(wimp_get_wind_state(w, &wstate));
    wstate.o.behind = (wimp_w) -1;          /* force to the front */
    wstate.o.box    = *box;
    win_send_open(w, immediate, &wstate.o);
}

extern BOOL
win_adjustbumphit(wimp_i * hitp /*inout*/, wimp_i val)
{
    wimp_i hit = *hitp;
    wimp_i dec = val - 1;
    wimp_i inc = dec - 1;
    BOOL   res = ((hit == inc)  ||  (hit == dec));

    if(res  &&  win_adjustclicked())
        *hitp = (hit ^ inc ^ dec);

    return(res);
}

extern BOOL
win_adjustclicked(void)
{
    wimp_mousestr m;

    wimpt_safe(wimp_get_point_info(&m));

    return((m.bbits & wimp_BRIGHT) != 0);
}

extern void
win_changedtitle(wimp_w w)
{
    wimp_wstate wstate;

    if(!wimpt_complain(wimp_get_wind_state(w, &wstate)))
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
            wimpt_safe(wimp_force_redraw(&redraw));
        }
}

extern void
win_getfield(wimp_w w, wimp_i i, char *buffer /*out*/, size_t size)
{
    wimp_icon    info;
    size_t       j;
    const char * from;
    const char * ptr;

    tracef4("[win_getfield(&%p, &%p, &%p, %d): ", w, i, buffer, size);

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    if(!(info.flags & wimp_ITEXT))
        {
        tracef0("- non-text icon: ");
        j = 0;  /* returns "" */
        }
    else
        {
        myassert2x(info.flags & wimp_INDIRECT, "win_getfield: window &%p icon &%u has inline buffer", w, i);

        from = info.data.indirecttext.buffer;
        ptr  = from;
        while(*ptr++ >= 32)
            ;
        j = ptr - from - 1;
        j = min(j, size - 1);
        memcpy(buffer, from, j);
        }

    buffer[j] = NULLCH;
    tracef1(" returns \"%s\"]", buffer);
}

extern void
win_setfield(wimp_w w, wimp_i i, const char * value)
{
    wimp_icon     info;
    wimp_caretstr caret;
    int           newlen, oldlen, maxlen;

    tracef3("[win_setfield(&%p, &%p, \"%s\"): ", w, i, value);

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    if(!(info.flags & wimp_ITEXT))
        {
        tracef0(" non-text icon - ignored");
        return;
        }

    myassert2x(info.flags & wimp_INDIRECT, "win_setfield: window &%p icon &%u has inline buffer", w, i);

    if(!strcmp(info.data.indirecttext.buffer, value))
        return;

    maxlen = info.data.indirecttext.bufflen - 1;  /* MUST be term */
    oldlen = strlen(info.data.indirecttext.buffer);
    info.data.indirecttext.buffer[0] = NULLCH;
    strncat(info.data.indirecttext.buffer, value, maxlen);
    tracef1(" indirect buffer now \"%s\"]", info.data.indirecttext.buffer);
    newlen = strlen(info.data.indirecttext.buffer);

    /* ensure that the caret moves correctly if it's in this icon */
    wimpt_safe(wimp_get_caret_pos(&caret));
    tracef2("[caret in window &%p, icon &%p]", caret.w, caret.i);

    if((caret.w == w)  &&  (caret.i == i))
        {
        if(caret.index == oldlen)
            caret.index = newlen;       /* if grown and caret was at end,
                                         * move caret right */
        if(caret.index > newlen)
            caret.index = newlen;       /* if shorter, bring caret left */

        caret.height = -1;   /* calc x,y,h from icon/index */

        wimpt_complain(wimp_set_caret_pos(&caret));
        }

    /* prod it, to cause redraw */
    wimpt_safe(
        wimp_set_icon_state(w, i,
                            /* EOR */ (wimp_iconflags) 0,
                            /* BIC */ wimp_ISELECTED));
}

extern void
win_fadefield(wimp_w w, wimp_i i)
{
    wimp_icon info;

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    if((info.flags & wimp_INOSELECT) == 0)
        /* prod it, to cause redraw */
        wimpt_safe(
            wimp_set_icon_state(w, i,
                                /* EOR */ wimp_INOSELECT,
                                /* BIC */ (wimp_iconflags) 0));
}

extern void
win_unfadefield(wimp_w w, wimp_i i)
{
    wimp_icon info;

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    if((info.flags & wimp_INOSELECT) != 0)
        /* prod it, to cause redraw */
        wimpt_safe(
            wimp_set_icon_state(w, i,
                                /* EOR */ wimp_INOSELECT,
                                /* BIC */ (wimp_iconflags) 0));
}

/* note that unhide is rather harder - would need a saved state to restore */

extern void
win_hidefield(wimp_w w, wimp_i i)
{
    wimpt_safe(
        wimp_set_icon_state(w, i,
                            /*EOR*/ (wimp_iconflags) 0,
                            /*BIC*/ (wimp_iconflags) (wimp_ITEXT | wimp_ISPRITE | wimp_IBORDER | wimp_IFILLED | wimp_IBUTMASK)));
}

extern void
win_invertfield(wimp_w w, wimp_i i)
{
    wimp_icon icon;

    wimpt_safe(wimp_get_icon_info(w, i, &icon));

    wimpt_safe(
        wimp_set_icon_state(w, i,
                            /* EOR */    wimp_ISELECTED,
                            /* BIC */    icon.flags & wimp_ISELECTED ? (wimp_iconflags) 0 : wimp_ISELECTED));
}

/*
double field handling
*/

static double
round_to_decplaces(
    const double * dvar,
    int decplaces)
{
    double val = *dvar;
    int    negative = (val < 0.0);
    double base;

    if(negative)
        val = fabs(val);

    base = pow(10.0, decplaces);
    val  = floor(val * base + 0.5) / base;

    return(negative ? -val : +val);
}

extern BOOL
win_bumpdouble(
    wimp_w w, wimp_i hit, wimp_i val,
    double * dvar /*inout*/,
    const double * ddelta,
    const double * dmin,
    const double * dmax,
    int            decplaces)
{
    wimp_i dec = val - 1;
    double try_ddelta, dval;
    BOOL   res = win_adjustbumphit(&hit, val);

    if(res)
        {
        try_ddelta = *ddelta;

        if(hit == dec)
            try_ddelta = 0.0 - try_ddelta;

        dval = win_getdouble(w, val, dvar, decplaces);

        dval += try_ddelta;

        if( dval > *dmax)
            dval = *dmax;
        else if(dval < *dmin)
            dval = *dmin;

        *dvar = dval;

        win_setdouble(w, val, dvar, decplaces);
        }

    return(res);
}

extern void
win_checkdouble(
    wimp_w w, wimp_i val,
    double * dvar /*inout*/,
    int * modify /*out-if-mod*/,
    const double * dmin,
     const double * dmax,
    int            decplaces)
{
    double dval = win_getdouble(w, val, dvar, decplaces);

    if( dval > *dmax)
        dval = *dmax;
    else if(dval < *dmin)
        dval = *dmin;

    if(*dvar != dval)
        {
        *dvar = dval;

        if(modify)
            *modify = 1;
        }

    /* NEVER set modify 0; that's for the caller to accumulate */
}

extern double
win_getdouble(
    wimp_w w, wimp_i i,
    const double * default_val,
    int            decplaces)
{
    char   buffer[64];
    char * ptr;
    double val;

    win_getfield(w, i, buffer, sizeof(buffer));

    val = strtod(buffer, &ptr);

    if(ptr == buffer)
        val = +HUGE_VAL;

    if((val == +HUGE_VAL)  ||  (val == -HUGE_VAL))
        if(default_val)
            return(*default_val);

    if(decplaces != INT_MAX)
        val = round_to_decplaces(&val, decplaces);

    return(val);
}

extern void
win_setdouble(
    wimp_w w, wimp_i i,
    const double * val,
    int decplaces)
{
    char buffer[32];
    double dval;

    if(decplaces != INT_MAX)
        dval = round_to_decplaces(val, decplaces);
    else
        dval = *val;

    (void) sprintf(buffer, "%g", dval);

    win_setfield(w, i, buffer);
}

/*
int field handling
*/

extern BOOL
win_bumpint(
    wimp_w w, wimp_i hit, wimp_i val,
    int * ivar /*inout*/,
    int idelta, int imin, int imax)
{
    wimp_i dec = val - 1;
    int    try_idelta, ival;
    BOOL   res = win_adjustbumphit(&hit, val);

    if(res)
        {
        try_idelta = idelta;

        if(hit == dec)
            try_idelta = 0 - try_idelta;

        ival = win_getint(w, val, *ivar);

        ival += try_idelta;

        if( ival > imax)
            ival = imax;
        else if(ival < imin)
            ival = imin;

        *ivar = ival;

        win_setint(w, val, *ivar);
        }

    return(res);
}

extern void
win_checkint(
    wimp_w w, wimp_i val,
    int * ivar /*inout*/,
    int * modify /*out-if-mod*/,
    int imin, int imax)
{
    int ival = win_getint(w, val, *ivar);

    if( ival > imax)
        ival = imax;
    else if(ival < imin)
        ival = imin;

    if(*ivar != ival)
        {
        *ivar = ival;

        if(modify)
            *modify = 1;
        }

    /* NEVER set modify 0; that's for the caller to accumulate */
}

extern int
win_getint(
    wimp_w w, wimp_i i,
    int default_val)
{
    char   buffer[32];
    char * ptr;
    int    val;

    win_getfield(w, i, buffer, sizeof(buffer));

    val = (int) strtol(buffer, &ptr, 0);

    if(ptr == buffer)
        return(default_val);

    return(val);
}

extern void
win_setint(
    wimp_w w, wimp_i i,
    int val)
{
    char buffer[16];

    (void) sprintf(buffer, "%d", val);

    win_setfield(w, i, buffer);
}

/*
on/off handling
*/

/*
* returns TRUE if icon selected
*/

extern BOOL
win_getonoff(wimp_w w, wimp_i i)
{
    wimp_icon info;

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    return((info.flags & wimp_ISELECTED) == wimp_ISELECTED);
}

extern void
win_setonoff(wimp_w w, wimp_i i, BOOL on)
{
    wimp_icon info;
    int       cur_state, req_state;

    wimpt_safe(wimp_get_icon_info(w, i, &info));

    cur_state = (info.flags & wimp_ISELECTED);
    req_state = (on ? wimp_ISELECTED : 0);

    if(cur_state != req_state)
        wimpt_complain(wimp_set_icon_state(w, i,
                                    /* EOR */   (wimp_iconflags) req_state,
                                    /* BIC */   wimp_ISELECTED));
}

extern void
win_setonoffpair(wimp_w w, wimp_i i, BOOL on)
{
    win_setonoff(w, i,      on);
    win_setonoff(w, i + 1, !on);
}

extern wimp_i
win_whichonoff(wimp_w w, wimp_i first, wimp_i last, wimp_i dft)
{
    wimp_i i;
    int    ival;

    for(i = last; i >= first; --i)
        {
        ival = win_getonoff(w, i);

        if(ival)
            break;

        if(i == first)
            return(dft);
        }

    return(i);
}

/* suss a pair of radio buttons, first being default too */

extern wimp_i
win_whichonoffpair(wimp_w w, wimp_i first)
{
    return(win_whichonoff(w, first, first + 1, first));
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

  wimp_w w;
  win_new_event_handler proc;
  void *handle;
  int new_proc;
  void *menuh;

  wimp_w parentw;  /* parent window handle if a child */
  struct WIN__CHILD_STR *children; /* windows can have children hanging off them */

  wimp_i menuhi_i; /* icon to which menu is registered */
  void * menuhi;   /* registered icon menu (if any) */

  /* extra data follows here */
} WIN__STR;

#define WIN_OFFSET_MENUH    ((int) offsetof(WIN__STR, menuh)    - (int) sizeof(WIN__STR))
#define WIN_OFFSET_PARENTW  ((int) offsetof(WIN__STR, parentw)  - (int) sizeof(WIN__STR))
#define WIN_OFFSET_CHILDREN ((int) offsetof(WIN__STR, children) - (int) sizeof(WIN__STR))

static WIN__STR * win__window_list = NULL;

/*
data structure for child windows hanging off a parent window
*/

typedef struct WIN__CHILD_STR
{
    struct WIN__CHILD_STR *link;
    wimp_w w; /* window handle of child */
    int x, y; /* offsets of child x0,y1 from parent x0,y1 */
}
WIN__CHILD_STR;

static wimp_w win__drag_w = 0;

static wimp_w win_submenu_w = 0;

static BOOL win__register_new(wimp_w w, win_new_event_handler eventproc, void *handle, int new_proc)
{
    /* search list to see if it's already there */
    WIN__STR *p = (WIN__STR *) &win__window_list;
    BOOL new_thing = TRUE;

    while((p = p->link) != NULL)
    {
        if(p->w == w)
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

            p->w = w;
        }

        p->proc     = eventproc;
        p->handle   = handle;
        p->new_proc = new_proc;
    }

    return(p != NULL);
}

static void win__discard(wimp_w w);

BOOL win_register_new_event_handler(wimp_w w, win_new_event_handler newproc, void *handle)
{
  if (newproc == 0) {
    win__discard(w);
    return(TRUE);
  } else {
    return(win__register_new(w, newproc, handle, 1 /* for new binaries */));
  }
}

#define win__str WIN__STR

#include "win.c"

extern os_error *
win_drag_box(wimp_dragstr * dr)
{
    WIN__STR *p = win__find(dr->window);

    /* MUST be valid, unlike what PRM doc says, due to changed event handling here */
    myassert1x(p != NULL, "win_drag_box for unregistered window &%p", dr->window);
    if(p)
        win__drag_w = dr->window;

    return(wimp_drag_box(dr));
}

extern void
win_setmenuhi(wimp_w w, wimp_i i, void *handle)
{
    WIN__STR *p;

    if(i == (wimp_i) -1)
        {
        win_setmenuh(w, handle);
        return;
        }

    p = win__find(w);

    if(p)
        {
        p->menuhi_i = i;
        p->menuhi   = handle;
        }
}

extern void *
win_getmenuhi(wimp_w w, wimp_i i) /* 0 if not set */
{
    WIN__STR *p;

    if(i == (wimp_i) -1)
        return(win_getmenuh(w));

    p = win__find(w);

    if(p)
        if(p->menuhi_i == i)
            return(p->menuhi);

    return(NULL);
}

/**********************************************************
*
* register a window as a child of another (parent) window
*
**********************************************************/

extern BOOL
win_register_child(wimp_w parentw, wimp_w w)
{
    WIN__STR *parp = win__find(parentw);
    WIN__STR *chip = win__find(w);
    WIN__CHILD_STR *c;
    wimp_wstate     wstate;
    int             parx, pary;

    tracef2("[win_register_child(child %p to parent %p): ", parentw, w);

    /* one or other window not created via this module? */
    if(!parp || !chip)
        return(FALSE);

    /* already a child of someone else? */
    if((chip->parentw != NULL_WIMP_W)  &&  (chip->parentw != parentw))
        return(FALSE);

    /* search parent's child list to see if it's already there */
    c = (WIN__CHILD_STR *) &parp->children;

    while((c = c->link) != NULL)
        if(c->w == w)
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

            c->w = w;

            chip->parentw = parentw;
        }
    }

    if(c)
    {
        /* note current parent<->child offset */
        wimpt_safe(wimp_get_wind_state(parentw, &wstate));
        parx = wstate.o.box.x0;
        pary = wstate.o.box.y1;
        wimpt_safe(wimp_get_wind_state(w,       &wstate));
        c->x = wstate.o.box.x0 - parx; /* add on to parent pos to recover child pos */
        c->y = wstate.o.box.y1 - pary;
        tracef2("at offset %d,%d from parent]", c->x, c->y);
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
win_deregister_child(wimp_w w)
{
    WIN__STR *parp;
    WIN__STR *chip = win__find(w);
    WIN__CHILD_STR *pp;
    WIN__CHILD_STR *cp;

    tracef1("win_deregister_child(child %p): ", w);

    if(!chip)
        return(FALSE);

    /* not a child? */
    if(chip->parentw == NULL_WIMP_W)
        return(FALSE);

    parp = win__find(chip->parentw);
    if(!parp)
        return(FALSE);

    /* remove parent window ref from child window structure */
    chip->parentw = NULL_WIMP_W;

    /* remove child window from parent's list */
    pp = (WIN__CHILD_STR *) &parp->children;

    while((cp = pp->link) != NULL)
    {
        if(cp->w == w)
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
use win_create_wind rather than wimp_create_wind to get enhanced window handling
*/

extern os_error *
win_create_wind(wimp_wind * wwp, wimp_w * wp, win_new_event_handler proc, void * handle)
{
    os_error * e;

    e = wimp_create_wind(wwp, wp);

    if(!e)
        if(!win__register_new(*wp, proc, handle, 1))
            e = os_set_error(0, msgs_lookup("winT1" /* "Not enough memory to create window" */));

    return(e);
}

/************************************************************
*
* close (but don't delete) this window and all its children
*
************************************************************/

extern os_error *
win_close_wind(wimp_w w)
{
    os_error * e = NULL;
    os_error * e1;
    WIN__STR *p = win__find(w);
    WIN__CHILD_STR *c;

    if(!p)
        return(NULL);

    if(win_submenu_query_is_submenu(w))
    {
        tracef0("closing submenu window ");

        if(!win_submenu_query_closed())
        {
            tracef0("- explicit closure of open window");
            /* window being closed without the menu tree knowing about it */
            /* cause the menu system to close the window */
            event_clear_current_menu();
        }
        else
            tracef0("- already been closed by Wimp");

        win_submenu_w = 0;
    }

    /* is this window a child? */
    if(p->parentw)
        /* remove from parent's list */
        win_deregister_child(w);

    /* loop closing child windows */
    c = (WIN__CHILD_STR *) &p->children;
    while((c = c->link) != NULL)
    {
        e1 = win_close_wind(c->w);
        e  = e ? e : e1;
    }

    /* close window to window manager */
    e1 = wimp_close_wind(w);

    return(e ? e : e1);
}

/************************************************************
*
* delete this window and all its children
*
************************************************************/

extern os_error *
win_delete_wind(wimp_w * wp)
{
    os_error * e;
    os_error * e1;
    WIN__STR *p;
    wimp_w w = *wp;

    if(!w)
        return(NULL);

    *wp = 0;

    /* close internally (closes all children too) */
    e = win_close_wind(w);

    /* delete children */
    p = win__find(w);

    if(p)
    {
        /* is this window a child? */
        if(p->parentw)
            /* remove from parent's list */
            win_deregister_child(w);

        /* loop deleting child windows, restarting each time */
        while(p->children)
        {
            e1 = win_delete_wind(&p->children->w);
            e  = e ? e : e1;
            p = win__find(w);
        }
    }

    win__discard(w);

    /* delete window to window manager */
    e1 = wimp_delete_wind(w);

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
* and that this applies recursively eg to child1.1 and child1.2 stacks
*
* recursive generation of win_open_wind() via wimp_EOPEN of each child
* window is the key to this happening, either by the child window doing
* the win_open_wind itself or by allowing the default event handler to
* do it for it. note that the child windows get events to allow them to
* set extents etc at a suitable place rather than this routine simply
* calling itself directly.
*
*/

extern os_error *
win_open_wind(wimp_openstr * o)
{
    wimp_w           w = o->w;
    os_error * e;
    os_error * e1 = NULL;
    WIN__STR *       p = win__find(w);
    WIN__CHILD_STR * c;
    wimp_openstr     childo;
    wimp_wstate      wstate;
    wimp_w           behind = o->behind;

    tracef6("[win_open_wind(&%p at %d,%d;%d,%d behind &%p]",
            w, o->box.x0, o->box.y0, o->box.x1, o->box.y1, o->behind);

    if(p)
    {
        if(behind != (wimp_w) -2)
        {
            /* not sending to back: bring stack forwards first to minimize redraws */

            /* on mode change, Neil tells us to open windows behind themselves! */
            if((behind == w)  &&  p->children)
            {
                /* see which window the pane stack is really behind */
                wimpt_safe(wimp_get_wind_state(p->children->w, &wstate));
                tracef2("[window &%p pane stack is behind window &%p]", w, wstate.o.behind);
                behind = wstate.o.behind;
            }

            /* loop trying to open child windows at new positions */
            c = (WIN__CHILD_STR *) &p->children;
            while((c = c->link) != NULL)
            {
                childo.w      = c->w;
                childo.box.x0 = o->box.x0 + c->x;
                childo.box.y0 = o->box.y0 + c->y;
                childo.box.x1 = o->box.x1 + c->x;
                childo.box.y1 = o->box.y1 + c->y;
                childo.behind = behind;
                e1 = win_open_wind(&childo);
                behind = c->w; /* open next pane behind this one */
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
            wimpt_safe(wimp_get_wind_state(c->w, &wstate));

            /* just translate to correct place, don't shuffle stack */
            wstate.o.box.x0 = o->box.x0 + c->x;
            wstate.o.box.y0 = o->box.y0 + c->y;
            wstate.o.box.x1 = o->box.x1 + c->x;
            wstate.o.box.y1 = o->box.y1 + c->y;

            e1 = win_open_wind(&wstate.o);

            e = e ? e : e1;
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
win_create_submenu(wimp_w w, int x, int y)
{
    os_error * err;

    /* do we have a different menu tree window up already? if so, close it */
    if((win_submenu_w != 0)  &&  (win_submenu_w != w))
        win_send_close(win_submenu_w);

    err = event_create_submenu((wimp_menustr *) w, x, y);

    if(!err)
        win_submenu_w = w; /* there can be only one */
    else
        win_submenu_w = 0;

    return(err);
}

/* only needed 'cos Wimp is too stupid to realise that submenu creation
 * without a parent menu should be valid
*/

extern os_error *
win_create_menu(wimp_w w, int x, int y)
{
    os_error * err;

    /* do we have a different menu tree window up already? if so, close it */
    if((win_submenu_w != 0)  &&  (win_submenu_w != w))
        win_send_close(win_submenu_w);

    err = event_create_menu((wimp_menustr *) w, x, y);

    if(!err)
        win_submenu_w = w; /* there can be only one */
    else
        win_submenu_w = 0;

    return(err);
}

extern BOOL
win_submenu_query_is_submenu(wimp_w w)
{
    if(w != 0)
        return(w == win_submenu_w);

    return(0);
}

extern BOOL
win_submenu_query_closed(void)
{
    wimp_wstate ws;

    if(win_submenu_w != 0)
        if(!wimpt_complain(wimp_get_wind_state(win_submenu_w, &ws)))
            return((ws.flags & wimp_WOPEN) == 0);

    /* there is none, you fool */
    return(1);
}

/* end of cs-winx.c */
