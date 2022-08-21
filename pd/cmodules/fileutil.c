/* fileutil.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* File handling module (utils section) */

/* SKS June 1991 */

#include "common/gflags.h"

#if RISCOS
#include "kernel.h" /*C:*/
#include "swis.h" /*C:*/

#include "cmodules/riscos/osfile.h"

#endif

#include "cmodules/file.h"

/*
internal functions
*/

static P_U8
file__make_usable_dir(
    PC_U8 dirname,
    char * buffer /*out*/,
    _InVal_     U32 elemof_buffer);

/*
places to look for files
*/

static char *search_path = NULL;

/******************************************************************************
*
* if a filename is rooted, use that
* otherwise add on a suitable prefix
*
******************************************************************************/

extern char *
file_add_prefix_to_name(
    char * destfilename /*out*/,
    _InVal_     U32 elemof_buffer,
    PC_U8 srcfilename,
    PC_U8 currentfilename)
{
    *destfilename = NULLCH;

    if(!file_is_rooted(srcfilename))
        file_get_prefix(destfilename, elemof_buffer, currentfilename);

    safe_strkat(destfilename, elemof_buffer, srcfilename);

    return(destfilename);
}

/******************************************************************************
*
* obtain a path composed of the current filename and search path
*
******************************************************************************/

extern char *
file_combined_path(
    char * destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename)
{
    *destpath = NULLCH;

    file_get_cwd(destpath, elemof_buffer, currentfilename);

    if(search_path && *search_path)
        {
        if(*destpath)
            safe_strkat(destpath, elemof_buffer, FILE_PATH_SEP_STR);

        safe_strkat(destpath, elemof_buffer, search_path);
        }

    return(*destpath ? destpath : NULL);
}

/******************************************************************************
*
* return the extension part of a filename
*
* eg.
*   file.c -> c
*   fixx_z -> z
*
******************************************************************************/

