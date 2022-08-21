/* ev_evali.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header for evaluator */

/* MRJC January 1991 */

#ifndef __evali_h
#define __evali_h

#define EV_MAX_ARGS         25      /* maximum number of arguments */

/*
named resources
*/

typedef struct EV_NAME
{
    EV_NAMEID key;              /* internal id allocated to name */
    char id[BUF_EV_INTNAMLEN];  /* name of resource */
    EV_SLR owner;               /* document that owns name definition */
    SS_DATA def_data;           /* data defined by name */
    EV_FLAGS flags;             /* flags about entry */
    EV_SERIAL visited;          /* last visited count */
}
EV_NAME, * P_EV_NAME; typedef const EV_NAME * PC_EV_NAME;

/*
user defined custom functions
*/

typedef struct EV_CUSTOM_ARGS
{
    S32 n;                      /* number of arguments for custom */
    SBCHARZ id[EV_MAX_ARGS][BUF_EV_INTNAMLEN];
    EV_TYPE types[EV_MAX_ARGS];
}
EV_CUSTOM_ARGS, * P_EV_CUSTOM_ARGS;

typedef struct EV_CUSTOM
{
    EV_NAMEID key;              /* internal id for custom function */
    char id[BUF_EV_INTNAMLEN];  /* custom function name */
    EV_SLR owner;               /* document/slr of custom function */
    EV_FLAGS flags;             /* flags about entry */
    EV_CUSTOM_ARGS args;        /* not pointer */
}
EV_CUSTOM, * P_EV_CUSTOM, ** P_P_EV_CUSTOM;

/*
types for sort routines
*/

typedef int (* sort_proctp) (
    const void * arg1,
    const void * arg2);

/*
types for exec routines
*/

typedef void (* P_PROC_EXEC) (
    P_SS_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_EV_SLR p_cur_slr);

#define PROC_EXEC_PROTO(_p_proc_exec) \
extern void _p_proc_exec( \
    P_SS_DATA args[EV_MAX_ARGS], \
    _InVal_     S32 n_args, \
    _InoutRef_  P_SS_DATA p_ss_data_res, \
    _InRef_     PC_EV_SLR p_cur_slr)

#define exec_func_ignore_parms() (void) ( \
    UNREFERENCED_PARAMETER_InVal_(n_args), \
    UNREFERENCED_PARAMETER_InRef_(p_cur_slr) )

#define exec_func_status_return(p_ss_data_res, status) \
    do  { \
        const STATUS status_e = (status); \
        if(status_fail(status_e)) \
        { \
            ss_data_set_error(p_ss_data_res, status_e); \
            return; \
        } \
    } while_constant(false)

/* symbol information */

typedef struct SYM_INF
{
    EV_IDNO sym_idno;
    char sym_cr;
    char sym_space;
}
SYM_INF, * P_SYM_INF;

typedef struct FUN_PARMS
{
    UBF ex_type : 3;    /* exec, database, control, lookup */
    UBF no_exec : 4;    /* parameter for functions with no exec routine */
    UBF control : 4;    /* custom function control statements */
    UBF datab   : 1;    /* database function */
    UBF var     : 1;    /* function makes RPN variable */
    UBF nodep   : 1;    /* function argument needs no dependency */
}
FUN_PARMS;

#define FP_AGG(ex_type, no_exec, control, datab, var, nodep) \
    { (ex_type), (no_exec), (control), (datab), (var), (nodep) }

/*
exec types
*/

enum EXEC_TYPES
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

enum EXEC_CONTROL
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

enum EXEC_DBASE
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

enum EXEC_LOOKUP
{
    LOOKUP_LOOKUP,
    LOOKUP_HLOOKUP,
    LOOKUP_VLOOKUP,
    LOOKUP_MATCH,
    LOOKUP_CHOOSE
};

#define LOOKUP_BREAK_COUNT 100

/*
table of RPN items
*/

typedef struct RPNDEF
{
    EV_IDNO     rpn_type;
    S8          n_args;
    U8          category;
    FUN_PARMS   parms;
    P_PROC_EXEC p_proc_exec;
    PC_EV_TYPE  arg_types;  /* pointer to argument flags */
}
RPNDEF; typedef const RPNDEF * PC_RPNDEF;

/*
RPN engine data block
*/

typedef struct RPNSTATE
{
    EV_IDNO num;            /* current RPN token number */
    PC_U8 pos;              /* current position in RPN string */
}
RPNSTATE, * P_RPNSTATE;

#define DECOMP_ERR "Decompiler error"

/*
compare SLRs
*/

#define slr_equal(slr1, slr2) ( \
    (ev_slr_docno(slr2) == ev_slr_docno(slr1)) && \
    (ev_slr_col(slr2) == ev_slr_col(slr1)) && \
    (ev_slr_row(slr2) == ev_slr_row(slr1)) )

/*
compare SLR to range
*/

#define ev_slr_in_range(range, slr) ( \
    (ev_slr_docno(slr) == (range)->s.docno) && \
    (ev_slr_col(slr) >= (range)->s.col) && \
    (ev_slr_row(slr) >= (range)->s.row) && \
    (ev_slr_col(slr) <  (range)->e.col) && \
    (ev_slr_row(slr) <  (range)->e.row) )

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

