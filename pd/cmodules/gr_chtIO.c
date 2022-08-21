/* gr_chtIO.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Charting module interface */

/* SKS January 1992 */

#include "common/gflags.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __dbox_h
#include "dbox.h"
#endif

#ifndef __cs_xfersend_h
#include "cs-xfersend.h"
#endif

/*
exported header
*/

#include "gr_chart.h"

/*
local header
*/

#include "gr_chari.h"

/*
internal functions
*/

static S32
gr_chart_save_chart_without_dialog_checking_xfersend(
    PC_GR_CHART_HANDLE chp,
    P_U8 filename /*const*/,
    S32 check_xfersend,
    S32 force_constructs);

static S32
gr_save_id_now(
    P_GR_CHART cp,
    FILE_HANDLE f);

#if RISCOS

static void
gr_chart_riscos_broadcast_changed(
    P_U8 filename /*const*/);

#endif

/* --------------------------------------------------------------------------------- */

static S32 saveres;

static BOOL
gr_chart_savechartproc(
    char * filename,
    P_ANY handle)
{
    GR_CHART_HANDLE ch = handle;
    S32             res;

    res = gr_chart_save_chart_without_dialog_checking_xfersend(&ch, filename, 1 /*check xfersend safe state*/, 0 /*force constructs*/);

    saveres = res; /* for with_dialog to pick up */

    if(res > 0)
        return(TRUE);

    gr_chart_save_chart_winge(GR_CHART_MSG_CHARTSAVE_ERROR, res);

    return(FALSE);
}

static BOOL
gr_chart_savedrawproc(
    char * filename,
    P_ANY handle)
{
    GR_CHART_HANDLE ch = handle;
    S32             res;

    res = gr_chart_save_draw_file_without_dialog(&ch, filename);

    saveres = res; /* for with_dialog to pick up */

    if(res > 0)
        return(TRUE);

    gr_chart_save_chart_winge(GR_CHART_MSG_DRAWFILESAVE_ERROR, res);

    return(FALSE);
}

/******************************************************************************
*
* send Draw file over via RAM transfer
*
******************************************************************************/

static S32 diagsize;
static S32 sentsize;

static S32 /* xfersend_sendproc */
gr_chart_senddrawproc(
    P_ANY handle,
    int * maxbuf)
{
    GR_CHART_HANDLE ch = handle;
    P_GR_DIAG p_gr_diag;
    S32 to_send;
    P_U8 data;
    S32 res;

    gr_chart_diagram(&ch, &p_gr_diag);

    to_send = diagsize - sentsize;

    if(to_send > *maxbuf)
        to_send = *maxbuf;

    /* send next chunk from current offset */
    data = gr_riscdiag_getoffptr(BYTE, &p_gr_diag->gr_riscdiag, sentsize);

    res = xfersend_sendbuf(data, to_send);

    sentsize += to_send;

    return(res);
}

static S32
gr_chart__saving_chart = FALSE;

static dbox
gr_chart__save_dbox = NULL;

/******************************************************************************
*
* save out the chart using a saveas-style dialog box
*
******************************************************************************/

extern S32
gr_chart_save_chart_with_dialog(
    PC_GR_CHART_HANDLE cehp)
{
    P_GR_CHART cp;
    P_GR_DIAG p_gr_diag;
    PTSTR filename;

#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
    return(gr_chart_save_draw_file_with_dialog(cehp));
#endif

    cp = gr_chart_cp_from_ch(*cehp);
    assert(cp);

    p_gr_diag = cp->core.p_gr_diag;

    saveres  = 1;
    sentsize = 0;
    diagsize = (S32) p_gr_diag->gr_riscdiag.dd_allocsize; /* only approximate in this case */

    filename = cp->core.currentfilename;

    if(0 != (gr_chart__save_dbox = dbox_new("xfer_send")))
        {
        gr_chart__saving_chart = TRUE;

        dbox_show(gr_chart__save_dbox);

        (void) xfersend_x(FILETYPE_PIPEDREAM,
               filename ? filename :
               (char *) string_lookup(GR_CHART_MSG_DEFAULT_CHARTLEAFNAME),
               diagsize,
               gr_chart_savechartproc,
               NULL /* sendproc  */,
               NULL /* printproc */,
               (P_ANY) cp->core.ch,
               gr_chart__save_dbox, NULL, NULL); /* errors reported internally */

        dbox_hide(gr_chart__save_dbox);
        dbox_dispose(&gr_chart__save_dbox);

        gr_chart__saving_chart = FALSE;
        }

    return(saveres);
}

/******************************************************************************
*
* save out a draw file using a saveas-style dialog box
*
******************************************************************************/

extern S32
gr_chart_save_draw_file_with_dialog(
    PC_GR_CHART_HANDLE cehp)
{
    P_GR_CHART cp;
    P_GR_DIAG p_gr_diag;
    PTSTR filename;

    cp = gr_chart_cp_from_ch(*cehp);
    assert(cp);

    p_gr_diag = cp->core.p_gr_diag;

    saveres  = 1;
    sentsize = 0;
    diagsize = (S32) p_gr_diag->gr_riscdiag.dd_allocsize;

    filename = cp->core.currentdrawname;

    if(0 != (gr_chart__save_dbox = dbox_new("xfer_send")))
        {
        gr_chart__saving_chart = TRUE;

        dbox_show(gr_chart__save_dbox);

        (void) xfersend_x(FILETYPE_DRAW,
               filename ? filename :
               (char *) string_lookup(GR_CHART_MSG_DEFAULT_DRAWLEAFNAME),
               diagsize,
               gr_chart_savedrawproc,
               gr_chart_senddrawproc,
               NULL /* printproc */,
               (P_ANY) cp->core.ch,
               gr_chart__save_dbox, NULL, NULL); /* errors reported internally */

        dbox_hide(gr_chart__save_dbox);
        dbox_dispose(&gr_chart__save_dbox);

        gr_chart__saving_chart = FALSE;
        }

    return(saveres);
}

extern S32
gr_chart_saving_chart(
    P_U8 name_during_send /*out*/)
{
    if(gr_chart__saving_chart)
        if(name_during_send)
            dbox_getfield(gr_chart__save_dbox, xfersend_FName, name_during_send, BUF_MAX_PATHSTRING);

    return(gr_chart__saving_chart);
}

/* SKS after 4.12 30mar92 - enhanced file error handling */

extern void
gr_chart_save_chart_winge(
    S32 error_id,
    S32 res)
{
    U8   errfmt[256];
    P_U8 poss_file_err;

    poss_file_err = file_get_error();

    string_lookup_buffer(error_id, errfmt, elemof32(errfmt));

    if(poss_file_err)
        xstrkat(errfmt, elemof32(errfmt), " because %s");

    messagef(errfmt, string_lookup(res), poss_file_err);
}

/******************************************************************************
*
* save out the chart under its current filename
* or the given filename (setting current filename)
* and clear the modification status if safe
*
******************************************************************************/

static S32
gr_chart_save_chart_without_dialog_checking_xfersend_core(
    PC_GR_CHART_HANDLE chp,
    P_U8 filename /*const*/,
    S32 check_xfersend)
{
    P_GR_CHART   cp;
    P_U8        save_filename;
    FILE_HANDLE f;
    S32         res, res1;

    cp = gr_chart_cp_from_ch(*chp);
    assert(cp);

    save_filename = filename ? filename : cp->core.currentfilename;
    assert(save_filename);

    (void) file_get_error();

    if((res = file_open(save_filename, file_open_write, &f)) <= 0)
        return(res ? res : create_error(FILE_ERR_CANTOPEN));

    file_set_type(f, FILETYPE_PIPEDREAM);

    res = gr_nodbg_chart_save(cp, f, save_filename);

    res1 = file_close(&f);

    if(!res)
        res = res1;

    if(!res)
        res = 1;

    if(res > 0)
        {
        if(!check_xfersend || xfersend_file_is_safe())
            {
            /* make this the current file unless scrap or already there.
             * do before clear_modified to ensure the new name is picked up
            */
            if(filename /*no point setting same back*/)
                res = gr_chart_name_set(&cp->core.ch, save_filename);

            cp->core.modified = 0;

            gr_chartedit_setwintitle(cp);

            gr_chart_riscos_broadcast_changed(save_filename);
            }
        }

    return(res);
}

static S32
gr_chart_save_chart_without_dialog_checking_xfersend(
    PC_GR_CHART_HANDLE chp,
    P_U8 filename /*const*/,
    S32 check_xfersend,
    S32 force_constructs)
{
    S32 res;

#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
    return(gr_chart_save_draw_file_without_dialog(chp, filename));
#endif

    res = gr_chart_save_chart_without_dialog_checking_xfersend_core(chp, filename, check_xfersend);

    /* if save failed try without diagram to get constructs out (else deep doo doo) */

    /* SKS after 4.12 30mar92 - only do this when allowed to */
    if((res < 0) && force_constructs)
        {
        P_GR_CHART cp;
        P_GR_DIAG p_gr_diag;

        cp = gr_chart_cp_from_ch(*chp);
        assert(cp);

        p_gr_diag = cp->core.p_gr_diag;
        cp->core.p_gr_diag = NULL;

        /* whinge about proper error */
        gr_chart_save_chart_winge(GR_CHART_MSG_CHARTSAVE_ERROR, res);

        res = gr_chart_save_chart_without_dialog_checking_xfersend_core(chp, filename, check_xfersend);

        cp->core.p_gr_diag = p_gr_diag;
        }

    return(res);
}

extern S32
gr_chart_save_chart_without_dialog(
    PC_GR_CHART_HANDLE chp,
    P_U8              filename /*const*/)
{
#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
    return(gr_chart_save_draw_file_without_dialog(chp, filename));
#endif

    return(gr_chart_save_chart_without_dialog_checking_xfersend(chp, filename, 0 /*don't check xfersend state*/, 1 /*force constructs*/));
}

extern S32
gr_chart_save_draw_file_without_dialog(
    PC_GR_CHART_HANDLE chp,
    P_U8              filename /*const*/)
{
    P_GR_CHART cp;
    P_GR_DIAG p_gr_diag;
    PTSTR save_filename;
    S32 res;

    cp = gr_chart_cp_from_ch(*chp);
    assert(cp);

    p_gr_diag = cp->core.p_gr_diag;
    if(!p_gr_diag)
        return(0);

    assert(filename || cp->core.currentdrawname);

    save_filename = filename ? filename : cp->core.currentdrawname;

    res = gr_riscdiag_diagram_save(&p_gr_diag->gr_riscdiag, save_filename);

    if(res > 0)
        {
        if(xfersend_file_is_safe())
            {
            if(filename /*no point setting same back*/)
                res = gr_chart_name_set(&cp->core.ch, save_filename);

#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
            if(filename)
                str_set(&cp->core.currentfilename, save_filename);

            cp->core.modified = 0;

            gr_chartedit_setwintitle(cp);
#endif

            gr_chart_riscos_broadcast_changed(save_filename);
            }
        }

    return(res);
}

