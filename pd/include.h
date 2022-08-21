/* include.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __include_h
#define __include_h

#if defined(SB_GLOBAL_REGISTER)
/*extern*/ __global_reg(6) struct DOCU * current_p_docu; /* keep current_p_docu in APCS v6 register (== ARM r9) */
#endif

/* standard includes for all programs */

#include "cmodules/coltsoft/ansi.h"

#include "cmodules/coltsoft/coltsoft.h"

#include "cmodules/coltsoft/drawxtra.h"

#include "cmodules/debug.h"
#include "cmodules/myassert.h"

typedef U8 DOCNO; typedef DOCNO * P_DOCNO; /* NB now exactly the same as EV_DOCNO */

#define DOCNO_NONE    ((DOCNO) 0)
#define DOCNO_FIRST   ((DOCNO) 1)
#define DOCNO_MAX     ((DOCNO) 255) /* 1..254 valid */
#define DOCNO_SEVERAL ((DOCNO) 255)

#include "cmodules/aligator.h"
#include "cmodules/alloc.h"
#include "cmodules/allocblk.h"
#include "cmodules/quickblk.h"
#include "cmodules/handlist.h"
#include "cmodules/ev_eval.h"
#include "cmodules/file.h"
#include "cmodules/im_cache.h"
#include "cmodules/gr_coord.h"
#include "cmodules/gr_chart.h"
#include "cmodules/monotime.h"
#include "cmodules/muldiv.h"
#include "cmodules/numform.h"
#include "cmodules/report.h"
#include "cmodules/xstring.h"

#if defined(SB_GLOBAL_REGISTER)
extern struct DOCU * current_p_docu_backup;
#define current_p_docu_global_register_assign(p_docu_new) \
    current_p_docu = current_p_docu_backup = (p_docu_new)
#define current_p_docu_global_register_restore_from_backup() \
    current_p_docu = current_p_docu_backup
#define current_p_docu_global_register_stash_block_start() \
    { \
    struct DOCU * current_p_docu_stash = current_p_docu
#define current_p_docu_global_register_stash_block_end() \
    current_p_docu = current_p_docu_stash; \
    } \
    /*EMPTY*/
#else
extern struct DOCU * current_p_docu; /* keep current_p_docu in global data (no backup needed) */
#define current_p_docu_global_register_assign(p_docu_new) \
    current_p_docu = (p_docu_new)
#define current_p_docu_global_register_restore_from_backup() /*EMPTY*/
#define current_p_docu_global_register_stash_block_start() /*EMPTY*/
#define current_p_docu_global_register_stash_block_end() /*EMPTY*/
#endif

#endif  /* __include_h */

/* end of include.h */
