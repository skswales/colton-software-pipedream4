/* savload.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

static void
rename_document(
    _InRef_     PC_DOCU_NAME p_docu_name);

static BOOL
rename_document_prep_docu_name_flags(
    _InoutRef_  P_DOCU_NAME p_docu_name);

static BOOL
savefile_core(
    _In_z_      PCTSTR filename,
    _InRef_     P_SAVE_FILE_OPTIONS p_save_file_options);

static BOOL
pd_save_slot(
    P_SLOT tslot,
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

static void
save_version_string(
    FILE_HANDLE output)
{
    uchar array[LIN_BUFSIZ];
    char *tmp;

    xstrkpy(array, elemof32(array), applicationversion);
    xstrkat(array, elemof32(array), ", ");

    xstrkat(array, elemof32(array), !str_isblank(user_id()) ? user_id() : "Colton Software");
    if(!str_isblank(user_organ_id()))
        {
        xstrkat(array, elemof32(array), " - ");
        xstrkat(array, elemof32(array), user_organ_id());
        }
    xstrkat(array, elemof32(array), ", ");

    xstrkat(array, elemof32(array), "R9200 7500 3900 8299");

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

    (void) mystr_set(&d_open_box[0].textfield, array);
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
        /* save print options and colours - and menu changes -  only to Choices file */

        save_opt_to_file(output, d_colours, dialog_head[D_COLOURS].items);
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
        /* save spreadsheet names - SKS after 4.11 08jan92 moved into non-Choices file section */
        save_names_to_file(output);

        /* save any row & column fixes */
        save_opt_to_file(output, d_fixes, dialog_head[D_FIXES].items);

        save_protected_bits(output);
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
    char array[LIN_BUFSIZ];
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
        if(ch1 == '\0' || ch2 == '\0')
            continue;

        ptr = linbuf + sprintf(linbuf, "%%" "OP" "%%" "%c%c", ch1, ch2);

        not_on_list = !search_list(&def_first_option, ((S32) ch1 << 8) + (S32) ch2);

        trace_3(TRACE_APP_PD4, "considering option %c%c, type %d", ch1, ch2, dptr->type);

        switch(dptr->type)
            {
            case F_SPECIAL:
                if(not_on_list  &&  (dptr->option == **dptr->optionlist))
                    continue;

                ptr[0] = dptr->option;
                ptr[1] = '\0';
                break;

            case F_ARRAY:
                if(not_on_list  &&  (dptr->option == 0))
                    continue;

                (void) sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_LIST:
            case F_NUMBER:
            case F_COLOUR:
                /* is it the default ? */

                if(not_on_list  &&  ((int) dptr->option == atoi(*dptr->optionlist)))
                    continue;

                (void) sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_TEXT:
#if 1
                ptr1 = dptr->textfield;
                if(str_isblank(ptr1))
                    ptr1 = NULLSTR;

                if(dptr->optionlist)
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

        while((ch1 = *ptr++) != '\0')
            if(ishighlight(ch1))
                {
                (void) sprintf(array, "%%" "H" "%c" "%%", (ch1 - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT);
                away_string_simple(array, output);
                }
            else
                away_byte(ch1, output);
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

extern BOOL
save_existing(void)
{
    S32 res = riscdialog_query_save_existing();

    return(res != riscdialog_query_CANCEL);
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

    riscos_frontmainwindow_atbox(TRUE);

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
    _In_z_      PCTSTR message)
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
        char buffer[256];
        (void) xsnprintf(buffer, elemof32(buffer), message, filename);
        return(riscdialog_query_YN(buffer) == riscdialog_query_YES);
    }

    return(TRUE);
}

/******************************************************************************
*
* name the current text
*
******************************************************************************/

