/* colourpick.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2018 Stuart Swales */

/* RISC OS specific colour picker routines for PipeDream (derived from Fireworkz) */

/* See RISC OS PRM 5a-555 onwards */

#include "common/gflags.h"

#if RISCOS

#include "kernel.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_event_h
#include "cs-event.h"   /* includes event.h -> menu.h, wimp.h */
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef                 __colourpick_h
#include "cmodules/riscos/colourpick.h" /* no includes */
#endif

/*
internal types
*/

enum COLOUR_MODEL
{
    COLOUR_MODEL_RGB  = 0,
    COLOUR_MODEL_CMYK = 1,
    COLOUR_MODEL_HSV  = 2
};

typedef struct COLOUR_DESCRIPTOR
{
    /* 0*/  U8 zero;
    /* 1*/  U8 red;
    /* 2*/  U8 green;
    /* 3*/  U8 blue;

    /* 4*/  U32 extra_bytes; /* 0 or 4+3*4 for RGB/HSV, 4+4*4 for CMYK */

    /* 8*/  enum COLOUR_MODEL colour_model;

    /*12*/  
    union COLOUR_MODEL_DATA
    {
        struct COLOUR_MODEL_DATA_RGB
        {
            /* 0*/  U32 red;
            /* 4*/  U32 green;
            /* 8*/  U32 blue;
        } rgb;

        struct COLOUR_MODEL_DATA_CMYK
        {
            /* 0*/  U32 cyan;
            /* 4*/  U32 magenta;
            /* 8*/  U32 yellow;
            /*12*/  U32 key;
        } cmyk;

        struct COLOUR_MODEL_DATA_HSV
        {
            /* 0*/  U32 hue;
            /* 4*/  U32 saturation;
            /* 8*/  U32 value;
        } hsv;
    } data;
}
COLOUR_DESCRIPTOR, * P_COLOUR_DESCRIPTOR; typedef const COLOUR_DESCRIPTOR * PC_COLOUR_DESCRIPTOR;

typedef struct COLOUR_PICKER_BLOCK
{
    /* 0*/  int flags;
    /* 4*/  const char * title;
    /* 8*/  int tl_x;
    /*12*/  U32 reserved_80000000;
    /*16*/  U32 reserved_7FFFFFFF;
    /*20*/  int tl_y;
    /*24*/  int reserved_0[2];
    /*32*/  COLOUR_DESCRIPTOR colour_descriptor;
}
COLOUR_PICKER_BLOCK, * P_COLOUR_PICKER_BLOCK;

static COLOUR_PICKER_BLOCK colour_picker_block;

typedef struct COLOURPICKER_CALLBACK
{
    enum COLOUR_PICKER_TYPE colour_picker_type;
    HOST_WND parent_window_handle;
    RGB rgb;
    BOOL chosen;

    int dialogue_handle;
    HOST_WND window_handle;
}
COLOURPICKER_CALLBACK, * P_COLOURPICKER_CALLBACK;

static void
colourpicker_close_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback);

/*
mostly no processing to do in this event handler - all are handled by ColourPicker module using Wimp filters
*/

/* this one is an exception - event being issued by winx layers when we are a child and our parent is closing */

_Check_return_
static int
colourpicker_event_close_window(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback,
    _In_        const WimpCloseWindowRequestEvent * const p_close_window_request)
{
    UNREFERENCED_PARAMETER_InRef_(p_close_window_request);

    colourpicker_close_dialogue(p_colourpicker_callback);

    return(TRUE /*processed*/);
}

_Check_return_
static int
colourpicker_event_handler(
    wimp_eventstr * event_str,
    void *handle)
{
    const int event_code = (int) event_str->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &event_str->data;

    reportf(/*trace_3(TRACE_RISCOS_HOST,*/ TEXT("%s: %s handle=") PTR_XTFMT, __func__, report_wimp_event(event_code, event_data), handle);

    switch(event_code)
    {
    case Wimp_ECloseWindow:
        return(colourpicker_event_close_window((P_COLOURPICKER_CALLBACK) handle, &event_data->close_window_request));

    default:
        return(TRUE /*processed*/); /* all are handled by ColourPicker module using Wimp filters */
    }
}

#define Wimp_MColourPickerColourChoice         0x47700
#define Wimp_MColourPickerColourChanged        0x47701
#define Wimp_MColourPickerCloseDialogueRequest 0x47702
#define Wimp_MColourPickerOpenParentRequest    0x47703
#define Wimp_MColourPickerResetColourRequest   0x47704

static BOOL
colourpicker_message_ColourPickerColourChoice(
    _In_        const WimpMessage * const p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    const PC_COLOUR_DESCRIPTOR p_colour_descriptor = (PC_COLOUR_DESCRIPTOR) &p_wimp_message->data.words[2];

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    p_colourpicker_callback->rgb.transparent = u8_from_int(0 != (p_wimp_message->data.words[1] & 1));

    p_colourpicker_callback->rgb.r = p_colour_descriptor->red;
    p_colourpicker_callback->rgb.g = p_colour_descriptor->green;
    p_colourpicker_callback->rgb.b = p_colour_descriptor->blue;

    p_colourpicker_callback->chosen = TRUE;

    return(TRUE);
}

