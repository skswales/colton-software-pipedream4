/* cs-winf.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "include.h"

/* field handling - for general windows not just dboxes */

extern BOOL
winf_adjustbumphit(int * p_hit_icon_handle /*inout*/, int value_icon_handle)
{
    int hit_icon_handle = *p_hit_icon_handle;
    const int dec_icon_handle = value_icon_handle - 1;
    const int inc_icon_handle = dec_icon_handle - 1;
    BOOL res = ((hit_icon_handle == inc_icon_handle)  ||  (hit_icon_handle == dec_icon_handle));

    if(res  &&  winx_adjustclicked())
        *p_hit_icon_handle = (hit_icon_handle ^ (inc_icon_handle ^ dec_icon_handle));

    return(res);
}

extern void
winf_changedfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = window_handle;
    set_icon_state_block.icon_handle = icon_handle;
    set_icon_state_block.EOR_word = 0; /* poke it for redraw */
    set_icon_state_block.clear_word = 0;
    (void) wimpt_complain(tbl_wimp_set_icon_state(&set_icon_state_block));
}

extern void
winf_getfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    char *buffer /*out*/,
    size_t size)
{
    wimp_icon    info;
    size_t       j;
    const char * from;
    const char * ptr;

    tracef4("[winf_getfield(&%p, %u, &%p, %d): ", (void *) window_handle, i, buffer, size);
    assert(size != 0); /* places at least CH_NULL */

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
    {
        j = 0;  /* returns "" */
    }
    else if(0 == (info.flags & wimp_ITEXT))
    {
        tracef0("- non-text icon: ");
        j = 0;  /* returns "" */
    }
    else
    {
        myassert2x(info.flags & wimp_INDIRECT, "winf_getfield: window &%p icon %u has inline buffer", (void *) window_handle, (void *) icon_handle);

        from = info.data.indirecttext.buffer;
        ptr  = from;
        while(*ptr++ >= 32)
            ;
        j = ptr - from - 1;
        j = min(j, size - 1);
        memcpy(buffer, from, j);
    }

    buffer[j] = CH_NULL;
    tracef1(" returns \"%s\"]", buffer);
}

extern void
winf_setfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    _In_z_      const char * value)
{
    wimp_icon info;
    WimpCaret caret;
    int newlen, oldlen, maxlen;

    tracef3("[winf_setfield(&%p, %u, \"%s\"): ", (void *) window_handle, icon_handle, value);

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
        return;

    if(0 == (info.flags & wimp_ITEXT))
    {
        tracef0(" non-text icon - ignored");
        return;
    }

    myassert2x(info.flags & wimp_INDIRECT, "winf_setfield: window &%p icon %u has inline buffer", (void *) window_handle, icon_handle);

    if(0 == strcmp(info.data.indirecttext.buffer, value))
        return;

    maxlen = info.data.indirecttext.bufflen - 1;  /* MUST be term */
    oldlen = strlen(info.data.indirecttext.buffer);
    info.data.indirecttext.buffer[0] = CH_NULL;
    strncat(info.data.indirecttext.buffer, value, maxlen);
    tracef1(" indirect buffer now \"%s\"]", info.data.indirecttext.buffer);
    newlen = strlen(info.data.indirecttext.buffer);

    /* ensure that the caret moves correctly if it's in this icon */
    if(wimpt_complain(tbl_wimp_get_caret_position(&caret)))
        return;
    tracef2("[caret in window &%p, icon %u]", (void *) caret.window_handle, caret.icon_handle);

    if((caret.window_handle == window_handle)  &&  (caret.icon_handle == icon_handle))
    {
        if(caret.index == oldlen)
            caret.index = newlen;       /* if grown and caret was at end,
                                         * move caret right */
        if(caret.index > newlen)
            caret.index = newlen;       /* if shorter, bring caret left */

        caret.height = -1;   /* calc x,y,h from icon/index */

        (void) wimpt_complain(
                    tbl_wimp_set_caret_position(caret.window_handle, caret.icon_handle,
                                                caret.xoffset, caret.yoffset,
                                                caret.height, caret.index));
    }

    /* prod it, to cause redraw */
    (void) wimpt_complain(
                wimp_set_icon_state(window_handle, icon_handle,
                                    /* EOR */ (wimp_iconflags) 0,
                                    /* BIC */ wimp_ISELECTED));
}

