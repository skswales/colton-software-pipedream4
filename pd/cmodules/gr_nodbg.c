/* gr_nodbg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Undebuggable parts of chart module */

/* SKS October 1991 */

#include "common/gflags.h"

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

extern unsigned int
gr_nodbg_bring_me_the_head_of_yuri_gagarin(
    P_GR_CHARTEDITOR cep,
    GR_CHART_OBJID * id,
    GR_DIAG_OFFSET * hitObject,
    GR_COORD x, GR_COORD y,
    BOOL adjust_clicked)
{
    GR_POINT point;

    /*gr_chartedit_riscos_point_from_abs(cep, &point, x, y);*/
    point.x = x;
    point.y = y;

    return(gr_chartedit_riscos_correlate(cep, &point, id, hitObject, 64, adjust_clicked));
}

_Check_return_
extern STATUS
gr_nodbg_chart_save(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 save_filename /*const*/)
{
    DRAW_DIAG_OFFSET tag_object_offset;

    if(cp->core.p_gr_diag && cp->core.p_gr_diag->gr_riscdiag.draw_diag.length)
    {
        /* write out the RISC OS Draw file representation */
        status_return(gr_riscdiag_diagram_save_into(&cp->core.p_gr_diag->gr_riscdiag, f));
    }
    else
    {
        /* write out a null recognizable header of a RISC OS Draw file */
        P_DRAW_DIAG diag;

        diag = image_cache_search_empty();

        status_return(file_write_bytes(diag->data, diag->length, f));
    }

    /* save tag header and object */
    status_return(gr_riscdiag_tag_pd_chart_code_save_start(f, &tag_object_offset));

    /* save user tag components for chart reload - client must note preferred save using NULL handle */
    status_return(gr_chart_save_external(cp->core.ch, f, save_filename,
                                         (cp->core.ext_handle == &gr_chart_preferred_ch)
                                                    ? NULL
                                                    : cp->core.ext_handle));

    /* save tag components for chart reload */
    status_return(gr_chart_save_internal(cp, f, save_filename));

    /* pad file and update tag object header size field */
    status_return(gr_riscdiag_tag_pd_chart_code_save_end(f, &tag_object_offset));

    return(1);
}

/* end of gr_nodbg.c */