/*
document flags
*/

#define DCF_IS_CUSTOM   1   /* document is a custom function sheet */
#define DCF_TREEDAMAGED 2   /* tree of document is damaged */

/*
recalc todo list
*/

typedef struct TODO_ENTRY
{
    EV_SLR slr;             /* slr to be done */
    EV_FLAGS flags;         /* flags about things */
}
TODO_ENTRY, * P_TODO_ENTRY;

/*
definition of the structure of
various dependency lists
*/

typedef struct EXTENTRY
{
    EV_RANGE  refto;        /* external reference to */
    U32       exthandle;    /* the outside world's bit */
    S32       inthandle;    /* internal handle */
    ev_urefct proc;         /* procedure to call */
    EV_FLAGS  flags;        /* flags about the entry */
}
* extentp;

typedef struct RANGE_USE
{
    EV_RANGE  refto;        /* range points to */
    EV_SLR    byslr;        /* cell containing range */
    S16       byoffset;     /* offset of range in containing cell */
    EV_FLAGS  flags;        /* flags about this reference */
    EV_SERIAL visited;      /* group visited flag */
}
RANGE_USE, * P_RANGE_USE;

typedef struct SLR_USE
{
    EV_SLR    refto;        /* reference points to */
    EV_SLR    byslr;        /* cell containing reference */
    S16       byoffset;     /* offset of reference/array in containing cell */
    EV_FLAGS  flags;        /* flags about this reference */
}
SLR_USE, * P_SLR_USE;

/*
structure of name use table
*/

typedef struct NAME_USE
{
    EV_NAMEID nameto;       /* reference to name.. */
    EV_SLR    byslr;        /* cell containing use of name */
    S16       byoffset;     /* offset of id in containing cell */
    EV_FLAGS  flags;        /* flags about this entry */
}
NAME_USE, * P_NAME_USE;

/*
structure of custom function use table
*/

