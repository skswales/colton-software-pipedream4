/* trace.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2011-2020 Stuart Swales */

#ifndef __trace_h
#define __trace_h /* replaces RISC_OSLib trace.h so suppress that */

#include "cmodules/debug.h"

#ifndef tracef0
#define tracef0(s) trace_0(TRACE_RISCOS_HOST, s)
#define tracef1(s, a) trace_1(TRACE_RISCOS_HOST, s, a)
#define tracef2(s, a, b) trace_2(TRACE_RISCOS_HOST, s, a, b)
#define tracef3(s, a, b, c) trace_3(TRACE_RISCOS_HOST, s, a, b, c)
#define tracef4(s, a, b, c, d) trace_4(TRACE_RISCOS_HOST, s, a, b, c, d)
#define tracef5(s, a, b, c, d, e) trace_5(TRACE_RISCOS_HOST, s, a, b, c, d, e)
#define tracef6(s, a, b, c, d, e, f) trace_6(TRACE_RISCOS_HOST, s, a, b, c, d, e, f)
#define tracef7(s, a, b, c, d, e, f, g) trace_7(TRACE_RISCOS_HOST, s, a, b, c, d, e, f, g)
#endif

#endif

/* end of trace.h */
