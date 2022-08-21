/* progvars.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Global variables used in PipeDream */

/* RJM 8-Apr-1989 */

#ifndef __progvars_h
#define __progvars_h

#define charheight normal_charheight
#ifdef SKS_NEW_CHAR_WIDTH
/* SKS 20.10.91 - make one char width become 1/12 of an inch in fonty world */
#define charwidth  (riscos_fonts ? 15 /*180/12*/ : normal_charwidth /*16*/)
#else
#define charwidth  normal_charwidth
#endif

#ifndef RISC_OS_3_1
#define RISC_OS_3_1 0xa4
#define RISC_OS_3_5 0xa5
#endif

/* ------------------------------ windvars.c ------------------------------- */

extern S32 nDocuments;

#if !defined(SB_GLOBAL_REGISTER)
extern struct _DOCU * current_p_docu;
#endif

extern P_DOCU document_list;

/* ------------------------------ pdmain.c --------------------------------- */

extern int ctrlflag;

/* ------------------------------ savload.c -------------------------------- */

extern BOOL in_load;

/* --------------------------- mcdiff.c ------------------------------------ */

extern S32 currently_inverted;
extern uchar highlights_on;

/* ----------------------------- commlin.c --------------------------------- */

#define NO_COMM ((S32) 0)          /* lukucm() is reporting error */
#define MAX_COMM_SIZE 4             /* maximum command length */

#define IS_SUBSET   -1
#define IS_ERROR    -2

#define M_IS_SUBSET ((void *) 1)
#define M_IS_ERROR  ((void *) 2)

extern uchar        alt_array[];
extern uchar       *buff_sofar;
extern uchar        cbuff[];                /* command line buffer */
extern S32          cbuff_offset;           /* length of string in cbuff */
extern BOOL         command_expansion;
extern uchar       *exec_ptr;
extern uchar        expanded_key_buff[];    /* for key expansions, always start with \ */
extern P_LIST_BLOCK first_command_redef;
extern MENU_HEAD    headline[];
extern S32          head_size;
extern BOOL         in_execfile;
extern BOOL         macro_recorder_on;
extern coord        this_heading_length;

/* ----------------------------- execs.c ----------------------------------- */

extern BOOL dont_save;

extern P_LIST_BLOCK protected_blocks;
extern P_LIST_BLOCK linked_columns;

/* ----------------------------- pdsearch.c -------------------------------- */

#define NOCASEINFO -1
#define FIRSTUPPER 1
#define SECONDUPPER 2

extern DOCNO sch_docno;
extern SLR sch_stt;             /* start of search block */
extern SLR sch_end;             /* end of search block */
extern SLR sch_pos_stt;         /* position of start of match */
extern SLR sch_pos_end;         /* position of end of match */
extern S32 sch_stt_offset;      /* offset of search string in slot */
extern S32 sch_end_offset;      /* offset of end of matched string */
extern BOOL dont_update_lescrl;

extern /* static */ uchar *next_replace_word;

#define WILD_FIELDS 9
extern /* static */ uchar wild_query[];
extern /* static */ S32 wild_queries;

extern /* static */ S32 wild_strings;

extern /* static */ P_LIST_BLOCK wild_string_list;

/* ---------------------------- pdriver.c ---------------------------------- */

extern BOOL driver_loaded;  /* is there a driver loaded? */
extern BOOL hmi_as_text;
extern BOOL send_linefeeds;

extern P_LIST_BLOCK highlight_list;

#define DEFAULT_LF FALSE

/* ------------------------------- slector.c ------------------------------- */

/* ------------------------------- browse.c -------------------------------- */

extern char *word_to_invert;            /* invert a word in check screen display */

#ifndef __spell_h
#include "cmodules/spell.h"
#endif

extern uchar word_to_insert[MAX_WORD];

extern P_LIST_BLOCK first_spell;
extern P_LIST_BLOCK first_user_dict;

extern /* static */ S32 most_recent;   /* no. of most recently used used dictionary */

