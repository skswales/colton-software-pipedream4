/* ev_dcom.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* RPN to infix decompiler */

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

static S32
dec_popptr(
    P_U8 *ptrp);

static S32
fptostr(
    P_U8 op_buf,
    PC_F64 fpval);

/*
* decompiler variables
*/

#define MAXSTACK 100

typedef struct decompiler_context
{
    EV_DOCNO        docno;          /* document number of decompile data */

    P_P_U8          arg_stk;        /* pointer to argument stack */
    S32             arg_stk_siz;    /* current size of argument stack */
    S32             arg_sp;         /* argument stack pointer */

    EV_DATA         cur_sym;        /* current symbol */
    SYM_INF         cur;            /* information about current rpn item */

    struct rpnstate cur_rpn;        /* rpn state */

    PC_EV_OPTBLOCK  p_optblock;     /* pointer to options block */
}
* decomp_contp;

static decomp_contp dc = NULL;

/******************************************************************************
*
* convert date/time value to a string
*
******************************************************************************/

static S32
date_time_val_to_str(
    P_U8 op_buf,
    _InRef_     PC_EV_DATE datep,
    S32 american)
{
    S32 len;
    EV_DATE temp;

    len = 0;
    temp = *datep;

    if(temp.date)
        {
        S32 day, month, year;

        if(temp.date < 0)
            {
            op_buf[len++] = '-';
            temp.date = -temp.date;
            }

        ss_dateval_to_ymd(&temp.date, &year, &month, &day);

        if(american)
            len += sprintf(op_buf + len, "%d.%d.%d", month + 1, day + 1, year + 1);
        else
            len += sprintf(op_buf + len, "%d.%d.%d", day + 1, month + 1, year + 1);
        }

    if(!temp.date || temp.time)
        {
        S32 hour, minute, second;

        /* separate time from date */
        if(temp.date)
            op_buf[len++] = ' ';

        if(temp.time < 0)
            {
            op_buf[len++] = '-';
            temp.time = -temp.time;
            }

        ss_timeval_to_hms(&temp.time, &hour, &minute, &second);

        len += sprintf(op_buf + len, "%.2d:%.2d", hour, minute);

        if(second)
            len += sprintf(op_buf + len, ":%.2d", second);
        }

    return(len);
}

/******************************************************************************
*
* decompile current symbol
*
* --out--
* length of string
*
******************************************************************************/

static S32
dec_const(
    P_U8 op_buf,
    _InRef_     PC_EV_DATA p_ev_data)
{
    S32 len;

    switch(p_ev_data->did_num)
        {
        case RPN_DAT_REAL:
            len = fptostr(op_buf, &p_ev_data->arg.fp);
            break;

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            len = sprintf(op_buf, "%d", p_ev_data->arg.integer);
            break;

        case RPN_DAT_SLR:
            len = ev_dec_slr(op_buf, dc->docno, &p_ev_data->arg.slr, dc->p_optblock->upper_case_slr);
            break;

        case RPN_DAT_RANGE:
            len = ev_dec_range(op_buf, dc->docno, &p_ev_data->arg.range, dc->p_optblock->upper_case_slr);
            break;

        case RPN_RES_STRING:
        case RPN_DAT_STRING:
        case RPN_TMP_STRING:
            {
            P_U8 ci, co;

            co = op_buf;
            ci = p_ev_data->arg.string.data;
            *co++ = '"';
            while(*ci)
                {
                if(*ci == '"')
                    *co++ = '"';
                *co++ = *ci++;
                }
            *co++ = '"';
            len = co - op_buf;
            break;
            }

        case RPN_DAT_DATE:
            len = date_time_val_to_str(op_buf, &p_ev_data->arg.date, dc->p_optblock->american_date);
            break;

        case RPN_RES_ARRAY:
        case RPN_TMP_ARRAY:
            {
            S32 ix, iy;

            *op_buf = '{';
            len = 1;

            for(iy = 0; iy < p_ev_data->arg.array.y_size; ++iy)
                {
                for(ix = 0; ix < p_ev_data->arg.array.x_size; ++ix)
                    {
                    len += dec_const(op_buf + len, ss_array_element_index_borrow(p_ev_data, ix, iy));

                    if(ix + 1 < p_ev_data->arg.array.x_size)
                        op_buf[len++] = ',';
                    }

                if(iy + 1 < p_ev_data->arg.array.y_size)
                    op_buf[len++] = ';';
                }

            op_buf[len++] = '}';

            break;
            }

        case RPN_DAT_NAME:
            {
            EV_NAMEID name_num;

            if((name_num = name_def_find(p_ev_data->arg.nameid)) >= 0)
                {
                P_EV_NAME p_ev_name;

                p_ev_name = name_ptr(name_num);
                if(p_ev_name->owner.docno != dc->docno)
                    len = ev_write_docname(op_buf, p_ev_name->owner.docno, dc->docno);
                else
                    len = 0;
                strcpy(op_buf + len, p_ev_name->id);
                len += strlen(p_ev_name->id);
                }
            else
                len = 0;

            break;
            }

        default:
        case RPN_DAT_BLANK:
            len = 0;
            break;
        }

    return(len);
}

