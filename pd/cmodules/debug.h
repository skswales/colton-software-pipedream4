/* debug.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for the debug module */

/* SKS October 1992 */

/*
Designed for multiple inclusion with different TRACE_ALLOWED
*/

#ifndef __report_h
#include "report.h" /* debug goes via report module */
#endif

#ifndef TRACE_ALLOWED
#if RELEASED
/* Default state for RELEASE is TRACE_ALLOWED OFF */
/* #error Default state for RELEASED is TRACE_ALLOWED OFF */
#define TRACE_ALLOWED 0
#else
/* Default state for DEBUG is TRACE_ALLOWED ON */
/* #error Default state for DEBUG is TRACE_ALLOWED ON */
#define TRACE_ALLOWED 1
#endif
#endif

#if !RISCOS && !WINDOWS && TRACE_ALLOWED
/* make logic cleaner */
#undef  TRACE_ALLOWED
#define TRACE_ALLOWED 0
#error defined TRACE_ALLOWED OFF for OS
#endif

#ifdef trace_disable
#undef trace_disable
#endif

#ifdef trace_is_enabled
#undef trace_is_enabled
#endif

#ifdef trace_is_on
#undef trace_is_on
#endif

#ifdef trace_off
#undef trace_off
#endif

#ifdef trace_on
#undef trace_on
#endif

#ifdef trace_procedure_name
#undef trace_procedure_name
#endif

#ifdef trace_list
#undef trace_list
#endif

#ifdef tracing
#undef tracing
#endif

#ifdef vtracef
#undef vtracef
#endif

#ifdef trace_v0
#undef trace_v0
#endif

#ifdef trace_v1
#undef trace_v1
#endif

#ifdef trace_v2
#undef trace_v2
#endif

#ifdef trace_v3
#undef trace_v3
#endif

#ifdef trace_v4
#undef trace_v4
#endif

#ifdef trace_v5
#undef trace_v5
#endif

#ifdef trace_v6
#undef trace_v6
#endif

#ifdef trace_v7
#undef trace_v7
#endif

#ifdef trace_v8
#undef trace_v8
#endif

#ifdef trace_0
#undef trace_0
#endif

#ifdef trace_1
#undef trace_1
#endif

#ifdef trace_2
#undef trace_2
#endif

#ifdef trace_3
#undef trace_3
#endif

#ifdef trace_4
#undef trace_4
#endif

#ifdef trace_5
#undef trace_5
#endif

#ifdef trace_6
#undef trace_6
#endif

#ifdef trace_7
#undef trace_7
#endif

#ifdef trace_8
#undef trace_8
#endif

/*
exported functions
*/

#if TRACE_ALLOWED

/* tracef(...) should really only appear #if TRACE_ALLOWED */

#if defined(__CC_NORCROFT) /* this can check parameters for matching format */
#pragma check_printf_formats
#endif

extern void __cdecl
tracef(
    _InVal_     U32 mask,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

#if defined(__CC_NORCROFT)
#pragma no_check_printf_formats
#endif

extern void
trace_disable(void);

extern BOOL
trace_is_enabled(void);

extern S32
trace_is_on(void);

#ifdef TRACE_LIST

extern STATUS
trace_list(
    _InVal_     U32 mask,
    P_ANY /*P_LIST_BLOCK*/ p_list_block);

#else
#define trace_list(mask, p_list_block) STATUS_OK
#endif

extern void
trace_off(void);

extern void
trace_on(void);

#define trace_procedure_name(proc) \
    report_procedure_name(proc)

extern void
vtracef(
    _InVal_     U32 mask,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args);

extern int trace__count; /* exported only for tracing() */

#define tracing(mask) ( \
    (0 != ((mask) & PERSONAL_TRACE_FLAGS)) \
        ? ((0 != ((mask) & TRACE_OUT)) || (0 != trace__count)) \
        : 0 )

/* you can use a variable format string in these ones ... */

#define trace_v0(z, v) \
    do { if_constant(tracing(z)) \
            tracef(z, (v)); } while_constant(0)
#define trace_v1(z, v, b) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b); } while_constant(0)
#define trace_v2(z, v, b, c) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c); } while_constant(0)
#define trace_v3(z, v, b, c, d) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d); } while_constant(0)
#define trace_v4(z, v, b, c, d, e) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d, e); } while_constant(0)
#define trace_v5(z, v, b, c, d, e, f) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d, e, f); } while_constant(0)
#define trace_v6(z, v, b, c, d, e, f, g) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d, e, f, g); } while_constant(0)
#define trace_v7(z, v, b, c, d, e, f, g, h) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d, e, f, g, h); } while_constant(0)
#define trace_v8(z, v, b, c, d, e, f, g, h, i) \
    do { if_constant(tracing(z)) \
            tracef(z, (v), b, c, d, e, f, g, h, i); } while_constant(0)

/* ... but not in these ones - used to get strings fed into a different segment */

#define trace_0(z, a) \
    do { if_constant(tracing(z)) \
            tracef(z, (a)); } while_constant(0)
