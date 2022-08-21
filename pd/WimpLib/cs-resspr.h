/* cs-resspr.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2021 Stuart Swales */

#ifndef __cs_resspr_h
#define __cs_resspr_h

#ifndef __resspr_h
#include "resspr.h"
#endif

/*
cs-resspr.c
*/

_Check_return_
extern BOOL
resspr_mergesprites(
    _In_z_      const char *leafname);

#endif /* __cs_resspr_h */

/* end of cs-resspr.h */
