/* ev_evali.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Local header for evaluator */

/* MRJC January 1991 */

#ifndef __evali_h
#define __evali_h

#define EV_MAX_ARGS         25      /* maximum number of arguments */

/*-------------------------------------------------------------------------*/

/*
named resources
*/

typedef struct _ev_name
{
    EV_NAMEID key;              /* internal id allocated to name */
    char id[BUF_EV_INTNAMLEN];  /* name of resource */
    EV_SLR owner;               /* document that owns name definition */
    EV_DATA def_data;           /* data defined by name */
    EV_FLAGS flags;             /* flags about entry */
    EV_SERIAL visited;          /* last visited count */
}
EV_NAME, * P_EV_NAME;

/*
user defined custom functions
*/

typedef struct _ev_custom_args
{
    S32 n;                      /* number of arguments for custom */
    L1_U8Z id[EV_MAX_ARGS][BUF_EV_INTNAMLEN];
    EV_TYPE types[EV_MAX_ARGS];
}
EV_CUSTOM_ARGS, * P_EV_CUSTOM_ARGS;

typedef struct _ev_custom
{
    EV_NAMEID key;              /* internal id for custom function */
    char id[BUF_EV_INTNAMLEN];  /* custom function name */
    EV_SLR owner;               /* document/slr of custom function */
    EV_FLAGS flags;             /* flags about entry */
    EV_CUSTOM_ARGS args;        /* not pointer */
}
EV_CUSTOM, * P_EV_CUSTOM, ** P_P_EV_CUSTOM;

/*-------------------------------------------------------------------------*/

/*
types for sort routines
*/

typedef int (* sort_proctp) (
    const void * arg1,
    const void * arg2);

/*
types for exec routines
*/

typedef void (* exec_proctp) (
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 nargs,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_SLR p_cur_slr);

#define PROC_EXEC_PROTO(_p_proc_exec) \
extern void _p_proc_exec( \
    P_EV_DATA args[EV_MAX_ARGS], \
    _InVal_     S32 nargs, \
    _InoutRef_  P_EV_DATA p_ev_data_res, \
    _InRef_     PC_EV_SLR p_cur_slr)

#define exec_func_ignore_parms() \
    (void) (IGNOREPARM(args), IGNOREPARM_InVal_(nargs), IGNOREPARM_InoutRef_(p_ev_data_res), IGNOREPARM_InRef_(p_cur_slr))

/*-------------------------------------------------------------------------*/

/* symbol information */

typedef struct _SYM_INF
{
    EV_IDNO did_num;
    char sym_cr;
    char sym_space;
}
SYM_INF, * P_SYM_INF;

typedef struct fun_parmsD
{
    unsigned int ex_type : 3;   /* exec, database, control, lookup */
    unsigned int no_exec : 4;   /* parameter for functions with no exec routine */
    unsigned int control : 4;   /* custom function control statements */
    unsigned int datab   : 1;   /* database function */
    unsigned int var     : 1;   /* function makes rpn variable */
    unsigned int nodep   : 1;   /* function argument needs no dependency */
}
fun_parms;

/*
exec types
*/

enum exec_types
{
    EXEC_EXEC = 0,
    EXEC_DBASE,
    EXEC_CONTROL,
    EXEC_LOOKUP,
    EXEC_ALERT
};

/*
parameters for non_exec functions
*/

enum exec_control
{
    CONTROL_GOTO,
    CONTROL_RESULT,
    CONTROL_WHILE,
    CONTROL_ENDWHILE,
    CONTROL_REPEAT,
    CONTROL_UNTIL,
    CONTROL_FOR,
    CONTROL_NEXT,
    CONTROL_BREAK,
    CONTROL_CONTINUE,
    CONTROL_IF,
    CONTROL_ELSE,
    CONTROL_ELSEIF,
    CONTROL_ENDIF
};

enum exec_dbase
{
    DBASE_DAVG,
    DBASE_DCOUNT,
    DBASE_DCOUNTA,
    DBASE_DMAX,
    DBASE_DMIN,
    DBASE_DSTD,
    DBASE_DSTDP,
    DBASE_DSUM,
    DBASE_DVAR,
    DBASE_DVARP
};

enum exec_lookup
{
    LOOKUP_LOOKUP,
    LOOKUP_HLOOKUP,
    LOOKUP_VLOOKUP,
    LOOKUP_MATCH,
    LOOKUP_CHOOSE
};

#define LOOKUP_BREAK_COUNT 100

/*
table of rpn items
*/

