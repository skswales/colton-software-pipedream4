/* numbers.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that handles spreadsheet bits and draw file management */

/* RJM Sep 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#include "riscos_x.h"
#include "ridialog.h"

/*
internal functions
*/

static void
draw_adjust_file_ref(
    P_DRAW_DIAG p_draw_diag,
    P_DRAW_FILE_REF p_draw_file_ref);

static P_DRAW_FILE_REF
draw_search_refs(
    S32 extdep_key);

static P_DRAW_FILE_REF
draw_search_refs_slot(
    _InRef_     PC_EV_SLR p_slr);

PROC_UREF_PROTO(static, draw_uref);

PROC_UREF_PROTO(static, graph_uref);

static BOOL
mergebuf_core(
    BOOL allow_check,
    S32 send_replace);

static BOOL
slot_ref_here(
    PC_U8 ptr);

static S32
text_slot_add_dependency(
    _InVal_     COL col,
    _InVal_     ROW row);

/******************************************************************************
*
* Activity indicator
* Every second switch on or off inverse message
*
******************************************************************************/

extern void
actind(
    S32 number)
{
    static S32 last_number = -1;

    /* don't bother with 0% change */
    if(number == last_number)
        return;

    if(number < 0)
        {
        if(last_number >= 0)
            {
            /* switch off and reset statics */
            last_number = -1;
            #if defined(DO_VISDELAY_BY_STEAM)
            trace_0(TRACE_APP_PD4, "visdelay_end()");
            riscos_visdelay_off();
            #endif
            }

        return;
        }

    if( number > 99)
        number = 99;

    #if defined(DO_VISDELAY_BY_STEAM)
    if(last_number < 0)
        {
        trace_0(TRACE_APP_PD4, "visdelay_begin()");
        riscos_visdelay_on();
        }
    #endif

    last_number = number;

    /* is there anything to output after the heading */

    if(number > 0)
        {
        trace_1(TRACE_APP_PD4, "riscos_hourglass_percent(%d)", number);
        riscos_hourglass_percent(number);
        }
}

extern void
actind_end(void)
{
    actind(-1); /* sugar */
}

extern void
actind_in_block(
    S32 down_columns)
{
    actind(percent_in_block(down_columns)); /* sugar */
}

/******************************************************************************
*
*  this routine does the work for ExchangeNumbersText and Snapshot
*
******************************************************************************/

#define bash_snapshot 0
#define bash_exchange 1
#define bash_tonumber 2
#define bash_totext   3

static void
bash_slots_about_a_bit(
    int action)
{
    SLR oldpos;
    char oldformat;
    char dateformat = d_options_DF;
    char thousands  = d_options_TH;
    P_SLOT tslot;

    if(blkstart.col != NO_COL && !set_up_block(TRUE))
        return;

    /* must ensure recalced for snapshot */
    if(bash_snapshot == action)
        ev_recalc_all();

    oldpos.col = curcol;
    oldpos.row = currow;

    init_marked_block();

    /* keep name definitions for decompilations */
    ev_name_del_hold();

    uref_block(UREF_REPLACE);

    escape_enable();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL && !ctrlflag)
        {
        actind_in_block(DOWN_COLUMNS);

        if(is_protected_slot(tslot))
            continue;

        d_options_DF = 'E';
        d_options_TH = TH_BLANK;

        switch(tslot->type)
            {
            case SL_TEXT:
                if(bash_snapshot == action)
                    {
                    d_options_DF = dateformat;
                    d_options_TH = thousands;
                    }
                break;

            case SL_NUMBER:
                break;

            default:
            case SL_PAGE:
                continue;   /* enough to confuse SKS for a while! */
            }

        /* keep dataflower happy - moved from a bit below */
        oldformat = tslot->format;

        curcol = in_block.col;
        currow = in_block.row;

        /* decompile into buffer */
        switch(action)
            {
            case bash_tonumber:
                xf_inexpression = TRUE;
                prccon(linbuf, tslot);
                break;

            case bash_totext:
                prccon(linbuf, tslot);
                break;

            case bash_exchange:
                {
                if(tslot->type == SL_TEXT)
                    xf_inexpression = TRUE;

                prccon(linbuf, tslot);
                break;
                }

            default:
            case bash_snapshot:
                {
                P_EV_RESULT p_ev_result;

                switch(result_extract(tslot, &p_ev_result))
                    {
                    case SL_NUMBER:
                        /* leave strings as text slots */
                        if(p_ev_result->did_num != RPN_RES_STRING)
                            xf_inexpression = TRUE;

                        /* poke the minus/brackets flag to minus */
                        tslot->format = F_DCPSID | F_DCP;

                        /* array results are left as arrays - not decompiled into
                         * their formula (same difference for constant arrays, but
                         * not for calculated arrays)
                         */
                        if(p_ev_result->did_num == RPN_RES_ARRAY)
                            {
                            EV_DATA data;
                            const EV_DOCNO cur_docno = (EV_DOCNO) current_docno();
                            EV_OPTBLOCK optblock;

                            ev_set_options(&optblock, cur_docno);

                            ev_result_to_data_convert(&data, p_ev_result);

                            ev_decode_data(linbuf, cur_docno, &data, &optblock);
                            break;
                            }

                    /* note fall thru */
                    default:
                        (void) expand_slot(current_docno(), tslot, in_block.row, linbuf, LIN_BUFSIZ /*fwidth*/,
                                           DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, FALSE /*expand_ctrl*/,
                                           FALSE /*allow_fonty_result*/, TRUE /*cff*/);
                        break;
                    }
                break;
                }
            }

        slot_in_buffer = buffer_altered = TRUE;

        /* compile into slot */

        if(xf_inexpression)
            {
            seteex_nodraw();    /*>>>RCM says: OK dog breath, what do I do with this */
            merexp_reterr(NULL, NULL, FALSE);
            endeex_nodraw();

            if(bash_snapshot == action)
                {
                /* reset format in new slot */
                if((NULL != (tslot = travel(in_block.col, in_block.row)))  &&  (tslot->type != SL_TEXT))
                    tslot->format = oldformat;
                }

            /* SKS after 4.11 03feb92 - added escape clause */
            if(been_error)
                goto EXIT_POINT;
            }
        else if(!mergebuf_core(FALSE, FALSE))
            goto EXIT_POINT;
        }

    curcol = oldpos.col;
    currow = oldpos.row;
    movement = 0;

EXIT_POINT:;

    /* SKS after 4.11 03feb92 - moved from above label */
    d_options_DF = dateformat;
    d_options_TH = thousands;

    escape_disable();

    actind_end();

    out_screen = TRUE;

    ev_name_del_release();

    uref_block(UREF_CHANGE);
}

/******************************************************************************
*
* compile from linbuf into array converting text-at fields to slot references
* return length (including terminating NULL)
*
******************************************************************************/

extern S32
compile_text_slot(
    P_U8 array /*out*/,
    PC_U8 from,
    P_U8 slot_refsp /*out*/)
{
    P_U8 to = array;
    const uchar text_at_char = get_text_at_char();

    trace_0(TRACE_APP_PD4, "compile_text_slot");

    if(slot_refsp)
        *slot_refsp = 0;

    if(NULLCH == text_at_char)
        {
        while(NULLCH != *from)
            *to++ = *from++;
        }
    else
        {
        while(NULLCH != *from)
            {
            if((*to++ = *from++) != text_at_char)
                continue;

            /* if block of more than one text-at chars just write them and continue */
            if(*from == text_at_char)
                {
                while(*from == text_at_char)
                    *to++ = *from++;

                continue;
                }

            /* if it's a slot ref deal with it */
            if(slot_ref_here(from))
                {
                COL tcol;
                ROW trow;
                BOOL abscol = FALSE, absrow = FALSE;
                DOCNO docno;

                if(slot_refsp)
                    *slot_refsp = SL_TREFS;

                from += ev_doc_establish_doc_from_name(from, (EV_DOCNO) current_docno(), &docno);

                /* set up getcol and getsbd */
                buff_sofar = (P_U8) from;
                if(*buff_sofar == '$')
                    {
                    abscol = TRUE;
                    buff_sofar++;
                    }

                tcol = getcol();

                if(abscol)
                    tcol = (COL) (tcol | ABSCOLBIT);

                if(*buff_sofar == '$')
                    {
                    absrow = TRUE;
                    buff_sofar++;
                    }

                trow = (ROW) getsbd() - 1;

                if(absrow)
                    trow = (ROW) (trow | ABSROWBIT);

                from = buff_sofar;

                /* emit a compiled slot reference */
                *to++ = SLRLD1;

#if defined(SLRLD2) /* additional byte helps speed compiled_text_len()! */
                *to++ = SLRLD2;
#endif

                /*eportf("compile_text_slot: splat docno %d col 0x%x row 0x%x", docno, tcol, trow);*/
                to = splat_csr(to, docno, tcol, trow);

                /* for a text-at field, write out subsequent text-at chars so they
                 * don't get confused with anything following */
                while(*from == text_at_char)
                    *to++ = *from++;

                continue;
                }

            /* check for any other text-at field */
            if(strchr("DTPNLFGdtpnlfg", *from))
                {
                /* MRJC 30.3.92 */
                if(slot_refsp)
                    *slot_refsp = SL_TREFS;

                /* copy across the text-at field contents */
                while(*from && (*from != text_at_char))
                    *to++ = *from++;

                /* for a text-at field, write out subsequent text-at chars so they
                 * don't get confused with anything following */
                while(*from == text_at_char)
                    *to++ = *from++;

                continue;
                }

            /* this was an isolated text-at char - just loop and put following content in as normal */
            }
        }

    *to = NULLCH;

    return((to - array) + 1);
}

