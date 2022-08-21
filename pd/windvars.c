/* windvars.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Variables pertaining to each PipeDream document (window) */

/* RJM 8-Apr-1989 */

#if defined(TRACE_DOC)
#define TRACE_ALLOWED 1
#endif /* TRACE_DOC */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"

#define DONT_CARE 0

/* exported array of document pointers for fast lookup */

P_DOCU docu_array[256]; /* NB > DOCNO_MAX */
DOCNO docu_array_size;

/* the template data to copy into every new document */

static DOCU initial_docu_data =
{
    NO_DOCUMENT,                /* P_DOCU link */

    DOCNO_NONE,                 /* DOCNO docno */

    NULL,
    /* PTSTR currentfilename */

    { DONT_CARE, DONT_CARE, DONT_CARE },
    /* riscos_fileinfo currentfileinfo */

    FALSE,
    /* BOOLEAN xf_loaded_from_path */

    FALSE,
    /* BOOLEAN xf_file_is_driver */

    FALSE,
    /* BOOLEAN xf_file_is_help */

    FALSE,
    /* BOOLEAN xf_filealtered */

    PD4_CHAR,
    /* char current_filetype_option */

    0,
    /* U8 current_line_sep_option */

/* ---------------------------- slot.c ----------------------------------- */

    NULL,
    /* P_COLENTRY colstart */

    0,
    /* COL colsintable */

    0,
    /* COL numcol */

    0,
    /* ROW numrow */

/* -------------------------- constr.c ----------------------------------- */

    "",
    /* uchar linbuf[LIN_BUFSIZ] */

    0,
    /* S32 lecpos */

    0,
    /* COL curcol */

    0,
    /* ROW currow */

/* ------------------------- markers.c ----------------------------------- */

    FALSE,
    /* BOOLEAN start_block */

    { DONT_CARE, DONT_CARE },
    /* SLR in_block */

    { DONT_CARE, DONT_CARE },
    /* SLR start_bl */

    { DONT_CARE, DONT_CARE },
    /* SLR end_bl */

/* --------------------------- mcdiff.c ---------------------------------- */

    0,
    /* coord paghyt */

    0,
    /* coord pagwid */

    1,
    /* coord pagwid_plus1 */

/* ---------------------------- scdraw.c --------------------------------- */

    0,
    /* ARRAY_HANDLE horzvec_mh */

    0,
    /* coord maxncol */

    0,
    /* ARRAY_HANDLE vertvec_mh */

    0,
    /* coord maxnrow */

    FALSE,
    /* BOOLEAN out_screen */

    FALSE,
    /* BOOLEAN out_rebuildhorz */

    FALSE,
    /* BOOLEAN out_rebuildvert */

    FALSE,
    /* BOOLEAN out_forcevertcentre */

    FALSE,
    /* BOOLEAN out_below */

    FALSE,
    /* BOOLEAN out_rowout */

    FALSE,
    /* BOOLEAN out_rowout1 */

    FALSE,
    /* BOOLEAN out_rowborout */

    FALSE,
    /* BOOLEAN out_rowborout1 */

    FALSE,
    /* BOOLEAN out_currslot */

    FALSE,
    /* BOOLEAN xf_inexpression */

    FALSE,
    /* BOOLEAN xf_inexpression_box  */

    FALSE,
    /* BOOLEAN xf_inexpression_line */

    FALSE,
    /* BOOLEAN xf_blockcursor */

    TRUE,
    /* BOOLEAN xf_draweverything */

    FALSE,
    /* BOOLEAN xf_front_document_window */

    FALSE,
    /* BOOLEAN xf_drawcolumnheadings */

    FALSE,
    /* BOOLEAN xf_drawsome */

    FALSE,
    /* BOOLEAN xf_drawcellcoordinates */

    FALSE,
    /* BOOLEAN xf_acquirecaret */

    FALSE,
    /* BOOLEAN xf_caretreposition */

    FALSE,
    /* BOOLEAN xf_interrupted */

    FALSE,
    /* BOOLEAN xf_draw_empty_right */

    FALSE,
    /* BOOLEAN unused_bit_at_bottom */

    0,
    /* coord rowout */

    0,
    /* coord rowout1 */

    0,
    /* coord rowborout */

    0,
    /* coord rowborout1 */

    0,
    /* coord maximum_cols */

    0,
    /* coord cols_available */

    0,
    /* coord colsonscreen */

    0,
    /* coord n_colfixes */

    0,
    /* coord maximum_rows */

    0,
    /* coord rows_available */

    0,
    /* coord rowsonscreen */

    0,
    /* coord n_rowfixes */

    0,
    /* coord curcoloffset */

    0,
    /* coord currowoffset */

    0,
    /* COL oldcol */

    0,
    /* COL newcol */

    0,
    /* ROW oldrow */

    0,
    /* ROW newrow */

    0,
    /* S32 movement */

    0,
    /* S32 lescrl */

    0,
    /* S32 old_lescroll */

    0,
    /* S32 old_lecpos */

    0,
    /* coord last_thing_drawn */

    BORDERWIDTH,
    /* coord borderwidth */

    0,
    /* S32 curpnm */

    0,
    /* gcoord curr_scroll_x */

    0,
    /* gcoord curr_scroll_y */

/* ----------------------------- cursmov.c ------------------------------- */

    0,
    /* coord scrbrc */

    0,
    /* coord rowtoend */

/* ----------------------------- numbers.c ------------------------------- */

    DONT_CARE,
    /* COL edtslr_col */

    DONT_CARE,
    /* ROW edtslr_row */

    FALSE,
    /* BOOLEAN slot_in_buffer */

    FALSE,
    /* BOOLEAN output_buffer */

    FALSE,
    /* BOOLEAN buffer_altered */

/* ---------------------------- doprint.c -------------------------------- */

    TRUE,
    /* BOOLEAN displaying_borders */

    DONT_CARE,
    /* S32 real_pagelength */

    DONT_CARE,
    /* S32 real_pagcnt */

    0,
    /* S32 encpln */

    0,
    /* S32 enclns */

    0,
    /* S32 pagoff */

    0,
    /* S32 pagnum */

    0,
    /* S32 filpof */

    0,
    /* S32 filpnm */

/* --------------------------- riscos.c ---------------------------------- */

    HOST_WND_NONE,
    /* HOST_WND rear_window_handle */

    NULL,
    /* void * rear_window_template (should be WimpWindowWithBitset *) */

    HOST_WND_NONE,
    /* HOST_WND main_window_handle */

    NULL,
    /* void * main_window_template (should be WimpWindowWithBitset *) */

    { 0, 0, 0, 0 },
    /* GDI_BOX open_box */

    HOST_WND_NONE,
    /* HOST_WND colh_window_handle */

    NULL,
    /* void * colh_window_template (should be WimpWindowWithBitset *) */

    NULL,
    /* FORMULA_WINDOW_HANDLE editexpression_formwind */

    0,
    /* coord lastcursorpos_x */

    0,
    /* coord lastcursorpos_y */

    FALSE,
    /* BOOLEAN grid_on */

    DONT_CARE,
    /* S32 charvspace */

    DONT_CARE,
    /* S32 charvrubout_pos */

    DONT_CARE,
    /* S32 textcell_xorg */

    DONT_CARE,
    /* S32 textcell_yorg */

    FALSE,
    /* BOOL recalc_forced */

    0,
    /* S32 pict_on_screen */

    0,
    /* S32 pict_was_on_screen_prev */


    "",
    /* char slotcoordinates[slotcoordinates_size] */

    FALSE,
    /* BOOL riscos_fonts */

    0,
    /* S32 riscos_font_error */

    NULL,
    /* char * global_font */

    12 * 16,
    /* S32 global_font_x (1/16th of a point) */

    12 * 16,
    /* S32 global_font_y (1/16th of a point) */

    14400,
    /* GR_MILLIPOINT global_font_leading_millipoints */

    0,
    /* S32 hbar_length */

    0,
    /* ROW pict_currow */

    0,
    /* ROW pict_on_currow */

    -1,
    /* S32 dspfld_from */

    FALSE,
    /* BOOL quitseen */

    0,
    /* S32 curr_extent_x */

    0,
    /* S32 curr_extent_y */

    0,
    /* S32 delta_scroll_x */

    0,
    /* S32 delta_scroll_y */

    DONT_CARE,
    /* S32 charvrubout_neg */

    DONT_CARE,
    /* S32 vdu5textoffset */

    DONT_CARE,
    /* S32 fontbaselineoffset */

    12000,
    /* GR_MILLIPOINT fontscreenheightlimit_mp */

    32,
    /* S32 charallocheight */

    FALSE,
    /* BOOL colh_mark_state_indicator */

    TRUE,
    /* auto_line_height */

    0,
    /* formline_stashedcaret */

#if defined(EXTENDED_COLOUR_WINDVARS)

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* U32 Xd_colours_CF */

    DONT_CARE,
    /* U32 Xd_colours_CB */

    DONT_CARE,
    /* U32 Xd_colours_CP */

    DONT_CARE,
    /* U32 Xd_colours_CL */

    DONT_CARE,
    /* U32 Xd_colours_CG */

    DONT_CARE,
    /* U32 Xd_colours_CC */

    DONT_CARE,
    /* U32 Xd_colours_CE */

    DONT_CARE,
    /* U32 Xd_colours_CH */

    DONT_CARE,
    /* U32 Xd_colours_CS */

    DONT_CARE,
    /* U32 Xd_colours_CU */

    DONT_CARE,
    /* U32 Xd_colours_CT */

    DONT_CARE,
    /* U32 Xd_colours_CV */

    DONT_CARE,
    /* U32 Xd_colours_CX */

#endif /* EXTENDED_COLOUR_WINDVARS */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar * d_options_DE */

    DONT_CARE,
    /* uchar d_options_TN */

    DONT_CARE,
    /* uchar d_options_IW */

    DONT_CARE,
    /* uchar d_options_BO */

    DONT_CARE,
    /* uchar d_options_JU */

    DONT_CARE,
    /* uchar d_options_WR */

    DONT_CARE,
    /* uchar d_options_DP */

    DONT_CARE,
    /* uchar d_options_MB */

    DONT_CARE,
    /* uchar d_options_TH */

    DONT_CARE,
    /* uchar d_options_IR */

    DONT_CARE,
    /* uchar d_options_DF */

    DONT_CARE,
    /* uchar * d_options_LP */

    DONT_CARE,
    /* uchar * d_options_TP */

    DONT_CARE,
    /* uchar * d_options_TA */

    DONT_CARE,
    /* uchar d_options_GR */

    DONT_CARE,
    /* uchar d_options_KE */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_poptions_PL */

    DONT_CARE,
    /* uchar d_poptions_LS */

    DONT_CARE,
    /* uchar * d_poptions_PS */

    DONT_CARE,
    /* uchar d_poptions_TM */

    DONT_CARE,
    /* uchar d_poptions_HM */

    DONT_CARE,
    /* uchar d_poptions_FM */

    DONT_CARE,
    /* uchar d_poptions_BM */

    DONT_CARE,
    /* uchar d_poptions_LM */

    DONT_CARE,
    /* uchar * d_poptions_HE */

    DONT_CARE,
    /* uchar * d_poptions_FO */

    80,
    /* uchar d_poptions_PX */

    DONT_CARE,
    /* uchar * d_poptions_H1 */

    DONT_CARE,
    /* uchar * d_poptions_H2 */

    DONT_CARE,
    /* uchar * d_poptions_F1 */

    DONT_CARE,
    /* uchar * d_poptions_F2 */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_recalc_AM */

    DONT_CARE,
    /* uchar d_recalc_RC */

    DONT_CARE,
    /* uchar d_recalc_RI */

    DONT_CARE,
    /* uchar * d_recalc_RN */

    DONT_CARE,
    /* uchar * d_recalc_RB */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_chart_options_CA */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_driver_PT */

    DONT_CARE,
    /* uchar * d_driver_PD */

    DONT_CARE,
    /* uchar * d_driver_PN */

    DONT_CARE,
    /* uchar d_driver_PB */

    DONT_CARE,
    /* uchar d_driver_PW */

    DONT_CARE,
    /* uchar d_driver_PP */

    DONT_CARE,
    /* uchar d_driver_PO */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_print_QP */

    DONT_CARE,
    /* uchar * d_print_QF */

    DONT_CARE,
    /* uchar d_print_QD */

    DONT_CARE,
    /* uchar d_print_QE */

    DONT_CARE,
    /* uchar d_print_QO */

    DONT_CARE,
    /* uchar d_print_QB */

    DONT_CARE,
    /* uchar d_print_QT */

    DONT_CARE,
    /* uchar d_print_QM */

    DONT_CARE,
    /* short d_print_QN */

    DONT_CARE,
    /* uchar d_print_QW */

    DONT_CARE,
    /* uchar d_print_QL */

    DONT_CARE,
    /* short d_print_QS */

/* ----------------------------------------------------------------------- */

    DONT_CARE,
    /* uchar d_mspace_MY */

    DONT_CARE,
    /* uchar * d_mspace_MZ */

/* ----------------------------------------------------------------------- */

    TRAVERSE_BLOCK_INIT,
    /* TRAVERSE_BLOCK tr_block */

    { 0 },
    /* TCHARZ window_title[] */

    NULL,                       /* the_last_thing */

/* ----------------------------------------------------------------------- */

    SS_INSTANCE_DATA_INIT       /* SS_INSTANCE_DATA ss_instance_data */
};
/* end of initial_docu_data */

