/* ev_rpn.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines that traverse expression rpn strings */

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

static void
read_date(
    _OutRef_    P_SS_DATE datep,
    PC_U8 ip_pos);

/*
macros to select efficient reading routines
*/

#if EV_COL_BITS == 32
#define read_ev_col(pos) (EV_COL) readval_U32((pos))
#else
#define read_ev_col(pos) (EV_COL) readuword_LE((pos), sizeof32(EV_COL))
#endif

#if EV_DOCNO_BITS == 8
#define read_ev_docno(pos) (EV_DOCNO) (* ((PC_U8) pos))
#else
#define read_ev_docno(pos) (EV_DOCNO) readuword_LE((pos), sizeof32(EV_DOCNO))
#endif

#if EV_FLAGS_BITS == 8
#define read_ev_flags(pos) (EV_FLAGS) (* ((PC_U8) pos))
#else
#define read_ev_flags(pos) (EV_FLAGS) readuword_LE((pos), sizeof32(EV_FLAGS))
#endif

#if EV_ROW_BITS == 32
#define read_ev_row(pos) (EV_ROW) readval_U32((pos))
#else
#define read_ev_row(pos) (EV_ROW) readuword_LE((pos), sizeof32(EV_ROW))
#endif

/******************************************************************************
*
* initialise grubber block for a cell
*
******************************************************************************/

extern void
grub_init(
    _InoutRef_  P_EV_GRUB grubp,
    _InRef_     PC_EV_SLR slrp)
{
    grubp->slr     = *slrp;
    grubp->offset  = 0;
    grubp->in_cond = 0;
}

/******************************************************************************
*
* grub about in a cell and
* return the cells, ranges and names
* on which a cell depends
*
* --out--
* rpn number of element returned
* RPN_FRM_END returned at end
*
******************************************************************************/

extern S32
grub_next(
    P_EV_CELL p_ev_cell,
    P_EV_GRUB grubp)
{
    S32 res, data_offset, nodep;
    PC_U8 rpn_start, at_pos;
    RPNSTATE cur_rpn;

    switch(p_ev_cell->parms.type)
    {
    case EVS_CON_DATA:
        rpn_start = at_pos = NULL;
        res = RPN_FRM_END;
        break;
    case EVS_CON_RPN:
        rpn_start = p_ev_cell->rpn.con.rpn_str;
        res = -1;
        break;
    case EVS_VAR_RPN:
        rpn_start = p_ev_cell->rpn.var.rpn_str;
        res = -1;
        break;
    }

    if(rpn_start)
    {
        cur_rpn.pos = rpn_start + grubp->offset;
        rpn_check(&cur_rpn);
        data_offset = 1;
        nodep = 0;
    }

    while(res < 0)
    {
        at_pos = cur_rpn.pos;

        switch(cur_rpn.num)
        {
        case DATA_ID_SLR:
        case DATA_ID_RANGE:
        case DATA_ID_NAME:
            read_cur_sym(&cur_rpn, &grubp->data);
            if(nodep)
                res = RPN_FRM_NODEP;
            else
                res = cur_rpn.num;
            break;

        /* make sure we grub about in the
        condition string as well */
        case RPN_FRM_COND:
            cur_rpn.pos += 1 + sizeof(S16);
            rpn_check(&cur_rpn);
            grubp->in_cond = 1;
            continue;

        /* next data item to be ignored for dependencies */
        case RPN_FRM_NODEP:
            /* set nodep flag */
            nodep = 1;
            break;

        /* return movie functions as
         * slr dependencies
         */
        case RPN_FNV_COL:
        case RPN_FNV_ROW:
        case RPN_FNV_COLS:
        case RPN_FNV_ROWS:
            /* if it has an argument, ignore since
             * this contains the dependency - already returned
             * in DATA_ID_SLR case above
             */
            if(!*(cur_rpn.pos + 1))
            {
                /* return self-reference */
                grubp->data.arg.slr = grubp->slr;
                grubp->data.data_id = DATA_ID_SLR;
                res = cur_rpn.num;
                at_pos = NULL;
            }
            break;

        /* custom functions */
        case RPN_FNM_CUSTOMCALL:
        case RPN_FNM_FUNCTION:
            read_nameid(&grubp->data.arg.nameid, cur_rpn.pos + 2);
            res = grubp->data.data_id = cur_rpn.num;
            data_offset = 2;
            break;

        case RPN_FRM_END:
            if(!grubp->in_cond)
                res = RPN_FRM_END;
            else
                grubp->in_cond = 0;
            break;
        }

        rpn_skip(&cur_rpn);
    }

    /* set current position in rpn string */
    if(rpn_start)
    {
        grubp->offset = cur_rpn.pos - rpn_start;

        /* set position of data item */
        if(at_pos)
            grubp->byoffset = at_pos - rpn_start + data_offset;
        else
            grubp->byoffset = -1;
    }

    return(res);
}

