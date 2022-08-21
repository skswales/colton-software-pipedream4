/* file.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* File handling module (stream section) */

/* SKS February 1990 */

#include "common/gflags.h"

#if RISCOS
#include "kernel.h" /*C:*/
#include "swis.h" /*C:*/

#include "cmodules/riscos/osfile.h"

#include "xstring.h"

#define _FILE_OBJECT_IS_FILE 1
#define _FILE_OBJECT_IS_DIR  2

#endif

#include "cmodules/file.h"

/*
internal functions
*/

static S32
file__closefile(
    FILE_HANDLE file_handle);

#if TRACE_ALLOWED

static S32
file__flushbuffer(
    FILE_HANDLE file_handle,
    _In_z_      PCTSTR caller);

#else

static S32
file___flushbuffer(
    FILE_HANDLE file_handle);

#define file__flushbuffer(file_handle, caller) \
    file___flushbuffer(file_handle)

#endif

static S32
file__read(
    P_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle);

static S32
file__seek(
    FILE_HANDLE file_handle,
    S32 offset,
    S32 origin);

static STATUS
file__set_error(
    FILE_HANDLE file_handle,
    _InVal_     STATUS status);

static S32
file__tell(
    FILE_HANDLE file_handle);

static S32
file__write(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle,
    S32 pos);

#if RISCOS
static S32
file__obtain_error_string(
    const _kernel_oserror * e);

static S32
file__set_error_string(
    PC_U8 errorstr);
#endif

/*
helper macros
*/

/*
private fields in file handle flags
*/

#define _FILE_READ       0x0001
#define _FILE_WRITE      0x0002
#define _FILE_CLOSED     0x0004
#ifndef _FILE_EOFREAD
#define _FILE_EOFREAD    0x0008
#endif
#define _FILE_BUFDIRTY   0x0010
#define _FILE_USERBUFFER 0x0020 /* user allocated buffer, not us */
#define _FILE_UNBUFFERED 0x0040 /* user specified no buffering */
#define _FILE_HASUNGOTCH 0x0080

/* ------------------------------------------------------------------------- */

static BOOL file__initialised = FALSE;

/*
a linked list of all open files
*/

#define _FILE_LIST_END ((FILE_HANDLE) 0x0000)
static FILE_HANDLE file_list = NULL;

/*
default buffer size to allocate for single byte access
*/

static S32 file__defbufsiz = FILE_DEFBUFSIZ;

#if RISCOS
/* hold an error string from the filing system */

static char * file__errorptr = NULL;

static char   file__errorbuffer[256-4] = "";
#endif

/******************************************************************************
*
* set up buffering
*
* --in--
*
*   f == 0 -> set default buffer size
*   f != 0 -> set buffer on open file
*
******************************************************************************/

