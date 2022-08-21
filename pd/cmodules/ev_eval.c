/* ev_eval.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RPN driven evaluator */

/* MRJC March 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "typepack.h"
#include "ev_eval.h"
#include "monotime.h"

/* local header file */
#include "ev_evali.h"

/*
internal functions
*/

static S32
eval_rpn(
    S32 stack_at);

/*ncr*/
static S32
custom_result_ERROR(
    STATUS error);

static S32
process_control(
    S32 action,
    P_EV_DATA args[],
    S32 n_args,
    S32 eval_stack_base);

static S32
process_control_for_cond(
    P_STACK_CONTROL_LOOP P_stack_control_loop,
    _InVal_     BOOL step);

static S32
process_control_search(
    S32 block_start,
    S32 block_end,
    S32 block_end_maybe1,
    S32 block_end_maybe2,
    _InRef_     PC_EV_SLR slrp,
    EV_ROW offset,
    S32 stack_after,
    S32 error);

_Check_return_
static STATUS
read_check_custom_def(
    PC_U8 ip_pos,
    EV_NAMEID *custom_num,
    P_P_EV_CUSTOM p_p_ev_custom);

static S32
slot_error_complete(
    STATUS error);

/*ncr*/
static STATUS
slot_set_error(
    _InRef_     PC_EV_SLR slrp,
    _InVal_     STATUS error);

static P_STACK_ENTRY
stack_back_search(
    P_STACK_ENTRY stkentp,
    S32 entry_type);

static P_STACK_ENTRY
stack_back_search_loop(
    S32 loop_type);

#define stack_check_n(n_spaces) (((stack_ptr + (n_spaces)) < stack_end) ? 0 : stack_grow(n_spaces))

static S32
stack_grow(
    S32 n_spaces);

/******************************************************************************
*
* index elements on the stack
*
* 0=bottom element
* 1=next element up etc.
*
******************************************************************************/

#define stack_index_ptr_data(base_offset, offset) (&stack_base[(base_offset)-(offset)].data.stack_data_item.data)

#define stack_inc(in_t, in_s, in_stack_flags) ( \
    (++stack_ptr)->type    = (in_t), \
    stack_ptr->slr         = (in_s), \
    stack_ptr->stack_flags = (in_stack_flags) )

#define stack_offset(stkp) ((S32) ((stkp) - stack_base))

static S32
stack_set(
    S32 stack_level);

static S32
visit_supporting_range(void);

/*
data
*/

EV_SERIAL ev_serial_num = 1;

static P_STACK_ENTRY stack_base = NULL;
static P_STACK_ENTRY stack_end  = NULL;
static P_STACK_ENTRY stack_ptr  = NULL;

#include <signal.h>

typedef void (__cdecl * P_PROC_SIGNAL) (
    _In_        int sig);

#include <setjmp.h>

static jmp_buf safepoint;

static MONOTIME time_started;

#define BACKGROUND_SLICE 25

#if TRACE_ALLOWED

/*
this is a local tracer routine
*/

static void
eval_trace(
    char *string)
{
    char buffer[BUF_EV_LONGNAMLEN];
    EV_SLR slr;

    slr = stack_ptr->slr;
    slr.flags |= SLR_EXT_REF;

    (void) ev_dec_slr(buffer, slr.docno, &slr, FALSE);

    trace_0(TRACE_MODULE_EVAL, buffer);
    trace_2(TRACE_MODULE_EVAL, " [SP=%d] %s", stack_offset(stack_ptr), string);
}

#else

#define eval_trace(one)

#endif

/******************************************************************************
*
* loop over the arguments on the stack, comparing
* them against the given type list
* errors are generated for mismatched arguments
* after argument type checking, array expansion is
* performed ready for auto-call of functions and
* customs with arrays
*
* arg_normalise keeps track of the maximum sizes of
* array arguments for those parameters which do not
* expect arrays to be passed
*
* then we loop over arguments and results to expand
* arrays and arguments read
*
* --out--
* < 0 error in arguments or array expansion;
*     error state is passed in result structure
* ==0 arguments processed OK, no arrays
* > 0 arguments processed OK, array sizes set
*
******************************************************************************/

static S32
args_check(
    S32 arg_count,
    P_EV_DATA *args,
    S32 type_count,
    PC_EV_TYPE arg_types,
    P_EV_DATA resultp,
    P_S32 max_x,
    P_S32 max_y)
{
    S32 args_ok;

    args_ok = 1;
    *max_x  = *max_y = 0;
    resultp->did_num = RPN_DAT_BLANK;

    if(arg_count)
    {
        S32 ix;
        S32 typec = type_count;
        PC_EV_TYPE typep = arg_types;

        for(ix = 0; ix < arg_count; ++ix)
        {
            /* check/convert each argument */
            if(arg_normalise(args[ix],
                             (EV_TYPE) (typep ? *typep
                                              : EM_REA | EM_STR | EM_DAT),
                             max_x,
                             max_y) < 0)
            {
                *resultp = *args[ix];
                args_ok = 0;
                break;
            }

            if(typec > 1)
            {
                --typec;
                ++typep;
            }
        }

        /* if any arrays were passed, we loop over the
         * arguments and do any necessary expansions
        */
        if(args_ok && (*max_x || *max_y))
        {
            typec = type_count;
            typep = arg_types;

            /* loop over arguments */
            for(ix = 0; ix < arg_count; ++ix)
            {
                /* check for expected array too - this is a fault */
                if(typep && (*typep & EM_ARY))
                {
                    switch(args[ix]->did_num)
                    {
                    case RPN_DAT_RANGE:
                    case RPN_TMP_ARRAY:
                    case RPN_RES_ARRAY:
                        ev_data_set_error(resultp, create_error(EVAL_ERR_NESTEDARRAY));
                        args_ok = 0;
                        break;
                    }
                }
                else if(array_expand(args[ix], *max_x, *max_y) < 0)
                {
                    *resultp = *args[ix];
                    args_ok = 0;
                }

                if(!args_ok)
                    break;

                if(typec > 1)
                {
                    --typec;
                    ++typep;
                }
            }

            /* expand result */
            if(args_ok)
            {
                resultp->did_num = RPN_DAT_BLANK;
                if(array_expand(resultp, *max_x, *max_y) < 0)
                    args_ok = 0;
            }
        }
    }

    return(args_ok ? (*max_x || *max_y ? 1 : 0)
                   : resultp->arg.ev_error.status);
}

/******************************************************************************
*
* check a supporting name to see if it needs
* recalculating; if it does, stack the current
* state and prepare for its recalculation
*
* this requires three stack entries - check
* before calling!
*
******************************************************************************/

static S32
check_supporting_name(
    P_EV_GRUB grubp)
{
    /* look up name definition */
    S32 res = SAME_STATE;
    EV_NAMEID name_num = name_def_find(grubp->data.arg.nameid);

    if(name_num >= 0)
    {
        S32 do_visit = 0;
        P_EV_NAME p_ev_name = name_ptr_must(name_num);

        switch(p_ev_name->def_data.did_num)
        {
        case RPN_DAT_SLR:
            if(p_ev_name->visited < ev_serial_num)
                do_visit = 1;
            break;
        case RPN_DAT_RANGE:
            if(p_ev_name->visited < ev_serial_num)
                do_visit = 1;
            break;
        }

        if(do_visit)
        {
            /* switch from VISIT_SLOT to VISIT_NAME */
            stack_inc(VISIT_COMPLETE_NAME, stack_ptr[-1].slr, 0);

            stack_ptr->data.stack_visit_name.nameid = grubp->data.arg.nameid;

            switch(p_ev_name->def_data.did_num)
            {
            case RPN_DAT_SLR:
                /* go check out the cell */
                stack_inc(VISIT_SLOT, p_ev_name->def_data.arg.slr, 0);
                grub_init(&stack_ptr->data.stack_visit_slot.grubb, &stack_ptr->slr);
                break;

            case RPN_DAT_RANGE:
                /* prime visit_supporting_range
                 * to check out supporters
                 */
                stack_inc(VISIT_SUPPORTRNG, p_ev_name->def_data.arg.range.s, 0);
                stack_ptr->data.stack_visit_range.range = p_ev_name->def_data.arg.range;
                break;
            }

            eval_trace("<check_supporting_name>");

            res = NEW_STATE;
        }
    }

    return(res);
}

/******************************************************************************
*
* check a supporting range to see if it needs
* recalculating; if it does, stack the current
* state and prepare for its recalculation
*
* this requires two stack entries - check
* before calling!
*
******************************************************************************/

static S32
check_supporting_rng(
    P_EV_GRUB grubp)
{
    EV_TRENT rix;

    /* check whether we need to visit range */
    if((rix = search_for_rng_ref(&grubp->data.arg.range)) >= 0)
    {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno_must(grubp->data.arg.range.s.docno);
        const P_RANGE_USE p_range_use = tree_rngptr(p_ss_doc, rix);
        PTR_ASSERT(p_range_use);
        if(p_range_use->visited >= ev_serial_num)
        {
            trace_0(TRACE_MODULE_EVAL, "*** range visit avoided since serial number up-to-date ***");
            return(SAME_STATE);
        }
    }

    /* stack the current state */
    stack_inc(VISIT_SUPPORTRNG, grubp->data.arg.range.s, 0);

    /* prime visit_supporting_range to check out supporters */
    stack_ptr->data.stack_visit_range.range = grubp->data.arg.range;

    return(NEW_STATE);
}

/******************************************************************************
*
* check for circular reference
*
******************************************************************************/

static S32
circ_check(
    P_EV_CELL p_ev_cell)
{
    S32 res;

    res = SAME_STATE;

    if(p_ev_cell->parms.circ)
    {
        P_STACK_ENTRY stkentp;
        S32 found_it;

        eval_trace("<circ_check> found EVS_CIRC");

        /* now we must search back up the stack to find the
         * first visit to the cell and stop this recalcing the
         * cell and thus removing our stored error
        */
        stkentp = stack_ptr - 1;
        found_it = 0;

        while(stkentp >= stack_base)
        {
            if(stkentp->type == VISIT_SLOT)
            {
                slot_set_error(&stkentp->slr, /*create_error*/(EVAL_ERR_CIRC));
                stkentp->stack_flags |= STF_CALCEDERROR | STF_CALCEDSUPPORTER;

#if TRACE_ALLOWED
                if_constant(tracing(TRACE_MODULE_EVAL))
                {
                    char buffer[BUF_EV_LONGNAMLEN];
                    ev_trace_slr(buffer, elemof32(buffer),
                                 "<circ_check> stack backtrack found $$",
                                 &stkentp->slr);
                    eval_trace(buffer);
                }
#endif

                if(slr_equal(&stack_ptr->slr, &stkentp->slr))
                    found_it = 1;
            }

            if(found_it)
                break;

            --stkentp;
        }

        res = slot_error_complete(/*create_error*/(EVAL_ERR_CIRC));
    }
    else
        p_ev_cell->parms.circ = 1;

    return(res);
}

/******************************************************************************
*
* evaluate an rpn string from outside the evaluator
*
******************************************************************************/

