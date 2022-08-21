/* mlec.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

#ifndef __mlec_h
#define __mlec_h

#define SUPPORT_SELECTION TRUE /* SKS 04nov95 for PD 4.50 */
#define SUPPORT_LOADSAVE  TRUE
#define SUPPORT_CUTPASTE  TRUE
#define SUPPORT_PANEMENU  TRUE
#define SUPPORT_GETTEXT   TRUE

#ifndef MLEC_OFF

#ifndef ERRDEF_EXPORT /* not errdef export */

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

/* exported types */

typedef struct MLEC_STRUCT * MLEC_HANDLE;

#define MLEC_ATTRIBUTE_LINESPACE 0
#define MLEC_ATTRIBUTE_CHARHEIGHT 1
#define MLEC_ATTRIBUTE_CARETHEIGHTPOS 2
#define MLEC_ATTRIBUTE_CARETHEIGHTNEG 3
#define MLEC_ATTRIBUTE_MARGIN_LEFT 4
#define MLEC_ATTRIBUTE_MARGIN_TOP 5
#define MLEC_ATTRIBUTE_BG_RGB 6
#define MLEC_ATTRIBUTE_FG_RGB 7
#define MLEC_ATTRIBUTE_MAX 8

#define MLEC_ATTRIBUTE int

/*
Exported routines
*/

_Check_return_
extern int
mlec_attribute_query(
    /*_In_*/    MLEC_HANDLE mlec,
    _InVal_     MLEC_ATTRIBUTE attribute);

extern void
mlec_attribute_set(
    /*_Inout_*/ MLEC_HANDLE mlec,
    _InVal_     MLEC_ATTRIBUTE attribute,
    _InVal_     int value);

extern int
mlec_create(
    MLEC_HANDLE *mlecp);

extern void
mlec_destroy(
    MLEC_HANDLE *mlecp);

extern void
mlec_attach(
    MLEC_HANDLE mlec,
    HOST_WND main_win_handle,
    HOST_WND pane_win_handle,
    const BBox * const p_pane_extent,
    const char *menu_title);

extern void
mlec_detach(
    MLEC_HANDLE mlec);

extern void
mlec_claim_focus(
    MLEC_HANDLE mlec);

extern void
mlec_release_focus(
    MLEC_HANDLE mlec);

/*   mlec_attach_eventhandler - see further down this file */

#if SUPPORT_GETTEXT

extern int
mlec_GetText(
    MLEC_HANDLE mlec,
    char *buffptr,
    int buffsize);

extern int
mlec_GetTextLen(
    MLEC_HANDLE mlec);

#endif

extern int
mlec_SetText(
    MLEC_HANDLE mlec,
    char *text);

extern BOOL
mlec__event_handler(
    wimp_eventstr *e,
    void *handle);

extern int
mlec__insert_char(
    MLEC_HANDLE mlec,
    char ch);

extern int
mlec__insert_text(
    MLEC_HANDLE mlec,
    char *text);

extern int
mlec__insert_newline(
    MLEC_HANDLE mlec);

extern void
mlec__cursor_get_position(
    MLEC_HANDLE mlec,
    int * p_col,
    int * p_row);

extern void
mlec__cursor_set_position(
    MLEC_HANDLE mlec,
    int col,
    int row);

#if SUPPORT_SELECTION

extern void
mlec__selection_adjust(
    MLEC_HANDLE mlec, 
    int col,
    int row);

extern void
mlec__selection_clear(
    MLEC_HANDLE mlec);

extern void
mlec__selection_delete(
    MLEC_HANDLE mlec);

#endif

#if SUPPORT_CUTPASTE

extern int
mlec__atcursor_paste(MLEC_HANDLE mlec);

extern int
mlec__selection_copy(MLEC_HANDLE mlec);

extern int
mlec__selection_cut(MLEC_HANDLE mlec);

#endif

typedef enum MLEC_EVENT_REASON_CODE
{
    Mlec_IsOpen            = 1,
    Mlec_IsClose           = 2,
    Mlec_IsReturn          = 3,
    Mlec_IsEscape          = 4,
    Mlec_IsClick           = 5,
    Mlec_IsWorkAreaChanged = 6
}
MLEC_EVENT_REASON_CODE;

typedef enum MLEC_EVENT_RETURN_CODE
{
    mlec_event_unknown         = 0x00,

    /* from IsOpen */
    mlec_event_opened          = 0x10,

    /* from IsClose */
    mlec_event_closed          = 0x20,

    /* from IsReturn */
    mlec_event_return          = 0x30,

    /* from IsEscape */
    mlec_event_escape          = 0x40,

    /* from IsClick */
    mlec_event_click           = 0x50,

    /* from IsWorkAreaChanged */
    mlec_event_workareachanged = 0x60
}
MLEC_EVENT_RETURN_CODE;

typedef MLEC_EVENT_RETURN_CODE (* MLEC_EVENT_PROC) (
    MLEC_EVENT_REASON_CODE rc,
    P_ANY handle,
    P_ANY p_eventdata);

#define MLEC_EVENT_PROTO(_e_s, _p_proc_mlec_event) \
_e_s MLEC_EVENT_RETURN_CODE _p_proc_mlec_event( \
    MLEC_EVENT_REASON_CODE rc, \
    P_ANY handle, \
    P_ANY p_eventdata) \

extern void
mlec_attach_eventhandler(
    MLEC_HANDLE mlec,
    MLEC_EVENT_PROC proc,
    P_ANY handle,
    S32 add);

#endif /* ERRDEF_EXPORT */

/*
error definition
*/

#define MLEC_ERRLIST_DEF \
    errorstring(MLEC_ERR_NOSELECTION      , "No selected text") \
    errorstring(MLEC_ERR_GETTEXT_BUFOVF   , "Destination string too small") \
    errorstring(MLEC_ERR_NOPASTEBUFFER    , "No paste buffer") \
    errorstring(MLEC_ERR_BUFFERWENT_AWOL  , "Paste buffer went AWOL") \
    errorstring(MLEC_ERR_INVALID_PASTE_OP , "Invalid paste buffer operation")

/*
error definition
*/

#define MLEC_ERR_BASE                   (-7000)

#define MLEC_ERR_spare_7000             (-7000)
#define MLEC_ERR_NOSELECTION            (-7001)
#define MLEC_ERR_GETTEXT_BUFOVF         (-7002)
#define MLEC_ERR_NOPASTEBUFFER          (-7003)
#define MLEC_ERR_BUFFERWENT_AWOL        (-7004)
#define MLEC_ERR_INVALID_PASTE_OP       (-7005)

#define MLEC_ERR_END                    (-7006)

/*
message definition (keep consistent with CModules.*.msg.mlec)
*/

#define MLEC_MSG_BASE                   (7000)

#define MLEC_MSG_MENUBDY                (7001)  /* NB menu_title is supplied by caller */
#define MLEC_MSG_MENUHDR_SAVE           (7010)
#define MLEC_MSG_MENUBDY_SAVE           (7011)
#define MLEC_MSG_MENUHDR_SELECTION      (7020)
#define MLEC_MSG_MENUBDY_SELECTION      (7021)

#endif /* MLEC_OFF */

#endif /* __mlec_h */

/* end of mlec.h */
