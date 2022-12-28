/* savload.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that saves and loads PipeDream (and other format) files */

/* RJM Oct 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

#ifndef __cs_xfersend_h
#include "cs-xfersend.h"
#endif

#include "cmodules/vsload.h"

#include "riscos_x.h"
#include "pd_x.h"
#include "version_x.h"
#include "ridialog.h"

#include "viewio.h"

#include "dtpsave.h"

#ifndef __cs_xferrecv_h
#include "cs-xferrecv.h"
#endif

/*
internal functions
*/

static BOOL collup(
    COL tcol,
    FILE_HANDLE output);

static BOOL
loadfile_core(
    _In_z_      PCTSTR filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options);

static BOOL
name_preprocess_docu_name_flags_for_rename(
    _InoutRef_  P_DOCU_NAME p_docu_name);

static void
rename_document(
    _InRef_     PC_DOCU_NAME p_docu_name);

static BOOL
savefile_core(
    _In_z_      PCTSTR filename,
    _InRef_     P_SAVE_FILE_OPTIONS p_save_file_options);

static BOOL
pd_save_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    BOOL saving_part_file);

static void
viewsheet_load_core(
    _In_        S32 vsrows,
    _InVal_     BOOL inserting,
    _InoutRef_  P_ROW p_insert_numrow,
    _InoutRef_  P_BOOL p_breakout);

/* ----------------------------------------------------------------------- */

enum PD_CONSTRUCT_OFFSETS
{
    C_FREE      = 0,    /* must be same as J_FREE */
    C_LEFT      = 1,    /* must be same as J_LEFT */
    C_CENTRE    = 2,    /* must be same as J_CENTRE */
    C_RIGHT     = 3,    /* must be same as J_RIGHT */
    C_JLEFT     = 4,    /* must be same as J_JLEFT */
    C_JRIGHT    = 5,    /* must be same as J_JRIGHT */
    C_LCR       = 6,    /* must be same as J_LCR */
    C_PERCENT   = 7,
    C_COL       = 8,
    C_HIGHL     = 9,
    C_DEC       = 10,
    C_BRAC      = 11,
    C_LEADCH    = 12,
    C_TRAILCH   = 13,
    C_VALUE     = 14,
    C_OPT       = 15,
    C_PAGE      = 16,
    C_LF        = 17
};

static const struct CONTAB { U32 len; const char * str; } contab[] =
{
    { 2, "F%"   },  /* free align */
    { 2, "L%"   },  /* left align */
    { 2, "C%"   },  /* centre align */
    { 2, "R%"   },  /* right align */
    { 3, "JL%"  },  /* justify left */
    { 3, "JR%"  },  /* justify right */
    { 4, "LCR%" },  /* lcr align */
    { 3, "PC%"  },  /* '%' character */
    { 3, "CO:"  },  /* column construct */
    { 1, "H"    },  /* highlight n */
    { 1, "D"    },  /* decimal places n */
    { 2, "B%"   },  /* brackets */
    { 3, "LC%"  },  /* leading char */
    { 3, "TC%"  },  /* trailing char */
    { 2, "V%"   },  /* number */
    { 3, "OP%"  },  /* option */
    { 1, "P"    },  /* page eject n */
    { 3, "LF%"  }   /* linefeed in multi line formula */
};
#define C_ITEMS (sizeof32(contab) / (sizeof32(contab[0])))

static S32 start_of_construct = 0;     /* used in load, lukcon */
static S32 in_option = 0;              /* used in load, lukcon */

static BOOL
away_construct(
    _InVal_      U32 contab_idx,
    FILE_HANDLE output)
{
    if(!away_byte('%', output))
        return(FALSE);

    return(away_string_simple(contab[contab_idx].str, output));
}

/* this is also used by pdchart.c */

extern void
common_save_version_string(
    _Out_writes_(elemof_array) P_U8 array,
    _InVal_     U32 elemof_array)
{
#if 1
    const BOOL empty_organ = str_isblank(user_organ_id());

    (void) xsnprintf(
                array, elemof_array,
                "%s, %s%s%s, R%s",
                applicationversion,
                user_id(),
                (empty_organ ? "" : " - "),
                (empty_organ ? "" : user_organ_id()),
                registration_number());
#else
    xstrkpy(array, elemof_array, applicationversion);
    xstrkat(array, elemof_array, ", ");

    xstrkat(array, elemof_array, !str_isblank(user_id()) ? user_id() : "Colton Software");
    if(!str_isblank(user_organ_id()))
    {
        xstrkat(array, elemof_array, " - ");
        xstrkat(array, elemof_array, user_organ_id());
    }

    xstrkat(array, elemof_array, ", R");

    xstrkat(array, elemof_array, registration_number());
#endif
}

static void
save_version_string(
    FILE_HANDLE output)
{
    uchar array[LIN_BUFSIZ];
    char *tmp;

    common_save_version_string(array, elemof32(array));

    /* SKS 15nov91 - temporarily borrow the data */
    tmp = d_version[0].textfield;
    d_version[0].textfield = array;

    save_opt_to_file(output, d_version, 1);

    d_version[0].textfield = tmp;
}

/*
write the window position into the open_box dialog box ready for output
*/

static void
write_box_sizes(void)
{
    char array[LIN_BUFSIZ];

    (void) xsnprintf(array, elemof32(array),
            "%d,%d,%d,%d,%d,%d,%d,%d",
            open_box.x0, open_box.x1,
            open_box.y0, open_box.y1,
            curcol, currow, fstncx(), fstnrx());

    consume_bool(mystr_set(&d_open_box[0].textfield, array));
}

/******************************************************************************
*
* save options to the file
*
******************************************************************************/

static BOOL
save_options_to_file(
    FILE_HANDLE output,
    _InVal_      BOOL saving_choices_file)
{
    char buffer[BUF_MAX_PATHSTRING];
    char *tmp;
    PC_LIST lptr;

    update_all_dialog_from_windvars();

    save_version_string(output);

    save_opt_to_file(output, d_options,  dialog_head[D_OPTIONS].items);
    save_opt_to_file(output, d_poptions, dialog_head[D_POPTIONS].items);

    /* update_dialog_from_fontinfo();*/
    save_opt_to_file(output, d_fonts,    dialog_head[D_FONTS].items);

    /* these two added by RJM on 22.9.91 */
    save_opt_to_file(output, d_driver,   dialog_head[D_DRIVER].items);
    save_opt_to_file(output, d_print,    dialog_head[D_PRINT].items);

    /* this one added by RJM on 16.10.91 */
    save_opt_to_file(output, d_mspace,   dialog_head[D_MSPACE].items);

    /* SKS 15nov91 */
    save_opt_to_file(output, d_chart_options, 1);

    /* save_window_position */
    write_box_sizes();
    save_opt_to_file(output, d_open_box, 1);

    save_linked_columns(output);

    if(saving_choices_file)
    {
        /* save print options and colours - and menu changes - only to Choices file */

        save_opt_to_file(output, d_colours, dialog_head[D_COLOURS].items); /* all colours */
        save_opt_to_file(output, d_menu,    dialog_head[D_MENU].items);
        save_opt_to_file(output, d_progvars,dialog_head[D_PROGVARS].items);
        save_opt_to_file(output, d_save + SAV_LINESEP, 1);
        save_opt_to_file(output, d_deleted, 1);

        save_opt_to_file(output, d_auto, dialog_head[D_AUTO].items);

        /* get rid of error possibility from str_set */
        tmp = buffer;
        str_swap(&tmp, &d_user_open[0].textfield);

        /* each user dictionary has to be saved to file */
        for(lptr = first_in_list(&first_user_dict);
            lptr;
            lptr = next_in_list(&first_user_dict))
        {
            strcpy(buffer, lptr->value);
            save_opt_to_file(output, d_user_open, dialog_head[D_USER_OPEN].items);
        }

        str_swap(&tmp, &d_user_open[0].textfield);

        save_menu_changes(output);
    }
    else
    {
        /* save spreadsheet names - SKS after PD 4.11 08jan92 moved into non-Choices file section */
        save_names_to_file(output);

        /* save any row & column fixes */
        save_opt_to_file(output, d_fixes, dialog_head[D_FIXES].items);

        save_protected_bits(output);

#if defined(EXTENDED_COLOUR_WINDVARS)
        save_opt_to_file(output, d_colours, 8); /* just sheet colours, not borders */
#endif
    }

    update_all_windvars_from_dialog();

    return(TRUE);
}

/******************************************************************************
*
* save one option to the file
* does not save the option if it is the default
* however, if there is an initialization file and the
* option is set in there, the option will be saved
*
******************************************************************************/

