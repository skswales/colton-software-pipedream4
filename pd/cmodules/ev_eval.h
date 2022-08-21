/* ev_eval.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Evaluator external header file */

/* MRJC April 1991 */

#ifndef __ev_eval_h
#define __ev_eval_h

#ifndef          __ss_const_h
#include "cmodules/ss_const.h"
#endif

#ifndef ERRDEF_EXPORT

#if RISCOS || WINDOWS

typedef int                 EV_TRENT;
#define EV_TRENT_INIT       0

typedef U32                 EV_SERIAL;

#define SLOTS_MOVE                      /* pool-based handlist */

#else
#error Needs sizeof things defining
#endif

/*
name block
*/

typedef struct DOCU_NAME_FLAGS
{
    U8 path_name_supplied /*UBF : 1*/;
}
DOCU_NAME_FLAGS;

typedef struct DOCU_NAME
{
    PTSTR path_name;
    PTSTR leaf_name;
    DOCU_NAME_FLAGS flags;
}
DOCU_NAME, * P_DOCU_NAME; typedef const DOCU_NAME * PC_DOCU_NAME;

#define DOCU_NAME_INIT { NULL, NULL, { 0 } }

/*
cell/range flags
*/

#define SLR_ABS_COL         1
#define SLR_ABS_ROW         2
#define SLR_BAD_REF         4
#define SLR_EXT_REF         8
#define SLR_ALL_REF         16

/*
evaluator result type - stored in cell
*/

typedef struct EV_RESULT
{
    EV_IDNO did_num;

    EV_CONSTANT arg;
}
EV_RESULT, * P_EV_RESULT; typedef const EV_RESULT * PC_EV_RESULT;

/*
definition of expression parameters
*/

typedef struct EV_PARMS
{
    UBF type    : 2;
    UBF control : 4;
    UBF circ    : 1;
}
EV_PARMS, * P_EV_PARMS;

/* expression types */

enum EXP_TYPES
{
    EVS_CON_DATA = 1,           /* constant data (no rpn field) */
    EVS_CON_RPN     ,           /* constant rpn */
    EVS_VAR_RPN                 /* variable rpn */
};

/* control statement types */

enum CONTROL_TYPES
{
    EVS_CNT_NONE   = 1,
    EVS_CNT_IFC       ,
    EVS_CNT_ELSE      ,
    EVS_CNT_ELSEIF    ,
    EVS_CNT_ENDIF     ,
    EVS_CNT_WHILE     ,
    EVS_CNT_ENDWHILE  ,
    EVS_CNT_REPEAT    ,
    EVS_CNT_UNTIL     ,
    EVS_CNT_FOR       ,
    EVS_CNT_NEXT
};

/*
evaluator's view of a cell
*/

typedef struct EV_CELL
{
    EV_RESULT ev_result;        /* result of expression */
    EV_PARMS parms;             /* parameters about cell */

    /* members below this point may  or may not be present,
     * depending on the type of cell; flags tells the cell type
     */
    union EV_CELL_RPN
    {
        struct EV_CELL_RPN_VAR
        {
            EV_SERIAL visited;          /* cell was last visited on.. */
            char rpn_str[1];            /* rpn string of expression */
        } var;

        struct EV_CELL_RPN_CON
        {
            char rpn_str[1];
        } con;

    } rpn;
}
EV_CELL, * P_EV_CELL, ** P_P_EV_CELL;

/*
evaluator option block
*/

typedef struct EV_OPTBLOCK
{
    UBF american_date : 1;
    UBF upper_case : 1;
    UBF cr : 1;
    UBF lf : 1;
    UBF upper_case_slr : 1;
}
EV_OPTBLOCK, * P_EV_OPTBLOCK; typedef const EV_OPTBLOCK * PC_EV_OPTBLOCK;

/*
uref things
*/

typedef struct UREF_PARM
{
    EV_SLR slr1;
    EV_SLR slr2;
    EV_SLR slr3;
    S32 action;
}
UREF_PARM, * P_UREF_PARM; typedef const UREF_PARM * PC_UREF_PARM;

typedef void (* ev_urefct) (
    _InoutRef_  P_EV_RANGE at_rng,
    _InRef_     PC_UREF_PARM upp,
    U32 exthandle,
    S32 inthandle,
    S32 status);

#define PROC_UREF_PROTO(_e_s, _p_proc_uref) \
_e_s void _p_proc_uref( \
    _InoutRef_  P_EV_RANGE at_rng, \
    _InRef_     PC_UREF_PARM upp, \
    U32 exthandle, \
    S32 inthandle, \
    S32 status)

enum UREF_ACTIONS
{
    UREF_UREF = 0  ,
    UREF_DELETE    ,
    UREF_SWAP      ,
    UREF_CHANGEDOC ,
    UREF_CHANGE    ,
    UREF_REPLACE   ,
    UREF_COPY      ,
    UREF_SWAPCELL  ,
    UREF_CLOSE     ,
    UREF_REDRAW    ,
    UREF_ADJUST    ,
    UREF_RENAME
};

/*
internal communication in uref
*/

enum UREF_COMMS
{
    DEP_DELETE    ,             /* dependent needs deleting */
    DEP_UPDATE    ,             /* dependent reference needs updating */
    DEP_INFORM    ,             /* dependent needs informing */
    DEP_NONE                    /* no action for dependent */
};

/*
grubber state block
*/

