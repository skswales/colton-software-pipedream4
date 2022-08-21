/* cs-template.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#ifndef __cs_template_h
#define __cs_template_h

/* SKS added to support new template mechanism */

#ifndef __wimp_h
#include "wimp.h"
#endif

#define template do_not_use_template

#ifndef __template_h
#include "template.h"
#endif

#undef template

/* This now has to be an abstract handle onto a template */
typedef struct _template *template;

#define template_copy do_not_use_template_copy

extern wimp_wind *
template_copy_new(template *templateHandle);

/* mechanism for disposing of the above copy and its workspace */
extern void
template_copy_dispose(wimp_wind ** ppww);

#define template_find do_not_use_template_find

extern template *
template_find_new(const char * name);

#undef template_readfile

extern void
template_readfile(const char * name);

extern void
template_require(int i, const char * name);

/* Set the title of the underlying window definition */
extern void
template_settitle(template * templateHandle, const char * title);

extern wimp_wind *
template_syshandle_ua(const char *name);

#endif /* __cs_template_h */

/* end of cs-template.h */
