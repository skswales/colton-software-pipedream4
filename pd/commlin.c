/* commlin.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that edits and calls commands */

/* RJM 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#include "riscos_x.h"
#include "riscmenu.h"

static MENU * command_edit(S32 ch);

static void AutoRecalculation_fn(void);
static void AutoChartRecalculation_fn(void);

static void stop_macro_recorder(void);

/*
exported variables
*/

coord this_heading_length = 0;

/*
internal functions
*/

static BOOL
find_command(
    MENU **mptrp, S32 funcnum);

static MENU *
lukucm(
    BOOL allow_redef);                /* look up command */

static MENU *
part_command(
    BOOL allow_redef);

static S32
fiddle_alt_riscos(
    S32 ch);

static BOOL
output_string_to_macro_file(
    char * str);

static S32
really_out_comm_start(void);

static void
cvt_string_arg_for_macro_file(
    _Out_writes_z_(elemof_buffer) P_USTR ptr /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR arg);

/******************************************************************************
*
* RISC OS has sent us some input we have
* interpreted as a command, so process it
*
******************************************************************************/

extern void
application_process_command(
    S32 c)
{
    in_execfile = command_expansion = been_error = FALSE;

    (void) act_on_c(-c);
}

/******************************************************************************
*
* RISC OS has sent us some keyboard input so process it
*
******************************************************************************/

_Check_return_
extern BOOL
application_process_key(
    KMAP_CODE c)
{
    BOOL res;

    in_execfile = command_expansion = been_error = FALSE;

    c = inpchr_riscos(c);

    if(c)
    {
        res = act_on_c(c);

        /* may have key expansion going etc. */
        while((c = inpchr_riscos(0)) != 0)
            (void) act_on_c(c);
    }
    else
        res = TRUE;

    return(res);
}

static MENU * last_command_menup;

/******************************************************************************
*
* --in--
*   +ve     normal characters
*   0       do nothing
*   -ve     a function number
*
******************************************************************************/

