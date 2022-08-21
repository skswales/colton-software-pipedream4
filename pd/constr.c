/* constr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that creates the PipeDream data structure
 * Also handles things that fiddle with the structure in non-standard ways
 */

/* RJM August 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"
#include "pd_x.h"
#include "ridialog.h"

/* structures and parameters for sort */

struct SORT_FIELD
{
    COL column;
    BOOL reverse;
};

/* structure of entry in key list */

typedef struct SORT_ENTRY
{
    ROW keyrow;
    ROW rowsinrec;
}
SORT_ENTRY, * P_SORT_ENTRY;

/* static data for sort */

static S32 fields;
static struct SORT_FIELD sort_fields[SORT_FIELD_DEPTH];

/******************************************************************************
*
* insert partial row of cells at currow
*
******************************************************************************/

static BOOL
insertrow(
    COL scol,
    COL ecol)
{
    COL tcol;
    ROW trow;

    if(!mergebuf())
        return(FALSE);

    trow = currow;

    for(tcol = scol; tcol < ecol; tcol++)
        if(!insertslotat(tcol, trow))
        {
            /* remove those we'd added */
            while(--tcol >= scol)
                killslot(tcol, trow);

            return(FALSE);
        }

    mark_to_end(currowoffset);
    out_rebuildvert = TRUE;

    /* rows in inserted columns move down */
    updref(scol, currow, ecol - 1, LARGEST_ROW_POSSIBLE, (COL) 0, (ROW) 1, UREF_UREF, DOCNO_NONE);

    /* callers of insertrow must do their own sendings */

    return(TRUE);
}

/******************************************************************************
*
* insert row in column
*
******************************************************************************/

extern void
InsertRowInColumn_fn(void)
{
    (void) insertrow(curcol, curcol+1);
}

/******************************************************************************
*
* insert row
*
******************************************************************************/

extern void
InsertRow_fn(void)
{
    (void) insertrow(0, numcol);
}

/******************************************************************************
*
* insert page break
*
******************************************************************************/

static BOOL
insertpagebreak_fn_core(void)
{
    P_CELL tcell = NULL;

    if(insertrow(0, numcol))
        tcell = createslot(0, currow, 1, SL_PAGE);

    if(NULL == tcell)
        return(reperr_null(status_nomem()));

    tcell->content.page.condval = (S32) d_insert_page_break[0].option;

    out_screen = out_rebuildvert = TRUE;
    filealtered(TRUE);

    return(TRUE);
}

static BOOL
insertpagebreak_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    xf_flush = TRUE;

    return(dialog_box_start());
}

