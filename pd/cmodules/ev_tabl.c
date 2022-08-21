/* ev_tabl.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Tables of data for evaluator */

/* MRJC February 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "ev_eval.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

/******************************************************************************
*
* table of permissible argument types
*
******************************************************************************/

/* generic argument lists */

static const EV_TYPE arg_ARY[]  = { 1, EM_ARY };

static const EV_TYPE arg_DAT[]  = { 1, EM_DAT };

static const EV_TYPE arg_INT[]  = { 1, EM_INT };

static const EV_TYPE arg_REA[]  = { 1, EM_REA };

static const EV_TYPE arg_BOO[]  = { 1, EM_LOGICAL };

static const EV_TYPE arg_SLR[]  = { 1, EM_SLR };

static const EV_TYPE arg_STR[]  = { 1, EM_STR };

/* IoR: integer or real */
static const EV_TYPE arg_IoR[]  = { 1, EM_INT | EM_REA };

/* IoRoD: integer, real or date */
static const EV_TYPE arg_IoRoD[] = { 1, EM_REA | EM_DAT | EM_INT };

/* D_I: date and integer */
static const EV_TYPE arg_D_I[]  = { 2, EM_DAT,
                                       EM_INT };

/* S_I: string and integer */
static const EV_TYPE arg_S_I[]  = { 2, EM_STR,
                                       EM_INT };

/* function-specific argument lists */

static const EV_TYPE arg_bse[]  = { 2, EM_REA | EM_INT,
                                       EM_INT };

static const EV_TYPE arg_cho[]  = { 2, EM_INT,
                                       EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY };

#if defined(COMPLEX_STRING)
static const EV_TYPE arg_CPX[]  = { 1, EM_REA | EM_STR | EM_ARY };
static const EV_TYPE arg_C_I[]  = { 2, EM_REA | EM_STR | EM_ARY,
                                       EM_INT };
#else
static const EV_TYPE arg_CPX[]  = { 1, EM_REA | EM_ARY };
static const EV_TYPE arg_C_I[]  = { 2, EM_REA | EM_ARY,
                                       EM_INT };
#endif

static const EV_TYPE arg_cvr[]  = { 2, EM_REA | EM_STR,
                                       EM_INT };

static const EV_TYPE arg_dan[]  = { 2, EM_INT | EM_DAT,
                                       EM_INT };

static const EV_TYPE arg_dbs[]  = { 2, EM_ARY,
                                       EM_CDX };

static const EV_TYPE arg_dmn[]  = { 1, EM_DAT | EM_INT };

static const EV_TYPE arg_drf[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_BLK | EM_ERR | EM_INT };

static const EV_TYPE arg_for[]  = { 4, EM_STR,
                                       EM_SLR | EM_REA,
                                       EM_SLR | EM_REA,
                                       EM_REA };

static const EV_TYPE arg_fnd[]  = { 3, EM_STR,
                                       EM_STR,
                                       EM_INT };

