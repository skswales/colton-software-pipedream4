/* fontlxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __fontlxtr_h
#define __fontlxtr_h

#ifndef __fontlist_h
#include "fontlist.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

/* --------------------------- fontlist_ensure_all_fonts -----------------------------
 * Description:   Read in the font list into a font tree
 *
 * Parameters:    BOOL * ensure -- TRUE to force (re)cache, FALSE to automatically
 *                                 detect need for recache
 * Returns:       a pointer to the start of the font tree,
 *                BOOL * ensure -- TRUE if recache performed
 * Other Info:
 *                with multiple clients of fontlist it is impossible to keep System
 *                in the client desired state. henceforth System is ALWAYS included
 *                as FontSelect forces this upon us.
 */

extern fontlist_node *
fontlist_ensure_all_fonts(
    BOOL * ensure /*inout*/);

/* -------------------------- fontselect_ensure_all_fonts ----------------------------
 * Description:   Read in the font list into a font tree and reinitialise font selector
 *
 * Returns:       whether font selector is initialised
*/

extern BOOL
fontselect_ensure_all_fonts(void);

/* ------------------------------ fontselect_check_open ------------------------------
 * Description:   Test whether the font selector is still open. If not open but hidden
 *                or on error then fontselect_closewindows() is called and the handle
 *                zeroed.
 *
 * Returns:       whether font selector is still visible
*/

extern BOOL
fontselect_check_open(
    wimp_w * w /*inout*/);

/* ------------------------------ fontlist_enumerate ---------------------------------
 * Description:   Call a user-supplied enumeration routine with fonts from a font tree
 *
 * Parameters:    BOOL system   -- TRUE if the weightless System font should be included
 *                in the enumeration. This is the only interface that a client of fontlist
 *                can truly select this without destroying the integrity of the single font tree
 *
 * Returns:       number of fonts enumerated
*/

typedef void (* fontlist_enumproc) (
    void * enumhandle,
    const char * szFontName,
    int seqno);

extern int
fontlist_enumerate(
    BOOL              system,
    fontlist_node *   fontlist,
    fontlist_enumproc enumproc,
    void *            enumhandle);

/* --------------------------- fontselect_process -----------------------------
 * Description:   Process the font chooser window.
 *
 * Parameters:    const char * title       -- The title for the window (can be NULL if flags SETTITLE is clear)
 *                int flags                -- The flags for the call (see below)
 *                const char * font_name   -- The font name to set the window contents to (only if SETFONT is set)
 *                const double * width     -- The width in point size of the font
 *                const double * height    --
 *                FONTSELECT_INIT_FN init_fn
 *                void *             init_handle
 *                FONTSELECT_TRY_FN try_me
 *                void *            try_handle
 *
 * Returns:       If the function call was successful it returns 1.
 *                Otherwise it returns 0.
 *
 * Other Info:    The flags word allows the call to have different effects.
 *                    If fontselect_SETFONT is set then the window contents wil be updated to reflect the font choice passed in
 *                    If fontselect_SETTITLE is set then the title of the window will be set, otherwise title is ignored.
 */

typedef void (* FONTSELECT_INIT_FN) (
    wimp_w fontselect_wind,
    void * init_handle);

typedef BOOL /* processed */ (* FONTSELECT_TRY_FN) (
    const char * font_name,
    PC_F64 width,
    PC_F64 height,
    const wimp_eventstr * e,
    void * try_handle,
    BOOL try_anyway);

extern BOOL
fontselect_process(
    const char * title,
    int flags,
    const char * font_name,
    PC_F64 width,
    PC_F64 height,
    FONTSELECT_INIT_FN init_fn,
    void * init_handle,
    FONTSELECT_TRY_FN  try_me,
    void * try_handle);

/* ----------------------- fontselect_prepare_process -------------------------
 * Description:   Prepares for processing the font chooser window.
 *
 * Returns:       If fontselect_process may be called it returns 1.
 *                Otherwise it returns 0.
*/

extern BOOL
fontselect_can_process(void);

extern BOOL
fontselect_prepare_process(void);

#endif /* fontlxtr.h */

/* end of fontlxtr.h */
