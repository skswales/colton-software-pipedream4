/* progvars.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Global variables used in PipeDream */

/* RJM 8-Apr-1989 */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"

#define DONT_CARE 0

/* ensure we get enlarged root stack segment */

/* NB. menu_new() and menu_extend() use acres of stack i.e. about 11K so beware */
#ifndef ROOT_STACK_SIZE
#define ROOT_STACK_SIZE 32 /*16*/
#endif

extern int __root_stack_size; /* stop compiler moan */
       int __root_stack_size = ROOT_STACK_SIZE * 1024;

/* ----------------------------- windvars.c ------------------------------ */

S32 nDocuments = 0;

/* pointer to current document window statics */
#if defined(SB_GLOBAL_REGISTER)
/* use global register to store this much-accessed global */
/* we need a backup copy to access it in routines traversed to via library e.g. bsearch() */
P_DOCU current_p_docu_backup = NO_DOCUMENT;
#else
P_DOCU current_p_docu = NO_DOCUMENT;
#endif

P_DOCU document_list = NO_DOCUMENT;  /* list of all the documents */

/* ----------------------------- pdmain.c -------------------------------- */

int ctrlflag = 0;

/* ----------------------------- savload.c ------------------------------- */

BOOL in_load = FALSE;

/* ----------------------------- mcdiff.c -------------------------------- */

S32 currently_inverted;
uchar highlights_on = FALSE;

/* ----------------------------- commlin.c ------------------------------- */

uchar *buff_sofar = NULL;
uchar cbuff[CMD_BUFSIZ];            /* command line buffer */
S32 cbuff_offset = 0;              /* length of string in cbuff */
uchar cmd_seq_array[MAX_COMM_SIZE+4]; /* only ever one Cmd-sequence going */
BOOL command_expansion = FALSE;
uchar *exec_ptr = NULL;
uchar expanded_key_buff[8]  = { CMDLDI, CH_NULL }; /* for key expansions, always start with \ */
P_LIST_BLOCK first_command_redef = NULL;
BOOL in_execfile = FALSE;
BOOL macro_recorder_on = FALSE;

/* ----------------------------- execs.c --------------------------------- */

BOOL dont_save = FALSE;

P_LIST_BLOCK protected_blocks = NULL;
P_LIST_BLOCK linked_columns = NULL;

/* ----------------------------- pdsearch.c ------------------------------ */

DOCNO sch_docno = DOCNO_NONE;
SLR sch_stt;                    /* start of search block */
SLR sch_end;                    /* end of search block */
SLR sch_pos_stt;                /* position of start of match */
SLR sch_pos_end;                /* position of end of match */
S32 sch_stt_offset;             /* offset of search string in cell */
S32 sch_end_offset;             /* offset of end of matched string */
BOOL dont_update_lescrl = FALSE;

/* static */ uchar *next_replace_word = NULL;

/* static */ uchar wild_query[WILD_FIELDS];
/* static */ S32 wild_queries = 0;

/* static */ S32 wild_strings = 0;

/* static */ P_LIST_BLOCK wild_string_list = NULL;

/* ---------------------------- browse.c ----------------------------------- */

char *word_to_invert = NULL;        /* invert a given word in check screen display */

/* static */ uchar word_to_insert[MAX_WORD];

P_LIST_BLOCK first_spell = NULL;
P_LIST_BLOCK first_user_dict = NULL;

/* static */ S32 most_recent = -1; /* no. of most recently used used dictionary */

S32 master_dictionary = -1;

/* ---------------------------- dialog.c --------------------------------- */

uchar *keyidx = NULL;           /* next character in soft key definition */

/* ---------------------------- scdraw.c ----------------------------------- */

BOOL xf_leftright   = FALSE;
BOOL xf_flush       = FALSE;
BOOL microspacing   = FALSE;
uchar smispc = 1;               /* standard microspacing */

/* ---------------------------- cursmov.c ----------------------------------- */

SAVPOS saved_pos[SAVE_DEPTH];
S32 saved_index = 0;

/* ---------------------------- numbers.c ----------------------------------- */

BOOL activity_indicator = FALSE;
NLISTS_BLK draw_file_refs = { NULL, 0, 0 };

/* ---------------------------- doprint.c ----------------------------------- */

BOOL prnbit = FALSE;        /* printer on */
BOOL sqobit = FALSE;        /* sequential (non-screen) output */
BOOL micbit = FALSE;        /* microspacing on */
BOOL spobit = FALSE;        /* spool */

uchar off_at_cr = 0xFF;     /* 8 bits for turning highlight off at CR */
uchar h_waiting = 0;        /* 8 bits determining which highlights are waiting */
uchar h_query   = 0;        /* 8 bits specifying whether the highlight is a ? field */