#define trace_1(z, a, b) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b); } while_constant(0)
#define trace_2(z, a, b, c) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c); } while_constant(0)
#define trace_3(z, a, b, c, d) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d); } while_constant(0)
#define trace_4(z, a, b, c, d, e) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d, e); } while_constant(0)
#define trace_5(z, a, b, c, d, e, f) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d, e, f); } while_constant(0)
#define trace_6(z, a, b, c, d, e, f, g) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d, e, f, g); } while_constant(0)
#define trace_7(z, a, b, c, d, e, f, g, h) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d, e, f, g, h); } while_constant(0)
#define trace_8(z, a, b, c, d, e, f, g, h, i) \
    do { if_constant(tracing(z)) \
            tracef(z, (a), b, c, d, e, f, g, h, i); } while_constant(0)

#endif /* TRACE_ALLOWED */

#if TRACE_ALLOWED

#else

#define trace_disable()
#define trace_is_enabled()              FALSE
#define trace_is_on()                   FALSE
#define trace_list(mask, p_list_block)  STATUS_OK
#define trace_off()
#define trace_on()
#define trace_procedure_name(proc)      NULL
#define tracing(mask)                   0

#define vtracef(mask, format, arg)

#define trace_v0(z, v)
#define trace_v1(z, v, b)
#define trace_v2(z, v, b, c)
#define trace_v3(z, v, b, c, d)
#define trace_v4(z, v, b, c, d, e)
#define trace_v5(z, v, b, c, d, e, f)
#define trace_v6(z, v, b, c, d, e, f, g)
#define trace_v7(z, v, b, c, d, e, f, g, h)
#define trace_v8(z, v, b, c, d, e, f, g, h, i)

#define trace_0(z, a)
#define trace_1(z, a, b)
#define trace_2(z, a, b, c)
#define trace_3(z, a, b, c, d)
#define trace_4(z, a, b, c, d, e)
#define trace_5(z, a, b, c, d, e, f)
#define trace_6(z, a, b, c, d, e, f, g)
#define trace_7(z, a, b, c, d, e, f, g, h)
#define trace_8(z, a, b, c, d, e, f, g, h, i)

#endif /* !TRACE_ALLOWED */

#ifndef TRACE_OUT

#define TRACE_MODULE_EVAL       0x00000001U
#define TRACE_MODULE_SPELL      0x00000002U
#define TRACE_MODULE_ALIGATOR   0x00000004U
#define TRACE_MODULE_NUMFORM    0x00000008U
#define TRACE_MODULE_FILE       0x00000010U
#define TRACE_MODULE_ALLOC      0x00000020U
#define TRACE_OLE               0x00000040U
#define TRACE_MODULE_UREF       0x00000080U
#define TRACE_MODULE_GR_CHART   0x00000100U

#define TRACE_APP_TYPE5_FONTS   0x00000200U
#define TRACE_APP_TYPE5_SKEL    0x00000400U
#define TRACE_APP_PD4           0x00000800U
#define TRACE_APP_DPLIB         0x00001000U
#define TRACE_MODULE_LIST       0x00002000U
#define TRACE_APP_TYPE5_CLICK   0x00004000U
#define TRACE_APP_TYPE5_CARET   0x00008000U
#define TRACE_APP_TYPE5_LOAD    0x00010000U
#define TRACE_APP_TYPE5_REDRAW  0x00020000U
#define TRACE_APP_TYPE5_HOST    0x00040000U
#define TRACE_APP_TYPE5_PRINT   0x00080000U
#define TRACE_APP_TYPE5_UREF    0x00100000U
#define TRACE_APP_TYPE5_ARGLIST 0x00200000U
#define TRACE_APP_TYPE5_FORMAT  0x00400000U
#define TRACE_APP_TYPE5_SK_DRAW 0x00800000U
#define TRACE_APP_TYPE5_MAEVE   0x01000000U
#define TRACE_APP_TYPE5_STYLE   0x02000000U
#define TRACE_APP_DIALOG        0x04000000U
#define TRACE_APP_TYPE5_ERRORS  0x08000000U
#define TRACE_APP_TYPE5_SSUI    0x10000000U
#define TRACE_APP_MEMORY_USE    0x20000000U
#define TRACE_WM_EVENT          0x40000000U

#define TRACE_OUT               0x80000000U /* overrides trace_on/off state so you don't have to bracket calls to trace_n() with on/off calls */
#define TRACE_ANY               0x7FFFFFFFU

#define TRACE_RISCOS_HOST       TRACE_APP_TYPE5_HOST
#define TRACE_WINDOWS_HOST      TRACE_APP_TYPE5_HOST

/*
PipeDream 4
*/

#define TRACE_APP_EXPEDIT       TRACE_APP_TYPE5_SSUI
#define TRACE_APP_PD4_RENDER    TRACE_APP_TYPE5_SK_DRAW

#define trace_string            report_sbstr

#endif /* TRACE_OUT */

/* end of debug.h */
