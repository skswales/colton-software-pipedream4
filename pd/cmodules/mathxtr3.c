/* mathxtr3.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2014 Stuart Swales */

/* Additional math routines */

/* SKS May 2012 */

#include "common/gflags.h"

#include "cmodules/mathxtr3.h"

/*
'random' numbers
*/

/*
Polar method of generating (paired) normal random variables.
Bit faster than Box-Muller transform that uses trig functions.

See review of such methods in
Gaussian Random Number Generators,
D. Thomas and W. Luk and P. Leong and J. Villasenor,
ACM Computing Surveys, Vol. 39(4), Article 11, 2007,
doi:10.1145/1287620.1287622
*/

extern F64
normal_distribution(void)
{
    static BOOL has_paired_deviate = FALSE;
    static F64 paired_deviate = 0.0;

    F64 x, y;
    F64 s;

    /* is one stashed already for our use? */
    if(has_paired_deviate)
    {
        has_paired_deviate = FALSE;
        return(paired_deviate);
    }

    do {
        x = uniform_distribution() * 2 - 1; /* -> [-1,+1] */
        y = uniform_distribution() * 2 - 1;

        s = (x * x) + (y * y);

        if(s == 0.0)
            continue;

    } while(s >= 1.0); /* reject if magnitude exceeds unit circle (about pi/4 samples get rejected) */

    /* stash this for next time round */
    paired_deviate = y * sqrt(-2.0 * log(s) / s);
    has_paired_deviate = TRUE;

    return(x * sqrt(-2.0 * log(s) / s));
}

#if WINDOWS

/*

Use double precision SIMD-oriented Fast Mersenne Twister (dSFMT)

http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html

(BSD) Licence information for this code is stored along with
the source code in the external/OtherBSD/dSFMT-src-* directory

*/

#if 1 /* use dSFMT */

#ifdef assert
#undef assert /* as it's contrarily included by dSFMT source */
#endif

#define inline __forceinline

#include "external/OtherBSD/dSFMT-src-2.2/dSFMT.c"

extern F64
uniform_distribution(void)
{
    /* Generates a random number on [0,1) */ 
    return(dsfmt_gv_genrand_close_open());
}

#else /* use SFMT */

#define MEXP 19937

#ifdef _M_IX86
#if _M_IX86_FP >= 2
#define HAVE_SSE2 1
#endif
#endif /* _M_IX86 */

#pragma warning(disable:4244) /* 'return' : conversion from 'long double' to 'double', possible loss of data */

#include "external/OtherBSD/SFMT-src-1.3.3/sfmt.c"

extern F64
uniform_distribution(void)
{
    /* Generates a random number on [0,1) with 53-bit resolution */ 
    return(genrand_res53());
}

#endif

extern void
uniform_distribution_seed(
    _In_ unsigned int seed)
{
    init_gen_rand(seed);
}

#else

/* implementation using ANSI C functions */

extern F64
uniform_distribution(void)
{
    int temp = rand() & RAND_MAX;
    return((F64) temp / (F64) RAND_MAX);
}

extern void
uniform_distribution_seed(
    _In_ unsigned int seed)
{
    srand(seed);
}

#endif /* OS */

/* end of mathxtr3.c */
