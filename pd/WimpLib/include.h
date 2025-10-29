/* include.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef INCLUDE_H
#define INCLUDE_H

#if defined(SB_GLOBAL_REGISTER)
/*extern*/ __global_reg(6) struct DOCU * current_p_docu; /* keep current_p_docu in v6 register (so don't let WimpLib corrupt it) */
#endif

#define COMPILING_WIMPLIB 1

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

#define SKS_ACW 1 /* Use custom RISC_OSLib (WimpLib) */

#if RELEASED && !defined(DEBUG)
/* Turn off assert checking */
#define NDEBUG

#if !defined(TRACE_IN_RELEASE)
/* Turn off any trace information */
#define TRACE_ALLOWED 0
#else
#define TRACE_ALLOWED 1
#endif

#endif /* RELEASED */

#define TRACE TRACE_ALLOWED


#define PDACTION 1
#define DBOX_MOTION_UPDATES 1


#if !CROSS_COMPILE
#define NORCROFT_INLINE_ASM 1
#if __CC_NORCROFT_VERSION >= 590
#define NORCROFT_INLINE_SWIX 1
#endif
#endif /* CROSS_COMPILE */


/* standard includes for WimpLib */

#include "cmodules/coltsoft/ansi.h"

#include "cmodules/coltsoft/coltsoft.h"

/* useful max/min macros */
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* always redirect WimpLib allocs */
#include "cmodules/riscos/wlalloc.h"

/* RISC OS can check parameters for matching format */
#if !CROSS_COMPILE
#pragma check_printf_formats
#endif
extern void messagef(_In_z_ _Printf_format_string_ PCTSTR format, ...); /* to message box */
#if !CROSS_COMPILE
#pragma no_check_printf_formats
#endif
extern void message_output(_In_z_ PCTSTR buffer);

#if defined(INCLUDE_FOR_NO_DELTA) && 0
/* just let them include what they need */
#else

#include "trace.h" /* NB this replaces RISC_OSLib trace.h */

#include "cmodules/myassert.h"

#include "cmodules/report.h"

#include "cmodules/aligator.h"

#ifndef            XTSTRING_H
#include "cmodules/xstring.h" /* for safe-string routines */
#endif

#define EXPOSE_RISCOS_SWIS 1
#include "cs-kernel.h" /* C: */

#define _XOS(swi_no) ((swi_no) | (1U << 17))

#ifndef __os_h
#include "os.h"
#endif

#ifndef   CS_WIMP_H
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef   CS_FONT_H
#include "cs-font.h"    /* includes font.h -> os.h, drawmod.h, wimp.h */
#endif

#include "cmodules/riscos/osfile.h"

#ifndef   CS_AKBD_H
#include "cs-akbd.h"    /* includes akbd.h */
#endif

#ifndef   CS_BBCX_H
#include "cs-bbcx.h"    /* includes bbc.h */
#endif

#ifndef   CS_DBOX_H
#include "cs-dbox.h"    /* includes dbox.h */
#endif

#ifndef   CS_EVENT_H
#include "cs-event.h"   /* includes event.h -> menu.h, wimp.h */
#endif

#ifndef   CS_FILEICON_H
#include "cs-fileicon.h" /* includes fileicon.h -> wimp.h */
#endif

#ifndef   CS_FLEX_H
#include "cs-flex.h"    /* includes flex.h */
#endif

#ifndef   CS_MENU_H
#include "cs-menu.h"    /* includes menu.h */
#endif

#ifndef   CS_MSGS_H
#include "cs-msgs.h"    /* includes msgs.h */
#endif

#ifndef __os_h
#include "os.h"         /* no includes */
#endif

#ifndef __print_h
#include "print.h"      /* no includes */
#endif

#ifndef __res_h
#include "res.h"        /* includes <stdio.h> */
#endif

#ifndef __resspr_h
#include "resspr.h"     /* includes sprite.h */
#endif

#ifndef   CS_XFERSEND_H
#include "cs-xfersend.h" /* includes xfersend.h */
#endif

#ifndef   CS_TEMPLATE_H
#include "cs-template.h" /* includes template.h */
#endif

#ifndef __werr_h
#include "werr.h"       /* includes <stdarg.h> */
#endif

#ifndef   CS_WIMP_H
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef   CS_WIMPT_H
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef   CS_WINX_H
#include "cs-winx.h"    /* includes win.h */
#endif

#if TRACE
/*#error barf*/
#endif

#endif /* INCLUDE_FOR_NO_DELTA */

#endif  /* INCLUDE_H */

/* end of include.h */
