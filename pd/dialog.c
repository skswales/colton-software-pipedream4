/* dialog.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Dialog box handling */

/* RJM Aug 1987; SKS Mar 1989 */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"
#include "pd_x.h"
#include "ridialog.h"

/*
internal functions
*/

static BOOL
init___dialog_box(
    _InVal_     U32 boxnumber,
    _InVal_     BOOL f_windvars,
    _InVal_     BOOL f_allocatestrings);

/* Stuart claims these should not be translated */
static const char *YESNOSTR       =     "YN";
static const char *NOYESSTR       =     "NY";

static const char *ZEROSTR        =     "0";
static const char *ONESTR         =     "1";
static const char *TWOSTR         =     "2";

static const char *EIGHTSTR       =     "8";
static const char *TWELVESTR      =     "12";
static const char *SIXTYSIXSTR    =     "66";
static const char *ONEHUNDREDSTR  =     "100";
static const char *EIGHT4567STR   =     "84567";
static const char *PSFSTR         =     "PSF";
static const char *PLSTR          =     "PL";
static const char *NEOSTR         =     "NEO";
static const char *TNSTR          =     "TN";
static const char *RCSTR          =     "RC";
static const char *MBSTR          =     "MB";
static const char *EATSTR         =     "TEA";  /* possibly EATSTR should be renamed */
static const char *ONE2345678STR  =     "12345678";
static const char *AMSTR          =     "AM";
static const char *ANSTR          =     "AN";
static const char *ACRSTR         =     "ACR";
static const char *DecimalPlaces_Parm_STR = "23456789F01";

static const char * TWELVE_PTx16_STR   = "192"; /*12pt*16*/
#ifdef SKS_NEW_LINESPACE
static const char * LINESPACE_MP_STR   = "0"; /*12pt*1000*1.2 was "12000" */
#else
static const char * LINESPACE_MP_STR   = "14400"; /*12pt*1000*1.2 was "12000" */
#endif

/******************************************************************************
* colours
******************************************************************************/

/*     The crap colours Rob wanted       The tasteful set */

#define TEXTFORESTR         "7"
#define TEXTBACKSTR         "0"
#define BORDERFORESTR       "8"
#define BORDERBACKSTR       "1"
#define CURBORDERFORESTR    "0"          /* "11" */
#define CURBORDERBACKSTR    "8"          /* "12" */
#define FIXBORDERBACKSTR    "3"          /* "8"  */

#define HARDPAGEBREAKSTR    "3"
#define SOFTPAGEBREAKSTR    "2"
#define NEGATIVESTR         "11"
#define PROTECTSTR          "14"
#define CARETSTR            "11"
#define GRIDSTR             "2"          /* "15" */

static const char *textforestr         =  TEXTFORESTR;
static const char *textbackstr         =  TEXTBACKSTR;
static const char *borderforestr       =  BORDERFORESTR;
static const char *borderbackstr       =  BORDERBACKSTR;
static const char *curborderforestr    =  CURBORDERFORESTR;
static const char *curborderbackstr    =  CURBORDERBACKSTR;
static const char *fixborderbackstr    =  FIXBORDERBACKSTR;

static const char *hardpagebreakstr    =  HARDPAGEBREAKSTR;
static const char *softpagebreakstr    =  SOFTPAGEBREAKSTR;
static const char *negativestr         =  NEGATIVESTR;
static const char *protectstr          =  PROTECTSTR;
static const char *caretstr            =  CARETSTR;
static const char *gridstr             =  GRIDSTR;

#if D_PASTE_SIZE == 10
static const char *pastesizestr   = "10";
#else
#error paste depth needs changing here
#endif

/******************************************************************************
*
*                                dialog boxes
*
******************************************************************************/

#define TEXTWIDTH  20

#define NO_OPT     0
#define NO_LIST    NULL

static const PC_U8Z *
line_sep_list[] =
/* note that the positions of these strings is given by
 *    LINE_SEP_LF
 *    LINE_SEP_CR
 *    LINE_SEP_CRLF
 *    LINE_SEP_LFCR
*/
{
    &lf_str,
    &cr_str,
    &crlf_str,
    &lfcr_str,
    NULL
};

static const PC_U8Z *
thousands_list[] =
{
    &thousands_none_STR,
    &thousands_comma_STR,
    &thousands_dot_STR,
    &thousands_space_STR,
    NULL
};

static const PC_U8Z *
port_list[] =
{
    &printertype_RISC_OS_STR,
    &printertype_Parallel_STR,
    &printertype_Serial_STR,
    &printertype_Network_STR,
    &printertype_User_STR,
    NULL
};

static const char *baud_75_str    = "75";
static const char *baud_150_str   = "150";
static const char *baud_300_str   = "300";
static const char *baud_1200_str  = "1200";
static const char *baud_2400_str  = "2400";
static const char *baud_4800_str  = "4800";
static const char *baud_9600_str  = "9600";
static const char *baud_19200_str = "19200";

static const PC_U8Z *
baud_list[] =
{
    &baud_9600_str,
    &baud_75_str,
    &baud_150_str,
    &baud_300_str,
    &baud_1200_str,
    &baud_2400_str,
    &baud_4800_str,
    &baud_9600_str,
    &baud_19200_str,
    NULL
};

static const PC_U8Z *
func_list[] =
{
    &F1_STR,
    &F2_STR,
    &F3_STR,
    &F4_STR,
    &F5_STR,
    &F6_STR,
    &F7_STR,
    &F8_STR,
    &F9_STR,
    &F10_STR,
    &F11_STR,
    &F12_STR,

    &Shift_F1_STR,
    &Shift_F2_STR,
    &Shift_F3_STR,
    &Shift_F4_STR,
    &Shift_F5_STR,
    &Shift_F6_STR,
    &Shift_F7_STR,
    &Shift_F8_STR,
    &Shift_F9_STR,
    &Shift_F10_STR,
    &Shift_F11_STR,
    &Shift_F12_STR,

    &Ctrl_F1_STR,
    &Ctrl_F2_STR,
    &Ctrl_F3_STR,
    &Ctrl_F4_STR,
    &Ctrl_F5_STR,
    &Ctrl_F6_STR,
    &Ctrl_F7_STR,
    &Ctrl_F8_STR,
    &Ctrl_F9_STR,
    &Ctrl_F10_STR,
    &Ctrl_F11_STR,
    &Ctrl_F12_STR,

    &Ctrl_Shift_F1_STR,
    &Ctrl_Shift_F2_STR,
    &Ctrl_Shift_F3_STR,
    &Ctrl_Shift_F4_STR,
    &Ctrl_Shift_F5_STR,
    &Ctrl_Shift_F6_STR,
    &Ctrl_Shift_F7_STR,
    &Ctrl_Shift_F8_STR,
    &Ctrl_Shift_F9_STR,
    &Ctrl_Shift_F10_STR,
    &Ctrl_Shift_F11_STR,
    &Ctrl_Shift_F12_STR,

    &F_Print_STR,
    &SF_Print_STR,
    &CF_Print_STR,
    &CSF_Print_STR,

    &F_Tab_STR,
    &SF_Tab_STR,
    &CF_Tab_STR,
    &CSF_Tab_STR,

    &F_Backspace_STR,
    &SF_Backspace_STR,
    /*&CF_Backspace_STR,*/
    /*&CSF_Backspace_STR,*/

    &F_Insert_STR,
    &SF_Insert_STR,
    &CF_Insert_STR,
    &CSF_Insert_STR,

    &F_Delete_STR,
    &SF_Delete_STR,
    &CF_Delete_STR,
    &CSF_Delete_STR,

    &F_Home_STR,
    &SF_Home_STR,
    &CF_Home_STR,
    &CSF_Home_STR,

    &F_End_STR,
    &SF_End_STR,
    &CF_End_STR,
    &CSF_End_STR,

    NULL
};

/* exported list of internal key values corresponding to the above list of key names */