static const EV_TYPE arg_hvl[]  = { 3, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT,
                                       EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_idx[]  = { 2, EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_if[]   = { 2, EM_INT,
                                       EM_ANY };

static const EV_TYPE arg_llg[]  = { 4, EM_ARY,
                                       EM_ARY,
                                       EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_lkp[]  = { 2, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT,
                                       EM_ARY };

static const EV_TYPE arg_mir[]  = { 2, EM_ARY,
                                       EM_REA };

static const EV_TYPE arg_mix[]  = { 1, EM_REA | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_INT };

static const EV_TYPE arg_nls[]  = { 1, EM_REA | EM_ARY };

static const EV_TYPE arg_ndp[]  = { 2, EM_REA | EM_INT, /* SKS 23may14 added EM_INT for rounding precision where simple enough (decimal_places >= 0) */
                                       EM_INT };

static const EV_TYPE arg_npv[]  = { 2, EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_rco[]  = { 1, EM_SLR | EM_ARY };

static const EV_TYPE arg_rel[]  = { 1, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT };

static const EV_TYPE arg_res[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_ERR | EM_INT};

static const EV_TYPE arg_rnk[]  = { 2, EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_rpl[]  = { 4, EM_STR,
                                       EM_INT,
                                       EM_INT,
                                       EM_STR };

static const EV_TYPE arg_setn[] = { 2, EM_STR,
                                       EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_INT };

static const EV_TYPE arg_setv[] = { 2, EM_SLR | EM_ARY,
                                       EM_REA | EM_STR | EM_DAT | EM_ARY | EM_INT };

static const EV_TYPE arg_ssm[]  = { 4, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_trd[]  = { 2, EM_ARY,
                                       EM_ARY | EM_REA };

static const EV_TYPE arg_txt[]  = { 2, EM_REA | EM_STR | EM_DAT,
                                       EM_STR };

static const EV_TYPE arg_typ[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT};

/******************************************************************************
*
* table of operators and functions
*
******************************************************************************/

#define NAI 0              /* na integer */
#define NAP FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0)    /* na parms */
#define NAS NULL           /* na semantic address */
#define NAA NULL           /* na argument flags address */
#define NAT 0              /* na type */

#define EXCTRL(a, b) \
    FP_AGG(EXEC_CONTROL, (a), (b), 0, 0, 0)

/*    [type]
               [#args]
                    [category]
                                              [func bits]
                                                   [semantic addr]
                                                                     [argument types] */

const RPNDEF rpn_table[] =
{
    /* data */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* real */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* S8 */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* S16 */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* S32 */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* slr */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* range */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* date */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* name */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* blank */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* error */

    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* rpn string       */ /* handle based resource owned by RPN */
    { RPN_TMP, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* temporary string */ /* temporary handle based resource */
    { RPN_RES, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* result string    */ /* handle based resource owned by a result */

    { RPN_TMP, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* temporary array  */ /* temporary handle based resource */
    { RPN_RES, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* result array     */ /* handle based resource owned by a result */

    /* local argument */
    { RPN_LCL, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* argument name */

    /* format information */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* brackets */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* space */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* return */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* end of expression */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* conditional expression */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* skip rpn if false */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* skip rpn if true */
    { RPN_FRM, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* no dependency */

    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_uop_not,        arg_BOO },
    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_uop_minus,      arg_IoR }, /* unary - */
    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_uop_plus,       arg_IoR }, /* unary + */

    /* binary operators */
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_and,        arg_BOO },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_div,        arg_IoR },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_sub,        arg_IoRoD },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_or,         arg_BOO },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_add,        arg_IoRoD },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_power,      arg_REA },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_bop_mul,        arg_IoR },

    /* binary relational operators */
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_eq,         arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_gt,         arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_gteq,       arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_lt,         arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_lteq,       arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_rel_neq,        arg_rel },

    /* functions */
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_abs,            arg_IoR },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosec,         arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosech,        arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosh,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acot,           arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acoth,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acos,           arg_REA },
    { RPN_FNF,   2, EV_RESO_DATE    ,         NAP, c_age,            arg_DAT },
    { RPN_FNV,  -3, EV_RESO_MISC    , FP_AGG(EXEC_ALERT, 0, 0, 0, 0, 0),
                                                   /*alert*/ NAS,    arg_STR },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asec,           arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asech,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asinh,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asin,           arg_REA },
    { RPN_FNF,   2, EV_RESO_TRIG    ,         NAP, c_atan_2,         arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_atanh,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_atan,           arg_REA },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_avg,            arg_mix },

    { RPN_FNV,  -3, EV_RESO_MATHS   ,         NAP, c_base,           arg_bse },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_beta,           arg_REA },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_bin,            arg_ARY },
    { RPN_FNV,  -1, EV_RESO_CONTROL , EXCTRL(CONTROL_BREAK, 0),
                                                   /*break*/ NAS,    arg_INT },

    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acos,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acosec,       arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acosech,      arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acosh,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acot,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_acoth,        arg_CPX },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_c_add,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_asec,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_asech,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_asin,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_asinh,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_atan,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_atanh,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_cos,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_cosec,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_cosech,       arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_cosh,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_cot,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_coth,         arg_CPX },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_c_div,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_exp,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_ln,           arg_CPX },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_c_mul,          arg_CPX },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_c_power,        arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_radius,       arg_CPX },
    { RPN_FNV,  -2, EV_RESO_COMPLEX ,         NAP, c_c_round,        arg_C_I },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_sec,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_sech,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_sin,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_sinh,         arg_CPX },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_c_sub,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_tan,          arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_tanh,         arg_CPX },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_c_theta,        arg_CPX },

    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_ceiling,        arg_REA },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_char,           arg_INT },
    { RPN_FNV,  -2, EV_RESO_LOOKUP  ,         NAP, c_choose,         arg_cho },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_code,           arg_STR },
    { RPN_FNV,  -1, EV_RESO_LOOKUP  , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 1/*nodep*/),
                                                   c_col,            arg_rco },
    { RPN_FNV,  -1, EV_RESO_LOOKUP  , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 1/*nodep*/),
                                                   c_cols,           arg_ARY },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_combin,         arg_INT },
    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_CONTINUE, 0),
                                                   /*continue*/ NAS, NAA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cos,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosec,          arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosech,         arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosh,           arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cot,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_coth,           arg_REA },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_count,          arg_mix },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_counta,         arg_mix },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_cterm,          arg_REA },

    { RPN_FNF,   3, EV_RESO_DATE    ,         NAP, c_date,           arg_INT },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_datevalue,      arg_STR },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DAVG,    0, 1/*dbase*/, 0, 0),
                                                   /*dvag*/ NAS,     arg_dbs },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_day,            arg_DAT },
    { RPN_FNV,  -2, EV_RESO_DATE    ,         NAP, c_dayname,        arg_dan },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DCOUNT,  0, 1/*dbase*/, 0, 0),
                                                   /*dcount*/ NAS,   arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DCOUNTA, 0, 1/*dbase*/, 0, 0),
                                                   /*dcounta*/ NAS,  arg_dbs },
    { RPN_FNV,  -5, EV_RESO_FINANCE ,         NAP, c_ddb,            arg_REA },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_decimal,        arg_cvr },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_deg,            arg_REA },
    { RPN_FNF,   1, EV_RESO_MISC    ,         NAP, c_deref,          arg_drf },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DMAX,    0, 1/*dbase*/, 0, 0),
                                                   /*dmax*/ NAS,     arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DMIN,    0, 1/*dbase*/, 0, 0),
                                                   /*dmin*/ NAS,     arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DSTD,    0, 1/*dbase*/, 0, 0),
                                                   /*dstd*/ NAS,     arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DSTDP,   0, 1/*dbase*/, 0, 0),
                                                   /*dstdp*/ NAS,    arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DSUM,    0, 1/*dbase*/, 0, 0),
                                                   /*dsum*/ NAS,     arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DVAR,    0, 1/*dbase*/, 0, 0),
                                                   /*dvar*/ NAS,     arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, FP_AGG(EXEC_DBASE, DBASE_DVARP,   0, 1/*dbase*/, 0, 0),
                                                   /*dvarp*/ NAS,    arg_dbs },

    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_ELSE, EVS_CNT_ELSE),
                                                   /*else*/ NAS,     NAA },
    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_ELSEIF, EVS_CNT_ELSEIF),
                                                   /*elseif*/ NAS,   arg_BOO },
    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_ENDIF, EVS_CNT_ENDIF),
                                                   /*endif*/ NAS,    NAA },
    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_ENDWHILE, EVS_CNT_ENDWHILE),
                                                   /*endwhile*/ NAS, NAA },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_exact,          arg_STR },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_exp,            arg_REA },

    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_fact,           arg_INT },
    { RPN_FNV,  -3, EV_RESO_STRING  ,         NAP, c_find,           arg_fnd },
    { RPN_FNF,   1, EV_RESO_MISC,             NAP, c_flip,           arg_ARY },
    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_floor,          arg_REA },
    { RPN_FNV,  -4, EV_RESO_CONTROL , EXCTRL(CONTROL_FOR, EVS_CNT_FOR),
                                                   /*for*/ NAS,      arg_for },
    { RPN_FNF,   1, EV_RESO_STRING,           NAP, c_formula_text,   arg_SLR },
    { RPN_FNM,  -1, EV_RESO_CONTROL ,         NAP, /*function*/ NAS, NAA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_fv,             arg_REA },

    { RPN_FNF,   1, EV_RESO_STATS   ,         NAP, c_gammaln,        arg_REA },
    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_GOTO, 0),
                                                   /*goto*/ NAS,     arg_SLR },
    { RPN_FNV,  -1, EV_RESO_STATS   , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 0),
                                                   c_grand,          arg_REA },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_growth,         arg_trd },

    { RPN_FNF,   3, EV_RESO_LOOKUP  , FP_AGG(EXEC_LOOKUP, LOOKUP_HLOOKUP, 0, 0, 0, 0),
                                                   /*hlookup*/ NAS,  arg_hvl },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_hour,           arg_DAT },

    { RPN_FNV,  -3, EV_RESO_MISC    ,         NAP, c_if,             arg_if },
    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_IF, EVS_CNT_IFC),
                                                   /*if*/ NAS,       arg_BOO },
    { RPN_FNV,  -4, EV_RESO_LOOKUP  ,         NAP, c_index,          arg_idx },
    { RPN_FNV,  -4, EV_RESO_MISC    , FP_AGG(EXEC_ALERT, 0, 0, 0, 0, 0),
                                                   /*input*/ NAS,    arg_STR },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_int,            arg_IoR },
    { RPN_FNF,   2, EV_RESO_FINANCE ,         NAP, c_irr,            arg_npv },

    { RPN_FNV,  -1, EV_RESO_STRING  ,         NAP, c_join,           arg_STR },

    { RPN_FNV,  -2, EV_RESO_STRING  ,         NAP, c_left,           arg_S_I },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_length,         arg_STR },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_linest,         arg_llg },
    { RPN_FNF,   1, EV_RESO_STATS,            NAP, c_listcount,      arg_ARY },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_ln,             arg_REA },
    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_log,            arg_REA },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_logest,         arg_llg },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , FP_AGG(EXEC_LOOKUP, LOOKUP_LOOKUP, 0, 0, 0, 0),
                                                   /*lookup*/ NAS,   arg_lkp },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_lower,          arg_STR },

    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_m_determ,       arg_ARY },
    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_m_inverse,      arg_ARY },
    { RPN_FNF,   2, EV_RESO_MATRIX  ,         NAP, c_m_mult,         arg_ARY },

    { RPN_FNF,   3, EV_RESO_LOOKUP  , FP_AGG(EXEC_LOOKUP, LOOKUP_MATCH, 0, 0, 0, 0),
                                                   /*match*/ NAS,    arg_hvl },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_max,            arg_mix },
    { RPN_FNF,   1, EV_RESO_STATS,            NAP, c_median,         arg_ARY },
    { RPN_FNF,   3, EV_RESO_STRING  ,         NAP, c_mid,            arg_S_I },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_min,            arg_mix },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_minute,         arg_DAT },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_mirr,           arg_mir },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_mod,            arg_IoR },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_month,          arg_DAT },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_monthdays,      arg_DAT },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_monthname,      arg_dmn },

    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_NEXT, EVS_CNT_NEXT),
                                                   /*next*/ NAS,     NAA },
    { RPN_FN0,   0, EV_RESO_DATE    ,         NAP, c_now,            NAA },
    { RPN_FNF,   2, EV_RESO_FINANCE ,         NAP, c_npv,            arg_npv },

    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_permut,         arg_INT },
    { RPN_FN0,   0, EV_RESO_TRIG    ,         NAP, c_pi,             NAA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_pmt,            arg_REA },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_proper,         arg_STR },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_pv,             arg_REA },

  /*{ RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_quotient,       arg_REA },*/

    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_rad,            arg_REA },
    { RPN_FNV,  -1, EV_RESO_STATS   , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 0),
                                                   c_rand,           arg_REA },
    { RPN_FNV,  -2, EV_RESO_STATS,            NAP, c_rank,           arg_rnk },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_rate,           arg_REA },
    { RPN_FN0,   0, EV_RESO_CONTROL , EXCTRL(CONTROL_REPEAT, EVS_CNT_REPEAT),
                                                   /*repeat*/ NAS,   NAA },
    { RPN_FNF,   4, EV_RESO_STRING  ,         NAP, c_replace,        arg_rpl },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_rept,           arg_S_I },
    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_RESULT, 0),
                                                   /*result*/ NAS,   arg_res },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_reverse,        arg_STR },
    { RPN_FNV,  -2, EV_RESO_STRING  ,         NAP, c_right,          arg_S_I },
    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_round,          arg_ndp },
    { RPN_FNV,  -1, EV_RESO_LOOKUP  , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 1/*nodep*/),
                                                   c_row,            arg_rco },
    { RPN_FNV,  -1, EV_RESO_LOOKUP  , FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*var*/, 1/*nodep*/),
                                                   c_rows,           arg_ARY },

    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sec,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sech,           arg_REA },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_second,         arg_DAT },
    { RPN_FNF,   4, EV_RESO_MATHS   ,         NAP, c_seriessum,      arg_ssm },
    { RPN_FNF,   2, EV_RESO_MISC    ,         NAP, c_set_name,       arg_setn },
    { RPN_FNF,   2, EV_RESO_MISC    , FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 1/*nodep*/),
                                                   c_setvalue,       arg_setv },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_sgn,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sin,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sinh,           arg_REA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_sln,            arg_REA },
    { RPN_FNV,  -2, EV_RESO_MISC,             NAP, c_sort,           arg_idx },
    { RPN_FNF,   2, EV_RESO_STATS,            NAP, c_spearman,       arg_ARY },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_sqr,            arg_REA },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_std,            arg_nls },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_stdp,           arg_nls },
    { RPN_FNV,  -2, EV_RESO_STRING  ,         NAP, c_string,         arg_ndp },
    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_sum,            arg_mix },
    { RPN_FNF,   4, EV_RESO_FINANCE ,         NAP, c_syd,            arg_REA },

    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_tan,            arg_REA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_tanh,           arg_REA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_term,           arg_REA },
    { RPN_FNF,   2, EV_RESO_STRING,           NAP, c_text,           arg_txt },
    { RPN_FNF,   3, EV_RESO_DATE    ,         NAP, c_time,           arg_INT },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_timevalue,      arg_STR },
    { RPN_FN0,   0, EV_RESO_DATE    ,         NAP, c_today,          NAA },
    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_transpose,      arg_ARY },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_trend,          arg_trd },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_trim,           arg_STR },
    { RPN_FNF,   1, EV_RESO_MISC    ,         NAP, c_type,           arg_typ },

    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_UNTIL, EVS_CNT_UNTIL),
                                                   /*until*/ NAS,    arg_BOO },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_upper,          arg_STR },

    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_value,          arg_STR },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_var,            arg_nls },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_varp,           arg_nls },
    { RPN_FN0,   0, EV_RESO_MISC    ,         NAP, c_version,        NAA },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , FP_AGG(EXEC_LOOKUP, LOOKUP_VLOOKUP, 0, 0, 0, 0),
                                                   /*vlookup*/ NAS,  arg_hvl },

    { RPN_FNV,  -2, EV_RESO_DATE    ,         NAP, c_weekday,        arg_D_I },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_weeknumber,     arg_DAT },
    { RPN_FNF,   1, EV_RESO_CONTROL , EXCTRL(CONTROL_WHILE, EVS_CNT_WHILE),
                                                   /*while*/ NAS,    arg_BOO },

    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_year,           arg_DAT },

    { RPN_FNM,  -1, EV_RESO_NOTME   ,         NAP, NAS,              NAA },

    { RPN_FNA,  -1, EV_RESO_NOTME   ,         NAP, NAS,              NAA }
};