static BOOL
colourpicker_message_ColourPickerCloseDialogueRequest(
    _In_        const WimpMessage * const p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    UNREFERENCED_PARAMETER_InRef_(p_wimp_message);

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    colourpicker_close_dialogue(p_colourpicker_callback);

    return(TRUE);
}

static BOOL
colourpicker_message_ColourPickerResetColourRequest(
    _In_        const WimpMessage * const p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    UNREFERENCED_PARAMETER_InRef_(p_wimp_message);

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    {
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 0
        | (1 << 6) /* update from RGB triplet */;
    rs.r[1] = p_colourpicker_callback->dialogue_handle;
    rs.r[2] = (int) &colour_picker_block;
    e = WrapOsErrorReporting(_kernel_swi(0x47704 /*ColourPicker_UpdateDialogue*/, &rs, &rs));
    } /*block*/

    return(TRUE);
}

static BOOL
colourpicker_message_MenusDeleted(
    _In_        const WimpMessage * const p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    UNREFERENCED_PARAMETER_InRef_(p_wimp_message);

    /* not a ColourPicker message, so first word is not dialogue_handle */

    colourpicker_close_dialogue(p_colourpicker_callback);

    return(TRUE);
}

/* the list of messages that we are specifically interested in */

static const int
colourpicker_wimp_messages[] =
{
    Wimp_MColourPickerColourChoice,
    Wimp_MColourPickerCloseDialogueRequest,
    Wimp_MColourPickerResetColourRequest,

  /*Wimp_MMenusDeleted,*/ /* already requested by PipeDream */

    0
};

static BOOL
colourpicker_message(
    _In_        const WimpMessage * const p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MColourPickerColourChoice:
        return(colourpicker_message_ColourPickerColourChoice(p_wimp_message, p_colourpicker_callback));

    case Wimp_MColourPickerCloseDialogueRequest:
        return(colourpicker_message_ColourPickerCloseDialogueRequest(p_wimp_message, p_colourpicker_callback));

    case Wimp_MColourPickerResetColourRequest:
        return(colourpicker_message_ColourPickerResetColourRequest(p_wimp_message, p_colourpicker_callback));

    case Wimp_MMenusDeleted:
        return(colourpicker_message_MenusDeleted(p_wimp_message, p_colourpicker_callback));

    default:
        return(FALSE); /* we don't want it - pass it on to the real handler */
    }
}

static BOOL
colourpicker_message_filter(
    wimp_eventstr * event_str,
    void * client_handle)
{
    const int event_code = event_str->e;
    const WimpPollBlock * const event_data = (WimpPollBlock *) &event_str->data;

    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        reportf(/*trace_2(TRACE_RISCOS_HOST,*/ TEXT("%s: %s"), __func__, report_wimp_event(event_code, event_data));
        return(colourpicker_message(&event_data->user_message, (P_COLOURPICKER_CALLBACK) client_handle));

    default:
        return(FALSE); /* we don't want it - pass it on to the real handler */
    }
}

static BOOL
colourpicker_open_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) p_colourpicker_callback->colour_picker_type;
    rs.r[1] = (int) &colour_picker_block;

    if(NULL != (e = WrapOsErrorReporting(_kernel_swi(0x47702 /*ColourPicker_OpenDialogue*/, &rs, &rs))))
    {
        p_colourpicker_callback->dialogue_handle = 0;
        p_colourpicker_callback->window_handle = 0;
        return(FALSE);
    }

    p_colourpicker_callback->dialogue_handle = rs.r[0];
    p_colourpicker_callback->window_handle = rs.r[1];

    reportf("got colourpicker window_handle " UINTPTR_XTFMT, p_colourpicker_callback->window_handle);
    switch(p_colourpicker_callback->colour_picker_type)
    {
        case COLOUR_PICKER_NORMAL:
             winx_register_new_event_handler(p_colourpicker_callback->window_handle, colourpicker_event_handler, p_colourpicker_callback);

            if(HOST_WND_NONE != p_colourpicker_callback->parent_window_handle)
                if(!winx_register_child(p_colourpicker_callback->parent_window_handle, p_colourpicker_callback->window_handle))
                    assert0();
            break;

        default:
            break;
    }

    return(TRUE);
}

static BOOL
colourpicker_is_open(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    return(p_colourpicker_callback->window_handle > 0);
}

static void
host_message_filter_add(
    const int * const wimp_messages)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) wimp_messages;
    e = _kernel_swi(0x400F6 /*Wimp_AddMessages*/, &rs, &rs);
}