extern void
save_opt_to_file(
    FILE_HANDLE output,
    DIALOG *start,
    S32 n)
{
    const DIALOG *dptr;
    char *ptr;
    const char *ptr1, *ptr2;
    char ch1, ch2;
    BOOL not_on_list;

    trace_2(TRACE_APP_PD4, "save_opt_to_file(output, &%p, %d)", report_ptr_cast(start), n);

    for(dptr = start; dptr < start + n; dptr++)
    {
        ch1 = dptr->ch1;
        ch2 = dptr->ch2;

        /* RJM adds on 22.9.91 cos print box may not have complete set of options */
        if((ch1 == CH_NULL) || (ch2 == CH_NULL))
            continue;

        ptr = linbuf + sprintf(linbuf, "%%" "OP" "%%" "%c%c", ch1, ch2);

        not_on_list = !search_list(&def_first_option, ((S32) ch1 << 8) + (S32) ch2);

        trace_3(TRACE_APP_PD4, "considering option %c%c, type %d", ch1, ch2, dptr->type);

        switch(dptr->type)
        {
        case F_SPECIAL:
            if(not_on_list  &&  (dptr->option == **dptr->optionlist))
                continue;

            ptr[0] = (char) dptr->option;
            ptr[1] = CH_NULL;
            break;

        case F_ARRAY:
            if(not_on_list  &&  (dptr->option == 0))
                continue;

            consume_int(sprintf(ptr, "%d", (int) dptr->option));
            break;

#if defined(EXTENDED_COLOUR)
        case F_COLOUR:
            {
            U32 wimp_colour_value = get_option_as_u32(&dptr->option);

            /* is it the default ? */
            if(not_on_list  &&  (wimp_colour_value == decode_wimp_colour_value(*dptr->optionlist)))
                continue;

            consume_int(encode_wimp_colour_value(ptr, LIN_BUFSIZ - 32, wimp_colour_value));

            break;
            }
#endif /* EXTENDED_COLOUR */

        case F_LIST:
        case F_NUMBER:
#if !defined(EXTENDED_COLOUR)
        case F_COLOUR:
#endif
            { /* is it the default ? */
            if(not_on_list  &&  (dptr->option == (optiontype) atoi(*dptr->optionlist)))
                continue;

            consume_int(sprintf(ptr, "%d", (int) dptr->option));

            break;
            }

        case F_TEXT:
#if 1
            ptr1 = dptr->textfield;
            if(str_isblank(ptr1))
                ptr1 = NULLSTR;

            if(NULL != dptr->optionlist)
            {
                ptr2 = *dptr->optionlist;
                if(str_isblank(ptr2))
                    ptr2 = NULLSTR;
            }
            else
                ptr2 = NULLSTR;

            if(0 == strcmp(ptr1, ptr2))
                continue;

            xstrkat(linbuf, LIN_BUFSIZ, ptr1);
#else
            /* default options are blank, except leading,trailing characters */

            if( (dptr == d_options + O_LP)  ||
                (dptr == d_options + O_TP)  )
            {
                if(0 == strcmp(dptr->textfield, *dptr->optionlist))
                    continue;
            }
            else if( not_on_list  &&
                     (str_isblank(dptr->textfield)  ||  (0 == strcmp(dptr->textfield, *dptr->optionlist))))
                continue;

            if(dptr->textfield)
                xstrkat(linbuf, LIN_BUFSIZ, dptr->textfield);
#endif
            break;

        default:
            break;
        }

        xstrkat(linbuf, LIN_BUFSIZ, CR_STR);

        /* output the string, expanding inline hilites */
        ptr = linbuf;

        while((ch1 = *ptr++) != CH_NULL)
        {
            if(ishighlight(ch1))
            {
                char buffer[32];
                consume_int(snprintf(buffer, sizeof32(buffer), "%%" "H" "%c" "%%", (ch1 - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT));
                away_string_simple(buffer, output);
            }
            else
                away_byte(ch1, output);
        }
    }
}

extern void
set_width_and_wrap(
    COL tcol,
    coord width,
    coord wrapwidth)
{
    P_S32 widp, wwidp;

    readpcolvars(tcol, &widp, &wwidp);
    *widp = width;
    *wwidp = wrapwidth;
}

static S32
get_right_margin(
    COL tcol)
{
    P_S32 widp, wwidp;

    readpcolvars(tcol, &widp, &wwidp);
    return(*wwidp);
}

/******************************************************************************
*
*  add the current directory to the start of the filename
*
******************************************************************************/

static void
addcurrentdir(
    char *name /*out*/,
    _InVal_     U32 elemof_buffer,
    const char *filename)
{
    xstrkpy(name, elemof_buffer, filename);
}

/******************************************************************************
*
* ask whether modified file can be overwritten
*
******************************************************************************/

_Check_return_
extern BOOL
save_or_discard_existing(void)
{
    enum RISCDIALOG_QUERY_SDC_REPLY SDC_res = riscdialog_query_save_or_discard_existing();

    return(riscdialog_query_SDC_CANCEL != SDC_res);
}

/*
full size the window
*/

extern void
FullScreen_fn(void)
{
    COL tcol;
    ROW trow;

    open_box.x0 = 0;
    open_box.x1 = 16384;
    open_box.y0 = -16384;
    open_box.y1 = 0;

    /* get first non-fixed stuff so as not lose position in window */
    tcol = fstncx();
    trow = fstnrx();

    riscos_front_document_window_atbox(TRUE);

    /* ensure window back as it was */
    filvert(trow, trow, CALL_FIXPAGE);
    filhorz(tcol, tcol);
}

static BOOL
name_read_tstr(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PCTSTR p_string)
{
    BOOL ok;
    PCTSTR wholename = p_string;

    while(*wholename++ == ' ')
    { /*EMPTY*/ }
    --wholename;

    name_init(p_docu_name);

    for(;;)
    { /* loop for structure */
        PCTSTR leafname;

        if(file_is_rooted(wholename))
        {
            leafname = file_leafname(wholename);

            ok = mystr_set(&p_docu_name->leaf_name, leafname);
            if(!ok)
                break;

            ok = mystr_set_n(&p_docu_name->path_name, wholename, leafname - wholename); /* including dir sep ch */
            if(!ok)
                break;

            p_docu_name->flags.path_name_supplied = 1;
        }
        else
        {
            leafname = wholename;

            ok = mystr_set(&p_docu_name->leaf_name, leafname);
        }

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    if(!ok)
        name_free(p_docu_name);

    return(ok);
}

static BOOL
same_name_warning(
    _In_z_      PCTSTR filename,
    _In_z_      PCTSTR statement,
    _In_z_      PCTSTR question)
{
    DOCU_NAME docu_name;
    EV_DOCNO docno;
    BOOL ok = name_read_tstr(&docu_name, filename);

    if(!ok)
        return(FALSE);

    docno = docno_find_name(&docu_name, is_current_document() ? (EV_DOCNO) current_docno() : DOCNO_NONE, TRUE);
    /* NB avoid finding the current document or any docu thunks */

    if(DOCNO_NONE != docno)
    {
        char statement_buffer[256];
        consume_int(xsnprintf(statement_buffer, elemof32(statement_buffer), statement, filename));
        return(riscdialog_query_YES ==
               riscdialog_query_YN(statement_buffer, question));
    }

    return(TRUE);
}

/******************************************************************************
*
* name the current text
*
******************************************************************************/

static BOOL
namefile_fn_core(void)
{
    const char * filename = d_name[0].textfield ? d_name[0].textfield : get_untitled_document();

    if(str_isblank(filename))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    if(0 == C_stricmp(filename, currentfilename()))
        return(TRUE);

    if(same_name_warning(filename, name_supporting_Zs_YN_S_STR, name_supporting_YN_Q_STR))
    {
        DOCU_NAME docu_name;
        BOOL ok = name_read_tstr(&docu_name, filename);

        if(ok)
        {
            xf_loaded_from_path = (BOOLEAN) name_preprocess_docu_name_flags_for_rename(&docu_name);

            rename_document(&docu_name);

            name_free(&docu_name);
        }
    }

    return(TRUE);
}

static BOOL
namefile_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(mystr_set(&d_name[0].textfield, currentfilename()));

    return(dialog_box_start());
}

extern void
NameFile_fn(void)
{
    if(!namefile_fn_prepare())
        return;

    while(dialog_box(D_NAME))
    {
        if(!namefile_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* save template file
*
******************************************************************************/

static BOOL
savetemplatefile_fn_core(void)
{
    PCTSTR leafname = d_save_template[0].textfield;
    TCHARZ buffer[BUF_MAX_PATHSTRING];

    if(str_isblank(leafname))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    /* append the leafname to <Choices$Write>.PipeDream.Templates. */
    (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), leafname, LTEMPLATE_SUBDIR_STR);

    { /* and save away in PipeDream format */
    SAVE_FILE_OPTIONS save_file_options;
    save_file_options_init(&save_file_options, PD4_CHAR, current_line_sep_option);
    return(savefile_core(buffer, &save_file_options));
    } /*block*/
}

static BOOL
savetemplatefile_fn_prepare(void)
{
    /* push out the leafname of the current file as a suggestion */
    PCTSTR leafname = file_leafname(currentfilename());

    false_return(dialog_box_can_start());

    false_return(mystr_set(&d_save_template[0].textfield, leafname));

    return(dialog_box_start());
}

extern void
SaveTemplateFile_fn(void)
{
    if(!savetemplatefile_fn_prepare())
        return;

    while(dialog_box(D_SAVE_TEMPLATE))
    {
        if(!savetemplatefile_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* save Choices and Preferred files in <Choices$Write>.PipeDream directory
*
******************************************************************************/

extern void
SaveChoices_fn(void)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];

    /* save Choices in <Choices$Write>.PipeDream directory */
    (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), CHOICES_FILE_STR, NULL);
    {
    SAVE_FILE_OPTIONS save_file_options;
    save_file_options_init(&save_file_options, PD4_CHAR, 0);
    save_file_options.saving_choices_file = TRUE;
    (void) savefile_core(buffer, &save_file_options);
    } /*block*/

    /* save Preferred in <Choices$Write>.PipeDream directory (iff needed) */
    (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), PREFERRED_FILE_STR, NULL);
    pdchart_preferred_save(buffer);
}

/******************************************************************************
*
* save file
*
******************************************************************************/

extern void
save_file_options_init(
    _OutRef_    P_SAVE_FILE_OPTIONS p_save_file_options,
    _InVal_     S32 filetype_option /* PD4_CHAR etc. */,
    _InVal_     S32 line_sep_option /* LINE_SEP_LF etc. */)
{
    zero_struct_ptr_fn(p_save_file_options);

    p_save_file_options->filetype_option = (char) filetype_option;
    p_save_file_options->line_sep_option = (char) line_sep_option;
}

static BOOL
do_SaveFile_prepare(
    _InVal_     U32 boxnumber)
{
    false_return(init_dialog_box(boxnumber));

    d_save[SAV_FORMAT].option = current_filetype_option;
    d_save[SAV_LINESEP].option = current_line_sep_option;

    false_return(mystr_set(&d_save[SAV_NAME].textfield, currentfilename()));

    return(dialog_box_start());
}

static int
do_SaveFile_core(void)
{
    SAVE_FILE_OPTIONS save_file_options;
    save_file_options_init(&save_file_options, d_save[SAV_FORMAT].option, d_save[SAV_LINESEP].option);

    if( ('Y' == d_save[SAV_ROWCOND].option)  &&  !str_isblank(d_save[SAV_ROWCOND].textfield) )
        save_file_options.row_condition = d_save[SAV_ROWCOND].textfield;
    if('Y' == d_save[SAV_BLOCK].option)
        save_file_options.saving_block = TRUE;

    if(str_isblank(d_save[SAV_NAME].textfield))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(savefile(d_save[SAV_NAME].textfield, &save_file_options));
}

static void
do_SaveFile(
    _InVal_     U32 boxnumber)
{
    if(!do_SaveFile_prepare(boxnumber))
        return;

    while(dialog_box(boxnumber))
    {
        int core_res = do_SaveFile_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

_Check_return_
static BOOL
try_save_immediate(void)
{
    SAVE_FILE_OPTIONS save_file_options;
    save_file_options_init(&save_file_options, current_filetype_option, current_line_sep_option);

    if(!file_is_rooted(currentfilename()))
        return(FALSE);

    (void) savefile_core(currentfilename(), &save_file_options);
    return(TRUE);
}

extern void
SaveFileAs_fn(void)
{
    do_SaveFile(D_SAVE_FILE_AS);
}

extern void
SaveFileAs_Imm_fn(void)
{
    if(try_save_immediate())
        return;

    do_SaveFile(D_SAVE_FILE_AS_POPUP); /* different as we may be in_execfile || command_expansion */
}

extern void
SaveFileSimple_fn(void)
{
    do_SaveFile(D_SAVE_FILE_SIMPLE);
}

extern void
SaveFileSimple_Imm_fn(void)
{
    if(try_save_immediate())
        return;

    do_SaveFile(D_SAVE_FILE_SIMPLE_POPUP); /* different as we may be in_execfile || command_expansion */
}

/******************************************************************************
*
* output a cell in plain text mode,
* saving results of numbers rather than contents
* note that fonts are OFF so no juicy formatting
*
******************************************************************************/

extern BOOL
plain_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    char filetype_option,
    uchar *buffer /*out*/)
{
    uchar oldformat;
    uchar thousands;
    S32 len;
    S32 colwid;
    S32 fwidth = LIN_BUFSIZ;

    switch(tcell->type)
    {
    case SL_TEXT:
    case SL_PAGE:
        (void) expand_cell(
                    current_docno(), tcell, trow, buffer, fwidth,
                    DEFAULT_EXPAND_REFS /*expand_refs*/,
                    EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                    EXPAND_FLAGS_DONT_EXPAND_CTRL /*!expand_ctrl*/ |
                    EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT /*!allow_fonty_result*/ /*expand_flags*/,
                    TRUE /*cff*/);
        return(FALSE);

    default:
    /* case SL_NUMBER: */
        break;
    }

    oldformat = tcell->format;
    thousands = d_options_TH;

    switch(filetype_option)
    {
    default:
        break;

    case CSV_CHAR:
        #if !defined(OLD_CSV_SAVE_STYLE)
        /* poke cell to minus format, no leading or trailing chars */
        tcell->format &= ~(F_BRAC | F_LDS | F_TRS);
        #else
        tcell->format = F_DCPSID | F_DCP;
        #endif
        /* ensure no silly commas etc. output! */
        d_options_TH = TH_BLANK;
        break;

    case PARAGRAPH_CHAR:
    case FWP_CHAR:
        /* format cells on output to the same as we'd print them */
        colwid = colwidth(tcol);
        fwidth = chkolp(tcell, tcol, trow);
        fwidth = MIN(fwidth, colwid);
        fwidth = MIN(fwidth, LIN_BUFSIZ);
        break;
    }

    trace_6(TRACE_APP_PD4, "plain_slot(&%p, %d, %d, '%c', &%p): fwidth %d", report_ptr_cast(tcell), tcol, trow, filetype_option, buffer, fwidth);

    (void) expand_cell(
                current_docno(), tcell, trow, buffer, fwidth,
                DEFAULT_EXPAND_REFS /*expand_refs*/,
                EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                EXPAND_FLAGS_DONT_EXPAND_CTRL /*!expand_ctrl*/ |
                EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT /*!allow_fonty_result*/ /*expand_flags*/,
                TRUE /*cff*/);
    /* does not move cell */

    tcell->format = oldformat;
    d_options_TH = thousands;

    /* remove padding space from plain non-fonty string */
    len = strlen(buffer);

    while(len--)
    {
        if(buffer[len] == FUNNYSPACE)
        {
            buffer[len] = CH_NULL;
            break;
        }
    }

    trace_1(TRACE_APP_PD4, "plain_slot returns '%s'", buffer);
    return(TRUE);
}

/******************************************************************************
*
* save the document to a file, renaming if necessary
*
******************************************************************************/

extern BOOL
savefile(
    _In_z_      const char * filename,
    _InRef_     P_SAVE_FILE_OPTIONS p_save_file_options)
{
    /* saving as PipeDream or in the current format? other options imply export as... */
    BOOL saving_whole_file = (PD4_CHAR == p_save_file_options->filetype_option) || (current_filetype_option == p_save_file_options->filetype_option);
    BOOL saving_part_file = p_save_file_options->saving_block  ||  (NULL != p_save_file_options->row_condition);

    if(str_isblank(filename))
        return(reperr_null(ERR_BAD_NAME));

    if(saving_part_file  ||  p_save_file_options->saving_choices_file  ||  p_save_file_options->temp_file)
        saving_whole_file = FALSE;

    /* rename file BEFORE saving to get external refs output correctly */
    if(saving_whole_file)
    {
        if(0 != C_stricmp(filename, currentfilename()))
        {
            if(same_name_warning(filename, name_supporting_Zs_YN_S_STR, name_supporting_YN_Q_STR))
            {
                DOCU_NAME docu_name;
                BOOL ok = name_read_tstr(&docu_name, filename);

                if(ok)
                {
                    xf_loaded_from_path = (BOOLEAN) name_preprocess_docu_name_flags_for_rename(&docu_name);

                    rename_document(&docu_name);

                    name_free(&docu_name);
                }
            }
            else
                return(FALSE);
        }
    }

    return(savefile_core(filename, p_save_file_options));
}

static BOOL
savefile_core(
    _In_z_      const char * filename,
    _InRef_     P_SAVE_FILE_OPTIONS p_save_file_options)
{
    /* saving as PipeDream or in the current format? other options imply export as... */
    BOOL saving_whole_file = (PD4_CHAR == p_save_file_options->filetype_option) || (current_filetype_option == p_save_file_options->filetype_option);
    BOOL saving_part_file = p_save_file_options->saving_block  ||  (NULL != p_save_file_options->row_condition);
    FILE_HANDLE output;
    RISCOS_FILEINFO fileinfo = currentfileinfo;
    BOOL goingdown = TRUE; /* for PD4 */
    BOOL all_ok = TRUE;
    ROW timer = 0;
    COL tcol, prevcol;
    ROW trow, prevrow;
    P_CELL tcell;
    SLR first, last;
    char rowselection[EV_MAX_OUT_LEN + 1];
    char array[32];
    char field_separator = CH_NULL;
    uchar condval;
    BOOL triscos_fonts;
    coord v_chars_sofar = 0;
    S32 splitlines = 0;
    coord rightmargin = 0;

    if(!mergebuf())
        return(FALSE);

    if(saving_part_file  ||  p_save_file_options->saving_choices_file  ||  p_save_file_options->temp_file)
        saving_whole_file = FALSE;

    /* if saving part file to a non-scrap file,
     * check that existing file shouldn't be overwritten
    */
    if(saving_part_file  &&  !p_save_file_options->temp_file)
        if(!checkoverwrite(filename))
            return(FALSE);

    /* sort out format to be saved */

    switch(p_save_file_options->filetype_option)
    {
    case PD4_CHAR:
        goingdown = TRUE;
        break;

    case CSV_CHAR:
        field_separator = COMMA;
        goingdown = FALSE;
        break;

    /* case TAB_CHAR: */
    /* case VIEW_CHAR: */
    /* case PARAGRAPH_CHAR: */
    /* case FWP_CHAR: */
    default:
        field_separator = TAB;
        goingdown = FALSE;
        break;
    }

    /* save marked block? */

    if(p_save_file_options->saving_block)
    {
        if(!MARKER_DEFINED())
            return(reperr_null(MARKER_SOMEWHERE() ? ERR_NOBLOCKINDOC : ERR_NOBLOCK));

        first = blkstart;
        last  = (blkend.col == NO_COL) ? blkstart : blkend;
    }
    else
    {
        first.col = (COL) 0;
        first.row = (ROW) 0;
        last.col  = numcol - 1;
        last.row  = numrow - 1;
    }

    prevcol = goingdown ? NO_COL : first.col;
    prevrow = first.row;

    /* open file and buffer it */
    if(NULL == (output = pd_file_open(filename, file_open_write)))
        return(reperr_null(ERR_CANNOTOPEN));

    (void) file_buffer(output, NULL, 16*1024); /* no messing about for save */

    /* file successfully opened at this point */

    actind(0);

    escape_enable();

    /* set up font dialog box for saving */

    update_dialog_from_fontinfo();

    triscos_fonts = font_stack(riscos_fonts);
    riscos_fonts = FALSE;

    /* row and column fixes have dummy dialog box - set it up */

    consume_bool(init_dialog_box(D_FIXES));

    if(0 != n_rowfixes)
    {
        assert(vertvec_entry_valid(0));
        consume_int(sprintf(array, "%d,%d", row_number(0), n_rowfixes));
        consume_bool(mystr_set(&d_fixes[0].textfield, array));
    }

    if(0 != n_colfixes)
    {
        assert(horzvec_entry_valid(0));
        consume_int(sprintf(array, "%d,%d", col_number(0), n_colfixes));
        consume_bool(mystr_set(&d_fixes[1].textfield, array));
    }

    /* NB some file types have specified line_sep_option */
    switch(p_save_file_options->filetype_option)
    {
    default:
        d_save[SAV_LINESEP].option = p_save_file_options->line_sep_option;
        break;

    case FWP_CHAR:
        assert(FWP_LINESEP == LF);
        d_save[SAV_LINESEP].option = LINE_SEP_LF;
        break;

    case VIEW_CHAR:
        assert(VIEW_LINESEP == CR);
        d_save[SAV_LINESEP].option = LINE_SEP_CR;
        break;
    }

    /* ensure recalced for any format where values are required */
    if(PD4_CHAR != p_save_file_options->filetype_option)
    {
        ev_recalc_all();

        if(ctrlflag)
            all_ok = FALSE;
    }

    /* start outputting preamble */

    /* RJM takes out !saving_part_file on 31.8.91 cos options should go */
    /* but SKS says fixed cols / rows ought not to go out ... */
    if( all_ok  &&  goingdown  &&  !save_options_to_file(output, p_save_file_options->saving_choices_file)) /* warning: destroys block! */
        all_ok = FALSE;

    switch(p_save_file_options->filetype_option)
    {
    case VIEW_CHAR: /* save VIEW preamble here */
        if( all_ok  &&  !view_save_ruler_options(&rightmargin, output))
            all_ok = FALSE;
        break;

    case FWP_CHAR: /* save FWP preamble here */
        if( all_ok  &&  !fwp_save_file_preamble(output, TRUE, first.row, last.row))
            all_ok = FALSE;
        /* send out style usage for first line */
        if( all_ok  &&  !fwp_save_line_preamble(output, TRUE))
            all_ok = FALSE;
        break;

    case PARAGRAPH_CHAR:
        if( all_ok  &&  !fwp_save_file_preamble(output, FALSE, first.row, last.row))
            all_ok = FALSE;
        /* send out style usage for first line */
        if( all_ok  &&  !fwp_save_line_preamble(output, FALSE))
            all_ok = FALSE;
        break;

    default:
        break;
    }

    /* print all cells in block, for plain text going
     * row by row, otherwise column by column.
     * NB. don't set any earlier or in danger of block damage
    */
    init_block(&first, &last);

    /* if just a Choices file, only print column constructs */

    if(p_save_file_options->saving_choices_file)
    {
        tcol = 0;

        while(!ctrlflag  &&  all_ok  &&  (tcol < numcol))
            if(!collup(tcol++, output))
                all_ok = FALSE;
    }
    else
    {
        /* ensure consistently bad page numbers are saved */
        curpnm = 0;

        /* save all cells */
        while(!ctrlflag  &&  all_ok  &&  next_in_block(goingdown))
        {
            tcol = in_block.col;
            trow = in_block.row;

            /* row selection - if it's a new column re-initialize row selection */

            if((trow == first.row)  &&  (goingdown  ||  (tcol == first.col)))
            {
                if(NULL != p_save_file_options->row_condition)
                {
                    S32 len;
                    S32 at_pos;        /* error position in source, not used, may be useful one day */
                    EV_RESULT ev_result;
                    EV_PARMS parms;

                    if((len = compile_expression(rowselection,
                                                 de_const_cast(char *, p_save_file_options->row_condition),
                                                 EV_MAX_OUT_LEN,
                                                 &at_pos,
                                                 &ev_result,
                                                 &parms)) < 0 ||
                       parms.type == EVS_CON_DATA)
                    {
                        escape_disable();
                        pd_file_close(&output);
                        return(reperr_null(ERR_BAD_SELECTION));
                    }
                }
                else
                    *rowselection = CH_NULL;
            }

            /* now check row selection for this row */
            if(!str_isblank(rowselection))
            {
                S32 res;

                if((res = row_select(rowselection, trow)) < 0)
                {
                    all_ok = FALSE;
                    reperr_null(res);
                    break;
                }
                else if(!res)
                    continue;
            }

            /* if going across and new row */
            if(!goingdown  &&  (prevrow != trow))
            {
                prevrow = trow;

                if(PARAGRAPH_CHAR != p_save_file_options->filetype_option)
                {
                    /* output return */
                    if(!away_eol(output))
                    {
                        all_ok = FALSE;
                        break;
                    }
                }

                /* reset VIEW line count */
                v_chars_sofar = 0;

                /* send out style usage for this line if FWP/DTP */
                if((PARAGRAPH_CHAR == p_save_file_options->filetype_option)  ||  (FWP_CHAR == p_save_file_options->filetype_option))
                    fwp_save_line_preamble(output, FWP_CHAR == p_save_file_options->filetype_option);
            }

            tcell = travel(tcol, trow);

            /* for a new column, either a column heading construct needs to be
             * be written or (when in plain text mode) tabs up to
             * the current column
            */
            if(prevcol != tcol)
            {
                if(!goingdown)
                {
                    /* in plain text (or commarated) mode don't bother writing
                     * field separator if there are no more cells in this row
                    */
                    if( prevcol > tcol)
                        prevcol = tcol;  /* might be new row */

                    /* if it's a blank cell don't bother outputting field separators
                     * watch out for number cells masquerading as blanks
                    */
                    if( !is_blank_cell(tcell)  ||
                        (tcell  &&  (tcell->type != SL_TEXT)))
                    {
                        while(prevcol < tcol)
                        {
                            if(!away_byte(field_separator, output))
                            {
                                all_ok = FALSE;
                                break;
                            }

                            prevcol++;
                        }
                    }
                }
                else
                {
                    /* write new column construct info */
                    prevcol = tcol;

                    if(!collup(tcol, output))
                        all_ok = FALSE;
                }
            }

            if(!tcell)
            {
                if(atend(tcol, trow))
                {
                    if(goingdown)
                        force_next_col();
                }
                else if(goingdown  &&  !away_eol(output))
                {
                    all_ok = FALSE;
                    break;
                }
                /* paragraph saving does its own linefeeds */
                else if(PARAGRAPH_CHAR == p_save_file_options->filetype_option)
                {
                    if(!dtp_save_slot(tcell, tcol, trow, output))
                        all_ok = FALSE;
                }
            }
            else
            {
                /* cell needs writing out */
                if(timer++ > TIMER_DELAY)
                {
                    timer = 0;
                    actind_in_block(goingdown);
                }

                /* RJM would redirect saving_part_file on 31.8.91, but we still need the cell bits!! */
                if(goingdown)
                {
                    if(!pd_save_slot(tcell, tcol, trow, output, saving_part_file)  ||
                       !away_eol(output)  )
                        all_ok = FALSE;
                }
                else
                {
                    switch(p_save_file_options->filetype_option)
                    {
                    case VIEW_CHAR:
                        /* check for page break */
                        if(chkrpb(trow))
                        {
                            if(tcol == 0)
                            {
                                char array[10];

                                strcpy(array, "PE");
                                condval = travel(0, trow)->content.page.condval;
                                if(condval > 0)
                                    consume_int(sprintf(array + 2, "%d", (int) condval));

                                if(!view_save_stored_command(array, output))
                                    all_ok = FALSE;
                            }

                            /* don't output anything on page break line */
                            continue;
                        }

                        if(tcol == 0)
                        {
                            const char * command;

                            switch((tcell->justify) & J_BITS)
                            {
                            case J_LEFT:   command = "LJ"; break;
                            case J_CENTRE: command = "CE"; break;
                            case J_RIGHT:  command = "RJ"; break;
                            default:       command = NULL; break;
                            }

                            if(NULL != command)
                                if(!view_save_stored_command(command, output))
                                    all_ok = FALSE;
                        }

                        if( all_ok  &&
                            tcell   &&  !view_save_slot(tcell, tcol, trow, output,
                                                        &v_chars_sofar,
                                                        &splitlines, rightmargin))
                            all_ok = FALSE;

                        break;

                    case PARAGRAPH_CHAR:
                    case FWP_CHAR:
                        /* check for page break */
                        if(chkrpb(trow))
                        {
                            if(!tcol)
                            {
                                condval = travel(0, trow)->content.page.condval;

                                /* don't bother with conditional breaks - they're too stupid */

                                if(!condval  &&  !away_byte(FWP_FORMFEED, output))
                                    all_ok = FALSE;
                            }

                            /* don't output anything on page break line */
                            continue;
                        }

                        if(FWP_CHAR == p_save_file_options->filetype_option)
                        {
                            if(!fwp_save_slot(tcell, tcol, trow, output, TRUE))
                                all_ok = FALSE;
                        }
                        else
                        {
                            if(!dtp_save_slot(tcell, tcol, trow, output))
                                all_ok = FALSE;
                        }
                        break;

                    default:
                        /* check for page break */
                        if(chkrpb(trow))
                            /* don't output anything on page break line */
                            continue;

                        {
                        /* write just text or formula part of cell out */
                        BOOL csv_quotes = !plain_slot(tcell, tcol, trow, p_save_file_options->filetype_option, linbuf)
                                          &&  (CSV_CHAR == p_save_file_options->filetype_option);
                        uchar * lptr = linbuf;
                        uchar ch;

                        /* output contents, not outputting highlight chars */
                        if(csv_quotes  &&  !away_byte(QUOTES, output))
                            all_ok = FALSE;

                        while(all_ok  &&  ((ch = *lptr++) != CH_NULL))
                        {
                            if((CSV_CHAR == p_save_file_options->filetype_option)  &&  (ch == QUOTES))
                                if(!away_byte(QUOTES, output))
                                {
                                    all_ok = FALSE;
                                    break;
                                }

                            if(!ishighlight(ch)  &&  !away_byte(ch, output))
                                all_ok = FALSE;
                        }

                        if( all_ok  &&  csv_quotes  &&  !away_byte(QUOTES, output))
                            all_ok = FALSE;
                        }

                        break;
                    }
                }
            }
        }
    }

    /* terminate with a line separator if not PipeDream file */
    if( all_ok  &&  !goingdown  &&  !away_eol(output))
        all_ok = FALSE;

    if((file_flush(output) < 0)  ||  !all_ok) /* ensures we always get an error report if needed */
    {
        reperr_null(ERR_CANNOTWRITE);
        all_ok = FALSE;
    }

    if(pd_file_close(&output))
    {
        reperr_null(ERR_CANNOTCLOSE);
        all_ok = FALSE;
    }

    /* set the correct file type and modification time on saved file in any case */
    riscos_settype(&fileinfo, currentfiletype(p_save_file_options->filetype_option));
    riscos_readtime(&fileinfo);
    riscos_writefileinfo(&fileinfo, filename);

    riscos_fonts = font_unstack(triscos_fonts);

    if(ctrlflag)
        all_ok = FALSE;

    escape_disable();

    actind_end();

    slot_in_buffer = FALSE;

    if(all_ok  &&  saving_whole_file)
    {
        currentfileinfo = fileinfo;

        filealtered(FALSE);

        current_line_sep_option = p_save_file_options->line_sep_option;

        /* if this is a printer driver kill the current one cos we might need to reload */
        if(xf_file_is_driver)
            kill_driver();
    }

    if(all_ok  &&  (splitlines > 0))
    {
        U8Z buffer[20];
        (void) xsnprintf(buffer, elemof32(buffer), "%d", splitlines);
        reperr(ERR_LINES_SPLIT, buffer);
    }

    return(all_ok);
}

/******************************************************************************
*
* output column heading info
*
******************************************************************************/

static BOOL
collup(
    COL tcol,
    FILE_HANDLE output)
{
    uchar colnoarray[40];
    char array[LIN_BUFSIZ];
    P_S32 widp, wwidp;

    (void) xtos_ustr_buf(colnoarray, elemof32(colnoarray),
                         tcol & COLNOBITS /* ignore bad and absolute */,
                         1 /*uppercase*/);

    /* note that to get the wrapwidth, we cannot call colwrapwidth here because it would not return a zero value */
    readpcolvars(tcol, &widp, &wwidp);

    (void) xsnprintf(array, elemof32(array),
            "%%" "CO:%s,%d,%d" "%%",
            colnoarray,
            colwidth(tcol),
            *wwidp);

    return(away_string_simple(array, output));
}

/******************************************************************************
*
* save cell
*
******************************************************************************/

static BOOL
pd_save_slot(
    P_CELL tcell,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    BOOL saving_part_file)
{
    uchar * lptr, ch;
    BOOL is_number = FALSE;
    uchar justify;

    /* save cell type, followed by justification, followed by formats */

    switch((int) tcell->type)
    {
    default:
    /* case SL_TEXT: */
        break;

    case SL_NUMBER:
        is_number = TRUE;
        if(!away_construct(C_VALUE, output))
            return(FALSE);
        break;

    case SL_PAGE:
        {
        char page_array[32];
        (void) xsnprintf(page_array, elemof32(page_array),
                        "%%" "P" "%d" "%%", tcell->content.page.condval);
        if(!away_string_simple(page_array, output))
            return(FALSE);
        break;
        }
    }

    justify = tcell->justify & J_BITS;

    if((justify > 0)  &&  (justify <= 6))
        if(!away_construct(justify, output))
            return(FALSE);

    if(is_number)
    {
        const uchar format = tcell->format;

        if(format & (F_LDS | F_TRS | F_DCP))
        {
            if(format & F_LDS)
                if(!away_construct(C_LEADCH, output))
                    return(FALSE);

            if(format & F_TRS)
                if(!away_construct(C_TRAILCH, output))
                    return(FALSE);

            /* if DCP set, save minus and decimal places */
            if(format & F_DCP)
            {
                static U8 decimals_array[] = "%D.%"; /* poke [2] */
                const uchar decimals = format & F_DCPSID; /* mask out non-decimal bits */

                /* sign minus, don't bother saving it. load() will set the DCP
                    bit by implication as the decimal places field is saved
                */

                if(format & F_BRAC)
                    if(!away_construct(C_BRAC, output))
                        return(FALSE);

                decimals_array[2] = (U8) ((decimals == 0xF) ? 'F' : (decimals + '0'));
                if(!away_string_simple(decimals_array, output))
                    return(FALSE);
            }
        }
    }

    /* RJM 9.10.91, this plain_slot ensures blocks and row selections aren't saved with silly cell references
     * it helps the PipeDream 4 example database to work, amongst other things
     */
    if(saving_part_file)
        plain_slot(tcell, tcol, trow, PD4_CHAR, linbuf);
    else
        prccon(linbuf, tcell);

    /* output contents, dealing with highlight chars and % */

    { /* SKS 20130403 quick check to see if we can avoid special case processing (usually we can) */
    P_U8 simple_string = lptr = linbuf;

    while((ch = *lptr++) != CH_NULL)
    {
        if((ch < 0x20) || (ch == '%'))
        {
            simple_string = NULL;
            break;
        }
    }

    if(NULL != simple_string)
        return(away_string_simple(simple_string, output));
    } /*block*/

    lptr = linbuf;

    while((ch = *lptr++) != CH_NULL)
    {
        if(ch == '%')
        {
            if(!away_construct(C_PERCENT, output))
                return(FALSE);
        }
        /* assumes multi-line separator of linefeed */
        else if(ch == LF)
        {
            if(!away_construct(C_LF, output))
                return(FALSE);
        }
        else if(ishighlight(ch))
        {
            static U8 highlight_array[] = "%H.%";
            highlight_array[2] = (ch - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;
            if(!away_string_simple(highlight_array, output))
                return(FALSE);
        }
        else if(!away_byte(ch, output))
            return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* look up construct in table.
* If valid, do whatever is needed
* It is passed the addresses of type,justify,format so that they can be updated
*
******************************************************************************/

static S32
lukcon(
    uchar *from,
    P_COL tcol,
    P_ROW trow,
    uchar *type,
    uchar *justify,
    uchar *format,
    COL firstcol,
    ROW firstrow,
    uchar *pageoffset,
    BOOL *outofmem,
    P_ROW rowinfile,
    BOOL build_cols,
    BOOL *first_column,
    BOOL inserting)
{
    uchar * tptr;
    U32 i;
    coord wid, wrapwid;
    COL luk_newcol;
    uchar or_in;
    uchar from2;
    S32 poffset;
    uchar * coloff;

    static COL column_offset;

    /* check starts with uppercase alpha */
    if((from[1] < 'A')  ||  (from[1] > 'Z'))
        return(0);

    /* check another % in range */
    for(tptr = from + 2; ; tptr++)
    {
        if(*tptr == '%')
            break;

        if(!*tptr  ||  tptr >= from + 15)
            return(0);
    }

    /* its a construct.  if can't find construct in table ignore it */
    for(i = 0; ; i++)
    {
        if(i >= C_ITEMS)
            return(2 /*unknown construct*/);

        if(0 == memcmp(contab[i].str, from + 1, contab[i].len))
            break;
    }

    switch(i)
    {
    default:
        return(0);

    case C_VALUE:
        *type = SL_NUMBER;
        break;

    case C_FREE:
    case C_LEFT:
    case C_CENTRE:
    case C_RIGHT:
    case C_JLEFT:
    case C_JRIGHT:
    case C_LCR:
        *justify = (uchar) i;
        break;

    case C_PERCENT:
        /* just adjust pos to point to the first % */
        start_of_construct++;
        break;

    case C_COL:
        coloff = buff_sofar = from + contab[i].len + 1;

        /* luk_newcol is number of new column */
        luk_newcol = getcol() + firstcol;

        /*reportf("lukcon: got C_COL; newcol = %d", luk_newcol);*/
        if(build_cols && !createcol(luk_newcol))
        {
            *outofmem = TRUE;
            return(reperr_null(status_nomem()));
        }

        if(*buff_sofar++ != ',')
            return(0);

        /* get the width - a valid response is a +number
         * or a -ve ',' meaning none specified
        */
        wid = (coord) getsbd();
        if(wid >= 0)
        {
            if(*buff_sofar++ != ',')
                return(0);
        }
        else if(0-wid != ',')
            return(0);

        /* get the wrapwidth - a valid response is a +ve number
         * or a -ve ',' meaning none specified
        */
        wrapwid = (coord) getsbd();
        if(wrapwid >= 0)
        {
            if(*buff_sofar != '%')
                return(0);
        }
        else if(0-wrapwid != '%')
            return(0);

        if(*first_column)
        {
            column_offset = inserting ? luk_newcol : 0;
            *first_column = FALSE;
        }

        *tcol = firstcol + luk_newcol - column_offset;
        *trow = firstrow;
        *rowinfile = 0;

        if(build_cols)
            set_width_and_wrap(*tcol, wid, wrapwid);

        break;

    /* for option, just remember position: gets dealt with at end of cell */

    case C_OPT:
        in_option = start_of_construct;
        return(0);

    case C_HIGHL:
        from2 = from[2];
        if((from2 < FIRST_HIGHLIGHT_TEXT)  ||  (from2 > LAST_HIGHLIGHT_TEXT)  ||  (from[3] != '%'))
            return(0);
        *from = FIRST_HIGHLIGHT + from2 - FIRST_HIGHLIGHT_TEXT;
        start_of_construct++;
        break;

    case C_LF:
        /* assumes multi-line separator of linefeed */
        *from = LF;
        start_of_construct++;
        break;

    case C_DEC:
        /* decimal places - set F_DCP bit
         * if %DF%, float format else
         * or in amount in last four bits
        */
        from2 = from[2];
        if(((from2 < '0'  ||  from2 > '9') && from2 != 'F')  ||  from[3] != '%')
            return(0);
        or_in = (uchar) ((from2 == 'F')     ? (F_DCP | 0xF)
                                            : (F_DCP | (from2-'0')));
        *format |= or_in;
        break;

    case C_LEADCH:
        *format |= F_LDS;
        break;

    case C_TRAILCH:
        *format |= F_TRS;
        break;

    case C_BRAC:
        *format |= (F_BRAC | F_DCP);
        break;

    case C_PAGE:
        *type = SL_PAGE;
        buff_sofar = from+2;
        poffset = (S32) getsbd();
        if(poffset < 0  ||  *buff_sofar++ != '%')
            return(0);
        *pageoffset = (uchar) poffset;
        break;
    }

    return(1);
}

/******************************************************************************
*
* store from linbuf in cell
*
******************************************************************************/

static BOOL
stoslt(
    COL tcol,
    ROW trow,
    char type,
    char justify,
    char format,
    char pageoffset,
    BOOL inserting,
    BOOL parse_as_expression)
{
    if(type == SL_NUMBER)
    {
        if(str_isblank(linbuf))
        {
            type    = SL_TEXT;
            justify = J_FREE;
        }
        else
        {
            char compiled_out[EV_MAX_OUT_LEN];
            S32 at_pos;        /* error position in source, not used, may be useful one day */
            EV_RESULT ev_result;
            EV_PARMS parms;
            S32 rpn_len;

            if(parse_as_expression)
                /* PipeDream, ViewSheet */
                rpn_len = compile_expression(compiled_out, linbuf, EV_MAX_OUT_LEN, &at_pos, &ev_result, &parms);
            else
                /* CSV */
                rpn_len = compile_constant(linbuf, &ev_result, &parms); /* rpn_len zero -> good result (no RPN output) */

            if(rpn_len < 0)
            {
                type    = SL_TEXT;
                justify = J_FREE;
            }
            else
            {
                EV_SLR slr;

                set_ev_slr(&slr, tcol, trow);

                if(merge_compiled_exp(&slr, justify, format, compiled_out, rpn_len, &ev_result, &parms, TRUE, !inserting) < 0)
                {
                    slot_in_buffer = buffer_altered = FALSE;
                    return(reperr_null(status_nomem()));
                }

                return(TRUE);
            }
        }
    }

    switch(type)
    {
    default:
    #ifndef NDEBUG
        assert(0);
    case SL_TEXT:
    #endif
        {
        P_CELL tcell;

        /* compile directly into cell */
        buffer_altered = TRUE;
        if(!merstr(tcol, trow, FALSE, !inserting))
        {
            buffer_altered = FALSE; /* so filbuf succeeds */
            return(FALSE);
        }

        if((tcell = travel(tcol, trow)) != NULL)
            tcell->justify = justify;

        break;
        }

    case SL_PAGE:
        {
        P_CELL tcell;

        /* type is not text */
        if((tcell = createslot(tcol, trow, sizeof(struct CELL_CONTENT_PAGE), type)) == NULL)
        {
            slot_in_buffer = buffer_altered = FALSE;
            return(reperr_null(status_nomem()));
        }

        tcell->type    = type;
        tcell->justify = justify;

        tcell->content.page.condval = pageoffset;
        break;
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* load file
*
******************************************************************************/

extern void
load_file_options_init(
    _OutRef_    P_LOAD_FILE_OPTIONS p_load_file_options,
    _In_z_      const char * document_name,
    _InVal_     S32 filetype_option /* PD4_CHAR etc. */)
{
    zero_struct_ptr_fn(p_load_file_options);

    p_load_file_options->document_name = document_name;
    p_load_file_options->filetype_option = (char) filetype_option;
}

static BOOL
loadfile_fn_core(void)
{
    LOAD_FILE_OPTIONS load_file_options;
    load_file_options_init(&load_file_options, d_load[0].textfield, d_load[3].option);

    load_file_options.inserting = ('Y' == d_load[1].option) && is_current_document();
    if(load_file_options.inserting)
        load_file_options.insert_at_slot = d_load[1].textfield;
    if('Y' == d_load[2].option)
        load_file_options.row_range = d_load[2].textfield;

    if(str_isblank(d_load[0].textfield))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(loadfile(d_load[0].textfield, &load_file_options));
}

static BOOL
loadfile_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_LOAD));

    d_load[3].option = **(d_load[3].optionlist); /* Init to Auto */

    return(dialog_box_start());
}

extern void
LoadFile_fn(void)
{
    if(!loadfile_fn_prepare())
        return;

    while(dialog_box(D_LOAD))
    {
        if(!loadfile_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* insert file
*
******************************************************************************/

static BOOL
insertfile_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_LOAD));

    d_load[1].option = 'Y';

    { /* write current position into the target range */
    U8 buffer[32];

    (void) write_ref(buffer, elemof32(buffer), current_docno(), curcol, currow);

    if(!mystr_set(&d_load[1].textfield, buffer))
        return(FALSE);
    } /*block*/

    d_load[3].option = **(d_load[3].optionlist); /* Init to Auto */

    return(dialog_box_start());
}

extern void
InsertFile_fn(void)
{
    if(!insertfile_fn_prepare())
        return;

    while(dialog_box(D_LOAD))
    {
        if(!loadfile_fn_core())
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* load template file
*
******************************************************************************/

static int
loadtemplate_fn_core(void)
{
    PCTSTR template_name = d_load_template[0].textfield;
    char buffer[BUF_MAX_PATHSTRING];
    S32 res;

    if(str_isblank(template_name))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    /* Add prefix '<PipeDream$Path>.Templates.' to template */
    if((res = add_path_using_dir(buffer, elemof32(buffer), template_name, LTEMPLATE_SUBDIR_STR)) <= 0)
    {
        consume_bool(reperr_null(res ? res : ERR_NOTFOUND));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    {
    LOAD_FILE_OPTIONS load_file_options;
    load_file_options_init(&load_file_options, get_untitled_document_with(template_name), PD4_CHAR);
    return(loadfile_recurse(buffer, &load_file_options));
    } /*block*/
}

static BOOL
loadtemplate_fn_prepare(void)
{
    TCHARZ templates_path[BUF_MAX_PATHSTRING * 4];
    PCTSTR templates_path_subdir;
    LIST_ITEMNO templates_list_key;

    false_return(dialog_box_can_start());

    if(str_isblank(d_load_template[0].textfield))
        false_return(mystr_set(&d_load_template[0].textfield, DEFAULT_LTEMPLATE_FILE_STR));

    delete_list(&templates_list);

    if(NULL == _kernel_getenv("PipeDream$TemplatesPath", templates_path, elemof32(templates_path)))
    {
        reportf("TemplatesPath(%u:%s)", strlen(templates_path), templates_path);
        templates_path_subdir = NULL; /* No subdir as path elements on PipeDream$TemplatesPath all have Templates suffix already */
    }
    else
    {   /* Not there, revert to the old way */
        tstr_xstrkpy(templates_path, elemof32(templates_path), file_get_search_path());
        templates_path_subdir = LTEMPLATE_SUBDIR_STR;
    }

    templates_list_key = 0;
    status_assert(enumerate_dir_to_list_using_path(&templates_list, &templates_list_key, LTEMPLATE_SUBDIR_STR, templates_path, templates_path_subdir, FILETYPE_UNDETERMINED));

    return(dialog_box_start());
}

extern void
LoadTemplate_fn(void)
{
    if(!loadtemplate_fn_prepare())
        return;

    while(dialog_box(D_LOAD_TEMPLATE))
    {
        int core_res = loadtemplate_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();

    delete_list(&templates_list);
}

/******************************************************************************
*
* Use Lotus 1-2-3 converter if available for ease-of-use
*
******************************************************************************/

static PTSTR u_ltp_name_buffer = NULL; /* allocate iff needed */

_Check_return_
static BOOL
u_ltp_available(void)
{
    const U32 elemof_u_ltp_name_buffer = BUF_MAX_PATHSTRING;

    STATUS status;

    if(NULL == u_ltp_name_buffer)
        if(NULL == (u_ltp_name_buffer = al_ptr_alloc_elem(TCHAR, elemof_u_ltp_name_buffer, &status)))
            return(FALSE);

    if(NULL != _kernel_getenv("123PD$Dir", u_ltp_name_buffer, elemof_u_ltp_name_buffer))
        return(FALSE);

    tstr_xstrkat(u_ltp_name_buffer, elemof_u_ltp_name_buffer, ".u_ltp");

    return(file_is_file(u_ltp_name_buffer));
}

_Check_return_
static inline U32 /*next slot size*/
wimp_get_next_slot_size(void)
{
#if defined(NORCROFT_INLINE_SWIX)
    return(
        _swi(Wimp_SlotSize, _INR(0,1)|_RETURN(1),
        /*in*/  -1      /* just read */,
                -1      /* read */));
#else
    _kernel_swi_regs rs;
    rs.r[0] = -1; /* just read */
    rs.r[1] = -1; /* read */
    (void) cs_kernel_swi(Wimp_SlotSize, &rs);
    return(rs.r[1]);
#endif
}

static inline void
wimp_set_next_slot_size(
    _InVal_     U32 next_slot)
{
#if defined(NORCROFT_INLINE_SWIX)
    (void) _swix(Wimp_SlotSize, _INR(0,1),
                 -1        /* just read */,
                 next_slot /* next slot */);
#else
    _kernel_swi_regs rs;
    rs.r[0] = -1; /* just read */
    rs.r[1] = next_slot; /* next slot */
    (void) cs_kernel_swi(Wimp_SlotSize, &rs);
#endif
}

_Check_return_
static BOOL
loadfile_convert_lotus123(
    _Out_       P_PTSTR p_new_filename,
    _In_z_      PCTSTR filename)
{
    static PTSTR command_line_buffer = NULL; /* allocate iff needed */

    TCHARZ new_filename[BUF_MAX_PATHSTRING];

    const U32 elemof_command_line_buffer = BUF_MAX_PATHSTRING * 3;

    STATUS status;

    if(!u_ltp_available())
        return(FALSE);

    tstr_xstrkpy(new_filename, BUF_MAX_PATHSTRING, filename);
    if(NULL != file_extension(new_filename))
    {
        PTSTR p_ext = file_extension(new_filename);
        p_ext[-1] = CH_NULL;
    }
    tstr_xstrkat(new_filename, BUF_MAX_PATHSTRING, "/pd");

    return(str_set(p_new_filename, new_filename));

    if(NULL == command_line_buffer)
        if(NULL == (command_line_buffer = al_ptr_alloc_elem(TCHAR, elemof_command_line_buffer, &status)))
            return(reperr_null(status));

#if 1
    consume_int(xsnprintf(command_line_buffer, elemof_command_line_buffer,
                "WimpTask Run %s %s %s > null:",
                u_ltp_name_buffer,
                filename,
                new_filename));
#else
    tstr_xstrkpy(command_line_buffer, elemof_command_line_buffer, "WimpTask Run ");
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, u_ltp_name_buffer);
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, " ");
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, filename     /* <in file>  */ );
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, " ");
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, new_filename /* <out file> */ );
    tstr_xstrkat(command_line_buffer, elemof_command_line_buffer, " > null:");
#endif

    /* check that there will be enough memory in the Window Manager next slot to run this converter */
#define MIN_NEXT_SLOT 512 * 1024U
    if(wimp_get_next_slot_size() < MIN_NEXT_SLOT)
        wimp_set_next_slot_size(MIN_NEXT_SLOT /* write next slot */); /* sorry for the override, but it does need some space to run! */

    reportf(command_line_buffer);
    _kernel_oscli(command_line_buffer);

    if(!file_is_file(new_filename))
        return(FALSE);

    /* caller should mutate the load to use the converted file */
    return(TRUE);
}

/******************************************************************************
*
* loadfile (called from startup and LoadFile_fn)
*
******************************************************************************/

extern BOOL
loadfile(
    _In_z_      PCTSTR filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options)
{
    BOOL res;

    reportf("loadfile(%u:%s): filetype_option '%c'", strlen32(filename), filename, p_load_file_options->filetype_option);

    /* NB all paths with p_load_file_options->filetype_option set to non-AUTO_CHAR have already checked readability */

    if(AUTO_CHAR == p_load_file_options->filetype_option)
    {
        /* check readable for loadfile_recurse() */
        STATUS auto_filetype_option;

        if((auto_filetype_option = find_filetype_option(filename, FILETYPE_UNDETERMINED)) <= 0)
        {
            if(0 == auto_filetype_option)
                reperr(ERR_NOTFOUND, filename);
            /* else already reported */
            return(FALSE);
        }

        p_load_file_options->filetype_option = (char) auto_filetype_option;

        reportf("loadfile: auto-detected filetype_option '%c'", p_load_file_options->filetype_option);
    }

    if(LOTUS123_CHAR == p_load_file_options->filetype_option)
    {
        PTSTR new_filename = NULL;

        if(!loadfile_convert_lotus123(&new_filename, filename))
            return(reperr(ERR_CANT_LOAD_FILETYPE, filename));

        /* mutate this load to use the converted file (yup, it's a memory leak, but small and rare) */
        p_load_file_options->document_name = filename = new_filename;
        p_load_file_options->filetype_option = PD4_CHAR;
    }

    res = loadfile_recurse(filename, p_load_file_options);

    if(res && is_current_document())
        xf_caretreposition = xf_acquirecaret = TRUE;

    return(res);
}

/*
initialise the window position
*/

static inline void
init_open_box_sizes(void)
{
    str_clr(&d_open_box[0].textfield);
}

/*
read window position from option entry
*/

static BOOL
get_open_box_sizes(void)
{
    COL tcol, fcol;
    ROW trow, frow;
    BOOL use_loaded = FALSE;

    if(!str_isblank(d_open_box[0].textfield))
    {
        frow = -1;

        consume_int(sscanf(d_open_box[0].textfield,
                           "%d,%d,%d,%d,%d,%d,%d,%d",
                           &open_box.x0, &open_box.x1,
                           &open_box.y0, &open_box.y1,
                           &tcol, &trow, &fcol, &frow));

        /* did we get them all? */
        if(frow != -1)
            use_loaded = TRUE;
    }

    /* SKS after PD 4.11 08jan92 - attempt to get window position scrolled back ok */
    if(use_loaded)
    {
        riscos_front_document_window_atbox(TRUE);

        if(!mergebuf_nocheck())
            return(FALSE); /* SKS 10.10.91 */

        chknlr(tcol, trow);
        filhorz(fcol, fcol);
        filvert(frow, frow, CALL_FIXPAGE);

        /*draw_screen();*/ /* SKS 08jan92 added; SKS 20130604 removed - see caller */
    }
    else
        riscos_front_document_window(in_execfile);

    return(TRUE);
}

/******************************************************************************
*
* open a new window, maybe, and load the file
* if there are any unresolved external references, get those files too
*
******************************************************************************/

extern BOOL
loadfile_recurse(
    _In_z_      PCTSTR filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options)
{
    BOOL ok;
    DOCNO docno = current_docno();

    reportf("loadfile_recurse(%u:%s): filetype_option '%c'", strlen32(filename), filename, p_load_file_options->filetype_option);

    if(PD4_CHART_CHAR == p_load_file_options->filetype_option)
        return(pdchart_load_isolated_chart(filename));

    if(p_load_file_options->inserting)
    {
        assert(is_current_document());
        ok = TRUE;
    }
    else
    {
        init_open_box_sizes();

        if(!p_load_file_options->temp_file)
            if(!same_name_warning(filename, load_supporting_Zs_YN_S_STR, load_supporting_YN_Q_STR))
                return(FALSE);

        { /* now go via DOCU_NAME to start off on the right foot */
        DOCU_NAME docu_name;

        name_init(&docu_name);

        if(p_load_file_options->temp_file)
            ok = mystr_set(&docu_name.leaf_name, get_untitled_document());
        else
            ok = name_read_tstr(&docu_name, p_load_file_options->document_name);

        if(ok)
        {
            BOOL will_be_loaded_from_path = name_preprocess_docu_name_flags_for_rename(&docu_name);

            ok = create_new_document(&docu_name);

            if(ok)
            {
                xf_loaded_from_path = (BOOLEAN) will_be_loaded_from_path;

                /* rename_document(&docu_name); no need now */
            }
        }

        name_free(&docu_name);
        } /*block*/
    }

    if(ok)
    {
        ok = loadfile_core(filename, p_load_file_options);

        if(ok)
        {
            xf_draweverything = TRUE;

            if(!p_load_file_options->inserting)
                get_open_box_sizes();

            draw_screen(); /* AFTER the get_open_box_sizes furtling (avoids doing it twice) */

            if(!been_error)
            {
                ev_todo_add_doc_dependents(current_docno());

                if(0 != loadfile_recurse_load_supporting_documents(filename))
                    riscos_front_document_window(in_execfile);
            }
        }
        else
            destroy_current_document();
    }

    if(!ok)
    {
        /* maybe old document destroyed! */
        P_DOCU p_docu;
        if((NO_DOCUMENT != (p_docu = p_docu_from_docno(docno)))  &&  !docu_is_thunk(p_docu))
            select_document(p_docu);
    }

    return(ok);
}

extern U32
loadfile_recurse_load_supporting_documents(
    _In_z_      PCTSTR filename)
{
    U32 n_loaded = 0;
    EV_DOCNO docno_array[DOCNO_MAX];
    EV_DOCNO cur_docno;
    P_EV_DOCNO p_docno;
    S32 i, n_doc;

    n_doc = ev_doc_get_sup_docs_for_sheet(current_docno(), &cur_docno, docno_array);

    reportf("There may be %d supporting document(s) after loading %u:%s", n_doc, strlen32(filename), filename);

    for(i = 0, p_docno = docno_array; i < n_doc && !been_error; ++i, ++p_docno)
    {
        P_DOCU p_docu = p_docu_from_docno(*p_docno);
        P_SS_DOC p_ss_doc;
        TCHARZ sup_doc_filename[BUF_MAX_PATHSTRING];
        BOOL path_name_supplied;

        if(NO_DOCUMENT == p_docu)
        {
            assert(NO_DOCUMENT != p_docu);
            continue;
        }

        p_ss_doc = &p_docu->ss_instance_data.ss_doc;

        name_make_wholename(&p_ss_doc->docu_name, sup_doc_filename, elemof32(sup_doc_filename));

        path_name_supplied = p_ss_doc->docu_name.flags.path_name_supplied;

        reportf("  sup-doc #%d: docno=%u loaded=%s path_name_supplied=%s %u:%s",
                i, *p_docno, report_boolstring(!docu_is_thunk(p_docu)), report_boolstring(path_name_supplied), strlen32(sup_doc_filename), sup_doc_filename);
    }

    for(i = 0, p_docno = docno_array; i < n_doc && !been_error; ++i, ++p_docno)
    {
        P_DOCU p_docu = p_docu_from_docno(*p_docno);
        P_SS_DOC p_ss_doc;
        TCHARZ sup_doc_filename[BUF_MAX_PATHSTRING];
        BOOL path_name_supplied;
        STATUS filetype_option;

        if(NO_DOCUMENT == p_docu)
        {
            assert(NO_DOCUMENT != p_docu);
            continue;
        }

        if(!docu_is_thunk(p_docu))
            continue; /* supporting document is already loaded */

        p_ss_doc = &p_docu->ss_instance_data.ss_doc;

        name_make_wholename(&p_ss_doc->docu_name, sup_doc_filename, elemof32(sup_doc_filename));

        path_name_supplied = p_ss_doc->docu_name.flags.path_name_supplied;

        reportf("Trying to load sup-doc #%d: docno=%u %u:%s", i, *p_docno, strlen32(sup_doc_filename), sup_doc_filename);

        { /* check the supporting document file exists and can be read (and while we're at it, suss file type as we need that for load anyway) */
        TCHARZ fullname[BUF_MAX_PATHSTRING];

        addcurrentdir(fullname, elemof32(fullname), sup_doc_filename);

        filetype_option = find_filetype_option(fullname, FILETYPE_UNDETERMINED);
        } /*block*/

        if(filetype_option > 0)
        {
            LOAD_FILE_OPTIONS load_file_options;
            load_file_options_init(&load_file_options, sup_doc_filename, filetype_option);
            (void) loadfile_recurse(sup_doc_filename, &load_file_options);
            n_loaded++;
        }
        else if(!path_name_supplied) /* try Library / along UserPath load? */
        {
            BOOL try_path;

            { /* don't recurse with too much on the stack */
            char leaf_name[BUF_MAX_PATHSTRING];

            /* copy out so that it doesn't get overwritten */
            xstrkpy(leaf_name, elemof32(leaf_name), file_leafname(sup_doc_filename));

            try_path = status_done(add_path_using_dir(sup_doc_filename, elemof32(sup_doc_filename), leaf_name, EXTREFS_SUBDIR_STR));
            } /*block*/

            if(try_path)
            {
                filetype_option = find_filetype_option(sup_doc_filename, FILETYPE_UNDETERMINED);

                if(filetype_option > 0)
                {
                    LOAD_FILE_OPTIONS load_file_options;
                    load_file_options_init(&load_file_options, sup_doc_filename, filetype_option);
                    (void) loadfile_recurse(sup_doc_filename, &load_file_options);
                    n_loaded++;
                }
            }

            if(0 == filetype_option)
                xstrkpy(sup_doc_filename, elemof32(sup_doc_filename), p_ss_doc->docu_name.leaf_name); /* restore a better filename for error */
        }

        if(0 == filetype_option)
            messagef(reperr_getstr(ERR_SUPPORTING_DOC_NOT_FOUND), sup_doc_filename); /* don't abort loading loop */

        select_document_using_docno(cur_docno);
    }

    return(n_loaded);
}

/******************************************************************************
*
* Enumerate all files in subdirectory 'subdir' found
* in the same directory as the current document (if there is one)
* and also in the directories listed by the PipeDream$Path variable.
*
******************************************************************************/

_Check_return_
static STATUS
enumerate_dir_to_list_common(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR subdir,
    _InVal_     FILETYPE_RISC_OS filetype)
{
    P_FILE_OBJENUM enumstrp;
    P_FILE_OBJINFO infostrp;
    STATUS res_error = 0;

    for(infostrp = file_find_first_subdir(&enumstrp, path, FILE_WILD_MULTIPLE_STR, subdir);
        infostrp;
        infostrp = file_find_next(&enumstrp))
    {
        if(file_objinfo_type(infostrp) != FILE_OBJECT_FILE)
            continue;

        if((FILETYPE_UNDETERMINED == filetype) || (file_objinfo_filetype(infostrp) == filetype))
        {
            char leafname[BUF_MAX_PATHSTRING]; /* SKS 26oct96 now cater for long leaf names (was BUF_MAX_LEAFNAME) */

            file_objinfo_name(infostrp, leafname, elemof32(leafname));
            reportf("enumerate_dir_to_list(subdir=%s) matched file %s", report_tstr(subdir), leafname);

            status_break(res_error = add_to_list(list, (*p_key)++, leafname));
        }
    }

    return(res_error);
}

/*ncr*/
extern STATUS
enumerate_dir_to_list_using_path(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _In_opt_z_  PCTSTR doc_subdir /*maybe NULL*/,
    _In_z_      PCTSTR path,
    _In_opt_z_  PCTSTR path_subdir /*maybe NULL*/,
    _InVal_     FILETYPE_RISC_OS filetype)
{
    if( (NULL != doc_subdir) && is_current_document() )
    {
        TCHAR doc_path[BUF_MAX_PATHSTRING];

        if(NULL != file_get_cwd(doc_path, elemof32(doc_path), currentfilename()))
        {
            status_return(enumerate_dir_to_list_common(list, p_key, doc_path, doc_subdir, filetype));
        }
    }

    return(enumerate_dir_to_list_common(list, p_key, path, path_subdir, filetype));
}

/*ncr*/
extern STATUS
enumerate_dir_to_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _In_opt_z_  PC_U8Z subdir /*maybe NULL*/,
    _InVal_     FILETYPE_RISC_OS filetype)
{
#if 1
    /* relative to current document first (if there is one), then use the path */
    return(enumerate_dir_to_list_using_path(list, p_key, subdir, file_get_search_path(), subdir, filetype));
#else
    P_FILE_OBJENUM     enumstrp;
    P_FILE_OBJINFO     infostrp;
    U8                 combined_path[BUF_MAX_PATHSTRING * 4];
    STATUS             res_error = 0;

    /* SKS after PD 4.11 03feb92 - why was this only file_get_path() before? */
    file_combine_path(combined_path, elemof32(combined_path), is_current_document() ? currentfilename() : NULL, file_get_search_path());

    for(infostrp = file_find_first_subdir(&enumstrp, combined_path, FILE_WILD_MULTIPLE_STR, subdir);
        infostrp;
        infostrp = file_find_next(&enumstrp))
    {
        if(file_objinfo_type(infostrp) != FILE_OBJECT_FILE)
            continue;

        if((FILETYPE_UNDETERMINED == filetype) || (file_objinfo_filetype(infostrp) == filetype))
        {
            char leafname[BUF_MAX_PATHSTRING]; /* SKS 26oct96 now cater for long leaf names (was BUF_MAX_LEAFNAME) */

            file_objinfo_name(infostrp, leafname, elemof32(leafname));
            reportf("enumerate_dir_to_list(subdir=%s) matched file %s", report_tstr(subdir), leafname);

            status_break(res_error = add_to_list(list, (*p_key)++, leafname));
        }
    }

    return(res_error);
#endif
}

/******************************************************************************
*
* open the load file and have a peek to see what type it is
*
******************************************************************************/

#define BYTES_TO_SEARCH 1024

_Check_return_
static BOOL
reject_this_filetype(
    _InVal_     FILETYPE_RISC_OS filetype)
{
    switch(filetype)
    {
    case FILETYPE_PNG:
    case FILETYPE_MS_XLS:
    case FILETYPE_DATAPOWER:
    case FILETYPE_DATAPOWERGPH:
    case FILETYPE_RTF:
        /* we know that it's not sensible to try to load these ones */
        return(TRUE);

    case FILETYPE_LOTUS123:
        /* can only load these ones if suitable converter is available */
        return(!u_ltp_available());

    default:
        return(FALSE);
    }
}

_Check_return_
static int
try_memcmp32(
    _In_reads_bytes_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _In_reads_bytes_(n_bytes_compare) PC_BYTE p_data_compare,
    _InVal_     U32 n_bytes_compare)
{
    if(n_bytes < n_bytes_compare)
        return(-1); /* not enough data to compare with pattern */

    return(memcmp32(p_data, p_data_compare, n_bytes_compare));
}

_Check_return_
static FILETYPE_RISC_OS
filetype_from_data(
    _In_reads_bytes_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes)
{
    static const BYTE buffer_pipedream[]    = { '%',    'O',    'P',    '%' };
    static const BYTE buffer_pipedream_2[]  = { '%',    'C',    'O',    ':' };
    static const BYTE buffer_acorn_draw[]   = { 'D',    'r',    'a',    'w',
                                                '\xC9', '\x00', '\x00', '\x00' };
    static const BYTE buffer_t5_draw_0C[]   = { 'F',    'w',    'k',    'z',
                                                'H',    'y',    'b',    'r',
                                                'i',    'd',    ' ',    ' ' };
    static const BYTE buffer_pd_chart_0C[]  = { 'P',    'D',    'r',    'e',
                                                'a',    'm',    'C',    'h',
                                                'a',    'r',    't',    's' };
    static const BYTE buffer_jfif_00[]      = { '\xFF', '\xD8', /*SOI*/ '\xFF', '\xE0' /*APP0*/ };
    static const BYTE buffer_jfif_06[]      = { 'J',    'F',    'I',    'F',    '\x00' /*JFIF*/ };
    static const BYTE buffer_dib_00[]       = { 'B',    'M' };
    static const BYTE buffer_dib_0E[]       = { '\x28', '\x00', '\x00', '\x00' /*BITMAPINFOHEADER*/ };
    static const BYTE buffer_dibv4_0E[]     = { '\x6C', '\x00', '\x00', '\x00' /*BITMAPV4HEADER*/ };
    static const BYTE buffer_dibv5_0E[]     = { '\x7C', '\x00', '\x00', '\x00' /*BITMAPV5HEADER*/ };
    static const BYTE buffer_fireworkz[]    = { '{',    'V',    'e',    'r',    's',    'i',    'o',    'n',   ':' };
    static const BYTE buffer_acorn_sprite[] = { '\x01', '\x00', '\x00', '\x00', '\x10', '\x00', '\x00', '\x00' }; /* assumes only one sprite and no extension area */
    static const BYTE buffer_compound_file[]= { '\xD0', '\xCF', '\x11', '\xE0', '\xA1', '\xB1', '\x1A', '\xE1' };
    static const BYTE buffer_excel_biff4w[] = { '\x09', '\x04', '\x06', '\x00', '\x00', '\x04', '\x00', '\x01' };
    static const BYTE buffer_excel_biff4[]  = { '\x09', '\x04', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_excel_biff3[]  = { '\x09', '\x02', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_excel_biff2[]  = { '\x09', '\x00', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_png[]          = { '\x89', 'P',    'N',    'G',    '\x0D', '\x0A', '\x1A', '\x0A' };
    static const BYTE buffer_gif87a[]       = { 'G',    'I',    'F',    '8',    '7',    'a' };
    static const BYTE buffer_gif89a[]       = { 'G',    'I',    'F',    '8',    '9',    'a' };
    static const BYTE buffer_rtf[]          = { '{',    '\\',   'r',    't',    'f' };
    static const BYTE buffer_tiff_LE[]      = { 'I',    'I',    '*',    '\x00' };
    static const BYTE buffer_tiff_BE[]      = { 'M',    'M',    '\x00', '*'    };
    static const BYTE buffer_lotus_wk1[]    = { '\x00', '\x00', '\x02', '\x00' /*BOF*/ };
    static const BYTE buffer_lotus_wk3[]    = { '\x00', '\x00', '\x1A', '\x00' /*BOF*/ };
    static const BYTE buffer_acorn_sid[]    = { '%',    '%' };

    FILETYPE_RISC_OS filetype = FILETYPE_UNDETERMINED;

    if(0 == try_memcmp32(p_data, n_bytes, buffer_pipedream, sizeof32(buffer_pipedream)))
        return(FILETYPE_PIPEDREAM);
    if(0 == try_memcmp32(p_data, n_bytes, buffer_pipedream_2, sizeof32(buffer_pipedream_2)))
        return(FILETYPE_PIPEDREAM);

    if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_draw, sizeof32(buffer_acorn_draw)))
    {
        if( (n_bytes > (0x0C + sizeof32(buffer_t5_draw_0C))) &&
            (0 == try_memcmp32(&p_data[0x0C], n_bytes - 0x0C, buffer_t5_draw_0C, sizeof32(buffer_t5_draw_0C))) )
            return(FILETYPE_T5_HYBRID_DRAW);

        if( (n_bytes > (0x0C + sizeof32(buffer_pd_chart_0C))) &&
            (0 == try_memcmp32(&p_data[0x0C], n_bytes - 0x0C, buffer_pd_chart_0C, sizeof32(buffer_pd_chart_0C))) )
            return(FILETYPE_PD_CHART);

        return(FILETYPE_DRAW);
    }

    if( (n_bytes > (0x06 + sizeof32(buffer_jfif_06))) &&
        (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_jfif_00, sizeof32(buffer_jfif_00))) &&
        (0 == try_memcmp32(&p_data[0x06], n_bytes - 0x06, buffer_jfif_06, sizeof32(buffer_jfif_06))) )
        return(FILETYPE_JPEG);

    if( (n_bytes > (0x0E + sizeof32(buffer_dib_0E))) &&
        (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00, sizeof32(buffer_dib_00))) &&
        (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dib_0E, sizeof32(buffer_dib_0E))) )
        return(FILETYPE_BMP);

    if( (n_bytes > (0x0E + sizeof32(buffer_dibv4_0E))) &&
        (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00,   sizeof32(buffer_dib_00)))   &&
        (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dibv4_0E, sizeof32(buffer_dibv4_0E))) )
        return(FILETYPE_BMP);

    if( (n_bytes > (0x0E + sizeof32(buffer_dibv5_0E))) &&
        (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00,   sizeof32(buffer_dib_00)))   &&
        (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dibv5_0E, sizeof32(buffer_dibv5_0E))) )
        return(FILETYPE_BMP);

    if(0 == try_memcmp32(p_data, n_bytes, buffer_fireworkz, sizeof32(buffer_fireworkz)))
        filetype = FILETYPE_T5_FIREWORKZ;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_sprite, sizeof32(buffer_acorn_sprite)))
        filetype = FILETYPE_SPRITE;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_compound_file, sizeof32(buffer_compound_file)))
        filetype = FILETYPE_MS_XLS; /*detect_compound_document_file(file_handle);*/
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff4w, sizeof32(buffer_excel_biff4w)))
        filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff4, sizeof32(buffer_excel_biff4)))
        filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff3, sizeof32(buffer_excel_biff3)))
        filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff2, sizeof32(buffer_excel_biff2)))
        filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_png, sizeof32(buffer_png)))
        filetype = FILETYPE_PNG;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_gif87a, sizeof32(buffer_gif87a)))
        filetype = FILETYPE_GIF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_gif89a, sizeof32(buffer_gif89a)))
        filetype = FILETYPE_GIF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_rtf, sizeof32(buffer_rtf)))
        filetype = FILETYPE_RTF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_tiff_LE, sizeof32(buffer_tiff_LE)))
        filetype = FILETYPE_TIFF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_tiff_BE, sizeof32(buffer_tiff_BE)))
        filetype = FILETYPE_TIFF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_lotus_wk1, sizeof32(buffer_lotus_wk1)))
        filetype = FILETYPE_LOTUS123;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_lotus_wk3, sizeof32(buffer_lotus_wk3)))
        filetype = FILETYPE_LOTUS123;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_sid, sizeof32(buffer_acorn_sid)))
        filetype = FILETYPE_SID;

    return(filetype);
}

