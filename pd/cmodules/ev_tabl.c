/* ev_tabl.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

static const EV_TYPE arg_ads[]  = { 1, EM_REA | EM_DAT | EM_INT };

static const EV_TYPE arg_any[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT};

static const EV_TYPE arg_ary[]  = { 1, EM_ARY };

static const EV_TYPE arg_cho[]  = { 2, EM_INT,
                                       EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY };

static const EV_TYPE arg_dat[]  = { 1, EM_DAT };

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

static const EV_TYPE arg_int[]  = { 1, EM_INT };

static const EV_TYPE arg_ion[]  = { 1, EM_INT | EM_REA };

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

static const EV_TYPE arg_nmi[]  = { 2, EM_REA,
                                       EM_INT };

static const EV_TYPE arg_npv[]  = { 2, EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_num[]  = { 1, EM_REA };

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

static const EV_TYPE arg_slr[]  = { 1, EM_SLR };

static const EV_TYPE arg_stn[]  = { 2, EM_STR,
                                       EM_INT };

static const EV_TYPE arg_str[]  = { 1, EM_STR };

static const EV_TYPE arg_trd[]  = { 2, EM_ARY,
                                       EM_ARY | EM_REA };

static const EV_TYPE arg_txt[]  = { 2, EM_REA | EM_STR | EM_DAT,
                                       EM_STR };

/******************************************************************************
*
* table of operators and functions
*
******************************************************************************/