/******************************************************************************
*
* insert formatting space and
* returns into output buffer
*
******************************************************************************/

static S32
dec_format_space(
    P_U8 buf_in)
{
    P_U8 ptr;

    ptr = buf_in;

    while(dc->cur.sym_cr)
        {
        --dc->cur.sym_cr;
        if(dc->p_optblock->cr)
            *ptr++ = CR;
        if(dc->p_optblock->lf)
            *ptr++ = LF;
        }
    while(dc->cur.sym_space)
        {
        --dc->cur.sym_space;
        *ptr++ = ' ';
        }

    return(ptr - buf_in);
}

/******************************************************************************
*
* free all elements from the decompiler stack
*
******************************************************************************/

static void
dec_freestk(void)
{
    P_U8 ptr;

    /* free any items on the stack */
    while(dec_popptr(&ptr) >= 0)
        al_ptr_dispose(P_P_ANY_PEDANTIC(&ptr));

    /* free the stack itself */
    if(dc->arg_stk)
        {
        al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&dc->arg_stk));
        dc->arg_stk_siz = 0;
        dc->arg_sp      = 0;
        }
}

/******************************************************************************
*
* pop a pointer from the decompiler stack
*
******************************************************************************/

static S32
dec_popptr(
    P_U8 *ptrp)
{
    if(!dc->arg_sp)
        return(-1);

    *ptrp = dc->arg_stk[--dc->arg_sp];
    return(0);
}

/******************************************************************************
*
* pop a string from the decompiler stack
* the output buffer had better be big enough!
*
* --out--
* length of copied string
*
******************************************************************************/

static S32
dec_popstr(
    P_U8 op_buf)
{
    P_U8 stringp;

    if(dec_popptr(&stringp) < 0)
        return(-1);

    strcpy(op_buf, stringp);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&stringp));
    return(strlen(op_buf));
}

/******************************************************************************
*
* push a decompiler string argument
*
******************************************************************************/

static S32
dec_pushstr(
    P_U8 arg,
    S32 len)
{
    STATUS status;
    P_U8 newp;

    /* make stack big enough */
    if(dc->arg_sp == dc->arg_stk_siz)
        {
        P_P_U8 newsp;

        if(NULL == (newsp = al_ptr_realloc_elem(P_U8, dc->arg_stk, dc->arg_stk_siz + 50, &status)))
            return(status);

        dc->arg_stk      = newsp;
        dc->arg_stk_siz += 50;
        }

    /* allocate space for argument */
    if(NULL == (newp = al_ptr_alloc_bytes(U8, len + 1/*NULLCH*/, &status)))
        return(status);

    /* copy argument into block */
    memcpy32(newp, arg, len);
    newp[len] = NULLCH;

    dc->arg_stk[dc->arg_sp++] = newp;

    return(0);
}

/******************************************************************************
*
* convert RNG to string
*
* --out--
* length of string rng
*
******************************************************************************/

/* SKS derived 02feb92 from dec_const for export */

extern S32
ev_dec_range(
    P_U8 op_buf,
    _InVal_     EV_DOCNO this_docno,
    _InRef_     PC_EV_RANGE p_ev_range,
    BOOL upper_case)
{
    EV_SLR slr;
    S32    len;

    len = ev_dec_slr(op_buf, this_docno, &p_ev_range->s, upper_case);

    slr = p_ev_range->e;
    slr.docno = 0;
    slr.col -= 1;
    slr.row -= 1;

    len += ev_dec_slr(op_buf + len, this_docno, &slr, upper_case);

    return(len);
}