typedef struct EV_GRUB_STATE
{
    S32 offset;             /* offset into rpn of cell being grubbed */
    EV_SLR slr;             /* slr being grubbed in */
    EV_DATA data;           /* data element grubbed */
    S16 byoffset;           /* rpn offset for use external to grubber */
    S32 in_cond;            /* in condition flag */
}
* P_EV_GRUB;

/*
evaluator resource enumeration
*/

enum EV_RESOURCE_TYPES
{
    EV_RESO_COMPLEX    ,
    EV_RESO_DATABASE   ,
    EV_RESO_DATE       ,
    EV_RESO_FINANCE    ,
    EV_RESO_MATHS      ,
    EV_RESO_MATRIX     ,
    EV_RESO_MISC       ,
    EV_RESO_STATS      ,
    EV_RESO_STRING     ,
    EV_RESO_TRIG       ,
    EV_RESO_CONTROL    ,
    EV_RESO_LOOKUP     ,
    EV_RESO_CUSTOM     ,
    EV_RESO_NAMES      ,
    EV_RESO_NOTME
};

typedef struct EV_RESOURCE
{
    enum EV_RESOURCE_TYPES category;
    EV_DOCNO docno_to;
    EV_DOCNO docno_from;
    S32 item_no;
    EV_OPTBLOCK optblock;
}
EV_RESOURCE, * P_EV_RESOURCE;

/*
evaluator dependents/supporters enumeration
*/

enum EV_DEPSUP_TYPES
{
    EV_DEPSUP_SLR,
    EV_DEPSUP_RANGE,
    EV_DEPSUP_NAME
};

typedef struct EV_DEPSUP
{
    S32 item_no;
    S32 get_deps;
    enum EV_DEPSUP_TYPES category;
    EV_SLR slr;
    EV_TRENT tab_pos;
}
EV_DEPSUP, * P_EV_DEPSUP;

/*
tree structures
*/

typedef struct DEPTABLE
{
    P_ANY ptr;                          /* pointer to table */
    EV_TRENT next;                      /* next free entry in table */
    EV_TRENT size;                      /* number of table entries allocated */

    EV_TRENT sorted;                    /* sorted limit */
    EV_FLAGS flags;                     /* flags about table */
    EV_TRENT mindel;                    /* minimum deleted entry */
}
DEPTABLE, * P_DEPTABLE;

#define DEPTABLE_INIT { \
    NULL, \
    EV_TRENT_INIT, \
    EV_TRENT_INIT, \
    EV_TRENT_INIT, \
    EV_FLAGS_INIT, \
    EV_TRENT_INIT  \
}

/*
* the document tree is an array of docentry structures, indexed by document number
*/

typedef struct SS_DOC
{
    DOCU_NAME docu_name;                /* document name details */
    EV_FLAGS flags;                     /* flags about documents */
    U8 is_docu_thunk;
    S32 error;                          /* error code */

    DEPTABLE slr_table;                 /* slr dependent table */
    DEPTABLE range_table;               /* range dependent table */
    DEPTABLE exttab;                    /* external dependent table */

    S32 nam_ref_count;                  /* ref count of names (to this doc) */
    S32 custom_ref_count;               /* ref count of custom functions (to this doc) */

    S32 stack_max;                      /* maximum size of stack used by this doc */
}
SS_DOC, * P_SS_DOC;

#define SS_DOC_INIT { \
    DOCU_NAME_INIT, \
    EV_FLAGS_INIT, \
    FALSE, \
    0, \
    DEPTABLE_INIT, \
    DEPTABLE_INIT, \
    DEPTABLE_INIT, \
    0, \
    0, \
    0  \
}

/*
rpn atomic numbers
*/

enum DID_NUMBERS
{
    /* data */
    RPN_DAT_REAL        =0,
    RPN_DAT_WORD8       ,
    RPN_DAT_WORD16      ,
    RPN_DAT_WORD32      ,
    RPN_DAT_SLR         ,
    RPN_DAT_RANGE       ,
    RPN_DAT_DATE        ,
    RPN_DAT_NAME        ,
    RPN_DAT_BLANK       ,
    RPN_DAT_ERROR       ,
#define RPN_DAT_BOOL8 RPN_DAT_WORD8 /* synonym in PipeDream */

    /* handle based resources owned by RPN */
    RPN_DAT_STRING      ,

    /* temporary handle based resources */
    RPN_TMP_STRING      ,
    RPN_TMP_ARRAY       ,

    /* handle based resources owned by a result */
    RPN_RES_STRING      ,
    RPN_RES_ARRAY       ,

    /* local argument */
    RPN_LCL_ARGUMENT    ,

    /* format information */
    RPN_FRM_BRACKETS    ,
    RPN_FRM_SPACE       ,
    RPN_FRM_RETURN      ,
    RPN_FRM_END         ,
    RPN_FRM_COND        ,
    RPN_FRM_SKIPFALSE   ,
    RPN_FRM_SKIPTRUE    ,
    RPN_FRM_NODEP       ,

    /* unary operators */
    RPN_UOP_NOT         ,
    RPN_UOP_UMINUS      ,
    RPN_UOP_UPLUS       ,

    /* binary operators */
    RPN_BOP_AND         ,
    RPN_BOP_DIVIDE      ,
    RPN_BOP_MINUS       ,
    RPN_BOP_OR          ,
    RPN_BOP_PLUS        ,
    RPN_BOP_POWER       ,
    RPN_BOP_TIMES       ,

