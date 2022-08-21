/* personal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* See master list in debug.h. Copy these as needed into PERSONAL_TRACE_FLAGS */

#define PERSONAL_TRACE_FLAGS_PARKING ( 0 | \
    TRACE_MODULE_EVAL       | \
    TRACE_MODULE_SPELL      | \
    TRACE_MODULE_ALIGATOR   | \
    TRACE_MODULE_NUMFORM    | \
    TRACE_MODULE_FILE       | \
    TRACE_MODULE_ALLOC      | \
    TRACE_OLE               | \
    TRACE_MODULE_UREF       | \
    TRACE_MODULE_GR_CHART   | \
    TRACE_APP_TYPE5_FONTS   | \
    TRACE_APP_TYPE5_SKEL    | \
    TRACE_APP_PD4           | \
    TRACE_APP_DPLIB         | \
    TRACE_MODULE_LIST       | \
    TRACE_APP_TYPE5_CLICK   | \
    TRACE_APP_TYPE5_CARET   | \
    TRACE_APP_TYPE5_LOAD    | \
    TRACE_APP_TYPE5_REDRAW  | \
    TRACE_APP_TYPE5_HOST    | \
    TRACE_APP_TYPE5_PRINT   | \
    TRACE_APP_TYPE5_UREF    | \
    TRACE_APP_TYPE5_ARGLIST | \
    TRACE_APP_TYPE5_FORMAT  | \
    TRACE_APP_TYPE5_SK_DRAW | \
    TRACE_APP_TYPE5_MAEVE   | \
    TRACE_APP_TYPE5_STYLE   | \
    TRACE_APP_DIALOG        | \
    TRACE_APP_TYPE5_ERRORS  | \
    TRACE_APP_TYPE5_SSUI    | \
    TRACE_APP_MEMORY_USE    | \
    TRACE_WM_EVENT          | \
    0 )

#define PERSONAL_TRACE_FLAGS ( 0 | \
    TRACE_RISCOS_HOST       | \
    TRACE_APP_PD4           | \
    TRACE_MODULE_ALLOC      | \
    0 )

#define TRACE_DOC 1

#define STUART 1

#if !RELEASED && STUART && 1
#define TRACE_MAIN_ALLOCS 1
#define VALIDATE_MAIN_ALLOCS 1
#define VALIDATE_MAIN_ALLOCS_START 1
#define ALLOC_CLEAR_FREE 1
#endif

/* end of personal.h */
