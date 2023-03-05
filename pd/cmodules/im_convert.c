/* im_convert.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2015-2023 Stuart Swales */

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

#if defined(REMEMBER_CFSI_APP)
static PTSTR
ChangeFSI_application;
#endif

_Check_return_
static BOOL
ChangeFSI_available(void)
{
    static int available = -1;

    if(available < 0)
    {
        TCHARZ ChangeFSI_name_buffer[256];

        available = 0;

        if(NULL == _kernel_getenv("ChangeFSI$Dir", ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer)))
        {
            tstr_xstrkat(ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer), "." "ChangeFSI");

            if(file_is_file(ChangeFSI_name_buffer))
#if defined(REMEMBER_CFSI_APP)
                if(status_ok(str_set(&ChangeFSI_application, ChangeFSI_name_buffer)))
#endif
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
#if RISCOS
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
    case FILETYPE_JPEG:
    case FILETYPE_PROARTISAN:
    case FILETYPE_WATFORD_DFA:
    case FILETYPE_WAP:
    case FILETYPE_TIFF:
        return(TRUE);

    default:
        return(FALSE);
    }
#else
    switch(filetype)
    {
    case FILETYPE_ICO:
    case FILETYPE_GIF:
    case FILETYPE_WMF:
    case FILETYPE_PNG:
    case FILETYPE_JPEG:
    case FILETYPE_TIFF:
    case FILETYPE_WINDOWS_EMF:
        return(TRUE);

    default:
        break;
    }
#endif /* RISCOS */
}

#if 0 /* for diff minimization */

/******************************************************************************
*
* convert some image data via a file
*
* --out--
* -ve = error
*   0 = converted file exists, caller must delete after reading
*
******************************************************************************/

_Check_return_
extern STATUS
image_convert_do_convert_data(
    _OutRef_    P_PTSTR p_converted_name,
    _OutRef_    P_T5_FILETYPE p_t5_filetype_converted,
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;
#if WINDOWS
    GdipImage gdip_image = NULL;
#endif
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 256);
    quick_tblock_with_buffer_setup(quick_tblock);

    *p_converted_name = NULL;
    *p_t5_filetype_converted = FILETYPE_UNDETERMINED;

#if RISCOS
    /* first save the data to a temp file */
    if(status_ok(status = file_tempname_null(TEXT("cd"), NULL, 0, &quick_tblock)))
    {
        _kernel_osfile_block osfile_block;

        osfile_block.load  = (int) t5_filetype;
        osfile_block.exec  = 0;
        osfile_block.start = (int) p_data;
        osfile_block.end   = osfile_block.start + (int) n_bytes;

        if(_kernel_ERROR == _kernel_osfile(OSFile_SaveStamp, quick_tblock_tstr(&quick_tblock), &osfile_block))
            status = file_error_set(_kernel_last_oserror()->errmess);
    }

    /* and then convert that */
    if(status_ok(status))
        status = image_convert_do_convert_file(p_converted_name, p_t5_filetype_converted, quick_tblock_tstr(&quick_tblock), t5_filetype);

    // <<< (void) _tremove(quick_tblock_tstr(&quick_tblock));
#elif WINDOWS
    /* simply save the data to a temp file, converting on-the-fly */
    if(status_ok(status = file_tempname_null(TEXT("cv"), FILE_EXT_SEP_TSTR TEXT("bmp"), 0, &quick_tblock)))
        status = tstr_set(p_converted_name, quick_tblock_tstr(&quick_tblock));

    if(status_ok(status))
         status = GdipImage_New(&gdip_image);

    if(status_ok(status))
    {
        BOOL ok;

        assert(NULL != gdip_image);

        ok = GdipImage_Load_Memory(gdip_image, p_data, n_bytes, t5_filetype);

        if(ok)
            ok = GdipImage_SaveAs_BMP(gdip_image, _wstr_from_tstr(quick_tblock_tstr(&quick_tblock)));

        if(ok)
            *p_t5_filetype_converted = FILETYPE_BMP;
        else
            status = STATUS_FAIL;

        GdipImage_Dispose(&gdip_image);
    }
#endif /* OS */

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