#if RISCOS

static void
gr_chart_riscos_broadcast_changed(
    P_U8 filename /*const*/)
{
    wimp_msgstr msg;
    size_t      nBytes;

    nBytes = strlen(filename) + 1; /* for NULLCH */

    nBytes = round_up(nBytes, 4);

    msg.hdr.size   = offsetof(wimp_msgstr, data.pd_dde.type.d) + nBytes;
    msg.hdr.my_ref = 0;        /* fresh msg */
    msg.hdr.action = Wimp_MPD_DDE;

    msg.data.pd_dde.id = Wimp_MPD_DDE_DrawFileChanged;

    strcpy(msg.data.pd_dde.type.d.leafname, filename);

    wimpt_safe(wimp_sendmessage(wimp_ESEND, &msg, (wimp_t) 0));
}

#endif /* RISCOS */

/******************************************************************************
*
* maintain a translation list between cache handles and stored fillstyles
*
******************************************************************************/

static NLISTS_BLK
fillstyle_translation_table = { NULL, 0, 0 };

static S32
gr_fillstyle_create_pict_for_load(
    P_GR_CHART cp,
    U32 ekey,
    /*const*/ P_U8 picture_name)
{
    U8                picture_namebuf[BUF_MAX_PATHSTRING];
    GR_CACHE_HANDLE   cah;
    P_GR_CACHE_HANDLE cahp;
    LIST_ITEMNO       key;
    S32               res;

    /* create entry to cache using picture_name
     * note that if a name is unrooted it was either relative
     * to the chart or from the application path
    */
    if((res = file_find_on_path_or_relative(picture_namebuf, elemof32(picture_namebuf), picture_name, cp->core.currentfilename)) <= 0)
        return(res ? res : create_error(FILE_ERR_NOTFOUND));

    if((res = gr_cache_entry_ensure(&cah, picture_namebuf)) <= 0)
        return(res ? res : status_nomem());

    /* use external handle as key into list */
    key = ekey;

    if(NULL == (cahp = collect_add_entry_elem(GR_CACHE_HANDLE, &fillstyle_translation_table, &key, &res)))
        return(res);

    /* stick cache handle on the list */
    *cahp = cah;

    return(1);
}

static S32
gr_fillstyle_translate_pict_for_load(
    P_GR_FILLSTYLE fillstyle /*inout*/)
{
    LIST_ITEMNO       key;
    P_GR_CACHE_HANDLE cahp;

    if(!fillstyle)
        return(0);

    /* use external handle as key into list */
    key = (LIST_ITEMNO) fillstyle->pattern; /* small non-zero integer here! */

    if(key)
        cahp = collect_goto_item(GR_CACHE_HANDLE, &fillstyle_translation_table.lbr, key);
    else
        cahp = NULL;

    if(!cahp)
        {
        fillstyle->pattern       = 0;

        fillstyle->bits.notsolid = 0; /* force to be solid */
        fillstyle->bits.pattern  = 0;

        return(0);
        }

    fillstyle->pattern      = (GR_FILL_PATTERN_HANDLE) *cahp;

    fillstyle->bits.pattern = 1;

    return(1);
}

static U32
gr_fillstyle_translate_pict_for_save(
    PC_GR_FILLSTYLE fillstyle)
{
    LIST_ITEMNO key;
    P_U32       p_u32;

    if(!fillstyle)
        return(0);

    /* use cache handle as key into list */
    key = (LIST_ITEMNO) fillstyle->pattern;

    if(!key)
        return(0);

    if((p_u32 = collect_goto_item(U32, &fillstyle_translation_table.lbr, key)) == NULL)
        return(0);

    return(*p_u32);
}

static U32
fillstyle_translation_ekey;

/*
exported for point list enumerators to call us back when we call them!
*/

extern S32
gr_fillstyle_make_key_for_save(
    PC_GR_FILLSTYLE fillstyle)
{
    LIST_ITEMNO key;
    P_U32 p_u32;
    STATUS status;

    /* use cache handle as key into list */
    key = (LIST_ITEMNO) fillstyle->pattern;

    if(!key)
        return(0);

    if((p_u32 = collect_goto_item(U32, &fillstyle_translation_table.lbr, key)) == NULL)
        {
        if(NULL == (p_u32 = collect_add_entry_elem(U32, &fillstyle_translation_table, &key, &status)))
            return(status);

        *p_u32 = fillstyle_translation_ekey++;
        }

    return(1);
}

/******************************************************************************
*
* need to create an external id from cache handle translation list
* has to be output before any constructs that have fill patterns
*
******************************************************************************/

static S32
gr_fillstyle_table_make_for_save(
    P_GR_CHART cp)
{
    S32          res;
    U32          plotidx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES   serp;

    /* note refs to pictures */
    if((res = gr_fillstyle_make_key_for_save(&cp->chart.areastyle)) < 0)
        return(res);

    for(plotidx = 0; plotidx < (U32) (cp->d3.bits.use ? GR_CHART_N_PLOTAREAS : 1); ++plotidx)
        if((res = gr_fillstyle_make_key_for_save(&cp->plotarea.area[plotidx].areastyle)) < 0)
            return(res);

    if(cp->legend.bits.on)
        if((res = gr_fillstyle_make_key_for_save(&cp->legend.areastyle)) < 0)
            return(res);

    for(series_idx = 0; series_idx < cp->series.n_defined; ++series_idx)
        {
        /* note refs to series pictures */
        serp = getserp(cp, series_idx);
        if((res = gr_fillstyle_make_key_for_save(&serp->style.pdrop_fill)) < 0) /* this may be overenthusiastic */
            return(res);
        if((res = gr_fillstyle_make_key_for_save(&serp->style.point_fill)) < 0)
            return(res);

        /* note refs to all point info from this series */
        if((res = gr_point_list_fillstyle_enum_for_save(cp, series_idx, GR_LIST_PDROP_FILLSTYLE)) < 0) /* this may be overenthusiastic */
            return(res);
        if((res = gr_point_list_fillstyle_enum_for_save(cp, series_idx, GR_LIST_POINT_FILLSTYLE)) < 0)
            return(res);
        }

    return(1);
}

#define BUF_MAX_LOADSAVELINE 1024

/******************************************************************************
*
* maintain a current id during load and save ops
* and some state dervived therefrom for direct pokers
*
******************************************************************************/

/*
NB. this table of offsets must be kept in touch with the table of entries they index into
*/

enum GR_CONSTRUCT_ARG_TYPE
{
    GR_ARG_UNKNOWN = 0,

    GR_ARG_OBJID,
    GR_ARG_OBJID_N,
    GR_ARG_OBJID_NN,

    GR_ARG_UBF32, /* 32 bit bitfield as hex */
    GR_ARG_S32,   /* one S32 */
    GR_ARG_S32_2, /* two S32s */
    GR_ARG_S32_4, /* four S32s */
    GR_ARG_F64_SAVE, /* one F64 for saving */
    GR_ARG_F64_LOAD, /* one F64 for loading */

    GR_ARG_PICT_TRANS,

    GR_ARG_FILLSTYLE,
    GR_ARG_LINESTYLE,
    GR_ARG_TEXTSTYLE,

    GR_ARG_BARCHSTYLE_SAVE,
    GR_ARG_BARCHSTYLE_LOAD,
    GR_ARG_BARLINECHSTYLE_SAVE,
    GR_ARG_BARLINECHSTYLE_LOAD,
    GR_ARG_LINECHSTYLE_SAVE,
    GR_ARG_LINECHSTYLE_LOAD,
    GR_ARG_PIECHDISPLSTYLE_SAVE,
    GR_ARG_PIECHDISPLSTYLE_LOAD,
    GR_ARG_PIECHLABELSTYLE, /* one style does both load and save */
    GR_ARG_SCATCHSTYLE_SAVE,
    GR_ARG_SCATCHSTYLE_LOAD,

    GR_ARG_TEXTPOS,
    GR_ARG_TEXTCONTENTS,

    GR_ARG_N_TYPES
};

/*
* NB. arg formats must remain constant from day 1 is so far as
*     the ordering and prefixing of argument is concerned.
*     adding extra arguments is ok however
*/

static P_U8
gr_construct_arg_format[GR_ARG_N_TYPES + 1 /*end marker*/] =
{
    /* unknown arg type */
    ""

    /* GR_CHART_OBJID (simply expressed) */
,       U16_FMT

    /* GR_CHART_OBJID (half expressed) */
,       U16_FMT "("  U16_FMT ")"

    /* GR_CHART_OBJID (fully expressed) */
,       U16_FMT "("  U16_FMT ","  U16_FMT ")"

,   "&" U32_XFMT
,       S32_FMT
,       S32_FMT     "," S32_FMT
,       S32_FMT     "," S32_FMT     "," S32_FMT     "," S32_FMT
,       F64_FMT
,       F64_SCANF_FMT /* GR_ARG_F64_LOAD */

    /* picture translation entry */
    /* pattPTE      pictname */
,      U32_FMT      "," "%s"

    /* GR_FILLSTYLE (external fill pattern handle) */
    /*  fg           pattPTE         bits */
,   "&" U32_XFMT    "," U32_FMT    "," "&" U16_XFMT

    /* GR_LINESTYLE */
    /*  fg           pattern           width */
,   "&" U32_XFMT    "," S32_FMT    "," S32_FMT

    /* GR_TEXTSTYLE */
    /*  fg                  bg              width          height         fontname */
,   "&" U32_XFMT    "," "&" U32_XFMT    "," S32_FMT    "," S32_FMT    "," "%s"

    /* GR_BARCHSTYLE */
    /*  bits            slot_width_percentage */
,   "&" U16_XFMT    "," F64_FMT

    /* GR_BARCHSTYLE */
    /*  bits            slot_width_percentage */
,   "&" U16_XFMT    "," F64_SCANF_FMT

    /* GR_BARLINECHSTYLE */
    /*  bits            slot_depth_percentage */
,   "&" U16_XFMT    "," F64_FMT

    /* GR_BARLINECHSTYLE */
    /*  bits            slot_depth_percentage */
,   "&" U16_XFMT    "," F64_SCANF_FMT

    /* GR_LINECHSTYLE */
    /*  bits            slot_width_percentage */
,   "&" U16_XFMT    "," F64_FMT

    /* GR_LINECHSTYLE */
    /*  bits            slot_width_percentage */
,   "&" U16_XFMT    "," F64_SCANF_FMT

    /* GR_PIECHDISPLSTYLE */
    /*  bits            radial_displacement */
,   "&" U16_XFMT    "," F64_FMT

    /* GR_PIECHDISPLSTYLE */
    /*  bits            radial_displacement */
,   "&" U16_XFMT    "," F64_SCANF_FMT

    /* GR_PIECHLABELSTYLE */
    /*  bits */
,   "&" U16_XFMT

    /* GR_SCATCHSTYLE */
    /*  bits            width_percentage */
,   "&" U16_XFMT    "," F64_FMT

    /* GR_SCATCHSTYLE */
    /*  bits            width_percentage */
,   "&" U16_XFMT    "," F64_SCANF_FMT

    /* text position and size */
,       S32_FMT     "," S32_FMT     "," S32_FMT     "," S32_FMT

    /* text contents */
,   NULL
};