/******************************************************************************
*
* return length of expression
*
******************************************************************************/

extern S32
ev_len(
    P_EV_CELL p_ev_cell)
{
    S32 len;

    len = 0;

    switch(p_ev_cell->parms.type)
    {
    case EVS_CON_DATA:
        len = offsetof(EV_CELL, parms) + sizeof(EV_PARMS);
        break;
    case EVS_CON_RPN:
        len = offsetof(EV_CELL, rpn.con.rpn_str) +
              len_rpn(p_ev_cell->rpn.con.rpn_str);
        break;
    case EVS_VAR_RPN:
        len = offsetof(EV_CELL, rpn.var.rpn_str) +
              len_rpn(p_ev_cell->rpn.var.rpn_str);
        break;
    }

    return(len);
}

/******************************************************************************
*
* adjust the references in an rpn string according to the supplied uref block
*
* this is used only for rpn strings not on the tree (e.g. replicate/copy/move)
*
******************************************************************************/

extern void
ev_rpn_adjust_refs(
    P_EV_CELL p_ev_cell,
    _InRef_     PC_UREF_PARM upp)
{
    switch(p_ev_cell->parms.type)
    {
    case EVS_CON_DATA:
    case EVS_CON_RPN:
        break;

    case EVS_VAR_RPN:
        {
        RPNSTATE cur_rpn;
        S32 in_cond = 0;

        cur_rpn.pos = p_ev_cell->rpn.var.rpn_str;

        rpn_check(&cur_rpn);

        while(cur_rpn.num != RPN_FRM_END || in_cond)
        {
            switch(cur_rpn.num)
            {
            case RPN_FRM_END:
                in_cond = 0;
                break;

            case RPN_FRM_COND:
                cur_rpn.pos += 1 + sizeof(S16);
                rpn_check(&cur_rpn);
                in_cond = 1;
                continue;

            case DATA_ID_SLR:
                {
                P_U8 p_u8 = de_const_cast(P_U8, cur_rpn.pos + 1);
                EV_SLR slr;

                read_slr    (&slr, p_u8);
                ev_match_slr(&slr, upp);
                write_slr   (&slr, p_u8);
                break;
                }

            case DATA_ID_RANGE:
                {
                P_U8 p_u8 = de_const_cast(P_U8, cur_rpn.pos + 1);
                EV_RANGE rng;

                read_range  (&rng, p_u8);
                ev_match_rng(&rng, upp);
                write_rng   (&rng, p_u8);
                break;
                }
            }

            rpn_skip(&cur_rpn);
        }
        }
    }
}

/******************************************************************************
*
* return length of expression
*
******************************************************************************/

_Check_return_
extern S32
len_rpn(
    PC_U8 rpn_str)
{
    RPNSTATE cur_rpn;
    S32 len;

    cur_rpn.pos = rpn_str;

    rpn_check(&cur_rpn);

    while(cur_rpn.num != RPN_FRM_END)
        rpn_skip(&cur_rpn);

    len = cur_rpn.pos - rpn_str + 1;

    return(len);
}

/******************************************************************************
*
* read argument for current rpn token into
* supplied symbol structure
*
* --out--
* size of argument read
*
******************************************************************************/

