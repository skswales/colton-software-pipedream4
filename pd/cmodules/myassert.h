/* myassert.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* NB Designed for multiple inclusion */

extern BOOL __cdecl
__myasserted(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

extern BOOL __cdecl
__myasserted_msg(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

extern BOOL
__myasserted_EQ(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _InVal_     U32 val1,
    _InVal_     U32 val2);

extern BOOL
__vmyasserted(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_opt_z_  PCTSTR message,
    _In_z_      PCTSTR format,
    /**/        va_list args);

extern void
__hard_assert(
    _InVal_     BOOL hard_mode);

extern STATUS
__status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR str);

#if WINDOWS

extern BOOL
__WrapOsBool(
    _InVal_     BOOL res,
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     int line_no,
    _In_z_      PCTSTR str);

#endif /* OS */

#ifndef myDebugBreak
#define myDebugBreak() /*EMPTY*/
#if CHECKING
#undef myDebugBreak
#if WINDOWS
#if defined(_X86_) || (defined(_AMD64_) && !defined(_WIN64))
#define myDebugBreak()  __asm { int 3 } /* break at this level when possible for ease of debugging rather than in kernel32.dll */
#else
#define myDebugBreak()  DebugBreak()
#endif /* _X86_ || _AMD64_ */
#else /* !WINDOWS */
#define myDebugBreak() * BAD_POINTER_X(P_U32, 0) = (U32) BAD_POINTER_X(P_U32, 0)
#endif /* OS */
#endif /* CHECKING */
#endif /* myDebugBreak */

#ifndef __crash_and_burn_here
#define __crash_and_burn_here() myDebugBreak()
#endif

#ifdef myassert0
#undef myassert0
#endif

#ifdef myassert1
#undef myassert1
#endif

#ifdef myassert2
#undef myassert2
#endif

#ifdef myassert3
#undef myassert3
#endif

#ifdef myassert4
#undef myassert4
#endif

#ifdef myassert5
#undef myassert5
#endif

#ifdef myassert6
#undef myassert6
#endif

#ifdef myassert7
#undef myassert7
#endif

#ifdef myassert8
#undef myassert8
#endif

#ifdef myassert9
#undef myassert9
#endif

#ifdef myassert10
#undef myassert10
#endif

#ifdef myassert11
#undef myassert11
#endif

#ifdef myassert12
#undef myassert12
#endif

#ifdef myassert13
#undef myassert13
#endif

#ifdef myassert0x
#undef myassert0x
#endif

#ifdef myassert1x
#undef myassert1x
#endif

#ifdef myassert2x
#undef myassert2x
#endif

#ifdef myassert3x
#undef myassert3x
#endif

#ifdef myassert4x
#undef myassert4x
#endif

#ifdef myassert5x
#undef myassert5x
#endif

#ifdef myassert6x
#undef myassert6x
#endif

#ifdef myassert7x
#undef myassert7x
#endif

#ifdef myassert9x
#undef myassert9x
#endif

#ifdef assert_EQ
#undef assert_EQ
#endif

#ifdef hard_assert
#undef hard_assert
#endif

#ifdef status_assert
#undef status_assert
#endif

#ifdef status_assertc
#undef status_assertc
#endif

#ifdef WrapOsBool
#undef WrapOsBool
#endif

#ifdef void_WrapOsBool
#undef void_WrapOsBool
#endif

#if !defined(_MSC_VER)
#ifndef __analysis_assume
#define __analysis_assume(expr) /*EMPTY*/
#endif
#endif /* _MSC_VER */

#if CHECKING

#define myassert0(str) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, TEXT("%s"), (str))) \
        __crash_and_burn_here(); }
#define myassert1(format, arg1) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1)) \
        __crash_and_burn_here(); }
#define myassert2(format, arg1, arg2) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2)) \
        __crash_and_burn_here(); }
#define myassert3(format, arg1, arg2, arg3) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3)) \
        __crash_and_burn_here(); }
#define myassert4(format, arg1, arg2, arg3, arg4) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4)) \
        __crash_and_burn_here(); }
#define myassert5(format, arg1, arg2, arg3, arg4, arg5) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5)) \
        __crash_and_burn_here(); }
#define myassert6(format, arg1, arg2, arg3, arg4, arg5, arg6) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6)) \
        __crash_and_burn_here(); }
#define myassert7(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7)) \
        __crash_and_burn_here();}
#define myassert8(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)) \
        __crash_and_burn_here(); }
#define myassert9(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)) \
        __crash_and_burn_here(); }
#define myassert10(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)) \
        __crash_and_burn_here(); }
#define myassert11(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)) \
        __crash_and_burn_here(); }
#define myassert12(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)) \
        __crash_and_burn_here(); }
#define myassert13(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) { \
    if(__myasserted(TEXT(__func__), TEXT(__FILE__), __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)) \
        __crash_and_burn_here(); }

#define myassert0x(exp, str) { \
    if_constant(!(exp)) \
        myassert0(      str) \
    __analysis_assume(exp); }