enum GR_CONSTRUCT_OBJ_TYPE
{
    GR_STR_NONE = 0,
    GR_STR_CHART,    /* poke field in cp */
    GR_STR_AXES,     /* poke field in cp->axes[axes_idx] */
    GR_STR_AXIS,     /* poke field in cp->axes[axes_idx].axis[axis_idx] */
    GR_STR_SERIES    /* poke field in cp->...series.mh[series_idx]  */
};

/*
NB. this table of offsets must be kept in touch with the table of entries they index into
*/

typedef enum GR_CONSTRUCT_TABLE_OFFSET
{
    GR_CON_OBJID = 0,
    GR_CON_OBJID_N,
    GR_CON_OBJID_NN,

    /*
    start of direct injection
    */

    GR_CON_DIRECT_CHART_STT, /* = wherever we are now */

    GR_CON_AXES_MAX          = GR_CON_DIRECT_CHART_STT,

    GR_CON_CORE_LAYOUT,
    GR_CON_CORE_MARGINS,

#if RISCOS
    GR_CON_EDITSAVE_BOX,
    GR_CON_EDITSAVE_SCROLLS,
#endif

    GR_CON_LEGEND_BITS,
    GR_CON_LEGEND_POSN,

    GR_CON_D3_BITS,
    GR_CON_D3_PITCH,
    GR_CON_D3_ROLL,

    GR_CON_BARCH_SLOT_2D_OVERLAP,

    GR_CON_LINECH_SLOT_2D_SHIFT,

    GR_CON_DIRECT_CHART_END,

    GR_CON_DIRECT_AXES_STT   = GR_CON_DIRECT_CHART_END,

    GR_CON_AXES_BITS         = GR_CON_DIRECT_AXES_STT,
    GR_CON_AXES_SERIES_TYPE,
    GR_CON_AXES_CHART_TYPE,
    GR_CON_AXES_START_SERIES,

    GR_CON_DIRECT_AXES_END,

    GR_CON_DIRECT_AXIS_STT   = GR_CON_DIRECT_AXES_END,

    GR_CON_AXIS_BITS         = GR_CON_DIRECT_AXIS_STT,
    GR_CON_AXIS_PUNTER_MIN,
    GR_CON_AXIS_PUNTER_MAX,

    GR_CON_AXIS_MAJOR_BITS,
    GR_CON_AXIS_MAJOR_PUNTER,

    GR_CON_AXIS_MINOR_BITS,
    GR_CON_AXIS_MINOR_PUNTER,

    GR_CON_DIRECT_AXIS_END,

    GR_CON_DIRECT_SERIES_STT = GR_CON_DIRECT_AXIS_END,

    GR_CON_SERIES_BITS       = GR_CON_DIRECT_SERIES_STT,
    GR_CON_SERIES_PIE_HEADING,
    GR_CON_SERIES_SERIES_TYPE,
    GR_CON_SERIES_CHART_TYPE,

    GR_CON_DIRECT_SERIES_END,

    /*
    end of direct injection
    */

    GR_CON_PICT_TRANS        = GR_CON_DIRECT_SERIES_END,

    GR_CON_FILLSTYLE,
    GR_CON_LINESTYLE,
    GR_CON_TEXTSTYLE,

    GR_CON_BARCHSTYLE,
    GR_CON_BARLINECHSTYLE,
    GR_CON_LINECHSTYLE,
    GR_CON_PIECHDISPLSTYLE,
    GR_CON_PIECHLABELSTYLE,
    GR_CON_SCATCHSTYLE,

    GR_CON_TEXTPOS,
    GR_CON_TEXTCONTENTS,

    GR_CON_N_TABLE_OFFSETS
}
GR_CONSTRUCT_TABLE_OFFSET;

/*
we put more into contab than clients
*/

#if CHECKING
#define gr_contab_enLSD(str, save_arg, load_arg, obj, off, contab_ix_dbg) { (str), (save_arg), (load_arg), (obj), (off), (contab_ix_dbg) }
#else
#define gr_contab_enLSD(str, save_arg, load_arg, obj, off, contab_ix_dbg) { (str), (save_arg), (load_arg), (obj), (off)                  }
#endif

#undef  gr_contab_entry
#define gr_contab_entry(str, arg, obj, off, contab_ix_dbg) gr_contab_enLSD(str, arg, arg, obj, off, contab_ix_dbg)