/*extern*/ const KMAP_CODE
func_key_list[] =
{
      KMAP_BASE_FUNC +  1,
      KMAP_BASE_FUNC +  2,
      KMAP_BASE_FUNC +  3,
      KMAP_BASE_FUNC +  4,
      KMAP_BASE_FUNC +  5,
      KMAP_BASE_FUNC +  6,
      KMAP_BASE_FUNC +  7,
      KMAP_BASE_FUNC +  8,
      KMAP_BASE_FUNC +  9,
      KMAP_BASE_FUNC + 10,
      KMAP_BASE_FUNC + 11,
      KMAP_BASE_FUNC + 12,

     KMAP_BASE_SFUNC +  1,
     KMAP_BASE_SFUNC +  2,
     KMAP_BASE_SFUNC +  3,
     KMAP_BASE_SFUNC +  4,
     KMAP_BASE_SFUNC +  5,
     KMAP_BASE_SFUNC +  6,
     KMAP_BASE_SFUNC +  7,
     KMAP_BASE_SFUNC +  8,
     KMAP_BASE_SFUNC +  9,
     KMAP_BASE_SFUNC + 10,
     KMAP_BASE_SFUNC + 11,
     KMAP_BASE_SFUNC + 12,

     KMAP_BASE_CFUNC +  1,
     KMAP_BASE_CFUNC +  2,
     KMAP_BASE_CFUNC +  3,
     KMAP_BASE_CFUNC +  4,
     KMAP_BASE_CFUNC +  5,
     KMAP_BASE_CFUNC +  6,
     KMAP_BASE_CFUNC +  7,
     KMAP_BASE_CFUNC +  8,
     KMAP_BASE_CFUNC +  9,
     KMAP_BASE_CFUNC + 10,
     KMAP_BASE_CFUNC + 11,
     KMAP_BASE_CFUNC + 12,

    KMAP_BASE_CSFUNC +  1,
    KMAP_BASE_CSFUNC +  2,
    KMAP_BASE_CSFUNC +  3,
    KMAP_BASE_CSFUNC +  4,
    KMAP_BASE_CSFUNC +  5,
    KMAP_BASE_CSFUNC +  6,
    KMAP_BASE_CSFUNC +  7,
    KMAP_BASE_CSFUNC +  8,
    KMAP_BASE_CSFUNC +  9,
    KMAP_BASE_CSFUNC + 10,
    KMAP_BASE_CSFUNC + 11,
    KMAP_BASE_CSFUNC + 12,

    KMAP_FUNC_PRINT,
    KMAP_FUNC_PRINT                        | KMAP_CODE_ADDED_SHIFT,
    KMAP_FUNC_PRINT | KMAP_CODE_ADDED_CTRL,
    KMAP_FUNC_PRINT | KMAP_CODE_ADDED_CTRL | KMAP_CODE_ADDED_SHIFT,

    KMAP_FUNC_TAB,
    KMAP_FUNC_STAB,
    KMAP_FUNC_CTAB,
    KMAP_FUNC_CSTAB,

    KMAP_FUNC_BACKSPACE,
    KMAP_FUNC_SBACKSPACE,
    /*KMAP_FUNC_CBACKSPACE,*/
    /*KMAP_FUNC_CSBACKSPACE,*/

    KMAP_FUNC_INSERT,
    KMAP_FUNC_INSERT                        | KMAP_CODE_ADDED_SHIFT,
    KMAP_FUNC_INSERT | KMAP_CODE_ADDED_CTRL,
    KMAP_FUNC_INSERT | KMAP_CODE_ADDED_CTRL | KMAP_CODE_ADDED_SHIFT,

    KMAP_FUNC_DELETE,
    KMAP_FUNC_SDELETE,
    KMAP_FUNC_CDELETE,
    KMAP_FUNC_CSDELETE,

    KMAP_FUNC_HOME,
    KMAP_FUNC_SHOME,
    KMAP_FUNC_CHOME,
    KMAP_FUNC_CSHOME,

    KMAP_FUNC_END,
    KMAP_FUNC_SEND,
    KMAP_FUNC_CEND,
    KMAP_FUNC_CSEND,
};

/* load/save dialog text/options */

static const char *
LOAD_TYPES_STR = AUTO_CHAR_STR PD_CHAR_STR TAB_CHAR_STR CSV_CHAR_STR FWP_CHAR_STR VIEWSHEET_CHAR_STR VIEW_CHAR_STR PARAGRAPH_CHAR_STR;

static const char *
SAVE_TYPES_STR =               PD_CHAR_STR TAB_CHAR_STR CSV_CHAR_STR FWP_CHAR_STR                    VIEW_CHAR_STR PARAGRAPH_CHAR_STR;

/* dialog boxes - arrays of dialog box entries */

/* get the offset of member var in the DOCU structure */
#define DV(var) offsetof(DOCU, var)

/* var field can be one of three things:
 * NULL             variable only stored in dialog box
 * DV(variable)     variable stored in a DOCU member
*/

#define DN(f, o1, o2, opt, ol, var)   {  f, o1, o2, opt, &ol, NULL, var }

#define DNNL(f, o1, o2, opt, var)     {  f, o1, o2, opt, NO_LIST, NULL, var }

#define DNAY(f, o1, o2, opt, ol, var) {  f, o1, o2, opt, (PC_U8 *) &ol, NULL, var }

DIALOG d_load[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, LOAD_TYPES_STR,      0 )
};