extern const uchar *
report_compiled_text_string(
    _In_z_      const uchar * cts_in)
{
    static uchar buffer[4096];

    const uchar * cts = cts_in;
    uchar * to = buffer;

    /* quick pass to see if we need to copy for rewriting */
    for(;;)
        {
        uchar c = *cts++;

        if(NULLCH == c)
            return(cts_in); /* nothing out-of-the-ordinary - just pass the source */

        if((c < 0x20) || (c == 0x7F))
            {
            --cts; /* retract for rewriting loop */
            break;
            }
        }

    memcpy32(to, cts_in, cts - cts_in); /* copy what we have scanned over so far */
    to += (cts - cts_in);

    for(;;)
        {
        uchar c = (*to++ = *cts++);

        if((to - buffer) >= elemof32(buffer))
            {
            to[-1] = NULLCH; /* ran out of space. NULLCH-terminate and exit */
            return(buffer);
            }

        if(NULLCH == c)
            return(buffer); /* buffer is NULLCH-terminated, exit */

        if((c < 0x20) || (c == 0x7F))
            {
            to[-1] = c | 0x80; /* overwrite with tbs C1 equivalent */
            if(SLRLD1 == c)
                cts += COMPILED_TEXT_SLR_SIZE-1;
            continue;
            }
        }
}

/******************************************************************************
*
* return the length of a compiled text (normally slot contents)
* NB. length is inclusive of NULLCH byte
*
******************************************************************************/

extern S32
compiled_text_len(
    PC_U8 str_in)
{
    PC_U8 str = str_in;

#if defined(SLRLD2) /* can optimise using fast Norcroft strlen() when SLRLD2==NULLCH is used after SLRLD1 */
    if(NULLCH == *str) /* verifying string is not empty saves a test per loop */
        return(1); /*NULLCH*/

    for(;;)
        {
        PC_U8 ptr = str + strlen(str);

        if(SLRLD1 == ptr[-1]) /* ptr is never == str */
            { /* the NULLCH we found is SLRLD2 - keep going */
            str = ptr + COMPILED_TEXT_SLR_SIZE-1; /* restart past the compiled SLR */
            /*eportf("ctl: SLRLD2 @ %d in %s", PtrDiffBytesS32(ptr, str_in), report_compiled_text_string(str_in));*/
            continue;
            }

        /* (NULLCH == *ptr) */
        /*eportf("ctl: NULLCH @ %d in %s", PtrDiffBytesS32(ptr, str_in), report_compiled_text_string(str_in));*/
        str = ptr + 1; /* want to leave str pointing past the real NULLCH */
        return(PtrDiffBytesS32(str, str_in));
        }
#else
    int c;

    while((c = *str++) != NULLCH) /* leave p pointing past NULLCH */
        if(SLRLD1 == c)
            str += COMPILED_TEXT_SLR_SIZE-1;

    return(PtrDiffBytesS32(str, str_in));
#endif
}

/******************************************************************************
*
* compile a slot; recognise a constant
*
******************************************************************************/

extern S32
compile_expression(
    P_U8 compiled_out,
    P_U8 text_in,
    S32 len_out,
    P_S32 at_pos,
    P_EV_RESULT p_ev_result,
    P_EV_PARMS parmsp)
{
    EV_OPTBLOCK optblock;
    EV_DOCNO docno;
    S32 rpn_len;
    EV_DATA data;

    trace_1(TRACE_MODULE_EVAL, "compile_expression(%s): in -", text_in);

    *at_pos = -1;       /* will be <0 unless ev_compile is called */

    docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, docno);

    /* get sensible defaults for result and parms */
    if(d_progvars[OR_AM].option != 'A')
        {
        p_ev_result->did_num = RPN_DAT_WORD8;
        p_ev_result->arg.integer = 0;
        }
    else
        p_ev_result->did_num = RPN_DAT_BLANK;

    parmsp->control   = 0;
    parmsp->circ      = 0;

    if(!ev_name_check(text_in, docno))
        rpn_len = create_error(EVAL_ERR_BADEXPR);
    else if((rpn_len = ss_recog_constant(&data, docno, text_in, &optblock, 0)) > 0)
        {
        ev_data_to_result_convert(p_ev_result, &data);
        rpn_len = 0;
        parmsp->type = EVS_CON_DATA;
        }
    else
        {
        rpn_len = ev_compile(docno,
                             text_in,
                             compiled_out,
                             len_out,
                             at_pos,
                             parmsp,
                             &optblock);

        if(!rpn_len)
            rpn_len = create_error(EVAL_ERR_BADEXPR);
        }

    trace_0(TRACE_MODULE_EVAL, " compiled");

    return(rpn_len);
}

/******************************************************************************
*
* add an entry to the draw file references list
*
******************************************************************************/

static S32
draw_add_file_ref(
    GR_CACHE_HANDLE draw_file_key,
    P_F64 xp,
    P_F64 yp,
    COL col,
    ROW row)
{
    P_DRAW_DIAG p_draw_diag;
    P_DRAW_FILE_REF p_draw_file_ref;
    EV_RANGE rng;
    S32 res;
    S32 ext_dep_han;

    /* prepare for adding external dependency */
    set_ev_slr(&rng.s, col,     row    );
    set_ev_slr(&rng.e, col + 1, row + 1);

    /* search for a duplicate reference */
    if((p_draw_file_ref = draw_search_refs_slot(&rng.s)) == NULL)
        {
        /* add external dependency */
        if((res = ev_add_extdependency(0,
                                       0,
                                       &ext_dep_han,
                                       draw_uref,
                                       &rng)) < 0)
            return(res);

        /* key into list using external dependency handle */

        if(NULL == (p_draw_file_ref = collect_add_entry_elem(DRAW_FILE_REF, &draw_file_refs, (P_LIST_ITEMNO) &ext_dep_han, &res)))
            {
            ev_del_extdependency(rng.s.docno, ext_dep_han);
            return(res);
            }

        p_draw_file_ref->draw_file_key = draw_file_key;
        p_draw_file_ref->docno = rng.s.docno;
        p_draw_file_ref->col = col;
        p_draw_file_ref->row = row;

        gr_cache_ref(&draw_file_key, 1); /* add a ref */

        trace_1(TRACE_MODULE_UREF, "draw_add_file_ref adding draw file ref: " PTR_XTFMT, report_ptr_cast(draw_file_key));
        }

    /* check for range and load factors */
    if((*xp > SHRT_MAX)  ||  (*xp < 1.0))
        p_draw_file_ref->xfactor = 1.0;
    else
        p_draw_file_ref->xfactor = *xp / 100.0;

    if((*yp > SHRT_MAX)  ||  (*yp < 1.0))
        p_draw_file_ref->yfactor = 1.0;
    else
        p_draw_file_ref->yfactor = *yp / 100.0;

    p_draw_diag = gr_cache_search(&p_draw_file_ref->draw_file_key);

    draw_adjust_file_ref(p_draw_diag, p_draw_file_ref);

    return(1);
}

/******************************************************************************
*
* adjust a draw file reference to reflect a new draw file entry
*
******************************************************************************/