extern S32
file_buffer(
    FILE_HANDLE file_handle,
    void * userbuffer,
    size_t bufsize)
{
    void * buffer;

    trace_3(TRACE_MODULE_FILE,
            "file_buffer(" PTR_XTFMT ", " PTR_XTFMT ", %u)", report_ptr_cast(file_handle), userbuffer, bufsize);

    if(!file_handle)
        {
        trace_1(TRACE_MODULE_FILE,
                "file_buffer -- NULL FILE_HANDLE sets up default buffer size of %u",
                bufsize);
        file__defbufsiz = bufsize;
        return(0);
        }

#if CHECKING
    if(file_handle->magic != _FILE_MAGIC_WORD)
        {
        assert(file_handle->magic == _FILE_MAGIC_WORD);
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    if(userbuffer)
        {
        myassert1x(bufsize != 0, "file_buffer with buffer &%p but bufsize 0", userbuffer);

        /* free buffer if any and we allocated it */
        if(!(file_handle->flags & _FILE_USERBUFFER))
            al_ptr_dispose(&file_handle->bufbase);

        buffer = userbuffer;

        file_handle->flags = (file_handle->flags & ~_FILE_UNBUFFERED) | _FILE_USERBUFFER;
        }
    else
        {
        /* consider use has done file_buffer(file_handle, b, sizeof(b)) then
         * file_buffer(file_handle, NULL, q) - we have to assume that he's doing
         * this for a good reason eg. the buffer is going out of scope
         * so failure to alloc can leave file in an unbuffered and error state
        */
        file_handle->flags = file_handle->flags & ~(_FILE_UNBUFFERED | _FILE_USERBUFFER);

        if(bufsize)
            {
            STATUS status;
            buffer = al_ptr_alloc_bytes(void *, bufsize, &status);
            if(!buffer)
                return(file__set_error(file_handle, create_error(FILE_ERR_HANDLEUNBUFFERED)));
            }
        else
            {
            file_handle->flags = file_handle->flags | _FILE_UNBUFFERED;
            buffer = NULL;
            }
        }

    file_handle->count   = -1;

    file_handle->bufbase = buffer;
    file_handle->bufsize = (S32) bufsize;

    return(0);
}

/******************************************************************************
*
* clear error indicator on stream
*
******************************************************************************/

extern S32
file_clearerror(
    FILE_HANDLE file_handle)
{
    S32 res;

    trace_1(TRACE_MODULE_FILE, "file_clearerror(" PTR_XTFMT ")", report_ptr_cast(file_handle));

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    res = file_handle->error;
    file_handle->error = 0;

    trace_1(TRACE_MODULE_FILE, "file_clearerror returns old error %d", res);
    return(res);
}

/******************************************************************************
*
* close stream, flushing data to disc
*
******************************************************************************/

extern S32
file_close(
    FILE_HANDLE * fp /*inout*/)
{
    FILE_HANDLE file_handle;
    FILE_HANDLE pp, cp;
    S32         res;

    trace_2(TRACE_MODULE_FILE,
            "file_close(" PTR_XTFMT " -> " PTR_XTFMT ")", report_ptr_cast(fp), report_ptr_cast(fp ? *fp : NULL));

    if(!fp)
        return(create_error(FILE_ERR_BADHANDLE));

    file_handle = *fp;

    /* if file already closed, don't worry */
    if(!file_handle)
        return(0);

#if CHECKING
    if(file_handle->magic != _FILE_MAGIC_WORD)
        {
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* search for file and delink from list
     * first time round always awkward
    */
    pp = (FILE_HANDLE) &file_list;
    cp =                file_list;

    while(cp != _FILE_LIST_END)
        {
        if(cp == file_handle)
            break;

        pp = cp;
        cp = pp->next;
        }

    if(cp == _FILE_LIST_END)
        res = create_error(FILE_ERR_BADHANDLE);
    else
        {
        /* take care in delinking */
        if(pp == (FILE_HANDLE) &file_list)
            file_list = file_handle->next;
        else
            pp->next = file_handle->next;

        if(file_handle->handle != _FILE_HANDLE_NULL)
            {
            res = file__closefile(file_handle);

            file_handle->handle = _FILE_HANDLE_NULL;

            /* free buffer if any and we allocated it */
            if(!(file_handle->flags & _FILE_USERBUFFER))
                al_ptr_dispose(&file_handle->bufbase);
            }
        else
            /* nop if already closed on fs but good */
            res = 0;

        /* free file descriptor */
        al_ptr_dispose(P_P_ANY_PEDANTIC(fp));
        }

    trace_3(TRACE_MODULE_FILE,
            "file_close(" PTR_XTFMT " -> " PTR_XTFMT ") yields %d", report_ptr_cast(fp), report_ptr_cast(file_handle), res);
    return(res);
}

_Check_return_
extern STATUS
file_create_directory(
    _In_z_      PCTSTR dirname)
{
#if RISCOS
    _kernel_osfile_block osfile_block;

    reportf("file_create_directory(%u:%s)", strlen32(dirname), dirname);

    memset32(&osfile_block, NULLCH, sizeof32(osfile_block));

    if(_kernel_ERROR == _kernel_osfile(OSFile_CreateDir, dirname, &osfile_block))
        /*oops*/ (void) _kernel_last_oserror();
#elif WINDOWS
    if(!CreateDirectory(dirname, NULL))
    {
        DWORD dwLastError = GetLastError();
        if(dwLastError != ERROR_ALREADY_EXISTS)
            return(FILE_ERR_MKDIR_FAILED);
    }
#else
    _mkdir(dirname);
#endif /* OS */

    return(STATUS_OK);
}

/******************************************************************************
*
* return the last error encountered on a given stream
* the error is not cleared -- use file_clearerr()
*
******************************************************************************/

extern S32
file_error(
    FILE_HANDLE file_handle)
{
    S32 res;

    trace_1(TRACE_MODULE_FILE, "file_error(" PTR_XTFMT ")", report_ptr_cast(file_handle));

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    file_handle->flags &= ~_FILE_EOFREAD;
    res = file_handle->error;

    trace_2(TRACE_MODULE_FILE, "file_error(" PTR_XTFMT ") returns error %d", report_ptr_cast(file_handle), res);
    return(res);
}

extern void
file_finalise(void)
{
    FILE_HANDLE file_handle;

    trace_0(TRACE_MODULE_FILE, "file_finalise() -- closedown");

    /* restart each time as file descriptors get deleted */
    while((file_handle = file_list) != _FILE_LIST_END)
        (void) file_close(&file_handle);
}

/******************************************************************************
*
* ensure all data flushed to disc on this stream
*
******************************************************************************/

extern S32
file_flush(
    FILE_HANDLE file_handle)
{
#if RISCOS
    _kernel_swi_regs rs;
#endif
    S32              res;

    trace_1(TRACE_MODULE_FILE, "file_flush(" PTR_XTFMT ")", report_ptr_cast(file_handle));

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* flush internal buffers to file */
    if((res = file__flushbuffer(file_handle, "file_flush")) < 0)
        return(res);

    /* flush external buffers to file -- no need to flush closed files! */
    if(!(file_handle->flags & _FILE_CLOSED))
        {
#if RISCOS
        rs.r[0] = OSArgs_Flush;
        rs.r[1] = file_handle->handle;

        if( file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs))  &&
            (res >= 0)  )
            res = file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE));
#elif WINDOWS
        /* no flushing to be done */
#else
        if(fflush(file_handle->handle))
            res = file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE));
#endif
        }

    trace_2(TRACE_MODULE_FILE, "file_flush(" PTR_XTFMT ") yields %d", report_ptr_cast(file_handle), res);
    return(res);
}

/******************************************************************************
*
* get a byte from stream
*
******************************************************************************/

extern S32
file_getbyte(
    FILE_HANDLE file_handle)
{
#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    return(file_getc(file_handle));
}

/******************************************************************************
*
* read the current seqptr of stream
*
******************************************************************************/

extern S32
file_getpos(
    FILE_HANDLE file_handle,
    filepos_t * pos /*out*/)
{
    S32 res32;

    trace_2(TRACE_MODULE_FILE, "file_getpos(" PTR_XTFMT " to " PTR_XTFMT ")", report_ptr_cast(file_handle), report_ptr_cast(pos));

    assert_EQ(sizeof(filepos_t), sizeof(U32));

    res32 = file_tell(file_handle); /* validates file */

    if(res32 >= 0)
        {
        * (P_S32 ) pos = res32;
        return(0);
        }

    return((S32) res32);
}

#if RISCOS

/******************************************************************************
*
* read the current error associated with this module if any
* clears error if set; error message only valid until next call to module
*
******************************************************************************/

extern char *
file_get_error(void)
{
    char * errorstr;

    errorstr = file__errorptr;
    file__errorptr = NULL;

    return(errorstr);
}

#endif /* RISCOS */

/******************************************************************************
*
* read a LF/CR/LF,CR/CR,LF/EOF terminated line into a buffer
*
******************************************************************************/