typedef struct _rpndef
{
    EV_IDNO     rpn_type;
    signed char nargs;
    char        category;
    fun_parms   parms;
    exec_proctp exec;
    PC_EV_TYPE arg_types;  /* pointer to argument flags */
}
RPNDEF; typedef const RPNDEF * PC_RPNDEF;

/*
rpn engine data block
*/

typedef struct rpnstate
{
    EV_IDNO num;            /* current rpn token number */
    PC_U8 pos;              /* current position in rpn string */
}
* rpnstatep;

#define DECOMP_ERR "Decompiler error"

/*-------------------------------------------------------------------------*/

/*
compare SLRs
*/

#define slr_equal(slr1, slr2) ( \
    ((slr2)->docno == (slr1)->docno) && \
    ((slr2)->col == (slr1)->col) && \
    ((slr2)->row == (slr1)->row) )

/*
compare SLR to range
*/

#define ev_slr_in_range(range, slr) ( \
    ((slr)->docno == (range)->s.docno) && \
    ((slr)->col >= (range)->s.col) && \
    ((slr)->row >= (range)->s.row) && \
    ((slr)->col <  (range)->e.col) && \
    ((slr)->row <  (range)->e.row) )

/*
compare ranges
*/

#define ev_range_equal(range1, range2) ( \
    ((range2)->s.doc == (range1)->s.docno) && \
    ((range2)->s.col == (range1)->s.col) && \
    ((range2)->s.row == (range1)->s.row) && \
    ((range2)->e.col == (range1)->e.col) && \
    ((range2)->e.row == (range1)->e.row) )

/*
range a subset of another ?
*/

#define ev_range_in_range(range1, range2) ( \
    ((range2)->s.docno == (range1)->s.docno) && \
    ((range2)->s.col >= (range1)->s.col) && \
    ((range2)->s.row >= (range1)->s.row) && \
    ((range2)->e.col <= (range1)->e.col) && \
    ((range2)->e.row <= (range1)->e.row) )

/*
ranges overlap ?
*/

#define ev_range_overlap(range1, range2) ( \
    ((range2)->s.docno == (range1)->s.docno) && \
    ((range2)->s.col <  (range1)->e.col) && \
    ((range2)->s.row <  (range1)->e.row) && \
    ((range2)->e.col >  (range1)->s.col) && \
    ((range2)->e.row >  (range1)->s.row) )

/*-------------------------------------------------------------------------*/

/*
document flags
*/

#define DCF_CUSTOM      1   /* document is a custom function sheet */
#define DCF_TREEDAMAGED 2   /* tree of document is damaged */

/*
recalc todo list
*/

typedef struct _todo_entry
{
    EV_SLR slr;             /* slr to be done */
    EV_FLAGS flags;         /* flags about things */
}
TODO_ENTRY, * P_TODO_ENTRY;

/*
definition of the structure of
various dependency lists
*/

typedef struct extentry
{
    EV_RANGE  refto;        /* external reference to */
    U32       exthandle;    /* the outside world's bit */
    S32       inthandle;    /* internal handle */
    ev_urefct proc;         /* procedure to call */
    EV_FLAGS  flags;        /* flags about the entry */
}
* extentp;

typedef struct _range_use
{
    EV_RANGE  refto;        /* range points to */
    EV_SLR    byslr;        /* slot containing range */
    S16       byoffset;     /* offset of range in containing slot */
    EV_FLAGS  flags;        /* flags about this reference */
    EV_SERIAL visited;      /* group visited flag */
}
RANGE_USE, * P_RANGE_USE;

typedef struct _slr_use
{
    EV_SLR    refto;        /* reference points to */
    EV_SLR    byslr;        /* slot containing reference */
    S16       byoffset;     /* offset of reference/array in containing slot */
    EV_FLAGS  flags;        /* flags about this reference */
}
SLR_USE, * P_SLR_USE;

/*
structure of name use table
*/

typedef struct _name_use
{
    EV_NAMEID nameto;       /* reference to name.. */
    EV_SLR    byslr;        /* slot containing use of name */
    S16       byoffset;     /* offset of id in containing slot */
    EV_FLAGS  flags;        /* flags about this entry */
}
NAME_USE, * P_NAME_USE;

/*
structure of custom function use table
*/

typedef struct _custom_use
{
    EV_NAMEID custom_to;    /* reference to custom.. */
    EV_SLR    byslr;        /* slot containing reference to custom */
    EV_FLAGS  flags;        /* flags about this entry */
    S16       byoffset;     /* offset of id in containing slot */
}
CUSTOM_USE, * P_CUSTOM_USE;