#endif

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
image_convert_do_convert_file(
    _OutRef_    P_PTSTR p_converted_name,
    _In_z_      PCTSTR source_file_name)
{
    /*static*/ TCHARZ command_line_buffer[BUF_MAX_PATHSTRING * 3]; /* PipeDream has big stack - see ROOT_STACK_SIZE */

    PCTSTR mode = "S32,90,90";
    U32 len;

    *p_converted_name = NULL;

    /* generate tempname for converted file */
    consume_int(xsnprintf(command_line_buffer, elemof32(command_line_buffer), "<Wimp$ScrapDir>.%s", product_id()));
    if(!file_is_dir(command_line_buffer))
        status_return(file_create_directory(command_line_buffer));
    len = tstrlen32(command_line_buffer);
    consume_int(xsnprintf(command_line_buffer + len, elemof32(command_line_buffer) - len, ".cv%08x", monotime()));

    status_return(str_set(p_converted_name, command_line_buffer));

#if RISCOS

#if defined(REMEMBER_CFSI_APP)
    { /* check that there will be enough memory in the Window Manager next slot to run ChangeFSI */
#define MIN_NEXT_SLOT (512 * 1024)
    _kernel_swi_regs rs;
    rs.r[0] = -1; /* read */
    rs.r[1] = -1; /* read next slot */
    (void) cs_kernel_swi(Wimp_SlotSize, &rs);
    if(rs.r[1] < MIN_NEXT_SLOT)
    {
        rs.r[0] = -1; /* read */
        rs.r[1] = MIN_NEXT_SLOT; /* write next slot */
        (void) cs_kernel_swi(Wimp_SlotSize, &rs); /* sorry for the override, but it does need some space to run! */
    }
    } /*block*/
#else
    /* Fireworkz:RISC_OS.ImgConvert will set the next slot when the correct task is running */
#endif

    if(wimptx_os_version_query() <= RISC_OS_3_5)
        mode = "28r";

#if 1
    (void) xsnprintf(command_line_buffer, elemof32(command_line_buffer),
                TEXT("WimpTask Run %s %s %s %s -nomode -noscale") /* <options> */,
#if defined(REMEMBER_CFSI_APP)
                ChangeFSI_application,
#else
                TEXT("PipeDream:RISC_OS.ImgConvert"), /* that sets WimpSlot and calls ChangeFSI */
#endif
                source_file_name  /* <in file>  */,
                *p_converted_name /* <out file> */,
                mode              /* <mode>     */);
#else
    tstr_xstrkpy(command_line_buffer, elemof32(command_line_buffer), TEXT("WimpTask Run "));
#if defined(REMEMBER_CFSI_APP)
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), ChangeFSI_application);
#else
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), TEXT("PipeDream:RISC_OS.ImgConvert") ); /* that sets WimpSlot and calls ChangeFSI */
#endif
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), TEXT(" "));
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), source_file_name  /* <in file>  */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), TEXT(" "));
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), *p_converted_name /* <out file> */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), TEXT(" "));
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), mode              /* <mode>     */ );
    tstr_xstrkat(command_line_buffer, elemof32(command_line_buffer), TEXT(" -nomode -noscale") /* <options> */ );
#endif

    reportf(command_line_buffer);
    _kernel_oscli(command_line_buffer);
#elif WINDOWS
    status = GdipImage_New(&gdip_image);

    if(status_ok(status))
    {
        BOOL ok;

        assert(NULL != gdip_image);

        ok = GdipImage_Load_File(gdip_image, _wstr_from_tstr(source_file_name), t5_filetype);

        if(ok)
            ok = GdipImage_SaveAs_BMP(gdip_image, _wstr_from_tstr(quick_tblock_tstr(&quick_tblock)));

        if(ok)
            *p_t5_filetype_converted = FILETYPE_BMP;
        else
            status = STATUS_FAIL;

        GdipImage_Dispose(&gdip_image);
    }
#endif /* OS */

    return(STATUS_OK);
}

#if RISCOS && 0 /* for diff minimization */

/* try loading the module - just the once, mind (remember the error too) */

_Check_return_
extern STATUS
image_convert_ensure_PicConvert(void)
{
    static STATUS status = STATUS_OK;

    TCHARZ command_buffer[BUF_MAX_PATHSTRING];
    PTSTR tstr = command_buffer;

    if(STATUS_OK != status)
        return(status);

    tstr_xstrkpy(tstr, elemof32(command_buffer), "%RMLoad ");
    tstr += tstrlen32(tstr);

    status_return(status = file_find_on_path(tstr, elemof32(command_buffer) - (tstr - command_buffer), file_get_resources_path(), TEXT("PicConvert")));

    if(STATUS_OK == status)
        return(status = create_error(FILE_ERR_NOTFOUND));

    if(_kernel_ERROR == _kernel_oscli(command_buffer))
        return(status = status_nomem());

    return(status = STATUS_DONE);
}

#endif /* RISCOS */

/* end of im_convert.c */
