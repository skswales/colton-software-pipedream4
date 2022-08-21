/* commlin.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

extern BOOL
application_process_key(
    S32 c)
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
        if((mptr->flags & NO_REPEAT) == 0)
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
            (void) xsnprintf(array, elemof32(array), ": command %d left escape unprocessed", -c);
            ack_esc();
            reperr(create_error(ERR_ESCAPE), array);
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

            /* rjm says caret left/right and delete chars should be faster */
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
*    0        do nothing as there's been an error
*    -ve        function number to execute
*
******************************************************************************/

extern S32
inpchr_riscos(
    S32 keyfromriscos)
{
    S32 c;

    trace_1(TRACE_APP_PD4, "inpchr_riscos(&%3.3X)", keyfromriscos);

    do  {
        do  {
            /* if in expansion get next char */
            if(keyidx)
                {
                if((c = *keyidx++) == '\0')
                    keyidx = NULL;        /* this expansion has ended */
                }
            else
                /* no expansion char so try 'keyboard' */
                {
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
        while(!alt_array[0]  &&  !cbuff_offset  &&  c  &&  !keyidx  &&  schkvl(c));

        if(keyidx  &&  !*keyidx)
            {
            /* expansion now completed after reading last char */
            keyidx = NULL;

            if(cbuff_offset  &&  (c != CR))
                {
                /* half a command entered on function key or something */
                cbuff_offset = 0;
                reperr_null(create_error(ERR_BAD_PARM));
                trace_0(TRACE_APP_PD4, "error in key expansion: returns 0");
                return(0);            /* an uninteresting result */
                }
            }

        /* see if we're doing an Alt-sequence */
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

/* used to be      c = command_edit(c);    */
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

    if((filetype_option = find_filetype_option(filename)) < 0)
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

    if(!input)
        {
        reperr(create_error(ERR_CANNOTOPEN), filename);
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
                keyidx     = cbuff;

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

                        case '\0':
                            /* unmatched / */
                            reperr_null(create_error(ERR_BAD_PARM));
                            goto BREAKOUT;

                        default:
                            *keyidx++ = *ptr++;
                            continue;
                        }
                    break;
                    }

                *keyidx = '\0';
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
                    while(*ptr != '\0' && *ptr != CR && *ptr != LF)
                        ptr++;
                    }

                keyidx = NULL;
                }
            else
                /* it's not a CMDLDI */
                {
                cbuff[0] = *ptr;
                cbuff[1] = '\0';
                keyidx = cbuff;

                /* reset keyidx if it's a command, eg CR, TAB */
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

            do { ch = *keyidx++; } while((ch != CR)  &&  (ch != '\0'));

            /* deliberate fall thru... */

        case CR:
            cbuff[cbuff_offset] = '\0';
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
        if(ch == '\0')
            return(NO_VAL);

        return((coord) (0 - (coord) ch));
        }

    consume(int, sscanf((char *) buff_sofar, "%d%n", &res, &count));

    if(!count)
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

#define MSP  { NULL, NULL, 0, 0, 0, NULL }

/* command, flags, title, default key, function address, number */

#define MIT(command, flags, title, key, cmdfunc, funcnum)  { & title, command, key, flags, funcnum, cmdfunc }

/* SKS 23oct96 introduces shortforms for compact table descriptions */
#define SHT 0
#define LNG LONG_ONLY
#define DRQ DOC_REQ
#define GEX GREY_EXPEDIT
#define DLG HAS_DIALOG
#define MBF DO_MERGEBUF
#define NMF NO_MACROFILE
#define NRP NO_REPEAT
#define NEU NOMENU
#define TCK TICKABLE

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
files_menu[] =
{
    MIT("Fl",   SHT |       GEX | DLG,                         Load_STR,                       0,              LoadFile_fn,               N_LoadFile),
    MIT("Fs",   SHT | DRQ | GEX | DLG | MBF | NMF | NRP,       Save_STR,                       SAVE,           SaveFile_fn,               N_SaveFile),
    MIT("Fi",   SHT | DRQ | GEX | DLG,                         Save_template_STR,              0,              SaveTemplateFile_fn,       N_SaveTemplateFile),
    MIT("Fc",   LNG | DRQ | GEX | DLG,                         Name_STR,                       0,              NameFile_fn,               N_NameFile),
    MIT("Fa",   SHT | DRQ       | DLG,                         About_STR,                      0,              About_fn,                  N_About),
    MSP,
    MIT("Bnew", LNG       | GEX,                               New_window_STR,                 0,              NewWindow_fn,              N_NewWindow),
    MIT("Fw",   LNG | DRQ | GEX       | MBF,                   Next_window_STR,                0,              NextWindow_fn,             N_NextWindow),
    MIT("Fq",   LNG | DRQ | GEX       | MBF,                   Close_window_STR,               0,              CloseWindow_fn,            N_CloseWindow),
    MSP,
    MIT("O",    SHT | DRQ       | DLG,                         Options_STR,                    0,              Options_fn,                N_Options),
    MSP,
    MIT("A",    LNG | DRQ | GEX       | MBF,                   Recalculate_STR,                0,              Recalculate_fn,            N_Recalculate),
    MIT("J",    SHT |       GEX             | NMF | NRP,       RepeatCommand_STR,              REPEAT,         RepeatCommand_fn,          N_RepeatCommand),
    MIT("Fh",   SHT,                                           Help_STR,                       HELP,           Help_fn,                   N_Help),
    MSP,
/* need address of tickable entry */
#define record_option files_menu[16]
    MIT("Fy",   LNG | DRQ | GEX             | NMF | TCK,       Record_macro_file_STR,          0,              RecordMacroFile_fn,        N_RecordMacroFile),
    MIT("Fz",   LNG | DRQ | GEX | DLG | MBF | NMF,             Do_macro_file_STR,              0,              DoMacroFile_fn,            N_DoMacroFile)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
edit_menu[] =
{
    MIT("R",    SHT | DRQ | GEX       | MBF,                   Format_paragraph_STR,           0,              FormatParagraph_fn,        N_FormatParagraph),
    MIT("Efb",  SHT | DRQ | GEX       | MBF,                   FormatBlock_STR,                0,              FormatBlock_fn,            N_FormatBlock),
    MSP,
    MIT("G",    LNG | DRQ | GEX,                               Delete_character_STR,           SDELETE_KEY,    DeleteCharacterRight_fn,   N_DeleteCharacterRight),
    MIT("U",    LNG | DRQ | GEX,                               Insert_space_STR,               INSERT_KEY,     InsertSpace_fn,            N_InsertSpace),
    MIT("T",    LNG | DRQ | GEX,                               Delete_word_STR,                DELETEWORD,     DeleteWord_fn,             N_DeleteWord),
    MIT("D",    LNG | DRQ | GEX,                               Delete_to_end_of_slot_STR,      DELEOL,         DeleteToEndOfSlot_fn,      N_DeleteToEndOfSlot),
    MIT("I",    SHT | DRQ | GEX,                               Paste_STR,                      PASTE,          Paste_fn,                  N_Paste),
    MIT("Y",    LNG | DRQ | GEX /*no mergebuf*/,               Delete_row_STR,                 DELETEROW,      DeleteRow_fn,              N_DeleteRow),
    MIT("N",    LNG | DRQ | GEX       | MBF,                   Insert_row_STR,                 INSERTROW,      InsertRow_fn,              N_InsertRow),
    MSP,
    MIT("ss",   LNG | DRQ | GEX,                               Swap_case_STR,                  0,              SwapCase_fn,               N_SwapCase),
    MIT("X",    SHT | DRQ | GEX             | NMF,             Edit_formula_STR,               EXPRESSION,     EditFormula_fn,            N_EditFormula),
    MIT("Efw",  LNG | DRQ                   | NMF,             Edit_formula_in_window_STR,     0,              EditFormulaInWindow_fn,    N_EditFormulaInWindow),
    MSP,
    MIT("Esl",  LNG | DRQ | GEX       | MBF,                   Split_line_STR,                 0,              SplitLine_fn,              N_SplitLine),
    MIT("Ejl",  LNG | DRQ | GEX       | MBF,                   Join_lines_STR,                 0,              JoinLines_fn,              N_JoinLines),
    MIT("Eirc", LNG | DRQ | GEX       | MBF,                   Insert_row_in_column_STR,       0,              InsertRowInColumn_fn,      N_InsertRowInColumn),
    MIT("Edrc", LNG | DRQ | GEX /*no mergebuf*/,               Delete_row_in_column_STR,       0,              DeleteRowInColumn_fn,      N_DeleteRowInColumn),
    MIT("Eic",  SHT | DRQ | GEX       | MBF,                   Insert_column_STR,              0,              InsertColumn_fn,           N_InsertColumn),
    MIT("Edc",  SHT | DRQ | GEX       | MBF,                   Delete_column_STR,              0,              DeleteColumn_fn,           N_DeleteColumn),
    MIT("Eac",  SHT | DRQ | GEX       | MBF,                   Add_column_STR,                 0,              AddColumn_fn,              N_AddColumn),
    MIT("Eip",  SHT | DRQ | GEX | DLG,                         Insert_page_STR,                0,              InsertPageBreak_fn,        N_InsertPageBreak)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
layout_menu[] =
{
    MIT("W",    LNG | DRQ | GEX | DLG,                         Set_column_width_STR,           0,              ColumnWidth_fn,            N_ColumnWidth),
    MIT("H",    LNG | DRQ | GEX | DLG,                         Set_right_margin_STR,           0,              RightMargin_fn,            N_RightMargin),
    MIT("Lml",  LNG | DRQ | GEX,                               Move_margin_left_STR,           0,              MoveMarginLeft_fn,         N_MoveMarginLeft),
    MIT("Lmr",  LNG | DRQ | GEX,                               Move_margin_right_STR,          0,              MoveMarginRight_fn,        N_MoveMarginRight),
    MIT("Law",  SHT | DRQ | GEX       | MBF,                   Auto_width_STR,                 0,              AutoWidth_fn,              N_AutoWidth),
    MIT("Lco",  SHT | DRQ | GEX,                               Link_columns_STR,               0,              LinkColumns_fn,            N_LinkColumns),

    MIT("Lfr",  SHT | DRQ | GEX       | MBF,                   Fix_row_STR,                    0,              FixRows_fn,                N_FixRows),
    MIT("Lfc",  SHT | DRQ | GEX       | MBF,                   Fix_column_STR,                 0,              FixColumns_fn,             N_FixColumns),
    MSP,
    MIT("Lal",  SHT | DRQ | GEX       | MBF,                   Left_align_STR,                 0,              LeftAlign_fn,              N_LeftAlign),
    MIT("Lac",  SHT | DRQ | GEX       | MBF,                   Centre_align_STR,               0,              CentreAlign_fn,            N_CentreAlign),
    MIT("Lar",  SHT | DRQ | GEX       | MBF,                   Right_align_STR,                0,              RightAlign_fn,             N_RightAlign),
    MIT("Llcr", LNG | DRQ | GEX       | MBF,                   LCR_align_STR,                  0,              LCRAlign_fn,               N_LCRAlign),
    MIT("Laf",  SHT | DRQ | GEX       | MBF,                   Free_align_STR,                 0,              FreeAlign_fn,              N_FreeAlign),
    MSP,
    MIT("Ldp",  SHT | DRQ | GEX | DLG,                         Decimal_places_STR,             0,              DecimalPlaces_fn,          N_DecimalPlaces),
    MIT("Lsb",  LNG | DRQ | GEX       | MBF,                   Sign_brackets_STR,              0,              SignBrackets_fn,           N_SignBrackets),
    MIT("Lsm",  LNG | DRQ | GEX       | MBF,                   Sign_minus_STR,                 0,              SignMinus_fn,              N_SignMinus),
    MIT("Lcl",  SHT | DRQ | GEX       | MBF,                   Leading_characters_STR,         0,              LeadingCharacters_fn,      N_LeadingCharacters),
    MIT("Lct",  SHT | DRQ | GEX       | MBF,                   Trailing_characters_STR,        0,              TrailingCharacters_fn,     N_TrailingCharacters),
    MIT("Ldf",  LNG | DRQ | GEX       | MBF,                   Default_format_STR,             0,              DefaultFormat_fn,          N_DefaultFormat)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
print_menu[] =
{
    MIT("Po",   SHT | DRQ | GEX | DLG,                         Print_STR,                      PRINT_KEY,      Print_fn,                  N_Print),
    MIT("Py",   SHT | DRQ | GEX | DLG,                         Page_layout_STR,                0,              PageLayout_fn,             N_PageLayout),
    MIT("Pd",   SHT | DRQ | GEX | DLG,                         Printer_configuration_STR,      0,              PrinterConfig_fn,          N_PrinterConfig),
    MIT("Pm",   LNG | DRQ | GEX | DLG,                         Microspace_pitch_STR,           0,              MicrospacePitch_fn,        N_MicrospacePitch),
    MIT("Pgd",  LNG | DRQ | GEX | DLG,                         Edit_printer_driver_STR,        0,              EditPrinterDriver_fn,      N_EditPrinterDriver),
    MSP,
    MIT("Pfg",  SHT | DRQ | GEX | DLG,                         Printer_font_STR,               0,              PrinterFont_fn,            N_PRINTERFONT),
    MIT("Pfl",  SHT | DRQ | GEX | DLG,                         Insert_font_STR,                0,              InsertFont_fn,             N_INSERTFONT),
    MSP,
    MIT("Pb",   SHT | DRQ | GEX,                               Bold_STR,                       0,              Bold_fn,                   N_Bold),
    MIT("Pi",   SHT | DRQ | GEX,                               Italic_STR,                     0,              Italic_fn,                 N_Italic),
    MIT("Pu",   SHT | DRQ | GEX,                               Underline_STR,                  0,              Underline_fn,              N_Underline),
    MIT("Pl",   SHT | DRQ | GEX,                               Subscript_STR,                  0,              Subscript_fn,              N_Subscript),
    MIT("Pr",   SHT | DRQ | GEX,                               Superscript_STR,                0,              Superscript_fn,            N_Superscript),
    MIT("Px",   LNG | DRQ | GEX,                               Extended_sequence_STR,          0,              ExtendedSequence_fn,       N_ExtendedSequence),
    MIT("Pa",   LNG | DRQ | GEX,                               Alternate_font_STR,             0,              AlternateFont_fn,          N_AlternateFont),
    MIT("Pe",   LNG | DRQ | GEX,                               User_defined_STR,               0,              UserDefinedHigh_fn,        N_UserDefinedHigh),
    MSP,
    MIT("Phb",  LNG | DRQ | GEX | DLG | MBF,                   Highlight_block_STR,            0,              HighlightBlock_fn,         N_HighlightBlock),
    MIT("Phr",  LNG | DRQ | GEX | DLG | MBF,                   Remove_highlights_STR,          0,              RemoveHighlights_fn,       N_RemoveHighlights)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
blocks_menu[] =
{
    MIT("Z",    LNG | DRQ | GEX       | MBF,                   Mark_block_STR,                 0,              MarkSlot_fn,               N_MarkSlot),
    MIT("Q",    LNG | DRQ             | MBF,                   Clear_markers_STR,              0,              ClearMarkedBlock_fn,       N_ClearMarkedBlock),
    MSP,
    MIT("Bc",   SHT | DRQ | GEX       | MBF,                   Copy_block_STR,                 0,              CopyBlock_fn,              N_CopyBlock),
    MIT("Bf",   LNG | DRQ | GEX       | MBF,                   Copy_block_to_paste_list_STR,   0,              CopyBlockToPasteList_fn,   N_CopyBlockToPasteList),
    MIT("Bm",   SHT | DRQ | GEX       | MBF,                   Move_block_STR,                 0,              MoveBlock_fn,              N_MoveBlock),
    MIT("Bd",   SHT | DRQ | GEX       | MBF,                   Delete_block_STR,               0,              DeleteBlock_fn,            N_DeleteBlock),
    MIT("Blc",  SHT | DRQ | GEX       | MBF,                   Clear_block_STR,                0,              ClearBlock_fn,             N_ClearBlock),
    MSP,
    MIT("Brd",  SHT | DRQ | GEX       | MBF,                   Replicate_down_STR,             0,              ReplicateDown_fn,          N_ReplicateDown),
    MIT("Brr",  SHT | DRQ | GEX       | MBF,                   Replicate_right_STR,            0,              ReplicateRight_fn,         N_ReplicateRight),
    MIT("Bre",  LNG | DRQ | GEX | DLG | MBF,                   Replicate_STR,                  0,              Replicate_fn,              N_Replicate),
    MIT("Bso",  SHT | DRQ | GEX | DLG | MBF,                   Sort_STR,                       0,              SortBlock_fn,              N_SortBlock),
/* following needs changing to Transpose_block_STR when everyone wants to recompile everything again */
    MIT("Bt",   LNG | DRQ | GEX       | MBF,                   SpreadArray_STR,                0,              TransposeBlock_fn,         N_TransposeBlock),
    MIT("Bse",  SHT | DRQ       | DLG,                         Search_STR,                     SEARCH,         Search_fn,                 N_Search),
    MIT("Bnm",  SHT | DRQ,                                     Next_match_STR,                 NEXTMATCH,      NextMatch_fn,              N_NextMatch),
    MIT("Bpm",  SHT | DRQ,                                     Previous_match_STR,             0,              PrevMatch_fn,              N_PrevMatch),
    MSP,
    MIT("Bps",  LNG | DRQ | GEX       | MBF,                   Set_protection_STR,             0,              SetProtectedBlock_fn,      N_SetProtectedBlock),
    MIT("Bpc",  LNG | DRQ | GEX       | MBF,                   Clear_protection_STR,           0,              ClearProtectedBlock_fn,    N_ClearProtectedBlock),
    MSP,
    MIT("Bln",  SHT | DRQ | GEX       | MBF,                   ToNumber_STR,                   0,              ToNumber_fn,               N_ToNumber),
    MIT("Blt",  SHT | DRQ | GEX       | MBF,                   ToText_STR,                     0,              ToText_fn,                 N_ToText),
    MIT("Bnt",  SHT | DRQ | GEX       | MBF,                   Number_X_Text_STR,              0,              ExchangeNumbersText_fn,    N_ExchangeNumbersText),
    MIT("Bss",  SHT | DRQ | GEX       | MBF,                   Snapshot_STR,                   0,              Snapshot_fn,               N_Snapshot),
    MIT("Bwc",  SHT | DRQ | GEX       | MBF,                   Word_count_STR,                 0,              WordCount_fn,              N_WordCount)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
cursor_menu[] =
{
    MIT("Cfc",  SHT | DRQ             | MBF | NRP,             First_column_STR,               CSTAB_KEY,      FirstColumn_fn,            N_FirstColumn),
    MIT("Clc",  SHT | DRQ             | MBF | NRP,             Last_column_STR,                CTAB_KEY,       LastColumn_fn,             N_LastColumn),
    MIT("Cnw",  SHT | DRQ | GEX             | NRP,             Next_word_STR,                  SRIGHTCURSOR,   NextWord_fn,               N_NextWord),
    MIT("Cpw",  SHT | DRQ | GEX             | NRP,             Previous_word_STR,              SLEFTCURSOR,    PrevWord_fn,               N_PrevWord),
    MIT("Cwi",  SHT | DRQ             | MBF | NRP,             Centre_window_STR,              0,              CentreWindow_fn,           N_CentreWindow),
    MSP,
    MIT("Csp",  LNG | DRQ,                                     Save_position_STR,              0,              SavePosition_fn,           N_SavePosition),
    MIT("Crp",  LNG | DRQ             | MBF,                   Restore_position_STR,           0,              RestorePosition_fn,        N_RestorePosition),
    MIT("Cwc",  LNG | DRQ             | MBF,                   Swap_position_and_caret_STR,    0,              SwapPosition_fn,           N_SwapPosition),
    MIT("Cgs",  LNG | DRQ       | DLG,                         Go_to_slot_STR,                 0,              GotoSlot_fn,               N_GotoSlot),
    MSP,
    MIT("Cdk",  LNG             | DLG,                         Define_key_STR,                 0,              DefineKey_fn,              N_DefineKey),
    MIT("Cdf",  LNG             | DLG,                         Define_function_key_STR,        0,              DefineFunctionKey_fn,      N_DefineFunctionKey),
    MIT("Cdc",  LNG             | DLG,                         Define_command_STR,             0,              DefineCommand_fn,          N_DefineCommand)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
spell_menu[] =  /*no mergebufs*/
{
    MIT("Sc",   SHT | DRQ | GEX | DLG,                         Check_document_STR,             0,              CheckDocument_fn,          N_CheckDocument),
    MIT("Sd",   LNG             | DLG,                         Delete_word_from_user_dict_STR, 0,              DeleteWordFromDict_fn,     N_DeleteWordFromDict),
    MIT("Si",   LNG             | DLG,                         Insert_word_in_user_dict_STR,   0,              InsertWordInDict_fn,       N_InsertWordInDict),
    MSP,
    MIT("Sb",   SHT             | DLG,                         Browse_STR,                     0,              BrowseDictionary_fn,       N_BrowseDictionary),
    MIT("Su",   LNG             | DLG,                         Dump_dictionary_STR,            0,              DumpDictionary_fn,         N_DumpDictionary),
    MIT("Sm",   LNG             | DLG,                         Merge_file_with_user_dict_STR,  0,              MergeFileWithDict_fn,      N_MergeFileWithDict),
    MIT("Sg",   SHT             | DLG,                         Anagrams_STR,                   0,              Anagrams_fn,               N_Anagrams),
    MIT("Sh",   SHT             | DLG,                         Subgrams_STR,                   0,              Subgrams_fn,               N_Subgrams),
    MSP,
    MIT("Sn",   LNG             | DLG,                         Create_user_dictionary_STR,     0,              CreateUserDict_fn,         N_CreateUserDict),
    MIT("So",   LNG             | DLG,                         Open_user_dictionary_STR,       0,              OpenUserDict_fn,           N_OpenUserDict),
    MIT("Sz",   LNG             | DLG,                         Close_user_dictionary_STR,      0,              CloseUserDict_fn,          N_CloseUserDict),
    MIT("Sf",   LNG,                                           Flush_user_dictionary_STR,      0,              FlushUserDict_fn,          N_FlushUserDict),
    MIT("Sp",   LNG             | DLG,                         Pack_user_dictionary_STR,       0,              PackUserDict_fn,           N_PackUserDict),
    MIT("Sq",   LNG             | DLG,                         Dictionary_options_STR,         0,              DictionaryOptions_fn,      N_DictionaryOptions),
    MIT("Sl",   LNG             | DLG,                         Lock_dictionary_STR,            0,              LockDictionary_fn,         N_LockDictionary),
    MIT("Sk",   LNG             | DLG,                         Unlock_dictionary_STR,          0,              UnlockDictionary_fn,       N_UnlockDictionary)
};

/*              LNG . DRQ . GEX . DLG . MBF . NMF . NRP */

static MENU
charts_menu[] =
{
    MIT("Chn",  SHT | DRQ | GEX       | MBF,                   New_chart_STR,                  0,              ChartNew_fn,               N_ChartNew),
    MIT("Cho",  SHT | DRQ | GEX | DLG,                         Chart_options_STR,              0,              ChartOptions_fn,           N_ChartOptions),
    MIT("Cha",  SHT | DRQ | GEX | DLG | MBF,                   Add_to_chart_STR,               0,              ChartAdd_fn,               N_ChartAdd),
    MIT("Che",  SHT | DRQ | GEX | DLG,                         Edit_chart_STR,                 0,              ChartEdit_fn,              N_ChartEdit),
    MIT("Chd",  SHT | DRQ | GEX | DLG,                         Delete_chart_STR,               0,              ChartDelete_fn,            N_ChartDelete),
    MIT("Chs",  SHT | DRQ | GEX | DLG,                         Select_chart_STR,               0,              ChartSelect_fn,            N_ChartSelect)
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
    MIT("Ccu",  SHT | DRQ | GEX       | MBF       | NRP | NEU, Cursor_up_STR,                  UPCURSOR,       CursorUp_fn,               N_CursorUp),
    MIT("Ccd",  SHT | DRQ | GEX       | MBF       | NRP | NEU, Cursor_down_STR,                DOWNCURSOR,     CursorDown_fn,             N_CursorDown),
    MIT("Ccl",  SHT | DRQ | GEX                   | NRP | NEU, Cursor_left_STR,                LEFTCURSOR,     CursorLeft_fn,             N_CursorLeft),
    MIT("Ccr",  SHT | DRQ | GEX                   | NRP | NEU, Cursor_right_STR,               RIGHTCURSOR,    CursorRight_fn,            N_CursorRight),
    MIT("Cbs",  SHT | DRQ | GEX                   | NRP | NEU, Start_of_slot_STR,              CLEFTCURSOR,    StartOfSlot_fn,            N_StartOfSlot),
    MIT("Ces",  SHT | DRQ | GEX                   | NRP | NEU, End_of_slot_STR,                CRIGHTCURSOR,   EndOfSlot_fn,              N_EndOfSlot),
    MIT("Csu",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_up_STR,                  CSUPCURSOR,     ScrollUp_fn,               N_ScrollUp),
    MIT("Csd",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_down_STR,                CSDOWNCURSOR,   ScrollDown_fn,             N_ScrollDown),
    MIT("Csl",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_left_STR,                CSLEFTCURSOR,   ScrollLeft_fn,             N_ScrollLeft),
    MIT("Csr",  SHT | DRQ             | MBF       | NRP | NEU, Scroll_right_STR,               CSRIGHTCURSOR,  ScrollRight_fn,            N_ScrollRight),
    MIT("Cpu",  SHT | DRQ             | MBF       | NRP | NEU, Page_up_STR,                    SUPCURSOR,      PageUp_fn,                 N_PageUp),
    MIT("Cpd",  SHT | DRQ             | MBF       | NRP | NEU, Page_down_STR,                  SDOWNCURSOR,    PageDown_fn,               N_PageDown),
    MIT("Cpl",  SHT | DRQ             | MBF       | NRP | NEU, Page_left_STR,                  0,              PageLeft_fn,               N_PageLeft),
    MIT("Cpr",  SHT | DRQ             | MBF       | NRP | NEU, Page_right_STR,                 0,              PageRight_fn,              N_PageRight),
    MIT("Ctc",  SHT | DRQ             | MBF       | NRP | NEU, Top_of_column_STR,              CUPCURSOR,      TopOfColumn_fn,            N_TopOfColumn),
    MIT("Cbc",  SHT | DRQ             | MBF       | NRP | NEU, Bottom_of_column_STR,           CDOWNCURSOR,    BottomOfColumn_fn,         N_BottomOfColumn),
    MIT("Cpc",  SHT | DRQ             | MBF       | NRP | NEU, Previous_column_STR,            STAB_KEY,       PrevColumn_fn,             N_PrevColumn),
    MIT("Cnc",  SHT | DRQ             | MBF       | NRP | NEU, Next_column_STR,                TAB_KEY,        NextColumn_fn,             N_NextColumn),
    MIT("Cen",  SHT | DRQ                         | NRP | NEU, Enter_STR,                      CR,             Return_fn,                 N_Return),
    MIT("Cx",   SHT | DRQ                               | NEU, Escape_STR,                     ESCAPE,         Escape_fn,                 N_Escape),
    MIT("Crb",  SHT | DRQ | GEX                   | NRP | NEU, Rubout_STR,                     DELETE_KEY,     DeleteCharacterLeft_fn,    N_DeleteCharacterLeft),
    MIT("Ccp",  SHT | DRQ | GEX                         | NEU, Pause_STR,                      0,              Pause_fn,                  N_Pause),
    MIT("Brp",  SHT | DRQ                               | NEU, Replace_STR,                    0,              Replace_fn,                N_OldSearch),
    MIT("Ent",  SHT | DRQ | GEX       | MBF             | NEU, Number_X_Text_STR,              0,              ExchangeNumbersText_fn,    N_OldExchangeNumbersAndText),
    MIT("Fgz",  SHT | DRQ | GEX       | MBF             | NEU, Mark_sheet_STR,                 0,              MarkSheet_fn,              N_MarkSheet),
    MIT("Esc",  SHT | DRQ | GEX       | MBF             | NEU, New_slot_contents_STR,          0,              Newslotcontents_fn,        N_Newslotcontents),
    MIT("Fgf",  SHT | DRQ                               | NEU, FullScreen_STR,                 0,              FullScreen_fn,             N_FullScreen),
    MIT("Fgs",  SHT | DRQ | GEX       | MBF             | NEU, Save_STR,                       0,              SaveFileAsIs_fn,           N_SaveFileAsIs),
    MIT("Fgl",  SHT       | GEX | DLG                   | NEU, Load_Template_STR,              0,              LoadTemplate_fn,           N_LoadTemplate),
    MIT("Fx",   SHT       | GEX                         | NEU, Exit_STR,                       0,              Quit_fn,                   N_Quit)
};

/* progvars control fns moved to iconbar menu */

/*              LNG . DRQ . gex . DLG . MBF . NMF . NRP */

static MENU
choices_menu[] =
{
/* need address of tickable entry */
#define menu_option     choices_menu[0]
    MIT("M",    ALWAYS_SHORT                            | NEU | TCK, Short_menus_STR,                0,              MenuSize_fn,               N_MenuSize),
    MIT("Fgm",  ALWAYS_SHORT                            | NEU,       ChangeMenus_STR,                0,              ChangeMenus_fn,            N_ChangeMenus),
    MSP,
    MIT("Se",   SHT             | DLG                   | NEU,       Display_user_dictionaries_STR,  0,              DisplayOpenDicts_fn,       N_DisplayOpenDicts),
/* need address of tickable entry */
#define check_option    choices_menu[4]
    MIT("Sa",   SHT                                     | NEU | TCK, Auto_check_STR,                 0,              AutoSpellCheck_fn,         N_AutoSpellCheck),
    MSP,
/* need address of tickable entry */
#define recalc_option   choices_menu[6]
    MIT("Fo",   SHT                                     | NEU | TCK, Auto_recalculation_STR,         0,              AutoRecalculation_fn,      N_AutoRecalculation),
/* need address of tickable entry */
#define chart_recalc_option choices_menu[7]
    MIT("Chc",  SHT                                     | NEU | TCK, Auto_chart_recalculation_STR,   0,              AutoChartRecalculation_fn, N_AutoChartRecalculation),
    MIT("Fr",   SHT             | DLG                   | NEU,       Colours_STR,                    0,              Colours_fn,                N_Colours),
/* need address of tickable entry */
#define insert_option   choices_menu[9]
    MIT("V",    SHT                                     | NEU | TCK, Insert_STR,                     0,              InsertOvertype_fn,         N_InsertOvertype),
    MIT("Bpd",  SHT             | DLG                   | NEU,       Size_of_paste_list_STR,         0,              PasteListDepth_fn,         N_PasteListDepth),
    MSP,
    MIT("Fgi",  SHT | DRQ                               | NEU,       SaveChoices_STR,                0,              SaveChoices_fn,            N_SaveChoices),
};

/******************************************************************************
*
* define the menu heading structure
*
******************************************************************************/

#define menu__head(name, tail) \
    { name, tail, TRUE, sizeof(tail)/sizeof(MENU), 0, 0, NULL }

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
{   /* menu head title,    submenu */
    menu__head(&Files_STR,  files_menu),
    menu__head(&Edit_STR,   edit_menu),
#define EDIT_MENU 1
    menu__head(&Layout_STR, layout_menu),
    menu__head(&Print_STR,  print_menu),
    menu__head(&Blocks_STR, blocks_menu),
    menu__head(&Cursor_STR, cursor_menu),

    menu__head(&Spell_STR,  spell_menu),
#define SPELL_MENU 6
    menu__head(&Chart_STR, charts_menu),

#if TRACE_ALLOWED || defined(DEBUG)
    /* allow user to see ALL commands */
    menu__head(&Random_STR, random_menu),
#endif

    menu__head(NULL,        random_menu)
};
/* note that the definition of HEAD_SIZE excludes the invisible 'random' menu */
#define HEAD_SIZE ((S32) (elemof32(headline) - 1))

S32 head_size = HEAD_SIZE;

/******************************************************************************
*
* get menu change option from ini file
*
******************************************************************************/

extern void
get_menu_change(
    char *from)
{
    MENU *mptr;
    char *to;

    for(to=alt_array; *from; from++, to++)
        *to = toupper(*from);
    *to = '\0';

    mptr = part_command(FALSE);
    if(mptr == M_IS_SUBSET || mptr == M_IS_ERROR)
        return;

    mptr->flags |= CHANGED;
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
    MENU_HEAD * mhptr)
{
    const MENU *firstmptr, *mptr;

    if(0 != mhptr->items)
        {
        firstmptr = mhptr->tail;
        mptr = firstmptr + mhptr->items;

        while(--mptr >= firstmptr)
            if(mptr->flags & CHANGED)
                {
                if( !away_string("%OP%MC", output) ||
                    !away_string(mptr->command, output) ||
                    !away_eol(output) )
                        break;
                }
        }
}

extern void
save_menu_changes(
    FILE_HANDLE output)
{
    S32 head_i;

    for(head_i = 0; head_i < HEAD_SIZE; ++head_i)
        if(headline[head_i].installed)
            save_this_menu_changes(output, &headline[head_i]);

    save_this_menu_changes(output, &iconbar_headline[0]);
}

/******************************************************************************
*
*  size the menus (titlelen only)
*
******************************************************************************/

static void
resize_submenu(
    MENU_HEAD * mhptr,
    BOOL short_m)
{
    MENU *mptr, *lastmptr;
    S32 titlelen, commandlen;
    S32 maxtitlelen, maxcommandlen;
    const char *ptr;
    S32 offset;

    mptr          = mhptr->tail;
    lastmptr      = mptr + mhptr->items;
    maxtitlelen   = 0;
    maxcommandlen = 0;
    offset = 1;        /* menu offsets start at 1 */

    do  {
        ptr = (char *) mptr->title;

        /* ignore lines between menus and missing items */
        if(ptr  &&  (!short_m  ||  on_short_menus(mptr->flags)))
            {
            ptr = * (char **) ptr;

            titlelen   = strlen(ptr);
            commandlen = strlen(mptr->command);

            maxtitlelen   = MAX(maxtitlelen, titlelen);
            maxcommandlen = MAX(maxcommandlen, commandlen);

            ++offset;
            }
        }
    while(++mptr < lastmptr);

    mhptr->titlelen   = maxtitlelen;
    mhptr->commandlen = maxcommandlen + 1;
    trace_2(TRACE_APP_PD4, "set titlelen for menu %s to %d", mhptr->name ? *(mhptr->name) : "<invisible>", mhptr->titlelen);
}

static void
resize_menus(
    BOOL short_m)
{
    MENU_HEAD *firstmhptr, *mhptr;

    resize_submenu(&iconbar_headline[0], short_m);

    firstmhptr = headline;
    mhptr      = firstmhptr + HEAD_SIZE;

    /* NB --mhptr kosher */
    do
        if(mhptr->installed)
            resize_submenu(mhptr, short_m);
    while(--mhptr >= firstmhptr);
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

    resize_menus(short_menus());
}

/* *********************************************************************** */

/* read the beginning of the command buffer, find and return function number.
 * If there is a function return its number (offset in function table) otherwise
 * generate an error and return a flag indicating error

 * If the lookup is successful, cbuff_offset is set to point at the first
 * parameter position.
*/

static MENU *
lukucm(
    BOOL allow_redef)
{
    S32 i = 0;
    MENU *mptr;

    do
        alt_array[i] = (uchar) toupper((int) cbuff[i+1]);
    while(++i < MAX_COMM_SIZE);

    mptr = part_command(allow_redef);

    return((mptr == M_IS_ERROR || mptr == M_IS_SUBSET) ? ((MENU *) NO_COMM) : mptr);
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
    MENU_HEAD * mhptr)
{
    MENU *firstmptr, *mptr;

    if(0 != mhptr->items)
        {
        firstmptr = mhptr->tail;
        mptr      = firstmptr + mhptr->items;

        while(--mptr >= firstmptr)
            if(mptr->key == c)
                {
                /* expand original definition into buffer
                 * adding \ and \n
                 */
                keyidx = expanded_key_buff;
                strcpy(keyidx + 1, mptr->command);
                strcat(keyidx + 1, CR_STR);
                trace_1(TRACE_APP_PD4, "schkvl found: %s", trace_string(keyidx));
                return(TRUE);
                }
        }

    return(FALSE);
}

extern BOOL
schkvl(
    S32 c)
{
    LIST *lptr;
    MENU_HEAD *firstmhptr, *mhptr;

    lptr = search_list(&first_key, c);

    if(lptr)
        keyidx = lptr->value;
    else if((c != 0)  &&  ((c < (S32) SPACE)  ||  (c >= (S32) 127)))
        {
        firstmhptr = headline;
        mhptr      = firstmhptr + HEAD_SIZE + 1; /* check random_menu too */

        trace_0(TRACE_APP_PD4, "schkvl searching menu definitions");

        /* NB --mhptr kosher */
        while(--mhptr >= firstmhptr)
            if(mhptr->installed)
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
    const MENU_HEAD * mhptr)
{
    MENU *firstmptr, *mptr;
    const char *aptr, *bptr;
    char a, b;

    if(0 != mhptr->items)
        {
        firstmptr = mhptr->tail;
        mptr = firstmptr + mhptr->items;

        while(--mptr >= firstmptr)
            {
            aptr = alt_array;
            bptr = mptr->command;

            if(bptr)
                do  {
                    a = *aptr++;
                    b = *bptr++;

                    if(!a  ||  (a == SPACE))
                        {
                        if(!b)
                            {
                            trace_0(TRACE_APP_PD4, "found whole command");
                            *alt_array = '\0';
                            return(mptr);
                            }

                        trace_0(TRACE_APP_PD4, "found partial command");
                        return((MENU *) M_IS_SUBSET);
                        }
                    }
                while(a == toupper(b));
            }
        }

    return(0);
}

static MENU *
part_command_core(void)
{
    const MENU_HEAD *firstmhptr, *mhptr;
    MENU * res;

    trace_1(TRACE_APP_PD4, "look up alt_array %s in menu structures", alt_array);

    firstmhptr = headline;
    mhptr      = firstmhptr + HEAD_SIZE + 1; /* check random_menu too */

    /* NB --mhptr kosher */
    while(--mhptr >= firstmhptr)
        if(mhptr->installed)
            if((res = part_command_core_menu(mhptr)) != 0)
                return(res);

    if((res = part_command_core_menu(&iconbar_headline[0])) != 0)
        return(res);

    trace_0(TRACE_APP_PD4, "invalid sequence - discarding");
    *alt_array = '\0';
    bleep();

    return(M_IS_ERROR);
}

static MENU *
part_command(
    BOOL allow_redef)
{
    LIST *lptr;
    const char *aptr, *bptr;
    char a, b;

    if(allow_redef)
        {
        trace_1(TRACE_APP_PD4, "look up alt_array %s on redefinition list", alt_array);

        for(lptr = first_in_list(&first_command_redef);
            lptr;
            lptr = next_in_list(&first_command_redef))
            {
            aptr = alt_array;
            bptr = lptr->value;

            trace_2(TRACE_APP_PD4, "comparing alt_array %s with redef %s", aptr, bptr);

            if(bptr)
                do  {
                    a = *aptr++;    /* alt_array always UC */
                    b = *bptr++;    /* def'n & redef'n always UC */

                    if(!a  ||  (a == SPACE))
                        {
                        if(b == '_')
                            {
                            trace_0(TRACE_APP_PD4, "found full redefined command - look it up");
                            strcpy(alt_array, bptr);
                            return(part_command_core());
                            }

                        trace_0(TRACE_APP_PD4, "found partial redefined command");
                        return(M_IS_SUBSET);
                        }
                    }
                while(a == b);
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
    MENU_HEAD * firstmhptr, * mhptr;
    MENU *      firstmptr,  * mptr;

    trace_1(TRACE_APP_PD4, "find_command(%d)", funcnum);

    firstmhptr = headline;
    mhptr      = firstmhptr + HEAD_SIZE + 1; /* check random_menu too */

    if(funcnum)
        {
        /* NB --mhptr kosher */
        while(--mhptr >= firstmhptr)
            if(mhptr->installed && (0 != mhptr->items))
                {
                firstmptr = mhptr->tail;
                mptr      = firstmptr + mhptr->items;

                while(--mptr >= firstmptr)
                    {
                    trace_1(FALSE, "mptr->funcnum = %d", mptr->funcnum);

                    if(mptr->funcnum == funcnum)
                        {
                        *mptrp  = mptr;
                        return(TRUE);
                        }
                    }
                }

        /* check 'choices' last as that's rarer use */
        mhptr = &iconbar_headline[0];

        if(0 != mhptr->items)
            {
            firstmptr = mhptr->tail;
            mptr = firstmptr + mhptr->items;

            while(--mptr >= firstmptr)
                {
                trace_1(FALSE, "mptr->funcnum = %d", mptr->funcnum);

                if(mptr->funcnum == funcnum)
                    {
                    *mptrp = mptr;
                    return(TRUE);
                    }
                }
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

/*
*
* process a command for internal use eg. reusing a command body for part of another function
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
    BOOL to_comm_file = (spool && ((mptr->flags & NO_MACROFILE) == 0));

    if(toggle_short_menu_entry)
        {
        toggle_short_menu_entry = FALSE;

        if((mptr->flags & ALWAYS_SHORT) == 0)
            {
            /* toggle the bit */
            mptr->flags ^= CHANGED;

            /* get new menus */
            riscmenu_clearmenutree();
            resize_menus(short_menus());
            riscmenu_buildmenutree(short_menus());
            }

        return(TRUE);
        }

    trace_1(TRACE_APP_PD4, "do_command(%d)", funcnum);

    if(is_current_document())
        {
        /* fault commands that shouldn't be done in editing expression context */
        if(mptr->flags & GREY_EXPEDIT)
            if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                {
                reperr_null(create_error(ERR_EDITINGEXP));
                return(TRUE);
                }

        /* many commands require buffers merged - but watch out for those that don't! */
        if(mptr->flags & DO_MERGEBUF)
            if(!mergebuf())
                return(TRUE);
        }
    else
        {
        /* some commands can be done without a current document */

        if(mptr->flags & DOC_REQ)
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
PD4 has funny formula line/window
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

    /* SKS after 4.12 01apr92 - formula wasn't reloaded correctly if it contained quotes */
    cvt_string_arg_for_macro_file(array, elemof32(array), str);

    if(output_string_to_macro_file(array))
        out_comm_end_to_macro_file(funcnum);
}

/******************************************************************************
*
* alt_letter takes a 16-bit character (eg from rdch()) and if it is
* an Alt-letter returns the upper case letter. Otherwise returns 0
*
******************************************************************************/

static uchar
alt_letter(
    S32 c)
{
    if((c <= 0)  ||  ((c & ALT_ADDED) == 0))
        return('\0');
    else
        return((uchar) (c & 0xFF));
}

/******************************************************************************
*
* get a menu item printed into core
* returns TRUE if submenu possible
*
******************************************************************************/

extern BOOL
get_menu_item(
    MENU_HEAD *header,
    MENU *mptr,
    char *array /*filled*/)
{
    char *out = array;
    const char *ptr;
    char ch;

    /* copy item title to array */
    strcpy(out, *mptr->title);
    out += strlen(out);

    /* pad title field with spaces */
    trace_2(TRACE_APP_PD4, "out - array = %d, header->titlelen = %d",
                out - array, header->titlelen);
    while((out - array) <= header->titlelen)
        *out++ = ' ';

    /* copy item command sequence to array */
    *out++='^';
    ptr = mptr->command;
    while((ch = *ptr++) != '\0')
        *out++ = toupper(ch);

    /* now need to force ^ output and skip Fn key 'cos we can't align it */
    *out = '\0';

    return((BOOL) mptr->flags & HAS_DIALOG);
}

/******************************************************************************
*
* --in--
*    +ve        char, maybe Alt
*    0        to be ignored
*    -ve        command number
*
* --out--
*    +ve        char
*    0        to be ignored, or error happened
*
******************************************************************************/

static S32
fiddle_alt_riscos(
    S32 ch)
{
    MENU *mptr;
    uchar *str    = alt_array + strlen((char *) alt_array);
    uchar kh = alt_letter(ch);

    trace_2(TRACE_APP_PD4, "fiddle_alt_riscos alt_letter: %d, %c",
            kh, kh);

    if(!kh)
        {
        if(str != alt_array)
            {
            bleep();
            *alt_array = '\0';
            return('\0');
            }
        else
            return(ch);
        }

    *str++    = kh;
    *str    = '\0';
    trace_1(TRACE_APP_PD4, "alt_array := \"%s\"", alt_array);

    mptr = part_command(TRUE);
    if(mptr == M_IS_SUBSET || mptr == M_IS_ERROR)
        return(0);

    return(0 - mptr->funcnum);
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
    myassert0x(record_option.flags & TICKABLE, "record state change failed");
    opt = record_option.flags;
    record_option.flags = macro_recorder_on
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
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
    myassert0x(check_option.flags & TICKABLE, "check state change failed");
    opt = check_option.flags;
    check_option.flags = (d_auto[0].option == 'Y')
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}

/******************************************************************************
*
* switch insert/overtype tick on or off
*
******************************************************************************/

extern void
insert_state_may_have_changed(void)
{
    optiontype opt;
    myassert0x(insert_option.flags & TICKABLE, "insert state change failed");
    opt = insert_option.flags;
    insert_option.flags = (d_progvars[OR_IO].option)
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}

/******************************************************************************
*
* switch short menu tick on or off
*
******************************************************************************/

extern void
menu_state_changed(void)
{
    BOOL short_m;
    optiontype opt;
    myassert0x(menu_option.flags & TICKABLE, "menu state change failed");
    opt = menu_option.flags;
    menu_option.flags = d_menu[0].option
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;

    short_m = short_menus();
    resize_menus(short_m);
    riscmenu_buildmenutree(short_m);
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
    myassert0x(recalc_option.flags & TICKABLE, "recalc state change failed");
    opt = recalc_option.flags;
    recalc_option.flags = (d_progvars[OR_AM].option == 'A')
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
    if(d_progvars[OR_AM].option == 'N')
        colh_draw_slot_count_in_document(NULL);
}

extern void
chart_recalc_state_may_have_changed(void)
{
    optiontype opt;
    myassert0x(chart_recalc_option.flags & TICKABLE, "chart recalc state change failed");
    opt = chart_recalc_option.flags;
    chart_recalc_option.flags = (d_progvars[OR_AC].option == 'A')
                                ? opt |  TICK_STATUS
                                : opt & ~TICK_STATUS;
}

extern void
MenuSize_fn(void)
{
    d_menu[0].option = (optiontype) !d_menu[0].option;

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
        reperr(create_error(ERR_CANNOTWRITE), macro_file_name);
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
    *ptr = '\0';

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
        safe_strkpy(array, elemof32(array), "\\");
        safe_strkat(array, elemof32(array), macro_command_name);
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
        safe_strkpy(array, elemof32(array), "|m        ");
        safe_strkpy(array + (10 - sofar), elemof32(array) - (10 - sofar), *mptr->title);
        safe_strkat(array, elemof32(array), CR_STR);

        macro_needs_lf = FALSE;

        (void) output_string_to_macro_file(array);
        }
}

/* SKS after 4.12 01apr92 - needed for \ESC */

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

    safe_strkpy(ptr, elemof_buffer, "|i \"");
    offset = strlen(ptr);

    while((ch = *arg++) != '\0')
        {
        if(ch == QUOTES)
            if(offset < buffer_limit)
                ptr[offset++] = '|';

        if(offset < buffer_limit)
            ptr[offset++] = ch;
        }

    safe_strkpy(&ptr[offset], elemof_buffer - offset, "\" ");
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

            case F_LIST:
            case F_NUMBER:
            case F_COLOUR:
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

        if(charout || numout)
            ptr += sprintf(ptr, charout ? "|i %c " : "|i %d ", (int) dptri->option);

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

extern void
RecordMacroFile_fn(void)
{
    char buffer[BUF_MAX_PATHSTRING];
    char *name;

    macro_needs_lf = FALSE;

    if(macro_recorder_on)
        stop_macro_recorder();
    else
        {
        if(!dialog_box_start())
            return;

        while(dialog_box(D_MACRO_FILE))
            {
            name = d_macro_file[0].textfield;

            if(str_isblank(name))
                {
                reperr_null(create_error(ERR_BAD_NAME));
                if(!dialog_box_can_retry())
                    break;
                been_error = FALSE;
                (void) init_dialog_box(D_MACRO_FILE);
                continue;
                }

            /* Record to subdirectory of <Choices$Write>.PipeDream */
            (void) add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), name, MACROS_SUBDIR_STR);

            if(!mystr_set(&macro_file_name, buffer))
                break;

            macro_recorder_file = pd_file_open(macro_file_name, file_open_write);

            if(!macro_recorder_file)
                {
                reperr(create_error(ERR_CANNOTOPEN), macro_file_name);
                if(!dialog_box_can_retry())
                    break;
                continue;
                }

            file_set_type(macro_recorder_file, FILETYPE_PDMACRO);

            macro_recorder_on = TRUE;

            /* if(!dialog_box_can_persist()) - no, job done */
            break;
            }

        dialog_box_end();
        }

    record_state_changed();
}

static void
stop_macro_recorder(void)
{
    if(pd_file_close(&macro_recorder_file))
        {
        str_clr(&d_execfile[0].textfield);
        reperr(create_error(ERR_CANNOTCLOSE), macro_file_name);
        }
    else
        (void) mystr_set(&d_execfile[0].textfield, d_macro_file[0].textfield);

    str_clr(&macro_file_name);

    macro_recorder_on = FALSE;
}

/* end of commlin.c */