/*
lowercase ASCII sorted compiler function lookup
*/

typedef struct LOOKDEF
{
    PC_USTR id;
    EV_IDNO did_num;
}
LOOKDEF; typedef const LOOKDEF * PC_LOOKDEF;

static const LOOKDEF
look_table[] =
{
    { "+",          RPN_UOP_UPLUS       },
    { "-",          RPN_UOP_UMINUS      },
    { "!",          RPN_UOP_NOT         },

    { "&",          RPN_BOP_AND         },
    { "*",          RPN_BOP_TIMES       },
    { "+",          RPN_BOP_PLUS        },
    { "-",          RPN_BOP_MINUS       },
    { "/",          RPN_BOP_DIVIDE      },

    { "<",          RPN_REL_LT          },
    { "<=",         RPN_REL_LTEQUAL     },
    { "<>",         RPN_REL_NOTEQUAL    },
    { "=",          RPN_REL_EQUALS      },
    { ">",          RPN_REL_GT          },
    { ">=",         RPN_REL_GTEQUAL     },

    { "^",          RPN_BOP_POWER       },

    { "abs",        RPN_FNF_ABS         },
    { "acosec",     RPN_FNF_ACOSEC      },
    { "acosech",    RPN_FNF_ACOSECH     },
    { "acosh",      RPN_FNF_ACOSH       },
    { "acot",       RPN_FNF_ACOT        },
    { "acoth",      RPN_FNF_ACOTH       },
    { "acs",        RPN_FNF_ACS         }, /* NB NOT acos! */
    { "age",        RPN_FNF_AGE         },
    { "alert",      RPN_FNV_ALERT       },
    { "asec",       RPN_FNF_ASEC        },
    { "asech",      RPN_FNF_ASECH       },
    { "asinh",      RPN_FNF_ASINH       },
    { "asn",        RPN_FNF_ASN         }, /* NB NOT asin! */
    { "atan_2",     RPN_FNF_ATAN_2      },
    { "atanh",      RPN_FNF_ATANH       },
    { "atn",        RPN_FNF_ATN         },
    { "avg",        RPN_FNV_AVG         },

    { "base",       RPN_FNV_BASE        },
    { "beta",       RPN_FNF_BETA        },
    { "bin",        RPN_FNF_BIN         },
    { "break",      RPN_FNV_BREAK       },

    { "c_acos",     RPN_FNF_C_ACOS      },
    { "c_acosec",   RPN_FNF_C_ACOSEC    },
    { "c_acosech",  RPN_FNF_C_ACOSECH   },
    { "c_acosh",    RPN_FNF_C_ACOSH     },
    { "c_acot",     RPN_FNF_C_ACOT      },
    { "c_acoth",    RPN_FNF_C_ACOTH     },
    { "c_add",      RPN_FNF_C_ADD       },
    { "c_asec",     RPN_FNF_C_ASEC      },
    { "c_asech",    RPN_FNF_C_ASECH     },
    { "c_asin",     RPN_FNF_C_ASIN      },
    { "c_asinh",    RPN_FNF_C_ASINH     },
    { "c_atan",     RPN_FNF_C_ATAN      },
    { "c_atanh",    RPN_FNF_C_ATANH     },
    { "c_cos",      RPN_FNF_C_COS       },
    { "c_cosec",    RPN_FNF_C_COSEC     },
    { "c_cosech",   RPN_FNF_C_COSECH    },
    { "c_cosh",     RPN_FNF_C_COSH      },
    { "c_cot",      RPN_FNF_C_COT       },
    { "c_coth",     RPN_FNF_C_COTH      },
    { "c_div",      RPN_FNF_C_DIV       },
    { "c_exp",      RPN_FNF_C_EXP       },
    { "c_ln",       RPN_FNF_C_LN        },
    { "c_mul",      RPN_FNF_C_MUL       },
    { "c_power",    RPN_FNF_C_POWER     },
    { "c_radius",   RPN_FNF_C_RADIUS    },
    { "c_round",    RPN_FNV_C_ROUND     },
    { "c_sec",      RPN_FNF_C_SEC       },
    { "c_sech",     RPN_FNF_C_SECH      },
    { "c_sin",      RPN_FNF_C_SIN       },
    { "c_sinh",     RPN_FNF_C_SINH      },
    { "c_sub",      RPN_FNF_C_SUB       },
    { "c_tan",      RPN_FNF_C_TAN       },
    { "c_tanh",     RPN_FNF_C_TANH      },
    { "c_theta",    RPN_FNF_C_THETA     },

    { "ceiling",    RPN_FNV_CEILING     },
    { "char",       RPN_FNF_CHAR        },
    { "choose",     RPN_FNV_CHOOSE      },
    { "code",       RPN_FNF_CODE        },
    { "col",        RPN_FNV_COL         },
    { "cols",       RPN_FNV_COLS        },
    { "combin",     RPN_FNF_COMBIN      },
    { "continue",   RPN_FN0_CONTINUE    },
    { "cos",        RPN_FNF_COS         },
    { "cosec",      RPN_FNF_COSEC       },
    { "cosech",     RPN_FNF_COSECH      },
    { "cosh",       RPN_FNF_COSH        },
    { "cot",        RPN_FNF_COT         },
    { "coth",       RPN_FNF_COTH        },
    { "count",      RPN_FNV_COUNT       },
    { "counta",     RPN_FNV_COUNTA      },
    { "cterm",      RPN_FNF_CTERM       },

    { "date",       RPN_FNF_DATE        },
    { "datevalue",  RPN_FNF_DATEVALUE   },
    { "davg",       RPN_FNF_DAVG        },
    { "day",        RPN_FNF_DAY         },
    { "dayname",    RPN_FNF_DAYNAME     },
    { "dcount",     RPN_FNF_DCOUNT      },
    { "dcounta",    RPN_FNF_DCOUNTA     },
    { "ddb",        RPN_FNV_DDB         },
    { "decimal",    RPN_FNF_DECIMAL     },
    { "deg",        RPN_FNF_DEG         },
    { "deref",      RPN_FNF_DEREF       },
    { "dmax",       RPN_FNF_DMAX        },
    { "dmin",       RPN_FNF_DMIN        },
    { "dstd",       RPN_FNF_DSTD        },
    { "dstdp",      RPN_FNF_DSTDP       },
    { "dsum",       RPN_FNF_DSUM        },
    { "dvar",       RPN_FNF_DVAR        },
    { "dvarp",      RPN_FNF_DVARP       },

    { "else",       RPN_FN0_ELSE        },
    { "elseif",     RPN_FNF_ELSEIF      },
    { "endif",      RPN_FN0_ENDIF       },
    { "endwhile",   RPN_FN0_ENDWHILE    },
    { "exact",      RPN_FNF_EXACT       },
    { "exp",        RPN_FNF_EXP         },

    { "fact",       RPN_FNF_FACT        },
    { "find",       RPN_FNV_FIND        },
    { "flip",       RPN_FNF_FLIP        },
    { "floor",      RPN_FNV_FLOOR       },
    { "for",        RPN_FNV_FOR         },
    { "formula_text",   RPN_FNF_FORMULA_TEXT },
    { "function",   RPN_FNM_FUNCTION    },
    { "fv",         RPN_FNF_FV          },

    { "gammaln",    RPN_FNF_GAMMALN     },
    { "goto",       RPN_FNF_GOTO        },
    { "grand",      RPN_FNV_GRAND       },
    { "growth",     RPN_FNF_GROWTH      },

    { "hlookup",    RPN_FNF_HLOOKUP     },
    { "hour",       RPN_FNF_HOUR        },

    { "if",         RPN_FNV_IF          },
    { "index",      RPN_FNV_INDEX       },
    { "input",      RPN_FNV_INPUT       },
    { "int",        RPN_FNF_INT         },
    { "irr",        RPN_FNF_IRR         },

    { "join",       RPN_FNV_JOIN        },

    { "left",       RPN_FNV_LEFT        },
    { "length",     RPN_FNF_LENGTH      },
    { "linest",     RPN_FNV_LINEST      },
    { "listcount",  RPN_FNF_LISTCOUNT   },
    { "ln",         RPN_FNF_LN          },
    { "log",        RPN_FNV_LOG         },
    { "logest",     RPN_FNV_LOGEST      },
    { "lookup",     RPN_FNF_LOOKUP      },
    { "lower",      RPN_FNF_LOWER       },

    { "m_determ",   RPN_FNF_M_DETERM    },
    { "m_inverse",  RPN_FNF_M_INVERSE   },
    { "m_mult",     RPN_FNF_M_MULT      },

    { "match",      RPN_FNF_MATCH       },
    { "max",        RPN_FNV_MAX         },
    { "median",     RPN_FNF_MEDIAN      },
    { "mid",        RPN_FNF_MID         },
    { "min",        RPN_FNV_MIN         },
    { "minute",     RPN_FNF_MINUTE      },
    { "mirr",       RPN_FNF_MIRR        },
    { "mod",        RPN_FNF_MOD         },
    { "month",      RPN_FNF_MONTH       },
    { "monthdays",  RPN_FNF_MONTHDAYS   },
    { "monthname",  RPN_FNF_MONTHNAME   },

    { "next",       RPN_FN0_NEXT        },
    { "now",        RPN_FN0_NOW         },
    { "npv",        RPN_FNF_NPV         },

    { "permut",     RPN_FNF_PERMUT      },
    { "pi",         RPN_FN0_PI          },
    { "pmt",        RPN_FNF_PMT         },
    { "proper",     RPN_FNF_PROPER      },
    { "pv",         RPN_FNF_PV          },

  /*{ "quotient",   RPN_FNF_QUOTIENT    },*/

    { "rad",        RPN_FNF_RAD         },
    { "rand",       RPN_FNV_RAND        },
    { "rank",       RPN_FNV_RANK        },
    { "rate",       RPN_FNF_RATE        },
    { "repeat",     RPN_FN0_REPEAT      },
    { "replace",    RPN_FNF_REPLACE     },
    { "rept",       RPN_FNF_REPT        },
    { "result",     RPN_FNF_RESULT      },
    { "reverse",    RPN_FNF_REVERSE     },
    { "right",      RPN_FNV_RIGHT       },
    { "round",      RPN_FNV_ROUND       },
    { "row",        RPN_FNV_ROW         },
    { "rows",       RPN_FNV_ROWS        },

    { "sec",        RPN_FNF_SEC         },
    { "sech",       RPN_FNF_SECH        },
    { "second",     RPN_FNF_SECOND      },
    { "seriessum",  RPN_FNF_SERIESSUM   },
    { "set_name",   RPN_FNF_SET_NAME    },
    { "set_value",  RPN_FNF_SET_VALUE   },
    { "sgn",        RPN_FNF_SGN         },
    { "sin",        RPN_FNF_SIN         },
    { "sinh",       RPN_FNF_SINH        },
    { "sln",        RPN_FNF_SLN         },
    { "sort",       RPN_FNV_SORT        },
    { "spearman",   RPN_FNF_SPEARMAN    },
    { "sqr",        RPN_FNF_SQR         },
    { "std",        RPN_FNV_STD         },
    { "stdp",       RPN_FNV_STDP        },
    { "string",     RPN_FNV_STRING      },
    { "sum",        RPN_FNV_SUM         },
    { "syd",        RPN_FNF_SYD         },

    { "tan",        RPN_FNF_TAN         },
    { "tanh",       RPN_FNF_TANH        },
    { "term",       RPN_FNF_TERM        },
    { "text",       RPN_FNF_TEXT        },
    { "time",       RPN_FNF_TIME        },
    { "timevalue",  RPN_FNF_TIMEVALUE   },
    { "today",      RPN_FN0_TODAY       },
    { "transpose",  RPN_FNF_TRANSPOSE   },
    { "trend",      RPN_FNF_TREND       },
    { "trim",       RPN_FNF_TRIM        },
    { "type",       RPN_FNF_TYPE        },

    { "until",      RPN_FNF_UNTIL       },
    { "upper",      RPN_FNF_UPPER       },

    { "value",      RPN_FNF_VALUE       },
    { "var",        RPN_FNV_VAR         },
    { "varp",       RPN_FNV_VARP        },
    { "version",    RPN_FN0_VERSION     },
    { "vlookup",    RPN_FNF_VLOOKUP     },

    { "weekday",    RPN_FNF_WEEKDAY     },
    { "weeknumber", RPN_FNF_WEEKNUMBER  },
    { "while",      RPN_FNF_WHILE       },

    { "year",       RPN_FNF_YEAR        },

    { "|",          RPN_BOP_OR          },

#define LOOK_TABLE_EXTRA 1 /* number of entries out of sequence, not to be searched for by bsearch */

    /* extra out-of-sequence functions start here */

    { "if",         RPN_FNF_IFC }
};