extern P_U8
file_extension(
    PC_U8 filename)
{
    PC_U8 ptr = file_leafname(filename);

    ptr = strchr(ptr, FILE_EXT_SEP_CH);

    if(ptr)
        return((P_U8) ptr + 1);

    return(NULL);
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure from file_find_first
*
* --out--
*
*   resources freed
*
******************************************************************************/

extern void
file_find_close(
    P_P_FILE_OBJENUM pp /*inout*/)
{
    P_FILE_OBJENUM p;

    p = *pp;

    if(p)
        {
        file_path_element_close(&p->pathenum);

        al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
        }
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure to initialise from path and pattern
*
* --out--
*
*   NULL    no matching objects
*   else    first matching object
*
******************************************************************************/

extern P_FILE_OBJINFO
file_find_first(
    _OutRef_    P_P_FILE_OBJENUM pp /*out*/,
    _In_z_      PC_U8Z path,
    _In_z_      PC_U8Z pattern)
{
    return(file_find_first_subdir(pp, path, pattern, NULL));
}

extern P_FILE_OBJINFO
file_find_first_subdir(
    _OutRef_    P_P_FILE_OBJENUM pp,
    _In_z_      PC_U8Z path,
    _In_z_      PC_U8Z pattern,
    _In_opt_z_  PC_U8Z subdir)
{
    STATUS status;
    P_FILE_OBJENUM p;

    *pp = p = al_ptr_alloc_elem(FILE_OBJENUM, 1, &status);

    if(!p)
        return(NULL);

    if(!file_path_element_first(&p->pathenum, path))
        {
        al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
        return(NULL);
        }

    /* initialise object enumeration structure */

    if(subdir)
        safe_strkpy(p->subdir, elemof32(p->subdir), subdir);
    else
        *p->subdir = NULLCH;

    safe_strkpy(p->pattern, elemof32(p->pattern), pattern);

    #if WINDOWS
    AnsiToOem(p->pattern, p->pattern);
    #endif

    /* have path element, requires validation */
    p->state = 1;

    return(file_find_next(pp));
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure from file_find_first
*
* --out--
*
*   NULL    no more matching objects
*   else    next matching object
*
*   current drive/directory corrupt
*
*   Note that if other processes in the program set the drive
*   or directory between calls then wrong results will be returned
*
******************************************************************************/

extern P_FILE_OBJINFO
file_find_next(
    P_P_FILE_OBJENUM pp /*inout*/)
{
    #if RISCOS
    _kernel_osgbpb_block gbpbblk;
    #elif WINDOWS
    S32              driveNumber;
    unsigned         usError;
    #endif
    P_U8            dirname;
    char             array[BUF_MAX_PATHSTRING];
    P_FILE_OBJENUM   p;

    p = *pp;

    /* loop finding objects */
    for(;;)
        {
        if(p->state == 3)
            {
            /* keep looking in current dir */

            #if RISCOS
            if(p->dirindex < 0)
                {
                /* already exhausted current dir, restart with new path element */
                p->state = 0;
                continue;
                }

            dirname = file__make_usable_dir(p->pathenum->res, array, elemof32(array));

            if(*p->subdir) /* not empty? */
                {
                if(dirname == p->pathenum->res)
                {
                    safe_strkpy(array, elemof32(array), dirname);
                    dirname = array;
                }

                safe_strkat(dirname, elemof32(array), FILE_DIR_SEP_STR);
                safe_strkat(dirname, elemof32(array), p->subdir);
                /* dirname IS valid, cos we checked it when p->state was 2 */
                }

            gbpbblk.dataptr  = &p->objinfo.fileinfo;            /* r2 Result buffer               */
            gbpbblk.nbytes   = 1;                               /* r3 One object                  */
            gbpbblk.fileptr  = p->dirindex;                     /* r4 Offset to item in directory */
            gbpbblk.buf_len  = sizeof(p->objinfo.fileinfo);     /* r5 Result buffer length        */
            gbpbblk.wild_fld = p->pattern;                      /* r6 Wildcarded name to match    */

            if(_kernel_ERROR == _kernel_osgbpb(OSGBPB_ReadDirEntriesInfo /*10*/, (unsigned) dirname, &gbpbblk))
                {
                /* error, restart with new path element */
                p->state = 0;
                continue;
                }

            p->dirindex = gbpbblk.fileptr;

            /* loop if returned none matched this time */
            if(gbpbblk.nbytes <= 0)
                continue;

            #elif WINDOWS
            usError = _dos_findnext(&p->objinfo.fileinfo);

            if(usError)
                {
                /* no more found, restart with new path element */
                p->state = 0;
                continue;
                }

            /* enumeration functions can return silly '.' and '..' */
            if(p->objinfo.fileinfo.name[0] == '.')
                continue;

            OemToAnsi(p->objinfo.fileinfo.name, p->objinfo.ansiname);
            #endif /* WINDOWS */

            return(&p->objinfo);
            }

        if(p->state == 2)
            {
            /* got a good dir, start search */
            dirname = file__make_usable_dir(p->pathenum->res, array, elemof32(array));

            if(*p->subdir) /* not empty? */
                {
                if(dirname == p->pathenum->res)
                {
                    safe_strkpy(array, elemof32(array), dirname);
                    dirname = array;
                }

                safe_strkat(dirname, elemof32(array), FILE_DIR_SEP_STR);
                safe_strkat(dirname, elemof32(array), p->subdir);

                if(!file_is_dir(dirname))
                    {
                    /* subdir invalid, get new path element */
                    p->state = 0;
                    continue;
                    }
                }

            #if RISCOS
            /* start hunting for files in this directory */
            p->dirindex = 0;
            p->state = 3;
            continue;

            #elif WINDOWS
            usError = 0;

            /* drive name to set? */
            driveNumber = file__DosDriveNumber(dirname);

            if(driveNumber != -1)
                {
                usError = _chdrive(driveNumber);

                dirname += 2;
                }

            if(!usError)
                {
                AnsiToOem(dirname, dirname);
                usError = chdir(dirname);
                }

            if(usError)
                {
                /* nothing found, restart with new path element */
                p->state = 0;
                continue;
                }

            usError = _dos_findfirst(p->pattern,
                                     _A_NORMAL | _A_RDONLY | _A_SUBDIR,
                                     &p->objinfo.fileinfo);

            if(usError)
                {
                /* nothing found, restart with new path element */
                p->state = 0;
                continue;
                }

            p->state = 3;

            /* enumeration functions can return silly '.' and '..' */
            if(p->objinfo.fileinfo.name[0] == '.')
                continue;

            OemToAnsi(p->objinfo.fileinfo.name, p->objinfo.ansiname);

            return(&p->objinfo);

            #else
            #error file_find_next not implemented on this system
            #endif /* OS */
            }

        /* path element requires validation? */
        if(p->state == 1)
            {
            if(file_is_dir(p->pathenum->res))
                /* valid dir found, restart search */
                p->state = 2;
            else
                /* dir invalid, get new path element */
                p->state = 0;

            continue;
            }

        /* require new path element? */
        if(p->state == 0)
            {
            if(file_path_element_next(&p->pathenum))
                /* possible dir found, requires validation */
                p->state = 1;
            else
                /* no more paths to search - close enumeration */
                break;

            continue;
            }

        myassert0x(p->state == 0, "bad object enumeration state");
        break;
        }

    file_find_close(pp);

    return(NULL);
}

/******************************************************************************
*
* return the directory in which the last object returned by file_find_first/file_find_next was found
*
* --in--
*
*   object enumeration structure from file_find_first
*
******************************************************************************/

extern char *
file_find_query_dirname(
    P_P_FILE_OBJENUM pp /*in*/, char * destpath,
    _InVal_     U32 elemof_buffer)
{
    P_FILE_OBJENUM p = *pp;
    P_U8 dirname;

    dirname = file__make_usable_dir(p->pathenum->res, destpath, elemof_buffer);

    if(dirname == p->pathenum->res)
    {
        safe_strkpy(destpath, elemof_buffer, dirname);
        dirname = destpath;
    }

    assert(dirname == destpath);

    if(*p->subdir) /* not empty? */
    {
        safe_strkat(dirname, elemof_buffer, FILE_DIR_SEP_STR);
        safe_strkat(dirname, elemof_buffer, p->subdir);
    }

    return(destpath);
}

extern void
file_objinfo_name(
    P_FILE_OBJINFO oip,
    char * destname /*out*/,
    _InVal_     U32 elemof_buffer)
{
    const char * name;

    #if RISCOS
    name = oip->fileinfo.name;
    #elif WINDOWS
    name = oip->ansiname;
    #endif

    safe_strkpy(destname, elemof_buffer, name);
}

extern U32
file_objinfo_size(
    P_FILE_OBJINFO oip)
{
    #if RISCOS
    return((oip->fileinfo.type == FILE_OBJECT_DIRECTORY)
                   ? (U32) 0
                   : (U32) oip->fileinfo.size);
    #elif WINDOWS
    return((oip->fileinfo.attrib & _A_SUBDIR)
                   ? (U32) 0
                   : (U32) oip->fileinfo.size);
    #endif
}

extern S32
file_objinfo_type(
    P_FILE_OBJINFO oip)
{
    #if RISCOS
    return(oip->fileinfo.type);
    #elif WINDOWS
    return((oip->fileinfo.attrib & _A_SUBDIR)
                   ? FILE_OBJECT_FILE
                   : FILE_OBJECT_DIRECTORY);
    #endif
}

extern FILETYPE_RISC_OS
file_objinfo_filetype(
    P_FILE_OBJINFO oip)
{
    #if RISCOS
    U32 filetype = oip->fileinfo.load;

    filetype = filetype >> 8;
    filetype = filetype & 0x00000FFF;

    return((FILETYPE_RISC_OS) filetype);
    #else
    return(FILETYPE_PIPEDREAM);
    #endif
}

/******************************************************************************
*
* try to find a file anywhere on the current path/cwd
*
* --out--
*
*   -ve: error
*     0: not found
*   +ve: found, filename set to found filename
*
******************************************************************************/

extern S32
file_find_on_path(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename)
{
    P_FILE_PATHENUM path;
    char * pathelem;
    S32 res;

    reportf("file_find_on_path(%u:%s)", strlen32(srcfilename), srcfilename);

    if(file_is_rooted(srcfilename))
        {
        /* ALWAYS copy in! */
        strcpy(filename, srcfilename);
        return(file_readable(filename));
        }

    res = 0;

    for(pathelem = file_path_element_first(&path, file_get_path());
        pathelem != NULL;
        pathelem = file_path_element_next(&path))
        {
        safe_strkpy(filename, elemof_buffer, pathelem);
        safe_strkat(filename, elemof_buffer, srcfilename);

        if(0 == (res = file_readable(filename)))
            continue;

        if(res > 0)
            break;

        if(file_is_dir(pathelem))
            break;

        res = 0; /* otherwise suppress the error (may be caused by malformed path element) and loop */
        }

    trace_2(TRACE_MODULE_FILE,
            "file_find_on_path() yields filename \"%s\" & %s",
            filename, trace_boolstring(res > 0));
    return(res);
}

extern S32
file_find_on_path_or_relative(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename,
    _In_opt_z_  PC_USTR currentfilename)
{
    char rawpath[BUF_MAX_PATHSTRING];
    P_FILE_PATHENUM path;
    char * pathelem;
    S32 res;

    if(NULL == currentfilename)
        return(file_find_on_path(filename, elemof_buffer, srcfilename));

    reportf("file_find_on_path_or_relative(%u:%s, cur=%u:%s)", strlen32(srcfilename), srcfilename, strlen32(currentfilename), currentfilename);

    if(file_is_rooted(srcfilename))
        {
        /* ALWAYS copy in! */
        strcpy(filename, srcfilename);
        return(file_readable(filename));
        }

    file_combined_path(rawpath, elemof32(rawpath), currentfilename);

    res = 0;

    for(pathelem = file_path_element_first(&path, rawpath);
        pathelem != NULL;
        pathelem = file_path_element_next(&path))
        {
        safe_strkpy(filename, elemof_buffer, pathelem);
        safe_strkat(filename, elemof_buffer, srcfilename);

        if(0 == (res = file_readable(filename)))
            continue;

        if(res > 0)
            break;

        if(file_is_dir(pathelem))
            break;

        res = 0; /* otherwise suppress the error (may be caused by malformed path element) and loop */
        }

    trace_2(TRACE_MODULE_FILE,
            "file_find_on_path() yields filename \"%s\" & %s",
            filename, trace_boolstring(res > 0));
    return(res);
}

/******************************************************************************
*
* try to find a dir. anywhere on the current path/cwd
*
* --out--
*
*   -ve: error
*     0: not found
*   +ve: found, filename set to found dirname
*
******************************************************************************/

extern S32
file_find_dir_on_path(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename,
    _In_opt_z_  PC_USTR currentfilename)
{
    char rawpath[BUF_MAX_PATHSTRING];
    P_FILE_PATHENUM path;
    char * pathelem;
    S32 res;

    reportf("file_find_dir_on_path(%u:%s, cur=%u:%s)", strlen32(srcfilename), srcfilename, currentfilename ? strlen32(currentfilename) : 0, currentfilename ? currentfilename : "<NULL>");

    if(file_is_rooted(srcfilename))
        {
        /* ALWAYS copy in! */
        strcpy(filename, srcfilename);
        return(file_is_dir(filename));
        }

    file_combined_path(rawpath, elemof32(rawpath), currentfilename);

    pathelem = file_path_element_first(&path, rawpath);

    res = 0;

    while(pathelem)
        {
        if(file_is_dir(pathelem))
            {
            safe_strkpy(filename, elemof_buffer, pathelem);
            safe_strkat(filename, elemof_buffer, srcfilename);

            if((res = file_is_dir(filename)) != 0)
                break;
            }

        pathelem = file_path_element_next(&path);
        }

    trace_2(TRACE_MODULE_FILE,
            "file_find_dir_on_path() yields dirname \"%s\" & %s",
            filename, trace_boolstring(res > 0));
    return(res);
}

/******************************************************************************
*
* obtain a directory prefix from current filename
*
******************************************************************************/

extern char *
file_get_cwd(
    char * destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename)
{
    PC_U8 namep;
    char * res;

    *destpath = NULLCH;

    if((namep = currentfilename) != NULL)
        {
        PC_U8 leafp = file_leafname(namep);
        S32 nchars = leafp - namep;
        if(nchars)
            safe_strnkpy(destpath, elemof_buffer, namep, nchars);
        }

    res = *destpath ? destpath : NULL;

    trace_2(TRACE_MODULE_FILE,
            "file_get_cwd() yields cwd = \"%s\", has cwd = %s",
            trace_string(res), trace_boolstring(*destpath));
    return(res);
}

/******************************************************************************
*
* --out--
*
*   current path being used for searches
*
* If WINDOWS, note that this name is Ansi-fied and so may need AnsiToOem
* processing before being passed to DOS in non file_xxx ways
*
******************************************************************************/

extern char *
file_get_path(void)
{
    return((search_path && *search_path) ? search_path : NULL);
}

/******************************************************************************
*
* obtain a directory prefix from current filename or first valid path element
*
******************************************************************************/

extern char *
file_get_prefix(
    char *destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename)
{
    char rawpath[BUF_MAX_PATHSTRING];
    P_FILE_PATHENUM path;
    char * pathelem;

    file_combined_path(rawpath, elemof32(rawpath), currentfilename);

    pathelem = file_path_element_first(&path, rawpath);

    *destpath = NULLCH;

    while(pathelem)
        {
        if(file_is_dir(pathelem))
            {
            safe_strkpy(destpath, elemof_buffer, pathelem);
            break;
            }

        pathelem = file_path_element_next(&path);
        }

    trace_1(TRACE_MODULE_FILE, "file_get_prefix yields %s", destpath);
    return(*destpath ? destpath : NULL);
}

/******************************************************************************
*
* determine whether a dir exists
*
******************************************************************************/

extern BOOL
file_is_dir(
    PC_U8 dirname)
{
    BOOL res;
    #if RISCOS
    _kernel_swi_regs r;
    #elif WINDOWS
    unsigned  usAttr, usError;
    #endif
    char      array[BUF_MAX_PATHSTRING];

    dirname = file__make_usable_dir(dirname, array, elemof32(array));

    #if RISCOS
    r.r[0] = OSFile_ReadNoPath;
    r.r[1] = (int) dirname;
    res = ((NULL == _kernel_swi(OS_File, &r, &r))  &&  (r.r[0] == OSFile_ObjectType_Dir));
    #elif WINDOWS
    #if WINDOWS
    AnsiToOem(array, (P_U8) dirname);
    #endif

    usError = _dos_getfileattr(dirname, &usAttr);

    res = (usError ? FALSE : ((usAttr & _A_SUBDIR) != 0)));
    #else
    res = FALSE;
    #endif

    reportf("file_is_dir(%u:%s): %d", strlen(dirname), dirname, res);

    return(res);
}

/******************************************************************************
*
* determine whether a file exists
*
******************************************************************************/

extern BOOL
file_is_file(
    PC_U8 filename)
{
    BOOL res;
    #if RISCOS
    _kernel_swi_regs r;
    #elif WINDOWS
    char      array[BUF_MAX_PATHSTRING];
    unsigned  usAttr, usError;
    #endif

    #if RISCOS
    r.r[0] = OSFile_ReadNoPath;
    r.r[1] = (int) filename;
    res = ((NULL == _kernel_swi(OS_File, &r, &r))  &&  (r.r[0] == OSFile_ObjectType_File));
    #elif WINDOWS
    #if WINDOWS
    AnsiToOem(array, (P_U8) filename);
    #endif

    usError = _dos_getfileattr(filename, &usAttr);

    res = (usError ? FALSE : ((usAttr & _A_SUBDIR) == 0));
    #else
    res = FALSE;
    #endif

    reportf("file_is_file(%u:%s): %d", strlen(filename), filename, res);

    return(res);
}

/******************************************************************************
*
* determine whether a filename is 'rooted' sufficiently
* to open not relative to cwd or path
*
******************************************************************************/

extern S32
file_is_rooted(
    PC_U8 filename)
{
    S32 res;

    #if RISCOS
    res = (strpbrk(filename, ":$&%@\\") != NULL);
    #elif WINDOWS
    /* rooted DOS filenames are either
     *   '\[dir\]*leaf'
     * or
     *   '<drive>:[\][dir\]*leaf'
     * where drive name always one a-zA-Z
    */
    if( (*filename == FILE_DIR_SEP_CH)
        #ifdef FILE_DIR_SEP_CH2
    ||  (*filename == FILE_DIR_SEP_CH2)
        #endif
        )
        res = TRUE;
    else
        res = (file__DosDriveNumber(filename) != -1);
    #endif

    trace_2(TRACE_MODULE_FILE,
            "file_is_rooted(%s) yields %s", filename, trace_boolstring(res));
    return(res);
}

/******************************************************************************
*
* return the leafname part of a filename
*
******************************************************************************/

extern P_U8
file_leafname(
    PC_U8 filename)
{
    PC_U8 leaf;

    leaf = filename + strlen(filename);  /* point to null */

    /* loop back over filename looking for a directory separator
     * or a root delimiter
    */
    while(leaf > filename)
        {
        U8 ch = *--leaf;

        if( (ch == FILE_ROOT_CH)
         || (ch == FILE_DIR_SEP_CH)
            #ifdef FILE_DIR_SEP_CH2
         || (ch == FILE_DIR_SEP_CH2)
            #endif
            )
            {
            ++leaf;
            break;
            }
        }

    return((P_U8) leaf);
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure from file_path_element_first
*
* --out--
*
*   resources freed
*
******************************************************************************/

extern void
file_path_element_close(
    P_P_FILE_PATHENUM pp /*inout*/)
{
    /* thankfully simple */
    al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure to initialise from path
*
* --out--
*
*   NULL    no more path elements
*   else    correctly terminated path element, might not exist
*
******************************************************************************/

extern char *
file_path_element_first(
    _OutRef_    P_P_FILE_PATHENUM pp,
    _In_z_      PC_U8Z path)
{
    STATUS status;
    P_FILE_PATHENUM p;
    PC_U8 ptr;

    *pp = p = al_ptr_alloc_elem(FILE_PATHENUM, 1, &status);

    if(!p)
        return(NULL);

    /* strip leading spaces */
    while(*path++ == ' ')
        ;
    --path;

    /* strip trailing spaces */
    ptr = path + strlen(path);
    while(ptr > path)
        {
        U8 ch = *--ptr;

        if(ch != ' ')
            {
            ++ptr;
            break;
            }
        }

    /* initialise path enumeration structure */
    p->ptr = p->path;
    p->pushed = NULLCH;

    safe_strnkpy(p->path, BUF_MAX_PATHSTRING, path, ptr - path);

    return(file_path_element_next(pp));
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure from file_path_element_first
*
* --out--
*
*   NULL    no more path elements
*   else    correctly terminated path element, might not exist
*
******************************************************************************/

extern char *
file_path_element_next(
    P_P_FILE_PATHENUM pp /*inout*/)
{
    P_FILE_PATHENUM p;
    char * res;
    char * ptr;
    char ch, lastch;
    S32 inquotes;

    p = *pp;

    res = p->ptr;

    if(p->pushed)
        {
        /* last operation pushed a char to make way for a dir sep and NULLCH */
        *res = p->pushed;
        p->pushed = NULLCH;
        }

    ch = *res;

    /* allow for quoted path elements - specifically for RISC OS or OS/2 */
    if(ch == '\"')
        {
        inquotes = 1;
        ch = *++res;
        }
    else
        inquotes = 0;

    /* path ended? */
    if(!ch)
        {
        file_path_element_close(pp);
        return(NULL);
        }

    /* loop till we find a path separator or eos */
    ptr = res;
    lastch = NULLCH;

    do  {
        if(ch == '\"')
            {
            /* end the returned part of the string */
            *ptr = NULLCH;
            ch = *ptr;
            }

        if(ch == FILE_PATH_SEP_CH)
            if(!inquotes)
                break;

        lastch = ch;
        ch = *++ptr;
        }
    while(ch);

    /* ensure path correctly terminated */
    if( lastch
     && (lastch != FILE_ROOT_CH)
     && (lastch != FILE_DIR_SEP_CH)
        #ifdef FILE_DIR_SEP_CH2
     && (lastch != FILE_DIR_SEP_CH2)
        #endif
        )
        {
        /* overwrite path sep or last NULLCH */
        lastch = *ptr;
        *ptr++ = FILE_DIR_SEP_CH;
        ch     = *ptr;
        *ptr   = NULLCH;

        if(lastch)
            p->pushed = ch;
        else
            /* that was the last path element, leave the pointer on this NULLCH */
            ;
        }
    else if(ch)
        {
        /* kill the path sep, start on next element next time */
        *ptr++ = NULLCH;
        }
    /* otherwise got a correctly terminated end element */

    p->ptr = ptr;
    p->res = res;

    return(res);
}

/******************************************************************************
*
* --out--
*
*   -ve: error
*     0: not readable
*   +ve: ok
*
******************************************************************************/

extern S32
file_readable(
    PC_U8 filename)
{
    FILE_HANDLE f;
    S32         res;

    if((res = file_open(filename, file_open_read, &f)) > 0)
        if((res = file_close(&f)) == 0)
            res = 1;

    reportf("file_readable(%u:%s): status=%d", strlen(filename), filename, res);

    trace_2(TRACE_MODULE_FILE,
            "file_readable(%s) returns %d", filename, res);
    return(res);
}

/******************************************************************************
*
* set the path that the application uses for resources.
*
******************************************************************************/

extern void
file_set_path(
    PC_U8 pathstr)
{
    if(!pathstr)
        str_clr(&search_path);
    else
        status_consume(str_set(&search_path, pathstr));

    reportf("file_set_path(%u:%s)",
            (NULL == search_path) ? 0 : strlen(search_path),
            (NULL == search_path) ? "NULL" : search_path);
}

/******************************************************************************
*
* return a pointer to the first wild part of a filename, if it is wild, or NULL
*
******************************************************************************/

extern P_U8
file_wild(
    PC_U8 filename)
{
    PC_U8 ptr;
    char   ch;

    ptr = filename;
    ch = *ptr;

    do  {
        if((ch == FILE_WILD_MULTIPLE)  ||  (ch == FILE_WILD_SINGLE))
            {
            trace_2(TRACE_MODULE_FILE,
                    "file_wild(%s) returns %s", filename, ptr);
            return((P_U8) ptr);
            }

        ch = *++ptr;
        }
    while(ch);

    trace_1(TRACE_MODULE_FILE, "file_wild(%s) returns NULL", filename);
    return(NULL);
}

/******************************************************************************
*
* internal routines
*
******************************************************************************/

/******************************************************************************
*
* given a dirname, strip off unneccesary trailing dir sep ch, copying to
* a buffer if mods are needed. return ptr to either this copy or original
*
******************************************************************************/

static P_U8
file__make_usable_dir(
    PC_U8 dirname,
    char * buffer /*out*/,
    _InVal_     U32 elemof_buffer)
{
    PC_U8 ptr;
    U8 ch;

    ptr = dirname + strlen(dirname);

    ch = *--ptr;

    if( (ch == FILE_DIR_SEP_CH)
        #ifdef FILE_DIR_SEP_CH2
    ||  (ch == FILE_DIR_SEP_CH2)
        #endif
      ) {
        #if WINDOWS
        /* must allow \ or c:\ */
        if(ptr != dirname)
            if((ptr != dirname + 2) || (file__DosDriveNumber(dirname) == -1))
        #endif
                {
                /* need to strip off that carefully placed dir sep */
                safe_strnkpy(buffer, elemof_buffer, dirname, ptr - dirname); /* no -1 ... */
                dirname = buffer;
                }
        }
    #if RISCOS && 0 /* no - this simply does not work */
    else if(ch == FILE_ROOT_CH)
        {
        /* need to add csd symbol on end */
        safe_strkpy(buffer, elemof_buffer, dirname);
        safe_strkat(buffer, elemof_buffer, "@");
        dirname = buffer;
        }
    #endif

    return((P_U8) dirname);
}

/* end of fileutil.c */
