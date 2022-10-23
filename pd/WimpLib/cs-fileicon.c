/* cs-fileicon.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2022 Stuart Swales */

#include "include.h"

#undef BOOL
#undef TRUE
#undef FALSE

#include "fileicon.c"

extern void
fileicon_name(char * icon_name /*[12]*/, int filetype)
{
    if(filetype == 0x1000)
        strcpy(icon_name, "Directory");
    else if(filetype == 0x2000)
        strcpy(icon_name, "Application");
    else
        sprintf(icon_name, "file_%03x", filetype);

    /* now to check if the sprite exists. */
    /* do a sprite_select on the Wimp sprite pool */
    if(wimp_spriteop(24, icon_name) == 0)
    {   /* the sprite exists: all is well */
        /*EMPTY*/
    }
    else
    {   /* the sprite does not exist: use general don't-know icon. */
        strcpy(icon_name, "file_xxx");
    }
}

/* cs-fileicon.c */