extern S32
ev_eval_rpn(
    P_EV_DATA resultp,
    _InRef_     PC_EV_SLR slrp,
    PC_U8 rpn_in)
{
    S32 res;

    /* first, make sure we have a stack */
    if((res = stack_check_n(1)) >= 0)
    {
        S32 stack_offset;

        stack_offset = stack_offset(stack_ptr);
        stack_inc(CALC_SLOT, *slrp, 0);

        stack_ptr->data.stack_in_calc.eval_block.offset = 0;
        stack_ptr->data.stack_in_calc.eval_block.slr = *slrp;
        stack_ptr->data.stack_in_calc.eval_block.p_ev_cell = NULL;
        stack_ptr->data.stack_in_calc.type = INCALC_PTR;
        stack_ptr->data.stack_in_calc.ptr = rpn_in;

        if((res = eval_rpn(stack_offset(stack_ptr))) == SAME_STATE)
            res = 0;
        else
            res = create_error(EVAL_ERR_BADEXPR);

        *resultp = stack_ptr->data.stack_in_calc.result_data;

        stack_set(stack_offset);
    }

    return(res);
}

/******************************************************************************
*
* backtrack up the stack to find the
* most recent cell we were calculating,
* set the error into the cell and go
* to calculate its dependents
*
******************************************************************************/

static S32
eval_backtrack_error(
    S32 error,
    S32 stack_at,
    P_PROC_SIGNAL oldfpe)
{
    signal(SIGFPE, oldfpe); /* restore old handler */

    stack_set(stack_at);
    ev_data_set_error(&stack_base[stack_at].data.stack_in_calc.result_data, error);

    return(SAME_STATE);
}

/******************************************************************************
*
* capture SIGNAL errors during eval_rpn
*
******************************************************************************/

static void
eval_jmp(
    int sig)
{
    /* reinstall ourselves, as SIG_DFL will have been restored */
    signal(sig, eval_jmp);

    longjmp(safepoint, create_error(EVAL_ERR_FPERROR));
}

/******************************************************************************
*
* perform certain operations optimally without
* going through the general argument checking
* and processing
*
******************************************************************************/

static S32
eval_optimise(
    P_RPNSTATE rpnp)
{
    S32 did_opt;

    did_opt = 0;
    switch(rpnp->num)
    {
    case RPN_BOP_PLUS:
        if(0 != (did_opt = two_nums_add_try(
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[ 0].data.stack_data_item.data)))
            --stack_ptr;
        break;

    case RPN_BOP_MINUS:
        if(0 != (did_opt = two_nums_subtract_try(
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[ 0].data.stack_data_item.data)))
            --stack_ptr;
        break;

    case RPN_BOP_DIVIDE:
        if(0 != (did_opt = two_nums_divide_try(
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[ 0].data.stack_data_item.data)))
            --stack_ptr;
        break;

    case RPN_BOP_TIMES:
        if(0 != (did_opt = two_nums_multiply_try(
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[-1].data.stack_data_item.data,
                                    &stack_ptr[ 0].data.stack_data_item.data)))
            --stack_ptr;
        break;
    }

    return(did_opt);
}

/* recache cell pointer as needed */

static inline void
stack_in_calc_ensure_slot(
    _InoutRef_  P_STACK_IN_CALC p_stack_in_calc,
    _InRef_     PC_EV_SLR p_ev_slr,
    _In_z_      PCTSTR caller)
{
#if 1
    if(NULL != p_stack_in_calc->eval_block.p_ev_cell)
    {
        P_EV_CELL p_ev_cell;
        S32 travel_res = ev_travel(&p_ev_cell, p_ev_slr);
        if(p_stack_in_calc->eval_block.p_ev_cell != p_ev_cell)
        {
            reportf(TEXT("%s: cell ") U32_TFMT TEXT(":") S32_TFMT TEXT(",") S32_TFMT TEXT(" moved from ") PTR_TFMT TEXT(" to ") PTR_TFMT,
                caller, (U32) ev_slr_docno(p_ev_slr), ev_slr_col(p_ev_slr), ev_slr_row(p_ev_slr), report_ptr_cast(stack_ptr->data.stack_in_calc.eval_block.p_ev_cell), report_ptr_cast(p_ev_cell));
            p_stack_in_calc->eval_block.p_ev_cell = p_ev_cell;
            p_stack_in_calc->travel_res = travel_res;
        }
    }
#else
    UNREFERENCED_PARAMETER(caller);
#endif

    if(NULL != p_stack_in_calc->eval_block.p_ev_cell)
        return;

    p_stack_in_calc->travel_res = ev_travel(&p_stack_in_calc->eval_block.p_ev_cell, p_ev_slr);
}

/******************************************************************************
*
* wander along an rpn string and evaluate it
*
******************************************************************************/