/*
flags for tree tables
*/

#define TRF_LOCK        (EV_FLAGS)      1   /* locked against alteration below sorted limit */

#define TRF_TOBEDEL     (EV_FLAGS)      2   /* table: some to delete; entry: delete this */
#define TRF_BLOWN       (EV_FLAGS)      4   /* set after table is hacked */
#define TRF_DELHOLD     (EV_FLAGS)      32  /* deletions are held temporarily (names/custom functions) */

/*
table flags used in definition tables
*/

#define TRF_UNDEFINED   (EV_FLAGS)      8   /* custom function or name is undefined */
#define TRF_CHECKUSE    (EV_FLAGS)      16  /* uses of definition must be checked */

/*-------------------------------------------------------------------------*/

/*
array scanning data
*/

typedef struct _array_scan_block
{
    P_EV_DATA p_ev_data; /* contains an array */
    S32 x_pos;
    S32 y_pos;
}
asblock, * asblockp;

/*
range scanning data
*/

typedef struct _range_scan_block
{
    EV_RANGE range;
    EV_COL col_size;
    EV_ROW row_size;
    EV_SLR pos;
    EV_SLR slr_of_result;
}
rsblock, * rsblockp;

/*-------------------------------------------------------------------------*/

typedef struct stat_blockD
{
    EV_IDNO function_id;
    S32 count;
    S32 count_a;
    S32 count1;
    F64 temp;
    F64 temp1;
    F64 parm;
    F64 parm1;
    F64 result1;
    EV_DATA result_data;
}
stat_block, * stat_blockp;

typedef struct lookup_blockD
{
    EV_DATA target_data;
    EV_DATA result_data;
    rsblock rsb;
    S32 in_range;
    S32 in_array;
    S32 lookup_id;
    S32 choose_count;
    S32 count;
    S32 break_count;
    S32 match;
    S32 had_one;
}
look_block, * look_blockp;

/*-------------------------------------------------------------------------*/

/*
evaluator stack definition
*/

struct stack_visit_slot
{
    struct ev_grub_state grubb;
};

struct stack_visit_range
{
    EV_RANGE range;
};

struct stack_visit_name
{
    EV_NAMEID nameid;
};

typedef struct stack_in_calc
{
    S32 offset;
    S32 type;
    EV_SLR slr;
    PC_U8 ptr;
    P_EV_DATA p_ev_data;
    P_EV_SLOT p_ev_slot;
    EV_DATA result_data;
    S32 did_calc;
    S32 travel_res;
    EV_SERIAL start_calc;
}
* stack_sicp;

enum in_calc_types
{
    INCALC_SLR,
    INCALC_PTR
};

struct stack_in_eval
{
    S32 stack_offset;
};

struct stack_data_item
{
    EV_DATA data;
};

typedef struct stack_control_loop
{
    S32 control_type;
    EV_SLR origin_slot;
    F64 step;
    F64 end;
    EV_NAMEID nameid;
}
* stack_sclp;

typedef struct stack_dbase_function
{
    EV_SLR dbase_slot;
    EV_RANGE dbase_rng;
    S16 cond_pos;
    EV_SLR offset;
    stat_blockp stbp;
}
* stack_dbasep;

typedef struct stack_lookup
{
    EV_DATA arg1;
    EV_DATA arg2;
    look_blockp lkbp;
}
* stack_lkp;

typedef struct _stack_executing_custom
{
    S32 stack_base;
    S32 nargs;
    EV_NAMEID custom_num;
    EV_SLR custom_slot;
    EV_SLR next_slot;
    S32 in_array;
    S32 xpos;
    S32 ypos;
    S32 elseif;
}
STACK_EXECUTING_CUSTOM, * P_STACK_EXECUTING_CUSTOM;

typedef struct _stack_processing_array
{
    S32 stack_base;
    S32 nargs;
    exec_proctp exec;
    S32 xpos;
    S32 ypos;
    S32 type_count;
    PC_EV_TYPE arg_types;
}
STACK_PROCESSING_ARRAY, * P_STACK_PROCESSING_ARRAY;

struct stack_alert_input
{
    S32 alert_input;
    char name_id[BUF_EV_INTNAMLEN];
};

