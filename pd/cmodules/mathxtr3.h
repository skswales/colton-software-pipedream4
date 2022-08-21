/* mathxtr3.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2014 Stuart Swales */

/* SKS May 2012 */

#ifndef __mathxtra_h
#define __mathxtra_h

/*
function declarations
*/

/*
'random' numbers
*/

extern F64
normal_distribution(void);

extern F64
uniform_distribution(void);

extern void
uniform_distribution_seed(
    _In_ unsigned int seed);

#endif /* __mathxtr3_h */

/* end of mathxtr3.h  */