    /* binary relational operators */
    RPN_REL_EQUALS      ,
    RPN_REL_GT          ,
    RPN_REL_GTEQUAL     ,
    RPN_REL_LT          ,
    RPN_REL_LTEQUAL     ,
    RPN_REL_NOTEQUAL    ,

    /* functions */
    RPN_FNF_ABS         ,
    RPN_FNF_ACOSEC      ,
    RPN_FNF_ACOSECH     ,
    RPN_FNF_ACOSH       ,
    RPN_FNF_ACOT        ,
    RPN_FNF_ACOTH       ,
    RPN_FNF_ACS         ,
    RPN_FNF_AGE         ,
    RPN_FNV_ALERT       ,
    RPN_FNF_ASEC        ,
    RPN_FNF_ASECH       ,
    RPN_FNF_ASINH       ,
    RPN_FNF_ASN         ,
    RPN_FNF_ATAN_2      ,
    RPN_FNF_ATANH       ,
    RPN_FNF_ATN         ,
    RPN_FNV_AVG         ,

    RPN_FNF_BETA        ,
    RPN_FNF_BIN         ,
    RPN_FNV_BREAK       ,

    RPN_FNF_C_ACOS      ,
    RPN_FNF_C_ACOSEC    ,
    RPN_FNF_C_ACOSECH   ,
    RPN_FNF_C_ACOSH     ,
    RPN_FNF_C_ACOT      ,
    RPN_FNF_C_ACOTH     ,
    RPN_FNF_C_ADD       ,
    RPN_FNF_C_ASEC      ,
    RPN_FNF_C_ASECH     ,
    RPN_FNF_C_ASIN      ,
    RPN_FNF_C_ASINH     ,
    RPN_FNF_C_ATAN      ,
    RPN_FNF_C_ATANH     ,
    RPN_FNF_C_COS       ,
    RPN_FNF_C_COSEC     ,
    RPN_FNF_C_COSECH    ,
    RPN_FNF_C_COSH      ,
    RPN_FNF_C_COT       ,
    RPN_FNF_C_COTH      ,
    RPN_FNF_C_DIV       ,
    RPN_FNF_C_EXP       ,
    RPN_FNF_C_LN        ,
    RPN_FNF_C_MUL       ,
    RPN_FNF_C_POWER     ,
    RPN_FNF_C_RADIUS    ,
    RPN_FNF_C_SEC       ,
    RPN_FNF_C_SECH      ,
    RPN_FNF_C_SIN       ,
    RPN_FNF_C_SINH      ,
    RPN_FNF_C_SUB       ,
    RPN_FNF_C_TAN       ,
    RPN_FNF_C_TANH      ,
    RPN_FNF_C_THETA     ,

    RPN_FNV_CEILING     ,
    RPN_FNF_CHAR        ,
    RPN_FNV_CHOOSE      ,
    RPN_FNF_CODE        ,
    RPN_FNV_COL         ,
    RPN_FNV_COLS        ,
    RPN_FNF_COMBIN      ,
    RPN_FN0_CONTINUE    ,
    RPN_FNF_COS         ,
    RPN_FNF_COSEC       ,
    RPN_FNF_COSECH      ,
    RPN_FNF_COSH        ,
    RPN_FNF_COT         ,
    RPN_FNF_COTH        ,
    RPN_FNV_COUNT       ,
    RPN_FNV_COUNTA      ,
    RPN_FNF_CTERM       ,

    RPN_FNF_DATE        ,
    RPN_FNF_DATEVALUE   ,
    RPN_FNF_DAVG        ,
    RPN_FNF_DAY         ,
    RPN_FNF_DAYNAME     ,
    RPN_FNF_DCOUNT      ,
    RPN_FNF_DCOUNTA     ,
    RPN_FNF_DDB         ,
    RPN_FNF_DEG         ,
    RPN_FNF_DEREF       ,
    RPN_FNF_DMAX        ,
    RPN_FNF_DMIN        ,
    RPN_FNF_DSTD        ,
    RPN_FNF_DSTDP       ,
    RPN_FNF_DSUM        ,
    RPN_FNF_DVAR        ,
    RPN_FNF_DVARP       ,

    RPN_FN0_ELSE        ,
    RPN_FNF_ELSEIF      ,
    RPN_FN0_ENDIF       ,
    RPN_FN0_ENDWHILE    ,
    RPN_FNF_EXACT       ,
    RPN_FNF_EXP         ,

    RPN_FNF_FACT        ,
    RPN_FNV_FIND        ,
    RPN_FNF_FLIP        ,
    RPN_FNV_FLOOR       ,
    RPN_FNV_FOR         ,
    RPN_FNF_FORMULA_TEXT,
    RPN_FNM_FUNCTION    ,
    RPN_FNF_FV          ,

    RPN_FNF_GAMMALN     ,
    RPN_FNF_GOTO        ,
    RPN_FNV_GRAND       ,
    RPN_FNF_GROWTH      ,

    RPN_FNF_HLOOKUP     ,
    RPN_FNF_HOUR        ,

    RPN_FNV_IF          ,
    RPN_FNF_IFC         ,
    RPN_FNV_INDEX       ,
    RPN_FNV_INPUT       ,
    RPN_FNF_INT         ,
    RPN_FNF_IRR         ,

    RPN_FNV_JOIN        ,

    RPN_FNV_LEFT        ,
    RPN_FNF_LENGTH      ,
    RPN_FNV_LINEST      ,
    RPN_FNF_LISTCOUNT   ,
    RPN_FNF_LN          ,
    RPN_FNV_LOG         ,
    RPN_FNV_LOGEST      ,
    RPN_FNF_LOOKUP      ,
    RPN_FNF_LOWER       ,