extern void
read_cur_sym(
    P_RPNSTATE rpnsp,
    P_SS_DATA p_ss_data)
{
    PC_U8 p_rpn_content = rpnsp->pos + sizeof32(EV_IDNO);

    ss_data_set_data_id(p_ss_data, rpnsp->num);

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        read_from_rpn(&p_ss_data->arg.fp, p_rpn_content, sizeof32(F64));
        return;

    case DATA_ID_LOGICAL:
        {
        U8 u8;
        read_from_rpn(&u8, p_rpn_content, sizeof32(U8)); /* 0 or 1 */
        p_ss_data->arg.integer = (S32) (U32) u8;
        return;
        }

    case DATA_ID_WORD16:
        {
        S16 s16;
        read_from_rpn(&s16, p_rpn_content, sizeof32(S16));
        p_ss_data->arg.integer = (S32) s16;
        return;
        }

    case DATA_ID_WORD32:
        read_from_rpn(&p_ss_data->arg.integer, p_rpn_content, sizeof32(S32));
        return;

    case DATA_ID_DATE:
        read_date(&p_ss_data->arg.ss_date, p_rpn_content);
        return;

    case DATA_ID_BLANK:
        return;

    case DATA_ID_SLR:
        read_slr(&p_ss_data->arg.slr, p_rpn_content);
        return;

    case DATA_ID_RANGE:
        read_range(&p_ss_data->arg.range, p_rpn_content);
        return;

    case DATA_ID_NAME:
        read_nameid(&p_ss_data->arg.nameid, p_rpn_content);
        return;

    case RPN_DAT_STRING:
        p_ss_data->arg.string.uchars = p_rpn_content + sizeof32(S16);
        p_ss_data->arg.string.size = ustrlen32(p_ss_data->arg.string.uchars);
        return;
    }
}

/******************************************************************************
*
* read a date from the rpn string
*
******************************************************************************/

static void
read_date(
    _OutRef_    P_SS_DATE datep,
    PC_U8 ip_pos)
{
    datep->date = (S32) readuword_LE(ip_pos, sizeof32(U32));
    ip_pos += sizeof32(U32);

    datep->time = (S32) readuword_LE(ip_pos, sizeof32(U32));
}

/******************************************************************************
*
* read a name id from rpn
*
******************************************************************************/

extern void
read_nameid(
    _OutRef_    P_EV_NAMEID nameidp,
    PC_U8 ip_pos)
{
    *nameidp = (EV_NAMEID) readuword_LE(ip_pos, sizeof32(EV_NAMEID));
}

/******************************************************************************
*
* read a pointer from the rpn string
*
******************************************************************************/

extern void
read_ptr(
    P_P_ANY ptrp,
    PC_U8 ip_pos)
{
    *ptrp = (P_ANY) readuword_LE(ip_pos, sizeof32(P_ANY));
}

/******************************************************************************
*
* read a range from the compiled string
*
******************************************************************************/

extern void
read_range(
    _OutRef_    P_EV_RANGE rngp,
    PC_U8 ip_pos)
{
    rngp->s.docno = read_ev_docno(ip_pos);
    ip_pos       += sizeof(EV_DOCNO);

    rngp->s.col   = read_ev_col(ip_pos);
    ip_pos       += sizeof(EV_COL);

    rngp->s.row   = read_ev_row(ip_pos);
    ip_pos       += sizeof(EV_ROW);

    rngp->s.flags = read_ev_flags(ip_pos);
    ip_pos       += sizeof(EV_FLAGS);

    rngp->e.docno = rngp->s.docno;

    rngp->e.col   = read_ev_col(ip_pos);
    ip_pos       += sizeof(EV_COL);

    rngp->e.row   = read_ev_row(ip_pos);
    ip_pos       += sizeof(EV_ROW);

    rngp->e.flags = read_ev_flags(ip_pos);
}

/******************************************************************************
*
* read an slr from the compiled string
*
******************************************************************************/

extern void
read_slr(
    _OutRef_    P_EV_SLR slrp,
    PC_U8 ip_pos)
{
    slrp->docno = read_ev_docno(ip_pos);
    ip_pos     += sizeof(EV_DOCNO);

    slrp->col   = read_ev_col(ip_pos);
    ip_pos     += sizeof(EV_COL);

    slrp->row   = read_ev_row(ip_pos);
    ip_pos     += sizeof(EV_ROW);

    slrp->flags = read_ev_flags(ip_pos);
}

/******************************************************************************
*
* skip to next rpn token
*
******************************************************************************/