typedef struct _stack_entry
{
    EV_SLR slr;                 /* slot to which entry relates */
    EV_FLAGS flags;             /* flags about entry */
    char type;                  /* type of impure part of entry */

    union _stack_entry_data
    {
        struct stack_visit_slot         svs;
        struct stack_visit_name         svn;
        struct stack_visit_range        svr;
        struct stack_in_calc            sic;
        struct stack_in_eval            sie;
        struct stack_data_item          sdi;
        struct stack_control_loop       scl;
        struct stack_dbase_function     sdb;
        struct stack_lookup             slk;
        STACK_EXECUTING_CUSTOM stack_executing_custom;
        struct _stack_processing_array  spa;
        struct stack_alert_input        sai;
    } data;
}
STACK_ENTRY, * stack_entryp;

#define STACK_INC 20 /* size of stack increments */

/*
definition of stack entry flags
*/

#define STF_CALCEDERROR     1   /* error has been stored in slot */
#define STF_CALCEDSUPPORTER 2   /* supporter was recalced */
#define STF_INCUSTOM        4   /* we are inside a custom */
#define STF_CIRCSOURCE      8   /* slot is source of circular reference */

enum stack_types
{
    VISIT_SLOT         ,
    VISIT_SUPPORTRNG   ,
    VISIT_COMPLETE_NAME,
    CALC_SLOT          ,
    IN_EVAL            ,
    END_CALC           ,
    CONTROL_LOOP       ,
    DBASE_FUNCTION     ,
    DBASE_CALC         ,
    LOOKUP_HAPPENING   ,
    CUSTOM_COMPLETE    ,
    EXECUTING_CUSTOM   ,
    DATA_ITEM          ,
    PROCESSING_ARRAY   ,
    SET_VALUE          ,
    ALERT_INPUT        ,
   _ERROR
};

enum recalc_states
{
    SAME_STATE         = 0,
    NEW_STATE
};

/*-------------------------------------------------------------------------*/

/*
ev_comp.c external functions
*/

extern S32
ident_validate(
    PC_U8 ident);

extern S32
ss_recog_date_time(
    _InoutRef_  P_EV_DATA p_ev_data,
    PC_U8 in_str,
    S32 american);

extern S32
recog_extref(
    _Out_writes_z_(elemof_buffer) P_U8 doc_name,
    _InVal_     S32 elemof_buffer,
    PC_U8 in_str,
    S32 keep_brackets);

/*
ev_dcom.c external functions
*/

extern S32
ev_dec_range(
    P_U8 op_buf,
    _InVal_     EV_DOCNO this_docno,
    _InRef_     PC_EV_RANGE p_ev_range,
    BOOL upper_case);

extern S32
ev_dec_slr(
    P_U8 op_buf,
    _InVal_     EV_DOCNO this_docno,
    _InRef_     PC_EV_SLR slrp,
    BOOL upper_case);

/*
ev_docs.c
*/

/*
ev_docs.c external functions
*/

#define doc_check_custom(docno) ( \
    ev_p_ss_doc_from_docno_must(docno)->flags & DCF_CUSTOM )

#define ev_doc_error(docno) ( \
    ev_p_ss_doc_from_docno_must(docno)->is_docu_thunk ? EVAL_ERR_CANTEXTREF : 0 )

extern S32
doc_move_extref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    extentp eep,
    EV_TRENT eix);

extern S32
doc_move_rngref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    P_RANGE_USE rep,
    EV_TRENT rix);

extern S32
doc_move_slrref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    P_SLR_USE sep,
    EV_TRENT six);

extern void
doc_reuse_hold(void);

extern void
doc_reuse_release(void);

_Check_return_
extern BOOL
docno_void_entry(
    _InVal_     EV_DOCNO docno);

/*
ev_eval.c
*/

/*
ev_eval.c external functions
*/

extern S32
stack_release_check(void);

extern void
stack_zap(
    _InRef_     PC_UREF_PARM upp);

/*
ev_exec.c external functions
*/

PROC_EXEC_PROTO(c_uplus);
PROC_EXEC_PROTO(c_umi);
PROC_EXEC_PROTO(c_not);

PROC_EXEC_PROTO(c_and);
PROC_EXEC_PROTO(c_mul);
PROC_EXEC_PROTO(c_add);
PROC_EXEC_PROTO(c_sub);
PROC_EXEC_PROTO(c_div);
PROC_EXEC_PROTO(c_power);
PROC_EXEC_PROTO(c_or);

PROC_EXEC_PROTO(c_eq);
PROC_EXEC_PROTO(c_gt);
PROC_EXEC_PROTO(c_gteq);
PROC_EXEC_PROTO(c_lt);
PROC_EXEC_PROTO(c_lteq);
PROC_EXEC_PROTO(c_neq);