#define NAI 0              /* na integer */
#define NAP {EXEC_EXEC, 0, 0, 0, 0, 0}   /* na parms */
#define NAS NULL           /* na semantic address */
#define NAA NULL           /* na argument flags address */
#define NAT 0              /* na type */

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

    /* handle based resources owned by RPN */
    { RPN_DAT, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* rpn string */

    /* temporary handle based resources */
    { RPN_TMP, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* temporary string */
    { RPN_TMP, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* temporary array */

    /* handle based resources owned by a result */
    { RPN_RES, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* result string */
    { RPN_RES, NAI, EV_RESO_NOTME   ,         NAP, NAS,              NAA }, /* result array */

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

    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_not,            arg_int },
    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_umi,            arg_num }, /* unary - */
    { RPN_UOP,   1, EV_RESO_NOTME   ,         NAP, c_uplus,          arg_num }, /* unary + */

    /* binary operators */
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_and,            arg_int },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_div,            arg_ion },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_sub,            arg_ads },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_or,             arg_int },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_add,            arg_ads },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_power,          arg_num },
    { RPN_BOP,   2, EV_RESO_NOTME   ,         NAP, c_mul,            arg_ion },

    /* binary relational operators */
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_eq,             arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_gt,             arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_gteq,           arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_lt,             arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_lteq,           arg_rel },
    { RPN_REL,   2, EV_RESO_NOTME   ,         NAP, c_neq,            arg_rel },

    /* functions */
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_abs,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosec,         arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosech,        arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acosh,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acot,           arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acoth,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_acos,           arg_num },
    { RPN_FNF,   2, EV_RESO_DATE    ,         NAP, c_age,            arg_dat },
    { RPN_FNV,  -3, EV_RESO_MISC    , {EXEC_ALERT, 0, 0, 0, 0, 0},
                                                   NAS,              arg_str },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asec,           arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asech,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asinh,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_asin,           arg_num },
    { RPN_FNF,   2, EV_RESO_TRIG    ,         NAP, c_atan_2,         arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_atanh,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_atan,           arg_num },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_avg,            arg_mix },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_beta,           arg_num },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_bin,            arg_ary },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_binom,          arg_int },
    { RPN_FNV,  -1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_BREAK, 0, 0, 0, 0},
                                                   NAS,              arg_int },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacos,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacosec,        arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacosech,       arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacosh,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacot,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cacoth,         arg_ary },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_cadd,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_casec,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_casech,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_casin,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_casinh,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_catan,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_catanh,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccos,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccosec,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccosech,        arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccosh,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccot,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ccoth,          arg_ary },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_cdiv,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cexp,           arg_ary },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_char,           arg_int },
    { RPN_FNV,  -2, EV_RESO_LOOKUP  ,         NAP, c_choose,         arg_cho },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cln,            arg_ary },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_cmul,           arg_ary },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_code,           arg_str },
    { RPN_FNV,  -1, EV_RESO_MISC    , {EXEC_EXEC, 0, 0, 0, 1, 1},
                                                   c_col,            arg_rco },
    { RPN_FNV,  -1, EV_RESO_MISC    , {EXEC_EXEC, 0, 0, 0, 1, 1},
                                                   c_cols,           arg_ary },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_combin,         arg_int },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_CONTINUE, 0, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cos,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosec,          arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosech,         arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cosh,           arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_cot,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_coth,           arg_num },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_count,          arg_mix },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_cpower,         arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_cradius,        arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_csec,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_csech,          arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_csin,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_csinh,          arg_ary },
    { RPN_FNF,   2, EV_RESO_COMPLEX ,         NAP, c_csub,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ctan,           arg_ary },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ctanh,          arg_ary },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_cterm,          arg_num },
    { RPN_FNF,   1, EV_RESO_COMPLEX ,         NAP, c_ctheta,         arg_ary },
    { RPN_FNF,   3, EV_RESO_DATE    ,         NAP, c_date,           arg_int },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_datevalue,      arg_str },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DAVG,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_day,            arg_dat },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_dayname,        arg_dmn },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DCOUNT,  0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DCOUNTA, 0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   4, EV_RESO_FINANCE ,         NAP, c_ddb,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_deg,            arg_num },
    { RPN_FNF,   1, EV_RESO_MISC    ,         NAP, c_deref,          arg_drf },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DMAX,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DMIN,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DSTD,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DSTDP,   0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DSUM,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DVAR,    0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FNF,   2, EV_RESO_DATABASE, {EXEC_DBASE, DBASE_DVARP,   0, 1, 0, 0},
                                                   NAS,              arg_dbs },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_ELSE, EVS_CNT_ELSE, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_ELSEIF, EVS_CNT_ELSEIF, 0, 0, 0},
                                                   NAS,              arg_int },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_ENDIF, EVS_CNT_ENDIF, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_ENDWHILE, EVS_CNT_ENDWHILE, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FNF,   1, EV_RESO_MISC    ,         NAP, c_error,          arg_int },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_exact,          arg_str },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_exp,            arg_num },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_fact,           arg_int },
    { RPN_FNV,  -3, EV_RESO_STRING  ,         NAP, c_find,           arg_fnd },
    { RPN_FNF,   1, EV_RESO_MISC,             NAP, c_flip,           arg_ary },
    { RPN_FNV,  -4, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_FOR, EVS_CNT_FOR, 0, 0, 0},
                                                   NAS,              arg_for },
    { RPN_FNF,   1, EV_RESO_STRING,           NAP, c_formula_text,   arg_slr },
    { RPN_FNM,  -1, EV_RESO_CONTROL ,         NAP, NAS,              NAA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_fv,             arg_num },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_gammaln,        arg_num },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_GOTO, 0, 0, 0, 0},
                                                   NAS,              arg_slr },
    { RPN_FNV,  -1, EV_RESO_STATS   , {EXEC_EXEC, 0, 0, 0, 1, 0},
                                                   c_grand,          arg_num },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_growth,         arg_trd },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , {EXEC_LOOKUP, LOOKUP_HLOOKUP, 0, 0, 0, 0},
                                                   NAS,              arg_hvl },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_hour,           arg_dat },
    { RPN_FNV,  -3, EV_RESO_MISC    ,         NAP, c_if,             arg_if },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_IF, EVS_CNT_IFC, 0, 0, 0},
                                                   NAS,              arg_int },
    { RPN_FNV,  -4, EV_RESO_LOOKUP  ,         NAP, c_index,          arg_idx },
    { RPN_FNV,  -4, EV_RESO_MISC    , {EXEC_ALERT, 0, 0, 0, 0, 0},
                                                   NAS,              arg_str },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_int,            arg_ion },
    { RPN_FNF,   2, EV_RESO_FINANCE ,         NAP, c_irr,            arg_npv },
    { RPN_FNV,  -2, EV_RESO_STRING  ,         NAP, c_join,           arg_str },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_left,           arg_stn },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_length,         arg_str },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_linest,         arg_llg },
    { RPN_FNF,   1, EV_RESO_STATS,            NAP, c_listcount,      arg_ary },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_ln,             arg_num },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_log,            arg_num },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_logest,         arg_llg },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , {EXEC_LOOKUP, LOOKUP_LOOKUP,  0, 0, 0, 0},
                                                   NAS,              arg_lkp },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_lower,          arg_str },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , {EXEC_LOOKUP, LOOKUP_MATCH,   0, 0, 0, 0},
                                                   NAS,              arg_hvl },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_max,            arg_mix },
    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_mdeterm,        arg_ary },
    { RPN_FNF,   1, EV_RESO_STATS,            NAP, c_median,         arg_ary },
    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_minverse,       arg_ary },
    { RPN_FNF,   2, EV_RESO_MATRIX  ,         NAP, c_mmult,          arg_ary },
    { RPN_FNF,   3, EV_RESO_STRING  ,         NAP, c_mid,            arg_stn },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_min,            arg_mix },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_minute,         arg_dat },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_mirr,           arg_mir },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_mod,            arg_ion },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_month,          arg_dat },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_monthdays,      arg_dat },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_monthname,      arg_dmn },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_NEXT, EVS_CNT_NEXT, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FN0,   0, EV_RESO_DATE    ,         NAP, c_now,            NAA },
    { RPN_FNF,   2, EV_RESO_FINANCE ,         NAP, c_npv,            arg_npv },
    { RPN_FNF,   2, EV_RESO_MATHS   ,         NAP, c_permut,         arg_int },
    { RPN_FN0,   0, EV_RESO_TRIG    ,         NAP, c_pi,             NAA },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_pmt,            arg_num },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_proper,         arg_str },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_pv,             arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_rad,            arg_num },
    { RPN_FNV,  -1, EV_RESO_STATS   , {EXEC_EXEC, 0, 0, 0, 1, 0},
                                                   c_rand,           arg_num },
    { RPN_FNV,  -2, EV_RESO_STATS,            NAP, c_rank,           arg_rnk },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_rate,           arg_num },
    { RPN_FN0,   0, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_REPEAT, EVS_CNT_REPEAT, 0, 0, 0},
                                                   NAS,              NAA },
    { RPN_FNF,   4, EV_RESO_STRING  ,         NAP, c_replace,        arg_rpl },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_rept,           arg_stn },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_RESULT, 0, 0, 0, 0},
                                                   NAS,              arg_res },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_reverse,        arg_str },
    { RPN_FNF,   2, EV_RESO_STRING  ,         NAP, c_right,          arg_stn },
    { RPN_FNV,  -2, EV_RESO_MISC    ,         NAP, c_round,          arg_nmi },
    { RPN_FNV,  -1, EV_RESO_MISC    , {EXEC_EXEC, 0, 0, 0, 1, 1},
                                                   c_row,            arg_rco },
    { RPN_FNV,  -1, EV_RESO_MISC    , {EXEC_EXEC, 0, 0, 0, 1, 1},
                                                   c_rows,           arg_ary },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sec,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sech,           arg_num },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_second,         arg_dat },
    { RPN_FNF,   2, EV_RESO_MISC    ,         NAP, c_setname,        arg_setn },
    { RPN_FNF,   2, EV_RESO_MISC    , {EXEC_EXEC, 0, 0, 0, 0, 1},
                                                   c_setvalue,       arg_setv },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_sgn,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sin,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_sinh,           arg_num },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_sln,            arg_num },
    { RPN_FNV,  -2, EV_RESO_MISC,             NAP, c_sort,           arg_idx },
    { RPN_FNF,   2, EV_RESO_STATS,            NAP, c_spearman,       arg_ary },
    { RPN_FNF,   1, EV_RESO_MATHS   ,         NAP, c_sqr,            arg_num },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_std,            arg_nls },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_stdp,           arg_nls },
    { RPN_FNV,  -2, EV_RESO_STRING  ,         NAP, c_string,         arg_nmi },
    { RPN_FNV,  -2, EV_RESO_MATHS   ,         NAP, c_sum,            arg_mix },
    { RPN_FNF,   4, EV_RESO_FINANCE ,         NAP, c_syd,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_tan,            arg_num },
    { RPN_FNF,   1, EV_RESO_TRIG    ,         NAP, c_tanh,           arg_num },
    { RPN_FNF,   3, EV_RESO_FINANCE ,         NAP, c_term,           arg_num },
    { RPN_FNF,   2, EV_RESO_STRING,           NAP, c_text,           arg_txt },
    { RPN_FNF,   3, EV_RESO_DATE    ,         NAP, c_time,           arg_int },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_timevalue,      arg_str },
    { RPN_FN0,   0, EV_RESO_DATE    ,         NAP, c_today,          NAA },
    { RPN_FNF,   1, EV_RESO_MATRIX  ,         NAP, c_transpose,      arg_ary },
    { RPN_FNF,   2, EV_RESO_STATS   ,         NAP, c_trend,          arg_trd },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_trim,           arg_str },
    { RPN_FNF,   1, EV_RESO_MISC    ,         NAP, c_type,           arg_any },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_UNTIL, EVS_CNT_UNTIL, 0, 0, 0},
                                                   NAS,              arg_int },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_upper,          arg_str },
    { RPN_FNF,   1, EV_RESO_STRING  ,         NAP, c_value,          arg_str },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_var,            arg_nls },
    { RPN_FNV,  -2, EV_RESO_STATS   ,         NAP, c_varp,           arg_nls },
    { RPN_FN0,   0, EV_RESO_MISC    ,         NAP, c_version,        NAA },
    { RPN_FNF,   3, EV_RESO_LOOKUP  , {EXEC_LOOKUP, LOOKUP_VLOOKUP, 0, 0, 0, 0},
                                                   NAS,              arg_hvl },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_weekday,        arg_dat },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_weeknumber,     arg_dat },
    { RPN_FNF,   1, EV_RESO_CONTROL , {EXEC_CONTROL, CONTROL_WHILE, EVS_CNT_WHILE, 0, 0, 0},
                                                   NAS,              arg_int },
    { RPN_FNF,   1, EV_RESO_DATE    ,         NAP, c_year,           arg_dat },

    { RPN_FNM,  -1, EV_RESO_NOTME   ,         NAP, NAS,              NAA },

    { RPN_FNA,  -1, EV_RESO_NOTME   ,         NAP, NAS,              NAA }
};