static void
draw_adjust_file_ref(
    P_DRAW_DIAG p_draw_diag,
    P_DRAW_FILE_REF p_draw_file_ref)
{
    P_DOCU p_docu;
    coord coff, roff;
    P_SCRCOL cptr;
    P_SCRROW rptr;
    COL tcol;
    ROW trow;
    P_DRAW_DIAG sch_p_draw_diag;

    trace_2(TRACE_APP_PD4, "draw_adjust_file_ref(" PTR_XTFMT ", " PTR_XTFMT ")", report_ptr_cast(p_draw_diag), report_ptr_cast(p_draw_file_ref));

    if(p_draw_diag->data)
        {
        DRAW_BOX box;
        S32 xsize, ysize;
        S32 scaled_xsize, scaled_ysize;

        /* get box in Draw units */
        box = ((PC_DRAW_FILE_HEADER) p_draw_diag->data)->bbox;

        xsize = box.x1 - box.x0;
        ysize = box.y1 - box.y0;

        scaled_xsize = (S32) ceil(xsize * p_draw_file_ref->xfactor);
        scaled_ysize = (S32) ceil(ysize * p_draw_file_ref->yfactor);

        /* save size in OS units rounded out to worst-case pixels */
        p_draw_file_ref->xsize_os = div_round_ceil(scaled_xsize, GR_RISCDRAW_PER_RISCOS * 4) * 4;
        p_draw_file_ref->ysize_os = div_round_ceil(scaled_ysize, GR_RISCDRAW_PER_RISCOS * 4) * 4;

        trace_4(TRACE_APP_PD4, "draw_adjust_file_ref: bbox of file is %d %d %d %d (os)",
                box.x0, box.y0, box.x1, box.y1);

        /* scale sizes */
        trace_2(TRACE_APP_PD4, "draw_adjust_file_ref: scaled sizes x: %d, y: %d (os)",
                p_draw_file_ref->xsize_os, p_draw_file_ref->ysize_os);
        }

    /* a certain amount of redrawing may be necessary */
    p_docu = p_docu_from_docno(p_draw_file_ref->docno);
    assert(NO_DOCUMENT != p_docu);
    assert(!docu_is_thunk(p_docu));

    if(p_docu->Xpict_on_screen)
        {
        DOCNO old_docno = current_docno();

        select_document(p_docu);

        rptr = vertvec();

        for(roff = 0; roff < rowsonscreen; roff++, rptr++)
            {
            if(rptr->flags & PICT)
                {
                trow = rptr->rowno;

                cptr = horzvec();

                for(coff = 0; !(cptr->flags & LAST); coff++, cptr++)
                    {
                    tcol = cptr->colno;

                    if(draw_find_file(current_docno(), tcol, trow, &sch_p_draw_diag, NULL))
                        {
                        trace_3(TRACE_APP_PD4, "found picture at col %d, row %d, p_draw_diag " PTR_XTFMT, tcol, trow, report_ptr_cast(sch_p_draw_diag));

                        if(p_draw_diag == sch_p_draw_diag)
                            {
                            xf_interrupted = TRUE;
#if 1
                            /* SKS after 4.11 22jan92 - slight (and careful) optimisation here; Draw files always hang down */
                            if(out_below)
                                rowtoend  = MIN(rowtoend, roff);
                            else
                                {
                                out_below = TRUE;
                                rowtoend  = roff;
                                }

                            xf_draw_empty_right = 1;
#else
                            out_screen = TRUE;
#endif
                            goto FOUND_ONE;
                            }
                        }
                    }
                }
            }

    FOUND_ONE:;

        select_document_using_docno(old_docno);
        }
}

/******************************************************************************
*
* redraw all pictures (esp. for palette change)
*
******************************************************************************/

extern void
draw_redraw_all_pictures(void)
{
    LIST_ITEMNO key;
    P_DRAW_FILE_REF p_draw_file_ref;

    trace_0(TRACE_APP_PD4, "draw_redraw_all_pictures()");

    for(p_draw_file_ref = collect_first(DRAW_FILE_REF, &draw_file_refs.lbr, &key);
        p_draw_file_ref;
        p_draw_file_ref = collect_next( DRAW_FILE_REF, &draw_file_refs.lbr, &key))
        {
        P_DRAW_DIAG p_draw_diag = gr_cache_search(&p_draw_file_ref->draw_file_key);

        draw_adjust_file_ref(p_draw_diag, p_draw_file_ref);
        }
}

/******************************************************************************
*
* a Draw file has been reloaded into the cache
*
******************************************************************************/

gr_cache_recache_proto(static, draw_file_recached)
{
    /* search draw file ref list and reload scale info, cause redraw etc. */
    LIST_ITEMNO key;
    P_DRAW_FILE_REF p_draw_file_ref;

    IGNOREPARM(handle);
    IGNOREPARM(cres);

    for(p_draw_file_ref = collect_first(DRAW_FILE_REF, &draw_file_refs.lbr, &key);
        p_draw_file_ref;
        p_draw_file_ref = collect_next( DRAW_FILE_REF, &draw_file_refs.lbr, &key))
        {
        if(p_draw_file_ref->draw_file_key == cah)
            {
            P_DRAW_DIAG p_draw_diag = gr_cache_search(&p_draw_file_ref->draw_file_key);

            if(NULL == p_draw_diag)
                p_draw_diag = gr_cache_search_empty(); /* paranoia in case of reload failure */

            draw_adjust_file_ref(p_draw_diag, p_draw_file_ref);
            }
        }
}

/******************************************************************************
*
* given the details of a draw file, make sure it is in the cache
*
* --out--
* -1   = error
* >= 0 = key number of draw file
*
******************************************************************************/

static S32
draw_cache_file(
    PC_U8 name,
    P_F64 xp,
    P_F64 yp,
    COL col,
    ROW row,
    S32 load_as_preferred,
    S32 explicit_load)
{
    GR_CACHE_HANDLE draw_file_key;
    U32 wacky_tag = GR_RISCDIAG_WACKYTAG;
    struct PDCHART_TAGSTRIP_INFO info;
    U8  namebuf[BUF_MAX_PATHSTRING];
    S32 res;
    GR_CHART_HANDLE ch;
    P_ANY ext_handle;
    S32 chart_exists;
    S32 added_stripper = 0;

    trace_1(TRACE_APP_PD4, "draw_cache_file(%s)", name);

    res = is_current_document()
        ? file_find_on_path_or_relative(namebuf, elemof32(namebuf), name, currentfilename)
        : file_find_on_path(namebuf, elemof32(namebuf), name);

    if(res <= 0)
        return(res ? res : create_error(FILE_ERR_NOTFOUND));

    info.pdchartdatakey = NULL;

    chart_exists = gr_chart_query_exists(&ch, &ext_handle, namebuf);

    if(chart_exists && explicit_load)
        {
        /*reperr(ERR_CHART_ALREADY_LOADED, namebuf);*/
        return(pdchart_show_editor_using_handle(ext_handle));
        }

    (void) gr_cache_entry_query(&draw_file_key, namebuf);

    if(!draw_file_key)
        {
        if((res = gr_cache_entry_ensure(&draw_file_key, namebuf)) < 0)
            return(res);

        /* add recache proc for next time it gets loaded */
        gr_cache_recache_inform(&draw_file_key, draw_file_recached, NULL);

        /* add tag stripper iff chart not already loaded */
        if(!chart_exists)
            {
            status_assert(gr_cache_tagstrip(pdchart_tagstrip, &info, wacky_tag, 1));

            added_stripper = 1;
            }
        }

    (void) gr_cache_loaded_ensure(&draw_file_key);

    if(added_stripper)
        status_assert(gr_cache_tagstrip(pdchart_tagstrip, &info, wacky_tag, 0));

    /* if we loaded a live Chart file we'll now want to ensure its dependent docs are loaded too */

    if(info.pdchartdatakey)
        {
        /* when all refs to this chart go to zero then kill the cache entry */
        gr_cache_entry_set_autokill(&draw_file_key);

        if((res = pdchart_load_dependents(info.pdchartdatakey, namebuf)) < 0)
            {
            reperr_null(res);
            been_error = FALSE;
            }

        pdchart_load_ended(info.pdchartdatakey, load_as_preferred);

        if(explicit_load && !load_as_preferred)
            {
            pdchart_select_using_handle(info.pdchartdatakey);

            if((res = pdchart_show_editor_using_handle(info.pdchartdatakey)) < 0)
                {
                reperr_null(res);
                been_error = FALSE;
                }
            }
        }

    if(gr_cache_error_query(&draw_file_key) >= 0)
        {
        if(is_current_document())
            {
            res = draw_add_file_ref(draw_file_key, xp, yp, col, row);

            if(res <= 0)
                return(res ? res : status_nomem());
            }
        }

    return((S32) draw_file_key);
}

extern BOOL
pdchart_load_isolated_chart(
    PC_U8 filename)
{
    S32 res;

    if((res = draw_cache_file(filename, NULL, NULL, 0, 0, 0 /*not_loading_preferred*/, 1 /*explicit load*/)) < 0)
        return(reperr(res, filename));

    return(TRUE);
}

extern BOOL
pdchart_load_preferred(
    P_U8 filename)
{
    S32 res;

    if((res = draw_cache_file(filename, NULL, NULL, 0, 0, 1 /*loading_preferred*/, 1 /*explicit load*/)) < 0)
        return(reperr(res, filename));

    return(TRUE);
}

/******************************************************************************
*
* return pointers to information for a draw file
* given the document and row
*
******************************************************************************/