PROC_EXEC_PROTO(c_age);
PROC_EXEC_PROTO(c_avg);

PROC_EXEC_PROTO(c_beta);
PROC_EXEC_PROTO(c_bin);
PROC_EXEC_PROTO(c_binom);

PROC_EXEC_PROTO(c_char);
PROC_EXEC_PROTO(c_choose);
PROC_EXEC_PROTO(c_code);
PROC_EXEC_PROTO(c_col);
PROC_EXEC_PROTO(c_cols);
PROC_EXEC_PROTO(c_combin);
PROC_EXEC_PROTO(c_count);
PROC_EXEC_PROTO(c_cterm);

PROC_EXEC_PROTO(c_date);
PROC_EXEC_PROTO(c_datevalue);
PROC_EXEC_PROTO(c_day);
PROC_EXEC_PROTO(c_dayname);
PROC_EXEC_PROTO(c_ddb);
PROC_EXEC_PROTO(c_deref);

PROC_EXEC_PROTO(c_error);
PROC_EXEC_PROTO(c_exact);

PROC_EXEC_PROTO(c_fact);
PROC_EXEC_PROTO(c_find);
PROC_EXEC_PROTO(c_flip);
PROC_EXEC_PROTO(c_formula_text);
PROC_EXEC_PROTO(c_fv);

PROC_EXEC_PROTO(c_gammaln);

PROC_EXEC_PROTO(c_hour);

PROC_EXEC_PROTO(c_if);
PROC_EXEC_PROTO(c_index);
PROC_EXEC_PROTO(c_irr);

PROC_EXEC_PROTO(c_join);

PROC_EXEC_PROTO(c_left);
PROC_EXEC_PROTO(c_length);
PROC_EXEC_PROTO(c_lower);

PROC_EXEC_PROTO(c_max);
PROC_EXEC_PROTO(c_min);
PROC_EXEC_PROTO(c_median);
PROC_EXEC_PROTO(c_mid);
PROC_EXEC_PROTO(c_minute);
PROC_EXEC_PROTO(c_mirr);
PROC_EXEC_PROTO(c_month);
PROC_EXEC_PROTO(c_monthdays);
PROC_EXEC_PROTO(c_monthname);

PROC_EXEC_PROTO(c_now);
PROC_EXEC_PROTO(c_npv);

PROC_EXEC_PROTO(c_permut);
PROC_EXEC_PROTO(c_pmt);
PROC_EXEC_PROTO(c_proper);
PROC_EXEC_PROTO(c_pv);

PROC_EXEC_PROTO(c_rand);
PROC_EXEC_PROTO(c_rank);
PROC_EXEC_PROTO(c_rate);
PROC_EXEC_PROTO(c_replace);
PROC_EXEC_PROTO(c_rept);
PROC_EXEC_PROTO(c_result);
PROC_EXEC_PROTO(c_reverse);
PROC_EXEC_PROTO(c_right);
PROC_EXEC_PROTO(c_round);
PROC_EXEC_PROTO(c_row);
PROC_EXEC_PROTO(c_rows);

PROC_EXEC_PROTO(c_second);
PROC_EXEC_PROTO(c_set_name);
PROC_EXEC_PROTO(c_setvalue);
PROC_EXEC_PROTO(c_sln);
PROC_EXEC_PROTO(c_sort);
PROC_EXEC_PROTO(c_spearman);
PROC_EXEC_PROTO(c_std);
PROC_EXEC_PROTO(c_stdp);
PROC_EXEC_PROTO(c_string);
PROC_EXEC_PROTO(c_sum);
PROC_EXEC_PROTO(c_syd);

PROC_EXEC_PROTO(c_term);
PROC_EXEC_PROTO(c_text);
PROC_EXEC_PROTO(c_time);
PROC_EXEC_PROTO(c_timevalue);
PROC_EXEC_PROTO(c_today);
PROC_EXEC_PROTO(c_trim);
PROC_EXEC_PROTO(c_type);

PROC_EXEC_PROTO(c_upper);

PROC_EXEC_PROTO(c_value);
PROC_EXEC_PROTO(c_var);
PROC_EXEC_PROTO(c_varp);
PROC_EXEC_PROTO(c_version);

PROC_EXEC_PROTO(c_weekday);
PROC_EXEC_PROTO(c_weeknumber);
PROC_EXEC_PROTO(c_while);

PROC_EXEC_PROTO(c_year);

extern void
dbase_sub_function(
    stack_dbasep sdbp,
    P_EV_DATA cond_flagp);