DIALOG d_overwrite[] =
{
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_save_template[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_save[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DNAY(F_ARRAY,     'C','R',   NO_OPT, line_sep_list,       0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, SAVE_TYPES_STR,      0 )
};

DIALOG d_print[] =
{
    DN(  F_SPECIAL,   'Q','P',   NO_OPT, PSFSTR,              DV(Xd_print_QP) ),
    DNNL(F_TEXT,      'Q','F',   NO_OPT,                      DV(Xd_print_QF) ),
    DN(  F_SPECIAL,   'Q','D',   NO_OPT, NOYESSTR,            DV(Xd_print_QD) ),
    DNNL(F_TEXT,      'Q','E',   NO_OPT,                      DV(Xd_print_QE) ),
    DN(  F_SPECIAL,   'Q','O',   NO_OPT, NOYESSTR,            DV(Xd_print_QO) ),
    DN(  F_SPECIAL,   'Q','B',   NO_OPT, NOYESSTR,            DV(Xd_print_QB) ),
    DN(  F_SPECIAL,   'Q','T',   NO_OPT, NOYESSTR,            DV(Xd_print_QT) ),
    DNNL(F_TEXT,      'Q','M',   NO_OPT,                      DV(Xd_print_QM) ),
    DN(  F_NUMBER,    'Q','N',   NO_OPT, ONESTR,              DV(Xd_print_QN) ),
    DN(  F_SPECIAL,   'Q','W',   NO_OPT, NOYESSTR,            DV(Xd_print_QW) ),
    DN(  F_SPECIAL,   'Q','L',   NO_OPT, PLSTR,               DV(Xd_print_QL) ),
    DN(  F_NUMBER,    'Q','S',   NO_OPT, ONEHUNDREDSTR,       DV(Xd_print_QS) )
};

DIALOG d_mspace[] =
{
    DN(  F_SPECIAL,   'M','Y',   NO_OPT, NOYESSTR,            DV(Xd_mspace_MY) ),
    DN(  F_NUMBER,    'M','Z',   NO_OPT, TWELVESTR,           DV(Xd_mspace_MZ) )
};

DIALOG d_driver[] =
{
    DNAY(F_ARRAY,     'P','T',   NO_OPT, port_list,           DV(Xd_driver_PT) ),
    DNNL(F_TEXT,      'P','D',   NO_OPT,                      DV(Xd_driver_PD) ),
    DN(  F_TEXT,      'P','N',   NO_OPT, DEFAULTPSERVER_STR,  DV(Xd_driver_PN) ),
    DNAY(F_ARRAY,     'P','B',   NO_OPT, baud_list,           DV(Xd_driver_PB) ),
    DN(  F_SPECIAL,   'P','W',   NO_OPT, EIGHT4567STR,        DV(Xd_driver_PW) ),
    DN(  F_SPECIAL,   'P','P',   NO_OPT, NEOSTR,              DV(Xd_driver_PP) ),
    DN(  F_SPECIAL,   'P','O',   NO_OPT, TWELVESTR,           DV(Xd_driver_PO) )
};

DIALOG d_poptions[] =
{
    DN(  F_NUMBER,    'P','L',   NO_OPT, SIXTYSIXSTR,         DV(Xd_poptions_PL) ),
    DN(  F_NUMBER,    'L','S',   NO_OPT, ONESTR,              DV(Xd_poptions_LS) ),
    DNNL(F_TEXT,      'P','S',   NO_OPT,                      DV(Xd_poptions_PS) ),
    DN(  F_NUMBER,    'T','M',   NO_OPT, ZEROSTR,             DV(Xd_poptions_TM) ),
    DN(  F_NUMBER,    'H','M',   NO_OPT, TWOSTR,              DV(Xd_poptions_HM) ),
    DN(  F_NUMBER,    'F','M',   NO_OPT, TWOSTR,              DV(Xd_poptions_FM) ),
    DN(  F_NUMBER,    'B','M',   NO_OPT, EIGHTSTR,            DV(Xd_poptions_BM) ),
    DN(  F_NUMBER,    'L','M',   NO_OPT, ZEROSTR,             DV(Xd_poptions_LM) ),
    DNNL(F_TEXT,      'H','E',   NO_OPT,                      DV(Xd_poptions_HE) ),
    DNNL(F_TEXT,      'H','1',   NO_OPT,                      DV(Xd_poptions_H1) ),
    DNNL(F_TEXT,      'H','2',   NO_OPT,                      DV(Xd_poptions_H2) ),
    DNNL(F_TEXT,      'F','O',   NO_OPT,                      DV(Xd_poptions_FO) ),
    DNNL(F_TEXT,      'F','1',   NO_OPT,                      DV(Xd_poptions_F1) ),
    DNNL(F_TEXT,      'F','2',   NO_OPT,                      DV(Xd_poptions_F2) ),
    DN(  F_NUMBER,    'P','X',   NO_OPT, ZEROSTR,             DV(Xd_poptions_PX) )
};

DIALOG d_sort[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),

    /* RJM switched out update refs and switches off multi-row records on 14.9.91 */
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_replicate[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

/* order of d_search has to correspond with manifests in pdsearch.c */

DIALOG d_search[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, YESNOSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_options[] =
{
    DNNL(F_TEXT,      'D','E',   NO_OPT,                      DV(Xd_options_DE) ),
    DN(  F_SPECIAL,   'T','N',   NO_OPT, TNSTR,               DV(Xd_options_TN) ),
    DN(  F_SPECIAL,   'I','W',   NO_OPT, RCSTR,               DV(Xd_options_IW) ),
    DN(  F_SPECIAL,   'B','O',   NO_OPT, YESNOSTR,            DV(Xd_options_BO) ),
    DN(  F_SPECIAL,   'J','U',   NO_OPT, NOYESSTR,            DV(Xd_options_JU) ),
    DN(  F_SPECIAL,   'W','R',   NO_OPT, YESNOSTR,            DV(Xd_options_WR) ),
    DN(  F_SPECIAL,   'D','P',   NO_OPT, DecimalPlaces_Parm_STR, DV(Xd_options_DP) ),
    DN(  F_SPECIAL,   'M','B',   NO_OPT, MBSTR,               DV(Xd_options_MB) ),
    DNAY(F_ARRAY,     'T','H',   NO_OPT, thousands_list,      DV(Xd_options_TH) ),
    DN(  F_SPECIAL,   'I','R',   NO_OPT, NOYESSTR,            DV(Xd_options_IR) ),
    DN(  F_SPECIAL,   'D','F',   NO_OPT, EATSTR,              DV(Xd_options_DF) ),
    DN(  F_TEXT,      'L','P',   NO_OPT, DEFAULTLEADING_STR,  DV(Xd_options_LP) ),
    DN(  F_TEXT,      'T','P',   NO_OPT, DEFAULTTRAILING_STR, DV(Xd_options_TP) ),
    DN(  F_TEXT,      'T','A',   NO_OPT, DEFAULTTEXTAT_STR,   DV(Xd_options_TA) ),
    DN(  F_SPECIAL,   'G','R',   NO_OPT, NOYESSTR,            DV(Xd_options_GR) ),
    DN(  F_SPECIAL,   'K','E',   NO_OPT, YESNOSTR,            DV(Xd_options_KE) )
};

DIALOG d_parameter[] =
{
    DNNL(F_LIST,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_width[] =
{
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ZEROSTR,             0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_name[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_execfile[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_goto[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_decimal[] =
{
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, DecimalPlaces_Parm_STR, 0 )
};

DIALOG d_insremhigh[] =
{
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, ONE2345678STR,       0 )
};

DIALOG d_defkey[] =
{
    DNNL(F_LIST,      '\0','\0', 128,                         0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_def_fkey[] =
{
    DNAY(F_ARRAY,     '\0','\0', NO_OPT, func_list,           0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_insert_page_break[] =
{
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ZEROSTR,             0 )
};

DIALOG d_create[] =
{
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ONESTR,              0 ),
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ONESTR,              0 ),
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ONESTR,              0 )
};

DIALOG d_colours[] =
{
#if defined(EXTENDED_COLOUR_WINDVARS)
    DN(  F_COLOUR,    'C','F',   NO_OPT, textforestr,         DV(Xd_colours_CF) ),
    DN(  F_COLOUR,    'C','B',   NO_OPT, textbackstr,         DV(Xd_colours_CB) ),
    DN(  F_COLOUR,    'C','P',   NO_OPT, protectstr,          DV(Xd_colours_CP) ),
    DN(  F_COLOUR,    'C','L',   NO_OPT, negativestr,         DV(Xd_colours_CL) ),
    DN(  F_COLOUR,    'C','G',   NO_OPT, gridstr,             DV(Xd_colours_CG) ),
    DN(  F_COLOUR,    'C','C',   NO_OPT, caretstr,            DV(Xd_colours_CC) ),
    DN(  F_COLOUR,    'C','E',   NO_OPT, softpagebreakstr,    DV(Xd_colours_CE) ),
    DN(  F_COLOUR,    'C','H',   NO_OPT, hardpagebreakstr,    DV(Xd_colours_CH) ),
    DN(  F_COLOUR,    'C','S',   NO_OPT, borderforestr,       DV(Xd_colours_CS) ),
    DN(  F_COLOUR,    'C','U',   NO_OPT, borderbackstr,       DV(Xd_colours_CU) ),
    DN(  F_COLOUR,    'C','T',   NO_OPT, curborderforestr,    DV(Xd_colours_CT) ),
    DN(  F_COLOUR,    'C','V',   NO_OPT, curborderbackstr,    DV(Xd_colours_CV) ),
    DN(  F_COLOUR,    'C','X',   NO_OPT, fixborderbackstr,    DV(Xd_colours_CX) )
#else
    DN(  F_COLOUR,    'C','F',   NO_OPT, textforestr,         0 ),
    DN(  F_COLOUR,    'C','B',   NO_OPT, textbackstr,         0 ),
    DN(  F_COLOUR,    'C','P',   NO_OPT, protectstr,          0 ),
    DN(  F_COLOUR,    'C','L',   NO_OPT, negativestr,         0 ),
    DN(  F_COLOUR,    'C','G',   NO_OPT, gridstr,             0 ),
    DN(  F_COLOUR,    'C','C',   NO_OPT, caretstr,            0 ),
    DN(  F_COLOUR,    'C','E',   NO_OPT, softpagebreakstr,    0 ),
    DN(  F_COLOUR,    'C','H',   NO_OPT, hardpagebreakstr,    0 ),
    DN(  F_COLOUR,    'C','S',   NO_OPT, borderforestr,       0 ),
    DN(  F_COLOUR,    'C','U',   NO_OPT, borderbackstr,       0 ),
    DN(  F_COLOUR,    'C','T',   NO_OPT, curborderforestr,    0 ),
    DN(  F_COLOUR,    'C','V',   NO_OPT, curborderbackstr,    0 ),
    DN(  F_COLOUR,    'C','X',   NO_OPT, fixborderbackstr,    0 )
#endif /* EXTENDED_COLOUR_WINDVARS */
};

#define DEF_INS_CHAR 0xC0

DIALOG d_inschar[] =
{
    DNNL(F_CHAR,      '\0','\0', DEF_INS_CHAR,                0 )
};

DIALOG d_about[] =
{
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 ),
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 ),
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 )
};

DIALOG d_count[] =
{
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 )
};

DIALOG d_auto[] =
{
    DN(  F_SPECIAL,   'S','A',   NO_OPT, NOYESSTR,            0 )
};

DIALOG d_checkon[] =
{
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_check[] =
{
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_SPECIAL,   '\0','\0', NO_OPT, NOYESSTR,            0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_check_mess[] =
{
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 ),
    DNNL(F_ERRORTYPE, '\0','\0', NO_OPT,                      0 )
};

DIALOG d_user_create[] =
{
    DN(  F_TEXT,      '\0','\0', NO_OPT, USERDICT_STR,        0 ),
    DNNL(F_TEXT,      'S','L',   NO_OPT,                      0 )
};

DIALOG d_user_open[] =
{
    DN(  F_TEXT,      'S','O',   NO_OPT, USERDICT_STR,        0 )
};

DIALOG d_user_close[] =
{
    DN(  F_TEXT,      '\0','\0', NO_OPT, USERDICT_STR,        0 )
};

DIALOG d_user_flush[] =
{
    DN(  F_TEXT,      '\0','\0', NO_OPT, USERDICT_STR,        0 )
};

DIALOG d_user_delete[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_user_insert[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_user_browse[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_user_dump[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_user_merge[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_user_anag[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_user_lock[] =
{
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_user_unlock[] =
{
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_user_pack[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 ),
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_user_options[] =
{
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

DIALOG d_pause[] =
{
    DN(  F_NUMBER,    '\0','\0', NO_OPT, ZEROSTR,             0 )
};

DIALOG d_save_deleted[] =
{
    DN(  F_COMPOSITE, '\0','\0', NO_OPT, NOYESSTR,            0 )
};

/* This is a dummy dialog box, used only for loading and saving fix info.
 * Doesn't need graphic representation.
*/

DIALOG d_fixes[] =
{
    DNNL(F_TEXT,      'F','R',   NO_OPT,                      0 ),
    DNNL(F_TEXT,      'F','C',   NO_OPT,                      0 )
};

DIALOG d_deleted[] =
{
    DN(  F_NUMBER,    'D','N',   NO_OPT, pastesizestr,        0 )
};

DIALOG d_progvars[] =
{
    DN(  F_SPECIAL,   'A','N',  NO_OPT, ANSTR,                0 ), /* auto/manual recalc */
    DN(  F_NUMBER,    'I','O',  NO_OPT, ONESTR,               0 ),
    DN(  F_SPECIAL,   'A','C',  NO_OPT, AMSTR,                0 ), /* auto/manual recalc charts */
    DN(  F_NUMBER,    'C','D',  NO_OPT, ONESTR,               0 )  /* new/old chart format (1/0) */
};

/* This is a dummy dialog box, used only for loading and saving protect info.
 * Doesn't need graphic representation.
*/

DIALOG d_protect[] =
{
    DNNL(F_TEXT,      'P','Z',   NO_OPT,                      0 )
};

/* This is a dummy dialog box, used only for saving version info.
 * Doesn't need graphic representation.
*/

DIALOG d_version[] =
{
    DNNL(F_TEXT,      'V','S',   NO_OPT,                      0 )
};

/* This is a dummy dialog box, used only for loading and saving names
 * Doesn't need graphic representation.
*/

DIALOG d_names_dbox[] =
{
    DNNL(F_TEXT,      'N','D',   NO_OPT,                      0 )
};

DIALOG d_macro_file[] =
{
    DNNL(F_TEXT,      '\0','\0', NO_OPT,                      0 )
};

DIALOG d_fonts[] =
{
    DNNL(F_TEXT,      'F','G',   NO_OPT,                      0 ),
    DN(  F_TEXT,      'F','X',   NO_OPT, TWELVE_PTx16_STR,    0 ),
    DN(  F_TEXT,      'F','Y',   NO_OPT, TWELVE_PTx16_STR,    0 ),
    DN(  F_TEXT,      'F','S',   NO_OPT, LINESPACE_MP_STR,    0 )
};

/*                    'M','C',   Menu Change is specially handled */

DIALOG d_menu[] =
{
    DN(  F_NUMBER,    'M','S',   NO_OPT, ONESTR,              0 ),
    DN(  F_NUMBER,    'M','A',   NO_OPT, ZEROSTR,             0 )
};

DIALOG d_def_cmd[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 ),
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_define_name[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 ),
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_edit_name[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 ),
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 ),
    DN(  F_SPECIAL,   '\0','\0',  NO_OPT, NOYESSTR,           0 )
};

DIALOG d_insert_page_number[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_insert_date[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_insert_time[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

/* This is a nuther dummy dialog box, used only for saving and loading the window coords
 * Doesn't need graphic representation.
*/

DIALOG d_open_box[] =
{
    DNNL(F_TEXT,      'W','C',    NO_OPT,                     0 ),
};

/* This is a dummy dialog box, used only for loading and saving linked column info.
 * Doesn't need graphic representation.
*/

DIALOG d_linked_columns[] =
{
    DNNL(F_TEXT,      'L','C',    NO_OPT,                     0 )
};

DIALOG d_load_template[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_edit_driver[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 )
};

DIALOG d_formula_error[] =
{
    DNNL(F_TEXT,      '\0','\0',  NO_OPT,                     0 ),
    DN(  F_SPECIAL,   '\0','\0',  NO_OPT, NOYESSTR,           0 )
};

DIALOG d_chart_options[] =
{
    DN(  F_SPECIAL,   'C','A',    NO_OPT, ACRSTR, DV(Xd_chart_options_CA) ) /* auto, column, row */
};

/*
dialog headers
*/

#define dhead(dial, proc, name, flags) { dial, sizeof(dial) / sizeof(DIALOG), flags, proc, name }

#define dhead_none        dhead(d_pause, NULL, BAD_POINTER_X(char *, 0), 0)

#define dhead_dummy(dial) dhead(dial,    NULL, BAD_POINTER_X(char *, 0), 0)

#define EXEC_CAN_FILL     0x0001

DHEADER dialog_head[] =
{
    dhead_none,
    dhead(d_load,               dproc_loadfile,          "loadfile",     EXEC_CAN_FILL ),
    dhead(d_overwrite,          dproc_overwrite,         NULL,           0             ),    /* may pop up; uses dboxquery on RISCOS */
    dhead(d_save,               dproc_savefileas,        "savefileas",   EXEC_CAN_FILL ),
    dhead(d_save_template,      dproc_onetext,           "savetempl",    EXEC_CAN_FILL ),
    dhead(d_print,              dproc_print,             "print",        EXEC_CAN_FILL ),
    dhead(d_mspace,             dproc_microspace,        "microspace",   EXEC_CAN_FILL ),
    dhead(d_driver,             dproc_printconfig,       "printconfig",  EXEC_CAN_FILL ),
    dhead(d_poptions,           dproc_pagelayout,        "pagelayout",   EXEC_CAN_FILL ),
    dhead(d_sort,               dproc_sort,              "sort",         EXEC_CAN_FILL ),
    dhead(d_replicate,          dproc_twotext,           "replicate",    EXEC_CAN_FILL ),
    dhead(d_search,             dproc_search,            "search",       EXEC_CAN_FILL ),
    dhead(d_options,            dproc_options,           "options",      EXEC_CAN_FILL ),
    dhead(d_parameter,          dproc_numtext,           "setparm",      EXEC_CAN_FILL ),
    dhead(d_width,              dproc_numtext,           "columnwidth",  EXEC_CAN_FILL ),
    dhead(d_width,              dproc_numtext,           "rightmargin",  EXEC_CAN_FILL ),
    dhead(d_name,               dproc_onetext,           "namefile",     EXEC_CAN_FILL ),
    dhead(d_execfile,           dproc_execfile,          "cmdf_exec",    EXEC_CAN_FILL ),
    dhead(d_goto,               dproc_onetext,           "gotoslot",     EXEC_CAN_FILL ),
    dhead(d_decimal,            dproc_decimal,           "decimal",      EXEC_CAN_FILL ),
    dhead(d_insremhigh,         dproc_onespecial,        "inshigh",      EXEC_CAN_FILL ),
    dhead(d_insremhigh,         dproc_onespecial,        "remhigh",      EXEC_CAN_FILL ),
    dhead(d_defkey,             dproc_defkey,            "defkey",       EXEC_CAN_FILL ),
    dhead(d_def_fkey,           dproc_deffnkey,          "deffnkey",     EXEC_CAN_FILL ),
    dhead(d_insert_page_break,  dproc_onenumeric,        "pagebreak",    EXEC_CAN_FILL ),
    dhead(d_save,               dproc_savefileas,        "savefileas",   0             ),    /* always pop up */
    dhead(d_colours,            dproc_colours,           "colours",      EXEC_CAN_FILL ),
    dhead(d_colours,            dproc_extended_colours,  "extcolours",   EXEC_CAN_FILL ),    /* NB this refers to the same data as D_COLOURS */
    dhead(d_about,              dproc_aboutfile,         "aboutfile",    EXEC_CAN_FILL ),
    dhead_none,
    dhead(d_count,              dproc_onetext,           "count",        EXEC_CAN_FILL ),
    dhead(d_pause,              dproc_onenumeric,        "pause",        EXEC_CAN_FILL ),
    dhead(d_save_deleted,       dproc_save_deleted,      NULL,           0             ),    /* may pop up; uses dboxquery on RISCOS */

    dhead_dummy(d_auto),
    dhead(d_checkon,            dproc_checkdoc,          "checkdoc",     EXEC_CAN_FILL ),
    dhead(d_check,              dproc_checking,          "checking",     0             ),    /* may pop up */
    dhead(d_check_mess,         dproc_checked,           "checked",      0             ),    /* may pop up */
    dhead(d_user_create,        dproc_createdict,        "createuser",   EXEC_CAN_FILL ),
    dhead(d_user_open,          dproc_onetext,           "openuser",     EXEC_CAN_FILL ),
    dhead(d_user_close,         dproc_onetext,           "closeuser",    EXEC_CAN_FILL ),
    dhead(d_user_insert,        dproc_twotext,           "insword",      EXEC_CAN_FILL ),
    dhead(d_user_delete,        dproc_twotext,           "delword",      EXEC_CAN_FILL ),
    dhead(d_user_browse,        dproc_browse,            "browse",       EXEC_CAN_FILL ),
    dhead(d_user_dump,          dproc_dumpdict,          "dumpdict",     EXEC_CAN_FILL ),
    dhead(d_user_merge,         dproc_twotext,           "mergeuser",    EXEC_CAN_FILL ),
    dhead(d_user_anag,          dproc_anasubgram,        "anasubgram",   EXEC_CAN_FILL ),
    dhead(d_user_anag,          dproc_anasubgram,        "anasubgram",   EXEC_CAN_FILL ),
    dhead(d_user_lock,          dproc_onecomponoff,      "lockdict",     EXEC_CAN_FILL ),
    dhead(d_user_unlock,        dproc_onecomponoff,      "unlockdict",   EXEC_CAN_FILL ),
    dhead(d_user_pack,          dproc_twotext,           "packuser",     EXEC_CAN_FILL ),

    dhead_dummy(d_fixes),           /* dummy dialog box for saving fixed rows + cols */
    dhead(d_deleted,            dproc_onenumeric,        "pastedepth",   EXEC_CAN_FILL ),
    dhead_dummy(d_progvars),        /* dummy dialog box for progvars options */
    dhead_dummy(d_protect),         /* dummy dialog box for saving protection */
    dhead_dummy(d_version),         /* dummy dialog box for saving version */
    dhead_dummy(d_names_dbox),      /* dummy dialog box for saving names */
    dhead(d_macro_file,         dproc_onetext,           "cmdf_record",  EXEC_CAN_FILL ),
    dhead_dummy(d_fonts),
    dhead_dummy(d_menu),
    dhead(d_def_cmd,            dproc_twotext,           "defcmd",       EXEC_CAN_FILL ),

    dhead(d_user_options,       dproc_onecomponoff,      "dictoptions",  EXEC_CAN_FILL ),
    dhead(d_user_flush,         dproc_onetext,           "opnclsdict",   EXEC_CAN_FILL ),

    dhead(d_define_name,        dproc_twotext,           "definename",   EXEC_CAN_FILL ),
    dhead(d_edit_name,          dproc_edit_name,         "editname",     EXEC_CAN_FILL ),
    dhead_dummy(d_open_box),        /* dummy dialog box for saving window position */
    dhead_dummy(d_linked_columns),  /* dummy dialog box for saving linked cols */
    dhead(d_load_template,      dproc_loadtemplate,      "ltemplate",    EXEC_CAN_FILL ),
    dhead(d_edit_driver,        dproc_edit_driver,       "prndrvedit",   EXEC_CAN_FILL ),
    dhead(d_formula_error,      dproc_formula_error,     "formula_err",  0             ),    /* may pop up */
    dhead(d_chart_options,      dproc_chartopts,         "chartopts",    EXEC_CAN_FILL ),
    dhead(d_insert_page_number, dproc_insert_page_number,"inspagenum",   EXEC_CAN_FILL ),
    dhead(d_insert_date,        dproc_insert_date,       "insdatetime",  EXEC_CAN_FILL ),
    dhead(d_insert_time,        dproc_insert_time,       "insdatetime",  EXEC_CAN_FILL ),
    dhead(d_save,               dproc_savefilesimple,    "savefilesim",  EXEC_CAN_FILL ),
    dhead(d_save,               dproc_savefilesimple,    "savefilesim",  0             )     /* always pop up */
};
#define NDIALOG_BOXES elemof32(dialog_head)

/******************************************************************************
*
*  find an option and return a pointer to the dialog entry
*
******************************************************************************/

static DIALOG *
find_option(
    uchar ch1,
    uchar ch2,
    P_S32 dbox_num)
{
    DHEADER * dhptr, * last_dhptr;
    DIALOG * dptr, * last_dptr;

    dhptr = dialog_head;
    last_dhptr = dhptr + NDIALOG_BOXES;

    /* for each dialog box */
    do  {
        dptr = dhptr->dialog_box;
        last_dptr = dptr + dhptr->items;

        /* for each entry in each dialog box */
        do  {
            if((dptr->ch1 == ch1)  &&  (dptr->ch2 == ch2))
            {
                *dbox_num = dhptr - dialog_head;
                return(dptr);
            }
        }
        while(++dptr < last_dptr);
    }
    while(++dhptr < last_dhptr);

    *dbox_num = 0;
    return(NULL);
}

/******************************************************************************
*
* get option reads an option from the input and adds it to list
* looks for %OP%mnstring CR where mn=option mnemonic
*
******************************************************************************/

extern void
getoption(
    uchar *optr)
{
    DIALOG *dptr;
    uchar *from;
    S32 dbox_num;

    if(0 != memcmp(optr, "%OP%", (unsigned) 4))
        return;

    trace_1(TRACE_APP_DIALOG, "getoption(%s)", optr);

    optr += 4;

    if((*optr == 'M') && (*(optr+1) == 'C'))
    {
        get_menu_change(optr+2);
        return;
    }

    dptr = find_option(optr[0], optr[1], &dbox_num);
    if(!dptr)
    {
        trace_0(TRACE_APP_DIALOG, "no such option found");
        return;
    }

    update_dialog_from_windvars(dbox_num);

    from = optr + 2;

    switch(dptr->type)
    {
    case F_SPECIAL:
        dptr->option = *from;
        break;

#if defined(EXTENDED_COLOUR)
    case F_COLOUR:
        set_option_from_u32(&dptr->option, decode_wimp_colour_value(from));
        reportf("getoption(%s) got " U32_XTFMT, optr, get_option_as_u32(&dptr->option));
        break;
#endif

    case F_ARRAY:
#if !defined(EXTENDED_COLOUR)
    case F_COLOUR:
#endif
    case F_NUMBER:
    case F_LIST:
        dptr->option = (uchar) atoi((const char *) from); /* will be U8 in WV so truncate here for consistency */
        break;

    case F_TEXT:
        {
        uchar buffer[LIN_BUFSIZ];
        U32 to_idx = 0;

        /* user dictionaries have to be treated specially,
         * 'cos there may be more than one
        */
        if(dptr == d_user_open)
            (void) dict_number((char *) from, TRUE);

        if(dptr == d_protect)
            add_to_protect_list(from);
        else if(dptr == d_linked_columns)
            add_to_linked_columns_list(from);
        else if(dptr == d_names_dbox)
            add_to_names_list(from);

        /* copy string and encode highlights  */
        while(CH_NULL != *from)
        {
            if( (from[0] == '%') &&
                (from[1] == 'H') &&
                (from[3] == '%') &&
                ishighlighttext(from[2]) )
            {
                buffer[to_idx++] = (from[2] - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT;
                from += 4;
            }
            else
                buffer[to_idx++] = *from++;
        }

        buffer[to_idx] = CH_NULL;

        consume_bool(mystr_set(&dptr->textfield, buffer));
        break;
        }

    default:
        break;
    }

    update_windvars_from_dialog(dbox_num);
}

/******************************************************************************
*
* save the option pointed at by dptr to the default option list
*
******************************************************************************/

static void
save_opt_to_list(
    DIALOG *start,
    S32 n)
{
    DIALOG *dptr, *last_dptr;
    S32 key;
    char array[40];
    PC_U8 ptr, ptr1, ptr2;
    S32 res;

    dptr      = start;
    last_dptr = dptr + n;

    for(; dptr < last_dptr; dptr++)
    {
        key = (((S32) dptr->ch1) << 8) + (S32) dptr->ch2;

        /* RJM adds on 22.9.91 */
        if(key == 0)
            continue;

        ptr = array;

        switch(dptr->type)
        {
        case F_TEXT:
            ptr1 = dptr->textfield;
            if(str_isblank(ptr1))
                ptr1 = NULLSTR;

            if(dptr->optionlist)
            {
                ptr2 = *dptr->optionlist;
                if(str_isblank(ptr2))
                    ptr2 = NULLSTR;
            }
            else
                ptr2 = NULLSTR;

            if(0 == strcmp(ptr1, ptr2))
                continue;

            ptr = ptr1;
            break;

        case F_ARRAY:
            if(!dptr->option)
                continue;

            consume_int(sprintf(array, "%d", (int) dptr->option));
            break;

#if defined(EXTENDED_COLOUR)
        case F_COLOUR:
            {
            const U32 wimp_colour_value = get_option_as_u32(&dptr->option);

            if(wimp_colour_value == decode_wimp_colour_value(*dptr->optionlist))
                continue;

            consume_int(encode_wimp_colour_value(array, sizeof32(array), wimp_colour_value));
            reportf("save_opt_to_list(%c%c%s) from " U32_XTFMT ")", (int) dptr->ch1, (int) dptr->ch2, array, wimp_colour_value);

            break;
            }
#endif /* EXTENDED_COLOUR */

        case F_LIST:
        case F_NUMBER:
#if !defined(EXTENDED_COLOUR)
        case F_COLOUR:
#endif
            if(dptr->option == (optiontype) atoi(*dptr->optionlist))
                continue;

            consume_int(sprintf(array, "%d", (int) dptr->option));
            break;

        case F_SPECIAL:
            if(dptr->option == **dptr->optionlist)
                continue;

            array[0] = (char) dptr->option;
            array[1] = CH_NULL;
            break;

        default:
            break;
        }

        if(status_fail(res = add_to_list(&def_first_option, key, ptr)))
        {
            reperr_null(res);
            break;
        }
    }
}

/* write an S32 into a str_set string */

static BOOL
write_int_to_string(
    char **var,
    S32 number)
{
    char array[32];
    consume_int(sprintf(array, "%d", number));
    return(mystr_set(var, array));
}

/******************************************************************************
*
* save the current options on to the default list
* done after loading the Choices file
* these will be recovered on an implicit or explicit new
*
******************************************************************************/

extern void
save_options_to_list(void)
{
    update_all_dialog_from_windvars();

    update_dialog_from_fontinfo();

    save_opt_to_list(d_options,  dialog_head[D_OPTIONS].items);
    save_opt_to_list(d_poptions, dialog_head[D_POPTIONS].items);

    /* added by RJM on 22.9.91 */
    save_opt_to_list(d_driver, dialog_head[D_DRIVER].items);
    save_opt_to_list(d_print,  dialog_head[D_PRINT].items);

    /* added by RJM on 16.10.91 */
    save_opt_to_list(d_mspace, dialog_head[D_MSPACE].items);

#if defined(EXTENDED_COLOUR_WINDVARS)
    /* added by SKS on 14.09.16 */
    save_opt_to_list(d_colours, dialog_head[D_COLOURS].items);
#endif

    save_opt_to_list(d_save + SAV_LINESEP, 1);

    save_opt_to_list(d_fonts,    dialog_head[D_FONTS].items);

    save_opt_to_list(d_chart_options, dialog_head[D_CHART_OPTIONS].items);

#if 0
    save_opt_to_list(d_open_box, 1);
    /* SKS after PD 4.11 08jan92 - restore size position etc. from saved choices. removed again 'cos it's silly */
#endif

/* no update_fontinfo_from_dialog(); needed */

    update_all_windvars_from_dialog();
}

/******************************************************************************
*
* on a new recover the options from the Choices file which were
* put on the default option list
* assumes that all variables are currently in dialog boxes
*
******************************************************************************/

static void
recover_options_from_list(void)
{
    PC_LIST lptr;
    DIALOG *dptr;
    S32 key;
    uchar ch1, ch2;
    S32 dbox_num;

    for(lptr = first_in_list(&def_first_option);
        lptr;
        lptr = next_in_list(&def_first_option))
    {
        key = lptr->key;

        ch2 = (uchar) (key & 0xFF);
        key >>= 8;
        ch1 = (uchar) (key & 0xFF);

        dptr = find_option(ch1, ch2, &dbox_num);
        if(!dptr)
            continue;

        switch(dptr->type)
        {
        case F_SPECIAL:
            dptr->option = *(lptr->value);
            break;

#if defined(EXTENDED_COLOUR)
        case F_COLOUR:
            set_option_from_u32(&dptr->option, decode_wimp_colour_value((const char *) lptr->value));
            reportf("recover_options_from_list(%c%c%s) got " U32_XTFMT, ch1, ch2, (const char *) lptr->value, get_option_as_u32(&dptr->option));
            break;
#endif

        case F_ARRAY:
#if !defined(EXTENDED_COLOUR)
        case F_COLOUR:
#endif
        case F_NUMBER:
        case F_LIST:
/*???*/
            dptr->option = (uchar) atoi((const char *) lptr->value);
            break;

        case F_TEXT:
            consume_bool(mystr_set(&dptr->textfield, lptr->value));    /* keep trying */
            break;

        default:
            break;
        }
    }

#if 0
    /* update any variables that are not accessed thru dialog boxes */
    update_fontinfo_from_dialog();
#endif
}

extern void
update_dialog_from_fontinfo(void)
{
    if(!is_current_document())
        return;
    
    init_dialog_box(D_FONTS);

    if(riscos_fonts)
    {
        /* even saves defaults onto list */
        consume_bool(mystr_set(&d_fonts[D_FONTS_G].textfield, global_font));
#if 0 /* SKS after PD 4.11 19jan92 moved these from here ... */
        write_int_to_string(&d_fonts[D_FONTS_X].textfield, global_font_x);
        write_int_to_string(&d_fonts[D_FONTS_Y].textfield, global_font_y);
        write_int_to_string(&d_fonts[D_FONTS_S].textfield, auto_line_height ? 0 : global_font_leading_millipoints);
#endif
    }
    else
    {
        str_clr(&d_fonts[D_FONTS_G].textfield);
#if 0 /* SKS after PD 4.11 19jan92 removed these */
        str_clr(&d_fonts[D_FONTS_X].textfield);
        str_clr(&d_fonts[D_FONTS_Y].textfield);
        str_clr(&d_fonts[D_FONTS_S].textfield);
#endif
        }

#if 1 /* SKS after PD 4.11 19jan92 moved these ... to here */
    write_int_to_string(&d_fonts[D_FONTS_X].textfield, global_font_x);
    write_int_to_string(&d_fonts[D_FONTS_Y].textfield, global_font_y);
    write_int_to_string(&d_fonts[D_FONTS_S].textfield, auto_line_height ? 0 : global_font_leading_millipoints);
#endif
}

extern void
update_fontinfo_from_dialog(void)
{
    if(!is_current_document())
        return;

    str_clr(&global_font);

    if(d_fonts[D_FONTS_G].textfield)
    {
        str_swap(&global_font, &d_fonts[D_FONTS_G].textfield);
        /* try the new font out */
        riscos_fonts = TRUE;
        riscos_font_error = FALSE;
        ensure_global_font_valid();
    }
    else
        riscos_fonts = FALSE;

    if(d_fonts[D_FONTS_X].textfield)
    {
        consume_int(sscanf(d_fonts[D_FONTS_X].textfield, "%d", &global_font_x));
        str_clr(&d_fonts[D_FONTS_X].textfield);
    }

    if(d_fonts[D_FONTS_Y].textfield)
    {
        consume_int(sscanf(d_fonts[D_FONTS_Y].textfield, "%d", &global_font_y));
        str_clr(&d_fonts[D_FONTS_Y].textfield);
    }

    if(d_fonts[D_FONTS_S].textfield)
    {
        consume_int(sscanf(d_fonts[D_FONTS_S].textfield, "%d", &global_font_leading_millipoints));
        str_clr(&d_fonts[D_FONTS_S].textfield);
    }

    auto_line_height = (global_font_leading_millipoints == 0);
    if(auto_line_height)
        new_font_leading_based_on_y(); /* <<<<RJM,RCM */
    else
        new_font_leading(global_font_leading_millipoints); /*<<<SKS*/
}

extern void
update_dialog_from_windvars(
    _InVal_     U32 boxnumber)
{
    DHEADER * dhptr;
    DIALOG * init_dptr, *dptr, *last_dptr;

    if(!is_current_document())
    {
        trace_0(TRACE_APP_DIALOG, "unable to update_dialog_from_windvars as no current document");
        return;
    }

    assert(boxnumber < NDIALOG_BOXES);
    dhptr = dialog_head + boxnumber;

    dptr = dhptr->dialog_box;
    init_dptr = dptr;
    last_dptr = dptr + dhptr->items;

    trace_2(TRACE_APP_DIALOG, "update_dialog_from_windvars(%d) dialog box " PTR_XTFMT, boxnumber, report_ptr_cast(dhptr));

    do  {
        const U32 wv_offset = dptr->wv_offset;

        if(0 != wv_offset)
        {
            void * wv_ptr = PtrAddBytes(void *, current_document(), wv_offset);

            trace_2(TRACE_APP_DIALOG, "offset/ptr to windvars variable = &%x, " PTR_XTFMT", ", wv_offset, report_ptr_cast(wv_ptr));

            switch(dptr->type)
            {
            case F_ERRORTYPE:
            case F_TEXT:
                /* copy the windvars variable (STR) to dialog[n].textfield */
                /* still need primary copy for screen updates */
                consume_bool(mystr_set(&dptr->textfield, * (char **) wv_ptr));
                trace_2(TRACE_APP_DIALOG, "dialog[%d].textfield is now \"%s\"",
                        dptr - init_dptr,
                        trace_string(dptr->textfield));
                break;

#if defined(EXTENDED_COLOUR_WINDVARS)
            case F_COLOUR:
                /* copy the windvars variable (U32) to dialog[n].option */
                set_option_from_u32(&dptr->option, * (PC_U32) wv_ptr);
                trace_2(TRACE_APP_DIALOG, "dialog[%d].option is now " U32_XTFMT,
                        dptr - init_dptr,
                        get_option_as_u32(&dptr->option));
                break;
#endif /* EXTENDED_COLOUR_WINDVARS */

            case F_NUMBER:
            case F_ARRAY:
            default:
                /* copy the windvars variable (U8) to dialog[n].option */
                dptr->option = * (const uchar *) wv_ptr;
                trace_3(TRACE_APP_DIALOG, "dialog[%d].option is now %d, '%c'",
                        dptr - init_dptr,
                        dptr->option, dptr->option);
                break;
            }
        }
    }
    while(++dptr < last_dptr);
}

extern void
update_windvars_from_dialog(
    _InVal_     U32 boxnumber)
{
    DHEADER * dhptr;
    DIALOG * init_dptr, * dptr, * last_dptr;

    if(!is_current_document())
    {
        reportf(/*trace_0(TRACE_APP_DIALOG,*/ "unable to update_windvars_from_dialog as no current document");
        return;
    }

    assert(boxnumber < NDIALOG_BOXES);
    dhptr = dialog_head + boxnumber;

    dptr = dhptr->dialog_box;
    init_dptr = dptr;
    last_dptr = dptr + dhptr->items;

    trace_2(TRACE_APP_DIALOG, "update_windvars_from_dialog(%d) dialog box " PTR_XTFMT, boxnumber, report_ptr_cast(dhptr));

    do  {
        const U32 wv_offset = dptr->wv_offset;

        if(0 != wv_offset)
        {
            void * wv_ptr = PtrAddBytes(void *, current_document(), wv_offset);

            trace_4(TRACE_APP_DIALOG, "offset/ptr to windvars %c%c variable = &%x, " PTR_XTFMT ", ", dptr->ch1, dptr->ch2, wv_offset, wv_ptr);

            switch(dptr->type)
            {
            case F_ERRORTYPE:
            case F_TEXT:
                /* move the dialog[n].textfield to windvars variable (STR) */
                str_clr((char **) wv_ptr);
                * (char **) wv_ptr = dptr->textfield;
                dptr->textfield = NULL;
                trace_2(TRACE_APP_DIALOG, "windvar for dialog[%d].textfield is now \"%s\"",
                            dptr - init_dptr,
                            trace_string(* (char **) wv_ptr));
                break;

#if defined(EXTENDED_COLOUR_WINDVARS)
            case F_COLOUR:
                * (P_U32) wv_ptr = get_option_as_u32(&dptr->option);
                trace_2(TRACE_APP_DIALOG, "windvar for dialog[%d].option is now " U32_XTFMT,
                            dptr - init_dptr,
                            * (PC_U32)  wv_ptr);
                break;
#endif /* EXTENDED_COLOUR_WINDVARS */

            case F_NUMBER:
            case F_ARRAY:
            default:
                /* copy the dialog[n].option to windvars variable (U8) */
                * (uchar *) wv_ptr = (uchar) dptr->option;
                trace_3(TRACE_APP_DIALOG, "windvar for dialog[%d].option is now %d, '%c'",
                            dptr - init_dptr,
                            * (const uchar *)  wv_ptr, * (const uchar *) wv_ptr);
                break;
            }
        }
    }
    while(++dptr < last_dptr);
}

extern void
update_all_dialog_from_windvars(void)
{
    U32 boxnumber;

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        update_dialog_from_windvars(boxnumber);        /* move wvars -> dialog */
}

extern void
update_all_windvars_from_dialog(void)
{
    U32 boxnumber;

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        update_windvars_from_dialog(boxnumber);        /* move dialog -> wvars */
}

/******************************************************************************
*
* destroy all windvars fields in all dialog boxes
* done on destruction of each new document
*
******************************************************************************/

extern void
dialog_finalise(void)
{
    U32 boxnumber;

    trace_0(TRACE_APP_DIALOG, "dialog_finalise()");

    update_all_dialog_from_windvars();                /* move wvars -> dialog */

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        /* deallocate just windvars bits */
        init___dialog_box(boxnumber, TRUE, FALSE);
}

extern void
dialog_hardwired_defaults(void)
{
    U32 boxnumber;

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        /* allocate just windvars bits */
        init___dialog_box(boxnumber, TRUE /*(boxnumber != D_FONTS) <<SKS*/, TRUE);

        /* NB. fonts are windvars but not used via dialog vars
         * so destroy all data stored in the dialog before it gets 'restored'
        *  to the new document!
        */
}

/******************************************************************************
*
* initialise all windvars fields in all dialog boxes
* done on creation of each new document
*
******************************************************************************/

extern BOOL
dialog_initialise(void)
{
    BOOL res = TRUE;

    trace_0(TRACE_APP_DIALOG, "dialog_initialise()");

    dialog_hardwired_defaults();

    recover_options_from_list();                    /* make default dialogs */

    update_all_windvars_from_dialog();              /* move dialog -> wvars */

#if 1
    /* now update any variables that are not accessed thru dialog boxes */
    update_fontinfo_from_dialog();
#endif

    return(res);
}

/******************************************************************************
*
* initialise all except windvars fields in all dialog boxes
* done only once to set up statics template
*
******************************************************************************/

extern void
dialog_initialise_once(void)
{
    U32 boxnumber;

    trace_0(TRACE_APP_DIALOG, "dialog_initialise_once()");

    /* can't do as preprocessor check */
    assert((D_THE_LAST_ONE) != (NDIALOG_BOXES+1));

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        (void) init___dialog_box(boxnumber, FALSE, TRUE);    /* allocating non-wv strings */
}

/******************************************************************************
*
* clear all fields in box i
*
******************************************************************************/

extern BOOL
init_dialog_box(
    _InVal_     U32 boxnumber)
{
    BOOL res;

    res =        init___dialog_box(boxnumber, TRUE,  TRUE);    /* do all windvars fields */
    res = res && init___dialog_box(boxnumber, FALSE, TRUE);    /* do all other fields */

    return(res);
}

static BOOL
init___dialog_box(
    _InVal_     U32 boxnumber,
    _InVal_     BOOL f_windvars,
    _InVal_     BOOL f_allocatestrings)
{
    DHEADER * dhptr;
    DIALOG * dptr;
    DIALOG * last_dptr;
    BOOL res = TRUE;

    assert(boxnumber < NDIALOG_BOXES);
    dhptr = dialog_head + boxnumber;

    dptr = dhptr->dialog_box;
    last_dptr = dptr + dhptr->items;

    do  {
        const U32 wv_offset = dptr->wv_offset;

        if((0 != wv_offset) == f_windvars)    /* init one set or the other */
        if(!f_windvars || is_current_document())
        {
            /* for F_TEXT, F_COMPOSITE */
            str_clr(&dptr->textfield);

            if(dptr->optionlist != NO_LIST)
            {
                switch(dptr->type)
                {
                case F_TEXT:
                    if(f_allocatestrings)
                        res = res && mystr_set(&dptr->textfield, *dptr->optionlist);
                    break;

                case F_ARRAY:
                    dptr->option = 0;
                    break;

#if defined(EXTENDED_COLOUR)
                case F_COLOUR:
                    set_option_from_u32(&dptr->option, decode_wimp_colour_value(*dptr->optionlist));
                    break;
#endif

                case F_NUMBER:
#if !defined(EXTENDED_COLOUR)
                case F_COLOUR:
#endif
                    dptr->option = (uchar) atoi(*dptr->optionlist);
                    break;

                case F_LIST:
                    /* optionlist gives list_block * */
                    break;

            /*  case F_COMPOSITE:  */
            /*  case F_SPECIAL:    */
            /*  case F_CHAR:       */
                default:
                    dptr->option = **dptr->optionlist;
                    break;
                }
            }
        }
    }
    while(++dptr < last_dptr);

    return(res);
}

/******************************************************************************
*
*  read a single parameter
*
******************************************************************************/

static BOOL
read_parm(
    uchar *array)
{
    for(;;)
    {
        switch(*exec_ptr)
        {
        case CH_NULL:
            return(FALSE);

        case CR:
        case TAB:
            *array = CH_NULL;
            return(TRUE);

        default:
            *array++ = *exec_ptr++;
            break;
        }
    }
}

/******************************************************************************
*
* when execing a file read parameters from \ commands into dialog box
* e.g. \GS |I a1|m
* exec_ptr has the first parameter, leave exec_ptr pointing at |M
*
******************************************************************************/

static void
extract_parameters(
    DIALOG *dptr,
    S32 items)
{
    S32 count = 0;
    char array[LIN_BUFSIZ+1];
    uchar *ptr;
    S32 c;

    trace_0(TRACE_APP_DIALOG, "extract parameters in");

    /* exec_ptr points at TAB to start */
    for(; *exec_ptr != CR; count++, dptr++)
    {
        if(count >= items)
        {
            strcpy(array, (char *) exec_ptr);
            goto ERRORPOINT;
        }

        /* read the parameter */
        exec_ptr++;

        if(!read_parm(array))
            goto ERRORPOINT;

        trace_1(TRACE_APP_DIALOG, "read_parm read: %s", trace_string(array));

        switch(dptr->type)
        {
#if defined(EXTENDED_COLOUR)
        case F_COLOUR:
            set_option_from_u32(&dptr->option, decode_wimp_colour_value(array));
            reportf("extract_parameters(%s) got " U32_XTFMT, array, get_option_as_u32(&dptr->option));
            break;
#endif

#if !defined(EXTENDED_COLOUR)
        case F_COLOUR:
#endif
        case F_CHAR:
        case F_NUMBER:
            dptr->option = (uchar) atoi((const char *) array);
            break;

        case F_COMPOSITE:
        case F_SPECIAL:
            ptr = array;
            while(*ptr++ == SPACE)
                ;

            c = *--ptr;

            if(!c  ||  !strchr(*dptr->optionlist, c))
                goto ERRORPOINT;

            dptr->option = (uchar) c;
            if(dptr->type == F_SPECIAL)
                break;

            /* this is composite */
            if(*exec_ptr == CR)
                return;

            /* so get the textfield */
            exec_ptr++;
            read_parm(array);

            /* deliberate fall thru */

        case F_TEXT:
            if(!mystr_set(&dptr->textfield, array))
                return;
            break;

        case F_LIST:
            dptr->option = (uchar) atoi((char *) array);
            break;

        case F_ARRAY:
            {
            BOOL found = FALSE;
            char ***list = (char ***) dptr->optionlist;

            trace_1(TRACE_APP_DIALOG, "F_ARRAY searching for: %s", trace_string(array));

            /* MRJC fix added here 16.8.89 to stop
             * this loop zooming off end of array
            */
            for(c = 0; *(list + c); c++)
            {
                trace_1(TRACE_APP_DIALOG, "F_ARRAY comparing with: %s", trace_string(**(list + c)));
                if(0 == C_stricmp(array, **(list + c)))
                {
                    dptr->option = c;
                    found = TRUE;
                    break;
                }
            }

            if(!found)
            {
                trace_0(TRACE_APP_DIALOG, "F_ARRAY key not found");
                goto ERRORPOINT;
            }
            break;
            }

        default:
            break;
        }
    }

    trace_0(TRACE_APP_DIALOG, "extract parameters out");

    return;

ERRORPOINT:
    reperr(ERR_BAD_PARM, array);
}

/******************************************************************************
*
* draw a dialog box and let the user fiddle around with the fields
*
* but if in an exec file, fillin fields from file
*
* always ensures current buffer to memory and
* exits with the buffer refilled but unmodified
*
* --out--
*    TRUE if dialog box filled in ok
*
******************************************************************************/

extern BOOL
dialog_box(
    _InVal_     U32 boxnumber)
{
    DHEADER * dhptr;
    DIALOG * dptr;
    BOOL res;

    trace_1(TRACE_APP_DIALOG, "dialog_box(%d)", boxnumber);

    if(is_current_document())
    {
        /* RISCOS doesn't use linbuf to process dialog: mergebuf just to set filealtered before going into dialog */
        if(!mergebuf_nocheck())
            return(FALSE);
        filbuf();
    }

    update_dialog_from_windvars(boxnumber);

    assert(boxnumber < NDIALOG_BOXES);
    dhptr = dialog_head + boxnumber;

    dptr = dhptr->dialog_box;

    /* if execing a file don't display a dialog box,
     * just read the parameters until a CR is found
     * unless the dialog box demands to be
     * displayed and have user interaction.
    */
    if((dhptr->flags & EXEC_CAN_FILL) &&  (in_execfile  ||  command_expansion))
    {
        if(exec_ptr)
        {
            /* exec_ptr points at TAB parameter...
             * read the parameters and leave exec_ptr on |M
            */
            extract_parameters(dptr, dhptr->items);
            exec_filled_dialog = TRUE;
        }

        res = !been_error;
    }
    else
    {
        res = (dhptr->dproc)
                    ? riscdialog_execute(dhptr->dproc, dhptr->dname, dptr, boxnumber)
                    : reperr_not_installed(ERR_GENFAIL);

        if((dhptr->flags & EXEC_CAN_FILL)  &&  macro_recorder_on)
            out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items, res);
    }

    if(res)
        update_windvars_from_dialog(boxnumber);

    return(res);
}

/******************************************************************************
*
* can this dialog box persist?
*
* don't allow dialog box to persist if fed from command file / key expansion
* or it has already been closed
* of the macro recorder is turned on
*
******************************************************************************/

extern BOOL
dialog_box_can_persist(void)
{
    BOOL ended = exec_filled_dialog  ||  riscdialog_ended();
    trace_1(TRACE_APP_DIALOG, "dialog_box_ended() returns %s", report_boolstring(ended));

    /* SKS after PD 4.12 01apr92 - ensure people don't get persistent dialogs 'cos that screws up command file recording (which is too hard to fix) */
    if(!ended && macro_recorder_on)
    {
        dialog_box_end();
        ended = TRUE;
    }

    if(!ended && is_current_document())
    {
        draw_screen();
        draw_caret();
    }

    return(!ended);
}

/******************************************************************************
*
* don't allow dialog box retry if fed from command file / key expansion
* of the macro recorder is turned on
*
******************************************************************************/

extern BOOL
dialog_box_can_retry(void)
{
    if(exec_filled_dialog)
        return(FALSE);

    if(macro_recorder_on)
        return(FALSE);

    return(TRUE); /* can pop it up again */
}

/******************************************************************************
*
* explicitly kill off any persisting dialog box
*
******************************************************************************/

extern void
dialog_box_end(void)
{
    trace_0(TRACE_APP_DIALOG, "dialog_box_end() --- explicit termination");
    riscdialog_dispose();
}

/******************************************************************************
*
* prepare to start dialog box processing
*
******************************************************************************/

extern BOOL
dialog_box_can_start(void)
{
    if(riscdialog_in_dialog())
    {
        riscdialog_front_dialog();
        return(reperr_null(ERR_ALREADY_IN_DIALOG));
    }

    return(TRUE);
}

extern BOOL
dialog_box_start(void)
{
    /* ensure font selector goes bye-bye */
    pdfontselect_finalise(TRUE);

    false_return(dialog_box_can_start());

    exec_filled_dialog = FALSE;

    return(TRUE);
}

/*
Example of how to call dialog box functions:

static BOOL
examplecommand_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_EXAMPLECOMMAND); .*may not be needed*.

    if(error_in_setup)
        return(FALSE);

    return(dialog_box_start());
}

static int (or BOOL)
examplecommand_fn_core(void)
{
    if(some_error_condition_wanting_retry)
    {
        reperr();
        return(dialog_box_can_retry() ? 2 .*continue*. : FALSE);
    }

    some_processing_of_args;

    if(error_in_processing)
        return(FALSE);

    return(TRUE);
}

extern void
ExampleCommand_fn(void)
{
    if(!examplecommand_fn_prepare())
        return;

    while(dialog_box(D_EXAMPLECOMMAND))
    {
        int core_res = examplecommand_fn_prepare();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}
*/

/* end of dialog.c */