extern S32
draw_find_file(
    _InVal_     DOCNO docno,
    COL col,
    ROW row,
    P_P_DRAW_DIAG p_p_draw_diag,
    P_P_DRAW_FILE_REF p_p_draw_file_ref)
{
    LIST_ITEMNO key;
    P_DRAW_FILE_REF p_draw_file_ref;

    trace_3(TRACE_APP_PD4, "draw_find_file: docno %d %d, %d", docno, col, row);

    for(p_draw_file_ref = collect_first(DRAW_FILE_REF, &draw_file_refs.lbr, &key);
        p_draw_file_ref;
        p_draw_file_ref = collect_next( DRAW_FILE_REF, &draw_file_refs.lbr, &key))
        {
        trace_3(TRACE_APP_PD4, "draw_find_file found docno: %d, col: %d, row: %d",
                p_draw_file_ref->docno, p_draw_file_ref->col, p_draw_file_ref->row);

        if( (docno == p_draw_file_ref->docno)  &&
            ((col == -1) || (col == p_draw_file_ref->col))  &&
            (row == p_draw_file_ref->row))
            {
            P_DRAW_DIAG p_draw_diag = gr_cache_search(&p_draw_file_ref->draw_file_key);

            if(p_draw_diag  &&  p_draw_diag->data)
                {
                trace_1(TRACE_APP_PD4, "draw_find_file found key: " PTR_XTFMT, report_ptr_cast(p_draw_file_ref->draw_file_key));

                if(p_p_draw_diag)
                    *p_p_draw_diag = p_draw_diag;

                if(p_p_draw_file_ref)
                    *p_p_draw_file_ref = p_draw_file_ref;

                return(1);
                }
            }
        }

    return(0);
}

/******************************************************************************
*
* remove a draw file reference
*
******************************************************************************/

static void
draw_remove_ref(
    S32 ext_dep_han)
{
    P_DRAW_FILE_REF p_draw_file_ref;

    if((p_draw_file_ref = draw_search_refs(ext_dep_han)) != NULL)
        {
        /* remove external dependency */
        ev_del_extdependency(p_draw_file_ref->docno, ext_dep_han);

        trace_1(TRACE_MODULE_UREF,
                "draw_remove_ref removing draw file ref: " PTR_XTFMT,
                report_ptr_cast(p_draw_file_ref->draw_file_key));
        gr_cache_ref(&p_draw_file_ref->draw_file_key, 0); /* remove a ref */

        collect_subtract_entry(&draw_file_refs.lbr, (LIST_ITEMNO) ext_dep_han);
        }
}

/******************************************************************************
*
* search list of draw file references
*
******************************************************************************/

static P_DRAW_FILE_REF
draw_search_refs(
    S32 ext_dep_han)
{
    return(collect_goto_item(DRAW_FILE_REF, &draw_file_refs.lbr, (LIST_ITEMNO) ext_dep_han));
}

/******************************************************************************
*
* search list of draw refs for entry for given slot
*
******************************************************************************/

static P_DRAW_FILE_REF
draw_search_refs_slot(
    _InRef_     PC_EV_SLR p_slr)
{
    LIST_ITEMNO key;
    P_DRAW_FILE_REF p_draw_file_ref;

    for(p_draw_file_ref = collect_first(DRAW_FILE_REF, &draw_file_refs.lbr, &key);
        p_draw_file_ref;
        p_draw_file_ref = collect_next( DRAW_FILE_REF, &draw_file_refs.lbr, &key))
        {
        if( (p_draw_file_ref->docno == p_slr->docno) &&
            (p_draw_file_ref->col == p_slr->col) &&
            (p_draw_file_ref->row == p_slr->row) )
                return(p_draw_file_ref);
        }

    return(NULL);
}

/******************************************************************************
*
* search a slot for a draw file reference
* and if found, add it to the list
*
******************************************************************************/

extern S32
draw_str_insertslot(
    COL col,
    ROW row)
{
    S32 found_file = 0;
    P_SLOT sl;
    F64 x, y;
    S32 len, res;
    PC_U8Z name;
    char tbuf[LIN_BUFSIZ];
    PC_U8 c;
    P_U8 to;
    const uchar text_at_char = get_text_at_char();

    trace_2(TRACE_APP_PD4, "draw_str_insertslot col: %d, row: %d", col, row);

    if(NULLCH == text_at_char)
        return(0);

    if(NULL == (sl = travel(col, row)))
        return(0);

    if(!(sl->flags & SL_TREFS))
        return(0);

    c = sl->content.text;

    while(NULLCH != *c)
        {
        if(SLRLD1 == *c) /* because of possibility of text-at char change, this need not be preceded by the current text-at char (orphaned) */
            {
            c += COMPILED_TEXT_SLR_SIZE;
            continue;
            }

        if(*c++ != text_at_char)
            continue;

        /* skip over multiple text-at chars */
        if(*c == text_at_char)
            {
            while(*c == text_at_char)
                ++c;
            continue;
            }

        /* check for a draw file reference */
        to = tbuf;
        len = readfxy('G', &c, &to, &name, &x, &y);
        if(len)
            {
            char namebuf[BUF_MAX_PATHSTRING];

            xstrnkpy(namebuf, elemof32(namebuf), name, len);

            trace_0(TRACE_APP_PD4, "draw_str_insertslot found draw file ref");
            if((res = draw_cache_file(namebuf, &x, &y, col, row, 0, 0)) >= 0)
                found_file = 1;
            else
                {
                reperr(res, namebuf);
                been_error = FALSE;
                }
            break;
            }
        }

    return(found_file);
}

/******************************************************************************
*
* insert a slot from both draw and tree structures
*
******************************************************************************/

extern S32
draw_tree_str_insertslot(
    COL col,
    ROW row,
    S32 sort)
{
    S32 res;

    IGNOREPARM(sort);

    res = text_slot_add_dependency(col, row);

    if(draw_str_insertslot(col, row))
        out_rebuildvert = TRUE;

    slot_changed(col, row);

    return(res);
}

/******************************************************************************
*
* handle uref calls for draw files
*
******************************************************************************/

PROC_UREF_PROTO(static, draw_uref)
{
    IGNOREPARM(exthandle);
    IGNOREPARM(status);

    trace_0(TRACE_MODULE_UREF, "draw_uref");

    switch(upp->action)
        {
        case UREF_CHANGEDOC:
        case UREF_CHANGE:
        case UREF_REDRAW:
            break;

        case UREF_REPLACE:
        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            {
            P_DRAW_FILE_REF p_draw_file_ref = draw_search_refs(inthandle);

            if(p_draw_file_ref != NULL)
                {
                switch(upp->action)
                    {
                    case UREF_SWAP:
                    case UREF_SWAPSLOT:
                    case UREF_UREF:
                        p_draw_file_ref->col = at_rng->s.col;
                        p_draw_file_ref->row = at_rng->s.row;

                        trace_3(TRACE_MODULE_UREF,
                                "draw uref updated doc: %d, col: %d, row: %d",
                                p_draw_file_ref->docno, p_draw_file_ref->col, p_draw_file_ref->row);
                        break;

                    case UREF_REPLACE:
                    case UREF_DELETE:
                    case UREF_CLOSE:
                        trace_3(TRACE_MODULE_UREF,
                                "draw uref frees doc: %d, col: %d, row: %d",
                                p_draw_file_ref->docno, p_draw_file_ref->col, p_draw_file_ref->row);
                        draw_remove_ref(inthandle);
                        break;

                    default:
                        break;
                    }
                }
            break;
            }
        }
}

/*
enter string as formula in numeric slot

useful for macro files in PD4
*/

extern void
Newslotcontents_fn(void)
{
    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_NAME))
        {
        strcpy(linbuf, d_name[0].textfield);

        buffer_altered  = TRUE;
        slot_in_buffer  = TRUE;
        xf_inexpression = TRUE;
        out_currslot    = TRUE;

        seteex_nodraw();
        merexp();
        endeex_nodraw();

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();
}

/******************************************************************************
*
* end expression editing
*
******************************************************************************/

extern void
endeex_nodraw(void)
{
    xf_inexpression = FALSE;
    slot_in_buffer = buffer_altered = FALSE;
    lecpos = lescrl = 0;
}

extern void
endeex(void)
{
    endeex_nodraw();

    /* need to redraw more if inverse cursor moved */
    if((edtslr_col != curcol)  ||  (edtslr_row != currow))
        mark_row(currowoffset);
    output_buffer = out_currslot = TRUE;
    xf_blockcursor = FALSE;
}

/******************************************************************************
*
* call-back from evaluator uref to redraw altered slots
*
******************************************************************************/

PROC_UREF_PROTO(extern, eval_wants_slot_drawn)
{
    IGNOREPARM(inthandle);
    IGNOREPARM_InRef_(at_rng);
    IGNOREPARM(status);

    switch(upp->action)
        {
        case UREF_REDRAW:
            {
            DOCNO old_docno = change_document_using_docno((DOCNO) exthandle);
            P_SLOT tslot;

            /* don't redraw blanks and text slots unless they contain slrs */
            tslot = travel((COL) upp->slr1.col, (ROW) upp->slr1.row);

            if(tslot != NULL)
                if((tslot->type == SL_NUMBER) ||
                   (tslot->type == SL_TEXT && (tslot->flags & SL_TREFS)) )
                    draw_one_altered_slot((COL) upp->slr1.col, (ROW) upp->slr1.row);

            select_document_using_docno(old_docno);
            break;
            }

#if 0 /* SKS 20130404 */
        case UREF_RENAME:
            {
            DOCNO old_docno = change_document_using_docno((DOCNO) exthandle);

            filealtered(TRUE);

            select_document_using_docno(old_docno);
            break;
            }
#endif
        }
}