/*
lowercase ASCII sorted compiler function lookup
*/

typedef struct lookdef
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
    { "acs",        RPN_FNF_ACS         },
    { "age",        RPN_FNF_AGE         },
    { "alert",      RPN_FNV_ALERT       },
    { "asec",       RPN_FNF_ASEC        },
    { "asech",      RPN_FNF_ASECH       },
    { "asinh",      RPN_FNF_ASINH       },
    { "asn",        RPN_FNF_ASN         },
    { "atan_2",     RPN_FNF_ATAN_2      },
    { "atanh",      RPN_FNF_ATANH       },
    { "atn",        RPN_FNF_ATN         },
    { "avg",        RPN_FNV_AVG         },
    { "beta",       RPN_FNF_BETA        },
    { "bin",        RPN_FNF_BIN         },
    { "binom",      RPN_FNF_BINOM       },
    { "break",      RPN_FNV_BREAK       },
    { "c_acos",     RPN_FNF_CACOS       },
    { "c_acosec",   RPN_FNF_CACOSEC     },
    { "c_acosech",  RPN_FNF_CACOSECH    },
    { "c_acosh",    RPN_FNF_CACOSH      },
    { "c_acot",     RPN_FNF_CACOT       },
    { "c_acoth",    RPN_FNF_CACOTH      },
    { "c_add",      RPN_FNF_CADD        },
    { "c_asec",     RPN_FNF_CASEC       },
    { "c_asech",    RPN_FNF_CASECH      },
    { "c_asin",     RPN_FNF_CASIN       },
    { "c_asinh",    RPN_FNF_CASINH      },
    { "c_atan",     RPN_FNF_CATAN       },
    { "c_atanh",    RPN_FNF_CATANH      },
    { "c_cos",      RPN_FNF_CCOS        },
    { "c_cosec",    RPN_FNF_CCOSEC      },
    { "c_cosech",   RPN_FNF_CCOSECH     },
    { "c_cosh",     RPN_FNF_CCOSH       },
    { "c_cot",      RPN_FNF_CCOT        },
    { "c_coth",     RPN_FNF_CCOTH       },
    { "c_div",      RPN_FNF_CDIV        },
    { "c_exp",      RPN_FNF_CEXP        },
    { "c_ln",       RPN_FNF_CLN         },
    { "c_mul",      RPN_FNF_CMUL        },
    { "c_power",    RPN_FNF_CPOWER      },
    { "c_radius",   RPN_FNF_CRADIUS     },
    { "c_sec",      RPN_FNF_CSEC        },
    { "c_sech",     RPN_FNF_CSECH       },
    { "c_sin",      RPN_FNF_CSIN        },
    { "c_sinh",     RPN_FNF_CSINH       },
    { "c_sub",      RPN_FNF_CSUB        },
    { "c_tan",      RPN_FNF_CTAN        },
    { "c_tanh",     RPN_FNF_CTANH       },
    { "c_theta",    RPN_FNF_CTHETA      },
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
    { "cterm",      RPN_FNF_CTERM       },
    { "date",       RPN_FNF_DATE        },
    { "datevalue",  RPN_FNF_DATEVALUE   },
    { "davg",       RPN_FNF_DAVG        },
    { "day",        RPN_FNF_DAY         },
    { "dayname",    RPN_FNF_DAYNAME     },
    { "dcount",     RPN_FNF_DCOUNT      },
    { "dcounta",    RPN_FNF_DCOUNTA     },
    { "ddb",        RPN_FNF_DDB         },
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
    { "error",      RPN_FNF_ERROR       },
    { "exact",      RPN_FNF_EXACT       },
    { "exp",        RPN_FNF_EXP         },
    { "fact",       RPN_FNF_FACT        },
    { "find",       RPN_FNV_FIND        },
    { "flip",       RPN_FNF_FLIP        },
    { "for",        RPN_FNV_FOR         },
    { "formula_text",   RPN_FNF_FORMULA     },
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
    { "left",       RPN_FNF_LEFT        },
    { "length",     RPN_FNF_LENGTH      },
    { "linest",     RPN_FNV_LINEST      },
    { "listcount",  RPN_FNF_LISTCOUNT   },
    { "ln",         RPN_FNF_LN          },
    { "log",        RPN_FNF_LOG         },
    { "logest",     RPN_FNV_LOGEST      },
    { "lookup",     RPN_FNF_LOOKUP      },
    { "lower",      RPN_FNF_LOWER       },
    { "m_determ",   RPN_FNF_MDETERM     },
    { "m_inverse",  RPN_FNF_MINVERSE    },
    { "m_mult",     RPN_FNF_MMULT       },
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
    { "rad",        RPN_FNF_RAD         },
    { "rand",       RPN_FNV_RAND        },
    { "rank",       RPN_FNV_RANK        },
    { "rate",       RPN_FNF_RATE        },
    { "repeat",     RPN_FN0_REPEAT      },
    { "replace",    RPN_FNF_REPLACE     },
    { "rept",       RPN_FNF_REPT        },
    { "result",     RPN_FNF_RESULT      },
    { "reverse",    RPN_FNF_REVERSE     },
    { "right",      RPN_FNF_RIGHT       },
    { "round",      RPN_FNV_ROUND       },
    { "row",        RPN_FNV_ROW         },
    { "rows",       RPN_FNV_ROWS        },
    { "sec",        RPN_FNF_SEC         },
    { "sech",       RPN_FNF_SECH        },
    { "second",     RPN_FNF_SECOND      },
    { "set_name",   RPN_FNF_SETNAME     },
    { "set_value",  RPN_FNF_SETVALUE    },
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

    /* extra out-of-sequence functions start here */

    { "if",         RPN_FNF_IFC }
};