extern S32
file_gets(
    char * buffer,
    S32 bufsize,
    FILE_HANDLE file_handle)
{
    S32 count = 0;
    S32 res, newres;

    *buffer = NULLCH;

    if(!bufsize--)
        return(0);

    do  {
        if((res = file_getc(file_handle)) < 0)
            {
            /* EOF terminating a line is ok normally, especially if chars read */
            if(res == EOF)
                if(count > 0)
                    return(count);

            return(res);
            }

        if((res == LF)  ||  (res == CR))
            {
            /* got line terminator, read ahead */
            if((newres = file_getc(file_handle)) < 0)
                {
                /* that EOF will terminate next line immediately */
                if(newres == EOF)
                    return(count);

                return(newres);
                }

            /* if not got alternate line terminator, put it back */
            if(res != (newres ^ LF ^ CR))
                if((newres = file_ungetc(newres, file_handle)) < 0)
                    return(newres);

            break;
            }

        buffer[count++] = res;
        buffer[count  ] = NULLCH; /* keep terminated */
        }
    while(count < bufsize);

    return(count);
}

#if RISCOS

extern FILETYPE_RISC_OS
file_get_type(
    FILE_HANDLE file_handle)
{
#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(FILETYPE_UNDETERMINED);
        }
#endif

    return(file_handle->riscos.filetype);
}

#endif /* RISCOS */

/******************************************************************************
*
* initialise module entry point
*
******************************************************************************/

extern void
file_init(void)
{
    file__initialised = TRUE;
}

/******************************************************************************
*
* return the length of a file
*
******************************************************************************/

extern S32
file_length(
    FILE_HANDLE file_handle)
{
    S32 length;
    S32 res;

    trace_1(TRACE_MODULE_FILE, "file_length(" PTR_XTFMT ")", report_ptr_cast(file_handle));

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* ensure anything written to output buffer is included in the length */
    if((res = file__flushbuffer(file_handle, "file_length")) < 0)
        return(res);

    {
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_ReadEXT;
    rs.r[1] = file_handle->handle;

    if(file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs)))
        length = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
    else
        length = rs.r[2];
#elif WINDOWS
    errno = 0;

    length = filelength(file_handle->handle);

    if(length == -1)
        length = file_handle->error = create_error(FILE_ERR_CANTREAD);
#else
    filepos_t pos;

    if((length = file_getpos(file_handle, &pos)) >= 0)
        length = file_seek(file_handle, 0L, SEEK_END);

    if(length >= 0)
        (void) file_setpos(file_handle, &pos);
#endif
    } /*block*/

    trace_2(TRACE_MODULE_FILE, "file_length(" PTR_XTFMT ") yields %d", report_ptr_cast(file_handle), length);
    return(length);
}

/******************************************************************************
*
* open a file:
*
* --out--
*
*   -ve:   error in open
*     0:   file could not be opened
*   +ve:   file opened
*
******************************************************************************/

extern S32
file_open(
    PC_U8 filename,
    FILE_OPEN_MODE openmode,
    /*out*/ FILE_HANDLE * p_file_handle)
{
#if RISCOS
    static const int
    openatts[] =
    {
        OSFind_OpenRead     | OSFind_UseNoPath | OSFind_EnsureNoDir,
        OSFind_CreateUpdate | OSFind_UseNoPath,
        OSFind_OpenUpdate   | OSFind_UseNoPath
    };
#elif WINDOWS
    static const DWORD
    openaccess[] =
    {
        GENERIC_READ,
        GENERIC_READ | GENERIC_WRITE,
        GENERIC_READ | GENERIC_WRITE
    };
    static const DWORD
    openshare[] =
    {
        FILE_SHARE_READ,
        0,
        0
    };
    static const DWORD
    opencreate[] =
    {
        OPEN_EXISTING,
        CREATE_ALWAYS,
        OPEN_EXISTING
    };
#else
    static const char *
    openatts[] =
    {
        "rb",
        "w+b",
        "r+b"
    };
#endif /* OS */

    FILE_HANDLE file_handle;
    S32 res;

#if WINDOWS
    if((U32) openmode >= elemof32(openaccess))
        return(status_check());
#endif

    trace_3(TRACE_MODULE_FILE, TEXT("file_open(%s, %d, " PTR_XTFMT ")"), filename, openmode, report_ptr_cast(p_file_handle));

    /* would the name be too long to put in the allocated structure? */
#if RISCOS
    if(strlen32(filename) >= sizeof32(file_handle->riscos.filename))
        {
        reportf("file_open(%u:%s, &%02x): res=FILE_ERR_NAMETOOLONG", strlen(filename), filename, openatts[openmode]);
        return(create_error(FILE_ERR_NAMETOOLONG));
        }
#endif

    if(NULL == (*p_file_handle = file_handle = al_ptr_alloc_elem(_FILE_HANDLE, 1, &res)))
        return(res);

    /* always initialise file to look silly */
    zero_struct_ptr(file_handle);

    file_handle->count = -1;  /* nothing buffered; forces getc/putc to fns */
    file_handle->flags = (openmode == file_open_read)
                          ?  _FILE_READ
                          : (_FILE_READ | _FILE_WRITE);
    file_handle->magic = _FILE_MAGIC_WORD;

#if RISCOS
    {
    _kernel_swi_regs rs;

    (void) strcpy(file_handle->riscos.filename, filename);

    rs.r[0] = openatts[openmode];
    rs.r[1] = (int) filename;

    if(file__obtain_error_string(_kernel_swi(OS_Find, &rs, &rs)))
        res = create_error(FILE_ERR_CANTOPEN);
    else
        {
        file_handle->handle = rs.r[0];
        res = (file_handle->handle != _FILE_HANDLE_NULL);
        }

    reportf("file_open(%u:%s, &%02x): res=%d, OS_handle=%d", strlen(filename), filename, openatts[openmode], res, file_handle->handle);

    if(res > 0)
        {
        rs.r[0] = OSFile_ReadInfo;
        rs.r[1] = (int) filename;
        rs.r[2] = 0; /* for error->untyped case */
        if( _kernel_swi(OS_File, &rs, &rs)  || /* oh, ignore that error (especially for device streams) */
            ((rs.r[2] >> (12+8)) != -1)    )  /* dated? */
            file_handle->riscos.filetype = FILETYPE_UNTYPED;
        else
            file_handle->riscos.filetype = (FILETYPE_RISC_OS) ((rs.r[2] >> 8) & 0xFFF);
        }
    } /*block*/
#elif WINDOWS
    file_handle->handle = CreateFile(filename, openaccess[openmode], openshare[openmode], NULL, opencreate[openmode], FILE_ATTRIBUTE_NORMAL, NULL);

    status = (_FILE_HANDLE_NULL != file_handle->handle);

    if(status == 1)
    {
        BY_HANDLE_FILE_INFORMATION info;

        GetFileInformationByHandle(file_handle->handle, &info);

        file_handle->windows.file_date = info.ftLastWriteTime;
    }
#else
    file_handle->handle = fopen(filename, openatts[openmode]);

    res = (file_handle->handle != _FILE_HANDLE_NULL);
#endif

    if((res == 0)  &&  (openmode == file_open_write))
        res = create_error(FILE_ERR_CANTOPEN);

    if(res <= 0)
        al_ptr_dispose(P_P_ANY_PEDANTIC(p_file_handle));
    else
        {
        if(!file__initialised)
            file_init();

        /* link file onto head of list */
        file_handle->next  = file_list;
        file_list = file_handle;
        }

    trace_2(TRACE_MODULE_FILE, TEXT("file_open yields " PTR_XTFMT ", %d"), report_ptr_cast(file_handle), res);
    return(res);
}