/******************************************************************************
*
*  initialiser called only on program startup
*  to set up the initial template data
*
******************************************************************************/

extern void
docu_array_init_once(void)
{
    DOCNO docno;

    docu_array[DOCNO_NONE] = NO_DOCUMENT;

    for(docno = DOCNO_FIRST; docno < DOCNO_MAX; docno++)
        docu_array[docno] = NO_DOCUMENT; /* allocatable hole */

    docu_array[DOCNO_MAX] = NO_DOCUMENT;

    select_document_none();
}

extern void
docu_init_once(void)
{
    /* don't use select_document() as this may check against document list */
    current_p_docu_global_register_assign(&initial_docu_data);

    dialog_initialise_once();       /* set default options correctly */

    /* ensure we don't mess the initial window data up accidentally */
    select_document_none();
}

_Check_return_
extern DOCNO
change_document_using_docno(
    _InVal_     DOCNO new_docno)
{
    DOCNO old_docno = current_docno();

    (void) select_document_using_docno(new_docno);

    return(old_docno);
}

/******************************************************************************
*
* called to convert a docu thunk to a fully initialised document
*
******************************************************************************/

extern BOOL
convert_docu_thunk_to_document(
    _InVal_     DOCNO docno)
{
    DOCNO old_docno = current_docno();
    P_DOCU p_docu = docu_array[docno];
    BOOL ok = FALSE;
    S32 res;

    reportf("convert_docu_thunk_to_document(docno=%u)", docno);

    if(!docu_is_thunk(p_docu))
    {
        assert(docu_is_thunk(p_docu));
        return(TRUE);
    }

    p_docu->ss_instance_data.ss_doc.is_docu_thunk = FALSE;

    /* add to head of document list (new ones appear at top of menus etc.) */
    p_docu->link = document_list;
    document_list = p_docu;

    current_p_docu_global_register_assign(p_docu); /* select it as current */

    for(;;) /* loop for structure */
    {
        /* call various people to initialise now */

        { /* obtain the desired full document name */
        U8 buffer[BUF_MAX_PATHSTRING];
        name_make_wholename(&current_p_docu->ss_instance_data.ss_doc.docu_name, buffer, elemof32(buffer));
        if(!mystr_set(&current_p_docu->Xcurrentfilename, buffer))
            break;
        } /*block*/

        if(!pdeval_initialise())
            break;

        if(!dialog_initialise())
            break;

        if(!constr_initialise())
            break;

        if(!riscos_initialise()) /* does screen_initialise() itself */
            break;

        ok = TRUE;
        break;
    }

    if(ok)
    {
        ++nDocuments;
        res = 1;
    }
    else
    {
        /* undo our work on this one on failure */
        destroy_current_document();
        res = status_nomem();
    }

    if(res < 0)
        reperr(res, _unable_to_create_new_document_STR);

    (void) select_document_using_docno(old_docno);

    reportf("convert_docu_thunk_to_document(docno=%u) returns %s", docno, report_boolstring(ok));

    return(ok);
}

