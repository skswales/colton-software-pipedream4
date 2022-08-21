/* stringlk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* System-independent string lookup */

/* SKS July 91 */

#include "common/gflags.h"

#include "stringlk.h"

#include "strings.h"

#include "msgs.h"

#ifndef MSGS_LOOKUP_DELIMITER_STR
#define MSGS_LOOKUP_DELIMITER_STR "_"
#endif

extern PC_USTR
string_lookup(
    S32 stringid)
{
    S32    base, increment, moduleno;
    char   m_or_e;
    char   tagbuffer[sizeof("e1234_999")];
    const char * str;

    if(stringid > 0)
        {
        /* message number */
        base      = +MODULE_MSG_BASE;
        increment = +MODULE_MSG_INCREMENT;
        m_or_e    = 'm';
        }
    else if(stringid < 0)
        {
        /* error number */
        stringid  = -stringid;
        base      = -MODULE_ERR_BASE;
        increment = +MODULE_ERR_INCREMENT;
        m_or_e    = 'e';
        }
    else
        return(NULL);

    if(stringid >= base)
        {
        /* module range */
        moduleno = stringid / increment;
        stringid = stringid % increment;
        (void) sprintf(tagbuffer, "%c%d" MSGS_LOOKUP_DELIMITER_STR "%d", m_or_e, moduleno, stringid);
        }
    else
        {
        /* application range */
        (void) sprintf(tagbuffer, "%c%d", m_or_e, stringid);
        }

    str = msgs_lookup(tagbuffer);
    if(str == tagbuffer)
        {
        /* lookup failed */
        return(NULL);
        }

    return(str);
}

extern void
string_lookup_buffer(
    S32 stringid,
    _Out_writes_z_(elemof_buffer) char * buffer,
    _InVal_     U32 elemof_buffer)
{
    PC_USTR str = string_lookup(stringid);

    void_strkpy(buffer, elemof_buffer, str ? str : NULLSTR);
}

/* end of stringlk.c */