extern void
dbase_sub_function_finish(
    P_EV_DATA p_ev_data,
    stack_dbasep sdbp);

extern S32
lookup_array_range_proc(
    look_blockp lkbp,
    P_EV_DATA p_ev_data);

extern void
lookup_block_init(
    look_blockp lkbp,
    _InRef_opt_ P_EV_DATA p_ev_data_target,
    S32 lookup_id,
    S32 choose_count,
    S32 match);

extern void
lookup_finish(
    P_EV_DATA p_ev_data_res,
    stack_lkp slkp);

extern void
stat_block_init(
    stat_blockp stbp,
    S32 function_id,
    F64 parm,
    F64 parm1);

/*
ev_help.c external functions
*/

extern S32
arg_normalise(
    P_EV_DATA p_ev_data,
    EV_TYPE type_flags,
    P_S32 max_x,
    P_S32 max_y);

extern STATUS
array_copy(
    P_EV_DATA p_ev_data_to,
    _InRef_     PC_EV_DATA p_ev_data_from);

extern S32
array_expand(
    P_EV_DATA p_ev_data,
    S32 max_x,
    S32 max_y);

extern EV_IDNO
array_range_index(
    P_EV_DATA p_ev_data_out,
    P_EV_DATA p_ev_data_in,
    S32 x,
    S32 y,
    EV_TYPE types);

extern void
array_range_mono_index(
    P_EV_DATA p_ev_data_out,
    P_EV_DATA p_ev_data_in,
    S32 ix,
    EV_TYPE types);

extern void
array_range_sizes(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 xs,
    _OutRef_    P_S32 ys);

extern S32
array_scan_element(
    asblockp asbp,
    P_EV_DATA p_ev_data,
    EV_TYPE type_flags);

extern S32
array_scan_init(
    asblockp asbp,
    P_EV_DATA p_ev_data);

extern STATUS
array_sort(
    P_EV_DATA p_ev_data,
    _InVal_     S32 x_coord);

extern void
data_ensure_constant(
    P_EV_DATA p_ev_data);

extern BOOL
data_is_array_range(
    P_EV_DATA p_ev_data);

extern void
data_limit_types(
    P_EV_DATA p_ev_data,
    S32 array);

extern void
name_deref(
    P_EV_DATA p_ev_data,
    EV_NAMEID nameid);

extern S32
range_next(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InoutRef_  P_EV_SLR slrp);

extern S32
range_scan_init(
    _InRef_     PC_EV_RANGE p_ev_range,
    _OutRef_    rsblockp rsbp);

extern S32
range_scan_element(
    _InoutRef_  rsblockp rsbp,
    P_EV_DATA p_ev_data,
    EV_TYPE type_flags);

extern S32
two_nums_divide(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2);

extern S32
two_nums_minus(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2);

extern S32
two_nums_plus(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2);

extern S32
two_nums_times(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2);

/*
ev_math.c external functions
*/

PROC_EXEC_PROTO(c_abs);
PROC_EXEC_PROTO(c_acosh);
PROC_EXEC_PROTO(c_acosec);
PROC_EXEC_PROTO(c_acosech);
PROC_EXEC_PROTO(c_acot);
PROC_EXEC_PROTO(c_acoth);
PROC_EXEC_PROTO(c_acos);
PROC_EXEC_PROTO(c_asec);
PROC_EXEC_PROTO(c_asech);
PROC_EXEC_PROTO(c_asinh);
PROC_EXEC_PROTO(c_asin);
PROC_EXEC_PROTO(c_atan_2);
PROC_EXEC_PROTO(c_atanh);
PROC_EXEC_PROTO(c_atan);

