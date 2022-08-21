/* cs-wimp.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_wimp_h
#define __cs_wimp_h

/* The main purpose of this file is to try to force the
 * inclusion of ACW's patched wimp.h rather than tboxlibs' wimp.h
 * but we also define PipeDream specific messages here
 */

typedef enum WIMP_MSGPD_DDE_ID {            /* ddetype */

/* PD tramsmits */
    Wimp_MPD_DDE_SendSlotContents = 1,      /* C, no reply expected */
    Wimp_MPD_DDE_SheetClosed,               /* B, no reply expected  */
    Wimp_MPD_DDE_ReturnHandleAndBlock,      /* A, no reply expected  */

/* PD receives */
    Wimp_MPD_DDE_IdentifyMarkedBlock,       /* E; replies with ReturnHandleAndBlock */
    Wimp_MPD_DDE_EstablishHandle,           /* A; replies with ReturnHandleAndBlock */
    Wimp_MPD_DDE_RequestUpdates,            /* B; no immediate reply, updates do SendSlotContents */
    Wimp_MPD_DDE_RequestContents,           /* B; replies with 0 or more SendSlotContents */
    Wimp_MPD_DDE_GraphClosed,               /* B; no reply */
    Wimp_MPD_DDE_DrawFileChanged,           /* D; no reply */
    Wimp_MPD_DDE_StopRequestUpdates         /* B; no immediate reply, updates don't SendSlotContents */
} WIMP_MSGPD_DDE_ID;

typedef char WIMP_MSGPD_DDETYPEA_TEXT[236 -4 -12];

typedef struct WIMP_MSGPD_DDETYPEA {
    int                         handle;
    int                         x_size;
    int                         y_size;
    WIMP_MSGPD_DDETYPEA_TEXT    text;       /* leafname & tag, both 0-terminated */
} WIMP_MSGPD_DDETYPEA;

typedef struct WIMP_MSGPD_DDETYPEB {
    int     handle;
} WIMP_MSGPD_DDETYPEB;

typedef enum WIMP_MSGPD_DDETYPEC_TYPE {
    Wimp_MPD_DDE_typeC_type_Text,
    Wimp_MPD_DDE_typeC_type_Number,
    Wimp_MPD_DDE_typeC_type_End
} WIMP_MSGPD_DDETYPEC_TYPE;

typedef struct WIMP_MSGPD_DDETYPEC {
    int                         handle;
    int                         x_off;
    int                         y_off;
    WIMP_MSGPD_DDETYPEC_TYPE    type;
    union WIMP_MSGPD_DDETYPEC_CONTENT {
        char                        text[236 -4 -16]; /* textual content 0-terminated */
        double                      number;
    } content;
} WIMP_MSGPD_DDETYPEC;

typedef struct WIMP_MSGPD_DDETYPED {
    char    leafname[236 -4];                   /* leafname of DrawFile, 0-terminated */
} WIMP_MSGPD_DDETYPED;

typedef struct WIMP_MSGPD_DDE {                 /* structure used in all PD DDE messages */
    WIMP_MSGPD_DDE_ID id;

    union WIMP_MSGPD_DDE_TYPE {
        WIMP_MSGPD_DDETYPEA a;
        WIMP_MSGPD_DDETYPEB b;
        WIMP_MSGPD_DDETYPEC c;
        WIMP_MSGPD_DDETYPED d;
    /*  WIMP_MSGPD_DDETYPEE e;  (no body) */
    } type;
} WIMP_MSGPD_DDE;

#if defined(COMPILING_WIMPLIB) && !defined(COMPILING_WIMPLIB_EXTENSIONS)

#ifndef __wimp_h
#include "./wimp.h"
#endif

#if defined(Wimp_MQuit)
#error That is the wrong wimp.h - it is from tboxlibs not ACW. Methinks you have not done the rlib_patch/apply.
#endif

#define WimpOpenWindowBlock struct _WimpOpenWindowBlock /* opaque for headers where it's not needed */
struct _WimpOpenWindowBlock;
#define WimpWindow struct _WimpWindow
#define WimpWindowWithBitset struct _WimpWindowWithBitset

typedef wimp_box BBox;

#else /* NOT COMPILING_WIMPLIB */

/* Allow use of tboxlibs names and structures in client code and WimpLib extension code if needed for sanity & clarity */

#if CROSS_COMPILE
#include "../tboxlibs/wimp.h" /* C: tboxlibs */
#else
#include "C:wimp.h" /* C: tboxlibs */
#endif

#if !defined(Wimp_MQuit)
#error That is the wrong wimp.h - it is from ACW not tboxlibs.
#endif