extern void
NameFile_fn(void)
{
    if(!dialog_box_start())
        return;

    if(!mystr_set(&d_name[0].textfield, currentfilename))
        return;

    while(dialog_box(D_NAME))
        {
        const char * filename = d_name[0].textfield ? d_name[0].textfield : get_untitled_document();

        if(0 != _stricmp(filename, currentfilename))
            {
            if(same_name_warning(filename, name_supporting_winge_STR))
                {
                DOCU_NAME docu_name;
                BOOL ok = name_read_tstr(&docu_name, filename);

                if(ok)
                    {
                    xf_loaded_from_path = rename_document_prep_docu_name_flags(&docu_name);

                    rename_document(&docu_name);

                    name_free(&docu_name);
                    }
                }
            }

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();
}

/******************************************************************************
*
* save template file
*
* put out a dialog box, get a leafname,
* append it to <Choices$Write>.PipeDream.Templates.
* and save away in PipeDream format
*
******************************************************************************/

extern void
SaveTemplateFile_fn(void)
{
    if(!dialog_box_start())
        return;

    /* push out the leafname of the current file as a suggestion */
    if(!mystr_set(&d_save_template[0].textfield, file_leafname(currentfilename)))
        return;

    while(dialog_box(D_SAVE_TEMPLATE))
        {
        /* try to save the file out in <Choices$Write>.PipeDream.Templates. */
        TCHARZ buffer[BUF_MAX_PATHSTRING];
        SAVE_FILE_OPTIONS save_file_options;

        (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), d_save_template[0].textfield, LTEMPLATE_SUBDIR_STR);
        zero_struct(save_file_options);
        save_file_options.filetype_option = PD4_CHAR;
        if(!savefile_core(buffer, &save_file_options))
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
    SAVE_FILE_OPTIONS save_file_options;

    /* save Choices in <Choices$Write>.PipeDream directory */
    add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), CHOICES_FILE_STR, NULL);
    zero_struct(save_file_options);
    save_file_options.filetype_option = PD4_CHAR;
    save_file_options.saving_choices_file = TRUE;
    (void) savefile_core(buffer, &save_file_options);

    /* save Preferred in <Choices$Write>.PipeDream directory (iff needed) */
    add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), PREFERRED_FILE_STR, NULL);
    pdchart_preferred_save(buffer);
}

/******************************************************************************
*
* save file
*
******************************************************************************/

