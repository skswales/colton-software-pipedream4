/* windvars.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Variables pertaining to each PipeDream document (window) */

/* RJM 8-Apr-1989 */

#ifndef __windvars_h
#define __windvars_h

typedef struct SS_INSTANCE_DATA
{
    SS_DOC ss_doc;
}
SS_INSTANCE_DATA;

#define SS_INSTANCE_DATA_INIT { \
    SS_DOC_INIT \
}

/*
all the document data collected together into one single object
*/

/*
NB if you change this remember to recompile everything!
*/

typedef struct DOCU
{
    struct DOCU * link;                 /* pointer to next document or NO_DOCUMENT */

    DOCNO docno;

    #define currentfilename()           (current_p_docu->Xcurrentfilename)
    PTSTR Xcurrentfilename;

    #define currentfileinfo             (current_p_docu->Xcurrentfileinfo)
    RISCOS_FILEINFO Xcurrentfileinfo;

    #define xf_loaded_from_path         (current_p_docu->Xxf_loaded_from_path)
    BOOLEAN Xxf_loaded_from_path;

    #define xf_file_is_driver           (current_p_docu->Xxf_file_is_driver)
    BOOLEAN Xxf_file_is_driver;

    #define xf_file_is_help             (current_p_docu->Xxf_file_is_help)
    BOOLEAN Xxf_file_is_help;

    #define xf_filealtered              (current_p_docu->Xxf_filealtered)
    BOOLEAN Xxf_filealtered;

    #define current_filetype_option     (current_p_docu->Xcurrent_filetype_option)
    char   Xcurrent_filetype_option;

    #define current_line_sep_option     (current_p_docu->Xcurrent_line_sep_option)
    char   Xcurrent_line_sep_option;

/* ---------------------------- slot.c ----------------------------------- */

    #define colstart                    (current_p_docu->Xcolstart)
    P_COLENTRY Xcolstart;

    #define colsintable                 (current_p_docu->Xcolsintable)
    COL    Xcolsintable;

    #define numcol                      (current_p_docu->Xnumcol)
    COL    Xnumcol;

    #define numrow                      (current_p_docu->Xnumrow)
    ROW    Xnumrow;

/* -------------------------- constr.c ----------------------------------- */

    #define linbuf                      (current_p_docu->Xlinbuf)
    uchar  Xlinbuf[LIN_BUFSIZ];

    #define lecpos                      (current_p_docu->Xlecpos)
    S32    Xlecpos;

    #define curcol                      (current_p_docu->Xcurcol)
    COL    Xcurcol;

    #define currow                      (current_p_docu->Xcurrow)
    ROW    Xcurrow;

/* ------------------------- markers.c ----------------------------------- */

    #define start_block                 (current_p_docu->Xstart_block)
    BOOLEAN Xstart_block;

    #define in_block                    (current_p_docu->Xin_block)
    SLR    Xin_block;

    #define start_bl                    (current_p_docu->Xstart_bl)
    SLR    Xstart_bl;

    #define end_bl                      (current_p_docu->Xend_bl)
    SLR    Xend_bl;

/* --------------------------- mcdiff.c ---------------------------------- */

    #define paghyt                      (current_p_docu->Xpaghyt)
    coord  Xpaghyt;

    #define pagwid                      (current_p_docu->Xpagwid)
    coord  Xpagwid;

    #define pagwid_plus1                (current_p_docu->Xpagwid_plus1)
    coord  Xpagwid_plus1;

/* ---------------------------- scdraw.c --------------------------------- */

    #define horzvec_mh                  (current_p_docu->Xhorzvec_mh)
    ARRAY_HANDLE Xhorzvec_mh;

    #define maxncol                     (current_p_docu->Xmaxncol)
    coord  Xmaxncol;

    #define vertvec_mh                  (current_p_docu->Xvertvec_mh)
    ARRAY_HANDLE Xvertvec_mh;

    #define maxnrow                     (current_p_docu->Xmaxnrow)
    coord  Xmaxnrow;

    #define out_screen                  (current_p_docu->Xout_screen)
    BOOLEAN Xout_screen;

    #define out_rebuildhorz             (current_p_docu->Xout_rebuildhorz)
    BOOLEAN Xout_rebuildhorz;

    #define out_rebuildvert             (current_p_docu->Xout_rebuildvert)
    BOOLEAN Xout_rebuildvert;

    #define out_forcevertcentre         (current_p_docu->Xout_forcevertcentre)
    BOOLEAN Xout_forcevertcentre;

    #define out_below                   (current_p_docu->Xout_below)
    BOOLEAN Xout_below;

    #define out_rowout                  (current_p_docu->Xout_rowout)
    BOOLEAN Xout_rowout;

    #define out_rowout1                 (current_p_docu->Xout_rowout1)
    BOOLEAN Xout_rowout1;

    #define out_rowborout               (current_p_docu->Xout_rowborout)
    BOOLEAN Xout_rowborout;

    #define out_rowborout1              (current_p_docu->Xout_rowborout1)
    BOOLEAN Xout_rowborout1;

    #define out_currslot                (current_p_docu->Xout_currslot)
    BOOLEAN Xout_currslot;

    #define xf_inexpression             (current_p_docu->Xxf_inexpression)
    BOOLEAN Xxf_inexpression;

    #define xf_inexpression_box         (current_p_docu->Xxf_inexpression_box)
    BOOLEAN Xxf_inexpression_box;

    #define xf_inexpression_line        (current_p_docu->Xxf_inexpression_line)
    BOOLEAN Xxf_inexpression_line;

    #define xf_blockcursor              (current_p_docu->Xxf_blockcursor)
    BOOLEAN Xxf_blockcursor;

    #define xf_draweverything           (current_p_docu->Xxf_draweverything)
    BOOLEAN Xxf_draweverything;

    #define xf_front_document_window    (current_p_docu->Xxf_front_document_window)
    BOOLEAN Xxf_front_document_window;

    #define xf_drawcolumnheadings       (current_p_docu->Xxf_drawcolumnheadings)
    BOOLEAN Xxf_drawcolumnheadings;

    #define xf_drawsome                 (current_p_docu->Xxf_drawsome)
    BOOLEAN Xxf_drawsome;

    #define xf_drawcellcoordinates      (current_p_docu->Xxf_drawcellcoordinates)
    BOOLEAN Xxf_drawcellcoordinates;

    #define xf_acquirecaret             (current_p_docu->Xxf_acquirecaret)
    BOOLEAN Xxf_acquirecaret;

    #define xf_caretreposition          (current_p_docu->Xxf_caretreposition)
    BOOLEAN Xxf_caretreposition;

    #define xf_interrupted              (current_p_docu->Xxf_interrupted)
    BOOLEAN Xxf_interrupted;

    #define  xf_draw_empty_right        (current_p_docu->Xxf_draw_empty_right)
    BOOLEAN Xxf_draw_empty_right;

    #define unused_bit_at_bottom        (current_p_docu->Xunused_bit_at_bottom)
    BOOLEAN Xunused_bit_at_bottom;

    #define rowout                      (current_p_docu->Xrowout)
    coord  Xrowout;

    #define rowout1                     (current_p_docu->Xrowout1)
    coord  Xrowout1;

    #define rowborout                   (current_p_docu->Xrowborout)
    coord  Xrowborout;

    #define rowborout1                  (current_p_docu->Xrowborout1)
    coord  Xrowborout1;

    #define maximum_cols                (current_p_docu->Xmaximum_cols)
    coord  Xmaximum_cols;

    #define cols_available              (current_p_docu->Xcols_available)
    coord  Xcols_available;

    #define colsonscreen                (current_p_docu->Xcolsonscreen)
    coord  Xcolsonscreen;

    #define n_colfixes                  (current_p_docu->Xn_colfixes)
    coord  Xn_colfixes;

    #define maximum_rows                (current_p_docu->Xmaximum_rows)
    coord  Xmaximum_rows;

    #define rows_available              (current_p_docu->Xrows_available)
    coord  Xrows_available;

    #define rowsonscreen                (current_p_docu->Xrowsonscreen)
    coord  Xrowsonscreen;

    #define n_rowfixes                  (current_p_docu->Xn_rowfixes)
    coord  Xn_rowfixes;

    #define curcoloffset                (current_p_docu->Xcurcoloffset)
    coord  Xcurcoloffset;

    #define currowoffset                (current_p_docu->Xcurrowoffset)
    coord  Xcurrowoffset;

    #define oldcol                      (current_p_docu->Xoldcol)
    COL    Xoldcol;

    #define newcol                      (current_p_docu->Xnewcol)
    COL    Xnewcol;

    #define oldrow                      (current_p_docu->Xoldrow)
    ROW    Xoldrow;

    #define newrow                      (current_p_docu->Xnewrow)
    ROW    Xnewrow;

    #define movement                    (current_p_docu->Xmovement)
    S32    Xmovement;

    #define lescrl                      (current_p_docu->Xlescrl)
    S32    Xlescrl;

    #define old_lescroll                (current_p_docu->Xold_lescroll)
    S32    Xold_lescroll;

    #define old_lecpos                  (current_p_docu->Xold_lecpos)
    S32    Xold_lecpos;

    #define last_thing_drawn            (current_p_docu->Xlast_thing_drawn)
    coord  Xlast_thing_drawn;

    #define borderwidth                 (current_p_docu->Xborderwidth)
    coord  Xborderwidth;

    #define curpnm                      (current_p_docu->Xcurpnm)
    S32    Xcurpnm;

    #define curr_scroll_x               (current_p_docu->Xcurr_scroll_x)
    gcoord Xcurr_scroll_x;

    #define curr_scroll_y               (current_p_docu->Xcurr_scroll_y)
    gcoord Xcurr_scroll_y;

/* ----------------------------- cursmov.c ------------------------------- */

    #define scrbrc                      (current_p_docu->Xscrbrc)
    coord  Xscrbrc;

    #define rowtoend                    (current_p_docu->Xrowtoend)
    coord  Xrowtoend;

/* ----------------------------- numbers.c ------------------------------- */

    #define edtslr_col                  (current_p_docu->Xedtslr_col)
    COL    Xedtslr_col;

    #define edtslr_row                  (current_p_docu->Xedtslr_row)
    ROW    Xedtslr_row;

    #define slot_in_buffer              (current_p_docu->Xslot_in_buffer)
    BOOLEAN Xslot_in_buffer;

    #define output_buffer               (current_p_docu->Xoutput_buffer)
    BOOLEAN Xoutput_buffer;

    #define buffer_altered              (current_p_docu->Xbuffer_altered)
    BOOLEAN Xbuffer_altered;

/* ---------------------------- doprint.c -------------------------------- */

    #define displaying_borders          (current_p_docu->Xdisplaying_borders)
    BOOLEAN Xdisplaying_borders;

    #define real_pagelength             (current_p_docu->Xreal_pagelength)
    S32    Xreal_pagelength;

    #define real_pagcnt                 (current_p_docu->Xreal_pagcnt)
    S32    Xreal_pagcnt;

    #define encpln                      (current_p_docu->Xencpln)
    S32    Xencpln;

    #define enclns                      (current_p_docu->Xenclns)
    S32    Xenclns;

    #define pagoff                      (current_p_docu->Xpagoff)
    S32    Xpagoff;

    #define pagnum                      (current_p_docu->Xpagnum)
    S32    Xpagnum;

    #define filpof                      (current_p_docu->Xfilpof)
    S32    Xfilpof;

    #define filpnm                      (current_p_docu->Xfilpnm)
    S32    Xfilpnm;

/* --------------------------- riscos.c ---------------------------------- */

    #define rear_window_handle          (current_p_docu->Xrear_window_handle)
    HOST_WND Xrear_window_handle;

    #define rear_window_template        (current_p_docu->Xrear_window_template)
    void * Xrear_window_template;       /* should be WimpWindowWithBitset * but ... */

    #define main_window_handle          (current_p_docu->Xmain_window_handle)
    HOST_WND Xmain_window_handle;

    #define main_window_template        (current_p_docu->Xmain_window_template)
    void * Xmain_window_template;       /* should be WimpWindowWithBitset * but ... */

    #define open_box                    (current_p_docu->Xopen_box)
    GDI_BOX Xopen_box;

    #define colh_window_handle          (current_p_docu->Xcolh_window_handle)
    HOST_WND Xcolh_window_handle;

    #define colh_window_template        (current_p_docu->Xcolh_window_template)
    void * Xcolh_window_template;       /* should be WimpWindowWithBitset * but ... */

    #define editexpression_formwind     (current_p_docu->Xeditexpression_formwind)
    struct FORMULA_WINDOW * Xeditexpression_formwind; /* should be FORMULA_WINDOW_HANDLE but ... */

    #define lastcursorpos_x             (current_p_docu->Xlastcursorpos_x)
    coord  Xlastcursorpos_x;

    #define lastcursorpos_y             (current_p_docu->Xlastcursorpos_y)
    coord  Xlastcursorpos_y;

    #define grid_on                     (current_p_docu->Xgrid_on)
    BOOLEAN Xgrid_on;

    #define charvspace                  (current_p_docu->Xcharvspace)
    S32    Xcharvspace;

    #define charvrubout_pos             (current_p_docu->Xcharvrubout_pos)
    S32    Xcharvrubout_pos;

    #define textcell_xorg               (current_p_docu->Xtextcell_xorg)
    S32    Xtextcell_xorg;

    #define textcell_yorg               (current_p_docu->Xtextcell_yorg)
    S32    Xtextcell_yorg;

    #define recalc_forced               (current_p_docu->Xrecalc_forced)
    S32    Xrecalc_forced;

    #define pict_on_screen              (current_p_docu->Xpict_on_screen)
    S32    Xpict_on_screen;

    #define pict_on_screen_prev         (current_p_docu->Xpict_on_screen_prev)
    S32    Xpict_on_screen_prev;

    #define slotcoordinates_size        16
    #define slotcoordinates             (current_p_docu->Xslotcoordinates)
    char   Xslotcoordinates[slotcoordinates_size];

    #define riscos_fonts                (current_p_docu->Xriscos_fonts)
    BOOL   Xriscos_fonts;

    #define riscos_font_error           (current_p_docu->Xriscos_font_error)
    S32    Xriscos_font_error;

    #define global_font                 (current_p_docu->Xglobal_font)
    char * Xglobal_font;

    #define global_font_x               (current_p_docu->Xglobal_font_x)
    S32    Xglobal_font_x;

    #define global_font_y               (current_p_docu->Xglobal_font_y)
    S32    Xglobal_font_y;

    #define global_font_leading_millipoints (current_p_docu->Xglobal_font_leading_millipoints)
    GR_MILLIPOINT Xglobal_font_leading_millipoints;

    #define hbar_length                 (current_p_docu->Xhbar_length)
    S32    Xhbar_length;

    #define pict_currow                 (current_p_docu->Xpict_currow)
    ROW    Xpict_currow;

    #define pict_on_currow              (current_p_docu->Xpict_on_currow)
    ROW    Xpict_on_currow;

    #define dspfld_from                 (current_p_docu->Xdspfld_from)
    S32    Xdspfld_from;

    #define quitseen                    (current_p_docu->Xquitseen)
    BOOL   Xquitseen;

    #define curr_extent_x               (current_p_docu->Xcurr_extent_x)
    S32    Xcurr_extent_x;

    #define curr_extent_y               (current_p_docu->Xcurr_extent_y)
    S32    Xcurr_extent_y;

    #define delta_scroll_x              (current_p_docu->Xdelta_scroll_x)
    S32    Xdelta_scroll_x;

    #define delta_scroll_y              (current_p_docu->Xdelta_scroll_y)
    S32    Xdelta_scroll_y;

    #define charvrubout_neg             (current_p_docu->Xcharvrubout_neg)
    S32    Xcharvrubout_neg;

    #define vdu5textoffset              (current_p_docu->Xvdu5textoffset)
    S32    Xvdu5textoffset;

    #define fontbaselineoffset          (current_p_docu->Xfontbaselineoffset)
    S32    Xfontbaselineoffset;

    #define fontscreenheightlimit_millipoints (current_p_docu->Xfontscreenheightlimit_millipoints)
    GR_MILLIPOINT Xfontscreenheightlimit_millipoints;

    #define charallocheight             (current_p_docu->Xcharallocheight)
    S32    Xcharallocheight;

    #define colh_mark_state_indicator   (current_p_docu->Xcolh_mark_state_indicator)
    BOOL   Xcolh_mark_state_indicator;

    #define auto_line_height            (current_p_docu->Xauto_line_height)
    BOOL   Xauto_line_height;

    #define formline_stashedcaret       (current_p_docu->Xformline_stashedcaret)
    S32    Xformline_stashedcaret;

#if defined(EXTENDED_COLOUR_WINDVARS)

/* ----------------------------------------------------------------------- */

    #define d_colours_CF                (current_p_docu->Xd_colours_CF)
    U32 Xd_colours_CF;

    #define d_colours_CB                (current_p_docu->Xd_colours_CB)
    U32 Xd_colours_CB;

    #define d_colours_CP                (current_p_docu->Xd_colours_CP)
    U32 Xd_colours_CP;

    #define d_colours_CL                (current_p_docu->Xd_colours_CL)
    U32 Xd_colours_CL;

    #define d_colours_CG                (current_p_docu->Xd_colours_CG)
    U32 Xd_colours_CG;

    #define d_colours_CC                (current_p_docu->Xd_colours_CC)
    U32 Xd_colours_CC;

    #define d_colours_CE                (current_p_docu->Xd_colours_CE)
    U32 Xd_colours_CE;

    #define d_colours_CH                (current_p_docu->Xd_colours_CH)
    U32 Xd_colours_CH;

    #define d_colours_CS                (current_p_docu->Xd_colours_CS)
    U32 Xd_colours_CS;

    #define d_colours_CU                (current_p_docu->Xd_colours_CU)
    U32 Xd_colours_CU;

    #define d_colours_CT                (current_p_docu->Xd_colours_CT)
    U32 Xd_colours_CT;

    #define d_colours_CV                (current_p_docu->Xd_colours_CV)
    U32 Xd_colours_CV;

    #define d_colours_CX                (current_p_docu->Xd_colours_CX)
    U32 Xd_colours_CX;

#endif /* EXTENDED_COLOUR_WINDVARS */

/* ----------------------------------------------------------------------- */

    #define d_options_DE                (current_p_docu->Xd_options_DE)
    uchar *Xd_options_DE;

    #define d_options_TN                (current_p_docu->Xd_options_TN)
    uchar  Xd_options_TN;

    #define d_options_IW                (current_p_docu->Xd_options_IW)
    uchar  Xd_options_IW;

    #define d_options_BO                (current_p_docu->Xd_options_BO)
    uchar  Xd_options_BO;

    #define d_options_JU                (current_p_docu->Xd_options_JU)
    uchar  Xd_options_JU;

    #define d_options_WR                (current_p_docu->Xd_options_WR)
    uchar  Xd_options_WR;

    #define d_options_DP                (current_p_docu->Xd_options_DP)
    uchar  Xd_options_DP;

    #define d_options_MB                (current_p_docu->Xd_options_MB)
    uchar  Xd_options_MB;

    #define d_options_TH                (current_p_docu->Xd_options_TH)
    uchar  Xd_options_TH;

    #define d_options_IR                (current_p_docu->Xd_options_IR)
    uchar  Xd_options_IR;

    #define d_options_DF                (current_p_docu->Xd_options_DF)
    uchar  Xd_options_DF;

    #define d_options_LP                (current_p_docu->Xd_options_LP)
    uchar *Xd_options_LP;

    #define d_options_TP                (current_p_docu->Xd_options_TP)
    uchar *Xd_options_TP;

    #define d_options_TA                (current_p_docu->Xd_options_TA)
    uchar *Xd_options_TA;

    #define d_options_GR                (current_p_docu->Xd_options_GR)
    uchar  Xd_options_GR;

    #define d_options_KE                (current_p_docu->Xd_options_KE)
    uchar  Xd_options_KE;

/* ----------------------------------------------------------------------- */

    #define d_poptions_PL               (current_p_docu->Xd_poptions_PL)
    uchar  Xd_poptions_PL;

    #define d_poptions_LS               (current_p_docu->Xd_poptions_LS)
    uchar  Xd_poptions_LS;

    #define d_poptions_PS               (current_p_docu->Xd_poptions_PS)
    uchar *Xd_poptions_PS;

    #define d_poptions_TM               (current_p_docu->Xd_poptions_TM)
    uchar  Xd_poptions_TM;

    #define d_poptions_HM               (current_p_docu->Xd_poptions_HM)
    uchar  Xd_poptions_HM;

    #define d_poptions_FM               (current_p_docu->Xd_poptions_FM)
    uchar  Xd_poptions_FM;

    #define d_poptions_BM               (current_p_docu->Xd_poptions_BM)
    uchar  Xd_poptions_BM;

    #define d_poptions_LM               (current_p_docu->Xd_poptions_LM)
    uchar  Xd_poptions_LM;

    #define d_poptions_HE               (current_p_docu->Xd_poptions_HE)
    uchar *Xd_poptions_HE;

    #define d_poptions_FO               (current_p_docu->Xd_poptions_FO)
    uchar *Xd_poptions_FO;

    #define d_poptions_PX               (current_p_docu->Xd_poptions_PX)
    uchar  Xd_poptions_PX;

    #define d_poptions_H1               (current_p_docu->Xd_poptions_H1)
    uchar *Xd_poptions_H1;

    #define d_poptions_H2               (current_p_docu->Xd_poptions_H2)
    uchar *Xd_poptions_H2;

    #define d_poptions_F1               (current_p_docu->Xd_poptions_F1)
    uchar *Xd_poptions_F1;

    #define d_poptions_F2               (current_p_docu->Xd_poptions_F2)
    uchar *Xd_poptions_F2;

/* ----------------------------------------------------------------------- */

    #define d_recalc_AM                 (current_p_docu->Xd_recalc_AM)
    uchar  Xd_recalc_AM;

    #define d_recalc_RC                 (current_p_docu->Xd_recalc_RC)
    uchar  Xd_recalc_RC;

    #define d_recalc_RI                 (current_p_docu->Xd_recalc_RI)
    uchar  Xd_recalc_RI;

    #define d_recalc_RN                 (current_p_docu->Xd_recalc_RN)
    uchar *Xd_recalc_RN;

    #define d_recalc_RB                 (current_p_docu->Xd_recalc_RB)
    uchar *Xd_recalc_RB;

/* ----------------------------------------------------------------------- */

    #define d_chart_options_CA          (current_p_docu->Xd_chart_options_CA)
    uchar  Xd_chart_options_CA;

/* ----------------------------------------------------------------------- */

    #define d_driver_PT                 (current_p_docu->Xd_driver_PT)  /* type */
    uchar  Xd_driver_PT;

    #define d_driver_PD                 (current_p_docu->Xd_driver_PD)  /* name */
    uchar *Xd_driver_PD;

    #define d_driver_PN                 (current_p_docu->Xd_driver_PN)  /* econ */
    uchar *Xd_driver_PN;

    #define d_driver_PB                 (current_p_docu->Xd_driver_PB)  /* baud*/
    uchar  Xd_driver_PB;

    #define d_driver_PW                 (current_p_docu->Xd_driver_PW)  /* data */
    uchar  Xd_driver_PW;

    #define d_driver_PP                 (current_p_docu->Xd_driver_PP)  /* prty */
    uchar  Xd_driver_PP;

    #define d_driver_PO                 (current_p_docu->Xd_driver_PO)  /* stop */
    uchar  Xd_driver_PO;

/* ----------------------------------------------------------------------- */

    #define d_print_QP                  (current_p_docu->Xd_print_QP)  /* p,s,f on/off */
    uchar  Xd_print_QP;

    #define d_print_QF                  (current_p_docu->Xd_print_QF)  /* p,s,f filename */
    uchar *Xd_print_QF;

    #define d_print_QD                  (current_p_docu->Xd_print_QD)  /* database on/off */
    uchar  Xd_print_QD;

    #define d_print_QE                  (current_p_docu->Xd_print_QE)  /* database name */
    uchar *Xd_print_QE;

    #define d_print_QO                  (current_p_docu->Xd_print_QO)  /* omit */
    uchar  Xd_print_QO;

    #define d_print_QB                  (current_p_docu->Xd_print_QB)  /* block */
    uchar  Xd_print_QB;

    #define d_print_QT                  (current_p_docu->Xd_print_QT)  /* two-sided on/off */
    uchar  Xd_print_QT;

    #define d_print_QM                  (current_p_docu->Xd_print_QM)  /* two-sided margin size */
    uchar *Xd_print_QM;

    #define d_print_QN                  (current_p_docu->Xd_print_QN)  /* number of copies */
    short  Xd_print_QN;

    #define d_print_QW                  (current_p_docu->Xd_print_QW)  /* wait between */
    uchar  Xd_print_QW;

    #define d_print_QL                  (current_p_docu->Xd_print_QL)  /* landscape */
    uchar  Xd_print_QL;

    #define d_print_QS                  (current_p_docu->Xd_print_QS)  /* scaling */
    short  Xd_print_QS;

/* ----------------------------------------------------------------------- */

    #define d_mspace_MY                 (current_p_docu->Xd_mspace_MY)  /* microspace? */
    uchar  Xd_mspace_MY;

    #define d_mspace_MZ                 (current_p_docu->Xd_mspace_MZ)  /* the other bit */
    uchar *Xd_mspace_MZ;

/* ----------------------------------------------------------------------- */

    #define tr_block                    (current_p_docu->Xtr_block)
    TRAVERSE_BLOCK Xtr_block;

#define BUF_WINDOW_TITLE_LEN 256
    TCHARZ  Xwindow_title[BUF_WINDOW_TITLE_LEN];

    void * the_last_thing;

/* ----------------------------------------------------------------------- */

    SS_INSTANCE_DATA ss_instance_data; /* SKS 20130406 stick this in here to associate more closely with DOCU */
}
DOCU, * P_DOCU;