/* other useful button state bits not defined by tboxlibs wimp.h */
#define Wimp_MouseButtonDragAdjust      0x0010
/* NB Menu button NEVER produces 0x0020 or 0x0200 */
#define Wimp_MouseButtonDragSelect      0x0040
#define Wimp_MouseButtonSingleAdjust    0x0100
#define Wimp_MouseButtonSingleSelect    0x0400
#define Wimp_MouseButtonTripleAdjust    0x1000
#define Wimp_MouseButtonTripleSelect    0x4000

/* other useful messages not defined by tboxlibs wimp.h */
#ifndef Wimp_MFilerOpenDir
#define Wimp_MFilerOpenDir  0x0400
#define Wimp_MFilerCloseDir 0x0401
#endif

/* But fundamentally we still need the RISC_OSLib (patched) ones too */

#undef __wimp_h
#include "./wimp.h"

#define wimp_w HOST_WND /* NB overrides */
#define wimp_i int

_kernel_oserror *tbl_wimp_create_window     (WimpWindow *defn, int *handle);

_kernel_oserror *tbl_wimp_create_icon       (int priority,
                                            WimpCreateIconBlock *defn,
                                            int *handle);

_kernel_oserror *tbl_wimp_delete_window     (WimpDeleteWindowBlock *block);

_kernel_oserror *tbl_wimp_open_window       (WimpOpenWindowBlock *show);

_kernel_oserror *tbl_wimp_redraw_window     (WimpRedrawWindowBlock *block, int *more);

_kernel_oserror *tbl_wimp_update_window     (WimpRedrawWindowBlock *block, int *more);

_kernel_oserror *tbl_wimp_get_rectangle     (WimpRedrawWindowBlock *block, int *more);

_kernel_oserror *tbl_wimp_get_window_state  (WimpGetWindowStateBlock *state);

_kernel_oserror *tbl_wimp_set_icon_state    (WimpSetIconStateBlock *block);

_kernel_oserror *tbl_wimp_get_icon_state    (WimpGetIconStateBlock *block);

_kernel_oserror *tbl_wimp_force_redraw      (int window_handle,
                                            int xmin,
                                            int ymin,
                                            int xmax,
                                            int ymax);

_kernel_oserror *tbl_wimp_set_caret_position (int window_handle,
                                             int icon_handle,
                                             int xoffset,
                                             int yoffset,
                                             int height,
                                             int index);

_kernel_oserror *tbl_wimp_get_caret_position (WimpGetCaretPositionBlock *block);

_kernel_oserror *tbl_wimp_set_extent        (int window_handle, BBox *area);

_kernel_oserror *tbl_wimp_plot_icon         (WimpPlotIconBlock *block);

_kernel_oserror *tbl_wimp_block_copy        (int handle,
                                            int sxmin,
                                            int symin,
                                            int sxmax,
                                            int symax,
                                            int dxmin,
                                            int dymin);

#define wimp_send_message_to_task(event_code, p_user_message, dest) \
    wimp_sendmessage((wimp_etype) (event_code), (wimp_msgstr *) (p_user_message), (wimp_t) (dest))

#define wimp_send_message_to_window(event_code, p_user_message, window_handle, icon_handle) \
    wimp_sendwmessage((wimp_etype) (event_code), (wimp_msgstr *) (p_user_message), (window_handle), (icon_handle))

extern const _kernel_oserror *
wimp_reporterror_rf(
	_In_        const _kernel_oserror * e,
	_InVal_     int errflags_in,
	_Out_       int * p_button_clicked,
	_In_opt_z_  const char * message,
	_InVal_     int error_level);

/* our additional helper structures and functions to go with tboxlibs */

typedef BBox * P_BBox; typedef const BBox * PC_BBox;

_Check_return_
static inline int
BBox_width(
    _InRef_     PC_BBox p_bbox)
{
    return(p_bbox->xmax - p_bbox->xmin);
}

_Check_return_
static inline int
BBox_height(
    _InRef_     PC_BBox p_bbox)
{
    return(p_bbox->ymax - p_bbox->ymin);
}

/*
Icons
*/

typedef struct WimpIconFlagsBitset /* flags bitset for ease of manipulation */
{
    UBF text              : 1;
    UBF sprite            : 1;
    UBF border            : 1;
    UBF horz_centred      : 1;

    UBF vert_centred      : 1;
    UBF filled            : 1;
    UBF anti_aliased      : 1;
    UBF needs_help        : 1;

    UBF indirected        : 1;
    UBF right_justified   : 1;
    UBF allow_adjust      : 1;
    UBF half_size         : 1;

    UBF button_type       : 4;

    UBF esg               : 5; /* as per PRM Vol III */

    UBF selected          : 1;
    UBF shaded            : 1;
    UBF deleted           : 1;

    UBF fg_colour         : 4;

    UBF bg_colour         : 4;
}
WimpIconFlagsBitset;