/*
definition of types available
*/

typedef struct TYPES
{
    U8Z id[10];
    EV_TYPE type_flags;
}
TYPES; typedef const TYPES * PC_TYPES;

static const TYPES
type_table[] =
{
    { "array",      EM_ARY },
    { "date",       EM_DAT },
    { "error",      EM_ERR },
    { "number",     EM_REA },
    { "reference",  EM_SLR },
    { "text",       EM_STR },

#define TYPE_TABLE_EXTRA 1 /* number of entries out of sequence, not to be searched for by bsearch */

    { "blank",      EM_BLK }
};

/******************************************************************************
*
* initialise resource enumerator
*
* docno is only relevant for names and custom functions
*
******************************************************************************/

extern void
ev_enum_resource_init(
    _OutRef_    P_EV_RESOURCE resop,
    _InVal_     enum EV_RESOURCE_TYPES category,
    _InVal_     EV_DOCNO docno_to,
    _InVal_     EV_DOCNO docno_from,
    _InRef_     PC_EV_OPTBLOCK p_optblock)
{
    resop->category = category;
    resop->docno_to   = docno_to;
    resop->docno_from = docno_from;
    resop->item_no  = 0;
    resop->optblock = *p_optblock;
}

/******************************************************************************
*
* get text about resource
*
* --in--
* item_no <  0 - get next resource
* item_no >= 0 - get specific resource
*
* --out--
* < 0 error (end)
* >=0 resource found
*
******************************************************************************/