/* special values of P_DOCU types */

#if CROSS_COMPILE && defined(CODE_ANALYSIS)
#define NO_DOCUMENT ((P_DOCU) NULL) /* Hmm... the below value freaks out VS Code Analysis */
#else
#define NO_DOCUMENT BAD_POINTER_X(P_DOCU, 3) /* trap if accessed - unaligned and very large (but still ARM rotated small constant) */
#endif

/*
exported variables for macros
*/

extern P_DOCU docu_array[256]; /* NB > DOCNO_MAX */
extern DOCNO  docu_array_size;

#define docno_from_p_docu(p_docu) ( \
    (p_docu)->docno )

#define docu_is_thunk(p_docu) ( \
    (p_docu)->ss_instance_data.ss_doc.is_docu_thunk )

/*
exported functions
*/

_Check_return_
extern DOCNO
change_document_using_docno(
    _InVal_     DOCNO new_docno);

extern BOOL
convert_docu_thunk_to_document(
    _InVal_     DOCNO docno);

extern DOCNO
create_new_docu_thunk(
    _InRef_     PC_DOCU_NAME p_docu_name);

extern BOOL
create_new_document(
    _InRef_     PC_DOCU_NAME p_docu_name);

extern BOOL
create_new_untitled_document(void);

extern void
destroy_docu_thunk(
    _InVal_     DOCNO docno);