    RPN_FNF_M_DETERM    ,
    RPN_FNF_M_INVERSE   ,
    RPN_FNF_M_MULT      ,

    RPN_FNF_MATCH       ,
    RPN_FNV_MAX         ,
    RPN_FNF_MEDIAN      ,
    RPN_FNF_MID         ,
    RPN_FNV_MIN         ,
    RPN_FNF_MINUTE      ,
    RPN_FNF_MIRR        ,
    RPN_FNF_MOD         ,
    RPN_FNF_MONTH       ,
    RPN_FNF_MONTHDAYS   ,
    RPN_FNF_MONTHNAME   ,

    RPN_FN0_NEXT        ,
    RPN_FN0_NOW         ,
    RPN_FNF_NPV         ,

    RPN_FNF_PERMUT      ,
    RPN_FN0_PI          ,
    RPN_FNF_PMT         ,
    RPN_FNF_PROPER      ,
    RPN_FNF_PV          ,

    RPN_FNF_RAD         ,
    RPN_FNV_RAND        ,
    RPN_FNV_RANK        ,
    RPN_FNF_RATE        ,
    RPN_FN0_REPEAT      ,
    RPN_FNF_REPLACE     ,
    RPN_FNF_REPT        ,
    RPN_FNF_RESULT      ,
    RPN_FNF_REVERSE     ,
    RPN_FNV_RIGHT       ,
    RPN_FNV_ROUND       ,
    RPN_FNV_ROW         ,
    RPN_FNV_ROWS        ,

    RPN_FNF_SEC         ,
    RPN_FNF_SECH        ,
    RPN_FNF_SECOND      ,
    RPN_FNF_SET_NAME    ,
    RPN_FNF_SET_VALUE   ,
    RPN_FNF_SGN         ,
    RPN_FNF_SIN         ,
    RPN_FNF_SINH        ,
    RPN_FNF_SLN         ,
    RPN_FNV_SORT        ,
    RPN_FNF_SPEARMAN    ,
    RPN_FNF_SQR         ,
    RPN_FNV_STD         ,
    RPN_FNV_STDP        ,
    RPN_FNV_STRING      ,
    RPN_FNV_SUM         ,
    RPN_FNF_SYD         ,

    RPN_FNF_TAN         ,
    RPN_FNF_TANH        ,
    RPN_FNF_TERM        ,
    RPN_FNF_TEXT        ,
    RPN_FNF_TIME        ,
    RPN_FNF_TIMEVALUE   ,
    RPN_FN0_TODAY       ,
    RPN_FNF_TRANSPOSE   ,
    RPN_FNF_TREND       ,
    RPN_FNF_TRIM        ,
    RPN_FNF_TYPE        ,

    RPN_FNF_UNTIL       ,
    RPN_FNF_UPPER       ,

    RPN_FNF_VALUE       ,
    RPN_FNV_VAR         ,
    RPN_FNV_VARP        ,
    RPN_FN0_VERSION     ,
    RPN_FNF_VLOOKUP     ,

    RPN_FNF_WEEKDAY     ,
    RPN_FNF_WEEKNUMBER,
    RPN_FNF_WHILE       ,

    RPN_FNF_YEAR        ,

    /* custom functions */
    RPN_FNM_CUSTOMCALL  ,

    /* arrays */
    RPN_FNA_MAKEARRAY   ,

#define ELEMOF_RPN_TABLE (RPN_FNA_MAKEARRAY + 1)

    /* special symbol constants - these symbols
     * don't appear in the final rpn string
    */

    SYM_BAD             ,
    SYM_BLANK           ,
    SYM_OBRACKET        ,
    SYM_CBRACKET        ,
    SYM_COMMA           ,
    SYM_OARRAY          ,
    SYM_CARRAY          ,
    SYM_SEMICOLON       ,
    SYM_TAG

};

/*
types of rpn atoms
*/

enum RPN_TYPES
{
    RPN_DAT     =0,         /* data */
    RPN_CON     ,           /* constant cells */
    RPN_TMP     ,           /* temporary data */
    RPN_RES     ,           /* result data */
    RPN_LCL     ,           /* local variables */
    RPN_EXT     ,           /* external cells */
    RPN_FRM     ,           /* formatting information */
    RPN_UOP     ,           /* unary operator */
    RPN_BOP     ,           /* binary operator */
    RPN_REL     ,           /* relational operator */
    RPN_FN0     ,           /* function, no arguments */
    RPN_FNF     ,           /* function, fixed number of args */
    RPN_FNV     ,           /* function, variable args */
    RPN_FNM     ,           /* function, custom call, variable args */
    RPN_FNA                 /* function, make array, variable args */
};

/*
type mask bits
*/

#define EM_REA      1
#define EM_SLR      2       /* if not set, SLRs are dereferenced first */
#define EM_STR      4
#define EM_DAT      8
#define EM_ARY      16      /* if not set, operator called for each element */
#define EM_BLK      32      /* if not set, blanks converted to real 0 */
#define EM_AR0      64      /* if set, element 0,0 of arrays returned */
#define EM_CDX      128     /* conditional subexpression */
#define EM_ERR      256
#define EM_INT      512     /* function wants integers */

#define EM_ANY      (EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT)
#define EM_CONST    (EM_REA          | EM_STR | EM_DAT          | EM_BLK | EM_ERR | EM_INT)

