/* stringlk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Library module for system-independent string lookup */

/* SKS July 1991 */

#ifndef __stringlk_h
#define __stringlk_h

#ifndef MODULE_ERR_BASE
#define MODULE_ERR_BASE      (-1000)
#endif

#ifndef MODULE_ERR_INCREMENT
#define MODULE_ERR_INCREMENT 1000
#endif

#ifndef MODULE_MSG_BASE
#define MODULE_MSG_BASE      1000
#endif

#ifndef MODULE_MSG_INCREMENT
#define MODULE_MSG_INCREMENT 1000
#endif

/*
exported functions
*/

extern PC_USTR
string_lookup(
    S32 stringid);

#define string_lookup_nc(stringid) \
    de_const_cast(char *, string_lookup(stringid))

extern void
string_lookup_buffer(
    S32 stringid,
    _Out_writes_z_(elemof_buffer) char * buffer,
    _InVal_     U32 elemof_buffer);

#define resource_lookup_ustr(stringid) \
    string_lookup(stringid)

#endif /* __stringlk_h */

/* end of stringlk.h */
