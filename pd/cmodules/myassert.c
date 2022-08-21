/* myassert.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Non-standard but helpful assertion */

/* SKS February 1991 */

#include "common/gflags.h"

#include "datafmt.h"

#if RISCOS
#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif
#endif

#if WINDOWS
#include <crtdbg.h>
#endif

#ifdef CHECKING
#undef CHECKING
#endif
#define CHECKING 1 /* allow CHECKING even in RELEASED code if really wanted */

#include "cmodules/myassert.h"

static S32
hard_assertion = 0;

#if WINDOWS && 0
hard_assertion = 1000; /* an OLE server really needs this */
#endif

/* on assertion failure, this procedure is called to report the failure in a system-dependent fashion */

#define ASSERTION_FAILURE_PREFIX TEXT("Runtime failure: %s in file %s, line ") U32_TFMT TEXT(".")

#if RISCOS
#define ASSERTION_FAILURE_YN TEXT("Click Continue to exit immediately, losing data, Cancel to attempt to resume execution.")
#elif WINDOWS
#define ASSERTION_FAILURE_YN TEXT("Click OK to exit immediately, losing data, Cancel to attempt to resume execution.")
#else
#define ASSERTION_FAILURE_YN TEXT("")
#endif

#if RISCOS
#define ASSERTION_FAILURE_PREFIX_RISCOS TEXT("Assert: ")
#endif

#if RISCOS
#define ASSERTION_TRAP_QUERY TEXT("Click Continue to trap to debugger, Cancel to exit normally")
#elif WINDOWS
#define ASSERTION_TRAP_QUERY TEXT("Click OK to trap to debugger, Cancel to exit normally")
#endif

extern BOOL __cdecl
__myasserted(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list va;
    BOOL crash_and_burn;

    va_start(va, format);
    crash_and_burn = __vmyasserted(p_function, p_file, line_no, NULL, format, va);
    va_end(va);

    return(crash_and_burn);
}

extern BOOL __cdecl
__myasserted_msg(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list va;
    BOOL crash_and_burn;

    va_start(va, format);
    crash_and_burn = __vmyasserted(p_function, p_file, line_no, message, format, va);
    va_end(va);

    return(crash_and_burn);
}

extern BOOL
__myasserted_EQ(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _InVal_     U32 val1,
    _InVal_     U32 val2)
{
    if(val1 == val2)
        return(FALSE);

    return(__myasserted(p_function, p_file, line_no, U32_TFMT TEXT("==") U32_TFMT, val1, val2));
}