/******************************************************************************
*
* called from evaluator to establish a docu thunk
*
******************************************************************************/

extern DOCNO
create_new_docu_thunk(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    DOCNO old_docno = current_docno();
    DOCNO new_docno;
    BOOL ok = FALSE;
    S32 res;
    P_DOCU p_docu;

    reportf("create_new_docu_thunk(path(%s) leaf(%s) %s)",
            report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied));

    /* allocate a docno for this docu thunk */
    for(new_docno = 1; new_docno < DOCNO_MAX; new_docno++)
        if(NO_DOCUMENT == docu_array[new_docno])
            break;

    if(new_docno == DOCNO_MAX)
    {
        reperr_null(ERR_TOOMANYDOCS);
        return(DOCNO_NONE);
    }

    if(NULL == (p_docu = al_ptr_alloc_elem(DOCU, 1, &res)))
    {
        trace_0(TRACE_APP_PD4, "Unable to claim space for new document data");
    }
    else
    {
        if(new_docno >= docu_array_size)
            docu_array_size = new_docno + 1;

        *p_docu = initial_docu_data; /* initialise using data from template */

        p_docu->docno = new_docno;

        p_docu->ss_instance_data.ss_doc.is_docu_thunk = TRUE;

        docu_array[new_docno] = p_docu; /* note in array */

        ok = name_dup(&p_docu->ss_instance_data.ss_doc.docu_name, p_docu_name);
    }

    if(ok)
    {
        res = 1;
    }
    else
    {
        /* undo our work on this one on failure */
        destroy_docu_thunk(new_docno);
        new_docno = DOCNO_NONE;
        res = status_nomem();
    }

    if(res < 0)
        reperr(res, _unable_to_create_new_document_STR);

    (void) select_document_using_docno(old_docno);

    reportf("create_new_docu_thunk(path(%s) leaf(%s) %s) returns %s, docno=%u, docu_array_size %u",
            report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied), report_boolstring(ok), new_docno, docu_array_size);

    return(new_docno);
}