#define myassert1x(exp, format, arg1) { \
    if_constant(!(exp)) \
        myassert1(      format, arg1) \
    __analysis_assume(exp); }
#define myassert2x(exp, format, arg1, arg2) { \
    if_constant(!(exp)) \
        myassert2(      format, arg1, arg2) \
    __analysis_assume(exp); }
#define myassert3x(exp, format, arg1, arg2, arg3) { \
    if_constant(!(exp)) \
        myassert3(      format, arg1, arg2, arg3) \
    __analysis_assume(exp); }
#define myassert4x(exp, format, arg1, arg2, arg3, arg4) { \
    if_constant(!(exp)) \
        myassert4(      format, arg1, arg2, arg3, arg4) \
    __analysis_assume(exp); }
#define myassert5x(exp, format, arg1, arg2, arg3, arg4, arg5) \
    __analysis_assume(exp); { \
    if_constant(!(exp)) \
        myassert5(      format, arg1, arg2, arg3, arg4, arg5) }
#define myassert6x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6) { \
    if_constant(!(exp)) \
        myassert6(      format, arg1, arg2, arg3, arg4, arg5, arg6) \
    __analysis_assume(exp); }
#define myassert7x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) { \
    if_constant(!(exp)) \
        myassert7(      format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    __analysis_assume(exp); }
#define myassert9x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { \
    if_constant(!(exp)) \
        myassert9(      format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
    __analysis_assume(exp); }

#define assert_EQ(exp1, exp2) { \
    if(__myasserted_EQ(TEXT(__func__), TEXT(__FILE__), __LINE__, (U32) (exp1), (U32) (exp2))) \
        __crash_and_burn_here(); }

#define hard_assert(hard_mode) \
    __hard_assert(hard_mode)

#ifndef _status_assert_function_declared
#define _status_assert_function_declared 1
/*ncr*/
static inline STATUS
_status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     int line_no,
    _In_z_      PCTSTR tstr)
{
    if(status_ok(status) || (STATUS_CANCEL == status))
        return(status);

    return(__status_assert(status, p_function, p_file, line_no, tstr));
}
#endif /* _status_assert_function_declared */

#define status_assert(status) \
    status_consume(_status_assert(status, TEXT(__func__), TEXT(__FILE__), __LINE__, TEXT(#status)))

#define status_assertc(status) \
    status_assert(status)

#if WINDOWS

#define WrapOsBool(f) \
    __WrapOsBool(f, TEXT(__func__), TEXT(__FILE__), __LINE__, TEXT(#f))

#define void_WrapOsBool(f) \
    (void) __WrapOsBool(f, TEXT(__func__), TEXT(__FILE__), __LINE__, TEXT(#f))

#endif /* OS */

#else

#define myassert0(format)
#define myassert1(format, arg1)
#define myassert2(format, arg1, arg2)
#define myassert3(format, arg1, arg2, arg3)
#define myassert4(format, arg1, arg2, arg3, arg4)
#define myassert5(format, arg1, arg2, arg3, arg4, arg5)
#define myassert6(format, arg1, arg2, arg3, arg4, arg5, arg6)
#define myassert7(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define myassert8(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define myassert9(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define myassert10(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define myassert11(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define myassert12(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define myassert13(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)

#define myassert0x(exp, format) __analysis_assume(exp)
#define myassert1x(exp, format, arg1) __analysis_assume(exp)
#define myassert2x(exp, format, arg1, arg2) __analysis_assume(exp)
#define myassert3x(exp, format, arg1, arg2, arg3) __analysis_assume(exp)
#define myassert4x(exp, format, arg1, arg2, arg3, arg4) __analysis_assume(exp)
#define myassert5x(exp, format, arg1, arg2, arg3, arg4, arg5) __analysis_assume(exp)
#define myassert6x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6) __analysis_assume(exp)
#define myassert7x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) __analysis_assume(exp)
#define myassert9x(exp, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) __analysis_assume(exp)

#define assert_EQ(exp1, exp2)

#define hard_assert(hard_mode)

#define status_assert(status)   status_consume(status)
#define status_assertc(status)  /* rien - don't evaluate arg */

#if WINDOWS

#define WrapOsBool(f) (f)

#define void_WrapOsBool(f) (void) (f)

#endif /* OS */

#endif /* CHECKING */

/*
override normal C assert() usage
*/

#ifdef assert
#undef assert
#endif

#define assert(expr) \
    myassert0x(expr, TEXT(#expr))

#ifdef assert0
#undef assert0
#endif

#define assert0() \
    myassert0(TEXT("assert(FALSE)"))

#ifdef default_unhandled
#undef default_unhandled
#endif

#define default_unhandled() \
    myassert0(TEXT("Unhandled value in switch")) /*then often...*/ /*FALLTHRU*/

#ifdef PREFAST_ONLY_ASSERT
#undef PREFAST_ONLY_ASSERT
#endif

#if defined(_PREFAST_)
#define PREFAST_ONLY_ASSERT(expr) assert(expr)
#else
#define PREFAST_ONLY_ASSERT(expr) /* no expr */
#endif

/* end of myassert.h */