/* we know that name is not null */

extern S32 /* filetype_option, 0 or error (reported) */
find_filetype_option(
    _In_z_      PCTSTR filename,
                FILETYPE_RISC_OS filetype)
{
    FILE_HANDLE input;
    char array[BYTES_TO_SEARCH];
    S32 res;
    S32 size;
    char *ptr, *start_of_line;
    char ch;
    S32 comma_values;
    BOOL in_number, in_string;
    COL dummy_col = 0;
    ROW dummy_row = 0;
    uchar dch;
    BOOL dbool;
    S32 filetype_option;

    /* do we know that we can't handle it? */
    if(reject_this_filetype(filetype))
        return(ERR_CANT_LOAD_FILETYPE);

    /* centralise handling for these just to check readability */
    switch(filetype)
    {
    case FILETYPE_CSV:
    case FILETYPE_LOTUS123:
        if((filetype_option = file_readable(filename)) > 0)
            filetype_option = (FILETYPE_CSV == filetype) ? CSV_CHAR : LOTUS123_CHAR;
        return(filetype_option);

    default:
        break;
    }

    input = pd_file_open(filename, file_open_read);

    if(!input)
        return(CH_NULL); /* not an error at this level */

    if(FILETYPE_UNDETERMINED == filetype)
    {
        filetype = file_get_type(input);

        /* do we now know that we can't handle it? */
        if(reject_this_filetype(filetype))
        {
            filetype_option = ERR_CANT_LOAD_FILETYPE;
            goto endpoint;
        }
    }

    size = pd_file_read(array, 1, BYTES_TO_SEARCH, input);

    reportf("find_filetype_option: read %d bytes from file, requested %d", size, BYTES_TO_SEARCH);

    /* error in reading? */
    if(size == -1)
    {
        filetype_option = -1;
        goto endpoint;
    }

    /* ensure terminated at a suitable place */
    if(size == BYTES_TO_SEARCH)
        --size;
    array[size] = CH_NULL;

    /* is it a chart file? search early or may get confused with just normal Draw file */
    res = image_cache_fileheader_is_chart(array, size);
    if(res != 0)
    {
        filetype_option = PD4_CHART_CHAR;
        goto endpoint;
    }

    if(vsload_fileheader_isvsfile(array, size))
    {
        trace_0(TRACE_APP_PD4, "ViewSheet file");
        filetype_option = VIEWSHEET_CHAR;
        goto endpoint;
    }

    /* test for 1WP file */
    if(fwp_isfwpfile(array))
    {
        trace_0(TRACE_APP_PD4, "1wp file");
        filetype_option = FWP_CHAR;
        goto endpoint;
    }

    /* ignore filetype we've got to this point and scan the buffer for other known patterns */
    filetype = filetype_from_data(array, size);

    /* do we now know that we can't handle it? */
    if(reject_this_filetype(filetype))
    {
        filetype_option = ERR_CANT_LOAD_FILETYPE;
        goto endpoint;
    }

    /* mutate this one into one we know we can handle - user will just have to ignore top row */
    if(FILETYPE_SID == filetype)
    {
        filetype_option = CSV_CHAR;
        goto endpoint;
    }

    ptr = array;
    start_of_line = array;
    comma_values = 0;
    in_number = FALSE;

    do  {
        ch = *ptr++;

        trace_2(TRACE_APP_PD4, "read char %c (%d)", ch, ch);

        if(ch == '%')
        {
            /* if it's a PipeDream construct return PD */
            in_option = -1;

            if( (lukcon(ptr - 1, &dummy_col, &dummy_row,
                       &dch, &dch, &dch, dummy_col, dummy_row,
                       &dch, &dbool, &dummy_row, FALSE, &dbool, FALSE) == 1) ||
                (in_option > -1))
            {
                /* if anyone needs to know what the version number of the PipeDream
                   that saved the file is, here's the place to do it.
                   Version number comes after %OP%VS
                */
                if(0 == memcmp(ptr, "OP%VS", 5))
                {
                    trace_0(TRACE_APP_PD4, "PipeDream 4 file found (versioned)");
                    filetype_option = PD4_CHAR;
                }
                else
                {
                    trace_0(TRACE_APP_PD4, "PipeDream 4 file found (assumed, might be PD3 or PD4-transitional)");
                    filetype_option = PD4_CHAR;
                }

                goto endpoint;
            }

            continue;
        }

        switch(ch)
        {
        case CTRLZ: /* MS-DOS end of file marker? */
        case TAB:
            trace_0(TRACE_APP_PD4, "Tab file found");
            filetype_option = TAB_CHAR;
            goto endpoint;

        case CR:
        case LF:
            /* if we complete a line full of only comma type options, ok */
            if(ptr - start_of_line > 1)
                if(comma_values > 1)
                {
                    trace_0(TRACE_APP_PD4, "Line of valid comma options: CSV");
                    filetype_option = CSV_CHAR;
                    goto endpoint;
                }

            comma_values = 0;
            start_of_line = ptr;

            break;

        case SPACE:
            if(in_number)
            {
                /* numbers followed by one or more spaces then comma tend to be CSV */
                do  {
                    ch = *ptr++;
                    trace_2(TRACE_APP_PD4, "read char %c in skipspaces (%d)", ch, ch);
                }
                while(ch == SPACE);
                --ptr;

                if(ch != COMMA)
                    in_number = FALSE;
            }

            break;

        case COMMA:
            if(in_number)
            {
                /* if we hit a comma whilst in a number, score one more for the CSV's */
                in_number = FALSE;
                ++comma_values;
            }
            else if(*ptr == COMMA)
                /* adjacent commas are a good sign of CSV format */
                ++comma_values;

            break;

        case QUOTES:
            in_number = FALSE;
            in_string = TRUE;

            do  {
                ch = *ptr++;

                trace_2(TRACE_APP_PD4, "read char %c in string read (%d)", ch, ch);

                switch(ch)
                {
                case CR:
                case LF:
                case CH_NULL:
                    /* faulty quoted string */
                    trace_0(TRACE_APP_PD4, "Tab file as quoted string was faulty");
                    filetype_option = TAB_CHAR;
                    goto endpoint;

                case QUOTES:
                    if(*ptr != QUOTES)
                    {
                        /* quoted string ended - one more kosher CSV element */
                        in_string = FALSE;
                        ++comma_values;
                    }
                    else
                        trace_0(TRACE_APP_PD4, "escaped quote");
                    break;

                default:
                    break;
                }
            }
            while(in_string);

            break;

        /* VIEW stored command? */
        case VIEW_STORED_COMMAND:
            in_number = FALSE;
            comma_values = 0;

            if(isupper(*ptr)  &&  isupper(*(ptr+1)))
            {
                trace_0(TRACE_APP_PD4, "VIEW stored command");
                filetype_option = VIEW_CHAR;
                goto endpoint;
            }

            break;

        /* VIEW ruler? */
        case VIEW_RULER:
            in_number = FALSE;
            comma_values = 0;

            if((*ptr == '.')  &&  (*(ptr+1) == '.'))
            {
                trace_0(TRACE_APP_PD4, "VIEW ruler");
                filetype_option = VIEW_CHAR;
                goto endpoint;
            }

            break;

        default:
            if(isdigit(ch)  ||  (ch == '.')  ||  (ch == '+')  ||  (ch == '-')  ||  (ch == 'e')  ||  (ch == 'E'))
                in_number = TRUE;
            else
            {
                in_number = FALSE;
                comma_values = 0;
            }

            break;
        }
    }
    while(ch);

    /* nothing recognized */

    trace_0(TRACE_APP_PD4, "End of BYTES_TO_SEARCH/EOF - so call it Tab");

    filetype_option = TAB_CHAR;

endpoint:;

    pd_file_close(&input);

    return(filetype_option);
}

