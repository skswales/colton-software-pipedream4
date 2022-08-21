/* file.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External interface to the file handling module (both stream & utils) */

/* SKS February 1990 */

#ifndef __file_h
#define __file_h

#include "cmodules/riscos/filetype.h"

/*
macro definitions
*/

#if RISCOS
/* rely on FileSwitch buffering and FileCore
 * readahead/writebehind for performance
*/
#define FILE_DEFBUFSIZ      256

/* possibly limited by C library */
#define MAX_PATHSTRING      (1+255)

#ifndef MAX_FILENAME_WIMP
/* limited by Wimp block */
#define MAX_FILENAME_WIMP   (1+211)
#endif
#define BUF_MAX_FILENAME_WIMP (MAX_FILENAME_WIMP + 1)

/* this is the maximum leafname size the program generates for its own resource lookups */
#define MAX_GENLEAFNAME     (1+10)

#define FILE_WILD_MULTIPLE      '*'
#define FILE_WILD_MULTIPLE_STR  "*"
#define FILE_WILD_SINGLE        '#'
#define FILE_WILD_SINGLE_STR    "#"

/* : anywhere in RISC OS -> rooted */
#define FILE_ROOT_CH        ':'
#define FILE_DIR_SEP_CH     '.'
#define FILE_DIR_SEP_STR    "."
/* no real file extensions on RISC OS but _ has become conventional (6_3) */
#define FILE_EXT_SEP_CH     '_'

#define FILE_PATH_SEP_CH    ','
#define FILE_PATH_SEP_STR   ","

#elif WINDOWS
/* Find optimal DOS buffer someday - this is
 * what MSC 6.0 claims to use itself.
*/
#define FILE_DEFBUFSIZ      512

/* something small */
#define MAX_PATHSTRING      (1+255)

/* 8.3 */
#define MAX_GENLEAFNAME     (1+12)

#define FILE_WILD_MULTIPLE      '*'
#define FILE_WILD_MULTIPLE_STR  "*"
#define FILE_WILD_SINGLE        '?'
#define FILE_WILD_SINGLE_STR    "?"

/* : anywhere OR first is \ on DOS -> rooted */
#define FILE_ROOT_CH        ':'
#define FILE_DIR_SEP_CH     '\\'
#define FILE_DIR_SEP_STR    "\\"
/* / can be used when talking to C library */
#define FILE_DIR_SEP_CH2    '/'
/* DOS has real extensions (8.3) */
#define FILE_EXT_SEP_CH     '.'

#define FILE_PATH_SEP_CH    ';'
#define FILE_PATH_SEP_STR   ";"

#endif /* OS */

#define BUF_MAX_GENLEAFNAME (MAX_GENLEAFNAME + 1)
#define BUF_MAX_PATHSTRING  (MAX_PATHSTRING  + 1)

#ifndef ERRDEF_EXPORT /* not errdef export */

/*
structure definitions
*/

/*
extradata definition
*/

#if WINDOWS
typedef struct FILE_EDATA_WINDOWS
{
    S32         openmode;
    OFSTRUCT    of;
}
FILE_EDATA_WINDOWS;
#endif

#if RISCOS
typedef struct FILE_EDATA_RISCOS
{
    FILETYPE_RISC_OS    filetype;
    BOOL                filetype_modified;
    char                filename[BUF_MAX_PATHSTRING];
}
FILE_EDATA_RISCOS;
#endif