/******************************************************************************
*
* create a new, initialised document
*
* --out--
*   TRUE    ok
*   FALSE   failed, destroyed, error reported
*
******************************************************************************/

extern BOOL
create_new_document(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    EV_DOCNO docno;
    DOCNO new_docno;
    BOOL ok;

    reportf("create_new_document(path(%s) leaf(%s) %s)",
            report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied));

    /* does this match any existing document? if so, we must load into a new document (consider documents loaded from different elements on path that we may be trying to sort out) */
    if(DOCNO_NONE != (docno = docno_find_name(p_docu_name, DOCNO_NONE, TRUE)))
    {
        reportf("creating a new document to avoid overwriting an existing document");
        new_docno = create_new_docu_thunk(p_docu_name);
    }
    else
    {
        /* does this match any existing docu thunk? if so, fulfil that */
        if(DOCNO_NONE != (docno = docno_find_name(p_docu_name, DOCNO_NONE, FALSE)))
        {
            P_SS_DOC p_ss_doc;

            new_docno = (DOCNO) docno;

            /* but make sure that the existing docu thunk gets our new docu_name and flags! (consider loading from path... NB it's not a full rename) */
            p_ss_doc = ev_p_ss_doc_from_docno_must(docno);
            reportf("document creation satisfies an existing docu thunk, docno=%u path(%s) leaf(%s) %s",
                    new_docno,
                    report_tstr(p_ss_doc->docu_name.path_name), report_tstr(p_ss_doc->docu_name.leaf_name), report_boolstring(p_ss_doc->docu_name.flags.path_name_supplied));
            name_free(&p_ss_doc->docu_name);
            (void) name_dup(&p_ss_doc->docu_name, p_docu_name);
        }
        else
        {
            reportf("creating a new document as there are no matching documents or docu thunks");
            new_docno = create_new_docu_thunk(p_docu_name);
        }
    }

    ok = (DOCNO_NONE != new_docno);

    if(ok)
        ok = convert_docu_thunk_to_document(new_docno);

    if(ok)
        (void) select_document_using_docno(new_docno);

    reportf("create_new_document(path(%s) leaf(%s) %s) returns %s, docno=%u",
            report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied),
            report_boolstring(ok), new_docno);

    return(ok);
}