static GR_CONSTRUCT_TABLE_ENTRY
gr_construct_table[GR_CON_N_TABLE_OFFSETS + 1 /*end marker*/] =
{
    gr_contab_entry("OBJ,G,", GR_ARG_OBJID,    GR_STR_NONE, 0, GR_CON_OBJID),
    gr_contab_entry("OBK,G,", GR_ARG_OBJID_N,  GR_STR_NONE, 0, GR_CON_OBJID_N),
    gr_contab_entry("OBL,G,", GR_ARG_OBJID_NN, GR_STR_NONE, 0, GR_CON_OBJID_NN),

    /*
    gr_chart
    */

    gr_contab_entry("CAM,G,", GR_ARG_S32,                       GR_STR_CHART,  offsetof(GR_CHART, axes_idx_max),    GR_CON_AXES_MAX),

    gr_contab_entry("CLS,G,", GR_ARG_S32_2,                     GR_STR_CHART,  offsetof(GR_CHART, core) + offsetof(struct GR_CHART_CORE, layout) + offsetof(struct GR_CHART_LAYOUT, width),   GR_CON_CORE_LAYOUT), /* and height too */
    gr_contab_entry("CLM,G,", GR_ARG_S32_4,                     GR_STR_CHART,  offsetof(GR_CHART, core) + offsetof(struct GR_CHART_CORE, layout) + offsetof(struct GR_CHART_LAYOUT, margins), GR_CON_CORE_MARGINS),

#if RISCOS
    gr_contab_entry("RSB,G,", GR_ARG_S32_4,                     GR_STR_CHART,  offsetof(GR_CHART, core) + offsetof(struct GR_CHART_CORE, editsave) + offsetof(struct GR_CHART_EDITSAVE, open_box), GR_CON_EDITSAVE_BOX),
    gr_contab_entry("RSS,G,", GR_ARG_S32_2,                     GR_STR_CHART,  offsetof(GR_CHART, core) + offsetof(struct GR_CHART_CORE, editsave) + offsetof(struct GR_CHART_EDITSAVE, open_scx), GR_CON_EDITSAVE_SCROLLS), /* and open_scy too */
#endif

    gr_contab_entry("LGB,G,", GR_ARG_UBF32,                     GR_STR_CHART,  offsetof(GR_CHART, legend) + offsetof(struct GR_CHART_LEGEND, bits),         GR_CON_LEGEND_BITS),
    gr_contab_entry("LGC,G,", GR_ARG_S32_4,                     GR_STR_CHART,  offsetof(GR_CHART, legend) + offsetof(struct GR_CHART_LEGEND, posn),         GR_CON_LEGEND_POSN), /* and size too */

    gr_contab_entry("D3B,G,", GR_ARG_UBF32,                     GR_STR_CHART,  offsetof(GR_CHART, d3) + offsetof(struct GR_CHART_D3, bits),                 GR_CON_D3_BITS),
    gr_contab_enLSD("D3P,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_CHART,  offsetof(GR_CHART, d3) + offsetof(struct GR_CHART_D3, pitch),                GR_CON_D3_PITCH),
    gr_contab_enLSD("D3R,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_CHART,  offsetof(GR_CHART, d3) + offsetof(struct GR_CHART_D3, roll),                 GR_CON_D3_ROLL),

    gr_contab_enLSD("B2O,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_CHART,  offsetof(GR_CHART, barch) + offsetof(struct GR_CHART_BARCH, slot_overlap_percentage), GR_CON_BARCH_SLOT_2D_OVERLAP),

    gr_contab_enLSD("L2S,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_CHART,  offsetof(GR_CHART, linech) + offsetof(struct GR_CHART_LINECH, slot_shift_percentage), GR_CON_LINECH_SLOT_2D_SHIFT),

    /*
    GR_AXES
    */

    gr_contab_entry("ASB,G,", GR_ARG_UBF32,                     GR_STR_AXES,   offsetof(GR_AXES, bits),      GR_CON_AXES_BITS),
    gr_contab_entry("ASS,G,", GR_ARG_S32,                       GR_STR_AXES,   offsetof(GR_AXES, sertype),   GR_CON_AXES_SERIES_TYPE), /* default series type */
    gr_contab_entry("AST,G,", GR_ARG_S32,                       GR_STR_AXES,   offsetof(GR_AXES, charttype), GR_CON_AXES_CHART_TYPE),  /* default series chart type */
    /* SKS after 4.12 26mar92 - new construct for overlay loading */
    gr_contab_entry("AFS,G,", GR_ARG_S32,                       GR_STR_AXES,   offsetof(GR_AXES, series) + offsetof(struct GR_AXES_SERIES, start_series), GR_CON_AXES_START_SERIES),

    /*
    GR_AXIS
    */

    gr_contab_entry("AXB,G,", GR_ARG_UBF32,                     GR_STR_AXIS,   offsetof(GR_AXIS, bits),                                            GR_CON_AXIS_BITS),
    gr_contab_enLSD("AXN,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_AXIS,   offsetof(GR_AXIS, punter) + offsetof(GR_MINMAX_NUMBER, min),        GR_CON_AXIS_PUNTER_MIN),
    gr_contab_enLSD("AXP,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_AXIS,   offsetof(GR_AXIS, punter) + offsetof(GR_MINMAX_NUMBER, max),        GR_CON_AXIS_PUNTER_MAX),

    gr_contab_entry("AMB,G,", GR_ARG_UBF32,                     GR_STR_AXIS,   offsetof(GR_AXIS, major) + offsetof(GR_AXIS_TICKS, bits),   GR_CON_AXIS_MAJOR_BITS),
    gr_contab_enLSD("AMP,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_AXIS,   offsetof(GR_AXIS, major) + offsetof(GR_AXIS_TICKS, punter), GR_CON_AXIS_MAJOR_PUNTER),

    gr_contab_entry("ANB,G,", GR_ARG_UBF32,                     GR_STR_AXIS,   offsetof(GR_AXIS, minor) + offsetof(GR_AXIS_TICKS, bits),   GR_CON_AXIS_MINOR_BITS),
    gr_contab_enLSD("ANP,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_AXIS,   offsetof(GR_AXIS, minor) + offsetof(GR_AXIS_TICKS, punter), GR_CON_AXIS_MINOR_PUNTER),

    /*
    GR_SERIES
    */

    gr_contab_entry("SEB,G,", GR_ARG_UBF32,                     GR_STR_SERIES, offsetof(GR_SERIES, bits),                                                        GR_CON_SERIES_BITS),
    gr_contab_enLSD("SEP,G,", GR_ARG_F64_SAVE, GR_ARG_F64_LOAD, GR_STR_SERIES, offsetof(GR_SERIES, style) + offsetof(struct GR_SERIES_STYLE, pie_start_heading), GR_CON_SERIES_PIE_HEADING),
    gr_contab_entry("SES,G,", GR_ARG_S32,                       GR_STR_SERIES, offsetof(GR_SERIES, sertype),                                                     GR_CON_SERIES_SERIES_TYPE),
    gr_contab_entry("SET,G,", GR_ARG_S32,                       GR_STR_SERIES, offsetof(GR_SERIES, charttype),                                                   GR_CON_SERIES_CHART_TYPE),

    /*
    harder to poke, more generic types
    */

    gr_contab_entry("PTE,G,", GR_ARG_PICT_TRANS,                                        GR_STR_NONE, 0, GR_CON_PICT_TRANS),

    gr_contab_entry("FIS,G,", GR_ARG_FILLSTYLE,                                         GR_STR_NONE, 0, GR_CON_FILLSTYLE),
    gr_contab_entry("LIS,G,", GR_ARG_LINESTYLE,                                         GR_STR_NONE, 0, GR_CON_LINESTYLE),
    gr_contab_entry("TXS,G,", GR_ARG_TEXTSTYLE,                                         GR_STR_NONE, 0, GR_CON_TEXTSTYLE),

    gr_contab_enLSD("BCS,G,", GR_ARG_BARCHSTYLE_SAVE,      GR_ARG_BARCHSTYLE_LOAD,      GR_STR_NONE, 0, GR_CON_BARCHSTYLE),
    gr_contab_enLSD("BLS,G,", GR_ARG_BARLINECHSTYLE_SAVE,  GR_ARG_BARLINECHSTYLE_LOAD,  GR_STR_NONE, 0, GR_CON_BARLINECHSTYLE),
    gr_contab_enLSD("LCS,G,", GR_ARG_LINECHSTYLE_SAVE,     GR_ARG_LINECHSTYLE_LOAD,     GR_STR_NONE, 0, GR_CON_LINECHSTYLE),
    gr_contab_enLSD("PDS,G,", GR_ARG_PIECHDISPLSTYLE_SAVE, GR_ARG_PIECHDISPLSTYLE_LOAD, GR_STR_NONE, 0, GR_CON_PIECHDISPLSTYLE),
    gr_contab_entry("PLS,G,", GR_ARG_PIECHLABELSTYLE,                                   GR_STR_NONE, 0, GR_CON_PIECHLABELSTYLE),
    gr_contab_enLSD("SCS,G,", GR_ARG_SCATCHSTYLE_SAVE,     GR_ARG_SCATCHSTYLE_LOAD,     GR_STR_NONE, 0, GR_CON_SCATCHSTYLE),

    gr_contab_entry("TXB,G,", GR_ARG_TEXTPOS,                                           GR_STR_NONE, 0, GR_CON_TEXTPOS),
    gr_contab_entry("TXC,G,", GR_ARG_TEXTCONTENTS,                                      GR_STR_NONE, 0, GR_CON_TEXTCONTENTS),

    /*
    end marker
    */
    gr_contab_entry_last
};

static P_GR_CONSTRUCT_TABLE_ENTRY gr_ext_contab;
static GR_CONSTRUCT_TABLE_OFFSET  gr_ext_contab_last_ix;

extern S32
gr_chart_construct_table_register(
    P_GR_CONSTRUCT_TABLE_ENTRY ext_contab,
    U16 last_ext_contab_ix)
{
    gr_ext_contab         = ext_contab;
    gr_ext_contab_last_ix = (GR_CONSTRUCT_TABLE_OFFSET) last_ext_contab_ix;
    return(1);
}

static GR_CHART_OBJID
gr_load_save_objid;

static GR_AXES_IDX
gr_load_save_use_axes_idx;

static GR_AXIS_IDX
gr_load_save_use_axis_idx;

static GR_SERIES_IDX
gr_load_save_use_series_idx;

static S32
gr_load_save_use_series_idx_ok;

/*
a three state object:
    0 - id construct has been output
    1 - id construct awaiting output
    2 - perform id construct output
*/

static S32
gr_load_save_id_out_pending;

static S32
gr_load_save_id_change(
    P_GR_CHART cp,
    S32 in_save)
{
    gr_load_save_use_axes_idx      = 0;
    gr_load_save_use_axis_idx      = 0;
    gr_load_save_use_series_idx    = 0;
    gr_load_save_use_series_idx_ok = 0;

    /* now we have to work out what this 'selected' for direct pokers */
    switch(gr_load_save_objid.name)
        {
        default:
            assert(0);
        case GR_CHART_OBJNAME_CHART:
        case GR_CHART_OBJNAME_PLOTAREA:
        case GR_CHART_OBJNAME_LEGEND:
        case GR_CHART_OBJNAME_TEXT:
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPSER:
        case GR_CHART_OBJNAME_DROPPOINT:
        case GR_CHART_OBJNAME_BESTFITSER:
            {
            GR_SERIES_IDX series_idx;

            if(in_save)
                series_idx = gr_series_idx_from_external(cp, gr_load_save_objid.no);
            else
                {
                P_GR_SERIES serp;
                S32        res;

                /* SKS after 4.12 26mar92 - series are loaded contiguously into axes set 0 and
                 * eventually split with explicit split point to preserve overlay series styles
                */
                series_idx = gr_load_save_objid.no - 1;

                if(series_idx >= cp->axes[0].series.end_idx)
                    {
                    if((res = gr_chart_add_series(cp, 0, 1 /*init if new*/)) < 0)
                        return(res);

                    /* NB. convert using fn as allocation may have changed */
                    series_idx = gr_series_idx_from_external(cp, gr_load_save_objid.no);

                    serp = getserp(cp, series_idx);

                    /* don't auto-reinitialise descriptors created by loading */
                    serp->internal_bits.descriptor_ok = 1;
                    }
                }

            gr_load_save_use_axes_idx      = gr_axes_idx_from_series_idx(cp, series_idx);
            gr_load_save_use_series_idx    = series_idx;
            gr_load_save_use_series_idx_ok = 1;
            }
            break;

        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            /* can poke unused axes ok */

            gr_load_save_use_axis_idx = gr_axes_idx_from_external(cp, gr_load_save_objid.no, &gr_load_save_use_axes_idx);
            break;
        }

    return(1);
}

static S32
gr_construct_save(
    P_GR_CHART cp,
    FILE_HANDLE f,
    GR_CONSTRUCT_TABLE_OFFSET contab_ix,
    ...)
{
    P_GR_CHART_OBJID p_id = &gr_load_save_objid;
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    U8    args[BUF_MAX_LOADSAVELINE];
    P_U8  out = args;
    P_ANY p0;
    P_U8  p_arg_format;
    S32   res;

    assert(contab_ix < GR_CON_N_TABLE_OFFSETS);

    p_con = &gr_construct_table[contab_ix];

#if CHECKING
    assert(p_con->contab_ix_dbg == contab_ix);
#endif

    *out++ = '%';

    strcpy(out, p_con->txt_id);
    out += strlen(out);

    switch(p_con->obj_type)
        {
        default:
            /* unknown structure type */
            assert(0);
            return(1);

        case GR_STR_NONE:
            p0 = NULL;
            break;

        case GR_STR_CHART:
            assert(!gr_load_save_id_out_pending);
            assert(cp);
            p0 = cp;
            if(!p0)
                return(1);
            break;

        case GR_STR_AXES:
            assert(!gr_load_save_id_out_pending);
            assert(cp);
            p0 = cp ? &cp->axes[gr_load_save_use_axes_idx] : NULL;
            if(!p0)
                return(1);
            break;

        case GR_STR_AXIS:
            assert(!gr_load_save_id_out_pending);
            assert(cp);
            p0 = cp ? &cp->axes[gr_load_save_use_axes_idx].axis[gr_load_save_use_axis_idx] : NULL;
            if(!p0)
                return(1);
            break;

        case GR_STR_SERIES:
            assert(!gr_load_save_id_out_pending);
            assert(cp);
            assert(gr_load_save_use_series_idx_ok);
            p0 = (cp && gr_load_save_use_series_idx_ok) ? getserp(cp, gr_load_save_use_series_idx) : NULL;
            if(!p0)
                return(1);
            break;
        }

    if(p0)
        p0 = (P_ANY) ((P_U8) p0 + p_con->offset);

    p_arg_format = gr_construct_arg_format[p_con->arg_type_save];

    switch(p_con->arg_type_save)
        {
        default:
            /* unknown arg type */
            assert(0);
            break;

        case GR_ARG_OBJID:
        case GR_ARG_OBJID_N:
        case GR_ARG_OBJID_NN:
            if(!gr_load_save_id_change(cp, 1)) /* failed with silly series */
                return(1);

            switch(gr_load_save_id_out_pending)
                {
                case 0:
                    gr_load_save_id_out_pending = 1;

                    out = NULL;
                    break;

                default:
                case 1:
                    /* no-one used that id for anything so ignore it, keep this new id pending */
                    out = NULL;
                    break;

                case 2:
                    gr_load_save_id_out_pending = 0;

                    out += sprintf(out, p_arg_format,
                                   p_id->name,
                                   p_id->no,
                                   p_id->subno);
                    break;
                }
            break;

        case GR_ARG_UBF32:
            {
            P_U32 p_u32 = p0;

            assert(p_u32);

            out += sprintf(out, p_arg_format, p_u32[0]);
            }
            break;

        case GR_ARG_S32:
            {
            P_S32 p_s32 = p0;

            assert(p_s32);

            out += sprintf(out, p_arg_format, p_s32[0]);
            }
            break;

        case GR_ARG_S32_2:
            {
            P_S32 p_s32 = p0;

            assert(p_s32);

            out += sprintf(out, p_arg_format, p_s32[0], p_s32[1]);
            }
            break;

        case GR_ARG_S32_4:
            {
            P_S32 p_s32 = p0;

            assert(p_s32);

            out += sprintf(out, p_arg_format,
                           p_s32[0], p_s32[1], p_s32[2], p_s32[3]);
            }
            break;

        case GR_ARG_F64_SAVE:
            {
            P_F64 p_f64 = p0;

            assert(p_f64);

            out += sprintf(out, p_arg_format, p_f64[0]);
            }
            break;

        case GR_ARG_PICT_TRANS:
            {
            va_list extra_args;

            va_start(extra_args, contab_ix);

            out += vsprintf(out, p_arg_format, extra_args);

            va_end(extra_args);
            }
            break;

        case GR_ARG_FILLSTYLE:
            {
            GR_FILLSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_fillstyle_query(cp, p_id, &style);

            if(using_default || !style.fg.manual)
                out = NULL;
            else
                {
                /* save some bytes for invisible colours */
                if(!style.fg.visible)
                    style.fg.red = style.fg.green = style.fg.blue = 0;

                out += sprintf(out, p_arg_format,
                               style.fg,
                               gr_fillstyle_translate_pict_for_save(&style), /* use translation */
                               style.bits);
                }
            }
            break;

        case GR_ARG_LINESTYLE:
            {
            GR_LINESTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_linestyle_query(cp, p_id, &style);

            if(using_default || !style.fg.manual)
                out = NULL;
            else
                {
                /* save some bytes for invisible colours */
                if(!style.fg.visible)
                    style.fg.red = style.fg.green = style.fg.blue = 0;

                out += sprintf(out, p_arg_format,
                               style.fg,
                               style.pattern,
                               style.width);
                }
            }
            break;

        case GR_ARG_TEXTSTYLE:
            {
            GR_TEXTSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_textstyle_query(cp, p_id, &style);

            if(using_default || !style.fg.manual)
                out = NULL;
            else
                {
                /* save some bytes for invisible colours */
                if(!style.fg.visible)
                    style.fg.red = style.fg.green = style.fg.blue = 0;

                if(!style.bg.visible)
                    style.bg.red = style.bg.green = style.bg.blue = 0;

                out += sprintf(out, p_arg_format,
                               style.fg,
                               style.bg,
                               style.width,
                               style.height,
                               style.szFontName);
                }
            }
            break;

        case GR_ARG_BARCHSTYLE_SAVE:
            {
            GR_BARCHSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_barchstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits,
                               style.slot_width_percentage);
            }
            break;

        case GR_ARG_BARLINECHSTYLE_SAVE:
            {
            GR_BARLINECHSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_barlinechstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits,
                               style.slot_depth_percentage);
            }
            break;

        case GR_ARG_LINECHSTYLE_SAVE:
            {
            GR_LINECHSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_linechstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits,
                               style.slot_width_percentage);
            }
            break;

        case GR_ARG_PIECHDISPLSTYLE_SAVE:
            {
            GR_PIECHDISPLSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_piechdisplstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits,
                               style.radial_displacement);
            }
            break;

        case GR_ARG_PIECHLABELSTYLE:
            {
            GR_PIECHLABELSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_piechlabelstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits);
            }
            break;

        case GR_ARG_SCATCHSTYLE_SAVE:
            {
            GR_SCATCHSTYLE style;
            S32 using_default;

            using_default = gr_chart_objid_scatchstyle_query(cp, p_id, &style);

            if(using_default || !style.bits.manual)
                out = NULL;
            else
                out += sprintf(out, p_arg_format,
                               style.bits,
                               style.width_percentage);
            }
            break;

        case GR_ARG_TEXTPOS:
            {
            GR_BOX box;

            gr_text_box_query(cp, p_id->no, &box);

            out += sprintf(out, p_arg_format,
                           box.x0, box.y0, box.x1, box.y1);
            }
            break;

        case GR_ARG_TEXTCONTENTS:
            {
            va_list extra_args;

            /* check flush id change */
            if(gr_load_save_id_out_pending)
                if((res = gr_save_id_now(cp, f)) < 0)
                    return(res);

            /* flush construct so far */
            if((res = file_puts(args, f)) < 0)
                return(res);

            out = args;
            *out = NULLCH;

            va_start(extra_args, contab_ix);

            if((res = gr_chart_construct_save_frag_txt(f, va_arg(extra_args, P_U8))) < 0)
                return(res);

            va_end(extra_args);
            }
            break;
        }

    /* construct cancelled; don't output to file */
    if(!out)
        return(1);

    strcat(out, "\x0A");

    /* need to flush id change first? */
    if(gr_load_save_id_out_pending)
        if((res = gr_save_id_now(cp, f)) < 0)
            return(res);

    /* output complete line */
    return(file_puts(args, f));
}

static S32
gr_save_id(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    P_GR_CHART_OBJID p_id = &gr_load_save_objid;
    GR_CONSTRUCT_TABLE_OFFSET contab_ix;

    if(p_id->has_subno)
        contab_ix = GR_CON_OBJID_NN;
    else if(p_id->has_no)
        contab_ix = GR_CON_OBJID_N;
    else
        contab_ix = GR_CON_OBJID;

    return(gr_construct_save(cp, f, contab_ix));
}

static S32
gr_save_id_now(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    gr_load_save_id_out_pending = 2;

    return(gr_save_id(cp, f));
}

/******************************************************************************
*
* create and save out a table of fill pattern translations
*
******************************************************************************/

static S32
gr_fillstyle_table_save(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 filename /*const*/)
{
    S32   res;
    U32   ekey;
    P_U32 p_u32;

    /* all picture references start going out from this value,
     * rather than as their current cache handles
    */
    fillstyle_translation_ekey = 1;

    if((res = gr_fillstyle_table_make_for_save(cp)) < 0)
        return(res);

    /* loop over the list saving correspondences */
    for(ekey = 1;
        ekey < fillstyle_translation_ekey;
        ekey++)
        {
        LIST_ITEMNO key;

        for(p_u32 = collect_first(U32, &fillstyle_translation_table.lbr, &key);
            p_u32;
            p_u32 = collect_next( U32, &fillstyle_translation_table.lbr, &key))
            {
            if(ekey == *p_u32)
                {
                GR_CACHE_HANDLE cah;
                U8              namebuffer[BUF_MAX_PATHSTRING];
                P_U8            picture_name;

                cah = (GR_CACHE_HANDLE) key;

                gr_cache_name_query(&cah, namebuffer, sizeof(namebuffer)-1);

                picture_name = namebuffer;

                {
                P_FILE_PATHENUM path;
                U8 pathbuffer[BUF_MAX_PATHSTRING];
                P_U8 pathelem;

                /* loop over path to find minimalist reference */
                file_combined_path(pathbuffer, elemof32(pathbuffer), file_is_rooted(filename) ? filename : NULL);

                pathelem = file_path_element_first(&path, pathbuffer);

                while(pathelem)
                    {
                    U32 pathlen = strlen(pathelem);

                    if(0 == _strnicmp(picture_name, pathelem, pathlen))
                        {
                        picture_name += pathlen;

                        file_path_element_close(&path);
                        break;
                        }

                    pathelem = file_path_element_next(&path);
                    }
                }

                if((res = gr_construct_save(cp, f, GR_CON_PICT_TRANS,
                                            /* extra extra */
                                            ekey, picture_name)) < 0)
                    return(res);

                /* stop this loop, go find another key to look up */
                break;
                }
            }
        }

    return(1);
}

static S32
gr_chart_save_chart(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    U32 contab_ix;
    S32 res;

    gr_load_save_objid = gr_chart_objid_chart;

    if((res = gr_save_id_now(cp, f)) < 0)
        return(res);

    /* blast over chart/plotarea/legend etc. direct pokers */
    for(contab_ix = GR_CON_DIRECT_CHART_STT;
        contab_ix < GR_CON_DIRECT_CHART_END;
        contab_ix++)
        if((res = gr_construct_save(cp, f, (GR_CONSTRUCT_TABLE_OFFSET) contab_ix)) < 0)
            return(res);

    if((res = gr_construct_save(cp, f, GR_CON_FILLSTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_TEXTSTYLE)) < 0)
        return(res);

    return(1);
}

static S32
gr_chart_save_plotareas(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    S32 res;
    U32 plotidx;

    gr_chart_objid_clear(&gr_load_save_objid);
    gr_load_save_objid.name   = GR_CHART_OBJNAME_PLOTAREA;
    gr_load_save_objid.has_no = 1;

    for(plotidx = 0; plotidx < (U32) (cp->d3.bits.use ? GR_CHART_N_PLOTAREAS : 1); ++plotidx)
        {
        gr_load_save_objid.no = plotidx;

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        if((res = gr_construct_save(cp, f, GR_CON_FILLSTYLE)) < 0)
            return(res);
        if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
            return(res);
        }

    return(1);
}

static S32
gr_chart_save_legend(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    S32 res;

    if(!cp->legend.bits.on)
        return(1);

    gr_load_save_objid = gr_chart_objid_legend;

    if((res = gr_save_id(cp, f)) < 0)
        return(res);

    if((res = gr_construct_save(cp, f, GR_CON_FILLSTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_TEXTSTYLE)) < 0)
        return(res);

    return(1);
}

/******************************************************************************
*
* save out data on this series and its points
*
******************************************************************************/

typedef struct GR_CHART_SAVE_POINT_DATA_LIST
{
    const GR_LIST_ID          list_id;
    GR_CONSTRUCT_TABLE_OFFSET contab_ix;
    LIST_ITEMNO               key;
    struct GR_CHART_SAVE_POINT_DATA_LIST_BITS
        {
        UBF def_save : 1;
        UBF cur_save : 1;
        }
    bits;
}
* P_GR_CHART_SAVE_POINT_DATA_LIST;

typedef struct GR_CHART_SAVE_POINT_DATA
{
    P_GR_CHART_SAVE_POINT_DATA_LIST list;
    U32                             list_n;
}
* P_GR_CHART_SAVE_POINT_DATA;

static struct GR_CHART_SAVE_POINT_DATA_LIST
gr_chart_save_pdrop_data_list[] =
{
    { GR_LIST_PDROP_FILLSTYLE,      GR_CON_FILLSTYLE,      0, 0 },
    { GR_LIST_PDROP_LINESTYLE,      GR_CON_LINESTYLE,      0, 0 }
};

static struct GR_CHART_SAVE_POINT_DATA
gr_chart_save_pdrop_data =
{
    gr_chart_save_pdrop_data_list, elemof32(gr_chart_save_pdrop_data_list)
};

static struct GR_CHART_SAVE_POINT_DATA_LIST
gr_chart_save_point_data_list[] =
{
    { GR_LIST_POINT_FILLSTYLE,       GR_CON_FILLSTYLE,       0, 1 },
    { GR_LIST_POINT_LINESTYLE,       GR_CON_LINESTYLE,       0, 1 },
    { GR_LIST_POINT_TEXTSTYLE,       GR_CON_TEXTSTYLE,       0, 1 },

    { GR_LIST_POINT_BARCHSTYLE,      GR_CON_BARCHSTYLE,      0, 0 },
    { GR_LIST_POINT_BARLINECHSTYLE,  GR_CON_BARLINECHSTYLE,  0, 0 },
    { GR_LIST_POINT_LINECHSTYLE,     GR_CON_LINECHSTYLE,     0, 0 },
    { GR_LIST_POINT_PIECHDISPLSTYLE, GR_CON_PIECHDISPLSTYLE, 0, 0 },
    { GR_LIST_POINT_PIECHLABELSTYLE, GR_CON_PIECHLABELSTYLE, 0, 0 },
    { GR_LIST_POINT_SCATCHSTYLE,     GR_CON_SCATCHSTYLE,     0, 0 }
};

static struct GR_CHART_SAVE_POINT_DATA
gr_chart_save_point_data =
{
    gr_chart_save_point_data_list, elemof32(gr_chart_save_point_data_list)
};

static void
gr_chart_save_points_init_save_bits(
    P_GR_CHART_SAVE_POINT_DATA p_data)
{
    U32 ix;

    /* clear bits down */
    for(ix = 0; ix < p_data->list_n; ++ix)
        p_data->list[ix].bits.cur_save = p_data->list[ix].bits.def_save;
}

static void
gr_chart_save_points_set_save_bit(
    P_GR_CHART_SAVE_POINT_DATA p_data,
    GR_CONSTRUCT_TABLE_OFFSET  contab_ix)
{
    U32 ix;

    /* set bit for saving */
    for(ix = 0; ix < p_data->list_n; ++ix)
        if(p_data->list[ix].contab_ix == contab_ix)
            p_data->list[ix].bits.cur_save = 1;
}

static S32
gr_chart_save_points(
    P_GR_CHART    cp,
    FILE_HANDLE  f,
    GR_SERIES_IDX series_idx,
    P_GR_CHART_SAVE_POINT_DATA p_data)
{
    LIST_ITEMNO max_key = gr_point_key_from_external(GR_CHART_OBJID_SUBNO_MAX) + 1; /* off the end of saveable range */
    S32 first = 1;
    U32 ix;
    S32 res;

    /* clear keys down, end jamming ones that aren't being saved */
    for(ix = 0; ix < p_data->list_n; ++ix)
        p_data->list[ix].key = p_data->list[ix].bits.cur_save ? 0 : max_key;

    /* loop over points together */
    for(;;)
        {
        LIST_ITEMNO key = max_key;

        /* loop over all in list to find next common key */
        for(ix = 0; ix < p_data->list_n; ++ix)
            {
            if(p_data->list[ix].key != max_key)
                {
                if(((first
                        ? gr_point_list_first
                        : gr_point_list_next )
                        (cp, series_idx, &p_data->list[ix].key, p_data->list[ix].list_id)) == NULL)
                    /* ensure key becomes end jammed if point data not found */
                    p_data->list[ix].key = max_key;
                else
                    key = MIN(key, p_data->list[ix].key);
                }
            }

        first = 0;

        /* no more points */
        if(key >= max_key)
            break;

        gr_load_save_objid.subno = (U16) gr_point_external_from_key(key);

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        /* save out all points which have stopped at this key, restarting all else from this key unless end jammed */
        for(ix = 0; ix < p_data->list_n; ++ix)
            {
            if(p_data->list[ix].key != key)
                {
                if(p_data->list[ix].key != max_key)
                    p_data->list[ix].key = key;
                }
            else
                {
                if((res = gr_construct_save(cp, f, p_data->list[ix].contab_ix)) < 0)
                    return(res);
                }
            }
        }

    return(1);
}

static S32
gr_chart_save_points_and_pdrops(
    P_GR_CHART    cp,
    FILE_HANDLE  f,
    GR_SERIES_IDX series_idx)
{
    S32 res;

    gr_load_save_objid.has_subno = 1;

    gr_load_save_objid.name = GR_CHART_OBJNAME_POINT;

    if((res = gr_chart_save_points(cp, f, series_idx, &gr_chart_save_point_data)) < 0)
        return(res);

    gr_load_save_objid.name = GR_CHART_OBJNAME_DROPPOINT;

    if((res = gr_chart_save_points(cp, f, series_idx, &gr_chart_save_pdrop_data)) < 0)
        return(res);

    return(1);
}

static S32
gr_chart_save_series(
    P_GR_CHART cp,
    FILE_HANDLE f,
    GR_SERIES_IDX series_idx)
{
    U32 contab_ix;
    S32 res;
    P_GR_SERIES serp;
    GR_CHARTTYPE charttype;

    gr_chart_objid_from_series_idx(cp, series_idx, &gr_load_save_objid);

    if((res = gr_save_id_now(cp, f)) < 0)
        return(res);

    for(contab_ix = GR_CON_DIRECT_SERIES_STT;
        contab_ix < GR_CON_DIRECT_SERIES_END;
        contab_ix++)
        if((res = gr_construct_save(cp, f, (GR_CONSTRUCT_TABLE_OFFSET) contab_ix)) < 0)
            return(res);

    if((res = gr_construct_save(cp, f, GR_CON_FILLSTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_TEXTSTYLE)) < 0)
        return(res);

    serp = getserp(cp, series_idx);

    charttype = cp->axes[0].charttype;

    switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
        case GR_CHARTTYPE_SCAT:
            break;

        default:
        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            charttype = cp->axes[gr_load_save_use_axes_idx].charttype;

            if(charttype != GR_CHARTTYPE_LINE)
                charttype = GR_CHARTTYPE_BAR;

            if(serp->charttype != GR_CHARTTYPE_NONE)
                charttype = serp->charttype;
            break;
        }

    gr_chart_save_points_init_save_bits(&gr_chart_save_point_data);

    switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
            if((res = gr_construct_save(cp, f, GR_CON_PIECHDISPLSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_PIECHLABELSTYLE)) < 0)
                return(res);

            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_PIECHDISPLSTYLE);
            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_PIECHLABELSTYLE);
            break;

        case GR_CHARTTYPE_BAR:
            if((res = gr_construct_save(cp, f, GR_CON_BARCHSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_BARLINECHSTYLE)) < 0)
                return(res);

            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_BARCHSTYLE);
            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_BARLINECHSTYLE);
            break;

        case GR_CHARTTYPE_LINE:
            if((res = gr_construct_save(cp, f, GR_CON_LINECHSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_BARLINECHSTYLE)) < 0)
                return(res);

            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_LINECHSTYLE);
            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_BARLINECHSTYLE);
            break;

        case GR_CHARTTYPE_SCAT:
            if((res = gr_construct_save(cp, f, GR_CON_SCATCHSTYLE)) < 0)
                return(res);

            gr_chart_save_points_set_save_bit(&gr_chart_save_point_data, GR_CON_SCATCHSTYLE);
            break;
        }

    /*
    drop lines
    */

    gr_chart_save_points_init_save_bits(&gr_chart_save_pdrop_data);

    switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
            break;

        case GR_CHARTTYPE_LINE:
            gr_load_save_objid.name = GR_CHART_OBJNAME_DROPSER;

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        if((res = gr_construct_save(cp, f, GR_CON_FILLSTYLE)) < 0)
            return(res);
        if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
            return(res);

        gr_chart_save_points_set_save_bit(&gr_chart_save_pdrop_data, GR_CON_FILLSTYLE);
        gr_chart_save_points_set_save_bit(&gr_chart_save_pdrop_data, GR_CON_LINESTYLE);

        /* deliberate drop thru ... */

        case GR_CHARTTYPE_SCAT:
        default:
        case GR_CHARTTYPE_BAR:
            gr_load_save_objid.name = GR_CHART_OBJNAME_BESTFITSER;

            if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
                return(res);

            break;
        }

    /*
    point data
    */

    if((res = gr_chart_save_points_and_pdrops(cp, f, series_idx)) < 0)
        return(res);

    return(1);
}

