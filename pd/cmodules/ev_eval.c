/* ev_eval.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

static S32
custom_result(
    P_EV_DATA p_ev_data,
    S32 error);

static S32
process_control(
    S32 action,
    P_EV_DATA args[],
    S32 nargs,
    S32 eval_stack_base);

static S32
process_control_for_cond(
    stack_sclp sclp,
    S32 step);

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

static S32
read_check_custom_def(
    PC_U8 ip_pos,
    EV_NAMEID *custom_num,
    P_P_EV_CUSTOM p_p_ev_custom);

static S32
slot_error_complete(
    S32 errno);

static S32
slot_set_error(
    _InRef_     PC_EV_SLR slrp,
    S32 errno);

static stack_entryp
stack_back_search(
    stack_entryp stkentp,
    S32 entry_type);

static stack_entryp
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
* 1=next element up etc
*
******************************************************************************/

#define stack_index_ptr_data(base_offset, offset) (&stack_base[(base_offset)-(offset)].data.sdi.data)

#define stack_inc(in_t, in_s, in_f) ((++stack_ptr)->type  = (in_t), \
                                        stack_ptr ->slr   = (in_s), \
                                        stack_ptr ->flags = (in_f))

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

static stack_entryp stack_base = NULL;
static stack_entryp stack_end  = NULL;
static stack_entryp stack_ptr  = NULL;

#include <signal.h>

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
                   : resultp->arg.error.num);
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
    EV_NAMEID name_num;
    S32 res;

    res = SAME_STATE;

    /* look up name definition */
    if((name_num = name_def_find(grubp->data.arg.nameid)) >= 0)
        {
        P_EV_NAME p_ev_name;
        S32 do_visit = 0;

        if((p_ev_name = name_ptr(name_num)) != NULL)
            {
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

                stack_ptr->data.svn.nameid = grubp->data.arg.nameid;

                switch(p_ev_name->def_data.did_num)
                    {
                    case RPN_DAT_SLR:
                        /* go check out the slot */
                        stack_inc(VISIT_SLOT, p_ev_name->def_data.arg.slr, 0);
                        grub_init(&stack_ptr->data.svs.grubb, &stack_ptr->slr);
                        break;

                    case RPN_DAT_RANGE:
                        /* prime visit_supporting_range
                         * to check out supporters
                         */
                        stack_inc(VISIT_SUPPORTRNG, p_ev_name->def_data.arg.range.s, 0);
                        stack_ptr->data.svr.range = p_ev_name->def_data.arg.range;
                        break;
                    }

                eval_trace("<check_supporting_name>");

                res = NEW_STATE;
                }
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
        if(tree_rngptr(ev_p_ss_doc_from_docno_must(grubp->data.arg.range.s.docno), rix)->visited >= ev_serial_num)
            {
            trace_0(TRACE_MODULE_EVAL, "*** range visit avoided since serial number up-to-date ***");
            return(SAME_STATE);
            }
        }

    /* stack the current state */
    stack_inc(VISIT_SUPPORTRNG, grubp->data.arg.range.s, 0);

    /* prime visit_supporting_range to check out supporters */
    stack_ptr->data.svr.range = grubp->data.arg.range;

    return(NEW_STATE);
}

/******************************************************************************
*
* check for circular reference
*
******************************************************************************/