/******************************************************************************
*
* take current rpn token details and
* decompile, combining with arguments on
* the stack; op_buf contains new argument;
*
* --out--
* length of resulting argument
*
******************************************************************************/

static S32
dec_rpn_token(
    P_U8 op_buf,
    P_S32 out_len)
{
    S32 res, len;

    res = 0;
    len = -1;
    switch(rpn_table[dc->cur_rpn.num].rpn_type)
        {
        case RPN_DAT:
            {
            /* read rpn argument */
            read_cur_sym(&dc->cur_rpn, &dc->cur_sym);
            len  = dec_format_space(op_buf);
            len += dec_const(op_buf + len, &dc->cur_sym);
            break;
            }

        case RPN_FRM:
            {
            switch(dc->cur.did_num)
                {
                /* brackets must be added to top argument on stack */
                case RPN_FRM_BRACKETS:
                    {
                    S32 pop_len;

                    len = dec_format_space(op_buf);

                    op_buf[len++] = '(';

                    if((pop_len = dec_popstr(op_buf + len)) < 0)
                        res = create_error(EVAL_ERR_BADRPN);
                    else
                        {
                        len += pop_len;
                        op_buf[len++] = ')';
                        }
                    break;
                    }

                case RPN_FRM_SPACE:
                    dc->cur.sym_space = *(dc->cur_rpn.pos + 1);
                    len = -1;
                    break;

                case RPN_FRM_RETURN:
                    dc->cur.sym_cr    = *(dc->cur_rpn.pos + 1);
                    len = -1;
                    break;

                case RPN_FRM_END:
                    len = -1;
                    break;

                case RPN_FRM_COND:
                    {
                    S32 pop_len;

                    len = dec_format_space(op_buf);

                    if((pop_len = ev_decompile(dc->docno, op_buf + len,
                                               dc->cur_rpn.pos + 3,
                                               dc->p_optblock)) < 0)
                        {
                        strcpy(op_buf + len, DECOMP_ERR);
                        pop_len = strlen(DECOMP_ERR);
                        }

                    len += pop_len;
                    break;
                    }

                case RPN_FRM_SKIPTRUE:
                case RPN_FRM_SKIPFALSE:
                case RPN_FRM_NODEP:
                    len = -1;
                    break;
                }
            break;
            }

        case RPN_LCL:
            {
            PC_U8 ci;

            len = dec_format_space(op_buf);

            op_buf[len++] = '@';

            ci = dc->cur_rpn.pos + 1;
            while(*ci)
                op_buf[len++] = *ci++;

            break;
            }

        case RPN_UOP:
            {
            S32 pop_len, fun_len;
            PC_USTR fname;

            len     = dec_format_space(op_buf);
            fname   = func_name(dc->cur.did_num);
            fun_len = strlen(fname);
            strcpy(op_buf + len, fname);
            len    += fun_len;

            if((pop_len = dec_popstr(op_buf + len)) < 0)
                res = create_error(EVAL_ERR_BADRPN);
            else
                len += pop_len;
            break;
            }

        case RPN_REL:
        case RPN_BOP:
            {
            S32 len1, fun_len, len2;
            P_U8 str2;

            if(dec_popptr(&str2) < 0 ||
               (len1 = dec_popstr(op_buf)) < 0)
                res = create_error(EVAL_ERR_BADRPN);
            else
                {
                PC_USTR fname;

                len = len1;
                len += dec_format_space(op_buf + len);

                fname = func_name(dc->cur.did_num);
                fun_len = strlen(fname);
                strcpy(op_buf + len, fname);
                len += fun_len;

                len2 = strlen(str2);
                strcpy(op_buf + len, str2);
                al_ptr_dispose(P_P_ANY_PEDANTIC(&str2));
                len += len2;
                }

            break;
            }

        case RPN_FN0:
            {
            PC_USTR fname;
            S32 fun_len;

            len = dec_format_space(op_buf);

            fname = func_name(dc->cur.did_num);
            fun_len = strlen(fname);
            strcpy(op_buf + len, fname);
            len += fun_len;
            break;
            }

        case RPN_FNF:
        case RPN_FNV:
        case RPN_FNM:
            {
            S32 narg, fun_len, first;

            len = dec_format_space(op_buf);

            /* work out number of arguments */
            if((narg = rpn_table[dc->cur_rpn.num].nargs) < 0)
                narg = (S32) *(dc->cur_rpn.pos + 1);

            /* copy in custom/function name */
            if(dc->cur.did_num == RPN_FNM_CUSTOMCALL)
                {
                EV_NAMEID custom_id, custom_num;
                P_EV_CUSTOM p_ev_custom;

                /* read custom id */
                read_nameid(&custom_id, dc->cur_rpn.pos + 2);
                custom_num = custom_def_find(custom_id);

                p_ev_custom = custom_ptr(custom_num);

                if(p_ev_custom->owner.docno != dc->docno)
                    len += ev_write_docname(op_buf + len,
                                            p_ev_custom->owner.docno,
                                            dc->docno);

                strcpy(op_buf + len, p_ev_custom->id);
                len += strlen(p_ev_custom->id);
                }
            else
                {
                PC_USTR fname;

                fname = func_name(dc->cur.did_num);
                fun_len = strlen(fname);
                strcpy(op_buf + len, fname);
                len += fun_len;
                }

            fun_len = len;
            first   = 1;
            if(narg || dc->cur.did_num == RPN_FNM_CUSTOMCALL)
                {
                op_buf[fun_len++] = '(';
                op_buf[fun_len]   = ')';
                len               = fun_len + 1;
                }

            /* loop to add arguments */
            while(narg--)
                {
                S32 sep_len, arg_len;
                P_U8 stringp;

                if(dec_popptr(&stringp) < 0)
                    res = create_error(EVAL_ERR_BADRPN);
                else
                    {
                    arg_len = strlen(stringp);
                    sep_len = arg_len + (first ? 0 : 1);

                    memmove32(op_buf + fun_len + sep_len,
                              op_buf + fun_len,
                              len - fun_len);

                    memmove32(op_buf + fun_len, stringp, arg_len);

                    al_ptr_dispose(P_P_ANY_PEDANTIC(&stringp));
                    len += sep_len;
                    if(!first)
                        op_buf[fun_len + arg_len] = ',';
                    first = 0;
                    }
                }

            break;
            }

        case RPN_FNA:
            {
            S32 x_size, y_size;
            S32 x, y, sp, startsp;

            len = dec_format_space(op_buf);

            x_size = readval_S32(dc->cur_rpn.pos + 1);
            y_size = readval_S32(dc->cur_rpn.pos + 1 + sizeof(S32));

            op_buf[len++] = '{';

            sp = startsp = dc->arg_sp - (S32) (x_size * y_size);

            for(y = 0; y < (S32) y_size; ++y)
                {
                for(x = 0; x < (S32) x_size; ++x)
                    {
                    strcpy(op_buf + len, dc->arg_stk[sp]);
                    len += strlen(dc->arg_stk[sp]);
                    al_ptr_dispose(P_P_ANY_PEDANTIC(&dc->arg_stk[sp]));
                    op_buf[len++] = ',';
                    ++sp;
                    }

                op_buf[len - 1] = ';';
                }

            op_buf[len - 1] = '}';

            dc->arg_sp = startsp;

            break;
            }

        default:
            res = create_error(EVAL_ERR_INTERNAL);
            break;
        }

    *out_len = len;

    return(res);
}

