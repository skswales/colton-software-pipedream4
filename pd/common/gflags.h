/* gflags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

/*
gr_rdiag.c and numbers.c require this to be defined
*/

#define GR_RISCDIAG_WACKYTAG 0x23311881

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

/* RISC OS can check parameters for matching format */
#pragma check_printf_formats
extern void messagef(_In_z_ _Printf_format_string_ PCTSTR format, ...); /* to message box */
#pragma no_check_printf_formats
extern void message_output(_In_z_ PCTSTR buffer);

#endif /* __flags_h */

/* end of gflags.h */
