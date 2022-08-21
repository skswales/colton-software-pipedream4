/* slector.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that deals with load file selector */

/* RJM Feb 1988 */

#include "common/gflags.h"

#include "datafmt.h"

#include "kernel.h" /*C:*/

#include "riscos_x.h"

/******************************************************************************
*
* get the file anywhere on the current path
*
******************************************************************************/

extern BOOL
add_path_using_dir( /* never relative to current document */
    _Out_writes_z_(elemof_buffer) P_USTR filename /*out*/,
    _InVal_     U32 elemof_buffer,
    const char * src,
    _In_z_      PC_USTR dir)
{
    char buffer[BUF_MAX_PATHSTRING];
    S32 res;

    reportf("add_path_using_dir(%u:%s, dir=%s)", strlen32(src), src, dir);

    if(file_is_rooted(src))
        return(file_readable(src) > 0);

    /* first try looking up dir.src along path */
    safe_strkpy(buffer, elemof32(buffer), dir);
    safe_strkat(buffer, elemof32(buffer), FILE_DIR_SEP_STR);
    safe_strkat(buffer, elemof32(buffer), src);

    res = file_find_on_path(filename, elemof_buffer, buffer);

    if(res != 0)
        return(res);

    /* finally try looking up just src along path */
    res = file_find_on_path(filename, elemof_buffer, src);

    return(res);
}

extern BOOL
add_path_or_relative_using_dir( /* may be relative to current document */
    _Out_writes_z_(elemof_buffer) P_USTR filename /*out*/,
    _InVal_     U32 elemof_buffer,
    const char * src,
    _InVal_     BOOL allow_cwd,
    _In_z_      PC_USTR dir)
{
    char buffer[BUF_MAX_PATHSTRING];
    S32 res;

    if(!allow_cwd)
        return(add_path_using_dir(filename, elemof_buffer, src, dir));

    reportf("add_path_or_relative_using_dir(%u:%s, dir=%s)", strlen32(src), src, dir);

    if(file_is_rooted(src))
        return(file_readable(src) > 0);

    /* first try looking up dir.src along path */
    safe_strkpy(buffer, elemof32(buffer), dir);
    safe_strkat(buffer, elemof32(buffer), FILE_DIR_SEP_STR);
    safe_strkat(buffer, elemof32(buffer), src);

    res = file_find_on_path_or_relative(filename, elemof_buffer, buffer, currentfilename);

    if(res != 0)
        return(res);

    /* finally try looking up just src along path */
    res = file_find_on_path_or_relative(filename, elemof_buffer, src, currentfilename);

    return(res);
}

#ifdef UNUSED

extern char *
add_prefix_to_name_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR buffer /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR name,
    _InVal_     BOOL allow_cwd,
    _In_z_      PCTSTR dir)
{
    reportf("add_prefix_to_name_using_dir(%u:%s, dir=%s, allow_cwd=%s)", strlen32(name), name, dir, report_boolstring(allow_cwd));

    if(!file_is_rooted(name))
        {
        /* see if we can invent a file in an existing directory */
        (void) file_add_prefix_to_name(buffer, elemof_buffer, dir, allow_cwd ? currentfilename : NULL);

        if(file_is_dir(buffer) > 0)
            {
            /* dir of that name exists, so add file */
            safe_strkat(buffer, elemof_buffer, FILE_DIR_SEP_STR);
            safe_strkat(buffer, elemof_buffer, name);
            return(buffer);
            }
        }

    return(file_add_prefix_to_name(buffer, elemof_buffer, name, allow_cwd ? currentfilename : NULL));
}

#endif

extern char *
add_choices_write_prefix_to_name_using_dir(
    _Out_writes_z_(elemof_buffer) P_USTR buffer /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR name,
    _In_opt_z_  PCTSTR dir)
{
    reportf("add_choices_write_prefix_to_name_using_dir(%u:%s, dir=%u:%s)", strlen32(name), name, dir ? strlen32(dir) : 0, dir ? dir : "<NULL>");

    if(file_is_rooted(name))
        {
        safe_strkpy(buffer, elemof_buffer, name);
        return(buffer);
        }

    safe_strkpy(buffer, elemof_buffer, "<Choices$Write>" FILE_DIR_SEP_STR "PipeDream");

    (void) file_create_directory(buffer);

    if(NULL != dir)
        {
        safe_strkat(buffer, elemof_buffer, FILE_DIR_SEP_STR);
        safe_strkat(buffer, elemof_buffer, dir);

        (void) file_create_directory(buffer);
        }

    safe_strkat(buffer, elemof_buffer, FILE_DIR_SEP_STR);
    safe_strkat(buffer, elemof_buffer, name);

    return(buffer);
}

extern BOOL
checkoverwrite(
    const char *name)
{
    if(file_readable(name) > 0)
        {
        (void) init_dialog_box(D_OVERWRITE);

        if(!dialog_box_start()  ||  !dialog_box(D_OVERWRITE))
            return(FALSE);

        dialog_box_end();

        return(d_overwrite[0].option != 'N');
        }

    return(TRUE);
}