/******************************************************************************
*
* pad bytes out from seqptr to a power of two on stream
*
******************************************************************************/

extern S32
file_pad(
    FILE_HANDLE file_handle,
    S32 alignpower)
{
    S32 alignment, alignmask, res32;
    S32    res;

    if(!alignpower)
        return(0);

    alignment = (S32) 1 << alignpower;
    alignmask = alignment - 1;

    if((res32 = file_tell(file_handle)) < 0) /* validates file */
        return((S32) res32);

    trace_6(TRACE_MODULE_FILE,
            "file_pad(" PTR_XTFMT ", %d): alignment %d, mask &%x, pos &%8.8x, needs %d",
            report_ptr_cast(file_handle), alignpower, alignment, alignmask, res32,
            ((res32 & alignmask) ? (alignment - (res32 & alignmask)) : 0));

    if(res32 & alignmask)
        {
        alignment = alignment - (res32 & alignmask);
        do  {
            trace_0(TRACE_MODULE_FILE, "file_pad outputting NULLCH");
            res = file_putc(NULLCH, file_handle);
            }
        while((res >= 0)  &&  --alignment);
        }
    else
        res = 0;

    return(res);
}

/******************************************************************************
*
* With buffered getc/putc we too make the restriction (a la ANSI)
* that getc and putc on the same stream are separated by a flush
* or a seek operation (use seek(file_handle, 0, SEEK_CUR))
*
******************************************************************************/

extern S32
file_putbyte(
    S32 c,
    FILE_HANDLE file_handle)
{
#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    return(file_putc(c, file_handle));
}

/******************************************************************************
*
* write a NULLCH terminated string to the file
*
******************************************************************************/

extern S32
file_puts(
    PC_U8 s,
    FILE_HANDLE file_handle)
{
    S32 c;
    S32 res;

    while((c = *s++) != NULLCH)
        if((res = file_putc(c, file_handle)) < 0)
            return(res);

    return(1);
}

extern S32
file_read(
    P_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle)
{
    S32 res;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* trivial case? */
    if(!size  ||  !nmemb)
        return(0);

    /* limit on number of bytes to write as we're casting to S32 result */
    myassert4x(((long) size * (long) nmemb) <= S32_MAX, "file_read(&%p, %u, %u, &%p): integer overflow size * nmemb", ptr, size, nmemb, file_handle);

    /* always perform read ops from file, so lose buffer, reposition seqptr */
    if((res = file__flushbuffer(file_handle, "file_read")) < 0)
        return(res);

    return(file__read(ptr, size, nmemb, file_handle));
}

/******************************************************************************
*
* sets the file position indicator for the stream pointed to by stream to
* the beginning of the file. It is equivalent to
*          file_seek(stream, 0L, SEEK_SET)
* except that the error indicator for the stream is also cleared.
*
******************************************************************************/

extern S32
file_rewind(
    FILE_HANDLE file_handle)
{
    /* stay close to ANSI */
    (void) file_clearerror(file_handle);

    return((S32) file_seek(file_handle, 0, SEEK_SET));
}

/******************************************************************************
*
* sets the file position indicator for the stream pointed to by f.
* The new position is at the signed number of characters specified
* by offset away from the point specified by origin.
* The specified point is the beginning of the file for SEEK_SET, the
* current position in the file for SEEK_CUR, or end-of-file for SEEK_END.
* After a file_seek call, the next operation on an update stream
* may be either input or output.
*
******************************************************************************/

extern S32
file_seek(
    FILE_HANDLE file_handle,
    S32 offset,
    S32 origin)
{
    S32 res;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* always lose buffer as a subsequent file_putc will need to
     * call file__flsbuf in order to set BUFDIRTY. Yes, it may be
     * tempting to reposition ptr within the buffer but DON'T DO IT!
    */
    if((res = file__flushbuffer(file_handle, "file_seek")) < 0)
        return(res);

    return(file__seek(file_handle, offset, origin));
}

/******************************************************************************
*
* restore a saved (from file_getpos) seqptr on stream
*
******************************************************************************/

extern S32
file_setpos(
    FILE_HANDLE file_handle,
    const filepos_t * pos)
{
    S32 res32;

    assert_EQ(sizeof(filepos_t), sizeof(U32));

    res32 = file_seek(file_handle, * (P_S32 ) pos, SEEK_SET);

    return((res32 <= 0) ? (S32) res32 : 1);
}

#if RISCOS