extern BOOL
__vmyasserted(
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_opt_z_  PCTSTR message,
    _In_z_      PCTSTR format,
    /**/        va_list va_in)
{

#if RISCOS

    va_list va;
    _kernel_oserror err;
    PTSTR p = err.errmess;
    wimp_errflags flags;

    /* we need to know how to copy this! */
#if !RISCOS
    va = va_in;
#else
    va[0] = va_in[0];
#endif

    err.errnum = 0;

    /* test output string for overrun of wimp error box */
    if(!IS_P_DATA_NONE(format))
    {
        if(vsnprintf(p, elemof32(err.errmess), format, va) < 0)
            *p = CH_NULL;

        /* unbeknown to SKS, vsprintf modifies va, so reload necessary ... */
#if !RISCOS
        va = va_in;
#else
        va[0] = va_in[0];
#endif

        if(strlen(p) > 32)
        {
            /* make a hole to put the first prefix in */
            memmove32(p +  sizeof32(ASSERTION_FAILURE_PREFIX_RISCOS)-1 /*to*/, p /*from*/, strlen32(p) + 1);

            /* plonk it in */
            memcpy32(p, ASSERTION_FAILURE_PREFIX_RISCOS, sizeof32(ASSERTION_FAILURE_PREFIX_RISCOS)-1);

            (void) wimp_reporterror_rf(&err, wimp_EOK, product_ui_id(), &flags, NULL);

            format = NULL;
        }
    }

    p += xsnprintf(p, elemof32(err.errmess) - (p - err.errmess), ASSERTION_FAILURE_PREFIX TEXT(" - "), p_function, p_file, line_no);

    p += xsnprintf(p, elemof32(err.errmess) - (p - err.errmess), TEXT("%s"), message ? message : ASSERTION_FAILURE_YN);

    if(!IS_P_DATA_NONE(format))
    {
        *p++ = ' ';
        if(vsnprintf(p, elemof32(err.errmess) - (p - err.errmess), format, va) < 0)
            *p = CH_NULL;
    }

    (void) wimp_reporterror_rf(&err, (wimp_errflags) (wimp_EOK | wimp_ECANCEL | wimp_ENOERRORFROM), product_ui_id(), &flags, NULL);

    if(flags & wimp_EOK)
    {
        tstr_xstrkpy(err.errmess, elemof32(err.errmess), message ? message : ASSERTION_TRAP_QUERY);

        (void) wimp_reporterror_rf(&err, (wimp_errflags) (wimp_EOK | wimp_ECANCEL), product_ui_id(), &flags, NULL);

        if(flags & wimp_EOK)
            return(TRUE);

        exit(EXIT_FAILURE);
    }

#elif WINDOWS

    TCHARZ szBuffer[1024];
    size_t len = 0;
    UINT style = MB_TASKMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION;
    PTSTR p_out;

    len = _sntprintf_s(szBuffer, elemof32(szBuffer), _TRUNCATE,
                       ASSERTION_FAILURE_PREFIX TEXT("\n\n%s"),
                       p_function, p_file, line_no, message ? message : ASSERTION_FAILURE_YN);
    assert(len != (size_t) -1);

#if TRACE_ALLOWED
    p_out = szBuffer + len;
#endif

    if(!IS_P_DATA_NONE(format))
    {
        szBuffer[len++] = '\n';
        szBuffer[len++] = '\n';
        p_out = szBuffer + len;
        consume_int(_vsntprintf_s(p_out, elemof32(szBuffer) - len, _TRUNCATE, format, va_in));
    }

#if TRACE_ALLOWED
    trace_1(0xFFFFFFFFU, TEXT("!!! %s"), p_out);
#endif

    if(0 != hard_assertion)
    {
#if defined(_DEBUG)
        /* avoid putting up a simple message box in this state */
#if T5_UNICODE
        if(1 == _CrtDbgReportW(_CRT_ASSERT, p_file, line_no, product_id(), TEXT("%s"), szBuffer))
            return(FALSE);
#else
        if(1 == _CrtDbgReport(_CRT_ASSERT, p_file, line_no, product_id(), TEXT("%s"), szBuffer))
            return(FALSE);
#endif
        return(TRUE);
#else
        return(TRUE);
        /*PTSTR tstr;
        style = MB_SYSTEMMODAL | MB_OKCANCEL | MB_ICONHAND;
        tstr = szBuffer;
        while(tstrlen32(tstr) > 60)
        {
            tstr = tstrchr(tstr + 60, ' ');
            if(NULL == tstr)
                break;
            *tstr++ = 0x0D;
            *tstr++ = 0x0A;
        }*/
#endif /* _DEBUG */
    }

    hard_assertion++;

    if(IDOK == MessageBox(0, szBuffer, product_ui_id(), style))
    {
        PCTSTR message_query = message ? message : ASSERTION_TRAP_QUERY;

        if(IDOK == MessageBox(0, message_query, product_ui_id(), style))
        {
            hard_assertion--;

            return(TRUE);
        }

        FatalExit(1);
    }

    hard_assertion--;

#else

    fprintf(stderr, ASSERTION_FAILURE_PREFIX, p_file, line_no);

    if(!IS_P_DATA_NONE(format))
    {
        fputc(' ', stderr);
        vfprintf(stderr, format, va_in);
    }

    fputc('\n', stderr);

    fflush(stderr);

#endif /* OS */

    return(FALSE);
}

extern void
__hard_assert(
    _InVal_     BOOL hard_mode)
{
    if(hard_mode)
        hard_assertion++;
    else
        hard_assertion--;
}

extern STATUS
__status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR tstr)
{
    if(status_ok(status) || (STATUS_CANCEL == status))
        return(status);

    {
    PCTSTR tstr_s = string_lookup(status);
    if(__myasserted(p_function, p_file, line_no, tstr_s ? TEXT("%s: status = ") S32_TFMT TEXT(", %s") : TEXT("%s: status = ") S32_TFMT, tstr, status, tstr_s))
        __crash_and_burn_here();
    } /*block*/

    return(status);
}

#if WINDOWS

extern BOOL
__WrapOsBool(
    _InVal_     BOOL res,
    _In_z_      PCTSTR p_function,
    _In_z_      PCTSTR p_file,
    _InVal_     int line_no,
    _In_z_      PCTSTR tstr)
{
    if(res)
        return(res);

    {
    DWORD dwLastError = GetLastError();

    if(NO_ERROR != dwLastError)
    {
        PTSTR buffer = NULL;
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            dwLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &buffer,
            0,
            NULL);

        if(hard_assertion)
        {   /* best not to do assert message box in this state */
            trace_7(TRACE_OUT | TRACE_ANY,
                    TEXT("%s\n")
                    TEXT("FAILED: err=%d:0x%08X %s at %s(%d)"),
                    tstr,
                    dwLastError, dwLastError,
                    buffer ? buffer : TEXT("Error message unavailable"),
                    p_function, p_file, line_no);
        }
        else
        {
            if(__myasserted(p_function, p_file, line_no,
                            TEXT("%s\n")
                            TEXT("FAILED: err=%d:0x%08X %s"),
                            tstr,
                            dwLastError, dwLastError,
                            buffer ? buffer : TEXT("Error message unavailable")))
                __crash_and_burn_here();

        }

        if(buffer) LocalFree(buffer);
    }
    } /*block*/

    return(res);
}

#endif

/* end of myassert.c */