/******************************************************************************
*
* create a new, initialised, untitled document
*
* --out--
*   TRUE    ok
*   FALSE   failed, destroyed, error reported
*
******************************************************************************/

extern BOOL
create_new_untitled_document(void)
{
    BOOL ok;
    DOCU_NAME docu_name;

    name_init(&docu_name);

    if(!mystr_set(&docu_name.leaf_name, get_untitled_document()))
        return(FALSE);

    ok = create_new_document(&docu_name);

    name_free(&docu_name);

    return(ok);
}

/******************************************************************************
*
* destroy the given docu thunk
*
* --out--
*   resources freed
*
******************************************************************************/

extern void
destroy_docu_thunk(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    reportf("destroy_docu_thunk(docno=%u)", docno);

    if(NO_DOCUMENT != (p_docu = docu_array[docno]))
    {
        assert(docu_is_thunk(p_docu));
        assert(current_p_docu != p_docu);

        docu_array[docno] = NO_DOCUMENT;

        name_free(&p_docu->ss_instance_data.ss_doc.docu_name);

        al_ptr_free(p_docu);
    }

    if(0 != docu_array_size)
    {
        /* do a bit of pruning to keep loops fast */
        while(NO_DOCUMENT == docu_array[docu_array_size - 1])
        {
            --docu_array_size;

            if(1 == docu_array_size)
            {
                docu_array_size = 0;
                break;
            }
        }
    }

    reportf("destroy_docu_thunk(docno=%u): docu_array_size AFTER %u", docno, docu_array_size);
}

