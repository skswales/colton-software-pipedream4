/* mathxtr3.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2025 Stuart Swales */

/* Additional math routines */

/* SKS May 2012 */

#ifndef MATHXTR3_H
#define MATHXTR3_H

/*
exported functions
*/

/*
'random' numbers
*/

_Check_return_
extern double
normal_distribution(void);

/* Generates a random number on [0,1) */ 

_Check_return_
extern double
uniform_distribution(void);

extern void
uniform_distribution_seed(
    _In_    const unsigned int seed);

/*ncr*/
extern bool
uniform_distribution_test_seeded(
    _In_    const bool fEnsure);

#endif /* MATHXTR3_H */

/* end of mathxtr3.h  */