/******************************************************************************
*
*  return the RISC OS filetype from our letter options
*
******************************************************************************/

#define FILETYPE_PARAGRAPH FILETYPE_TEXT

extern FILETYPE_RISC_OS
rft_from_filetype_option(
    _InVal_     char filetype_option)
{
    FILETYPE_RISC_OS res;

    switch(filetype_option)
    {
    case CSV_CHAR:
        res = FILETYPE_CSV;
        break;

    case VIEW_CHAR:
        res = FILETYPE_VIEW;
        break;

    case FWP_CHAR:
        res = FILETYPE_FWP;
        break;

    case PARAGRAPH_CHAR:
        res = FILETYPE_PARAGRAPH;
        break;

    case TAB_CHAR:
        res = FILETYPE_TEXT;
        break;

    case PD4_CHART_CHAR:
        res = gr_chart_save_as_filetype();
        break;

    /* case VIEWSHEET_CHAR: */
    /* case PD4_CHAR: */
    default:
        res = FILETYPE_PIPEDREAM;
        break;
    }

    trace_2(TRACE_APP_PD4, "rft_from_filetype_option('%c') yields &%3.3X", filetype_option, res);
    return(res);
}

/******************************************************************************
*
******************************************************************************/

extern FILETYPE_RISC_OS
currentfiletype(
    _InVal_     char filetype_option)
{
    if(TAB_CHAR != filetype_option)
        return(rft_from_filetype_option(filetype_option));

    if(TAB_CHAR == current_filetype_option)
        return((FILETYPE_RISC_OS) ((currentfileinfo.load >> 8) & 0xFFF));

    return(FILETYPE_TEXT);
}