PROC_EXEC_PROTO(c_c_acos);
PROC_EXEC_PROTO(c_c_acosh);
PROC_EXEC_PROTO(c_c_acosec);
PROC_EXEC_PROTO(c_c_acosech);
PROC_EXEC_PROTO(c_c_acot);
PROC_EXEC_PROTO(c_c_acoth);
PROC_EXEC_PROTO(c_c_add);
PROC_EXEC_PROTO(c_c_asec);
PROC_EXEC_PROTO(c_c_asech);
PROC_EXEC_PROTO(c_c_asin);
PROC_EXEC_PROTO(c_c_asinh);
PROC_EXEC_PROTO(c_c_atan);
PROC_EXEC_PROTO(c_c_atanh);
PROC_EXEC_PROTO(c_c_cos);
PROC_EXEC_PROTO(c_c_cosh);
PROC_EXEC_PROTO(c_c_cosec);
PROC_EXEC_PROTO(c_c_cosech);
PROC_EXEC_PROTO(c_c_cot);
PROC_EXEC_PROTO(c_c_coth);
PROC_EXEC_PROTO(c_c_div);
PROC_EXEC_PROTO(c_c_exp);
PROC_EXEC_PROTO(c_c_ln);
PROC_EXEC_PROTO(c_c_mul);
PROC_EXEC_PROTO(c_c_power);
PROC_EXEC_PROTO(c_c_radius);
PROC_EXEC_PROTO(c_c_sec);
PROC_EXEC_PROTO(c_c_sech);
PROC_EXEC_PROTO(c_c_sin);
PROC_EXEC_PROTO(c_c_sinh);
PROC_EXEC_PROTO(c_c_sub);
PROC_EXEC_PROTO(c_c_tan);
PROC_EXEC_PROTO(c_c_tanh);
PROC_EXEC_PROTO(c_c_theta);

PROC_EXEC_PROTO(c_cos);
PROC_EXEC_PROTO(c_cosh);
PROC_EXEC_PROTO(c_cosec);
PROC_EXEC_PROTO(c_cosech);
PROC_EXEC_PROTO(c_cot);
PROC_EXEC_PROTO(c_coth);

PROC_EXEC_PROTO(c_deg);

PROC_EXEC_PROTO(c_exp);

PROC_EXEC_PROTO(c_grand);
PROC_EXEC_PROTO(c_growth);

PROC_EXEC_PROTO(c_int);

PROC_EXEC_PROTO(c_linest);
PROC_EXEC_PROTO(c_listcount);
PROC_EXEC_PROTO(c_ln);
PROC_EXEC_PROTO(c_log);
PROC_EXEC_PROTO(c_logest);

PROC_EXEC_PROTO(c_m_determ);
PROC_EXEC_PROTO(c_m_inverse);
PROC_EXEC_PROTO(c_m_mult);

PROC_EXEC_PROTO(c_mod);

PROC_EXEC_PROTO(c_pi);

PROC_EXEC_PROTO(c_rad);

PROC_EXEC_PROTO(c_sec);
PROC_EXEC_PROTO(c_sech);
PROC_EXEC_PROTO(c_sin);
PROC_EXEC_PROTO(c_sinh);
PROC_EXEC_PROTO(c_sgn);
PROC_EXEC_PROTO(c_sqr);

PROC_EXEC_PROTO(c_tan);
PROC_EXEC_PROTO(c_tanh);
PROC_EXEC_PROTO(c_transpose);
PROC_EXEC_PROTO(c_trend);

/*
ev_name.c
*/

extern DEPTABLE custom_def;

extern DEPTABLE names_def;

/*
ev_name.c external functions
*/

extern void
change_doc_mac_nam(
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from);

extern EV_NAMEID
ensure_custom_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR custom_name);

extern EV_NAMEID
ensure_name_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR name);

extern EV_NAMEID
find_custom_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR custom_name);

extern EV_NAMEID
find_name_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR name);

extern EV_NAMEID
custom_def_find(
    EV_NAMEID key);

extern void
custom_list_sort(void);

#define custom_ptr(custom_num) ( \
    custom_def.ptr \
    ? ((P_EV_CUSTOM) custom_def.ptr + (S32) (custom_num)) \
    : NULL )

extern EV_NAMEID
name_def_find(
    EV_NAMEID key);

extern void
name_free_resources(
    P_EV_NAME p_ev_name);

extern void
name_list_sort(void);

extern S32
name_make(
    EV_NAMEID *nameidp,
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_USTR name,
    P_EV_DATA p_ev_data_in);

#define name_ptr(name_num) ( \
    names_def.ptr \
    ? ((P_EV_NAME) names_def.ptr + (S32) (name_num)) \
    : NULL )

/*
ev_rpn.c external functions
*/

extern void
grub_init(
    _InoutRef_  P_EV_GRUB grubp,
    _InRef_     PC_EV_SLR slrp);

extern S32
grub_next(
    P_EV_SLOT p_ev_slot,
    P_EV_GRUB grubp);

_Check_return_
extern S32
len_rpn(
    PC_U8 rpn_str);

extern void
read_cur_sym(
    rpnstatep rpnsp,
    P_EV_DATA p_ev_data);

#define read_from_rpn(to, from, size) \
    memcpy32((to), (from), (size))

extern void
read_nameid(
    _OutRef_    P_EV_NAMEID nameidp,
    PC_U8 ip_pos);