extern EV_IDNO
rpn_skip(
    P_RPNSTATE rpnsp)
{
    assert(rpnsp->num < ELEMOF_RPN_TABLE);
    if(rpnsp->num < ELEMOF_RPN_TABLE)
    {
        /* work out how to skip symbol */
        ++rpnsp->pos;

        switch(rpn_table[rpnsp->num].rpn_type)
        {
        case RPN_DAT:
            switch(rpnsp->num)
            {
            case DATA_ID_REAL:
                rpnsp->pos += sizeof32(F64);
                break;
            case DATA_ID_LOGICAL:
                rpnsp->pos += sizeof32(U8);
                break;
            case DATA_ID_WORD16:
                rpnsp->pos += sizeof32(S16);
                break;
            case DATA_ID_WORD32:
                rpnsp->pos += sizeof32(S32);
                break;
            case DATA_ID_DATE:
                rpnsp->pos += PACKED_DATESIZE;
                break;
            case DATA_ID_SLR:
                rpnsp->pos += PACKED_SLRSIZE;
                break;
            case DATA_ID_RANGE:
                rpnsp->pos += PACKED_RNGSIZE;
                break;
            case DATA_ID_NAME:
                rpnsp->pos += sizeof32(EV_NAMEID);
                break;
            case RPN_DAT_STRING:
                rpnsp->pos += readval_S16(rpnsp->pos);
                break;
            case DATA_ID_BLANK:
            default:
                break;
            }
            break;

        case RPN_FRM:
            switch(rpnsp->num)
            {
            case RPN_FRM_SPACE:
            case RPN_FRM_RETURN:
                /* skip cr/space argument */
                rpnsp->pos += 1;
                break;
            case RPN_FRM_COND:
                /* skip entire conditional rpn */
                rpnsp->pos += readval_S16(rpnsp->pos);
                break;
            case RPN_FRM_SKIPTRUE:
            case RPN_FRM_SKIPFALSE:
                /* skip skip argument */
                rpnsp->pos += 1 + sizeof(S16);
                break;
            case RPN_FRM_NODEP:
            default:
                break;
            }
            break;

        case RPN_LCL:
            while(*rpnsp->pos)
                ++rpnsp->pos;
            ++rpnsp->pos;
            break;

        case RPN_UOP:
        case RPN_BOP:
        case RPN_REL:
            break;

        case RPN_FN0:
        case RPN_FNF:
            break;

        case RPN_FNV:
            /* skip argument count */
            ++rpnsp->pos;
            break;

        case RPN_FNM:
            /* skip argument count and custom id */
            rpnsp->pos += (1 + sizeof(EV_NAMEID));
            break;

        case RPN_FNA:
            /* skip x, y counts */
            rpnsp->pos += sizeof(S32) * 2;
            break;

        default:
            break;
        }
    }

    return(rpn_check(rpnsp));
}

/******************************************************************************
*
* output name id to rpn string
*
******************************************************************************/

extern S32
write_nameid(
    EV_NAMEID nameid,
    P_U8 op_at)
{
    P_U8 op_pos = op_at;

    op_pos += writeuword_LE(op_pos, nameid, sizeof32(EV_NAMEID));

    return(op_pos - op_at);
}

/******************************************************************************
*
* output range in packed form
*
******************************************************************************/

extern S32
write_rng(
    _InRef_     PC_EV_RANGE rngp,
    P_U8 op_at)
{
    P_U8 op_pos = op_at;

    op_pos += writeuword_LE(op_pos, (U32) ev_slr_docno(&rngp->s), sizeof32(EV_DOCNO));

    op_pos += writeuword_LE(op_pos, (U32) ev_slr_col(&rngp->s), sizeof32(EV_COL));
    op_pos += writeuword_LE(op_pos, (U32) ev_slr_row(&rngp->s), sizeof32(EV_ROW));
    op_pos += writeuword_LE(op_pos, (U32) rngp->s.flags, sizeof32(EV_FLAGS));

    op_pos += writeuword_LE(op_pos, (U32) ev_slr_col(&rngp->e), sizeof32(EV_COL));
    op_pos += writeuword_LE(op_pos, (U32) ev_slr_row(&rngp->e), sizeof32(EV_ROW));
    op_pos += writeuword_LE(op_pos, (U32) rngp->e.flags, sizeof32(EV_FLAGS));

    return(op_pos - op_at);
}

/******************************************************************************
*
* output slr in packed form
*
******************************************************************************/

extern S32
write_slr(
    _InRef_     PC_EV_SLR slrp,
    P_U8 op_at)
{
    P_U8 op_pos = op_at;

    op_pos += writeuword_LE(op_pos, (U32) ev_slr_docno(slrp), sizeof32(EV_DOCNO));

    op_pos += writeuword_LE(op_pos, (U32) ev_slr_col(slrp), sizeof32(EV_COL));
    op_pos += writeuword_LE(op_pos, (U32) ev_slr_row(slrp), sizeof32(EV_ROW));
    op_pos += writeuword_LE(op_pos, (U32) slrp->flags, sizeof32(EV_FLAGS));

    return(op_pos - op_at);
}

/* end of ev_rpn.c */