typedef S16 EV_TYPE; typedef EV_TYPE * P_EV_TYPE; typedef const EV_TYPE * PC_EV_TYPE;

/*
should really be in ss_const.h but need RPN_DAT_xxx
*/

static inline EV_IDNO
ev_integer_size(
    _InVal_     S32 integer)
{
    if( (integer <=   (S32) S8_MAX) &&
        (integer >=   (S32) S8_MIN) )
        return(RPN_DAT_WORD8);

    /* S16_MAX + 1 for ARM immediate constant */
    if( (integer <   ((S32) S16_MAX + 1)) &&
        (integer > - ((S32) S16_MAX + 1)) /*NB NOT S16_MIN */ )
        return(RPN_DAT_WORD16);

    return(RPN_DAT_WORD32);
}

static inline void
ev_data_init(
    _OutRef_    P_EV_DATA p_ev_data)
{
    zero_struct_ptr(p_ev_data);
    p_ev_data->did_num = RPN_DAT_BLANK;
}

static inline void
ev_data_set_blank(
    _OutRef_    P_EV_DATA p_ev_data)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ev_data)); /* keep dataflower happy */
    p_ev_data->did_num = RPN_DAT_BLANK;
}

static inline void
ev_data_set_boolean(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     BOOL boolean)
{
    p_ev_data->did_num = RPN_DAT_BOOL8;
    p_ev_data->arg.boolean = boolean;
}

static inline void
ev_data_set_integer(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 s32)
{
    p_ev_data->arg.integer = s32;
    p_ev_data->did_num = ev_integer_size(s32);
}

static inline void
ev_data_set_WORD32( /* set the widest integer type */
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 s32)
{
    p_ev_data->did_num = RPN_DAT_WORD32;
    p_ev_data->arg.integer = s32;
}

static inline void
ev_data_set_real(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 f64)
{
    p_ev_data->did_num = RPN_DAT_REAL;
    p_ev_data->arg.fp = f64;
}

static inline void
ev_data_set_real_ti(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 f64)
{
    p_ev_data->did_num = RPN_DAT_REAL;
    p_ev_data->arg.fp = f64;

    (void) real_to_integer_try(p_ev_data);
}

/*
evaluator external routines
*/

/*
ev_comp.c external functions
*/

extern S32
ev_compile(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_in,
    P_U8 rpn_out,
    S32 maxlen,
    P_S32 at_pos,
    P_EV_PARMS parmsp,
    _InRef_     PC_EV_OPTBLOCK p_optblock);

extern S32
ev_name_check(
    P_U8 name,
    _InVal_     EV_DOCNO docno);

extern S32
ss_recog_constant(
    P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z txt_in,
    _InRef_     PC_EV_OPTBLOCK p_optblock,
    S32 refs_ok);

/*
ev_dcom.c external functions
*/

extern S32
ev_decode_data(
    P_U8 txt_out,
    _InVal_     EV_DOCNO docno_from,
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_OPTBLOCK p_optblock);

extern S32
ev_decode_slot(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_out,
    P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_OPTBLOCK p_optblock);

extern S32
ev_decompile(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_out,
    PC_U8 rpn_in,
    _InRef_     PC_EV_OPTBLOCK p_optblock);

/*
ev_docs.c external routines
*/

extern void
ev_close_sheet(
    _InVal_     DOCNO docno);

_Check_return_
extern EV_DOCNO
ev_doc_establish_doc_from_docu_name(
    _InRef_     PC_DOCU_NAME p_docu_name);

extern S32
ev_doc_establish_doc_from_name(
    _In_z_      PC_USTR str,
    _InVal_     EV_DOCNO cur_docno,
    _OutRef_    P_EV_DOCNO docp);

extern S32
ev_doc_get_dep_docs_for_sheet(
    _InVal_     DOCNO docno,
    _OutRef_    P_EV_DOCNO p_docno,
    P_EV_DOCNO p_docno_array);

extern S32
ev_doc_get_sup_docs_for_sheet(
    _InVal_     DOCNO docno,
    _OutRef_    P_EV_DOCNO p_docno,
    P_EV_DOCNO p_docno_array);

extern S32
ev_doc_is_custom_sheet(
    _InVal_     DOCNO docno);

extern void
ev_rename_sheet(
    _InRef_     PC_DOCU_NAME p_new_docu_name,
    _InVal_     DOCNO docno);

extern S32
ev_write_docname(
    P_U8 str,
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from);

_Check_return_
extern EV_DOCNO
docno_find_name(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO docno_avoid,
    _InVal_     BOOL avoid_thunks);

_Check_return_
extern S32
name_compare(
    _InRef_     PC_DOCU_NAME p_docu_name_1,
    _InRef_     PC_DOCU_NAME p_docu_name_2);

_Check_return_
extern S32
name_dup(
    _OutRef_    P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in);

extern void
name_free(
    _InoutRef_  P_DOCU_NAME p_docu_name);

static inline void
name_init(
    _OutRef_    P_DOCU_NAME p_docu_name)
{
    p_docu_name->path_name = NULL;
    p_docu_name->leaf_name = NULL;
    p_docu_name->flags.path_name_supplied = 0;
}

extern void
name_make_wholename(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer);

/*
ev_eval.c
*/

extern EV_SERIAL ev_serial_num;

/*
ev_eval.c external functions
*/

extern S32
ev_eval_rpn(
    P_EV_DATA resultp,
    _InRef_     PC_EV_SLR p_ev_slr,
    PC_U8 rpn_in);