extern void
read_ptr(
    P_P_ANY ptrp,
    PC_U8 ip_pos);

extern void
read_range(
    _OutRef_    P_EV_RANGE p_ev_range,
    PC_U8 ip_pos);

extern void
read_slr(
    _OutRef_    P_EV_SLR slrp,
    PC_U8 ip_pos);

#define rpn_check(rpnsp) ( \
    (rpnsp)->num = (EV_IDNO) *((rpnsp)->pos) )

extern EV_IDNO
rpn_skip(
    rpnstatep rpnsp);

extern S32
write_nameid(
    EV_NAMEID nameid,
    P_U8 op_at);

extern S32
write_rng(
    _InRef_     PC_EV_RANGE p_ev_range,
    P_U8 op_at);

extern S32
write_slr(
    _InRef_     PC_EV_SLR slrp,
    P_U8 op_at);

/*
ev_tabl.c
*/

extern const RPNDEF rpn_table[];

/*
ev_tabl.c external functions
*/

extern S32
func_lookup(
    PC_USTR id);

extern PC_USTR
func_name(
    EV_IDNO did_num);

extern PC_USTR
type_from_flags(
    EV_TYPE type);

extern EV_TYPE
type_lookup(
    PC_USTR id);

/*
ev_tree.c
*/

extern DEPTABLE custom_use_table;

extern DEPTABLE namtab;

extern DEPTABLE todotab;

extern EV_FLAGS tree_flags;

/*
ev_tree.c external functions
*/

extern S32
dep_table_check_add_one(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size);

extern S32
ev_add_rngdependency(
    P_EV_GRUB grubp);

extern S32
ev_add_exp_slot_to_tree(
    P_EV_SLOT p_ev_slot,
    _InRef_     PC_EV_SLR slrp);

extern S32
ev_todo_add_custom_dependents(
    EV_NAMEID customid);

PROC_QSORT_PROTO(extern, namcomp, NAME_USE);

extern EV_TRENT
search_for_custom_use(
    EV_NAMEID customid);

extern EV_TRENT
search_for_name_use(
    EV_NAMEID nameid);

extern EV_TRENT
search_for_rng_ref(
    _InRef_     PC_EV_RANGE p_ev_range);

extern EV_TRENT
search_for_slrdependent(
    _InRef_     PC_EV_SLR slrp);

extern void
todo_add_dependents(
    _InRef_     PC_EV_SLR slrp);

extern S32
todo_add_custom_dependents(
    EV_NAMEID customid);

extern S32
todo_add_name_dependents(
    EV_NAMEID nameid);

#define TODO_SORT 1

_Check_return_ _Success_(return > 0)
extern S32
todo_get_slr(
    _OutRef_    P_EV_SLR slrp);

_Check_return_
_Ret_maybenull_
extern P_TODO_ENTRY
todo_has_slr(
    _InRef_     PC_EV_SLR slrp);

extern void
todo_remove_slr(
    _InRef_     PC_EV_SLR slrp);

#define todo_ptr(entry) ( \
    todotab.ptr \
    ? (P_TODO_ENTRY) todotab.ptr + (S32) (entry) \
    : NULL )

#define tree_extptr(p_ss_doc, entry) ( \
    (p_ss_doc)->exttab.ptr \
    ? (extentp) (p_ss_doc)->exttab.ptr + (S32) (entry) \
    : NULL )

#define tree_macptr(entry) ( \
    custom_use_table.ptr \
    ? (P_CUSTOM_USE) custom_use_table.ptr + (S32) (entry) \
    : NULL )

#define tree_namptr(entry) ( \
    namtab.ptr \
    ? (P_NAME_USE) namtab.ptr + (S32) (entry) \
    : NULL )

#define tree_rngptr(p_ss_doc, entry) ( \
    (p_ss_doc)->range_table.ptr \
    ? (P_RANGE_USE) (p_ss_doc)->range_table.ptr + (S32) (entry) \
    : NULL )

#define tree_slrptr(p_ss_doc, entry) ( \
    (p_ss_doc)->slr_table.ptr \
    ? (P_SLR_USE) (p_ss_doc)->slr_table.ptr + (S32) (entry) \
    : NULL )

extern S32
tree_sort(
    P_DEPTABLE dpp,
    S32 ent_size,
    EV_TRENT inc_size,
    S32 flag_offset,
    sort_proctp sort_proc);

extern void
tree_sort_all(void);

/*-------------------------------------------------------------------------*/

#endif /* __evali_h */

/* end of ev_evali.h */