static S32
gr_chart_save_axis(
    P_GR_CHART cp,
    FILE_HANDLE f,
    GR_AXES_IDX axes_idx,
    GR_AXIS_IDX axis_idx)
{
    U32 contab_ix;
    S32 res;
    U32 mmix;

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &gr_load_save_objid);

    if((res = gr_save_id_now(cp, f)) < 0)
        return(res);

    /* blast over axis direct pokers */
    for(contab_ix = GR_CON_DIRECT_AXIS_STT;
        contab_ix < GR_CON_DIRECT_AXIS_END;
        contab_ix++)
        if((res = gr_construct_save(cp, f, (GR_CONSTRUCT_TABLE_OFFSET) contab_ix)) < 0)
            return(res);

    if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
        return(res);
    if((res = gr_construct_save(cp, f, GR_CON_TEXTSTYLE)) < 0)
        return(res);

    for(mmix = GR_CHART_AXISTICK_MAJOR; mmix <= GR_CHART_AXISTICK_MINOR; ++mmix)
        {
        gr_chart_objid_from_axis_grid(&gr_load_save_objid, mmix);

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
            return(res);

        gr_chart_objid_from_axis_tick(&gr_load_save_objid, mmix);

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        if((res = gr_construct_save(cp, f, GR_CON_LINESTYLE)) < 0)
            return(res);
        }

    return(1);
}