typedef struct CUSTOM_USE
{
    EV_NAMEID custom_to;    /* reference to custom.. */
    EV_SLR    byslr;        /* cell containing reference to custom */
    EV_FLAGS  flags;        /* flags about this entry */
    S16       byoffset;     /* offset of id in containing cell */
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

/*
array scanning data
*/

typedef struct ARRAY_SCAN_BLOCK
{
    P_SS_DATA p_ss_data; /* contains an array */
    S32 x_pos;
    S32 y_pos;
}
ARRAY_SCAN_BLOCK, * P_ARRAY_SCAN_BLOCK;

/*
range scanning data
*/

typedef struct RANGE_SCAN_BLOCK
{
    EV_RANGE range;
    EV_COL col_size;
    EV_ROW row_size;
    EV_SLR pos;
    EV_SLR slr_of_result;
}
RANGE_SCAN_BLOCK, * P_RANGE_SCAN_BLOCK;

enum EXEC_ARRAY_RANGE
{
    ARRAY_RANGE_AVERAGE = 1,
    ARRAY_RANGE_COUNT,
    ARRAY_RANGE_COUNTA,
    ARRAY_RANGE_MAX,
    ARRAY_RANGE_MIN,
    ARRAY_RANGE_MIRR,
    ARRAY_RANGE_NPV,
    ARRAY_RANGE_STD,
    ARRAY_RANGE_STDP,
    ARRAY_RANGE_SUM,
    ARRAY_RANGE_VAR,
    ARRAY_RANGE_VARP
};

typedef struct STAT_BLOCK
{
    enum EXEC_ARRAY_RANGE exec_array_range_id;
    SS_DATA running_data;
    S32 count;
    S32 count_a;

    S32 count1;
    F64 temp;
    F64 temp1;
    F64 parm;
    F64 parm1;
    F64 result1;

    /* STD/STDP/VAR/VARP */
    F64 sum_x2;
  /*F64 shift_value;*/

#if CHECKING
    EV_IDNO _function_id;
#if !defined(EV_IDNO_U32)
    U8 _spare[4-sizeof(EV_IDNO)];
#endif
#endif
}
STAT_BLOCK, * P_STAT_BLOCK;

typedef struct LOOKUP_BLOCK
{
    SS_DATA target_data;
    SS_DATA result_data;
    RANGE_SCAN_BLOCK rsb;
    S32 in_range;
    S32 in_array;
    S32 lookup_id;
    S32 choose_count;
    S32 count;
    S32 break_count;
    S32 match;
    S32 had_one;
}
LOOKUP_BLOCK, * P_LOOKUP_BLOCK;

/*
evaluator stack definition
*/

typedef struct STACK_VISIT_SLOT
{
    struct EV_GRUB_STATE grubb;
}
STACK_VISIT_SLOT;

typedef struct STACK_VISIT_RANGE
{
    EV_RANGE range;
}
STACK_VISIT_RANGE;

typedef struct STACK_VISIT_NAME
{
    EV_NAMEID nameid;
}
STACK_VISIT_NAME;

typedef struct EVAL_BLOCK
{
    S32 offset;
    EV_SLR slr;
    P_EV_CELL p_ev_cell;
}
EVAL_BLOCK;

typedef struct STACK_IN_CALC
{
    EVAL_BLOCK eval_block;
    S32 travel_res;
    SS_DATA result_data;
    S32 did_calc;

    S32 type;
    PC_U8 ptr;
    EV_SERIAL start_calc;
}
STACK_IN_CALC, * P_STACK_IN_CALC;

enum IN_CALC_TYPES
{
    INCALC_SLR,
    INCALC_PTR
};

typedef struct STACK_IN_EVAL
{
    S32 stack_offset;
}
STACK_IN_EVAL;

typedef struct STACK_DATA_ITEM
{
    SS_DATA data;
}
STACK_DATA_ITEM;

typedef struct STACK_CONTROL_LOOP
{
    S32 control_type;
    EV_SLR origin_slot;
    F64 step;
    F64 end;
    EV_NAMEID nameid;
}
STACK_CONTROL_LOOP, * P_STACK_CONTROL_LOOP;

typedef struct STACK_DBASE
{
    EV_SLR dbase_slot;
    EV_RANGE dbase_rng;
    S16 cond_pos;
    EV_SLR offset;
    P_STAT_BLOCK p_stat_block;
}
STACK_DBASE, * P_STACK_DBASE;

typedef struct STACK_LOOKUP
{
    SS_DATA arg1;
    SS_DATA arg2;
    P_LOOKUP_BLOCK p_lookup_block;
}
STACK_LOOKUP, * P_STACK_LOOKUP;

typedef struct STACK_EXECUTING_CUSTOM
{
    S32 stack_base;
    S32 n_args;
    EV_NAMEID custom_num;
    EV_SLR custom_slot;
    EV_SLR next_slot;
    S32 in_array;
    S32 x_pos;
    S32 y_pos;
    S32 elseif;
}
STACK_EXECUTING_CUSTOM, * P_STACK_EXECUTING_CUSTOM;

typedef struct STACK_PROCESSING_ARRAY
{
    S32 stack_base;
    S32 n_args;
    P_PROC_EXEC exec;
    S32 x_pos;
    S32 y_pos;
    S32 type_count;
    PC_EV_TYPE arg_types;
}
STACK_PROCESSING_ARRAY, * P_STACK_PROCESSING_ARRAY;

typedef struct STACK_ALERT_INPUT
{
    S32 alert_input;
    char name_id[BUF_EV_INTNAMLEN];
}
STACK_ALERT_INPUT;

typedef struct STACK_ENTRY
{
    EV_SLR slr;                 /* cell to which entry relates */
    EV_FLAGS stack_flags;       /* flags about entry */
    char type;                  /* type of impure part of entry */

    union STACK_ENTRY_DATA
    {
        STACK_VISIT_SLOT stack_visit_slot;
        STACK_VISIT_NAME stack_visit_name;
        STACK_VISIT_RANGE stack_visit_range;
        STACK_IN_CALC stack_in_calc;
        STACK_IN_EVAL stack_in_eval;
        STACK_DATA_ITEM stack_data_item;
        STACK_CONTROL_LOOP stack_control_loop;
        STACK_DBASE stack_dbase;
        STACK_LOOKUP stack_lookup;
        STACK_EXECUTING_CUSTOM stack_executing_custom;
        STACK_PROCESSING_ARRAY stack_processing_array;
        STACK_ALERT_INPUT stack_alert_input;
    } data;
}
STACK_ENTRY, * P_STACK_ENTRY;

#define STACK_INC 20 /* size of stack increments */

/*
definition of stack entry flags
*/

#define STF_CALCEDERROR     1   /* error has been stored in cell */
#define STF_CALCEDSUPPORTER 2   /* supporter was recalced */
#define STF_INCUSTOM        4   /* we are inside a custom */
#define STF_CIRCSOURCE      8   /* cell is source of circular reference */

enum STACK_TYPES
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

enum RECALC_STATES
{
    SAME_STATE         = 0,
    NEW_STATE
};

/*
ev_comp.c external functions
*/

extern S32
ident_validate(
    PC_U8 ident);

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
    _InVal_     bool upper_case);

extern S32
ev_dec_slr(
    P_U8 op_buf,
    _InVal_     EV_DOCNO this_docno,
    _InRef_     PC_EV_SLR slrp,
    _InVal_     bool upper_case);

/*
ev_docs.c
*/

/*
ev_docs.c external functions
*/

#define doc_check_is_custom(ev_docno) ( \
    ev_p_ss_doc_from_docno_must(ev_docno)->flags & DCF_IS_CUSTOM )

#define ev_doc_error(ev_docno) ( \
    ev_p_ss_doc_from_docno_must(ev_docno)->is_docu_thunk ? EVAL_ERR_CANTEXTREF : 0 )

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
    _InoutRef_  P_RANGE_USE rep,
    EV_TRENT rix);