/******************************************************************************
*
* convert SLR to string
*
* --out--
* length of string slr
*
******************************************************************************/

extern S32
ev_dec_slr(
    P_U8 op_buf,
    _InVal_     EV_DOCNO this_docno,
    _InRef_     PC_EV_SLR slrp,
    BOOL upper_case)
{
    P_U8 op_pos;

    op_pos = op_buf;

    if(slrp->docno != this_docno || (slrp->flags & SLR_EXT_REF))
        op_pos += ev_write_docname(op_buf, slrp->docno, this_docno);

    if(slrp->flags & SLR_BAD_REF)
        *op_pos++ = '%';

    if(slrp->flags & SLR_ABS_COL)
        *op_pos++ = '$';

    op_pos += xtos_ubuf(op_pos, BUF_EV_INTNAMLEN, slrp->col, upper_case);

    if(slrp->flags & SLR_ABS_ROW)
        *op_pos++ = '$';

    op_pos += sprintf(op_pos, "%d", slrp->row + 1);

    return(op_pos - op_buf);
}

/******************************************************************************
*
* decode a data item to text
*
******************************************************************************/

extern S32
ev_decode_data(
    P_U8 txt_out,
    _InVal_     EV_DOCNO docno_from,
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_OPTBLOCK p_optblock)
{
    struct decompiler_context decomp_cont;
    decomp_contp old_dc;
    S32 len;

    old_dc = dc;
    dc = &decomp_cont;

    /* save initial data */
    dc->docno = docno_from;
    dc->p_optblock = p_optblock;

    len = dec_const(txt_out, p_ev_data);
    txt_out[len] = '\0';

    dc = old_dc;
    return(len);
}