extern S32 master_dictionary;

/* ------------------------------ help.c ----------------------------------- */

/* ------------------------------ viewio.c --------------------------------- */

/* ------------------------------- dialog.c -------------------------------- */

extern uchar *keyidx;           /* next character in soft key definition */

/* ---------------------------- bufedit.c -------------------------------- */

/* ---------------------------- reperr.c --------------------------------- */

extern S32 been_error; /* reperr sets this for others to notice */

/* ---------------------------- scdraw.c --------------------------------- */

extern BOOL microspacing;

extern uchar smispc;            /* standard microspacing */
extern BOOL xf_flush;
extern BOOL xf_leftright;

#define xf_iovbit d_progvars[OR_IO].option

/* --------------------------- cursmov.c --------------------------------- */

extern SAVPOS saved_pos[];
extern S32 saved_index;

/* --------------------------- numbers.c --------------------------------- */

extern BOOL activity_indicator;
extern P_LIST_BLOCK draw_file_list;
extern NLISTS_BLK draw_file_refs;

/* --------------------------- doprint.c --------------------------------- */

extern BOOL prnbit;         /* printer currently on */
extern BOOL sqobit;         /* sequential (non-screen) output */
extern BOOL micbit;         /* microspacing on */
extern BOOL spobit;         /* spool */

extern uchar off_at_cr;     /* 8 bits for turning highlight off at CR */
extern uchar h_waiting;     /* 8 bits determining which highlights are waiting */
extern uchar h_query;       /* 8 bits specifying whether the highlight is a ? field */

extern P_LIST_BLOCK first_macro;      /* list of macro parameters */

/* --------------------------- lists.c ----------------------------------- */

extern P_LIST_BLOCK def_first_option;   /* default options read from pd.ini */
extern P_LIST_BLOCK deleted_words;      /* list of deleted strings */
extern P_LIST_BLOCK first_key;          /* first key expansion in chain */

/* --------------------------- slot.c ------------------------------------ */

extern P_COLENTRY def_colstart;         /* default column structure from pd.ini */
extern COL def_numcol;

/* ----------------------------- riscos.c -------------------------------- */

extern HOST_HWND caret_window;

extern DOCNO slotcount_docno;

extern DOCNO blk_docno;
extern SLR blkanchor;                   /* block anchor */
extern SLR blkstart;                    /* block start */
extern SLR blkend;                      /* block end */

/* ----------------------------------------------------------------------- */

extern DOCNO browsing_docno;
extern DOCNO pausing_docno;

/* on screen only! - printer conversion fixed */
#define ch_to_os(c) ((c) * charwidth)
#define os_to_mp(o) ((o) * x_scale)
#define ch_to_mp(c) os_to_mp(ch_to_os(c))
extern S32 x_scale;
extern S32 y_scale;

extern S32 screen_x_os;
extern S32 screen_y_os;

extern GDI_BOX graphics_window;
extern GDI_BOX cliparea;
extern GDI_BOX thisarea;
extern BOOL paint_is_update;

extern S32 riscos_font_xad; /* mp coordinates of left hand baseline point */
extern S32 riscos_font_yad;
extern BOOL riscos_printing;
extern BOOL draw_to_screen;

extern P_LIST_BLOCK graphics_link_list;

extern BOOL currently_protected;
extern BOOL out_alteredstate;
extern BOOL exec_filled_dialog;

extern S32 insert_reference_row_number;
extern S32 insert_reference_stt_offset;
extern S32 insert_reference_end_offset;
extern BOOL insert_reference_abs_col;
extern BOOL insert_reference_abs_row;
extern DOCNO insert_reference_docno;
extern SLR insert_reference_slr;

/* enumeration of dict defn files */
extern P_LIST_BLOCK language_list;

/* temporary enumeration of templates or printer drivers */
extern P_LIST_BLOCK ltemplate_or_driver_list;

extern S32 paper_width_mp;
extern S32 paper_length_mp;

#endif /* __progvars_h */

/* end of progvars.h */