/*
munges input character for CSV loading
returns 0 if dealt with character completely, else new char
*/

static S32
comma_convert(
    S32 c,
    FILE_HANDLE loadinput,
    uchar *lastch,
    uchar *typep,
    uchar *justifyp,
    BOOL *in_quotes)
{
    switch(c)
    {
    case QUOTES:
        *typep = SL_TEXT;
        *justifyp = J_FREE;

        if(lecpos == 0)
        {
            /* RJM changes this on 16.1.92 so that ,"", is understood correctly */
            /* *in_quotes = TRUE; */
            *in_quotes = !*in_quotes;

            return(0);
        }

        if(!*in_quotes)
        {
            *lastch = linbuf[lecpos++] = QUOTES;
            return(0);
        }

        if((c = pd_file_getc(loadinput)) < 0)
            return(c);

        /* continue with new character */
        break;

    case COMMA:
        if(*in_quotes)
        {
            *lastch = linbuf[lecpos++] = (uchar) c;
            return(0);
        }

        break;

    default:
        break;
    }

    return(c);
}

/******************************************************************************
*
* actually read the file
* (which must exist and be readable by now)
* into the current document
*
******************************************************************************/

static BOOL
loadfile_core(
    _In_z_      PC_U8Z filename,
    _InRef_     P_LOAD_FILE_OPTIONS p_load_file_options)
{
    P_U8Z name = (P_U8Z) filename;
    S32 c = 0;
    uchar ch, lastch = 0;
    COL tcol;
    ROW trow;
    SLR first;
    BOOL something_on_this_line = FALSE;
    BOOL breakout = FALSE;
    BOOL ctrl_chars = FALSE;
    BOOL first_column = TRUE;
    ROW row_in_file = 0;
    ROW row_range_start = 0; /* start of row range */
    ROW row_range_end = LARGEST_ROW_POSSIBLE; /* end of row range */
    BOOL plaintext;
    U8 field_separator;
    U8 line_sep_option = LINE_SEP_LF;
    U32 flength;
    U32 chars_read = 0;
    U32 chars_read_trigger_actind = 0;
    FILE_HANDLE loadinput;
    BOOL outofmem = FALSE;
    BOOL been_ruler = FALSE;
    S32 vsrows = -1; /* dataflower */
    /* 1WP highlights have to be reinstated at the start of each cell */
    uchar h_byte = FWP_NOHIGHLIGHTS;
    COL insert_col = -1; /* dataflower */
    ROW insert_row = -1; /* ditto */
    ROW insert_numrow = -1;
    S32    thiswrapwidth = get_right_margin(0);
    char * paragraph_saved_word = NULL;
    COL    paragraph_in_column = -1;

    /* find the data format */
    switch(p_load_file_options->filetype_option)
    {
    case PD4_CHAR:
        plaintext = FALSE;
        field_separator = CH_NULL;
        break;

    case VIEWSHEET_CHAR:
        plaintext = TRUE;
        field_separator = TAB;
        break;

    case CSV_CHAR:
        plaintext = TRUE;
        field_separator = COMMA;
        break;

    /* case TAB_CHAR: */
    /* case VIEW_CHAR: */
    /* case FWP_CHAR: */
    default:
        plaintext = TRUE;
        field_separator = TAB;
        break;
    }

    /* are we loading a range of rows? */
    if(NULL != p_load_file_options->row_range)
    {
        buff_sofar = (uchar *) p_load_file_options->row_range;
        row_range_start = (ROW) getsbd()-1;
        row_range_end   = (ROW) getsbd()-1;

        if( (row_range_start < 0)    ||
            (row_range_end   < 0)    ||
            (row_range_end < row_range_start))
            return(reperr_null(ERR_BAD_RANGE));
    }

    /* are we inserting? */
    if(p_load_file_options->inserting)
    {
        /* insert off to the right, don't update references during load,
         * bodge references of inserted bit afterwards, and move in to where
         * it's supposed to be.
        */
        first.col = numcol;
        first.row = BLOCK_UPDREF_ROW;

        insert_col = curcol;
        insert_row = currow;

        if(NULL != p_load_file_options->insert_at_slot)
        {
            buff_sofar = (uchar *) p_load_file_options->insert_at_slot;
            insert_col = getcol();
            insert_row = (ROW) getsbd()-1;
        }

        if(bad_reference(insert_col, insert_row))
            return(reperr_null(ERR_BAD_CELL));
    }
    else  /* overwriting existing(?) file */
    {
        current_filetype_option = p_load_file_options->filetype_option;

        first.col = (COL) 0;
        first.row = (ROW) 0;

        if(plaintext)
        {
            dftcol(); /* >>>>>> may now fail */

            if(VIEWSHEET_CHAR == p_load_file_options->filetype_option) /* can't save as ViewSheet, suggest save as PipeDream */
                current_filetype_option = PD4_CHAR;

            switch(p_load_file_options->filetype_option)
            {
            case FWP_CHAR:
            case VIEW_CHAR:
            case PARAGRAPH_CHAR: /* SKS after PD 4.11 08jan92 - stop paragraph loading turning wrap off */
                break;

            default:
                /* turn off wrap in plain text documents e.g. ASCII, CSV, VIEWSHEET(!) */
                d_options_WR = 'N';
                break;
            }
        }
        else
        {
            /* PD files build their own column table */
            killcoltab();

            /* SKS, RJM 1.9.91 */
            update_all_dialog_from_windvars();
            dialog_hardwired_defaults();
            update_all_windvars_from_dialog();
        }

        riscos_readfileinfo(&currentfileinfo, name);
        trace_2(TRACE_APP_PD4, "fileinfo: load = %8.8X exec = %8.8X", currentfileinfo.load, currentfileinfo.exec);

        if(TAB_CHAR != p_load_file_options->filetype_option)
            riscos_settype(&currentfileinfo, rft_from_filetype_option(p_load_file_options->filetype_option));
    }

    tcol = first.col;
    trow = first.row;

    /* open the file and buffer it */
    if(NULL == (loadinput = pd_file_open(filename, file_open_read)))
        return(reperr(ERR_CANNOTOPEN, filename));

    (void) file_buffer(loadinput, NULL, 16*1024); /* no messing about for load */

    flength = file_length(loadinput);
    /* we're going to divide by this */
    if(!flength)
        flength = 1;

    reportf("loadfile_core: reading data from %u:%s, length=%d", strlen32(filename), filename, flength);

    switch(p_load_file_options->filetype_option)
    {
    default:
        break;

    case FWP_CHAR:
        fwp_load_preinit(loadinput, p_load_file_options->inserting);
        break;

    case VIEW_CHAR:
        view_load_preinit(&been_ruler, p_load_file_options->inserting);
        break;

    case VIEWSHEET_CHAR:
        vsrows = vsload_loadvsfile(loadinput);

        if(vsrows < 0)
        {
            pd_file_close(&loadinput);
            return(reperr_null(vsrows));
        }

        break;
    }

    (void) init_dialog_box(D_FIXES);

    (void) init_dialog_box(D_FONTS);

    in_load = TRUE;

    escape_enable();

    /* read each cell from the file */

    if(VIEWSHEET_CHAR == p_load_file_options->filetype_option)
        viewsheet_load_core(vsrows, p_load_file_options->inserting, &insert_numrow, &breakout);
    else

    /* each cell */
    while(c >= 0)
    {
        BOOL in_construct = FALSE;
        uchar type = SL_TEXT;
        uchar justify = J_FREE;
        char pageoffset = 0;
        uchar format = 0;
        BOOL in_quotes = FALSE;

        in_option = -1;
        start_of_construct = 0;

        if(CSV_CHAR == p_load_file_options->filetype_option)
        {
            type = SL_NUMBER;
            justify = J_RIGHT;
        }

        chars_read = file_tell(loadinput); /* this is usually very fast */

        if(chars_read >= chars_read_trigger_actind)
        {
            U32 percent = ((100U * chars_read) / flength);

            actind(percent);

            /* set chars_read_trigger_actind to next percent */
            chars_read_trigger_actind = (flength * (percent + 1)) / 100U;
        }

        if(ctrlflag)
        {
            breakout = 1;
            break;
        }

        if(PD4_CHAR == p_load_file_options->filetype_option) /* optimise for PipeDream load */
        {
            /* begin PipeDream format read processing for this cell */
            lecpos = 0;

            do  {
                if(file_getc_fast_ready(loadinput))
                    c = file_getc_fast(loadinput);
                else
                    c = pd_file_getc(loadinput);

                /* check for control chars (and error from getc) */
                if(c < 0x20)
                {
                    if(c < 0)
                        break;

                    /* ignore CR or LF if previous char was LF or CR */
                    if((c == CR)  ||  (c == LF))
                    {
                        if((c + lastch) == (CR + LF))
                        {
                            lastch = 0;
                            continue;
                        }

                        lastch = (uchar) c;

                        break; /* either CR or LF marks end of cell */
                    }
                    else
                    {
                        ctrl_chars = TRUE;
                        continue;
                    }
                }

                lastch = ch = (uchar) c;

                /* add char to buffer */
                linbuf[lecpos++] = ch;

                if((ch == '%')  &&  (in_option < 0))
                {
                    if(!in_construct)
                    {
                        in_construct = TRUE;
                        start_of_construct = lecpos - 1; /* points at first % */
                        continue;
                    }

                    linbuf[lecpos] = CH_NULL;

                    if(lukcon(linbuf + start_of_construct, &tcol, &trow, &type,
                                &justify, &format, first.col, first.row,
                                &pageoffset, &outofmem, &row_in_file, TRUE, &first_column,
                                p_load_file_options->inserting) > 0)
                    {
                        lecpos = start_of_construct;
                        in_construct = FALSE;
                    }
                    else if(outofmem)
                        break;
                    else
                        start_of_construct = lecpos;
                }
            }
            while(lecpos < MAXFLD);

            /* end of PipeDream format read processing for this cell */
            }
        else
        {
            /* begin other format read processing for this cell */

            if(/*loading_paragraph &&*/ paragraph_saved_word && paragraph_in_column == tcol)
            {
                /* put the word at start of this line */
                strcpy(linbuf, paragraph_saved_word);
                lecpos = strlen(linbuf);
                consume_bool(mystr_set(&paragraph_saved_word, NULL));
            }
            else
                lecpos = 0;

            if(FWP_CHAR == p_load_file_options->filetype_option)
                fwp_change_highlights(h_byte, FWP_NOHIGHLIGHTS);

            do  {
                if(file_getc_fast_ready(loadinput))
                    c = file_getc_fast(loadinput);
                else
                    c = pd_file_getc(loadinput);

                /* check for control chars (and error from getc) */
                if(c < 0x20)
                {
                    if(c < 0)
                        break;

                    /* ignore CR or LF if previous char was LF or CR */
                    if((c == CR)  ||  (c == LF))
                    {
                        line_sep_option = (LF == c) ? LINE_SEP_LF : LINE_SEP_CR;

                        if((c + lastch) == (CR + LF))
                        {
                            line_sep_option = (LF == lastch) ? LINE_SEP_LFCR : LINE_SEP_CRLF;
                            lastch = 0;
                            continue;
                        }
                    }
                    else
                    {
                        switch(p_load_file_options->filetype_option)
                        {
                        default:
                            if(field_separator  &&  (c == field_separator)) /* allows TAB etc. through */
                                break;

                            ctrl_chars = TRUE;
                            continue;

                        case VIEW_CHAR: /* these (silently) prune control chars themselves during convert op */
                        case FWP_CHAR:
                            break;
                        }
                    }
                }

                switch(p_load_file_options->filetype_option)
                {
                case VIEW_CHAR:
                    c = view_convert(c, loadinput, &lastch, &type, &justify, &been_ruler, &pageoffset);
                    break;

                case FWP_CHAR:
                    c = fwp_convert(c, loadinput, &type, &justify, &field_separator, &h_byte, &pageoffset);
                    break;

                case CSV_CHAR:
                    c = comma_convert(c, loadinput, &lastch, &type, &justify, &in_quotes);
                    break;

                default:
                    break;
                }

                if(c <= 0)
                {
                    if(c < 0)
                        break;

                    continue;
                }

                lastch = ch = (uchar) c;

                /* end of cell? */
                if((ch == CR)  ||  (ch == LF))
                    break;

                if((ch == field_separator)  &&  field_separator /*plaintext*/)
                    break;

                /* add char to buffer */
                linbuf[lecpos++] = ch;

                /* have we gone beyond of the line? */
                if((lecpos > thiswrapwidth)  &&  (ch != SPACE)  &&  (PARAGRAPH_CHAR == p_load_file_options->filetype_option))
                {
                    /* find start of word and save word away for the next line */
                    uchar *mptr = linbuf+lecpos-1;

                    for(; *mptr != SPACE && mptr>linbuf; )
                        mptr--;

                    /* point mylecpos at start of word */
                    mptr++;

                    linbuf[lecpos] = CH_NULL;

                    consume_bool(mystr_set(&paragraph_saved_word, mptr));
                    paragraph_in_column = tcol;

                    /* write only up to start of word into cell */
                    lecpos = mptr-linbuf;
                    break;
                }
            }
            while(lecpos < MAXFLD);

            /* end of other format read processing for this cell */
        }

        if(outofmem)
            break;

        linbuf[lecpos] = CH_NULL;

        if(VIEW_CHAR == p_load_file_options->filetype_option)
            view_load_line_ended();

        if(in_option > -1)
        {
            if(!p_load_file_options->inserting)
                getoption(linbuf + in_option);
        }
        else
        {
            if((row_in_file >= row_range_start)  &&  (row_in_file <= row_range_end))
            {
                something_on_this_line = TRUE;

                /* if inserting, insert cell here */
                if(p_load_file_options->inserting  &&  ((c >= 0)  ||  (lecpos > 0)))
                {
                    if(!(insertslotat(tcol, trow)))
                        break;

                    /* need to know how deep inserted bit is */
                    if( insert_numrow < trow+1)
                        insert_numrow = trow+1;
                }

                /* if something to put in cell, put it in */
                if( ((lecpos > 0)  ||  (type == SL_PAGE))  &&
                    !stoslt(tcol, trow, type, justify, format, pageoffset, p_load_file_options->inserting, (PD4_CHAR == p_load_file_options->filetype_option)))
                {
                    breakout = TRUE;
                    break;
                }
            }

            /* plain text mode, move right */
            if(lastch == field_separator)
            {
                tcol++;
                if(PARAGRAPH_CHAR == p_load_file_options->filetype_option)
                    thiswrapwidth = get_right_margin(tcol);
            }
            else
            {
                row_in_file++; /* move down */

                if(something_on_this_line)
                {
                    something_on_this_line = FALSE;
                    trow++;
                }

                if(plaintext)
                {
                    /* if loading paragraphs and tripped up, just move down */
                    if(!paragraph_saved_word)
                        tcol = first.col;
                }
            }
        }
    }

    switch(p_load_file_options->filetype_option)
    {
    default:
        break;

    case VIEW_CHAR:
        view_load_postinit();
        break;

    case VIEWSHEET_CHAR:
        vsload_fileend();
        break;
    }

    pd_file_close(&loadinput);

    in_load = FALSE;

    if(!check_not_blank_sheet())
        breakout = TRUE;

    out_screen = out_rebuildvert = out_rebuildhorz = TRUE;

    if(p_load_file_options->inserting)
    {
        DOCNO old_blk_docno = blk_docno;
        SLR old_blkstart    = blkstart;
        SLR old_blkend      = blkend;
        COL width_of_new, first_blank, col;

        /* move block to insert_col, insert_row, insert_numrow rows deep */
        blk_docno    = current_docno();
        blkstart.col = first.col;
        blkstart.row = BLOCK_UPDREF_ROW;
        blkend.col   = numcol-1;
        blkend.row   = numrow-1;

        curcol = insert_col;
        currow = insert_row;

        switch(p_load_file_options->filetype_option)
        {
        default:
            block_updref(blk_docno, blk_docno, first.col, first.row, insert_col, insert_row);
            break;

        case PARAGRAPH_CHAR: /* always ignore in this case as paragraphing will have messed up any @cell@ refs */
        case TAB_CHAR: /* optionally in this case as we are very unlikely to want to updref inserted @cell@ refs (see Richard Torrens' request) */
            break;
        }

        /* if ran out of memory, throw away what's been loaded */
        if(breakout)
        {   /* second parameter added 19.8.91 by RJM for new block move code */
            do_delete_block(FALSE/*do_save*/, FALSE/*allow_widening*/, TRUE/*ignore_protection*/);
        }
        else
            MoveBlock_fn_do(TRUE);

        blk_docno = old_blk_docno;
        blkstart  = old_blkstart;
        blkend    = old_blkend;

        /* determine whether new bit makes sheet wider */
        width_of_new = numcol - first.col;
        if(width_of_new + curcol >= first.col)
            first_blank = width_of_new + curcol;
        else
            first_blank = first.col;

        /* delete spurious rows in new non-blank columns */
        for(col = first.col; col < first_blank; ++col)
        {
            travel(col, BLOCK_UPDREF_ROW - 1);
            blow_away(col, atrow(col), numrow - atrow(col));
        }

        /* delete all the columns at end */
        delcolentry(first_blank, numcol - first_blank);

        /* RJM, 21.11.91 */
        reset_numrow();
    }
    else
    {
        current_line_sep_option = line_sep_option;

        curcol = first.col;
        currow = first.row;

        internal_process_command(N_FirstColumn);
        internal_process_command(N_TopOfColumn);

        update_fontinfo_from_dialog();

        /* get the fixed rows and cols info and do something with it */

        if(!str_isblank(d_fixes[0].textfield))
        {
            /* rows */
            ROW firstrow = 0, nrows = 0;

            consume_int(sscanf(d_fixes[0].textfield, "%d,%d", &firstrow, &nrows));

            if(nrows > 0)
            {
                filvert((ROW) firstrow, (ROW) 0, FALSE);
                /* fix rows */
                currow = firstrow + nrows - 1;
                internal_process_command(N_FixRows);
                currow = 0;
            }
        }

        if(!str_isblank(d_fixes[1].textfield))
        {
            /* cols */
            int firstcol = 0, ncols = 0;

            consume_int(sscanf(d_fixes[1].textfield, "%d,%d", &firstcol, &ncols));

            if(ncols > 0)
            {
                filhorz((COL) firstcol, (COL) 0);
                /* fix cols */
                curcoloffset = ncols - 1;
                internal_process_command(N_FixColumns);
                curcol = 0;
            }
        }
    }

    /* updating cells - must do after possible insertion discard */
    clear_protect_list();
    clear_linked_columns(); /* set the linked columns */

    escape_disable();

    actind_end();

    /* update from d_progvars[] */
    recalc_state_may_have_changed();
    chart_recalc_state_may_have_changed();
    chart_format_state_may_have_changed();
    insert_overtype_state_may_have_changed();

    update_variables();

    filealtered(p_load_file_options->temp_file  ||  p_load_file_options->inserting);

    if(ctrl_chars)
        reperr_null(ERR_CTRL_CHARS);

    return(!breakout);
}

