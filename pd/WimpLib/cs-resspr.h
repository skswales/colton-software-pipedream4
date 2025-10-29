/* cs-resspr.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2025 Stuart Swales */

#ifndef CS_RESSPR_H
#define CS_RESSPR_H

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

#endif /* CS_RESSPR_H */

/* end of cs-resspr.h */
