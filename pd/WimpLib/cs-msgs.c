/* cs-msgs.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Stuart K. Swales, 06-Jun-90 */

/* This code written to replace the supplied RISC_OSLib code
 * as that had (i) too much overhead, was (ii) too slow in loading
 * and (iii) used file i/o without bothering about buffering
 *
 * See msgs.h for explanation of functions / file format
 */

#include "include.h"

#include "msgs.h"

#include "cmodules/file.h"

static char * msgs__block;

static void
__msgs_readfile(
    _In_z_      const char *filename);

static STATUS
__msgs_load_messages_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle);

/*
compare two strings backwards
*/

static int inline
__strrncmp(const char * a, const char * b, size_t n)
{
    if(n)
        {
        a += n;
        b += n;

        do
            if(*--a != *--b)
                break; /* returns number of leading characters different */
        while(--n);
        }

    return(n);
}

#define INVALID_MSGS_BLOCK ((void *) 1)

#define COMMENT_CH    '#'
#define TAG_DELIMITER ':'

extern void
msgs_init(void)
{
    char filename[256];
    int tmp_length = res_findname("Messages", filename);
    if(tmp_length > 0)
        __msgs_readfile(filename);
}

extern /*const*/ char *
msgs_lookup(/*const*/ char *tag_and_default)
{
    char tag_buffer[msgs_TAG_MAX + 1];
    const char * default_ptr;
    size_t tag_length, lookup_length;
    const char * init_lookup_ptr;

    static const char * lookup_ptr = NULL;

    default_ptr = strchr(tag_and_default, TAG_DELIMITER);
    if(!default_ptr)
        tag_length = strlen(tag_and_default);
    else
        tag_length = default_ptr - tag_and_default;

    if(tag_length > msgs_TAG_MAX)
        {
        messagef("Tag too long: %s", tag_buffer);
        return(NULL);
        }

    /* copy tag to buffer */
    *tag_buffer = NULLCH;
    strncat(tag_buffer, tag_and_default, tag_length);

    /* if there ain't a message block or it's bad then we can't do lookup */
    if(msgs__block  &&  (msgs__block != INVALID_MSGS_BLOCK))
        {
        init_lookup_ptr = lookup_ptr;

        for(;;)
            {
            /* try resuming search at last position or at start */
            if(!lookup_ptr)
                lookup_ptr = msgs__block;

            while(*lookup_ptr)
                {
                lookup_length = strlen(lookup_ptr);

                if(lookup_length > tag_length)
                    if(*(lookup_ptr + tag_length) == TAG_DELIMITER)
                        if(!__strrncmp(tag_buffer, lookup_ptr, tag_length))
                            return((char *) (lookup_ptr + tag_length + 1));

                /* step to next message */
                lookup_ptr += lookup_length + 1;

                if(lookup_ptr == init_lookup_ptr)
                    {
                    init_lookup_ptr = NULL;
                    break; /* out of both loops */
                    }
                }

            lookup_ptr = NULL; /* ensure restart */

            if(!init_lookup_ptr)
                break;
            }

        }

    /* return the default, or if that fails, the tag */
    return((char *) (default_ptr ? default_ptr + 1 : tag_and_default));
}

static void
__msgs_readfile(
    _In_z_      const char *filename)
{
    size_t real_length, alloc_length;

    {
    FILE_HANDLE file_handle;

    if(status_fail(file_open(filename, file_open_read, &file_handle)))
        return;

    real_length = __msgs_load_messages_file((P_P_ANY) &msgs__block, file_handle);

    file_close(&file_handle);

    if(!msgs__block)
        {
        /* stop msgs trying to translate during application death */
        msgs__block = INVALID_MSGS_BLOCK;
        message_output("Memory full: unable to load messages");
        return;
        }

    alloc_length = real_length;
    } /*block*/

    if(NULL == memchr(msgs__block, NULLCH, real_length)) /* 20aug96 allow loading of preprocessed messages */
        {
        /* loop over loaded messages: end -> real end of messages */
        const char * in = msgs__block;
        char * out = msgs__block;
#if TRACE
        const char * start = msgs__block;
#endif
        const char * end = in + real_length;
        int ch;
        int lastch = NULLCH;

        do  {
            ch = (in != end) ? *in++ : LF;

            if(ch == COMMENT_CH)
                {
                tracef1("[msgs_readfile: found comment at &%p]\n", in - 1);
                do
                    ch = (in != end) ? *in++ : LF;
                while((ch != LF)  &&  (ch != CR));

                if(in == end)
                    break;

                lastch = NULLCH;
                }

            if((ch == LF)  ||  (ch == CR))
                {
                if((ch ^ lastch) == (LF ^ CR))
                    {
                    tracef0("[msgs_readfile: just got the second of a pair of LF,CR or CR,LF - NULLCH already placed so loop]\n");
                    lastch = NULLCH;
                    continue;
                    }
                else
                    {
                    tracef1("[msgs_readfile: got a line terminator at &%p - place a NULLCH]\n", in - 1);
                    lastch = ch;
                    ch = NULLCH;
                    }
                }
            else
                lastch = ch;

            /* don't place two NULLCHs together or one at the start (this also means you can have blank lines) */
            if(!ch)
                {
                tracef1("[msgs_readfile: placing NULLCH at &%p]\n", out);
                if((out == msgs__block)  ||  !*(out - 1))
                    continue;
#if TRACE
                *out = ch;
                tracef1("[msgs_readfile: ended line '%s']\n", start);
                start = out + 1;
#endif
                }

            *out++ = ch;
            }
        while(in != end);

        tracef1("[msgs_readfile: placing last NULLCH at &%p]\n", out);
        *out++ = NULLCH; /* need this last byte as end marker */
        }
}

static STATUS
__msgs_load_messages_file(
    P_P_ANY p_p_any /*out*/,
    FILE_HANDLE file_handle)
{
    STATUS status;
    S32 length = file_length(file_handle);
    const U32 messages_bodge = 2; /* one for trailing linesep, one for final NUL */

    for (;;) /* loop for structure */
        {
        if(NULL == (*p_p_any = _al_ptr_alloc(length + messages_bodge, &status)))
            break;

        if((status = file_read(*p_p_any, 1, length, file_handle)) != length)
            {
            if(status_ok(status))
                status = STATUS_FAIL;
            break;
            }

        status = STATUS_OK;
        break;
        }

    if(status_fail(status))
        {
        al_ptr_dispose(p_p_any);

        return(status);
        }

    return(length);
}

/* end of cs-msgs.c */
