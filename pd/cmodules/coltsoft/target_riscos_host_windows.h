/* target_riscos_host_windows.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2014 Stuart Swales */

/* SKS 2012 */

#ifndef __target_riscos_host_windows_h
#define __target_riscos_host_windows_h

#if defined(_MSC_VER)
//#error You need to undefine all compiler defines with /u
/* Now we can survive OK with them - particularly needed for SAL */
#else
#define _MSC_VER 1500 /* VC2008 */
#endif

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 0L /* MSVC is still not C99 */
#endif

#ifndef _CHAR_UNSIGNED
#if 1
#define _CHAR_UNSIGNED 1 /* We usually compile with switches that undefine all MS' macros! */
#else
#error  _CHAR_UNSIGNED must be set (use -J switch)
#endif
#endif

/* preempt definition by sourceannotations.h */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* sorry, RISC OS headers ... */
#ifndef __wchar_t
#  define __wchar_t 1
#endif

#pragma warning(push)
#pragma warning(disable:4820) /* 'x' bytes padding added after data member */

#ifndef SAL_SUPP_H /* try to suppress inclusion as we get macro redefinition warnings when using Visual C++ 11.0 with the Windows 7.1 SDK */
#define SAL_SUPP_H 1
#endif

#if defined(WINCE)
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 1
#include "C:\Program Files\Microsoft Visual Studio 9.0\VC\ce\include\sal.h"
#else /* WINCE */
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 1
#include "C:\Program Files\Microsoft Visual Studio 9.0\VC\include\sal.h"
#endif /* WINCE */

#pragma warning(pop)

#ifndef _In_reads_
#include "cmodules/no-sal.h"
#endif

__pragma(warning(disable:4514)) /* 'x' : unreferenced inline function has been removed */
__pragma(warning(disable:4710)) /* 'x' : function not inlined */
__pragma(warning(disable:4820)) /* padding added after data member */

/* Temporarily disable some /analyze warnings */
__pragma(warning(disable:6246)) /* Local declaration of 'x' hides declaration of the same name in outer scope. For additional information, see previous declaration at line 'y'  */

#undef  F64_IS_64_BIT_ALIGNED
#define F64_IS_64_BIT_ALIGNED 1

#ifdef _WIN64
#error Just don't even think about it!
#endif

#define __swi(inline_swi_number) /* Norcroft specific */

#define ___assert(e, s) 0 /* Norcroft specific */

#endif /* __target_riscos_host_windows_h */

/* end of target_riscos_host_windows.h */
