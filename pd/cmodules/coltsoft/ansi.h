/* ansi.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Standard includes */

#ifndef ANSI_H
#define ANSI_H

#include <stddef.h>
#include <stdlib.h>

#include <stdarg.h>
#ifdef __CC_NORCROFT
#define VA_END_SUPERFLUOUS 1 /* NB the compiler does a good job of optimising this out anyhow */
#endif

#ifndef __CC_NORCROFT
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

#include <float.h>

#include <errno.h>

#if defined(__SOFTFP__)
#include "external/SSwales/apcs_softpcs/apcs_softpcs.h"
#endif

#endif /* ANSI_H */

/* end of ansi.h */
