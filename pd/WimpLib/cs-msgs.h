/* cs_msgs.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2022-2023 Stuart Swales */

#ifndef __cs_msgs_h
#define __cs_msgs_h

#ifndef __msgs_h
#include "msgs.h"
#endif

extern void
messages_readfile(
    _OutRef_    P_P_U8 p_msgs,
    _In_z_      const char * filename);

extern const char *
messages_lookup(
    _InRef_     P_P_U8 p_msgs,
    _In_z_      const char * tag_and_default,
    _InVal_     BOOL case_insensitive);

/*
implemented by PipeDream but needed as old flavour by FontSelect
*/

extern const char *
help_messages_lookup(const char * tag);

#define help_msgs_lookup(tag) \
    de_const_cast(char *, help_messages_lookup(tag))

#endif /* __cs_msgs_h */

/* end of cs_msgs.h */