static void
viewsheet_load_core(
    _In_        S32 vsrows,
    _InVal_     BOOL inserting,
    _InoutRef_  P_ROW p_insert_numrow,
    _InoutRef_  P_BOOL p_breakout)
{
    S32 col, row;

    trace_0(TRACE_APP_PD4, "loading ViewSheet");

    for(col = 0; col < 255; col++)
    {
        for(row = 0; row < vsrows; row++)
        {
            S32 vstype, vsdecp, vsrjust, vsminus;
            char *vslot;

            if(ctrlflag)
            {
                *p_breakout = TRUE;
                goto ENDSHEET;
            }

            actind(row * 100 / vsrows);

            vslot = vsload_travel(col, row, &vstype,
                                  &vsdecp, &vsrjust, &vsminus);

            if(vslot)
            {
                uchar justify = vsrjust ? J_RIGHT : J_LEFT;
                uchar type;
                uchar format = 0;

                if(vstype == VS_TEXT)
                    type = SL_TEXT;
                else
                {
                    type = SL_NUMBER;

                    if(vsminus < 0)
                    {
                        justify = J_RIGHT;
                        /*format = 0;*/
                    }
                    else
                    {
                        format = (uchar) (vsminus ? 0 : F_BRAC);
                        format |= F_DCP | ((vsdecp < 0) ? 0xF : vsdecp);
                    }
                }

                if(inserting)
                {
                    if(!(insertslotat((COL) col, (ROW) row)))
                        goto ENDSHEET;

                    /* need to know how deep inserted bit is */
                    if( *p_insert_numrow < (ROW) row + 1)
                        *p_insert_numrow = (ROW) row + 1;
                }

                /* if something to put in cell, put it in */
                strcpy(linbuf, vslot);
                if(!stoslt((COL) col, (ROW) row, type, justify, format, 0, inserting, TRUE /*parse_as_expression*/))
                {
                    *p_breakout = TRUE;
                    goto ENDSHEET;
                }
            }
        }
    }

    ENDSHEET:
        ;
}

