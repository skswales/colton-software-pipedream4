/* mlec.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

#ifndef __mlec_h
#define __mlec_h

#define SUPPORT_SELECTION TRUE /* SKS 04nov95 for 450 */
#define SUPPORT_LOADSAVE  TRUE
#define SUPPORT_CUTPASTE  TRUE
#define SUPPORT_PANEMENU  TRUE
#define SUPPORT_GETTEXT   TRUE

#ifndef MLEC_OFF

#ifndef RC_INVOKED /* not resource compiler */
#ifndef ERRDEF_EXPORT /* not errdef export */

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

/* exported types */

typedef struct __mlec_struct * mlec_handle;

/* exported routines */

extern int
mlec_create(
    mlec_handle *mlecp);

extern void
mlec_destroy(
    mlec_handle *mlecp);

extern void
mlec_attach(
    mlec_handle mlec,
    wimp_w main_win_handle,
    wimp_w pane_win_handle,
    wimp_box paneWorkArea,
    const char *menu_title);

extern void
mlec_detach(
    mlec_handle mlec);

extern void
mlec_claim_focus(
    mlec_handle mlec);

extern void
mlec_release_focus(
    mlec_handle mlec);

/*   mlec_attach_eventhandler - see further down this file */

#if SUPPORT_GETTEXT

extern int
mlec_GetText(
    mlec_handle mlec,
    char *buffptr,
    int buffsize);

extern int
mlec_GetTextLen(
    mlec_handle mlec);

#endif

extern int
mlec_SetText(
    mlec_handle mlec,
    char *text);

extern BOOL
mlec__event_handler(
    wimp_eventstr *e,
    void *handle);

extern int
mlec__insert_char(
    mlec_handle mlec,
    char ch);

extern int
mlec__insert_text(
    mlec_handle mlec,
    char *text);

extern int
mlec__insert_newline(
    mlec_handle mlec);

extern void
mlec__cursor_getpos(
    mlec_handle mlec,
    int * p_col,
    int * p_row);

extern void
mlec__cursor_setpos(
    mlec_handle mlec,
    int col,
    int row);

#if SUPPORT_SELECTION

extern void
mlec__selection_adjust(
    mlec_handle mlec, 
    int col,
    int row);

extern void
mlec__selection_clear(
    mlec_handle mlec);

extern void
mlec__selection_delete(
    mlec_handle mlec);

#endif

typedef enum
{
    Mlec_IsOpen            = 1,
    Mlec_IsClose           = 2,
    Mlec_IsReturn          = 3,
    Mlec_IsEscape          = 4,
    Mlec_IsClick           = 5,
    Mlec_IsWorkAreaChanged = 6
}
mlec_event_reason_code;

typedef enum
{
    mlec_event_unknown         = 0x00,

    /* from IsOpen */
    mlec_event_openned         = 0x10,

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
mlec_event_return_code;

typedef mlec_event_return_code (* mlec_event_proc) (
    mlec_event_reason_code rc,
    P_ANY handle,
    P_ANY p_eventdata);

#define mlec_event_proto(_e_s, _p_proc_mlec_event) \
_e_s mlec_event_return_code _p_proc_mlec_event( \
    mlec_event_reason_code rc, \
    P_ANY handle, \
    P_ANY p_eventdata) \

extern void
mlec_attach_eventhandler(
    mlec_handle mlec,
    mlec_event_proc proc,
    P_ANY handle,
    S32 add);

#endif /* ERRDEF_EXPORT */
#endif /* RC_INVOKED */

#ifndef RC_INVOKED /* not resource compiler */

/*
error definition
*/

#define MLEC_ERRLIST_DEF \
    errorstring(MLEC_ERR_NOSELECTION      , "No selected text") \
    errorstring(MLEC_ERR_GETTEXT_BUFOVF   , "Program error - Destination string too small") \
    errorstring(MLEC_ERR_NOPASTEBUFFER    , "Program error - No paste buffer") \
    errorstring(MLEC_ERR_BUFFERWENT_AWOL  , "Program error - Paste buffer went AWOL") \
    errorstring(MLEC_ERR_INVALID_PASTE_OP , "Program error - Invalid paste buffer operation")

#endif /* RC_INVOKED */

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
