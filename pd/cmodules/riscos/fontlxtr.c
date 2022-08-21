/* fontlxtr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* More fontlist stuff - complementary to RISC_OSLib:fontlist.c */

#include "common/gflags.h"

#include "cmodules/riscos/fontlxtr.h"

#ifndef __cs_event_h
#include "cs-event.h"
#endif

#ifndef __menu_h
#include "menu.h"
#endif

#ifndef __fontselect_h
#include "fontselect.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

extern int
_stricmp(const char * a, const char * b);

extern void
wm_events_get(
    BOOL arg);

/******************************************************************************
*
* cause the fontlist to be made up-to-date
*
******************************************************************************/

static char fontlxtr__path[256]   = "";
static char fontlxtr__prefix[256] = "";
static BOOL fontlxtr__select_init = FALSE;

static BOOL
fontlxtr__ensure_test(
    BOOL * ensure /*inout*/)
{
    char * new_font_var;

    if(!font__tree)
        *ensure = 1;

    new_font_var = getenv("Font$Path");

    if(!new_font_var)
    {
        fontlxtr__path[0] = '\0';
        *ensure = 1;
    }
    else if(0 != strcmp(fontlxtr__path, new_font_var))
    {
        strcpy(fontlxtr__path, new_font_var);
        *ensure = 1;
    }

    new_font_var = getenv("Font$Prefix");

    if(!new_font_var)
    {
        fontlxtr__prefix[0] = '\0';
        *ensure = 1;
    }
    else if(0 != strcmp(fontlxtr__prefix, new_font_var))
    {
        strcpy(fontlxtr__prefix, new_font_var);
        *ensure = 1;
    }

    return(*ensure);
}

extern fontlist_node *
fontlist_ensure_all_fonts(
    BOOL * ensure /*inout*/)
{
    if(fontlxtr__ensure_test(ensure))
        if(font__tree)
            fontlist_free_font_tree(font__tree);

    return(*ensure ? fontlist_list_all_fonts(TRUE /* force System */) : font__tree);
}

extern BOOL
fontselect_ensure_all_fonts(void)
{
    BOOL ensure = FALSE;

    if(fontlxtr__ensure_test(&ensure) || !fontlxtr__select_init)
    {
        if(fontlxtr__select_init)
            fontselect_closedown();

        fontlxtr__select_init = fontselect_init();
    }

    return(fontlxtr__select_init);
}

extern BOOL
fontselect_check_open(
    wimp_w * w /*inout*/)
{
    wimp_wstate ws;

    if(*w)
    {
        if(!wimp_get_wind_state(*w, &ws))
            if((ws.flags & wimp_WOPEN) != 0)
                return(TRUE);

        fontselect_closewindows();
        *w = 0;
    }

    return(FALSE);
}

typedef enum FONTSELECT_STATES
{
    FONTSELECT_WAIT = 0,
    FONTSELECT_ENDED
}
FONTSELECT_STATES;

static struct FONTSELECT
{
    wimp_w            w;
    FONTSELECT_STATES state;

    FONTSELECT_TRY_FN try_fn;
    void *            try_handle;
}
fontselect;