/*
ev_help.c external functions
*/

extern void
ev_data_to_result_convert(
    _OutRef_    P_EV_RESULT p_ev_result,
    _InRef_     PC_EV_DATA p_ev_data);

extern S32
ev_is_formula(
    P_EV_CELL p_ev_cell);

extern S32
ev_exp_copy(
    P_EV_CELL out_slotp,
    P_EV_CELL in_slotp);

extern void
ev_exp_free_resources(
    _InoutRef_  P_EV_CELL p_ev_cell);

extern S32
ev_proc_conditional_rpn(
    P_U8 rpn_out,
    P_U8 rpn_in,
    _InRef_     PC_EV_SLR p_ev_slr_target,
    BOOL abs_col,
    BOOL abs_row);

extern S32
ev_result_free_resources(
    _InoutRef_  P_EV_RESULT p_ev_result);

extern void
ev_result_to_data_convert(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_RESULT p_ev_result);

extern S32
ev_slr_deref(
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR p_ev_slr,
    S32 copy_ext);

extern void
ev_trace_slr(
    _Out_writes_z_(elemof_buffer) P_U8 string_out,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_U8 string_in,
    _InRef_     PC_EV_SLR p_ev_slr);

/*
link_ev.c external functions
*/

extern S32
ev_alert(
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_U8Z message,
    _In_z_      PC_U8Z but1_text,
    _In_z_      PC_U8Z but2_text);

extern void
ev_alert_close(void);

extern S32
ev_alert_poll(void);

_Check_return_
_Ret_maybenull_
extern P_SS_DOC
ev_create_new_ss_doc(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _OutRef_    P_EV_DOCNO p_ev_docno);

extern void
ev_destroy_ss_doc(
    _InVal_     EV_DOCNO ev_docno);

extern S32
ev_external_string(
    _InRef_     PC_EV_SLR p_ev_slr,
    P_U8 *outpp,
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer);

extern S32
ev_ext_uref(
    _InRef_     PC_EV_SLR p_ev_slr_by,
    S16 byoffset,
    _InRef_     PC_UREF_PARM upp);

extern S32
ev_input(
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_U8Z message,
    _In_z_      PC_U8Z but1_text,
    _In_z_      PC_U8Z but2_text);

extern void
ev_input_close(void);

extern S32
ev_input_poll(
    P_U8 result_string,
    S32 buf_size);

extern S32
ev_make_slot(
    _InRef_     PC_EV_SLR p_ev_slr,
    P_EV_RESULT p_ev_result);

_Check_return_
extern EV_COL
ev_numcol(
    _InRef_     PC_EV_SLR p_ev_slr);

_Check_return_
extern EV_ROW
ev_numrow(
    _InRef_     PC_EV_SLR p_ev_slr);

_Check_return_
_Ret_maybenone_
extern P_SS_DOC
ev_p_ss_doc_from_docno(
    _InVal_     EV_DOCNO docno);

_Check_return_
_Ret_notnone_
static inline P_SS_DOC
ev_p_ss_doc_from_docno_must(
    _InVal_     EV_DOCNO docno)
{
    P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);
    PTR_ASSERT(p_ss_doc);
    return(p_ss_doc);
}

extern void
ev_recalc(void);

extern void
ev_recalc_all(void);

extern void
ev_recalc_status(
    S32 to_calc);

extern void
ev_set_options(
    _OutRef_    P_EV_OPTBLOCK p_optblock,
    _InVal_     EV_DOCNO docno);

extern S32
ev_todo_check(void);

extern S32
ev_travel(
    _OutRef_    P_P_EV_CELL slotpp,
    _InRef_     PC_EV_SLR p_ev_slr);

extern DOCNO
docu_array_size;

enum NAME_SUCCESS
{
    NAME_OK = 0,
    NAME_MANY  ,
    NAME_NONE
};

/*
ev_name.c external functions
*/

extern void
ev_custom_del_hold(void);

extern void
ev_custom_del_release(void);

extern void
ev_name_del_hold(void);

extern void
ev_name_del_release(void);

_Check_return_
extern STATUS
ev_name_make(
    P_U8 name_text,
    _InVal_     EV_DOCNO docno_from,
    P_U8 name_def_text,
    _InRef_     PC_EV_OPTBLOCK p_optblock,
    S32 undefine);

/*
ev_rpn.c external functions
*/

extern S32
ev_len(
    P_EV_CELL p_ev_cell);

extern void
ev_rpn_adjust_refs(
    P_EV_CELL p_ev_cell,
    _InRef_     PC_UREF_PARM upp);

/*
ev_tabl.c
*/

extern void
ev_enum_resource_init(
    _OutRef_    P_EV_RESOURCE resop,
    _InVal_     enum EV_RESOURCE_TYPES category,
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from,
    _InRef_     PC_EV_OPTBLOCK p_optblock);

extern S32
ev_enum_resource_get(
    P_EV_RESOURCE resop,
    P_S32 item_no,
    P_U8 name_out,
    S32 name_buf_siz,
    P_U8 arg_out,
    S32 arg_buf_siz,
    P_S32 n_args);

/*
ev_tree.c external functions
*/

extern S32
ev_add_exp_slot_to_tree(
    P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr);

extern S32
ev_add_extdependency(
    U32 exthandle,
    S32 int_handle_in,
    P_S32 int_handle_out,
    ev_urefct proc,
    _InRef_     PC_EV_RANGE rngto);

extern S32
ev_add_slrdependency(
    P_EV_GRUB grubp);