static S32
eval_rpn(
    S32 stack_at)
{
    RPNSTATE rpnb;
    PC_U8 rpn_start;
    P_STACK_IN_CALC p_stack_in_calc;
    EV_SLR cur_slr;
    int jmpval;
    P_PROC_SIGNAL oldfpe;

    oldfpe = signal(SIGFPE, eval_jmp);

    /* catch FP errors etc. */
    if((jmpval = setjmp(safepoint)) != 0)
        return(eval_backtrack_error(jmpval, stack_at, oldfpe));

    p_stack_in_calc = &stack_base[stack_at].data.stack_in_calc;

    /* set up address of rpn string */
    switch(p_stack_in_calc->type)
    {
    case INCALC_PTR:
        rpn_start = p_stack_in_calc->ptr;
        break;

    case INCALC_SLR:
    default:
        {
        stack_in_calc_ensure_slot(p_stack_in_calc, &p_stack_in_calc->eval_block.slr, "eval_rpn");

        if(p_stack_in_calc->eval_block.p_ev_cell->parms.type == EVS_CON_RPN)
            rpn_start = p_stack_in_calc->eval_block.p_ev_cell->rpn.con.rpn_str;
        else
            rpn_start = p_stack_in_calc->eval_block.p_ev_cell->rpn.var.rpn_str;
        break;
        }
    }

    rpnb.pos = rpn_start + p_stack_in_calc->eval_block.offset;
    rpn_check(&rpnb);
    cur_slr = p_stack_in_calc->eval_block.slr;

    /* we can't access p_stack_in_calc after this point: the stack may move!
     * we must indirect through stack_base each time
     */
    while(rpnb.num != RPN_FRM_END)
    {
        /* check that we have stack available */
        if(stack_check_n(3) < 0)
            return(eval_backtrack_error(status_nomem(), stack_at, oldfpe));

        switch(rpn_table[rpnb.num].rpn_type)
        {
        /* needs 1 stack entry */
        case RPN_DAT:
            {
            EV_FLAGS check_flags = 0;
            EV_DOCNO check_docno = 0; /* dataflow */

            stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
            read_cur_sym(&rpnb, &stack_ptr->data.stack_data_item.data);

            switch(stack_ptr->data.stack_data_item.data.did_num)
            {
            #ifdef SLOTS_MOVE
            /* we can't push DAT_STRINGs because they point into the rpn
             * which may move with a cell during an interruption; if cells
             * stop moving about, then we needn't do this duplication
             */
            case RPN_DAT_STRING:
                {
                EV_DATA temp;

                ss_data_resource_copy(&temp, &stack_ptr->data.stack_data_item.data);
                stack_ptr->data.stack_data_item.data = temp;
                break;
                }
            #endif

            /* check for duff slrs and ranges */
            case RPN_DAT_SLR:
                check_flags = stack_ptr->data.stack_data_item.data.arg.slr.flags;
                check_docno = stack_ptr->data.stack_data_item.data.arg.slr.docno;
                break;

            case RPN_DAT_RANGE:
                check_flags = stack_ptr->data.stack_data_item.data.arg.range.s.flags |
                              stack_ptr->data.stack_data_item.data.arg.range.e.flags;
                check_docno = stack_ptr->data.stack_data_item.data.arg.range.s.docno;
                break;
            }

            if(check_flags & SLR_BAD_REF)
                ev_data_set_error(&stack_ptr->data.stack_data_item.data, create_error(EVAL_ERR_SLR_BAD_REF));
            else if((check_flags & SLR_EXT_REF) && ev_doc_error(check_docno))
                ev_data_set_error(&stack_ptr->data.stack_data_item.data, ev_doc_error(check_docno));

            break;
            }

        /* needs 1 stack entry */
        case RPN_FRM:
            switch(rpnb.num)
            {
            /* conditional subexpression */
            case RPN_FRM_COND:
                {
                /* stack offset of conditional expression */
                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                stack_ptr->data.stack_data_item.data.did_num      = RPN_FRM_COND;
                stack_ptr->data.stack_data_item.data.arg.cond_pos =
                                                        rpnb.pos  -
                                                        rpn_start +
                                                        1         +
                                                        sizeof(S16);
                break;
                }

            case RPN_FRM_SKIPTRUE:
            case RPN_FRM_SKIPFALSE:
                {
                S32 res;
                S32 stack_offset;
                P_EV_DATA argp;

                /* check boolean value on bottom of stack
                 */
                stack_offset = (S32) *(rpnb.pos + 1);
                argp = stack_index_ptr_data(stack_offset(stack_ptr), stack_offset);
                if((res = arg_normalise(argp, EM_REA, NULL, NULL)) >= 0)
                    res = (argp->arg.fp != 0);
                else
                    break;

                if(rpnb.num == RPN_FRM_SKIPTRUE)
                {
                    if(!res)
                        break;
                }
                else if(res)
                    break;

                /* skip argument evaluation - push a dummy */
                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                stack_ptr->data.stack_data_item.data.did_num = RPN_DAT_BLANK;

                rpnb.pos += readval_S16(rpnb.pos + 2);
                rpn_check(&rpnb);
                continue;
                }

            /* most formatting bits are ignored in evaluation */
            case RPN_FRM_BRACKETS:
            case RPN_FRM_RETURN:
            case RPN_FRM_SPACE:
            case RPN_FRM_END:
            case RPN_FRM_NODEP:
            default:
                break;
            }
            break;

        /* custom local arguments
         * needs 1 stack entry
         */
        case RPN_LCL:
            {
            P_STACK_ENTRY stkentp;

            stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);

            /* default to local argument undefined */
            ev_data_set_error(&stack_ptr->data.stack_data_item.data, create_error(EVAL_ERR_LOCALUNDEF));

            /* look back up stack for custom call */
            if((stkentp = stack_back_search(stack_ptr - 1, EXECUTING_CUSTOM)) != NULL)
            {
                char arg_name[BUF_EV_INTNAMLEN];
                PC_U8 ip;
                P_U8 op;
                P_EV_CUSTOM p_ev_custom;
                S32 arg_ix;
                P_EV_DATA p_ev_data;

                /* load up local name */
                ip = rpnb.pos + 1;
                op = arg_name;
                do
                    *op++ = *ip;
                while(*ip++);

                arg_ix = -1;
                p_ev_custom = custom_ptr(stkentp->data.stack_executing_custom.custom_num);
                if(p_ev_custom != NULL)
                {
                    S32 i;

                    for(i = 0; i < p_ev_custom->args.n; ++i)
                    {
                        if(0 == _stricmp(p_ev_custom->args.id[i], arg_name))
                        {
                            arg_ix = i;
                            break;
                        }
                    }
                }

                if(arg_ix >= 0 && arg_ix < stkentp->data.stack_executing_custom.n_args)
                {
                    /* get pointer to data on stack */
                    p_ev_data = stack_index_ptr_data(stkentp->data.stack_executing_custom.stack_base,
                                                     stkentp->data.stack_executing_custom.n_args - (S32) arg_ix - 1);

                    /* if it's an expanded array, we must
                     * index the relevant array element
                    */
                    if(stkentp->data.stack_executing_custom.in_array &&
                       data_is_array_range(p_ev_data))
                    {
                        EV_DATA temp;

                        array_range_index(&temp,
                                          p_ev_data,
                                          stkentp->data.stack_executing_custom.x_pos,
                                          stkentp->data.stack_executing_custom.y_pos,
                                          p_ev_custom->args.types[arg_ix]);

                        ss_data_resource_copy(&stack_ptr->data.stack_data_item.data, &temp);
                    }
                    else
                        ss_data_resource_copy(&stack_ptr->data.stack_data_item.data, p_ev_data);
                }
            }
            break;
            }

        /* process a custom call
         * needs 3 stack entries
         * stack during custom is like this:
         * [argn]
         * .
         * [arg1]
         * [arg0]
         * [IN_EVAL]
         * [result]
         * [EXECUTING_CUSTOM]
         */
        case RPN_FNM:
            {
            STATUS res;
            S32 arg_count;
            EV_NAMEID custom_num;
            P_EV_CUSTOM p_ev_custom;

            /* read custom call argument count */
            arg_count = (S32) *(rpnb.pos + 1);

            /* read the custom id */
            if((res = read_check_custom_def(rpnb.pos + 2,
                                           &custom_num,
                                           &p_ev_custom)) >= 0)
            {
                P_EV_DATA args[EV_MAX_ARGS];
                S32 ix, max_x, max_y;
                EV_DATA custom_result;

                /* get pointer to each argument */
                for(ix = 0; ix < arg_count; ++ix)
                    args[ix] = stack_index_ptr_data(stack_offset(stack_ptr),
                                                    arg_count - (S32) ix - 1);

                /* check the argument types and look for an array */
                if((res = args_check(arg_count, args,
                                     p_ev_custom->args.n,
                                     p_ev_custom->args.n ? p_ev_custom->args.types : NULL,
                                     &custom_result,
                                     &max_x, &max_y)) >= 0)
                {
                    /* save current state */
                    rpn_skip(&rpnb);
                    stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].stack_flags);
                    stack_base[stack_at].data.stack_in_calc.eval_block.offset = rpnb.pos - rpn_start;
                    stack_ptr->data.stack_in_eval.stack_offset = stack_at;

                    /* create custom result area on stack */
                    stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                    stack_ptr->data.stack_data_item.data = custom_result;

                    /* switch to executing the custom */
                    stack_inc(EXECUTING_CUSTOM, cur_slr, stack_ptr[-1].stack_flags);

                    stack_ptr->data.stack_executing_custom.n_args = arg_count;
                    stack_ptr->data.stack_executing_custom.stack_base = stack_offset(stack_ptr - 3);
                    stack_ptr->data.stack_executing_custom.in_array = (max_x || max_y);
                    stack_ptr->data.stack_executing_custom.x_pos =
                    stack_ptr->data.stack_executing_custom.y_pos = 0;
                    stack_ptr->data.stack_executing_custom.custom_slot = p_ev_custom->owner;
                    stack_ptr->data.stack_executing_custom.custom_slot.row += 1;
                    stack_ptr->data.stack_executing_custom.next_slot =
                    stack_ptr->data.stack_executing_custom.custom_slot;
                    stack_ptr->data.stack_executing_custom.custom_num = custom_num;
                    stack_ptr->data.stack_executing_custom.elseif = 0;
                }
            }

            /* stack error instead of calling custom */
            if(res < 0)
            {
                /* remove custom arguments */
                stack_set(stack_offset(stack_ptr) - arg_count);

                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                ev_data_set_error(&stack_ptr->data.stack_data_item.data, res);
                break;
            }

            return(NEW_STATE);
            }

        /* needs 3 stack entries
         * format for semantix is:
         * arg1, arg2, ... argn opr [# arg] [custom id]
         */
        case RPN_UOP:
        case RPN_BOP:
        case RPN_REL:
        case RPN_FN0:
        case RPN_FNF:
        case RPN_FNV:
            if(!eval_optimise(&rpnb))
            {
                P_EV_DATA args[EV_MAX_ARGS];
                S32 res, type_count, arg_count, ix, max_x, max_y;
                PC_EV_TYPE arg_types;
                PC_RPNDEF func_data = &rpn_table[rpnb.num];
                EV_DATA func_result;

                /* establish argument count */
                if((arg_count = func_data->n_args) < 0)
                    arg_count = (S32) *(rpnb.pos + 1);

                if((arg_types = func_data->arg_types) != NULL)
                    type_count = (S32) *arg_types++;
                else
                    type_count = 0;

                /* get pointer to each argument */
                for(ix = 0; ix < arg_count; ++ix)
                    args[ix] = stack_index_ptr_data(stack_offset(stack_ptr),
                                                    arg_count - (S32) ix - 1);

                /* check the argument types and look for an array */
                res = 0;
                if((res = args_check(arg_count, args,
                                     type_count, arg_types,
                                     &func_result,
                                     &max_x, &max_y)) > 0)
                {
                    /* save calculating cell state */
                    const P_PROC_EXEC p_proc_exec = func_data->p_proc_exec;

                    /* save resume position */
                    rpn_skip(&rpnb);
                    stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].stack_flags);
                    stack_base[stack_at].data.stack_in_calc.eval_block.offset = rpnb.pos - rpn_start;
                    stack_ptr->data.stack_in_eval.stack_offset = stack_at;

                    /* push result array */
                    stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                    stack_ptr->data.stack_data_item.data = func_result;

                    /* swap to processing array state */
                    stack_inc(PROCESSING_ARRAY, cur_slr, stack_ptr[-1].stack_flags);
                    stack_ptr->data.stack_processing_array.x_pos =
                    stack_ptr->data.stack_processing_array.y_pos = 0;
                    stack_ptr->data.stack_processing_array.n_args = arg_count;
                    stack_ptr->data.stack_processing_array.stack_base = stack_offset(stack_ptr - 3);
                    stack_ptr->data.stack_processing_array.exec = p_proc_exec;
                    stack_ptr->data.stack_processing_array.type_count = type_count;
                    stack_ptr->data.stack_processing_array.arg_types = arg_types;
                    res = NEW_STATE;
                }
                /* no array processing to do */
                else if(!res)
                {
                    switch(func_data->parms.ex_type)
                    {
                    case EXEC_EXEC:
                        (*func_data->p_proc_exec)
                            (args,
                             arg_count,
                             &func_result,
                             &stack_ptr->slr);
                        break;

                    /* needs 3 stack entries */
                    case EXEC_DBASE:
                        {
                        /* we can't process arrays here */
                        if(args[0]->did_num == RPN_DAT_RANGE)
                        {
                            STACK_DBASE stack_dbase;

                            if(NULL != (stack_dbase.p_stat_block = al_ptr_alloc_elem(STAT_BLOCK, 1, &res)))
                            {
                                /* initialise database function data block */
                                stack_dbase.dbase_slot = stack_ptr->slr;
                                stack_dbase.dbase_rng = args[0]->arg.range;
                                stack_dbase.cond_pos = (S32) args[1]->arg.cond_pos;
                                stack_dbase.offset.col = 0;
                                stack_dbase.offset.row = 0;
                                dbase_stat_block_init(stack_dbase.p_stat_block,
                                                      (U32) func_data->parms.no_exec);

                                stack_set(stack_offset(stack_ptr) - arg_count);

                                /* save current state */
                                rpn_skip(&rpnb);
                                stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].stack_flags);
                                stack_base[stack_at].data.stack_in_calc.eval_block.offset = rpnb.pos - rpn_start;
                                stack_ptr->data.stack_in_eval.stack_offset = stack_at;

                                /* switch to dbase state */
                                stack_inc(DBASE_FUNCTION, cur_slr, stack_ptr[-1].stack_flags);
                                stack_ptr->data.stack_dbase = stack_dbase;
                                res = NEW_STATE;
                            }
                        }
                        else
                            res = create_error(EVAL_ERR_ARGTYPE);

                        break;
                        }

                    case EXEC_CONTROL:
                        if(stack_ptr->stack_flags & STF_INCUSTOM)
                            res = process_control(func_data->parms.no_exec,
                                                  args, arg_count,
                                                  stack_at);
                        else
                        {
                            ss_data_free_resources(&func_result);
                            res = create_error(EVAL_ERR_BADCONTROL);
                        }
                        break;

                    case EXEC_LOOKUP:
                        {
                        STACK_LOOKUP stack_lookup;
                        S32 match;

                        if(NULL != (stack_lookup.p_lookup_block = al_ptr_alloc_elem(LOOKUP_BLOCK, 1, &res)))
                        {
                            ss_data_resource_copy(&stack_lookup.arg1, args[1]);
                            ss_data_resource_copy(&stack_lookup.arg2, args[2]);

                            if(func_data->parms.no_exec == LOOKUP_MATCH)
                                match = (S32) args[2]->arg.integer;
                            else
                                match = 0;

                            lookup_block_init(stack_lookup.p_lookup_block,
                                              args[0],
                                              func_data->parms.no_exec,
                                              0,
                                              match);

                            /* blow away lookup arguments */
                            stack_set(stack_offset(stack_ptr) - arg_count);

                            /* save current state */
                            rpn_skip(&rpnb);
                            stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].stack_flags);
                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = rpnb.pos - rpn_start;
                            stack_ptr->data.stack_in_eval.stack_offset = stack_at;

                            /* switch to lookup state */
                            stack_inc(LOOKUP_HAPPENING, cur_slr, stack_ptr[-1].stack_flags);
                            stack_ptr->data.stack_lookup = stack_lookup;
                            res = NEW_STATE;
                        }

                        break;
                        }

                    case EXEC_ALERT:
                        {
                        STACK_ALERT_INPUT stack_alert_input;

                        stack_alert_input.alert_input = rpnb.num;
                        switch(rpnb.num)
                        {
                        case RPN_FNV_ALERT:
                            res = ev_alert(stack_ptr->slr.docno,
                                           args[0]->arg.string.uchars,
                                           args[1]->arg.string.uchars,
                                           arg_count > 2 ? args[2]->arg.string.uchars : NULL);
                            break;

                        case RPN_FNV_INPUT:
                            /* save away name to receive result */
                            strcpy(stack_alert_input.name_id, args[1]->arg.string.uchars);
                            res = ev_input(stack_ptr->slr.docno,
                                           args[0]->arg.string.uchars,
                                           args[2]->arg.string.uchars,
                                           arg_count > 3 ? args[3]->arg.string.uchars : NULL);
                            break;
                        }

                        if(res >= 0)
                        {
                            /* blow away arguments */
                            stack_set(stack_offset(stack_ptr) - arg_count);

                            /* save current state */
                            rpn_skip(&rpnb);
                            stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].stack_flags);
                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = rpnb.pos - rpn_start;
                            stack_ptr->data.stack_in_eval.stack_offset = stack_at;

                            /* switch to lookup state */
                            stack_inc(ALERT_INPUT, cur_slr, stack_ptr[-1].stack_flags);
                            stack_ptr->data.stack_alert_input = stack_alert_input;
                            res = NEW_STATE;
                        }

                        break;
                        }
                    }
                }

                /* have we switched state ? */
                if(res == NEW_STATE)
                    return(res);

                if(res < 0)
                    ev_data_set_error(&func_result, res);

                /* remove function arguments from stack */
                stack_set(stack_offset(stack_ptr) - arg_count);

                /* push result onto stack */
                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
                stack_ptr->data.stack_data_item.data = func_result;
            }
            break;

        /* make an array from args on stack */
        case RPN_FNA:
            {
            S32 x_size, y_size;
            EV_DATA array_data;

            x_size = readval_S32(rpnb.pos + 1);
            y_size = readval_S32(rpnb.pos + 1 + sizeof(S32));

            if(status_ok(ss_array_make(&array_data, x_size, y_size)))
            {
                S32 x, y;
                S32 ix = 0;

                for(y = y_size - 1; y >= 0; --y)
                    for(x = x_size - 1; x >= 0; --x)
                    {
                        ss_data_resource_copy(ss_array_element_index_wr(&array_data, x, y),
                                              stack_index_ptr_data(stack_offset(stack_ptr), ix));
                        ++ix;
                    }
            }

            /* remove arguments */
            stack_set(stack_offset(stack_ptr) - (S32) x_size * y_size);

            /* push result */
            stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].stack_flags);
            stack_ptr->data.stack_data_item.data = array_data;

            break;
            }
        }
        rpn_skip(&rpnb);
    }

    if(stack_ptr->type != DATA_ITEM)
        return(eval_backtrack_error(create_error(EVAL_ERR_INTERNAL), stack_at, oldfpe));
    else
        stack_base[stack_at].data.stack_in_calc.result_data = stack_ptr->data.stack_data_item.data;

    /* clear final result from stack */
    --stack_ptr;

    /* restore old handler */
    signal(SIGFPE, oldfpe);

    /* return complete code */
    return(SAME_STATE);
}