extern void
destroy_current_document(void);

extern void
docu_array_init_once(void);

extern void
docu_init_once(void);

extern S32
documents_modified(void);

_Check_return_
extern DOCNO
find_document_using_leafname(
    _In_z_      PCTSTR wholename);

_Check_return_
extern DOCNO
find_document_using_wholename(
    _In_z_      PCTSTR wholename);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
find_document_using_window_handle(
    _HwndRef_   HOST_WND window_handle);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
find_document_with_input_focus(void);

extern void
front_document_using_docno(
    _InVal_     DOCNO docno);

_Check_return_
_Ret_notnull_
extern PCTSTR
get_untitled_document(void);

_Check_return_
_Ret_notnull_
extern PCTSTR
get_untitled_document_with(
    _In_z_      PCTSTR leafname);

extern BOOL
mergebuf_all(void);

extern BOOL
select_document_using_callback_handle(
    void * handle);

/*ncr*/
extern BOOL
select_document_using_docno(
    _InVal_     DOCNO docno);

#if defined(TRACE_DOC)

_Check_return_
extern DOCNO
current_docno(void);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
current_document(void);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT but never a docu thunk */
first_document(void);

_Check_return_
extern BOOL
is_current_document(void);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT but never a docu thunk */
next_document(
    _InRef_     P_DOCU p_docu_cur);

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT or a docu thunk */
p_docu_from_docno(
    _InVal_     DOCNO docno);

extern void
select_document(
    _InRef_     P_DOCU p_docu);

extern void
select_document_none(void);

#else /* NOT TRACE_DOC */
/*
functions as macros
*/

#define current_docno() ( \
    is_current_document() ? current_document()->docno : DOCNO_NONE )

#define current_document() ( \
    current_p_docu )

#define first_document() ( /*P_DOCU*/ /* may be NO_DOCUMENT but never a docu thunk */ \
    document_list )

#define is_current_document() ( \
    NO_DOCUMENT != current_document() )

#define next_document(p_docu) ( /*P_DOCU*/ /* may be NO_DOCUMENT but never a docu thunk */ \
    (p_docu)->link )

#define p_docu_from_docno(docno) ( /*P_DOCU*/ /* may be NO_DOCUMENT or a docu thunk */ \
    docu_array[docno] )

#define select_document(p_docu) \
    current_p_docu_global_register_assign(p_docu)

#define select_document_none() \
    current_p_docu_global_register_assign(NO_DOCUMENT)

#endif /* TRACE_DOC */

#endif  /* __windvars_h */

/* end of windvars.h */