extern S32
ev_enum_resource_get(
    P_EV_RESOURCE resop,
    P_S32 item_no,
    P_U8 name_out,
    S32 name_buf_siz,
    P_U8 arg_out,
    S32 arg_buf_siz,
    P_S32 n_args)
{
    S32 res, item_get;

    UNREFERENCED_PARAMETER(arg_buf_siz);

    if(*item_no < 0)
    {
        item_get = resop->item_no++;
        *item_no = item_get;
    }
    else
        item_get = *item_no;

    res = -1;
    switch(resop->category)
    {
    /* look thru custom functions */
    case EV_RESO_CUSTOM:
        {
        EV_NAMEID custom_num;
        P_EV_CUSTOM p_ev_custom;
        char doc_name_buf[BUF_EV_LONGNAMLEN];
        S32 item_at;

        for(custom_num = 0, p_ev_custom = custom_def_deptable.ptr, item_at = 0;
            custom_num < custom_def_deptable.next;
            ++custom_num, ++p_ev_custom)
        {
            __assume(p_ev_custom);
            if((resop->docno_to == DOCNO_NONE ||
                resop->docno_to == p_ev_custom->owner.docno) &&
               !(p_ev_custom->flags & TRF_UNDEFINED))
            {
                if(item_at == item_get)
                {
                    arg_out[0] = CH_NULL;
                    name_out[0] = CH_NULL;
                    if(p_ev_custom->owner.docno != resop->docno_from)
                    {
                        ev_write_docname(doc_name_buf, p_ev_custom->owner.docno, resop->docno_from);
                        strncat(name_out, doc_name_buf, name_buf_siz);
                    }
                    strncat(name_out, p_ev_custom->id, name_buf_siz - strlen(name_out));
                    if((*n_args = p_ev_custom->args.n) == 0)
                        *n_args = 1;
                    res = item_at;
                    break;
                }

                ++item_at;
            }
        }
        break;
        }

    /* look thru names */
    case EV_RESO_NAMES:
        {
        EV_NAMEID name_num;
        P_EV_NAME p_ev_name;
        char doc_name_buf[BUF_EV_LONGNAMLEN];
        S32 item_at;

        for(name_num = 0, p_ev_name = names_def_deptable.ptr, item_at = 0;
            name_num < names_def_deptable.next;
            ++name_num, ++p_ev_name)
        {
            __assume(p_ev_name);
            if((resop->docno_to == DOCNO_NONE ||
                resop->docno_to == p_ev_name->owner.docno) &&
               !(p_ev_name->flags & TRF_UNDEFINED))
            {
                if(item_at == item_get)
                {
                    name_out[0] = CH_NULL;
                    if(p_ev_name->owner.docno != resop->docno_from)
                    {
                        ev_write_docname(doc_name_buf, p_ev_name->owner.docno, resop->docno_from);
                        strncat(name_out, doc_name_buf, name_buf_siz);
                    }
                    strncat(name_out, p_ev_name->id, name_buf_siz - strlen(name_out));
                    *n_args = 0;
                    ev_decode_data(arg_out, resop->docno_from, &p_ev_name->def_data, &resop->optblock);
                    res = item_at;
                    break;
                }

                ++item_at;
            }
        }
        break;
        }

    /* deal with built-in functions */
    default:
        {
        S32 rpn_num, item_at;
        PC_RPNDEF rpn_p;

        for(rpn_num = RPN_REL_NOTEQUAL, rpn_p = &rpn_table[rpn_num], item_at = 0;
            rpn_num < RPN_FNM_CUSTOMCALL;
            ++rpn_num, ++rpn_p)
        {
            S32 x;
            if(resop->category == EV_RESO_COMPLEX &&
               rpn_num == RPN_FNF_C_RADIUS)
                x = 1;

            if(rpn_p->category == (U8) resop->category)
            {
                if(item_at == item_get)
                {
                    arg_out[0] = CH_NULL;
                    *n_args = 0;
                    name_out[0] = CH_NULL;
                    strncat(name_out, func_name(rpn_num), name_buf_siz);

                    if(rpn_p->n_args >= 0)
                        *n_args = rpn_p->n_args;
                    else if(rpn_p->n_args == -1)
                        *n_args = 1;
                    else
                        *n_args = -(rpn_p->n_args + 1);

                    res = item_at;
                    break;
                }

                ++item_at;
            }
        }

        break;
        }
    }

    return(res);
}

