/* ev_comp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Infix to RPN compiler */

/* MRJC May 1988 / January 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "ev_eval.h"

#include "math.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

static void
out_byte(
    char byt);

static S32
out_chkspace(
    S32 needed);

static void
out_idno(
    EV_IDNO idno);

static void
out_idno_format(
    P_SYM_INF p_sym_inf);

static void
out_nameid(
    EV_NAMEID nameid);

static S32
proc_func(
    PC_RPNDEF funp);

static void
proc_func_arg_maybe_blank(void);

static S32
proc_func_dbs(void);

static S32
proc_func_if(
    P_EV_IDNO which_if);

static S32
proc_func_custom(
    EV_NAMEID *customidp);

static S32
proc_custom_argument(
    PC_U8 text_in,
    P_U8 name_out,
    P_EV_TYPE typep);

static S32
recog_array(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok);

static S32
recog_array_row(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok,
    P_S32 x,
    P_S32 y);

static U32
recog_dt(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 one,
    _OutRef_    P_S32 two,
    _OutRef_    P_S32 three,
    char separator,
    S32 count);

static S32
recog_n_dt_str_slr_rng(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    PC_U8 in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok);

static S32
recog_name(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    P_U8 doc_name);

static S32
ss_recog_number(
    _InoutRef_  P_EV_DATA p_ev_data,
    _In_z_      PC_U8Z in_str);

static S32
recog_slr_range(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    P_U8 doc_name);

static S32
recog_string(
    _InoutRef_  P_EV_DATA p_ev_data,
    _In_z_      PC_U8Z in_str);

static void
rec_array_row(
    S32 cur_y,
    P_S32 x_size);

static void
rec_bterm(void);

static void
rec_cterm(void);

static void
rec_dterm(void);

static void
rec_eterm(void);

static void
rec_expr(void);

static void
rec_fterm(void);

static void
rec_gterm(void);

static void
rec_lterm(void);

static EV_IDNO
scan_check_next(
    P_SYM_INF p_sym_inf);

static void
scan_next_symbol(void);

static S32
set_error(
    S32 err);

static S32
ev_stox(
    PC_U8 string,
    EV_COL * col);

/*
* compiler IO variables
*/

typedef struct COMPILER_CONTEXT
{
    EV_DOCNO        docno;      /* owner document of expression */
    P_U8            ip_pos;     /* position in input */

    char doc_name[BUF_EV_LONGNAMLEN]; /* scanned external ref */
    char ident[BUF_EV_INTNAMLEN];     /* scanned identifier */
    EV_FLAGS        type_flags; /* local argument type qualifier */

    EV_DATA         cur_sym;    /* details of symbol scanned */
    SYM_INF         cur;
    P_U8            err_pos;    /* position of error */
    S32             error;      /* reason for non-compilation */

    S32             dbs_nest;   /* database function nest count */
    S32             array_nest; /* array nest count */

    P_U8            op_start;   /* start of output buffer */
    P_U8            op_pos;     /* position in output */
    S32             op_maxlen;  /* maximum length of output */

    FUN_PARMS       parms;      /* things about compiled expression */
    PC_EV_OPTBLOCK  p_optblock; /* pointer to options */
}
* P_COMPILER_CONTEXT;

static P_COMPILER_CONTEXT cc = NULL;

/******************************************************************************
*
* Grammar:
*
*   <expr>     := <aterm>  { <group6><aterm> }
*   <aterm>    := <bterm>  { <group5><bterm> }
*   <bterm>    := <cterm>  { <group4><cterm> }
*   <cterm>    := <dterm>  { <group3><dterm> }
*   <dterm>    := <eterm>  { <group2><eterm> }
*   <eterm>    := <fterm>  { <group1><fterm> }
*   <fterm>    := { <unary><fterm> } | <gterm>
*   <gterm>    := <lterm> | (<expr>)
*   <lterm>    := number        |
*                 date          |
*                 slr           |
*                 range         |
*                 string        |
*                 '{'<array>'}' |
*                 <function>    |
*                 <custom>      |
*                 <name>
*
*   <array>    := <row>  {;<row>}
*   <row>      := <item> {,<item>}
*   <item>     := number | slr | date | string | name
*
*   <function> := <ownid> | <owndb>(range, <expr>) | <ownfunc>(<args>)
*   <custom>   := <name>(<args>)
*
*   <name>     := {<external>} ident
*   <external> := [ ident ]
*   <args>     := {<expr>} {,<expr>}
*   <ownid>    := col | pi | row ...
*   <owndb>    := dsum | dmax | dmin ...
*   <ownfunc>  := abs | int | lookup | max | year ...
*
*   <unary>    := + | - | !
*   <group1>   := ^
*   <group2>   := * | /
*   <group3>   := + | -
*   <group4>   := = | <> | < | > | <= | >=
*   <group5>   := &
*   <group6>   := |
*
* examples:
*   number     9.9
*   slr        [ext]$A1
*   range      [ext]$A1$A10
*   string     "fred"
*   ident      abc_123
*   date       1.2.3 10:56
*
******************************************************************************/

/******************************************************************************
*
* compile infix expression into RPN
*
* --out--
* <0  error in processing input string
* >=0 length of compiled rpn string
*
******************************************************************************/

extern S32
ev_compile(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_in,
    P_U8 rpn_out,
    S32 maxlen,
    P_S32 at_pos,
    P_EV_PARMS parmsp,
    _InRef_     PC_EV_OPTBLOCK p_optblock)
{
    S32 res;
    P_COMPILER_CONTEXT old_cc;
    struct COMPILER_CONTEXT comp_cont;

    /* set up current compiler context */
    old_cc = cc;
    cc     = &comp_cont;

    /* save options */
    cc->p_optblock = p_optblock;

    /* default to being a constant slot */
    parmsp->type = EVS_CON_RPN;

    /* return zero for blank slot */
    if(!*txt_in)
        return(0);

    cc->docno         = docno;
    cc->ip_pos        = txt_in;
    cc->op_start      = rpn_out;
    cc->op_pos        = rpn_out;
    cc->op_maxlen     = maxlen;
    cc->parms.var     = 0;
    cc->parms.control = 0;
    cc->cur.did_num   = SYM_BLANK;
    cc->error         = 0;
    cc->dbs_nest      = 0;
    cc->array_nest    = 0;

    /* recognise the expression */
    doc_reuse_hold();
    rec_expr();
    doc_reuse_release();

    /* set scanner position */
    *at_pos = cc->ip_pos - txt_in;

    if(cc->cur.did_num != RPN_FRM_END)
        {
        res = cc->error;
        *at_pos = cc->err_pos - txt_in;
        }
    else
        {
        out_idno(RPN_FRM_END);

        /* we are allowed to fill the buffer with cc->op_maxlen bytes */
        if(cc->op_pos - cc->op_start > cc->op_maxlen)
            {
            trace_2(TRACE_MODULE_EVAL | TRACE_OUT, "ev_compile: expression too long %d %d", cc->op_pos - cc->op_start, cc->op_maxlen);
            res = create_error(EVAL_ERR_EXPTOOLONG);
            }
        else
            {
            parmsp->control = cc->parms.control;
            if(cc->parms.var)
                parmsp->type = EVS_VAR_RPN;

            res = cc->op_pos - rpn_out;
            }
        }

    /* restore old compiler context */
    cc = old_cc;

    return(res);
}

/******************************************************************************
*
* check if string can be name, and then
* check if name is defined
*
* <0 bad name string
* =0 name string OK, not defined
* >0 name string OK & defined
*
******************************************************************************/

extern S32
ev_name_check(
    P_U8 name,
    _InVal_     EV_DOCNO docno)
{
    S32 res;

    /* MRJC 28.4.92 */
    while(*name == ' ')
        ++name;

    if((res = ident_validate(name)) >= 0)
        {
        if(func_lookup(name) >= 0)
            res = -1;
        else
            {
            if(find_name_in_list(docno, name) < 0)
                res = 0;
            else
                res = 1;
            }
        }

    return(res);
}