/******************************************************************************
*
*  do open, dealing with errors
*
******************************************************************************/

extern FILE_HANDLE
pd_file_open(
    const char * name,
    file_open_mode mode)
{
    FILE_HANDLE file;
    S32 res;

    trace_2(TRACE_APP_PD4, "pd_file_open(%s, %d): ", name, mode);

    /* clear lingering errors */
    (void) file_get_error();

    res = file_open(name, mode, &file);

    if(res < 0)
        {
        char * err = file_get_error();

        if(err)
            rep_fserr(err);
        else
            reperr(res, name);
        }

    trace_1(TRACE_APP_PD4, "returns " PTR_XTFMT, report_ptr_cast(file));

    return(file);
}

/******************************************************************************
*
*  close file
*
******************************************************************************/

extern S32
pd_file_close(
    FILE_HANDLE * xput)
{
    S32 res;

    res = file_close(xput);

    if(res < 0)
        {
        char * err = file_get_error();
        rep_fserr(err ? err : reperr_getstr(res));
        return(EOF);
        }

    return(0);
}

/******************************************************************************
*
* get a byte from the file
*
******************************************************************************/

extern int
pd_file_getc(
    FILE_HANDLE input)
{
    int res;

    if(file_getc_fast_ready(input))
    {
        res = file_getc_fast(input);
        return(res);
    }

    res = file_getc(input);

    if(res >= 0)
        return(res);

    if(res == EOF)
        return(EOF);

    {
    char * err = file_get_error();
    rep_fserr(err ? err : reperr_getstr(res));
    return(EOF);
    } /*block*/
}

/******************************************************************************
*
* send a byte to the file (raw)
*
******************************************************************************/

extern int
pd_file_putc(
    _InVal_     U8 ch,
    FILE_HANDLE output)
{
    int res;

    if(file_putc_fast_ready(output))
        {
        res = file_putc_fast(ch, output);
        return(res);
        }

    res = file_putc(ch, output);

    if(res >= 0)
        return(res);

    {
    char * err = file_get_error();
    rep_fserr(err ? err : reperr_getstr(res));
    return(EOF);
    } /*block*/
}

extern int
pd_file_read(
    void *ptr,
    size_t size,
    size_t nmemb,
    FILE_HANDLE stream)
{
    int res;

    res = file_read(ptr, size, nmemb, stream);

    if(res >= 0)
        return(res);

    if(res == EOF)
        return(EOF);

    {
    char * err = file_get_error();
    rep_fserr(err ? err : reperr_getstr(res));
    return(EOF);
    } /*block*/
}

/******************************************************************************
*
* send byte to file (raw)
*
******************************************************************************/

static inline BOOL
go_away_byte(
    _InVal_     U8 ch,
    FILE_HANDLE output)
{
    if(file_putc_fast_ready(output))
    {
        (void) file_putc_fast(ch, output);
        return(TRUE);
    }

    return(EOF != pd_file_putc(ch, output));
}

/******************************************************************************
*
* send byte to file (possibly interpreting CR as EOL)
*
******************************************************************************/

extern BOOL
away_byte(
    _InVal_     U8 ch,
    FILE_HANDLE output)
{
    if(ch == CR)
        return(away_eol(output));

    return(go_away_byte(ch, output));
}

/******************************************************************************
*
*  send line separator to file
*
******************************************************************************/

extern BOOL
away_eol(
    FILE_HANDLE output)
{
    U8 ch;

    switch(d_save[SAV_LINESEP].option)
        {
        case LINE_SEP_LFCR:
            if(!go_away_byte(LF, output))
                return(FALSE);

            /* drop thru */

        case LINE_SEP_CR:
            ch = CR;
            break;

        case LINE_SEP_CRLF:
            if(!go_away_byte(CR, output))
                return(FALSE);

            /* drop thru */

    /*  case LINE_SEP_LF:   */
        default:
            ch = LF;
            break;
        }

    return(go_away_byte(ch, output));
}

/******************************************************************************
*
*  send string to file (possibly interpreting CR as EOL)
*
******************************************************************************/

extern BOOL
away_string(
    _In_z_      PC_U8Z str,
    FILE_HANDLE output)
{
    U8 ch;

    /* even inline of such a simple function can be improved on to help Save speed */
    while((ch = *str++) != '\0')
    {
        if(ch == CR)
        {
            if(!away_eol(output))
                return(FALSE);
            continue;
        }

        if(file_putc_fast_ready(output))
        {
            (void) file_putc_fast(ch, output);
            continue;
        }

        if(EOF == pd_file_putc(ch, output))
            return(FALSE);
    }

    return(TRUE);
}

extern BOOL
away_string_simple(
    _In_z_      PC_U8Z str,
    FILE_HANDLE output)
{
    U8 ch;

    /* even inline of such a simple function can be improved on to help Save speed */
    while((ch = *str++) != '\0')
    {
        if(file_putc_fast_ready(output))
        {
            (void) file_putc_fast(ch, output);
            continue;
        }

        if(EOF == pd_file_putc(ch, output))
            return(FALSE);
    }

    return(TRUE);
}

/* end of slector.c */