/******************************************************************************
*
* destroy the current document
*
* --out--
*   resources freed
*   no document selected
*
******************************************************************************/

extern void
destroy_current_document(void)
{
    DOCNO docno = current_docno();
    S32 i;
    P_DOCU pp;
    P_DOCU cp;

    reportf("destroy_current_document(docno=%u)", docno);

    myassert1x(docno != DOCNO_NONE, "destroy_current_document(%u)", docno);

    /* clear marked block */
    if(blk_docno == docno)
    {
        blk_docno = DOCNO_NONE;
        blkstart.col = blkend.col = NO_COL;
    }

    /* clear search block */
    if(sch_docno == docno)
        sch_docno = DOCNO_NONE;

    /* clear from saved position stack */
    for(i = 0; i < saved_index; i++)
        if(saved_pos[i].ref_docno == docno)
            saved_pos[i].ref_docno = DOCNO_NONE;

    /* call various people to finalise now */

    /* clear font selector only if this window */
    pdfontselect_finalise(FALSE);

    expedit_close_file(current_docno());

    screen_finalise();

    constr_finalise();

    dialog_finalise();

    riscos_finalise();

    str_clr(&current_p_docu->Xcurrentfilename);

    /* detach current document from document list */
    for(pp = NO_DOCUMENT, cp = document_list;
        NO_DOCUMENT != cp;
        pp = cp, cp = pp->link)
    {
        if(cp == current_p_docu)
        {
            if(document_list == cp)
                document_list = cp->link;
            else
                pp->link = cp->link;
            break;
        }
    }

    myassert0x(NO_DOCUMENT != cp, "Failed to find current document again for destroy");

    --nDocuments;

    /* this is now just a docu thunk */

    /* paranoically copy the core of the template data again (but not the SS_INSTANCE_DATA) */
    memcpy32(current_document(), &initial_docu_data, offsetof32(DOCU, ss_instance_data));

    /* just reinstating this one critical element in the core */
    current_document()->docno = docno;

    /* see if we can get rid of the docu thunk */
    current_document()->ss_instance_data.ss_doc.is_docu_thunk = TRUE;

    select_document_none();

    /* finally see if we can get rid of the docu thunk */
    ev_close_sheet(docno); /* will call destroy_docu_thunk() as needed */

    reportf("destroy_current_document(docno=%d) ended, nDocuments=%d", docno, nDocuments);
}

/******************************************************************************
*
* find out how many loaded documents are modified
*
******************************************************************************/

extern S32
documents_modified(void)
{
    P_DOCU p_docu;
    S32 count = 0;

    /* loop over documents */
    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        if(p_docu->Xxf_filealtered)
            count++;
    }

    return(count);
}

/******************************************************************************
*
* get an untitled document name
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern PCTSTR
get_untitled_document_with(
    _In_z_      PCTSTR leafname)
{
    static TCHARZ buffer[32];
    U32 name_len = tstrlen32(leafname);
    S32 i = 1;

    /* ensure we get a unique <Untitled>N that doesn't collide with
     * any that the user may have foolishly renamed / reloaded
    */
    for(;;)
    {
        DOCU_NAME docu_name;

        name_init(&docu_name);

        docu_name.leaf_name = buffer;

        tstr_xstrnkpy(buffer, elemof32(buffer), leafname, name_len);

        consume_int(xsnprintf(buffer + name_len, elemof32(buffer) - name_len, S32_FMT, i++));

        if(DOCNO_NONE == docno_find_name(&docu_name, DOCNO_NONE, FALSE))
            break;

        /* Otherwise it is already valid (loaded document or docu thunk), or DOCNO_SEVERAL. */
        /* In any case, loop and try the next one */
    }

    return(buffer);
}

_Check_return_
_Ret_notnull_
extern PCTSTR
get_untitled_document(void)
{
    return(get_untitled_document_with(UNTITLED_STR));
}

/******************************************************************************
*
*  search document list for document with given key = leafname of possibly wholename
*
******************************************************************************/

