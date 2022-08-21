/* gflags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the compile-time flags for PipeDream */

/* Stuart K. Swales 14-Feb-1989 */

#ifndef __flags_h
#define __flags_h

#ifdef RELEASED
#undef UNRELEASED
#endif

#ifdef UNRELEASED
#undef UNRELEASED /* no further testing on this */
#define RELEASED 0
#else
#define RELEASED 1
#endif

#ifndef CHECKING
#if RELEASED
/* #error   Default state for RELEASED is CHECKING OFF */
#define CHECKING 0
#else
/* #error   Default state for DEBUG is CHECKING ON */
#define CHECKING 1
#endif
#endif /* CHECKING */

/* target machine type - mutually exclusive options */

#define RISCOS  1
#define WINDOWS 0

#include "cmodules/coltsoft/defineos.h"

#include "personal.h"

/* ------------------ features that are now standard --------------------- */

#define SKS_ACW 1 /* Use custom RISC_OSLib (WimpLib) */

#define ALIGATOR_USE_ALLOC 1

#define COMPLEX_STRING 1

#define EXTENDED_COLOUR 1 /* BBGGRR1n or 0000000n */

/*
gr_rdiag.c and numbers.c require these to be defined
*/

#define GR_RISCDIAG_TAG_PD_CHART_CODE_LEGACY 0x23311881U

#define GR_RISCDIAG_TAG_PD_CHART_CODE 0x00001500U

/* -------------- new features, not in release version yet --------------- */

#if !RELEASED
/*#define SKS_FONTCOLOURS 1- nobody bar SKS likes this */
#endif

/* ------------- checking features, never in release version ------------- */

#if !RELEASED
#define SPELL_VERIFY_DICT 1
#endif

/* ---------- paranoid checks to ensure kosher release version ----------- */

#if RELEASED

#if !CHECKING
#define NDEBUG 1
#endif

#endif /* RELEASED */

/* standard header files after all flags defined */
#include "include.h"

/* certain bits of PipeDream exported for cmodules use */

_Check_return_
_Ret_z_
extern PC_USTR
product_ui_id(void);

#if defined(__CC_NORCROFT) /* this can check parameters for matching format */
#pragma check_printf_formats
#endif

extern void messagef(_In_z_ _Printf_format_string_ PCTSTR format, ...); /* to message box */

#if defined(__CC_NORCROFT)
#pragma no_check_printf_formats
#endif

extern void message_output(_In_z_ PCTSTR buffer);

/* These are useful for wrapping up calls to system functions
 * and distinguishing our own errors from those emanating from
 * the RISC_OSLib wimpt_xxx API.
 */

/*ncr*/
extern _kernel_oserror *
__WrapOsErrorChecking(
    _In_opt_    _kernel_oserror * const p_kernel_oserror,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr);

/*ncr*/
extern BOOL
reperr_kernel_oserror(
    _In_        _kernel_oserror * const p_kernel_oserror);

/*ncr*/
static inline _kernel_oserror *
WrapOsErrorReporting(
    _In_opt_    _kernel_oserror * p_kernel_oserror)
{
    if(NULL != p_kernel_oserror)
        (void) reperr_kernel_oserror(p_kernel_oserror);

    return(p_kernel_oserror);
}

static inline void
void_WrapOsErrorReporting(
    _In_opt_    _kernel_oserror * p_kernel_oserror)
{
    if(NULL != p_kernel_oserror)
        (void) reperr_kernel_oserror(p_kernel_oserror);
}

#ifndef sbchar_isdigit
#define sbchar_isdigit /*"C"*/isdigit
#endif
#ifndef sbchar_isupper
#define sbchar_isupper /*"C"*/isupper
#endif
#ifndef sbchar_tolower
#define sbchar_tolower /*"C"*/tolower
#endif

#endif /* __flags_h */

/* end of gflags.h */
