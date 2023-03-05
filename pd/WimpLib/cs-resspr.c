/* cs-resspr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2023 Stuart Swales */

#include "include.h"

#include "cs-resspr.h"

static void
resspr_report_attempt(
    _In_z_      char * file_name)
{
    reportf("locatesprites trying %u:%s", _inl_strlen32(file_name), file_name);
}

_Check_return_
static BOOL
resspr_locatesprites(
    _In_z_      const char *leaf_name,
    _Out_       char * file_name,
    _InVal_     size_t elemof_file_name) /* needs to be long enough for <ProgramName$Dir>.<Wimp$IconTheme>SpritesXX */
{
    char prefixed_leaf_name[64];
    _kernel_oserror *e;
    int bufferspace = 0;
    const int wimp_extend_code = 13;

    if(wimptx_os_version_query() < RISC_OS_5)
        goto ye_olde_way;

    strcpy(prefixed_leaf_name, "<Wimp$IconTheme>");
    strcat(prefixed_leaf_name, leaf_name);
    res_prefixnamewithpath(prefixed_leaf_name, file_name);
    resspr_report_attempt(file_name);
    e = _swix(Wimp_Extend, _INR(0,3)|_OUT(3), wimp_extend_code, file_name, file_name, elemof_file_name, &bufferspace);
    // reportf("locatesprites0: e %s f %s (s %u b %u)", e?e->errmess:"-", file_name, elemof_file_name, bufferspace);

    if(bufferspace != elemof_file_name) /* Wimp_Extend 13 is supported on this Wimp (i.e. it has modified R3) */
    {
        if(e)
        {
            res_prefixnamewithdir(prefixed_leaf_name, file_name);
            resspr_report_attempt(file_name);
            e = _swix(Wimp_Extend, _INR(0,3), wimp_extend_code, file_name, file_name, elemof_file_name);
            // reportf("locatesprites2: e %s f %s", e?e->errmess:"-", file_name);
        }

        if(e)
        {
            strcpy(prefixed_leaf_name, leaf_name);
            res_prefixnamewithpath(prefixed_leaf_name, file_name);
            resspr_report_attempt(file_name);
            e = _swix(Wimp_Extend, _INR(0,3), wimp_extend_code, file_name, file_name, elemof_file_name);
            // reportf("locatesprites3: e %s f %s", e?e->errmess:"-", file_name);
        }

        if(e)
        {
            res_prefixnamewithdir(prefixed_leaf_name, file_name);
            resspr_report_attempt(file_name);
            e = _swix(Wimp_Extend, _INR(0,3), wimp_extend_code, file_name, file_name, elemof_file_name);
            // reportf("locatesprites4: e %s f %s", e?e->errmess:"-", file_name);
        }

        if(!e) /* Wimp_Extend 13 indicates whether file it suggests has actually been found unlike code 257 */
        {
            reportf("locate_sprites FOUND %u:%s", strlen(file_name), file_name);
            return(TRUE);
        }
    }

ye_olde_way:;/* fall back to old code in case of old Wimp or failure */

    res_prefixnamewithpath(leaf_name, file_name);

#if defined(NORCROFT_INLINE_ASM)
    const char *mode;
    __asm {
        MOV     R0, #2;
        SVC     #Wimp_ReadSysInfo, /*in*/ {R0}, /*out*/ {R0,PSR}, /*corrupted*/ {};
        MOV     mode, R0;
    }
#else /* NORCROFT_INLINE_SWIX_NOT_YET */
    const char *mode = (const char *)_swi(Wimp_ReadSysInfo, _IN(0), 2);
#endif

    /* Mode 24 is the default mode, ignore it */
    if(strcmp(mode, "24"))
        strcat(file_name, mode);

    resspr_report_attempt(file_name);
    return(TRUE); /* try to load this, even though we haven't checked here */
}

#include "resspr.c"

_Check_return_
static BOOL
resspr_attemptmerge(
    _In_z_      const char * file_name)
{
    sprite_area * new_area;
    int new_area_size = resspr__area->size;
    int file_size;

    int fh = _kernel_osfind(0x47, de_const_cast(char *, file_name));
    if(0 == fh)
        return(FALSE);
    file_size = _kernel_osargs(2, fh, 0);
    (void) _kernel_osfind(0, (char *) fh);
    reportf("resspr_attemptmerge(%u:%s): length=%u\n", strlen(file_name), file_name, file_size); 

    /* realloc the existing sprite area so that a merge can be attempted */
    new_area_size += file_size;
    new_area = malloc(new_area_size); /* was realloc(resspr__area, new_area_size); */
    if(NULL == new_area)
    {
        // reportf("resspr_attemptmerge(%u:%s): NOMEM - FAILED\n", strlen(file_name), file_name); 
        return(FALSE);
    }
    /* leave the old copy in place - iconbar sprite, toolbar icons etc. will be pointing to the old area */
    (void) memcpy(new_area, resspr__area, resspr__area->size);

    new_area->size = new_area_size; /* loads of space now on the end */
    resspr__area = new_area;

#ifdef NORCROFT_INLINE_SWIX
    if(NULL != _swix(OS_SpriteOp, _INR(0,2), 
        /*r[0]*/ 11 + 256 /* Merge (user area) */,
        /*r[1]*/ resspr__area,
        /*r[2]*/ file_name))
#else
    _kernel_swi_regs rs;
    rs.r[0] = 11 + 256; /* Merge (user area) */
    rs.r[1] = (int) resspr__area;
    rs.r[2] = (int) file_name;
    if(NULL != cs_kernel_swi(OS_SpriteOp, &rs))
#endif
    {
        reportf("resspr_attemptmerge(%u:%s): MERGE FAILED\n", strlen(file_name), file_name); 
        return(FALSE); /* merge failed */
    }

    reportf("resspr_attemptmerge(%u:%s): MERGE OK\n", strlen(file_name), file_name); 
    return(TRUE); /* merge succeeded */
}

_Check_return_
extern BOOL
resspr_mergesprites(
    _In_z_      const char * leaf_name)
{
    char file_name[60]; /* long enough for <ProgramName$Dir>.<Wimp$IconTheme>SpritesXX */

    if(NULL == resspr__area)
        return(FALSE); /* failed */ /* must have had something loaded already */

    if(!resspr_locatesprites(leaf_name, file_name, sizeof(file_name)))
        return(FALSE); /* failed */

    if(!resspr_attemptmerge(file_name))
        return(FALSE); /* failed */

    return(TRUE);
}

/* end of cs-resspr.c */