/******************************************************************************
*
* try to scan a constant:
* number, string or date
*
* --out--
* <0 error
* =0 nothing found
* >0 #characters scanned
*
******************************************************************************/

extern S32
ss_recog_constant(
    P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno,
    _In_z_      PC_U8Z in_str,
    _InRef_     PC_EV_OPTBLOCK p_optblock,
    S32 refs_ok)
{
    S32 res = 0;
    PC_U8 in_pos = in_str;

    if(in_str)
        {
        S32 len;

        while(*in_pos == ' ')
            ++in_pos;

        if((len = recog_n_dt_str_slr_rng(p_ev_data, docno, in_pos, p_optblock->american_date, refs_ok, 0)) > 0 ||
           (len = recog_array(p_ev_data, docno, in_pos, p_optblock->american_date, 0, 0)) > 0
          )
            {
            in_pos += len;

            while(*in_pos == ' ' ||
                  *in_pos == LF  ||
                  *in_pos == CR)
                ++in_pos;

            /* check there's nothing following */
            if(!*in_pos)
                res = 1;
            else
                ss_data_free_resources(p_ev_data);
            }
        else
            res = len;
        }

    return(res > 0 ? in_pos - in_str : res);
}

/******************************************************************************
*
* an internal or custom function call has been identified
*
******************************************************************************/

static void
func_call(
    P_SYM_INF p_sym_inf,
    EV_NAMEID nameid)
{
    PC_RPNDEF funp;

    funp = &rpn_table[p_sym_inf->did_num];

    if(funp->parms.datab)
        {
        /*
         * database functions
         */
        cc->dbs_nest += 1;

        if(cc->dbs_nest > 1)
            set_error(create_error(EVAL_ERR_DBASENEST));
        else if(proc_func_dbs() >= 0)
            out_idno_format(p_sym_inf);

        cc->dbs_nest -= 1;
        }
    else if(p_sym_inf->did_num == RPN_FNV_IF)
        {
        S32 narg;
        EV_IDNO which_if;

        /*
         * if()
         */
        if((narg = proc_func_if(&which_if)) >= 0)
            {
            p_sym_inf->did_num = which_if;
            out_idno_format(p_sym_inf);

            if(which_if == RPN_FNV_IF)
                out_byte((char) narg);
            }
        }
    else if(p_sym_inf->did_num == RPN_FNM_FUNCTION)
        {
        /*
         * custom definitions
         */
        S32 narg;
        EV_NAMEID customid;

        if((narg = proc_func_custom(&customid)) >= 0)
            {
            out_idno_format(p_sym_inf);
            out_byte((char) narg);
            out_nameid(customid);
            }
        }
    else
        {
        /*
         * other functions
         */
        if(funp->nargs < 0)
            {
            /*
             * variable argument functions
             */
            S32 narg;

            if((narg = proc_func(funp)) >= 0)
                {
                out_idno_format(p_sym_inf);
                out_byte((char) narg);

                /* custom calls have their id tagged on t'end */
                if(p_sym_inf->did_num == RPN_FNM_CUSTOMCALL)
                    out_nameid(nameid);
                }
            }
        else if(funp->nargs == 0)
            {
            /*
             * zero argument functions
             */
            out_idno_format(p_sym_inf);
            }
        else
            {
            /*
             * fixed argument functions
             */
            if(proc_func(funp) >= 0)
                out_idno_format(p_sym_inf);
            }
        }

    /* done at the end cos p_sym_inf->did_num
     * may be adjusted */
    funp = &rpn_table[p_sym_inf->did_num];

    /* accumulate flags */
    cc->parms.control  = funp->parms.control;
    cc->parms.var     |= funp->parms.var;
}

/******************************************************************************
*
* validate an identifier
*
* --out--
* <0  error
* >=0 length of id
*
******************************************************************************/

extern S32
ident_validate(
    PC_U8 ident)
{
    PC_U8 pos;
    S32 had_digit, digit_ok;

    pos       = ident;
    had_digit = 0;
    digit_ok  = 0;

    /* MRJC 24.3.92 - skip lead spaces */
    while(*pos == ' ')
        ++pos;

    /* digits in identifiers can
     * occur only after an underscore
    */
    while(isalnum(*pos) || *pos == '_')
        {
        if(isdigit(*pos))
            had_digit = 1;
        else if(*pos == '_')
            digit_ok = 1;

        ++pos;
        }

    /* MRJC 24.3.92 - skip trail spaces */
    while(*pos == ' ')
        ++pos;

    if(*pos                   ||
       pos == ident           ||
       had_digit && !digit_ok ||
       pos - ident > EV_INTNAMLEN)
        return(create_error(EVAL_ERR_BADIDENT));

    return(pos - ident);
}

/******************************************************************************
*
* output byte to compiled expression
*
******************************************************************************/

static void
out_byte(
    U8 byt)
{
    if(out_chkspace(sizeof32(U8)))
        *cc->op_pos++ = byt;
}

/******************************************************************************
*
* check that we can output to buffer
*
******************************************************************************/

static S32
out_chkspace(
    S32 needed)
{
    if((cc->op_pos - cc->op_start) + needed > cc->op_maxlen)
        {
        set_error(create_error(EVAL_ERR_EXPTOOLONG));
        return(0);
        }

    return(1);
}

/******************************************************************************
*
* output id number to compiled expression
*
******************************************************************************/

static void
out_idno(
    EV_IDNO idno)
{
    if(out_chkspace(sizeof32(EV_IDNO)))
        cc->op_pos += writeuword_LE(cc->op_pos, idno, sizeof32(EV_IDNO));
}

/******************************************************************************
*
* output id number with accumulated
* formatting information
*
******************************************************************************/

static void
out_idno_format(
    P_SYM_INF p_sym_inf)
{
    if(p_sym_inf->sym_cr)
        if(out_chkspace(2 * sizeof32(char)))
            {
            *cc->op_pos++ = RPN_FRM_RETURN;
            *cc->op_pos++ = p_sym_inf->sym_cr;
            }

    if(p_sym_inf->sym_space)
        if(out_chkspace(2 * sizeof32(char)))
            {
            *cc->op_pos++ = RPN_FRM_SPACE;
            *cc->op_pos++ = p_sym_inf->sym_space;
            }

    out_idno(p_sym_inf->did_num);
}

/******************************************************************************
*
* output name id to rpn string
*
******************************************************************************/

static void
out_nameid(
    EV_NAMEID nameid)
{
    if(out_chkspace(sizeof32(EV_NAMEID)))
        cc->op_pos += write_nameid(nameid, cc->op_pos);
}

/******************************************************************************
*
* output a range to the compiled string
*
******************************************************************************/

static void
out_range(
    _InRef_     PC_EV_RANGE p_ev_range)
{
    if(out_chkspace(PACKED_RNGSIZE))
        cc->op_pos += write_rng(p_ev_range, cc->op_pos);
}

/******************************************************************************
*
* output a slot reference to the compiled string
*
******************************************************************************/

static void
out_slr(
    _InRef_     PC_EV_SLR slrp)
{
    if(out_chkspace(PACKED_SLRSIZE))
        cc->op_pos += write_slr(slrp, cc->op_pos);
}

/******************************************************************************
*
* output string to rpn
*
******************************************************************************/

static void
out_string_free(
    P_SYM_INF p_sym_inf,
    P_U8 *stringpp)
{
    S32 len;

    p_sym_inf->did_num = RPN_DAT_STRING;
    out_idno_format(p_sym_inf);

    len = strlen(*stringpp) + 1;

    if(out_chkspace(sizeof32(S16) + len))
        {
        writeval_S16(cc->op_pos, len + sizeof32(S16));
        cc->op_pos += sizeof32(S16);
        strcpy(cc->op_pos, *stringpp);
        cc->op_pos += len;
        }

    al_ptr_dispose(P_P_ANY_PEDANTIC(stringpp));
}