typedef struct __FILE_HANDLE
{
    /* public (only for macros) */
    S32    count;      /* number of bytes left in buffer */
    char * ptr;        /* pointer to current data item (char for loading) */
    S32    flags;

    /* keep your hands off these privates (or what?) */

    struct __FILE_HANDLE * next;

#define _FILE_MAGIC_TYPE U32
#define _FILE_MAGIC_WORD 0x00027C00 /* keep short for rotated constant compare on ARM - was 0x21147226*/

    _FILE_MAGIC_TYPE       magic;

#if RISCOS
#define _FILE_HANDLE_TYPE int
#define _FILE_HANDLE_NULL ((_FILE_HANDLE_TYPE)  0)
#elif WINDOWS
#define _FILE_HANDLE_TYPE HANDLE
#define _FILE_HANDLE_NULL ((_FILE_HANDLE_TYPE) INVALID_HANDLE_VALUE)
#endif

    _FILE_HANDLE_TYPE      handle;              /* host filing system handle */

    STATUS                 error;               /* sticky error */
    S32                    ungotch;             /* room for one ungot character */
    void *                 bufbase;             /* address of buffer */
    S32                    bufsize;             /* allocated size of that buffer */
    S32                    bufbytes;            /* amount of buffer valid (bufbytes - count) */
    S32                    bufpos;              /* file position of buffer */

#if RISCOS
    FILE_EDATA_RISCOS   riscos;
#elif WINDOWS
    FILE_EDATA_WINDOWS  windows;
#endif
}
_FILE_HANDLE, * FILE_HANDLE;

/* mode of opening for file */

typedef enum FILE_OPEN_MODE
{
    file_open_read      = 0,
    file_open_write     = 1,
    file_open_readwrite = 2
}
FILE_OPEN_MODE;

/* position within file */

typedef struct _filepos_t
{
    U32 lo;
}
filepos_t;

/* types of object */

#define FILE_OBJECT_NONE 0
#define FILE_OBJECT_FILE 1
#define FILE_OBJECT_DIRECTORY 2

/*
this structure is also private to fileutil.c but needs to be made visible for typedef safety
*/

typedef struct FILE_PATHENUM
{
    char * res;     /* last result */
    char * ptr;     /* current state */
    char   pushed;  /* ditto */
    char   path[MAX_PATHSTRING + 1]; /* +1 for incorrectly terminated paths */
}
FILE_PATHENUM, * P_FILE_PATHENUM, ** P_P_FILE_PATHENUM;

/*
as does this one
*/

typedef struct FILE_OBJINFO
{
    #if RISCOS
    struct FILE_OBJINFO_FILEINFO
    {
        S32       load;
        S32       exec;
        S32       size;
        U32       attr;
        U32       type; /* SKS says I thought RCM had moved this once already ... */
        char      name[BUF_MAX_PATHSTRING]; /* SKS 25oct96 now cater for long leafnames (was MAX_LEAFNAME) */
    }
    fileinfo;
    #endif
}
FILE_OBJINFO, * P_FILE_OBJINFO;

/*
as does this one
*/

typedef struct FILE_OBJENUM
{
    char              pattern[BUF_MAX_GENLEAFNAME];
    char              subdir[BUF_MAX_PATHSTRING]; /* SKS 25oct96 now cater for long leafnames (was MAX_LEAFNAME) */
    S32               state;
    P_FILE_PATHENUM   pathenum;
    FILE_OBJINFO      objinfo;
    #if RISCOS
    S32               dirindex;
    #endif
}
FILE_OBJENUM, * P_FILE_OBJENUM, ** P_P_FILE_OBJENUM;

/*
exported functions
*/

/*
file.c
*/

extern S32
file_buffer(
    FILE_HANDLE file_handle,
    void * userbuffer,
    size_t bufsize);

extern S32
file_clearerror(
    FILE_HANDLE file_handle);

extern S32
file_close(
    FILE_HANDLE * fp /*inout*/);

extern S32
file_error(
    FILE_HANDLE file_handle);

extern void
file_finalise(void);

extern S32
file_flush(
    FILE_HANDLE file_handle);

extern S32
file_getbyte(
    FILE_HANDLE file_handle);

extern S32
file_getpos(
    FILE_HANDLE file_handle,
    filepos_t * pos /*out*/);

extern S32
file_gets(
    char * buffer,
    S32 bufsize,
    FILE_HANDLE file_handle);

extern void
file_init(void);

extern S32
file_length(
    FILE_HANDLE file_handle);

extern S32
file_open(
    PC_U8 filename,
    FILE_OPEN_MODE openmode,
    FILE_HANDLE * fp /*out*/);

extern S32
file_pad(
    FILE_HANDLE file_handle,
    S32 alignpwer);

extern S32
file_putbyte(
    S32 c,
    FILE_HANDLE file_handle);