/******************************************************************************
*
* set the file type of a stream (written on closing)
*
******************************************************************************/

extern S32
file_set_type(
    FILE_HANDLE file_handle,
    FILETYPE_RISC_OS filetype)
{
#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    file_handle->riscos.filetype          = filetype;
    file_handle->riscos.filetype_modified = 1;

    return(1);
}

#endif /* RISCOS */

#define file__tell_using_buffer(file_handle) ( \
        file_handle->bufpos + file_handle->bufbytes - file_handle->count )

extern S32
file_tell(
    FILE_HANDLE file_handle)
{
#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* return result either deduced from offset in buffer or real seq ptr */
    if(file_handle->count != -1)
        return(file__tell_using_buffer(file_handle));

    return(file__tell(file_handle));
}

/******************************************************************************
*
* one level of ungetc provided
*
******************************************************************************/

extern S32
file_ungetc(
    S32 c,
    FILE_HANDLE file_handle)
{
    S32 res32;
    S32    res;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    if(c != EOF)
        {
        /* lose any buffer on this file */
        if((res = file__flushbuffer(file_handle, "file_ungetc")) < 0)
            return(res);

        /* step file ptr back in file one byte -- ANSI */
        if((res32 = file__seek(file_handle, -1, SEEK_CUR)) < 0)
            return((S32) res32);

        file_handle->flags |= _FILE_HASUNGOTCH;
        file_handle->ungotch = c;
        }

    return(c);
}

/******************************************************************************
*
* writes, from the array pointed to by ptr up to nmemb members whose size
* is specified by size, to the file_handler specified by f. The file
* position indicator is advanced by the number of characters
* successfully written. If an error occurs, the resulting value of the file
* position indicator is indeterminate.
* Returns: the number of members successfully written, which will be less
*          than nmemb only if a write error is encountered.
*
******************************************************************************/

extern S32
file_write(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle)
{
    S32 res;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* trivial call? */
    if(!size  ||  !nmemb)
        return(0);

    /* limit on number of bytes to write as we're casting to S32 result */
    myassert4x(((long) size * (long) nmemb) <= S32_MAX, "file_write(&%p, %u, %u, &%p): integer overflow size * nmemb", ptr, size, nmemb, file_handle);

    /* simple implementation: must flush out buffered data as we're going
     * to write direct to filing system at seqptr
    */
    if((res = file__flushbuffer(file_handle, "file_write")) < 0)
        return(res);

    return(file__write(ptr, size, nmemb, file_handle, -1));
}

/******************************************************************************
*
* write to a file but return an error if
* not all members were written out
*
******************************************************************************/

extern S32
file_write_err(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle)
{
    S32 res;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* trivial call? */
    if(!size  ||  !nmemb)
        return(0);

    myassert4x(((long) size * (long) nmemb) <= S32_MAX, "file_write_err(&%p, %u, %u, &%p): integer overflow size * nmemb", ptr, size, nmemb, file_handle);

    /* simple implementation: must flush out buffered data as we're going
     * to write direct to filing system at seqptr
    */
    if((res = file__flushbuffer(file_handle, "file_write_err")) < 0)
        return(res);

    res = file__write(ptr, size, nmemb, file_handle, -1);

    return(((size_t) res == nmemb)
                ? res
                : (res < 0)
                      ? res
                      : (file_handle->error = create_error(FILE_ERR_CANTWRITE)));
}

/******************************************************************************
*
* internal routines
*
******************************************************************************/

/******************************************************************************
*
* close file
*
******************************************************************************/

static S32
file__closefile(
    FILE_HANDLE file_handle)
{
    S32 res;

    trace_1(TRACE_MODULE_FILE, "file__closefile(" PTR_XTFMT ")", report_ptr_cast(file_handle));

    if(file_handle->flags & _FILE_CLOSED)
        /* no need to do anything; already closed on fs */
        res = 0;
    else
        {
        res = file_flush(file_handle);

        {
#if RISCOS
        _kernel_swi_regs rs;

        rs.r[0] = 0;
        rs.r[1] = file_handle->handle;

        reportf("file__closefile(OS_handle=%d)", file_handle->handle);

        if(file__obtain_error_string(_kernel_swi(OS_Find, &rs, &rs)))
            {
            if(res >= 0)
                res = create_error(FILE_ERR_CANTCLOSE);
            }
        else
            {
            if(file_handle->riscos.filetype_modified)
                {
                file_handle->riscos.filetype_modified = 0;
                rs.r[0] = OSFile_SetType;
                rs.r[1] = (int) &file_handle->riscos.filename;
                rs.r[2] = file_handle->riscos.filetype;
                if(file__obtain_error_string(_kernel_swi(OS_File, &rs, &rs)))
                    if(res >= 0)
                        res = create_error(FILE_ERR_CANTCLOSE);
                }
            }
#elif WINDOWS
    /* update this near to the close */
    BY_HANDLE_FILE_INFORMATION info;

    GetFileInformationByHandle(file_handle->handle, &info);

    file_handle->windows.file_date = info.ftLastWriteTime;

    if(!CloseHandle(file_handle->handle))
        status = create_error(FILE_ERR_CANTCLOSE);
#else
        if(fclose(file_handle->handle))
            res = create_error(FILE_ERR_CANTCLOSE);
#endif
    } /*block*/

        file_handle->flags |= _FILE_CLOSED;
        }

    return(res);
}

#if WINDOWS

/******************************************************************************
*
* ensure that a file is reopened on the filing system
*
******************************************************************************/

static S32
file___ensureopen(
    FILE_HANDLE file_handle)
{
    S32 res;

    res = OpenFile((LPSTR) NULL,
                   &file_handle->windows.of,
                   OF_PROMPT | OF_REOPEN | file_handle->windows.openmode);

    return(0);
}

#endif /* WINDOWS */

/******************************************************************************
*
* fill a buffer on a file from
* the filing system at seqptr
* only called if nothing in buffer (count = -1)
*
******************************************************************************/