static BOOL
fontselect_xtra_unknown_fn(
    char * font_name,
    F64 width,
    F64 height,
    wimp_eventstr * e)
{
    /* pitiful function doesn't get passed a handle so that's tough! */
    BOOL processed     = FALSE;
    BOOL close_windows = FALSE;
    BOOL try_anyway    = FALSE;

    switch(e->e)
    {
    case wimp_EBUT:
        switch(e->data.but.m.i)
        {
        case 0:
            /* OK */
            close_windows = (e->data.but.m.bbits != wimp_BRIGHT);
            try_anyway = TRUE;
            break;

        case 1:
            /* CANCEL */
            close_windows = TRUE;
            processed = TRUE;
            break;

        default:
            break;
        }
        break;

    case wimp_EKEY:
        switch(e->data.key.chcode)
        {
        case 13:
            /* Return */
            close_windows = TRUE;
            try_anyway = TRUE;
            break;

        case 27:
            /* Esc */
            close_windows = TRUE;
            processed = TRUE;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if(!processed)
        /* send the uk event anyway if not already done so */
        processed = (* fontselect.try_fn) (font_name, &width, &height, e, fontselect.try_handle, try_anyway);

    if(!processed && (e->e == wimp_EKEY))
        /* send it on */
        wimpt_complain(wimp_processkey(e->data.key.chcode));

    if(close_windows  &&  (fontselect.state == FONTSELECT_WAIT))
        fontselect.state = FONTSELECT_ENDED;

    return(close_windows);
}

extern BOOL
fontselect_can_process(void)
{
    return(fontselect.w == (wimp_w) 0);
}

extern BOOL
fontselect_prepare_process(void)
{
    /* this ***nasty*** piece of code relies on the first level monitoring of
     * the state of the font selector returning without further ado to its caller
     * and several milliseconds later the Window Manager will issue yet another
     * submenu open request. or so we hope. if the punter is miles too quick,
     * it will merely close the old font selector and not open a new one, so
     * he'll have to go and move over the right arrow again!
    */
    if(fontselect.w != (wimp_w) 0)
    {
        /* can't requeue it as it'd come through again instantly without the below process loop terminating */
        fontselect_closewindows();
        fontselect.state = FONTSELECT_ENDED;
        return(0);
    }

    return(1);
}

extern BOOL
fontselect_process(
    const char * title,
    int flags,
    const char * font_name,
    PC_F64 width,
    PC_F64 height,
    FONTSELECT_INIT_FN init_fn,
    void * init_handle,
    FONTSELECT_TRY_FN try_fn,
    void * try_handle)
{
    if(!fontselect_prepare_process())
        return(0);

    if(!fontselect_ensure_all_fonts())
        return(0);

    fontselect.try_fn     = try_fn;
    fontselect.try_handle = try_handle;

    fontselect.w = (wimp_w)
        fontselect_selector((char *) title,                   /* win title   */
                            flags,                            /* flags       */
                            (char *) font_name,               /* font_name   */
                            *width,                           /* font width  */
                            *height,                          /* font height */
                            fontselect_xtra_unknown_fn);

    fontselect.state = (fontselect.w != 0)
                               ? FONTSELECT_WAIT
                               : FONTSELECT_ENDED;

    if(init_fn)
        (*init_fn)(fontselect.w, init_handle);

    /* wait for process to complete. user process will suffer upcalls from wm_events_get() */
    while(fontselect.state == FONTSELECT_WAIT)
    {
        if(!fontselect_check_open(&fontselect.w))
            fontselect.state = FONTSELECT_ENDED;
        else
            wm_events_get(TRUE);
    }

    /* clear the interlock */
    fontselect.w = (wimp_w) 0;

    return(1);
}

extern S32
fontlist_enumerate(
    BOOL              system,
    fontlist_node *   fontlist,
    fontlist_enumproc enumproc,
    P_ANY              enumhandle)
{
    char            szFontName[256];
    S32             seqno = 0;
    fontlist_node * typeface;

    /* loop over typefaces */
    typeface = fontlist;

    if(typeface)
        do  {
            U32 weight_offset;
            fontlist_node * weight;

            xstrkpy(szFontName, elemof32(szFontName), typeface->name);

            weight_offset = strlen(szFontName);

            /* loop over weights */
            weight = typeface->son;

            if( !weight        /* weightless font */                ||
                typeface->flag /* weighted and weightless in one */ )
            {
                /* weightless font - may be asked to ignore weightless System font */
                if(system || (0 != _stricmp(szFontName, "System")))
                    (* enumproc) (enumhandle, szFontName, ++seqno);
            }

            if(weight)
                do  {
                    U32 style_offset;
                    fontlist_node * style;

                    xstrkpy(szFontName + weight_offset, elemof32(szFontName) - weight_offset, ".");
                    xstrkat(szFontName + weight_offset, elemof32(szFontName) - weight_offset, weight->name);

                    style_offset = strlen(szFontName);

                    /* loop over styles - no further recursion needed */
                    style = weight->son;

                    if( !style       /* styleless but weighted font */ ||
                        weight->flag /*styles and styleless in one*/   )
                        (* enumproc) (enumhandle, szFontName, ++seqno);

                    if(style)
                        do  {
                            xstrkpy(szFontName + style_offset, elemof32(szFontName) - style_offset, ".");
                            xstrkat(szFontName + style_offset, elemof32(szFontName) - style_offset, style->name);

                            (* enumproc) (enumhandle, szFontName, ++seqno);

                            style = style->brother;
                        }
                        while(style);

                    weight = weight->brother;
                }
                while(weight);

            typeface = typeface->brother;
        }
        while(typeface);

    return(seqno);
}

/* end of fontlxtr.c */