extern void
ev_del_extdependency(
    _InVal_     EV_DOCNO docno,
    S32 handle_in);

extern void
ev_enum_dep_sup_init(
    _OutRef_    P_EV_DEPSUP dsp,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     enum EV_DEPSUP_TYPES category,
    _InVal_     S32 get_deps);

extern S32
ev_enum_dep_sup_get(
    P_EV_DEPSUP dsp,
    P_S32 item_no,
    P_EV_DATA p_ev_data);

extern void
ev_todo_add_doc_dependents(
    _InVal_     EV_DOCNO docno);

extern void
ev_todo_add_dependents(
    _InRef_     PC_EV_SLR p_ev_slr);

extern S32
ev_todo_add_name_dependents(
    EV_NAMEID nameid);

extern S32
ev_todo_add_slr(
    _InRef_     PC_EV_SLR p_ev_slr,
    S32 sort);

extern void
ev_tree_close(
    _InVal_     EV_DOCNO docno);

/*
ev_uref.c external functions
*/

extern S32
ev_match_rng(
    _InoutRef_  P_EV_RANGE p_ev_range,
    _InRef_     PC_UREF_PARM upp);

extern S32
ev_match_slr(
    _InoutRef_  P_EV_SLR ref,
    _InRef_     PC_UREF_PARM upp);

extern void
ev_uref(
    _InRef_     PC_UREF_PARM upp);

/*
outside evaluator
*/

#endif /* ERRDEF_EXPORT */

/*
error definition
*/

#define EVAL_ERRLIST_DEF \
    errorstring(EVAL_ERR_IRR,            "Irr") \
    errorstring(EVAL_ERR_BAD_LOG,        "Bad logarithm") \
    errorstring(EVAL_ERR_BAD_DATE,       "Bad date") \
    errorstring(EVAL_ERR_LOOKUP,         "Lookup") \
    errorstring(EVAL_ERR_NEG_ROOT,       "Negative root") \
    errorstring(EVAL_ERR_PROPAGATED,     "Propagated") \
    errorstring(EVAL_ERR_DIVIDEBY0,      "Divide by zero") \
    errorstring(EVAL_ERR_BAD_INDEX,      "Bad index") \
    errorstring(EVAL_ERR_FPERROR,        "FP error") \
    errorstring(EVAL_ERR_CIRC,           "Circular reference") \
    errorstring(EVAL_ERR_CANTEXTREF,     "File not loaded") \
    errorstring(EVAL_ERR_MANYEXTREF,     "More than 1 file loaded") \
    errorstring(EVAL_ERR_BADRPN,         "Bad compiled data") \
    errorstring(EVAL_ERR_EXPTOOLONG,     "Formula too long") \
    errorstring(EVAL_ERR_BADEXPR,        "Mistake in formula") \
    errorstring(EVAL_ERR_UNEXNUMBER,     "Number not expected") \
    errorstring(EVAL_ERR_UNEXSTRING,     "String not expected") \
    errorstring(EVAL_ERR_UNEXDATE,       "Date not expected") \
    errorstring(EVAL_ERR_NAMEUNDEF,      "Name not defined") \
    errorstring(EVAL_ERR_UNEXRANGE,      "Range not expected") \
    errorstring(EVAL_ERR_INTERNAL,       "Internal evaluator error") \
    errorstring(EVAL_ERR_CUSTOMUNDEF,    "Custom function not defined") \
    errorstring(EVAL_ERR_NESTEDARRAY,    "Nested array") \
    errorstring(EVAL_ERR_UNEXARRAY,      "Array not expected") \
    errorstring(EVAL_ERR_LOCALUNDEF,     "Parameter to custom function not declared") \
    errorstring(EVAL_ERR_NORETURN,       "No RESULT in custom function") \
    errorstring(EVAL_ERR_NA,             "Value not defined") \
    errorstring(EVAL_ERR_NOTIMPLEMENTED, "Not implemented") \
    errorstring(EVAL_ERR_SLR_BAD_REF,    "Cell deleted") \
    errorstring(EVAL_ERR_NOTIME,         "No time value") \
    errorstring(EVAL_ERR_NODATE,         "No date value") \
    errorstring(EVAL_ERR_BADIDENT,       "Bad identifier") \
    errorstring(EVAL_ERR_BADCOMPLEX,     "Bad complex number") \
    errorstring(EVAL_ERR_OUTOFRANGE,     "Out of range") \
    errorstring(EVAL_ERR_SUBSCRIPT,      "Array wrong size") \
    errorstring(EVAL_ERR_BADCONTROL,     "Control function outside custom function") \
    errorstring(EVAL_ERR_BADGOTO,        "Bad target for GOTO") \
    errorstring(EVAL_ERR_BADLOOPNEST,    "Wrongly nested loop") \
    errorstring(EVAL_ERR_ARGRANGE,       "Parameter out of range") \
    errorstring(EVAL_ERR_BADTIME,        "Bad time") \
    errorstring(EVAL_ERR_BADIFNEST,      "Wrongly nested IF ELSE ELSEIF ENDIF") \
    errorstring(EVAL_ERR_MISMATCHED_MATRICES, "Mismatched matrices") \
    errorstring(EVAL_ERR_MATRIX_NOT_NUMERIC,  "Matrix not numeric") \
    errorstring(EVAL_ERR_MATRIX_WRONG_SIZE,   "Matrix wrong size") \
    errorstring(EVAL_ERR_MIXED_SIGNS,         "Mixed signs") \
    errorstring(EVAL_ERR_ACCURACY_LOST,  "Accuracy lost") \
    errorstring(EVAL_ERR_UNEXFORMULA,    "Can't overwrite formula") \
    errorstring(EVAL_ERR_FUNARGS,        "Wrong number of parameters to function") \
    errorstring(EVAL_ERR_CBRACKETS,      "Missing )") \
    errorstring(EVAL_ERR_DBASENEST,      "Can't nest database functions") \
    errorstring(EVAL_ERR_ARGTYPE,        "Parameter is wrong type") \
    errorstring(EVAL_ERR_CUSTOMEXISTS,   "Custom function already defined") \
    errorstring(EVAL_ERR_ARGCUSTTYPE,    "Type not recognised") \
    errorstring(EVAL_ERR_ARRAYEXPAND,    "Array size is set by first row") \
    errorstring(EVAL_ERR_CARRAY,         "Missing }") \
    errorstring(EVAL_ERR_OWNCOLUMN,      "Set_value can't write above itself in same column") \
    errorstring(EVAL_ERR_MATRIX_NOT_SQUARE,   "Matrix is not square") \
    errorstring(EVAL_ERR_MATRIX_SINGULAR,     "Matrix is singular") \
    errorstring(EVAL_ERR_NO_VALID_DATA,       "No valid data found")