static void
do_SaveFile(
    _InVal_     S32 boxnumber)
{
    SAVE_FILE_OPTIONS save_file_options;

    if(!init_dialog_box(boxnumber))
        return;

    if(!dialog_box_start())
        return;

    if(!mystr_set(&d_save[SAV_NAME].textfield, currentfilename))
        return;

    d_save[SAV_FORMAT].option = current_filetype_option;
    d_save[SAV_LINESEP].option = current_line_sep_option;

    while(dialog_box(boxnumber))
        {
        if(str_isblank(d_save[SAV_NAME].textfield))
            {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        zero_struct(save_file_options);
        save_file_options.filetype_option = d_save[SAV_FORMAT].option;
        save_file_options.line_sep_option = d_save[SAV_LINESEP].option;
        if(('Y' == d_save[SAV_ROWCOND].option)  &&  !str_isblank(d_save[SAV_ROWCOND].textfield))
            save_file_options.row_condition = d_save[SAV_ROWCOND].textfield;
        if('Y' == d_save[SAV_BLOCK].option)
            save_file_options.saving_block = TRUE;
        if(!savefile(d_save[SAV_NAME].textfield, &save_file_options))
            break;

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();
}

extern void
SaveFile_fn(void)
{
    do_SaveFile(D_SAVE);
}

extern void
SaveFileAsIs_fn(void)
{
    SAVE_FILE_OPTIONS save_file_options;

    if(!file_is_rooted(currentfilename))
        {
        do_SaveFile(D_SAVE_POPUP); /* different as we may be in_execfile || command_expansion */
        return;
        }

    zero_struct(save_file_options);
    save_file_options.filetype_option = current_filetype_option;
    save_file_options.line_sep_option = current_line_sep_option;
    (void) savefile_core(currentfilename, &save_file_options);
}

/******************************************************************************
*
* output a slot in plain text mode,
* saving results of numbers rather than contents
* note that fonts are OFF so no juicy formatting
*
******************************************************************************/

extern BOOL
plain_slot(
    P_SLOT tslot,
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

    switch(tslot->type)
    {
    case SL_TEXT:
    case SL_PAGE:
        (void) expand_slot(current_docno(), tslot, trow, buffer, fwidth,
                           DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, FALSE /*expand_ctrl*/,
                           FALSE /*allow_fonty_result*/, TRUE /*cff*/);
        return(FALSE);

    default:
    /* case SL_NUMBER: */
        break;
    }

    oldformat = tslot->format;
    thousands = d_options_TH;

    switch(filetype_option)
    {
    default:
        break;

    case CSV_CHAR:
        #if !defined(OLD_CSV_SAVE_STYLE)
        /* poke slot to minus format, no leading or trailing chars */
        tslot->format &= ~(F_BRAC | F_LDS | F_TRS);
        #else
        tslot->format = F_DCPSID | F_DCP;
        #endif
        /* ensure no silly commas etc. output! */
        d_options_TH = TH_BLANK;
        break;

    case PARAGRAPH_CHAR:
    case FWP_CHAR:
        /* format slots on output to the same as we'd print them */
        colwid = colwidth(tcol);
        fwidth = chkolp(tslot, tcol, trow);
        fwidth = MIN(fwidth, colwid);
        fwidth = MIN(fwidth, LIN_BUFSIZ);
        break;
    }

    trace_6(TRACE_APP_PD4, "plain_slot(&%p, %d, %d, '%c', &%p): fwidth %d", report_ptr_cast(tslot), tcol, trow, filetype_option, buffer, fwidth);

    (void) expand_slot(current_docno(), tslot, trow, buffer, fwidth,
                       DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, FALSE /*expand_ctrl*/,
                       FALSE /*allow_fonty_result*/, TRUE /*cff*/);
    /* does not move slot */

    tslot->format = oldformat;
    d_options_TH = thousands;

    /* remove padding space from plain non-fonty string */
    len = strlen(buffer);

    while(len--)
        if(buffer[len] == FUNNYSPACE)
            {
            buffer[len] = '\0';
            break;
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
        return(reperr_null(create_error(ERR_BAD_NAME)));

    if(saving_part_file  ||  p_save_file_options->saving_choices_file  ||  p_save_file_options->temp_file)
        saving_whole_file = FALSE;

    /* rename file BEFORE saving to get external refs output correctly */
    if(saving_whole_file)
        {
        if(0 != _stricmp(filename, currentfilename))
            {
            if(same_name_warning(filename, name_supporting_winge_STR))
                {
                DOCU_NAME docu_name;
                BOOL ok = name_read_tstr(&docu_name, filename);

                if(ok)
                    {
                    xf_loaded_from_path = rename_document_prep_docu_name_flags(&docu_name);

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
    P_SLOT tslot;
    SLR first, last;
    char rowselection[EV_MAX_OUT_LEN + 1];
    char array[32];
    char field_separator = NULLCH;
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
            return(reperr_null(MARKER_SOMEWHERE() ? create_error(ERR_NOBLOCKINDOC) : create_error(ERR_NOBLOCK)));

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
        return(reperr_null(create_error(ERR_CANNOTOPEN)));

    (void) file_buffer(output, NULL, 64*1024); /* no messing about for save */

    /* file successfully opened at this point */

    actind(0);

    escape_enable();

    /* set up font dialog box for saving */

    update_dialog_from_fontinfo();

    triscos_fonts = font_stack(riscos_fonts);
    riscos_fonts = FALSE;

    /* row and column fixes have dummy dialog box - set it up */

    (void) init_dialog_box(D_FIXES);

    if(n_rowfixes)
        {
        (void) sprintf(array, "%d,%d", row_number(0), n_rowfixes);
        (void) mystr_set(&d_fixes[0].textfield, array);
        }

    if(n_colfixes)
        {
        (void) sprintf(array, "%d,%d", col_number(0), n_colfixes);
        (void) mystr_set(&d_fixes[1].textfield, array);
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

    /* print all slots in block, for plain text going
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

        /* save all slots */
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
                        return(reperr_null(create_error(ERR_BAD_SELECTION)));
                        }
                    }
                else
                    *rowselection = '\0';
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

            tslot = travel(tcol, trow);

            /* for a new column, either a column heading construct needs to be
             * be written or (when in plain text mode) tabs up to
             * the current column
            */
            if(prevcol != tcol)
                {
                if(!goingdown)
                    {
                    /* in plain text (or commarated) mode don't bother writing
                     * field separator if there are no more slots in this row
                    */
                    if( prevcol > tcol)
                        prevcol = tcol;  /* might be new row */

                    /* if it's a blank slot don't bother outputting field separators
                     * watch out for numeric slots masquerading as blanks
                    */
                    if( !isslotblank(tslot)  ||
                        (tslot  &&  (tslot->type != SL_TEXT)))
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

            if(!tslot)
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
                    if(!dtp_save_slot(tslot, tcol, trow, output))
                        all_ok = FALSE;
                    }
                }
            else
                {
                /* slot needs writing out */
                if(timer++ > TIMER_DELAY)
                    {
                    timer = 0;
                    actind_in_block(goingdown);
                    }

                /* RJM would redirect saving_part_file on 31.8.91, but we still need the slot bits!! */
                if(goingdown)
                    {
                    if(!pd_save_slot(tslot, tcol, trow, output, saving_part_file)  ||
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
                                        (void) sprintf(array + 2, "%d", (int) condval);

                                    if(!view_save_stored_command(array, output))
                                        all_ok = FALSE;
                                    }

                                /* don't output anything on page break line */
                                continue;
                                }

                            if(tcol == 0)
                                {
                                switch((tslot->justify) & J_BITS)
                                    {
                                    case J_LEFT:
                                        if(!view_save_stored_command("LJ", output))
                                            all_ok = FALSE;
                                        break;

                                    case J_CENTRE:
                                        if(!view_save_stored_command("CE", output))
                                            all_ok = FALSE;
                                        break;

                                    case J_RIGHT:
                                        if(!view_save_stored_command("RJ", output))
                                            all_ok = FALSE;
                                        break;

                                    default:
                                        break;
                                    }
                                }

                            if( all_ok  &&
                                tslot   &&  !view_save_slot(tslot, tcol, trow, output,
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
                                if(!fwp_save_slot(tslot, tcol, trow, output, TRUE))
                                    all_ok = FALSE;
                                }
                            else
                                {
                                if(!dtp_save_slot(tslot, tcol, trow, output))
                                    all_ok = FALSE;
                                }
                            break;

                        default:
                            /* check for page break */
                            if(chkrpb(trow))
                                /* don't output anything on page break line */
                                continue;

                            {
                            /* write just text or formula part of slot out */
                            BOOL csv_quotes = !plain_slot(tslot, tcol, trow, p_save_file_options->filetype_option, linbuf)
                                              &&  (CSV_CHAR == p_save_file_options->filetype_option);
                            uchar * lptr = linbuf;
                            uchar ch;

                            /* output contents, not outputting highlight chars */
                            if(csv_quotes  &&  !away_byte(QUOTES, output))
                                all_ok = FALSE;

                            while(all_ok  &&  ((ch = *lptr++) != NULLCH))
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
        reperr_null(create_error(ERR_CANNOTWRITE));
        all_ok = FALSE;
        }

    if(pd_file_close(&output))
        {
        reperr_null(create_error(ERR_CANNOTCLOSE));
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
        reperr(create_error(ERR_LINES_SPLIT), buffer);
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
* save slot
*
******************************************************************************/

static BOOL
pd_save_slot(
    P_SLOT tslot,
    COL tcol,
    ROW trow,
    FILE_HANDLE output,
    BOOL saving_part_file)
{
    uchar * lptr, ch;
    BOOL numerictype = FALSE;
    uchar justify;

    /* save slot type, followed by justification, followed by formats */

    switch((int) tslot->type)
    {
    default:
    /* case SL_TEXT: */
        break;

    case SL_NUMBER:
        numerictype = TRUE;
        if(!away_construct(C_VALUE, output))
            return(FALSE);
        break;

    case SL_PAGE:
        {
        char page_array[32];
        (void) xsnprintf(page_array, elemof32(page_array),
                        "%%" "P" "%d" "%%", tslot->content.page.condval);
        if(!away_string_simple(page_array, output))
            return(FALSE);
        break;
        }
    }

    justify = tslot->justify & J_BITS;

    if((justify > 0)  &&  (justify <= 6))
        if(!away_construct(justify, output))
            return(FALSE);

    if(numerictype)
        {
        const uchar format = tslot->format;

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

    /* RJM 9.10.91, this plain_slot ensures blocks and row selections aren't saved with silly slot refs
        it helps the PD4 example database to work, amongst other things
    */
    if(saving_part_file)
        plain_slot(tslot, tcol, trow, PD4_CHAR, linbuf);
    else
        prccon(linbuf, tslot);

    /* output contents, dealing with highlight chars and % */

    { /* SKS 20130403 quick check to see if we can avoid special case processing (usually we can) */
    P_U8 simple_string = lptr = linbuf;

    while((ch = *lptr++) != '\0')
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

    while((ch = *lptr++) != '\0')
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

        /* for option, just remember position: gets dealt with at end of slot */

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
* store from linbuf in slot
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
    S32 inserting)
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

            if((rpn_len = compile_expression(compiled_out, linbuf, EV_MAX_OUT_LEN, &at_pos, &ev_result, &parms)) < 0)
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
            P_SLOT tslot;

            /* compile directly into slot */
            buffer_altered = TRUE;
            if(!merstr(tcol, trow, FALSE, !inserting))
                {
                buffer_altered = FALSE; /* so filbuf succeeds */
                return(FALSE);
                }

            if((tslot = travel(tcol, trow)) != NULL)
                tslot->justify = justify;

            break;
            }

        case SL_PAGE:
            {
            P_SLOT tslot;

            /* type is not text */
            if((tslot = createslot(tcol, trow, sizeof(struct SLOT_CONTENT_PAGE), type)) == NULL)
                {
                slot_in_buffer = buffer_altered = FALSE;
                return(reperr_null(status_nomem()));
                }

            tslot->type    = type;
            tslot->justify = justify;

            tslot->content.page.condval = pageoffset;
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
LoadFile_fn(void)
{
    LOAD_FILE_OPTIONS load_file_options;

    if(!init_dialog_box(D_LOAD))
        return;

    if(!dialog_box_start())
        return;

    d_load[3].option = **(d_load[3].optionlist); /* Init to Auto */

    while(dialog_box(D_LOAD))
        {
        zero_struct(load_file_options);
        load_file_options.document_name = d_load[0].textfield;
        load_file_options.inserting = ('Y' == d_load[1].option) && is_current_document();
        if(load_file_options.inserting)
            load_file_options.insert_at_slot = d_load[1].textfield;
        if('Y' == d_load[2].option)
            load_file_options.row_range = d_load[2].textfield;
        load_file_options.filetype_option = d_load[3].option;

        if(!loadfile(d_load[0].textfield, &load_file_options))
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

extern void
LoadTemplate_fn(void)
{
    if(!dialog_box_start())
        return;

    if(str_isblank(d_load_template[0].textfield))
        if(!mystr_set(&d_load_template[0].textfield, DEFAULT_LTEMPLATE_FILE_STR))
            return;

    status_assert(enumerate_dir_to_list(&ltemplate_or_driver_list, LTEMPLATE_SUBDIR_STR, FILETYPE_UNDETERMINED));

    while(dialog_box(D_LOAD_TEMPLATE))
        {
        /* Add prefix '<PipeDream$Dir>.Templates.' to template */
        char buffer[BUF_MAX_PATHSTRING];
        PCTSTR tname = d_load_template[0].textfield;
        LOAD_FILE_OPTIONS load_file_options;
        S32 res;

        if(str_isblank(tname))
            {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        if((res = add_path_using_dir(buffer, elemof32(buffer), tname, LTEMPLATE_SUBDIR_STR)) <= 0)
            {
            reperr_null(res);
            if(!dialog_box_can_retry())
                break;
            continue;
            }

        zero_struct(load_file_options);
        load_file_options.document_name = get_untitled_document();
        load_file_options.filetype_option = PD4_CHAR;

        if(!loadfile_recurse(buffer, &load_file_options))
            break;

        if(!dialog_box_can_persist())
            break;
        }

    dialog_box_end();

    delete_list(&ltemplate_or_driver_list);
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

        if((auto_filetype_option = find_filetype_option(filename)) <= 0)
            {
            if(0 == auto_filetype_option)
                reperr(ERR_NOTFOUND, filename);
            /* else already reported */
            return(FALSE);
            }

        p_load_file_options->filetype_option = (char) auto_filetype_option;

        reportf("loadfile: auto-detected filetype_option '%c'", p_load_file_options->filetype_option);
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

        consume(int,
            sscanf(d_open_box[0].textfield,
                   "%d,%d,%d,%d,%d,%d,%d,%d",
                   &open_box.x0, &open_box.x1,
                   &open_box.y0, &open_box.y1,
                   &tcol, &trow, &fcol, &frow));

        /* did we get them all? */
        if(frow != -1)
            use_loaded = TRUE;
        }

    /* SKS after 4.11 08jan92 - attempt to get window position scrolled back ok */
    if(use_loaded)
        {
        riscos_frontmainwindow_atbox(TRUE);

        if(!mergebuf_nocheck())
            return(FALSE); /* SKS 10.10.91 */

        chknlr(tcol, trow);
        filhorz(fcol, fcol);
        filvert(frow, frow, CALL_FIXPAGE);

        /*draw_screen();*/ /* SKS 08jan92 added; SKS 20130604 removed - see caller */
        }
    else
        riscos_frontmainwindow(in_execfile);

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

        if(!p_load_file_options->temp_file  &&  !same_name_warning(filename, load_supporting_winge_STR))
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
            BOOL will_be_loaded_from_path = rename_document_prep_docu_name_flags(&docu_name);

            ok = create_new_document(&docu_name);

            if(ok)
                {
                xf_loaded_from_path = will_be_loaded_from_path;

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
                    riscos_frontmainwindow(in_execfile);
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

        filetype_option = find_filetype_option(fullname);
        } /*block*/

        if(filetype_option > 0)
            {
            LOAD_FILE_OPTIONS load_file_options;
            zero_struct(load_file_options);
            load_file_options.document_name = sup_doc_filename;
            load_file_options.filetype_option = filetype_option;
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

            try_path = add_path_using_dir(sup_doc_filename, elemof32(sup_doc_filename), leaf_name, EXTREFS_SUBDIR_STR);
            } /*block*/

            if(try_path)
                {
                filetype_option = find_filetype_option(sup_doc_filename);

                if(filetype_option > 0)
                    {
                    LOAD_FILE_OPTIONS load_file_options;
                    zero_struct(load_file_options);
                    load_file_options.document_name = sup_doc_filename;
                    load_file_options.filetype_option = filetype_option;
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
* Enumerate all files in subdirectory 'subdir' found in the directories listed by the PipeDream$Path variable.
*
******************************************************************************/

/*ncr*/
extern STATUS
enumerate_dir_to_list(
    _InoutRef_  P_P_LIST_BLOCK list,
    _In_opt_z_  PC_U8Z subdir /*maybe NULL*/,
    _InVal_     FILETYPE_RISC_OS filetype)
{
    P_FILE_OBJENUM     enumstrp;
    P_FILE_OBJINFO     infostrp;
    U8                 path[BUF_MAX_PATHSTRING];
    S32                entry = 0;
    STATUS             res_error = 0;

    /* SKS after 4.11 03feb92 - why was this only file_get_path() before? */
    file_combined_path(path, elemof32(path), is_current_document() ? currentfilename : NULL);

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

            status_break(res_error = add_to_list(list, entry++, leafname));
            }
        }

    return(res_error);
}

/******************************************************************************
*
* open the load file and have a peek to see what type it is
*
******************************************************************************/

static const char lotus_leadin[4] = { '\x00', '\x00', '\x02', '\x00' };

/* we know that name is not null */

#define BYTES_TO_SEARCH 1024

extern S32 /* filetype_option, 0 or error (reported) */
find_filetype_option(
    _In_z_      PCTSTR name)
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
    S32 type;

    input = pd_file_open(name, file_open_read);

    if(!input)
        return('\0'); /* not an error at this level */

    size = pd_file_read(array, 1, BYTES_TO_SEARCH, input);

    reportf("find_filetype_option: read %d bytes from file, requested %d", size, BYTES_TO_SEARCH);

    /* error in reading? */
    if(size == -1)
        {
        type = -1;
        goto endpoint;
        }

    /* ensure terminated at a suitable place */
    if(size == BYTES_TO_SEARCH)
        --size;
    array[size] = '\0';

    /* is it a chart file? */
    res = gr_cache_fileheader_is_chart(array, size);
    if(res != 0)
        {
        type = PD4_CHART_CHAR;
        goto endpoint;
        }

    if(vsload_fileheader_isvsfile(array, size))
        {
        trace_0(TRACE_APP_PD4, "ViewSheet file");
        type = VIEWSHEET_CHAR;
        goto endpoint;
        }

    /* look to see if it's a LOTUS file, yuk */
    if( (size > 8)                              &&
        (0 == memcmp(array, lotus_leadin, 4))   &&
        (array[4] >= 4)  &&  (array[4] <= 6)    &&
        (array[5] == 4)                         )
        {
        reportf("LOTUS FILE DETECTED: %s!!!", name);
        reperr_not_installed(create_error(ERR_LOTUS));
        type = ERR_LOTUS;
        goto endpoint;
        }

    /* not lotus */

    /* test for 1WP file */
    if(fwp_isfwpfile(array))
        {
        trace_0(TRACE_APP_PD4, "1wp file");
        type = FWP_CHAR;
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
                        type = PD4_CHAR;
                        }
                    else
                        {
                        trace_0(TRACE_APP_PD4, "PipeDream 4 file found (assumed, might be PD3 or PD4-transitional)");
                        type = PD4_CHAR;
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
                type = TAB_CHAR;
                goto endpoint;

            case CR:
            case LF:
                /* if we complete a line full of only comma type options, ok */
                if(ptr - start_of_line > 1)
                    if(comma_values > 1)
                        {
                        trace_0(TRACE_APP_PD4, "Line of valid comma options: CSV");
                        type = CSV_CHAR;
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
                        case '\0':
                            /* faulty quoted string */
                            trace_0(TRACE_APP_PD4, "Tab file as quoted string was faulty");
                            type = TAB_CHAR;
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
                    type = VIEW_CHAR;
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
                    type = VIEW_CHAR;
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

    type = TAB_CHAR;

endpoint:;

    pd_file_close(&input);

    return(type);
}

/******************************************************************************
*
*  return the RISC OS filetype from our letter options
*
******************************************************************************/

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

        /* case VIEWSHEET_CHAR: */
        /* case PD4_CHAR: */
        default:
            res = FILETYPE_PIPEDREAM;
            break;
        }

    trace_2(TRACE_APP_PD4, "rft_from_filetype_option('%c') yields &%3.3X", filetype_option, res);
    return(res);
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
    /* 1WP highlights have to be reinstated at the start of each slot */
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
        field_separator = NULLCH;
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
            return(reperr_null(create_error(ERR_BAD_RANGE)));
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
            return(reperr_null(create_error(ERR_BAD_SLOT)));
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
                case PARAGRAPH_CHAR: /* SKS after 4.11 08jan92 - stop paragraph loading turning wrap off */
                    break;

                default:
                    /* turn off wrap in plain text documents eg ASCII, CSV, VIEWSHEET(!) */
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
        return(reperr(create_error(ERR_CANNOTOPEN), filename));

    (void) file_buffer(loadinput, NULL, 64*1024); /* no messing about for load */

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
                return(reperr_module(ERR_SHEET, vsrows));
                }

            break;
        }

    (void) init_dialog_box(D_FIXES);

    (void) init_dialog_box(D_FONTS);

    in_load = TRUE;

    escape_enable();

    /* read each slot from the file */

    if(VIEWSHEET_CHAR == p_load_file_options->filetype_option)
        viewsheet_load_core(vsrows, p_load_file_options->inserting, &insert_numrow, &breakout);
    else

    /* each slot */
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
            /* begin PipeDream format read processing for this slot */
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

                        break; /* either CR or LF marks end of slot */
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

                    linbuf[lecpos] = NULLCH;

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

            /* end of PipeDream format read processing for this slot */
            }
        else
            {
            /* begin other format read processing for this slot */

            if(/*loading_paragraph &&*/ paragraph_saved_word && paragraph_in_column == tcol)
                {
                /* put the word at start of this line */
                strcpy(linbuf, paragraph_saved_word);
                lecpos = strlen(linbuf);
                (void) mystr_set(&paragraph_saved_word, NULL);
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
                                if(field_separator  &&  (c == field_separator)) /* allows TAB etc through */
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

                /* end of slot? */
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

                    linbuf[lecpos] = '\0';

                    (void) mystr_set(&paragraph_saved_word, mptr);
                    paragraph_in_column = tcol;

                    /* write only up to start of word into slot */
                    lecpos = mptr-linbuf;
                    break;
                    }
                }
            while(lecpos < MAXFLD);

            /* end of other format read processing for this slot */
            }

        if(outofmem)
            break;

        linbuf[lecpos] = NULLCH;

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

                /* if inserting, insert slot here */
                if(p_load_file_options->inserting  &&  ((c >= 0)  ||  (lecpos > 0)))
                    {
                    if(!(insertslotat(tcol, trow)))
                        break;

                    /* need to know how deep inserted bit is */
                    if( insert_numrow < trow+1)
                        insert_numrow = trow+1;
                    }

                /* if something to put in slot, put it in */
                if( ((lecpos > 0)  ||  (type == SL_PAGE))  &&
                    !stoslt(tcol, trow, type, justify, format, pageoffset, p_load_file_options->inserting))
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

        block_updref(blk_docno, blk_docno, first.col, first.row, insert_col, insert_row);

        /* if ran out of memory, throw away what's been loaded */
        if(breakout)
            /* second parameter added 19.8.91 by RJM for new block move code */
            do_delete_block(FALSE, FALSE, TRUE);
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

            consume(int, sscanf(d_fixes[0].textfield, "%d,%d", &firstrow, &nrows));

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

            consume(int, sscanf(d_fixes[1].textfield, "%d,%d", &firstcol, &ncols));

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

    /* updating slots - must do after possible insertion discard */
    clear_protect_list();
    clear_linked_columns(); /* set the linked columns */

    escape_disable();

    actind_end();

    recalc_state_may_have_changed();
    chart_recalc_state_may_have_changed();
    insert_state_may_have_changed();
    update_variables();

    filealtered(p_load_file_options->temp_file  ||  p_load_file_options->inserting);

    if(ctrl_chars)
        reperr_null(create_error(ERR_CTRL_CHARS));

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

                /* if something to put in slot, put it in */
                strcpy(linbuf, vslot);
                if(!stoslt((COL) col, (ROW) row,
                           type, justify, format, 0, inserting))
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
    (void) mystr_set(&currentfilename, buffer);
    } /*block*/

    riscos_settitlebar(currentfilename);
}

static BOOL
rename_document_prep_docu_name_flags(
    _InoutRef_  P_DOCU_NAME p_docu_name)
{
    BOOL is_loaded_from_path = 0;

    trace_2(TRACE_APP_PD4, "rename_document_prep_docu_name_flags(path(%s) leaf(%s))", trace_tstr(p_docu_name->path_name), trace_tstr(p_docu_name->leaf_name));

    if(p_docu_name->path_name && file_is_rooted(p_docu_name->path_name))
        {
        char dir_name[BUF_MAX_PATHSTRING];
        char rawpath[BUF_MAX_PATHSTRING];
        P_FILE_PATHENUM path;
        PCTSTR pathelem;
        P_U8 trail_ptr;
        P_U8 leaf_ptr;

        p_docu_name->flags.path_name_supplied = 1;

        xstrkpy(dir_name, elemof32(dir_name), p_docu_name->path_name);
        trail_ptr = dir_name + strlen32(dir_name) - 1;
        if(*trail_ptr == FILE_DIR_SEP_CH) /* overwrite trailing '.' but NOT ':' */
            *trail_ptr = NULLCH;

        /* strip off trailing Library from dir_name if present */
        leaf_ptr = file_leafname(dir_name);
        if(leaf_ptr && (0 == _stricmp(leaf_ptr, EXTREFS_SUBDIR_STR)))
            *leaf_ptr = NULLCH;
        else if(*trail_ptr == NULLCH) /* otherwise restore trailing '.' iff it was removed */
            *trail_ptr = FILE_DIR_SEP_CH;

        reportf("dir_name: %s", dir_name);

        file_combined_path(rawpath, elemof32(rawpath), NULL);

        for(pathelem = file_path_element_first(&path, rawpath); pathelem; pathelem = file_path_element_next(&path))
            {
            reportf("compare with pathelem: %s", pathelem);

            if((0 == _stricmp(dir_name, pathelem))  /*&&  file_is_dir(pathelem)*/)
                {
                is_loaded_from_path = 1;
                p_docu_name->flags.path_name_supplied = 0;
                break;
                }
            }
        }
    else
        p_docu_name->flags.path_name_supplied = 0;

    reportf("is_loaded_from_path = %s, p_docu_name->flags.path_name_supplied = %s", report_boolstring(is_loaded_from_path), report_boolstring(p_docu_name->flags.path_name_supplied));
    return(is_loaded_from_path);
}

/* end of savload.c */