/******************************************************************************
*
* output signed 16 bit word to compiled expression
*
******************************************************************************/

static void
out_S16(
    _InVal_     S16 wrd)
{
    if(out_chkspace(sizeof32(S16)))
    {
        writeval_S16(cc->op_pos, wrd);
        cc->op_pos += sizeof32(S16);
    }
}

/******************************************************************************
*
* output data to the rpn string
*
******************************************************************************/

static void
out_to_rpn(
    _InVal_     U32 n_bytes,
    _In_reads_bytes_(n_bytes) PC_ANY p_data)
{
    if(out_chkspace(n_bytes))
    {
        memcpy32(cc->op_pos, p_data, n_bytes);
        cc->op_pos += n_bytes;
    }
}

/******************************************************************************
*
* process a function with arguments
*
* --out--
* <0  error encountered
* >=0 number of arguments processed
*
******************************************************************************/

static S32
proc_func(
    PC_RPNDEF funp)
{
    S32 narg, nodep;

    /* have we any arguments at all ? */
    if(scan_check_next(NULL) != SYM_OBRACKET)
        {
        /* function allowed to be totally devoid ? */
        if(funp->nargs == -1)
            return(0);

        return(set_error(create_error(EVAL_ERR_FUNARGS)));
        }
    else
        cc->cur.did_num = SYM_BLANK;

    narg = 0;
    nodep = funp->parms.nodep;

    /* loop over function arguments */
    if(scan_check_next(NULL) != SYM_CBRACKET)
        {
        /* indicate non-dependent arguments */
        if(nodep)
            {
            out_byte(RPN_FRM_NODEP);
            --nodep;
            }

        while(TRUE)
            {
            proc_func_arg_maybe_blank();
            ++narg;
            if(scan_check_next(NULL) != SYM_COMMA)
                break;
            cc->cur.did_num = SYM_BLANK;
            }
        }

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_error(create_error(EVAL_ERR_CBRACKETS)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* functions with fixed number of arguments must
     * have exactly the correct number of arguments
    */
    if(funp->nargs >= 0)
        {
        if(narg != funp->nargs)
            return(set_error(create_error(EVAL_ERR_FUNARGS)));
        }
    /* functions with variable number of arguments must
     * have at least (-funp->nargs - 1) arguments
    */
    else if(narg < -funp->nargs - 1)
        return(set_error(create_error(EVAL_ERR_FUNARGS)));

    return(narg);
}

/******************************************************************************
*
* process a function argument which may be blank
*
******************************************************************************/

static void
proc_func_arg_maybe_blank(void)
{
    SYM_INF sym_inf;

    scan_check_next(&sym_inf);

    if(sym_inf.did_num == SYM_COMMA ||
       sym_inf.did_num == SYM_CBRACKET)
        {
        sym_inf.did_num = RPN_DAT_BLANK;
        out_idno_format(&sym_inf);
        }
    else
        rec_expr();
}

/******************************************************************************
*
* process a database function
* <datab> := <identd> (range, conditional)
*
******************************************************************************/

static S32
proc_func_dbs(void)
{
    P_U8 dbase_start;

    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_error(create_error(EVAL_ERR_FUNARGS)));
    else
        cc->cur.did_num = SYM_BLANK;

    proc_func_arg_maybe_blank();

    /* munge the comma */
    if(scan_check_next(NULL) != SYM_COMMA)
        return(set_error(create_error(EVAL_ERR_FUNARGS)));
    else
        cc->cur.did_num = SYM_BLANK;

    out_idno(RPN_FRM_COND);
    dbase_start = cc->op_pos;
    /* reserve space for condition length */
    out_S16(0);

    /* scan conditional */
    rec_expr();

    /* put on the end */
    out_idno(RPN_FRM_END);

    /* update condition length */
    writeval_S16(dbase_start, cc->op_pos - dbase_start);

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_error(create_error(EVAL_ERR_CBRACKETS)));
    else
        cc->cur.did_num = SYM_BLANK;

    return(0);
}

/******************************************************************************
*
* process an if function
*
* --out--
* <0  error encountered
* >=0 OK (nargs)
*
******************************************************************************/