extern S32
file_puts(
    PC_U8 s,
    FILE_HANDLE file_handle);

extern S32
file_read(
    P_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle);

extern S32
file_rewind(
    FILE_HANDLE file_handle);

extern S32
file_seek(
    FILE_HANDLE file_handle,
    S32 offset,
    S32 origin);

extern S32
file_setpos(
    FILE_HANDLE file_handle,
    const filepos_t * pos);

extern S32
file_tell(
    FILE_HANDLE file_handle);

extern S32
file_ungetc(
    S32 c,
    FILE_HANDLE file_handle);

extern S32
file_write(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle);

extern S32
file_write_err(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle);

#if RISCOS
extern char *
file_get_error(void);

extern FILETYPE_RISC_OS
file_get_type(
    FILE_HANDLE file_handle);

extern S32
file_set_type(
    FILE_HANDLE file_handle,
    FILETYPE_RISC_OS filetype);
#endif

_Check_return_
extern STATUS
file_create_directory(
    _In_z_      PCTSTR name);

/*
fileutil.c
*/

extern char *
file_add_prefix_to_name(
    char * destfilename /*out*/,
    _InVal_     U32 elemof_buffer,
    PC_U8 srcfilename,
    PC_U8 currentfilename);

extern char *
file_combined_path(
    char * destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename);

extern P_U8
file_extension(
    PC_U8 filename);

extern void
file_find_close(
    P_P_FILE_OBJENUM pp /*inout*/);

extern P_FILE_OBJINFO
file_find_first(
    _OutRef_    P_P_FILE_OBJENUM pp,
    _In_z_      PC_U8Z path,
    _In_z_      PC_U8Z pattern);

extern P_FILE_OBJINFO
file_find_first_subdir(
    _OutRef_    P_P_FILE_OBJENUM pp,
    _In_z_      PC_U8Z path,
    _In_z_      PC_U8Z pattern,
    _In_opt_z_  PC_U8Z subdir);

extern P_FILE_OBJINFO
file_find_next(
    P_P_FILE_OBJENUM pp /*inout*/);

extern S32
file_find_on_path(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename);

extern S32
file_find_on_path_or_relative(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename,
    _In_opt_z_  PC_USTR currentfilename);

extern S32
file_find_dir_on_path(
    _Out_writes_z_(elemof_buffer) char * filename /*out*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR srcfilename,
    _In_opt_z_  PC_USTR currentfilename);

extern char *
file_find_query_dirname(
    P_P_FILE_OBJENUM pp /*in*/,
    char * destpath,
    _InVal_     U32 elemof_buffer);

extern char *
file_get_cwd(
    char * destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename);

extern char *
file_get_path(void);

extern char *
file_get_prefix(
    char * destpath,
    _InVal_     U32 elemof_buffer,
    PC_U8 currentfilename);

extern S32
file_is_dir(
    PC_U8 dirname);

extern S32
file_is_file(
    PC_U8 filename);

extern S32
file_is_rooted(
    PC_U8 filename);

extern P_U8
file_leafname(
    PC_U8 filename);

extern void
file_objinfo_name(
    P_FILE_OBJINFO oip,
    char * destname /*out*/,
    _InVal_     U32 elemof_buffer);

extern U32
file_objinfo_size(
    P_FILE_OBJINFO oip);

extern S32
file_objinfo_type(
    P_FILE_OBJINFO oip);

extern FILETYPE_RISC_OS
file_objinfo_filetype(
    P_FILE_OBJINFO oip);

extern S32
file_pad(
    FILE_HANDLE file_handle,
    S32 alignpwer);

extern void
file_path_element_close(
    P_P_FILE_PATHENUM p /*inout*/);

extern char *
file_path_element_first(
    _OutRef_    P_P_FILE_PATHENUM p,
    _In_z_      PC_U8Z path);

extern char *
file_path_element_next(
    P_P_FILE_PATHENUM p /*inout*/);

extern S32
file_readable(
    PC_U8 filename);

extern void
file_set_path(
    PC_U8 pathstr);

extern P_U8
file_wild(
    PC_U8 filename);

/*
exported functions for sole use of macros
*/