extern void
winf_fadefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    wimp_icon info;

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
        return;

    if(0 == (info.flags & wimp_INOSELECT))
        /* prod it, to cause redraw */
        (void) wimpt_complain(
                    wimp_set_icon_state(window_handle, icon_handle,
                                        /* EOR */ wimp_INOSELECT,
                                        /* BIC */ (wimp_iconflags) 0));
}

extern void
winf_unfadefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    wimp_icon info;

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
        return;

    if(0 != (info.flags & wimp_INOSELECT))
        /* prod it, to cause redraw */
        (void) wimpt_complain(
                    wimp_set_icon_state(window_handle, icon_handle,
                                        /* EOR */ wimp_INOSELECT,
                                        /* BIC */ (wimp_iconflags) 0));
}

/* note that unhide is rather harder - would need a saved state to restore */

extern void
winf_hidefield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    (void) wimpt_complain(
                wimp_set_icon_state(window_handle, icon_handle,
                                    /*EOR*/ (wimp_iconflags) 0,
                                    /*BIC*/ (wimp_iconflags) (wimp_ITEXT | wimp_ISPRITE | wimp_IBORDER | wimp_IFILLED | wimp_IBUTMASK)));
}

extern void
winf_invertfield(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    wimp_icon icon;

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &icon)))
        return;

    (void) wimpt_complain(
                wimp_set_icon_state(window_handle, icon_handle,
                                    /* EOR */ wimp_ISELECTED,
                                    /* BIC */ (icon.flags & wimp_ISELECTED) ? (wimp_iconflags) 0 : wimp_ISELECTED));
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
winf_bumpdouble(
    _HwndRef_   HOST_WND window_handle,
    int hit_icon_handle,
    int value_icon_handle,
    double * dvar /*inout*/,
    const double * ddelta,
    const double * dmin,
    const double * dmax,
    int            decplaces)
{
    const int dec_icon_handle = value_icon_handle - 1;
    double try_ddelta, dval;
    BOOL res = winf_adjustbumphit(&hit_icon_handle, value_icon_handle);

    if(res)
    {
        try_ddelta = *ddelta;

        if(hit_icon_handle == dec_icon_handle)
            try_ddelta = 0.0 - try_ddelta;

        dval = winf_getdouble(window_handle, value_icon_handle, dvar, decplaces);

        dval += try_ddelta;

        if( dval > *dmax)
            dval = *dmax;
        else if(dval < *dmin)
            dval = *dmin;

        *dvar = dval;

        winf_setdouble(window_handle, value_icon_handle, dvar, decplaces);
    }

    return(res);
}