/******************************************************************************
*
* exchange numbers for text
*
******************************************************************************/

extern void
ExchangeNumbersText_fn(void)
{
    bash_slots_about_a_bit(bash_exchange);
}

extern void
ToNumber_fn(void)
{
    bash_slots_about_a_bit(bash_tonumber);
}

extern void
ToText_fn(void)
{
    bash_slots_about_a_bit(bash_totext);
}

/******************************************************************************
*
* fill buffer from slot
*
******************************************************************************/

extern void
filbuf(void)
{
    BOOL zap = FALSE;
    P_SLOT nslot;

    if(slot_in_buffer || buffer_altered || xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return;

    nslot = travel_here();

    if(nslot)
        {
        if((nslot->type == SL_TEXT)  &&  !(nslot->justify & PROTECTED))
            {
            prccon(linbuf, nslot);
            slot_in_buffer = TRUE;
            }
        else
            /* ensure linbuf zapped */
            zap = TRUE;
        }

    if(!nslot  ||  zap)
        {
        *linbuf = '\0';
        lescrl = 0;
        slot_in_buffer = !zap;
        }
}

/*
Return column number. Assumes it can read as many alphas as it likes
Deals with leading spaces.

Reads from extern buff_sofar, updating it
*/

extern COL
getcol(void)
{
    COL res = -1;
    char ch;

    if(buff_sofar == NULL)
        return(NO_COL);

    while(ch = *buff_sofar, ispunct(ch)  ||  (ch == SPACE))
        buff_sofar++;

    while(ch = *buff_sofar, isalpha(ch))
        {
        buff_sofar++;
        res = (res+1)*26 + toupper(ch) - 'A';
        }

    return((res == -1) ? NO_COL : res);
}

/******************************************************************************
*
* give warning if graphics links
*
******************************************************************************/

extern BOOL
dependent_links_warning(void)
{
    PC_LIST lptr;
    const DOCNO docno = current_docno();
    S32 nLinks = 0;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        PC_GRAPHICS_LINK_ENTRY glp = (P_GRAPHICS_LINK_ENTRY) lptr->value;

        if(glp->docno == docno)
            ++nLinks;
        }

    if(nLinks)
        return(riscdialog_query_YN(close_dependent_links_winge_STR) == riscdialog_query_YES);

    return(TRUE);
}

/******************************************************************************
*
* add an entry to the graphics link list
*
******************************************************************************/

extern S32
graph_add_entry(
    ghandle ghan,
    _InVal_     DOCNO docno,
    COL col,
    ROW row,
    S32 xsize,
    S32 ysize,
    PC_U8 leaf,
    PC_U8 tag,
    S32 task)
{
    S32 leaflen = strlen(leaf);
    S32 taglen  = strlen(tag);
    P_LIST lptr;
    P_GRAPHICS_LINK_ENTRY glp;
    P_U8 ptr;
    S32 res;
    EV_RANGE rng;

    /* extra +1 accounted for in sizeof() */
    lptr = add_list_entry(&graphics_link_list,
                          sizeof32(struct GRAPHICS_LINK_ENTRY) + leaflen + taglen + 1,
                          &res);

    if(!lptr)
        return(res);

    lptr->key = (S32) ghan;
    glp = (P_GRAPHICS_LINK_ENTRY) (lptr->value);

    glp->docno = docno;
    glp->col    = col;
    glp->row    = row;

    glp->task   = task;
    glp->update = FALSE;

    glp->ghan   = ghan;
    glp->xsize  = xsize;
    glp->ysize  = ysize;

    ptr = glp->text;
    strcpy(ptr, leaf);
    ptr += leaflen + 1;
    strcpy(ptr, tag);

    /* add external dependency */
    set_ev_slr(&rng.s,             col,             row);
    set_ev_slr(&rng.e, col + xsize + 1, row + ysize + 1);
    if((res = ev_add_extdependency((U32) ghan,
                                   0,
                                   &glp->ext_dep_han,
                                   graph_uref,
                                   &rng)) < 0)
        {
        graph_remove_entry(ghan);
        return(res);
        }

    return((S32) ghan);
}

/******************************************************************************
*
* any active graphs in this document?
*
******************************************************************************/

extern BOOL
graph_active_present(
    _InVal_     DOCNO docno)
{
    PC_LIST lptr;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
        {
        PC_GRAPHICS_LINK_ENTRY glp = (PC_GRAPHICS_LINK_ENTRY) lptr->value;

        if(glp->update  &&  ((docno == DOCNO_NONE)  ||  (glp->docno == docno)))
            return(TRUE);
        }

    return(FALSE);
}

/******************************************************************************
*
* delete the graphics list entry for this handle
*
******************************************************************************/

extern void
graph_remove_entry(
    ghandle han)
{
    PC_GRAPHICS_LINK_ENTRY glp = graph_search_list(han);

    if(glp != NULL)
        {
        /* remove external dependency */
        ev_del_extdependency(glp->docno,
                             glp->ext_dep_han);
        delete_from_list(&graphics_link_list, (S32) han);
        }
}

/******************************************************************************
*
* find the graphics list entry for this handle
*
******************************************************************************/

extern P_GRAPHICS_LINK_ENTRY
graph_search_list(
    ghandle han)
{
    P_LIST lptr = search_list(&graphics_link_list, (S32) han);

    return(lptr ? (P_GRAPHICS_LINK_ENTRY) (lptr->value) : NULL);
}

/******************************************************************************
*
* send a block of slots off to graphics link
*
******************************************************************************/

static void
graph_send_block(
    P_GRAPHICS_LINK_ENTRY glp,
    COL scol,
    ROW srow,
    COL ecol,
    ROW erow)
{
    trace_4(TRACE_MODULE_UREF, "graph_send_block scol: %d, srow: %d, ecol: %d, erow: %d", scol, srow, ecol, erow);

    if(glp->update)
        {
        COL tcol;
        ROW trow;

        scol = MAX(scol, glp->col);
        srow = MAX(srow, glp->row);
        ecol = MIN(ecol, glp->col + glp->xsize + 1);
        erow = MIN(erow, glp->row + glp->ysize + 1);

        for(tcol = scol; tcol < ecol; ++tcol)
            for(trow = srow; trow < erow; ++trow)
                riscos_sendslotcontents(glp, (S32) (tcol - glp->col), (S32) (trow - glp->row));
        }
}

/******************************************************************************
*
* handle call-back from uref for draw file references
*
******************************************************************************/

PROC_UREF_PROTO(static, graph_uref)
{
    IGNOREPARM(inthandle);

    trace_0(TRACE_MODULE_UREF, "graph_uref");

    switch(upp->action)
        {
        case UREF_CHANGEDOC:
        case UREF_REPLACE:
        case UREF_REDRAW:
            break;

        case UREF_CHANGE:
        case UREF_UREF:
        case UREF_DELETE:
        case UREF_SWAP:
        case UREF_SWAPSLOT:
        case UREF_CLOSE:
            {
            P_GRAPHICS_LINK_ENTRY glp = graph_search_list((ghandle) exthandle);

            if(glp != NULL)
                {
                DOCNO old_docno = change_document_using_docno(glp->docno);

                switch(upp->action)
                    {
                    case UREF_CHANGE:
                        graph_send_block(glp, upp->slr1.col, upp->slr1.row, upp->slr1.col + 1, upp->slr1.row + 1);
                        break;

                    case UREF_DELETE:
                        if(status == DEP_DELETE)
                            goto kill_ref;
                        else
                            goto update_ref;
                        break;

                    case UREF_UREF:
                    update_ref:
                        if(status == DEP_UPDATE)
                            {
                            glp->col = at_rng->s.col;
                            glp->row = at_rng->s.row;

                            /* transmit if block has changed size */
                            if( at_rng->e.col - at_rng->s.col != glp->xsize + 1 ||
                                at_rng->e.row - at_rng->s.row != glp->ysize + 1)
                                {
                                riscos_sendallslots(glp);

                                /* reset extent of dependency */
                                at_rng->e.col = at_rng->s.col + glp->xsize + 1;
                                at_rng->e.row = at_rng->s.row + glp->ysize + 1;
                                }
                            else
                                graph_send_block(glp, upp->slr1.col, upp->slr1.row, upp->slr2.col, upp->slr2.row);
                            }
                        else
                            graph_send_block(glp, upp->slr1.col, upp->slr1.row, upp->slr2.col, upp->slr2.row);
                        break;

                    case UREF_SWAP:
                        glp->col = at_rng->s.col;
                        glp->row = at_rng->s.row;
                        graph_send_block(glp, upp->slr1.col, upp->slr1.row, upp->slr2.col, upp->slr1.row + 1);
                        graph_send_block(glp, upp->slr1.col, upp->slr2.row, upp->slr2.col, upp->slr2.row + 1);
                        break;

                    case UREF_SWAPSLOT:
                        glp->col = at_rng->s.col;
                        glp->row = at_rng->s.row;
                        graph_send_block(glp, upp->slr1.col, upp->slr1.row, upp->slr1.col + 1, upp->slr1.row + 1);
                        graph_send_block(glp, upp->slr2.col, upp->slr2.row, upp->slr2.col + 1, upp->slr2.row + 1);
                        break;

                    case UREF_CLOSE:
                    kill_ref:
                        riscos_sendsheetclosed(glp);
                        graph_remove_entry((ghandle) exthandle);
                        break;

                    default:
                        break;
                    }

                select_document_using_docno(old_docno);
                }
            break;
            }
        }
}