static S32
file__fillbuffer(
    FILE_HANDLE file_handle)
{
    S32 res;
    S32    bytesread;

    /* note where buffer belongs in the file */
    if((res = file__tell(file_handle)) < 0)
        return((S32) res);

    file_handle->bufpos = res;

    /* try to read as many bytes as possible */
    bytesread = file__read(file_handle->bufbase, 1, file_handle->bufsize, file_handle);

    if(bytesread != file_handle->bufsize)
        /* only set if EOF been read in this buffer */
        file_handle->flags |=  _FILE_EOFREAD;
    else
        file_handle->flags &= ~_FILE_EOFREAD;

    if(bytesread <= 0)
        {
        file_handle->count = -1;
        return((bytesread < 0) ? bytesread : EOF);
        }

    file_handle->count    = bytesread; /* doesn't matter if bytesread < bufsize */
    file_handle->ptr      = file_handle->bufbase;
    file_handle->bufbytes = bytesread;

    return(0);
}

/******************************************************************************
*
* if a file is buffered and bytes have
* been written to it then flush them
* to the filing system, but update seqptr
* in any case. file must be valid and open
*
******************************************************************************/

static S32
#if TRACE_ALLOWED
file__flushbuffer(
    FILE_HANDLE file_handle,
    PC_U8 caller)
#else
file___flushbuffer(
    FILE_HANDLE file_handle)
#endif
{
    size_t bytesused;
    S32    res;

    trace_2(TRACE_MODULE_FILE,
            "file__flushbuffer(" PTR_XTFMT ") called by %s", report_ptr_cast(file_handle), caller);

    if(file_handle->error)
        /* sticky error for ANSI closeness */
        res = file_handle->error;
    else if(file_handle->count != -1)
        {
        /* we have a buffer, determine usage and dirtiness */
        bytesused = file_handle->bufbytes - file_handle->count;

        /* zap buffer */
        file_handle->count = -1;

        if(bytesused)
            {
            if(file_handle->flags & _FILE_BUFDIRTY)
                {
                /* write used portion of buffer out to filing system
                 * at the correct place, updating seqptr
                */
                file_handle->flags &= ~_FILE_BUFDIRTY;
                res = file__write(file_handle->bufbase, 1, bytesused, file_handle, file_handle->bufpos);
                }
            else
                {
                /* update filing system seqptr to the point
                 * at which the client has used the read-only buffer
                */
                res = (S32) file__seek(file_handle, file_handle->bufpos + bytesused, SEEK_SET);
                }
            }
        else
            res = 0;
        }
    else
        res = 0;

#if TRACE_ALLOWED
    if(res < 0)
        {
        trace_3(TRACE_MODULE_FILE | TRACE_OUT,
                "*** file_flushbuffer(" PTR_XTFMT ") from %s returning error %d ***",
                report_ptr_cast(file_handle), caller, res);
        }
#endif

    return(res);
}

#if RISCOS

/******************************************************************************
*
* if an os call yielded an error, try to remember it
*
* --out--
*
*   TRUE    if call was ok
*   FALSE   if call yielded an error (this being stored too)
*
******************************************************************************/

static S32
file__obtain_error_string(
    const _kernel_oserror * e)
{
    return(e ? file__set_error_string(e->errmess) : FALSE);
}

#endif /* RISCOS */

/******************************************************************************
*
* read a buffer from file at a given position. updates seqptr
*
* --in--
*
*   all parameters must be valid, file open
*
* --out--
*
*   <  0  error condition
*   >= 0  nmemb read
*
******************************************************************************/

static S32
file__read(
    P_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle)
{
    size_t bytestoread = size * nmemb;
    size_t bytesread;
    size_t membersread;

    trace_6(TRACE_MODULE_FILE,
            "file__read(" PTR_XTFMT ", %u, %u, " PTR_XTFMT "): nbytes %u <- handle %d",
            report_ptr_cast(ptr), size, nmemb, report_ptr_cast(file_handle), size * nmemb, file_handle->handle);

    /* sticky error for ANSI closeness */
    if(file_handle->error)
        {
        trace_1(TRACE_MODULE_FILE,
                "file__read returns sticky error %d", file_handle->error);
        return(file_handle->error);
        }

    {
#if RISCOS
    _kernel_osgbpb_block blk;

    blk.dataptr = ptr;
    blk.nbytes  = bytestoread;

    if(_kernel_ERROR == _kernel_osgbpb(OSGBPB_ReadFromPTR, file_handle->handle, &blk))
        {
        (void) file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
        return(file__obtain_error_string(_kernel_last_oserror()));
        }
    else
        bytesread = bytestoread - blk.nbytes;
#elif WINDOWS
    if(!ReadFile(file_handle->handle, ptr, bytestoread, (LPDWORD) &bytesread, NULL))
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));
#else
    membersread = fread(ptr, size, nmemb, file_handle->handle);
#endif
    } /*block*/

    /* save spurious division */
    membersread = (size == 1) ? bytesread : bytesread / size;

    trace_1(TRACE_MODULE_FILE, "file__read read %d members", membersread);
    return(membersread);
}

/******************************************************************************
*
* internal seek; doesn't flush buffers
* assumed done prior to this
* file must be valid and open
*
******************************************************************************/

static S32
file__seek(
    FILE_HANDLE file_handle,
    S32 offset,
    S32 origin)
{
    S32 newptr;

    file_handle->flags &= ~(_FILE_EOFREAD | _FILE_HASUNGOTCH);

    {
#if RISCOS
    switch(origin)
        {
        default:
#if CHECKING
            myassert3x(0, "file__seek(&%p, %d, origin=%d INVALID)", file_handle, offset, origin);

        /*FALLTHRU*/

        case SEEK_SET:
#endif
            newptr = 0;
            break;

        case SEEK_CUR:
            newptr = file__tell(file_handle);
            if(!offset)
                {
                trace_4(TRACE_MODULE_FILE,
                        "file__seek(" PTR_XTFMT ", &%8.8x, %d) (NO MOTION) yields &%8.8x",
                        report_ptr_cast(file_handle), offset, origin, newptr);
                return(newptr);
                }
            break;

        case SEEK_END:
            newptr = file_length(file_handle);
            break;
        }

    if(newptr < 0)
        (void) file__set_error(file_handle, (STATUS) newptr);
    else
        {
        newptr += offset;

        if(newptr < 0)
            newptr = file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION));
        else
            {
            _kernel_swi_regs rs;

            rs.r[0] = OSArgs_SetPTR;
            rs.r[1] = file_handle->handle;
            rs.r[2] = (int) newptr;

            if(file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs)))
                newptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
            }
        }