static S32
circ_check(
    P_EV_SLOT p_ev_slot)
{
    S32 res;

    res = SAME_STATE;

    if(p_ev_slot->parms.circ)
        {
        stack_entryp stkentp;
        S32 found_it;

        eval_trace("<circ_check> found EVS_CIRC");

        /* now we must search back up the stack to find the
         * first visit to the slot and stop this recalcing the
         * slot and thus removing our stored error
        */
        stkentp = stack_ptr - 1;
        found_it = 0;

        while(stkentp >= stack_base)
            {
            if(stkentp->type == VISIT_SLOT)
                {
                slot_set_error(&stkentp->slr, create_error(EVAL_ERR_CIRC));
                stkentp->flags |= STF_CALCEDERROR | STF_CALCEDSUPPORTER;

#if TRACE_ALLOWED
                if(tracing(TRACE_MODULE_EVAL))
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

        res = slot_error_complete(create_error(EVAL_ERR_CIRC));
        }
    else
        p_ev_slot->parms.circ = 1;

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

        stack_ptr->data.sic.type = INCALC_PTR;
        stack_ptr->data.sic.ptr = rpn_in;
        stack_ptr->data.sic.offset = 0;
        stack_ptr->data.sic.slr = *slrp;
        stack_ptr->data.sic.p_ev_slot = NULL;

        if((res = eval_rpn(stack_offset(stack_ptr))) == SAME_STATE)
            res = 0;
        else
            res = create_error(EVAL_ERR_BADEXPR);

        *resultp = stack_ptr->data.sic.result_data;

        stack_set(stack_offset);
        }

    return(res);
}

/******************************************************************************
*
* backtrack up the stack to find the
* most recent slot we were calculating,
* set the error into the slot and go
* to calculate its dependents
*
******************************************************************************/

static S32
eval_backtrack_error(
    S32 error,
    S32 stack_at,
    void (*oldfpe)(int))
{
    stack_set(stack_at);
    ev_data_set_error(&stack_base[stack_at].data.sic.result_data, error);
    signal(SIGFPE, oldfpe);

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
    /* reset signal trapping */
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
    rpnstatep rpnp)
{
    S32 did_opt;

    did_opt = 0;
    switch(rpnp->num)
        {
        case RPN_BOP_PLUS:
            if((did_opt = two_nums_plus(&stack_ptr[-1].data.sdi.data,
                                        &stack_ptr[-1].data.sdi.data,
                                        &stack_ptr[ 0].data.sdi.data)) != 0)
                --stack_ptr;
            break;

        case RPN_BOP_MINUS:
            if((did_opt = two_nums_minus(&stack_ptr[-1].data.sdi.data,
                                         &stack_ptr[-1].data.sdi.data,
                                         &stack_ptr[ 0].data.sdi.data)) != 0)
                --stack_ptr;
            break;

        case RPN_BOP_DIVIDE:
            if((did_opt = two_nums_divide(&stack_ptr[-1].data.sdi.data,
                                          &stack_ptr[-1].data.sdi.data,
                                          &stack_ptr[ 0].data.sdi.data)) != 0)
                --stack_ptr;
            break;

        case RPN_BOP_TIMES:
            if((did_opt = two_nums_times(&stack_ptr[-1].data.sdi.data,
                                         &stack_ptr[-1].data.sdi.data,
                                         &stack_ptr[ 0].data.sdi.data)) != 0)
                --stack_ptr;
            break;
        }

    return(did_opt);
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
    struct rpnstate rpnb;
    int jmpval;
    void (*oldfpe)(int);
    PC_U8 rpn_start;
    stack_sicp sicp;
    EV_SLR cur_slr;

    oldfpe = signal(SIGFPE, eval_jmp);

    /* catch FP errors etc */
    if((jmpval = setjmp(safepoint)) != 0)
        return(eval_backtrack_error(jmpval, stack_at, oldfpe));

    /* set up address of rpn string */
    switch((sicp = &stack_base[stack_at].data.sic)->type)
        {
        case INCALC_PTR:
            rpn_start = sicp->ptr;
            break;
        case INCALC_SLR:
        default:
            {
            if(!sicp->p_ev_slot)
                ev_travel(&sicp->p_ev_slot, &sicp->slr);

            if(sicp->p_ev_slot->parms.type == EVS_CON_RPN)
                rpn_start = sicp->p_ev_slot->rpn.con.rpn_str;
            else
                rpn_start = sicp->p_ev_slot->rpn.var.rpn_str;
            break;
            }
        }

    rpnb.pos = rpn_start + sicp->offset;
    rpn_check(&rpnb);
    cur_slr = sicp->slr;

    /* we can't access sicp after this point: the stack may move!
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

                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                read_cur_sym(&rpnb, &stack_ptr->data.sdi.data);

                switch(stack_ptr->data.sdi.data.did_num)
                    {
                    #ifdef SLOTS_MOVE
                    /* we can't push DAT_STRINGs because they point into the rpn
                     * which may move with a slot during an interruption; if slots
                     * stop moving about, then we needn't do this duplication
                     */
                    case RPN_DAT_STRING:
                        {
                        EV_DATA temp;

                        ss_data_resource_copy(&temp, &stack_ptr->data.sdi.data);
                        stack_ptr->data.sdi.data = temp;
                        break;
                        }
                    #endif

                    /* check for duff slrs and ranges */
                    case RPN_DAT_SLR:
                        check_flags = stack_ptr->data.sdi.data.arg.slr.flags;
                        check_docno = stack_ptr->data.sdi.data.arg.slr.docno;
                        break;

                    case RPN_DAT_RANGE:
                        check_flags = stack_ptr->data.sdi.data.arg.range.s.flags |
                                      stack_ptr->data.sdi.data.arg.range.e.flags;
                        check_docno = stack_ptr->data.sdi.data.arg.range.s.docno;
                        break;
                    }

                if(check_flags & SLR_BAD_REF)
                    ev_data_set_error(&stack_ptr->data.sdi.data, create_error(EVAL_ERR_SLR_BAD_REF));
                else if((check_flags & SLR_EXT_REF) && ev_doc_error(check_docno))
                    ev_data_set_error(&stack_ptr->data.sdi.data, ev_doc_error(check_docno));

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
                        stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                        stack_ptr->data.sdi.data.did_num      = RPN_FRM_COND;
                        stack_ptr->data.sdi.data.arg.cond_pos = rpnb.pos  -
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
                        stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                        stack_ptr->data.sdi.data.did_num = RPN_DAT_BLANK;

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
                stack_entryp stkentp;

                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);

                /* default to local argument undefined */
                ev_data_set_error(&stack_ptr->data.sdi.data, create_error(EVAL_ERR_LOCALUNDEF));

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
                    if((p_ev_custom = custom_ptr(stkentp->data.stack_executing_custom.custom_num)) != NULL)
                        {
                        S32 i;

                        for(i = 0; i < p_ev_custom->args.n; ++i)
                            if(0 == _stricmp(p_ev_custom->args.id[i], arg_name))
                                {
                                arg_ix = i;
                                break;
                                }
                        }

                    if(arg_ix >= 0 && arg_ix < stkentp->data.stack_executing_custom.nargs)
                        {
                        /* get pointer to data on stack */
                        p_ev_data = stack_index_ptr_data(stkentp->data.stack_executing_custom.stack_base,
                                                         stkentp->data.stack_executing_custom.nargs - (S32) arg_ix - 1);

                        /* if it's an expanded array, we must
                         * index the relevant array element
                        */
                        if(stkentp->data.stack_executing_custom.in_array &&
                           data_is_array_range(p_ev_data))
                            {
                            EV_DATA temp;

                            array_range_index(&temp,
                                              p_ev_data,
                                              stkentp->data.stack_executing_custom.xpos,
                                              stkentp->data.stack_executing_custom.ypos,
                                              p_ev_custom->args.types[arg_ix]);

                            ss_data_resource_copy(&stack_ptr->data.sdi.data, &temp);
                            }
                        else
                            ss_data_resource_copy(&stack_ptr->data.sdi.data, p_ev_data);
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
                S32 res, arg_count;
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
                        stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].flags);
                        stack_base[stack_at].data.sic.offset = rpnb.pos - rpn_start;
                        stack_ptr->data.sie.stack_offset     = stack_at;

                        /* create custom result area on stack */
                        stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                        stack_ptr->data.sdi.data = custom_result;

                        /* switch to executing the custom */
                        stack_inc(EXECUTING_CUSTOM, cur_slr, stack_ptr[-1].flags);

                        stack_ptr->data.stack_executing_custom.nargs = arg_count;
                        stack_ptr->data.stack_executing_custom.stack_base = stack_offset(stack_ptr - 3);
                        stack_ptr->data.stack_executing_custom.in_array = (max_x || max_y);
                        stack_ptr->data.stack_executing_custom.xpos =
                        stack_ptr->data.stack_executing_custom.ypos = 0;
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

                    stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                    ev_data_set_error(&stack_ptr->data.sdi.data, res);
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
                    if((arg_count = func_data->nargs) < 0)
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
                        /* save calculating slot state */
                        exec_proctp exec;

                        exec = func_data->exec;

                        /* save resume position */
                        rpn_skip(&rpnb);
                        stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].flags);
                        stack_base[stack_at].data.sic.offset = rpnb.pos - rpn_start;
                        stack_ptr->data.sie.stack_offset = stack_at;

                        /* push result array */
                        stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                        stack_ptr->data.sdi.data = func_result;

                        /* swap to processing array state */
                        stack_inc(PROCESSING_ARRAY, cur_slr, stack_ptr[-1].flags);
                        stack_ptr->data.spa.xpos =
                        stack_ptr->data.spa.ypos = 0;
                        stack_ptr->data.spa.nargs = arg_count;
                        stack_ptr->data.spa.stack_base = stack_offset(stack_ptr - 3);
                        stack_ptr->data.spa.exec = exec;
                        stack_ptr->data.spa.type_count = type_count;
                        stack_ptr->data.spa.arg_types = arg_types;
                        res = NEW_STATE;
                        }
                    /* no array processing to do */
                    else if(!res)
                        {
                        switch(func_data->parms.ex_type)
                            {
                            case EXEC_EXEC:
                                (*func_data->exec)(args,
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
                                    struct stack_dbase_function sdb;
                                    if(NULL != (sdb.stbp = al_ptr_alloc_elem(stat_block, 1, &res)))
                                        {
                                        /* initialise database function data block */
                                        sdb.dbase_slot = stack_ptr->slr;
                                        sdb.dbase_rng = args[0]->arg.range;
                                        sdb.cond_pos = (S32) args[1]->arg.cond_pos;
                                        sdb.offset.col = 0;
                                        sdb.offset.row = 0;
                                        stat_block_init(sdb.stbp,
                                                        (EV_IDNO) func_data->parms.no_exec,
                                                        0, 0);

                                        stack_set(stack_offset(stack_ptr) - arg_count);

                                        /* save current state */
                                        rpn_skip(&rpnb);
                                        stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].flags);
                                        stack_base[stack_at].data.sic.offset = rpnb.pos - rpn_start;
                                        stack_ptr->data.sie.stack_offset = stack_at;

                                        /* switch to dbase state */
                                        stack_inc(DBASE_FUNCTION, cur_slr, stack_ptr[-1].flags);
                                        stack_ptr->data.sdb = sdb;
                                        res = NEW_STATE;
                                        }
                                    }
                                else
                                    res = create_error(EVAL_ERR_ARGTYPE);

                                break;
                                }

                            case EXEC_CONTROL:
                                if(stack_ptr->flags & STF_INCUSTOM)
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
                                struct stack_lookup slk;
                                S32 match;

                                if(NULL != (slk.lkbp = al_ptr_alloc_elem(look_block, 1, &res)))
                                    {
                                    ss_data_resource_copy(&slk.arg1, args[1]);
                                    ss_data_resource_copy(&slk.arg2, args[2]);

                                    if(func_data->parms.no_exec == LOOKUP_MATCH)
                                        match = (S32) args[2]->arg.integer;
                                    else
                                        match = 0;

                                    lookup_block_init(slk.lkbp,
                                                      args[0],
                                                      func_data->parms.no_exec,
                                                      0,
                                                      match);

                                    /* blow away lookup arguments */
                                    stack_set(stack_offset(stack_ptr) - arg_count);

                                    /* save current state */
                                    rpn_skip(&rpnb);
                                    stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].flags);
                                    stack_base[stack_at].data.sic.offset = rpnb.pos - rpn_start;
                                    stack_ptr->data.sie.stack_offset = stack_at;

                                    /* switch to lookup state */
                                    stack_inc(LOOKUP_HAPPENING, cur_slr, stack_ptr[-1].flags);
                                    stack_ptr->data.slk = slk;
                                    res = NEW_STATE;
                                    }

                                break;
                                }

                            case EXEC_ALERT:
                                {
                                struct stack_alert_input sai;

                                sai.alert_input = rpnb.num;
                                switch(rpnb.num)
                                    {
                                    case RPN_FNV_ALERT:
                                        res = ev_alert(stack_ptr->slr.docno,
                                                       args[0]->arg.string.data,
                                                       args[1]->arg.string.data,
                                                       arg_count > 2 ? args[2]->arg.string.data : NULL);
                                        break;

                                    case RPN_FNV_INPUT:
                                        /* save away name to receive result */
                                        strcpy(sai.name_id, args[1]->arg.string.data);
                                        res = ev_input(stack_ptr->slr.docno,
                                                       args[0]->arg.string.data,
                                                       args[2]->arg.string.data,
                                                       arg_count > 3 ? args[3]->arg.string.data : NULL);
                                        break;
                                    }

                                if(res >= 0)
                                    {
                                    /* blow away arguments */
                                    stack_set(stack_offset(stack_ptr) - arg_count);

                                    /* save current state */
                                    rpn_skip(&rpnb);
                                    stack_inc(IN_EVAL, cur_slr, stack_ptr[-1].flags);
                                    stack_base[stack_at].data.sic.offset = rpnb.pos - rpn_start;
                                    stack_ptr->data.sie.stack_offset = stack_at;

                                    /* switch to lookup state */
                                    stack_inc(ALERT_INPUT, cur_slr, stack_ptr[-1].flags);
                                    stack_ptr->data.sai = sai;
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
                    stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                    stack_ptr->data.sdi.data = func_result;
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
                stack_inc(DATA_ITEM, cur_slr, stack_ptr[-1].flags);
                stack_ptr->data.sdi.data = array_data;

                break;
                }
            }
        rpn_skip(&rpnb);
        }

    if(stack_ptr->type != DATA_ITEM)
        return(eval_backtrack_error(create_error(EVAL_ERR_INTERNAL), stack_at, oldfpe));
    else
        stack_base[stack_at].data.sic.result_data = stack_ptr->data.sdi.data;

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
    stack_lkp slkp)
{
    ss_data_free_resources(&slkp->arg1);
    ss_data_free_resources(&slkp->arg2);
    ss_data_free_resources(&slkp->lkbp->target_data);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&slkp->lkbp));
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
    stack_entryp stkentp;

    if((stkentp = stack_back_search(stack_ptr, EXECUTING_CUSTOM)) == NULL)
        stack_ptr->type =_ERROR;
    else
        {
        P_EV_CUSTOM p_ev_custom;
        S32 res;

        res = 0;

        if((p_ev_custom = custom_ptr(stkentp->data.stack_executing_custom.custom_num)) != NULL)
            {
            EV_ROW first_row, last_row;
            EV_SLR slr;

            last_row  = ev_numrow(slrp);
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
            custom_result(NULL, res);
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

static S32
custom_result(
    P_EV_DATA p_ev_data,
    S32 error)
{
    EV_DATA err_res;
    P_EV_DATA resultp, finalp;
    stack_entryp custom_stackp;

    /* if we were passed an error... */
    if(!p_ev_data)
        {
        ev_data_set_error(&err_res, error);
        p_ev_data = &err_res;
        }

    /* find the most recent custom */
    if((custom_stackp = stack_back_search(stack_ptr, EXECUTING_CUSTOM)) == NULL)
        stack_ptr->type =_ERROR;
    else
        {
        /* point to custom result area on stack */
        resultp = stack_index_ptr_data(custom_stackp->data.stack_executing_custom.stack_base, -2);

        if(custom_stackp->data.stack_executing_custom.in_array)
            {
            P_EV_DATA array_elementp;
            EV_DATA temp_data;

            temp_data.arg.array = resultp->arg.array; /* temp loan to get at array */

            array_elementp =
                ss_array_element_index_wr(&temp_data,
                                          custom_stackp->data.stack_executing_custom.xpos,
                                          custom_stackp->data.stack_executing_custom.ypos);

            if(data_is_array_range(p_ev_data))
                ev_data_set_error(array_elementp, create_error(EVAL_ERR_NESTEDARRAY));
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
            if(finalp->arg.error.type != ERROR_CUSTOM)
                { /* if possible, mark error's origin as given row in a custom function */
                finalp->arg.error.type = ERROR_CUSTOM;
                finalp->arg.error.docno = stack_ptr->slr.docno;
                finalp->arg.error.col = stack_ptr->slr.col;
                finalp->arg.error.row = stack_ptr->slr.row;
                }
            }

        /* reset stack to executing custom */
        stack_set(stack_offset(custom_stackp));

        /* switch to custom_complete */
        stack_ptr->type = CUSTOM_COMPLETE;
        }

    return(NEW_STATE);
}

/******************************************************************************
*
* do the sequential steps invloved
* in custom function execution
*
******************************************************************************/

static S32
custom_sequence(
    stack_entryp semp)
{
    EV_SLR next_slot, last_slot;
    S32 res;

    next_slot = last_slot = semp->data.stack_executing_custom.next_slot;

    /* get pointer to result */
    res = 0;

    ++semp->data.stack_executing_custom.next_slot.row;
    last_slot.row = ev_numrow(&last_slot);

    /* have we gone off end of file ? */
    if(next_slot.row >= last_slot.row)
        res = create_error(EVAL_ERR_NORETURN);
    /* switch to calculate next slot in custom */
    else if(stack_check_n(1) < 0)
        res = status_nomem();
    else
        {
        stack_inc(CALC_SLOT, next_slot, 0);
        stack_ptr->flags               |= STF_INCUSTOM;
        stack_ptr->data.sic.start_calc  = ev_serial_num;
        stack_ptr->data.sic.p_ev_slot   = NULL;
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
    P_EV_DATA args[], S32 nargs,
    S32 eval_stack_base)
{
    EV_SLR current_slot;

    /* save slot on which control statement encountered */
    current_slot = stack_ptr->slr;

    switch(action)
        {
        case CONTROL_GOTO:
            custom_jmp(&args[0]->arg.slr, 0, eval_stack_base - 1);
            break;

        case CONTROL_RESULT:
            custom_result(args[0], 0);
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
                stack_ptr->data.scl.control_type = CONTROL_WHILE;
                stack_ptr->data.scl.origin_slot = current_slot;
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
                                       create_error(EVAL_ERR_BADLOOPNEST));
                }
            break;

        /* found endwhile statement; pop last while
         * from stack and continue there
         */
        case CONTROL_ENDWHILE:
            {
            stack_entryp stkentp;

            if((stkentp = stack_back_search_loop(CONTROL_WHILE)) == NULL)
                custom_result(NULL, create_error(EVAL_ERR_BADLOOPNEST));
            else
                custom_jmp(&stkentp->data.scl.origin_slot, 0, stack_offset(stkentp - 1));

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
                                       create_error(EVAL_ERR_BADIFNEST));
                break;
                }
            }

        /* note fall thru */

        case CONTROL_IF:
            {
            stack_entryp stkentp;

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
                                          create_error(EVAL_ERR_BADIFNEST)) == EVS_CNT_ELSEIF)
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
            /* start repeat by resetting stack, then
             * pushing current position etc
             */
            stack_set(eval_stack_base);

            /* switch to control loop state */
            stack_ptr->type = CONTROL_LOOP;
            stack_ptr->data.scl.control_type = CONTROL_REPEAT;
            stack_ptr->data.scl.origin_slot = current_slot;
            break;

        case CONTROL_UNTIL:
            {
            stack_entryp stkentp;

            if((stkentp = stack_back_search_loop(CONTROL_REPEAT)) == NULL)
                custom_result(NULL, create_error(EVAL_ERR_BADLOOPNEST));
            else if(!args[0]->arg.integer)
                custom_jmp(&stkentp->data.scl.origin_slot, 1, eval_stack_base - 1);
            else
                custom_jmp(&current_slot, 1, stack_offset(stkentp - 1));
            break;
            }

        case CONTROL_FOR:
            {
            S32 res;
            struct stack_control_loop scl;

            scl.control_type = CONTROL_FOR;
            scl.origin_slot = current_slot;
            scl.end = args[2]->arg.fp;

            if(nargs > 3)
                scl.step = args[3]->arg.fp;
            else
                scl.step = 1.;

            if((res = name_make(&scl.nameid,
                                stack_ptr->slr.docno,
                                args[0]->arg.string.data,
                                args[1])) < 0)
                custom_result(NULL, res);
            else if(!process_control_for_cond(&scl, 0))
                process_control_search(EVS_CNT_FOR,
                                       EVS_CNT_NEXT,
                                       EVS_CNT_NONE,
                                       EVS_CNT_NONE,
                                       &current_slot,
                                       1,
                                       eval_stack_base - 1,
                                       create_error(EVAL_ERR_BADLOOPNEST));
            else
                {
                /* clear stack and switch to for loop */
                stack_set(eval_stack_base - 1);

                /* removing FOR args made room for this */
                stack_inc(CONTROL_LOOP, stack_ptr[-1].slr, stack_ptr[-1].flags);
                stack_ptr->data.scl = scl;
                }

            break;
            }

        case CONTROL_NEXT:
            {
            stack_entryp stkentp;

            if((stkentp = stack_back_search_loop(CONTROL_FOR)) == NULL)
                custom_result(NULL, create_error(EVAL_ERR_BADLOOPNEST));
            else if(process_control_for_cond(&stkentp->data.scl, 1))
                custom_jmp(&stkentp->data.scl.origin_slot, 1, eval_stack_base - 1);
            else
                custom_jmp(&current_slot, 1, stack_offset(stkentp - 1));

            break;
            }

        case CONTROL_BREAK:
            {
            S32 loop_count;
            stack_entryp stkentp;

            if(nargs)
                loop_count = (S32) args[0]->arg.integer;
            else
                loop_count = 1;

            loop_count = MAX(1, loop_count);

            stkentp = stack_ptr;
            while(loop_count--)
                if((stkentp = stack_back_search(stkentp, CONTROL_LOOP)) == NULL)
                    break;

            if(!stkentp)
                custom_result(NULL, create_error(EVAL_ERR_BADLOOPNEST));
            else
                {
                S32 block_start, block_end;
                EV_SLR slr;

                switch(stkentp->data.scl.control_type)
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

                slr = stkentp->data.scl.origin_slot;
                slr.row += 1;
                process_control_search(block_start,
                                       block_end,
                                       EVS_CNT_NONE,
                                       EVS_CNT_NONE,
                                       &slr,
                                       1,
                                       stack_offset(stkentp - 1),
                                       create_error(EVAL_ERR_BADLOOPNEST));
                }

            break;
            }

        case CONTROL_CONTINUE:
            {
            stack_entryp stkentp;

            if((stkentp = stack_back_search(stack_ptr, CONTROL_LOOP)) == NULL)
                custom_result(NULL, create_error(EVAL_ERR_BADLOOPNEST));
            else
                {
                S32 block_start, block_end;

                switch(stkentp->data.scl.control_type)
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
                                       create_error(EVAL_ERR_BADLOOPNEST));
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
    stack_sclp sclp,
    S32 step)
{
    EV_NAMEID name_num;
    S32 res;

    res = 0;
    if((name_num = name_def_find(sclp->nameid)) >= 0)
        {
        P_EV_NAME p_ev_name = name_ptr(name_num);

        if(p_ev_name->def_data.did_num == RPN_DAT_REAL)
            {
            if(step)
                p_ev_name->def_data.arg.fp += sclp->step;

            if(sclp->step >= 0)
                res = !(p_ev_name->def_data.arg.fp > sclp->end);
            else
                res = !(p_ev_name->def_data.arg.fp < sclp->end);
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
    last_row = ev_numrow(&slr);

    while(!found && ++slr.row < last_row)
        {
        P_EV_SLOT p_ev_slot;

        if(ev_travel(&p_ev_slot, &slr) > 0)
            {
            if(p_ev_slot->parms.control == (unsigned) block_start)
                ++nest;
            else if(p_ev_slot->parms.control == (unsigned) block_end)
                {
                if(nest)
                    --nest;
                else
                    {
                    found_type = p_ev_slot->parms.control;
                    found = 1;
                    }
                }
            else if(!nest)
                {
                if(p_ev_slot->parms.control == (unsigned) block_end_maybe1 ||
                   p_ev_slot->parms.control == (unsigned) block_end_maybe2)
                    {
                    found_type = p_ev_slot->parms.control;
                    found = 1;
                    }
                }
            }
        }

    if(!found)
        {
        custom_result(NULL, error);
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

static S32
read_check_custom_def(
    PC_U8 ip_pos,
    EV_NAMEID *custom_num,
    P_P_EV_CUSTOM p_p_ev_custom)
{
    EV_NAMEID custom_id;

    read_nameid(&custom_id, ip_pos);

    if((*custom_num = custom_def_find(custom_id)) < 0)
        return(create_error(EVAL_ERR_CUSTOMUNDEF));

    if((*p_p_ev_custom = custom_ptr(*custom_num)) != NULL)
        if(!((*p_p_ev_custom)->flags & TRF_UNDEFINED))
            {
            P_SS_DOC p_ss_doc;

            if((NULL == (p_ss_doc = ev_p_ss_doc_from_docno((*p_p_ev_custom)->owner.docno)))  ||  p_ss_doc->is_docu_thunk)
                return(create_error(EVAL_ERR_CANTEXTREF));

            return(0);
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
        /* get next slot from todo list */
        if(complete)
            {
            if(todo_get_slr(&stack_ptr->slr))
                {
                #if TRACE_ALLOWED
                eval_trace("*******************<ev_recalc>*********************");
                #endif

                stack_ptr->type  = VISIT_SLOT;
                stack_ptr->flags = 0;

                grub_init(&stack_ptr->data.svs.grubb, &stack_ptr->slr);
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
                P_EV_SLOT p_ev_slot;
                S32 travel_res, res;

                if((travel_res = ev_travel(&p_ev_slot, &stack_ptr->slr)) > 0)
                    {
                    S32 did_num;

                    if(!stack_ptr->data.svs.grubb.offset)
                        {
                        if(p_ev_slot->parms.type == EVS_VAR_RPN &&
                           p_ev_slot->rpn.var.visited >= ev_serial_num)
                            {
                            eval_trace("<visit_slot> ");
                            trace_2(TRACE_MODULE_EVAL,
                                    "avoided slot visit: visited: %d, serial_num: %d",
                                    p_ev_slot->rpn.var.visited,
                                    ev_serial_num);

                            /* remove this slot's entry from the todo list */
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

                        if(circ_check(p_ev_slot) == NEW_STATE)
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
                    while(!res && (did_num = grub_next(p_ev_slot, &stack_ptr->data.svs.grubb)) != RPN_FRM_END)
                        {
                        switch(did_num)
                            {
                            case RPN_DAT_SLR:
                                /* we must go and visit the slot;
                                 * stack the current state
                                 */
                                stack_inc(VISIT_SLOT, stack_ptr[-1].data.svs.grubb.data.arg.slr, 0);
                                grub_init(&stack_ptr->data.svs.grubb, &stack_ptr->slr);
                                res = 1;
                                break;

                            /* may need two stack entries */
                            case RPN_DAT_RANGE:
                                if(check_supporting_rng(&stack_ptr->data.svs.grubb) == NEW_STATE)
                                    res = 1;
                                break;

                            /* may need three stack entries */
                            case RPN_DAT_NAME:
                                if(check_supporting_name(&stack_ptr->data.svs.grubb) == NEW_STATE)
                                    res = 1;
                                break;
                            }
                        }

                    if(res)
                        break;
                    }

                /* all slot's supporters are calced - check
                if this slot now needs to be recalced */
                stack_ptr->type                = CALC_SLOT;
                stack_ptr->data.sic.start_calc = ev_serial_num;
                stack_ptr->data.sic.p_ev_slot  = p_ev_slot;
                stack_ptr->data.sic.travel_res = travel_res;

                trace_2(TRACE_MODULE_EVAL,
                        "VISIT_SLOT sic.start_calc set to: %d, serial_num: %d",
                        stack_ptr->data.sic.start_calc,
                        ev_serial_num);

                break;
                }

            case VISIT_SUPPORTRNG:
                visit_supporting_range();
                break;

            /* called to start recalc of a slot
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
                if(!stack_ptr->data.sic.p_ev_slot)
                    stack_ptr->data.sic.travel_res = ev_travel(&stack_ptr->data.sic.p_ev_slot, &stack_ptr->slr);

                if(stack_ptr->data.sic.travel_res > 0                                        &&
                   !(stack_ptr->flags & STF_CALCEDERROR)                                     &&
                   (doc_check_custom(stack_ptr->slr.docno)                                 ||
                    (stack_ptr->flags & STF_CALCEDSUPPORTER)                               ||
                    (stack_ptr->data.sic.p_ev_slot->parms.type == EVS_VAR_RPN &&
                     changed)                                                              ||
                    (stack_ptr->data.sic.p_ev_slot->parms.type == EVS_CON_RPN &&
                     changed)                                                              ||
                    (stack_ptr->data.sic.p_ev_slot->result.did_num == RPN_DAT_ERROR &&
                     stack_ptr->data.sic.p_ev_slot->result.arg.error.num == status_nomem())
                   )
                  )
                    {
                    stack_ptr->data.sic.did_calc = 1;
                    stack_ptr->data.sic.offset   = 0;
                    stack_ptr->data.sic.type     = INCALC_SLR;
                    stack_ptr->data.sic.slr      = stack_ptr->slr;

                    eval_rpn(stack_offset(stack_ptr));
                    }
                else
                    stack_ptr->data.sic.did_calc = 0;

                break;
                }

            /* called during recalc of a slot after eval_rpn
             * has released control for some reason
             */
            case IN_EVAL:
                {
                eval_trace("<IN_EVAL>");

                #ifdef SLOTS_MOVE
                /* clear out any cached slot pointer */
                stack_base[stack_ptr->data.sie.stack_offset].data.sic.p_ev_slot = NULL;
                #endif

                /* remove IN_EVAL state and continue with recalc */
                eval_rpn((stack_ptr--)->data.sie.stack_offset);
                break;
                }

            /* called when recalculation of a slot is over
             */
            case END_CALC:
                {
                S32 calc_parent;
                P_TODO_ENTRY todop;

                /* re-travel to slot if necessary */
                if(!stack_ptr->data.sic.p_ev_slot)
                    stack_ptr->data.sic.travel_res = ev_travel(&stack_ptr->data.sic.p_ev_slot, &stack_ptr->slr);

                /* did we actually get a result ? */
                if(stack_ptr->data.sic.travel_res > 0 &&
                   stack_ptr->data.sic.did_calc)
                    {
                    /* store result in slot */
                    ev_result_free_resources(&stack_ptr->data.sic.p_ev_slot->result);
                    ev_data_to_result_convert(&stack_ptr->data.sic.p_ev_slot->result,
                                              &stack_ptr->data.sic.result_data);

                    /* on error in custom function sheet, return custom function result error */
                    if(stack_ptr->data.sic.p_ev_slot->result.did_num == RPN_DAT_ERROR &&
                       doc_check_custom(stack_ptr->slr.docno))
                        {
                        EV_DATA data;

                        data.did_num = RPN_DAT_ERROR;
                        data.arg.error = stack_ptr->data.sic.p_ev_slot->result.arg.error;

                        if(data.arg.error.type != ERROR_CUSTOM)
                        {
                            data.arg.error.type = stack_ptr->data.sic.p_ev_slot->result.arg.error.type = ERROR_CUSTOM;
                        
                            data.arg.error.docno = stack_ptr->slr.docno;
                            data.arg.error.col = stack_ptr->slr.col;
                            data.arg.error.row = stack_ptr->slr.row;
                        }

                        custom_result(&data, 0);
                        break;
                        }
                    }

                if(!doc_check_custom(stack_ptr->slr.docno)               &&
                   (stack_ptr->data.sic.did_calc                       ||
                    stack_ptr->flags & STF_CALCEDERROR                 ||
                    (stack_ptr->data.sic.travel_res < 0 &&
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

                if(stack_ptr->data.sic.travel_res > 0)
                    {
                    /* clear circular flag and set visited */
                    if(stack_ptr->data.sic.p_ev_slot->parms.type == EVS_VAR_RPN)
                        {
                        stack_ptr->data.sic.p_ev_slot->rpn.var.visited = stack_ptr->data.sic.start_calc;
                        trace_2(TRACE_MODULE_EVAL,
                                "END_CALC, slot->visited set to: %d, serial_num: %d",
                                stack_ptr->data.sic.p_ev_slot->rpn.var.visited,
                                ev_serial_num);
                        }

                    stack_ptr->data.sic.p_ev_slot->parms.circ = 0;
                    }

                /* remove this slot's entry from the todo list */
                todo_remove_slr(&stack_ptr->slr);

                if((stack_ptr->flags & STF_CALCEDSUPPORTER) ||
                    stack_ptr->data.sic.did_calc)
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
                        stack_ptr->flags |= STF_CALCEDSUPPORTER;
                    }

                break;
                }

            case VISIT_COMPLETE_NAME:
                {
                EV_FLAGS changed;
                EV_NAMEID name_num;

                /* look up name definition */
                if((name_num = name_def_find(stack_ptr->data.svn.nameid)) >= 0)
                    {
                    P_EV_NAME p_ev_name;

                    if((p_ev_name = name_ptr(name_num)) != NULL)
                        p_ev_name->visited = ev_serial_num;
                    }

                changed = stack_ptr->flags & (EV_FLAGS) STF_CALCEDSUPPORTER;

                /* continue with what we were doing previously */
                --stack_ptr;

                /* update parent's changed state */
                stack_ptr->flags |= changed;

                break;
                }

            /* called to start a loop; the loop data
             * is pushed onto the stack; execution continues
             * at the slot after the origin slot
             */
            case CONTROL_LOOP:
                {
                stack_entryp stkentp;

                if((stkentp = stack_back_search(stack_ptr, EXECUTING_CUSTOM)) == NULL)
                    stack_ptr->type =_ERROR;
                else
                    custom_sequence(stkentp);
                break;
                }

            /* called for each slot in a database condition
             * eval_rpn checks that stack is available
             * needs 1 stack entry
             */
            case DBASE_FUNCTION:
                {
                P_EV_SLOT p_ev_slot;
                S32 error = 0;

                /* set up conditional string ready for munging */
                if(ev_travel(&p_ev_slot, &stack_ptr->data.sdb.dbase_slot) > 0)
                    {
                    char dbase_rpn[EV_MAX_OUT_LEN + 1];
                    S32 rpn_len;
                    P_U8 rpn_ptr;
                    EV_SLR slr;

                    /* calculate slr for slot being processed conditionally */
                    slr = stack_ptr->data.sdb.dbase_rng.s;

                    /* check for errors with external refs */
                    if((slr.flags & SLR_EXT_REF) && ev_doc_error(slr.docno))
                        error = ev_doc_error(slr.docno);
                    else
                        {
                        EV_SLR target_slr;
                        S32 abs_col, abs_row;

                        /* calculate slr of slot in database range */
                        slr.col += stack_ptr->data.sdb.offset.col;
                        slr.row += stack_ptr->data.sdb.offset.row;

                        target_slr.col = stack_ptr->data.sdb.offset.col;
                        target_slr.row = stack_ptr->data.sdb.offset.row;
                        abs_col = abs_row = 0;

                        if(stack_ptr->data.sdb.dbase_rng.s.col + 1 != stack_ptr->data.sdb.dbase_rng.e.col)
                            {
                            target_slr.col = slr.col;
                            abs_col = 1;
                            }

                        if(stack_ptr->data.sdb.dbase_rng.s.row + 1 != stack_ptr->data.sdb.dbase_rng.e.row)
                            {
                            target_slr.row = slr.row;
                            abs_row = 1;
                            }

                        rpn_len = ev_proc_conditional_rpn(dbase_rpn,
                                                          p_ev_slot->rpn.var.rpn_str + stack_ptr->data.sdb.cond_pos,
                                                          &target_slr, abs_col, abs_row);

                        if(NULL != (rpn_ptr = al_ptr_alloc_bytes(U8, rpn_len, &error)))
                            {
                            stack_inc(DBASE_CALC, slr, stack_ptr[-1].flags);
                            memcpy32(rpn_ptr, dbase_rpn, rpn_len);
                            stack_ptr->data.sic.ptr = rpn_ptr;
                            stack_ptr->data.sic.offset = 0;
                            stack_ptr->data.sic.type = INCALC_PTR;
                            stack_ptr->data.sic.slr = slr;

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
                    ev_data_set_error(&stack_ptr[-1].data.sdi.data, error);
                    }

                break;
                }

            /* receives result of database condition calculation */
            case DBASE_CALC:
                {
                EV_SLR span;

                /* throw away current conditional rpn string */
                al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&stack_ptr->data.sic.ptr));

                /* call the dbase routine to process the data */
                dbase_sub_function(&stack_ptr[-1].data.sdb, &stack_ptr->data.sic.result_data);

                /* remove DBASE_CALC state from stack */
                --stack_ptr;
                stack_ptr->data.sdb.offset.col += 1;

                span.col = stack_ptr->data.sdb.dbase_rng.e.col -
                           stack_ptr->data.sdb.dbase_rng.s.col;
                span.row = stack_ptr->data.sdb.dbase_rng.e.row -
                           stack_ptr->data.sdb.dbase_rng.s.row;

                if(stack_ptr->data.sdb.offset.col >= span.col)
                    {
                    stack_ptr->data.sdb.offset.col  = 0;
                    stack_ptr->data.sdb.offset.row += 1;

                    /* check if function finished yet */
                    if(stack_ptr->data.sdb.offset.row >= span.row)
                        {
                        EV_DATA data;

                        dbase_sub_function_finish(&data, &stack_ptr->data.sdb);
                        al_ptr_dispose(P_P_ANY_PEDANTIC(&stack_ptr->data.sdb.stbp));

                        /* pop previous state
                         * push dbase result
                         * this leaves copies of flags & slr in [-1]
                         */
                        stack_ptr[0] = stack_ptr[-1];

                        stack_ptr[-1].type = DATA_ITEM;
                        stack_ptr[-1].data.sdi.data = data;
                        }
                    }
                break;
                }

            case LOOKUP_HAPPENING:
                {
                S32 res;

                if((res = lookup_array_range_proc(stack_ptr->data.slk.lkbp,
                                                  &stack_ptr->data.slk.arg1)) >= 0)
                    {
                    EV_DATA data;

                    if(!res || !stack_ptr->data.slk.lkbp->had_one)
                        ev_data_set_error(&data, create_error(EVAL_ERR_LOOKUP));
                    else
                        lookup_finish(&data, &stack_ptr->data.slk);

                    lookup_block_dispose(&stack_ptr->data.slk);

                    /* pop previous state
                     * push lookup result
                     * this leaves copies of flags & slr in [-1]
                     */
                    stack_ptr[0] = stack_ptr[-1];

                    stack_ptr[-1].type = DATA_ITEM;
                    stack_ptr[-1].data.sdi.data = data;
                    }
                break;
                }

            /* called as result of RESULT function or
             * error during custom evaluation
            */
            case CUSTOM_COMPLETE:
                {
                S32 custom_over, nargs;
                struct _stack_entry result, state;

                custom_over = 0;
                nargs = stack_ptr->data.stack_executing_custom.nargs;

                if(stack_ptr->data.stack_executing_custom.in_array)
                    {
                    ++stack_ptr->data.stack_executing_custom.xpos;
                    if(stack_ptr->data.stack_executing_custom.xpos >= stack_ptr[-1].data.sdi.data.arg.array.x_size)
                        {
                        stack_ptr->data.stack_executing_custom.xpos  = 0;
                        stack_ptr->data.stack_executing_custom.ypos += 1;

                        if(stack_ptr->data.stack_executing_custom.ypos >= stack_ptr[-1].data.sdi.data.arg.array.y_size)
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
                stack_set(stack_offset(stack_ptr) - 1 - nargs);

                /* push custom result */
                stack_ptr += 1;
                stack_ptr[0] = result;

                /* set state */
                stack_ptr += 1;
                stack_ptr[0] = state;
                break;
                }

            /* whilst executing custom, select
             * next sequential slot for evaluation
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
                struct _stack_processing_array spa;
                PC_EV_TYPE typep;
                P_EV_DATA resp;

                if(stack_ptr->data.spa.xpos >= stack_ptr[-1].data.sdi.data.arg.array.x_size)
                    {
                    stack_ptr->data.spa.xpos  = 0;
                    stack_ptr->data.spa.ypos += 1;

                    /* have we completed array ? */
                    if(stack_ptr->data.spa.ypos >= stack_ptr[-1].data.sdi.data.arg.array.y_size)
                        {
                        S32 nargs = stack_ptr->data.spa.nargs; /* SKS fixed 01may95 in Fireworkz; 21mar12 in PipeDream! */
                        struct _stack_entry result, state;

                        --stack_ptr;
                        result = stack_ptr[0];

                        --stack_ptr;
                        state  = stack_ptr[0];

                        /* remove array arguments from stack */
                        stack_set(stack_offset(stack_ptr) - 1 - nargs);

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
                spa = stack_ptr->data.spa;

                stack_ptr->data.spa.xpos += 1;

                /* get the arguments and array pointers */
                for(ix = 0, datapp = args_in, typec = spa.type_count,
                    typep = spa.arg_types;
                    ix < spa.nargs;
                    ++ix, ++datapp)
                    {
                    *datapp = stack_index_ptr_data(spa.stack_base,
                                                   spa.nargs - (S32) ix - 1);

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
                            slr.col += spa.xpos;
                            slr.row += spa.ypos;

                            ev_slr_deref(&arg_dat[ix], &slr, FALSE);
                            args[ix] = &arg_dat[ix];
                            }
                        else
                            {
                            /* don't give a pointer into the array itself,
                             * otherwise someone will poo on it
                             */
                            arg_dat[ix] = *ss_array_element_index_borrow((*datapp), spa.xpos, spa.ypos);
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
                resp = ss_array_element_index_wr(&stack_ptr[-1].data.sdi.data, spa.xpos, spa.ypos);

                /* call semantic routine with array
                 * elements as arguments instead of arrays
                 */
                if((res = args_check(spa.nargs, args,
                                     spa.type_count, spa.arg_types,
                                     resp,
                                     &max_x, &max_y)) == 0)
                    (*spa.exec)(args, spa.nargs, resp, &stack_ptr->slr);
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

                switch(stack_ptr->data.sai.alert_input)
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
                                                         stack_ptr->data.sai.name_id,
                                                         &string_data)) < 0)
                                    {
                                    ss_data_free_resources(&string_data);
                                    ev_data_set_error(&ev_data, name_res);
                                    }
                                else
                                    {
                                    EV_NAMEID name_num;

                                    name_num = name_def_find(name_key);
                                    name_ptr(name_num)->def_data = string_data;

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
                    stack_ptr[-1].data.sdi.data = ev_data;
                    }

                break;
                }

            /* all other states are treated as an error condition */
            /*case_ERROR:*/
            default:
                stack_zap(NULL);
                slot_set_error(&stack_ptr->slr, create_error(EVAL_ERR_INTERNAL));
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
    /* clear cached slot pointer when leaving evaluator */
    switch(stack_ptr->type)
        {
        case CALC_SLOT:
        case END_CALC:
            stack_ptr->data.sic.p_ev_slot = NULL;
            break;
        }
    #endif

    ev_recalc_status(todotab.next);

    tree_flags &= ~TRF_LOCK;
}

/******************************************************************************
*
* when an error is encountered processing
* a slot, store the error and terminate
* the evaluation
*
******************************************************************************/

static S32
slot_error_complete(
    S32 errnum)
{
    slot_set_error(&stack_ptr->slr, errnum);

    stack_ptr->type                = CALC_SLOT;
    stack_ptr->data.sic.start_calc = ev_serial_num;
    stack_ptr->data.sic.p_ev_slot  = NULL;

    stack_ptr->flags |= STF_CALCEDERROR | STF_CALCEDSUPPORTER;

    return(NEW_STATE);
}

/******************************************************************************
*
* store an error in a slot and switch state
* to calculating the slot's dependents
*
******************************************************************************/

static S32
slot_set_error(
    _InRef_     PC_EV_SLR slrp,
    S32 errnum)
{
    P_EV_SLOT p_ev_slot;

    if(ev_travel(&p_ev_slot, slrp) > 0)
        {
        ev_result_free_resources(&p_ev_slot->result);

        p_ev_slot->result.did_num        = RPN_DAT_ERROR;
        p_ev_slot->result.arg.error.num  = errnum;
        p_ev_slot->result.arg.error.type = ERROR_NORMAL;
        }

    return(errno);
}

/******************************************************************************
*
* search back up stack for an
* entry of the given type
*
******************************************************************************/

static stack_entryp
stack_back_search(
    stack_entryp start_ptr,
    S32 entry_type)
{
    stack_entryp stkentp, res;

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
* search back up stack for a
* control entry of the given type
*
******************************************************************************/

static stack_entryp
stack_back_search_loop(
    S32 loop_type)
{
    stack_entryp stkentp;

    /* find the most recent loop */
    if((stkentp = stack_back_search(stack_ptr, CONTROL_LOOP)) == NULL)
        return(NULL);

    if(stkentp->data.scl.control_type != loop_type)
        return(NULL);

    return(stkentp);
}

/******************************************************************************
*
* free any resources owned by
* a data item in a stack entry
*
******************************************************************************/

static void
stack_free_resources(
    stack_entryp stkentp,
    _InRef_     PC_UREF_PARM upp)
{
    /* free resources allocated to data items on stack */
    switch(stkentp->type)
        {
        case DATA_ITEM:
            {
#if 1 /* SKS 21oct12 - simplify like Fireworkz */
            ss_data_free_resources(&stkentp->data.sdi.data);
#else
            switch(stkentp->data.sdi.data.did_num)
                {
                case RPN_TMP_STRING:
                    trace_1(TRACE_MODULE_EVAL,
                            "stack_free_resources freeing string: %s",
                            stkentp->data.sdi.data.arg.string.data);
                    str_clr(&stkentp->data.sdi.data.arg.string.data);
                    break;

                case RPN_TMP_ARRAY:
                    ss_array_free(&stkentp->data.sdi.data);
                    break;
                }
#endif
            break;
            }

        /* blow away the circ bit for incompletely
         * calculated and visited slots
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
            if(res != DEP_DELETE && ev_travel(&stkentp->data.sic.p_ev_slot, &stkentp->slr) > 0)
                stkentp->data.sic.p_ev_slot->parms.circ = 0;
            break;
            }

        case VISIT_SLOT:
            {
            P_EV_SLOT p_ev_slot;
            S32 res;

            if(upp)
                res = ev_match_slr(&stkentp->slr, upp);
            else
                res = DEP_UPDATE;

            if(res != DEP_DELETE && ev_travel(&p_ev_slot, &stkentp->slr) > 0)
                p_ev_slot->parms.circ = 0;
            break;
            }

        /* free indirected lookup block */
        case LOOKUP_HAPPENING:
            lookup_block_dispose(&stkentp->data.slk);
            break;

        /* free indirected stats block */
        case DBASE_FUNCTION:
            al_ptr_dispose(P_P_ANY_PEDANTIC(&stkentp->data.sdb.stbp));
            break;

        /* free stored conditional rpn string */
        case DBASE_CALC:
            if(stkentp->data.sic.ptr)
                al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&stkentp->data.sic.ptr));
            break;

        /* kill off an alert or input box */
        case ALERT_INPUT:
            switch(stkentp->data.sai.alert_input)
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
* check and ensure that n spaces are
* available on the stack
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
        stack_entryp old_stack_base;

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
                sizeof(struct _stack_entry) * (stack_new_size));
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
    /* MC after 4.12 23mar92 - was testing stack_base against stack_ptr but this left
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
* follow a supporting range to
* see if any of its bits need recalcing
*
******************************************************************************/

static S32
visit_supporting_range(void)
{
    if(stack_ptr->slr.row >= stack_ptr->data.svr.range.e.row ||
       stack_ptr->slr.row >= ev_numrow(&stack_ptr->slr))
        {
        stack_ptr->slr.row  = stack_ptr->data.svr.range.s.row;
        stack_ptr->slr.col += 1;

        /* hit the end of the range ? */
        if(stack_ptr->slr.col >= stack_ptr->data.svr.range.e.col ||
           stack_ptr->slr.col >= ev_numcol(&stack_ptr->slr))
            {
            EV_TRENT rix;
            EV_FLAGS changed;

            /* update range serial number */
            if((rix = search_for_rng_ref(&stack_ptr->data.svr.range)) >= 0)
                tree_rngptr(ev_p_ss_doc_from_docno_must(stack_ptr->data.svr.range.s.docno), rix)->visited = ev_serial_num;

            changed = stack_ptr->flags & (EV_FLAGS) STF_CALCEDSUPPORTER;

            /* restore previous state */
            --stack_ptr;

            /* pass changed back to parent */
            stack_ptr->flags |= changed;

            return(NEW_STATE);
            }
        }

    stack_ptr->slr.row += 1;

    /* we know we must have enough stack (!),
     * caller to check_supporting_range chekced for us
     */
    stack_inc(VISIT_SLOT, stack_ptr[-1].slr, 0);
    stack_ptr->slr.row -= 1;
    grub_init(&stack_ptr->data.svs.grubb, &stack_ptr->slr);
    return(NEW_STATE);
}

/* end of ev_eval.c */