extern void
mark_slot(
    P_SLOT tslot)
{
    if(tslot)
        {
        tslot->flags |= SL_ALTERED;
        xf_drawsome = TRUE;
        trace_0(TRACE_APP_PD4, "slot marked altered");
        }
}

/******************************************************************************
*
* compile and merge expression
*
* merexp()                     - compile expression, if it contains an error return as a text slot
*
* merexp_reterr(NULL, NULL)    - compile expression, if it contains an error return as a text slot
* merexp_reterr(&err, &at_pos) - compile expression, return error and abort compilation
*
******************************************************************************/

extern void
merexp(void)
{
    merexp_reterr(NULL, NULL, TRUE);
}

extern void
merexp_reterr(
    P_S32 errp,
    P_S32 at_posp,
    S32 send_replace)
{
    P_SLOT tslot;
    S32 rpn_len, res, at_pos;
    char justify, format;
    char compiled_out[EV_MAX_OUT_LEN];
    EV_RESULT result;
    EV_PARMS parms;
    EV_SLR slr;

    movement |= ABSOLUTE;
    newcol    = edtslr_col;
    newrow    = edtslr_row;

    if(errp)
        *errp = 0;

    if(at_posp)
        *at_posp = -1;

    if(!buffer_altered)
        {
        slot_in_buffer = FALSE;
        return;
        }

    filealtered(TRUE);

    /* do replace before compile otherwise we delete
     * references that compile establishes (names, funcs etc)
     */
    tslot = travel(newcol, newrow);
    if(send_replace)
        slot_to_be_replaced(newcol, newrow, tslot);
    else if(tslot)
        slot_free_resources(tslot);

    if((rpn_len = compile_expression(compiled_out,
                                     linbuf,
                                     EV_MAX_OUT_LEN,
                                     &at_pos,
                                     &result,
                                     &parms)) < 0)
        {
        /* expression won't compile */

        /* if caller asked for errors to be returned, do so */
        if(errp)
            {
            *errp = rpn_len;

            if(at_posp)
                *at_posp = at_pos;

            return;
            }

        /* else make it a text slot */
        merstr(curcol, currow, send_replace, TRUE);
        return;
        }

    justify = J_RIGHT;
    format = 0;

    /* tell uref we're about to replace slot */
    set_ev_slr(&slr, newcol, newrow);

    /* save justify flags for numeric slot */
    if(tslot)
        {
        switch(tslot->type)
            {
            case SL_NUMBER:
                justify = tslot->justify;
                format  = tslot->format;
                break;

            default:
                break;
            }
        }

    if((res = merge_compiled_exp(&slr, justify, format, compiled_out, rpn_len, &result, &parms, FALSE, TRUE)) < 0)
        {
        *linbuf = '\0';
        reperr_null(status_nomem());
        }

    if(send_replace && (parms.type == EVS_CON_DATA || res < 0))
        slot_changed(newcol, newrow);

    buffer_altered = slot_in_buffer = FALSE;
}

/******************************************************************************
*
* merge a compiled expression into a slot
*
******************************************************************************/

extern S32
merge_compiled_exp(
    _InRef_     PC_EV_SLR slrp,
    char justify,
    char format,
    P_U8 compiled_out,
    S32 rpn_len,
    P_EV_RESULT p_ev_result,
    P_EV_PARMS parmsp,
    S32 load_flag,
    S32 add_refs)
{
    S32 add_to_tree, tot_len, res;
    P_SLOT tslot;

    /* calculate new total length */
    switch(parmsp->type)
        {
        default:
            assert(0);
        case EVS_CON_DATA:
            tot_len = NUM_DAT_OVH;
            break;
        case EVS_CON_RPN:
            tot_len = NUM_CON_OVH + rpn_len;
            break;
        case EVS_VAR_RPN:
            tot_len = NUM_VAR_OVH + rpn_len;
            break;
        }

    res = 0;

    /* get a slot the correct size */
    if((tslot = createslot((COL) slrp->col, (ROW) slrp->row, tot_len, SL_NUMBER)) == NULL)
        res = -1;
    else
        {
        /* copy across data */
        tslot->justify                       = justify;
        tslot->format                        = format;
        tslot->content.number.guts.ev_result = *p_ev_result;
        tslot->content.number.guts.parms     = *parmsp;

        add_to_tree = 0;
        switch(parmsp->type)
            {
            case EVS_CON_DATA:
                if(!load_flag)
                    ev_todo_add_dependents(slrp);
                break;

            case EVS_CON_RPN:
                memcpy32(tslot->content.number.guts.rpn.con.rpn_str, compiled_out, rpn_len);
                add_to_tree = 1;
                break;

            case EVS_VAR_RPN:
                tslot->content.number.guts.rpn.var.visited = 0;
                memcpy32(tslot->content.number.guts.rpn.var.rpn_str, compiled_out, rpn_len);
                add_to_tree = 1;
                break;
            }

        if(add_refs && add_to_tree)
            {
            if(ev_add_exp_slot_to_tree(&tslot->content.number.guts, slrp) < 0)
                {
                tree_insert_fail(slrp);
                res = -1;
                }
            else
                {
                ev_todo_add_slr(slrp, load_flag ? 0 : 1);

                /* force left justification for macro slots */
                if(ev_doc_is_custom_sheet(current_docno()))
                    tslot->justify = J_LEFT;
                }
            }
        }

    return(res);
}

/******************************************************************************
*
* merge line buf
*
******************************************************************************/

static BOOL
mergebuf_core(
    BOOL allow_check,
    S32 send_replace)
{
    if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
        return(TRUE);

    if(!buffer_altered)
        slot_in_buffer = FALSE;

    if(!slot_in_buffer)
        return(TRUE);

    filealtered(TRUE);

    if(!merstr(curcol, currow, send_replace, TRUE))
        return(FALSE);

    if(allow_check)
        check_word();

    return(TRUE);
}

extern BOOL
mergebuf(void)
{
    return(mergebuf_core(TRUE, TRUE));
}

extern BOOL
mergebuf_nocheck(void)
{
    return(mergebuf_core(FALSE, TRUE));
}

/* find first instance of ch or NULLCH in s */

_Check_return_
static inline const uchar *
xstrzchr(
    _In_z_      const uchar * s,
    _InVal_     uchar ch)
{
    for(;;)
    {
        uchar c = *s++;

        if((ch == c) || (NULLCH == c))
            return(s - 1);
    }
}

/*
reasonably fast combined strlen / strchr for text-at fields
*/

static S32
buffer_length_detecting_text_at_char(
    _In_z_      PC_U8 str_in,
    _OutRef_    P_BOOL p_contains_tac)
{
    PC_U8 str = str_in;
    const uchar text_at_char = get_text_at_char();

    *p_contains_tac = FALSE;

    /* Norcroft strlen() is FAST */
    if(NULLCH == text_at_char)
        return(strlen(str) + 1 /*NULLCH*/);

#if defined(SLRLD2) /* can optimise when SLRLD2==NULLCH is used after SLRLD1 */
    if(NULLCH == *str) /* verifying string is not empty saves a test per loop */
        return(1); /*NULLCH*/

    for(;;)
        {
        PC_U8 ptr;

        /* search for text-at char and NULLCH together using our xstrzchr() */
        /* this will also find orphaned (text-at char changed) SLRLD1/2 */
        /* if fact we only need to find first instance of text-at char, then speed up with fast Norcroft strlen() */
        if(!*p_contains_tac)
            {
            ptr = xstrzchr(str, text_at_char);

            if(text_at_char == *ptr)
                {
                *p_contains_tac = TRUE;
                str = ptr + 1;
                if(SLRLD1 == *str) /* cater for this common occurrence (saves one loop) */
                    str += COMPILED_TEXT_SLR_SIZE;
                continue;
                }

            /* failed to find text_at_char but ptr is pointing at a NULLCH (either real or SLRLD2) */
            }
        else
            {
            ptr = str + strlen(str);
            }

        if(SLRLD1 == ptr[-1]) /* ptr is never == str */
            { /* the NULLCH we found is SLRLD2 - keep going */
            str = ptr + COMPILED_TEXT_SLR_SIZE-1; /* restart past the compiled SLR */
            continue;
            }

        /* (NULLCH == *ptr) */
        str = ptr + 1; /* want to leave str pointing past the real NULLCH */
        return(PtrDiffBytesS32(str, str_in));
        }
#else
    for(;;)
        {
        uchar ch = *str++; /* want to leave str pointing past NULLCH */

        if(NULLCH == ch)
            break;

        if(SLRLD1 == ch)
            {
            str += COMPILED_TEXT_SLR_SIZE-1;
            continue;
            }

        if(text_at_char == ch) /* catered for NULLCH t-a-c above */
            *p_contains_tac = TRUE;
    }

    return(PtrDiffBytesS32(str, str_in));
#endif
}

