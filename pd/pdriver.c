/* pdriver.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Printer driver system for PipeDream */

/* MRJC 1988 */

#include "common/gflags.h"

#include "datafmt.h"

/******************************************************************************
*
* read to specified character on this line
*
******************************************************************************/

static BOOL
read_to_char(
    uchar find,
    FILE_HANDLE input)
{
    int ch;

    while(((ch = pd_file_getc(input)) != (int) find)  &&  (ch != CR)  &&  (ch != LF))
        if(ch == EOF)
            return(FALSE);

    return((ch != CR)  &&  (ch != LF));
}

/******************************************************************************
*
* encode the highlight field in a printer driver
* convert numbers, and funnies to ASCIIs
* a field containing a ? is special,
*   it means for each character do this string
*   ? is coded ESC ?
* note can contain 0
* so ESC is coded as ESC ESC and 0 as ESC 255
*
******************************************************************************/

static BOOL
encode_highlight_field(
    uchar *buffer,
    uchar mask)
{
    uchar array[256];
    uchar *from = buffer;
    uchar *to = array;

    while(*from != '\0')
    {
        int ch;

        switch(toupper(*from))
        {
        case 'E':
            if((toupper(from[1]) == 'S')  &&  (toupper(from[2]) == 'C'))
            {
                from += 3;
                ch = (int) ESCAPE;
            }
            else
                return(FALSE);
            break;

        /* a query by itself is a special field, set the relevant bit */

        case QUERY:
            *to++ = ESCAPE;
            ch = QUERY;
            if(from[1] == '\0')
                from[2] = '\0';

            from += 2;
            h_query |= mask;
            break;

        case QUOTES:
            ch = (int) from[1];
            if(from[2] != QUOTES)
                return(FALSE);

            from += 3;
            break;

        case '$':
        case '&':
            from++;
            if(!isxdigit(*from))
                return(FALSE);

            consume(int, sscanf((char *) from, "%x", &ch));
            for( ; isxdigit(*from); from++)
                ;
            break;

        case SPACE:
        case COMMA:
            from++;
            continue;

        default:
            if((*from == '-')  ||  isdigit(*from))
            {
                if((ch = atoi((char *) from)) < 0)
                    ch += 256;
                if(*from == '-')
                    from++;
                for( ; isdigit(*from); from++)
                    ;
                break;
            }

            return(FALSE);
        }

        if(ch == 0)
        {
            *to   = ESCAPE;
            to[1] = 0xFF;
            to += 2;
        }
        else if((uchar) ch == ESCAPE)
        {
            *to = to[1] = ESCAPE;
            to += 2;
        }
        else
            *to++ = (uchar) ch;
    }

    *to = '\0';
    strcpy((char *) buffer, (char *) array);
    return(TRUE);
}

/******************************************************************************
*
* load printer driver, name = d_driver_PD
*
******************************************************************************/

/* RJM added this STATIC on 22.9.91 */
static char *current_driver_name = NULL;

extern void
kill_driver(void)
{
    (void) mystr_set(&current_driver_name, NULL);
    delete_list(&highlight_list);
    micbit = driver_loaded = FALSE;
}