/*
file.c
*/

extern S32
file__eof(
    FILE_HANDLE file_handle);

extern S32
file__filbuf(
    FILE_HANDLE file_handle);

extern S32
file__flsbuf(
    S32 c,
    FILE_HANDLE file_handle);

/*
functions as macros
*/

#ifndef _FILE_EOFREAD
/* keep in step with file.h */
#define _FILE_EOFREAD 0x0008
#endif

/*
can only say for sure about EOF if read last buffer in and not yet at end
*/

#define file_eof(f) ( \
    (f)->flags & _FILE_EOFREAD \
        ? ((f)->count > 0 \
            ? STATUS_OK \
            : file__eof(f)) \
        : file__eof(f) )

#define file_getc(f) ( \
    --(f)->count >= 0 \
        ? (*(f)->ptr++) \
        : file__filbuf(f) )

/* can improve file_getc code by using these two */
#define file_getc_fast_ready(f) ( \
    (f)->count > 0 ) \

#define file_getc_fast(f) ( \
    --(f)->count \
        , (*(f)->ptr++) )

#define file_putc(c, f) ( \
    --(f)->count >= 0 \
        ? (*(f)->ptr++ = (char)(c)) \
        : file__flsbuf((c), (f)) )

/* can improve file_putc code by using these two */

#define file_putc_fast_ready(f) ( \
    (f)->count > 0 ) \

#define file_putc_fast(c, f) ( \
    --(f)->count, \
        (*(f)->ptr++ = (char)(c)) )

#define file_postooff(fileposp) ( \
    (S32) ((fileposp)->lo) )

#endif /* ERRDEF_EXPORT */

/*
error definition
*/

#define FILE_ERRLIST_DEF \
    errorstring(FILE_ERR_CANTOPEN,          "Can't open file") \
    errorstring(FILE_ERR_CANTCLOSE,         "Can't close file") \
    errorstring(FILE_ERR_CANTREAD,          "Can't read from file") \
    errorstring(FILE_ERR_CANTWRITE,         "Can't write to file") \
    errorstring(FILE_ERR_HANDLEUNBUFFERED,  "File handle is unbuffered") \
    errorstring(FILE_ERR_INVALIDPOSITION,   "Invalid position in file") \
    errorstring(FILE_ERR_DEVICEFULL,        "Device full") \
    errorstring(FILE_ERR_ACCESSDENIED,      "Access denied to file") \
    errorstring(FILE_ERR_TOOMANYFILES,      "Too many open files%0.s") \
    errorstring(FILE_ERR_BADHANDLE,         "Bad file handle") \
    errorstring(FILE_ERR_BADNAME,           "Bad filename") \
    errorstring(FILE_ERR_NOTFOUND,          "File %s not found") \
    errorstring(FILE_ERR_ISAFILE,           "%s is a file") \
    errorstring(FILE_ERR_ISADIR,            "%s is a directory") \
    errorstring(FILE_ERR_NAMETOOLONG,       "Filename %.64s... too long")

/*
error definition
*/

#define FILE_ERR_BASE             (-6000)

#define FILE_ERR_CANTOPEN         (-6000)
#define FILE_ERR_CANTCLOSE        (-6001)
#define FILE_ERR_CANTREAD         (-6002)
#define FILE_ERR_CANTWRITE        (-6003)
#define FILE_ERR_HANDLEUNBUFFERED (-6004)
#define FILE_ERR_INVALIDPOSITION  (-6005)
#define FILE_ERR_DEVICEFULL       (-6006)
#define FILE_ERR_ACCESSDENIED     (-6007)
#define FILE_ERR_TOOMANYFILES     (-6008)
#define FILE_ERR_spare_6009       (-6009)
#define FILE_ERR_BADHANDLE        (-6010)
#define FILE_ERR_BADNAME          (-6011)
#define FILE_ERR_NOTFOUND         (-6012)
#define FILE_ERR_ISAFILE          (-6013)
#define FILE_ERR_ISADIR           (-6014)
#define FILE_ERR_NAMETOOLONG      (-6015)

#define FILE_ERR_END              (-6016)

#endif /* __file_h */

/* end of file.h */