extern S32
doc_move_slrref(
    P_SS_DOC docep_from,
    _InVal_     EV_DOCNO docno_from,
    _InoutRef_  P_SLR_USE sep,
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

PROC_EXEC_PROTO(c_uop_plus);
PROC_EXEC_PROTO(c_uop_minus);
PROC_EXEC_PROTO(c_uop_not);

PROC_EXEC_PROTO(c_bop_and);
PROC_EXEC_PROTO(c_bop_mul);
PROC_EXEC_PROTO(c_bop_add);
PROC_EXEC_PROTO(c_bop_sub);
PROC_EXEC_PROTO(c_bop_div);
PROC_EXEC_PROTO(c_bop_power);
PROC_EXEC_PROTO(c_bop_or);

PROC_EXEC_PROTO(c_rel_eq);
PROC_EXEC_PROTO(c_rel_gt);
PROC_EXEC_PROTO(c_rel_gteq);
PROC_EXEC_PROTO(c_rel_lt);
PROC_EXEC_PROTO(c_rel_lteq);
PROC_EXEC_PROTO(c_rel_neq);

PROC_EXEC_PROTO(c_if);

/*
financial functions that use array processing
*/

PROC_EXEC_PROTO(c_mirr);
PROC_EXEC_PROTO(c_npv);

/*
mathematical functions that use array processing
*/

PROC_EXEC_PROTO(c_sum);

/*
statistical functions that use array processing
*/

PROC_EXEC_PROTO(c_avg);
PROC_EXEC_PROTO(c_count);
PROC_EXEC_PROTO(c_counta);
PROC_EXEC_PROTO(c_max);
PROC_EXEC_PROTO(c_min);
PROC_EXEC_PROTO(c_std);
PROC_EXEC_PROTO(c_stdp);
PROC_EXEC_PROTO(c_var);
PROC_EXEC_PROTO(c_varp);

extern void
dbase_stat_block_init(
    P_STAT_BLOCK stbp,
    _InVal_     U32 no_exec_id);

extern void
dbase_sub_function(
    P_STACK_DBASE p_stack_dbase,
    P_SS_DATA cond_flagp);

extern void
dbase_sub_function_finish(
    P_SS_DATA p_ss_data,
    P_STACK_DBASE p_stack_dbase);

extern S32
lookup_array_range_proc(
    P_LOOKUP_BLOCK lkbp,
    P_SS_DATA p_ss_data);

extern void
lookup_block_init(
    P_LOOKUP_BLOCK lkbp,
    _InRef_opt_ P_SS_DATA p_ss_data_target,
    S32 lookup_id,
    S32 choose_count,
    S32 match);

extern void
lookup_finish(
    P_SS_DATA p_ss_data_res,
    P_STACK_LOOKUP p_stack_lookup);

extern void
stat_block_init(
    P_STAT_BLOCK stbp,
    _InVal_     EV_IDNO function_id,
    F64 parm,
    F64 parm1);

/*
ev_func.c external functions
*/

/*
lookup and reference functions
*/

PROC_EXEC_PROTO(c_address);
PROC_EXEC_PROTO(c_choose);
PROC_EXEC_PROTO(c_col);
PROC_EXEC_PROTO(c_cols);
/* NO c_hlookup: LOOKUP_HLOOKUP */
PROC_EXEC_PROTO(c_index);
/* NO c_lookup: LOOKUP_LOOKUP */
/* NO c_match: LOOKUP_MATCH */
PROC_EXEC_PROTO(c_odf_index);
PROC_EXEC_PROTO(c_row);
PROC_EXEC_PROTO(c_rows);
/* NO c_vlookup: LOOKUP_VLOOKUP */

/*
miscellaneous functions
*/

/* NO PROC_EXEC_PROTO(c_command); - PipeDream can't do this */
/* NO c_countblank */
PROC_EXEC_PROTO(c_current_cell);
PROC_EXEC_PROTO(c_deref);
PROC_EXEC_PROTO(c_doubleclick);
PROC_EXEC_PROTO(c_even);
PROC_EXEC_PROTO(c_false);
PROC_EXEC_PROTO(c_flip);
PROC_EXEC_PROTO(c_isblank);
PROC_EXEC_PROTO(c_iserr);
PROC_EXEC_PROTO(c_iserror);
PROC_EXEC_PROTO(c_iseven);
PROC_EXEC_PROTO(c_islogical);
PROC_EXEC_PROTO(c_isna);
PROC_EXEC_PROTO(c_isnontext);
PROC_EXEC_PROTO(c_isnumber);
PROC_EXEC_PROTO(c_isodd);
PROC_EXEC_PROTO(c_isref);
PROC_EXEC_PROTO(c_istext);
PROC_EXEC_PROTO(c_na);
PROC_EXEC_PROTO(c_odd);
PROC_EXEC_PROTO(c_odf_type);
/* PROC_EXEC_PROTO(c_page); - PipeDream can't do this */
/* PROC_EXEC_PROTO(c_pages); - PipeDream can't do this */
PROC_EXEC_PROTO(c_set_name);
PROC_EXEC_PROTO(c_sort);
PROC_EXEC_PROTO(c_true);
PROC_EXEC_PROTO(c_type);
PROC_EXEC_PROTO(c_version);

PROC_EXEC_PROTO(c_setvalue); /* PipeDream does this differently */

/*
ev_fndat.c external functions (date and time)
*/

PROC_EXEC_PROTO(c_age);
PROC_EXEC_PROTO(c_date);
PROC_EXEC_PROTO(c_datevalue);
PROC_EXEC_PROTO(c_day);
PROC_EXEC_PROTO(c_dayname);
PROC_EXEC_PROTO(c_days_360);
PROC_EXEC_PROTO(c_edate);
PROC_EXEC_PROTO(c_eomonth);
PROC_EXEC_PROTO(c_hour);
PROC_EXEC_PROTO(c_minute);
PROC_EXEC_PROTO(c_month);
PROC_EXEC_PROTO(c_monthdays);
PROC_EXEC_PROTO(c_monthname);
PROC_EXEC_PROTO(c_now);
PROC_EXEC_PROTO(c_second);
PROC_EXEC_PROTO(c_time);
PROC_EXEC_PROTO(c_timevalue);
PROC_EXEC_PROTO(c_today);
PROC_EXEC_PROTO(c_weekday);
PROC_EXEC_PROTO(c_weeknumber);
PROC_EXEC_PROTO(c_year);

/*
NO ev_fndba.c (database)
*/

/* NO c_davg: DBASE_AVG */
/* NO c_dcount: DBASE_COUNT */
/* NO c_dcounta: DBASE_COUNTA */
/* NO c_dmax: DBASE_MAX */
/* NO c_dmin: DBASE_MIN */
/* NO c_dproduct: DBASE_PRODUCT */
/* NO c_dstd: DBASE_STD */
/* NO c_dstdp: DBASE_STDP */
/* NO c_dsum: DBASE_SUM */
/* NO c_dvar: DBASE_VAR */
/* NO c_dvarp: DBASE_VARP */

/*
ev_fneng.c external functions (engineering)
*/

PROC_EXEC_PROTO(c_besseli);
PROC_EXEC_PROTO(c_besselj);
PROC_EXEC_PROTO(c_besselk);
PROC_EXEC_PROTO(c_bessely);
PROC_EXEC_PROTO(c_delta);
PROC_EXEC_PROTO(c_erf);
PROC_EXEC_PROTO(c_erfc);
PROC_EXEC_PROTO(c_gestep);

/*
ev_fnfin.c external functions (financial)
*/

PROC_EXEC_PROTO(c_cterm);
PROC_EXEC_PROTO(c_db);
PROC_EXEC_PROTO(c_ddb);
PROC_EXEC_PROTO(c_fv);
PROC_EXEC_PROTO(c_fvschedule);
PROC_EXEC_PROTO(c_irr);
/* NO c_mirr */
PROC_EXEC_PROTO(c_nper);
/* NO c_npv */
PROC_EXEC_PROTO(c_odf_fv);
PROC_EXEC_PROTO(c_odf_irr);
PROC_EXEC_PROTO(c_odf_pmt);
PROC_EXEC_PROTO(c_pmt);
PROC_EXEC_PROTO(c_pv);
PROC_EXEC_PROTO(c_rate);
PROC_EXEC_PROTO(c_sln);
PROC_EXEC_PROTO(c_syd);
PROC_EXEC_PROTO(c_term);

/*
ev_fnsta.c external functions (statistical)
*/

/* NO c_avedev */
/* NO c_averagea */
/* NO c_avg */
PROC_EXEC_PROTO(c_beta);
PROC_EXEC_PROTO(c_bin);
PROC_EXEC_PROTO(c_combin);
/* NO c_count */
/* NO c_counta */
/* NO c_devsq */
PROC_EXEC_PROTO(c_frequency);
PROC_EXEC_PROTO(c_gammaln);
/* NO c_geomean */
PROC_EXEC_PROTO(c_grand);
/* NO c_harmean */
/* NO c_kurt */
PROC_EXEC_PROTO(c_listcount);
/* NO c_max */
/* c_median - PipeDream does this differently */
/* NO c_min */
PROC_EXEC_PROTO(c_permut);
PROC_EXEC_PROTO(c_rand);
PROC_EXEC_PROTO(c_randbetween);
PROC_EXEC_PROTO(c_rank);
/* NO c_skew */
/* NO c_skew_p */
PROC_EXEC_PROTO(c_spearman);
/* NO c_std */
/* NO c_stdp */
/* NO c_var */
/* NO c_varp */

extern void
binomial_coefficient_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 n,
    _InVal_     S32 k);