/******************************************************************************
*
* compare operators
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, func_name_compare, U8Z, LOOKDEF)
{
    BSEARCH_KEY_VAR_DECL(PC_USTR, key_id);
    BSEARCH_DATUM_VAR_DECL(PC_LOOKDEF, datum);
    PC_USTR datum_id = datum->id;

    /* NB no current_p_docu global register furtling required */

    return(C_stricmp(key_id, datum_id));
}

/******************************************************************************
*
* lookup id in master function table
*
* --out--
* < 0 (SYM_BAD) id not found in function table
* >=0 rpn number of found function
*
******************************************************************************/

extern S32
func_lookup(
    _In_z_      PC_USTR id)
{
    PC_LOOKDEF opr;

    assert(elemof32(rpn_table) == ELEMOF_RPN_TABLE);

    {
        opr = (PC_LOOKDEF)
            bsearch(id, look_table, elemof(look_table) - LOOK_TABLE_EXTRA, sizeof(look_table[0]), func_name_compare);
    }

    if(NULL == opr)
        return(-1);

    return(opr->did_num);
}

/******************************************************************************
*
* return pointer to function/operator name given its rpn number
*
******************************************************************************/

extern PC_USTR
func_name(
    EV_IDNO did_num)
{
    U32 i;
    PC_LOOKDEF opr;

    for(i = 0, opr = look_table; i < elemof32(look_table); ++i, ++opr)
        if(opr->did_num == did_num)
            return(opr->id);

    return(NULL);
}