/*
error definition
*/

#define EVAL_ERR_BASE                (-2000)

#define EVAL_ERR_IRR                 (-2000)
#define EVAL_ERR_BAD_LOG             (-2001)
#define EVAL_ERR_BAD_DATE            (-2002)
#define EVAL_ERR_LOOKUP              (-2003)
#define EVAL_ERR_NEG_ROOT            (-2004)
#define EVAL_ERR_PROPAGATED          (-2005)
#define EVAL_ERR_DIVIDEBY0           (-2006)
#define EVAL_ERR_BAD_INDEX           (-2007)
#define EVAL_ERR_FPERROR             (-2008)
#define EVAL_ERR_CIRC                (-2009)
#define EVAL_ERR_CANTEXTREF          (-2010)
#define EVAL_ERR_MANYEXTREF          (-2011)
#define EVAL_ERR_BADRPN              (-2012)
#define EVAL_ERR_EXPTOOLONG          (-2013)
#define EVAL_ERR_spare_2014          (-2014)
#define EVAL_ERR_BADEXPR             (-2015)
#define EVAL_ERR_UNEXNUMBER          (-2016)
#define EVAL_ERR_UNEXSTRING          (-2017)
#define EVAL_ERR_UNEXDATE            (-2018)
#define EVAL_ERR_NAMEUNDEF           (-2019)
#define EVAL_ERR_UNEXRANGE           (-2020)
#define EVAL_ERR_INTERNAL            (-2021)
#define EVAL_ERR_CUSTOMUNDEF         (-2022)
#define EVAL_ERR_NESTEDARRAY         (-2023)
#define EVAL_ERR_UNEXARRAY           (-2024)
#define EVAL_ERR_LOCALUNDEF          (-2025)
#define EVAL_ERR_NORETURN            (-2026)
#define EVAL_ERR_NA                  (-2027)
#define EVAL_ERR_NOTIMPLEMENTED      (-2028)
#define EVAL_ERR_SLR_BAD_REF         (-2029)
#define EVAL_ERR_NOTIME              (-2030)
#define EVAL_ERR_NODATE              (-2031)
#define EVAL_ERR_BADIDENT            (-2032)
#define EVAL_ERR_BADCOMPLEX          (-2033)
#define EVAL_ERR_OUTOFRANGE          (-2034)
#define EVAL_ERR_SUBSCRIPT           (-2035)
#define EVAL_ERR_BADCONTROL          (-2036)
#define EVAL_ERR_BADGOTO             (-2037)
#define EVAL_ERR_BADLOOPNEST         (-2038)
#define EVAL_ERR_ARGRANGE            (-2039)
#define EVAL_ERR_BADTIME             (-2040)
#define EVAL_ERR_BADIFNEST           (-2041)
#define EVAL_ERR_MISMATCHED_MATRICES (-2042)
#define EVAL_ERR_MATRIX_NOT_NUMERIC  (-2043)
#define EVAL_ERR_MATRIX_WRONG_SIZE   (-2044)
#define EVAL_ERR_MIXED_SIGNS         (-2045)
#define EVAL_ERR_ACCURACY_LOST       (-2046)
#define EVAL_ERR_UNEXFORMULA         (-2047)
#define EVAL_ERR_FUNARGS             (-2048)
#define EVAL_ERR_CBRACKETS           (-2049)
#define EVAL_ERR_DBASENEST           (-2050)
#define EVAL_ERR_ARGTYPE             (-2051)
#define EVAL_ERR_CUSTOMEXISTS        (-2052)
#define EVAL_ERR_ARGCUSTTYPE         (-2053)
#define EVAL_ERR_ARRAYEXPAND         (-2054)
#define EVAL_ERR_CARRAY              (-2055)
#define EVAL_ERR_OWNCOLUMN           (-2056)
#define EVAL_ERR_MATRIX_NOT_SQUARE   (-2057)
#define EVAL_ERR_MATRIX_SINGULAR     (-2058)
#define EVAL_ERR_NO_VALID_DATA       (-2059)

#define EVAL_ERR_END                 (-2060)

#endif /* __ev_eval_h */

/* end of ev_eval.h */