/*
ev_fnstb.c external functions (statistical)
*/


PROC_EXEC_PROTO(c_combina);
PROC_EXEC_PROTO(c_fisher);
PROC_EXEC_PROTO(c_fisherinv);
PROC_EXEC_PROTO(c_gamma);
PROC_EXEC_PROTO(c_large);
PROC_EXEC_PROTO(c_mode_sngl);
PROC_EXEC_PROTO(c_percentile_exc);
PROC_EXEC_PROTO(c_percentile_inc);
PROC_EXEC_PROTO(c_percentrank_exc);
PROC_EXEC_PROTO(c_percentrank_inc);
PROC_EXEC_PROTO(c_quartile_exc);
PROC_EXEC_PROTO(c_quartile_inc);
PROC_EXEC_PROTO(c_rank_eq);
PROC_EXEC_PROTO(c_small);
PROC_EXEC_PROTO(c_standardize);
PROC_EXEC_PROTO(c_trimmean);

PROC_EXEC_PROTO(c_median); /* PipeDream does this differently */

_Check_return_
extern F64
median_calc_span(
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     S32 y_start /*incl*/,
    _InVal_     S32 y_end   /*excl*/);

_Check_return_
extern BOOL
statistics_value_next(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size);

/*
ev_fnstc.c external functions (statistical - continuous distributions)
*/