/******************************************************************************
*
* dispose of a lookup block
*
******************************************************************************/

static void
lookup_block_dispose(
    _InoutRef_  P_STACK_LOOKUP p_stack_lookup)
{
    ss_data_free_resources(&p_stack_lookup->arg1);
    ss_data_free_resources(&p_stack_lookup->arg2);
    ss_data_free_resources(&p_stack_lookup->p_lookup_block->target_data);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_stack_lookup->p_lookup_block));
}

/******************************************************************************
*
* jump to a given row in a custom
*
******************************************************************************/

static S32
custom_jmp(
    _InRef_     PC_EV_SLR slrp,
    EV_ROW offset,
    S32 stack_after)
{
    P_STACK_ENTRY stkentp = stack_back_search(stack_ptr, EXECUTING_CUSTOM);

    if(stkentp == NULL)
        stack_ptr->type =_ERROR;
    else
    {
        S32 res = 0;
        P_EV_CUSTOM p_ev_custom = custom_ptr(stkentp->data.stack_executing_custom.custom_num);

        if(p_ev_custom != NULL)
        {
            EV_ROW first_row, last_row;
            EV_SLR slr;

            last_row  = ev_numrow(ev_slr_docno(slrp));
            first_row = p_ev_custom->owner.row;
            slr       = *slrp;
            slr.row  += offset;

            if((slr.docno != stack_ptr->slr.docno) ||
               (slr.col != stack_ptr->slr.col)     ||
               (slr.row >= last_row)               ||
               (slr.row <= first_row)              )
                res = create_error(EVAL_ERR_BADGOTO);
            else
                stkentp->data.stack_executing_custom.next_slot = slr;
        }
        else
            res = create_error(EVAL_ERR_BADGOTO);

        if(res < 0)
            custom_result_ERROR(res);
        else
            stack_set(stack_after);
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* finish custom returning result given
*
******************************************************************************/

/*ncr*/
static S32
custom_result(
    _InRef_     PC_EV_DATA p_ev_data)
{
    P_EV_DATA resultp, finalp;
    P_STACK_ENTRY custom_stackp;

    /* find the most recent custom */
    if((custom_stackp = stack_back_search(stack_ptr, EXECUTING_CUSTOM)) == NULL)
    {
        stack_ptr->type =_ERROR;
    }
    else
    {
        /* point to custom result area on stack */
        resultp = stack_index_ptr_data(custom_stackp->data.stack_executing_custom.stack_base, -2);

        if(custom_stackp->data.stack_executing_custom.in_array)
        {
            P_EV_DATA array_elementp;
            EV_DATA temp_data;

            temp_data.arg.ev_array = resultp->arg.ev_array; /* temp loan to get at array */

            array_elementp =
                ss_array_element_index_wr(&temp_data,
                                          custom_stackp->data.stack_executing_custom.x_pos,
                                          custom_stackp->data.stack_executing_custom.y_pos);

            if(data_is_array_range(p_ev_data))
                ev_data_set_error(array_elementp, EVAL_ERR_NESTEDARRAY);
            else
                ss_data_resource_copy(array_elementp, p_ev_data);

            finalp = array_elementp;
        }
        else
        {
            ss_data_resource_copy(resultp, p_ev_data);
            finalp = resultp;
        }

        if(finalp->did_num == RPN_DAT_ERROR)
        {
            if(finalp->arg.ev_error.type != ERROR_CUSTOM)
            {   /* if possible, mark error's origin as given row in a custom function */
                finalp->arg.ev_error.type = ERROR_CUSTOM;
                finalp->arg.ev_error.docno = stack_ptr->slr.docno;
                finalp->arg.ev_error.col = stack_ptr->slr.col;
                finalp->arg.ev_error.row = stack_ptr->slr.row;
            }
            ev_report_ERROR_CUSTOM(finalp);
        }

        /* reset stack to executing custom */
        stack_set(stack_offset(custom_stackp));

        /* switch to custom_complete */
        stack_ptr->type = CUSTOM_COMPLETE;
    }

    return(NEW_STATE);
}

/*ncr*/
static S32
custom_result_ERROR(
    STATUS error)
{
    EV_DATA err_res = { RPN_DAT_BLANK };
    ev_data_set_error(&err_res, error);
    return(custom_result(&err_res));
}

/******************************************************************************
*
* do the sequential steps involved in custom function execution
*
******************************************************************************/

static S32
custom_sequence(
    P_STACK_ENTRY semp)
{
    EV_SLR next_slot, last_slot;
    S32 res;

    next_slot = last_slot = semp->data.stack_executing_custom.next_slot;

    /* get pointer to result */
    res = 0;

    ++semp->data.stack_executing_custom.next_slot.row;
    last_slot.row = ev_numrow(ev_slr_docno(&last_slot));

    /* have we gone off end of file ? */
    if(next_slot.row >= last_slot.row)
    {
        res = create_error(EVAL_ERR_NORETURN);
    }
    /* switch to calculate next cell in custom */
    else if(stack_check_n(1) < 0)
    {
        res = status_nomem();
    }
    else
    {
        stack_inc(CALC_SLOT, next_slot, 0);
        stack_ptr->stack_flags |= STF_INCUSTOM;
        stack_ptr->data.stack_in_calc.eval_block.p_ev_cell = NULL;
        stack_ptr->data.stack_in_calc.start_calc = ev_serial_num;
    }

    /* if we get an error, abort and go to complete state */
    if(res < 0)
    {
        P_EV_DATA data_resp;

        data_resp = stack_index_ptr_data(semp->data.stack_executing_custom.stack_base, -2);
        ss_data_free_resources(data_resp);
        ev_data_set_error(data_resp, res);
        stack_ptr->type = CUSTOM_COMPLETE;
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* custom function control statement processing
*
******************************************************************************/

static S32
process_control(
    S32 action,
    P_EV_DATA args[], S32 n_args,
    S32 eval_stack_base)
{
    EV_SLR current_slot;

    /* save cell on which control statement encountered */
    current_slot = stack_ptr->slr;

    switch(action)
    {
    case CONTROL_GOTO:
        custom_jmp(&args[0]->arg.slr, 0, eval_stack_base - 1);
        break;

    case CONTROL_RESULT:
        custom_result(args[0]);
        break;

    case CONTROL_WHILE:
        if(args[0]->arg.integer)
        {
            /* while condition is true - start while:
             * clear stack and switch to while loop
             */
            stack_set(eval_stack_base);

            /* switch to control loop state */
            stack_ptr->type = CONTROL_LOOP;
            stack_ptr->data.stack_control_loop.control_type = CONTROL_WHILE;
            stack_ptr->data.stack_control_loop.origin_slot = current_slot;
        }
        else
        {
            /* while condition is false - continue
             * execution at next endwhile
             */
            process_control_search(EVS_CNT_WHILE,
                                   EVS_CNT_ENDWHILE,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        }
        break;

    /* found endwhile statement; pop last while
     * from stack and continue there
     */
    case CONTROL_ENDWHILE:
        {
        P_STACK_ENTRY stkentp = stack_back_search_loop(CONTROL_WHILE);

        if(NULL == stkentp)
        {
            custom_result_ERROR(EVAL_ERR_BADLOOPNEST);
        }
        else
        {
            custom_jmp(&stkentp->data.stack_control_loop.origin_slot, 0, stack_offset(stkentp - 1));
        }

        break;
        }

    /* if elseif encountered as result of if(FALSE)
     * fall thru to if to check argument, otherwise
     * skip to endif
     */
    case CONTROL_ELSEIF:
        {
        if(!stack_back_search(stack_ptr, EXECUTING_CUSTOM)->data.stack_executing_custom.elseif)
        {
            process_control_search(EVS_CNT_IFC,
                                   EVS_CNT_ENDIF,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADIFNEST);
            break;
        }
        }

    /* note fall thru */

    case CONTROL_IF:
        {
        P_STACK_ENTRY stkentp;

        stkentp = stack_back_search(stack_ptr, EXECUTING_CUSTOM);
        stkentp->data.stack_executing_custom.elseif = 0;

        /* if true, skip to next statement */
        if(args[0]->arg.integer)
            custom_jmp(&current_slot, 1, eval_stack_base - 1);
        else
        {
            /* after if(FALSE), look for:
             * endif:  continue normally
             * else:   continue normally
             * elseif: try again
             */
            if(process_control_search(EVS_CNT_IFC,
                                      EVS_CNT_ENDIF,
                                      EVS_CNT_ELSE,
                                      EVS_CNT_ELSEIF,
                                      &current_slot,
                                      1,
                                      eval_stack_base - 1,
                                      EVAL_ERR_BADIFNEST) == EVS_CNT_ELSEIF)
                stkentp->data.stack_executing_custom.elseif = 1;
        }

        break;
        }

    /* when else encountered
     * when executing, skip upto the endif - we
     * must have already had an if(TRUE) or elseif(TRUE)
     */
    case CONTROL_ELSE:
        process_control_search(EVS_CNT_IFC,
                               EVS_CNT_ENDIF,
                               EVS_CNT_NONE,
                               EVS_CNT_NONE,
                               &current_slot,
                               1,
                               eval_stack_base - 1,
                               create_error(EVAL_ERR_BADIFNEST));
        break;

    /* ENDIF encountered when executing - ignore by
     * moving onto next statement
     */
    case CONTROL_ENDIF:
        custom_jmp(&current_slot, 1, eval_stack_base - 1);
        break;

    case CONTROL_REPEAT:
        /* start repeat by resetting stack,
         * then pushing current position etc.
         */
        stack_set(eval_stack_base);

        /* switch to control loop state */
        stack_ptr->type = CONTROL_LOOP;
        stack_ptr->data.stack_control_loop.control_type = CONTROL_REPEAT;
        stack_ptr->data.stack_control_loop.origin_slot = current_slot;
        break;

    case CONTROL_UNTIL:
        {
        P_STACK_ENTRY stkentp = stack_back_search_loop(CONTROL_REPEAT);

        if(NULL == stkentp)
        {
            custom_result_ERROR(EVAL_ERR_BADLOOPNEST);
        }
        else if(0 == args[0]->arg.integer)
        {
            custom_jmp(&stkentp->data.stack_control_loop.origin_slot, 1, eval_stack_base - 1);
        }
        else
        {
            custom_jmp(&current_slot, 1, stack_offset(stkentp - 1));
        }

        break;
        }

    case CONTROL_FOR:
        {
        STATUS res;
        STACK_CONTROL_LOOP stack_control_loop;

        stack_control_loop.control_type = CONTROL_FOR;
        stack_control_loop.origin_slot = current_slot;
        stack_control_loop.end = args[2]->arg.fp;

        if(n_args > 3)
            stack_control_loop.step = args[3]->arg.fp;
        else
            stack_control_loop.step = 1.;

        if((res = name_make(&stack_control_loop.nameid,
                            stack_ptr->slr.docno,
                            args[0]->arg.string.uchars,
                            args[1])) < 0)
        {
            custom_result_ERROR(res);
        }
        else if(!process_control_for_cond(&stack_control_loop, 0))
        {
            process_control_search(EVS_CNT_FOR,
                                   EVS_CNT_NEXT,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        }
        else
        {
            /* clear stack and switch to for loop */
            stack_set(eval_stack_base - 1);

            /* removing FOR args made room for this */
            stack_inc(CONTROL_LOOP, stack_ptr[-1].slr, stack_ptr[-1].stack_flags);
            stack_ptr->data.stack_control_loop = stack_control_loop;
        }

        break;
        }

    case CONTROL_NEXT:
        {
        P_STACK_ENTRY stkentp = stack_back_search_loop(CONTROL_FOR);

        if(NULL == stkentp)
        {
            custom_result_ERROR(EVAL_ERR_BADLOOPNEST);
        }
        else if(process_control_for_cond(&stkentp->data.stack_control_loop, 1))
        {
            custom_jmp(&stkentp->data.stack_control_loop.origin_slot, 1, eval_stack_base - 1);
        }
        else
        {
            custom_jmp(&current_slot, 1, stack_offset(stkentp - 1));
        }

        break;
        }

    case CONTROL_BREAK:
        {
        S32 loop_count;
        P_STACK_ENTRY stkentp;

        if(n_args)
            loop_count = (S32) args[0]->arg.integer;
        else
            loop_count = 1;

        loop_count = MAX(1, loop_count);

        stkentp = stack_ptr;
        while(loop_count--)
            if(NULL == (stkentp = stack_back_search(stkentp, CONTROL_LOOP)))
                break;

        if(NULL == stkentp)
        {
            custom_result_ERROR(EVAL_ERR_BADLOOPNEST);
        }
        else
        {
            S32 block_start, block_end;
            EV_SLR slr;

            switch(stkentp->data.stack_control_loop.control_type)
            {
            case CONTROL_REPEAT:
                block_start = EVS_CNT_REPEAT;
                block_end   = EVS_CNT_UNTIL;
                break;

            case CONTROL_FOR:
                block_start = EVS_CNT_FOR;
                block_end   = EVS_CNT_NEXT;
                break;

            case CONTROL_WHILE:
            default:
                block_start = EVS_CNT_WHILE;
                block_end   = EVS_CNT_ENDWHILE;
                break;
            }

            slr = stkentp->data.stack_control_loop.origin_slot;
            slr.row += 1;
            process_control_search(block_start,
                                   block_end,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &slr,
                                   1,
                                   stack_offset(stkentp - 1),
                                   EVAL_ERR_BADLOOPNEST);
        }

        break;
        }

    case CONTROL_CONTINUE:
        {
        P_STACK_ENTRY stkentp = stack_back_search(stack_ptr, CONTROL_LOOP);

        if(NULL == stkentp)
        {
            custom_result_ERROR(EVAL_ERR_BADLOOPNEST);
        }
        else
        {
            S32 block_start, block_end;

            switch(stkentp->data.stack_control_loop.control_type)
            {
            case CONTROL_REPEAT:
                block_start = EVS_CNT_REPEAT;
                block_end   = EVS_CNT_UNTIL;
                break;

            case CONTROL_FOR:
                block_start = EVS_CNT_FOR;
                block_end   = EVS_CNT_NEXT;
                break;

            case CONTROL_WHILE:
            default:
                block_start = EVS_CNT_WHILE;
                block_end   = EVS_CNT_ENDWHILE;
                break;
            }

            process_control_search(block_start,
                                   block_end,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   0,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        }

        break;
        }
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* process a for condition
*
******************************************************************************/

static S32
process_control_for_cond(
    P_STACK_CONTROL_LOOP p_stack_control_loop,
    _InVal_     BOOL step)
{
    S32 res = 0;
    EV_NAMEID name_num = name_def_find(p_stack_control_loop->nameid);

    if(name_num >= 0)
    {
        const P_EV_NAME p_ev_name = name_ptr_must(name_num);

        if(RPN_DAT_REAL == p_ev_name->def_data.did_num)
        {
            if(step)
                p_ev_name->def_data.arg.fp += p_stack_control_loop->step;

            if(p_stack_control_loop->step >= 0)
                res = !(p_ev_name->def_data.arg.fp > p_stack_control_loop->end);
            else
                res = !(p_ev_name->def_data.arg.fp < p_stack_control_loop->end);
        }
    }

    return(res);
}

/******************************************************************************
*
* search forwards for control statements - the
* search stops on a block_end type, but nested
* blocks are counted as the search proceeds
* block_end_maybe1/2 are optional extra stop points
*
******************************************************************************/

static S32
process_control_search(
    S32 block_start,
    S32 block_end,
    S32 block_end_maybe1,
    S32 block_end_maybe2,
    _InRef_     PC_EV_SLR slrp,
    EV_ROW offset,
    S32 stack_after,
    S32 error)
{
    S32 found_type = 0 /* keep dataflower happy */;
    S32 nest, found, res;
    EV_ROW last_row;
    EV_SLR slr;

    slr      = *slrp;
    nest     = found = 0;
    last_row = ev_numrow(ev_slr_docno(&slr));

    while(!found && ++slr.row < last_row)
    {
        P_EV_CELL p_ev_cell;

        if(ev_travel(&p_ev_cell, &slr) > 0)
        {
            if(p_ev_cell->parms.control == (unsigned) block_start)
                ++nest;
            else if(p_ev_cell->parms.control == (unsigned) block_end)
            {
                if(nest)
                    --nest;
                else
                {
                    found_type = p_ev_cell->parms.control;
                    found = 1;
                }
            }
            else if(!nest)
            {
                if(p_ev_cell->parms.control == (unsigned) block_end_maybe1 ||
                   p_ev_cell->parms.control == (unsigned) block_end_maybe2)
                {
                    found_type = p_ev_cell->parms.control;
                    found = 1;
                }
            }
        }
    }

    if(!found)
    {
        custom_result_ERROR(error);
        res = error;
    }
    else
    {
        EV_ROW jmp_offset;

        /* elseifs must be evaluated */
        if(found_type == EVS_CNT_ELSEIF)
            jmp_offset = 0;
        else
            jmp_offset = offset;

        custom_jmp(&slr, jmp_offset, stack_after);
        res = found_type;
    }

    return(res);
}

/******************************************************************************
*
* read a custom id from the rpn string
* and look it up in the custom table
*
* --out--
* < 0 error
* >=0 definition found OK
*
******************************************************************************/

_Check_return_
static STATUS
read_check_custom_def(
    PC_U8 ip_pos,
    EV_NAMEID *custom_num,
    P_P_EV_CUSTOM p_p_ev_custom)
{
    EV_NAMEID custom_id;

    read_nameid(&custom_id, ip_pos);

    if((*custom_num = custom_def_find(custom_id)) < 0)
        return(create_error(EVAL_ERR_CUSTOMUNDEF));

    *p_p_ev_custom = custom_ptr_must(*custom_num);

    if(!((*p_p_ev_custom)->flags & TRF_UNDEFINED))
    {
        P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno((*p_p_ev_custom)->owner.docno);

        if((NULL != p_ss_doc)  &&  !p_ss_doc->is_docu_thunk)
            return(STATUS_OK);

        return(create_error(EVAL_ERR_CANTEXTREF));
    }

    return(create_error(EVAL_ERR_CUSTOMUNDEF));
}

/******************************************************************************
*
* function called to recalc sheet
*
* call as often as possible; it will return
* as often as is reasonable
*
******************************************************************************/

extern void
ev_recalc(void)
{
    static S32 complete = 1;

    if(tree_flags & TRF_BLOWN)
    {
        EV_SLR slr;

        /* see if there's anything to do */
        if(!todo_get_slr(&slr))
        {
            ev_recalc_status(0);
            return;
        }

        /* give up right away with no stack */
        if(stack_check_n(1) < 0)
            return;

        /* if evaluator has been disturbed,
         * clear the stack and restart
         */
        stack_zap(NULL);

        /* tell the world there's something to do */
        ev_recalc_status(todotab.next);

        /* get trees ready for action */
        tree_sort_all();

        trace_0(TRACE_MODULE_EVAL, "**************<recalc restarting after blow-up>*************");

        tree_flags &= ~TRF_BLOWN;
        complete = 1;
    }

    #if TRACE_ALLOWED
    if(!complete)
        eval_trace("-------- recalc continuing -------");
    #endif

    time_started = monotime();

    /* lock trees */
    tree_flags |= TRF_LOCK;

    do  {
        /* get next cell from todo list */
        if(complete)
        {
            if(todo_get_slr(&stack_ptr->slr))
            {
                #if TRACE_ALLOWED
                eval_trace("*******************<ev_recalc>*********************");
                #endif

                stack_ptr->type = VISIT_SLOT;
                stack_ptr->stack_flags = 0;

                grub_init(&stack_ptr->data.stack_visit_slot.grubb, &stack_ptr->slr);
                complete = 0;
            }
            else
                /* nothing to do - go home */
                break;
        }

        /* continue according to state on stack */
        switch(stack_ptr->type)
        {
        case VISIT_SLOT:
            {
            P_EV_CELL p_ev_cell;
            S32 travel_res, res;

            if((travel_res = ev_travel(&p_ev_cell, &stack_ptr->slr)) > 0)
            {
                S32 did_num;

                if(!stack_ptr->data.stack_visit_slot.grubb.offset)
                {
                    if(p_ev_cell->parms.type == EVS_VAR_RPN &&
                       p_ev_cell->rpn.var.visited >= ev_serial_num)
                    {
                        eval_trace("<visit_slot> ");
                        trace_2(TRACE_MODULE_EVAL,
                                "avoided cell visit: visited: %d, serial_num: %d",
                                p_ev_cell->rpn.var.visited,
                                ev_serial_num);

                        /* remove this cell's entry from the todo list */
                        todo_remove_slr(&stack_ptr->slr);

                        /* continue with what we were doing previously */
                        if(!stack_offset(stack_ptr))
                            /* if no previous it's all done */
                            complete = 1;
                        else
                            --stack_ptr;

                        break;
                    }

                    eval_trace("<visit_slot>");

                    if(circ_check(p_ev_cell) == NEW_STATE)
                        break;
                }

                /* check we have enough stack to push current state
                 * and a visiting state
                 */
                if(stack_check_n(3) < 0)
                {
                    slot_error_complete(status_nomem());
                    break;
                }

                res = 0;
                while(!res && (did_num = grub_next(p_ev_cell, &stack_ptr->data.stack_visit_slot.grubb)) != RPN_FRM_END)
                {
                    switch(did_num)
                    {
                    case RPN_DAT_SLR:
                        /* we must go and visit the cell;
                         * stack the current state
                         */
                        stack_inc(VISIT_SLOT, stack_ptr[-1].data.stack_visit_slot.grubb.data.arg.slr, 0);
                        grub_init(&stack_ptr->data.stack_visit_slot.grubb, &stack_ptr->slr);
                        res = 1;
                        break;

                    /* may need two stack entries */
                    case RPN_DAT_RANGE:
                        if(check_supporting_rng(&stack_ptr->data.stack_visit_slot.grubb) == NEW_STATE)
                            res = 1;
                        break;

                    /* may need three stack entries */
                    case RPN_DAT_NAME:
                        if(check_supporting_name(&stack_ptr->data.stack_visit_slot.grubb) == NEW_STATE)
                            res = 1;
                        break;
                    }
                }

                if(res)
                    break;
            }

            /* all cell's supporters are calced - check
            if this cell now needs to be recalced */
            stack_ptr->type = CALC_SLOT;
            stack_ptr->data.stack_in_calc.eval_block.p_ev_cell = p_ev_cell;
            stack_ptr->data.stack_in_calc.travel_res = travel_res;
            stack_ptr->data.stack_in_calc.start_calc = ev_serial_num;

            trace_2(TRACE_MODULE_EVAL,
                    "VISIT_SLOT sic.start_calc set to: %d, serial_num: %d",
                    stack_ptr->data.stack_in_calc.start_calc,
                    ev_serial_num);

            break;
            }

        case VISIT_SUPPORTRNG:
            visit_supporting_range();
            break;

        /* called to start recalc of a cell
         */
        case CALC_SLOT:
            {
            P_TODO_ENTRY todop;
            S32 changed;

            /* flip to end state */
            stack_ptr->type = END_CALC;

            if((todop = todo_has_slr(&stack_ptr->slr)) != NULL &&
              !(todop->flags & TRF_TOBEDEL))
                changed = 1;
            else
                changed = 0;

            /* recache pointer */
            stack_in_calc_ensure_slot(&stack_ptr->data.stack_in_calc, &stack_ptr->slr, "CALC_SLOT");

            if((stack_ptr->data.stack_in_calc.travel_res > 0)                            &&
               !(stack_ptr->stack_flags & STF_CALCEDERROR)                               &&
               (doc_check_custom(stack_ptr->slr.docno)                                 ||
                (stack_ptr->stack_flags & STF_CALCEDSUPPORTER)                         ||
                (stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->parms.type == EVS_VAR_RPN &&
                 changed)                                                                        ||
                (stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->parms.type == EVS_CON_RPN &&
                 changed)                                                                        ||
                (stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result.did_num == RPN_DAT_ERROR &&
                 stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result.arg.ev_error.status == STATUS_NOMEM)
               )
              )
            {
                stack_ptr->data.stack_in_calc.eval_block.offset = 0;
                stack_ptr->data.stack_in_calc.eval_block.slr = stack_ptr->slr;
                stack_ptr->data.stack_in_calc.did_calc = 1;
                stack_ptr->data.stack_in_calc.type = INCALC_SLR;

                eval_rpn(stack_offset(stack_ptr));
            }
            else
                stack_ptr->data.stack_in_calc.did_calc = 0;

            break;
            }

        /* called during recalc of a cell after eval_rpn
         * has released control for some reason
         */
        case IN_EVAL:
            {
            eval_trace("<IN_EVAL>");

            #ifdef SLOTS_MOVE
            /* clear out any cached cell pointer for later reload */
            stack_base[stack_ptr->data.stack_in_eval.stack_offset].data.stack_in_calc.eval_block.p_ev_cell = NULL;
            #endif

            /* remove IN_EVAL state and continue with recalc */
            eval_rpn((stack_ptr--)->data.stack_in_eval.stack_offset);
            break;
            }

        /* called when recalculation of a cell is over
         */
        case END_CALC:
            {
            S32 calc_parent;
            P_TODO_ENTRY todop;

            /* recache pointer */
            stack_in_calc_ensure_slot(&stack_ptr->data.stack_in_calc, &stack_ptr->slr, "END_CALC");

            /* did we actually get a result ? */
            if(stack_ptr->data.stack_in_calc.travel_res > 0 &&
               stack_ptr->data.stack_in_calc.did_calc)
            {
                /* store result in cell */
                ev_result_free_resources(&stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result);
                ev_data_to_result_convert(&stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result,
                                          &stack_ptr->data.stack_in_calc.result_data);

                /* on error in custom function sheet, return custom function result error */
                if(stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result.did_num == RPN_DAT_ERROR &&
                   doc_check_custom(stack_ptr->slr.docno))
                {
                    EV_DATA data;

                    data.did_num = RPN_DAT_ERROR;
                    data.arg.ev_error = stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result.arg.ev_error;

                    if(data.arg.ev_error.type != ERROR_CUSTOM)
                    {
                        data.arg.ev_error.type = stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->ev_result.arg.ev_error.type = ERROR_CUSTOM;
                    
                        data.arg.ev_error.docno = stack_ptr->slr.docno;
                        data.arg.ev_error.col = stack_ptr->slr.col;
                        data.arg.ev_error.row = stack_ptr->slr.row;
                    }

                    custom_result(&data);
                    break;
                }
            }

            if(!doc_check_custom(stack_ptr->slr.docno)               &&
               (stack_ptr->data.stack_in_calc.did_calc             ||
                stack_ptr->stack_flags & STF_CALCEDERROR           ||
                (stack_ptr->data.stack_in_calc.travel_res < 0 &&
                 (todop = todo_has_slr(&stack_ptr->slr)) != NULL &&
                 !(todop->flags & TRF_TOBEDEL)
                )
               )
              )
            {
                UREF_PARM urefb;

                /* notify anybody about the change */
                urefb.slr1   = stack_ptr->slr;
                urefb.action = UREF_CHANGE;
                ev_uref(&urefb);
                urefb.slr1   = stack_ptr->slr;
                urefb.action = UREF_REDRAW;
                ev_uref(&urefb);

                /* mark dependents for recalc */
                todo_add_dependents(&stack_ptr->slr);
            }

            if(stack_ptr->data.stack_in_calc.travel_res > 0)
            {
                /* clear circular flag and set visited */
                if(stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->parms.type == EVS_VAR_RPN)
                {
                    stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->rpn.var.visited = stack_ptr->data.stack_in_calc.start_calc;
                    trace_2(TRACE_MODULE_EVAL,
                            "END_CALC, p_ev_cell->visited set to: %d, serial_num: %d",
                            stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->rpn.var.visited,
                            ev_serial_num);
                }

                stack_ptr->data.stack_in_calc.eval_block.p_ev_cell->parms.circ = 0;
            }

            /* remove this cell's entry from the todo list */
            todo_remove_slr(&stack_ptr->slr);

            if((stack_ptr->stack_flags & STF_CALCEDSUPPORTER) ||
                stack_ptr->data.stack_in_calc.did_calc)
                calc_parent = 1;
            else
                calc_parent = 0;

            /* continue with what we were doing previously */
            if(!stack_offset(stack_ptr))
                /* if no previous it's all done */
                complete = 1;
            /* update parent's supporter change */
            else
            {
                --stack_ptr;
                if(calc_parent)
                    stack_ptr->stack_flags |= STF_CALCEDSUPPORTER;
            }

            break;
            }

        case VISIT_COMPLETE_NAME:
            {
            EV_FLAGS changed;
            /* look up name definition */
            EV_NAMEID name_num = name_def_find(stack_ptr->data.stack_visit_name.nameid);

            if(name_num >= 0)
            {
                P_EV_NAME p_ev_name = name_ptr_must(name_num);

                p_ev_name->visited = ev_serial_num;
            }

            changed = stack_ptr->stack_flags & (EV_FLAGS) STF_CALCEDSUPPORTER;

            /* continue with what we were doing previously */
            --stack_ptr;

            /* update parent's changed state */
            stack_ptr->stack_flags |= changed;

            break;
            }

        /* called to start a loop; the loop data
         * is pushed onto the stack; execution continues
         * at the cell after the origin cell
         */
        case CONTROL_LOOP:
            {
            P_STACK_ENTRY stkentp;

            if((stkentp = stack_back_search(stack_ptr, EXECUTING_CUSTOM)) == NULL)
                stack_ptr->type =_ERROR;
            else
                custom_sequence(stkentp);
            break;
            }

        /* called for each cell in a database condition
         * eval_rpn checks that stack is available
         * needs 1 stack entry
         */
        case DBASE_FUNCTION:
            {
            P_EV_CELL p_ev_cell;
            S32 error = 0;

            /* set up conditional string ready for munging */
            if(ev_travel(&p_ev_cell, &stack_ptr->data.stack_dbase.dbase_slot) > 0)
            {
                char dbase_rpn[EV_MAX_OUT_LEN + 1];
                S32 rpn_len;
                P_U8 rpn_ptr;
                EV_SLR slr;

                /* calculate slr for cell being processed conditionally */
                slr = stack_ptr->data.stack_dbase.dbase_rng.s;

                /* check for errors with external refs */
                if((slr.flags & SLR_EXT_REF) && ev_doc_error(slr.docno))
                    error = ev_doc_error(slr.docno);
                else
                {
                    EV_SLR target_slr;
                    S32 abs_col, abs_row;

                    /* calculate slr of cell in database range */
                    slr.col += stack_ptr->data.stack_dbase.offset.col;
                    slr.row += stack_ptr->data.stack_dbase.offset.row;

                    target_slr.col = stack_ptr->data.stack_dbase.offset.col;
                    target_slr.row = stack_ptr->data.stack_dbase.offset.row;
                    abs_col = abs_row = 0;

                    if(stack_ptr->data.stack_dbase.dbase_rng.s.col + 1 != stack_ptr->data.stack_dbase.dbase_rng.e.col)
                    {
                        target_slr.col = slr.col;
                        abs_col = 1;
                    }

                    if(stack_ptr->data.stack_dbase.dbase_rng.s.row + 1 != stack_ptr->data.stack_dbase.dbase_rng.e.row)
                    {
                        target_slr.row = slr.row;
                        abs_row = 1;
                    }

                    rpn_len = ev_proc_conditional_rpn(dbase_rpn,
                                                      p_ev_cell->rpn.var.rpn_str + stack_ptr->data.stack_dbase.cond_pos,
                                                      &target_slr, abs_col, abs_row);

                    if(NULL != (rpn_ptr = al_ptr_alloc_bytes(P_U8, rpn_len, &error)))
                    {
                        stack_inc(DBASE_CALC, slr, stack_ptr[-1].stack_flags);
                        memcpy32(rpn_ptr, dbase_rpn, rpn_len);

                        stack_ptr->data.stack_in_calc.eval_block.slr = slr;
                        stack_ptr->data.stack_in_calc.eval_block.offset = 0;
                        stack_ptr->data.stack_in_calc.ptr = rpn_ptr;
                        stack_ptr->data.stack_in_calc.type = INCALC_PTR;

                        /* go to evaluate condition */
                        eval_rpn(stack_offset(stack_ptr));
                    }
                }
            }
            else
                error = create_error(EVAL_ERR_INTERNAL);

            /* on error, cancel function and pop previous state,
             * pushing error as result
             */
            if(error)
            {
                /* pop previous state */
                stack_ptr[0] = stack_ptr[-1];

                stack_ptr[-1].type = DATA_ITEM;
                ev_data_set_error(&stack_ptr[-1].data.stack_data_item.data, error);
            }

            break;
            }

        /* receives result of database condition calculation */
        case DBASE_CALC:
            {
            EV_SLR span;

            /* throw away current conditional rpn string */
            al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&stack_ptr->data.stack_in_calc.ptr));

            /* call the dbase routine to process the data */
            dbase_sub_function(&stack_ptr[-1].data.stack_dbase, &stack_ptr->data.stack_in_calc.result_data);

            /* remove DBASE_CALC state from stack */
            --stack_ptr;
            stack_ptr->data.stack_dbase.offset.col += 1;

            span.col = stack_ptr->data.stack_dbase.dbase_rng.e.col -
                       stack_ptr->data.stack_dbase.dbase_rng.s.col;
            span.row = stack_ptr->data.stack_dbase.dbase_rng.e.row -
                       stack_ptr->data.stack_dbase.dbase_rng.s.row;

            if(stack_ptr->data.stack_dbase.offset.col >= span.col)
            {
                stack_ptr->data.stack_dbase.offset.col  = 0;
                stack_ptr->data.stack_dbase.offset.row += 1;

                /* check if function finished yet */
                if(stack_ptr->data.stack_dbase.offset.row >= span.row)
                {
                    EV_DATA data;

                    dbase_sub_function_finish(&data, &stack_ptr->data.stack_dbase);
                    al_ptr_dispose(P_P_ANY_PEDANTIC(&stack_ptr->data.stack_dbase.p_stat_block));

                    /* pop previous state
                     * push dbase result
                     * this leaves copies of flags & slr in [-1]
                     */
                    stack_ptr[0] = stack_ptr[-1];

                    stack_ptr[-1].type = DATA_ITEM;
                    stack_ptr[-1].data.stack_data_item.data = data;
                }
            }
            break;
            }

        case LOOKUP_HAPPENING:
            {
            S32 res;

            if((res = lookup_array_range_proc(stack_ptr->data.stack_lookup.p_lookup_block,
                                              &stack_ptr->data.stack_lookup.arg1)) >= 0)
            {
                EV_DATA data;

                if(!res || !stack_ptr->data.stack_lookup.p_lookup_block->had_one)
                    ev_data_set_error(&data, create_error(EVAL_ERR_LOOKUP));
                else
                    lookup_finish(&data, &stack_ptr->data.stack_lookup);

                lookup_block_dispose(&stack_ptr->data.stack_lookup);

                /* pop previous state
                 * push lookup result
                 * this leaves copies of flags & slr in [-1]
                 */
                stack_ptr[0] = stack_ptr[-1];

                stack_ptr[-1].type = DATA_ITEM;
                stack_ptr[-1].data.stack_data_item.data = data;
            }
            break;
            }

        /* called as result of RESULT function or
         * error during custom evaluation
        */
        case CUSTOM_COMPLETE:
            {
            S32 custom_over, n_args;
            STACK_ENTRY result, state;

            custom_over = 0;
            n_args = stack_ptr->data.stack_executing_custom.n_args;

            if(stack_ptr->data.stack_executing_custom.in_array)
            {
                ++stack_ptr->data.stack_executing_custom.x_pos;
                if(stack_ptr->data.stack_executing_custom.x_pos >= stack_ptr[-1].data.stack_data_item.data.arg.ev_array.x_size)
                {
                    stack_ptr->data.stack_executing_custom.x_pos  = 0;
                    stack_ptr->data.stack_executing_custom.y_pos += 1;

                    if(stack_ptr->data.stack_executing_custom.y_pos >= stack_ptr[-1].data.stack_data_item.data.arg.ev_array.y_size)
                        custom_over = 1;
                }
            }
            else
                custom_over = 1;

            /* custom may need re-calling for next array element */
            if(!custom_over)
            {
                stack_ptr->data.stack_executing_custom.next_slot =
                stack_ptr->data.stack_executing_custom.custom_slot;
                stack_ptr->type = EXECUTING_CUSTOM;
                break;
            }

            /* pop custom result */
            --stack_ptr;
            result = stack_ptr[0];

            /* pop previous state */
            --stack_ptr;
            state = stack_ptr[0];

            /* pop custom arguments from stack */
            stack_set(stack_offset(stack_ptr) - 1 - n_args);

            /* push custom result */
            stack_ptr += 1;
            stack_ptr[0] = result;

            /* set state */
            stack_ptr += 1;
            stack_ptr[0] = state;
            break;
            }

        /* whilst executing custom, select
         * next sequential cell for evaluation
        */
        case EXECUTING_CUSTOM:
            custom_sequence(stack_ptr);
            break;

        /* the semantic routine is called
         * for each element in the argument arrays
        */
        case PROCESSING_ARRAY:
            {
            EV_DATA   arg_dat[EV_MAX_ARGS];
            P_EV_DATA  args_in[EV_MAX_ARGS];
            P_EV_DATA  args   [EV_MAX_ARGS];
            P_EV_DATA *datapp;
            S32 ix, typec, max_x, max_y, res;
            STACK_PROCESSING_ARRAY stack_processing_array;
            PC_EV_TYPE typep;
            P_EV_DATA resp;

            if(stack_ptr->data.stack_processing_array.x_pos >= stack_ptr[-1].data.stack_data_item.data.arg.ev_array.x_size)
            {
                stack_ptr->data.stack_processing_array.x_pos  = 0;
                stack_ptr->data.stack_processing_array.y_pos += 1;

                /* have we completed array ? */
                if(stack_ptr->data.stack_processing_array.y_pos >= stack_ptr[-1].data.stack_data_item.data.arg.ev_array.y_size)
                {
                    S32 n_args = stack_ptr->data.stack_processing_array.n_args; /* SKS fixed 01may95 in Fireworkz; 21mar12 in PipeDream! */
                    STACK_ENTRY result, state;

                    --stack_ptr;
                    result = stack_ptr[0];

                    --stack_ptr;
                    state  = stack_ptr[0];

                    /* remove array arguments from stack */
                    stack_set(stack_offset(stack_ptr) - 1 - n_args);

                    /* push result of array fuddling on stack;
                     * assume we can push since we just popped
                     */
                    stack_ptr += 1;
                    stack_ptr[0] = result;

                    stack_ptr += 1;
                    stack_ptr[0] = state;
                    break;
                }
            }

            /* make a copy of processing_array data */
            stack_processing_array = stack_ptr->data.stack_processing_array;

            stack_ptr->data.stack_processing_array.x_pos += 1;

            /* get the arguments and array pointers */
            for(ix = 0, datapp = args_in, typec = stack_processing_array.type_count,
                typep = stack_processing_array.arg_types;
                ix < stack_processing_array.n_args;
                ++ix, ++datapp)
            {
                *datapp = stack_index_ptr_data(stack_processing_array.stack_base,
                                               stack_processing_array.n_args - (S32) ix - 1);

                /* replace stack array pointer with
                 * pointer to relevant array element
                 */
                if(typep && (*typep & EM_ARY))
                    args[ix] = *datapp;
                else
                {
                    if((*datapp)->did_num == RPN_DAT_RANGE)
                    {
                        EV_SLR slr;

                        slr = (*datapp)->arg.range.s;
                        slr.col += stack_processing_array.x_pos;
                        slr.row += stack_processing_array.y_pos;

                        ev_slr_deref(&arg_dat[ix], &slr, FALSE);
                        args[ix] = &arg_dat[ix];
                    }
                    else
                    {
                        /* don't give a pointer into the array itself,
                         * otherwise someone will poo on it
                         */
                        arg_dat[ix] = *ss_array_element_index_borrow((*datapp), stack_processing_array.x_pos, stack_processing_array.y_pos);
                        args[ix] = &arg_dat[ix];
                    }
                }

                if(typec > 1)
                {
                    ++typep;
                    --typec;
                }
            }

            /* get pointer to result element */
            resp = ss_array_element_index_wr(&stack_ptr[-1].data.stack_data_item.data, stack_processing_array.x_pos, stack_processing_array.y_pos);

            /* call semantic routine with array
             * elements as arguments instead of arrays
             */
            if((res = args_check(stack_processing_array.n_args, args,
                                 stack_processing_array.type_count, stack_processing_array.arg_types,
                                 resp,
                                 &max_x, &max_y)) == 0)
                (*stack_processing_array.exec)(args, stack_processing_array.n_args, resp, &stack_ptr->slr);
            else if(res > 0)
            {
                ss_data_free_resources(resp);
                ev_data_set_error(resp, create_error(EVAL_ERR_NESTEDARRAY));
            }

            /* if function returned an array, correct and complain */
            if(data_is_array_range(resp))
            {
                ss_data_free_resources(resp);
                ev_data_set_error(resp, create_error(EVAL_ERR_NESTEDARRAY));
            }

            break;
            }

        case ALERT_INPUT:
            {
            S32 res;
            EV_DATA ev_data;

            switch(stack_ptr->data.stack_alert_input.alert_input)
            {
            case RPN_FNV_ALERT:
                if((res = ev_alert_poll()) >= 0)
                {
                    ev_data_set_integer(&ev_data, (S32) res);
                    ev_alert_close();
                }
                break;

            case RPN_FNV_INPUT:
                {
                char result_string[BUF_EV_LONGNAMLEN];

                if((res = ev_input_poll(result_string, EV_LONGNAMLEN)) >= 0)
                {
                    EV_DATA string_data;

                    /* get rid of input box */
                    ev_input_close();

                    if(status_ok(ss_string_make_ustr(&string_data, result_string)))
                    {
                        S32 name_res;
                        EV_NAMEID name_key;

                        if((name_res = name_make(&name_key,
                                                 stack_ptr->slr.docno,
                                                 stack_ptr->data.stack_alert_input.name_id,
                                                 &string_data)) < 0)
                        {
                            ss_data_free_resources(&string_data);
                            ev_data_set_error(&ev_data, name_res);
                        }
                        else
                        {
                            EV_NAMEID name_num = name_def_find(name_key);
                            assert(name_num >= 0);

                            name_ptr_must(name_num)->def_data = string_data;

                            ev_data_set_integer(&ev_data, (S32) res);
                        }
                    }
                }
                break;
                }
            }

            if(res >= 0)
            {
                /* pop previous state
                 * push alert/input result
                 * this leaves copies of flags & slr in [-1]
                 */
                stack_ptr[0] = stack_ptr[-1];

                stack_ptr[-1].type = DATA_ITEM;
                stack_ptr[-1].data.stack_data_item.data = ev_data;
            }

            break;
            }

        /* all other states are treated as an error condition */
        /*case_ERROR:*/
        default:
            stack_zap(NULL);
            slot_set_error(&stack_ptr->slr, /*create_error*/(EVAL_ERR_INTERNAL));
            complete = 1;
            break;
        }
        }
    #if TRACE_ALLOWED
    while(monotime_diff(time_started) <= (trace_is_on()
                                          ? BACKGROUND_SLICE * 10
                                          : BACKGROUND_SLICE));
    #else
    while(monotime_diff(time_started) <= BACKGROUND_SLICE);
    #endif

    #ifdef SLOTS_MOVE
    /* clear cached cell pointer when leaving evaluator */
    switch(stack_ptr->type)
    {
    case CALC_SLOT:
    case END_CALC:
        stack_ptr->data.stack_in_calc.eval_block.p_ev_cell = NULL;
        break;
    }
    #endif

    ev_recalc_status(todotab.next);

    tree_flags &= ~TRF_LOCK;
}

/******************************************************************************
*
* when an error is encountered processing a cell,
* store the error and terminate the evaluation
*
******************************************************************************/

static S32
slot_error_complete(
    STATUS error)
{
    slot_set_error(&stack_ptr->slr, error);

    stack_ptr->type = CALC_SLOT;
    stack_ptr->data.stack_in_calc.start_calc = ev_serial_num;
    stack_ptr->data.stack_in_calc.eval_block.p_ev_cell  = NULL;

    stack_ptr->stack_flags |= STF_CALCEDERROR | STF_CALCEDSUPPORTER;

    return(NEW_STATE);
}

/******************************************************************************
*
* store an error in a cell and switch state
* to calculating the cell's dependents
*
******************************************************************************/

/*ncr*/
static STATUS
slot_set_error(
    _InRef_     PC_EV_SLR slrp,
    _InVal_     STATUS error)
{
    P_EV_CELL p_ev_cell;

    if(ev_travel(&p_ev_cell, slrp) > 0)
    {
        ev_result_free_resources(&p_ev_cell->ev_result);

        p_ev_cell->ev_result.did_num = RPN_DAT_ERROR;
        p_ev_cell->ev_result.arg.ev_error.status  = error;
        p_ev_cell->ev_result.arg.ev_error.type = ERROR_NORMAL;
    }

    return(error);
}

/******************************************************************************
*
* search back up stack for an entry of the given type
*
******************************************************************************/

static P_STACK_ENTRY
stack_back_search(
    P_STACK_ENTRY start_ptr,
    S32 entry_type)
{
    P_STACK_ENTRY stkentp, res;

    /* search back up stack for the entry */
    for(stkentp = start_ptr - 1, res = NULL;
        stkentp > stack_base && !res;
        --stkentp)
        if(stkentp->type == (unsigned) entry_type)
            res = stkentp;

    return(res);
}

/******************************************************************************
*
* search back up stack for a control entry of the given type
*
******************************************************************************/

static P_STACK_ENTRY
stack_back_search_loop(
    S32 loop_type)
{
    P_STACK_ENTRY stkentp;

    /* find the most recent loop */
    if((stkentp = stack_back_search(stack_ptr, CONTROL_LOOP)) == NULL)
        return(NULL);

    if(stkentp->data.stack_control_loop.control_type != loop_type)
        return(NULL);

    return(stkentp);
}

/******************************************************************************
*
* free any resources owned by a data item in a stack entry
*
******************************************************************************/

static void
stack_free_resources(
    P_STACK_ENTRY stkentp,
    _InRef_     PC_UREF_PARM upp)
{
    /* free resources allocated to data items on stack */
    switch(stkentp->type)
    {
    case DATA_ITEM:
        {
#if 1 /* SKS 21oct12 - simplify like Fireworkz */
        ss_data_free_resources(&stkentp->data.stack_data_item.data);
#else
        switch(stkentp->data.stack_data_item.data.did_num)
        {
        case RPN_TMP_STRING:
            trace_1(TRACE_MODULE_EVAL,
                    "stack_free_resources freeing string: %s",
                    stkentp->data.stack_data_item.data.arg.string.uchars);
            str_clr(&stkentp->data.stack_data_item.data.arg.string.uchars);
            break;

        case RPN_TMP_ARRAY:
            ss_array_free(&stkentp->data.stack_data_item.data);
            break;
        }
#endif
        break;
        }

    /* blow away the circ bit for incompletely
     * calculated and visited cells
     */
    case CALC_SLOT:
    case END_CALC:
        {
        S32 res;

        if(upp)
            res = ev_match_slr(&stkentp->slr, upp);
        else
            res = DEP_UPDATE;

        /* we must do travel since document may have been deleted */
        if(res != DEP_DELETE && ev_travel(&stkentp->data.stack_in_calc.eval_block.p_ev_cell, &stkentp->slr) > 0)
            stkentp->data.stack_in_calc.eval_block.p_ev_cell->parms.circ = 0;
        break;
        }

    case VISIT_SLOT:
        {
        P_EV_CELL p_ev_cell;
        S32 res;

        if(upp)
            res = ev_match_slr(&stkentp->slr, upp);
        else
            res = DEP_UPDATE;

        if(res != DEP_DELETE && ev_travel(&p_ev_cell, &stkentp->slr) > 0)
            p_ev_cell->parms.circ = 0;
        break;
        }

    /* free indirected lookup block */
    case LOOKUP_HAPPENING:
        lookup_block_dispose(&stkentp->data.stack_lookup);
        break;

    /* free indirected stats block */
    case DBASE_FUNCTION:
        al_ptr_dispose(P_P_ANY_PEDANTIC(&stkentp->data.stack_dbase.p_stat_block));
        break;

    /* free stored conditional rpn string */
    case DBASE_CALC:
        if(stkentp->data.stack_in_calc.ptr)
            al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&stkentp->data.stack_in_calc.ptr));
        break;

    /* kill off an alert or input box */
    case ALERT_INPUT:
        switch(stkentp->data.stack_alert_input.alert_input)
        {
        case RPN_FNV_ALERT:
            ev_alert_close();
            break;
        case RPN_FNV_INPUT:
            ev_input_close();
            break;
        }
        break;
    }
}

/******************************************************************************
*
* check and ensure that n spaces are available on the stack
*
******************************************************************************/

static S32
stack_grow(
    S32 n_spaces)
{
    if(stack_ptr + n_spaces >= stack_end)
    {
        STATUS status;
        P_ANY newp;
        S32 stack_ptr_offset, stack_new_size;
        P_STACK_ENTRY old_stack_base;

        old_stack_base   = stack_base;
        stack_ptr_offset = stack_offset(stack_ptr);
        stack_new_size   = stack_offset(stack_end) + STACK_INC;

        if(NULL == (newp = al_ptr_realloc_elem(STACK_ENTRY, stack_base, stack_new_size, &status)))
            return(status);

        #if TRACE_ALLOWED
        if(newp != stack_base)
            trace_0(TRACE_MODULE_EVAL | TRACE_OUT, "!!!!!!!!!!!!!!!!!!!! stack moved !!!!!!!!!!!!!!!!!!");
        #endif

        stack_base = newp;
        stack_end  = stack_base + stack_new_size;
        stack_ptr  = stack_base + stack_ptr_offset;

        /* use base stack entry to determine base document
         * for this recalc fragment
         */
        if(old_stack_base)
        {
            P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno_must(stack_base->slr.docno);
            p_ss_doc->stack_max = MAX(p_ss_doc->stack_max, stack_new_size);
        }
        else
            /* make sure new stack contains nowt */
            stack_base->type =_ERROR;

        trace_2(TRACE_MODULE_EVAL,
                "stack realloced, now: %d entries, %d bytes",
                stack_new_size,
                sizeof(STACK_ENTRY) * (stack_new_size));
    }

    return(0);
}

/******************************************************************************
*
* look at the documents maximum stack
* usage to see if we can free some stack
*
******************************************************************************/

extern S32
stack_release_check(void)
{
    EV_DOCNO docno;
    S32 stack_max = STACK_INC;

    /* check if we can free some stack */
    for(docno = DOCNO_FIRST; docno < docu_array_size; ++docno)
    {
        P_SS_DOC p_ss_doc;

        if(docno_void_entry(docno))
            continue;

        p_ss_doc = ev_p_ss_doc_from_docno_must(docno);

        stack_max = MAX(p_ss_doc->stack_max, stack_max);
    }

    if(stack_max < stack_end - stack_base)
    {
        STATUS status;
        P_ANY newp;
        S32 stack_ptr_offset;

        stack_ptr_offset = stack_offset(stack_ptr);

        if(NULL == (newp = al_ptr_realloc_elem(STACK_ENTRY, stack_base, stack_max, &status)))
            return(status);

        #if TRACE_ALLOWED
        if(newp != stack_base)
            trace_0(TRACE_MODULE_EVAL | TRACE_OUT, "!!!!!!!!!!!!!!!!!!!! stack moved !!!!!!!!!!!!!!!!!!");
        #endif

        stack_base = newp;
        stack_end  = stack_base + stack_max;
        stack_ptr  = stack_base + stack_ptr_offset;

        trace_2(TRACE_MODULE_EVAL,
                "stack freed, now: %d entries, %d bytes",
                stack_max,
                sizeof32(STACK_ENTRY) * (stack_max));
    }

    return(0);
}

/******************************************************************************
*
* set stack to given level
*
******************************************************************************/

static S32
stack_set(
    S32 stack_level)
{
    while(stack_offset(stack_ptr) > stack_level)
    {
        stack_free_resources(stack_ptr, NULL);
        --stack_ptr;
    }

    return(0);
}

/******************************************************************************
*
* blow away the stack completely
*
******************************************************************************/

extern void
stack_zap(
    _InRef_     PC_UREF_PARM upp)
{
    /* MC after PD 4.12 23mar92 - was testing stack_base against stack_ptr but this left
     * current state/resources leading to circular ref errors. but don't free NULL stacks!
    */
    if(/*stack_ptr !=*/ stack_base)
    {
        while(stack_offset(stack_ptr) >= 0)
            stack_free_resources(stack_ptr--, upp);

        stack_ptr = stack_base;
        stack_ptr->type =_ERROR;
    }

    tree_flags |= TRF_BLOWN;
}

/******************************************************************************
*
* follow a supporting range to see if any of its bits need recalcing
*
******************************************************************************/

static S32
visit_supporting_range(void)
{
    if( (ev_slr_row(&stack_ptr->slr) >= ev_slr_row(&stack_ptr->data.stack_visit_range.range.e)) ||
        (ev_slr_row(&stack_ptr->slr) >= ev_numrow(ev_slr_docno(&stack_ptr->slr))) )
    {
        stack_ptr->slr.row  = ev_slr_row(&stack_ptr->data.stack_visit_range.range.s);
        stack_ptr->slr.col += 1;

        /* hit the end of the range ? */
        if( (ev_slr_col(&stack_ptr->slr) >= ev_slr_col(&stack_ptr->data.stack_visit_range.range.e)) ||
            (ev_slr_col(&stack_ptr->slr) >= ev_numcol(ev_slr_docno(&stack_ptr->slr))) )
        {
            EV_TRENT rix;
            EV_FLAGS changed;

            /* update range serial number */
            if((rix = search_for_rng_ref(&stack_ptr->data.stack_visit_range.range)) >= 0)
            {
                const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno_must(ev_slr_docno(&stack_ptr->data.stack_visit_range.range.s));
                const P_RANGE_USE p_range_use = tree_rngptr(p_ss_doc, rix);
                PTR_ASSERT(p_range_use);
                p_range_use->visited = ev_serial_num;
            }

            changed = stack_ptr->stack_flags & (EV_FLAGS) STF_CALCEDSUPPORTER;

            /* restore previous state */
            --stack_ptr;

            /* pass changed back to parent */
            stack_ptr->stack_flags |= changed;

            return(NEW_STATE);
        }
    }

    stack_ptr->slr.row += 1;

    /* we know we must have enough stack (!),
     * caller to check_supporting_range chekced for us
     */
    stack_inc(VISIT_SLOT, stack_ptr[-1].slr, 0);
    stack_ptr->slr.row -= 1;
    grub_init(&stack_ptr->data.stack_visit_slot.grubb, &stack_ptr->slr);
    return(NEW_STATE);
}

/* end of ev_eval.c */
