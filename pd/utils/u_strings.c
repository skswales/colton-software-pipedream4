/* u_strings.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2021 Stuart Swales */

/* Strings from PipeDream - compile to message file*/

#include "common/gflags.h" /* leave here for PCH */

#define MAKE_MESSAGE_FILE 1

#if CROSS_COMPILE
#define strings_init u_strings_init
#endif

#include "strings.c"

/* end of u_strings.c */