P_LIST_BLOCK first_macro = NULL;      /* list of macro parameters */

/* ---------------------------- pdriver.c ----------------------------------- */

BOOL driver_loaded = FALSE;         /* no driver loaded initially */
BOOL hmi_as_text = FALSE;
BOOL send_linefeeds = DEFAULT_LF;

P_LIST_BLOCK highlight_list = NULL;

/* ---------------------------- reperr.c ------------------------------------ */

S32 been_error = 0; /* reperr sets this for others to notice */

/* ---------------------------- lists.c ------------------------------------- */

P_LIST_BLOCK def_first_option  = NULL;  /* default options read from Choices file */
P_LIST_BLOCK deleted_words     = NULL;  /* list of deleted words */
P_LIST_BLOCK first_key         = NULL;  /* list of key definitions */

/* ---------------------------- slot.c ------------------------------------- */

P_COLENTRY def_colstart = NULL;         /* default column structure from Choices file */
COL def_numcol = 0;

/* ---------------------------- riscos.c ----------------------------------- */

HOST_WND caret_window_handle = HOST_WND_NONE;
HOST_WND caret_stolen_from_window_handle = HOST_WND_NONE;

DOCNO slotcount_docno = DOCNO_NONE;

/* ------------------------------------------------------------------------- */

DOCNO blk_docno  = DONT_CARE;
SLR blkanchor           = { NO_COL, 0 };    /* block anchor */
SLR blkstart            = { NO_COL, 0 };    /* block start */
SLR blkend              = { NO_COL, 0 };    /* block end */

DOCNO browsing_docno = DOCNO_NONE;
DOCNO pausing_docno  = DOCNO_NONE;

S32 millipoints_per_os_x;              /* millipoints per OS unit */
S32 millipoints_per_os_y;              /* millipoints per OS unit */

GDI_SIZE g_os_display_size;            /* width/height of screen in OS units */

/* boxes are inclusive, inclusive, exclusive, exclusive */

/* current absolute graphics window coordinates set by Window Manager
 *
 * NB. this is in OS units
*/
GDI_BOX graphics_window;

/* current relative graphics window coordinates set by Window Manager
 *
 * NB. this is in text cells (y0 >= y1)
*/
GDI_BOX cliparea;     /* left, bottom, off right, off top */

/* currently intersected object coordinates to save recalculations
 * between the maybe_ and really_ phases of drawing an object.
 *
 * Important: can't be used between the draw_ and maybe_ phases as only
 * the Window Manager knows what is going on in the world nowadays.
*/
GDI_BOX thisarea;

/* whether screen paint is an update caused by us (background not cleared)
 * or a redraw caused by Window Manager (background already cleared to BACK)
 * still needed for RISC OS printing
*/
BOOL paint_is_update;

GR_MILLIPOINT riscos_font_ad_millipoints_x;
GR_MILLIPOINT riscos_font_ad_millipoints_y;
BOOL riscos_printing = FALSE;
BOOL draw_to_screen = TRUE;

P_LIST_BLOCK graphics_link_list = NULL;

BOOL currently_protected;
BOOL out_alteredstate = FALSE;          /* doesn't need to be in windvars */

BOOL exec_filled_dialog = FALSE;        /* was dialog box filled in by exec file? */

S32 insert_reference_row_number;
S32 insert_reference_stt_offset;
S32 insert_reference_end_offset;
BOOL insert_reference_abs_col;
BOOL insert_reference_abs_row;
DOCNO insert_reference_docno;
SLR insert_reference_slr;

GR_MILLIPOINT paper_width_millipoints  =  8 * (72 * 1000);
GR_MILLIPOINT paper_length_millipoints = 11 * (72 * 1000);

/* list of dict defns */
P_LIST_BLOCK languages_list = NULL;

/* temp list of macros */
P_LIST_BLOCK macros_list = NULL;

/* temp list of printer drivers */
P_LIST_BLOCK pdrivers_list = NULL;

/* temp list of templates */
P_LIST_BLOCK templates_list = NULL;

/* enumeration of page number formats */
P_LIST_BLOCK page_number_formats_list = NULL;

/* enumeration of date formats */
P_LIST_BLOCK date_formats_list = NULL;

/* enumeration of time formats */
P_LIST_BLOCK time_formats_list = NULL;

static NUMFORM_CONTEXT config_numform_context;

static PD_CONFIG pd_config =
{
    &config_numform_context
};

P_PD_CONFIG p_pd_config = &pd_config;

_Check_return_
_Ret_valid_
extern P_NUMFORM_CONTEXT
get_p_numform_context(void)
{
    return(p_pd_config->p_numform_context);
}

/* end of progvars.c */