#elif WINDOWS
    DWORD dwMoveMethod;

    switch(origin)
    {
    default:
#if CHECKING
        myassert3(TEXT("file__seek(&%p, ") S32_TFMT TEXT(", origin=") S32_TFMT TEXT(" INVALID)"), file_handle, offset, origin);
    case SEEK_SET:
#endif
        dwMoveMethod = FILE_BEGIN;
        break;

    case SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;

    case SEEK_END:
        dwMoveMethod = FILE_END;
        break;
    }

    newptr = SetFilePointer(file_handle->handle, offset, NULL, dwMoveMethod);

    if(newptr == -1)
        switch(GetLastError())
        {
        default:
            newptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
            break;

        case ERROR_NEGATIVE_SEEK:
            newptr = file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION));
            break;
        }
#else
    if(fseek(file_handle->handle, offset, origin))
        newptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
    else
        newptr = file__tell(file_handle);
#endif
    } /*block*/

    trace_4(TRACE_MODULE_FILE, "file__seek(" PTR_XTFMT ", &%8.8x, %d) yields &%8.8x",
            report_ptr_cast(file_handle), offset, origin, newptr);
    return(newptr);
}

#if RISCOS

/******************************************************************************
*
* remember errors to get back later
*
******************************************************************************/

static S32
file__set_error_string(
    PC_U8 errorstr)
{
    trace_1(TRACE_MODULE_FILE, "file__set_error_string(%s)", errorstr);

    if(errorstr  &&  !file__errorptr)
        {
        file__errorptr = file__errorbuffer;
        strcpy(file__errorptr, errorstr);
        }
#if TRACE_ALLOWED
    else if(file__errorptr)
        trace_0(TRACE_MODULE_FILE, "*** ERROR LOST ***");
#endif

    return(errorstr != NULL);
}

#endif /* RISCOS */

static STATUS
file__set_error(
    FILE_HANDLE file_handle,
    _InVal_     STATUS status)
{
    if(status_fail(status) && !file_handle->error)
        file_handle->error = status;

    return(status);
}

/******************************************************************************
*
* return position of seqptr.
*
* --in--
*
*   all parameters must be valid, file open on filing system
*
* --out--
*
*   <  0  error condition
*   >= 0  seqptr
*
******************************************************************************/

static S32
file__tell(
    FILE_HANDLE file_handle)
{
    S32 curptr;

    /* answer easy if buffered:   pos of buffer in file
     *                          + n bytes actually buffered
     *                          - n bytes still to be used
    */
    if(file_handle->count != -1)
        {
        curptr = file__tell_using_buffer(file_handle);
        trace_2(TRACE_MODULE_FILE,
                "file__tell(" PTR_XTFMT ") yields &%8.8x", report_ptr_cast(file_handle), curptr);
        return(curptr);
        }

    /* must ask filing system */

    {
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_ReadPTR;
    rs.r[1] = file_handle->handle;

    if(file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs)))
        curptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
    else
        curptr = rs.r[2];
#elif WINDOWS
    curptr = SetFilePointer(file_handle->handle, 0, NULL, FILE_CURRENT);

    if(curptr == -1)
        curptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
#else
    curptr = ftell(file_handle->handle);

    if(curptr == EOF)
        curptr = file__set_error(file_handle, create_error(FILE_ERR_CANTREAD));
#endif
    } /*block*/

    trace_2(TRACE_MODULE_FILE, "file__tell(" PTR_XTFMT ") yields &%8.8x", report_ptr_cast(file_handle), curptr);
    return(curptr);
}

/******************************************************************************
*
* write a buffer to file at a given position. updates seqptr
*
* --in--
*
*   all parameters must be valid, file open on filing system
*
* --out--
*
*   <  0  error condition
*   >= 0  nmemb written (will be same as nmemb requested)
*
******************************************************************************/

static S32
file__write(
    PC_ANY ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE file_handle,
    S32 pos)
{
    size_t bytestowrite = size * nmemb;
    size_t byteswritten;

    trace_7(TRACE_MODULE_FILE,
            "file__write(" PTR_XTFMT ", %u, %u, " PTR_XTFMT "): nbytes %d -> handle %d at &%8.8X",
            report_ptr_cast(ptr), size, nmemb, report_ptr_cast(file_handle), size * nmemb, file_handle->handle, pos);

    /* sticky error for ANSI closeness */
    if(file_handle->error)
        {
        trace_1(TRACE_MODULE_FILE,
                "file__write returns sticky error %d", file_handle->error);
        return(file_handle->error);
        }

    {
#if RISCOS
    _kernel_osgbpb_block blk;

    blk.dataptr = (P_ANY) ptr;
    blk.nbytes  = bytestowrite;
    blk.fileptr = (int) pos;

    if(_kernel_ERROR ==
       _kernel_osgbpb(((pos != -1)
                            /* can ensure position in call */
                            ? OSGBPB_WriteAtGiven
                            : OSGBPB_WriteAtPTR),
                      file_handle->handle, &blk)
      ) {
        file__obtain_error_string(_kernel_last_oserror());
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE)));
        }

    byteswritten = bytestowrite - blk.nbytes;
#elif WINDOWS
#else
    /* ensure positioned in file for fwrite if needed */
    if(pos != -1)
        if((pos = file__seek(file_handle, pos, SEEK_SET)) < 0)
            return((S32) pos);

    nmemb = fwrite(ptr, size, nmemb, file_handle->handle);

    if(memberswritten != nmemb)
        return(file__set_error(file_handle, create_error(FILE_ERR_DEVICEFULL)));
