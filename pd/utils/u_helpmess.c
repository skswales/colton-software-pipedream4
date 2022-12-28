/* u_helpmess.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2022 Stuart Swales */

/* Strings from PipeDream - compile to message file */

#include "common/gflags.h" /* leave here for PCH */

#define MAKE_MESSAGE_FILE 1

#if CROSS_COMPILE
#define helpmess_init u_helpmess_init
#endif

#include "helpmess.c"

/* end of u_helpmess.c */