/* number of functions out of sequence, not
 * to be searched for by bsearch */

#define LOOKTAB_EXTRA 1

/*
definition of types available
*/

typedef struct _types
{
    U8Z id[10];
    EV_TYPE types;
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
    { "blank",      EM_BLK }
};

/******************************************************************************
*
* initialise resource enumerator
*
* docno is only relevant for names
* and custom functions
*
******************************************************************************/

extern void
ev_enum_resource_init(
    P_EV_RESOURCE resop,
    S32 category,
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
    P_S32 nargs)
{
    S32 res, item_get;

    IGNOREPARM(arg_buf_siz);

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

            for(custom_num = 0, p_ev_custom = custom_ptr(0), item_at = 0;
                custom_num < custom_def.next;
                ++custom_num, ++p_ev_custom)
                {
                __assume(p_ev_custom);
                if((resop->docno_to == DOCNO_NONE ||
                    resop->docno_to == p_ev_custom->owner.docno) &&
                   !(p_ev_custom->flags & TRF_UNDEFINED))
                    {
                    if(item_at == item_get)
                        {
                        arg_out[0] = '\0';
                        name_out[0] = '\0';
                        if(p_ev_custom->owner.docno != resop->docno_from)
                            {
                            ev_write_docname(doc_name_buf, p_ev_custom->owner.docno, resop->docno_from);
                            strncat(name_out, doc_name_buf, name_buf_siz);
                            }
                        strncat(name_out, p_ev_custom->id, name_buf_siz - strlen(name_out));
                        if((*nargs = p_ev_custom->args.n) == 0)
                            *nargs = 1;
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

            for(name_num = 0, p_ev_name = name_ptr(0), item_at = 0;
                name_num < names_def.next;
                ++name_num, ++p_ev_name)
                {
                __assume(p_ev_name);
                if((resop->docno_to == DOCNO_NONE ||
                    resop->docno_to == p_ev_name->owner.docno) &&
                   !(p_ev_name->flags & TRF_UNDEFINED))
                    {
                    if(item_at == item_get)
                        {
                        name_out[0] = '\0';
                        if(p_ev_name->owner.docno != resop->docno_from)
                            {
                            ev_write_docname(doc_name_buf, p_ev_name->owner.docno, resop->docno_from);
                            strncat(name_out, doc_name_buf, name_buf_siz);
                            }
                        strncat(name_out, p_ev_name->id, name_buf_siz - strlen(name_out));
                        *nargs = 0;
                        ev_decode_data(arg_out, resop->docno_from, &p_ev_name->def, &resop->optblock);
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
                   rpn_num == RPN_FNF_CRADIUS)
                    x = 1;

                if(rpn_p->category == (unsigned) resop->category)
                    {
                    if(item_at == item_get)
                        {
                        arg_out[0] = '\0';
                        *nargs = 0;
                        name_out[0] = '\0';
                        strncat(name_out, func_name(rpn_num), name_buf_siz);

                        if(rpn_p->nargs >= 0)
                            *nargs = rpn_p->nargs;
                        else if(rpn_p->nargs == -1)
                            *nargs = 1;
                        else
                            *nargs = -(rpn_p->nargs + 1);

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

PROC_BSEARCH_PROTO(static, func_lookcomp, U8Z, LOOKDEF)
{
    PC_USTR key_id = (PC_USTR) key;
    PC_LOOKDEF p_lookdef = (PC_LOOKDEF) datum;
    PC_USTR datum_id = p_lookdef->id;

    /* NB no current_p_docu global register furtling required */

    return(/*"C"*/_stricmp(key_id, datum_id));
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
    PC_USTR id)
{
    PC_LOOKDEF opr;

    {
        opr = (PC_LOOKDEF)
            bsearch(id, look_table, elemof(look_table) - LOOKTAB_EXTRA, sizeof(look_table[0]), func_lookcomp);
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

extern PC_USTR
type_from_flags(
    EV_TYPE type)
{
    S32 i;
    PC_TYPES typ;

    for(i = 0, typ = type_table; i < elemof32(type_table); ++i, ++typ)
        if(typ->types == type)
            return(typ->id);

    return(NULL);
}

/******************************************************************************
*
* compare types
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, type_lookcomp, U8Z, TYPES)
{
    PC_USTR key_id = (PC_USTR) key;
    PC_TYPES p_types = (PC_TYPES) datum;
    PC_USTR datum_id = p_types->id;

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
    PC_USTR id)
{
    PC_TYPES p_types = (PC_TYPES)
        bsearch(id, type_table, elemof(type_table), sizeof(type_table[0]), type_lookcomp);

    if(NULL == p_types)
        return(0);

    return(p_types->types);
}

/* end of ev_tabl.c */