static void
host_message_filter_remove(
    const int * const wimp_messages)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) wimp_messages;
    e = _kernel_swi(0x400F7 /*Wimp_RemoveMessages*/, &rs, &rs);
}

static void
colourpicker_process_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    reportf("colourpicker_process_dialogue - in");
    win_add_unknown_event_processor(colourpicker_message_filter, (void *) p_colourpicker_callback);

    host_message_filter_add(colourpicker_wimp_messages);

    do { event_process(); } while(colourpicker_is_open(p_colourpicker_callback));

    host_message_filter_remove(colourpicker_wimp_messages);

    win_remove_unknown_event_processor(colourpicker_message_filter, (void *) p_colourpicker_callback);
    reportf("colourpicker_process_dialogue - out");
}

static void
colourpicker_close_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    reportf("colourpicker_close_dialogue");

    if(p_colourpicker_callback->dialogue_handle)
    {
        switch(p_colourpicker_callback->colour_picker_type)
        {
            case COLOUR_PICKER_NORMAL:
                consume_bool(winx_deregister_child(p_colourpicker_callback->window_handle));

                {
                _kernel_swi_regs rs;
                _kernel_oserror * e;
                rs.r[0] = 0;
                rs.r[1] = p_colourpicker_callback->dialogue_handle;
                e = WrapOsErrorReporting(_kernel_swi(0x47703 /*ColourPicker_CloseDialogue*/, &rs, &rs));
                } /*block*/

                break;

            default:
                break;
        }

        p_colourpicker_callback->dialogue_handle = 0;
    }

    if(p_colourpicker_callback->window_handle > 0)
    {
        /* window will actually have been destroyed in the ColourPicker_CloseDialogue SWI */
        p_colourpicker_callback->window_handle = 0;
    }
}

extern BOOL
riscos_colour_picker(
    _InVal_     enum COLOUR_PICKER_TYPE colour_picker_type,
    _HwndRef_   HOST_WND parent_window_handle,
    _InoutRef_  P_RGB p_rgb,
    _InVal_     BOOL allow_transparent,
    _In_z_      PC_U8Z p_title)
{
    COLOURPICKER_CALLBACK colourpicker_callback;
    P_COLOURPICKER_CALLBACK p_colourpicker_callback = &colourpicker_callback;

    zero_struct(colourpicker_callback);
    colourpicker_callback.colour_picker_type = colour_picker_type;
    colourpicker_callback.parent_window_handle = parent_window_handle;
    colourpicker_callback.rgb = *p_rgb;
    colourpicker_callback.chosen = FALSE;

colour_picker_block.flags = 0;
if(allow_transparent)
{
    colour_picker_block.flags |= (1 << 0);
    if(p_rgb->transparent)
        colour_picker_block.flags |= (1 << 1);
}
/*colour_picker_block.flags |= (1 << 4);*/ /* ignore Message_HelpRequest */

colour_picker_block.title = p_title;

    if(HOST_WND_NONE != parent_window_handle)
    {   /* obtain current parent window position */
        WimpGetWindowStateBlock window_state;
        int parent_x, parent_y;

        window_state.window_handle = parent_window_handle;
        void_WrapOsErrorReporting(tbl_wimp_get_window_state(&window_state));
        parent_x = window_state.visible_area.xmin;
        parent_y = window_state.visible_area.ymax;

colour_picker_block.tl_x = parent_x + 108;
colour_picker_block.tl_y = parent_y - 64;
    }
    else
    {
        int m_x, m_y;
        event_read_submenupos(&m_x, &m_y);

colour_picker_block.tl_x = m_x;
colour_picker_block.tl_y = m_y;
    }
colour_picker_block.reserved_80000000 = 0x80000000U;
colour_picker_block.reserved_7FFFFFFF = 0x7FFFFFFFU;

colour_picker_block.reserved_0[0] = 0;
colour_picker_block.reserved_0[1] = 0;

colour_picker_block.colour_descriptor.zero = 0;
colour_picker_block.colour_descriptor.red = p_colourpicker_callback->rgb.r;
colour_picker_block.colour_descriptor.green = p_colourpicker_callback->rgb.g;
colour_picker_block.colour_descriptor.blue = p_colourpicker_callback->rgb.b;
colour_picker_block.colour_descriptor.extra_bytes = 0;
colour_picker_block.colour_descriptor.colour_model = COLOUR_MODEL_RGB;

    if(!colourpicker_open_dialogue(p_colourpicker_callback))
        return(FALSE);

    colourpicker_process_dialogue(p_colourpicker_callback);

    colourpicker_close_dialogue(p_colourpicker_callback);

reportf("RGB out: %d,%d,%d(%d)", colourpicker_callback.rgb.r, colourpicker_callback.rgb.g, colourpicker_callback.rgb.b, colourpicker_callback.rgb.transparent);
    *p_rgb = colourpicker_callback.rgb;
    return(colourpicker_callback.chosen);
}

#endif /* RISCOS */

/* end of colourpick.c */
