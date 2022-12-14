/* defineos.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Some target-specific definitions for the target machine and OS */

#ifndef __defineos_h
#define __defineos_h

/*
 * target machine definition
 */

#ifndef CROSS_COMPILE
#define CROSS_COMPILE 0
#endif

/*
check that setup from selectos.h is ok
*/

#ifndef WINDOWS
#define WINDOWS 0
#endif

#ifndef RISCOS
#define RISCOS 0
#endif

#if !(WINDOWS || RISCOS)
#   error Unable to compile code for this OS
#endif

/* define something we can consistently test on */
#if (WINDOWS && defined(_UNICODE)) || (RISCOS && 0)
#define APP_UNICODE 1
#if WINDOWS
#define UNICODE 1 /* Many Windows headers still need this rather than the compiler-defined _UNICODE */
#endif
#else
#define APP_UNICODE 0
#endif

/* First time set-up */
#include "cmodules/coltsoft/pragma.h"

#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234

#define __ORDER_BIG_ENDIAN__    BIG_ENDIAN
#define __ORDER_LITTLE_ENDIAN__ LITTLE_ENDIAN

#if RISCOS

#define   BYTE_ORDER           LITTLE_ENDIAN
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__

#if !CROSS_COMPILE && defined(__CC_NORCROFT)

/* Norcroft compiler, target ARM / RISC OS */

#ifndef __CHAR_UNSIGNED__
#define __CHAR_UNSIGNED__ 1
#endif

#if __STDC_VERSION__ >= 199901L
#define __FUNCTION__ __func__
#else
#define __FUNCTION__ ""
#endif /* __STDC_VERSION__ */

#define __TFILE__ __FILE__
#define __Tfunc__ __FUNCTION__

#include "cmodules/coltsoft/ns-sal.h"

#endif /* CROSS_COMPILE */

#define F64_IS_64_BIT_ALIGNED 0
#define P_ANY_IS_64_BIT_ALIGNED 0

#if CROSS_COMPILE && defined(HOST_WINDOWS)
#include "cmodules/coltsoft/target_riscos_host_windows.h"
#endif /* CROSS_COMPILE */

#if CROSS_COMPILE && defined(HOST_GCCSDK)
#include "cmodules/coltsoft/target_riscos_host_gccsdk.h"
#endif /* CROSS_COMPILE */

#if CROSS_COMPILE && defined(HOST_CLANG)
#include "cmodules/coltsoft/target_riscos_host_clang.h"
#endif /* CROSS_COMPILE */

#endif /* RISCOS */

#if WINDOWS

#define   BYTE_ORDER           LITTLE_ENDIAN
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__

#error PipeDream can not be compiled for WINDOWS

#if !defined(__cplusplus)
#define inline __inline
#endif

#endif /* WINDOWS */

/* Nobble Microsoft extensions and definitions on sensible systems */

#if RISCOS

#define __cdecl
#define PASCAL

#if CROSS_COMPILE && defined(HOST_WINDOWS)
#define inline __inline /* for MSVC < C99 */
#else
/*#define __forceinline inline*/ /* macro MSVC tuning keyword for Norcroft et al */
#endif

#endif /* RISCOS */

#if !defined(_MSC_VER)
#ifndef __assume
#define __assume(expr) /*EMPTY*/
#endif
#endif /* _MSC_VER */

#ifndef __analysis_assume
#define __analysis_assume(expr) __assume(expr)
#endif

#ifndef _Analysis_assume_
#define _Analysis_assume_(expr) __assume(expr)
#endif

/* much more of differences covered in coltsoft/coltsoft.h ... */

#endif /* __defineos_h */

/* end of defineos.h */