/******************************************************************************
*
* return a type string given the type flags
*
******************************************************************************/

extern PC_A7STR
type_name_from_type_flags(
    EV_TYPE type_flags)
{
    U32 i;
    PC_TYPES typ;

    for(i = 0, typ = type_table; i < elemof32(type_table); ++i, ++typ)
        if(typ->type_flags == type_flags)
            return(typ->id);

    return(NULL);
}

/******************************************************************************
*
* compare types
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, type_name_compare, U8Z, TYPES)
{
    BSEARCH_KEY_VAR_DECL(PC_USTR, key_id);
    BSEARCH_DATUM_VAR_DECL(PC_TYPES, datum);
    PC_USTR datum_id = datum->id;

    /* NB no current_p_docu global register furtling required */

    return(/*"C"*/strcmp(key_id, datum_id));
}

/******************************************************************************
*
* look up type name in table
*
******************************************************************************/

extern EV_TYPE
type_lookup(
    _In_z_      PC_USTR id)
{
    PC_TYPES p_types = (PC_TYPES)
        bsearch(id, type_table, elemof(type_table) - TYPE_TABLE_EXTRA, sizeof(type_table[0]), type_name_compare);

    if(NULL == p_types)
        return(0);

    return(p_types->type_flags);
}

/* end of ev_tabl.c */