static S32
gr_chart_save_axes(
    P_GR_CHART cp,
    FILE_HANDLE f,
    GR_AXES_IDX axes_idx)
{
    U32 contab_ix;
    GR_CHARTTYPE charttype;
    S32 res;

    gr_chart_objid_from_axes_idx(cp, axes_idx, 0, &gr_load_save_objid);

    if((res = gr_save_id_now(cp, f)) < 0)
        return(res);

    /* blast over axes direct pokers */
    for(contab_ix = GR_CON_DIRECT_AXES_STT;
        contab_ix < GR_CON_DIRECT_AXES_END;
        contab_ix++)
        if((res = gr_construct_save(cp, f, (GR_CONSTRUCT_TABLE_OFFSET) contab_ix)) < 0)
            return(res);

    /*
    save default styles from axes
    */

    charttype = cp->axes[0].charttype;

    switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
        case GR_CHARTTYPE_SCAT:
            break;

        default:
        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            charttype = cp->axes[gr_load_save_use_axes_idx].charttype;

            if(charttype != GR_CHARTTYPE_LINE)
                charttype = GR_CHARTTYPE_BAR;
            break;
        }

    switch(charttype)
        {
        case GR_CHARTTYPE_PIE:
            if((res = gr_construct_save(cp, f, GR_CON_PIECHDISPLSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_PIECHLABELSTYLE)) < 0)
                return(res);
            break;

        case GR_CHARTTYPE_BAR:
            if((res = gr_construct_save(cp, f, GR_CON_BARCHSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_BARLINECHSTYLE)) < 0)
                return(res);
            break;

        case GR_CHARTTYPE_LINE:
            if((res = gr_construct_save(cp, f, GR_CON_LINECHSTYLE)) < 0)
                return(res);
            if((res = gr_construct_save(cp, f, GR_CON_BARLINECHSTYLE)) < 0)
                return(res);
            break;

        case GR_CHARTTYPE_SCAT:
            if((res = gr_construct_save(cp, f, GR_CON_SCATCHSTYLE)) < 0)
                return(res);
            break;
        }

    return(1);
}

