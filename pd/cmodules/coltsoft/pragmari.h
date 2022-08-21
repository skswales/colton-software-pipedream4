/* pragmari.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Controls pragmas for RISC OS Norcroft compiler */

/* Stuart K. Swales 15-Jan-1991 */

/* Designed for multiple inclusion */

#if defined(__CC_NORCROFT)

/* --- stack overflow checking --- */

#ifdef    PRAGMA_CHECK_STACK
#undef    PRAGMA_CHECK_STACK
#ifdef  __PRAGMA_CHECK_STACK_ON
#define   PRAGMA_CHECK_STACK_OFF
#else
#define   PRAGMA_CHECK_STACK_ON
#endif
#endif

#ifdef    PRAGMA_CHECK_STACK_ON
#undef    PRAGMA_CHECK_STACK_ON
#pragma check_stack
#define __PRAGMA_CHECK_STACK_ON
#endif

#ifdef    PRAGMA_CHECK_STACK_OFF
#undef    PRAGMA_CHECK_STACK_OFF
#pragma no_check_stack
#undef  __PRAGMA_CHECK_STACK_ON
#endif

/* --- CSE optimisation --- */

#ifdef    PRAGMA_SIDE_EFFECTS
#undef    PRAGMA_SIDE_EFFECTS
#pragma side_effects
#endif

#ifdef    PRAGMA_SIDE_EFFECTS_OFF
#undef    PRAGMA_SIDE_EFFECTS_OFF
#pragma no_side_effects
#endif

#endif /* __CC_NORCROFT */

/* end of pragmari.h */