extern void
winf_checkdouble(
    _HwndRef_   HOST_WND window_handle,
    int value_icon_handle,
    double * dvar /*inout*/,
    int * modify /*out-if-mod*/,
    const double * dmin,
    const double * dmax,
    int            decplaces)
{
    double dval = winf_getdouble(window_handle, value_icon_handle, dvar, decplaces);

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
winf_getdouble(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    const double * default_val,
    int            decplaces)
{
    char   buffer[64];
    char * ptr;
    double val;

    winf_getfield(window_handle, icon_handle, buffer, sizeof(buffer));

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
winf_setdouble(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    const double * val,
    int decplaces)
{
    char buffer[32];
    double dval;

    if(decplaces != INT_MAX)
        dval = round_to_decplaces(val, decplaces);
    else
        dval = *val;

    consume_int(sprintf(buffer, "%g", dval));

    winf_setfield(window_handle, icon_handle, buffer);
}

/*
int field handling
*/

extern BOOL
winf_bumpint(
    _HwndRef_   HOST_WND window_handle,
    int hit_icon_handle,
    int value_icon_handle,
    int * ivar /*inout*/,
    int idelta, int imin, int imax)
{
    const int dec_icon_handle = value_icon_handle - 1;
    int try_idelta, ival;
    BOOL res = winf_adjustbumphit(&hit_icon_handle, value_icon_handle);

    if(res)
    {
        try_idelta = idelta;

        if(hit_icon_handle == dec_icon_handle)
            try_idelta = 0 - try_idelta;

        ival = winf_getint(window_handle, value_icon_handle, *ivar);

        ival += try_idelta;

        if( ival > imax)
            ival = imax;
        else if(ival < imin)
            ival = imin;

        *ivar = ival;

        winf_setint(window_handle, value_icon_handle, *ivar);
    }

    return(res);
}

extern void
winf_checkint(
    _HwndRef_   HOST_WND window_handle,
    int value_icon_handle,
    int * ivar /*inout*/,
    int * modify /*out-if-mod*/,
    int imin, int imax)
{
    int ival = winf_getint(window_handle, value_icon_handle, *ivar);

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
winf_getint(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    int default_val)
{
    char   buffer[32];
    char * ptr;
    int    val;

    winf_getfield(window_handle, icon_handle, buffer, sizeof(buffer));

    val = (int) strtol(buffer, &ptr, 0);

    if(ptr == buffer)
        return(default_val);

    return(val);
}

extern void
winf_setint(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    int val)
{
    char buffer[16];

    consume_int(sprintf(buffer, "%d", val));

    winf_setfield(window_handle, icon_handle, buffer);
}

/*
on/off handling
*/

/*
* returns TRUE if icon selected
*/

extern BOOL
winf_getonoff(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle)
{
    wimp_icon info;

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
        return(FALSE);

    return((info.flags & wimp_ISELECTED) == wimp_ISELECTED);
}

extern void
winf_setonoff(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    BOOL on)
{
    wimp_icon info;
    int cur_state, req_state;

    if(wimpt_complain(wimp_get_icon_info(window_handle, icon_handle, &info)))
        return;

    cur_state = (info.flags & wimp_ISELECTED);
    req_state = (on ? wimp_ISELECTED : 0);

    if(cur_state == req_state)
        return;

    (void) wimpt_complain(
                wimp_set_icon_state(window_handle, icon_handle,
                                    /* EOR */ (wimp_iconflags) req_state,
                                    /* BIC */ wimp_ISELECTED));
}

extern void
winf_setonoffpair(
    _HwndRef_   HOST_WND window_handle,
    int icon_handle,
    BOOL on)
{
    winf_setonoff(window_handle, icon_handle,      on);
    winf_setonoff(window_handle, icon_handle + 1, !on);
}

extern int /*icon_handle*/
winf_whichonoff(
    _HwndRef_   HOST_WND window_handle,
    int first_icon_handle,
    int last_icon_handle,
    int dft_icon_handle)
{
    int icon_handle;

    for(icon_handle = last_icon_handle; icon_handle >= first_icon_handle; --icon_handle)
    {
        BOOL on = winf_getonoff(window_handle, icon_handle);

        if(on)
            break;

        if(icon_handle == first_icon_handle)
            return(dft_icon_handle);
    }

    return(icon_handle);
}

/* suss a pair of radio buttons, first being default too */

extern int /*icon_handle*/
winf_whichonoffpair(
    _HwndRef_   HOST_WND window_handle,
    int first_icon_handle)
{
    return(winf_whichonoff(window_handle, first_icon_handle, first_icon_handle + 1, first_icon_handle));
}

/* end of cs-winf.c */