extern BOOL
merstr(
    COL tcol,
    ROW trow,
    S32 send_replace,
    S32 add_refs)
{
    BOOL res;

    slot_in_buffer = FALSE;
    res = TRUE;

    if(buffer_altered)
        {
        P_SLOT newslot, oldslot;
        char justifyflag;
        U8 slot_refs = 0;
        /* note that SLRs can expand hugely on compilation */
        char array[COMPILED_TEXT_BUFSIZ];
        P_U8 source_text;
        S32 source_len;

        oldslot = travel(tcol, trow);

        if(send_replace)
            slot_to_be_replaced(tcol, trow, oldslot);
        else if(oldslot)
            slot_free_resources(oldslot);

        if(oldslot  &&  (oldslot->type == SL_TEXT))
            justifyflag = oldslot->justify;
        else
            justifyflag = J_FREE;

        { /* SKS 20130402 - most slots have no text-at chars, saves copying data yet again */
        BOOL contains_text_at_char;
        source_text = linbuf;
        source_len = buffer_length_detecting_text_at_char(source_text, &contains_text_at_char);
        if(contains_text_at_char)
            {
            source_len = compile_text_slot(array, linbuf, &slot_refs);
            source_text = array;
            }
        } /*block*/

        if(source_len == 1)
            {
            buffer_altered = FALSE;

            if((res = createhole(tcol, trow)) == FALSE)
                {
                *linbuf = '\0';
                reperr_null(status_nomem());
                }
            }
        else
            {
            if((newslot = createslot(tcol, trow, source_len, SL_TEXT)) == NULL)
                {
                /* if user is typing in, delete the line, so we don't get caught.
                 * But if loading a file, don't delete the line because rebuffer
                 * may free some memory and merstr will get called again
                */
                if(!in_load)
                    *linbuf = '\0';

                res = reperr_null(status_nomem());
                }
            else
                {
                newslot->justify = justifyflag;
                newslot->flags   = slot_refs;
                memcpy32(newslot->content.text, source_text, source_len);

                if(add_refs && draw_tree_str_insertslot(tcol, trow, TRUE) < 0)
                    {
                    reperr_null(status_nomem());
                    res = FALSE;
                    }
                }

            if(res)
                buffer_altered = FALSE;
            }

        if(send_replace)
            slot_changed(tcol, trow);
        }

    return(res);
}

/******************************************************************************
*
* initialise evaluator
*
******************************************************************************/

extern BOOL
pdeval_initialise(void)
{
    EV_RANGE rng;
    S32 handle_out;

    rng.s.docno = rng.e.docno = (EV_DOCNO) current_docno();
    rng.s.flags = rng.e.flags = SLR_ALL_REF;
    rng.s.col = 0;
    rng.s.row = 0;
    rng.e.col = EV_MAX_COL;
    rng.e.row = EV_MAX_ROW;

    if(ev_add_extdependency((U32) current_docno(),
                            0,
                            &handle_out,
                            eval_wants_slot_drawn,
                            &rng) < 0)
        return(FALSE);

    return(TRUE);
}

/*
decompile slot, dealing with compiled slot references, text-at fields etc
decompile from ptr (text field in slot) to linbuf

compiled slot references are stored as SLR leadin bytes, DOCNO docno, COL colno, ROW rowno.
Note this may contain NULLCH as part of compiled slot reference, but not at end of string.
*/

extern void
prccon(
    P_U8 target,
    P_SLOT ptr)
{
    P_U8 to = target;
    PC_U8 from;
    int c;

    if((NULL == ptr)  ||  (ptr->type == SL_PAGE))
        {
        *to = '\0';
        return;
        }

    if(ptr->type == SL_NUMBER)
        {
        const EV_DOCNO cur_docno = (EV_DOCNO) current_docno();
        EV_OPTBLOCK optblock;

        ev_set_options(&optblock, cur_docno);

        ev_decode_slot(cur_docno, to, &ptr->content.number.guts, &optblock);

        return;
        }

    from = ptr->content.text;

    if(!(ptr->flags & SL_TREFS))
        {
        strcpy(to, from);
        return;
        }

    /* this copes with compiled slot references in text slots.
     * On compilation, text-at chars are left in the slot so they can
     * be ignored on decompilation
    */
    do  {
        if(SLRLD1 == (c = (*to++ = *from++)))
            {
            const uchar * csr = from + 1; /* CSR is past the SLRLD1/2 */
            EV_DOCNO docno;
            COL tcol;
            ROW trow;

            from = talps_csr(csr, &docno, &tcol, &trow);
            /*eportf("prccon: decompiled CSR docno %d col 0x%x row 0x%x", docno, tcol, trow);*/

            to -= 1; /* we want to overwrite the SLRLD1 that we copied */

            to += write_ref(to, BUF_MAX_REFERENCE, docno, tcol, trow);
            }
        }
    while(NULLCH != c);
}

/******************************************************************************
*
* read a text-at field in the form <t-a-c>l:f,x,y<t-a-c> where:
*   l is the identifier,
*   f is a filename,
*   x and y are optional x and y parameters
*
* NB NULLCH != get_text_at_char()
*
******************************************************************************/

extern S32
readfxy(
    S32 id,
    PC_U8 *pfrom,
    P_U8 *pto,
    PC_U8Z *name,
    _OutRef_ P_F64 xp,
    _OutRef_ P_F64 yp)
{
    const uchar text_at_char = get_text_at_char();
    PC_U8 from;
    P_U8 to;
    S32 namelen, scanned;

    trace_1(TRACE_APP_PD4, "readfxy from: %s", *pfrom);

    from = *pfrom;

    /* check for a draw file */
    if((toupper(*from) == id)  &&  (*(from + 1) == ':'))
        {
        /* load to pointer and copy l: */
        to = *pto;
        *to++ = *from++;
        *to++ = *from++;

        while(*from == ' ')
            ++from;

        *name = to;
        namelen = scanned = 0;
        *xp = *yp = -1;

        /* copy across the filename */
        while(*from             &&
              (*from != ' ')    &&
              (*from != text_at_char)  &&
              (*from != ',')    )
            {
            *to++ = *from++;
            ++namelen;
            }

        /* must have a useful name */
        if(!namelen)
            return(0);

        while(*from++ == ' ')
            ;

        /* scan following x and y parameters */
        if(*--from == ',')
            {
            consume(int, sscanf(from, ", %lg %n", xp, &scanned));
            trace_2(TRACE_APP_PD4, "readfxy scanned: %d, from: %s", scanned, from);
            while(scanned--)
                *to++ = *from++;

            scanned = 0;
            consume(int, sscanf(from, ", %lg %n", yp, &scanned));
            trace_2(TRACE_APP_PD4, "readfxy scanned: %d, from: %s", scanned, from);
            while(scanned--)
                *to++ = *from++;
            }

        /* there must be a following text-at-char */
        if(*from != text_at_char)
            {
            trace_0(TRACE_APP_PD4, "readfxy found no trailing text-at-char");
            return(0);
            }

        /* copy second parameter to first if no second */
        if((*xp != -1)  &&  (*yp == -1))
            *yp = *xp;

        /* save new pointers and return name length */
        *pfrom = from;
        *pto = to;

        #if TRACE_ALLOWED
        {
        char namebuf[BUF_MAX_PATHSTRING];
        xstrnkpy(namebuf, elemof32(namebuf), *name, namelen);
        trace_3(TRACE_APP_PD4, "readfxy name: %s, xp: %g, yp: %g", namebuf, *xp, *yp);
        }
        #endif

        return(namelen);
        }
    else
        return(0);
}

/******************************************************************************
*
* determine whether row is selected
* during row selection
*
******************************************************************************/

extern S32
row_select(
    P_U8 rpn_in,
    ROW row)
{
    char cond_rpn[EV_MAX_OUT_LEN];
    EV_DATA result;
    S32 res;
    EV_SLR slr;

    set_ev_slr(&slr, 0, row);

    ev_proc_conditional_rpn(cond_rpn, rpn_in, &slr, FALSE, TRUE);

    if((res = ev_eval_rpn(&result, &slr, cond_rpn)) >= 0)
        {
        res = 0;
        switch(result.did_num)
            {
            case RPN_DAT_REAL:
                if(result.arg.fp != 0)
                   res = 1;
                break;
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
                if(result.arg.integer != 0)
                    res = 1;
                break;
            case RPN_DAT_ERROR:
                res = result.arg.ev_error.status;
                break;
            }
        }

    return(res);
}

/******************************************************************************
*
* make an evaluator slr for the current document, given col and row
*
******************************************************************************/

