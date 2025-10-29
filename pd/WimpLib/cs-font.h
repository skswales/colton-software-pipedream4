/* cs-font.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2025 Stuart Swales */

#ifndef CS_FONT_H
#define CS_FONT_H

#ifndef   CS_WIMP_H
#include "cs-wimp.h" /* ensure we get ours in first */
#endif

#ifndef __font_h
#include "font.h"
#endif

extern _kernel_oserror *
font_LoseFont(
    font f);

extern _kernel_oserror *
font_SetFont(
    font f);

#endif /* CS_FONT_H */

/* end of cs-font.h */