PROC_EXEC_PROTO(c_beta_dist);
PROC_EXEC_PROTO(c_beta_inv);
PROC_EXEC_PROTO(c_chisq_dist);
PROC_EXEC_PROTO(c_chisq_dist_rt);
PROC_EXEC_PROTO(c_chisq_inv);
PROC_EXEC_PROTO(c_chisq_inv_rt);
PROC_EXEC_PROTO(c_chisq_test);
PROC_EXEC_PROTO(c_confidence_norm);
PROC_EXEC_PROTO(c_confidence_t);
PROC_EXEC_PROTO(c_expon_dist);
PROC_EXEC_PROTO(c_F_dist);
PROC_EXEC_PROTO(c_F_dist_rt);
PROC_EXEC_PROTO(c_F_inv);
PROC_EXEC_PROTO(c_F_inv_rt);
PROC_EXEC_PROTO(c_F_test);
PROC_EXEC_PROTO(c_gamma_dist);
PROC_EXEC_PROTO(c_gamma_inv);
PROC_EXEC_PROTO(c_lognorm_dist);
PROC_EXEC_PROTO(c_lognorm_inv);
PROC_EXEC_PROTO(c_norm_dist);
PROC_EXEC_PROTO(c_norm_s_dist);
PROC_EXEC_PROTO(c_norm_inv);
PROC_EXEC_PROTO(c_norm_s_inv);
PROC_EXEC_PROTO(c_odf_betadist);
PROC_EXEC_PROTO(c_odf_tdist);
PROC_EXEC_PROTO(c_phi);
PROC_EXEC_PROTO(c_t_dist);
PROC_EXEC_PROTO(c_t_dist_rt);
PROC_EXEC_PROTO(c_t_dist_2t);
PROC_EXEC_PROTO(c_t_inv);
PROC_EXEC_PROTO(c_t_inv_2t);
PROC_EXEC_PROTO(c_t_test);
PROC_EXEC_PROTO(c_weibull_dist);
PROC_EXEC_PROTO(c_z_test);

/*
ev_fnstd.c external functions (statistical - discrete distributions)
*/

PROC_EXEC_PROTO(c_binom_dist);
PROC_EXEC_PROTO(c_binom_dist_range);
PROC_EXEC_PROTO(c_binom_inv);
PROC_EXEC_PROTO(c_hypgeom_dist);
PROC_EXEC_PROTO(c_negbinom_dist);
PROC_EXEC_PROTO(c_poisson_dist);

/*
ev_fnstm.c external functions (statistical - multivariate fit)
*/

PROC_EXEC_PROTO(c_growth);
PROC_EXEC_PROTO(c_linest);
PROC_EXEC_PROTO(c_logest);
PROC_EXEC_PROTO(c_trend);

/*
ev_fnstp.c external functions (statistical - paired statistics)
*/

PROC_EXEC_PROTO(c_correl);
PROC_EXEC_PROTO(c_covariance_p);
PROC_EXEC_PROTO(c_covariance_s);
PROC_EXEC_PROTO(c_forecast);
PROC_EXEC_PROTO(c_frequency);
PROC_EXEC_PROTO(c_intercept);
PROC_EXEC_PROTO(c_pearson);
PROC_EXEC_PROTO(c_prob);
PROC_EXEC_PROTO(c_rsq);
PROC_EXEC_PROTO(c_slope);
PROC_EXEC_PROTO(c_steyx);

_Check_return_
extern BOOL
statistics_paired_values_next(
    _OutRef_    P_SS_DATA p_ss_data_out_a,
    _OutRef_    P_SS_DATA p_ss_data_out_b,
    _InRef_     PC_SS_DATA p_ss_data_in_a,
    _InRef_     PC_SS_DATA p_ss_data_in_b,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size);

/*
ev_fnstr.c external functions (string)
*/

PROC_EXEC_PROTO(c_char);
PROC_EXEC_PROTO(c_clean);
PROC_EXEC_PROTO(c_code);
PROC_EXEC_PROTO(c_dollar);
PROC_EXEC_PROTO(c_exact);
PROC_EXEC_PROTO(c_find);
PROC_EXEC_PROTO(c_fixed);
PROC_EXEC_PROTO(c_formula_text);
PROC_EXEC_PROTO(c_join);
PROC_EXEC_PROTO(c_left);
PROC_EXEC_PROTO(c_length);
PROC_EXEC_PROTO(c_lower);
PROC_EXEC_PROTO(c_mid);
PROC_EXEC_PROTO(c_n);
PROC_EXEC_PROTO(c_proper);
PROC_EXEC_PROTO(c_rept);
PROC_EXEC_PROTO(c_replace);
PROC_EXEC_PROTO(c_reverse);
PROC_EXEC_PROTO(c_right);
PROC_EXEC_PROTO(c_string);
PROC_EXEC_PROTO(c_substitute);
PROC_EXEC_PROTO(c_t);
PROC_EXEC_PROTO(c_text);
PROC_EXEC_PROTO(c_trim);
PROC_EXEC_PROTO(c_upper);
PROC_EXEC_PROTO(c_value);

