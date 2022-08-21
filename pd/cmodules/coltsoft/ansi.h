/* ansi.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Standard includes */

#ifndef __ansi_h
#define __ansi_h

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#if CROSS_COMPILE && defined(HOST_WINDOWS)
#define __func__ __FUNCTION__ /* MSVC is not yet C99 compliant */
#endif

#ifndef __arm
#ifndef __stdint_ll
#define __stdint_ll /* DO need 64-bit integer base types even though cross ain't C99 */
#define __stdint_ll_hack_defined
#endif
#endif

#include <inttypes.h> /* C99 header */

#ifdef __stdint_ll_hack_defined
#undef __stdint_ll_hack_defined
#undef __stdint_ll /* but don't confuse the rest of the program */
#endif

#include <stdio.h>

#include <string.h>

#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <float.h>

#include <time.h>
#include <locale.h>
#include <errno.h>

#endif /* __ansi_h */

/* end of ansi.h */