_Check_return_
extern DOCNO
find_document_using_leafname(
    _In_z_      PCTSTR wholename)
{
    P_DOCU p_docu;
    P_DOCU p_docu_found = NULL;
    S32 count = 0;
    PCTSTR leafname = file_leafname(wholename);

    trace_1(TRACE_APP_PD4, "find_document_using_leafname(%s)", wholename);

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        PC_DOCU_NAME test_docu_name = &p_docu->ss_instance_data.ss_doc.docu_name;

        if(0 == C_stricmp(test_docu_name->leaf_name, leafname))
        {
            p_docu_found = p_docu;
            count++;
        }
    }

    if(count > 1)
        return(DOCNO_SEVERAL);

    if(count == 1)
        return(p_docu_found->docno);

    return(DOCNO_NONE);
}

/******************************************************************************
*
* search document list for document with given wholename
*
******************************************************************************/

_Check_return_
extern DOCNO
find_document_using_wholename(
    _In_z_      PCTSTR wholename)
{
    P_DOCU p_docu;

    trace_1(TRACE_APP_PD4, "find_document_using_wholename(%s)", wholename);

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        if(0 == C_stricmp(p_docu->Xcurrentfilename, wholename))
            return(p_docu->docno);
    }

    return(DOCNO_NONE);
}

/******************************************************************************
*
* search document list for document with given key = window handle
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
find_document_using_window_handle(
    _HwndRef_   HOST_WND window_handle)
{
    P_DOCU p_docu;

    trace_1(TRACE_APP_PD4, "find_document_using_window_handle(%u) ", window_handle);

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        if(p_docu->Xmain_window_handle == window_handle)
            break;
    }

    trace_1(TRACE_APP_PD4, "yields &%p", p_docu);
    return(p_docu);
}

/******************************************************************************
*
* search document list for the document with the input focus
*
* --out--
* NO_DOCUMENT     no document found
* P_DOCU          document whose document window or edit control has the input focus
*
******************************************************************************/

_Check_return_
_Ret_notnone_
extern P_DOCU /* may be NO_DOCUMENT */
find_document_with_input_focus(void)
{
    WimpGetCaretPositionBlock caret;
    P_DOCU p_docu;

    trace_0(TRACE_APP_PD4, "find_document_with_input_focus ");

    if(NULL != WrapOsErrorReporting(tbl_wimp_get_caret_position(&caret)))
        return(NO_DOCUMENT);

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        if((p_docu->Xxf_inexpression_box) && expedit_owns_window(p_docu, caret.window_handle))
            break;

        if(p_docu->Xcolh_window_handle == caret.window_handle)
            break;

        if(p_docu->Xmain_window_handle == caret.window_handle)
            break;
    }

    trace_1(TRACE_APP_PD4, "yields " PTR_XTFMT, report_ptr_cast(p_docu));
    return(p_docu);
}

/******************************************************************************
*
* bring to the front the given document
*
******************************************************************************/

extern void
front_document_using_docno(
    _InVal_     DOCNO docno)
{
    if(docno < DOCNO_MAX)
    {
        DOCNO old_docno = change_document_using_docno(docno);

        xf_acquirecaret = TRUE;

        /* must do NOW as we're unlikely to draw_screen() this document */
        /* and as we're doing an immediate, we MUST preserve current doc */
        riscos_front_document_window(TRUE);

        (void) select_document_using_docno(old_docno);
    }
}

/******************************************************************************
*
* ensure all documents with modified buffers are updated
*
******************************************************************************/

extern BOOL
mergebuf_all(void)
{
    BOOL res = TRUE;
    DOCNO old_docno = current_docno();
    P_DOCU p_docu;

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        select_document(p_docu);
        res &= mergebuf_nocheck();      /* ensure buffer changes to main data */
        filbuf();
    }

    (void) select_document_using_docno(old_docno);

    return(res);            /* whether any mergebuf() failed */
}

/******************************************************************************
*
* make the given document current (from a callback-type handle)
*
******************************************************************************/

extern BOOL
select_document_using_callback_handle(
    void * handle)
{
    uintptr_t u_handle = (uintptr_t) handle;
    DOCNO docno;
    P_DOCU p_docu;

    if(u_handle >= (uintptr_t) DOCNO_MAX)
    {
        assert(u_handle < (uintptr_t) DOCNO_MAX);
        return(FALSE);
    }

    docno = (DOCNO) u_handle;

    p_docu = p_docu_from_docno(docno);

    if(NO_DOCUMENT == p_docu)
    {
        assert(NO_DOCUMENT != p_docu);
        return(FALSE);
    }

    if(docu_is_thunk(p_docu))
    {
        assert(!docu_is_thunk(p_docu));
        return(FALSE);
    }

    current_p_docu_global_register_assign(p_docu);
    return(TRUE);
}