extern void
set_ev_slr(
    _OutRef_    P_EV_SLR slrp,
    _InVal_     COL col,
    _InVal_     ROW row)
{
    slrp->docno = (EV_DOCNO) current_docno();
    slrp->col   = (EV_COL) col;
    slrp->row   = (EV_ROW) row;
    slrp->flags = 0;
}

/******************************************************************************
*
* set editing expression
*
******************************************************************************/

extern void
seteex_nodraw(void)
{
    edtslr_col = curcol;
    edtslr_row = currow;        /* save current position */

    slot_in_buffer = xf_inexpression = TRUE;
}

extern void
seteex(void)
{
    output_buffer = TRUE;

    xf_blockcursor = TRUE;

    seteex_nodraw();
}

/******************************************************************************
*
* say whether a given slot displays
* its contents which may overlap
*
******************************************************************************/

extern S32
slot_displays_contents(
    P_SLOT tslot)
{
    switch(tslot->type)
        {
        case SL_TEXT:
            return(TRUE);

        case SL_NUMBER:
            return(ev_doc_is_custom_sheet(current_docno()) &&
                   ev_is_formula(&tslot->content.number.guts));

        default:
            return(FALSE);
        }
}

/*
is there a slot ref here?
must be sequence of letters followed by digits followed by at least one text-at char
and (rule number 6) NO spaces
*/

static BOOL
slot_ref_here(
    PC_U8 ptr)
{
    BOOL row_zero = TRUE;
    int ch;

    /* MRJC added this to cope with external references */
    if(*ptr == '[')
        {
        ++ptr;
        while(isalnum(*ptr))
            ++ptr;
        if(*ptr++ != ']')
            return(FALSE);
        }

    if(*ptr == '$')
        ptr++;

    if(!isalpha((int) *ptr))
        return(FALSE);

    do  {
        ch = *++ptr;
        }
    while(isalpha(ch));

    if(ch == '$')
        ch = *++ptr;

    if(!isdigit(ch))
        return(FALSE);

    do  {
        if(ch)
            row_zero = FALSE;
        ch = *++ptr;
        }
    while(isdigit(ch));

    return(!row_zero  &&  is_text_at_char(ch));
}

/*
splat takes a U32 and writes it as size (>0) bytes at to, LSB first.
used for converting compiled slot references to stream of bytes in slot
*/

static inline void
splat(
    _Out_writes_bytes_(size) P_U8 to,
    _InVal_     U32 num,
    _InVal_     U32 size)
{
    *to++ = (U8) num;

    if(size >= 2)
        *to++ = (U8) (num >>  8);

    if(size >= 3)
        *to++ = (U8) (num >> 16);

    if(size >= 4)
        *to++ = (U8) (num >> 24);
}

extern uchar *
splat_csr(
    uchar * csr,
    _InVal_     EV_DOCNO docno,
    _InVal_     COL col,
    _InVal_     ROW row)
{
    splat(csr,                                  (U32) docno, sizeof(EV_DOCNO));
    splat(csr + sizeof(EV_DOCNO),               (U32) col,   sizeof(COL));
    splat(csr + sizeof(EV_DOCNO) + sizeof(COL), (U32) row,   sizeof(ROW));

    return(csr + sizeof(EV_DOCNO) + sizeof(COL) + sizeof(ROW));
}

extern void
Snapshot_fn(void)
{
    /* fudge lead,trail chars and bracket bits */

/*  char minus   = d_options_MB;*/
/*  char leadch  = d_options_LP;*/
/*  char trailch = d_options_TP;*/

/*  d_options_MB = 'M';*/
/*  d_options_LP = '\0';*/
/*  d_options_TP = '\0';*/

    bash_slots_about_a_bit(bash_snapshot);

    /* restore brackets bit, leading trailing chars */

/*  d_options_MB = minus;*/
/*  d_options_LP = leadch;*/
/*  d_options_TP = trailch;*/
}

/*
talps reads size (>0) bytes from from, LSB first,
generates a U32 which it returns.
used for converting stream of bytes to slot reference
*/

static inline U32
talps(
    _In_reads_bytes_(size) PC_U8 from,
    _InVal_     U32 size)
{
    U32 res = (U32) *from++;

    if(size >= 2)
        res |= ((U32) *from++) <<  8;

    if(size >= 3)
        res |= ((U32) *from++) << 16;

    if(size >= 4)
        res |= ((U32) *from++) << 24;

    return(res);
}

extern const uchar *
talps_csr(
    const uchar * csr,
    _OutRef_    P_EV_DOCNO p_docno,
    _OutRef_    P_COL p_col,
    _OutRef_    P_ROW p_row)
{
    *p_docno = (EV_DOCNO) talps(csr,  sizeof(EV_DOCNO));
    *p_col   = (COL)      talps(csr + sizeof(EV_DOCNO),  sizeof(COL));
    *p_row   = (ROW)      talps(csr + sizeof(EV_DOCNO) + sizeof(COL), sizeof(ROW));

    return(csr + sizeof(EV_DOCNO) + sizeof(COL) + sizeof(ROW));
}

/******************************************************************************
*
* add references to the tree for a text slot
*
******************************************************************************/

static S32
text_slot_add_dependency(
    _InVal_     COL col,
    _InVal_     ROW row)
{
    uchar * csr;
    P_SLOT sl;
    struct EV_GRUB_STATE grubb;

    if((sl = travel(col, row)) == NULL)
        return(0);

    if(!(sl->flags & SL_TREFS))
        return(0);

    set_ev_slr(&grubb.slr, col, row);

    /* add dependency for each compiled slot reference found */
    csr = sl->content.text;

    while(NULL != (csr = find_next_csr(csr)))
        {
        COL tcol;
        ROW trow;
        S32 e_off;

        grubb.byoffset = csr - sl->content.text;

        grubb.data.did_num = RPN_DAT_SLR;

        csr = (uchar *) talps_csr(csr, &grubb.data.arg.slr.docno, &tcol, &trow);

        /* calculate current offset for reload below */
        e_off = csr - sl->content.text;

        if(!bad_reference(tcol, trow))
            {
            grubb.data.arg.slr.col = (EV_COL) (tcol & COLNOBITS);
            grubb.data.arg.slr.row = (EV_ROW) (trow & ROWNOBITS);
            /*eportf("adding SLR dependency byoffset %d for CSR docno %d col 0x%x row 0x%x", grubb.byoffset, grubb.data.arg.slr.docno, grubb.data.arg.slr.col, grubb.data.arg.slr.row);*/
            if(ev_add_slrdependency(&grubb) < 0)
                return(-1);
            }
        else
            {/*EMPTY*/
            /*eportf("NOT adding SLR dependency byoffset %d for CSR docno %d col 0x%x row 0x%x", grubb.byoffset, grubb.data.arg.slr.docno, tcol, trow);*/
            }

        /* reload after adding dependency */
        sl = travel(col, row);
        __assume(sl);
        csr = sl->content.text + e_off;
        }

    return(0);
}

/*
expand column into buffer returning length
*/

extern S32
write_col(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
                COL col)
{
    return(xtos_ustr_buf(buffer, elemof_buffer,
                         col & COLNOBITS /* ignore bad and absolute */,
                         1 /*uppercase*/));
}

/******************************************************************************
*
* write a slot reference to ptr, returning number of characters written
*
******************************************************************************/

extern S32
write_ref(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO docno,
    COL tcol,
    ROW trow)
{
    EV_SLR slr;

    slr.docno = docno;
    slr.col   = (EV_COL) (tcol & COLNOBITS) /* ignore bad and absolute */;
    slr.row   = (EV_ROW) (trow & ROWNOBITS) /* ignore bad and absolute */;
    slr.flags = 0;

    if(ABSCOLBIT & tcol)
        slr.flags |= SLR_ABS_COL;

    if(ABSROWBIT & trow)
        slr.flags |= SLR_ABS_ROW;

    if(bad_reference(tcol, trow))
        slr.flags |= SLR_BAD_REF;

    return(write_slr_ref(buffer, elemof_buffer, &slr));
}

/******************************************************************************
*
* slightly more exotic write_ref which also
* can cope with external and absolute refs
*
******************************************************************************/

extern S32
write_slr_ref(
    _Out_writes_z_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_EV_SLR slrp)
{
    const EV_DOCNO cur_docno = (EV_DOCNO) current_docno();
    S32 count = 0;

    if(slrp->docno != cur_docno)
        count = ev_write_docname(buffer, slrp->docno, cur_docno);

    if(slrp->flags & SLR_BAD_REF)
        buffer[count++] = '%';

    if(slrp->flags & SLR_ABS_COL)
        buffer[count++] = '$';

    count += write_col(buffer + count, elemof_buffer - count, (COL) slrp->col);

    if(slrp->flags & SLR_ABS_ROW)
        buffer[count++] = '$';

    count += xsnprintf(buffer + count, elemof_buffer - count, "%d", (int) slrp->row + 1);

    return(count);
}

/* end of numbers.c */
