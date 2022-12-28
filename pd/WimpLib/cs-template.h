/* cs-template.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __cs_template_h
#define __cs_template_h

/* SKS added to support new template mechanism */

#ifndef __wimp_h
#include "wimp.h"
#endif

#define template do_not_use_template

#define template_copy do_not_use_template_copy

#define template_find do_not_use_template_find

#ifndef __template_h
#include "template.h"
#endif

#undef template

#undef template_readfile

/* This now has to be an abstract handle onto a template */
typedef struct _template *template;

extern WimpWindowWithBitset *
template_copy_new(template *templateHandle);

/* mechanism for disposing of the above copy and its workspace */
extern void
template_copy_dispose(WimpWindowWithBitset ** ppww);

extern BOOL
template_ensure(const unsigned int idx);

extern template *
template_find_new(const char * name);

extern void
template_readfile(const char * name);

extern void
template_require(const unsigned int idx, const char * name);

/* Set the title of the underlying window definition */
extern void
template_settitle(template * templateHandle, const char * title);

extern WimpWindowWithBitset *
template_syshandle_unaligned(const char *name);

#endif /* __cs_template_h */

/* end of cs-template.h */
