/* im_convert.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2015-2019 Stuart Swales */

/* Module that handles conversion for image file cache */

#include "common/gflags.h"

#include "gr_chart.h"

#include "cmodules/typepack.h"

#include "swis.h" /*"C:"*/

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

_Check_return_
_Ret_z_
extern PC_USTR
product_id(void);

static TCHARZ
ChangeFSI_name_buffer[256];

_Check_return_
static BOOL
ChangeFSI_available(void)
{
    static int available = -1;

    if(available < 0)
    {
        const char * var_name = "ChangeFSI$Dir";

        available = 0;

        if(NULL == _kernel_getenv(var_name, ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer)))
        {
            tstr_xstrkat(ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer), ".ChangeFSI");

            if(file_is_file(ChangeFSI_name_buffer))
                available = 1;
        }
    }

    return(available);
}

/******************************************************************************
*
* query of RISC OS filetypes that might sensibly be converted
*
******************************************************************************/

_Check_return_
extern BOOL
image_convert_can_convert(
    _InVal_     FILETYPE_RISC_OS filetype)
{
    if(!ChangeFSI_available())
        return(FALSE);

    switch(filetype)
    {
    case FILETYPE_WV_V10:
    case FILETYPE_WV_V12:
    case FILETYPE_ICO:
    case FILETYPE_TSTEP_128W:
    case FILETYPE_RAYSHADE:
    case FILETYPE_CCIR_601:
    case FILETYPE_TCLEAR:
    case FILETYPE_DEGAS:
    case FILETYPE_GIF:
    case FILETYPE_PCX:
    case FILETYPE_QRT:
    case FILETYPE_MTV:
    case FILETYPE_BMP:
    case FILETYPE_TGA:
    case FILETYPE_CUR:
    case FILETYPE_TSTEP_800S:
    case FILETYPE_PNG:
    case FILETYPE_PCD:
    case FILETYPE_PROARTISAN:
    case FILETYPE_WATFORD_DFA:
    case FILETYPE_WAP:
    case FILETYPE_TIFF:
        return(TRUE);

    default:
        return(FALSE);
    }
}

/******************************************************************************
*
* convert an image file
*
* --out--
* -ve = error
*   0 = converted file exists, caller must delete after reading
*
******************************************************************************/

_Check_return_
extern STATUS
image_convert_do_convert(
    _OutRef_    P_PTSTR p_converted_name,
    _In_z_      PCTSTR name)
{
    static TCHARZ command_line_buffer[BUF_MAX_PATHSTRING * 3];

    PCTSTR mode = "S32,90,90";
    U32 len;

    *p_converted_name = NULL;

    tstr_xstrkpy(command_line_buffer, elemof32(command_line_buffer), "<Wimp$ScrapDir>.");
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), product_id());
    if(!file_is_dir(command_line_buffer))
        status_return(file_create_directory(command_line_buffer));
    len = tstrlen32(command_line_buffer);
    consume_int(xsnprintf(command_line_buffer + len, elemof32(command_line_buffer) - len, ".cv%08x", monotime()));

    status_return(str_set(p_converted_name, command_line_buffer));

    if(wimptx_os_version_query() <= RISC_OS_3_5)
        mode = "28r";

    tstr_xstrkpy(command_line_buffer, elemof32(command_line_buffer), "WimpTask Run ");
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), ChangeFSI_name_buffer);
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), " ");
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), name              /* <in file>  */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), " ");
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), *p_converted_name /* <out file> */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), " ");
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), mode              /* <mode>     */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), " -nomode -noscale" /* <options> */ );

    { /* check that there will be enough memory in the Window Manager next slot to run ChangeFSI */
#define MIN_NEXT_SLOT 512 * 1024
    _kernel_swi_regs rs;
    rs.r[0] = -1; /* read */
    rs.r[1] = -1; /* read next slot */
    (void) _kernel_swi(Wimp_SlotSize, &rs, &rs);
    if(rs.r[1] < MIN_NEXT_SLOT)
    {
        rs.r[0] = -1; /* read */
        rs.r[1] = MIN_NEXT_SLOT; /* write next slot */
        (void) _kernel_swi(Wimp_SlotSize, &rs, &rs); /* sorry for the override, but it does need some space to run! */
    }
    } /*block*/

    reportf(command_line_buffer);
    _kernel_oscli(command_line_buffer);

    return(STATUS_OK);
}

/* end of im_convert.c */