/*ncr*/
extern BOOL
act_on_c(
    S32 c)
{
    if(c < 0)
    {
        MENU * mptr;

        trace_1(TRACE_APP_PD4, "act_on_c(%d) (command)", c);

        if(!find_command(&mptr, -c))
            return(FALSE);

        /* remember the comand in order to repeat it, perhaps */
        if(0 == (mptr->flags & MF_NO_REPEAT))
            last_command_menup = mptr;

        if(is_current_document() && xf_interrupted)
        {
            draw_screen();
            draw_caret();
        }

        do_command(mptr, -c, TRUE);

#if TRACE_ALLOWED
        if(ctrlflag)                /* should have all been caught by now */
        {
            char array[256];
            consume_int(xsnprintf(array, elemof32(array), ": command %d left escape unprocessed", -c));
            ack_esc();
            reperr(ERR_ESCAPE, array);
        }
#else
        ack_esc();
#endif

        if(is_current_document())
        {
            draw_screen();
            draw_caret();
        }
        else
            trace_0(TRACE_APP_PD4, "document disappeared from under our feet");
    }
    else if(c > 0)
    {
        trace_1(TRACE_APP_PD4, "act_on_c(%d) (key)", c);

        /* only add real keys to buffer! */
        if(c >= 0x100)
            return(FALSE);

        if(is_current_document())
        {
            inschr((uchar) c);          /* insert char in buffer */
            chkwrp();                   /* format if necessary */
#define RJM_QUICKDRAW
#ifdef RJM_QUICKDRAW
            /* formatting might require movement; only other
             * case is buffer ouput - make this interruptible
            */

            /* RJM says caret left/right and delete chars should be faster */
            if(movement || (!in_execfile && !keyinbuffer()))
#endif
            {
                draw_screen();
                draw_caret();
            }
#ifdef RJM_QUICKDRAW
            else
                xf_interrupted = TRUE;
#endif
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* repeat the last repeatable command
*
******************************************************************************/

extern void
RepeatCommand_fn(void)
{
    if(last_command_menup)
        do_command(last_command_menup, last_command_menup->funcnum, TRUE);
}

/******************************************************************************
*
* variant of inpchr for RISC OS
*
* --in--
*    16-bit character (as from rdch())
*
* --out--
*    +ve        char to be inserted
*    0          do nothing as there's been an error
*    -ve        function number to execute
*
******************************************************************************/

extern S32
inpchr_riscos(
    KMAP_CODE keyfromriscos)
{
    S32 c;

    trace_1(TRACE_APP_PD4, "inpchr_riscos(&%3.3X)", keyfromriscos);

    do  {
        do  {
            /* if in expansion get next char */
            if(NULL != keyidx)
            {
                if((c = *keyidx++) == CH_NULL)
                    keyidx = NULL;        /* this expansion has ended */
            }
            else
            {   /* no expansion char so try 'keyboard' */
                if(keyfromriscos == 0)
                {
                    trace_0(TRACE_APP_PD4, "read key for risc os already: returns 0");
                    return(0);
                }

                c = keyfromriscos;
                keyfromriscos = 0;
            }
        }
        /* c guaranteed set by here */
        /* expand char if not in edit line and not already expanding */
        while(cmd_seq_is_empty()  &&  (0 == cbuff_offset)  &&  (CH_NULL != c)  &&  (NULL == keyidx)  &&  schkvl(c));

        if((NULL != keyidx)  &&  (CH_NULL == *keyidx))
        {
            /* expansion now completed after reading last char */
            keyidx = NULL;

            if(cbuff_offset  &&  (c != CR))
            {
                /* half a command entered on function key or something */
                cbuff_offset = 0;
                reperr_null(ERR_BAD_PARM);
                trace_0(TRACE_APP_PD4, "error in key expansion: returns 0");
                return(0);            /* an uninteresting result */
            }
        }

        /* see if we're doing an Cmd-sequence */
        c = fiddle_alt_riscos(c);

        /* starting, or continuing a command from a key expansion? */
        if((c == CMDLDI)  ||  cbuff_offset)
        {
            MENU *mptr;

            mptr = command_edit(c);
            if(mptr == M_IS_ERROR)
                return(IS_ERROR);
            if(mptr == M_IS_SUBSET)
                return(IS_SUBSET);

            if(mptr == NULL)
                c = 0;
            else
                c = 0 - mptr->funcnum;
        }
    }
    while(!c  &&  (cbuff_offset > 0));    /* incomplete command? */

    trace_1(TRACE_APP_PD4, "inpchr_riscos returns %d", c);
    return(c);
}

/******************************************************************************
*
* exec the file, cannot assume it exists or that it is correct type
*
******************************************************************************/

extern void
do_execfile(
    _In_z_      PCTSTR filename)
{
    STATUS filetype_option;
    int c;
    FILE_HANDLE input;
    uchar array[LIN_BUFSIZ];
    uchar *ptr;

    if((filetype_option = find_filetype_option(filename, FILETYPE_UNDETERMINED)) < 0)
        return; /* error already reported */

    if(filetype_option != 'T')
    {
        if(0 == filetype_option)
            reperr(ERR_NOTFOUND, filename);
        else
            reperr(ERR_NOTTABFILE, filename);
        return;
    }

    input = pd_file_open(filename, file_open_read);

    if(NULL == input)
    {
        reperr(ERR_CANNOTOPEN, filename);
        return;
    }

    filbuf();

    in_execfile = TRUE;

    do  {
        c = getfield(input, array, FALSE);    /* get a line or something */

        prccml(array);        /* convert funny characters */

        ptr = array;

        while(*ptr)
        {
            /* if it's a command execute it, else if it's a key, expand it */

            if(been_error)
            {
                pd_file_close(&input);

                in_execfile = FALSE;
                return;
            }

            if(*ptr == CMDLDI)
            {
                MENU *mptr;
                uchar *oldpos = ptr;

                exec_ptr = NULL;
                keyidx = cbuff;

                /* copy command to small array for execution */

                for(;;)
                {
                    switch(*ptr)
                    {
                    case TAB:
                    case CR:
                    /* exec_ptr points to the start of a possible parameter string
                     * for dialog box.  If there is a dialog box then after act_on_c()
                     * exec_ptr will point to the carriage-return
                    */
                        exec_ptr = ptr++;
                        break;

                    case CH_NULL:
                        /* unmatched / */
                        reperr_null(ERR_BAD_PARM);
                        goto BREAKOUT;

                    default:
                        *keyidx++ = *ptr++;
                        continue;
                    }
                    break;
                }

                *keyidx = CH_NULL;
                keyidx = cbuff;
                mptr = lukucm(FALSE);
                if(mptr == (MENU *) NO_COMM)
                {
                    /* can't find command so convert CMDLDI back to backslash */
                    ptr = oldpos;
                    *ptr = '\\';
                }
                else
                {
                    act_on_c(0 - mptr->funcnum);
                    ptr = exec_ptr+1;

                    /* having done a command, read to end of line */
                    while(*ptr != CH_NULL && *ptr != CR && *ptr != LF)
                        ptr++;
                }

                keyidx = NULL;
            }
            else
                /* it's not a CMDLDI */
            {
                cbuff[0] = *ptr;
                cbuff[1] = CH_NULL;
                keyidx = cbuff;

                /* reset keyidx if it's a command, e.g. CR, TAB */
                schkvl(*ptr++);

                if(keyidx)
                    /* 'input' the characters */
                    act_on_c(inpchr_riscos(0));
            }

            if(is_current_document())
                draw_screen();
        }
    }
    while (c != EOF);

BREAKOUT:
    pd_file_close(&input);

    exec_ptr    = NULL;
    in_execfile = FALSE;
}

/******************************************************************************
*
* command_edit is called by inpchr when a key is being expanded
*
******************************************************************************/

static MENU *
command_edit(
    S32 ch)
{
    MENU *mptr;

    switch((int) ch)
    {
    case TAB:
        /* have got a command parameter
         * set exec_ptr to the TAB position
         * set keyidx to first character after return
        */
        exec_ptr = keyidx - 1;
        command_expansion = TRUE;

        do { ch = *keyidx++; } while((ch != CR)  &&  (ch != CH_NULL));

        /* deliberate fall thru... */

    case CR:
        cbuff[cbuff_offset] = CH_NULL;
        mptr = lukucm(FALSE);
        cbuff_offset = 0;
        return(mptr);

    default:
        if((ch != CMDLDI)  &&  ((ch < SPACE)  ||  (ch > 127)))
            break;

        /* add character to end of command buff */
        if(cbuff_offset >= CMD_BUFSIZ-1)
        {
            bleep();
            return(0);
        }

        cbuff[cbuff_offset++] = (uchar) ch;
        break;
    }

    return(0);             /* nothing to add to text buffer */
}

/******************************************************************************
*
* get and return decimal, updating pointer
* assumed capable of reading row numbers, therefore must read longs
* note usage of buff_sofar
* note NO_VAL assumed negative by things calling getsbd
*
******************************************************************************/

extern S32
getsbd(void)
{
    S32 count;
    S32 res;
    uchar ch;

    if(!buff_sofar)
        return(NO_VAL);

    do { ch = *buff_sofar++; } while(ch == SPACE);
    --buff_sofar;

    if(!isdigit(*buff_sofar))
    {
        /* for define key need to return chars, return as negative */
        ch = *buff_sofar++;
        if(ch == CH_NULL)
            return(NO_VAL);

        return((coord) (0 - (coord) ch));
    }

    consume_int(sscanf((char *) buff_sofar, "%d%n", &res, &count));

    if(0 == count)
        return(NO_VAL);

    buff_sofar += count;

    return(res);
}

/******************************************************************************
*
*                                Menus
*
******************************************************************************/

/******************************************************************************
*
* menu definitions
*
******************************************************************************/

/* special menu item: draw line across menu at this point */

#define MSP \
    { NULL, 0, NULL, NULL, 0, 0, 0 }

#define MIK(command, flags, title, cmdfunc, funcnum, classic_key, sg_key) \
    { (command), (flags), & (title), (cmdfunc), (funcnum), (classic_key), (sg_key) }

#define MIT(command, flags, title, cmdfunc, funcnum) \
    { (command), (flags), & (title), (cmdfunc), (funcnum), 0, 0 }

/* SKS 23oct96 introduces shortforms for compact table descriptions */
#define SHT 0
#define LNG (MF_LONG_ONLY)
#define DRQ (MF_DOCUMENT_REQUIRED)
#define GEX (MF_GREY_EXPEDIT)
#define DLG (MF_HAS_DIALOG)
#define MBF (MF_DO_MERGEBUF)
#define NMF (MF_NO_MACROFILE)
#define NRP (MF_NO_REPEAT)
#define NEU (MF_NOMENU)
#define TCK (MF_TICKABLE)

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
files_menu[] =
{
    MIK("Fl",   SHT |       GEX | DLG,                         Load_file_STR,                  LoadFile_fn,               N_LoadFile,                 0, SG_KEY_LOAD_DOCUMENT),
    MIK("Fgt",  LNG | DRQ | GEX | DLG | MBF       | NRP,       Insert_file_STR,                InsertFile_fn,             N_InsertFile,               0, SG_KEY_INSERT_DOCUMENT),
    MIK("Fs",   SHT | DRQ | GEX | DLG | MBF | NMF | NRP,       Save_file_STR,                  SaveFile_fn,               N_SaveFile,                 CLASSIC_KEY_SAVE, SG_KEY_SAVE_DOCUMENT),
    MIT("Fi",   SHT | DRQ | GEX | DLG,                         Save_template_STR,              SaveTemplateFile_fn,       N_SaveTemplateFile),
    MIT("Fc",   LNG | DRQ | GEX | DLG,                         Name_STR,                       NameFile_fn,               N_NameFile),
    MIT("Fa",   SHT | DRQ       | DLG,                         About_STR,                      About_fn,                  N_About),
    MSP,

    MIT("Bnew", LNG       | GEX,                               New_window_STR,                 NewWindow_fn,              N_NewWindow),
    MIT("Fw",   LNG | DRQ | GEX       | MBF,                   Next_window_STR,                NextWindow_fn,             N_NextWindow),
    MIK("Fq",   LNG | DRQ | GEX       | MBF,                   Close_window_STR,               CloseWindow_fn,            N_CloseWindow,              0, SG_KEY_CLOSE_WINDOW),
    MSP,
    MIT("O",    SHT | DRQ       | DLG,                         Options_STR,                    Options_fn,                N_Options),
#if defined(EXTENDED_COLOUR_WINDVARS)
    MIT("Fr",   SHT             | DLG,                         Colours_STR,                    Colours_fn,                N_Colours),
#endif
    MSP,

    MIT("A",    LNG | DRQ | GEX       | MBF,                   Recalculate_STR,                Recalculate_fn,            N_Recalculate),
    MIK("J",    SHT |       GEX             | NMF | NRP,       RepeatCommand_STR,              RepeatCommand_fn,          N_RepeatCommand,            CLASSIC_KEY_REPEAT_COMMAND, 0),
    MSP,

/* need address of tickable entry */
#define record_option files_menu[16]
    MIT("Fy",   LNG | DRQ | GEX             | NMF | TCK,       Record_macro_file_STR,          RecordMacroFile_fn,        N_RecordMacroFile),
    MIT("Fz",   LNG | DRQ | GEX | DLG | MBF | NMF,             Do_macro_file_STR,              DoMacroFile_fn,            N_DoMacroFile)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
edit_menu[] =
{
    MIT("R",    SHT | DRQ | GEX       | MBF,                   Format_paragraph_STR,           FormatParagraph_fn,        N_FormatParagraph),
    MIT("Efb",  SHT | DRQ | GEX       | MBF,                   FormatBlock_STR,                FormatBlock_fn,            N_FormatBlock),
    MSP,

    MIK("G",    LNG | DRQ | GEX,                               Delete_character_STR,           DeleteCharacterRight_fn,   N_DeleteCharacterRight,     KMAP_FUNC_SDELETE, 0),
    MIK("U",    LNG | DRQ | GEX,                               Insert_space_STR,               InsertSpace_fn,            N_InsertSpace,              KMAP_FUNC_INSERT, 0),
    MIK("T",    LNG | DRQ | GEX,                               Delete_word_STR,                DeleteWord_fn,             N_DeleteWord,               CLASSIC_KEY_DELETE_WORD, 0),
    MIK("D",    LNG | DRQ | GEX,                               Delete_to_end_of_slot_STR,      DeleteToEndOfSlot_fn,      N_DeleteToEndOfSlot,        CLASSIC_KEY_DELETE_EOL, 0),
    MIK("I",    SHT | DRQ | GEX,                               Paste_STR,                      Paste_fn,                  N_Paste,                    CLASSIC_KEY_PASTE, (KMAP_CODE_ADDED_CTRL | 'V')),
    MSP,

    MIT("ss",   LNG | DRQ | GEX,                               Swap_case_STR,                  SwapCase_fn,               N_SwapCase),
    MIK("X",    SHT | DRQ | GEX             | NMF,             Edit_formula_STR,               EditFormula_fn,            N_EditFormula,              CLASSIC_KEY_EDIT_EXPRESSION, 0),
    MIT("Efw",  LNG | DRQ                   | NMF,             Edit_formula_in_window_STR,     EditFormulaInWindow_fn,    N_EditFormulaInWindow),
    MSP,

    MIT("Esl",  LNG | DRQ | GEX       | MBF,                   Split_line_STR,                 SplitLine_fn,              N_SplitLine),
    MIT("Ejl",  LNG | DRQ | GEX       | MBF,                   Join_lines_STR,                 JoinLines_fn,              N_JoinLines),
    MIK("N",    LNG | DRQ | GEX       | MBF,                   Insert_row_STR,                 InsertRow_fn,              N_InsertRow,                CLASSIC_KEY_INSERT_ROW, SG_KEY_INSERT_ROW),
    MIK("Y",    LNG | DRQ | GEX /*no mergebuf*/,               Delete_row_STR,                 DeleteRow_fn,              N_DeleteRow,                CLASSIC_KEY_DELETE_ROW, SG_KEY_DELETE_ROW),
    MIT("Eirc", LNG | DRQ | GEX       | MBF,                   Insert_row_in_column_STR,       InsertRowInColumn_fn,      N_InsertRowInColumn),
    MIT("Edrc", LNG | DRQ | GEX /*no mergebuf*/,               Delete_row_in_column_STR,       DeleteRowInColumn_fn,      N_DeleteRowInColumn),
    MIK("Eic",  SHT | DRQ | GEX       | MBF,                   Insert_column_STR,              InsertColumn_fn,           N_InsertColumn,             CLASSIC_KEY_INSERT_COLUMN, SG_KEY_INSERT_COLUMN),
    MIK("Edc",  SHT | DRQ | GEX       | MBF,                   Delete_column_STR,              DeleteColumn_fn,           N_DeleteColumn,             CLASSIC_KEY_DELETE_COLUMN, SG_KEY_DELETE_COLUMN),
    MIT("Eac",  SHT | DRQ | GEX       | MBF,                   Add_column_STR,                 AddColumn_fn,              N_AddColumn),
    MIT("Eip",  SHT | DRQ | GEX | DLG,                         Insert_page_break_STR,          InsertPageBreak_fn,        N_InsertPageBreak),
    MIT("Eitc", SHT | DRQ | GEX | DLG       | NMF,             Insert_colour_change_STR,       InsertColourChange_fn,     N_InsertColourChange),
    MIK("Eitd", SHT | DRQ | GEX | DLG,                         Insert_date_STR,                InsertDate_fn,             N_InsertDate,               0, (KMAP_CODE_ADDED_CTRL | 'D')),
    MIK("Eitt", SHT | DRQ | GEX | DLG,                         Insert_time_STR,                InsertTime_fn,             N_InsertTime,               0, (KMAP_CODE_ADDED_CTRL | 'T')),
    MIK("Eitp", SHT | DRQ | GEX | DLG,                         Insert_page_number_STR,         InsertPageNumber_fn,       N_InsertPageNumber,         0, (KMAP_CODE_ADDED_CTRL | 'P'))
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
layout_menu[] =
{
    MIT("W",    LNG | DRQ | GEX | DLG,                         Set_column_width_STR,           ColumnWidth_fn,            N_ColumnWidth),
    MIT("H",    LNG | DRQ | GEX | DLG,                         Set_right_margin_STR,           RightMargin_fn,            N_RightMargin),
    MIT("Lml",  LNG | DRQ | GEX,                               Move_margin_left_STR,           MoveMarginLeft_fn,         N_MoveMarginLeft),
    MIT("Lmr",  LNG | DRQ | GEX,                               Move_margin_right_STR,          MoveMarginRight_fn,        N_MoveMarginRight),
    MIT("Law",  SHT | DRQ | GEX       | MBF,                   Auto_width_STR,                 AutoWidth_fn,              N_AutoWidth),
    MIT("Lco",  SHT | DRQ | GEX,                               Link_columns_STR,               LinkColumns_fn,            N_LinkColumns),
    MSP,

    MIT("Lfr",  SHT | DRQ | GEX       | MBF,                   Fix_row_STR,                    FixRows_fn,                N_FixRows),
    MIT("Lfc",  SHT | DRQ | GEX       | MBF,                   Fix_column_STR,                 FixColumns_fn,             N_FixColumns),
    MSP,

    MIT("Lal",  SHT | DRQ | GEX       | MBF,                   Left_align_STR,                 LeftAlign_fn,              N_LeftAlign),
    MIT("Lac",  SHT | DRQ | GEX       | MBF,                   Centre_align_STR,               CentreAlign_fn,            N_CentreAlign),
    MIT("Lar",  SHT | DRQ | GEX       | MBF,                   Right_align_STR,                RightAlign_fn,             N_RightAlign),
    MIT("Llcr", LNG | DRQ | GEX       | MBF,                   LCR_align_STR,                  LCRAlign_fn,               N_LCRAlign),
    MIT("Laf",  SHT | DRQ | GEX       | MBF,                   Free_align_STR,                 FreeAlign_fn,              N_FreeAlign),
    MSP,

    MIT("Ldp",  SHT | DRQ | GEX | DLG,                         Decimal_places_STR,             DecimalPlaces_fn,          N_DecimalPlaces),
    MIT("Lsb",  LNG | DRQ | GEX       | MBF,                   Sign_brackets_STR,              SignBrackets_fn,           N_SignBrackets),
    MIT("Lsm",  LNG | DRQ | GEX       | MBF,                   Sign_minus_STR,                 SignMinus_fn,              N_SignMinus),
    MIT("Lcl",  SHT | DRQ | GEX       | MBF,                   Leading_characters_STR,         LeadingCharacters_fn,      N_LeadingCharacters),
    MIT("Lct",  SHT | DRQ | GEX       | MBF,                   Trailing_characters_STR,        TrailingCharacters_fn,     N_TrailingCharacters),
    MIT("Ldf",  LNG | DRQ | GEX       | MBF,                   Default_format_STR,             DefaultFormat_fn,          N_DefaultFormat)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
print_menu[] =
{
    MIK("Po",   SHT | DRQ | GEX | DLG,                         Print_STR,                      Print_fn,                  N_Print,                    KMAP_FUNC_PRINT, KMAP_FUNC_PRINT),
    MIT("Py",   SHT | DRQ | GEX | DLG,                         Page_layout_STR,                PageLayout_fn,             N_PageLayout),
    MIT("Pd",   SHT | DRQ | GEX | DLG,                         Printer_configuration_STR,      PrinterConfig_fn,          N_PrinterConfig),
    MIT("Pm",   LNG | DRQ | GEX | DLG,                         Microspace_pitch_STR,           MicrospacePitch_fn,        N_MicrospacePitch),
    MIT("Pgd",  LNG | DRQ | GEX | DLG,                         Edit_printer_driver_STR,        EditPrinterDriver_fn,      N_EditPrinterDriver),
    MSP,

    MIT("Pfg",  SHT | DRQ | GEX | DLG,                         Printer_font_STR,               PrinterFont_fn,            N_PRINTERFONT),
    MIT("Pfl",  SHT | DRQ | GEX | DLG,                         Insert_font_STR,                InsertFont_fn,             N_INSERTFONT),
    MSP,

    MIK("Pb",   SHT | DRQ | GEX,                               Bold_STR,                       Bold_fn,                   N_Bold,                     0, (KMAP_CODE_ADDED_CTRL | 'B')),
    MIK("Pi",   SHT | DRQ | GEX,                               Italic_STR,                     Italic_fn,                 N_Italic,                   0, (KMAP_CODE_ADDED_CTRL | 'I')),
    MIT("Pu",   SHT | DRQ | GEX,                               Underline_STR,                  Underline_fn,              N_Underline),
    MIT("Pl",   SHT | DRQ | GEX,                               Subscript_STR,                  Subscript_fn,              N_Subscript),
    MIT("Pr",   SHT | DRQ | GEX,                               Superscript_STR,                Superscript_fn,            N_Superscript),
    MIT("Px",   LNG | DRQ | GEX,                               Extended_sequence_STR,          ExtendedSequence_fn,       N_ExtendedSequence),
    MIT("Pa",   LNG | DRQ | GEX,                               Alternate_font_STR,             AlternateFont_fn,          N_AlternateFont),
    MIT("Pe",   LNG | DRQ | GEX,                               User_defined_STR,               UserDefinedHigh_fn,        N_UserDefinedHigh),
    MSP,

    MIT("Phb",  LNG | DRQ | GEX | DLG | MBF,                   Highlight_block_STR,            HighlightBlock_fn,         N_HighlightBlock),
    MIT("Phr",  LNG | DRQ | GEX | DLG | MBF,                   Remove_highlights_STR,          RemoveHighlights_fn,       N_RemoveHighlights)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
blocks_menu[] =
{
    MIT("Z",    LNG | DRQ | GEX       | MBF,                   Mark_block_STR,                 MarkSlot_fn,               N_MarkSlot),
    MIT("Q",    LNG | DRQ             | MBF,                   Clear_markers_STR,              ClearMarkedBlock_fn,       N_ClearMarkedBlock),
    MSP,

    MIT("Bc",   SHT | DRQ | GEX       | MBF,                   Copy_block_STR,                 CopyBlock_fn,              N_CopyBlock),
    MIT("Bf",   LNG | DRQ | GEX       | MBF,                   Copy_block_to_paste_list_STR,   CopyBlockToPasteList_fn,   N_CopyBlockToPasteList),
    MIT("Bm",   SHT | DRQ | GEX       | MBF,                   Move_block_STR,                 MoveBlock_fn,              N_MoveBlock),
    MIT("Bd",   SHT | DRQ | GEX       | MBF,                   Delete_block_STR,               DeleteBlock_fn,            N_DeleteBlock),
    MIT("Blc",  SHT | DRQ | GEX       | MBF,                   Clear_block_STR,                ClearBlock_fn,             N_ClearBlock),
    MSP,

    MIT("Brd",  SHT | DRQ | GEX       | MBF,                   Replicate_down_STR,             ReplicateDown_fn,          N_ReplicateDown),
    MIT("Bru",  LNG | DRQ | GEX       | MBF,                   Replicate_up_STR,               ReplicateUp_fn,            N_ReplicateUp),
    MIT("Brr",  SHT | DRQ | GEX       | MBF,                   Replicate_right_STR,            ReplicateRight_fn,         N_ReplicateRight),
    MIT("Brl",  LNG | DRQ | GEX       | MBF,                   Replicate_left_STR,             ReplicateLeft_fn,          N_ReplicateLeft),
    MIT("Bre",  SHT | DRQ | GEX | DLG | MBF,                   Replicate_STR,                  Replicate_fn,              N_Replicate),
    MIK("Bso",  SHT | DRQ | GEX | DLG | MBF,                   Sort_STR,                       SortBlock_fn,              N_SortBlock,                CLASSIC_KEY_SORT, SG_KEY_SORT),
/* following needs changing to Transpose_block_STR when everyone wants to recompile everything again */
    MIT("Bt",   LNG | DRQ | GEX       | MBF,                   SpreadArray_STR,                TransposeBlock_fn,         N_TransposeBlock),
    MIK("Bse",  SHT | DRQ       | DLG,                         Search_STR,                     Search_fn,                 N_Search,                   CLASSIC_KEY_SEARCH, SG_KEY_FIND),
    MIK("Bnm",  SHT | DRQ,                                     Next_match_STR,                 NextMatch_fn,              N_NextMatch,                CLASSIC_KEY_NEXT_MATCH, 0),
    MIT("Bpm",  SHT | DRQ,                                     Previous_match_STR,             PrevMatch_fn,              N_PrevMatch),
    MSP,

    MIT("Bps",  LNG | DRQ | GEX       | MBF,                   Set_protection_STR,             SetProtectedBlock_fn,      N_SetProtectedBlock),
    MIT("Bpc",  LNG | DRQ | GEX       | MBF,                   Clear_protection_STR,           ClearProtectedBlock_fn,    N_ClearProtectedBlock),
    MSP,

    MIT("Bln",  SHT | DRQ | GEX       | MBF,                   ToNumber_STR,                   ToNumber_fn,               N_ToNumber),
    MIT("Blt",  SHT | DRQ | GEX       | MBF,                   ToText_STR,                     ToText_fn,                 N_ToText),
    MIT("Bnt",  SHT | DRQ | GEX       | MBF,                   Number_X_Text_STR,              ExchangeNumbersText_fn,    N_ExchangeNumbersText),
    MIT("Bsc",  SHT | DRQ | GEX       | MBF,                   ToConstant_STR,                 ToConstant_fn,             N_ToConstant),
    MIT("Bss",  SHT | DRQ | GEX       | MBF,                   Snapshot_STR,                   Snapshot_fn,               N_Snapshot),
    MIT("Bwc",  SHT | DRQ | GEX       | MBF,                   Word_count_STR,                 WordCount_fn,              N_WordCount)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
cursor_menu[] =
{
    MIK("Cfc",  SHT | DRQ             | MBF | NRP,             First_column_STR,               FirstColumn_fn,            N_FirstColumn,              KMAP_FUNC_CSTAB, 0),
    MIK("Clc",  SHT | DRQ             | MBF | NRP,             Last_column_STR,                LastColumn_fn,             N_LastColumn,               KMAP_FUNC_CTAB, 0),
    MIK("Cnw",  SHT | DRQ | GEX             | NRP,             Next_word_STR,                  NextWord_fn,               N_NextWord,                 KMAP_FUNC_SARROW_RIGHT, 0),
    MIK("Cpw",  SHT | DRQ | GEX             | NRP,             Previous_word_STR,              PrevWord_fn,               N_PrevWord,                 KMAP_FUNC_SARROW_LEFT, 0),
    MIT("Cwi",  SHT | DRQ             | MBF | NRP,             Centre_window_STR,              CentreWindow_fn,           N_CentreWindow),
    MSP,

    MIT("Csp",  LNG | DRQ,                                     Save_position_STR,              SavePosition_fn,           N_SavePosition),
    MIT("Crp",  LNG | DRQ             | MBF,                   Restore_position_STR,           RestorePosition_fn,        N_RestorePosition),
    MIT("Cwc",  LNG | DRQ             | MBF,                   Swap_position_and_caret_STR,    SwapPosition_fn,           N_SwapPosition),
    MIK("Cgs",  LNG | DRQ       | DLG,                         Go_to_slot_STR,                 GotoSlot_fn,               N_GotoSlot,                 0, SG_KEY_GOTO),
    MSP,

    MIT("Cdk",  LNG             | DLG,                         Define_key_STR,                 DefineKey_fn,              N_DefineKey),
    MIT("Cdf",  LNG             | DLG,                         Define_function_key_STR,        DefineFunctionKey_fn,      N_DefineFunctionKey),
    MIT("Cdc",  LNG             | DLG,                         Define_command_STR,             DefineCommand_fn,          N_DefineCommand)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
spell_menu[] =  /*no mergebufs*/
{
    MIT("Sc",   SHT | DRQ | GEX | DLG,                         Check_document_STR,             CheckDocument_fn,          N_CheckDocument),
    MIT("Sd",   LNG             | DLG,                         Delete_word_from_user_dict_STR, DeleteWordFromDict_fn,     N_DeleteWordFromDict),
    MIT("Si",   LNG             | DLG,                         Insert_word_in_user_dict_STR,   InsertWordInDict_fn,       N_InsertWordInDict),
    MSP,

    MIT("Sb",   SHT             | DLG,                         Browse_STR,                     BrowseDictionary_fn,       N_BrowseDictionary),
    MIT("Su",   LNG             | DLG,                         Dump_dictionary_STR,            DumpDictionary_fn,         N_DumpDictionary),
    MIT("Sm",   LNG             | DLG,                         Merge_file_with_user_dict_STR,  MergeFileWithDict_fn,      N_MergeFileWithDict),
    MIT("Sg",   SHT             | DLG,                         Anagrams_STR,                   Anagrams_fn,               N_Anagrams),
    MIT("Sh",   SHT             | DLG,                         Subgrams_STR,                   Subgrams_fn,               N_Subgrams),
    MSP,

    MIT("Sn",   LNG             | DLG,                         Create_user_dictionary_STR,     CreateUserDict_fn,         N_CreateUserDict),
    MIT("So",   LNG             | DLG,                         Open_user_dictionary_STR,       OpenUserDict_fn,           N_OpenUserDict),
    MIT("Sz",   LNG             | DLG,                         Close_user_dictionary_STR,      CloseUserDict_fn,          N_CloseUserDict),
    MIT("Sf",   LNG,                                           Flush_user_dictionary_STR,      FlushUserDict_fn,          N_FlushUserDict),
    MIT("Sp",   LNG             | DLG,                         Pack_user_dictionary_STR,       PackUserDict_fn,           N_PackUserDict),
    MIT("Sq",   LNG             | DLG,                         Dictionary_options_STR,         DictionaryOptions_fn,      N_DictionaryOptions),
    MIT("Sl",   LNG             | DLG,                         Lock_dictionary_STR,            LockDictionary_fn,         N_LockDictionary),
    MIT("Sk",   LNG             | DLG,                         Unlock_dictionary_STR,          UnlockDictionary_fn,       N_UnlockDictionary)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
charts_menu[] =
{
    MIT("Chn",  SHT | DRQ | GEX       | MBF,                   New_chart_STR,                  ChartNew_fn,               N_ChartNew),
    MIT("Cho",  SHT | DRQ | GEX | DLG,                         Chart_options_STR,              ChartOptions_fn,           N_ChartOptions),
    MIT("Cha",  SHT | DRQ | GEX | DLG | MBF,                   Add_to_chart_STR,               ChartAdd_fn,               N_ChartAdd),
    MIT("Che",  SHT | DRQ | GEX | DLG,                         Edit_chart_STR,                 ChartEdit_fn,              N_ChartEdit),
    MIT("Chd",  SHT | DRQ | GEX | DLG,                         Delete_chart_STR,               ChartDelete_fn,            N_ChartDelete),
    MIT("Chs",  SHT | DRQ | GEX | DLG,                         Select_chart_STR,               ChartSelect_fn,            N_ChartSelect)
};

static MENU
help_menu[] =
{
    MIK("Fh",   SHT,                                           Help_STR,                       Help_fn,                   N_Help,                     CLASSIC_KEY_HELP, SG_KEY_HELP),
    MSP,

    MIT("Fghl", SHT,                                           Licence_STR,                    ShowLicence_fn,            N_ShowLicence),
    MSP,

    MIT("Fghi", SHT,                                           Interactive_help_STR,           InteractiveHelp_fn,        N_InteractiveHelp)
};

/******************************************************************************
*
* the rest are commands which don't go on menus
* they are stored for the function and cursor key definitions
*
******************************************************************************/

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
random_menu[] =
{
    MIK("Ccu",  SHT | DRQ | GEX       | MBF       | NRP | NEU, Cursor_up_STR,                  CursorUp_fn,               N_CursorUp,                 KMAP_FUNC_ARROW_UP, KMAP_FUNC_ARROW_UP),
    MIK("Ccd",  SHT | DRQ | GEX       | MBF       | NRP | NEU, Cursor_down_STR,                CursorDown_fn,             N_CursorDown,               KMAP_FUNC_ARROW_DOWN, KMAP_FUNC_ARROW_DOWN),
    MIK("Ccl",  SHT | DRQ | GEX                   | NRP | NEU, Cursor_left_STR,                CursorLeft_fn,             N_CursorLeft,               KMAP_FUNC_ARROW_LEFT, KMAP_FUNC_ARROW_LEFT),
    MIK("Ccr",  SHT | DRQ | GEX                   | NRP | NEU, Cursor_right_STR,               CursorRight_fn,            N_CursorRight,              KMAP_FUNC_ARROW_RIGHT, KMAP_FUNC_ARROW_RIGHT),

    MIK("Csu",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_up_STR,                  ScrollUp_fn,               N_ScrollUp,                 KMAP_FUNC_CSARROW_UP, 0),
    MIK("Csd",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_down_STR,                ScrollDown_fn,             N_ScrollDown,               KMAP_FUNC_CSARROW_DOWN, 0),
    MIK("Csl",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_left_STR,                ScrollLeft_fn,             N_ScrollLeft,               KMAP_FUNC_CSARROW_LEFT, 0),
    MIK("Csr",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_right_STR,               ScrollRight_fn,            N_ScrollRight,              KMAP_FUNC_CSARROW_RIGHT, 0),

    MIK("Cpu",  SHT | DRQ             | MBF       | NRP | NEU, Page_up_STR,                    PageUp_fn,                 N_PageUp,                   KMAP_FUNC_SARROW_UP, 0),
    MIK("Cpd",  SHT | DRQ             | MBF       | NRP | NEU, Page_down_STR,                  PageDown_fn,               N_PageDown,                 KMAP_FUNC_SARROW_DOWN, 0),
    MIT("Cpl",  SHT | DRQ             | MBF       | NRP | NEU, Page_left_STR,                  PageLeft_fn,               N_PageLeft),
    MIT("Cpr",  SHT | DRQ             | MBF       | NRP | NEU, Page_right_STR,                 PageRight_fn,              N_PageRight),

    MIK("Ctc",  SHT | DRQ             | MBF       | NRP | NEU, Top_of_column_STR,              TopOfColumn_fn,            N_TopOfColumn,              KMAP_FUNC_CARROW_UP, 0),
    MIK("Cbc",  SHT | DRQ             | MBF       | NRP | NEU, Bottom_of_column_STR,           BottomOfColumn_fn,         N_BottomOfColumn,           KMAP_FUNC_CARROW_DOWN, 0),
    MIK("Cpc",  SHT | DRQ             | MBF       | NRP | NEU, Previous_column_STR,            PrevColumn_fn,             N_PrevColumn,               KMAP_FUNC_STAB, KMAP_FUNC_STAB),
    MIK("Cnc",  SHT | DRQ             | MBF       | NRP | NEU, Next_column_STR,                NextColumn_fn,             N_NextColumn,               KMAP_FUNC_TAB, KMAP_FUNC_TAB),

    MIK("Cbs",  SHT | DRQ | GEX                   | NRP | NEU, Start_of_slot_STR,              StartOfSlot_fn,            N_StartOfSlot,              KMAP_FUNC_CARROW_LEFT, 0),
    MIK("Ces",  SHT | DRQ | GEX                   | NRP | NEU, End_of_slot_STR,                EndOfSlot_fn,              N_EndOfSlot,                KMAP_FUNC_CARROW_RIGHT, 0),

    MIK("Cen",  SHT | DRQ                         | NRP | NEU, Enter_STR,                      Return_fn,                 N_Return,                   CR, CR),
    MIK("Cx",   SHT | DRQ                               | NEU, Escape_STR,                     Escape_fn,                 N_Escape,                   ESCAPE, ESCAPE),
    MIK("Crb",  SHT | DRQ | GEX                   | NRP | NEU, Rubout_STR,                     DeleteCharacterLeft_fn,    N_DeleteCharacterLeft,      KMAP_FUNC_DELETE, KMAP_FUNC_DELETE),
    MIT("Ccp",  SHT | DRQ | GEX                         | NEU, Pause_STR,                      Pause_fn,                  N_Pause),

    MIK("Brp",  SHT | DRQ                               | NEU, Replace_STR,                    Replace_fn,                N_OldSearch,                0, SG_KEY_REPLACE),

    MIT("Ent",  SHT | DRQ | GEX       | MBF             | NEU, Number_X_Text_STR,              ExchangeNumbersText_fn,    N_OldExchangeNumbersAndText),
    MIT("Esc",  SHT | DRQ | GEX       | MBF             | NEU, New_slot_contents_STR,          Newslotcontents_fn,        N_Newslotcontents),

    MIK("Fgz",  SHT | DRQ | GEX       | MBF             | NEU, Mark_sheet_STR,                 MarkSheet_fn,              N_MarkSheet,                0, (KMAP_CODE_ADDED_CTRL | 'A')),
    MIT("Fgf",  SHT | DRQ                               | NEU, FullScreen_STR,                 FullScreen_fn,             N_FullScreen),
    MIT("Fgs",  SHT | DRQ | GEX       | MBF             | NEU, Save_file_STR,                  SaveFileAsIs_fn,           N_SaveFileAsIs),
    MIT("Fgl",  SHT       | GEX | DLG                   | NEU, Load_Template_STR,              LoadTemplate_fn,           N_LoadTemplate),
    MIT("Fx",   SHT       | GEX                         | NEU, Exit_STR,                       Quit_fn,                   N_Quit)
};

/* progvars control fns moved to iconbar menu */

/*              LNG . DRQ . gex . DLG . MBF . NMF . NRP */

static MENU
choices_menu[] =
{
/* need address of tickable entry */
#define classic_menu_option choices_menu[0]
    MIT("Fgc",  MF_ALWAYS_SHORT                         | NEU | TCK, Classic_menus_STR,              MenuStyle_fn,              N_MenuStyle),
#define short_menu_option choices_menu[1]
    MIT("M",    MF_ALWAYS_SHORT                         | NEU | TCK, Short_menus_STR,                MenuSize_fn,               N_MenuSize),
    MIT("Fgm",  MF_ALWAYS_SHORT                         | NEU,       ChangeMenus_STR,                ChangeMenus_fn,            N_ChangeMenus),
    MSP,

    MIT("Se",   SHT             | DLG                   | NEU,       Display_user_dictionaries_STR,  DisplayOpenDicts_fn,       N_DisplayOpenDicts),
/* need address of tickable entry */
#define check_option    choices_menu[5]
    MIT("Sa",   SHT                                     | NEU | TCK, Auto_check_STR,                 AutoSpellCheck_fn,         N_AutoSpellCheck),
    MSP,

/* need address of tickable entry */
#define recalc_option   choices_menu[7]
    MIT("Fo",   SHT                                     | NEU | TCK, Auto_recalculation_STR,         AutoRecalculation_fn,      N_AutoRecalculation),
/* need address of tickable entry */
#define chart_recalc_option choices_menu[8]
    MIT("Chc",  SHT                                     | NEU | TCK, Auto_chart_recalculation_STR,   AutoChartRecalculation_fn, N_AutoChartRecalculation),
#if !defined(EXTENDED_COLOUR_WINDVARS)
    MIT("Fr",   SHT             | DLG                   | NEU,       Colours_STR,                    Colours_fn,                N_Colours),
#endif
/* need address of tickable entry */
#define insert_overtype_option   choices_menu[10]
    MIT("V",    SHT                                     | NEU | TCK, Overtype_STR,                   InsertOvertype_fn,         N_InsertOvertype),
    MIT("Bpd",  SHT             | DLG                   | NEU,       Size_of_paste_list_STR,         PasteListDepth_fn,         N_PasteListDepth),
    MSP,

    MIT("Fgi",  SHT | DRQ                               | NEU,       SaveChoices_STR,                SaveChoices_fn,            N_SaveChoices),
};

/******************************************************************************
*
* define the menu heading structure
*
******************************************************************************/

#define menu__head(name, tail) \
    { (name), (tail), sizeof(tail)/sizeof(MENU), 0, 0, NULL }

#if TRACE_ALLOWED || defined(DEBUG)
static PC_U8 Random_STR = "Random";
#endif

/*extern*/ MENU_HEAD
iconbar_headline[] =
{
    menu__head(&ic_menu_title, choices_menu),
};

/*extern*/ MENU_HEAD
headline[] =
{   /* menu head title,                    submenu */
    menu__head(&Files_top_level_menu_STR,  files_menu),
    menu__head(&Edit_top_level_menu_STR,   edit_menu),
    menu__head(&Layout_top_level_menu_STR, layout_menu),
    menu__head(&Print_top_level_menu_STR,  print_menu),
    menu__head(&Blocks_top_level_menu_STR, blocks_menu),
    menu__head(&Cursor_top_level_menu_STR, cursor_menu),
    menu__head(&Spell_top_level_menu_STR,  spell_menu),
    menu__head(&Chart_top_level_menu_STR,  charts_menu),
    menu__head(&Help_top_level_menu_STR,   help_menu),

#if TRACE_ALLOWED || defined(DEBUG)
    /* allow user to see ALL commands */
    menu__head(&Random_STR, random_menu),
#endif

    menu__head(NULL,        random_menu)
};
/* note that the definition of HEAD_SIZE excludes the invisible 'random' menu */
#define HEAD_SIZE (elemof32(headline) - 1)

U32 head_size = HEAD_SIZE; /* exported for riscmenu.c */

/******************************************************************************
*
* get menu change option from ini file
*
******************************************************************************/

extern void
get_menu_change(
    const char *from)
{
    MENU * mptr;
    char * to = &cmd_seq_array[0];

    *to++ = UCH_REVERSE_SOLIDUS;
    for(; CH_NULL != *from; ++from)
        *to++ = toupper(*from);
    *to = CH_NULL;

    mptr = part_command(FALSE);
    if((mptr == M_IS_SUBSET) || (mptr == M_IS_ERROR))
        return;

    mptr->flags |= MF_LONG_SHORT_CHANGED;
}

/******************************************************************************
*
* save out as options any commands which are now on
* different menus to the default.
* Only for initialisation files
*
******************************************************************************/

static void
save_this_menu_changes(
    FILE_HANDLE output,
    const MENU_HEAD * const mhptr)
{
    const MENU * const firstmptr = mhptr->tail;
    const MENU * mptr = firstmptr + mhptr->items;

    while(--mptr >= firstmptr)
    {
        if(0 == (mptr->flags & MF_LONG_SHORT_CHANGED))
            continue; /* not changed */

        if( !away_string("%OP%" "MC", output) ||
            !away_string(mptr->command, output) ||
            !away_eol(output) )
        {
            break;
        }
    }
}

extern void
save_menu_changes(
    FILE_HANDLE output)
{
    U32 head_idx;

    for(head_idx = 0; head_idx < head_size; ++head_idx)
        save_this_menu_changes(output, &headline[head_idx]);

    save_this_menu_changes(output, &iconbar_headline[0]);
}

/******************************************************************************
*
*  size the menus (titlelen only)
*
******************************************************************************/

static void
resize_submenu(
    _Inout_     MENU_HEAD * const mhptr,
    BOOL classic_m,
    BOOL short_m)
{
    const MENU * mptr = mhptr->tail;
    const MENU * const lastmptr = mptr + mhptr->items;
    S32 offset = 1; /* menu offsets start at 1 */
    S32 maxtitlelen = 0;
    S32 maxcommandlen = 0;

    do  {
        const BOOL ignore = short_m  &&  !on_short_menus(mptr->flags);
        const char * ptr;
        S32 titlelen, commandlen;

        if(ignore)
            continue;

        if(NULL == mptr->title) /* ignore gaps */
            continue;

        ptr = * (char **) mptr->title;

        titlelen = strlen(ptr);

        if(classic_m)
            commandlen = strlen(mptr->command);
        else
            commandlen = 0;

        maxtitlelen   = MAX(maxtitlelen, titlelen);
        maxcommandlen = MAX(maxcommandlen, commandlen);

        ++offset;
    }
    while(++mptr < lastmptr);

    mhptr->titlelen   = maxtitlelen;
    mhptr->commandlen = maxcommandlen + 1;
    trace_2(TRACE_APP_PD4, "set titlelen for menu %s to %d", mhptr->name ? *(mhptr->name) : "<invisible>", mhptr->titlelen);
}

static void
resize_menus(
    BOOL classic_m,
    BOOL short_m)
{
    U32 head_idx;

    for(head_idx = 0; head_idx < head_size; ++head_idx)
        resize_submenu(&headline[head_idx], classic_m, short_m);

    resize_submenu(&iconbar_headline[0], classic_m, short_m);
}

/******************************************************************************
*
* initialise menu header line. only called at startup
*
* Sizes menus dynamically too
*
******************************************************************************/

extern void
headline_initialise_once(void)
{
    trace_0(TRACE_APP_PD4, "headline_initialise_once()");

    resize_menus(classic_menus(), short_menus());
}

/******************************************************************************
*
* Read the beginning of the command buffer, find and return function number.
* If there is a function return its number (offset in function table) otherwise
* generate an error and return a flag indicating error
*
* If the lookup is successful, cbuff_offset is set to point at the first
* parameter position.
*
******************************************************************************/

static MENU *
lukucm(
    BOOL allow_redef)
{
    S32 i = 0;
    MENU *mptr;

    cmd_seq_array[0] = UCH_REVERSE_SOLIDUS;
    do  {
        cmd_seq_array[i+1] = (uchar) toupper((int) cbuff[i+1]);
    }
    while(++i < MAX_COMM_SIZE);

    mptr = part_command(allow_redef);
    if((mptr == M_IS_ERROR) || (mptr == M_IS_SUBSET))
        return((MENU *) NO_COMM);

    return(mptr);
}

/******************************************************************************
*
* search for an expansion of the key value in the key expansion list.
* If not there, and the key is possibly a function key or control char look
* in the original definitions.  If there, copy into an array, adding \,\n
*
******************************************************************************/

static BOOL
schkvl_this_menu(
    S32 c,
    const MENU_HEAD * const mhptr)
{
    const MENU * const firstmptr = mhptr->tail;
    const MENU *mptr = firstmptr + mhptr->items;

    while(--mptr >= firstmptr)
    {
        const S32 this_key = classic_menus() ? mptr->classic_key : mptr->sg_key;

        if(this_key != c)
            continue;

        /* expand canonical command definition into buffer
         * adding leading \ and trailing \n
         */
        keyidx = expanded_key_buff;
        keyidx[0] = CMDLDI;
        strcpy(keyidx + 1, mptr->command);
        strcat(keyidx + 1, CR_STR);
        trace_2(TRACE_APP_PD4, "schkvl(%d) found: %s", c, trace_string(keyidx));
        return(TRUE);
    }

    return(FALSE);
}

extern BOOL
schkvl(
    S32 c)
{
    P_LIST lptr;

    lptr = search_list(&first_key, c);

    if(NULL != lptr)
    {
        keyidx = lptr->value;
    }
    else if((c != 0)  &&  ((c < (S32) SPACE)  ||  (c >= (S32) 127)))
    {
        const MENU_HEAD * const firstmhptr = headline;
        const MENU_HEAD * mhptr = firstmhptr + HEAD_SIZE + 1; /* check random_menu too */

        trace_0(TRACE_APP_PD4, "schkvl searching menu definitions");

        /* NB --mhptr kosher */
        while(--mhptr >= firstmhptr)
            if(schkvl_this_menu(c, mhptr))
                return(TRUE);

        if(schkvl_this_menu(c, &iconbar_headline[0]))
            return(TRUE);
    }

    if(keyidx  &&  !*keyidx)
        keyidx = NULL;

#if TRACE_ALLOWED
    if(keyidx)
        trace_1(TRACE_APP_PD4, "schkvl found: %s", trace_string(keyidx));
#endif

    return(keyidx != NULL);
}

/******************************************************************************
*
* scan all the menus to see if what we have is a part of a command
* if it is return TRUE
* if it is a whole command, do it and return FALSE
* if it is an invalid sequence, return FALSE
*
******************************************************************************/

static MENU *
part_command_core_menu(
    const MENU_HEAD * const mhptr)
{
    MENU * const firstmptr = mhptr->tail;
    MENU * mptr = firstmptr + mhptr->items;
    const char *aptr, *bptr;
    char a, b;

    while(--mptr >= firstmptr)
    {
        aptr = &cmd_seq_array[1]; /* past the Cmd-sequence flag byte */
        bptr = mptr->command;

        if(NULL != bptr)
        {
            do  {
                a = *aptr++;
                b = *bptr++;

                if((CH_NULL == a)  ||  (a == SPACE))
                {
                    if(CH_NULL == b)
                    {
                        trace_0(TRACE_APP_PD4, "found whole command");
                        cmd_seq_cancel(); /* used this one, about to return command pointer */
                        return(mptr);
                    }

                    trace_0(TRACE_APP_PD4, "found partial command");
                    return((MENU *) M_IS_SUBSET);
                }
            }
            while(a == toupper(b));
        }
    }

    return(NULL);
}

static MENU *
part_command_core(void)
{
    const MENU_HEAD * const firstmhptr = headline;
    const MENU_HEAD * mhptr = firstmhptr + (HEAD_SIZE + 1); /* check random_menu too */
    MENU * res;

    trace_1(TRACE_APP_PD4, "look up cmd_seq_array %s in menu structures", cmd_seq_array);

    if(CH_NULL == cmd_seq_array[1])
        return((MENU *) M_IS_SUBSET); /* explicit test makes clearer */

    /* NB --mhptr kosher */
    while(--mhptr >= firstmhptr)
        if((res = part_command_core_menu(mhptr)) != 0)
            return(res);

    if((res = part_command_core_menu(&iconbar_headline[0])) != 0)
        return(res);

    trace_0(TRACE_APP_PD4, "invalid sequence - discarding");
    cmd_seq_cancel();
    bleep();
    return(M_IS_ERROR);
}

static MENU *
part_command(
    BOOL allow_redef)
{
    PC_LIST lptr;
    const char *aptr, *bptr;
    char a, b;

    if(allow_redef)
    {
        trace_1(TRACE_APP_PD4, "look up cmd_seq_array %s on redefinition list", cmd_seq_array);

        for(lptr = first_in_list(&first_command_redef);
            lptr;
            lptr = next_in_list(&first_command_redef))
        {
            aptr = &cmd_seq_array[1]; /* past the Cmd-sequence flag byte */
            bptr = lptr->value;

            trace_2(TRACE_APP_PD4, "comparing cmd_seq_array %s with redef %s", aptr, bptr);

            if(NULL != bptr)
            {
                do  {
                    a = *aptr++;    /* cmd_seq_array always UC */
                    b = *bptr++;    /* def'n & redef'n always UC */

                    if((CH_NULL == a)  ||  (a == SPACE))
                    {
                        if(b == '_')
                        {
                            trace_0(TRACE_APP_PD4, "found full redefined command - look it up");
                            strcpy(&cmd_seq_array[1], bptr);
                            return(part_command_core());
                        }

                        trace_0(TRACE_APP_PD4, "found partial redefined command");
                        return(M_IS_SUBSET);
                    }
                }
                while(a == b);
            }
        }
    }

    return(part_command_core());
}

extern MENU *
find_loadcommand(void)
{
    return(files_menu);
}

static BOOL
find_command(
    MENU **mptrp,
    S32 funcnum)
{
    MENU * firstmptr, * mptr;

    trace_1(TRACE_APP_PD4, "find_command(%d)", funcnum);

    if(0 != funcnum)
    {
        const MENU_HEAD * const firstmhptr = headline;
        const MENU_HEAD * mhptr = firstmhptr + (HEAD_SIZE + 1); /* check random_menu too */

        /* NB --mhptr kosher */
        while(--mhptr >= firstmhptr)
        {
            firstmptr = mhptr->tail;
            mptr = firstmptr + mhptr->items;

            while(--mptr >= firstmptr)
            {
                trace_1(FALSE, "mptr->funcnum = %d", mptr->funcnum);

                if(mptr->funcnum != funcnum)
                    continue;

                *mptrp  = mptr;
                return(TRUE);
            }
        }

        /* check 'choices' last as that's rarer use */
        mhptr = &iconbar_headline[0];

        firstmptr = mhptr->tail;
        mptr = firstmptr + mhptr->items;

        while(--mptr >= firstmptr)
        {
            trace_1(FALSE, "mptr->funcnum = %d", mptr->funcnum);

            if(mptr->funcnum != funcnum)
                continue;

            *mptrp = mptr;
            return(TRUE);
        }
    }

    *mptrp = NULL;
    return(FALSE);
}

/******************************************************************************
*
* toggle short menu entries
*
******************************************************************************/

static BOOL toggle_short_menu_entry;

extern void
ChangeMenus_fn(void)
{
    toggle_short_menu_entry = TRUE;
}

static BOOL
process_toggle_short_menu_entry(
    MENU * mptr)
{
    toggle_short_menu_entry = FALSE;

    if(0 == (mptr->flags & MF_ALWAYS_SHORT))
    {
        /* toggle the menu change bit */
        mptr->flags ^= MF_LONG_SHORT_CHANGED;

        /* get new menus */
        riscmenu_clearmenutree();
        resize_menus(classic_menus(), short_menus());
        riscmenu_buildmenutree(classic_menus(), short_menus());
    }

    return(TRUE); /* for do_command() return value */
}

/*
*
* process a command for internal use e.g. reusing a command body for part of another function
*
*/

extern void
internal_process_command(
    S32 funcnum)
{
    MENU * mptr;

    if(find_command(&mptr, funcnum))
        (void) do_command(mptr, funcnum, FALSE);
}

extern BOOL
do_command(
    MENU * mptr,
    S32 funcnum,
    BOOL spool)
{
    BOOL to_comm_file = (spool && ((mptr->flags & MF_NO_MACROFILE) == 0));

    if(toggle_short_menu_entry)
        return(process_toggle_short_menu_entry(mptr)); /* rather than executing the command */

    trace_1(TRACE_APP_PD4, "do_command(%d)", funcnum);

    if(is_current_document())
    {
        /* fault commands that shouldn't be done in editing expression context */
        if(mptr->flags & MF_GREY_EXPEDIT)
        {
            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
            {
                reperr_null(ERR_EDITINGEXP);
                return(TRUE);
            }
        }

        /* many commands require buffers merged - but watch out for those that don't! */
        if(mptr->flags & MF_DO_MERGEBUF)
        {
            if(!mergebuf())
                return(TRUE);
        }
    }
    else
    {
        /* some commands can be done without a current document */

        if(mptr->flags & MF_DOCUMENT_REQUIRED)
        {
            reportf("no current document to process command");
            return(TRUE);
        }
    }

    if(to_comm_file && macro_recorder_on)
        out_comm_start_to_macro_file(funcnum);

    /* call xxx_fn() */
    (* mptr->cmdfunc) ();

    /* only output command if not Ctrl-FY */
    if(to_comm_file && macro_recorder_on)
        out_comm_end_to_macro_file(funcnum);

    return(TRUE);
}

/*
PipeDream 4 has funny formula line/window
when tick-click happens output the formula to the macro file
*/

extern void
output_formula_to_macro_file(
    char * str)
{
    char array[LIN_BUFSIZ+6];
    S32 funcnum = N_Newslotcontents;

    out_comm_start_to_macro_file(funcnum);

    really_out_comm_start();

    /* SKS after PD 4.12 01apr92 - formula wasn't reloaded correctly if it contained quotes */
    cvt_string_arg_for_macro_file(array, elemof32(array), str);

    if(output_string_to_macro_file(array))
        out_comm_end_to_macro_file(funcnum);
}

/******************************************************************************
*
* alt_letter takes a 16-bit character (e.g. from rdch()) and if it is
* an Alt-letter returns the upper case letter. Otherwise returns 0
*
******************************************************************************/

static uchar
alt_letter(
    S32 c)
{
    if((c <= 0)  ||  (0 == (c & KMAP_CODE_ADDED_ALT)))
        return(CH_NULL);

    return((uchar) (c & 0xFF));
}

/******************************************************************************
*
* get a menu item printed into core
* returns TRUE if submenu possible
*
******************************************************************************/

_Check_return_
static const char * /*low-lifetime*/
key_name_lookup(
    _InVal_     KMAP_CODE kmap_code)
{
    switch(kmap_code)
    {
    /* KMAP_BASE_FUNC range */
    case KMAP_FUNC_PRINT:       return("Print");

    case KMAP_BASE_FUNC + 1:    return("F1");
    case KMAP_BASE_FUNC + 2:    return("F2");
    case KMAP_BASE_FUNC + 3:    return("F3");
    case KMAP_BASE_FUNC + 4:    return("F4");
    case KMAP_BASE_FUNC + 5:    return("F5");
    case KMAP_BASE_FUNC + 6:    return("F6");
    case KMAP_BASE_FUNC + 7:    return("F7");
    case KMAP_BASE_FUNC + 8:    return("F8");
    case KMAP_BASE_FUNC + 9:    return("F9");
    case KMAP_BASE_FUNC + 10:   return("F10");
    case KMAP_BASE_FUNC + 11:   return("F11");
    case KMAP_BASE_FUNC + 12:   return("F12");

    case KMAP_FUNC_INSERT:      return("Insert");

    /* KMAP_BASE_FUNC2 range */
    case KMAP_FUNC_DELETE:      return("Delete");
    case KMAP_FUNC_HOME:        return("Home");
    case KMAP_FUNC_BACKSPACE:   return("Backspace");

    case KMAP_FUNC_TAB:         return("Tab");
    case KMAP_FUNC_END:         return("End");

    default:
        if((kmap_code >= 'A') && (kmap_code <= 'Z'))
        {
            static char buffer[16];
            buffer[0] = (char) kmap_code;
            buffer[1] = CH_NULL;
            return(buffer);
        }
        return(NULL);
    }
}

_Check_return_
extern BOOL
get_menu_item(
    const MENU_HEAD * const header,
    MENU *mptr,
    BOOL classic_m,
    char *array /*filled*/)
{
    char *out = array;
    const char *ptr;
    char ch;

    /* copy item title to array */
    strcpy(out, *mptr->title);
    out += strlen(out);

#if FALSE /* padding hasn't been needed for a VERY long time */
    /* pad title field with spaces */
    trace_2(TRACE_APP_PD4, "out - array = %d, header->titlelen = %d",
                out - array, header->titlelen);
    while((out - array) <= header->titlelen)
        *out++ = ' ';
#else
    UNREFERENCED_PARAMETER_InRef_(header);
    /* just a single space needed before command sequence */
#endif

    if(classic_m)
    {
        /* copy item command sequence to array */
        *out++ = ' ';
        *out++ = '^';
        ptr = mptr->command;
        while((ch = *ptr++) != CH_NULL)
            *out++ = toupper(ch);
    }
    else
    {
        if(0 != mptr->sg_key)
        {
            KMAP_CODE kmap_code = mptr->sg_key;
            const BOOL ctrl_added  = (0 != (kmap_code & KMAP_CODE_ADDED_CTRL ));
            const BOOL shift_added = (0 != (kmap_code & KMAP_CODE_ADDED_SHIFT));
            const char * base_name;

            if(ctrl_added)
                kmap_code &= ~KMAP_CODE_ADDED_CTRL;
            if(shift_added)
                kmap_code &= ~KMAP_CODE_ADDED_SHIFT;

            base_name = key_name_lookup(kmap_code);

            if(NULL != base_name)
            {
                *out++ = ' ';

                if(ctrl_added)
                    *out++ = '^';
                if(shift_added)
                    *out++ = '\x8B'; /* up arrow */

                strcpy(out, base_name);
                out += strlen(out);
            }
        }
    }

    *out = CH_NULL;

    return(0 != (mptr->flags & MF_HAS_DIALOG));
}

extern void
cmd_seq_cancel(void)
{
    cmd_seq_array[0] = CH_NULL; /* clears the Cmd-sequence flag byte */
    cmd_seq_array[1] = CH_NULL;
}

extern BOOL
cmd_seq_is_empty(void)
{
    return(CH_NULL == cmd_seq_array[0]); /* tests the Cmd-sequence flag byte */
}

/******************************************************************************
*
* --in--
*    +ve        char, maybe Alt
*      0        to be ignored
*    -ve        command number
*
* --out--
*    +ve        char
*      0        to be ignored, or error happened
*
******************************************************************************/

static S32
fiddle_alt_riscos(
    S32 ch)
{
    uchar kh = alt_letter(ch);

    trace_2(TRACE_APP_PD4, "fiddle_alt_riscos alt_letter: %d, %c",
            kh, kh);

    if(CH_NULL != kh)
    {
        uchar * str = &cmd_seq_array[1] + strlen((char *) &cmd_seq_array[1]);
        MENU * mptr;

        cmd_seq_array[0] = UCH_REVERSE_SOLIDUS;
        if(UCH_REVERSE_SOLIDUS != kh)
            *str++ = kh;
        *str = CH_NULL;
        trace_1(TRACE_APP_PD4, "cmd_seq_array := \"%s\"", cmd_seq_array);

        mptr = part_command(TRUE);
        if((mptr == M_IS_SUBSET) || (mptr == M_IS_ERROR))
            return(CH_NULL);

        return(0 - mptr->funcnum);
    }

    if(!cmd_seq_is_empty())
    {
        cmd_seq_cancel();
        bleep();
        return(CH_NULL);
    }

    return(ch);
}

/******************************************************************************
*
* switch macro recorder tick on or off
*
******************************************************************************/

static void
record_state_changed(void)
{
    optiontype opt;

    opt = record_option.flags;
    myassert0x(opt & MF_TICKABLE, "record state change failed");
    record_option.flags =
        macro_recorder_on
            ? opt |  MF_TICK_STATUS
            : opt & ~MF_TICK_STATUS;
}

/******************************************************************************
*
* switch auto check tick on or off
*
******************************************************************************/

extern void
check_state_changed(void)
{
    optiontype opt;

    opt = check_option.flags;
    myassert0x(opt & MF_TICKABLE, "check state change failed");
    check_option.flags =
        (d_auto[0].option == 'Y')
            ? opt |  MF_TICK_STATUS
            : opt & ~MF_TICK_STATUS;
}

/******************************************************************************
*
* switch insert/overtype tick on or off
*
******************************************************************************/

extern void
insert_overtype_state_may_have_changed(void)
{
    optiontype opt;

    opt = insert_overtype_option.flags;
    myassert0x(opt & MF_TICKABLE, "insert state change failed");
    insert_overtype_option.flags =
        (0 == d_progvars[OR_IO].option)
            ? opt |  MF_TICK_STATUS  /* overtype->ticked */
            : opt & ~MF_TICK_STATUS; /* insert->not ticked */
}

/******************************************************************************
*
* switch short menu tick on or off (and the classic menu tick too)
*
******************************************************************************/

extern void
menu_state_changed(void)
{
    optiontype opt;

    opt = short_menu_option.flags;
    opt = opt & ~MF_TICK_STATUS;
    if(short_menus())
        opt = opt | MF_TICK_STATUS;
    short_menu_option.flags = opt;
    myassert0x(opt & MF_TICKABLE, "short menu state change failed");

    opt = classic_menu_option.flags;
    opt = opt & ~MF_TICK_STATUS;
    if(classic_menus())
        opt = opt | MF_TICK_STATUS;
    classic_menu_option.flags = opt;
    myassert0x(opt & MF_TICKABLE, "classic menu state change failed");

    //reportf("Menus: classic %s, short %s", report_boolstring(classic_menus()), report_boolstring(short_menus()));
    resize_menus(classic_menus(), short_menus());
    riscmenu_buildmenutree(classic_menus(), short_menus());
}

/******************************************************************************
*
* switch autorecalc tick on or off
*
******************************************************************************/

extern void
recalc_state_may_have_changed(void)
{
    optiontype opt;

    opt = recalc_option.flags;
    myassert0x(opt & MF_TICKABLE, "recalc state change failed");
    recalc_option.flags =
        (d_progvars[OR_AM].option == 'A')
            ? opt |  MF_TICK_STATUS
            : opt & ~MF_TICK_STATUS;

    if(d_progvars[OR_AM].option == 'N')
        colh_draw_cell_count_in_document(NULL);
}

extern void
chart_recalc_state_may_have_changed(void)
{
    optiontype opt;

    opt = chart_recalc_option.flags;
    myassert0x(opt & MF_TICKABLE, "chart recalc state change failed");
    chart_recalc_option.flags =
        (d_progvars[OR_AC].option == 'A')
            ? opt |  MF_TICK_STATUS
            : opt & ~MF_TICK_STATUS;
}

extern void
MenuSize_fn(void)
{
    d_menu[0].option = (optiontype) !d_menu[0].option;

    menu_state_changed();

    riscmenu_clearmenutree();
}

extern void
MenuStyle_fn(void)
{
    d_menu[1].option = (optiontype) !d_menu[1].option;

    menu_state_changed();

    riscmenu_clearmenutree();
}

static void
AutoRecalculation_fn(void)
{
    d_progvars[OR_AM].option = d_progvars[OR_AM].option ^ 'A' ^ 'N'; /* goofy or what */

    recalc_state_may_have_changed();
}

static void
AutoChartRecalculation_fn(void)
{
    d_progvars[OR_AC].option = d_progvars[OR_AC].option ^ 'A' ^ 'M'; /* swap between Auto and Manual */

    chart_recalc_state_may_have_changed();
}

static char *       macro_file_name = NULL;
static FILE_HANDLE  macro_recorder_file = NULL;

static BOOL         macro_needs_lf = FALSE;
static const char * macro_command_name = NULL;
static BOOL         macro_fillin_failed = FALSE;

/* output string to macro file */

static BOOL
output_string_to_macro_file(
    char *str)
{
    BOOL res = away_string(str, macro_recorder_file);

    if(!res)
    {
        reperr(ERR_CANNOTWRITE, macro_file_name);
        stop_macro_recorder();
    }

    return(res);
}

extern void
output_char_to_macro_file(
    uchar ch)
{
    uchar array[3];
    uchar *ptr = array;

    if((ch == QUOTES)  ||  (ch == '|'))
        *ptr++ = '|';

    *ptr++ = ch;
    *ptr = CH_NULL;

    macro_needs_lf = TRUE;

    (void) output_string_to_macro_file(array);
}

/* output the command string to the macro file */

extern void
out_comm_start_to_macro_file(
    S32 funcnum)
{
    MENU * mptr;

    if(find_command(&mptr, funcnum))
    {
        macro_command_name = mptr->command;

        macro_fillin_failed = FALSE;
    }
}

static S32
really_out_comm_start(void)
{
    uchar array[LIN_BUFSIZ];

    if(macro_needs_lf)
    {
        macro_needs_lf = FALSE;
        (void) output_string_to_macro_file(CR_STR);
    }

    if(macro_command_name)
    {
        xstrkpy(array, elemof32(array), "\\");
        xstrkat(array, elemof32(array), macro_command_name);
        macro_command_name = NULL;
        if(output_string_to_macro_file(array))
            return(strlen(array));
    }

    return(0);
}

/* output the command string to the macro file */

extern void
out_comm_end_to_macro_file(
    S32 funcnum)
{
    uchar array[LIN_BUFSIZ];
    S32 sofar;
    MENU * mptr;

    if(!macro_fillin_failed && find_command(&mptr, funcnum))
    {
        sofar = really_out_comm_start();
                    /* 0123456789 */
        xstrkpy(array, elemof32(array), "|m        ");
        xstrkpy(array + (10 - sofar), elemof32(array) - (10 - sofar), *mptr->title);
        xstrkat(array, elemof32(array), CR_STR);

        macro_needs_lf = FALSE;

        (void) output_string_to_macro_file(array);
    }
}

/* SKS after PD 4.12 01apr92 - needed for \ESC */

static void
cvt_string_arg_for_macro_file(
    _Out_writes_z_(elemof_buffer) P_USTR ptr /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR arg)
{
    /* expand string in, putting | before " */
    U32 buffer_limit = elemof_buffer - 1; /* leave room for terminator */
    U32 offset = 0;
    U8 ch;

    xstrkpy(ptr, elemof_buffer, "|i \"");
    offset = strlen(ptr);

    while((ch = *arg++) != CH_NULL)
    {
        if(ch == QUOTES)
            if(offset < buffer_limit)
                ptr[offset++] = '|';

        if(offset < buffer_limit)
            ptr[offset++] = ch;
    }

    xstrkpy(&ptr[offset], elemof_buffer - offset, "\" ");
}

/* output the dialog box parameters to the macro file */

extern void
out_comm_parms_to_macro_file(
    DIALOG *dptr,
    S32 size, BOOL ok)
{
    uchar array[LIN_BUFSIZ];
    uchar *ptr, *textptr;
    DIALOG *dptri;
    S32 i;
    BOOL charout, numout, textout, arrayout;

    if(!ok)
    {
        macro_fillin_failed = TRUE;
        macro_needs_lf = FALSE;
        return;
    }

    really_out_comm_start();

    for(i = 0, dptri = dptr; i < size; i++, dptri++)
    {
        ptr = array;
        textptr = dptri->textfield;

        charout = FALSE;
        numout = FALSE;
        textout = FALSE;
        arrayout = FALSE;

        switch(dptr[i].type)
        {
        case F_ERRORTYPE:
            return;

        case F_COMPOSITE:
            charout = textout = TRUE;
            break;

        case F_ARRAY:
            arrayout = TRUE;
            break;

#if defined(EXTENDED_COLOUR)
        case F_COLOUR:
            {
            const U32 wimp_colour_value = get_option_as_u32(&dptri->option);
            U8 encoded_value[32];

            consume_int(encode_wimp_colour_value(encoded_value, sizeof32(encoded_value), wimp_colour_value));

            ptr += sprintf(ptr, "|i %s ", encoded_value);
            break;
            }
#endif /* EXTENDED_COLOUR */

        case F_LIST:
        case F_NUMBER:
#if !defined(EXTENDED_COLOUR)
        case F_COLOUR:
#endif
            numout = TRUE;
            break;

        case F_SPECIAL:
        case F_CHAR:
            charout = TRUE;
            break;

        case F_TEXT:
            textout = TRUE;
            break;

        default:
            break;
        }

        if(charout)
            ptr += sprintf(ptr, "|i %c ", (int) dptri->option);

        if(numout)
            ptr += sprintf(ptr, "|i %d ", (int) dptri->option);

        if(textout || arrayout)
        {
            /* expand string in, putting | before " */
            uchar *fptr;

            if(textout)
                fptr = !textptr ? UNULLSTR : textptr;
            else
                /* optionlist refers to an array of pointers to char *s in F_ARRAY case*/
                fptr = ** ((char ***) dptri->optionlist + dptri->option);

            cvt_string_arg_for_macro_file(ptr, elemof32(array) - (ptr - array), fptr);
        }

        if(!output_string_to_macro_file(array))
            break;
    }
}

/*
switch macro recording on and off
when switching on, get filename from dialog box and stick tick on menu item
when switching off remove tick from menu item
*/

static int
start_macro_recorder_core(void)
{
    char *name = d_macro_file[0].textfield;
    char buffer[BUF_MAX_PATHSTRING];

    if(str_isblank(name))
    {
        reperr_null(ERR_BAD_NAME);
        if(!dialog_box_can_retry())
            return(FALSE);
        been_error = FALSE;
        (void) init_dialog_box(D_MACRO_FILE);
        return(2); /*continue*/
    }

    if(file_is_rooted(currentfilename))
    {   /* Record to the directory of the current document */
        file_get_cwd(buffer, elemof32(buffer), currentfilename);
        xstrkat(buffer, elemof32(buffer), name);
    }
    else
    {   /* Record to subdirectory of <Choices$Write>.PipeDream */
        (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), name, MACROS_SUBDIR_STR);
    }

    reportf("Recording command file to %s", buffer);

    if(!mystr_set(&macro_file_name, buffer))
        return(FALSE);

    macro_recorder_file = pd_file_open(macro_file_name, file_open_write);

    if(NULL == macro_recorder_file)
    {
        reperr(ERR_CANNOTOPEN, macro_file_name);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    file_set_type(macro_recorder_file, FILETYPE_PDMACRO);

    macro_recorder_on = TRUE;

    return(TRUE);
}

static void
start_macro_recorder(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_MACRO_FILE))
    {
        int core_res = start_macro_recorder_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        /* if(!dialog_box_can_persist()) - no, job done */
        break;
    }

    dialog_box_end();
}

static void
stop_macro_recorder(void)
{
    if(pd_file_close(&macro_recorder_file))
    {
        str_clr(&d_execfile[0].textfield);
        reperr(ERR_CANNOTCLOSE, macro_file_name);
    }
    else
    {
        consume_bool(mystr_set(&d_execfile[0].textfield, d_macro_file[0].textfield));
    }

    str_clr(&macro_file_name);

    macro_recorder_on = FALSE;
}

extern void
RecordMacroFile_fn(void)
{
    macro_needs_lf = FALSE;

    if(macro_recorder_on)
        stop_macro_recorder();
    else
        start_macro_recorder();

    record_state_changed();
}

/* end of commlin.c */