/*ncr*/
extern BOOL
select_document_using_docno(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    if(DOCNO_NONE == docno)
        return(FALSE); /* no winge */

    p_docu = p_docu_from_docno(docno);

    if(NO_DOCUMENT == p_docu)
    {
        assert(NO_DOCUMENT != p_docu);
        return(FALSE);
    }

    if(docu_is_thunk(p_docu))
    {
        assert(!docu_is_thunk(p_docu));
        return(FALSE);
    }

    current_p_docu_global_register_assign(p_docu);
    return(TRUE);
}

#if defined(TRACE_DOC)  /* macroed otherwise */

/******************************************************************************
*
*  export the current document number
*
******************************************************************************/

_Check_return_
extern DOCNO
current_docno(void)
{
    DOCNO docno = (current_p_docu != NO_DOCUMENT) ? current_p_docu->docno: DOCNO_NONE;

    trace_1(TRACE_APP_PD4, TEXT("current_docno() yields docno=%u"), docno);
    return(docno);
}

/******************************************************************************
*
*  export the current document address
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
current_document(void)
{
    trace_1(TRACE_APP_PD4, TEXT("current_document() yields p_docu=") PTR_XTFMT, report_ptr_cast(current_p_docu));
    return(current_p_docu);
}

/******************************************************************************
*
* return the first document address
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
first_document(void)
{
    P_DOCU p_docu = document_list;

    trace_1(TRACE_APP_PD4, TEXT("first_document() yields p_docu=") PTR_XTFMT, report_ptr_cast(p_docu));
    return(p_docu);
}

/******************************************************************************
*
*  is a document selected
*
******************************************************************************/

_Check_return_
extern BOOL
is_current_document(void)
{
    BOOL res = (current_p_docu != NO_DOCUMENT);

    trace_1(TRACE_APP_PD4, TEXT("is_current_document() yields %s"), report_boolstring(res));
    return(res);
}

/******************************************************************************
*
* return the next document address
*
* relies on documents not being created / deleted between calls
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
next_document(
    _InRef_     P_DOCU p_docu_cur)
{
    P_DOCU p_docu = p_docu_cur->link;

    trace_2(TRACE_APP_PD4, TEXT("next_document(p_docu=") PTR_XTFMT TEXT(") yields p_docu=") PTR_XTFMT, report_ptr_cast(p_docu_cur), report_ptr_cast(p_docu));
    return(p_docu);
}

/******************************************************************************
*
* search document list for document with given key = document number
*
* --out--
*   NO_DOCUMENT     no document found with this document number
*   P_DOCU          otherwise
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_DOCU /* may be NO_DOCUMENT */
p_docu_from_docno(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    if(docno < DOCNO_MAX)
    {
        p_docu = docu_array[docno];
    }
    else
    {
        assert(docno < DOCNO_MAX);
        p_docu = NO_DOCUMENT;
    }

    trace_2(TRACE_APP_PD4, TEXT("p_docu_from_docno(docno=%u) yields p_docu=") PTR_XTFMT, docno, report_ptr_cast(p_docu));
    return(p_docu);
}

/******************************************************************************
*
* make the given document current
*
******************************************************************************/

extern void
select_document(
    _InRef_     P_DOCU p_docu_new)
{
    trace_1(TRACE_APP_PD4, TEXT("selecting document p_docu=") PTR_XTFMT, report_ptr_cast(p_docu_new));

    if(NO_DOCUMENT != p_docu_new)
    {
        BOOL found = FALSE;
        P_DOCU p_docu;

        assert(!docu_is_thunk(p_docu_new));

        for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
        {
            if(p_docu == p_docu_new)
            {
                found = TRUE;
                break;
            }
        }

        assert(found); UNREFERENCED_LOCAL_VARIABLE(found);
    }

    current_p_docu_global_register_assign(p_docu_new);
}

extern void
select_document_none(void)
{
    trace_0(TRACE_APP_PD4, TEXT("selecting document NONE"));

    current_p_docu_global_register_assign(NO_DOCUMENT);
}

#endif /* TRACE_DOC */

/* end of windvars.c */