extern void
InsertPageBreak_fn(void)
{
    if(!insertpagebreak_fn_prepare())
        return;

    while(dialog_box(D_INSERT_PAGE_BREAK))
    {
        if(!insertpagebreak_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* insert page number
*
******************************************************************************/

static BOOL
insertpagenumber_fn_core(void)
{
    PC_U8Z format = d_insert_page_number[0].textfield;

    if(NULL != format)
    if(CH_NULL != get_text_at_char())
    {
        if( insert_string(d_options_TA, FALSE)  &&
            insert_string("P:", FALSE)          &&
            insert_string(format, FALSE)        &&
            insert_string(d_options_TA, FALSE)  )
        {
            draw_screen();
        }
    }

    return(TRUE);
}

static STATUS
insert_D_N_P_fn_create_list(
    const P_P_LIST_BLOCK list,
    const P_PC_U8Z * strings) /* NULL terminated list of pointers to strings */
{
    LIST_ITEMNO key = 0;
    U32 idx = 0;

    if(NULL != *list)
        return(STATUS_OK);

    while(NULL != strings[idx])
        status_return(add_to_list(list, key++, *(strings[idx++])));

    return(STATUS_OK);
}

static const P_PC_U8Z
page_number_format_strings[] =
{
    &PN_FMT1_STR,
    &PN_FMT2_STR,
    &PN_FMT3_STR,
    NULL
};

static STATUS
insertpagenumber_fn_create_list(void)
{
    return(insert_D_N_P_fn_create_list(&page_number_formats_list, page_number_format_strings));
}

static BOOL
insertpagenumber_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(CH_NULL != get_text_at_char());

    /* enumerate formats */
    if(status_fail(insertpagenumber_fn_create_list()))
        return(FALSE);

    return(dialog_box_start());
}

extern void
InsertPageNumber_fn(void)
{
    if(!insertpagenumber_fn_prepare())
        return;

    while(dialog_box(D_INSERT_PAGE_NUMBER))
    {
        if(!insertpagenumber_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* insert date
*
******************************************************************************/

static BOOL
insertdate_fn_core(void)
{
    PC_U8Z format = d_insert_date[0].textfield;

    if(NULL != format)
    if(CH_NULL != get_text_at_char())
    {
        if( insert_string(d_options_TA, FALSE)  &&
            insert_string("D:", FALSE)          &&
            insert_string(format, FALSE)        &&
            insert_string(d_options_TA, FALSE)  )
        { /*EMPTY*/ }
    }

    return(TRUE);
}

static const P_PC_U8Z
date_format_strings[] =
{
    &DATE_FMT01_STR,
    &DATE_FMT02_STR,
    &DATE_FMT03_STR,
    &DATE_FMT04_STR,
    &DATE_FMT05_STR,
    &DATE_FMT06_STR,
    &DATE_FMT07_STR,
    &DATE_FMT08_STR,
    &DATE_FMT09_STR,
    &DATE_FMT10_STR,
    &DATE_FMT11_STR,
    &DATE_FMT12_STR,
    &DATE_FMT13_STR,
    &DATE_FMT14_STR,
    &DATE_FMT15_STR,
    &DATE_FMT16_STR,
    &DATE_FMT17_STR,
    &DATE_FMT18_STR,
    &DATE_FMT19_STR,
    NULL
};

static STATUS
insertdate_fn_create_list(void)
{
    return(insert_D_N_P_fn_create_list(&date_formats_list, date_format_strings));
}

static BOOL
insertdate_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(CH_NULL != get_text_at_char());

    /* enumerate formats */
    if(status_fail(insertdate_fn_create_list()))
        return(FALSE);

    return(dialog_box_start());
}

extern void
InsertDate_fn(void)
{
    if(!insertdate_fn_prepare())
        return;

    while(dialog_box(D_INSERT_DATE))
    {
        if(!insertdate_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* insert time
*
******************************************************************************/

static BOOL
inserttime_fn_core(void)
{
    PC_U8Z format = d_insert_time[0].textfield;

    if(NULL != format)
    if(CH_NULL != get_text_at_char())
    {
        if( insert_string(d_options_TA, FALSE)  &&
            insert_string("N:", FALSE)          &&
            insert_string(format, FALSE)        &&
            insert_string(d_options_TA, FALSE)  )
        { /*EMPTY*/ }
    }

    return(TRUE);
}

static const P_PC_U8Z
time_format_strings[] =
{
    &TIME_FMT1_STR,
    &TIME_FMT2_STR,
    &TIME_FMT3_STR,
    NULL
};

static STATUS
inserttime_fn_create_list(void)
{
    return(insert_D_N_P_fn_create_list(&time_formats_list, time_format_strings));
}

static BOOL
inserttime_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(CH_NULL != get_text_at_char());

    /* enumerate formats */
    if(status_fail(inserttime_fn_create_list()))
        return(FALSE);

    return(dialog_box_start());
}

extern void
InsertTime_fn(void)
{
    if(!inserttime_fn_prepare())
        return;

    while(dialog_box(D_INSERT_TIME))
    {
        if(!inserttime_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* insert colour change
*
******************************************************************************/

extern void
InsertColourChange_fn(void)
{
    U32 wimp_colour_value;

    if(CH_NULL == get_text_at_char())
        return;

    if(!riscdialog_choose_colour(&wimp_colour_value, "title"))
        return;

    if(CH_NULL != get_text_at_char())
    {
        char buffer[32];

        if(0 != (0x10 & wimp_colour_value))
        {
            const U32 red   = (wimp_colour_value >>  8) & 0xFF;
            const U32 green = (wimp_colour_value >> 16) & 0xFF;
            const U32 blue  = (wimp_colour_value >> 24) & 0xFF;

            consume_int(snprintf(buffer, elemof32(buffer), "%u,%u,%u", red, green, blue));
        }
        else
        {
            consume_int(snprintf(buffer, elemof32(buffer), "%d", (int) (wimp_colour_value & 0x0F)));
        }

        if( insert_string(d_options_TA, FALSE)  &&
            insert_string("C:", FALSE)          &&
            insert_string(buffer, FALSE)        &&
            insert_string(d_options_TA, FALSE)  )
        { /* EMPTY */ }
    }
}

/******************************************************************************
*
* check if we have a sheet, if not create one
*
******************************************************************************/

extern BOOL
check_not_blank_sheet(void)
{
    if(!numrow)
    {
        if(!createslot((COL) 0, (ROW) 0, 1, SL_TEXT))
            return(FALSE);

        filealtered(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* destroy this document's data
*
******************************************************************************/

extern void
constr_finalise(void)
{
    trace_0(TRACE_APP_PD4, "constr_finalise()");

    delcol(0, numcol);

    killcoltab();                   /* delete the column structure */
}

/******************************************************************************
*
* initialise data for this document
*
******************************************************************************/

extern BOOL
constr_initialise(void)
{
    trace_0(TRACE_APP_PD4, "constr_initialise()");

    reset_filpnm();

    killcoltab();

    riscos_settype(&currentfileinfo, FILETYPE_PIPEDREAM);
    riscos_readtime(&currentfileinfo);

    rowsonscreen = colsonscreen = 0;
    n_rowfixes = n_colfixes = 0;

    curcol = 0;
    currow = 0;

    numcol = colsintable = 0;
    reset_numrow();

    pagnum = curpnm = 1;
    pagoff = 1;

    lecpos = lescrl = 0;
    linbuf[0] = CH_NULL;

    /* create new option list from def_option_list */
    update_variables();

    /* get new colstart and copy from def_colstart */
    /* note that the first time round this does not create any structure */
    if(!restcoltab())
        return(FALSE);

    return(dftcol());
}

/******************************************************************************
*
* initialise data for this program: load init file and remember defaults
*
******************************************************************************/

extern void
constr_initialise_once(void)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    LOAD_FILE_OPTIONS load_file_options;
    BOOL ok;

    trace_0(TRACE_APP_PD4, "constr_initialise_once()");

    { /* initialise Config object */
    const P_NUMFORM_CONTEXT p_numform_context = p_pd_config->p_numform_context;

    p_numform_context->month_names[0]  = month_name_01_STR;
    p_numform_context->month_names[1]  = month_name_02_STR;
    p_numform_context->month_names[2]  = month_name_03_STR;
    p_numform_context->month_names[3]  = month_name_04_STR;
    p_numform_context->month_names[4]  = month_name_05_STR;
    p_numform_context->month_names[5]  = month_name_06_STR;
    p_numform_context->month_names[6]  = month_name_07_STR;
    p_numform_context->month_names[7]  = month_name_08_STR;
    p_numform_context->month_names[8]  = month_name_09_STR;
    p_numform_context->month_names[9]  = month_name_10_STR;
    p_numform_context->month_names[10] = month_name_11_STR;
    p_numform_context->month_names[11] = month_name_12_STR;

    p_numform_context->day_names[0] = day_name_01_STR;
    p_numform_context->day_names[1] = day_name_02_STR;
    p_numform_context->day_names[2] = day_name_03_STR;
    p_numform_context->day_names[3] = day_name_04_STR;
    p_numform_context->day_names[4] = day_name_05_STR;
    p_numform_context->day_names[5] = day_name_06_STR;
    p_numform_context->day_names[6] = day_name_07_STR;

    {
    static char endings_buffer[256];
    P_USTR ustr_arg_string = endings_buffer;
    S32 day;

    /* take a copy of this string - we decompose it later on here and form pointers into it */
    xstrkpy(endings_buffer, sizeof(endings_buffer), day_endings_STR);

    for(day = 0; day < elemof32(p_numform_context->day_endings); ++day)
    {
        P_USTR ustr;

        /* arg is string of space-separated strings */
        if(NULL != (ustr = strchr(ustr_arg_string, CH_SPACE)))
        {
            PtrPutByte(ustr, CH_NULL); ustr_IncByte_wr(ustr);
        }

        p_numform_context->day_endings[day] = ustr_arg_string;

        ustr_arg_string = ustr;

        /* last space-separated arg reached? */
        if(NULL == ustr_arg_string)
            break;
    }
    } /*block*/

    } /*block*/

    /* load Choices file iff present */
    if(status_done(file_find_on_path(buffer, elemof32(buffer), file_get_search_path(), CHOICES_FILE_STR)))
    {
        zero_struct(load_file_options);
        load_file_options.document_name = buffer;
        load_file_options.filetype_option = PD4_CHAR;

        ok = loadfile_recurse(buffer, &load_file_options);

        /* save away default options if loaded ok, then kill it */
        if(ok)
        {
            savecoltab();

            save_options_to_list();

            destroy_current_document();
        }
    }

    /* load driver */
    load_driver(&d_driver[D_DRIVER_NAME].textfield, FALSE);

    /* load Preferred file iff present */
    if(status_done(file_find_on_path(buffer, elemof32(buffer), file_get_search_path(), PREFERRED_FILE_STR)))
        pdchart_load_preferred(buffer);

    /* some tickable things may be updated or not yet initialised */
    check_state_changed();
    recalc_state_may_have_changed();
    chart_recalc_state_may_have_changed();
    insert_overtype_state_may_have_changed();
    menu_state_changed();
}

/******************************************************************************
*
* reset the page numbers at the start of this file
*
******************************************************************************/

extern void
reset_filpnm(void)
{
    filpnm = curpnm = pagnum = filpof = pagoff = 1;

    if(!str_isblank(d_poptions_PS))
        filpnm = curpnm = pagnum = atoi(d_poptions_PS);
}

/******************************************************************************
*
* set up default cols, a-f, wrapwidth descending from 72
*
******************************************************************************/

extern BOOL
dftcol(void)
{
    if(!numcol)
    {
        if(!createcol(5))
            return(FALSE);

        dstwrp(0, 72);

        filealtered(FALSE);
    }

    return(TRUE /*check_not_blank_sheet()*/);
}

/******************************************************************************
*
* new
*
******************************************************************************/

extern void
NewWindow_fn(void)
{
    if(create_new_untitled_document())
    {
        /* bring to front immediately if in execfile, else sometime later */
        riscos_front_document_window(in_execfile);
        /* draw_screen(); --- no need to do explicitly here */
        xf_acquirecaret = TRUE;
    }
}

/******************************************************************************
*
* work out whether this is a new record
*
******************************************************************************/

static S32
isnewrec(
    P_CELL sl)
{
    /* test multi-row sort option */
    if(d_sort[SORT_MULTI_ROW].option != 'Y')
        return(TRUE);

    if(!sl  ||  is_blank_cell(sl))
        return(FALSE);

    #if 0
    /* SKS removed 21.3.90 'cos RJM complained   --   I should think so! RJM, 14.9.91 */
    if((sl->type == SL_TEXT)  &&  (sl->content.text[0] == SPACE))
        return(FALSE);
    #endif

    return(TRUE);
}

/******************************************************************************
*
* get next compiled cell reference in either text or number cell
*
******************************************************************************/

extern uchar *
find_next_csr(
    uchar * csr)
{
    while(CH_NULL != *csr)
        if(SLRLD1 == *csr++)
            return(csr + 1); /* past the SLRLD1/2 */

    return(NULL);
}

/******************************************************************************
*
* compare two rows when sorting
*
******************************************************************************/

#include <setjmp.h>

static jmp_buf sortpoint;

PROC_QSORT_PROTO(static, rowcomp, SORT_ENTRY)
{
    EV_SLR slr1, slr2;
    S32 res;
    COL col;

    /* NB current_p_docu global register furtling IS required as caller (library) doesn't know about our little scheme... */
    current_p_docu_global_register_stash_block_start();
    current_p_docu_global_register_restore_from_backup();

    col = 0;
    slr1.docno = slr2.docno = (EV_DOCNO) current_docno();

    do  {
        SS_DATA data1, data2;

        /* make long sorts escapeable - at this point no data has been exchanged */
        if(ctrlflag)
            longjmp(sortpoint, 1);

        slr1.col   = slr2.col = (EV_COL) sort_fields[col].column;
        slr1.row   = (EV_ROW) ((P_SORT_ENTRY) _arg1)->keyrow;
        slr2.row   = (EV_ROW) ((P_SORT_ENTRY) _arg2)->keyrow;
        slr1.flags = slr2.flags = 0;

        ev_slr_deref(&data1, &slr1, FALSE);
        ev_slr_deref(&data2, &slr2, FALSE);

        res = ss_data_compare(&data1, &data2);

        ss_data_free_resources(&data1);
        ss_data_free_resources(&data2);

        /* if equal at this column, loop */
        if(res)
            break;
    }
    while(++col < fields);

    if(res)
        if(sort_fields[col].reverse)
            res = (res > 0) ? -1 : 1;

    current_p_docu_global_register_stash_block_end(); /* restore the register we just trashed for caller (library) */

    return(res);
}

/******************************************************************************
*
* Fast sort
* MRJC May 1989
* This code uses a new algorithm to build up a separate list
* of the keys to be sorted, keeping 'records' together, and
* then sorting the list of keys, followed by swapping the
* rows in the main spreadsheet to correspond with the sorted
* list. 'Records' start with a non-blank entry in a column -
* the key, and include all following blank rows upto the
* next non-blank entry
*
* sort a block of cells by a specified or the current column
* update cell references to cells in block by default
*
******************************************************************************/

static int
sortblock_fn_core(void)
{
    int res = TRUE;
    ROW i, n, row;
    S32 nrecs, rec, nrows;
    COL pkeycol;
    SC_ARRAY_INIT_BLOCK sortblk_init_block = aib_init(1, sizeof32(SORT_ENTRY), FALSE);
    ARRAY_HANDLE sortblkh = 0;
    SC_ARRAY_INIT_BLOCK sortrowblk_init_block = aib_init(1, sizeof32(ROW), FALSE);
    ARRAY_HANDLE sortrowblkh = 0;
    P_SORT_ENTRY sortblkp, sortp;
    P_ROW sortrowblkp, rowtp;
    STATUS status;

    /* read the fields in - note that variable fields ends up as number read */
    fields = 0;

    do  {
        buff_sofar = d_sort[fields * 2 + SORT_FIELD_COLUMN].textfield;
        sort_fields[fields].column = getcol();
        if(sort_fields[fields].column == NO_COL)
            break;
        sort_fields[fields].reverse = (d_sort[fields * 2 + SORT_FIELD_ASCENDING].option == 'N');
    }
    while(++fields < SORT_FIELD_DEPTH);

    if(0 == fields)
    {
        consume_bool(reperr_null(ERR_BAD_COL));
        return(2); /*continue*/
    }

    /* wait till all is recalced */
    ev_recalc_all();

#if 0 /* RJM removes updaterefs option on 14.9.91 */
    updaterefs = (d_sort[SORT_UPDATE_REFS].option == 'Y');
#endif

  /*n = blkend.row - blkstart.row + 1; never used */

    /* do the actual sort */

    /* we did a merge above */
    slot_in_buffer = FALSE;

    /* load column number of primary key */
    pkeycol = sort_fields[0].column;

    /* count the number of records to be sorted */
    for(i = blkstart.row + 1, nrecs = 1; i <= blkend.row; ++i)
        if(isnewrec(travel(pkeycol, i)))
            ++nrecs;

    /* allocate array to be sorted */
    if(NULL == al_array_alloc(&sortblkh, SORT_ENTRY, nrecs, &sortblk_init_block, &status))
    {
        dialog_box_end();
        res = reperr_null(status);
        goto endpoint;
    }

    trace_1(TRACE_MODULE_UREF, "allocated sort block, %d entries", nrecs);

    /* allocate row table */
    if(NULL == al_array_alloc(&sortrowblkh, ROW, ((S32) blkend.row - blkstart.row + 1), &sortrowblk_init_block, &status))
    {
        dialog_box_end();
        res = reperr_null(status);
        goto endpoint;
    }

    trace_1(TRACE_MODULE_UREF, "allocated sort row block, %d entries", ((S32) blkend.row - blkstart.row + 1));

    /* switch on indicator */
    actind(0);

    escape_enable();

    /* load pointer to array */
    sortblkp = array_base(&sortblkh, SORT_ENTRY);

    /* load array with records */
    for(i = blkstart.row, sortp = sortblkp; i <= blkend.row; ++i, ++sortp)
    {
        trace_2(TRACE_APP_PD4, "SortBlock: adding " PTR_XTFMT ", row %d", report_ptr_cast(sortp), i);
        sortp->keyrow = i;

        while(++i <= blkend.row)
            if(isnewrec(travel(pkeycol, i)))
                break;

        sortp->rowsinrec = (i--) - sortp->keyrow;
    }

    trace_2(TRACE_MODULE_UREF, "SortBlock: sorting array " PTR_XTFMT", %d elements", report_ptr_cast(sortblkp), nrecs);

    if(setjmp(sortpoint))
    {   /* NB current_p_docu global register furtling IS required as
         * return here via longjmp() *may* have corrupted the register
         */
        current_p_docu_global_register_restore_from_backup();
        goto endpoint;
    }
    else
    {   /* sort the array */
        qsort(sortblkp, nrecs, sizeof32(SORT_ENTRY), rowcomp);
    }

    /* build table of rows to be sorted */
    sortrowblkp = array_base(&sortrowblkh, ROW);

    /* for each record */
    for(sortp = sortblkp, rec = 0, rowtp = sortrowblkp;
        rec < nrecs;
        ++rec, ++sortp)
    {
        /* for each row in the record */
        for(row = sortp->keyrow, n = sortp->rowsinrec;
            n;
            --n, ++row, ++rowtp)
        {
            *rowtp = row;
        }
    }

    /* free sort array */
    al_array_dispose(&sortblkh);

    trace_0(TRACE_MODULE_UREF, "sort array freed; exchange starting");

    /* exchange the rows in the spreadsheet */
    /* NB. difference between pointers still valid after dealloc */
    nrows = rowtp - sortrowblkp;

    for(n = 0; n < nrows; ++n)
    {
        if(ctrlflag)
            goto endpoint;

        actind((S32) ((100 * n) / nrows));

        /* check that we have reasonable memory free before a row exchange to stop the exchange
         * falling over in the middle of a row - it will be happy, but the user probably won't!
        */
        if(status_fail(alloc_ensure_froth(0x1000)))
        {
            dialog_box_end();
            res = reperr_null(status_nomem());
            goto endpoint;
        }

        /* must re-load row block pointer each time, ahem */
        assert(array_index_is_valid(&sortrowblkh, n));
        rowtp = array_ptr(&sortrowblkh, ROW, n);
        row = n + blkstart.row;
        if(*rowtp != row)
        {   /* do physical swap */
            if(!swap_rows(row,
                          *rowtp,
                          blkstart.col,
                          blkend.col,
#if 0
                          updaterefs
#else
                          TRUE /* always update refs */
#endif
                        ))
            {
                res = reperr_null(status_nomem());
                goto endpoint;
            }
            else
            {   /* update list of unsorted rows */
                ROW nn;
                P_ROW nrowtp;

                /* must re-load row block pointer each time, ahem */
                assert(array_index_is_valid(&sortrowblkh, n));
                rowtp = array_ptr(&sortrowblkh, ROW, n);

                for(nn = n + 1, nrowtp = rowtp + 1;
                    nn < nrows;
                    ++nn, ++nrowtp)
                {
                    if(*nrowtp == row)
                    {
                        *nrowtp = *rowtp;
                        break;
                    }
                }
            }
        }
    }

endpoint:

    /* free sort array */
    al_array_dispose(&sortblkh);

    /* free row table array */
    al_array_dispose(&sortrowblkh);

    /* release any redundant storage */
    garbagecollect();

    if(escape_disable())
        dialog_box_end();

    actind_end();

    out_screen = TRUE;
    filealtered(TRUE);

    return(res);
}

static BOOL
sortblock_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_SORT));

    /* check two markers set in this document & more than one row in block */
    if(!MARKER_DEFINED())
        return(reperr_null((blkstart.col != NO_COL)
                            ? create_error(ERR_NOBLOCKINDOC)
                            : create_error(ERR_NOBLOCK)));

    if(blkend.row == blkstart.row)
        return(reperr_null(ERR_NOBLOCK));

    /* first, get the user's requirements */

    /* if this column is in the block, give it to the dialog box */
    if((curcol >= blkstart.col)  &&  (curcol <= blkend.col))
    {
        char array[32];

        (void) write_col(array, elemof32(array), curcol);

        false_return(mystr_set(&d_sort[SORT_FIELD_COLUMN].textfield, array));
    }

    return(dialog_box_start());
}

extern void
SortBlock_fn(void)
{
    if(!sortblock_fn_prepare())
        return;

    while(dialog_box(D_SORT))
    {
        int core_res = sortblock_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

extern BOOL
dependent_files_warning(void)
{
    EV_DOCNO docno_array[DOCNO_MAX];
    EV_DOCNO cur_docno;
    P_EV_DOCNO p_docno;
    S32 n_docs, i;
    S32 nDepDocs = 0;

    n_docs = ev_doc_get_dep_docs_for_sheet(current_docno(),
                                           &cur_docno,
                                           docno_array);

    for(i = 0, p_docno = docno_array; i < n_docs; ++i, ++p_docno)
    {
        if(*p_docno == cur_docno)
            continue;

        ++nDepDocs;
    }

    nDepDocs += pdchart_dependent_documents(cur_docno);

    if(0 == nDepDocs)
        return(TRUE);

    return(riscdialog_query_YES ==
           riscdialog_query_YN(close_dependent_files_YN_S_STR,
                               close_dependent_files_YN_Q_STR));
}

extern void
close_window(void)
{
    const BOOL main_window_had_caret = (main_window_handle == caret_window_handle);
    P_DOCU p_docu_next;

    p_docu_next = next_document(current_document());

    destroy_current_document();

    /* if was off end, try first - won't be the deleted one! */
    if(NO_DOCUMENT == p_docu_next)
        p_docu_next = first_document();

    if(main_window_had_caret  &&  (NO_DOCUMENT != p_docu_next))
    {
        select_document(p_docu_next);
        /* xf_front_document_window = xf_acquirecaret = TRUE; */ /* SKS 31aug2012 - avoid setting focus in other window; avoid unpinning documents */
    }
}

/******************************************************************************
*
* close window
*
******************************************************************************/

extern void
CloseWindow_fn(void)
{
    BOOL want_to_close;

    filbuf();

    /* see rear_event_Close_Window_Request */
    want_to_close = riscdialog_warning();

   if(want_to_close)
        want_to_close = save_or_discard_existing();

   if(want_to_close)
        want_to_close = dependent_files_warning();

   if(want_to_close)
        want_to_close = dependent_charts_warning();

   if(want_to_close)
        want_to_close = dependent_links_warning();

    if(want_to_close)
        close_window();
}

/******************************************************************************
*
*  next window
*
******************************************************************************/

extern void
NextWindow_fn(void)
{
    P_DOCU p_docu_next = next_document(current_document());

    /* end of list, start again */
    if(NO_DOCUMENT == p_docu_next)
        p_docu_next = first_document();

    select_document(p_docu_next);

    /* front even if current - may not be at front (and will unpin if needed) */
    xf_front_document_window = TRUE;
}

_Check_return_
extern BOOL
mystr_set(
    _InoutRef_  P_P_USTR aa,
    _In_opt_z_  PC_USTR b)
{
    STATUS status = str_set(aa, b);

    return(status_fail(status) ? reperr_null(status) : TRUE);
}

_Check_return_
extern BOOL
mystr_set_n(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b,
    _InVal_     U32 n)
{
    STATUS status = str_set_n(aa, b, n);

    return(status_fail(status) ? reperr_null(status) : TRUE);
}

/* end of constr.c */