typedef union WimpIconFlagsWithBitset /* contains bitset for ease of manipulation */
{
    U32 u32;
    WimpIconFlagsBitset bits;
}
WimpIconFlagsWithBitset;

typedef struct WimpIconBlockWithBitset /* contains bitset for ease of manipulation */
{
    BBox            bbox;
    WimpIconFlagsWithBitset flags;
    WimpIconData    data;
}
WimpIconBlockWithBitset; /* analogous to WimpIconBlock */

/*
Windows
*/

typedef struct WimpWindowFlagsBitset /* flags bitset for ease of manipulation */
{
    UBF old_bit_0         : 1;
    UBF moveable          : 1;
    UBF old_bit_2         : 1;
    UBF old_bit_3         : 1;

    UBF auto_redraw       : 1;
    UBF pane              : 1;
    UBF no_bounds         : 1;
    UBF old_bit_7         : 1;

    UBF scroll_repeat     : 1;
    UBF scroll            : 1;
    UBF real_colours      : 1;
    UBF back              : 1;

    UBF hot_keys          : 1;
    UBF stay_on_screen    : 1;
    UBF ignore_x_extent   : 1;
    UBF ignore_y_extent   : 1;

    UBF open              : 1;
    UBF not_covered       : 1;
    UBF full_size         : 1;
    UBF toggled           : 1;

    UBF has_input_focus   : 1;
    UBF force_once_on_screen : 1; /* on next open */
    UBF reserved_bit_22   : 1;
    UBF reserved_bit_23   : 1;

    UBF has_back_icon     : 1;
    UBF has_close_icon    : 1;
    UBF has_title_icon    : 1;
    UBF has_toggle_size   : 1;

    UBF has_vert_scroll   : 1;
    UBF has_size_icon     : 1;
    UBF has_horz_scroll   : 1;
    UBF new_format        : 1;
}
WimpWindowFlagsBitset;

typedef union WimpWindowFlagsWithBitset /* contains bitset for ease of manipulation */
{
    U32 u32;
    WimpWindowFlagsBitset bits;
}
WimpWindowFlagsWithBitset;

typedef struct WimpWindowWithBitset /* contains bitset for ease of manipulation */
{
    BBox          visible_area;
    int           xscroll;
    int           yscroll;
    int           behind;
    WimpWindowFlagsWithBitset flags;
    char          title_fg;
    char          title_bg;
    char          work_fg;
    char          work_bg;
    char          scroll_outer;
    char          scroll_inner;
    char          highlight_bg;
    char          reserved;
    BBox          extent;
    WimpIconFlagsWithBitset title_flags;
    WimpIconFlagsWithBitset work_flags;
    void         *sprite_area;
    short         min_width;
    short         min_height;
    WimpIconData  title_data;
    int           nicons;
}
WimpWindowWithBitset; /* analogous to WimpWindow */

typedef union WimpUpdateAndRedrawWindowBlock
{
    WimpUpdateWindowBlock update;
    WimpRedrawWindowBlock redraw;
}
WimpUpdateAndRedrawWindowBlock;

typedef struct WimpAutoScrollBlock
{
    int window_handle;
    int left_pause_zone_width;
    int bottom_pause_zone_width;
    int right_pause_zone_width;
    int top_pause_zone_width;
    int pause_duration_cs;
    int state_change_handler;
    int state_change_handler_handle;
}
WimpAutoScrollBlock;

typedef struct WimpDataRequestMessage
{
    wimp_w     destination_window_handle;
    int        destination_handle;
    int        destination_x;               /* abs */
    int        destination_y;
    int        flags;
    int        type[236/4 - 5];             /* list of acceptable file types, -1 at end */
}
WimpDataRequestMessage;

/* additional messages */
typedef struct
{
    struct
    {
        int    size;
        int    sender;
        int    my_ref;
        int    your_ref;
        int    action_code;
    } hdr;

    union
    {
        int                       words[59];
        char                      bytes[236];

        /* PipeDream specifics */
        WIMP_MSGPD_DDE            pd_dde;

        /* ones not defined by tboxlibs */
        wimp_msgprequitrequest    prequit;
        wimp_msgtaskinit          task_initialise;
        wimp_msgwindowinfo        window_info;
    } data;

} WimpMessageExtra;

typedef struct
{
    int event_code;

    WimpPollBlock event_data;

} WimpEvent;

#endif /* COMPILING_WIMPLIB */

#endif /* __cs_wimp_h */

/* end of cs-wimp.h */