#endif
    } /*block*/

    /* this case should never happen with a RISC OS filing system */
    /* pretty well defined to be the error we should get on MS-DOS */
    if(byteswritten != bytestowrite)
        return(file__set_error(file_handle, create_error(FILE_ERR_DEVICEFULL)));

    return(nmemb);  /* number of members written */
}

/* ***                                                                      ***
*  ***                                                                      ***
*  *** procedures that are exported for file_getc/file_putc/file_eof macros ***
*  ***                                                                      ***
*  ***                                                                      *** */

/******************************************************************************
*
* routine called from file_getc on failure
*
******************************************************************************/

extern S32
file__filbuf(
    FILE_HANDLE file_handle)
{
    char   c;
    S32 res32;
    S32    res;

    /* ++ as count got decremented to -1 or -2
     * (do even in case of naff file - might repair damage at least)
    */
    ++file_handle->count;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD));
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    if(file_handle->flags & _FILE_HASUNGOTCH)
        {
        c = file_handle->ungotch;

        /* guaranteed no buffered data in this state! */

        /* move file ptr forwards one byte in file -- ANSI clears ungot state */
        if((res32 = file__seek(file_handle, +1, SEEK_CUR)) < 0)
            return((S32) res32);

        return(c);
        }

    if(!file_handle->bufbase)
        {
        if(!(file_handle->flags & _FILE_UNBUFFERED))
            /* ensure buffered (or explicitly not, if file__defbufsiz == 0) */
            if((res = file_buffer(file_handle, NULL, file__defbufsiz)) < 0)
                return(res);

        /* DO NOT combine these two! (side effects from file_buffer()) */

        if(file_handle->flags & _FILE_UNBUFFERED)
            {
            /* explicitly unbuffered i/o required? */
            res = file_read(&c, 1, 1, file_handle);
            return((res < 0) ? res : c);
            }
        }

    if((res = file__fillbuffer(file_handle)) < 0)
        return(res);

    return(file_getc(file_handle));
}

/******************************************************************************
*
* routine called from file_putc on failure
*
* can be either
*   (i)  buffer has been filled and requires flushing, or
*   (ii) no buffer, so ensure we have an empty buffer we
*        can write to and make it dirty so it will be flushed
*
* NB. getc/putc MUST be seek etc. separated a la ANSI spec
*     or corruption of thy files wilst be the result
*
******************************************************************************/

extern S32
file__flsbuf(
    S32 c,
    FILE_HANDLE file_handle)
{
    S32 res;

    /* ++ as count got decremented to -1 or -2
     * (do even in case of naff file - might repair damage at least)
    */
    ++file_handle->count;

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD));
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* no buffered data? */
    if(file_handle->count == -1)
        {
        /* writeable file? */
        if(!(file_handle->flags & _FILE_WRITE))
            {
            file_handle->error = create_error(FILE_ERR_ACCESSDENIED);
            return(file_handle->error);
            }

        /* no buffer present? */
        if(!file_handle->bufbase)
            {
            if(!(file_handle->flags & _FILE_UNBUFFERED))
                /* ensure buffered (or explicitly not, if file__defbufsiz == 0) */
                if((res = file_buffer(file_handle, NULL, file__defbufsiz)) < 0)
                    return(res);

            /* DO NOT combine these two! (side effects from file_buffer()) */

            if(file_handle->flags & _FILE_UNBUFFERED)
                {
                /* explicitly unbuffered i/o required */
                res = file_write_err(&c, 1, 1, file_handle);
                return((res < 0) ? res : c);
                }
            }

        /* buffer is empty, make it so we can write to it,
         * noting where it belongs in the file.
        */
        if((file_handle->bufpos = file__tell(file_handle)) < 0)
            return((S32) file_handle->bufpos);

        file_handle->count    = file_handle->bufsize - 1; /* bytes of buffer still available */
        file_handle->ptr      = file_handle->bufbase;
        assert(file_handle->ptr);
        *file_handle->ptr++   = c;              /* put the char in the buffer */

        file_handle->flags   |= _FILE_BUFDIRTY; /* it is now dirty */
        file_handle->bufbytes = file_handle->bufsize;     /* bytes of buffer available in total */

        return(c);
        }

    /* file has filled output buffer, clear out to disc and reset */
    if((res = file__flushbuffer(file_handle, "file__flsbuf")) < 0)
        return(res);

    /* we expect the putc to recurse in this case to the above case ... */
    return(file_putc(c, file_handle));
}

/******************************************************************************
*
* return end-of-file status
*
******************************************************************************/

extern S32
file__eof(
    FILE_HANDLE file_handle)
{
    S32 res;

    trace_1(TRACE_MODULE_FILE, "file__eof(" PTR_XTFMT ")", report_ptr_cast(file_handle));

#if CHECKING
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        {
        assert(file_handle);
        assert(file_handle && (file_handle->magic == _FILE_MAGIC_WORD));
        return(create_error(FILE_ERR_BADHANDLE));
        }
#endif

    /* must flush internal buffers to do EOF check */
    if((res = file__flushbuffer(file_handle, "file_eof")) < 0)
        return(res);

    {
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_EOFCheck;
    rs.r[1] = file_handle->handle;

    if(file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs)))
        res = file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE));
    else
        res = (rs.r[2] != 0); /* non-zero -> EOF */
#elif WINDOWS
    int res = _eof(file_handle->handle);

    if(res == -1)
        status = file__set_error(file_handle, create_error(FILE_ERR_BADHANDLE));
    else
        status = res ? EOF : STATUS_OK;
#else
    res = feof(file_handle->handle);
#endif
    } /*block*/

    trace_2(TRACE_MODULE_FILE, "file__eof(" PTR_XTFMT ") yields %d", report_ptr_cast(file_handle), res);
    return(res);
}

/* end of file.c */
