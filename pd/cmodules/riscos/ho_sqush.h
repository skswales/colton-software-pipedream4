/* ho_sqush.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __file_h
#include "cmodules/file.h"
#endif

extern STATUS
host_squash_expand_from_file(
    P_ANY p_any /*filled*/,
    FILE_HANDLE file_handle,
    U32 compress_size,
    U32 expand_size);

extern STATUS
host_squash_load_data_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle);

extern STATUS
host_squash_load_messages_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle);

extern STATUS
host_squash_load_sprite_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle);

extern STATUS
host_squash_alloc(
    P_P_ANY p_p_any /*out*/,
    U32 size,
    BOOL may_be_code);

extern void
host_squash_dispose(
    P_P_ANY p_p_any /*inout*/);

/* end of ho_sqush.h */