static S32
proc_func_if(
    P_EV_IDNO which_if)
{
    P_U8 skip_post, skip_posf;

    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_error(create_error(EVAL_ERR_FUNARGS)));
    else
        cc->cur.did_num = SYM_BLANK;

    proc_func_arg_maybe_blank();

    if(scan_check_next(NULL) != SYM_COMMA)
        {
        /* check for single argument if */
        if(scan_check_next(NULL) == SYM_CBRACKET)
            {
            cc->cur.did_num = SYM_BLANK;
            *which_if = RPN_FNF_IFC;
            return(1);
            }
        else
            return(set_error(create_error(EVAL_ERR_BADEXPR)));
        }
    else
        cc->cur.did_num = SYM_BLANK;

    /* two or three-part if */
    *which_if = RPN_FNV_IF;

    skip_post = cc->op_pos;
    out_idno(RPN_FRM_SKIPFALSE);
    /* boolean stack offset = 0 */
    out_byte(0);
    out_S16(0);

    /* get next argument */
    proc_func_arg_maybe_blank();

    writeval_S16(skip_post + 2, cc->op_pos - skip_post);

    /* did he give up with 2 arguments? */
    if(scan_check_next(NULL) == SYM_CBRACKET)
        {
        cc->cur.did_num = SYM_BLANK;
        return(2);
        }

    skip_posf = cc->op_pos;
    out_idno(RPN_FRM_SKIPTRUE);
    /* boolean stack offset = 1 */
    out_byte(1);
    out_S16(0);

    if(scan_check_next(NULL) != SYM_COMMA)
        return(set_error(create_error(EVAL_ERR_BADEXPR)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* next argument */
    proc_func_arg_maybe_blank();

    writeval_S16(skip_posf + 2, cc->op_pos - skip_posf);

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_error(create_error(EVAL_ERR_CBRACKETS)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* we did the full three arguments */
    return(3);
}

/******************************************************************************
*
* process a custom function definition of the form:
* function("name:type", "arg1:type", ... "argn:type")
*
******************************************************************************/

static S32
proc_func_custom(
    EV_NAMEID *customidp)
{
    SYM_INF sym_inf;
    EV_NAMEID custom_num;
    char custom_name[BUF_EV_INTNAMLEN];
    EV_TYPE dummy_type;
    P_EV_CUSTOM p_ev_custom;
    S32 narg, res;

    /* must start with opening bracket */
    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_error(create_error(EVAL_ERR_FUNARGS)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* scan first argument which must be custom function name */
    if(scan_check_next(&sym_inf) != RPN_TMP_STRING)
        return(set_error(create_error(EVAL_ERR_ARGTYPE)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* extract custom function name from first argument */
    if((res = proc_custom_argument(cc->cur_sym.arg.string.data,
                                  custom_name,
                                  &dummy_type)) < 0)
        return(set_error(res));

    /* does the custom function already exist ? */
    if((custom_num = find_custom_in_list(cc->docno, custom_name)) >= 0)
        {
        p_ev_custom = custom_ptr_must(custom_num);
        if(!(p_ev_custom->flags & TRF_UNDEFINED))
            return(set_error(create_error(EVAL_ERR_CUSTOMEXISTS)));
        }
    /* ensure custom function is in list */
    else if((custom_num = ensure_custom_in_list(cc->docno, custom_name)) >= 0)
        {
        p_ev_custom = custom_ptr_must(custom_num);
        }
    else
        return(set_error(status_nomem()));

    /* send custom identifier home */
    *customidp = p_ev_custom->key;

    /* store custom name (first arg to function()) in rpn */
    out_string_free(&sym_inf, &cc->cur_sym.arg.string_wr.data);

    /* now loop over arguments, commas etc storing
     * the results in the custom definition structure
     * as well as sending them to the rpn for decompilation
     * closing bracket will stop argument scanning
     */

    res = 0;
    narg = 0;
    while(scan_check_next(NULL) == SYM_COMMA && narg < EV_MAX_ARGS)
        {
        cc->cur.did_num = SYM_BLANK;

        if(scan_check_next(&sym_inf) != RPN_TMP_STRING)
            {
            res = -1;
            break;
            }
        else
            cc->cur.did_num = SYM_BLANK;

        /* extract argument names and types */
        if((res = proc_custom_argument(cc->cur_sym.arg.string.data,
                                       p_ev_custom->args.id[narg],
                                       &p_ev_custom->args.types[narg])) < 0)
            break;

        /* output argument text to rpn */
        out_string_free(&sym_inf, &cc->cur_sym.arg.string_wr.data);

        ++narg;
        }

    p_ev_custom->args.n = narg;

    if(res < 0)
        return(set_error(res));

    /* must finish with closing bracket */
    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_error(create_error(EVAL_ERR_CBRACKETS)));
    else
        cc->cur.did_num = SYM_BLANK;

    /* return number of args to function() */
    return(narg + 1);
}

/******************************************************************************
*
* process a custom function definition argument in the form
* "name:type"; the text is expected in a
* conventional handle based evaluator string
* variable
*
******************************************************************************/

static S32
proc_custom_argument(
    PC_U8 text_in,
    P_U8 name_out,
    P_EV_TYPE typep)
{
    PC_U8 ci;
    P_U8 co;
    S32 i;

    for(ci = text_in, co = name_out, i = 0;
        (isalnum(*ci) || *ci == '_') && i < EV_INTNAMLEN;
        ++i, ++ci, ++co)
        *co = (char) tolower(*ci);

    *co++ = '\0';

    if(ident_validate(name_out) < 0)
        return(create_error(EVAL_ERR_BADIDENT));

    #if 0 /* MRJC removed 24.3.92 - now done in ident_validate */
    while(*ci == ' ')
        ++ci;
    #endif

    /* default to real, string, date */
    *typep = EM_REA | EM_STR | EM_DAT;

    /* is there a type following ? */
    if(*ci == ':')
        {
        EV_TYPE type_list;

        type_list = 0;
        do  {
            char type_id[BUF_EV_INTNAMLEN];
            EV_TYPE type_res;

            ++ci;
            while(*ci == ' ')
                ++ci;

            for(co = type_id, i = 0;
                isalnum(*ci) && i < EV_INTNAMLEN;
                ++i, ++ci, ++co)
                *co = (char) tolower(*ci);

            *co++ = '\0';

            if((type_res = type_lookup(type_id)) == 0)
                return(create_error(EVAL_ERR_ARGCUSTTYPE));

            type_list |= type_res;

            while(*ci == ' ')
                ++ci;
            }
        while(*ci == ',');

        *typep = type_list;
        }

    return(0);
}

/******************************************************************************
*
* recognise a constant array
*
* --in--
* refs_ok says whether slot references
* and ranges are allowed in the array
*
* --out--
* < 0 error
* >=0 characters processed
*
******************************************************************************/

static S32
recog_array(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok)
{
    S32 res = 0;
    PC_U8 in_pos = in_str;

    if(*in_pos == '{')
        {
        S32 x, y;

        p_ev_data->did_num = RPN_TMP_ARRAY;
        p_ev_data->arg.ev_array.x_size = 0;
        p_ev_data->arg.ev_array.y_size = 0;
        p_ev_data->arg.ev_array.elements = NULL;

        y = 0;
        do  {
            x = 0;
            if((res = recog_array_row(p_ev_data,
                                      docno_from,
                                      in_pos,
                                      american,
                                      refs_ok,
                                      names_ok,
                                      &x, &y)) >= 0)
                {
                in_pos += res;
                y += 1;
                }
            else
                {
                ss_array_free(p_ev_data);
                break;
                }
            }
        while(*in_pos == ';');
        }

    /* check it's finished correctly */
    if(res > 0 && *in_pos++ != '}')
        res = 0;

    return(res > 0 ? in_pos - in_str : res);
}

/******************************************************************************
*
* recognise a row of an array
*
******************************************************************************/

static S32
recog_array_row(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok,
    P_S32 x,
    P_S32 y)
{
    PC_U8 in_pos = in_str;
    S32 res = 0;

    do  {
        S32 len;
        EV_DATA data;

        /* skip separator */
        ++in_pos;

        /* skip blanks */
        while(*in_pos == ' ')
            ++in_pos;

        data.did_num = RPN_DAT_BLANK;
        if((len = recog_n_dt_str_slr_rng(&data, docno_from, in_pos, american, refs_ok, names_ok)) > 0)
            in_pos += len;
        else if(len < 0)
            {
            res = len;
            break;
            }

        /* can't expand array on subsequent rows */
        if(*y && *x >= p_ev_data_out->arg.ev_array.x_size)
            {
            res = create_error(EVAL_ERR_SUBSCRIPT);
            break;
            }

        /* put data into array */
        if(ss_array_element_make(p_ev_data_out, *x, *y) >= 0)
            *ss_array_element_index_wr(p_ev_data_out, *x, *y) = data;
        else
            {
            res = status_nomem();
            break;
            }

        *x += 1;
        res = 1;

        while(*in_pos == ' ')
            ++in_pos;
        }
    while(*in_pos == ',');

    return(res > 0 ? in_pos - in_str : res);
}

/******************************************************************************
*
* try to recognise date or time
*
* --out--
* =0 no date or time found
* >0 # chars scanned
*
******************************************************************************/

static inline U32
recog_date(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 one,
    _OutRef_    P_S32 two,
    _OutRef_    P_S32 three)
{
    return(recog_dt(spos, one, two, three, '.', 3));
}

static inline U32
recog_time(
    _In_z_      PC_USTR spos,
    _OutRef_    P_S32 one,
    _OutRef_    P_S32 two,
    _OutRef_    P_S32 three)
{
    return(recog_dt(spos, one, two, three, ':', 2));
}

extern S32
ss_recog_date_time(
    _InoutRef_  P_EV_DATA p_ev_data,
    PC_U8 in_str,
    S32 american)
{
    S32 day, month, year, date_scanned;
    S32 hour, minute, second, time_scanned;
    PC_U8 pos = in_str;

    p_ev_data->arg.ev_date.date = EV_DATE_INVALID;
    p_ev_data->arg.ev_date.time = EV_TIME_INVALID;

    /* check for a date */
    if((date_scanned = recog_date(pos, &day, &month, &year)) != 0)
        {
        S32 tres;

        if(year < 100)
            year = sliding_window_year(year);

        day -= 1;
        month -= 1;
        year -= 1;

        if(american)
            tres = ss_ymd_to_dateval(&p_ev_data->arg.ev_date.date, year, day, month);
        else
            tres = ss_ymd_to_dateval(&p_ev_data->arg.ev_date.date, year, month, day);

        if(tres >= 0)
            {
            pos += date_scanned;
            p_ev_data->did_num = RPN_DAT_DATE;
            }
        else
            p_ev_data->arg.ev_date.date = 0;
        }

    /* check for a time */
    second = 0;
    if((time_scanned = recog_time(pos, &hour, &minute, &second)) != 0)
        {
        if(ss_hms_to_timeval(&p_ev_data->arg.ev_date.time, hour, minute, second) >= 0)
            {
            pos += time_scanned;
            p_ev_data->did_num = RPN_DAT_DATE;
            }

        ss_date_normalise(&p_ev_data->arg.ev_date);
        }

    return(pos - in_str);
}

/******************************************************************************
*
* read a three part date or two or three part time value
*
* this exists because of the unreliability of sscanf
*
******************************************************************************/

static U32
recog_dt(
    PC_USTR spos,
    _OutRef_    P_S32 one,
    _OutRef_    P_S32 two,
    _OutRef_    P_S32 three,
    char separator,
    S32 count)
{
    PC_USTR pos = spos;
    P_U8 epos;
    S32 scan_val;
    S32 another;

    *one = *two = *three = 0;

    while(' ' == *pos++)
    { /*EMPTY*/ }
    if(NULLCH == *--pos)
        return(0);

    scan_val = (S32) strtol(pos, &epos, 10);

    if(epos == pos)
        return(0);

    *one = (S32) scan_val;
    --count;

    another = 0;
    if(*epos == separator)
        {
        ++epos;
        another = 1;
        }
    else if(count)
        return(0);

    if(another)
        {
        pos = epos;
        scan_val = (S32) strtol(pos, &epos, 10);

        if(epos == pos)
            return(0);

        *two = (S32) scan_val;
        --count;

        another = 0;
        if(*epos == separator)
            {
            ++epos;
            another = 1;
            }
        else if(count)
            return(0);
        }

    if(another)
        {
        pos = epos;
        scan_val = (S32) strtol(pos, &epos, 10);

        if(epos == pos)
            return(0);

        *three = (S32) scan_val;
        }

    return(epos - spos);
}

/******************************************************************************
*
* read an external document reference
* caller strips spaces
*
* --out--
* number of chars read inc []
*
******************************************************************************/

extern S32
recog_extref(
    _Out_writes_z_(elemof_buffer) P_U8 doc_name,
    _InVal_     S32 elemof_buffer,
    PC_U8 in_str,
    S32 keep_brackets)
{
    S32 buf_siz = elemof_buffer - 1;
    PC_U8 pos = in_str;

    doc_name[0] = NULLCH;

    if(*pos == '[')
        {
        S32 len, name_start;

        len = 0;
        if(keep_brackets)
            doc_name[len++] = '[';

        name_start = len;

        for(pos += 1;
            *pos && *pos != ']' && len < buf_siz;
            ++len, ++pos)
            doc_name[len] = *pos;

        if(*pos++ == ']')
            {
            S32 i, name_end;

            name_end = len;
            if(keep_brackets)
                doc_name[len++] = ']';
            doc_name[len] = NULLCH;

            /* check there's something worthwhile in the brackets */
            for(i = name_start; i < name_end; ++i)
                if(doc_name[i] > ' ')
                    break;

            /* was there owt ? */
            if(i == name_end)
                pos = in_str;
            }
        else
            pos = in_str;
        }

    return(pos - in_str);
}

/******************************************************************************
*
* scan an identifier
*
* --out--
* length of id copied to co
*
******************************************************************************/

static S32
recog_ident(
    P_U8 id_out,
    PC_U8 in_str)
{
    S32 count, id_ch;
    char *co;

    co = id_out;

    /* copy identifier to co, ready for lookup */
    count = 0;
    id_ch = (S32) *in_str;

    while((isalnum(id_ch) || id_ch == '_') && count < EV_INTNAMLEN)
        {
        *co++ = (char) tolower(id_ch);
        id_ch = (S32) *++in_str;
        ++count;
        }

    if(count)
        {
        *co++ = '\0';
        count = ident_validate(id_out);
        }

    return(count);
}

/******************************************************************************
*
* recognise a number, date, string and optionally an slr or range
*
******************************************************************************/

static S32
recog_n_dt_str_slr_rng(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    PC_U8 in_str,
    S32 american,
    S32 refs_ok,
    S32 names_ok)
{
    S32 len;

    if((len = ss_recog_number(p_ev_data, in_str)) == 0 &&
       (len = ss_recog_date_time(p_ev_data, in_str, american)) == 0 &&
       (len = recog_string(p_ev_data, in_str)) == 0)
        {
        if(refs_ok)
            {
            char doc_name[BUF_EV_LONGNAMLEN];

            /* check for external document reference */
            doc_name[0] = '\0';

            if((len = recog_extref(doc_name, elemof32(doc_name), in_str, 1)) >= 0)
                {
                S32 len1;

                len1 = recog_slr_range(p_ev_data, docno_from, in_str + len, doc_name);

                if(len1 > 0)
                    len += len1;
                else if(len1 < 0)
                    len = len1;
                else if(names_ok)
                    {
                    len1 = recog_name(p_ev_data, docno_from, in_str + len, doc_name);

                    if(len1 >= 0)
                        len += len1;
                    else
                        len = len1;
                    }
                }
            }
        }

    return(len);
}

/******************************************************************************
*
* read a name and establish an id for it: this will find a table entry
* for any string which is a valid identifier!
*
******************************************************************************/

static S32
recog_name(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    P_U8 doc_name)
{
    char name_name[BUF_EV_INTNAMLEN];
    S32 len;

    if((len = recog_ident(name_name, in_str)) > 0)
        {
        EV_DOCNO refto_docno;
        EV_NAMEID name_num;

        /* get a target document */
        ev_doc_establish_doc_from_name(doc_name, docno_from, &refto_docno);

        if((name_num = ensure_name_in_list(refto_docno, name_name)) < 0)
            len = 0;
        else
            {
            p_ev_data->did_num = RPN_DAT_NAME;
            p_ev_data->arg.nameid = name_ptr_must(name_num)->key;
            }
        }

    return(len);
}

/******************************************************************************
*
* read a number
*
* --out--
* < 0 no constant found
* >=0 constant found
*
******************************************************************************/

static S32
ss_recog_number(
    _InoutRef_  P_EV_DATA p_ev_data,
    _In_z_      PC_U8Z in_str_in)
{
    S32 res = 0;
    BOOL negative;
    BOOL try_fp;
    PC_U8Z in_str = in_str_in;
    P_U8Z ep;

    /* no need to skip leading spaces as this has been done */

    negative = (*in_str == '-');

    if(negative || (*in_str == '+'))
        ++in_str;

    try_fp = (*in_str == '.');

    if(/*!try_fp &&*/ isdigit(*in_str))
    {
        U32 u32 = fast_strtoul(in_str, &ep);

        if((ep != in_str) && (*ep != ':')) /* ':' -> time */
        {
            try_fp = (*ep == '.') || (*ep == 'e') || (*ep == 'E') || (u32 > (U32) S32_MAX) /*covers U32_MAX ERANGE case*/;

            if(!try_fp)
            {
                ev_data_set_integer(p_ev_data, (negative ? - (S32) u32 : (S32) u32));
                res = PtrDiffBytesS32(ep, in_str_in);
                /*reportf("recog_number(%s) X scanned %d", in_str_in, p_ev_data->arg.integer);*/
            }
            else
            {   /* just quickly check that it's not a date without the strtod overhead (NB Fireworkz doesn't need this step) */
                if((*ep == '.')  &&  isdigit(*(ep + 1)))
                {
#if 0 /* keep safe for when we introduce prescaled FP */
                    /*In fact, given the scanned digits, why not compose FP unless hard? */
                    /* Answer: we would likely get tiny FP differences from before! */
                    static const F64 tens_array[] = { 10.0, 100.0, 1000.0, 10000.0 };
                    U32 frac_u32, frac_digits;
                    PC_U8Z in_str_frac = ep + 1; /* points to first digit of the fraction part */
                    frac_u32 = fast_strtoul(in_str_frac, &ep);
                    if(*ep == '.') /* x.y.something will be date (or at least not a valid number) */
                        try_fp = FALSE;
                    else if((*ep == 'e') || (*ep == 'E') || (frac_u32 > (U32) S32_MAX) /*covers U32_MAX ERANGE case*/)
                        try_fp = TRUE;
                    else
                    {
                        frac_digits = PtrDiffBytesU32(ep, in_str_frac);
                        if(frac_digits > elemof32(tens_array)) /* NB [frac_digits - 1] */
                            try_fp = TRUE;
                        else
                        {
                            F64 f64;
                            try_fp = FALSE;
                            f64 = frac_u32 / tens_array[frac_digits - 1];
                            f64 += u32;
                            ev_data_set_real(p_ev_data, negative ? -f64 : f64);
                            res = PtrDiffBytesS32(ep, in_str_in);
                            /*reportf("recog_number(%s) X.Y scanned %g from %u. %u(%u digits)", in_str_in, p_ev_data->arg.fp, u32, frac_u32, frac_digits);*/
                        }
                    }
#else
                    for(ep = ep + 2; isdigit(*ep); ++ep)
                        /*EMPTY*/;

                    try_fp = (*ep != '.'); /* x.y.something will be date (or at least not a valid number) */
#endif
                }
            }
        }
    }

    /* must have scanned something and not be a date or a real */

    if(try_fp)
    {
        F64 f64 = strtod(in_str, &ep);

        /* must have scanned something and not be a date */
        if((ep != in_str) && (*ep != '.') && (*ep != ':'))
            {
            ev_data_set_real(p_ev_data, negative ? -f64 : f64);
            /*real_to_integer_try(p_ev_data);*//*SKS for t5 1.30 why the hell did it bother? means you cant preserve 1.0 typed in */
            res = PtrDiffBytesS32(ep, in_str_in);
            /*reportf("recog_number(%s) FP scanned %g", in_str_in, p_ev_data->arg.fp);*/
        }
    }

    return(res);
}

/******************************************************************************
*
* read a slot reference
*
* --out--
* =0 no slr found
* >0 number of chars read
*
******************************************************************************/

static S32
recog_slr(
    _InoutRef_  P_EV_SLR slrp,
    PC_U8 in_str)
{
    PC_U8 pos = in_str;
    S32 res;
    S32 row_temp;
    P_U8 epos;

    /* clear slr flags */
    slrp->flags = 0;

    if(*pos == '$')
        {
        ++pos;
        slrp->flags |= SLR_ABS_COL;
        }

    if((res = ev_stox(pos, &slrp->col)) == 0)
        return(res);

    pos += res;

    if(*pos == '$')
        {
        ++pos;
        slrp->flags |= SLR_ABS_ROW;
        }

    /* stop things like + and - being munged
     */
    if(!isdigit(*pos))
        return(0);

    row_temp = (S32) strtol(pos, &epos, 10);

    if(epos == pos)
        return(0);

    if(row_temp > EV_MAX_ROW)
        return(0);

    slrp->row = (EV_ROW) (row_temp - 1);

    return(epos - in_str);
}

/******************************************************************************
*
* read an slr or range
*
* --out--
* =0 no slr or range found
* >0 number of chars read
*
******************************************************************************/

static S32
recog_slr_range(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_DOCNO docno_from,
    _In_z_      PC_U8Z in_str,
    P_U8 doc_name)
{
    EV_SLR s_slr, e_slr;
    S32 len;

    if((len = recog_slr(&s_slr, in_str)) > 0)
        {
        char doc_temp[BUF_EV_LONGNAMLEN];
        S32 len1;
        PC_U8 pos = in_str + len;
        S32 len_colon = 0;

        /* allow foreign range input */
        if(CH_COLON == PtrGetByte(pos))
        {
            len_colon = 1;
            ustr_IncByte(pos);
        }
        else
            len_colon = 0;

        /* check for a second external reference, and
         * allow it if it's the same as the one passed in
         */
        if(doc_name[0] && (len1 = recog_extref(doc_temp, elemof32(doc_temp), pos, 1)) != 0)
            if(0 == _stricmp(doc_name, doc_temp))
                len += len1;

        ev_doc_establish_doc_from_name(doc_name, docno_from, &s_slr.docno);

        /* were we passed an external id ? */
        if(doc_name[0])
            s_slr.flags |= SLR_EXT_REF;

        /* check for another SLR to make range */
        e_slr = s_slr;
        pos = in_str + len;
        if((len1 = recog_slr(&e_slr, pos)) > 0)
            {
            /* set up range */
            e_slr.col += 1;
            e_slr.row += 1;

            if(s_slr.col < e_slr.col &&
               s_slr.row < e_slr.row)
                {
                p_ev_data->arg.range.s = s_slr;
                p_ev_data->arg.range.e = e_slr;
                p_ev_data->did_num = RPN_DAT_RANGE;
                len += len1 + len_colon;
                }
            else
                len = 0;
            }
        else
            {
            p_ev_data->arg.slr = s_slr;
            p_ev_data->did_num = RPN_DAT_SLR;
            }
        }

    return(len);
}

/******************************************************************************
*
* read a string
*
* <0 error
* =0 no string found
* >0 #chars read
*
******************************************************************************/

static S32
recog_string(
    _InoutRef_  P_EV_DATA p_ev_data,
    _In_z_      PC_U8Z in_str)
{
    S32 res = 0;

    /* check for valid start characters */
    if(*in_str == '"')
        {
        PC_U8 ci;
        P_U8 stringp, co;
        S32 len;

        ci = in_str + 1;
        len = 0;
        while(*ci)
            {
            if(*ci == '"')
                {
                ci += 1;

                if(*ci != '"')
                    break;
                }

            ci += 1;
            len += 1;
            }

        /* allocate memory for string */
        if(NULL != (stringp = al_ptr_alloc_bytes(P_U8, len + 1/*NULLCH*/, &res)))
            {
            ci = in_str + 1;
            co = stringp;
            while(*ci)
                {
                if(*ci == '"')
                    {
                    ci += 1;

                    if(*ci != '"')
                        break;
                    }

                *co++ = *ci;
                ci += 1;
                }

            *co = NULLCH;

            p_ev_data->arg.string.data = stringp;
            p_ev_data->arg.string.size = len;
            p_ev_data->did_num = RPN_TMP_STRING;

            res = ci - in_str;
            }
        }

    return(res);
}

/******************************************************************************
*
* <array> := <row> {;<row>}
*
******************************************************************************/

static void
rec_array(
    P_S32 x_size,
    P_S32 y_size)
{
    S32 cur_y;

    cur_y = 0;
    *x_size = 0;

    do  {
        cc->cur.did_num = SYM_BLANK;
        rec_array_row(cur_y, x_size);
        ++cur_y;
        }
    while(scan_check_next(NULL) == SYM_SEMICOLON);

    *y_size = cur_y;
}

/******************************************************************************
*
* <row> := <data> {,<data>}
*
******************************************************************************/

static void
rec_array_row(
    S32 cur_y,
    P_S32 x_size)
{
    S32 cur_x;

    cur_x = 0;

    do  {
        EV_IDNO next;

        cc->cur.did_num = SYM_BLANK;

        next = scan_check_next(NULL);

        if(next == SYM_COMMA     ||
           next == SYM_SEMICOLON ||
           next == SYM_CARRAY)
            out_idno(RPN_DAT_BLANK);
        else
            rec_expr();
        ++cur_x;
        }
    while(scan_check_next(NULL) == SYM_COMMA);

    /* check array x dimensions, and pad with
     * blanks if necessary
     */
    if(!cc->error)
        {
        if(!cur_y)
            *x_size = cur_x;
        else if(cur_x > *x_size)
            set_error(create_error(EVAL_ERR_ARRAYEXPAND));
        else if(cur_x < *x_size)
            {
            S32 i;

            for(i = cur_x; i < *x_size; ++i)
                out_idno(RPN_DAT_BLANK);
            }
        }
}

/******************************************************************************
*
* recognise &
*
******************************************************************************/

static void
rec_aterm(void)
{
    SYM_INF sym_inf;

    rec_bterm();
    while(scan_check_next(&sym_inf) == RPN_BOP_AND)
        {
        P_U8 skip_pos, skip_parm;

        skip_pos = cc->op_pos;
        out_idno(RPN_FRM_SKIPFALSE);
        /* boolean stack offset = 0 */
        out_byte(0);
        skip_parm = cc->op_pos;
        out_S16(0);

        if(!cc->error)
            {
            cc->cur.did_num = SYM_BLANK;
            rec_bterm();
            out_idno_format(&sym_inf);

            /* write in skip parameter */
            writeval_S16(skip_parm, cc->op_pos - skip_pos - sizeof(EV_IDNO));
            }
        }
    return;
}

/******************************************************************************
*
* recognise =, <>, <, >, <=, >=
*
******************************************************************************/

static void
rec_bterm(void)
{
    SYM_INF sym_inf;

    rec_cterm();
    do
        {
        switch(scan_check_next(&sym_inf))
            {
            case RPN_REL_EQUALS:
            case RPN_REL_NOTEQUAL:
            case RPN_REL_LT:
            case RPN_REL_GT:
            case RPN_REL_LTEQUAL:
            case RPN_REL_GTEQUAL:
                cc->cur.did_num = SYM_BLANK;
                break;
            default:
                return;
            }
        rec_cterm();
        out_idno_format(&sym_inf);
        }
    while(TRUE);
}

/******************************************************************************
*
* recognise +, -
*
******************************************************************************/

static void
rec_cterm(void)
{
    SYM_INF sym_inf;

    rec_dterm();
    do
        {
        switch(scan_check_next(&sym_inf))
            {
            case RPN_BOP_PLUS:
            case RPN_BOP_MINUS:
                cc->cur.did_num = SYM_BLANK;
                break;
            default:
                return;
            }
        rec_dterm();
        out_idno_format(&sym_inf);
        }
    while(TRUE);
}

/******************************************************************************
*
* recognise *, /
*
******************************************************************************/

static void
rec_dterm(void)
{
    SYM_INF sym_inf;

    rec_eterm();
    do
        {
        switch(scan_check_next(&sym_inf))
            {
            case RPN_BOP_TIMES:
            case RPN_BOP_DIVIDE:
                cc->cur.did_num = SYM_BLANK;
                break;
            default:
                return;
            }
        rec_eterm();
        out_idno_format(&sym_inf);
        }
    while(TRUE);
}

/******************************************************************************
*
* recognise ^
*
******************************************************************************/

static void
rec_eterm(void)
{
    SYM_INF sym_inf;

    rec_fterm();
    while(scan_check_next(&sym_inf) == RPN_BOP_POWER)
        {
        cc->cur.did_num = SYM_BLANK;
        rec_fterm();
        out_idno_format(&sym_inf);
        }
    return;
}

/******************************************************************************
*
* recognise expression
* works by recursive descent
* recognise |
*
******************************************************************************/

static void
rec_expr(void)
{
    SYM_INF sym_inf;

    rec_aterm();
    while(scan_check_next(&sym_inf) == RPN_BOP_OR)
        {
        P_U8 skip_pos, skip_parm;

        skip_pos = cc->op_pos;
        out_idno(RPN_FRM_SKIPTRUE);
        /* boolean stack offset = 0 */
        out_byte(0);
        skip_parm = cc->op_pos;
        out_S16(0);

        if(!cc->error)
            {
            cc->cur.did_num = SYM_BLANK;
            rec_aterm();
            out_idno_format(&sym_inf);

            writeval_S16(skip_parm, cc->op_pos - skip_pos - sizeof(EV_IDNO));
            }
        }
    return;
}

/******************************************************************************
*
* recognise unary +, -, !
*
******************************************************************************/

static void
rec_fterm(void)
{
    SYM_INF sym_inf;

    switch(scan_check_next(&sym_inf))
        {
        case RPN_BOP_PLUS:
            sym_inf.did_num = RPN_UOP_UPLUS;
            goto proc_op;
        case RPN_BOP_MINUS:
            sym_inf.did_num = RPN_UOP_UMINUS;
        case RPN_UOP_NOT:
        proc_op:
            cc->cur.did_num = SYM_BLANK;
            rec_fterm();
            out_idno_format(&sym_inf);
            return;
        default:
            rec_gterm();
            return;
        }
}

/******************************************************************************
*
* recognise lterm or brackets
*
******************************************************************************/

static void
rec_gterm(void)
{
    SYM_INF sym_inf;

    if(scan_check_next(&sym_inf) == SYM_OBRACKET)
        {
        cc->cur.did_num = SYM_BLANK;
        rec_expr();
        if(scan_check_next(NULL) != SYM_CBRACKET)
            set_error(create_error(EVAL_ERR_CBRACKETS));
        else
            {
            cc->cur.did_num = SYM_BLANK;
            sym_inf.did_num = RPN_FRM_BRACKETS;
            out_idno_format(&sym_inf);
            }
        }
    else
        rec_lterm();
}

/******************************************************************************
*
*   <lterm> := number        |
*              date          |
*              slr           |
*              range         |
*              string        |
*              '{'<array>'}' |
*              <function>    |
*              <custom>      |
*              <name>
*
******************************************************************************/

static void
rec_lterm(void)
{
    SYM_INF sym_inf;

    switch(scan_check_next(&sym_inf))
        {
        case RPN_DAT_REAL:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_to_rpn(sizeof32(F64), &cc->cur_sym.arg.fp);
            break;

        case RPN_DAT_WORD8:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_byte((char) cc->cur_sym.arg.integer);
            break;

        case RPN_DAT_WORD16:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_S16((S16) cc->cur_sym.arg.integer);
            break;

        case RPN_DAT_WORD32:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_to_rpn(sizeof32(S32), &cc->cur_sym.arg.integer);
            break;

        case RPN_DAT_SLR:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_slr(&cc->cur_sym.arg.slr);
            break;

        case RPN_DAT_RANGE:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_range(&cc->cur_sym.arg.range);
            break;

        case RPN_TMP_STRING:
            cc->cur.did_num = SYM_BLANK;
            out_string_free(&sym_inf, &cc->cur_sym.arg.string_wr.data);
            break;

        case RPN_DAT_DATE:
            cc->cur.did_num = SYM_BLANK;
            out_idno_format(&sym_inf);
            out_to_rpn(sizeof32(EV_DATE), &cc->cur_sym.arg.ev_date);
            break;

        case SYM_OARRAY:
            {
            S32 x_size, y_size;

            cc->array_nest += 1;

            if(cc->array_nest == 1)
                rec_array(&x_size, &y_size);

            cc->array_nest -= 1;

            if(cc->array_nest)
                {
                set_error(create_error(EVAL_ERR_NESTEDARRAY));
                break;
                }

            if(scan_check_next(NULL) != SYM_CARRAY)
                {
                set_error(create_error(EVAL_ERR_CARRAY));
                break;
                }

            /* array has been scanned */
            cc->cur.did_num = SYM_BLANK;

            /* output array details to compiled expression */
            sym_inf.did_num = RPN_FNA_MAKEARRAY;
            out_idno_format(&sym_inf);

            /* output array sizes */
            out_to_rpn(sizeof32(S32), &x_size);
            out_to_rpn(sizeof32(S32), &y_size);

            break;
            }

        case RPN_LCL_ARGUMENT:
            {
            P_U8 ci;

            out_idno_format(&sym_inf);

            ci = cc->ident;
            do
                out_byte(*ci);
            while(*ci++);

            cc->cur.did_num = SYM_BLANK;
            break;
            }

        case SYM_TAG:
            {
            EV_DOCNO refto_docno;
            char ident[BUF_EV_INTNAMLEN];

            ev_doc_establish_doc_from_name(cc->doc_name, cc->docno, &refto_docno);
            strcpy(ident, cc->ident);

            /* we scanned the tag */
            cc->cur.did_num = SYM_BLANK;

            /* lookup id as internal function */
            if(!cc->doc_name[0])
                {
                S32 int_func;

                if((int_func = func_lookup(ident)) >= 0)
                    {
                    sym_inf.did_num = (EV_IDNO) int_func;
                    func_call(&sym_inf, 0);
                    break;
                    }
                 }

            /* is it a custom function call ? */
            if(scan_check_next(NULL) == SYM_OBRACKET)
                {
                /* must be a custom function - establish reference */
                EV_NAMEID custom_num = ensure_custom_in_list(refto_docno, ident);
                P_EV_CUSTOM p_ev_custom;

                if(custom_num < 0)
                    {
                    set_error(status_nomem());
                    break;
                    }

                p_ev_custom = custom_ptr_must(custom_num);
                sym_inf.did_num = RPN_FNM_CUSTOMCALL;
                func_call(&sym_inf, p_ev_custom->key);
                }
            else
                {
                EV_NAMEID name_num = ensure_name_in_list(refto_docno, ident);
                P_EV_NAME p_ev_name;

                if(name_num < 0)
                    {
                    set_error(status_nomem());
                    break;
                    }

                p_ev_name = name_ptr_must(name_num);
                sym_inf.did_num = RPN_DAT_NAME;
                out_idno_format(&sym_inf);
                out_nameid(p_ev_name->key);

                cc->parms.var = 1;
                }

            break;
            }

        default:
            set_error(create_error(EVAL_ERR_BADEXPR));
            break;
        }
}

/******************************************************************************
*
* return information about the next symbol -
* scan a new symbol if we have used up the stored one
*
******************************************************************************/

static EV_IDNO
scan_check_next(
    P_SYM_INF p_sym_inf)
{
    if(cc->cur.did_num == SYM_BLANK)
        {
        cc->err_pos = cc->ip_pos;
        scan_next_symbol();
        }

    if(p_sym_inf)
        *p_sym_inf = cc->cur;

    return(cc->cur.did_num);
}

/******************************************************************************
*
* scan next symbol from the text input
*
* <0  bad symbol encountered
* >=0 symbol scanned OK
*
******************************************************************************/

static void
scan_next_symbol(void)
{
    S32 count, res;

    /* accumulate CRs and space */
    cc->cur.sym_cr = 0;
    do  {
        if(*cc->ip_pos == CR)
            {
            ++cc->ip_pos;
            if(*cc->ip_pos == LF)
                ++cc->ip_pos;
            ++cc->cur.sym_cr;
            }
        else if(*cc->ip_pos == LF)
            {
            ++cc->ip_pos;
            if(*cc->ip_pos == CR)
                ++cc->ip_pos;
            ++cc->cur.sym_cr;
            }

        cc->cur.sym_space = 0;
        while(*cc->ip_pos == ' ')
            {
            ++cc->ip_pos;
            ++cc->cur.sym_space;
            }
        }
    while(*cc->ip_pos == CR ||
          *cc->ip_pos == LF);

    /* try to scan a data item */
    if(*cc->ip_pos != '-' &&
       *cc->ip_pos != '+')
        {
        /* check for number */
        if((res = ss_recog_number(&cc->cur_sym, cc->ip_pos)) > 0)
            cc->ip_pos += res;
        /* check for date and/or time */
        else if((res = ss_recog_date_time(&cc->cur_sym, cc->ip_pos, cc->p_optblock->american_date)) > 0)
            cc->ip_pos += res;
        /* read a string ? */
        else if((res = recog_string(&cc->cur_sym, cc->ip_pos)) > 0)
            cc->ip_pos += res;

        if(res > 0)
            {
            cc->cur.did_num = cc->cur_sym.did_num;
            return;
            }
        }

    /* clear external ref buffer */
    cc->doc_name[0] = '\0';

    /* check for internal slr or range */
    if((res = recog_slr_range(&cc->cur_sym, cc->docno, cc->ip_pos, cc->doc_name)) > 0)
        {
        cc->ip_pos += res;
        cc->parms.var = 1;
        cc->cur.did_num = cc->cur_sym.did_num;
        return;
        }

    /* check for special symbols */
    switch(*cc->ip_pos)
        {
        case '\0':
            cc->cur.did_num = RPN_FRM_END;
            return;
        case '(':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_OBRACKET;
            return;
        case ')':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_CBRACKET;
            return;
        case ',':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_COMMA;
            return;
        case ';':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_SEMICOLON;
            return;
        case '{':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_OARRAY;
            return;
        case '}':
            ++cc->ip_pos;
            cc->cur.did_num = SYM_CARRAY;
            return;

        /* local name */
        case '@':
            if((res = recog_ident(cc->ident, ++cc->ip_pos)) <= 0)
                {
                set_error(create_error(EVAL_ERR_BADIDENT));
                return;
                }

            cc->ip_pos += res;
            cc->cur.did_num = RPN_LCL_ARGUMENT;
            return;

        default:
            break;
        }

    /* check for external document reference */
    if((res = recog_extref(cc->doc_name, BUF_EV_LONGNAMLEN, cc->ip_pos, 1)) > 0)
        {
        cc->ip_pos += res;

        if((res = recog_slr_range(&cc->cur_sym, cc->docno, cc->ip_pos, cc->doc_name)) > 0)
            {
            cc->ip_pos += res;
            cc->parms.var = 1;
            cc->cur.did_num = cc->cur_sym.did_num;
            return;
            }
        }
    else if(res < 0)
        {
        set_error(res);
        return;
        }

    /* scan an id */
    if(isalnum(*cc->ip_pos) || *cc->ip_pos == '_' || cc->doc_name[0])
        {
        if((res = recog_ident(cc->ident, cc->ip_pos)) <= 0)
            {
            set_error(res ? res : create_error(EVAL_ERR_BADIDENT));
            return;
            }

        cc->ip_pos += res;
        cc->cur.did_num = SYM_TAG;
        return;
        }
    /* must be an operator or a dud */
    else if(ispunct(*cc->ip_pos))
        {
        P_U8 co;
        S32 cur_ch;

        co = cc->ident;
        count = 0;
        cur_ch = *cc->ip_pos;
        while(ispunct(cur_ch) && count < EV_INTNAMLEN)
            {
            *co++  = cur_ch;
            cur_ch = *(++cc->ip_pos);
            ++count;

            switch(cur_ch)
                {
                case '>':
                case '<':
                case '=':
                    continue;

                default:
                    break;
                }

            break;
            }

        *co++ = '\0';

        if(count)
            if((cc->cur.did_num = func_lookup(cc->ident)) >= 0)
                return;
        }

    set_error(create_error(EVAL_ERR_BADEXPR));
}

/******************************************************************************
*
* set compiler error condition
*
******************************************************************************/

static S32
set_error(
    S32 err)
{
    cc->cur.did_num = SYM_BAD;
    if(!cc->error)
        cc->error = err;

    return(-1);
}

/******************************************************************************
*
* convert column string into column number
*
* --out--
* number of character scanned
*
******************************************************************************/

static S32
ev_stox(
    PC_U8 string,
    EV_COL * col)
{
    S32 scanned, i;

    i = toupper(*string) - 'A';
    ++string;
    scanned = 0;

    if(i >= 0 && i <= 25)
        {
        EV_COL tcol;

        tcol    = i;
        scanned = 1;
        do  {
            i = toupper(*string) - 'A';
            ++string;
            if(i < 0 || i > 25)
                break;
            tcol = (tcol + 1) * 26 + i;
            }
        while(++scanned < EV_MAX_COL_LETS);

        *col = tcol;
        }

    return(scanned);
}

/* end of ev_comp.c */