extern void
load_driver(
    char **namep,
    BOOL force_load)
{
    char *name;
    FILE_HANDLE input;
    int newch;
    char array[BUF_MAX_PATHSTRING];
    uchar linarray[LIN_BUFSIZ];
    S32 error = 0;
    S32 blank_name;
    S32 res;

    /* maybe we already have the driver ? RJM 22.9.91 */
    name = *namep;

    blank_name = str_isblank(name);

    if(!force_load)
    {
        /* SKS after 4.11 21jan92 - stop comparing against the string at 0! */
        if(!blank_name)
            if(0 == strcmp(name, current_driver_name))
                return;
    }

    /* just in case it doesn't load */
    kill_driver();

    off_at_cr = 0xFF;           /* default all highlights get switched off */
    h_query = 0;    /* no query fields */

    send_linefeeds = DEFAULT_LF;    /* drivers output LF with CR by default */
    hmi_as_text = FALSE;            /* drivers output HMI as byte by default */

    if(blank_name)
        return;

    res = add_path_using_dir(array, elemof32(array), name, PDRIVERS_SUBDIR_STR);
    if(res > 0)
        res = find_filetype_option(array, FILETYPE_UNDETERMINED);

    if(res != 'T')
    {
        error = (res > 0)
                    ? create_error(ERR_NOTTABFILE)
                    : create_error(ERR_NOTFOUND);
    }

    if(error)
    {
        reperr(error, name);
        str_clr(namep);
        return;
    }

    input = pd_file_open(array, file_open_read);

    if(!input)
    {
        reperr(create_error(ERR_CANNOTOPEN), name);
        return;
    }

    /* for each field */

    for(;;)
    {
        uchar *ptr = linarray;
        S32 saveparm = 0;
        int delim;
        uchar ch;
        S32 res;

        while((newch = getfield(input, linarray, TRUE)) != TAB)
            if(newch == EOF)
            {
                driver_loaded = TRUE;
                goto ENDPOINT;
            }

        if(str_isblank(linarray))
        {
            read_to_char(CR, input);
            continue;
        }

        /* we have the first field in a record */

        switch(toupper(*ptr))
        {
        case 'H':           /* either highlight or HMI */
            ++ptr;
            if(isdigit(*ptr))
            {
                int h_no = (int) (*ptr - '1');
                uchar mask = (uchar) (1 << h_no);

                delim = getfield(input, linarray, TRUE);

                if(((delim == TAB)  ||  (delim == CR))  &&  !str_isblank(linarray))
                {
                    /* highlight on field in linarray */

                    if(!encode_highlight_field(linarray, mask))
                    {
                        linarray[15] = '\0';
                        reperr(create_error(ERR_BAD_PARM), (char *) linarray);
                        goto ENDPOINT;
                    }

                    if(status_fail(res = add_to_list(&highlight_list, ((S32) h_no) + FIRST_HIGHLIGHT, linarray)))
                    {
                        reperr_null(res);
                        goto ENDPOINT;
                    }
                }
                else
                    break;

                /* if we have a query field, don't read the others */
                if(h_query & mask)
                {
                    read_to_char(CR, input);
                    break;
                }

                delim = getfield(input, linarray, TRUE);

                if(((delim == TAB)  ||  (delim == CR))  &&  !str_isblank(linarray))
                {
                    /* use the highlight number + 256 as key for off */

                    if(!encode_highlight_field(linarray, 0))
                    {
                        linarray[15] = '\0';
                        reperr(create_error(ERR_BAD_PARM), (char *) linarray);
                        goto ENDPOINT;
                    }

                    if(status_fail(res = add_to_list(&highlight_list, ((S32) h_no) + FIRST_HIGHLIGHT + 256, linarray)))
                    {
                        reperr_null(res);
                        goto ENDPOINT;
                    }
                }
                else
                    break;

                delim = getfield(input, linarray, TRUE);

                if((delim == TAB)  ||  (delim == CR))
                {
                    /* switch off at CR ? */
                    if(toupper(*linarray) == 'N')
                        off_at_cr &= ~mask;     /* clear bit */
                    else
                        off_at_cr |= mask;      /* set bit */
                }
            }
            else if((toupper(*ptr) == 'M')  &&  (toupper(ptr[1]) == 'I'))
            {
                ch = toupper(ptr[2]);

                if(ch == 'T')
                {
                    delim = getfield(input, linarray, TRUE);

                    if((delim == TAB)  ||  (delim == CR))
                        hmi_as_text = (toupper(*linarray) == 'Y');
                }
                else
                    saveparm =  (ch == 'P') ?   HMI_P :
                                (ch == 'S') ?   HMI_S :
                                (ch == 'O') ?   HMI_O :
                                                (S32) 0;
                break;
            }

            read_to_char(CR, input);
            break;

        case 'P':           /* should be printer on or off */
            ptr++;
            if(toupper(*ptr) != 'O')
                break;

            ptr++;
            if(toupper(*ptr) == 'N')
                saveparm = P_ON;
            else if((toupper(*ptr) == 'F')  &&  (toupper(ptr[1]) == 'F'))
                saveparm = P_OFF;
            break;

        case 'E':           /* should be EP - end page */
            if(toupper(ptr[1]) == 'P')
                saveparm = E_P;
            break;

        case 'L':           /* should be LF, on or off */
            if(toupper(ptr[1]) == 'F')
            {
                delim = getfield(input, linarray, TRUE);

                if((delim == TAB)  ||  (delim == CR))
                {
                    ch = toupper(*linarray);

                    send_linefeeds = (ch == 'N') ?  FALSE :
                                     (ch == 'Y') ?  TRUE  :
                                                    DEFAULT_LF;
                }
            }
            break;

        case QUOTES:    /* character translation */
            if(ptr[2] != QUOTES)
                break;

            saveparm = (S32) ptr[1];
            break;

        default:    /* unrecognised, don't fault cos of future expansion */
            break;
        }

        if( saveparm
            &&  (((delim = getfield(input, linarray, TRUE)) == TAB)  ||  (delim == CR))
            &&  !str_isblank(linarray))
        {
            if(!encode_highlight_field(linarray, 0))
            {
                linarray[15] = '\0';
                reperr(create_error(ERR_BAD_PARM), (char *) linarray);
                goto ENDPOINT;
            }

            if(status_fail(res = add_to_list(&highlight_list, saveparm, linarray)))
            {
                reperr_null(res);
                goto ENDPOINT;
            }
        }
    }

ENDPOINT:
    pd_file_close(&input);

    /* remember driver name */
    (void) mystr_set(&current_driver_name, name);
}

/* end of pdriver.c */
