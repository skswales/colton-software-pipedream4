/* ho_dll.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

#if RISCOS

/* Structure used when loading a module into memory.
 * This defines the entry and code areas taken up by the loading.
 */

typedef struct _runtime_info
{
    void (__cdecl * p_module_entry) (void);

    S32 code_size;
    P_ANY p_code;

    S32 reloc_size;
    void * reloc_flex; /* flex_alloc'ed */
}
RUNTIME_INFO, * P_RUNTIME_INFO;

typedef struct _stub_desc
{
    P_ANY p_stub_data;
}
STUB_DESC, * P_STUB_DESC;

extern STATUS
host_load_stubs(
    _In_z_      PCTSTR filename,
    _OutRef_    P_STUB_DESC p_stub_desc);

extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     S32 host_version,
    _In_        P_ANY p_stub_table);

extern void
host_discard_module(
    P_RUNTIME_INFO p_runtime_info);

#endif /* OS */

/* end of ho_dll.h */
