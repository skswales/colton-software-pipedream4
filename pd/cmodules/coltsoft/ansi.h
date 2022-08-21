/* ansi.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Standard includes */

#ifndef __ansi_h
#define __ansi_h

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef __arm
#ifndef __stdint_ll
#define __stdint_ll /* DO need 64-bit integer base types even though cross ain't C99 */
#define __stdint_ll_hack_defined
#endif
#endif

/* C99 headers */
#include <inttypes.h>
#include <stdbool.h>

#ifdef __stdint_ll_hack_defined
#undef __stdint_ll_hack_defined
#undef __stdint_ll /* but don't confuse the rest of the program */
#endif

#include <stdio.h>

#include <string.h>

#include <ctype.h>
#include <limits.h>

#include <math.h>
#if 1
#undef fma /* Norcroft is currently broken w.r.t. __d_fma inline */
#endif

#include <float.h>

#include <errno.h>

#endif /* __ansi_h */

/* end of ansi.h */