/******************************************************************************
*
* given a pointer to an expression slot, return a textual form for it
*
******************************************************************************/

extern S32
ev_decode_slot(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_out,
    P_EV_SLOT p_ev_slot,
    _InRef_     PC_EV_OPTBLOCK p_optblock)
{
    S32 res;

    switch(p_ev_slot->parms.type)
        {
        case EVS_CON_DATA:
            {
            EV_DATA data;

            ev_result_to_data_convert(&data, &p_ev_slot->result);
            ev_decode_data(txt_out, docno, &data, p_optblock);
            res = 0;
            break;
            }

        case EVS_CON_RPN:
            res = ev_decompile(docno, txt_out, p_ev_slot->rpn.con.rpn_str, p_optblock);
            break;

        case EVS_VAR_RPN:
            res = ev_decompile(docno, txt_out, p_ev_slot->rpn.var.rpn_str, p_optblock);
            break;

        default:
            res = create_error(EVAL_ERR_INTERNAL);
            break;
        }

    return(res);
}

/******************************************************************************
*
* decompile from RPN to plain text
*
******************************************************************************/

extern S32
ev_decompile(
    _InVal_     EV_DOCNO docno,
    P_U8 txt_out,
    PC_U8 rpn_in,
    _InRef_     PC_EV_OPTBLOCK p_optblock)
{
    S32 len, res;
    struct decompiler_context decomp_cont;
    decomp_contp old_dc;
    char op_buf[EV_MAX_OUT_LEN];

    /* set up decompiler context */
    old_dc = dc;
    dc = &decomp_cont;

    /* save initial data */
    dc->docno       = docno;
    dc->cur_rpn.pos = rpn_in;
    dc->p_optblock  = p_optblock;

    /* clear stack info */
    dc->arg_stk     = 0;
    dc->arg_sp      = 0;
    dc->arg_stk_siz = 0;

    /* set up symbol info */
    dc->cur.sym_space = 0;
    dc->cur.sym_cr    = 0;
    dc->cur.did_num   = rpn_check(&dc->cur_rpn);

    do  {
        if((res = dec_rpn_token(op_buf, &len)) < 0)
            break;

        if((dc->cur.did_num = rpn_skip(&dc->cur_rpn)) == RPN_FRM_END)
            break;

        if(len >= 0)
            res = dec_pushstr(op_buf, len);
        }
    while(!res);

    if(dc->arg_sp && !res)
        res = create_error(EVAL_ERR_BADRPN);

    if(!res)
        {
        S32 temp_len;
        P_U8 ci, co;

        for(temp_len = len, ci = op_buf, co = txt_out;
            temp_len;
            --temp_len, ++ci, ++co)
            if(dc->p_optblock->upper_case)
                *co = (char) toupper(*ci);
            else
                *co = *ci;

        *co = '\0';
        res = len;
        }

    /* release decompiler stack and contents */
    dec_freestk();

    /* restore old context */
    dc = old_dc;

    return(res);
}

/******************************************************************************
*
* convert floating point number to string
*
******************************************************************************/

static S32
fptostr(
    P_U8 op_buf,
    PC_F64 fpval)
{
    P_U8 exp;
    S32 len;

    len = sprintf(op_buf, "%.15g", *fpval);
    op_buf[len] = '\0';

    /* search for exponent and remove leading zeroes because
    they are confusing; remove the + for good measure */
    if((exp = strstr(op_buf, "e")) != NULL)
        {
        char sign;
        P_U8 exps;

        sign = *(++exp);
        exps = exp;
        if(sign == '-')
            {
            ++exp;
            ++exps;
            }

        if(sign == '+')
            ++exp;

        while(*exp == '0')
            ++exp;

        strncpy(exps, exp, len - (exp - op_buf));
        len = len - (exp - exps);
        op_buf[len] = '\0';
        }

    return(len);
}

/* end of ev_dcom.c */