/*
ev_help.c external functions
*/

_Check_return_
extern S32
arg_normalise(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 max_x,
    _InoutRef_opt_ P_S32 max_y);

/* several spreadsheet functions define that they INT() their real arg (ODF INT() towards -inf for consistency) */
#define arg_get_real_INT(p_ss_data) \
    real_floor(ss_data_get_real(p_ss_data))

_Check_return_
extern STATUS
array_copy(
    _InoutRef_  P_SS_DATA p_ss_data_to,
    _InRef_     PC_SS_DATA p_ss_data_from);

_Check_return_
extern S32
array_expand(
    P_SS_DATA p_ss_data,
    S32 max_x,
    S32 max_y);

/*ncr*/
extern EV_IDNO
array_range_index(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types);

extern void
array_range_mono_index(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE types);

extern void
array_range_sizes(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size);

extern void
array_range_mono_size(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_mono_size);

_Check_return_
extern S32
array_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK asbp,
    P_SS_DATA p_ss_data,
    EV_TYPE type_flags);

_Check_return_
extern S32
array_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK asbp,
    P_SS_DATA p_ss_data);

_Check_return_
extern STATUS
array_sort(
    P_SS_DATA p_ss_data,
    _InVal_     U32 x_index);

extern void
data_ensure_constant(
    P_SS_DATA p_ss_data);

_Check_return_
extern BOOL
data_is_array_range(
    _InRef_     PC_SS_DATA p_ss_data);

extern void
data_limit_types(
    P_SS_DATA p_ss_data,
    S32 array);

extern void
name_deref(
    P_SS_DATA p_ss_data,
    EV_NAMEID nameid);

_Check_return_
extern S32
range_next(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InoutRef_  P_EV_SLR slrp);

_Check_return_
extern S32
range_scan_init(
    _InRef_     PC_EV_RANGE p_ev_range,
    _OutRef_    P_RANGE_SCAN_BLOCK rsbp);

_Check_return_
extern S32
range_scan_element(
    _InoutRef_  P_RANGE_SCAN_BLOCK rsbp,
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags);

_Check_return_ _Success_(return)
extern bool
two_nums_add_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2);

_Check_return_ _Success_(return)
extern bool
two_nums_subtract_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2);

_Check_return_ _Success_(return)
extern bool
two_nums_multiply_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2);

_Check_return_ _Success_(return)
extern bool
two_nums_divide_try(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2);

/*
ev_math.c external functions
*/

/*
mathematical functions
*/

PROC_EXEC_PROTO(c_abs);
PROC_EXEC_PROTO(c_exp);
PROC_EXEC_PROTO(c_fact);
PROC_EXEC_PROTO(c_int);
PROC_EXEC_PROTO(c_ln);
PROC_EXEC_PROTO(c_odf_log10);
PROC_EXEC_PROTO(c_mod);
PROC_EXEC_PROTO(c_sgn);
PROC_EXEC_PROTO(c_sqr);
/* c_sum */

extern void
factorial_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 n);

extern void
product_between_calc(
    _InoutRef_  P_SS_DATA p_ss_data_res, /* denotes integer or real; may return integer or real. NB must contain 'start' value as integer or real */
  /*_InVal_     S32 start,*/
    _InVal_     S32 end);

_Check_return_
extern STATUS
status_from_errno(void);

/*
ev_matb.c external functions (mathematical)
*/

PROC_EXEC_PROTO(c_base);
PROC_EXEC_PROTO(c_ceiling);
PROC_EXEC_PROTO(c_decimal);
PROC_EXEC_PROTO(c_factdouble);
PROC_EXEC_PROTO(c_floor);
PROC_EXEC_PROTO(c_log);
PROC_EXEC_PROTO(c_mround);
PROC_EXEC_PROTO(c_odf_int);
PROC_EXEC_PROTO(c_quotient);
PROC_EXEC_PROTO(c_round);
PROC_EXEC_PROTO(c_rounddown);
PROC_EXEC_PROTO(c_roundup);
PROC_EXEC_PROTO(c_seriessum);
/* c_sumsq */
PROC_EXEC_PROTO(c_sumproduct);
PROC_EXEC_PROTO(c_sum_x2my2);
PROC_EXEC_PROTO(c_sum_x2py2);
PROC_EXEC_PROTO(c_sum_xmy2);
PROC_EXEC_PROTO(c_trunc);

extern void
round_common(
    P_SS_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     U32 rpn_did_num);

/*
ev_matr.c external functions (matrix)
*/

PROC_EXEC_PROTO(c_m_determ);
PROC_EXEC_PROTO(c_m_inverse);
PROC_EXEC_PROTO(c_m_mult);
PROC_EXEC_PROTO(c_m_unit);
PROC_EXEC_PROTO(c_transpose);