static S32
gr_chart_save_texts(
    P_GR_CHART cp,
    FILE_HANDLE f)
{
    S32         res;
    LIST_ITEMNO key;
    P_GR_TEXT    t;

    for(t = collect_first(GR_TEXT, &cp->text.lbr, &key);
        t;
        t = collect_next( GR_TEXT, &cp->text.lbr, &key))
        {
        if(t->bits.unused)
            continue;

        gr_chart_objid_from_text(key, &gr_load_save_objid);

        if((res = gr_save_id(cp, f)) < 0)
            return(res);

        /* we have to add static texts; live texts will be added by punter reload */
        if(!t->bits.live_text)
            if((res = gr_construct_save(cp, f, GR_CON_TEXTCONTENTS,
                                        /* extra extra */
                                        (t + 1))) < 0)
                return(res);

        if((res = gr_construct_save(cp, f,  GR_CON_TEXTPOS)) < 0)
            return(res);

        if((res = gr_construct_save(cp, f, GR_CON_TEXTSTYLE)) < 0)
            return(res);
        }

    return(1);
}

/******************************************************************************
*
* save out gr_chart's information as constructs in the Draw file
* only exported due to debugging problems
*
******************************************************************************/

extern S32
gr_chart_save_internal(
    P_GR_CHART cp,
    FILE_HANDLE f,
    P_U8 filename /*const*/)
{
    GR_AXES_IDX axes_idx;
    GR_AXIS_IDX axis_idx;
    S32 res;

    if((res = gr_fillstyle_table_save(cp, f, filename)) < 0)
        return(res);

    gr_load_save_id_out_pending = 0;

    if((res = gr_chart_save_chart(cp, f)) < 0)
        return(res);

    if((res = gr_chart_save_plotareas(cp, f)) < 0)
        return(res);

    if((res = gr_chart_save_legend(cp, f)) < 0)
        return(res);

    switch(cp->axes[0].charttype)
        {
        case GR_CHARTTYPE_PIE:
            /* save axes 0 info */
            if((res = gr_chart_save_axes(cp, f, 0)) < 0)
                return(res);

            /* no axis info to save */
            break;

        case GR_CHARTTYPE_SCAT:
            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
                {
                if((res = gr_chart_save_axes(cp, f, axes_idx)) < 0)
                    return(res);

                /* output value X and Y axes */
                for(axis_idx = 0; axis_idx <= 1; ++axis_idx)
                    if((res = gr_chart_save_axis(cp, f, axes_idx, axis_idx)) < 0)
                        return(res);
                }
            break;

        default:
        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
                {
                if((res = gr_chart_save_axes(cp, f, axes_idx)) < 0)
                    return(res);

                /* output category X and 1 or 2 value Y axes.
                 * ignore Z axes as they are currently irrelevant
                */
                for(axis_idx = 0; axis_idx <= 1; ++axis_idx)
                    {
                    /* it is pointless outputting the second, identical, category axis */
                    if((axis_idx == 0) && (axes_idx == 1))
                        continue;

                    if((res = gr_chart_save_axis(cp, f, axes_idx, axis_idx)) < 0)
                        return(res);
                    }
                }
            break;
        }

    switch(cp->axes[0].charttype)
        {
        GR_SERIES_IDX series_idx;

        case GR_CHARTTYPE_PIE:
            /* just the one series */
            series_idx = cp->pie_series_idx;
            if((res = gr_chart_save_series(cp, f, series_idx)) < 0)
                return(res);
            break;

        case GR_CHARTTYPE_SCAT:
        default:
        case GR_CHARTTYPE_BAR:
        case GR_CHARTTYPE_LINE:
            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
                for(series_idx = cp->axes[axes_idx].series.stt_idx;
                    series_idx < cp->axes[axes_idx].series.end_idx;
                    series_idx++)
                        if((res = gr_chart_save_series(cp, f, series_idx)) < 0)
                            return(res);
            break;
        }

    if((res = gr_chart_save_texts(cp, f)) < 0)
        return(0);

    collect_delete(&fillstyle_translation_table.lbr);

    return(res);
}

static struct CONSTRUCT_QUICK_CVT
{
    P_U8 str;
    U8   ch;
}
construct_cvt[] =
{
    { "|J", '\x0A' },
    { "||", '|'    }
};

extern S32
gr_chart_construct_save_frag_txt(
    FILE_HANDLE f,
    P_U8 args /*const*/)
{
    U8   szText[BUF_MAX_LOADSAVELINE];
    P_U8 out;
    P_U8 curptr, cvt_ptr;

    /* convert LF chars etc. in input to escape sequences in output */

    curptr = args;
    out    = szText;

    do  {
        /* keeping looking for conversions */
        S32 cvt_ix = elemof32(construct_cvt) - 1;

        cvt_ptr = NULL;

        do  {
            /* found character to replace? */
            if((cvt_ptr = strchr(curptr, construct_cvt[cvt_ix].ch)) != NULL)
                {
                U32 nbytes;

                /* copy good fragment to output stream */
                nbytes = cvt_ptr - curptr;
                memcpy32(out, curptr, nbytes);
                curptr += nbytes;
                out += nbytes;
                *out = NULLCH;

                /* skip character in input stream */
                curptr += 1;

                /* copy replacement string to output stream */
                strcpy(out, construct_cvt[cvt_ix].str);
                out += strlen(out);

                break;
                }
            }
        while(--cvt_ix >= 0);
        }
    while(cvt_ptr);

    /* copy tail fragment */
    strcpy(out, curptr);

    return(file_puts(szText, f));
}

static void
gr_chart_construct_decode_frag_txt(
    P_U8 args /*inout*/)
{
    P_U8 curptr, cvt_ptr;
    P_U8 out;

    curptr = args;
    out    = args;

    do  {
        /* keeping looking for conversions */
        S32 cvt_ix = elemof32(construct_cvt) - 1;

        cvt_ptr = NULL;

        do  {
            /* found string to replace? */
            if((cvt_ptr = strstr(curptr, construct_cvt[cvt_ix].str)) != NULL)
                {
                U32 nbytes;

                /* copy good fragment down to output pos */
                nbytes = cvt_ptr - curptr;
                memmove32(out, curptr, nbytes);
                curptr += nbytes;
                out    += nbytes;

                /* skip string from input pos */
                curptr += strlen(construct_cvt[cvt_ix].str);

                /* put replacement character in output */
                *out++ = construct_cvt[cvt_ix].ch;
                break;
                }
            }
        while(--cvt_ix >= 0);
        }
    while(cvt_ptr);

    /* move tail fragment down INCLUDING NULLCH TERMINATOR */
    if(out != curptr)
        memmove32(out, curptr, strlen32p1(curptr));
}

extern S32
gr_chart_construct_save_frag_end(
    FILE_HANDLE f)
{
    return(file_putc('\x0A', f));
}

extern S32
gr_chart_construct_save_frag_stt(
    FILE_HANDLE f,
    U16 ext_contab_ix)
{
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    U8   args[BUF_MAX_LOADSAVELINE];
    P_U8 out = args;

    assert(gr_ext_contab);
    assert(ext_contab_ix < gr_ext_contab_last_ix);

    p_con = &gr_ext_contab[ext_contab_ix];

#if CHECKING
    assert(p_con->contab_ix_dbg == ext_contab_ix);
#endif

    *out++ = '%';

    strcpy(out, p_con->txt_id);

    return(file_puts(args, f));
}

extern S32
gr_chart_construct_save_txt(
    FILE_HANDLE f,
    U16 ext_contab_ix,
    P_U8 args /*const*/)
{
    S32 res;

    if((res = gr_chart_construct_save_frag_stt(f, ext_contab_ix)) < 0)
        return(res);
    if((res = gr_chart_construct_save_frag_txt(f, args)) < 0)
        return(res);
    if((res = gr_chart_construct_save_frag_end(f)) < 0)
        return(res);

    return(1);
}

/******************************************************************************
*
* loading
*
******************************************************************************/