/******************************************************************************
*
* rename file - catch renames to and from
* external reference libraries on the path
*
******************************************************************************/

static void
rename_document(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    reportf("rename_document(path(%s) leaf(%s) %s)", report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name), report_boolstring(p_docu_name->flags.path_name_supplied));

    ev_rename_sheet(p_docu_name, current_docno());

    { /* NB the given DOCU_NAME now always takes prority */
    U8 buffer[BUF_MAX_PATHSTRING];
    name_make_wholename(p_docu_name, buffer, elemof32(buffer));
    consume_bool(mystr_set(&current_p_docu->Xcurrentfilename, buffer));
    } /*block*/

    riscos_settitlebar(currentfilename());
}

static BOOL
name_preprocess_docu_name_flags_for_rename(
    _InoutRef_  P_DOCU_NAME p_docu_name)
{
    BOOL is_loaded_from_path = 0;

    trace_2(TRACE_APP_PD4, "name_preprocess_docu_name_flags_for_rename(path(%s) leaf(%s))", report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name));

    if(p_docu_name->path_name && file_is_rooted(p_docu_name->path_name))
    {
        char dir_name[BUF_MAX_PATHSTRING];
        P_FILE_PATHENUM pathenum;
        PCTSTR pathelem;
        P_U8 trail_ptr;
        P_U8 leaf_ptr;

        p_docu_name->flags.path_name_supplied = 1;

        xstrkpy(dir_name, elemof32(dir_name), p_docu_name->path_name);
        trail_ptr = dir_name + strlen32(dir_name) - 1;
        if(*trail_ptr == FILE_DIR_SEP_CH) /* overwrite trailing '.' but NOT ':' */
            *trail_ptr = CH_NULL;

        /* strip off trailing Library from dir_name if present */
        leaf_ptr = file_leafname(dir_name);
        if(leaf_ptr && (0 == C_stricmp(leaf_ptr, EXTREFS_SUBDIR_STR)))
            *leaf_ptr = CH_NULL;
        else if(*trail_ptr == CH_NULL) /* otherwise restore trailing '.' iff it was removed */
            *trail_ptr = FILE_DIR_SEP_CH;

        reportf("dir_name: %s", dir_name);

        for(pathelem = file_path_element_first(&pathenum, file_get_search_path()); NULL != pathelem; pathelem = file_path_element_next(&pathenum))
        {
            reportf("compare with pathelem: %s", pathelem);

            if((0 == C_stricmp(dir_name, pathelem))  /*&&  file_is_dir(pathelem)*/)
            {
                is_loaded_from_path = 1;
                p_docu_name->flags.path_name_supplied = 0;
                break;
            }
        }

        file_path_element_close(&pathenum);
    }
    else
    {
        p_docu_name->flags.path_name_supplied = 0;
    }

    reportf("is_loaded_from_path = %s, p_docu_name->flags.path_name_supplied = %s", report_boolstring(is_loaded_from_path), report_boolstring(p_docu_name->flags.path_name_supplied));
    return(is_loaded_from_path);
}

/* end of savload.c */