/*
ev_mcpx.c external functions (complex number)
*/

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
PROC_EXEC_PROTO(c_c_complex);
PROC_EXEC_PROTO(c_c_conjugate);
PROC_EXEC_PROTO(c_c_cos);
PROC_EXEC_PROTO(c_c_cosh);
PROC_EXEC_PROTO(c_c_cosec);
PROC_EXEC_PROTO(c_c_cosech);
PROC_EXEC_PROTO(c_c_cot);
PROC_EXEC_PROTO(c_c_coth);
PROC_EXEC_PROTO(c_c_div);
PROC_EXEC_PROTO(c_c_exp);
PROC_EXEC_PROTO(c_c_imaginary);
PROC_EXEC_PROTO(c_c_ln);
PROC_EXEC_PROTO(c_c_log_10);
PROC_EXEC_PROTO(c_c_log_2);
PROC_EXEC_PROTO(c_c_mul);
PROC_EXEC_PROTO(c_c_power);
PROC_EXEC_PROTO(c_c_radius);
PROC_EXEC_PROTO(c_c_real);
PROC_EXEC_PROTO(c_c_round);
PROC_EXEC_PROTO(c_c_sec);
PROC_EXEC_PROTO(c_c_sech);
PROC_EXEC_PROTO(c_c_sin);
PROC_EXEC_PROTO(c_c_sinh);
PROC_EXEC_PROTO(c_c_sqrt);
PROC_EXEC_PROTO(c_c_sub);
PROC_EXEC_PROTO(c_c_tan);
PROC_EXEC_PROTO(c_c_tanh);
PROC_EXEC_PROTO(c_c_theta);

PROC_EXEC_PROTO(c_odf_complex);

/*
ev_mtri.c external functions (trigonometrical)
*/

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
PROC_EXEC_PROTO(c_cos);
PROC_EXEC_PROTO(c_cosh);
PROC_EXEC_PROTO(c_cosec);
PROC_EXEC_PROTO(c_cosech);
PROC_EXEC_PROTO(c_cot);
PROC_EXEC_PROTO(c_coth);
PROC_EXEC_PROTO(c_deg);
PROC_EXEC_PROTO(c_pi);
PROC_EXEC_PROTO(c_rad);
PROC_EXEC_PROTO(c_sec);
PROC_EXEC_PROTO(c_sech);
PROC_EXEC_PROTO(c_sin);
PROC_EXEC_PROTO(c_sinh);
PROC_EXEC_PROTO(c_tan);
PROC_EXEC_PROTO(c_tanh);

/*
ev_name.c
*/

extern DEPTABLE custom_def_deptable;

extern DEPTABLE names_def_deptable;

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

_Check_return_
extern EV_NAMEID
find_custom_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR custom_name);

_Check_return_
extern EV_NAMEID
find_name_in_list(
    _InVal_     EV_DOCNO owner_docno,
    _In_z_      PC_USTR name);

_Check_return_
extern EV_NAMEID
custom_def_find(
    EV_NAMEID key);

extern void
custom_list_sort(void);

#define custom_ptr(custom_num) ( \
    custom_def_deptable.ptr \
    ? ((P_EV_CUSTOM) custom_def_deptable.ptr + (S32) (custom_num)) \
    : NULL )

#define custom_ptr_must(custom_num) ( \
    ((P_EV_CUSTOM) custom_def_deptable.ptr + (S32) (custom_num)) )

_Check_return_
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
    P_SS_DATA p_ss_data_in);

#define name_ptr(name_num) ( \
    names_def_deptable.ptr \
    ? ((P_EV_NAME) names_def_deptable.ptr + (S32) (name_num)) \
    : NULL )

#define name_ptr_must(name_num) ( \
    ((P_EV_NAME) names_def_deptable.ptr + (S32) (name_num)) )

/*
ev_rpn.c external functions
*/

extern void
grub_init(
    _InoutRef_  P_EV_GRUB grubp,
    _InRef_     PC_EV_SLR slrp);

extern S32
grub_next(
    P_EV_CELL p_ev_cell,
    P_EV_GRUB grubp);

_Check_return_
extern S32
len_rpn(
    PC_U8 rpn_str);

extern void
read_cur_sym(
    P_RPNSTATE rpnsp,
    P_SS_DATA p_ss_data);

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
    P_RPNSTATE rpnsp);

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
    _In_z_      PC_USTR id);

extern PC_USTR
func_name(
    EV_IDNO did_num);

extern PC_A7STR
type_name_from_type_flags(
    EV_TYPE type_flags);

extern EV_TYPE
type_lookup(
    _In_z_      PC_USTR id);

/*
ev_tree.c
*/

extern DEPTABLE custom_use_deptable;

extern DEPTABLE name_use_deptable;

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
    P_EV_CELL p_ev_cell,
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
    custom_use_deptable.ptr \
    ? (P_CUSTOM_USE) custom_use_deptable.ptr + (S32) (entry) \
    : NULL )

#define tree_namptr(entry) ( \
    name_use_deptable.ptr \
    ? (P_NAME_USE) name_use_deptable.ptr + (S32) (entry) \
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

#endif /* __evali_h */

/* end of ev_evali.h */