static S32
gr_construct_load_this(
    P_GR_CHART cp,
    P_U8 args,
    GR_CONSTRUCT_TABLE_OFFSET contab_ix)
{
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    P_GR_CHART_OBJID p_id = &gr_load_save_objid;
    P_ANY            p0, p1, p2, p3;
    P_U8             p_arg_format;
    S32              res;

    assert(contab_ix < GR_CON_N_TABLE_OFFSETS);

    p_con = &gr_construct_table[contab_ix];

    switch(p_con->obj_type)
        {
        default:
            /* unknown structure type */
            assert(0);
            return(1);

        case GR_STR_NONE:
            p0 = NULL;
            break;

        case GR_STR_CHART:
            assert(cp);
            p0 = cp;
            if(!p0)
                return(1);
            break;

        case GR_STR_AXES:
            assert(cp);
            p0 = cp ? &cp->axes[gr_load_save_use_axes_idx] : NULL;
            if(!p0)
                return(1);
            break;

        case GR_STR_AXIS:
            assert(cp);
            p0 = cp ? &cp->axes[gr_load_save_use_axes_idx].axis[gr_load_save_use_axis_idx] : NULL;
            if(!p0)
                return(1);
            break;

        case GR_STR_SERIES:
            assert(cp);
            assert(gr_load_save_use_series_idx_ok);
            p0 = (cp && gr_load_save_use_series_idx_ok) ? getserp(cp, gr_load_save_use_series_idx) : NULL;
            if(!p0)
                return(1);
            break;
        }

    if(p0)
        p0 = (P_ANY) ((P_U8) p0 + p_con->offset);

    p1 = NULL;
    p2 = NULL;
    p3 = NULL;

    res = 1;

    p_arg_format = gr_construct_arg_format[p_con->arg_type_load];

    switch(p_con->arg_type_load)
        {
        default:
            /* unknown arg type */
            assert(0);
            break;

        case GR_ARG_OBJID:
        case GR_ARG_OBJID_N:
        case GR_ARG_OBJID_NN:
            {
            U16 name  = U16_MAX;
            U16 no    = U16_MAX;
            U16 subno = U16_MAX;

            consume(int, sscanf(args, p_arg_format,
                   &name, &no, &subno));

            gr_chart_objid_clear(p_id);

            if(name != U16_MAX)
                {
                p_id->name = name;

                if(no != U16_MAX)
                    {
                    p_id->no     = no;
                    p_id->has_no = 1;

                    if(subno != U16_MAX)
                        {
                        p_id->subno     = subno;
                        p_id->has_subno = 1;
                        }
                    }
                }

            res = gr_load_save_id_change(cp, 0);
            }
            break;

        case GR_ARG_S32_4:
            p3 = (P_S32) p0 + 3;
            p2 = (P_S32) p0 + 2;

            /* deliberate drop thru ... */

        case GR_ARG_S32_2:
            p1 = (P_S32) p0 + 1;

            /* deliberate drop thru ... */

        case GR_ARG_UBF32:
        case GR_ARG_S32:
        case GR_ARG_F64_LOAD:
            /* scan up to 4 arguments in at 32-bit intervals, directly poking structure */
            consume(int, sscanf(args, p_arg_format, p0, p1, p2, p3));
            break;

        case GR_ARG_PICT_TRANS:
            {
            U8  filename[BUF_MAX_PATHSTRING];
            U32 ekey;

            consume(int, sscanf(args, p_arg_format,
                   &ekey,
                   &filename[0]));

            res = gr_fillstyle_create_pict_for_load(cp, ekey, filename);
            }
            break;

        case GR_ARG_FILLSTYLE:
            {
            GR_FILLSTYLE style;

            gr_chart_objid_fillstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.fg,
                   &style.pattern,
                   &style.bits));

            gr_fillstyle_translate_pict_for_load(&style); /* translation from PTE to cah */

            res = gr_chart_objid_fillstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_LINESTYLE:
            {
            GR_LINESTYLE style;

            gr_chart_objid_linestyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.fg,
                   &style.pattern,
                   &style.width));

            res = gr_chart_objid_linestyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_TEXTSTYLE:
            {
            GR_TEXTSTYLE style;

            gr_chart_objid_textstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.fg,
                   &style.bg,
                   &style.width,
                   &style.height,
                   &style.szFontName[0]));

            res = gr_chart_objid_textstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_BARCHSTYLE_LOAD:
            {
            GR_BARCHSTYLE style;

            gr_chart_objid_barchstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits,
                   &style.slot_width_percentage));

            res = gr_chart_objid_barchstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_BARLINECHSTYLE_LOAD:
            {
            GR_BARLINECHSTYLE style;

            gr_chart_objid_barlinechstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits,
                   &style.slot_depth_percentage));

            res = gr_chart_objid_barlinechstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_LINECHSTYLE_LOAD:
            {
            GR_LINECHSTYLE style;

            gr_chart_objid_linechstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits,
                   &style.slot_width_percentage));

            res = gr_chart_objid_linechstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_PIECHDISPLSTYLE_LOAD:
            {
            GR_PIECHDISPLSTYLE style;

            gr_chart_objid_piechdisplstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits,
                   &style.radial_displacement));

            res = gr_chart_objid_piechdisplstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_PIECHLABELSTYLE:
            {
            GR_PIECHLABELSTYLE style;

            gr_chart_objid_piechlabelstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits));

            res = gr_chart_objid_piechlabelstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_SCATCHSTYLE_LOAD:
            {
            GR_SCATCHSTYLE style;

            gr_chart_objid_scatchstyle_query(cp, p_id, &style);

            consume(int, sscanf(args, p_arg_format,
                   &style.bits,
                   &style.width_percentage));

            res = gr_chart_objid_scatchstyle_set(cp, p_id, &style);
            }
            break;

        case GR_ARG_TEXTPOS:
            {
            GR_BOX box;

            gr_text_box_query(cp, p_id->no, &box);

            consume(int, sscanf(args, p_arg_format,
                   &box.x0, &box.y0, &box.x1, &box.y1));

            res = gr_text_box_set(cp, p_id->no, &box);
            }
            break;

        case GR_ARG_TEXTCONTENTS:
            gr_chart_construct_decode_frag_txt(args);

            res = gr_text_new(cp, p_id->no, args, NULL);
            break;
        }

    return(res);
}

static S32
gr_construct_load(
    P_GR_CHART cp,
    P_U8 arg_line)
{
    P_GR_CONSTRUCT_TABLE_ENTRY i_p_con;
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    P_U8 in   = arg_line;
    P_U8 args = in;

    if(*args == '%')
    if((args = strchr(args + 1, ',')) != NULL)
    if((args = strchr(args + 1, ',')) != NULL)
        {
        /* step past construct, leadin */
        U32 con_len = ++args - ++in;

        /* lookup construct in table, throw away if unmatched */
        i_p_con = gr_construct_table;
        p_con   = i_p_con + GR_CON_N_TABLE_OFFSETS;

        /* NB --p_con kosher */
        while(--p_con >= i_p_con)
            {
            if(0 == strncmp(in, p_con->txt_id, con_len))
                return(gr_construct_load_this(cp, args, (GR_CONSTRUCT_TABLE_OFFSET) (p_con - i_p_con)));
            }

        if(gr_ext_contab && (0 != gr_ext_contab_last_ix))
            {
            /* lookup construct in external table, throw away if unmatched */
            i_p_con = gr_ext_contab;
            p_con   = i_p_con + gr_ext_contab_last_ix;

            while(--p_con >= i_p_con)
                {
                if(0 == strncmp(in, p_con->txt_id, con_len))
                    return(gr_ext_construct_load_this(cp->core.ch, args, p_con - i_p_con));
                }
            }

        /* unmatched */
        return(1);
        }

    /* ignore line anyway */
    return(1);
}

static S32
gr_chart_construct_getc(
    P_GR_CACHE_TAGSTRIP_INFO p_info)
{
    GR_DIAG_OFFSET offset;
    P_U8 p;
    U8   c;

    if(!p_info->r.goopSize)
        return(EOF);

    --p_info->r.goopSize;

    offset = p_info->r.goopOffset++;

    p = *p_info->r.ppDiag; /* this is a void **, so * yields a void * */

    c = p[offset];

    return(c);
}

static void
gr_chart_construct_ungetc(
    P_GR_CACHE_TAGSTRIP_INFO p_info)
{
    ++p_info->r.goopSize;

    --p_info->r.goopOffset;
}

static S32
gr_chart_construct_gets(
    P_GR_CACHE_TAGSTRIP_INFO p_info,
    P_U8 buffer,
    U32 bufsize)
{
    U32 count = 0;
    S32 res, newres;

    *buffer = NULLCH;

    if(!bufsize)
        return(0);

    --bufsize;

    do  {
        if((res = gr_chart_construct_getc(p_info)) == EOF)
            {
            /* EOF terminating a line is ok normally, especially if chars read */
            if(count > 0)
                return(count);

            return(EOF);
            }

        if(res == '\x0A')
            {
            /* got line terminator, read ahead */
            if((newres = gr_chart_construct_getc(p_info)) == EOF)
                /* that EOF will terminate next line immediately */
                return(count);

            /* sod alternate line terminators */
            gr_chart_construct_ungetc(p_info);
            break;
            }

        buffer[count++] = res;
        buffer[count  ] = NULLCH; /* keep terminated */
        }
    while(count < bufsize);

    return(count);
}

extern S32
gr_chart_construct_tagstrip_process(
    P_GR_CHART_HANDLE chp,
    P_GR_CACHE_TAGSTRIP_INFO p_info)
{
    U8  args[BUF_MAX_LOADSAVELINE];
    S32 res;
    P_GR_CHART cp;

    cp = gr_chart_cp_from_ch(*chp);
    assert(cp);

    while((res = gr_chart_construct_gets(p_info, args, sizeof(args)-1)) != EOF)
        {
        if((res = gr_construct_load(cp, args)) < 0)
            return(res);
        }

    /* SKS after 4.12 26mar92 - end of that lot, post-process overlay chart to preserve styles */
    if(cp->axes_idx_max > 0)
        {
        GR_ESERIES_NO first_overlay_series;

        first_overlay_series = cp->axes[1].series.start_series;
        if(first_overlay_series < 0)
            first_overlay_series = 0 - first_overlay_series;

        if(first_overlay_series /*>= 0*/ && (first_overlay_series <= cp->series.n_defined))
            {
            /* split between axes 0 and 1 sets */
            GR_SERIES_IDX first_overlay_seridx;

            first_overlay_seridx = gr_series_idx_from_external(cp, first_overlay_series);

            cp->axes[1].series.end_idx = cp->axes[0].series.end_idx;
            cp->axes[1].series.stt_idx = first_overlay_seridx;

            cp->axes[0].series.end_idx = first_overlay_seridx;

            cp->axes[0].cache.n_series = cp->axes[0].series.end_idx - cp->axes[0].series.stt_idx;
            cp->axes[1].cache.n_series = cp->axes[1].series.end_idx - cp->axes[1].series.stt_idx;

            cp->series.n = cp->axes[0].cache.n_series + cp->axes[1].cache.n_series;
            }
        }

    return(1);
}

/* end of gr_chtIO.c */
