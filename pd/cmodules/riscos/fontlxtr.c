/* fontlxtr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

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

extern void
wm_events_get(
    BOOL arg);

/******************************************************************************
*
* cause the fontlist to be made up-to-date if changes detected
*
******************************************************************************/

static char fontlxtr__path[256]   = "";
static char fontlxtr__prefix[256] = "";
static BOOL fontlxtr__select_init = FALSE;

static BOOL
fontlxtr__ensure_test(
    BOOL * ensure /*inout*/)
{
    char var_buffer[256];

    if(!font__tree)
        *ensure = 1;

    if(NULL != _kernel_getenv("Font$Path", var_buffer, elemof(var_buffer)))
        var_buffer[0] = CH_NULL;

    if(0 != strcmp(fontlxtr__path, var_buffer))
    {
        strcpy(fontlxtr__path, var_buffer);
        *ensure = 1;
    }

    if(NULL != _kernel_getenv("Font$Prefix", var_buffer, elemof(var_buffer)))
        var_buffer[0] = CH_NULL;

    if(0 != strcmp(fontlxtr__prefix, var_buffer))
    {
        strcpy(fontlxtr__prefix, var_buffer);
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
    /*inout*/ HOST_WND * p_window_handle)
{
    if(*p_window_handle)
    {
        WimpGetWindowStateBlock window_state;
        if(NULL == tbl_wimp_get_window_state_x(*p_window_handle, &window_state))
            if((window_state.flags & WimpWindow_Open) != 0)
                return(TRUE);

        fontselect_closewindows();
        *p_window_handle = HOST_WND_NONE;
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
    HOST_WND window_handle;

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
    case Wimp_EMouseClick:
        {
        const WimpMouseClickEvent * const mouse_click = (const WimpMouseClickEvent *) &e->data;

        switch(mouse_click->icon_handle)
        {
        case 0: /* OK */
            close_windows = (mouse_click->buttons != Wimp_MouseButtonAdjust);
            try_anyway = TRUE;
            break;

        case 1: /* CANCEL */
            close_windows = TRUE;
            processed = TRUE;
            break;

        default:
            break;
        }

        break;
        }

    case Wimp_EKeyPressed:
        {
        const WimpKeyPressedEvent * const key_pressed = (const WimpKeyPressedEvent *) &e->data;

        switch(key_pressed->key_code)
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
        }

    default:
        break;
    }

    if(!processed)
        /* send the uk event anyway if not already done so */
        processed = (* fontselect.try_fn) (font_name, width, height, e, fontselect.try_handle, try_anyway);

    if(!processed && (e->e == Wimp_EKeyPressed))
        /* send it on */
        void_WrapOsErrorReporting(wimp_processkey(e->data.key.chcode));

    if(close_windows  &&  (fontselect.state == FONTSELECT_WAIT))
        fontselect.state = FONTSELECT_ENDED;

    return(close_windows);
}

extern BOOL
fontselect_can_process(void)
{
    return(fontselect.window_handle == HOST_WND_NONE);
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
    if(fontselect.window_handle != HOST_WND_NONE)
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
    _InVal_     F64 width,
    _InVal_     F64 height,
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

    fontselect.window_handle = (HOST_WND)
        fontselect_selector((char *) title,                   /* win title   */
                            flags,                            /* flags       */
                            (char *) font_name,               /* font name   */
                            width,                            /* font width  */
                            height,                           /* font height */
                            fontselect_xtra_unknown_fn);

    fontselect.state = (fontselect.window_handle != HOST_WND_NONE)
                               ? FONTSELECT_WAIT
                               : FONTSELECT_ENDED;

    if(init_fn)
        (*init_fn)(fontselect.window_handle, init_handle);

    /* wait for process to complete. user process will suffer upcalls from wm_events_get() */
    while(fontselect.state == FONTSELECT_WAIT)
    {
        if(!fontselect_check_open(&fontselect.window_handle))
            fontselect.state = FONTSELECT_ENDED;
        else
            wm_events_get(TRUE);
    }

    /* clear the interlock */
    fontselect.window_handle = HOST_WND_NONE;

    return(1);
}

extern S32
fontlist_enumerate(
    BOOL              system,
    fontlist_node *   fontlist,
    fontlist_enumproc enumproc,
    P_ANY             enumhandle)
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
                if(system || (0 != C_stricmp(szFontName, "System")))
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
